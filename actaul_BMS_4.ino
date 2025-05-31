#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define BATTERY_PIN PA0     // ADC pin to measure battery voltage
#define CURRENT_PIN PA1     // ADC pin to measure current voltage
#define BUZZER_PIN PA5      // Buzzer pin

const float R_SHUNT = 10000.0;            // 10k ohm resistor
const float VOLTAGE_DIVIDER_RATIO = 6.0; // voltage divider ratio
const float MAX_BATTERY_VOLTAGE = 3.3;   // max battery voltage (example)
const float CURRENT_THRESHOLD = 0.1;      // threshold current (Amps) for sudden current spike

unsigned long buzzerOnUntil = 0;

void setup() {
  Serial.begin(9600);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // Test buzzer at startup for 0.5 seconds
  digitalWrite(BUZZER_PIN, HIGH);
  delay(500);
  digitalWrite(BUZZER_PIN, LOW);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED not found");
    while (1);
  }
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(35, 20);
  display.println("BMS");
  display.display();
  delay(2000);
}

void loop() {
  float batteryVoltage = readBatteryVoltage();
  float batteryPercent = (batteryVoltage / MAX_BATTERY_VOLTAGE) * 100.0;
  if (batteryPercent > 100) batteryPercent = 100;
  if (batteryPercent < 0) batteryPercent = 0;

  float current = readCurrent();

  unsigned long now = millis();

  static float prevCurrent = 0;
  if ((current - prevCurrent) > CURRENT_THRESHOLD) {
    buzzerOnUntil = now + 5000;  // buzzer ON for 5 seconds on current spike
  }
  prevCurrent = current;

  bool buzzerShouldBeOn = false;

  if (batteryPercent < 20) {
    buzzerShouldBeOn = true;
  }
  if (now < buzzerOnUntil) {
    buzzerShouldBeOn = true;
  }

  digitalWrite(BUZZER_PIN, buzzerShouldBeOn ? HIGH : LOW);

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);

  if (batteryPercent >= 20) {
    display.println("Battery Voltage:");
    display.setTextSize(2);
    display.setCursor(0, 15);
    display.print(batteryVoltage, 2);
    display.println(" V");

    display.setTextSize(1);
    display.setCursor(0, 45);
    display.print("Current: ");
    display.print(current * 1000, 2);  // mA
    display.println(" mA");

    display.setCursor(0, 55);
    display.print("Battery %: ");
    display.print(batteryPercent, 1);
    display.println("%");
  } else {
    display.setTextSize(2);
    display.setCursor(0, 20);
    display.println("SOC is below");
    display.setCursor(0, 50);
    display.println("20% !!!");
  }

  display.display();

  Serial.print("Battery %: ");
  Serial.print(batteryPercent);
  Serial.print(" | Current (mA): ");
  Serial.print(current * 1000);
  Serial.print(" | Buzzer: ");
  Serial.println(buzzerShouldBeOn ? "ON" : "OFF");

  delay(500);
}

float readBatteryVoltage() {
  int analogValue = analogRead(BATTERY_PIN);         // 0 - 4095
  float voltage = (analogValue / 4095.0) * 3.3;      // voltage at ADC pin
  return voltage * VOLTAGE_DIVIDER_RATIO;            // real battery voltage
}

float readCurrent() {
  int analogValue = analogRead(CURRENT_PIN);
  float voltageAcrossResistor = (analogValue / 4095.0) * 3.3;
  return voltageAcrossResistor / R_SHUNT;            // current (I = V / R)
}
