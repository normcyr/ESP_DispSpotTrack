# Copilot Instructions - ESP8266 Spotify Display

## Project Overview

ESP8266-based MAX7219 LED display showing current Spotify track via MQTT from Home Assistant.

**Tech**: Arduino C++, MAX7219 LED matrix, MQTT, WiFi, EEPROM

## Hardware

- **Microcontroller**: ESP8266 (Wemos D1 Mini)
- **Display**: MAX7219 8×32 (4× 8×8 modules)
- **Power**: 5V USB
- **Broker**: MQTT (Home Assistant)

## Wiring

```
GPIO14 (D5) → CLK
GPIO13 (D7) → CS
GPIO15 (D8) → DIN
GND         → GND
5V          → VCC
```

## Core Features

1. **Display**: Shows `Artist - Song Title` with smooth scrolling (50 FPS)
2. **WiFi**: Captive portal for initial config (192.168.4.1)
3. **MQTT**: Subscribes to `home_assistant/spotify/current`
4. **Storage**: Config saved to EEPROM (persists across reboot)

## Code Structure

**Main File**: `ESP_DispSpotTrack/ESP_DispSpotTrack.ino` (425 lines)

Sections:

1. Configuration (pins, defines)
2. Data structures (Config struct)
3. Global objects (WebServer, MQTT, Display)
4. setup() - Initialization
5. loop() - Main event loop
6. Helper functions (~40 functions)
   - Network (WiFi, MQTT)
   - Display (MAX7219 control)
   - Configuration (EEPROM, web form)
   - Web server handlers

## Libraries

- **PubSubClient**: MQTT client
- **MD_MAX72XX**: MAX7219 driver
- **MD_Parola**: Text animation
- **EEPROM**: Persistent storage

## Build & Upload

```bash
make build       # Compile with PlatformIO
make upload      # Upload to ESP8266
make monitor     # Serial monitor (115200 baud)
```

Or use Arduino IDE:

1. File → Open → ESP_DispSpotTrack.ino
2. Tools → Board: Generic ESP8266 Module
3. Tools → Upload Speed: 921600
4. Click Upload

## Configuration

Default MQTT topic: `home_assistant/spotify/current`

Home Assistant automation:

```yaml
automation:
  - id: spotify_display
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

## Testing

```bash
mosquitto_pub -h 192.168.0.204 -t "home_assistant/spotify/current" \
  -m "Artist - Song Title"
```

Text should scroll on display immediately.

## Code Guidelines

- **Language**: C++ (Arduino)
- **Style**: Clean, readable
- **Comments**: For complex logic
- **Functions**: Well-organized by purpose
- **Indentation**: 2 spaces (Arduino standard)

## Configuration (Modifiable)

Edit `ESP_DispSpotTrack.ino`:

```cpp
#define MAX_INTENSITY 3              // Brightness (1-15)
#define MQTT_RECONNECT_INTERVAL 5000 // ms between reconnect attempts
#define MQTT_TOPIC "home_assistant/spotify/current"
#define CLK_PIN D5                   // GPIO14
#define CS_PIN D7                    // GPIO13
#define DIN_PIN D8                   // GPIO15
```

## Troubleshooting

| Problem | Solution |
|---------|----------|
| Upload fails | Board: Generic ESP8266 Module |
| WiFi AP missing | Reset device (unplug/replug) |
| MQTT won't connect | Check broker IP & port |
| Display blank | Verify wiring D5/D7/D8 |

## References

- [Arduino IDE](https://docs.arduino.cc/)
- [ESP8266 Arduino](https://github.com/esp8266/Arduino)
- [MD_MAX72XX](https://github.com/MajicDesigns/MD_MAX72XX)
- [MD_Parola](https://github.com/MajicDesigns/MD_Parola)
- [PubSubClient](https://github.com/knolleary/pubsubclient)
