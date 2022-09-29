#include <Arduino.h>
#include <hardware.h>
#include <wifi_mgr.h>

TaskHandle_t wifi_task;

void setup() {
  Serial.begin(115200);
  Serial.println("Starting setup");
  setup_gpio();

  /* Start WiFi task */
  xTaskCreate(
    etask_wifi_mgr,
    "WiFi_Task",
    5000,
    NULL,
    0,
    &wifi_task
  );
//  load_prefs();
//  start_wifi();
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(1000);
}