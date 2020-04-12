// ----------------------------------------------------------------------------
// 
// 
// ----------------------------------------------------------------------------
#include "esp_log.h"
#include "esp_camera.h"
#include "esp_http_server.h"
#include "esp_timer.h"
#include "img_converters.h"
#include "quirc.h"

// ----------------------------------------------------------------------------
#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

// ----------------------------------------------------------------------------
esp_err_t live_stream_handler(httpd_req_t *req);

// ----------------------------------------------------------------------------
httpd_handle_t camera_httpd = NULL;

// ----------------------------------------------------------------------------
uint8_t* rgb_buffer;
struct quirc *qr;

// ----------------------------------------------------------------------------
httpd_uri_t _live_stream_handler = {
  .uri       = "/live_stream",
  .method    = HTTP_GET,
  .handler   = live_stream_handler,
  .user_ctx  = NULL
};

// ----------------------------------------------------------------------------
esp_err_t live_stream_handler(httpd_req_t *req)
{
  char* part_buf[64];
  esp_err_t res = ESP_OK;
  camera_fb_t* fb = NULL;

  static int64_t last_frame = 0;
  if(!last_frame) 
  {
    last_frame = esp_timer_get_time();
  }

  httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);

  while(true)
  {
    fb = esp_camera_fb_get();

    res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
    if(res != ESP_OK)
    {
      esp_camera_fb_return(fb);
      break;
    }

    size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, fb->len);
    res = httpd_resp_send_chunk(req, (const char*)part_buf, hlen);
    if(res!= ESP_OK)
    {
      esp_camera_fb_return(fb);
      break;
    }

    res = httpd_resp_send_chunk(req, (const char*)fb->buf, fb->len);
    if(res!= ESP_OK)
    {
      esp_camera_fb_return(fb);
      break;
    }

    // ...
    fmt2rgb888(fb->buf, fb->len, PIXFORMAT_JPEG, rgb_buffer);

    // ...
    int w, h;
    uint8_t *image;
    image = quirc_begin(qr, &w, &h);

    // ...
    for(int32_t i = 0; i < w; i++)
    {
      for(int32_t j = 0; j < h; j++)
      {
        int32_t offset = 3 * ((i * h) + j);
        #if 0
          float r = rgb_buffer[offset + 0];
          float g = rgb_buffer[offset + 1];
          float b = rgb_buffer[offset + 2];
          image[(i * h) + j] = (r * 0.3) + (0.59 * g) + (0.11 * b);
        #else
          int32_t pix = 0;
          pix += (int32_t)(rgb_buffer[offset + 0]) * 77;
          pix += (int32_t)(rgb_buffer[offset + 1]) * 151;
          pix += (int32_t)(rgb_buffer[offset + 2]) * 28;
          image[(i * h) + j] = pix >> 8;
        #endif
      }
    }
    
    // ...
    quirc_end(qr);

    // ...
    int32_t num_codes = quirc_count(qr);
    for(int32_t i = 0; i < num_codes; i++) 
    {
      struct quirc_code code;
      struct quirc_data data;
      quirc_decode_error_t err;

      // Extract
      quirc_extract(qr, i, &code);

      // Decoding stage 
      err = quirc_decode(&code, &data);
      if(err)
      {
        // printf("DECODE FAILED: %s\n", quirc_strerror(err));        
      }
      else
      {
        printf("Data: %s\n", data.payload);        
      }
    }

    #if 0
      int64_t fr_end = esp_timer_get_time();
      int64_t frame_time = fr_end - last_frame;
      last_frame = fr_end;
      frame_time /= 1000;
      ESP_LOGI(__func__, "MJPG: %uKB %ums (%.1ffps)",
        (uint32_t)(fb->len/1024),
        (uint32_t)frame_time, 1000.0 / (uint32_t)frame_time
      );
    #endif

    esp_camera_fb_return(fb);
  }

  last_frame = 0;
  return ESP_OK;
}

// ----------------------------------------------------------------------------
void app_httpserver_init ()
{
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.stack_size = 24 * 4096;

  // ...
  static int32_t im_width = 240;
  static int32_t im_height = 176;
  rgb_buffer = malloc(im_width * im_width * 3);
  
  // ...
  qr = quirc_new();

  if(!qr) 
  {
    ESP_LOGE(__func__, "Failed to allocate memory for QR obj");
  }

  if(quirc_resize(qr, im_width, im_height) < 0) 
  {
    ESP_LOGE(__func__, "Failed to allocate video memory");
  }

  if(httpd_start(&camera_httpd, &config) == ESP_OK)
  {
    httpd_register_uri_handler(camera_httpd, &_live_stream_handler);
  }
}
