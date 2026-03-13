/*!
 * @brief PIR motion sensor library for Port B of the M5Stack Core2 for AWS IoT Kit.
 *
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
 * Supports two usage patterns:
 *
 * **Polling** — check for motion on demand (e.g. inside a task loop):
 * @code{c}
 *   #include "unit_pir.h"
 *
 *   void pir_poll_task( void *pvParameters )
 *   {
 *       unit_pir_init();
 *       bool motion = false;
 *       for ( ;; )
 *       {
 *           unit_pir_read( &motion );
 *           ESP_LOGI( "PIR", "Motion: %s", motion ? "YES" : "no" );
 *           vTaskDelay( pdMS_TO_TICKS( 500 ) );
 *       }
 *   }
 * @endcode
 *
 * **Interrupt-driven (callback)** — get notified the instant motion changes
 * without polling. The callback runs in a safe FreeRTOS task context:
 * @code{c}
 *   #include "unit_pir.h"
 *
 *   void on_motion( bool motion_detected )
 *   {
 *       ESP_LOGI( "PIR", "Motion %s", motion_detected ? "started!" : "stopped." );
 *   }
 *
 *   void app_main( void )
 *   {
 *       core2foraws_init();
 *       unit_pir_init();
 *       unit_pir_register_callback( on_motion );
 *       // on_motion() will now be called automatically whenever motion changes.
 *   }
 * @endcode
 *
 * @links [PIR Motion Sensor](https://docs.m5stack.com/en/unit/pir)
 * @version  V1.1.0
 * @date  2026-03-13
 */

#ifndef _UNIT_PIR_H_
#define _UNIT_PIR_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "core2foraws.h"
#include <esp_err.h>
#include <stdbool.h>

/**
 * @brief GPIO pin used by the PIR sensor (Port B on the Core2 for AWS IoT Kit).
 *
 * This is GPIO 36, an input-only pin with no internal pull resistor.
 * The BSP automatically configures it for digital input on first use.
 */
#define PIR_SENSOR_PIN PORT_B_ADC_PIN /* GPIO 36 */

/**
 * @brief Callback function type for PIR motion events.
 *
 * Your function with this signature will be called from a dedicated
 * FreeRTOS task (not from an ISR), so it is safe to use ESP-IDF APIs,
 * log messages, and most FreeRTOS functions inside it.
 *
 * @param[in] motion_detected true when motion starts, false when motion stops.
 *
 * Example:
 * @code{c}
 *   void my_motion_handler( bool motion_detected )
 *   {
 *       if ( motion_detected )
 *           ESP_LOGI( "APP", "Motion detected!" );
 *       else
 *           ESP_LOGI( "APP", "Motion stopped." );
 *   }
 * @endcode
 */
typedef void ( *unit_pir_callback_t )( bool motion_detected );

  /**
   * @brief Initialize the PIR motion sensor.
   *
   * Configures GPIO 36 (Port B) as a digital input and validates that the
   * sensor can be read. Call this once before unit_pir_read() or
   * unit_pir_register_callback(). Calling it again while already initialized
   * returns ESP_OK immediately.
   *
   * @return
   * [esp_err_t](https://docs.espressif.com/projects/esp-idf/en/release-v4.3/esp32/api-reference/system/esp_err.html#macros).
   *  - ESP_OK   : Success
   *  - ESP_FAIL : GPIO configuration or initial read failed
   */
  esp_err_t unit_pir_init( void );

  /**
   * @brief Read the current motion detection state (polling).
   *
   * Call this inside a loop to check whether the sensor currently detects
   * motion. For event-driven detection without polling, use
   * unit_pir_register_callback() instead.
   *
   * @param[out] motion_detected Set to true if motion is detected right now,
   *             false if no motion.
   * @return
   * [esp_err_t](https://docs.espressif.com/projects/esp-idf/en/release-v4.3/esp32/api-reference/system/esp_err.html#macros).
   *  - ESP_OK                : Success
   *  - ESP_ERR_INVALID_ARG   : motion_detected pointer is NULL
   *  - ESP_ERR_INVALID_STATE : Sensor not initialized — call unit_pir_init() first
   *  - ESP_FAIL              : GPIO read error
   */
  esp_err_t unit_pir_read( bool *motion_detected );

  /**
   * @brief Register a callback for interrupt-driven motion detection.
   *
   * After calling this, your callback will be invoked automatically whenever
   * the sensor's output changes (motion starts or stops). This avoids the
   * need to poll in a loop.
   *
   * Only one callback can be registered at a time. Calling this a second time
   * replaces the previous callback. Pass NULL to disable (same as calling
   * unit_pir_unregister_callback()).
   *
   * Must be called after unit_pir_init().
   *
   * @param[in] callback Function to call on motion change events.
   * @return
   * [esp_err_t](https://docs.espressif.com/projects/esp-idf/en/release-v4.3/esp32/api-reference/system/esp_err.html#macros).
   *  - ESP_OK                : Success
   *  - ESP_ERR_INVALID_STATE : Sensor not initialized
   *  - ESP_ERR_NO_MEM        : Could not allocate FreeRTOS queue or task
   *  - Other                 : GPIO interrupt configuration failed
   */
  esp_err_t unit_pir_register_callback( unit_pir_callback_t callback );

  /**
   * @brief Unregister the motion callback and disable interrupts.
   *
   * Stops interrupt-driven motion detection. Polling via unit_pir_read()
   * continues to work normally after this call.
   *
   * Safe to call even if no callback was registered.
   *
   * @return
   * [esp_err_t](https://docs.espressif.com/projects/esp-idf/en/release-v4.3/esp32/api-reference/system/esp_err.html#macros).
   *  - ESP_OK : Success
   */
  esp_err_t unit_pir_unregister_callback( void );

  /**
   * @brief Deinitialize the PIR sensor and release all resources.
   *
   * Stops any active callback, resets the GPIO pin, and frees internal
   * resources. Call this when the sensor is no longer needed.
   *
   * @return
   * [esp_err_t](https://docs.espressif.com/projects/esp-idf/en/release-v4.3/esp32/api-reference/system/esp_err.html#macros).
   *  - ESP_OK : Success
   */
  esp_err_t unit_pir_deinit( void );

#ifdef __cplusplus
}
#endif
#endif /* _UNIT_PIR_H_ */
