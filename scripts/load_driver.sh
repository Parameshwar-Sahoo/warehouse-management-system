#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DRIVER_DIR="$SCRIPT_DIR/../driver"

echo "=== Loading Linux Character Device Driver (wms_driver) ==="

cd "$DRIVER_DIR"
if [ ! -f wms_driver.ko ]; then
    echo "Building kernel module..."
    make
fi

# Insert module
sudo insmod wms_driver.ko
echo "Module inserted successfully."

# Ensure permissions on /dev/wms_driver
if [ -e /dev/wms_driver ]; then
    sudo chmod 666 /dev/wms_driver
    echo "Device node /dev/wms_driver permissions set to 666."
else
    echo "Warning: /dev/wms_driver not found automatically by udev."
fi

# Display dmesg
dmesg | tail -n 5
