// ----------------------------------------------------------------------------
// 
// 
// ----------------------------------------------------------------------------
#include "visionCamera.h"
#include "esp_camera.h"
#include "esp_timer.h"
#include "esp_log.h"

// ----------------------------------------------------------------------------
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ----------------------------------------------------------------------------
#define CAM_PIN_PWDN    -1
#define CAM_PIN_RESET   -1
#define CAM_PIN_XCLK     4
#define CAM_PIN_SIOD    18
#define CAM_PIN_SIOC    23
#define CAM_PIN_D7      36
#define CAM_PIN_D6      37
#define CAM_PIN_D5      38
#define CAM_PIN_D4      39
#define CAM_PIN_D3      35
#define CAM_PIN_D2      14
#define CAM_PIN_D1      13
#define CAM_PIN_D0      34
#define CAM_PIN_VSYNC    5
#define CAM_PIN_HREF    27
#define CAM_PIN_PCLK    25

// ----------------------------------------------------------------------------
static camera_config_t camera_config = 
{
  // --------------------------------------------------------------------------
  .pin_pwdn = CAM_PIN_PWDN,
  .pin_reset = CAM_PIN_RESET,
  .pin_xclk = CAM_PIN_XCLK,
  .pin_sscb_sda = CAM_PIN_SIOD,
  .pin_sscb_scl = CAM_PIN_SIOC,
  // --------------------------------------------------------------------------
  .pin_d7 = CAM_PIN_D7,
  .pin_d6 = CAM_PIN_D6,
  .pin_d5 = CAM_PIN_D5,
  .pin_d4 = CAM_PIN_D4,
  .pin_d3 = CAM_PIN_D3,
  .pin_d2 = CAM_PIN_D2,
  .pin_d1 = CAM_PIN_D1,
  .pin_d0 = CAM_PIN_D0,
  .pin_vsync = CAM_PIN_VSYNC,
  .pin_href = CAM_PIN_HREF,
  .pin_pclk = CAM_PIN_PCLK,
  // --------------------------------------------------------------------------
  .xclk_freq_hz = 20000000, // XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
  .ledc_timer = LEDC_TIMER_0,
  .ledc_channel = LEDC_CHANNEL_0,
  // --------------------------------------------------------------------------
  .pixel_format = PIXFORMAT_JPEG, // YUV422, GRAYSCALE, RGB565, JPEG
  .frame_size = FRAMESIZE_HQVGA, // Do not use sizes above QVGA when not JPEG
  // --------------------------------------------------------------------------
  .jpeg_quality = 10, // 0-63 Lower number means higher quality
  .fb_count = 3 // If more than one, i2s runs in continuous mode. Use only with JPEG
  // --------------------------------------------------------------------------
};

// ----------------------------------------------------------------------------
int32_t visionCamera_init()
{
  // IO13, IO14 is designed for JTAG by default. To use it as 
  // generalized input, firstly declare it as pullup input.
  gpio_config_t conf;
  conf.mode = GPIO_MODE_INPUT;
  conf.pull_up_en = GPIO_PULLUP_ENABLE;
  conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  conf.intr_type = GPIO_INTR_DISABLE;
  conf.pin_bit_mask = 1LL << 13;
  gpio_config(&conf);
  conf.pin_bit_mask = 1LL << 14;
  gpio_config(&conf);

  // Initialize the camera
  esp_err_t err = esp_camera_init(&camera_config);
  if(err != ESP_OK) 
  {
    ESP_LOGE(__func__, "Camera Init Failed");
    return err;
  }

  return ESP_OK;
}
