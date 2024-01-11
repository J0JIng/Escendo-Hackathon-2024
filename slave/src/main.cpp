#include <Arduino.h>
#include "esp_now.h"
#include "WiFi.h"

#include <WiFiUdp.h>

uint8_t broadcastAddress1[] = {0x30, 0xAE, 0xA4, 0x99, 0x39, 0x94};
// put function declarations here:
int myFunction(int, int);
esp_now_peer_info_t peerInfo;

typedef struct
{
  float humidity;
  float temperature;
  float globe_temperature;
  bool is_danger_WBGT;
  bool is_danger_ICT;
  bool friend_warning;
} __attribute__((packed)) slave_info_t;

void cb_espnow_tx(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void cb_espnow_rx(const uint8_t *mac, const uint8_t *incomingData, int len)
{

  Serial.println("recived warning from central");
}
void setup()
{
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);

  Serial.println(WiFi.macAddress());
  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_recv_cb(cb_espnow_rx);
  esp_now_register_send_cb(cb_espnow_tx);

  // register peer
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  // register first peer
  memcpy(peerInfo.peer_addr, broadcastAddress1, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    Serial.println("Failed to add peer");
    return;
  }
}

void loop()
{
  slave_info_t test;
  test.humidity = 0.5;
  test.temperature = 0.5;
  test.globe_temperature = 0.5;
  test.is_danger_WBGT = false;
  test.is_danger_ICT = false;
  test.friend_warning = false;
  esp_err_t result = esp_now_send(0, (uint8_t *)&test, sizeof(slave_info_t));

  if (result == ESP_OK)
  {
    Serial.println("Sent with success");
  }
  else
  {
    Serial.println("Error sending the data");
  }
  delay(2000);
}

// put function definitions here:
int myFunction(int x, int y)
{
  return x + y;
}