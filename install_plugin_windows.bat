:: Copyright © 2024 Seagate Technology LLC and/or its Affiliates
@echo off

for %%I in (cloudfuse*.exe) do (
    set "installer=%%I"
    break
)

REM Check if the installer file exists
if exist "%installer%" (
    echo "Installing Cloudfuse"
    "%installer%" /VERYSILENT /SUPPRESSMSGBOXES /NORESTART
) else (
    echo "Installer file not found"
)

echo "Installing plugin"

copy ".\cloudfuse_plugin.dll" "C:\Program Files\Network Optix\Nx Meta\MediaServer\plugins\"

echo "Finished installing Cloudfuse plugin"

pause
