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
 * @links [PIR Motion Sensor](https://docs.m5stack.com/en/unit/pir)
 * @version  V1.1.0
 * @date  2026-03-13
 */

#include "unit_pir.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <driver/gpio.h>
#include <esp_log.h>

static const char *TAG = "UNIT_PIR";

/* -- Static state --------------------------------------------------------- */

static bool               s_pir_initialized = false;
static unit_pir_callback_t s_callback        = NULL;
static QueueHandle_t      s_event_queue     = NULL;
static TaskHandle_t       s_event_task      = NULL;

/* -- Internal helpers ----------------------------------------------------- */

/**
 * ISR handler — called directly from the GPIO interrupt.
 * Kept minimal: reads the current pin level and posts it to a queue.
 * The actual user callback runs in the safe task context below.
 */
static void IRAM_ATTR pir_isr_handler( void *arg )
{
    bool level = ( bool )gpio_get_level( PIR_SENSOR_PIN );
    BaseType_t higher_priority_woken = pdFALSE;
    xQueueSendFromISR( s_event_queue, &level, &higher_priority_woken );
    portYIELD_FROM_ISR( higher_priority_woken );
}

/**
 * FreeRTOS task that drains the event queue and calls the user callback.
 * Running in task context means the callback can safely use ESP-IDF APIs
 * and log functions.
 */
static void pir_event_task( void *pvParameters )
{
    bool motion;
    for ( ;; )
    {
        if ( xQueueReceive( s_event_queue, &motion, portMAX_DELAY ) == pdTRUE )
        {
            if ( s_callback != NULL )
            {
                s_callback( motion );
            }
        }
    }
}

/* -- Public API ----------------------------------------------------------- */

esp_err_t unit_pir_init( void )
{
    if ( s_pir_initialized )
    {
        return ESP_OK;
    }

    ESP_LOGI( TAG, "Initializing PIR motion sensor on GPIO %d", PIR_SENSOR_PIN );

    /* A single read is enough to configure the pin as INPUT via the BSP and
     * verify the sensor is responsive. Subsequent reads fast-path through the
     * BSP's mode check and only call gpio_get_level(). */
    bool initial_state = false;
    esp_err_t err = core2foraws_expports_digital_read( PIR_SENSOR_PIN, &initial_state );
    if ( err != ESP_OK )
    {
        ESP_LOGE( TAG, "Failed to configure PIR sensor GPIO: %s", esp_err_to_name( err ) );
        return err;
    }

    s_pir_initialized = true;
    ESP_LOGI( TAG, "PIR sensor ready (current state: %s)",
              initial_state ? "motion" : "no motion" );

    return ESP_OK;
}

esp_err_t unit_pir_read( bool *motion_detected )
{
    if ( motion_detected == NULL )
    {
        ESP_LOGE( TAG, "motion_detected pointer is NULL" );
        return ESP_ERR_INVALID_ARG;
    }

    if ( !s_pir_initialized )
    {
        ESP_LOGE( TAG, "Sensor not initialized — call unit_pir_init() first" );
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = core2foraws_expports_digital_read( PIR_SENSOR_PIN, motion_detected );
    if ( err != ESP_OK )
    {
        ESP_LOGE( TAG, "PIR read failed: %s", esp_err_to_name( err ) );
        return ESP_FAIL;
    }

    ESP_LOGV( TAG, "PIR: %s", *motion_detected ? "motion" : "no motion" );
    return ESP_OK;
}

esp_err_t unit_pir_register_callback( unit_pir_callback_t callback )
{
    if ( !s_pir_initialized )
    {
        ESP_LOGE( TAG, "Sensor not initialized — call unit_pir_init() first" );
        return ESP_ERR_INVALID_STATE;
    }

    /* NULL acts as a convenient alias for unregister */
    if ( callback == NULL )
    {
        return unit_pir_unregister_callback();
    }

    /* Replace any previously registered callback cleanly */
    if ( s_callback != NULL )
    {
        unit_pir_unregister_callback();
    }

    /* Queue depth of 4 prevents missed quick bursts without wasting memory */
    s_event_queue = xQueueCreate( 4, sizeof( bool ) );
    if ( s_event_queue == NULL )
    {
        ESP_LOGE( TAG, "Failed to create PIR event queue (out of memory)" );
        return ESP_ERR_NO_MEM;
    }

    s_callback = callback;

    /* Pinned to core 1 to avoid contention with Wi-Fi/BT on core 0 */
    BaseType_t ret = xTaskCreatePinnedToCore(
        pir_event_task, "pir_event", 2048, NULL, 5, &s_event_task, 1 );
    if ( ret != pdPASS )
    {
        ESP_LOGE( TAG, "Failed to create PIR event task (out of memory)" );
        vQueueDelete( s_event_queue );
        s_event_queue = NULL;
        s_callback    = NULL;
        return ESP_ERR_NO_MEM;
    }

    /* Install the shared GPIO ISR service if it hasn't been installed yet.
     * ESP_ERR_INVALID_STATE means it is already installed — treat as success. */
    esp_err_t err = gpio_install_isr_service( 0 );
    if ( err != ESP_OK && err != ESP_ERR_INVALID_STATE )
    {
        ESP_LOGE( TAG, "Failed to install GPIO ISR service: %s", esp_err_to_name( err ) );
        goto cleanup;
    }

    /* Trigger on both edges: rising = motion starts, falling = motion stops */
    err = gpio_set_intr_type( PIR_SENSOR_PIN, GPIO_INTR_ANYEDGE );
    if ( err != ESP_OK )
    {
        ESP_LOGE( TAG, "Failed to set interrupt type: %s", esp_err_to_name( err ) );
        goto cleanup;
    }

    err = gpio_isr_handler_add( PIR_SENSOR_PIN, pir_isr_handler, NULL );
    if ( err != ESP_OK )
    {
        ESP_LOGE( TAG, "Failed to add GPIO ISR handler: %s", esp_err_to_name( err ) );
        gpio_set_intr_type( PIR_SENSOR_PIN, GPIO_INTR_DISABLE );
        goto cleanup;
    }

    ESP_LOGI( TAG, "PIR motion callback registered" );
    return ESP_OK;

cleanup:
    vTaskDelete( s_event_task );
    s_event_task = NULL;
    vQueueDelete( s_event_queue );
    s_event_queue = NULL;
    s_callback    = NULL;
    return err;
}

esp_err_t unit_pir_unregister_callback( void )
{
    if ( s_callback == NULL )
    {
        return ESP_OK;
    }

    gpio_isr_handler_remove( PIR_SENSOR_PIN );
    gpio_set_intr_type( PIR_SENSOR_PIN, GPIO_INTR_DISABLE );

    if ( s_event_task != NULL )
    {
        vTaskDelete( s_event_task );
        s_event_task = NULL;
    }

    if ( s_event_queue != NULL )
    {
        vQueueDelete( s_event_queue );
        s_event_queue = NULL;
    }

    s_callback = NULL;
    ESP_LOGI( TAG, "PIR motion callback unregistered" );
    return ESP_OK;
}

esp_err_t unit_pir_deinit( void )
{
    ESP_LOGI( TAG, "Deinitializing PIR sensor" );

    unit_pir_unregister_callback();

    esp_err_t err = core2foraws_expports_pin_reset( PIR_SENSOR_PIN );
    if ( err != ESP_OK )
    {
        ESP_LOGW( TAG, "Failed to reset PIR sensor GPIO: %s", esp_err_to_name( err ) );
    }

    s_pir_initialized = false;
    ESP_LOGI( TAG, "PIR sensor deinitialized" );
    return err;
}
