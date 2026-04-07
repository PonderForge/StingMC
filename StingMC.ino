#include <MAX31855.h>
#include <EEPROM.h>

#define uCModeLED 11
#define uCStatusLED 10
#define uCErrorLED 9
#define throttleProgSW 12
#define throttleIn 27
#define MCIn1 0
#define MCIn2 1
#define MCEN 2
#define nFault 4
#define MCCurrent 3

#define SCK_PIN  22
#define CS_PIN   21
#define MISO_PIN 20

MAX31855 tc(SCK_PIN, CS_PIN, MISO_PIN);
int throttleMin = 400;
int throttleMax = 800;

void setup() {
  // Boot Code900
  tc.begin();
  EEPROM.begin(256);
  Serial.begin(115200);
  delay(1000);
  Serial.println("Cedarville MC Start.");
  pinMode(uCModeLED, OUTPUT);
  pinMode(uCStatusLED, OUTPUT);
  pinMode(uCErrorLED, OUTPUT);
  pinMode(throttleProgSW, INPUT);
  pinMode(throttleIn, INPUT);
  pinMode(nFault, INPUT);
  pinMode(MCIn2, OUTPUT);
  pinMode(MCIn1, OUTPUT);
  pinMode(MCEN, OUTPUT);
  pinMode(MCCurrent, INPUT);

  // Set MC to be active
  digitalWrite(MCIn1, LOW);
  digitalWrite(MCEN, HIGH);

  // Enable MC IN1 PWM
  analogWriteFreq(10000);

  // Print inital temp
  Serial.print("internal temp = ");
  Serial.print(tc.getInternalTemperature());
  Serial.println(" Celsius");

  // Read Throttle Bounds into Memory
  int min = makeWord(EEPROM.read(0), EEPROM.read(1));
  if (min <= 1024) {
    throttleMin = min;
  } else {
    throttleMin = analogRead(throttleIn);
  }
  int max = makeWord(EEPROM.read(2), EEPROM.read(3));
  Serial.println(max);
  if (max <= 1024) {
    throttleMax = max;
  }

  digitalWrite(uCModeLED, HIGH);
}

void loop() {
  // Change Speed to Throttle
  int drive = map(analogRead(throttleIn), throttleMin, throttleMax, 0, 255);
  if (drive < 0)
    drive = 0;
  if (drive > 255)
    drive = 255;
  analogWrite(MCIn2, drive);

  // Check for Configuration reqiurements
  throttleConfig();

  // Drive Fault Checking
  while (!digitalRead(nFault)) {
    digitalWrite(uCErrorLED, HIGH);
    analogWrite(MCIn2, 0);
    delay(100);
    digitalWrite(uCErrorLED, LOW);
  } 

  // Heartbeat
  digitalWrite(uCStatusLED, HIGH);
  delay(25);
  digitalWrite(uCStatusLED, LOW);
  delay(25);

  // Show Current consumption on ErrorLED
  digitalWrite(uCErrorLED, digitalRead(MCCurrent));
}

void throttleConfig () {
  if (digitalRead(throttleProgSW)) {
    analogWrite(MCIn2, 0);
    digitalWrite(uCModeLED, LOW);
    throttleMin = 400;
    throttleMax = 800;

    // Find Max and Min of Throttle
    while (digitalRead(throttleProgSW)) {
      digitalWrite(uCStatusLED, LOW);
      int val = analogRead(throttleIn);
      if (val > throttleMax) {
        throttleMax = val;
        digitalWrite(uCStatusLED, HIGH);
      } else if (val < throttleMin) {
        throttleMin = val;
        digitalWrite(uCStatusLED, HIGH);
      }
      delay(100);
    }

    // Add Padding to Throttle
    throttleMax = throttleMax - 50;
    throttleMin = throttleMin + 50;

    // Write Throttle Bounds to Flash
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

