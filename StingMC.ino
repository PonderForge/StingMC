#include "MAX31855.h"
#include <EEPROM.h>

#define uCModeLED 11
#define uCStatusLED 10
#define uCErrorLED 9
#define throttleProg 12
#define throttle 27
#define mcin1 0
#define mcin2 1
#define mcen 2
#define nFault 4

#define SCK_PIN  22
#define CS_PIN   21
#define MISO_PIN 20

MAX31855 tc(SCK_PIN, CS_PIN, MISO_PIN);
int throttleMin = 300;
int throttleMax = 900;

void setup() {
  // Start up Code
  tc.begin();
  EEPROM.begin(256);
  Serial.begin(115200);
  delay(1000);
  Serial.println("Cedarville MC Start.");
  pinMode(uCModeLED, OUTPUT);
  pinMode(uCStatusLED, OUTPUT);
  pinMode(uCErrorLED, OUTPUT);
  pinMode(throttleProg, INPUT);
  pinMode(throttle, INPUT);
  pinMode(nFault, INPUT);
  pinMode(mcin2, OUTPUT);
  pinMode(mcin1, OUTPUT);
  pinMode(mcen, OUTPUT);

  // Set MC to be active
  digitalWrite(mcin1, LOW);
  digitalWrite(mcen, HIGH);

  // Enable MC IN1 PWM
  analogWriteFreq(10000);

  // Print inital temp
  Serial.print("internal temp = ");
  Serial.print(tc.getInternalTemperature());
  Serial.println(" Celsius");

  int min = (EEPROM.read(0) << 8) | EEPROM.read(1);
  if (min != 0) {
    throttleMin = min;
  }
  int max = (EEPROM.read(2) << 8) | EEPROM.read(3);
  if (max != 0) {
    throttleMax = max;
  }

  digitalWrite(uCModeLED, HIGH);
}

void loop() {
  int drive = map(analogRead(throttle), throttleMin, throttleMax, 0, 255);
  if (drive < 0)
    drive = 0;
  if (drive > 255)
    drive = 255;
  Serial.print("value: ");
  Serial.println(drive);
  analogWrite(mcin2, drive);

  throttleConfig();

  while (digitalRead(nFault)) {
    digitalWrite(uCErrorLED, HIGH);
    analogWrite(mcin2, 0);
  } 

  delay(100);
}

void throttleConfig () {
  if (digitalRead(throttleProg)) {
    analogWrite(mcin2, 0);
    digitalWrite(uCModeLED, LOW);
    digitalWrite(uCStatusLED, LOW);

    while (digitalRead(throttleProg)) {
      int val = analogRead(throttle);
      if (val > throttleMax) {
        throttleMax = val;
      } else if (val < throttleMin) {
        throttleMin = val;
      }
    }

    throttleMax = throttleMax - 50;
    throttleMin = throttleMin + 50;

    EEPROM.write(0, (throttleMin >> 8) & 0xFF);
    EEPROM.write(1, throttleMin & 0xFF);
    EEPROM.write(2, (throttleMax >> 8) & 0xFF);
    EEPROM.write(3, throttleMax & 0xFF);
    if (EEPROM.commit()) {
      Serial.println("Throttle program saved!");
    } else {
      Serial.println("Throttle program failed :(");
    }
    digitalWrite(uCModeLED, HIGH);
  }
}

