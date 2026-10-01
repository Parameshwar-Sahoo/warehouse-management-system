#!/bin/bash
# Shell script to inject hardware scan telemetry or run automated tests

DEV="/dev/wms_driver"

echo "=== WMS Hardware Scan Simulator ==="

if [ ! -e "$DEV" ]; then
    echo "Notice: $DEV is not present in /dev."
    echo "Running simulated scan flow through wms_app --demo instead."
    ../build/wms_app --demo
    exit 0
fi

echo "Injecting simulated scans into $DEV..."
# Example binary stream or ioctl trigger can be done here
echo "Driver device node is available."
