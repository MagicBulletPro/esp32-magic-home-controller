# Quick Reference - Sensor Integration

## What's New? 🎉

### ✅ Robust Error Handling
- **3-strike rule**: Sensor marked disconnected after 3 consecutive failures
- **Auto-recovery**: Automatically reconnects when sensor comes back online
- **Clear messages**: Know exactly what's wrong with detailed error messages

### ✅ Automatic WebSocket Broadcasting
- **Every 10 seconds**: Temperature data pushed to all connected clients
- **No manual requests needed**: Clients receive updates automatically
- **Efficient**: Only broadcasts when clients are connected

### ✅ Enhanced Status Tracking
```json
{
  "temperature": 23.50,
  "humidity": 45.30,
  "temperature_f": 74.30,
  "valid": true,           // ← Reading is valid
  "connected": true,       // ← Sensor is connected
  "last_update": 12345,
  "error": ""             // ← Error message (if any)
}
```

## Testing Your Setup

### 1. Check Serial Monitor
```bash
pio device monitor
```

**Look for:**
```
✓ DHT sensor initialized successfully
  Initial reading - Temp: 23.50°C, Humidity: 45.30%
✓ Temperature: 23.50°C, Humidity: 45.30%
📡 Sensor data broadcasted to 1 client(s)
```

### 2. Test WebSocket
Open browser console:
```javascript
ws = new WebSocket('ws://YOUR_ESP32_IP/ws');
ws.onmessage = (e) => console.log(JSON.parse(e.data));
// Wait 10 seconds - you'll receive sensor data automatically!
```

### 3. Test Disconnection
1. Disconnect sensor DATA wire from GPIO 4
2. Wait 15 seconds
3. Check serial monitor:
```
⚠ Failed to read from DHT sensor (attempt 1/3)
⚠ Failed to read from DHT sensor (attempt 2/3)
⚠ Failed to read from DHT sensor (attempt 3/3)
⚠ DHT sensor disconnected or not responding!
```

4. Reconnect sensor
5. Wait 5 seconds - should auto-reconnect!

### 4. Check Web Interface
Navigate to `http://YOUR_ESP32_IP/`

**Sensor Connected:**
```
Temperature & Humidity:
✓ Sensor Connected
Temperature: 23.5°C (74.3°F)
Humidity: 45.3%
```

**Sensor Disconnected:**
```
Temperature & Humidity:
✗ Sensor Disconnected
Error: Sensor not connected or failed to read
Please check sensor wiring and power supply.
```

## Timing Configuration

| Event | Interval | Purpose |
|-------|----------|---------|
| Sensor Read | 5 seconds | Check temperature/humidity |
| WebSocket Broadcast | 10 seconds | Push data to clients |
| Status Log | 30 seconds | Print status to serial |

## Common Scenarios

### Scenario 1: Sensor Working Perfectly ✅
```
Every 5 seconds:  ✓ Temperature: 23.50°C, Humidity: 45.30%
Every 10 seconds: 📡 Sensor data broadcasted to X client(s)
Every 30 seconds: Status - ... Temp: 23.5°C, Humidity: 45.3%
```

### Scenario 2: Sensor Disconnected ⚠️
```
After 15 seconds (3 failures):
⚠ DHT sensor disconnected or not responding!
  Please check:
  - Sensor wiring (VCC, GND, DATA)
  - Power supply
  - Pull-up resistor (10kΩ recommended)

Every 30 seconds: Status - ... Sensor: DISCONNECTED
```

### Scenario 3: Sensor Reconnects 🔄
```
✓ Temperature: 23.50°C, Humidity: 45.30%
(Sensor automatically recovered)
```

## API Endpoints

### GET /api/sensor
Returns current sensor data with connection status

**Response (Connected):**
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

**Response (Disconnected):**
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

## WebSocket Commands

### Request Sensor Data (Manual)
```json
{"action": "sensor"}
```

### Automatic Updates
No action needed! Data arrives every 10 seconds automatically.

## Troubleshooting

| Problem | Solution |
|---------|----------|
| "Sensor disconnected" on startup | Check wiring: VCC→3.3V, GND→GND, DATA→GPIO4 |
| "Temporary read failure" | Normal - system will retry automatically |
| No WebSocket broadcasts | Check if clients are connected (`ws.count()`) |
| Wrong temperature values | Verify `DHTTYPE` matches your sensor (DHT11/DHT22) |
| Sensor disconnects randomly | Add/check 10kΩ pull-up resistor |

## Hardware Setup Reminder

```
DHT11/DHT22 Sensor    ESP32
------------------    -----
VCC (pin 1)      →    3.3V
DATA (pin 2)     →    GPIO 4
NC (pin 3)       →    (not connected)
GND (pin 4)      →    GND

Optional: 10kΩ resistor between VCC and DATA
```

## Code Changes Summary

### New Structure Fields
```cpp
struct SensorData {
  float temperature;
  float humidity;
  unsigned long lastUpdate;
  bool valid;           // ← NEW: Reading valid?
  bool connected;       // ← NEW: Sensor connected?
  int failedReadCount;  // ← NEW: Failure counter
  String errorMessage;  // ← NEW: Error description
};
```

### New Function
```cpp
void broadcastSensorData();  // Broadcasts to WebSocket clients
```

### Modified Functions
- `readSensorData()` - Enhanced with error tracking
- `getSensorDataJson()` - Includes connection status
- `setup()` - Improved initialization with retries
- `loop()` - Added automatic broadcasting

## Next Steps

1. **Upload the code**: `pio run -t upload`
2. **Monitor output**: `pio device monitor`
3. **Test web interface**: Open `http://ESP32_IP/`
4. **Test WebSocket**: Use browser console to connect
5. **Test disconnection**: Physically disconnect sensor
6. **Verify auto-recovery**: Reconnect sensor

## Documentation Files

- **TEMPERATURE_SENSOR.md** - Complete sensor setup guide
- **SENSOR_ERROR_HANDLING.md** - Detailed error handling documentation
- **INTEGRATION_SUMMARY.md** - Integration overview
- **README.md** - Updated with sensor features

---

**Status**: ✅ Ready to deploy!  
**Branch**: `feature/temperature-sensor-integration`  
**Commits**: 3 commits ahead of main
