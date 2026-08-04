
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BMP280.h>
#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include "images.h"

// pins for more info look into the schmeatic in the github
static const int PIN_LED    = 48;
static const int PIN_SDA    = 8;
static const int PIN_SCL    = 9;
static const int PIN_EPD_CS   = 10;
static const int PIN_EPD_MOSI = 11;
static const int PIN_EPD_SCK  = 12;
static const int PIN_EPD_DC   = 14;
static const int PIN_EPD_RST  = 15;
static const int PIN_EPD_BUSY = 16;
static const uint8_t SENSOR_ADDR = 0x76;
static const float   SEA_LEVEL_HPA = 1013.25f; 

Adafruit_BMP280 sensor;
GxEPD2_BW<GxEPD2_290_T94_V2, GxEPD2_290_T94_V2::HEIGHT>
    display(GxEPD2_290_T94_V2(PIN_EPD_CS, PIN_EPD_DC, PIN_EPD_RST, PIN_EPD_BUSY));

struct Telemetry {
  float temperature;   // C
  float pressure;      // hPa
  float altitude;      // m
  bool  valid;
};

static bool sensorReady  = false;
static bool displayReady = false;
RTC_DATA_ATTR int currentPage = 0;


static uint8_t readChipId() {
  Wire.beginTransmission(SENSOR_ADDR);
  Wire.write(0xD0);
  if (Wire.endTransmission() != 0) return 0xFF;
  Wire.requestFrom(SENSOR_ADDR, (uint8_t)1);
  return Wire.available() ? Wire.read() : 0xFF;
}

static bool initSensor() {
  Wire.begin(PIN_SDA, PIN_SCL);
  delay(100);

  uint8_t id = readChipId();
  Serial.printf("Sensor chip ID: 0x%02X\n", id);

  if (!sensor.begin(SENSOR_ADDR)) {
    Serial.println("Sensor init failed.");
    return false;
  }

  sensor.setSampling(Adafruit_BMP280::MODE_NORMAL,
                     Adafruit_BMP280::SAMPLING_X2,
                     Adafruit_BMP280::SAMPLING_X16,
                     Adafruit_BMP280::FILTER_X16,
                     Adafruit_BMP280::STANDBY_MS_500);

  Serial.println("Sensor ready.");
  return true;
}

static Telemetry readTelemetry() {
  Telemetry t = {0, 0, 0, false};
  if (!sensorReady) return t;

  t.temperature = sensor.readTemperature();
  t.pressure    = sensor.readPressure() / 100.0f;
  t.altitude    = sensor.readAltitude(SEA_LEVEL_HPA);
  t.valid       = !isnan(t.temperature);
  return t;
}

static bool initDisplay() {
  display.init(0, true, 10, false);
  SPI.end();
  SPI.begin(PIN_EPD_SCK, -1, PIN_EPD_MOSI, PIN_EPD_CS);
  display.setRotation(1);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);
  if (digitalRead(PIN_EPD_BUSY) == HIGH) {
    Serial.println("Display busy line stuck high.");
    return false;
  }

  Serial.println("Display ready.");
  return true;
}

static void renderDashboard(const Telemetry &t) {
  if (!displayReady) return;

  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.drawInvertedBitmap(0, 0, world_map_bitmap, 296, 128, GxEPD_BLACK);

    
    display.fillRect(0, 0, 160, 22, GxEPD_WHITE);
    display.setCursor(4, 16);
    display.print("VOYAGER / ORBIT");

    display.fillRect(0, 106, 296, 22, GxEPD_WHITE);
    display.setCursor(4, 122);
    if (t.valid) {
      display.printf("%.1fC  %.0fhPa  %.0fm", t.temperature, t.pressure, t.altitude);
    } else {
      display.print("SENSOR OFFLINE");
    }
  } while (display.nextPage());
}

//inicio de todo

void setup() {
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);

  Serial.begin(115200);
  uint32_t start = millis();
  while (!Serial && millis() - start < 3000) delay(10);

  Serial.println("\nVoyager Panel booting...");

  sensorReady  = initSensor();
  displayReady = initDisplay();

  renderDashboard(readTelemetry());
}

void loop() {
  static uint32_t lastRefresh = 0;
  const uint32_t REFRESH_INTERVAL = 60000;   

  Telemetry t = readTelemetry();

  if (t.valid) {
    Serial.printf("T=%.2fC  P=%.2fhPa  Alt=%.1fm\n",
                  t.temperature, t.pressure, t.altitude);
  }

  if (displayReady && millis() - lastRefresh > REFRESH_INTERVAL) {
    renderDashboard(t);
    lastRefresh = millis();
  }

  digitalWrite(PIN_LED, HIGH);
  delay(100);
  digitalWrite(PIN_LED, LOW);
  delay(1900);
}