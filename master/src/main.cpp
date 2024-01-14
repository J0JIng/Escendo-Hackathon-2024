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
#define BLUE_LED_PIN 27

// Initialize hardware components and objects
WiFiClientSecure net = WiFiClientSecure(); // Initialize secure WiFi client
PubSubClient client(net);                  // Initialize MQTT client with secure WiFi client
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 0;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServer, gmtOffset_sec, daylightOffset_sec);

// Functions statements
void connectAWS();                                                    // Function to establish connection with AWS IoT
void publishMessage(float wbgt, float ict) ;                          // Function to publish sensor data to AWS IoT
void messageHandler(char *topic, byte *payload, unsigned int length); // Function to handle incoming MQTT messages
void blinkLed(int numBlinks, int blinkDuration, int pauseDuration);    

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
  float slave_WBGT;
  float slave_ICT;
  bool is_danger_WBGT;
  bool is_danger_ICT;
} slave_info_t;

static TaskHandle_t controller; // task to rx slave_info_t and then broadcast, msg to awsTask
static TaskHandle_t aws;        // on rx msg from controller, send to aws
QueueHandle_t incomingDataQueue = xQueueCreate(MAX_PENDING_DEVICEINFO_QUEUE, sizeof(slave_info_t));
void controllerTask(void *);
void awsTask(void *);

static slave_info_t GLOBAL_data;
static volatile bool GLOBAL_data_ready = false;
static volatile bool GLOBAL_Slave_in_Danger = false;

// Update accordingly
uint8_t broadcastAddress1[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // Slave A
uint8_t broadcastAddress2[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // Slave B

esp_now_peer_info_t peerInfoA;
esp_now_peer_info_t peerInfoB;

// CHANGE END  //

void cb_espnow_rx(const uint8_t *mac, const uint8_t *incomingData, int len)
{

  memcpy(&GLOBAL_data, incomingData, sizeof(GLOBAL_data));
  GLOBAL_data_ready = true;

  Serial.print("Bytes received: ");
  Serial.println(len);
  /*
  Serial.print("slave Temperature: ");
  Serial.println(GLOBAL_data.temperature);
  Serial.print("slave Humidity: ");
  Serial.println(GLOBAL_data.humidity);
  Serial.print("slave WBGT: ");
  Serial.println(GLOBAL_data.slave_WBGT);
  Serial.print("slave ICT: ");
  Serial.println(GLOBAL_data.slave_ICT);
  */

  // distress signal
  if(GLOBAL_data.is_danger_WBGT||GLOBAL_data.is_danger_ICT) GLOBAL_Slave_in_Danger = true;
}

void cb_espnow_tx(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}


/* Initialize and configure components:
   - Initialize logging for debugging
   - Set WiFi mode to Access Point and Station
   - Log the MAC address of the WiFi interface
   - Create and start tasks for controller and AWS functionality
*/
void setup()
{
  APP_LOG_START();
  pinMode(BLUE_LED_PIN, OUTPUT);
  WiFi.mode(WIFI_AP_STA);// important or nothing will work.
  //WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
 
  //Serial.println("Connecting to Wi-Fi");
 
  //while (WiFi.status() != WL_CONNECTED)
  //{
  //  delay(500);
  //  Serial.print(".");
  //}
  /*
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);

  Serial.println("Connecting to Wi-Fi");
  
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("[INIT] ERROR CONNECTING TO WIFI");
    vTaskDelay(2000);
  }
  Serial.println("[INIT] WIFI CONNECTION OK");
  */
  //connectAWS();
  
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

/* 
   Controller Task:
   - Initialize ESP-NOW for communication with peers
   - Register receive and send callback functions
   - Register a peer with a specified broadcast address
   - Continuously monitor for data readiness and send data to the slave through ESP-NOW
*/
void controllerTask(void *){
  Serial.println("Master Test!");

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
  peerInfoA.channel = 0;
  peerInfoA.encrypt = false;
  // register first peer
  memcpy(peerInfoA.peer_addr, broadcastAddress1, 6);
  if (esp_now_add_peer(&peerInfoA) != ESP_OK)
  {
    Serial.println("Failed to add Slave A");
    return;
  }

  
  // register peer
  peerInfoB.channel = 0;
  peerInfoB.encrypt = false;
  // register first peer
  memcpy(peerInfoB.peer_addr, broadcastAddress2, 6);
  if (esp_now_add_peer(&peerInfoB) != ESP_OK)
  {
    Serial.println("Failed to add Slave B");
    return;
  }
  
  
  static slave_info_t data;
  for (;;)
  {
    //APP_LOG_INFO("waiting..");
    if (GLOBAL_data_ready)
    {
      GLOBAL_data_ready = false;
       blinkLed(3, 200, 100);
      if (xQueueSend(incomingDataQueue, (void *)&GLOBAL_data, 1) == pdPASS)
      { 

        //Serial.println("slave status");
        //Serial.println(GLOBAL_Slave_in_Danger);

        if(GLOBAL_Slave_in_Danger){
          APP_LOG_INFO("Slave in Danger");
          esp_now_send(broadcastAddress2, (uint8_t *)&GLOBAL_data, sizeof(slave_info_t));
         // placeholder for broadcast ^^^^^
          APP_LOG_INFO("TX TO QUEUE");
          GLOBAL_Slave_in_Danger = false;
        }
      }
    }

    vTaskDelay(MIN_TICKS_YIELD / portTICK_PERIOD_MS);
  }
}

/* 
   AWS Task:
   - Continuously monitor the incoming data queue
   - If there is data in the queue, receive and process it
     - Log reception from the queue
     - Log specific data (e.g., humidity)
     - Placeholder for an expensive function to send data to AWS (commented out)
*/
void awsTask(void *){

  static slave_info_t data;
  for (;;)
  {
    if ((uxQueueMessagesWaiting(incomingDataQueue) > 0))
    {

      if (xQueueReceive(incomingDataQueue, (void *)&data, 1) == pdPASS)
      {

        APP_LOG_INFO("RX FROM QUEUE");
        APP_LOG_INFO(data.slave_WBGT);
        APP_LOG_INFO(data.is_danger_WBGT);
        APP_LOG_INFO(data.is_danger_ICT);

#warning "PLACEHOLDER FOR SOME EXPENSIVE FN TO SEND TO AWS"
        //publishMessage(data.slave_WBGT,data.slave_ICT);
        //client.loop();
      }
    }

    vTaskDelay(MIN_TICKS_YIELD / portTICK_PERIOD_MS);
  }
}

void loop()
{
}


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
void publishMessage(float wbgt, float ict)
{
  StaticJsonDocument<200> doc;
  doc["Master_ID"] = "Master_01";
// doc["time"] = getCurrentUnixTimestamp();
#warning "COMMENTED OUT BECAUSE OF COMPILER ERROR"
  doc["slave_WBGT"] = wbgt;
  doc["slave_internal_core_body_temperature"] = ict;

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

unsigned long getCurrentUnixTimestamp()
{
  // Update the NTP client to get the current time
  timeClient.update();

  // Get the current Unix timestamp (in seconds)
  return timeClient.getEpochTime();
}

// Function to blink the red LED
void blinkLed(int numBlinks, int blinkDuration, int pauseDuration) {
  for (int i = 0; i < numBlinks; i++) {
    digitalWrite(BLUE_LED_PIN, HIGH);
    delay(blinkDuration);
    digitalWrite(BLUE_LED_PIN, LOW);
    delay(pauseDuration);
  }
}

