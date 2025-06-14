# M5Stack PIR Motion Sensor Unit ESP-IDF Component for the Core2 for AWS IoT

This is a component library for use with the M5Stack PIR motion sensor unit connected to Port B (GPIO 36) on the Core2 for AWS IoT Kit. Uses the digital read abstractions built in to the [BSP for the Core2 for AWS](https://github.com/m5stack/Core2-for-AWS-IoT-Kit/tree/BSP-dev).

For official documentation and specifications, see the [M5Stack PIR Unit documentation](https://docs.m5stack.com/en/unit/pir).

## Features

- Simple digital motion detection
- Direct GPIO reading without state management
- Thread-safe initialization and reading
- Minimal memory footprint
- Easy integration with existing applications

## Usage

```c
#include "unit_pir.h"

// Initialize the PIR sensor
esp_err_t err = unit_pir_init();
if (err == ESP_OK) {
    bool motion = false;
    
    // Read current motion state (returns immediate sensor reading)
    err = unit_pir_read(&motion);
    if (err == ESP_OK && motion) {
        printf("Motion detected!\n");
    }
}

// Clean up when done
unit_pir_deinit();
```

## API Reference

### Functions

- `unit_pir_init()` - Initialize the PIR sensor on GPIO 36
- `unit_pir_read(bool *motion_detected)` - Read current motion state
- `unit_pir_deinit()` - Deinitialize and reset GPIO

### Behavior

The sensor outputs:
- `true` (1) when motion is detected
- `false` (0) when no motion is detected

The PIR sensor has built-in timing logic that maintains a high output for approximately 2 seconds after detecting motion, then returns to low state if no further motion is detected.

## Hardware Connection

Connect the PIR sensor to Port B of the Core2 for AWS IoT Kit:
- Signal pin → GPIO 36 (Port B ADC pin)
- VCC → 5V (or 3.3V depending on sensor requirements)
- GND → Ground

## Integration Example

For applications requiring motion tracking or debouncing, implement this logic in your application layer:

```c
void motion_monitoring_task(void *pvParameters) {
    static uint32_t motion_count = 0;
    static bool last_state = false;
    
    while (1) {
        bool current_motion = false;
        
        if (unit_pir_read(&current_motion) == ESP_OK) {
            // Detect rising edge (new motion)
            if (current_motion && !last_state) {
                motion_count++;
                ESP_LOGI("MOTION", "Motion detected! Count: %lu", motion_count);
            }
            last_state = current_motion;
        }
        
        vTaskDelay(pdMS_TO_TICKS(100)); // Check every 100ms
    }
}
```

