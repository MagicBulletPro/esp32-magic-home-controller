# Temperature Sensor Integration

## Overview
This document describes the DHT22 temperature and humidity sensor integration for the ESP32 Magic Home Controller.

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

### Automatic Reading
- Sensor data is automatically read every 5 seconds
- Data is cached to reduce sensor queries
- Invalid readings are detected and reported

### Status Reporting
- Status messages every 30 seconds include temperature and humidity
- Web interface displays real-time sensor data
- Both Celsius and Fahrenheit temperatures provided

### Error Handling
- Gracefully handles sensor disconnection or failures
- `valid` flag indicates data reliability
- Warning messages logged when sensor fails to read

## Testing

### Using cURL
```bash
# Get sensor data
curl http://ESP32_IP/api/sensor

# Get full device info including sensor
curl http://ESP32_IP/info
```

### Using WebSocket
You can test WebSocket commands using any WebSocket client:
```javascript
ws = new WebSocket("ws://ESP32_IP/ws");
ws.onmessage = (event) => console.log(event.data);
ws.send('{"action":"sensor"}');
```

### Serial Monitor
Monitor the Serial output (115200 baud) to see:
- Sensor initialization status
- Real-time temperature and humidity readings
- Status updates every 30 seconds

## Troubleshooting

### Sensor Not Reading
1. **Check wiring:** Ensure VCC, GND, and DATA are properly connected
2. **Pull-up resistor:** Add a 10kΩ resistor between VCC and DATA if not built-in
3. **Power supply:** Ensure stable 3.3V or 5V power
4. **Pin configuration:** Verify `DHTPIN` matches your wiring

### Invalid Readings
- Wait 2 seconds after power-on for sensor to stabilize
- DHT22 has a minimum 2-second sampling period
- Check for loose connections or damaged sensor

### Compilation Errors
Ensure PlatformIO has installed the required libraries:
```bash
pio lib install
```

## Future Enhancements

Potential improvements for temperature sensor integration:
- [ ] Add temperature-based automation rules
- [ ] Implement temperature thresholds with alerts
- [ ] Add historical data logging
- [ ] Support for multiple temperature sensors
- [ ] Integration with weather APIs for outdoor comparison
- [ ] Automatic relay control based on temperature
- [ ] Data export functionality (CSV, JSON)

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
