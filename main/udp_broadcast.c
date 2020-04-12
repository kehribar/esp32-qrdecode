// ----------------------------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#include "udp_broadcast.h"
#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <lwip/sockets.h>
#include <lwip/err.h>
#include <lwip/sockets.h>
#include <lwip/sys.h>
#include <lwip/netdb.h>
#include <lwip/dns.h>
#include "tcpip_adapter.h"
#include "lwip/udp.h"
#include <string.h>
#include "wifi.h"

// ----------------------------------------------------------------------------
#define UDPSERVER_PORT 12345
#define UDP_ADV_NAME "CAMERA"

// ----------------------------------------------------------------------------
static struct udp_pcb *pcb;
static uint8_t m_running = false;

// ----------------------------------------------------------------------------
static void udp_broadcast_task(void *pvParameters)
{
  static struct pbuf* p;
  static int32_t m_cnt = 0;
  static char broadcastString[128];
  static tcpip_adapter_ip_info_t local_ip;

  p = pbuf_alloc(PBUF_TRANSPORT, sizeof(broadcastString), PBUF_RAM);

  const portTickType xPeriod = (1000 / portTICK_RATE_MS);
  portTickType xLastWakeTime = xTaskGetTickCount();

  while(1)
  {
    vTaskDelayUntil(&xLastWakeTime, xPeriod);

    if(m_running == true)
    {
      tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &local_ip);

      sprintf(broadcastString, "%s, %d.%d.%d.%d, %d",
        UDP_ADV_NAME, IP2STR(&local_ip.ip), m_cnt++
      );

      memcpy(p->payload, broadcastString, sizeof(broadcastString));

      udp_send(pcb, p);
    }
  }
}

// ----------------------------------------------------------------------------
void udp_broadcast_init()
{
  if(m_running == false)
  {
    pcb = udp_new();

    #if 1
      ip_addr_t tmp;
      IP_ADDR4(&tmp, 255, 255, 255, 255);
      int32_t rv = udp_connect(pcb, &tmp, UDPSERVER_PORT);
    #else
      int32_t rv = udp_connect(pcb, IP_ADDR_ANY, UDPSERVER_PORT);
    #endif

    if(rv == 0)
    {
      m_running = true;

      ESP_LOGI("udp_broadcast", "Init ok.");

      xTaskCreate(&udp_broadcast_task, "udpServer", 2048, NULL, 5, NULL);
    }
  }
}

// ----------------------------------------------------------------------------
void udp_broadcast_stop()
{
  m_running = false;
}
