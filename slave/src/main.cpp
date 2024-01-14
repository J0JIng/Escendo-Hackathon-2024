#include <Arduino.h>
#include "DHT.h"
#include "esp_now.h"
#include "WiFi.h"
#include "esp_wifi.h"
#include <WiFiUdp.h>

// Define constants for pins and thresholds
#define DHTTYPE DHT11       // DHT 11
#define DHT_PIN 14          // Pin connected to the DHT sensor
#define BUZZ_PIN 26         // Pin connected to the Buzzer
#define RED_LED_PIN 25      // Pin connected to the LED
#define TOUCH_SENSOR_PIN 27 // Pin connected to the TTP223

//#define LIVE

// Define the broadcast address for ESP-NOW communication

uint8_t broadcastAddress1[] = {0xD8, 0xBC, 0x38, 0x75, 0x37, 0x60};

// Declare ESP-NOW peer information
esp_now_peer_info_t peerInfo;

// Declare DHT sensor object
DHT dht(DHT_PIN, DHTTYPE);

// Initialize variables for touch sensor cooldown
unsigned long lastTouchTime = 0;
const unsigned long cooldownDuration = 10 * 1000; // 10 seconds cooldown

// Thresholds for danger conditions
const float WBGT_THRESHOLD = 30.0;
const float ICT_THRESHOLD = 38.5;

// Define the structure for slave information
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

// Function to produce short pulses on the buzzer
void buzzShortPulses(int numPulses, int pulseDuration, int pauseDuration) {
  for (int i = 0; i < numPulses; i++) {
    digitalWrite(BUZZ_PIN, HIGH);
    delay(pulseDuration);  // Buzz for pulseDuration milliseconds
    digitalWrite(BUZZ_PIN, LOW);
    delay(pauseDuration);  // Pause for pauseDuration milliseconds
  }
}

// Function to blink the red LED
void blinkRedLed(int numBlinks, int blinkDuration, int pauseDuration) {
  for (int i = 0; i < numBlinks; i++) {
    digitalWrite(RED_LED_PIN, HIGH);
    delay(blinkDuration);
    digitalWrite(RED_LED_PIN, LOW);
    delay(pauseDuration);
  }
}

// Callback function for ESP-NOW transmission status
void cb_espnow_tx(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

// Callback function for ESP-NOW reception
void cb_espnow_rx(const uint8_t *mac, const uint8_t *incomingData, int len) {
  Serial.println("Received warning from central");
  //buzzShortPulses(3, 1000, 100);
}

// Function to compute WBGT (Wet Bulb Globe Temperature)
float compute_WBGT(float rh, float T, float Tg) {
#ifdef LIVE
  // Actual WBGT computation when in LIVE mode
  float Tw = T * atan(0.151977 * pow((rh + 8.313659), 0.5)) + atan(T + rh) - atan(rh - 1.676331) + 0.00391838 * pow(rh, 1.5) * atan(0.023101 * rh) - 4.686035;
  float wbgt = (0.7 * Tw) + (0.2 * Tg) + (0.1 * T);
  return wbgt;
#else
  // Return temperature if not in LIVE mode
  return T;
#endif
}

// Function to read Internal Core Temperature (ICT)
float read_ICT() {
#ifdef LIVE
  // Placeholder function to read internal core body temperature in LIVE mode
#else
  // Return a fixed value if not in LIVE mode
  return 37.3;
#endif
}

void setup() {
  Serial.begin(115200);
  Serial.println(F("Slave test!"));

  // Set pin modes
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(BUZZ_PIN, OUTPUT);
  pinMode(TOUCH_SENSOR_PIN, INPUT);

  // Initialize DHT sensor
  dht.begin();

  // Set WiFi mode
  WiFi.mode(WIFI_STA);
  Serial.println(WiFi.macAddress());
  
  esp_wifi_set_channel(11,WIFI_SECOND_CHAN_NONE);

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register callback functions
  esp_now_register_recv_cb(cb_espnow_rx);
  esp_now_register_send_cb(cb_espnow_tx);

  // Register ESP-NOW peer
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  memcpy(peerInfo.peer_addr, broadcastAddress1, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
}

void loop() {
  // Create a structure to hold sensor readings
  slave_info_t test;

  // Read sensor values
  test.humidity = dht.readHumidity();
  test.temperature = dht.readTemperature();
  test.globe_temperature = dht.readTemperature();

  // Check for sensor reading errors
  if (isnan(test.humidity) || isnan(test.temperature)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  // Compute WBGT and ICT values
  test.slave_WBGT = compute_WBGT(test.humidity, test.temperature, test.globe_temperature);
  test.slave_ICT = read_ICT();

  // Determine danger conditions
  test.is_danger_WBGT = (test.slave_WBGT >= WBGT_THRESHOLD) ? true : false;
  test.is_danger_ICT = (test.slave_ICT >= ICT_THRESHOLD) ? true : false;

  // Print sensor readings to Serial Monitor
  Serial.print(F("Humidity: "));
  Serial.print(test.humidity);
  Serial.print(F("%  Temperature: "));
  Serial.print(test.temperature);
  Serial.print(F("°C WBGT "));
  Serial.print(test.slave_WBGT);
  Serial.println(F("°C "));

  // Check if cooldown period has elapsed
  if (millis() - lastTouchTime >= cooldownDuration) {
    // Check if touch sensor is detected
    if (digitalRead(TOUCH_SENSOR_PIN) == HIGH) {
      digitalWrite(RED_LED_PIN, LOW);
      // Record the time when touch sensor was last detected
      lastTouchTime = millis();
    } else if (test.is_danger_WBGT || test.is_danger_ICT) {
      // If touch sensor not detected and exceed safe threshold value
      buzzShortPulses(3, 50, 100);
      blinkRedLed(3, 200, 100);
    }
  }

  // Send sensor data using ESP-NOW
  esp_err_t result = esp_now_send(0, (uint8_t *)&test, sizeof(slave_info_t));

  // Print ESP-NOW transmission result to Serial Monitor
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
