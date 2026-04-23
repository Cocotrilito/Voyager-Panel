#include <Arduino.h>
#include <GxEPD2_BW.h> // libreria de la waveshare chaval!
#include <Adafruit_BME280.h> // Libreria del sensor
#include <Fonts/FreeMonoBold9pt7b.h>
#include "images.h"


//rtc cuaderno q no olvbida
RTC_DATA_ATTR int currentPage = 0;


#define BTN_NEXT 9
#define BTN_SELECT 10
Adafruit_BME280 bme;
GxEPD2_BW<GxEPD2_290_T94_V2, GxEPD2_290_T94_V2::HEIGHT> display(GxEPD2_290_T94_V2(10, 14, 15, 16));

void showMap();
void showCountdown();
void showWeather();
void sleep();

void sleep() {
  Serial.println("Voyager entrando en hibernacion!");
  display.hibernate();
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_5, 0);
  esp_deep_sleep_start();

}

void setup() {
  Serial.begin(115200);

  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) {
    currentPage++;
    if (currentPage > 2) currentPage = 0;
  }

  display.init(115200);
  display.setRotation(1);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);

  if( currentPage == 0) {
    showMap();
  } else if (currentPage == 1) {
    showCountdown();
  } else if (currentPage == 2) {
    Wire.begin(12, 13);
    if (bme.begin(0x76)) {
      showWeather();
    } else {
      Serial.println("Error no BME280");
    }
  }
  
  sleep();

  
}

void loop() {
  // no loop cause Im using the deep sleep
}
// pages
void showMap() {
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.drawInvertedBitmap(0, 0, world_map_bitmap, 296, 128, GxEPD_BLACK);
    display.fillRect(0, 0, 150, 20, GxEPD_WHITE);
    display.setCursor(5, 18);
    display.print("Voyager: ORBIT");
    display.setCursor(5, 120);
    display.print("POS: Ecuador");
  } while (display.nextPage());
}

void showCountdown() {
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setCursor(10, 30);
    display.print("STASIS COUNTDOWN:");
    display.setCursor(10, 80);
    display.print("4 days left");
  } while (display.nextPage());
}
void showWeather() {
  float temp = bme.readTemperature();
  float hum = bme.readHumidity();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setCursor(10, 30);
    display.print("AMBIENT SENSORS:");
    display.setCursor(10, 75);
    display.printf("TEMP: %.1f C", temp);
    display.setCursor(10, 110);
    display.printf("HUM: %1f %%", hum);
  } while (display.nextPage());
}
