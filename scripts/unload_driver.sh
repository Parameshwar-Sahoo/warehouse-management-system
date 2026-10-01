#!/bin/bash
set -e

echo "=== Unloading Linux Character Device Driver (wms_driver) ==="

sudo rmmod wms_driver || {
    echo "Module wms_driver was not currently loaded."
    exit 0
}

echo "wms_driver unloaded successfully."
dmesg | tail -n 5
