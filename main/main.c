// ----------------------------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#include "main.h"
#include "webserver.h"

// ----------------------------------------------------------------------------
#define SSID "PRIMARY_SSID"
#define PASS "PRIMARY_PASS"

// ----------------------------------------------------------------------------
#define SSID2 "SECONDARY_SSID"
#define PASS2 "SECONDARY_PASS"

// ----------------------------------------------------------------------------
void main_task(void* arg);

// ----------------------------------------------------------------------------
// Hardware & library initialise code
void app_main()
{
  // ...
  nvs_flash_init();
  wifi_init();
  
  // ...
  visionCamera_init();
  app_httpserver_init();

  // ...
  xTaskCreate(&main_task, "main_task", 2048, NULL, 10, NULL);
  return;
}

// ----------------------------------------------------------------------------
// 100ms, main housekeeping method.
void main_task(void* arg)
{
  static int32_t m_cnt = 0;
  static int32_t m_lowVal = INT32_MAX;
  portTickType xLastWakeTime = xTaskGetTickCount();
  const portTickType xPeriod = (100 / portTICK_RATE_MS);

  while(true)
  {
    // ...
    if(m_cnt == 30)
    {
      wifi_connectAndStore(SSID, PASS, 0);
    }

    // Wifi connection trial timeout check
    static int32_t m_timeout_cnt = 0;
    static int32_t m_connectTryStart_cnt = 0;
    static uint8_t wifiState_d = EspWifiState_None;
    uint8_t wifiState = wifi_currentState_get();
    if((wifiState == EspWifiState_Connecting) &&
       (wifiState_d != EspWifiState_Connecting))
    {
      m_connectTryStart_cnt = m_cnt;
    }
    else if(wifiState == EspWifiState_Connecting)
    {
      if((m_cnt - m_connectTryStart_cnt) == 100)
      {
        m_timeout_cnt += 1;
        wifi_connectionTrialAbort();
        ESP_LOGW("Wifi", "Connection timeout?");
      }
    }
    if(wifiState == EspWifiState_ReadyToConnect)
    {
      if((m_timeout_cnt > 0) && (m_timeout_cnt < 5))
      {
        ESP_LOGW("Wifi", "Trying secondary network configuration");
        wifi_connectAndStore(SSID2, PASS2, 0);
      }
    }
    wifiState_d = wifiState;

    // ...
    m_cnt += 1;
    vTaskDelayUntil(&xLastWakeTime, xPeriod);
  }
}
