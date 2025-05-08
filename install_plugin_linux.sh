#!/bin/bash

## Copyright © 2024 Seagate Technology LLC and/or its Affiliates
set -e

# Function to prompt for username and password
prompt_credentials() {
    read -rp "Enter Nx server username: " username
    read -srp "Enter Nx server password: " password
    echo
}

# Function to send a curl request with retries
send_request() {
    local url=$1
    local data=$2
    local headers=$3
    local method=$4
    local response
    local http_code

    response=$(curl -s -k -w "%{http_code}" -X "$method" "$url" -H "Content-Type: application/json" -H "$headers" -d "$data")
    http_code=${response: -3}  # Extract the last 3 characters for the HTTP code
    response=${response%???}  # Remove the last 3 characters (HTTP code) from the response

    if [ "$http_code" -eq 200 ]; then
        echo "$response"
        return 0
    else
        echo "Error: HTTP $http_code"
        echo "Response: $response"
        return 1
    fi
}

# Get the port number from the configuration file
port=$(grep -oP 'port=\K\d+' /opt/networkoptix-metavms/mediaserver/etc/mediaserver.conf)
if [ -z "$port" ]; then
    echo "Error: Unable to find port number in configuration file."
    exit 1
fi

# Prompt for credentials
prompt_credentials

# Send a request to the login endpoint
login_url="https://localhost:$port/rest/v2/login/sessions"
login_data="{\"username\":\"$username\",\"password\":\"$password\",\"setCookie\":true}"
if ! login_response=$(send_request "$login_url" "$login_data" "" "POST"); then
    echo "Error: Login failed. Please check your credentials and try again."
    echo "$login_response"
    exit 1
fi

# Extract the token from the login response
token=$(echo "$login_response" | grep -oP '"token":"\K[^"]+')
if [ -z "$token" ]; then
    echo "Error: Unable to extract token from login response."
    exit 1
fi

# Send a request to the system settings endpoint
settings_url="https://localhost:$port/rest/v2/system/settings"
settings_data='{"additionalLocalFsTypes": "fuse.cloudfuse"}'
settings_headers="Cookie: x-runtime-guid=ahhh"
if ! send_request "$settings_url" "$settings_data" "$settings_headers" "PATCH"; then
    echo "Error: Failed to get a response from the system settings endpoint. Unable to set additional local file system types."
    exit 1
fi

### Setup Fuse Allow Other
FUSE_CONF="/etc/fuse.conf"

# Check if "user_allow_other" is already in the file
if grep -q "^user_allow_other" "$FUSE_CONF"
then
    echo "user_allow_other is already set in $FUSE_CONF"
else
    # Check if "user_allow_other" is in the file but commented out
    if grep -q "^#user_allow_other" "$FUSE_CONF"
    then
      sudo sed -i 's/^#user_allow_other/user_allow_other/' "$FUSE_CONF"
      echo "Uncommented user_allow_other in $FUSE_CONF"
    else
        # If not found, add "user_allow_other" to the file
        echo "user_allow_other" | sudo tee -a "$FUSE_CONF" > /dev/null
        echo "Added user_allow_other added to $FUSE_CONF"
    fi
fi

### Install Cloudfuse
echo "Installing Cloudfuse"
apt-get install libssl-dev
apt-get install ./cloudfuse*.deb

### Install plugin
echo "Installing Cloudfuse plugin"
cp cloudfuse_plugin.so /opt/networkoptix-metavms/mediaserver/bin/plugins/
systemctl restart networkoptix-metavms-mediaserver.service

echo "Finished installing Cloudfuse plugin"
