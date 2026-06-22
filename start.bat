@echo off
REM ---------------------------------------------------------------------------
REM Launch a dedicated CubeServer and one CubeWorld client that connects to it.
REM
REM Usage:   start.bat [Config] [Host]
REM   Config   build config to run: Debug (default) or Release
REM   Host     server address the client connects to (default 127.0.0.1)
REM
REM The server window writes its save into build\bin\<Config>\world. Close the
REM server window (or press a key in it / Ctrl+C) to stop the server.
REM ---------------------------------------------------------------------------
setlocal

set "CONFIG=%~1"
if "%CONFIG%"=="" set "CONFIG=Debug"

set "HOST=%~2"
if "%HOST%"=="" set "HOST=127.0.0.1"

set "EXE_DIR=%~dp0build\bin\%CONFIG%"
set "SERVER=%EXE_DIR%\CubeServer.exe"
set "CLIENT=%EXE_DIR%\CubeWorld.exe"

if not exist "%SERVER%" (
    echo [start] Cannot find "%SERVER%".
    echo [start] Build first, e.g.:  cmake --build build --config %CONFIG%
    exit /b 1
)
if not exist "%CLIENT%" (
    echo [start] Cannot find "%CLIENT%".
    echo [start] Build first, e.g.:  cmake --build build --config %CONFIG%
    exit /b 1
)

echo [start] Launching server (%CONFIG%) on port 25565...
start "CubeServer" /D "%EXE_DIR%" "%SERVER%"

REM Give the server a moment to bind the socket before the client dials in.
REM (ping is used instead of timeout so the delay works even when stdin is piped.)
ping -n 3 127.0.0.1 >nul

echo [start] Launching client -> %HOST%:25565 ...
start "CubeWorld Client" /D "%EXE_DIR%" "%CLIENT%" %HOST%

echo [start] Done. Two windows opened (server + client).
endlocal
