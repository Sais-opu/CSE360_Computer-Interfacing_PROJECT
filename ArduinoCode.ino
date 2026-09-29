#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <SoftwareSerial.h>


// =======================================
// Pet Food Monitoring System (with DHT11 + HC-06 Bluetooth + Water Level + Cooling Fan + Buzzer)
// MEMORY-OPTIMIZED VERSION:
//  - No String objects (they heap-allocate and fragment RAM on AVR)
//  - All text literals wrapped in F() so they live in flash, not RAM
//  - Status is tracked as small enums (1 byte) instead of String text
// =======================================


#define FOOD_LEVEL_IR_PIN 4
#define FOOD_BOWL_IR_PIN 11
#define SERVO_PIN 8


// DHT11 Temperature & Humidity sensor
#define DHTPIN 3
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);


// Water level sensor
#define WATER_LEVEL_PIN A3
const int waterLowThreshold = 250;  // sensor maxes near 500, so below 250 = low


// HC-06 Bluetooth module
#define BT_RX_PIN 7
#define BT_TX_PIN 12
SoftwareSerial bluetooth(BT_RX_PIN, BT_TX_PIN);


// Cooling fan (40mm 5V 2-pin, driven through a transistor)
#define FAN_PIN 5
const float humidityFanThreshold = 75.0;  // fan on above this, off at/below this


// Buzzer (active buzzer)
#define BUZZER_PIN 2
const unsigned long buzzerInterval = 20000;   // 20 seconds between buzzes
const unsigned long buzzerDuration = 2000;    // buzz lasts 2 seconds


Servo feederServo;


// LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);


const unsigned long warningTime = 10000;         // 10 seconds
const unsigned long consumedTime = 50000;        // 50 seconds
const unsigned long servoInterval = 10000;       // 10 seconds
const unsigned long foodLowWarningTime = 20000;  // 20 seconds


const unsigned long dhtReadInterval = 2000;      // DHT11 needs >=1-2s between reads
const unsigned long lcdRotateInterval = 3000;    // switch LCD view every 3 seconds


unsigned long foodDetectedStart = 0;
unsigned long foodConsumedStart = 0;
unsigned long lastServoTime = 0;
unsigned long foodLowStartTime = 0;
unsigned long lastDhtReadTime = 0;
unsigned long lastLcdSwitchTime = 0;


// Water level timers, mirroring the food consumption timers above
unsigned long waterOkStart = 0;
unsigned long waterConsumedStart = 0;


// Buzzer timers
unsigned long lastBuzzerTime = 0;
unsigned long buzzerStartTime = 0;


bool previousFoodDetected = false;
bool consumedMode = false;
bool foodLowTimerRunning = false;
byte lcdViewIndex = 0;  // 0 = status, 1 = DHT, 2 = water level


// Water level state, mirroring the food consumption state machine
bool previousWaterOk = true;
bool waterConsumedMode = false;
bool waterQuotaWarningActive = false;


// Warning flags
bool foodContainerWarningActive = false;
bool foodNotConsumedWarningActive = false;


// Fan state
bool fanOn = false;


// Buzzer state
bool buzzerActive = false;
bool previousWarningActive = false;


// ---------------------------------------
// Status is now stored as tiny enums (1 byte) instead of String text.
// The actual text only exists once, in flash, inside the print
// helper functions further down.
// ---------------------------------------
enum DispenseState : uint8_t { DISP_WAITING, DISP_DISPENSED, DISP_MANUAL, DISP_OVERFLOW, DISP_EMPTY, DISP_BOTH_WARN };
enum ConsumeState  : uint8_t { CONSUME_MONITORING, CONSUME_NOT_EATEN, CONSUME_WARNING, CONSUME_CONSUMED };
enum WaterState    : uint8_t { WATER_MONITORING, WATER_WARNING, WATER_CONSUMED };


DispenseState dispenseState = DISP_WAITING;
ConsumeState  consumeState  = CONSUME_MONITORING;
WaterState    waterState    = WATER_MONITORING;


// DHT11 readings
float currentTemperature = NAN;
float currentHumidity = NAN;


// Water level reading
int currentWaterLevel = 0;


// =========================================================
// PRINT HELPERS
// These take a Print& so the SAME function can write to
// Serial, the LCD, or the Bluetooth SoftwareSerial - all three
// classes inherit from Print. This avoids ever building a
// String and avoids duplicating text literals in code.
// =========================================================


void printDispenseFull(Print &out) {
  switch (dispenseState) {
    case DISP_WAITING:   out.print(F("Waiting")); break;
    case DISP_DISPENSED: out.print(F("Food dispensed")); break;
    case DISP_MANUAL:    out.print(F("Food dispensed (manual)")); break;
    case DISP_OVERFLOW:  out.print(F("Food could not be dispensed to prevent overflow")); break;
    case DISP_EMPTY:     out.print(F("Food could not be dispensed, No food in food container")); break;
    case DISP_BOTH_WARN: out.print(F("Food not consumed and Container is empty")); break;
  }
}


void printDispenseShort(Print &out) {
  switch (dispenseState) {
    case DISP_WAITING:   out.print(F("Disp: Waiting")); break;
    case DISP_DISPENSED: out.print(F("Disp: Dispensed")); break;
    case DISP_MANUAL:    out.print(F("Disp: Dispensed")); break;
    case DISP_OVERFLOW:  out.print(F("Disp: Overflow")); break;
    case DISP_EMPTY:     out.print(F("Disp: Empty")); break;
    case DISP_BOTH_WARN: out.print(F("Disp: Both Warn")); break;
  }
}


void printConsumeFull(Print &out) {
  switch (consumeState) {
    case CONSUME_MONITORING: out.print(F("Monitoring")); break;
    case CONSUME_NOT_EATEN:  out.print(F("Required food not eaten")); break;
    case CONSUME_WARNING:    out.print(F("Required food not consumed in the last 24hrs")); break;
    case CONSUME_CONSUMED:   out.print(F("Food consumed")); break;
  }
}


void printConsumeShort(Print &out) {
  switch (consumeState) {
    case CONSUME_MONITORING: out.print(F("Monitoring")); break;
    case CONSUME_NOT_EATEN:  out.print(F("Food Not Eaten")); break;
    case CONSUME_WARNING:    out.print(F("Food Warning")); break;
    case CONSUME_CONSUMED:   out.print(F("Food Consumed")); break;
  }
}


void printWaterState(Print &out) {
  switch (waterState) {
    case WATER_MONITORING: out.print(F("Monitoring")); break;
    case WATER_WARNING:    out.print(F("Quota for 24hrs not met")); break;
    case WATER_CONSUMED:   out.print(F("Consumed(Refill)")); break;
  }
}


// Dispenses food, attaching/detaching the servo only for the moment it moves
// This avoids the Servo <-> SoftwareSerial timer conflict that caused
// continuous spinning/jitter when the servo stayed attached full time.
void dispenseFood() {
  feederServo.attach(SERVO_PIN);
  delay(50);              // brief settle time after attach
  feederServo.write(90);
  delay(1000);
  feederServo.write(0);
  delay(300);              // let it reach 0 before detaching
  feederServo.detach();
}


void setup() {


  Serial.begin(9600);


  pinMode(FOOD_LEVEL_IR_PIN, INPUT);
  pinMode(FOOD_BOWL_IR_PIN, INPUT);
  pinMode(WATER_LEVEL_PIN, INPUT);


  pinMode(FAN_PIN, OUTPUT);
  digitalWrite(FAN_PIN, LOW);  // fan starts off


  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);  // buzzer starts off


  // Servo is intentionally NOT attached here anymore.
  // It is only attached inside dispenseFood() right when it needs to move.


  dht.begin();


  bluetooth.begin(9600);  // HC-06 default baud rate is 9600
  bluetooth.println(F("Pet Feeder Bluetooth Ready"));


  // LCD Setup
  lcd.init();
  lcd.backlight();


  lcd.setCursor(0, 0);
  lcd.print(F("Pet Feeder"));
  lcd.setCursor(0, 1);
  lcd.print(F("Starting..."));
  delay(2000);
  lcd.clear();


  Serial.println(F("Pet Food Monitoring Started"));
}


void loop() {


  // =====================================
  // BLUETOOTH - MANUAL COMMANDS FROM PHONE
  // =====================================


  if (bluetooth.available()) {


    char cmd = bluetooth.read();


    if (cmd == 'D' || cmd == 'd') {


      bluetooth.println(F("Manual dispense triggered"));
      Serial.println(F("Manual dispense triggered via Bluetooth"));


      dispenseFood();


      dispenseState = DISP_MANUAL;
      lastServoTime = millis();  // reset auto-dispense timer too
    }
  }


  // =====================================
  // SENSOR 1 - FOOD CONTAINER
  // =====================================


  int foodLevel = digitalRead(FOOD_LEVEL_IR_PIN);


  if (foodLevel == HIGH) {


    Serial.println(F("Food LOW"));


    if (!foodLowTimerRunning) {
      foodLowStartTime = millis();
      foodLowTimerRunning = true;
    }


    if (millis() - foodLowStartTime >= foodLowWarningTime) {
      foodContainerWarningActive = true;
    }


  } else {


    Serial.println(F("Food OK"));


    foodLowTimerRunning = false;
    foodContainerWarningActive = false;
  }


  // =====================================
  // SENSOR 2 - FOOD BOWL
  // =====================================


  bool foodDetected = (digitalRead(FOOD_BOWL_IR_PIN) == LOW);


  // -------------------------------------
  // FOOD CONSUMED MODE
  // -------------------------------------


  if (consumedMode) {


    consumeState = CONSUME_CONSUMED;


    if (millis() - foodConsumedStart >= consumedTime) {
      consumedMode = false;
      foodDetectedStart = millis();
    }
  }


  // -------------------------------------
  // NORMAL MONITORING MODE
  // -------------------------------------


  else {


    if (foodDetected) {


      consumeState = CONSUME_NOT_EATEN;


      if (!previousFoodDetected) {
        foodDetectedStart = millis();
        foodNotConsumedWarningActive = false;
      }


      if (millis() - foodDetectedStart >= warningTime) {


        foodNotConsumedWarningActive = true;


        consumeState = CONSUME_WARNING;
      }
    }


    // Food was just removed
    if (!foodDetected && previousFoodDetected) {


      foodNotConsumedWarningActive = false;


      consumedMode = true;
      foodConsumedStart = millis();


      consumeState = CONSUME_CONSUMED;
    }
  }


  // =====================================
  // SERVO DISPENSING / DISPENSING WARNINGS
  // =====================================


  if (millis() - lastServoTime >= servoInterval) {


    // Both warnings active
    if (foodContainerWarningActive &&
        foodNotConsumedWarningActive) {


      dispenseState = DISP_BOTH_WARN;


      lastServoTime = millis();
    }


    // Food not consumed warning active
    else if (foodNotConsumedWarningActive) {


      dispenseState = DISP_OVERFLOW;


      lastServoTime = millis();
    }


    // Food container empty warning active
    else if (foodContainerWarningActive) {


      dispenseState = DISP_EMPTY;


      lastServoTime = millis();
    }


    // No warnings active -> dispense
    else {


      dispenseState = DISP_DISPENSED;


      Serial.println(F("Food dispensed"));


      dispenseFood();


      lastServoTime = millis();
    }
  }


  previousFoodDetected = foodDetected;


  // =====================================
  // SENSOR 3 - DHT11 TEMPERATURE & HUMIDITY
  // =====================================


  if (millis() - lastDhtReadTime >= dhtReadInterval) {


    lastDhtReadTime = millis();


    float h = dht.readHumidity();
    float t = dht.readTemperature();  // Celsius by default


    if (isnan(h) || isnan(t)) {
      Serial.println(F("Failed to read from DHT11 sensor!"));
    } else {
      currentHumidity = h;
      currentTemperature = t;
    }


    // -------------------------------------
    // FAN CONTROL - based on humidity, single threshold
    // Fan turns ON above 75%, turns OFF at/below 75%
    // -------------------------------------


    if (!isnan(currentHumidity)) {


      if (currentHumidity > humidityFanThreshold && !fanOn) {
        digitalWrite(FAN_PIN, HIGH);
        fanOn = true;
        Serial.println(F("Fan ON (humidity above 75%)"));
      }


      else if (currentHumidity <= humidityFanThreshold && fanOn) {
        digitalWrite(FAN_PIN, LOW);
        fanOn = false;
        Serial.println(F("Fan OFF (humidity at or below 75%)"));
      }
    }
  }


  // =====================================
  // SENSOR 4 - WATER LEVEL
  // Mirrors the exact same state-machine pattern as the food
  // consumption logic above: "present" sustained -> warning after
  // warningTime (10s); "removed"/low -> immediate "consumed" state
  // that holds for consumedTime (50s) before returning to monitoring.
  // =====================================


  currentWaterLevel = analogRead(WATER_LEVEL_PIN);


  bool waterOk = (currentWaterLevel >= waterLowThreshold);


  // -------------------------------------
  // WATER CONSUMED MODE (mirrors FOOD CONSUMED MODE)
  // -------------------------------------


  if (waterConsumedMode) {


    waterState = WATER_CONSUMED;


    if (millis() - waterConsumedStart >= consumedTime) {
      waterConsumedMode = false;
      waterOkStart = millis();
    }
  }


  // -------------------------------------
  // NORMAL WATER MONITORING MODE (mirrors NORMAL MONITORING MODE)
  // -------------------------------------


  else {


    if (waterOk) {


      waterState = WATER_MONITORING;


      if (!previousWaterOk) {
        waterOkStart = millis();
        waterQuotaWarningActive = false;
      }


      if (millis() - waterOkStart >= warningTime) {


        waterQuotaWarningActive = true;


        waterState = WATER_WARNING;
      }
    }


    // Water just dropped low
    if (!waterOk && previousWaterOk) {


      waterQuotaWarningActive = false;


      waterConsumedMode = true;
      waterConsumedStart = millis();


      waterState = WATER_CONSUMED;
    }
  }


  previousWaterOk = waterOk;


  // =====================================
  // BUZZER - warns every 20 seconds for 2 seconds
  // while any food or water warning is active
  // =====================================


  bool anyWarningActive = foodContainerWarningActive ||
                           foodNotConsumedWarningActive ||
                           waterQuotaWarningActive;


  if (anyWarningActive) {


    // Warning just turned on -> buzz immediately
    if (!previousWarningActive) {
      buzzerActive = true;
      buzzerStartTime = millis();
      lastBuzzerTime = millis();
      digitalWrite(BUZZER_PIN, HIGH);
    }


    // Otherwise, buzz again every 20 seconds while warning persists
    else if (!buzzerActive && millis() - lastBuzzerTime >= buzzerInterval) {
      buzzerActive = true;
      buzzerStartTime = millis();
      lastBuzzerTime = millis();
      digitalWrite(BUZZER_PIN, HIGH);
    }


    // Stop the buzz after 2 seconds
    if (buzzerActive && millis() - buzzerStartTime >= buzzerDuration) {
      digitalWrite(BUZZER_PIN, LOW);
      buzzerActive = false;
    }


  } else {
    // No warning -> make sure buzzer is off
    if (buzzerActive) {
      digitalWrite(BUZZER_PIN, LOW);
      buzzerActive = false;
    }
  }


  previousWarningActive = anyWarningActive;


  // =====================================
  // STATUS DISPLAY - SERIAL
  // =====================================


  Serial.print(F("Food Dispense Status: "));
  printDispenseFull(Serial);
  Serial.println();


  Serial.print(F("Food Consumption Status: "));
  printConsumeFull(Serial);
  Serial.println();


  if (!isnan(currentTemperature) && !isnan(currentHumidity)) {
    Serial.print(F("Temperature: "));
    Serial.print(currentTemperature);
    Serial.print(F(" C  Humidity: "));
    Serial.print(currentHumidity);
    Serial.print(F(" %  Fan: "));
    Serial.println(fanOn ? F("ON") : F("OFF"));
  }


  Serial.print(F("Water status: "));
  Serial.print(currentWaterLevel);
  Serial.print(F(" - "));
  printWaterState(Serial);
  Serial.println();


  Serial.println(F("----------------------"));


  // =====================================
  // STATUS DISPLAY - LCD (rotates between
  // feeder status, temp/humidity, and water level)
  // =====================================


  if (millis() - lastLcdSwitchTime >= lcdRotateInterval) {
    lcdViewIndex = (lcdViewIndex + 1) % 3;
    lastLcdSwitchTime = millis();
  }


  lcd.clear();


  if (lcdViewIndex == 1) {


    lcd.setCursor(0, 0);


    if (!isnan(currentTemperature) && !isnan(currentHumidity)) {
      lcd.print(F("T:"));
      lcd.print(currentTemperature, 1);
      lcd.print((char)223);  // degree symbol
      lcd.print(F("C F:"));
      lcd.print(fanOn ? F("ON") : F("OFF"));


      lcd.setCursor(0, 1);
      lcd.print(F("Humidity: "));
      lcd.print(currentHumidity, 0);
      lcd.print(F("%"));


      bluetooth.print(F("Temp: "));
      bluetooth.print(currentTemperature, 1);
      bluetooth.print(F("C Fan:"));
      bluetooth.print(fanOn ? F("ON") : F("OFF"));
      bluetooth.print(F(" | Humidity: "));
      bluetooth.print(currentHumidity, 0);
      bluetooth.println(F("%"));
    } else {
      lcd.print(F("DHT11 Error"));
      lcd.setCursor(0, 1);
      lcd.print(F("Check wiring"));


      bluetooth.println(F("DHT11 Error | Check wiring"));
    }


  } else if (lcdViewIndex == 2) {


    lcd.setCursor(0, 0);
    lcd.print(F("Water: "));
    lcd.print(currentWaterLevel);


    lcd.setCursor(0, 1);
    printWaterState(lcd);  // will truncate to 16 chars on the LCD


    bluetooth.print(F("Water status: "));
    bluetooth.print(currentWaterLevel);
    bluetooth.print(F(" | "));
    printWaterState(bluetooth);  // sent in full over Bluetooth
    bluetooth.println();


  } else {


    lcd.setCursor(0, 0);
    printDispenseShort(lcd);


    lcd.setCursor(0, 1);
    printConsumeShort(lcd);


    // =====================================
    // STATUS DISPLAY - BLUETOOTH
    // Mirrors exactly what the LCD is currently showing,
    // sent as one line so it doesn't come in broken.
    // =====================================
    printDispenseShort(bluetooth);
    bluetooth.print(F(" | "));
    printConsumeShort(bluetooth);
    bluetooth.println();
    
  }


  delay(1000);
}



