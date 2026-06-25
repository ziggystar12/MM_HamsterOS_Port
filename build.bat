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
docker compose -f ..\..\HamsterOS\compose.yaml run --rm ^
    -v "%CD%:/parent/MM/MM_HamsterOS_Port" ^
    -v "%CD%\..\Original_Source:/parent/MM/Original_Source:ro" ^
    -w /parent/MM/MM_HamsterOS_Port ^
    builder make all HAMSTEROS_DIR=/work MMDATA_DIR=/parent/MM/Original_Source
if not errorlevel 1 (
    echo.
    echo Output: dist\MM.APP
    if exist "..\..\HamsterOS\build\mm1.img" (
        docker compose -f ..\..\HamsterOS\compose.yaml run --rm ^
            -v "%CD%\dist:/parent/MM/MM_HamsterOS_Port/dist:ro" ^
            builder sh -lc "mdel -i build/mm1.img ::/MM1/MM.APP 2>/dev/null || true; mcopy -i build/mm1.img /parent/MM/MM_HamsterOS_Port/dist/MM.APP ::/MM1/MM.APP"
        if errorlevel 1 exit /b 1
        echo Updated ..\..\HamsterOS\build\mm1.img ::/MM1/MM.APP
    )
)
exit /b %errorlevel%
