#!/bin/bash


# Path to your binary (relative to this script)
BINARY=$1

# Output directory for libraries (relative to this script)
OUTDIR=$2
#mkdir -p "$OUTDIR"

# Check if the binary exists
if [ ! -f "$BINARY" ]; then
    echo "Error: Binary $BINARY not found!"
    exit 1
fi

# List of system libraries NOT to copy (edit as needed)
EXCLUDE_LIBS="(^|/)libc\.so|(^|/)libm\.so|(^|/)libpthread\.so|(^|/)libdl\.so|(^|/)librt\.so|(^|/)libstdc\+\+\.so|(^|/)libgcc_s\.so|(^|/)ld-linux|(^|/)libresolv\.so|(^|/)libnsl\.so|(^|/)libutil\.so|(^|/)libanl\.so|(^|/)libcrypt\.so|(^|/)libxcrypt\.so|(^|/)libz\.so"

ldd "$BINARY" | awk '/=>/ {print $3} !/=>/ {print $1}' | grep -v '^$' | while read -r lib; do
    if [[ -n "$lib" && ! "$lib" =~ $EXCLUDE_LIBS ]]; then
        echo "Copying $lib"
        cp -v "$lib" "$OUTDIR"
    fi
done
