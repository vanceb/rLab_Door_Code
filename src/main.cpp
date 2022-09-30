#include <Arduino.h>
#include <WiFi.h>

#include <hardware.h>
#include <wifi_mgr.h>
#include <monitor.h>
#include <pi_control.h>

TaskHandle_t wifi_task;
TaskHandle_t monitorTaskHandle;

#if FEATURE_PUSHOVER
#include <pushover.h>
Pushover pushover;
TaskHandle_t pushoverTaskHandle;
#endif

void setup() {
  Serial.begin(115200);
  Serial.println("Starting setup");
  setup_gpio();

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


#if FEATURE_PUSHOVER
  xTaskCreate(pushoverTask, "Pushover Task", 8000, (void*) &pushover, 8, &pushoverTaskHandle);
  int wifi_wait = 10;
  while ((--wifi_wait >= 0) && (WiFi.status() != WL_CONNECTED))
    delay(1000);
  if(WiFi.status() == WL_CONNECTED) { 
    QueueHandle_t po_queue = pushover.getQueue();
    Message msg;
    strlcpy(msg.title, "rLabDoor Booting", PUSHOVER_MAX_TITLE_LEN);
    strlcpy(msg.body, "Coming up...", PUSHOVER_MAX_MESSAGE_LEN);
    msg.priority = -1;
    xQueueSendToBack(po_queue, &msg, 1);
  } else {
    log_e("Wifi not connected, unable to send booting Pushover message!");
  }

#endif  // FEATURE_PUSHOVER
#endif  // FEATURE_WIFI
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(1000);
}