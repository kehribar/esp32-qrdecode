// ----------------------------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#include <stdio.h>
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_smartconfig.h"
#include <string.h>
#include "nvs.h"
#include "wifi.h"
#include "visionCamera.h"