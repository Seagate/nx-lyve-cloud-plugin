#!/bin/bash

## Copyright © 2024 Seagate Technology LLC and/or its Affiliates
set -e

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
systemctl stop networkoptix-mediaserver.service
cp cloudfuse_plugin.so /opt/networkoptix/mediaserver/bin/plugins/
systemctl start networkoptix-mediaserver.service

echo "Finished installing Cloudfuse plugin"
