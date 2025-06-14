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

#include <string.h>
#include <esp_log.h>
#include "unit_pir.h"

static const char *TAG = "UNIT_PIR";

// Simple initialization state
static bool g_pir_initialized = false;

esp_err_t unit_pir_init(void)
{
    ESP_LOGI(TAG, "Initializing PIR motion sensor on GPIO %d", PIR_SENSOR_PIN);
    
    // Test initial GPIO read to ensure it's working
    bool test_reading = false;
    esp_err_t err = core2foraws_expports_digital_read(PIR_SENSOR_PIN, &test_reading);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read from PIR sensor GPIO: %s", esp_err_to_name(err));
        return ESP_ERR_INVALID_STATE;
    }
    
    g_pir_initialized = true;
    
    ESP_LOGI(TAG, "PIR sensor initialized successfully (initial reading: %s)", 
             test_reading ? "motion" : "no motion");
    
    return ESP_OK;
}

esp_err_t unit_pir_read(bool *motion_detected)
{
    if (motion_detected == NULL) {
        ESP_LOGE(TAG, "Invalid parameter: motion_detected is NULL");
        return ESP_ERR_INVALID_ARG;
    }
    
    if (!g_pir_initialized) {
        ESP_LOGE(TAG, "PIR sensor not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    // Read current sensor value
    esp_err_t err = core2foraws_expports_digital_read(PIR_SENSOR_PIN, motion_detected);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read PIR sensor: %s", esp_err_to_name(err));
        return ESP_FAIL;
    }
    
    ESP_LOGV(TAG, "PIR read: %s", *motion_detected ? "motion" : "no motion");
    
    return ESP_OK;
}

esp_err_t unit_pir_deinit(void)
{
    ESP_LOGI(TAG, "Deinitializing PIR sensor");
    
    // Reset GPIO pin to default state
    esp_err_t err = core2foraws_expports_pin_reset(PIR_SENSOR_PIN);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to reset PIR sensor GPIO: %s", esp_err_to_name(err));
    }
    
    // Clear initialization state
    g_pir_initialized = false;
    
    ESP_LOGI(TAG, "PIR sensor deinitialized");
    
    return ESP_OK;
}
