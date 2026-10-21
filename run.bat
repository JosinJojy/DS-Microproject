@echo off
REM MazeLab Windows startup script
REM Usage: run.bat [port]
REM Default port: 8080

setlocal

set PORT=8080
if not "%1"=="" set PORT=%1

echo ============================================================
echo   MazeLab - Interactive Maze Solver
echo ============================================================

REM Check for GCC
where gcc >nul 2>&1
if errorlevel 1 (
    echo ERROR: gcc not found in PATH.
    echo Please install MinGW-w64 and add it to your PATH.
    pause
    exit /b 1
)

REM Check for make
where mingw32-make >nul 2>&1
if errorlevel 1 (
    where make >nul 2>&1
    if errorlevel 1 (
        echo ERROR: mingw32-make or make not found in PATH.
        echo Please install MinGW-w64 and add it to your PATH.
        pause
        exit /b 1
    )
    set MAKE=make
) else (
    set MAKE=mingw32-make
)

REM Build if needed
if not exist mazelab.exe (
    echo Building MazeLab...
    %MAKE% all
    if errorlevel 1 (
        echo ERROR: Build failed. See output above.
        pause
        exit /b 1
    )
    echo Build complete.
) else (
    echo Using existing mazelab.exe
)

echo.
echo   Server will start on: http://127.0.0.1:%PORT%
echo   Open this URL in your browser.
echo   Press Ctrl+C to stop the server.
echo ============================================================
echo.

REM Start server
mazelab.exe %PORT%
