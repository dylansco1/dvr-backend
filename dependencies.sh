#!/bin/bash
apt install ffmpeg jq
echo 'Select Prowlarr'
wget https://raw.githubusercontent.com/Cyneric/servarr-install-script/main/servarr-install-script.sh
chmod +x servarr-install-script.sh
./servarr-install-script.sh