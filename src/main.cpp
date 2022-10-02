#include <Arduino.h>
#include <WiFi.h>

#include <hardware.h>
#include <wifi_mgr.h>
#include <monitor.h>
#include <pi_control.h>

TaskHandle_t wifi_task;
TaskHandle_t monitorTaskHandle;

#if FEATURE_NFC
extern "C" {
  #include <pn532.h>
}
#include <rfid.h>
TaskHandle_t rfidTaskHandle;
#endif  // FEATURE_NFC

#if FEATURE_PUSHOVER
#include <pushover.h>
TaskHandle_t pushoverTaskHandle;
#endif

void setup() {
  Serial.begin(115200);
  Serial.println("Starting setup");
  setup_gpio();

  #if FEATURE_PI
  /* Check the Raspberry Pi */
  Pi rpi;
  rpi.begin(
    GPIO_PI_POWERED,
    GPIO_PI_HEARTBEAT,
    GPIO_PI_OPEN_CMD_1,
    GPIO_PI_OPEN_CMD_2
  );
  delay(1000);  // give the ISR chance to fire
  if(rpi.is_connected()) {
    log_i("Detected Raspberry Pi");
  } else {
    log_w("No Raspberry Pi detected");
  }
  if(rpi.is_alive()) {
    log_i("Detected Pi Heartbeat - service running");
  } else {
    log_w("No Heartbeat detected from the Pi - check if service is running");
  }
#endif  // FEATURE_PI


/* Monitor Task - The main task for controlling the door and displays */
  xTaskCreate(monitorTask, "Monitor Task", 5000, NULL, 16, &monitorTaskHandle);


#if FEATURE_NFC
//  pn532_t * nfc = pn532_init(Serial2, GPIO_TXDO_NFC, GPIO_RXDI_NFC, 0);
  pn532_t * nfc = pn532_init(2, 26, 27, 0);
  if (!nfc) {
    Serial.println("Failed to setup PN532 NFC Reader");
  } else {
    Serial.println("Successfully setup PN532 NFC Reader");
    xTaskCreate(rfidTask, "RFID Task", 5000, (void*) nfc, 8, &rfidTaskHandle);
  }
#endif  // FEATURE_NFC

#if FEATURE_WIFI
  /* Start WiFi task */
  xTaskCreate(
    etask_wifi_mgr,
    "WiFi_Task",
    5000,
    NULL,
    0,
    &wifi_task
  );

#if FEATURE_PUSHOVER
  xTaskCreate(pushoverTask, "Pushover Task", 8000, (void*) &pushover, 8, &pushoverTaskHandle);
  while (!pushover.is_configured()) {
    delay(100);
  }
  pushover.send("rLabDoor booting", "Coming up!", 0);
#endif  // FEATURE_PUSHOVER
#endif  // FEATURE_WIFI
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(1000);
}