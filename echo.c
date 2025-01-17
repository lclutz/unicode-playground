#include <stdbool.h>
#include <stdint.h>

#include <windows.h>

#define ARENA_SIZE 128
#define UTF8BOM "\xef\xbb\xbf"

#define Print(Handle, ...) Win32Print(Handle, __VA_ARGS__, NULL)

static HANDLE GlobalOutputHandle = INVALID_HANDLE_VALUE;
static UINT GlobalPreviousConsoleCP;
static UINT GlobalPreviousConsoleOutputCP;

typedef struct
{
    size_t Capacity;
    size_t Offset;
    char *Buffer;
} arena;

static char *
ArenaAlloc(arena *Arena, size_t NumBytes)
{
    char *Result = 0;
    if (Arena->Offset + NumBytes <= Arena->Capacity)
    {
        Result = Arena->Buffer + Arena->Offset;
        Arena->Offset += NumBytes;
    }
    return Result;
}

static void
ArenaFree(arena *Arena)
{
    Arena->Offset = 0;
}

static void
Win32Die(int ReturnCode)
{
    if (GlobalOutputHandle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(GlobalOutputHandle);
    }

    // TODO: Should I reset the code page to the previous state?
    if (GlobalPreviousConsoleCP) { SetConsoleCP(GlobalPreviousConsoleCP); }
    if (GlobalPreviousConsoleOutputCP) { SetConsoleOutputCP(GlobalPreviousConsoleOutputCP); }

    ExitProcess(ReturnCode);
}

static void
Win32Print(HANDLE OutputHandle, ...)
{
    if (OutputHandle != INVALID_HANDLE_VALUE)
    {
        va_list ArgumentPointer;
        va_start(ArgumentPointer, OutputHandle);

        for (char *String = va_arg(ArgumentPointer, char *);
             String;
             String = va_arg(ArgumentPointer, char *))
        {
            DWORD BytesToWrite = (DWORD)strlen(String);
            DWORD BytesWritten = 0;
            DWORD BytesWrittenThisWriteCall = 0;
            while (BytesToWrite - BytesWritten)
            {
                bool WriteSuccess = WriteFile(
                    OutputHandle,
                    (PVOID)String,
                    BytesToWrite - BytesWritten,
                    &BytesWrittenThisWriteCall,
                    0);

                if (!WriteSuccess) { break; }

                BytesWritten += BytesWrittenThisWriteCall;
            }
        }

        va_end(ArgumentPointer);
    }
}

static char *
Win32ConvertUTF16ToUTF8(arena *Arena, LPWSTR UTF16String)
{
    char *Result = NULL;
    if (UTF16String)
    {
        int SizeRequiredIncludingNullTerminator =
            WideCharToMultiByte(CP_UTF8, 0, UTF16String, -1, 0, 0, 0, 0);

        char *OutputBuffer = ArenaAlloc(Arena, SizeRequiredIncludingNullTerminator);
        if (OutputBuffer)
        {
            int ConversionResult =
                WideCharToMultiByte(CP_UTF8, 0, UTF16String, -1,
                                    OutputBuffer, SizeRequiredIncludingNullTerminator,
                                    0, 0);

            if (ConversionResult > 0)
            {
                Result = OutputBuffer;
            }
        }
        else
        {
            Print(GlobalOutputHandle, "[ERROR]: Out of memory.\n");
            Win32Die(1);
        }

    }
    return Result;
}

static bool
Win32CheckIfConsoleOutput(HANDLE OutputHandle)
{
    bool Result = false;
    if (OutputHandle != INVALID_HANDLE_VALUE)
    {
        DWORD FileType = GetFileType(OutputHandle);
        if ((FileType != FILE_TYPE_UNKNOWN) || (GetLastError() == ERROR_SUCCESS))
        {
            FileType &= ~(FILE_TYPE_REMOTE);
            if (FileType == FILE_TYPE_CHAR)
            {
                DWORD ConsoleMode;
                bool GetConsoleModeResult = GetConsoleMode(OutputHandle, &ConsoleMode);
                if (GetConsoleModeResult || (GetLastError() != ERROR_INVALID_HANDLE))
                {
                    Result = true;
                }
            }
        }
    }
    return Result;
}

int
main(void)
{
    GlobalOutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    GlobalPreviousConsoleCP = GetConsoleCP();
    GlobalPreviousConsoleOutputCP = GetConsoleOutputCP();
    if (!(SetConsoleOutputCP(CP_UTF8) && SetConsoleCP(CP_UTF8)))
    {
        Print(GlobalOutputHandle, "[ERROR]: Failed to set UTF-8 code page.\n");
        Win32Die(1);
    }

    arena Arena = {
        .Capacity = ARENA_SIZE,
        .Offset = 0,
        .Buffer = malloc(ARENA_SIZE)
    };

    if (!Arena.Buffer)
    {
        Print(GlobalOutputHandle, "[ERROR]: Failed to allocate arena space.\n");
        Win32Die(1);
    }

    // Get command line parameters in wide character format and convert them
    // all to UTF-8
    int argc;
    LPWSTR *argvUTF16LE = CommandLineToArgvW(GetCommandLineW(), &argc);
    char **argv = (char **)ArenaAlloc(&Arena, argc*sizeof(char *));
    if (!argv)
    {
        Print(GlobalOutputHandle, "[ERROR]: Out of memory.\n");
        Win32Die(1);
    }

    for (int i = 0; i < argc; ++i)
    {
        argv[i] = Win32ConvertUTF16ToUTF8(&Arena, argvUTF16LE[i]);
    }

    bool OutputtingToAConsole = Win32CheckIfConsoleOutput(GlobalOutputHandle);
    if (!OutputtingToAConsole)
    {
        // It's stupid we have to output a BOM at all but what can you do.
        // This makes the pipes in PowerShell 5.1 happy.
        Print(GlobalOutputHandle, UTF8BOM);
    }

    for (int i = 1; i < argc; ++i)
    {
        Print(GlobalOutputHandle, argv[i], "\n");
    }

    Win32Die(0);
}
