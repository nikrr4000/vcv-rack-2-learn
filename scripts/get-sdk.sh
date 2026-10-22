#!/bin/bash
# Downloads the VCV Rack 2 SDK into .sdk/Rack-SDK (the plugin and the tests build against it).
#   scripts/get-sdk.sh            the SDK for this machine (mac-arm64, mac-x64, lin-x64, win-x64)
#   RACK_SDK_URL=… scripts/get-sdk.sh   another SDK archive
set -euo pipefail
cd "$(dirname "$0")/.."
if [ -f .sdk/Rack-SDK/plugin.mk ]; then
	echo "Rack SDK уже на месте: .sdk/Rack-SDK"
	exit 0
fi
case "$(uname -s)-$(uname -m)" in
	Darwin-arm64) ARCH=mac-arm64 ;;
	Darwin-x86_64) ARCH=mac-x64 ;;
	Linux-x86_64) ARCH=lin-x64 ;;
	*) ARCH=win-x64 ;;
esac
URL="${RACK_SDK_URL:-https://vcvrack.com/downloads/Rack-SDK-latest-$ARCH.zip}"
mkdir -p .sdk
echo "Скачиваю $URL"
curl -fL "$URL" -o .sdk/Rack-SDK.zip
unzip -q -o .sdk/Rack-SDK.zip -d .sdk
rm .sdk/Rack-SDK.zip
test -f .sdk/Rack-SDK/plugin.mk
echo "Rack SDK → .sdk/Rack-SDK"
