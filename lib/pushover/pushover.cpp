#include <Arduino.h>
#include <hardware.h>

#if FEATURE_PUSHOVER
#include <pushover.h>

#include <wifi_mgr.h>

#include <WiFi.h>
#include <HTTPClient.h>

Pushover::Pushover()
{
    msg_queue = xQueueCreate(PUSHOVER_MESSAGE_QUEUE_LEN, sizeof(Message));
}

Pushover::~Pushover()
{
}

bool Pushover::is_configured() {
    if((strlen(custom_PUSHOVER_API_KEY) > 0) &&
       (strlen(custom_PUSHOVER_API_URL) > 0) &&
       (strlen(custom_PUSHOVER_USERKEY) > 0)) {
        return true;
    } else {
        return false;
    }
}

int Pushover::send(const char * title, const char * msg, int priority) {
    int timeout =  5;
    int httpCode = 0;
    if(is_configured()) {
        while((WiFi.status() != WL_CONNECTED) && timeout > 0) {
            delay(1000);
            timeout--;
        }
        if(timeout < 0) {
            log_w("Failed to send message to pushover, wifi down!");
            return -2;
        } else {
            /* Send the message */
            char p[3];
            if (priority < -2 || priority > 2) {
                log_w("Priority out of range (%d), must be -2 <= priority <= 2", priority);
                itoa(0, p, 10);
            } else {
                itoa(priority, p, 10);
            }
            
            int max_msg_len = PUSHOVER_MAX_PAYLOAD_LEN - (106 + strlen(title));
            char payload[PUSHOVER_MAX_PAYLOAD_LEN];
            strcpy(payload, "token=");                  // 6 bytes
            strcat(payload, custom_PUSHOVER_API_KEY);   // 32 bytes
            strcat(payload, "&user=");                  // 6 bytes
            strcat(payload, custom_PUSHOVER_USERKEY);   // 32 bytes
            strcat(payload, "&title=");                 // 7 bytes
            strcat(payload, title);                     // Unknown
            strcat(payload, "&priority=");              // 10 bytes
            strcat(payload, p);                         // 2 bytes
            strcat(payload, "&message=");               // 9 bytes
            strncat(payload, msg, max_msg_len);

            log_d("%s", payload);

            HTTPClient https;
            https.begin(custom_PUSHOVER_API_URL, DIGICERT_ROOT_CA);
            https.addHeader("Content-Type", "application/x-www-form-urlencoded");
            httpCode = https.POST(payload);

        }
        return httpCode;
    }
    log_w("Attempt to send message to pushover, but it has not been configured");
    return -1;
}

int Pushover::send(Message * msg) {
    return send(msg->title, msg->body, msg->priority);
}

QueueHandle_t Pushover::getQueue() {
    return msg_queue;
}

#endif  // FEATURE_PUSHOVER

void pushoverTask( void * pvParameters) {
#if FEATURE_PUSHOVER
    /* Cast the incoming parameter to a Pushover object */
    Pushover * po;
    po = (Pushover*) pvParameters;
    /* Then get its queue */
    QueueHandle_t queue;
    queue = po->getQueue();
    Message msg;

#endif  // FEATURE_PUSHOVER

    for (;;) {
#if FEATURE_PUSHOVER
        /* Check queue for message */
        if (xQueueReceive(queue, (void*) &msg, 0)) {
            log_d("Pushover sender got msg: %s", msg.title);
            po->send(&msg);
        }
#endif  // FEATURE_PUSHOVER
        delay(100);
    }
}