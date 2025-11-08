# ESP32 Magic Home Controller

A powerful ESP32-based home automation controller that provides WebSocket and REST API interfaces for controlling multiple relays. Perfect for smart home applications, lighting control, and IoT projects.

## Features

- **Multi-Relay Control**: Support for multiple relays with individual control
- **Temperature & Humidity Monitoring**: DHT22 sensor integration for environmental monitoring
- **WebSocket Interface**: Real-time bidirectional communication
- **REST API**: Standard HTTP endpoints for easy integration
- **JSON Protocol**: Modern, structured command format
- **mDNS Discovery**: Automatic device discovery on local network
- **Status Monitoring**: Real-time relay and sensor status reporting
- **CORS Enabled**: Ready for web and mobile app integration

## Hardware Configuration

### Default Relay Setup
- **Relay 1**: GPIO 18 - "Living Light" (Living room main lighting)
- **Relay 2**: GPIO 19 - "Bedroom Light" (Master bedroom tube light)
- **Status LED**: GPIO 2 - Built-in LED for connection status

### Temperature Sensor Setup
- **DHT22 Sensor**: GPIO 4 - Temperature and humidity monitoring
- See [TEMPERATURE_SENSOR.md](TEMPERATURE_SENSOR.md) for detailed setup instructions

### Wi-Fi Configuration
Update the credentials in `main.cpp`:
```cpp
const char *ssid = "Your-WiFi-SSID";
const char *password = "Your-WiFi-Password";
```

## Getting Started

### Prerequisites
- PlatformIO IDE or CLI
- ESP32 development board
- Relay modules or LEDs for testing

### Installation
1. Clone or download this project
2. Open in PlatformIO
3. Update Wi-Fi credentials in `src/main.cpp`
4. Build and upload to ESP32
5. Monitor serial output for IP address

### Quick Test
Once uploaded and connected to Wi-Fi, the device will print its IP address. You can:
- Access web interface: `http://[ESP32_IP]/`
- View device info: `http://[ESP32_IP]/info`
- Connect WebSocket: `ws://[ESP32_IP]/ws`

## API Documentation

### WebSocket API

Connect to: `ws://[ESP32_IP]/ws`

#### Command Format
All WebSocket commands use JSON format:

#### Individual Relay Control
```json
{
  "relay_id": 1,
  "action": "on"
}
```

**Parameters:**
- `relay_id`: Integer (1-N, where N is number of relays)
- `action`: String - "on", "off", "toggle", "status"

**Response:**
```json
{
  "status": "success",
  "relay_id": 1,
  "action": "on",
  "state": true
}
```

#### Global Relay Control
```json
{
  "action": "all_on"
}
```

**Actions:**
- `all_on`: Turn on all relays
- `all_off`: Turn off all relays
- `status`: Get status of all relays

**Response:**
```json
{
  "status": "success",
  "action": "all_on",
  "message": "All relays turned ON"
}
```

#### Status Query
```json
{
  "action": "status"
}
```

**Response:**
```json
{
  "relays": [
    {
      "id": 1,
      "name": "Living Light",
      "description": "Living room main lighting",
      "pin": 18,
      "state": false
    },
    {
      "id": 2,
      "name": "Bedroom Light",
      "description": "Master bedroom tube light",
      "pin": 19,
      "state": true
    }
  ]
}
```

#### Sensor Data Query
```json
{
  "action": "sensor"
}
```

**Alternative commands:** `"get_sensor"`, `"temperature"`

**Response:**
```json
{
  "temperature": 23.50,
  "humidity": 45.30,
  "temperature_f": 74.30,
  "valid": true,
  "last_update": 12345678
}
```
  ]
}
```

### REST API

Base URL: `http://[ESP32_IP]`

#### Device Information

##### GET /
Returns HTML interface with device status

##### GET /info
Returns device information and relay status

**Response:**
```json
{
  "device_name": "ESP32 Controller",
  "device_type": "home_automation",
  "ip_address": "192.168.1.100",
  "mac_address": "AA:BB:CC:DD:EE:FF",
  "num_relays": 2,
  "sensor": {
    "temperature": 23.50,
    "humidity": 45.30,
    "temperature_f": 74.30,
    "valid": true,
    "last_update": 12345678
  },
  "relays": [
    {
      "id": 1,
      "name": "Living Light",
      "description": "Living room main lighting",
      "pin": 18,
      "state": false
    }
  ]
}
```

#### Sensor Data

##### GET /api/sensor
Get current temperature and humidity readings

**Response:**
```json
{
  "temperature": 23.50,
  "humidity": 45.30,
  "temperature_f": 74.30,
  "valid": true,
  "last_update": 12345678
}
```

**Fields:**
- `temperature`: Temperature in Celsius
- `humidity`: Relative humidity percentage
- `temperature_f`: Temperature in Fahrenheit
- `valid`: Whether the reading is valid
- `last_update`: Timestamp of last sensor read (milliseconds)

#### Relay Management

##### GET /api/relays
Get status of all relays

**Response:**
```json
{
  "relays": [
    {
      "id": 1,
      "name": "Living Light",
      "description": "Living room main lighting",
      "pin": 18,
      "state": false
    },
    {
      "id": 2,
      "name": "Bedroom Light",
      "description": "Master bedroom tube light",
      "pin": 19,
      "state": true
    }
  ]
}
```

##### GET /api/relay?id={relay_id}
Get status of specific relay

**Parameters:**
- `id`: Relay ID (1-N)

**Response:**
```json
{
  "id": 1,
  "name": "Living Light",
  "description": "Living room main lighting",
  "pin": 18,
  "state": false
}
```

##### POST /api/relay/control
Control individual relay

**Parameters:**
- `id`: Relay ID (1-N)
- `action`: "on", "off", or "toggle"

**Example:**
```
POST /api/relay/control?id=1&action=on
```

**Response:**
```json
{
  "success": true,
  "relay": 1,
  "state": true
}
```

##### POST /api/relays/all
Control all relays

**Parameters:**
- `action`: "on" or "off"

**Example:**
```
POST /api/relays/all?action=on
```

**Response:**
```json
{
  "success": true,
  "message": "All relays turned ON"
}
```

## Error Handling

### WebSocket Errors
```json
{
  "status": "error",
  "message": "Invalid relay_id. Must be between 1 and 2"
}
```

### HTTP Errors
```json
{
  "error": "Invalid relay ID. Must be between 1 and 2"
}
```

**Common Error Codes:**
- `400`: Bad Request (invalid parameters)
- `404`: Not Found (invalid endpoint)

## Integration Examples

### JavaScript (Web)
```javascript
// WebSocket connection
const ws = new WebSocket('ws://192.168.1.100/ws');

// Turn on relay 1
ws.send(JSON.stringify({
  relay_id: 1,
  action: "on"
}));

// Get status
ws.send(JSON.stringify({
  action: "status"
}));

// Handle responses
ws.onmessage = function(event) {
  const data = JSON.parse(event.data);
  console.log('Response:', data);
};
```

### Python
```python
import requests
import json

# Device IP
BASE_URL = "http://192.168.1.100"

# Turn on relay 1
response = requests.post(f"{BASE_URL}/api/relay/control", 
                        params={"id": 1, "action": "on"})
print(response.json())

# Get all relay status
response = requests.get(f"{BASE_URL}/api/relays")
print(response.json())

# Turn off all relays
response = requests.post(f"{BASE_URL}/api/relays/all", 
                        params={"action": "off"})
print(response.json())
```

### cURL
```bash
# Get device info
curl http://192.168.1.100/info

# Turn on relay 1
curl -X POST "http://192.168.1.100/api/relay/control?id=1&action=on"

# Get relay status
curl http://192.168.1.100/api/relays

# Turn off all relays
curl -X POST "http://192.168.1.100/api/relays/all?action=off"
```

## Customization

### Adding More Relays
1. Update `NUM_RELAYS` constant
2. Add relay configurations to the `relays[]` array:
```cpp
const int NUM_RELAYS = 4;
Relay relays[NUM_RELAYS] = {
    {18, false, "Living Light", "Living room main lighting"},
    {19, false, "Bedroom Light", "Master bedroom tube light"},
    {21, false, "Kitchen Light", "Kitchen lighting"},
    {22, false, "Office Light", "Office lighting"},
};
```

### Changing Device Name
```cpp
const char *deviceName = "My ESP32 Controller";
```

### Modifying GPIO Pins
Update the `pin` values in the relay configuration array.

## Network Discovery

The device supports mDNS for automatic discovery:
- Device name: `ESP32_Controller.local`
- Services: HTTP (port 80), WebSocket (port 80)

## Troubleshooting

### Common Issues

1. **Device not connecting to Wi-Fi**
   - Check SSID and password
   - Ensure 2.4GHz network (ESP32 doesn't support 5GHz)
   - Check signal strength

2. **Cannot access web interface**
   - Verify IP address from serial monitor
   - Check firewall settings
   - Ensure device and client on same network

3. **WebSocket connection fails**
   - Verify WebSocket URL format: `ws://IP/ws`
   - Check for proxy or firewall blocking
   - Use browser developer tools to debug

4. **Relays not responding**
   - Check GPIO pin connections
   - Verify relay module power supply
   - Test with multimeter for voltage output

### Debug Information

Enable serial monitoring (115200 baud) to see:
- Connection status
- Command processing
- Error messages
- System status updates

## Development

### Dependencies
- WiFi (ESP32 core)
- AsyncTCP
- ESPAsyncWebServer
- ArduinoJson
- ESPmDNS

### Building
```bash
# Using PlatformIO CLI
pio run

# Upload to device
pio run --target upload

# Monitor serial output
pio device monitor
```

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
