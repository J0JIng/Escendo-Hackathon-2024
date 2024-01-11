// Import Libarary 
#include "secrets.h"
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "WiFi.h"
#include <TimeLib.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#define SAMPLING_INTERVAL 1000 // Sampling interval to receive slave info
#define AWS_IOT_PUBLISH_TOPIC   "esp32/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/sub"

// Initialize hardware components and objects
WiFiClientSecure net = WiFiClientSecure();    // Initialize secure WiFi client
PubSubClient client(net);                     // Initialize MQTT client with secure WiFi client
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0; 
const int daylightOffset_sec = 0;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServer, gmtOffset_sec, daylightOffset_sec);

// Declare global constants 
float slave_WBGT; // Slave WBGT info 
float slave_ICT;  // Slave Internal Core body temperature
float slave_ID; // slave ID
unsigned long lastReadTime = 0;

// Functions statements 
void connectAWS(); // Function to establish connection with AWS IoT
void publishMessage(); // Function to publish sensor data to AWS IoT
void messageHandler(char* topic, byte* payload, unsigned int length); // Function to handle incoming MQTT messages
 
void setup()
{
  Serial.begin(115200);
  connectAWS();
  
}

void loop()
{
  // Wait for xx minutes to elapse since the last reading
  if (millis() - lastReadTime >= SAMPLING_INTERVAL)
  {
    
    // Get Information on Slave
    /*
    
    
    
    */

    // Update the last pH reading time
    lastReadTime = millis();
  }

  // Transfer data to cloud
  publishMessage();
  client.loop();
}

/***************** Display *********************/

/***************** AWS API *********************/

/**
 * Establishes a connection to the AWS IoT platform.
 * Configures WiFi and sets up the MQTT connection.
 */

void connectAWS()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
 
  Serial.println("Connecting to Wi-Fi");
 
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
 
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
  doc["time"] = getCurrentUnixTimestamp();
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
void messageHandler(char* topic, byte* payload, unsigned int length)
{
  Serial.print("incoming: ");
  Serial.println(topic);
 
  StaticJsonDocument<200> doc;
  deserializeJson(doc, payload);
  const char* message = doc["message"];
  Serial.println(message);
}

/***************** Get time  *********************/

unsigned long getCurrentUnixTimestamp() {
  // Update the NTP client to get the current time
  timeClient.update();

  // Get the current Unix timestamp (in seconds)
  return timeClient.getEpochTime();
}

