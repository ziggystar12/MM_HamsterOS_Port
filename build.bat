@echo off
setlocal
cd /d "%~dp0"

set "DOCKER_BIN=C:\Program Files\Docker\Docker\resources\bin"
if exist "%DOCKER_BIN%\docker.exe" set "PATH=%DOCKER_BIN%;%PATH%"

where docker >nul 2>nul
if errorlevel 1 (
    echo Docker not found. Start Docker Desktop first.
    exit /b 1
)

echo Building MM.APP...
docker compose -f ..\HamsterOS\compose.yaml run --rm builder make -C /work/../MM_HamsterOS_Port MM.APP
if errorlevel 0 (
    echo.
    echo Output: dist\MM.APP
    echo Copy to HamsterOS: copy dist\MM.APP ..\HamsterOS\build\Compiled_Apps\
)
exit /b %errorlevel%
