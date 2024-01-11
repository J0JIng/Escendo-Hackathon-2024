// Import Libarary
#include "secrets.h"
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "WiFi.h"
#include <TimeLib.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "esp_now.h"

#define SAMPLING_INTERVAL 1000 // Sampling interval to receive slave info
#define AWS_IOT_PUBLISH_TOPIC "esp32/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/sub"

// Initialize hardware components and objects
WiFiClientSecure net = WiFiClientSecure(); // Initialize secure WiFi client
PubSubClient client(net);                  // Initialize MQTT client with secure WiFi client
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 0;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServer, gmtOffset_sec, daylightOffset_sec);

// Declare global constants
float slave_WBGT; // Slave WBGT info
float slave_ICT;  // Slave Internal Core body temperature
float slave_ID;   // slave ID
unsigned long lastReadTime = 0;

// Functions statements
void connectAWS();                                                    // Function to establish connection with AWS IoT
void publishMessage();                                                // Function to publish sensor data to AWS IoT
void messageHandler(char *topic, byte *payload, unsigned int length); // Function to handle incoming MQTT messages

// CHANGE START //
static constexpr uint32_t MIN_TICKS_YIELD = 25UL;
static constexpr uint32_t MAX_PENDING_DEVICEINFO_QUEUE = 10UL;

#define DEBUG 1
#ifdef DEBUG
#define DEBUG_LOGS 1
#else
#define DEBUG_LOGS 0
#endif

#define APP_LOG_START()     \
  do                        \
  {                         \
    if (DEBUG_LOGS)         \
      Serial.begin(115200); \
  } while (0)
#define APP_LOG_INFO(...)          \
  do                               \
  {                                \
    if (DEBUG_LOGS)                \
      Serial.println(__VA_ARGS__); \
  } while (0)

typedef struct
{
  float humidity;
  float temperature;
  float globe_temperature;
  bool is_danger_WBGT;
  bool is_danger_ICT;
  bool friend_warning;
} __attribute__((packed)) slave_info_t;

static TaskHandle_t controller; // task to rx slave_info_t and then broadcast, msg to awsTask
static TaskHandle_t aws;        // on rx msg from controller, send to aws
QueueHandle_t incomingDataQueue = xQueueCreate(MAX_PENDING_DEVICEINFO_QUEUE, sizeof(slave_info_t));
void controllerTask(void *);
void awsTask(void *);

static slave_info_t GLOBAL_data;
static volatile bool GLOBAL_data_ready = false;

uint8_t broadcastAddress1[] = {0x30, 0xAE, 0xA4, 0x05, 0x4C, 0x0C};

esp_now_peer_info_t peerInfo;

// CHANGE END  //

void cb_espnow_rx(const uint8_t *mac, const uint8_t *incomingData, int len)
{

  memcpy(&GLOBAL_data, incomingData, sizeof(GLOBAL_data));
  GLOBAL_data_ready = true;
  Serial.print("Bytes received: ");
  Serial.println(len);
}

void cb_espnow_tx(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void setup()
{
  APP_LOG_START();
  WiFi.mode(WIFI_AP_STA); // important or nothing will work.

 /* WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);

  Serial.println("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("[INIT] ERROR CONNECTING TO WIFI");
    vTaskDelay(2000);
  }
  Serial.println("[INIT] WIFI CONNECTION OK");
  */
 


  APP_LOG_INFO("MAC address: ");
  APP_LOG_INFO(WiFi.macAddress());
  xTaskCreatePinnedToCore(
      controllerTask, // Function to be called
      "Ctrl",         // Name of task
      32768,          // Stack size
      NULL,           // Parameter to pass
      1,              // Task priority
      &controller,    // Task handle
      0);             // CPU Core
  xTaskCreatePinnedToCore(
      awsTask, // Function to be called
      "AWS",   // Name of task
      32768,   // Stack size
      NULL,    // Parameter to pass
      1,       // Task priority
      &aws,    // Task handle
      0);      // CPU Core

  vTaskDelete(NULL);
}

void controllerTask(void *)
{
  // Init ESP-NOW
  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  // esp_wifi_config_espnow_rate(WIFI_IF_STA, WIFI_ PHY_RATE_MCS0_LGI);
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
  
  static slave_info_t data;
  for (;;)
  {
   // APP_LOG_INFO("waiting..");
    if (GLOBAL_data_ready)
    {
      GLOBAL_data_ready = false;
      if (xQueueSend(incomingDataQueue, (void *)&data, 1) == pdPASS)
      {
        esp_now_send(0, (uint8_t *)&data, sizeof(slave_info_t));
        // placeholder for broadcast ^^^^^
        APP_LOG_INFO("TX TO QUEUE");
      }
    }

    vTaskDelay(MIN_TICKS_YIELD / portTICK_PERIOD_MS);
  }
}
void awsTask(void *)
{
  static slave_info_t data;
  for (;;)
  {
    if ((uxQueueMessagesWaiting(incomingDataQueue) > 0))
    {

      if (xQueueReceive(incomingDataQueue, (void *)&data, 1) == pdPASS)
      {

        APP_LOG_INFO("RX FROM QUEUE");
        APP_LOG_INFO(data.humidity);
#warning "PLACEHOLDER FOR SOME EXPENSIVE FN TO SEND TO AWS"
        // publishMessage();
        // client.loop();
      }
    }

    vTaskDelay(MIN_TICKS_YIELD / portTICK_PERIOD_MS);
  }
}
void loop()
{
}

/***************** Display *********************/

/***************** AWS API *********************/

/**
 * Establishes a connection to the AWS IoT platform.
 * Configures WiFi and sets up the MQTT connection.
 */

void connectAWS()
{

  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  // Connect to the MQTT broker on the AWS endpoint
  client.setServer(AWS_IOT_ENDPOINT, 8883);

  // Create a message handler
  client.setCallback(messageHandler);

  Serial.println("Connecting to AWS IoT");

  while (!client.connect(THINGNAME))
  {
    Serial.print(".");
    delay(100);
  }

  if (!client.connected())
  {
    Serial.println("AWS IoT Timeout!");
    return;
  }

  // Subscribe to a topic
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);

  Serial.println("AWS IoT Connected!");
}

/**
 * Publishes sensor data to the AWS IoT platform.
 * Creates a JSON document and publishes it as a message.
 */
void publishMessage()
{
  StaticJsonDocument<200> doc;
  doc["slave_ID"] = slave_ID;
// doc["time"] = getCurrentUnixTimestamp();
#warning "COMMENTED OUT BECAUSE OF COMPILER ERROR"
  doc["slave_WBGT"] = slave_WBGT;
  doc["slave_internal_core_body_temperature"] = slave_ICT;

  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to client
  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
}

/**
 * Handles incoming MQTT messages.
 * Deserializes the JSON message and prints the content.
 */
void messageHandler(char *topic, byte *payload, unsigned int length)
{
  Serial.print("incoming: ");
  Serial.println(topic);

  StaticJsonDocument<200> doc;
  deserializeJson(doc, payload);
  const char *message = doc["message"];
  Serial.println(message);
}

/***************** Get time  *********************/

unsigned long getCurrentUnixTimestamp()
{
  // Update the NTP client to get the current time
  timeClient.update();

  // Get the current Unix timestamp (in seconds)
  return timeClient.getEpochTime();
}
