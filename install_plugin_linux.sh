#!/bin/bash
set -e

echo "Installing Cloudfuse plugin"

apt-get install libssl-dev
apt-get install ./cloudfuse*.deb
systemctl stop networkoptix-mediaserver.service
cp cloudfuse_plugin.so /opt/networkoptix/mediaserver/bin/plugins/
systemctl restart networkoptix-mediaserver.service

echo "Finished installing Cloudfuse plugin"

