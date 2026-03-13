# M5Stack PIR Motion Sensor Unit — ESP-IDF Component for the Core2 for AWS IoT Kit

ESP-IDF component driver for the [M5Stack PIR motion sensor unit](https://docs.m5stack.com/en/unit/pir) (AS312-based) connected to **Port B** (GPIO 36) on the M5Stack Core2 for AWS IoT Kit. Uses the digital read abstractions built in to the [Core2 for AWS BSP](https://github.com/m5stack/Core2-for-AWS-IoT-Kit/tree/BSP-dev).

## How It Works

The PIR (Passive Infrared) unit contains an AS312 sensor that detects infrared radiation changes caused by moving warm objects (people, animals). It outputs a simple digital signal:

- **HIGH (1)** — motion detected
- **LOW (0)** — no motion detected

After detecting motion, the sensor holds its output HIGH for approximately **2 seconds** before returning LOW (if no further motion occurs). The detection range is approximately **3 meters** with a **100° cone** field of view.

## Hardware Connection

Plug the PIR unit into **Port B** (black Grove connector) on the Core2 for AWS IoT Kit:

| PIR Unit Pin | Core2 Port B Pin |
|-------------|------------------|
| Signal      | GPIO 36 (input-only) |
| VCC         | 5V               |
| GND         | GND              |

> **Note:** GPIO 36 is an input-only pin on the ESP32, which is ideal for reading sensor outputs.

## Usage

```c
#include "unit_pir.h"

// Initialize the PIR sensor
esp_err_t err = unit_pir_init();
if (err == ESP_OK) {
    bool motion = false;

    // Read current motion state
    err = unit_pir_read(&motion);
    if (err == ESP_OK && motion) {
        printf("Motion detected!\n");
    }
}

// Clean up when done
unit_pir_deinit();
```

## API Reference

### `unit_pir_init()`

Configures GPIO 36 as a digital input and verifies the pin can be read.

**Returns:** `ESP_OK` on success, `ESP_ERR_INVALID_STATE` if GPIO setup fails.

### `unit_pir_read(bool *motion_detected)`

Reads the current sensor state.

| Parameter | Description |
|-----------|-------------|
| `motion_detected` | Output — `true` if motion is detected, `false` otherwise |

**Returns:** `ESP_OK` on success, `ESP_ERR_INVALID_ARG` if pointer is NULL, `ESP_ERR_INVALID_STATE` if not initialized.

### `unit_pir_deinit()`

Resets the GPIO pin to its default state. Call this when the sensor is no longer needed.

**Returns:** `ESP_OK` on success.

## Integration Example

For applications that need to count motion events (edge detection), implement this in your application layer:

```c
void motion_monitoring_task(void *pvParameters) {
    uint32_t motion_count = 0;
    bool last_state = false;

    while (1) {
        bool current_motion = false;

        if (unit_pir_read(&current_motion) == ESP_OK) {
            // Detect rising edge (new motion event)
            if (current_motion && !last_state) {
                motion_count++;
                ESP_LOGI("MOTION", "Motion event #%lu", motion_count);
            }
            last_state = current_motion;
        }

        vTaskDelay(pdMS_TO_TICKS(100)); // Poll every 100ms
    }
}
```

## License

Apache 2.0 — see [LICENSE](LICENSE) for details.
