/*
  IoT Humidity & Temperature Monitor with Visual Feedback
  Platform: ESP32 + DHT22, RGB LED, Slide Switch, Buzzer
  Ordered for clarity: 1. Monitoring, 2. RGB+Buzzer, 3. Interrupt Overrides
*/

#include <Arduino.h>
#include <DHT.h>

// Pin Mapping 
#define DHTPIN        15
#define DHTTYPE       DHT22
#define BUZZER        14
#define SWITCH_RIGHT  5
#define SWITCH_LEFT   4
#define RED           16
#define GREEN         17
#define BLUE          18

// Timing 
#define DEBOUNCE_DELAY   250
#define OVERRIDE_TIME    2000
#define SENSOR_REFRESH   2000

//  Objects & State 
DHT dht(DHTPIN, DHTTYPE);

unsigned long prevSensorRead = 0;
float savedHumidity = 0;
float savedTemp = 0;

bool overrideActive = false;
unsigned long overrideStart = 0;
uint8_t overrideR, overrideG, overrideB;

volatile bool triggerRight = false;
volatile bool triggerLeft = false;
volatile unsigned long lastTriggerTime = 0;

//  RGB Control 
void setRGB(uint8_t r, uint8_t g, uint8_t b) {
  analogWrite(RED, r);
  analogWrite(GREEN, g);
  analogWrite(BLUE, b);
}

//  Interrupt Handlers 
void IRAM_ATTR onRightTrigger() {
  if (millis() - lastTriggerTime > DEBOUNCE_DELAY) {
    triggerRight = true;
    lastTriggerTime = millis();
  }
}

void IRAM_ATTR onLeftTrigger() {
  if (millis() - lastTriggerTime > DEBOUNCE_DELAY) {
    triggerLeft = true;
    lastTriggerTime = millis();
  }
}

//  1. Continuous Monitoring (DHT22 Read + Serial Display) 
void monitorEnvironment() {
  if (millis() - prevSensorRead < SENSOR_REFRESH) return;

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println("Sensor Read Failed — Using cached data");
    h = savedHumidity;
    t = savedTemp;
  } else {
    savedHumidity = h;
    savedTemp = t;
  }

  Serial.printf("%.1f°C | %.1f%%\n", t, h);
  prevSensorRead = millis();

  // Proceed to RGB + Buzzer feedback
  handleVisualAndAudioFeedback(h);
}

//  Humidity Level Indication (RGB LED + Buzzer) 
void handleVisualAndAudioFeedback(float humidity) {
  if (humidity >= 80) {
    tone(BUZZER, 1000);
  } else {
    noTone(BUZZER);
  }

  if (humidity >= 90)       setRGB(128, 0, 128);   // Purple
  else if (humidity >= 80)  setRGB(255, 0, 0);     // Red
  else if (humidity >= 70)  setRGB(255, 127, 0);   // Orange
  else if (humidity >= 60)  setRGB(255, 255, 0);   // Yellow
  else if (humidity >= 50)  setRGB(0, 255, 0);     // Green
  else if (humidity >= 40)  setRGB(0, 0, 255);     // Blue
  else if (humidity >= 30)  setRGB(255, 255, 255); // White
  else                      setRGB(0, 0, 0);       // Off
}

//  3. Interrupt-Based Slide Switch Overrides 
void manageOverrides() {
  if (!overrideActive) {
    if (triggerRight) {
      triggerRight = false;
      overrideActive = true;
      overrideR = 0;
      overrideG = 255;
      overrideB = 255;
      overrideStart = millis();
      noTone(BUZZER);
      Serial.println("Right Slide → Cyan Override Activated");
    } else if (triggerLeft) {
      triggerLeft = false;
      overrideActive = true;
      overrideR = 255;
      overrideG = 0;
      overrideB = 255;
      overrideStart = millis();
      noTone(BUZZER);
      Serial.println("Left Slide → Pink Override Activated");
    }
  }
}

// Setup 
void setup() {
  Serial.begin(115200);

  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BLUE, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  pinMode(SWITCH_RIGHT, INPUT_PULLUP);
  pinMode(SWITCH_LEFT, INPUT_PULLUP);

  attachInterrupt(SWITCH_RIGHT, onRightTrigger, FALLING);
  attachInterrupt(SWITCH_LEFT, onLeftTrigger, FALLING);

  dht.begin();
  Serial.println("Device Ready: Monitoring in progress...");
}

//  Loop 
void loop() {
  manageOverrides();

  if (overrideActive) {
    if (millis() - overrideStart >= OVERRIDE_TIME) {
      overrideActive = false;
      prevSensorRead = 0;
      Serial.println("Override Complete — Resuming monitoring");
    } else {
      setRGB(overrideR, overrideG, overrideB);
    }
  } else {
    monitorEnvironment();
  }
}
