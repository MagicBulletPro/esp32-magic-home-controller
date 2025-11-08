# Temperature Sensor Integration

## Overview
This document describes the DHT22 temperature and humidity sensor integration for the ESP32 Magic Home Controller, including robust error handling and automatic WebSocket broadcasting of temperature data.

## Hardware Requirements

### DHT22 Sensor (AM2302)
- Temperature range: -40°C to 80°C (±0.5°C accuracy)
- Humidity range: 0-100% RH (±2-5% accuracy)
- Operating voltage: 3.3V - 5V
- Digital output via single-wire interface

### Wiring Diagram
```
DHT22 Sensor    ESP32
-----------     -----
VCC       →     3.3V
DATA      →     GPIO 4
GND       →     GND
```

**Note:** A 10kΩ pull-up resistor is recommended between VCC and DATA pin (some DHT22 modules have this built-in).

## Software Implementation

### Dependencies
The following libraries are required (automatically installed via PlatformIO):
- `adafruit/DHT sensor library@^1.4.4`
- `adafruit/Adafruit Unified Sensor@^1.1.9`

### Configuration
The sensor is configured in `src/main.cpp`:
```cpp
#define DHTPIN 4          // GPIO pin connected to DHT sensor
#define DHTTYPE DHT22     // DHT22 (AM2302) sensor type
```

To use a different GPIO pin, modify the `DHTPIN` value.

### Supported Sensor Types
To use a different DHT sensor, change `DHTTYPE`:
- `DHT11` - Basic, lower accuracy sensor
- `DHT22` - Higher accuracy sensor (recommended)
- `DHT21` - AM2301 sensor

## API Endpoints

### GET /api/sensor
Returns current temperature and humidity readings.

**Response (Sensor Connected):**
```json
{
  "temperature": 23.50,
  "humidity": 45.30,
  "temperature_f": 74.30,
  "valid": true,
  "connected": true,
  "last_update": 12345678
}
```

**Response (Sensor Disconnected):**
```json
{
  "temperature": 0.00,
  "humidity": 0.00,
  "temperature_f": 32.00,
  "valid": false,
  "connected": false,
  "last_update": 12345678,
  "error": "Sensor not connected or failed to read"
}
```

### GET /info
Returns device information including sensor data.

**Response includes:**
```json
{
  "device_name": "ESP32_Controller",
  "sensor": {
    "temperature": 23.50,
    "humidity": 45.30,
    "temperature_f": 74.30,
    "valid": true,
    "last_update": 12345678
  },
  ...
}
```

## WebSocket Commands

### Get Sensor Data
Send via WebSocket to get current sensor readings:

```json
{"action": "sensor"}
```

**Alternative commands:**
```json
{"action": "get_sensor"}
{"action": "temperature"}
```

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

## Features

### Enhanced Error Detection
The system tracks sensor connection status with multiple indicators:

- **`connected`**: Boolean flag indicating if sensor is responding
- **`valid`**: Boolean flag indicating if last reading was successful
- **`failedReadCount`**: Counter for consecutive failed reads
- **`errorMessage`**: Descriptive error message for troubleshooting

### Sensor Disconnection Handling

#### Automatic Detection
- After **3 consecutive failed reads**, sensor is marked as disconnected
- System continues operating normally without sensor data
- Clear error messages help with troubleshooting
- Automatic reconnection when sensor comes back online

### Automatic Reading
- Sensor data is automatically read every 5 seconds
- Data is cached to reduce sensor queries
- Invalid readings are detected and reported

### Automatic WebSocket Broadcasting
- **Sensor reading**: Every 5 seconds
- **WebSocket broadcast**: Every 10 seconds (when clients connected)
- **Status logging**: Every 30 seconds
- Real-time sensor data pushed to all connected WebSocket clients

#### Broadcasting Implementation
```cpp
// Broadcasts to all connected WebSocket clients
void broadcastSensorData() {
  if (ws.count() > 0) {
    String sensorJson = getSensorDataJson();
    ws.textAll(sensorJson);
    Serial.println("📡 Sensor data broadcasted to " + String(ws.count()) + " client(s)");
  }
}
```

### Status Reporting
- Status messages every 30 seconds include temperature and humidity
- Web interface displays real-time sensor data with connection status
- Both Celsius and Fahrenheit temperatures provided

### Enhanced Web Interface
The homepage shows clear sensor status:

**Connected:**
```
Temperature & Humidity:
✓ Sensor Connected
Temperature: 23.5°C (74.3°F)
Humidity: 45.3%
```

**Disconnected:**
```
Temperature & Humidity:
✗ Sensor Disconnected
Error: Sensor not connected or failed to read
Please check sensor wiring and power supply.
```

## Testing

### Using cURL
```bash
# Get sensor data
curl http://ESP32_IP/api/sensor

# Get full device info including sensor
curl http://ESP32_IP/info
```

### Using WebSocket

#### Manual Request
You can test WebSocket commands using any WebSocket client:
```javascript
ws = new WebSocket("ws://ESP32_IP/ws");
ws.onmessage = (event) => console.log(event.data);
ws.send('{"action":"sensor"}');
```

#### Automatic Updates
Clients will receive sensor data every 10 seconds automatically:

```javascript
const ws = new WebSocket('ws://ESP32_IP/ws');

ws.onmessage = function(event) {
  const data = JSON.parse(event.data);
  
  // Check if it's sensor data
  if (data.hasOwnProperty('temperature')) {
    if (data.connected && data.valid) {
      console.log(`Temperature: ${data.temperature}°C`);
      console.log(`Humidity: ${data.humidity}%`);
    } else {
      console.error('Sensor error:', data.error);
    }
  }
};

// You can also request sensor data manually
ws.send('{"action":"sensor"}');
```

#### React/Vue Example

```javascript
// State management
const [sensorData, setSensorData] = useState({
  temperature: 0,
  humidity: 0,
  connected: false,
  valid: false,
  error: ''
});

// WebSocket handler
ws.onmessage = (event) => {
  const data = JSON.parse(event.data);
  if (data.hasOwnProperty('temperature')) {
    setSensorData(data);
  }
};

// Display in UI
{sensorData.connected ? (
  <div>
    <p>Temperature: {sensorData.temperature}°C</p>
    <p>Humidity: {sensorData.humidity}%</p>
  </div>
) : (
  <div className="error">
    <p>Sensor Disconnected</p>
    <p>{sensorData.error}</p>
  </div>
)}
```

### Test Sensor Disconnection
```bash
# Upload code and monitor
pio run -t upload && pio device monitor

# Physically disconnect sensor DATA wire
# Wait 15 seconds (3 read attempts × 5 seconds)
# Check serial output for disconnection message

# Reconnect sensor
# Wait 5 seconds for next read attempt
# Check serial output for successful reconnection
```

### Serial Monitor
Monitor the Serial output (115200 baud) to see:
- Sensor initialization status
- Real-time temperature and humidity readings
- Status updates every 30 seconds

#### Serial Output Examples

**Sensor Connected:**
```
Initializing DHT sensor...
✓ Temperature: 23.50°C, Humidity: 45.30%
✓ DHT sensor initialized successfully
  Initial reading - Temp: 23.50°C, Humidity: 45.30%

✓ Temperature: 23.50°C, Humidity: 45.30%
📡 Sensor data broadcasted to 2 client(s)
```

**Sensor Disconnected:**
```
Initializing DHT sensor...
⚠ Failed to read from DHT sensor (attempt 1/3)
  Retry 2/3...
⚠ Failed to read from DHT sensor (attempt 2/3)
  Retry 3/3...
⚠ Failed to read from DHT sensor (attempt 3/3)
⚠ DHT sensor not detected!
  System will continue without sensor data.
  Sensor readings will show 'disconnected' status.
```

**Status Messages (every 30 seconds):**
```
// Sensor connected
Status - Connected clients: 2, Relays: R1:OFF, R2:ON, Temp: 23.5°C, Humidity: 45.3%, Free heap: 245632 bytes

// Sensor disconnected
Status - Connected clients: 2, Relays: R1:OFF, R2:ON, Sensor: DISCONNECTED, Free heap: 245632 bytes
```

## Troubleshooting

### Error: "Sensor not connected or failed to read"

**Check List:**
1. ✓ Verify sensor is connected to GPIO 4
2. ✓ Check VCC is connected to 3.3V or 5V
3. ✓ Check GND is properly connected
4. ✓ Ensure 10kΩ pull-up resistor between VCC and DATA
5. ✓ Try a different GPIO pin (update `DHTPIN` in code)
6. ✓ Verify sensor is DHT11 or DHT22 (update `DHTTYPE`)
7. ✓ Check for damaged sensor

### Error: "Temporary read failure, retrying..."

**This is normal** - DHT sensors occasionally have transient read failures.
- System will retry automatically
- If it fails 3 times, sensor marked as disconnected
- Sensor will attempt reconnection on next read cycle

### Sensor Not Reading
1. **Check wiring:** Ensure VCC, GND, and DATA are properly connected
2. **Pull-up resistor:** Add a 10kΩ resistor between VCC and DATA if not built-in
3. **Power supply:** Ensure stable 3.3V or 5V power
4. **Pin configuration:** Verify `DHTPIN` matches your wiring

### Sensor Was Working, Now Disconnected

**Possible causes:**
- Loose wiring connection
- Power supply issue
- Sensor overheated
- Physical damage to sensor

**Action:**
- Check serial monitor for specific error messages
- Verify wiring is secure
- Restart ESP32
- Try replacing sensor if issue persists

### Invalid Readings
- Wait 2 seconds after power-on for sensor to stabilize
- DHT22 has a minimum 2-second sampling period
- Check for loose connections or damaged sensor

### Compilation Errors
Ensure PlatformIO has installed the required libraries:
```bash
pio lib install
```

## Configuration Options

### Adjust Timing Intervals

In the `loop()` function in `src/main.cpp`:

```cpp
// Sensor read interval (default: 5000ms = 5 seconds)
if (millis() - lastSensorRead > 5000)

// Broadcast interval (default: 10000ms = 10 seconds)
if (millis() - lastSensorBroadcast > 10000)

// Status print interval (default: 30000ms = 30 seconds)
if (millis() - lastStatusPrint > 30000)
```

### Adjust Failure Threshold

In `readSensorData()` function:

```cpp
// Change from 3 to your desired number of failures
if (sensorData.failedReadCount >= 3) {
```

## Performance Impact

### Memory Usage
- Additional ~200 bytes for error tracking
- Negligible impact on overall memory

### CPU Usage
- Broadcast function: < 1ms execution time
- Only broadcasts if clients connected
- No performance impact on relay operations

### Network Usage
- 10-second broadcast interval prevents network flooding
- ~150 bytes per broadcast message
- Bandwidth: ~15 bytes/second per client (minimal)

## Best Practices

### For Production Use
1. **Monitor sensor health**: Track `failedReadCount` over time
2. **Alert on disconnection**: Send notification when sensor disconnects
3. **Log sensor data**: Store readings for historical analysis
4. **Validate ranges**: Check if temperature/humidity are within expected ranges

### For Development
1. **Use serial monitor**: Keep it open to see real-time sensor status
2. **Test both states**: Verify behavior with sensor connected and disconnected
3. **Check WebSocket**: Use browser dev tools to monitor broadcasts
4. **Simulate failures**: Temporarily disconnect sensor to test error handling

## Future Enhancements

Potential improvements for temperature sensor integration:
- [ ] Add sensor reconnection notification via WebSocket
- [ ] Implement exponential backoff for failed reads
- [ ] Add temperature-based automation rules
- [ ] Implement temperature thresholds with alerts
- [ ] Add historical data logging
- [ ] Store sensor error history
- [ ] Add sensor diagnostics endpoint
- [ ] Support for multiple temperature sensors
- [ ] Integration with weather APIs for outdoor comparison
- [ ] Automatic relay control based on temperature
- [ ] Data export functionality (CSV, JSON)
- [ ] Add data smoothing/averaging

## Pin Usage Summary

| Component | GPIO Pin | Notes |
|-----------|----------|-------|
| DHT22 Data | GPIO 4 | Can be changed in code |
| Relay 1 | GPIO 18 | Living Light |
| Relay 2 | GPIO 19 | Bedroom Light |
| Status LED | GPIO 2 | Built-in LED |

## References

- [DHT22 Datasheet](https://www.sparkfun.com/datasheets/Sensors/Temperature/DHT22.pdf)
- [Adafruit DHT Library](https://github.com/adafruit/DHT-sensor-library)
- [ESP32 GPIO Reference](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/gpio.html)

## Summary

The temperature sensor integration provides:

✅ **Robust disconnection detection** - 3-strike rule for failure detection  
✅ **Automatic WebSocket broadcasting** - Real-time updates every 10 seconds  
✅ **Clear error messages** - For easy troubleshooting  
✅ **Graceful degradation** - System works without sensor  
✅ **Status monitoring** - Connection state always available  
✅ **Improved UX** - Web interface shows connection status  
✅ **Auto-recovery** - Automatically reconnects when sensor comes back online  

The system is production-ready with professional error handling and real-time data streaming!
