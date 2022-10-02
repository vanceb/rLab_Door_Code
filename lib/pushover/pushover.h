#ifndef PUSHOVER_H
#define PUSHOVER_H

#if FEATURE_PUSHOVER

#include <Arduino.h>

/* Pushover (https://pushover.net) */
#define PUSHOVER_DEFAULT_URL        "https://api.pushover.net/1/messages.json"
#define PUSHOVER_URL_MAX_LEN        64
#define PUSHOVER_USER_KEY_MAX_LEN   32
#define PUSHOVER_API_KEY_MAX_LEN    32
#define PUSHOVER_MAX_PAYLOAD_LEN    512
#define PUSHOVER_MAX_TITLE_LEN      64
#define PUSHOVER_MAX_BODY_LEN    384
#define PUSHOVER_MESSAGE_QUEUE_LEN  3  //Should be at least the number of tasks that want to use it


/* CA Certificate used by Pushover */
#define DIGICERT_ROOT_CA "-----BEGIN CERTIFICATE-----\n" \
"MIIFUTCCBDmgAwIBAgIQB5g2A63jmQghnKAMJ7yKbDANBgkqhkiG9w0BAQsFADBh\n" \
"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n" \
"d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD\n" \
"QTAeFw0yMDA3MTYxMjI1MjdaFw0yMzA1MzEyMzU5NTlaMFkxCzAJBgNVBAYTAlVT\n" \
"MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxMzAxBgNVBAMTKlJhcGlkU1NMIFRMUyBE\n" \
"ViBSU0EgTWl4ZWQgU0hBMjU2IDIwMjAgQ0EtMTCCASIwDQYJKoZIhvcNAQEBBQAD\n" \
"ggEPADCCAQoCggEBANpuQ1VVmXvZlaJmxGVYotAMFzoApohbJAeNpzN+49LbgkrM\n" \
"Lv2tblII8H43vN7UFumxV7lJdPwLP22qa0sV9cwCr6QZoGEobda+4pufG0aSfHQC\n" \
"QhulaqKpPcYYOPjTwgqJA84AFYj8l/IeQ8n01VyCurMIHA478ts2G6GGtEx0ucnE\n" \
"fV2QHUL64EC2yh7ybboo5v8nFWV4lx/xcfxoxkFTVnAIRgHrH2vUdOiV9slOix3z\n" \
"5KPs2rK2bbach8Sh5GSkgp2HRoS/my0tCq1vjyLJeP0aNwPd3rk5O8LiffLev9j+\n" \
"UKZo0tt0VvTLkdGmSN4h1mVY6DnGfOwp1C5SK0MCAwEAAaOCAgswggIHMB0GA1Ud\n" \
"DgQWBBSkjeW+fHnkcCNtLik0rSNY3PUxfzAfBgNVHSMEGDAWgBQD3lA1VtFMu2bw\n" \
"o+IbG8OXsj3RVTAOBgNVHQ8BAf8EBAMCAYYwHQYDVR0lBBYwFAYIKwYBBQUHAwEG\n" \
"CCsGAQUFBwMCMBIGA1UdEwEB/wQIMAYBAf8CAQAwNAYIKwYBBQUHAQEEKDAmMCQG\n" \
"CCsGAQUFBzABhhhodHRwOi8vb2NzcC5kaWdpY2VydC5jb20wewYDVR0fBHQwcjA3\n" \
"oDWgM4YxaHR0cDovL2NybDMuZGlnaWNlcnQuY29tL0RpZ2lDZXJ0R2xvYmFsUm9v\n" \
"dENBLmNybDA3oDWgM4YxaHR0cDovL2NybDQuZGlnaWNlcnQuY29tL0RpZ2lDZXJ0\n" \
"R2xvYmFsUm9vdENBLmNybDCBzgYDVR0gBIHGMIHDMIHABgRVHSAAMIG3MCgGCCsG\n" \
"AQUFBwIBFhxodHRwczovL3d3dy5kaWdpY2VydC5jb20vQ1BTMIGKBggrBgEFBQcC\n" \
"AjB+DHxBbnkgdXNlIG9mIHRoaXMgQ2VydGlmaWNhdGUgY29uc3RpdHV0ZXMgYWNj\n" \
"ZXB0YW5jZSBvZiB0aGUgUmVseWluZyBQYXJ0eSBBZ3JlZW1lbnQgbG9jYXRlZCBh\n" \
"dCBodHRwczovL3d3dy5kaWdpY2VydC5jb20vcnBhLXVhMA0GCSqGSIb3DQEBCwUA\n" \
"A4IBAQAi49xtSOuOygBycy50quCThG45xIdUAsQCaXFVRa9asPaB/jLINXJL3qV9\n" \
"J0Gh2bZM0k4yOMeAMZ57smP6JkcJihhOFlfQa18aljd+xNc6b+GX6oFcCHGr+gsE\n" \
"yPM8qvlKGxc5T5eHVzV6jpjpyzl6VEKpaxH6gdGVpQVgjkOR9yY9XAUlFnzlOCpq\n" \
"sm7r2ZUKpDfrhUnVzX2nSM15XSj48rVBBAnGJWkLPijlACd3sWFMVUiKRz1C5PZy\n" \
"el2l7J/W4d99KFLSYgoy5GDmARpwLc//fXfkr40nMY8ibCmxCsjXQTe0fJbtrrLL\n" \
"yWQlk9VDV296EI/kQOJNLVEkJ54P\n" \
"-----END CERTIFICATE-----\n"

/* Message Structure */
typedef struct Message {
    char title[PUSHOVER_MAX_TITLE_LEN];
    char body[PUSHOVER_MAX_BODY_LEN];
    int  priority;
} Message;

class Pushover
{
private:
    bool configured;
    QueueHandle_t msg_queue;
    char po_user_key[PUSHOVER_USER_KEY_MAX_LEN];
    char po_api_key [PUSHOVER_API_KEY_MAX_LEN];
    char po_api_url [PUSHOVER_URL_MAX_LEN];

public:
    Pushover(/* args */);
    ~Pushover();
    int configure(const char* user_key, const char* api_key, const char* url = PUSHOVER_DEFAULT_URL);
    bool is_configured();
    int _send_to_api(const char * title, const char * msg, int priority);
    int send(const char * title, const char * msg, int priority);
    int send(Message * msg);
    QueueHandle_t getQueue();
};

/* Global variable instance of the Pushover class */
extern Pushover pushover;

#endif  // FEATURE_PUSHOVER
/* Task to send queued messages */
void pushoverTask( void * pvParameters);

#endif