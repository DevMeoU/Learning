@echo off
REM Windows build helper script for lwIP component
REM This script helps handle file locking issues during the build process

setlocal enabledelayedexpansion

REM Check if we're being asked to delete a file
if "%1"=="delete" (
    if exist "%2" (
        REM Try to delete the file, with a small delay to allow any locks to be released
        ping -n 1 127.0.0.1 > nul
        del /F /Q "%2" 2>nul
        if exist "%2" (
            REM If file still exists, try again with a longer delay
            ping -n 2 127.0.0.1 > nul
            del /F /Q "%2" 2>nul
        )
    )
    exit /b 0
)

REM Check if we're being asked to compile a file
if "%1"=="compile" (
    REM Extract the compiler and arguments
    set compiler=%2
    set src_file=%3
    set obj_file=%4
    set dep_file=%5
    shift
    shift
    shift
    shift
    shift
    
    REM Compile the file
    %compiler% %* -c %src_file% -o %obj_file%
    
    REM If a dependency file was generated and we need to move it
    if exist "%dep_file%.tmp" (
        move /Y "%dep_file%.tmp" "%dep_file%" >nul 2>&1
    )
    
    exit /b 0
)

REM If no recognized command, just echo the arguments
echo Unknown command: %*
exit /b 1