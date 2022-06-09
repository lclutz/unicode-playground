#pragma comment( lib, "kernel32.lib" )
#pragma comment( lib, "shell32.lib" )

#include <stdbool.h>
#include <stdint.h>
#include <windows.h>

#define ARENA_SIZE 1024
#define UTF8BOM "\xef\xbb\xbf"

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

static char *
Win32ConvertUTF16ToUTF8(arena *Arena, LPWSTR UTF16String)
{
    char *Result = NULL;
    if (UTF16String)
    {
        size_t SizeRequiredIncludingNullTerminator =
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

static void
Win32Print(HANDLE OutputHandle, char *String)
{
    if (String)
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
}

int
main(void)
{
    HANDLE StdOut = GetStdHandle(STD_OUTPUT_HANDLE);

    if (!SetConsoleOutputCP(CP_UTF8))
    {
        Win32Print(StdOut, "[ERROR]: Failed to set UTF-8 code page\n");
        return 1;
    }

    arena Arena = {
        .Capacity = ARENA_SIZE,
        .Offset = 0,
        .Buffer = malloc(ARENA_SIZE)
    };

    if (!Arena.Buffer)
    {
        Win32Print(StdOut, "[ERROR]: Failed to allocate enough memory\n");
        return 1;
    }

    int argc;
    LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    if (!Win32CheckIfConsoleOutput(StdOut))
    {
        // NOTE: Only write out a BOM if we are not outputting to a console
        // window. This avoids artifacts of non printable characters in
        // console windows and enables correct behaviour when redirecting
        // stdout to a file in Windows PowerShell.
        Win32Print(StdOut, UTF8BOM);
    }

    for (int i = 1; i < argc; ++i)
    {
        char *UTF8String = Win32ConvertUTF16ToUTF8(&Arena, argv[i]);
        Win32Print(StdOut, UTF8String);
        Win32Print(StdOut, "\n");
        ArenaFree(&Arena);
    }

    return 0;
}
