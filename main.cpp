#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include <BH1750.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

// Pin definitions
#define SOIL_PIN         35
#define WATER_LEVEL_PIN  34
#define PUMP_PIN         19
#define LED_PIN          2
#define POWER_CONTROL    16

// OLED setup
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Sensors
Adafruit_AHTX0 aht;
BH1750 lightMeter;

void setup() {
    Serial.begin(115200);
    pinMode(PUMP_PIN, OUTPUT);
    pinMode(LED_PIN, OUTPUT);
    pinMode(POWER_CONTROL,OUTPUT);
    digitalWrite(POWER_CONTROL,HIGH); 
    digitalWrite(PUMP_PIN, LOW);

    Wire.begin();

    // OLED init
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(" ");
        while (true);
    }

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    if (!aht.begin()) Serial.println("AHT10 not found!");
    if (!lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) Serial.println("BH1750 not found!");
}

void loop() {
    // Read sensors
    int soilRaw = analogRead(SOIL_PIN);
    float soilPercent = map(soilRaw, 3000, 1000, 0, 100);
    soilPercent = constrain(soilPercent, 0, 100);

    float lux = lightMeter.readLightLevel();

    sensors_event_t humidity, temp;
    aht.getEvent(&humidity, &temp);

    int waterLevelRaw = analogRead(WATER_LEVEL_PIN);
    bool waterLow = waterLevelRaw < 1000;

    // Watering logic
    if (soilPercent < 30 && !waterLow) {
        digitalWrite(PUMP_PIN, HIGH);
        digitalWrite(LED_PIN, HIGH);
        delay(3000);
        digitalWrite(PUMP_PIN, LOW);
        digitalWrite(LED_PIN, LOW);
    }

    // Print to Serial
    Serial.printf("Soil: %.1f%% | Temp: %.1f°C | Hum: %.1f%% | Lux: %.1f lx | Water Low: %s\n",
        soilPercent, temp.temperature, humidity.relative_humidity, lux, waterLow ? "YES" : "NO");

    // OLED Display (only 4 lines for 128x32)
    display.clearDisplay();
    display.setCursor(0, 0);
    display.printf("S:%.0f%% T:%.0fC\n", soilPercent, temp.temperature);
    display.printf("H:%.0f%% L:%.0f lx\n", humidity.relative_humidity, lux);
    display.printf("Water:%s\n", waterLow ? "LOW" : "OK");
    display.display();

    delay(1000);
}
