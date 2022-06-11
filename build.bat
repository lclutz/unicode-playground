@echo off
setlocal
cd /D "%~dp0"

where /q cl || (
    echo ERROR: "cl" not found
    echo        Please run this from the MSVC x64 native tools command prompt.
    exit /B 1
)

set CFLAGS=/utf-8 /FC /Zi /MT /nologo /W3
set LDFLAGS=/opt:ref /incremental:no /subsystem:console
set LIBS=shell32.lib shell32.lib


if not exist build\ mkdir build
pushd build

cl.exe %CFLAGS% /Fe:"echo.exe" "..\echo.c" /link %LDFLAGS% %LIBS%

popd
endlocal
