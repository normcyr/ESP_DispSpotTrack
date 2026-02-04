#!/bin/bash
# Build and release ESP8266 Spotify Display firmware

set -e

# Read version from version.txt
VERSION=$(cat version.txt | tr -d '\n' | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')
RELEASE_DIR="releases"
BUILD_DATE=$(date +%Y%m%d)
FIRMWARE_NAME="esp8266-spotify-v${VERSION}-${BUILD_DATE}.bin"

echo "🔨 Building firmware v${VERSION}..."

# Update version in main.cpp
echo "📝 Updating version in main.cpp..."
sed -i '' "s/#define FIRMWARE_VERSION \"[^\"]*\"/#define FIRMWARE_VERSION \"${VERSION}\"/" \
  ESP_DispSpotTrack/src/main.cpp

if [ $? -ne 0 ]; then
  echo "❌ Failed to update version in main.cpp"
  exit 1
fi

echo "✓ Version updated to ${VERSION}"

cd ESP_DispSpotTrack
pio run -e esp8266
cd ..

echo "📦 Creating release directory..."
mkdir -p "$RELEASE_DIR"

# Extract firmware from ELF
echo "📝 Extracting firmware binary..."
esptool.py --chip esp8266 elf2image \
  -o "$RELEASE_DIR/tmp.bin" \
  ESP_DispSpotTrack/.pio/build/esp8266/firmware.elf

# Move the main firmware fragment to the final name
if [ -f "$RELEASE_DIR/tmp.bin0x00000.bin" ]; then
  mv "$RELEASE_DIR/tmp.bin0x00000.bin" "$RELEASE_DIR/$FIRMWARE_NAME"
  rm -f "$RELEASE_DIR/tmp.bin"*
else
  echo "❌ Failed to extract firmware"
  exit 1
fi

# Get file info
SIZE=$(ls -lh "$RELEASE_DIR/$FIRMWARE_NAME" | awk '{print $5}')
SHA=$(shasum -a 256 "$RELEASE_DIR/$FIRMWARE_NAME" | awk '{print $1}')

echo ""
echo "✅ Firmware built successfully!"
echo ""
echo "📋 Release Info:"
echo "   File: $FIRMWARE_NAME"
echo "   Size: $SIZE"
echo "   SHA256: $SHA"
echo ""
echo "📖 To flash this firmware:"
echo "   esptool.py -p /dev/cu.usbserial-XXXX write_flash 0x0 $RELEASE_DIR/$FIRMWARE_NAME"
echo ""
echo "📂 Output: $RELEASE_DIR/$FIRMWARE_NAME"
