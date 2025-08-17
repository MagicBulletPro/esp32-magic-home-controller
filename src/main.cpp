#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <ESPmDNS.h>

// Wi-Fi credentials
const char *ssid = "Your-WiFi-SSID";
const char *password = "Your-WiFi-Password";

// Device information
const char *deviceName = "ESP32_Controller";
const char *deviceType = "home_automation";

// Relay structure for multiple relay management
struct Relay
{
  int pin;
  bool state;
  String name;
  String description;
};

// Multiple relay configuration
const int NUM_RELAYS = 2;
Relay relays[NUM_RELAYS] = {
    {18, false, "Living Light", "Living room main lighting"},
    {19, false, "Bedroom Light", "Master bedroom tube light"},
  };

// Status LED pin
const int statusLED = 2; // Built-in LED for status indication

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Helper functions for relay management
void setRelayState(int relayIndex, bool state)
{
  if (relayIndex >= 0 && relayIndex < NUM_RELAYS)
  {
    relays[relayIndex].state = state;
    digitalWrite(relays[relayIndex].pin, state ? HIGH : LOW);
    Serial.println("Relay " + String(relayIndex + 1) + " (" + relays[relayIndex].name + ") turned " + (state ? "ON" : "OFF"));
  }
}

void toggleRelay(int relayIndex)
{
  if (relayIndex >= 0 && relayIndex < NUM_RELAYS)
  {
    setRelayState(relayIndex, !relays[relayIndex].state);
  }
}

String getRelayStatusJson()
{
  String json = "{\"relays\":[";
  for (int i = 0; i < NUM_RELAYS; i++)
  {
    if (i > 0)
      json += ",";
    json += "{";
    json += "\"id\":" + String(i + 1) + ",";
    json += "\"name\":\"" + relays[i].name + "\",";
    json += "\"description\":\"" + relays[i].description + "\",";
    json += "\"pin\":" + String(relays[i].pin) + ",";
    json += "\"state\":" + String(relays[i].state ? "true" : "false");
    json += "}";
  }
  json += "]}";
  return json;
}

// Handle incoming WebSocket messages
void onWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
  {
    // Create a null-terminated string from the data
    char *buffer = new char[len + 1];
    memcpy(buffer, data, len);
    buffer[len] = '\0';

    String msg = String(buffer);
    delete[] buffer;

    // Clean the message - remove whitespace and non-printable characters
    msg.trim();
    msg.replace("\r", "");
    msg.replace("\n", "");

    // Remove any non-ASCII characters
    String cleanMsg = "";
    for (int i = 0; i < msg.length(); i++)
    {
      char c = msg.charAt(i);
      if (c >= 32 && c <= 126)
      { // Only printable ASCII characters
        cleanMsg += c;
      }
    }
    msg = cleanMsg;
    msg.toLowerCase(); // Make case-insensitive

    Serial.print("WebSocket message received (raw length: ");
    Serial.print(len);
    Serial.print("): '");
    Serial.print(msg);
    Serial.println("'");

    // Try to parse as JSON first
    DynamicJsonDocument doc(256);
    DeserializationError error = deserializeJson(doc, msg);

    if (!error)
    {
      // Handle JSON commands
      String action = doc["action"] | "";
      action.toLowerCase();

      if (doc.containsKey("relay_id"))
      {
        // Individual relay control
        int relayId = doc["relay_id"];
        if (relayId >= 1 && relayId <= NUM_RELAYS)
        {
          int relayIndex = relayId - 1;

          if (action == "on" || action == "turn_on")
          {
            setRelayState(relayIndex, true);
            ws.textAll("{\"status\":\"success\",\"relay_id\":" + String(relayId) + ",\"action\":\"on\",\"state\":true}");
          }
          else if (action == "off" || action == "turn_off")
          {
            setRelayState(relayIndex, false);
            ws.textAll("{\"status\":\"success\",\"relay_id\":" + String(relayId) + ",\"action\":\"off\",\"state\":false}");
          }
          else if (action == "toggle")
          {
            toggleRelay(relayIndex);
            ws.textAll("{\"status\":\"success\",\"relay_id\":" + String(relayId) + ",\"action\":\"toggle\",\"state\":" + String(relays[relayIndex].state ? "true" : "false") + "}");
          }
          else if (action == "status" || action == "get_status")
          {
            String status = "{\"relay_id\":" + String(relayId) + ",\"name\":\"" + relays[relayIndex].name + "\",\"description\":\"" + relays[relayIndex].description + "\",\"pin\":" + String(relays[relayIndex].pin) + ",\"state\":" + String(relays[relayIndex].state ? "true" : "false") + "}";
            ws.textAll(status);
            Serial.println("✓ Relay " + String(relayId) + " status sent");
          }
          else
          {
            ws.textAll("{\"status\":\"error\",\"message\":\"Invalid action for relay\",\"valid_actions\":[\"on\",\"off\",\"toggle\",\"status\"]}");
          }
        }
        else
        {
          ws.textAll("{\"status\":\"error\",\"message\":\"Invalid relay_id. Must be between 1 and " + String(NUM_RELAYS) + "\"}");
        }
        return;
      }
      else if (action == "get_all_status" || action == "status")
      {
        // Get status of all relays
        ws.textAll(getRelayStatusJson());
        Serial.println("✓ All relay status sent");
        return;
      }
      else if (action == "all_on" || action == "turn_all_on")
      {
        // Turn on all relays
        for (int i = 0; i < NUM_RELAYS; i++)
        {
          setRelayState(i, true);
        }
        Serial.println("✓ All relays turned ON");
        ws.textAll("{\"status\":\"success\",\"action\":\"all_on\",\"message\":\"All relays turned ON\"}");
        return;
      }
      else if (action == "all_off" || action == "turn_all_off")
      {
        // Turn off all relays
        for (int i = 0; i < NUM_RELAYS; i++)
        {
          setRelayState(i, false);
        }
        Serial.println("✓ All relays turned OFF");
        ws.textAll("{\"status\":\"success\",\"action\":\"all_off\",\"message\":\"All relays turned OFF\"}");
        return;
      }
      else
      {
        ws.textAll("{\"status\":\"error\",\"message\":\"Invalid action\",\"valid_actions\":[\"on\",\"off\",\"toggle\",\"status\",\"all_on\",\"all_off\"]}");
        return;
      }
    }
    else
    {
      Serial.println("✗ Unknown command: '" + msg + "'");
      Serial.print("  Hex dump: ");
      for (size_t i = 0; i < len; i++)
      {
        Serial.printf("%02X ", data[i]);
      }
      Serial.println();

      String helpMsg = "{\"status\":\"error\",\"message\":\"Unknown command\",\"command\":\"" + msg + "\",";
      helpMsg += "\"help\":{";
      helpMsg += "\"json_format\":{";
      helpMsg += "\"relay_control\":\"{\\\"relay_id\\\":1,\\\"action\\\":\\\"on\\\"}\",";
      helpMsg += "\"all_control\":\"{\\\"action\\\":\\\"all_on\\\"}\",";
      helpMsg += "\"status\":\"{\\\"action\\\":\\\"status\\\"}\"";
      helpMsg += "}}}";
      ws.textAll(helpMsg);
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
             AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  switch (type)
  {
  case WS_EVT_CONNECT:
    Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    // Send welcome message with current status of all relays
    client->text("{\"message\":\"Connected to ESP32\",\"relays\":" + getRelayStatusJson().substring(10, getRelayStatusJson().length() - 1) + "}");
    break;

  case WS_EVT_DISCONNECT:
    Serial.printf("WebSocket client #%u disconnected\n", client->id());
    break;

  case WS_EVT_DATA:
    onWebSocketMessage(arg, data, len);
    break;

  case WS_EVT_PONG:
    Serial.printf("WebSocket pong received from client #%u\n", client->id());
    break;

  case WS_EVT_ERROR:
    Serial.printf("WebSocket error on client #%u\n", client->id());
    break;

  default:
    break;
  }
}

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=== ESP32 WebSocket Home Controller ===");
  Serial.print("Device: ");
  Serial.println(deviceName);

  // Initialize GPIO pins for all relays
  Serial.println("Initializing relays:");
  for (int i = 0; i < NUM_RELAYS; i++)
  {
    pinMode(relays[i].pin, OUTPUT);
    digitalWrite(relays[i].pin, LOW);
    relays[i].state = false;
    Serial.println("  Relay " + String(i + 1) + " (" + relays[i].name + "): GPIO " + String(relays[i].pin));
  }

  // Initialize status LED
  pinMode(statusLED, OUTPUT);
  digitalWrite(statusLED, LOW);

  Serial.print("Status LED: GPIO ");
  Serial.println(statusLED);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi network: ");
  Serial.println(ssid);

  int connectionAttempts = 0;
  while (WiFi.status() != WL_CONNECTED && connectionAttempts < 30)
  {
    delay(500);
    Serial.print(".");
    connectionAttempts++;
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("\n✓ Wi-Fi Connected Successfully!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("MAC Address: ");
    Serial.println(WiFi.macAddress());
    Serial.print("Signal Strength: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");

    // Blink status LED to indicate successful connection
    for (int i = 0; i < 3; i++)
    {
      digitalWrite(statusLED, HIGH);
      delay(200);
      digitalWrite(statusLED, LOW);
      delay(200);
    }
  }
  else
  {
    Serial.println("\n✗ Wi-Fi Connection Failed!");
    Serial.println("Please check your credentials and try again.");
    // Keep blinking LED to indicate error
    while (true)
    {
      digitalWrite(statusLED, HIGH);
      delay(100);
      digitalWrite(statusLED, LOW);
      delay(100);
    }
  }

  // Setup mDNS for device discovery
  if (MDNS.begin(deviceName))
  {
    Serial.println("✓ mDNS responder started");
    MDNS.addService("http", "tcp", 80);
    MDNS.addService("ws", "tcp", 80);
    Serial.print("Device discoverable as: ");
    Serial.print(deviceName);
    Serial.println(".local");
  }
  else
  {
    Serial.println("✗ mDNS setup failed");
  }

  // Setup WebSocket
  ws.onEvent(onEvent);
  server.addHandler(&ws);

  // Add HTTP endpoints for device discovery and status
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    String html = "<html><body><h1>ESP32 Home Controller</h1>";
    html += "<p>Device: " + String(deviceName) + "</p>";
    html += "<p>IP: " + WiFi.localIP().toString() + "</p>";
    html += "<p>WebSocket: ws://" + WiFi.localIP().toString() + "/ws</p>";
    html += "<h2>Relay Status:</h2>";
    for (int i = 0; i < NUM_RELAYS; i++) {
      html += "<p>" + relays[i].name + " (GPIO " + String(relays[i].pin) + "): " + String(relays[i].state ? "ON" : "OFF") + "</p>";
    }
    html += "</body></html>";
    request->send(200, "text/html", html); });

  server.on("/info", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    String json = "{";
    json += "\"device_name\":\"" + String(deviceName) + "\",";
    json += "\"device_type\":\"" + String(deviceType) + "\",";
    json += "\"ip_address\":\"" + WiFi.localIP().toString() + "\",";
    json += "\"mac_address\":\"" + WiFi.macAddress() + "\",";
    json += "\"num_relays\":" + String(NUM_RELAYS) + ",";
    json += "\"relays\":" + getRelayStatusJson().substring(10, getRelayStatusJson().length() - 1);
    json += "}";
    request->send(200, "application/json", json); });

  // API endpoints for relay management
  server.on("/api/relays", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "application/json", getRelayStatusJson()); });

  server.on("/api/relay", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    if (request->hasParam("id")) {
      int relayId = request->getParam("id")->value().toInt();
      if (relayId >= 1 && relayId <= NUM_RELAYS) {
        int index = relayId - 1;
        String json = "{";
        json += "\"id\":" + String(relayId) + ",";
        json += "\"name\":\"" + relays[index].name + "\",";
        json += "\"description\":\"" + relays[index].description + "\",";
        json += "\"pin\":" + String(relays[index].pin) + ",";
        json += "\"state\":" + String(relays[index].state ? "true" : "false");
        json += "}";
        request->send(200, "application/json", json);
      } else {
        request->send(400, "application/json", "{\"error\":\"Invalid relay ID. Must be between 1 and " + String(NUM_RELAYS) + "\"}");
      }
    } else {
      request->send(400, "application/json", "{\"error\":\"Missing relay ID parameter\"}");
    } });

  server.on("/api/relay/control", HTTP_POST, [](AsyncWebServerRequest *request)
            {
    if (request->hasParam("id") && request->hasParam("action")) {
      int relayId = request->getParam("id")->value().toInt();
      String action = request->getParam("action")->value();
      
      if (relayId >= 1 && relayId <= NUM_RELAYS) {
        int index = relayId - 1;
        action.toLowerCase();
        
        if (action == "on" || action == "1" || action == "true") {
          setRelayState(index, true);
          request->send(200, "application/json", "{\"success\":true,\"relay\":" + String(relayId) + ",\"state\":true}");
        } else if (action == "off" || action == "0" || action == "false") {
          setRelayState(index, false);
          request->send(200, "application/json", "{\"success\":true,\"relay\":" + String(relayId) + ",\"state\":false}");
        } else if (action == "toggle") {
          toggleRelay(index);
          request->send(200, "application/json", "{\"success\":true,\"relay\":" + String(relayId) + ",\"state\":" + String(relays[index].state ? "true" : "false") + "}");
        } else {
          request->send(400, "application/json", "{\"error\":\"Invalid action. Use: on, off, or toggle\"}");
        }
      } else {
        request->send(400, "application/json", "{\"error\":\"Invalid relay ID. Must be between 1 and " + String(NUM_RELAYS) + "\"}");
      }
    } else {
      request->send(400, "application/json", "{\"error\":\"Missing required parameters: id and action\"}");
    } });

  server.on("/api/relays/all", HTTP_POST, [](AsyncWebServerRequest *request)
            {
    if (request->hasParam("action")) {
      String action = request->getParam("action")->value();
      action.toLowerCase();
      
      if (action == "on") {
        for (int i = 0; i < NUM_RELAYS; i++) {
          setRelayState(i, true);
        }
        request->send(200, "application/json", "{\"success\":true,\"message\":\"All relays turned ON\"}");
      } else if (action == "off") {
        for (int i = 0; i < NUM_RELAYS; i++) {
          setRelayState(i, false);
        }
        request->send(200, "application/json", "{\"success\":true,\"message\":\"All relays turned OFF\"}");
      } else {
        request->send(400, "application/json", "{\"error\":\"Invalid action. Use: on or off\"}");
      }
    } else {
      request->send(400, "application/json", "{\"error\":\"Missing action parameter\"}");
    } });

  // Enable CORS for mobile app access
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");

  server.begin();

  Serial.println("✓ WebSocket server started");
  Serial.println("✓ HTTP server started");
  Serial.println("\n=== System Ready ===");
  Serial.println("WebSocket JSON Commands:");
  Serial.println("  Individual Relay Control:");
  Serial.println("    {\"relay_id\":1,\"action\":\"on\"}");
  Serial.println("    {\"relay_id\":2,\"action\":\"off\"}");
  Serial.println("    {\"relay_id\":3,\"action\":\"toggle\"}");
  Serial.println("    {\"relay_id\":4,\"action\":\"status\"}");
  Serial.println("  Global Control:");
  Serial.println("    {\"action\":\"all_on\"}");
  Serial.println("    {\"action\":\"all_off\"}");
  Serial.println("    {\"action\":\"status\"}");
  Serial.println("API Endpoints:");
  Serial.println("  - GET /api/relays - Get all relay status");
  Serial.println("  - GET /api/relay?id=N - Get specific relay status");
  Serial.println("  - POST /api/relay/control?id=N&action=on/off/toggle");
  Serial.println("  - POST /api/relays/all?action=on/off");
  Serial.print("WebSocket URL: ws://");
  Serial.print(WiFi.localIP());
  Serial.println("/ws");
  Serial.println("========================\n");
}

void loop()
{
  // Clean up WebSocket connections
  ws.cleanupClients();

  // Handle any pending tasks
  delay(10);

  // Optional: Print status every 30 seconds
  static unsigned long lastStatusPrint = 0;
  if (millis() - lastStatusPrint > 30000)
  {
    Serial.print("Status - Connected clients: ");
    Serial.print(ws.count());
    Serial.print(", Relays: ");
    for (int i = 0; i < NUM_RELAYS; i++)
    {
      if (i > 0)
        Serial.print(", ");
      Serial.print("R" + String(i + 1) + ":" + String(relays[i].state ? "ON" : "OFF"));
    }
    Serial.print(", Free heap: ");
    Serial.print(ESP.getFreeHeap());
    Serial.println(" bytes");
    lastStatusPrint = millis();
  }
}
