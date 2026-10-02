#!/bin/bash
# Script to launch the interactive visual dashboard in your browser

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DASHBOARD_FILE="$SCRIPT_DIR/../web/index.html"

echo "=== Opening Warehouse Management System Interactive Dashboard ==="

if command -v wslview >/dev/null 2>&1; then
    wslview "$DASHBOARD_FILE"
elif command -v xdg-open >/dev/null 2>&1; then
    xdg-open "$DASHBOARD_FILE"
elif [ -f "/mnt/c/Windows/explorer.exe" ]; then
    /mnt/c/Windows/explorer.exe "D:\\warehouse-management-system\\web\\index.html"
else
    echo "Please open this file directly in any web browser:"
    echo "  $DASHBOARD_FILE"
fi
