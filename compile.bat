@echo off
setlocal EnableExtensions DisableDelayedExpansion
cd /d "%~dp0"
if errorlevel 1 goto fail
rem Build using the x64 Microsoft Visual C++ toolchain and installed Windows SDK.
rem Works from a normal cmd.exe or the x64 Native Tools Command Prompt.
if /I "%VSCMD_ARG_TGT_ARCH%"=="x64" goto tools
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" goto missing
set "VSROOT="
for /f "usebackq delims=" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSROOT=%%I"
if not defined VSROOT goto missing
if not exist "%VSROOT%\VC\Auxiliary\Build\vcvars64.bat" goto missing
call "%VSROOT%\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 goto fail
:tools
where cl.exe >nul 2>&1 || goto missing
where link.exe >nul 2>&1 || goto missing
where rc.exe >nul 2>&1 || goto missing
if not exist build mkdir build
if errorlevel 1 goto fail
rem Ignore externally injected compiler/linker flags; keep this build reproducible.
set "CL="
set "_CL_="
set "LINK="
set "_LINK_="
echo Building miniGestures 2.0.9 (native C++ x64)...
cl.exe /nologo /c /std:c++17 /utf-8 /O2 /MT /EHsc /GS /guard:cf /W3 /DUNICODE /D_UNICODE /D_WIN32_WINNT=0x0A00 /Fo"build\core.obj" "src\main.cpp"
if errorlevel 1 goto fail
cl.exe /nologo /c /std:c++17 /utf-8 /O2 /MT /EHsc /GS /guard:cf /W3 /DUNICODE /D_UNICODE /D_WIN32_WINNT=0x0A00 /Fo"build\native_host.obj" "src\native_host.cpp"
if errorlevel 1 goto fail
rc.exe /nologo /fo "build\miniGestures.res" "assets\app.rc"
if errorlevel 1 goto fail
rem The resource already embeds the original PerMonitorV2/asInvoker manifest.
link.exe /nologo /MACHINE:X64 /SUBSYSTEM:WINDOWS /OUT:"build\miniGestures.exe" /STACK:8388608 /INCREMENTAL:NO /OPT:REF /OPT:ICF /DYNAMICBASE /HIGHENTROPYVA /NXCOMPAT /GUARD:CF /MANIFEST:NO "build\core.obj" "build\native_host.obj" "build\miniGestures.res" kernel32.lib user32.lib gdi32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib taskschd.lib uuid.lib
if errorlevel 1 goto fail
copy /Y "build\miniGestures.exe" "miniGestures.exe" >nul
if errorlevel 1 goto fail
echo.
echo BUILD SUCCEEDED: "%CD%\miniGestures.exe"
echo No Go, Python, CMake or separately installed VC redistributable is required.
if /I not "%~1"=="--no-pause" pause
exit /b 0
:missing
echo ERROR: Microsoft C++ x64 tools or Windows SDK not found.
echo Install Visual Studio Build Tools with Desktop development with C++ and Windows SDK.
echo https://visualstudio.microsoft.com/downloads/
:fail
echo BUILD FAILED. See errors above. Existing miniGestures.exe was not intentionally replaced before a successful link.
if /I not "%~1"=="--no-pause" pause
exit /b 1
