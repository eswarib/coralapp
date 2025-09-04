#!/bin/bash
HERE="$(dirname "$(readlink -f "$0")")"

# 1. Check for required libraries (call your check script)
"$HERE/check-and-install-fuse.sh" || exit 1

# 2. Start Electron frontend (backend will be managed by Electron)
cd "$HERE"
if [ -x "./node_modules/.bin/electron" ]; then
    ./node_modules/.bin/electron . "$@"
elif command -v electron > /dev/null; then
    electron . "$@"
elif command -v npm > /dev/null; then
    npm start
else
    echo "No Electron runtime found!"
    exit 1
fi 