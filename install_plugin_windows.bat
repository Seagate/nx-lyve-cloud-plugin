:: Copyright © 2024 Seagate Technology LLC and/or its Affiliates
@echo off

:: Find the installer file
for %%I in (cloudfuse*.exe) do (
    set "installer=%%I"
    break
)

:: Check if the installer file exists and if so install it
if exist "%installer%" (
    echo Installing Cloudfuse
    "%installer%" /VERYSILENT /SUPPRESSMSGBOXES /NORESTART
) else (
    echo Installer file not found
    exit /b
)

echo Installing plugin

:: Stop the VMS server
set serviceName="metavmsMediaServer"
echo Attempting to stop %serviceName%
net stop %serviceName% >NUL 2>&1
echo Service stopped successfully.

:: Copy the plugin file
echo Attempting to copy the plugin file
copy ".\cloudfuse_plugin.dll" "C:\Program Files\Network Optix\Nx Meta\MediaServer\plugins\"
set copyError=%errorlevel%
if %copyError% neq 0 (
    echo Failed to copy the plugin file
)

:: Restart the VMS
echo Attempting to start %serviceName%
net start %serviceName% >NUL 2>&1
if %errorlevel% neq 0 (
    echo Installation failed: Unable to restart %serviceName%. It must be restarted manually.
    exit /b
)

if %copyError% neq 0 (
    echo Installation failed: Unable to copy plugin file
    exit /b
)


echo Service started successfully.
echo Finished installing Cloudfuse plugin

pause
