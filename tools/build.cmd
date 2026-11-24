@echo off
rem Builds the core and the app into build\release, then puts the Qt runtime beside MKArcade.exe.
rem   tools\build.cmd            (Release)
rem   tools\build.cmd Debug
setlocal
set "ROOT=%~dp0.."
set "CFG=%~1"
if "%CFG%"=="" set "CFG=Release"
set "OUT=%ROOT%\build\%CFG%"
call "%~dp0vsenv.cmd" cmake -S "%ROOT%" -B "%OUT%" -G Ninja -DCMAKE_BUILD_TYPE=%CFG% || exit /b 1
call "%~dp0vsenv.cmd" cmake --build "%OUT%" --parallel || exit /b 1
for /f "usebackq delims=" %%q in (`findstr /b /c:"CMAKE_PREFIX_PATH:" "%OUT%\CMakeCache.txt"`) do set "QTLINE=%%q"
set "QTDIR=%QTLINE:*=%"
if not exist "%OUT%\app\Qt6Quick.dll" if not exist "%OUT%\app\Qt6Quickd.dll" (
    "%QTDIR%\bin\windeployqt.exe" --no-translations --no-system-d3d-compiler --no-opengl-sw ^
        --qmldir "%ROOT%\app\qml" "%OUT%\app\MKArcade.exe" || exit /b 1
)
echo Built %OUT%\app\MKArcade.exe
