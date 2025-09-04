#!/bin/bash
set -eu

# Paths
ELECTRON_DIR="../coral-electron"
BACKEND_DIR="../coral"
APPDIR="../coral.appdir"
SCRIPTS_DIR="../coral/scripts"

# Check if required directories exist
if [ ! -d "$ELECTRON_DIR" ]; then
    echo "Error: Electron directory not found: $ELECTRON_DIR"
    exit 1
fi

if [ ! -d "$BACKEND_DIR" ]; then
    echo "Error: Backend directory not found: $BACKEND_DIR"
    exit 1
fi

# Pull version info from package.json
APP_VERSION=$(node -p "require('$ELECTRON_DIR/package.json').version")
BUILD_DATE=$(date +%Y-%m-%d)
GIT_COMMIT=$(git rev-parse --short HEAD)

echo "Embedding version $APP_VERSION ($GIT_COMMIT, $BUILD_DATE) into backend"

# Check if version.h.in exists
if [ ! -f "$BACKEND_DIR/src/version.h.in" ]; then
    echo "Warning: version.h.in not found, skipping version embedding"
else
    # Generate version.h from template
    sed -e "s/@APP_VERSION@/$APP_VERSION/" \
        -e "s/@BUILD_DATE@/$BUILD_DATE/" \
        -e "s/@GIT_COMMIT@/$GIT_COMMIT/" \
        "$BACKEND_DIR/src/version.h.in" > "$BACKEND_DIR/src/version.h"
fi

# Build backend
pushd "$BACKEND_DIR"
make clean
make all
popd

echo "Backend build complete."

# Optional: Build Electron frontend
echo "Building Electron frontend..."
pushd "$ELECTRON_DIR"
npm ci
# npm run build   # Uncomment if you have a build script

# Remove devDependencies to keep node_modules small
npm prune --production

popd
echo "Frontend build complete."

echo "Copying Electron app into AppDir..."
rm -rf "$APPDIR"
mkdir -p "$APPDIR"

# Copy Electron app files to root of AppDir (this is what the AppRun script expects)
cp -r "$ELECTRON_DIR/main.js" "$APPDIR/"
cp -r "$ELECTRON_DIR/renderer" "$APPDIR/"
cp -r "$ELECTRON_DIR/node_modules" "$APPDIR/"
cp -r "$ELECTRON_DIR/package.json" "$APPDIR/"


echo "Copying backend binary into AppDir..."
# Copy backend binary into AppDir
mkdir -p "$APPDIR/usr/bin"
cp -r "$BACKEND_DIR/bin/coral" "$APPDIR/usr/bin/"
strip "$APPDIR/usr/bin/coral"

echo "Copied backend binary into AppDir..."

#copy libraries needed for whisper
mkdir -p "$APPDIR/usr/lib"
cp -r "$BACKEND_DIR/lib/"* "$APPDIR/usr/lib/"
bash $BACKEND_DIR/scripts/collect-libs.sh $BACKEND_DIR/bin/coral $APPDIR/usr/lib

#copy config
mkdir -p "$APPDIR/usr/share/coral/conf"
cp "$BACKEND_DIR/conf/config.json" "$APPDIR/usr/share/coral/conf/config.json"

MODEL_DIR="$APPDIR/usr/share/coral/models"
mkdir -p "$MODEL_DIR"

#copy models - check multiple possible locations
# Download models if not already present
download_model() {
    local url="$1"
    local dest="$2"
    if [ ! -f "$dest" ]; then
        echo "Downloading $dest..."
        wget -O "$dest" "$url"
    else
        echo "$dest already exists, skipping."
    fi
}

#download_model "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-tiny.en.bin" "$MODEL_DIR/ggml-tiny.en.bin"
download_model "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.en.bin" "$MODEL_DIR/ggml-base.en.bin"
#download_model "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-small.en.bin" "$MODEL_DIR/ggml-small.en.bin"
#download_model "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-medium.en.bin" "$MODEL_DIR/ggml-medium.en.bin"
#download_model "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-large-v3.bin" "$MODEL_DIR/ggml-large-v3.bin"


# Copy icons, desktop file, and AppRun
cp "$SCRIPTS_DIR/coral.desktop" "$APPDIR/"
cp "../logo/coral.png" "$APPDIR/node_modules/electron/dist/resources/coral.png"
cp "../logo/coral.png" "$APPDIR/usr/share/coral/coral.png"
cp "../logo/coral.png" "$APPDIR/coral.png"
# Create directory for desktop icon and copy icon
mkdir -p "$APPDIR/usr/share/icons/hicolor/256x256/apps/"
cp "../logo/coral.png" "$APPDIR/usr/share/coral/coral.png"

cp -r "$ELECTRON_DIR/icons" "$APPDIR/"
cp -r "$ELECTRON_DIR/icons" "$APPDIR/node_modules/electron/dist/resources/"
cp -r "$ELECTRON_DIR/icons" "$APPDIR/usr/share/coral/"
cp -r "$ELECTRON_DIR/icons" "$APPDIR/usr/share/coral/"

cp "$SCRIPTS_DIR/check-and-install-fuse.sh" "$APPDIR/"
cp "$ELECTRON_DIR/package.json" "$APPDIR/"
cp "$SCRIPTS_DIR/AppRun" "$APPDIR/"
chmod +x "$APPDIR/AppRun"

#if already present removes it
rm -f CoralApp-x86_64.AppImage

echo "Building AppImage..."
appimagetool "$APPDIR" ../CoralApp-x86_64.AppImage

# Remove the AppDir after building the AppImage
rm -rf "$APPDIR"

echo "AppImage built successfully!"
