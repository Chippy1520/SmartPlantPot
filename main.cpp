#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include <BH1750.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

// Pins
#define SOIL_PIN          35
#define WATER_LEVEL_PIN   34
#define PUMP_PIN          19
#define LED_PIN           2
#define POWER_CONTROL     16
#define MENU_BUTTON_PIN   25
#define BATTERY_LEVEL_PIN 32

// OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Sensors
Adafruit_AHTX0 aht;
BH1750 lightMeter;

// Globals
float soilPercent = 0, temperature = 0, humidityVal = 0, lux = 0;
float maxTemp = -100, minTemp = 100;
float maxHumidity = 0, minHumidity = 100;
float maxSoil = 0, minSoil = 100;
float maxLux = 0, minLux = 99999;
bool waterLow = false;

float batteryVoltage = 0;
float batteryPercent = 0;

int currentCard = 0;
const int totalCards = 6;
bool lastButtonState = LOW;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 200;

// ---------- Forward Declaration ----------
void drawCardContent(int card, int xOffset);

// ---------- UI Helpers ----------
void drawBar(float percent, int y = 20) {
    int barWidth = (int)(percent * (SCREEN_WIDTH - 20) / 100.0);
    display.drawRect(10, y, SCREEN_WIDTH - 20, 8, SSD1306_WHITE);
    display.fillRect(10, y, barWidth, 8, SSD1306_WHITE);
}

void drawBatteryIcon(float percent, float voltage, int x = 40, int y = 10) {
    display.drawRect(x, y, 40, 14, SSD1306_WHITE);
    display.drawRect(x + 40, y + 4, 3, 6, SSD1306_WHITE); // battery cap

    int fill = percent * 40 / 100.0;
    if (fill > 0)
        display.fillRect(x + 1, y + 1, fill - 1, 12, SSD1306_WHITE);

    display.setCursor(x + 46, y);
    if (voltage < 3.4) {
        display.setTextSize(1);
        display.print("❗");
    } else if (voltage > 4.15) {
        display.setTextSize(1);
        display.print("⚡");
    }
}

void animateCardChange(int fromCard, int toCard) {
    const int step = 8;
    for (int offset = 0; offset <= SCREEN_WIDTH; offset += step) {
        display.clearDisplay();
        drawCardContent(fromCard, -offset);
        drawCardContent(toCard, SCREEN_WIDTH - offset);
        delay(10);
    }
}

void handleButton() {
    bool reading = digitalRead(MENU_BUTTON_PIN);
    if (reading != lastButtonState && (millis() - lastDebounceTime > debounceDelay)) {
        lastDebounceTime = millis();
        if (reading == HIGH) {
            int prevCard = currentCard;
            currentCard = (currentCard + 1) % totalCards;
            animateCardChange(prevCard, currentCard);
        }
    }
    lastButtonState = reading;
}

// ---------- Sensor Logic ----------
void readSensors() {
    // Soil
    int soilRaw = analogRead(SOIL_PIN);
    soilPercent = map(soilRaw, 3000, 1000, 0, 100);
    soilPercent = constrain(soilPercent, 0, 100);
    maxSoil = max(maxSoil, soilPercent);
    minSoil = min(minSoil, soilPercent);

    // Temp and Humidity
    sensors_event_t humidity, temp;
    aht.getEvent(&humidity, &temp);
    temperature = temp.temperature;
    humidityVal = humidity.relative_humidity;
    maxTemp = max(maxTemp, temperature);
    minTemp = min(minTemp, temperature);
    maxHumidity = max(maxHumidity, humidityVal);
    minHumidity = min(minHumidity, humidityVal);

    // Light
    lux = lightMeter.readLightLevel();
    maxLux = max(maxLux, lux);
    minLux = min(minLux, lux);

    // Water
    int waterRaw = analogRead(WATER_LEVEL_PIN);
    waterLow = waterRaw < 1000;

    // Battery
    int adc = analogRead(BATTERY_LEVEL_PIN);
    float v_adc = adc * 3.3 / 4095.0;
    batteryVoltage = v_adc * (4.2 / 3.3);  // scale back to actual voltage
    batteryPercent = constrain(map(batteryVoltage * 100, 330, 420, 0, 100), 0, 100);
}

// ---------- Cards ----------
void drawCardContent(int card, int xOffset) {
    display.clearDisplay();
    display.setCursor(xOffset, 0);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    switch (card) {
        case 0:
            display.print("Soil: "); display.print((int)soilPercent); display.println(" %");
            display.print("Min "); display.print(minSoil, 0); display.print(" Max "); display.println(maxSoil, 0);
            drawBar(soilPercent);
            break;
        case 1:
            display.print("Temp: "); display.print(temperature, 1); display.println(" C");
            display.print("Min "); display.print(minTemp, 1); display.print(" Max "); display.println(maxTemp, 1);
            drawBar(map(temperature, 0, 50, 0, 100));
            break;
        case 2:
            display.print("Humidity: "); display.print(humidityVal, 1); display.println(" %");
            display.print("Min "); display.print(minHumidity, 1); display.print(" Max "); display.println(maxHumidity, 1);
            drawBar(humidityVal);
            break;
        case 3:
            display.print("Light: "); display.print(lux, 0); display.println(" lx");
            display.print("Min "); display.print(minLux, 0); display.print(" Max "); display.println(maxLux, 0);
            drawBar(min(lux / 1000.0 * 100, 100.0));
            break;
        case 4:
            display.setTextSize(2);
            display.setCursor(xOffset + 20, 0);
            display.print("Water:");
            display.setCursor(xOffset + 30, 20);
            display.print(waterLow ? "LOW" : "OK");
            break;
        case 5:
            display.setTextSize(1);
            display.print("Battery: ");
            display.print(batteryVoltage, 2); display.println(" V");
            display.print("Charge: ");
            display.print((int)batteryPercent); display.println(" %");
            drawBatteryIcon(batteryPercent, batteryVoltage);
            break;
    }

    display.display();
}

// ---------- Setup & Loop ----------
void setup() {
    Serial.begin(115200);

    pinMode(PUMP_PIN, OUTPUT);
    pinMode(LED_PIN, OUTPUT);
    pinMode(POWER_CONTROL, OUTPUT);
    pinMode(MENU_BUTTON_PIN, INPUT);
    digitalWrite(POWER_CONTROL, HIGH);
    digitalWrite(PUMP_PIN, LOW);

    Wire.begin();

    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("OLED not found");
        while (true);
    }

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    if (!aht.begin()) Serial.println("AHT10 not found!");
    if (!lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) Serial.println("BH1750 not found!");
}

void loop() {
    readSensors();
    handleButton();

    drawCardContent(currentCard, 0);

    // Watering condition
    if (batteryVoltage >= 3.4 && soilPercent < 30 && !waterLow) {
        digitalWrite(PUMP_PIN, HIGH);
        digitalWrite(LED_PIN, HIGH);
        delay(3000);
        digitalWrite(PUMP_PIN, LOW);
        digitalWrite(LED_PIN, LOW);
    }

    delay(200);
}
