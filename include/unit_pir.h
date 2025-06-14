/*!
 * @brief Library for the PIR motion sensor unit on Port B of the Core2 for AWS IoT Kit
 * @copyright Copyright (c) 2025 by Rashed Talukder[https://rashedtalukder.com]
 *  
 * @license SPDX-License-Identifier: Apache 2.0
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * 
 * @Links [PIR Motion Sensor](https://docs.m5stack.com/en/unit/pir)
 * @version  V1.0.0
 * @date  2025-01-25
 */

#ifndef _UNIT_PIR_H_
#define _UNIT_PIR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <esp_err.h>
#include "core2foraws.h"

// PIR sensor configuration
#define PIR_SENSOR_PIN              PORT_B_ADC_PIN    // GPIO 36

// PIR sensor states
typedef enum {
    PIR_STATE_NO_MOTION = 0,    // No motion detected
    PIR_STATE_MOTION_DETECTED,  // Motion currently detected
    PIR_STATE_ERROR             // Sensor read error
} pir_state_t;

/** 
 * @brief Initialize the PIR motion sensor.
 * 
 * Configures GPIO 36 (Port B) for digital input to read PIR sensor values.
 * 
 * @return [esp_err_t](https://docs.espressif.com/projects/esp-idf/en/release-v4.3/esp32/api-reference/system/esp_err.html#macros).
 *  - ESP_OK                : Success
 *  - ESP_ERR_INVALID_STATE : GPIO configuration failed
 */
esp_err_t unit_pir_init(void);

/** 
 * @brief Read the current motion detection state from the PIR sensor.
 * 
 * Reads the digital value from GPIO 36. Returns true if motion is detected,
 * false if no motion is detected.
 * 
 * @param[out] motion_detected Pointer to store the motion detection state (true = motion, false = no motion)
 * @return [esp_err_t](https://docs.espressif.com/projects/esp-idf/en/release-v4.3/esp32/api-reference/system/esp_err.html#macros).
 *  - ESP_OK                : Success
 *  - ESP_ERR_INVALID_ARG   : Invalid parameter (motion_detected is NULL)
 *  - ESP_ERR_INVALID_STATE : Sensor not initialized
 *  - ESP_FAIL              : GPIO read error
 */
esp_err_t unit_pir_read(bool *motion_detected);

/**
 * @brief Deinitialize the PIR sensor.
 * 
 * Resets the GPIO pin configuration. Should be called when the sensor 
 * is no longer needed.
 * 
 * @return [esp_err_t](https://docs.espressif.com/projects/esp-idf/en/release-v4.3/esp32/api-reference/system/esp_err.html#macros).
 *  - ESP_OK : Success
 */
esp_err_t unit_pir_deinit(void);

#ifdef __cplusplus
}
#endif
#endif
