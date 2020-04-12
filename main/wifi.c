// ----------------------------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#include "wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_smartconfig.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include <stdbool.h>
#include <string.h>
#include "udp_broadcast.h"

// ----------------------------------------------------------------------------
static wifi_config_t m_wifiConfig;
static EspWifiState_t m_state = EspWifiState_None;

// ----------------------------------------------------------------------------
static int32_t m_disconnectCount = 0;
static uint8_t m_desiredChannel = 1;

// ----------------------------------------------------------------------------
int32_t wifi_disconnectCountGet()
{
  return m_disconnectCount;
}

// ----------------------------------------------------------------------------
static uint8_t m_forceDisconnectFlag;
static uint8_t m_connectionTrialAbortFlag;

// ----------------------------------------------------------------------------
void wifi_disconnect()
{
  if(m_state == EspWifiState_GotIP)
  {
    m_forceDisconnectFlag = true;
    wifi_connectionTrialAbort();
  }
}

// ----------------------------------------------------------------------------
void wifi_connectionTrialAbort()
{
  esp_wifi_stop();
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);
  esp_wifi_set_mode(WIFI_MODE_STA);
  esp_wifi_start();

  m_connectionTrialAbortFlag = true;
}

// ----------------------------------------------------------------------------
static esp_err_t event_handler(void *ctx, system_event_t *event)
{
  switch(event->event_id)
  {
    case SYSTEM_EVENT_STA_START:
    {
      ESP_LOGI("Wifi", "eventHandler: SYSTEM_EVENT_STA_START");
      m_state = EspWifiState_ReadyToConnect;
      break;
    }
    case SYSTEM_EVENT_STA_CONNECTED:
    {
      ESP_LOGI("Wifi", "eventHandler: SYSTEM_EVENT_STA_CONNECTED");
      break;
    }
    case SYSTEM_EVENT_STA_DISCONNECTED:
    {
      uint8_t reason = event->event_info.disconnected.reason;
      ESP_LOGI("Wifi", "eventHandler: SYSTEM_EVENT_STA_DISCONNECTED");
      ESP_LOGI("Wifi", "Reason: %d", reason);

      if(reason == WIFI_REASON_NO_AP_FOUND)
      {
        ESP_LOGI("Wifi", "No AP found");
      }

      if(reason == WIFI_REASON_HANDSHAKE_TIMEOUT)
      {
        ESP_LOGI("Wifi", "Password issue?");
      }

      if(reason == WIFI_REASON_ASSOC_LEAVE)
      {
        ESP_LOGI("Wifi", "Assoc leave");
      }

      if((m_state == EspWifiState_GotIP) && (m_forceDisconnectFlag == false))
      {
        printf("m_forceDisconnectFlag: %d\n", m_forceDisconnectFlag);
        esp_restart();
      }
      else if(m_connectionTrialAbortFlag == false)
      {
        esp_wifi_set_config(ESP_IF_WIFI_STA, &m_wifiConfig);
        esp_wifi_connect();
      }
      else
      {
        m_state = EspWifiState_ConnectionTrialFailed;
      }
      break;
    }
    case SYSTEM_EVENT_AP_START:
    {
      ESP_LOGI("Wifi", "eventHandler: SYSTEM_EVENT_AP_START");
      break;
    }
    case SYSTEM_EVENT_AP_STOP:
    {
      ESP_LOGI("Wifi", "eventHandler: SYSTEM_EVENT_AP_STADISCONNECTED");
      break;
    }
    case SYSTEM_EVENT_AP_STACONNECTED:
    {
      ESP_LOGI("Wifi", "eventHandler: SYSTEM_EVENT_AP_STACONNECTED: " MACSTR " id=%d",
        MAC2STR(event->event_info.sta_connected.mac), event->event_info.sta_connected.aid
      );
      break;
    }
    case SYSTEM_EVENT_AP_STADISCONNECTED:
    {
      ESP_LOGI("Wifi", "eventHandler: SYSTEM_EVENT_AP_STADISCONNECTED: " MACSTR " id=%d",
        MAC2STR(event->event_info.sta_disconnected.mac),
        event->event_info.sta_disconnected.aid
      );
      break;
    }
    case SYSTEM_EVENT_AP_PROBEREQRECVED:
    {
      ESP_LOGI("Wifi", "eventHandler: SYSTEM_EVENT_AP_STADISCONNECTED: " MACSTR " rssi=%d",
        MAC2STR(event->event_info.ap_probereqrecved.mac),
        event->event_info.ap_probereqrecved.rssi
      );
      break;
    }
    case SYSTEM_EVENT_STA_GOT_IP:
    {
      uint8_t primary;
      wifi_second_chan_t second;
      esp_wifi_get_channel(&primary, &second);
      ESP_LOGI("Get channel", "prim: %d secn: %d", primary, second);
      ESP_LOGI("Cfg channel", "%d", m_wifiConfig.sta.channel);

      if((m_wifiConfig.sta.channel != 0) &&
         (primary != m_wifiConfig.sta.channel))
      {
        esp_wifi_disconnect();
      }
      else
      {
        m_state = EspWifiState_GotIP;
        ESP_LOGI("Wifi", "eventHandler: SYSTEM_EVENT_STA_GOT_IP");
        udp_broadcast_init();
      }
      break;
    }
    default:
    {
      ESP_LOGI("Wifi", "eventHandler: Unknown event_id: %d\n", event->event_id);
      break;
    }
  }
  return ESP_OK;
}

// ----------------------------------------------------------------------------
void wifi_init()
{
  tcpip_adapter_init();

  // ...
  m_state = EspWifiState_Booting;
  esp_event_loop_init(event_handler, NULL);

  // ...
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);
  esp_wifi_set_mode(WIFI_MODE_STA);
  esp_wifi_start();
}

// ----------------------------------------------------------------------------
void wifi_connectFromMemory()
{
  // Do we have a valid ssid / pass combo?
  if(wifi_configReadFromMemory(&m_wifiConfig) == true)
  {
    if(m_state == EspWifiState_ReadyToConnect)
    {
      m_forceDisconnectFlag = false;
      m_state = EspWifiState_Connecting;
      m_connectionTrialAbortFlag = false;
      esp_wifi_set_config(ESP_IF_WIFI_STA, &m_wifiConfig);
      esp_wifi_connect();
    }
  }
}

// ----------------------------------------------------------------------------
void wifi_connectAndStore(char* ssid, char* password, uint8_t channel)
{
  wifi_configStoreToMemory(ssid, password, channel);
  wifi_connectFromMemory();
}

// ----------------------------------------------------------------------------
uint8_t wifi_configStoreToMemory(char* ssid, char* pass, uint8_t channel)
{
  esp_err_t rv;
  nvs_handle my_handle;
  rv = nvs_open("storage", NVS_READWRITE, &my_handle);

  rv = nvs_set_str(my_handle, "ssid", ssid);
  if(rv != 0)
  {
    nvs_close(my_handle);
    return false;
  }

  rv = nvs_set_str(my_handle, "pass", pass);
  if(rv != 0)
  {
    nvs_close(my_handle);
    return false;
  }

  rv = nvs_set_u8(my_handle, "chan", channel);
  if(rv != 0)
  {
    nvs_close(my_handle);
    return false;
  }

  rv = nvs_commit(my_handle);
  if(rv != 0)
  {
    return false;
  }

  return true;
}

// ----------------------------------------------------------------------------
int32_t wifi_configReadFromMemory(wifi_config_t* config)
{
  esp_err_t rv;
  nvs_handle my_handle;
  rv = nvs_open("storage", NVS_READWRITE, &my_handle);

  size_t required_size;
  rv = nvs_get_str(my_handle, "ssid", NULL, &required_size);
  if(rv != 0)
  {
    nvs_close(my_handle);
    return false;
  }
  else
  {
    char* ssid = malloc(required_size);
    nvs_get_str(my_handle, "ssid", ssid, &required_size);
    memcpy(config->sta.ssid, ssid, required_size);
  }

  rv = nvs_get_str(my_handle, "pass", NULL, &required_size);
  if(rv != 0)
  {
    nvs_close(my_handle);
    return false;
  }
  else
  {
    char* pass = malloc(required_size);
    nvs_get_str(my_handle, "pass", pass, &required_size);
    memcpy(config->sta.password, pass, required_size);
  }

  rv = nvs_get_u8(my_handle, "chan", &(config->sta.channel));
  if(rv != 0)
  {
    nvs_close(my_handle);
    return false;
  }

  nvs_close(my_handle);
  return true;
}

// ----------------------------------------------------------------------------
EspWifiState_t wifi_currentState_get()
{
  return m_state;
}

// ----------------------------------------------------------------------------
uint8_t wifi_autoConnectEnable_set(uint8_t enable)
{
  esp_err_t rv;
  nvs_handle my_handle;
  rv = nvs_open("storage", NVS_READWRITE, &my_handle);

  rv = nvs_set_u8(my_handle, "autoConn", enable);
  if(rv != 0)
  {
    nvs_close(my_handle);
    return false;
  }

  rv = nvs_commit(my_handle);
  if(rv != 0)
  {
    return false;
  }

  return true;
}

// ----------------------------------------------------------------------------
int32_t wifi_autoConnectEnable_get()
{
  esp_err_t rv;
  uint8_t isEnabled;
  nvs_handle my_handle;
  rv = nvs_open("storage", NVS_READWRITE, &my_handle);

  rv = nvs_get_u8(my_handle, "autoConn", &isEnabled);
  if(rv != 0)
  {
    nvs_close(my_handle);
    return -1;
  }

  nvs_close(my_handle);
  return isEnabled;
}
