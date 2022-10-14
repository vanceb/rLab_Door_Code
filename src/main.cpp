#include <Arduino.h>
#include <WiFi.h>

#include <hardware.h>
#include <wifi_mgr.h>
#include <monitor.h>
#include <pi_control.h>
#include <etask_ota.h>

TaskHandle_t wifi_task;
TaskHandle_t monitorTaskHandle;
TaskHandle_t ota_task;

#if FEATURE_NFC
extern "C"
{
#include <pn532.h>
}
#include <rfid.h>
TaskHandle_t rfidTaskHandle;
#endif // FEATURE_NFC

#if FEATURE_PUSHOVER
#include <pushover.h>
TaskHandle_t pushoverTaskHandle;
#endif

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting setup");
  setup_gpio();

  /* Populate the id */
  uint64_t raw_id = ESP.getEfuseMac();
  const char *chip = ESP.getChipModel();
  snprintf(id, CHIP_ID_LEN, "%04X%08X", (uint16_t)(raw_id >> 32), (uint32_t)(raw_id));
  Serial.print("ID: ");
  Serial.println(id);

  /* Show the software version */
  Serial.print("Software Version: ");
  Serial.println(AUTO_VERSION);
  /* Show the project name */
  Serial.printf("Project Name: %s\n", PROJECT_NAME);

  /* Monitor Task - The main task for controlling the door and displays */
  xTaskCreate(monitorTask, "Monitor Task", 5000, NULL, 16, &monitorTaskHandle);

#if FEATURE_NFC
  //  pn532_t * nfc = pn532_init(Serial2, GPIO_TXDO_NFC, GPIO_RXDI_NFC, 0);
  pn532_t *nfc = pn532_init(2, 26, 27, 0);
  if (!nfc)
  {
    Serial.println("Failed to setup PN532 NFC Reader");
  }
  else
  {
    Serial.println("Successfully setup PN532 NFC Reader");
    xTaskCreate(rfidTask, "RFID Task", 5000, (void *)nfc, 8, &rfidTaskHandle);
  }
#endif // FEATURE_NFC

#if FEATURE_WIFI
  /* Start WiFi task */
  xTaskCreate(
      etask_wifi_mgr,
      "WiFi_Task",
      5000,
      NULL,
      0,
      &wifi_task);

#if FEATURE_OTA
  // Create a task to get OTA updates
  xTaskCreate(
      etask_ota,
      "OTA_Task",
      8000,
      NULL,
      0,
      &ota_task);
#endif

#if FEATURE_PUSHOVER
  /* Configure the pushover client */
  /* Must wait for credentials to be populated from SPIFFS - Done in wifi_mgr task...*/
  while (strlen(custom_PUSHOVER_API_KEY) == 0 ||
         strlen(custom_PUSHOVER_USERKEY) == 0 ||
         strlen(custom_PUSHOVER_API_URL) == 0)
  {
    delay(100);
  }
  pushover.begin(custom_PUSHOVER_USERKEY, custom_PUSHOVER_API_KEY, custom_PUSHOVER_API_URL);
  while (!pushover.is_configured())
  {
    delay(100);
  }
  char body[128];
  snprintf(body, 128, "ID: %s\nProject: %s\nVersion: %s", id, PROJECT_NAME, AUTO_VERSION);
  pushover.send("rLabDoor booting", body, -1);
#endif // FEATURE_PUSHOVER
#endif // FEATURE_WIFI
}

void loop()
{
  // put your main code here, to run repeatedly:
  delay(1000);
}