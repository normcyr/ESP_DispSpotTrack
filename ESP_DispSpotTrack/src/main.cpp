/**
 * ESP_DispSpotTrack.ino - Main Arduino sketch
 * 
 * ESP8266 Spotify Display with MAX7219 LED matrix
 * Fetches current track from Home Assistant via MQTT and displays on LED matrix
 * 
 * Hardware:
 *   - ESP8266 (D1 Mini)
 *   - MAX7219 8×32 LED matrix
 *   - MQTT Broker (Home Assistant)
 * 
 * Pins:
 *   - GPIO14 (D5) = CLK
 *   - GPIO13 (D7) = CS
 *   - GPIO15 (D8) = DIN
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>
#include <PubSubClient.h>
#include <MD_MAX72xx.h>
#include <MD_Parola.h>

// ============ Configuration ============
#define EEPROM_SIZE 512
#define CONFIG_START 0
#define CONFIG_SIZE 256

// Network
#define AP_SSID "ESP8266-Setup"
#define MQTT_TOPIC "home_assistant/spotify/current"

// Hardware Pins
#define CLK_PIN D5   // GPIO14
#define CS_PIN D7    // GPIO13
#define DIN_PIN D8   // GPIO15

// MAX7219
#define MAX_DEVICES 4  // 4 x 8×8 = 8×32 matrix
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_INTENSITY 3

// ============ Configuration Structure ============
struct Config {
  char ssid[32];
  char password[64];
  char mqtt_host[32];
  int mqtt_port;
  char mqtt_user[32];
  char mqtt_pass[64];
  char client_id[32];
};

Config config;
bool config_valid = false;

// ============ Global Objects ============
ESP8266WebServer server(80);
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);
MD_MAX72XX mx(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
MD_Parola display(MD_MAX72XX::FC16_HW, DIN_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

// ============ Global Variables ============
String current_message = "";
String scroll_text = "";  // Store the scrolling text
unsigned long last_mqtt_attempt = 0;
const unsigned long MQTT_RECONNECT_INTERVAL = 5000;  // 5 seconds
bool display_enabled = true;
unsigned long wifi_connected_time = 0;  // Track when WiFi connects
bool ready_shown = false;  // Track if READY was shown
bool message_looping = false;  // Track if we're looping a message
int brightness = MAX_INTENSITY;  // Current brightness (0-15)
int scroll_speed = 100;  // Scroll speed in ms per step
const int EEPROM_MESSAGE_START = 256;  // Where to store last message
const int EEPROM_MESSAGE_SIZE = 128;  // Max size of message
const int EEPROM_BRIGHTNESS_ADDR = 384;  // Brightness setting
const int EEPROM_SCROLL_SPEED_ADDR = 385;  // Scroll speed (2 bytes: high, low)

// ============ Function Declarations ============
void loadConfig();
void saveConfig();
void resetConfig();
void loadLastMessage();
void saveLastMessage(const String& message);
void loadBrightness();
void saveBrightness();
void loadScrollSpeed();
void saveScrollSpeed();
void createAccessPoint();
void setupWebServer();
void setupMQTT();
void setupDisplay();
void connectWiFi();
void connectMQTT();
void handleRoot();
void handleConfig();
void handleBrightnessAPI();
void handleScrollSpeedAPI();
void handleTestMessageAPI();
void handleNotFound();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void updateDisplay(const String& message);
void loopMessage();
void scrollText(const String& text);

// ============ Setup ============
void setup() {
  Serial.begin(115200);
  delay(100);
  
  Serial.println("\n\n[*] ESP_DispSpotTrack - Arduino Edition");
  Serial.println("[*] Initializing...");
  
  // Initialize EEPROM
  EEPROM.begin(EEPROM_SIZE);
  
  // Load configuration
  loadConfig();
  
  // Load last message from EEPROM
  loadLastMessage();
  
  // Load display settings from EEPROM
  loadBrightness();
  loadScrollSpeed();
  
  // Initialize display
  setupDisplay();
  
  // Setup web server (always, for AP + settings access)
  setupWebServer();
  
  if (!config_valid) {
    Serial.println("[!] No valid config found. Starting WiFi AP...");
    createAccessPoint();
    display.displayText("CONFIG", PA_CENTER, 100, 2000, PA_PRINT, PA_PRINT);
  } else {
    Serial.println("[✓] Valid config loaded");
    Serial.printf("[→] WiFi SSID: %s\n", config.ssid);
    Serial.printf("[→] MQTT Host: %s:%d\n", config.mqtt_host, config.mqtt_port);
    
    connectWiFi();
    setupMQTT();
  }
}

// ============ Main Loop ============
void loop() {
  // Handle WiFi AP mode
  // Handle web server requests (AP mode + mDNS)
  server.handleClient();
  MDNS.update();
  
  if (WiFi.getMode() == WIFI_AP && WiFi.status() != WL_CONNECTED) {
    // Mode AP pur - afficher l'adresse IP
    static unsigned long last_update = 0;
    if (millis() - last_update > 2000) {
      last_update = millis();
      display.displayClear();
      display.print("AP:192.168.4.1");
    }
    return;
  }
  
  // Handle WiFi STA mode
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[!] WiFi disconnected, reconnecting...");
    connectWiFi();
    ready_shown = false;
    message_looping = false;
  }
  
  // Show READY for 5 seconds after WiFi connects
  if (WiFi.status() == WL_CONNECTED && !ready_shown) {
    if (wifi_connected_time == 0) {
      wifi_connected_time = millis();
    }
    
    if (millis() - wifi_connected_time < 5000) {
      display.displayClear();
      display.print("READY");
    } else {
      ready_shown = true;
      display.displayClear();
    }
  }
  
  // Handle MQTT connection
  if (!mqtt.connected()) {
    if (ready_shown && !message_looping) {
      // Show connection failed after READY phase
      display.displayClear();
      display.print("FAILED");
    }
    
    unsigned long now = millis();
    if (now - last_mqtt_attempt > MQTT_RECONNECT_INTERVAL) {
      last_mqtt_attempt = now;
      connectMQTT();
    }
  } else {
    mqtt.loop();
  }
  
  // Update display animation for looping messages
  if (message_looping) {
    loopMessage();
  }
  
  delay(10);
}

// ============ Configuration Management ============
void loadConfig() {
  EEPROM.get(CONFIG_START, config);
  
  // Validate config
  config_valid = (config.ssid[0] != 0 && config.ssid[0] != 255 && 
                  config.mqtt_host[0] != 0 && config.mqtt_host[0] != 255);
  
  if (config_valid) {
    config.ssid[31] = 0;
    config.password[63] = 0;
    config.mqtt_host[31] = 0;
    config.client_id[31] = 0;
  }
}

void saveConfig() {
  EEPROM.put(CONFIG_START, config);
  EEPROM.commit();
  config_valid = true;
  Serial.println("[✓] Config saved to EEPROM");
}

void resetConfig() {
  memset(&config, 0, sizeof(config));
  EEPROM.put(CONFIG_START, config);
  EEPROM.commit();
  config_valid = false;
  Serial.println("[✓] Config reset");
}

void saveLastMessage(const String& message) {
  // Save last message to EEPROM for persistence
  char buffer[EEPROM_MESSAGE_SIZE];
  memset(buffer, 0, EEPROM_MESSAGE_SIZE);
  message.toCharArray(buffer, EEPROM_MESSAGE_SIZE - 1);
  
  for (int i = 0; i < EEPROM_MESSAGE_SIZE; i++) {
    EEPROM.write(EEPROM_MESSAGE_START + i, buffer[i]);
  }
  EEPROM.commit();
  Serial.printf("[✓] Message saved to EEPROM: %s\n", message.c_str());
}

void loadLastMessage() {
  // Load last message from EEPROM on boot
  char buffer[EEPROM_MESSAGE_SIZE];
  memset(buffer, 0, EEPROM_MESSAGE_SIZE);
  
  for (int i = 0; i < EEPROM_MESSAGE_SIZE; i++) {
    buffer[i] = EEPROM.read(EEPROM_MESSAGE_START + i);
  }
  
  if (buffer[0] != 0 && buffer[0] != 255) {
    current_message = String(buffer);
    Serial.printf("[✓] Loaded message from EEPROM: %s\n", current_message.c_str());
    // Don't auto-display yet, wait for MQTT connection
  }
}

void loadBrightness() {
  int stored = EEPROM.read(EEPROM_BRIGHTNESS_ADDR);
  if (stored != 255 && stored >= 0 && stored <= 15) {
    brightness = stored;
    display.setIntensity(brightness);
    Serial.printf("[✓] Loaded brightness from EEPROM: %d\n", brightness);
  }
}

void saveBrightness() {
  EEPROM.write(EEPROM_BRIGHTNESS_ADDR, brightness);
  EEPROM.commit();
  Serial.printf("[✓] Brightness saved to EEPROM: %d\n", brightness);
}

void loadScrollSpeed() {
  byte high = EEPROM.read(EEPROM_SCROLL_SPEED_ADDR);
  byte low = EEPROM.read(EEPROM_SCROLL_SPEED_ADDR + 1);
  int stored = (high << 8) | low;
  
  if (stored != 0xFFFF && stored >= 50 && stored <= 500) {
    scroll_speed = stored;
    Serial.printf("[✓] Loaded scroll_speed from EEPROM: %d ms\n", scroll_speed);
  }
}

void saveScrollSpeed() {
  EEPROM.write(EEPROM_SCROLL_SPEED_ADDR, (scroll_speed >> 8) & 0xFF);
  EEPROM.write(EEPROM_SCROLL_SPEED_ADDR + 1, scroll_speed & 0xFF);
  EEPROM.commit();
  Serial.printf("[✓] Scroll speed saved to EEPROM: %d ms\n", scroll_speed);
}

// ============ WiFi & Network ============
void createAccessPoint() {
  // Get MAC address for unique SSID
  uint8_t mac[6];
  WiFi.macAddress(mac);
  String device_id = String(mac[4], HEX) + String(mac[5], HEX);
  device_id.toUpperCase();
  
  String ap_ssid = String(AP_SSID) + "-" + device_id;
  
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid.c_str(), "", 1, false, 4);
  WiFi.softAPConfig(IPAddress(192, 168, 4, 1), 
                    IPAddress(192, 168, 4, 1), 
                    IPAddress(255, 255, 255, 0));
  
  Serial.printf("[✓] WiFi AP created: %s\n", ap_ssid.c_str());
  Serial.println("[✓] IP: 192.168.4.1");
}

void connectWiFi() {
  Serial.printf("[→] Connecting to WiFi: %s\n", config.ssid);
  
  // Mode AP+STA: Garder l'AP actif pour accès web + connexion à réseau principal
  WiFi.mode(WIFI_AP_STA);
  
  // Configurer l'AP toujours actif
  String ap_ssid = "ESP8266-Setup-" + String(WiFi.macAddress().substring(9));
  WiFi.softAP(ap_ssid.c_str(), "12345678");
  Serial.printf("[✓] Soft AP started: %s on 192.168.4.1\n", ap_ssid.c_str());
  
  // Connecter au réseau WiFi principal
  WiFi.begin(config.ssid, config.password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\n[✓] WiFi connected: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("\n[!] WiFi connection failed");
    display.displayText("WiFi FAILED", PA_CENTER, 100, 3000, PA_PRINT, PA_PRINT);
  }
}

void setupMQTT() {
  mqtt.setServer(config.mqtt_host, config.mqtt_port);
  mqtt.setCallback(mqttCallback);
  
  Serial.printf("[→] MQTT Server: %s:%d\n", config.mqtt_host, config.mqtt_port);
}

void connectMQTT() {
  if (!WiFi.isConnected()) {
    return;
  }
  
  Serial.printf("[→] Connecting to MQTT: %s\n", config.mqtt_host);
  
  if (mqtt.connect(config.client_id, config.mqtt_user, config.mqtt_pass)) {
    Serial.println("[✓] MQTT connected");
    mqtt.subscribe(MQTT_TOPIC);
    mqtt.subscribe("home_assistant/spotify/brightness");
    mqtt.subscribe("home_assistant/spotify/scroll_speed");
    Serial.printf("[✓] Subscribed to: %s\n", MQTT_TOPIC);
    Serial.println("[✓] Subscribed to: home_assistant/spotify/brightness");
    Serial.println("[✓] Subscribed to: home_assistant/spotify/scroll_speed");
  } else {
    Serial.printf("[!] MQTT connection failed, code: %d\n", mqtt.state());
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // Convert payload to null-terminated string
  char buffer[512];
  if (length > 511) length = 511;
  memcpy(buffer, payload, length);
  buffer[length] = '\0';
  
  String message = String(buffer);
  String topic_str = String(topic);
  
  // Trim whitespace
  message.trim();
  
  Serial.printf("[MQTT] %s: %s\n", topic, message.c_str());
  
  // Check which topic this is
  if (topic_str == MQTT_TOPIC) {
    // Main display message
    if (message.length() > 0) {
      updateDisplay(message);
    } else {
      updateDisplay("");  // Call updateDisplay with empty to clear
    }
  } else if (topic_str == "home_assistant/spotify/brightness") {
    // Control brightness
    int new_brightness = message.toInt();
    if (new_brightness >= 0 && new_brightness <= 15) {
      brightness = new_brightness;
      display.setIntensity(brightness);
      saveBrightness();  // Save to EEPROM
      Serial.printf("[✓] Brightness set to %d\n", brightness);
    }
  } else if (topic_str == "home_assistant/spotify/scroll_speed") {
    // Control scroll speed
    int new_speed = message.toInt();
    if (new_speed >= 50 && new_speed <= 500) {
      scroll_speed = new_speed;
      saveScrollSpeed();  // Save to EEPROM
      Serial.printf("[✓] Scroll speed set to %d ms\n", scroll_speed);
    }
  }
}

// ============ Web Server ============
void setupWebServer() {
  server.on("/", handleRoot);
  server.on("/config", HTTP_POST, handleConfig);
  server.on("/api/brightness", handleBrightnessAPI);
  server.on("/api/scroll_speed", handleScrollSpeedAPI);
  server.on("/api/test-message", handleTestMessageAPI);
  server.onNotFound(handleNotFound);
  
  server.begin();
  
  // Initialiser mDNS pour accès par hostname.local
  if (MDNS.begin("esp8266-spotify")) {
    MDNS.addService("http", "tcp", 80);
    Serial.println("[✓] mDNS started: http://esp8266-spotify.local");
  } else {
    Serial.println("[!] mDNS failed");
  }
  
  Serial.println("[✓] Web server started on http://192.168.4.1 ou http://esp8266-spotify.local");
}

void handleRoot() {
  String html = R"EOF(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP8266 Spotify Display</title>
  <style>
    body { font-family: Arial; margin: 0; padding: 20px; background: #f5f5f5; }
    .container { max-width: 500px; margin: 30px auto; background: white; padding: 30px; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
    h1 { color: #333; text-align: center; margin-top: 0; }
    h2 { color: #555; border-bottom: 2px solid #007bff; padding-bottom: 10px; margin-top: 25px; }
    label { display: block; margin-top: 15px; font-weight: bold; color: #555; }
    input[type="text"], input[type="password"], input[type="number"], input[type="range"] { 
      width: 100%; 
      padding: 10px; 
      margin-top: 5px; 
      border: 1px solid #ddd; 
      border-radius: 4px; 
      box-sizing: border-box; 
      font-size: 14px;
    }
    input[type="range"] { padding: 0; height: 8px; }
    .range-value { display: inline-block; margin-left: 10px; font-weight: bold; color: #007bff; min-width: 60px; }
    button { 
      width: 100%; 
      padding: 12px; 
      margin-top: 20px; 
      background: #007bff; 
      color: white; 
      border: none; 
      border-radius: 4px; 
      cursor: pointer; 
      font-size: 16px; 
      font-weight: bold;
    }
    button:hover { background: #0056b3; }
    .info { background: #e7f3ff; padding: 10px; border-radius: 4px; margin-bottom: 15px; font-size: 13px; color: #004085; border-left: 4px solid #0056b3; }
    .success { color: green; display: none; margin-top: 10px; font-weight: bold; text-align: center; }
    .setting { margin: 15px 0; padding: 15px; background: #f9f9f9; border-radius: 6px; }
  </style>
</head>
<body>
  <div class="container">
    <h1>🎵 ESP8266 Spotify Display</h1>
    
    <h2>📡 WiFi & MQTT Configuration</h2>
    <div class="info">
      Configure your WiFi and MQTT broker connection.
    </div>
    
    <form method="POST" action="/config">
      <label for="ssid">WiFi SSID <span style="color:#999; font-size:12px;\">(leave empty to keep current)</span></label>
      <input type="text" id="ssid" name="ssid" placeholder="Leave empty to keep current WiFi">
      
      <label for="password">WiFi Password <span style="color:#999; font-size:12px;\">(leave empty to keep current)</span></label>
      <input type="password" id="password" name="password" placeholder="Leave empty to keep current password">
      
      <label for="mqtt_host">MQTT Broker Host <span style="color:red">*</span></label>
      <input type="text" id="mqtt_host" name="mqtt_host" required placeholder="192.168.1.100 or hostname">
      
      <label for="mqtt_port">MQTT Broker Port</label>
      <input type="number" id="mqtt_port" name="mqtt_port" value="1883" placeholder="1883">
      
      <label for="mqtt_user">MQTT Username</label>
      <input type="text" id="mqtt_user" name="mqtt_user" placeholder="username (optional)">
      
      <label for="mqtt_pass">MQTT Password</label>
      <input type="password" id="mqtt_pass" name="mqtt_pass" placeholder="password (optional)">
      
      <button type="submit">💾 Save WiFi & MQTT</button>
    </form>

    <h2>🎨 Display Settings</h2>
    
    <div class="setting">
      <label for="brightness">💡 Brightness (0-15)</label>
      <div>
        <input type="range" id="brightness" min="0" max="15" value=")EOF";
  
  html += brightness;
  html += R"EOF(">
        <span class="range-value" id="brightnessValue">)EOF";
  
  html += brightness;
  html += R"EOF(</span>/15
      </div>
    </div>
    
    <div class="setting">
      <label for="scrollSpeed">⚡ Scroll Speed (50-500ms)</label>
      <div>
        <input type="range" id="scrollSpeed" min="50" max="500" step="10" value=")EOF";
  
  html += scroll_speed;
  html += R"EOF(">
        <span class="range-value" id="scrollSpeedValue">)EOF";
  
  html += scroll_speed;
  html += R"EOF(</span>ms
      </div>
    </div>
    
    <button onclick="applySettings()">✓ Apply Settings</button>
    <div class="success" id="success">Settings applied!</div>

    <h2>📨 Test Message</h2>
    <div class="setting">
      <label for="testMessage">Send test message to display:</label>
      <input type="text" id="testMessage" placeholder="Enter test message..." value="TEST MESSAGE">
      <button onclick="sendTestMessage()" style="margin-top: 10px;">📤 Send Test</button>
    </div>
  </div>

  <script>
    const brightness = document.getElementById('brightness');
    const scrollSpeed = document.getElementById('scrollSpeed');
    const brightnessValue = document.getElementById('brightnessValue');
    const scrollSpeedValue = document.getElementById('scrollSpeedValue');
    const success = document.getElementById('success');
    const testMessage = document.getElementById('testMessage');
    
    brightness.addEventListener('input', (e) => {
      brightnessValue.textContent = e.target.value;
    });
    
    scrollSpeed.addEventListener('input', (e) => {
      scrollSpeedValue.textContent = e.target.value;
    });
    
    function applySettings() {
      const b = brightness.value;
      const s = scrollSpeed.value;
      
      Promise.all([
        fetch('/api/brightness?value=' + b),
        fetch('/api/scroll_speed?value=' + s)
      ]).then(() => {
        success.style.display = 'block';
        setTimeout(() => { success.style.display = 'none'; }, 2000);
      });
    }
    
    function sendTestMessage() {
      const msg = encodeURIComponent(testMessage.value);
      fetch('/api/test-message?text=' + msg).then(() => {
        success.style.display = 'block';
        success.textContent = 'Test message sent!';
        setTimeout(() => { success.style.display = 'none'; }, 2000);
      });
    }
  </script>
</body>
</html>
)EOF";
  
  server.send(200, "text/html; charset=utf-8", html);
}

void handleConfig() {
  if (!server.hasArg("mqtt_host")) {
    server.send(400, "text/plain", "Missing MQTT host");
    return;
  }
  
  // Only update SSID if provided
  if (server.hasArg("ssid") && server.arg("ssid").length() > 0) {
    strncpy(config.ssid, server.arg("ssid").c_str(), 31);
  }
  
  // Only update password if provided
  if (server.hasArg("password") && server.arg("password").length() > 0) {
    strncpy(config.password, server.arg("password").c_str(), 63);
  }
  
  // Always update MQTT settings
  strncpy(config.mqtt_host, server.arg("mqtt_host").c_str(), 31);
  config.mqtt_port = server.arg("mqtt_port").toInt();
  if (config.mqtt_port <= 0) config.mqtt_port = 1883;
  strncpy(config.mqtt_user, server.arg("mqtt_user").c_str(), 31);
  strncpy(config.mqtt_pass, server.arg("mqtt_pass").c_str(), 63);
  
  // Generate client ID from MAC
  uint8_t mac[6];
  WiFi.macAddress(mac);
  snprintf(config.client_id, 31, "esp8266_spotify_%02x%02x%02x", mac[3], mac[4], mac[5]);
  
  saveConfig();
  
  // Send success response
  String html = R"EOF(
<!DOCTYPE html>
<html>
<head>
  <title>Configuration Saved</title>
  <style>
    body { font-family: Arial; text-align: center; padding: 50px; }
    .success { color: green; font-size: 24px; }
  </style>
</head>
<body>
  <div class="success">✓ Configuration saved!</div>
  <p>Device is connecting to WiFi and MQTT...</p>
  <p>Restarting in 2 seconds...</p>
  <script>setTimeout(() => window.location = '/', 2000);</script>
</body>
</html>
)EOF";
  
  server.send(200, "text/html; charset=utf-8", html);
  
  // Restart device
  delay(2000);
  ESP.restart();
}

void handleNotFound() {
  server.send(404, "text/plain", "Not Found");
}

void handleBrightnessAPI() {
  if (!server.hasArg("value")) {
    server.send(400, "text/plain", "Missing value");
    return;
  }
  
  int value = server.arg("value").toInt();
  if (value < 0) value = 0;
  if (value > 15) value = 15;
  
  brightness = value;
  display.setIntensity(brightness);
  saveBrightness();  // Save to EEPROM
  
  Serial.print("[✓] Brightness set to ");
  Serial.println(brightness);
  
  server.send(200, "text/plain", "OK");
}

void handleScrollSpeedAPI() {
  if (!server.hasArg("value")) {
    server.send(400, "text/plain", "Missing value");
    return;
  }
  
  int value = server.arg("value").toInt();
  if (value < 50) value = 50;
  if (value > 500) value = 500;
  
  scroll_speed = value;
  saveScrollSpeed();  // Save to EEPROM
  
  Serial.print("[✓] Scroll speed set to ");
  Serial.print(scroll_speed);
  Serial.println("ms");
  
  server.send(200, "text/plain", "OK");
}

void handleTestMessageAPI() {
  if (!server.hasArg("text")) {
    server.send(400, "text/plain", "Missing text");
    return;
  }
  
  String test_msg = server.arg("text");
  Serial.printf("[→] Test message received: %s\n", test_msg.c_str());
  updateDisplay(test_msg);
  
  server.send(200, "text/plain", "OK");
}

// ============ Display ============
void setupDisplay() {
  display.begin();
  display.setIntensity(MAX_INTENSITY);
  display.setCharSpacing(1);
  display.setTextAlignment(PA_LEFT);
  display.displayClear();
  
  Serial.println("[✓] MAX7219 display initialized");
}

void updateDisplay(const String& message) {
  current_message = message;
  Serial.printf("[→] Displaying: %s\n", message.c_str());
  
  // Handle empty message - clear display
  if (message.length() == 0) {
    display.displayClear();
    message_looping = false;
    scroll_text = "";
    Serial.println("[→] Display cleared");
    return;
  }
  
  // Save this message to EEPROM for persistence
  saveLastMessage(message);
  
  // Convert to uppercase
  String display_text = message;
  display_text.toUpperCase();
  
  // Remove non-ASCII characters (MAX7219 only supports ASCII)
  String clean_text = "";
  for (unsigned int i = 0; i < display_text.length(); i++) {
    char c = display_text[i];
    if (c >= 32 && c <= 126) {
      clean_text += c;
    }
  }
  display_text = clean_text;
  
  // Trim to reasonable length
  if (display_text.length() > 64) {
    display_text = display_text.substring(0, 64);
  }
  
  // Add padding spaces for proper scrolling
  scroll_text = "    " + display_text + "    ";
  
  // Clear and setup scrolling (ONLY ONCE)
  display.displayClear();
  display.setTextAlignment(PA_LEFT);
  display.setCharSpacing(1);
  display.displayScroll(scroll_text.c_str(), PA_LEFT, PA_SCROLL_LEFT, scroll_speed);
  
  // Mark that we're looping a message
  message_looping = true;
}

// Helper to continue looping the current message
void loopMessage() {
  if (message_looping && scroll_text.length() > 0) {
    if (display.displayAnimate()) {
      // Scroll finished, restart it - but use the SAME stored text
      display.displayScroll(scroll_text.c_str(), PA_LEFT, PA_SCROLL_LEFT, scroll_speed);
    }
  }
}
