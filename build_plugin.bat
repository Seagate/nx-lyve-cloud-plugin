:: Copyright © 2024 Seagate Technology LLC and/or its Affiliates
:: Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

@echo off
setlocal %= Reset errorlevel and prohibit changing env vars of the parent shell. =%

if [%1] == [/?] goto :show_usage
if [%1] == [-h] goto :show_usage
if [%1] == [--help] goto :show_usage
goto :skip_show_usage
:show_usage
    echo Usage: %~n0%~x0 [--no-tests] [--debug] [^<cmake-generation-args^>...]
    echo  --debug Compile using Debug configuration (without optimizations) instead of Release.
    goto :exit
:skip_show_usage

:: Make the build dir at the same level as the parent dir of this script, suffixed with "-build".
set BASE_DIR_WITH_BACKSLASH=%~dp0
set BASE_DIR=%BASE_DIR_WITH_BACKSLASH:~0,-1%
set BUILD_DIR=%BASE_DIR%-build

if [%1] == [--no-tests] (
    shift
    set NO_TESTS=1
) else (
    set NO_TESTS=0
)

if [%1] == [--debug] (
    shift
    set BUILD_TYPE=Debug
) else (
    set BUILD_TYPE=Release
)

if [%BUILD_TYPE%] == [Release] (
    set BUILD_OPTIONS=--config %BUILD_TYPE%
)

:: Build source
set SOURCE_DIR=%BASE_DIR%\src
set PLUGIN_NAME=cloudfuse_plugin

set PLUGIN_BUILD_DIR=%BUILD_DIR%\%PLUGIN_NAME%
@echo on
    if not exist "%PLUGIN_BUILD_DIR%\" mkdir "%PLUGIN_BUILD_DIR%" || @exit /b 70
    cd "%PLUGIN_BUILD_DIR%" || @exit /b 70

    cmake "%SOURCE_DIR%" -G "Visual Studio 17 2022" -Ax64 %1 %2 %3 %4 %5 %6 %7 %8 %9 --preset default || @exit /b 70
    cmake --build . %BUILD_OPTIONS% || @exit /b 70
@echo off

set ARTIFACT=%PLUGIN_BUILD_DIR%\%BUILD_TYPE%\%PLUGIN_NAME%.dll

if not exist "%ARTIFACT%" (
    echo ERROR: Failed to build %PLUGIN_NAME%.
    exit /b 70
)
echo:
echo Built: %ARTIFACT%

::Build Tests
set SOURCE_DIR=%BASE_DIR%\src\unit_tests
set UNIT_TESTS="unit_tests"

set UNIT_TESTS_BUILD_DIR=%BUILD_DIR%\%UNIT_TESTS%
@echo on
    if not exist "%UNIT_TESTS_BUILD_DIR%" mkdir "%UNIT_TESTS_BUILD_DIR%" || @exit /b 70
    cd "%UNIT_TESTS_BUILD_DIR%" || @exit /b 70

    cmake "%SOURCE_DIR%" -G "Visual Studio 17 2022" -Ax64 %1 %2 %3 %4 %5 %6 %7 %8 %9 --preset default || @exit /b 70
    cmake --build . %BUILD_OPTIONS% || @exit /b 70
@echo off

set ARTIFACT=%UNIT_TESTS_BUILD_DIR%\%BUILD_TYPE%\analytics_plugin_ut.exe

if not exist "%ARTIFACT%" (
    echo ERROR: Failed to build %UNIT_TESTS%.
    exit /b 70
)
echo:
echo Built: %ARTIFACT%

:: Run unit tests if needed.
if [%NO_TESTS%] == [1] echo NOTE: Unit tests were not run. & goto :skip_tests
echo on
    cd "%BUILD_DIR%\unit_tests" || @goto :exit

    ctest --output-on-failure -C %BUILD_TYPE% || @goto :exit
@echo off
:skip_tests

echo:
echo Samples built successfully, see the binaries in %BUILD_DIR%
goto :exit

:exit
    exit /b %ERRORLEVEL% %= Needed for a proper cmd.exe exit status. =%
