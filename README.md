# Unicode Playground

Do the rain dance required to load the VisualStudio environment into your
command line and build the program by running `build.bat`.

Example usage:

```shell
> .\build\echo.exe äaöoüu αβγδ ßßßß 漢字
äaöoüu
αβγδ
ßßßß
漢字
```

# Notes

## PowerShell pipes were a mistake

PowerShell does not support piping of raw byte streams. PowerShell pipes will
try to parse the output of native commands into a .NET string. Which is an
epic disaster because it will corrupt binary data (see
[issue #1908](https://github.com/PowerShell/PowerShell/issues/1908)).

PowerShell v5.1 is able to recognize and correctly parse a UTF8 byte stream
from a native application if the UTF8 BOM is present.

In recent PowerShell versions this no longer works (tested with PowerShell
v7.2.4 on my machine).

`$OutputEncoding` is a variable that contains preferences for how powershell
will encode it's output when piping it to native applications.

`[System.Console]::OutputEncoding` contains preferences for what PowerShell
will expect when reading output from native applications.

This naming scheme is not helpful at all.

Setting `[System.Console]::OutputEncoding` will fix this demo program in newer
PowerShell versions
(`[System.Console]::OutputEncoding = [System.Text.Encodings]::UTF8`).

Why can the new PowerShell versions not detect that the output is UTF8? Even
though I am outputting the completely useless UTF8 BOM? No idea.
