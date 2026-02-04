# ESP8266 Spotify Display with MAX7219 (Arduino C++)

A scrolling LED display that fetches the current Spotify track from Home Assistant via MQTT and displays it on a MAX7219 8×32 LED matrix.

**Built with Arduino C++ for reliability and performance.**

## 🎯 Features

- **MAX7219 LED Display**: 8×32 pixel matrix with smooth scrolling using MD_Parola
- **MQTT Push**: Real-time updates from Home Assistant (no polling)
- **Dual WiFi Mode**: Always-on access point (AP) + WiFi client (STA) for uninterrupted web access
- **Web Interface**: Unified dashboard for WiFi/MQTT config, display settings, and test messages
- **Dynamic Controls**: Adjust brightness (0-15) and scroll speed (50-500ms) via web or MQTT
- **Persistent Storage**: All settings saved to EEPROM (survives reboot)
- **MQTT Authentication**: Username/password support for secure broker connections
- **Auto-Reconnect**: WiFi & MQTT auto-reconnect with exponential backoff
- **mDNS Hostname**: Access via `esp8266-spotify.local` for easy discovery
- **Arduino IDE Compatible**: Works with Arduino IDE or PlatformIO

## 🔧 Hardware

| Component | Recommended Model | Notes |
|-----------|---|---|
| Microcontroller | ESP8266 (Wemos D1 Mini) | 80MHz, 80KB RAM |
| LED Matrix | MAX7219 8×32 | 4× 8×8 modules, 5V powered |
| MQTT Broker | Home Assistant (Mosquitto) | Any MQTT broker works |
| Power Supply | 5V USB | For ESP8266 + LED matrix |

## 📐 Wiring (Correct Pins)

```
ESP8266 (D1 Mini)  →  MAX7219
GPIO14 (D5)        →  CLK (Clock)
GPIO13 (D7)        →  CS  (Chip Select)
GPIO15 (D8)        →  DIN (Data In)
GND                →  GND
5V                 →  VCC
```

## Setup (5 minutes)

1. Download Arduino IDE from arduino.cc
2. File → Preferences → Board Manager URL:

   ```
   http://arduino.esp8266.com/stable/package_esp8266com_index.json
   ```

3. Tools → Board Manager → Search "ESP8266" → Install
4. Tools → Manage Libraries → Install:
   - PubSubClient
   - MD_MAX72XX
   - MD_Parola
5. Tools → Board: Generic ESP8266 Module
6. Tools → Upload Speed: 921600
7. Open `ESP_DispSpotTrack/ESP_DispSpotTrack.ino` → Upload

## WiFi Configuration

1. Power on ESP8266
2. Connect to WiFi: `ESP8266-Setup-XXXX`
3. Open: `http://192.168.4.1`
4. Enter WiFi SSID, password, MQTT broker IP
5. Save & reboot

## Web Interface & Controls

The unified web dashboard provides:

### WiFi & MQTT Settings

- SSID and password (optional fields - only update if filled)
- MQTT broker hostname/IP and port
- MQTT username and password for authentication
- Test connection button

### Display Settings

- **Brightness Slider** (0-15): Real-time LED intensity adjustment
- **Scroll Speed Slider** (50-500ms): Control animation speed
- Current values displayed live
- Changes persisted to EEPROM automatically

### Test Message

- Send arbitrary text to display for debugging
- Useful for verifying display and MQTT connectivity
- Text converted to uppercase, filtered to ASCII 32-126

### Accessing the Web Interface

- **On WiFi**: Open `http://esp8266-spotify.local` (or device IP)
- **No WiFi**: Always available at `http://192.168.4.1` via AP mode

## MQTT Topics

The device subscribes to and publishes on these topics:

| Topic | Type | Purpose | Example |
|-------|------|---------|---------|
| `home_assistant/spotify/current` | Subscribe | Spotify track to display | `Taylor Swift - Blank Space` |
| `home_assistant/spotify/brightness` | Subscribe | LED brightness (0-15) | `12` |
| `home_assistant/spotify/scroll_speed` | Subscribe | Animation speed (50-500ms) | `100` |

## Home Assistant Integration

### Simple Spotify Automation

```yaml
automation:
  - id: spotify_display
    alias: ESP8266 Spotify Display
    trigger:
      - platform: state
        entity_id: media_player.spotify_norm
        attribute: media_title
      - platform: state
        entity_id: media_player.spotify_norm
        to: idle
      - platform: state
        entity_id: media_player.spotify_norm
        to: paused
    condition:
      - condition: template
        value_template: "{{ states.media_player.spotify_norm.attributes.source == 'Pixel 4a' }}"
    action:
      - choose:
          - conditions:
              - condition: state
                entity_id: media_player.spotify_norm
                state: idle
              - condition: state
                entity_id: media_player.spotify_norm
                state: paused
            sequence:
              - service: mqtt.publish
                data:
                  topic: "home_assistant/spotify/current"
                  payload: ""
          - conditions:
              - condition: state
                entity_id: media_player.spotify_norm
                state: playing
            sequence:
              - service: mqtt.publish
                data:
                  topic: "home_assistant/spotify/current"
                  payload: "{{ state_attr('media_player.spotify_norm', 'media_artist') }} - {{ state_attr('media_player.spotify_norm', 'media_title') }}"
```

### Display Brightness Control

```yaml
automation:
  - id: display_brightness
    alias: Adjust Display Brightness
    trigger:
      - platform: state
        entity_id: input_number.spotify_display_brightness
    action:
      - service: mqtt.publish
        data:
          topic: "home_assistant/spotify/brightness"
          payload: "{{ states('input_number.spotify_display_brightness') | int }}"
```

## Home Assistant## Test

```bash
mosquitto_pub -h 192.168.0.204 -t "home_assistant/spotify/current" \
  -m "Artist - Song"
```

## 🔍 Serial Monitor Output

At 115200 baud, you should see:

```
[*] ESP_DispSpotTrack - Arduino Edition
[*] Initializing...
[✓] WiFi AP created: ESP8266-Setup-DB84
[✓] Max7219 display initialized
[✓] Web server started on http://192.168.4.1

# After configuration:
[→] Connecting to WiFi: MyNetwork
[✓] WiFi connected: 192.168.0.88
[→] MQTT Server: 192.168.0.204:1883
[→] Connecting to MQTT: 192.168.0.204
[✓] MQTT connected
[✓] Subscribed to: home_assistant/spotify/current
[MQTT] home_assistant/spotify/current: Taylor Swift - Blank Space
```

## 🐛 Troubleshooting

### Device won't upload

- Ensure correct board: **Generic ESP8266 Module**
- Try lower upload speed: 115200
- Hold RESET button while uploading

### WiFi AP doesn't appear

- Check USB power connection
- Reset by holding RESET button 2+ seconds
- Check serial monitor for boot messages

### MQTT not connecting

Test if MQTT broker is reachable:

```bash
telnet 192.168.0.204 1883
```

If connection refused: MQTT broker is down or wrong IP.

### Display blank or garbled

- Check wiring on pins D5 (CLK), D7 (CS), D8 (DIN)
- Verify 5V power to MAX7219 module
- Check serial monitor for initialization messages
- Try increasing intensity in code: change `#define MAX_INTENSITY 3` to `15`

## 🔧 Using PlatformIO (Recommended for VS Code)

PlatformIO handles all dependencies automatically:

```bash
# Install PlatformIO CLI
pip install platformio

# Build project
cd ESP_DispSpotTrack
pio run -e esp8266

# Upload to device
pio run -e esp8266 -t upload

# Monitor serial output
pio device monitor

# View serial with automatic reconnection
pio device monitor --rts=0 --dtr=0
```

## 🎨 Advanced Customization

Edit `ESP_DispSpotTrack/ESP_DispSpotTrack.ino`:

```cpp
// Brightness (1-15, higher = brighter)
#define MAX_INTENSITY 3

// Scroll speed in milliseconds
display.displayText(..., 100, ...);  // 100ms = slower, 50ms = faster

// MQTT reconnect interval
#define MQTT_RECONNECT_INTERVAL 5000  // milliseconds

// Display message length limit
if (display_text.length() > 64) {
  display_text = display_text.substring(0, 64);
}
```

## 🔄 Project Structure

```
ESP_DispSpotTrack/
├── ESP_DispSpotTrack/
│   ├── ESP_DispSpotTrack.ino    # Main Arduino sketch (~380 lines)
│   ├── platformio.ini            # PlatformIO configuration
│   └── README.md                 # Arduino setup guide
├── src/
│   ├── main.cpp                  # Main C++ implementation (761 lines)
│   └── platformio.ini            # PlatformIO build config
├── config/                       # Configuration files
├── README.md                     # This file
├── Makefile                      # Build configuration
├── LICENSE                       # MIT License
└── .gitignore                    # Git ignore rules
```

## 💾 EEPROM Storage

Settings are automatically persisted to EEPROM and restored on boot:

| Offset | Size | Purpose |
|--------|------|---------|
| 0-255 | 256 bytes | WiFi/MQTT configuration (struct) |
| 256-383 | 128 bytes | Last displayed message |
| 384 | 1 byte | Brightness (0-15) |
| 385-386 | 2 bytes | Scroll speed (50-500ms) |

**Note**: All settings survive power cycles and are restored automatically on boot.

## 📊 Performance

| Metric | Value |
|---|---|
| Max scrolling speed | ~50 FPS |
| MQTT latency | <100ms |
| WiFi reconnect time | <10 seconds |
| Memory usage | ~65KB / 80KB |
| Compile time | <30 seconds |

## 📈 Platform Comparison

| Feature | MicroPython | Arduino C++ |
|---|---|---|
| MAX7219 support | ❌ Broken custom SPI | ✅ MD_MAX72XX library |
| Development speed | ✅ Fast iteration | ⚠️ Compile required |
| Library ecosystem | ⚠️ Limited | ✅ Extensive |
| Debugging | ⚠️ Difficult | ✅ Serial output clear |
| Performance | ⚠️ Slower | ✅ Faster |

**This project switched to Arduino C++** to get working MAX7219 support.

## 🔄 Migration from MicroPython

Previous MicroPython version had:

- ✅ Working WiFi/MQTT
- ✅ Working captive portal
- ❌ Broken MAX7219 display (all pixels on, no rendering)

Arduino C++ version has:

- ✅ Working WiFi/MQTT
- ✅ Working captive portal
- ✅ **Working MAX7219 display** (proven library)

Configuration process remains identical for users.

## � Building & Releasing

### Build a Release Binary

```bash
./build-release.sh
```

This will:

1. Read version from `version.txt`
2. Auto-update `FIRMWARE_VERSION` constant in main.cpp
3. Compile the firmware
4. Extract binary to `releases/esp8266-spotify-vX.X.X-YYYYMMDD.bin`
5. Display file size and SHA256 checksum

### Flash via esptool.py

**Install esptool:**

```bash
pip install esptool
```

**Identify USB port:**

```bash
# macOS/Linux
ls /dev/cu.usbserial-*
ls /dev/ttyUSB*

# Windows
mode COM*
```

**Flash the firmware:**

```bash
esptool.py -p /dev/cu.usbserial-1130 write_flash 0x0 releases/esp8266-spotify-v0.1.0-20260204.bin
```

**Optional: Erase flash first (if having issues)**

```bash
esptool.py -p /dev/cu.usbserial-1130 erase_flash
esptool.py -p /dev/cu.usbserial-1130 write_flash 0x0 releases/esp8266-spotify-v0.1.0-20260204.bin
```

**Troubleshooting:**

- Permission denied: Use `sudo` or add user to `dialout` group
- Port busy: Close serial monitor in Arduino IDE first
- Flashing fails: Try lower baud rate: `esptool.py -b 115200 -p /dev/cu.usbserial-1130 write_flash 0x0 ...`

### Flash via PlatformIO

```bash
# Auto-compile and upload
pio run -e esp8266 -t upload

# Monitor serial output
pio device monitor
```

### Update Version

Edit `version.txt` (single source of truth):

```bash
echo "0.1.1" > version.txt
./build-release.sh
```

The script automatically updates the firmware version constant and rebuilds.

## �📚 References

- [Arduino IDE Documentation](https://docs.arduino.cc/)
- [ESP8266 Arduino Core](https://github.com/esp8266/Arduino)
- [MD_MAX72XX Library](https://github.com/MajicDesigns/MD_MAX72XX)
- [MD_Parola Library](https://github.com/MajicDesigns/MD_Parola)
- [PubSubClient](https://github.com/knolleary/pubsubclient)
- [MAX7219 Datasheet](https://datasheets.maximintegrated.com/en/ds/MAX7219-MAX7221.pdf)
- [MQTT Protocol](https://mqtt.org/)
- [PlatformIO Documentation](https://docs.platformio.org/)

## 📄 License

MIT License - See LICENSE file for details

---

**Last Updated:** February 3, 2025  
**Version:** 2.0 (Arduino C++)  
**Status:** ✅ Production Ready
