# ESP8266 Spotify Display - Arduino Edition

A scrolling LED display that fetches the current Spotify track from Home Assistant via MQTT and displays it on a MAX7219 8×32 LED matrix.

## Hardware

| Component | Model | Notes |
|-----------|-------|-------|
| Microcontroller | ESP8266 (Wemos D1 Mini) | Main controller |
| LED Display | MAX7219 8×32 | 4× 8×8 matrix modules |
| Power Supply | 5V USB | Powers ESP8266 + LED matrix |
| MQTT Broker | Mosquitto (Home Assistant) | Message transport |

## Pinout

| ESP8266 Pin | Function | MAX7219 Pin |
|---|---|---|
| GPIO14 (D5) | Clock (CLK) | CLK |
| GPIO13 (D7) | Chip Select (CS) | CS |
| GPIO15 (D8) | Data In (DIN) | DIN |
| GND | Ground | GND |
| 5V | Power | VCC |

## Installation

### 1. Install Arduino IDE + Board Support

```bash
# Install Arduino IDE from https://www.arduino.cc/

# Or use VS Code with PlatformIO extension
```

### 2. Add ESP8266 Board to Arduino IDE

In Arduino IDE:

1. **File** → **Preferences**
2. Under "Additional Board Manager URLs", add:

   ```
   http://arduino.esp8266.com/stable/package_esp8266com_index.json
   ```

3. **Tools** → **Board Manager**
4. Search "ESP8266" and install `esp8266 by ESP8266 Community`

### 3. Install Libraries

In Arduino IDE → **Tools** → **Manage Libraries**, install:

- `PubSubClient` by Nick O'Leary (v2.8.0+)
- `MD_MAX72XX` by majicDesigns (v3.4.1+)
- `MD_Parola` by majicDesigns (v3.5.2+)

### 4. Configure Board Settings

In Arduino IDE → **Tools**:

- **Board**: Generic ESP8266 Module
- **Flash Size**: 4M (FS:2M OTA:~1019KB)
- **Upload Speed**: 921600
- **CPU Frequency**: 80 MHz
- **Port**: `/dev/cu.usbserial-*` (your serial port)

### 5. Flash the Sketch

1. Open `ESP_DispSpotTrack.ino` in Arduino IDE
2. Click **Upload** (→ button) or **Sketch** → **Upload**
3. Monitor output at 115200 baud

## Using PlatformIO (Recommended)

PlatformIO handles all dependencies automatically:

```bash
# Install PlatformIO
pip install platformio

# Build the project
pio run -e esp8266

# Upload to device
pio run -e esp8266 -t upload

# Monitor serial output
pio device monitor
```

## Configuration

### Initial Setup

1. **Power on** the ESP8266
2. **Scan WiFi networks** - look for `ESP8266-Setup-XXXX`
3. **Connect** to that AP (no password)
4. **Open browser** to `http://192.168.4.1`
5. **Fill in form**:
   - WiFi SSID
   - WiFi Password
   - MQTT Broker IP/hostname
   - MQTT Broker Port (default: 1883)
6. **Click "Save & Connect"**
7. Device will reboot and connect to your network

### Stored Configuration

Configuration is saved to **EEPROM** and persists across reboots:

- WiFi SSID & Password
- MQTT Broker Host & Port
- Client ID (auto-generated from MAC)

To **reset** configuration, uncomment `resetConfig()` in setup().

## Home Assistant Integration

Add this automation to publish Spotify changes to MQTT:

```yaml
# configuration.yaml
automation:
  - id: "spotify_display_update"
    alias: "Spotify Display Update"
    trigger:
      platform: state
      entity_id: media_player.spotify
    action:
      service: mqtt.publish
      data:
        topic: "home_assistant/spotify/current"
        payload: >
          {%- if is_state('media_player.spotify', 'playing') -%}
          {{ state_attr('media_player.spotify', 'artist') }} - {{ state_attr('media_player.spotify', 'title') }}
          {%- else -%}
          
          {%- endif -%}
```

The ESP8266 will display the track name whenever Spotify changes.

## Troubleshooting

### Device not in WiFi list

- Hold RESET button for 2+ seconds
- Check USB power connection
- Try different USB cable

### Can't upload

```bash
# Reset device before upload
esptool.py -p /dev/cu.usbserial-XXXX read_mac

# Erase completely and reflash
esptool.py -p /dev/cu.usbserial-XXXX erase_flash
```

### MQTT connection fails

- Check MQTT broker is reachable: `telnet 192.168.0.204 1883`
- Verify WiFi is connected (check serial output)
- Confirm MQTT IP in config portal

### Display shows nothing

- Check wiring on SPI pins (D5, D7, D8)
- Verify 5V power to MAX7219
- Check serial monitor for initialization messages
- Try increasing `MAX_INTENSITY` (1-15)

### Garbled text on display

- This is normal - LED matrix resolution is limited
- Text is scaled and rotated for 8×32 resolution
- Longer messages scroll smoothly

## Serial Monitor

Connect at **115200 baud** to see debug output:

```
[*] ESP_DispSpotTrack - Arduino Edition
[*] Initializing...
[✓] WiFi AP created: ESP8266-Setup-DB84
[✓] IP: 192.168.4.1
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

## Performance Notes

- **Smooth scrolling** at 100ms per frame
- **WiFi reconnection** automatic with 5s retry interval
- **MQTT reconnection** automatic with 5s retry interval
- **Memory usage** ~65KB of 80KB available
- **Display refresh** ~50 FPS

## License

MIT License - See LICENSE file
