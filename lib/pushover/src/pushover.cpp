#include <Arduino.h>
#include <hardware.h>

#if FEATURE_PUSHOVER
#include <pushover.h>

#include <wifi_mgr.h>

#include <WiFi.h>
#include <HTTPClient.h>

Pushover pushover;

Pushover::Pushover()
{
    msg_queue = xQueueCreate(PUSHOVER_MESSAGE_QUEUE_LEN, sizeof(Message));
}

Pushover::~Pushover()
{
}

int Pushover::configure(const char * user_key, const char * api_key, const char * url) { 
    memset(po_user_key, 0, PUSHOVER_USER_KEY_MAX_LEN);
    memset(po_api_key,  0, PUSHOVER_API_KEY_MAX_LEN);
    memset(po_api_url,  0, PUSHOVER_URL_MAX_LEN);
    strlcpy(po_user_key, user_key, PUSHOVER_USER_KEY_MAX_LEN);
    strlcpy(po_api_key, api_key, PUSHOVER_API_KEY_MAX_LEN);
    strlcpy(po_api_url, url, PUSHOVER_URL_MAX_LEN);

    msg_queue = xQueueCreate(PUSHOVER_MESSAGE_QUEUE_LEN, sizeof(Message));

    log_d("Pushover: %s %s %s", po_user_key, po_api_key, po_api_url);
    /* Sanity check */
    if (strlen(po_user_key) > 0 &&
        strlen(po_api_key)  > 0 &&
        strlen(po_api_url)  > 0) {
            configured = true;
    } else {
        configured = false;
    }
    return configured;
}


bool Pushover::is_configured()
{
    return configured;
}

/* Actually perform the send to the Pushover API */
int Pushover::_send_to_api(const char *title, const char *msg, int priority)
{
    int timeout = 10;
    int httpCode = 0;
    if (is_configured())
    {
        while ((WiFi.status() != WL_CONNECTED) && timeout > 0)
        {
            delay(1000);
            timeout--;
        }
        if (timeout < 0)
        {
            log_w("Failed to send message to pushover, wifi down!");
            return -2;
        }
        else
        {
            /* Send the message */
            char p[3];
            if (priority < -2 || priority > 2)
            {
                log_w("Priority out of range (%d), must be -2 <= priority <= 2", priority);
                itoa(0, p, 10);
            }
            else
            {
                itoa(priority, p, 10);
            }

            int max_msg_len = PUSHOVER_MAX_PAYLOAD_LEN - (106 + strlen(title));
            char payload[PUSHOVER_MAX_PAYLOAD_LEN];
            strcpy(payload, "token=");                // 6 bytes
            strcat(payload, po_api_key);              // 32 bytes
            strcat(payload, "&user=");                // 6 bytes
            strcat(payload, po_user_key);             // 32 bytes
            strcat(payload, "&title=");               // 7 bytes
            strcat(payload, title);                   // Unknown
            strcat(payload, "&priority=");            // 10 bytes
            strcat(payload, p);                       // 2 bytes
            strcat(payload, "&message=");             // 9 bytes
            strncat(payload, msg, max_msg_len);

            log_d("%s", payload);

            HTTPClient https;
            https.begin(po_api_url, DIGICERT_ROOT_CA);
            https.addHeader("Content-Type", "application/x-www-form-urlencoded");
            httpCode = https.POST(payload);
        }
        return httpCode;
    }
    log_w("Attempt to send message to pushover, but it has not been configured");
    return -1;
}

int Pushover::send(const char *title, const char *body, int priority)
{
    Message msg;
    strlcpy(msg.title, title, PUSHOVER_MAX_TITLE_LEN);
    strlcpy(msg.body, body, PUSHOVER_MAX_BODY_LEN);
    msg.priority = priority;
    return xQueueSendToBack(this->msg_queue, &msg, 1);
}

int Pushover::send(Message *msg)
{
    return xQueueSendToBack(this->msg_queue, msg, 1);
}

QueueHandle_t Pushover::getQueue()
{
    return msg_queue;
}

#endif // FEATURE_PUSHOVER

void pushoverTask(void *pvParameters)
{
#if FEATURE_PUSHOVER
    /* Configure the pushover client */
    /* Must wait for credentials to be populated from SPIFFS - Done in wifi_mgr task...*/
    while (strlen(custom_PUSHOVER_API_KEY) == 0 ||
           strlen(custom_PUSHOVER_USERKEY) == 0 ||
           strlen(custom_PUSHOVER_API_URL) == 0) 
    {
        delay(100);
    }
    pushover.configure(custom_PUSHOVER_USERKEY, custom_PUSHOVER_API_KEY, custom_PUSHOVER_API_URL);
    /* Then get the variables needed for the main task loop */
    QueueHandle_t queue;
    queue = pushover.getQueue();
    Message msg;

#endif // FEATURE_PUSHOVER

    for (;;)
    {
#if FEATURE_PUSHOVER
        /* Check queue for message */
        if (xQueueReceive(queue, (void *)&msg, 0))
        {
            /* Send the message to the Pushover API */
            log_i("Pushover sender got msg: %s: %s", msg.title, msg.body);
            pushover._send_to_api(msg.title, msg.body, msg.priority);
        }
#endif // FEATURE_PUSHOVER
        delay(10);
    }
}