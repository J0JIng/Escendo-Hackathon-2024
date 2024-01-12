#include <Arduino.h>
#include "DHT.h"

#define DHTTYPE DHT11       // DHT 11
#define DHT_PIN 14          // pin connected to the DHT sensor
#define BUZZ_PIN 26         // pin connected to the Buzzer
#define RED_LED_PIN 25      // pin connected to the LED
#define TOUCH_SENSOR_PIN 27 // pin connected to the TTP223

DHT dht(DHT_PIN, DHTTYPE);

unsigned long lastTouchTime = 0;
const unsigned long cooldownDuration = 10 * 1000; // 10 seconds cooldown

void buzzShortPulses(int numPulses, int pulseDuration, int pauseDuration) {
  for (int i = 0; i < numPulses; i++) {
    digitalWrite(BUZZ_PIN, HIGH);
    delay(pulseDuration);  // Buzz for pulseDuration milliseconds
    digitalWrite(BUZZ_PIN, LOW);
    delay(pauseDuration);  // Pause for pauseDuration milliseconds
  }
}

void blinkRedLed(int numBlinks, int blinkDuration, int pauseDuration) {
  for (int i = 0; i < numBlinks; i++) {
    digitalWrite(RED_LED_PIN, HIGH);
    delay(blinkDuration);
    digitalWrite(RED_LED_PIN, LOW);
    delay(pauseDuration);
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println(F("slave test!"));

  pinMode(RED_LED_PIN,OUTPUT);
  pinMode(BUZZ_PIN, OUTPUT);
  pinMode(TOUCH_SENSOR_PIN, INPUT);
  dht.begin();
}

void loop() {
  delay(2000);

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(t);
  Serial.println(F("°C "));

  // Check if cooldown period has elapsed
  if (millis() - lastTouchTime >= cooldownDuration) {
    // Check if touch sensor is detected
    if (digitalRead(TOUCH_SENSOR_PIN) == HIGH) {
      digitalWrite(RED_LED_PIN, LOW);
      // Record the time when touch sensor was last detected
      lastTouchTime = millis();
    } else if (t > 30.0) {
      // If touch sensor not detected and temperature is greater than 30.0, buzz and blink
      buzzShortPulses(3, 50, 100);
      blinkRedLed(3,200,100);
    }
  }
}
