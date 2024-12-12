:: Copyright © 2024 Seagate Technology LLC and/or its Affiliates
@echo off

:: Check / get privileges
net file 1>NUL 2>NUL
if not '%errorlevel%' == '0' (
    powershell Start-Process -FilePath "%0" -verb runas >NUL 2>&1
    exit /b
) else (
    :: If we just got privileges, drill back into the directory where the script is
    cd /d %~dp0
)

:: Find the installer file
for %%I in (cloudfuse*.exe) do (
    set "installer=%%I"
    break
)

:: Check if the installer file exists and if so install it
if exist "%installer%" (
    echo Installing DW Cumulus Cloud
    "%installer%" /VERYSILENT /SUPPRESSMSGBOXES /NORESTART
) else (
    echo Installer file not found
    pause
    exit /b
)

echo Installing plugin

:: Stop the VMS server
set serviceName="digitalwatchdogMediaServer"
echo Attempting to stop %serviceName%
net stop %serviceName% >NUL 2>&1
echo Service stopped successfully.

:: Copy the plugin file
echo Attempting to copy the plugin file
copy ".\cloudfuse_plugin.dll" "C:\Program Files\Digital Watchdog\DW Spectrum\MediaServer\plugins\"
set copyError=%errorlevel%
if %copyError% neq 0 (
    echo Failed to copy the plugin file
)

:: Restart the VMS
echo Attempting to start %serviceName%
net start %serviceName% >NUL 2>&1
if %errorlevel% neq 0 (
    echo Installation failed: Unable to restart %serviceName%. It must be restarted manually.
    pause
    exit /b
)

if %copyError% neq 0 (
    echo Installation failed: Unable to copy plugin file
    pause
    exit /b
)

echo Service started successfully.
echo Finished installing DW Cumulus Cloud plugin

pause
