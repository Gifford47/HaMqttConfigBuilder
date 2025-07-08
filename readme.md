# HaMqttConfigBuilderLite

A lightweight, zero-dependency JSON builder for Home Assistant MQTT Discovery payloads.

**Features:**

- No `ArduinoJson` or STL containers
- Single pre-allocated buffer (`String`)
- Very low RAM usage (no heap fragmentation)
- Support for nested objects (device, availability, etc.)
- Efficient reuse of the `device` block for multiple sensor entities

---

## 🔧 Basic Usage

```cpp
#include "HaMqttConfigBuilder.h"

HaMqttConfigBuilder cfg;

// Step 1: Define the device block once
cfg.beginDevice()
   .add("name",  "Plant Sensor")
   .add("model", "v1.0")
   .add("sw",    "0.1")
.endDevice();

// Step 2: Add the first sensor
cfg.add("name",         "Soil Moisture")
   .add("state_topic",  "plant/sensor/soil")
   .add("unit_of_meas", "%")
   .add("device_class", "humidity");

Serial.println(cfg.generate());

// Step 3: Reuse device block, add a second sensor
cfg.nextSensor();
cfg.add("name",         "Plant Battery")
   .add("state_topic",  "plant/sensor/batt")
   .add("unit_of_meas", "%")
   .add("device_class", "battery");

Serial.println(cfg.generate());
```

---

## 📦 Output Example

```json
{
  "device": {
    "name": "Plant Sensor",
    "model": "v1.0",
    "sw": "0.1"
  },
  "name": "Soil Moisture",
  "state_topic": "plant/sensor/soil",
  "unit_of_meas": "%",
  "device_class": "humidity"
}
```

---

## 🔍 Extract Field Value

You can extract values from the generated JSON using `getString()`:

```cpp
String topic;
if (cfg.getString("state_topic", topic)) {
  Serial.println("Topic is: " + topic);
}
```

---

## 🧠 Memory Efficiency

- Internally uses a single `String` buffer
- Nested objects supported up to configurable max depth (default 6)
- Device block is cached via position pointer to avoid duplication

---

## 💡 Tip

Use `cfg.clear()` if you want to start over completely and redefine the device block.

---

## License

MIT

