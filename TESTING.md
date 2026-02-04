# Testing Checklist for ESP8266 Spotify Display

## Pre-Build

- [ ] Code compiles without warnings: `pio run -e esp8266`
- [ ] Code compiles with strict flags: `pio run -e lint`
- [ ] No unused variables or parameters
- [ ] Code formatting valid: `clang-format --dry-run -Werror src/main.cpp`

## Hardware Setup

- [ ] ESP8266 connected to USB (power + serial)
- [ ] MAX7219 wiring correct (D5/D7/D8)
- [ ] LED matrix powered separately (5V)
- [ ] Serial monitor ready: `pio device monitor`

## Initial Boot

- [ ] Device boots successfully
- [ ] Serial output shows: `[*] ESP_DispSpotTrack - Arduino Edition`
- [ ] AP mode active: `ESP8266-Setup-XXXX` visible in WiFi list
- [ ] LED matrix shows: `AP:192.168.4.1` (or scrolling text)

## Web Interface Access (AP Mode)

- [ ] Open `http://192.168.4.1` in browser
- [ ] Page loads with 3 sections:
  1. WiFi & MQTT Config (empty fields)
  2. Display Settings (brightness/speed sliders)
  3. Test Message (input field + button)
- [ ] Test Message works: type text → Send → displays on LED
- [ ] Brightness slider: 0-15 changes LED intensity live
- [ ] Scroll Speed slider: 50-500ms changes animation speed live

## WiFi Connection

- [ ] Enter WiFi SSID and password in web form
- [ ] Enter MQTT broker IP (e.g., 192.168.0.88)
- [ ] Enter MQTT port (1883)
- [ ] Leave MQTT user/pass empty for now
- [ ] Click Save
- [ ] Device reboots (wait 5-10s)
- [ ] Serial shows: `[→] Connecting to WiFi: YourSSID`
- [ ] Serial shows: `[✓] WiFi connected: 192.168.X.X`
- [ ] LED shows: `READY` for 5 seconds
- [ ] Can ping device: `ping esp8266-spotify.local` (or IP)

## WiFi + Web Interface (STA Mode)

- [ ] Open `http://esp8266-spotify.local` (or direct IP)
- [ ] Web form shows saved WiFi/MQTT values
- [ ] AP mode still broadcasts (can connect for failsafe)
- [ ] Test message still works from web form
- [ ] Brightness/speed sliders still update LED live

## MQTT Connection

- [ ] Verify broker is running: `mosquitto_pub -h 192.168.0.88 -t "test" -m "hello"`
- [ ] Serial shows: `[→] Connecting to MQTT: 192.168.0.88:1883`
- [ ] Serial shows: `[✓] MQTT connected`
- [ ] Serial shows: `[✓] Subscribed to: home_assistant/spotify/current`

## MQTT Test Messages

- [ ] Publish test via CLI:

  ```bash
  mosquitto_pub -h 192.168.0.88 -t "home_assistant/spotify/current" -m "Artist - Song"
  ```

- [ ] LED displays: `ARTIST - SONG` (uppercase, scrolling)
- [ ] Message loops continuously until new MQTT message
- [ ] Empty message clears display:

  ```bash
  mosquitto_pub -h 192.168.0.88 -t "home_assistant/spotify/current" -m ""
  ```

- [ ] LED turns black (all pixels off)

## MQTT Brightness Control

- [ ] Publish brightness via CLI:

  ```bash
  mosquitto_pub -h 192.168.0.88 -t "home_assistant/spotify/brightness" -m "15"
  ```

- [ ] LED becomes brighter
- [ ] Publish `0` (very dim) - LED barely visible
- [ ] Publish `8` (medium) - LED normal
- [ ] Publish `15` (max) - LED bright
- [ ] Reboot device - brightness value persists

## MQTT Scroll Speed Control

- [ ] Publish speed via CLI:

  ```bash
  mosquitto_pub -h 192.168.0.88 -t "home_assistant/spotify/scroll_speed" -m "50"
  ```

- [ ] Text scrolls VERY FAST
- [ ] Publish `100` - normal speed
- [ ] Publish `500` - very slow scroll
- [ ] Reboot device - speed value persists

## MQTT Authentication

- [ ] Add username/password in web form: `mqtt_user` / `mqtt_pass`
- [ ] Test with broker that requires auth
- [ ] Serial shows successful connection with credentials
- [ ] Settings persist across reboot

## Boot Persistence

- [ ] Change brightness to 10, speed to 200 via web
- [ ] Send message: `"TEST MESSAGE"`
- [ ] Power off ESP8266
- [ ] Power on ESP8266
- [ ] Message displays: `TEST MESSAGE`
- [ ] Brightness is 10, speed is 200 (from EEPROM)

## Home Assistant Automation (Optional)

- [ ] Create HA automation to publish Spotify track
- [ ] Play song on Spotify device
- [ ] LED displays: `ARTIST - TITLE`
- [ ] Pause song → empty message → LED clears
- [ ] Change to next song → LED updates with new track

## Edge Cases

- [ ] Very long text (>64 chars) - truncated and displayed
- [ ] Special characters - filtered to ASCII 32-126
- [ ] Empty message - display clears
- [ ] MQTT broker offline - shows `FAILED` for 5s, then continues
- [ ] WiFi disconnect - falls back to AP mode, web interface works
- [ ] Rapid message updates - no crashes, smooth animation

## Performance

- [ ] No memory leaks: monitor RAM over 10 min
- [ ] Responsive: web buttons respond in <1s
- [ ] MQTT latency: <100ms between publish and display
- [ ] Animation smooth: no stuttering or flicker

## Final Checklist

- [ ] All above tests pass
- [ ] Serial monitor output clean (no errors)
- [ ] Code compiles without warnings
- [ ] Ready for production deployment

---

**Test Date**: ____________________  
**Tested By**: ____________________  
**Result**: ✅ PASS / ❌ FAIL  
**Notes**:
