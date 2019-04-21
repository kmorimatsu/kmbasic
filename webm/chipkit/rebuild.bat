@cmd /c clean.bat
@for %%F in (*.c) do cmd /c compile.bat "%%F"
@for %%F in (interface\*.c) do cmd /c compile.bat "%%F"
@build.bat
