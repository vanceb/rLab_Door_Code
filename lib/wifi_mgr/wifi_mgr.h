#ifndef WIFI_MGR_H
#define WIFI_MGR_H

#include <Arduino.h>
#include <hardware.h>

#define WIFI_MGR_DEBUG          1
#define WIFI_MGR_RESET          0

// From v1.1.0
// You only need to format the filesystem once
//#define FORMAT_FILESYSTEM       true
#define FORMAT_FILESYSTEM       false

#define MIN_AP_PASSWORD_SIZE    8

#define SSID_MAX_LEN            32
//From v1.0.10, WPA2 passwords can be up to 63 characters long.
#define PASS_MAX_LEN            64

#define NUM_WIFI_CREDENTIALS    1

// Assuming max 490 chars
#define TZNAME_MAX_LEN          50
#define TIMEZONE_MAX_LEN        50

// Use false if you don't like to display Available Pages in Information Page of Config Portal
// Comment out or use true to display Available Pages in Information Page of Config Portal
// Must be placed before #include <ESP_WiFiManager.h>
#define USE_AVAILABLE_PAGES     false

// From v1.0.10 to permit disable/enable StaticIP configuration in Config Portal from sketch. Valid only if DHCP is used.
// You'll loose the feature of dynamically changing from DHCP to static IP, or vice versa
// You have to explicitly specify false to disable the feature.
//#define USE_STATIC_IP_CONFIG_IN_CP          false

// Use false to disable NTP config. Advisable when using Cellphone, Tablet to access Config Portal.
// See Issue 23: On Android phone ConfigPortal is unresponsive (https://github.com/khoih-prog/ESP_WiFiManager/issues/23)
#define USE_ESP_WIFIMANAGER_NTP     true

// Just use enough to save memory. On ESP8266, can cause blank ConfigPortal screen
// if using too much memory
#define USING_AFRICA        false
#define USING_AMERICA       false
#define USING_ANTARCTICA    false
#define USING_ASIA          false
#define USING_ATLANTIC      false
#define USING_AUSTRALIA     false
#define USING_EUROPE        true
#define USING_INDIAN        false
#define USING_PACIFIC       false
#define USING_ETC_GMT       false

// Use true to enable CloudFlare NTP service. System can hang if you don't have Internet access while accessing CloudFlare
// See Issue #21: CloudFlare link in the default portal (https://github.com/khoih-prog/ESP_WiFiManager/issues/21)
#define USE_CLOUDFLARE_NTP          false

// New in v1.0.11
#define USING_CORS_FEATURE          true

#if FEATURE_PUSHOVER
/* Pushover */

/* Default values */
#define PUSHOVER_CONFIG_FILE        "/pushover.json"
#define PUSHOVER_API_URL            "https://api.pushover.net/1/messages.json"
#define PUSHOVER_USERKEY            "private"
#define PUSHOVER_APIKEY             "private"

// Labels for custom parameters in WiFi manager
#define PUSHOVER_API_URL_Label      "PUSHOVER_API_URL"
#define PUSHOVER_USERKEY_Label      "PUSHOVER_USERKEY"
#define PUSHOVER_API_KEY_Label      "PUSHOVER_API_KEY"

/* Pushover - Variables to be populated from config settings */
#define custom_PUSHOVER_API_URL_LEN     40
#define custom_PUSHOVER_USERKEY_LEN     32
#define custom_PUSHOVER_API_KEY_LEN     32

extern char custom_PUSHOVER_API_URL[custom_PUSHOVER_API_URL_LEN];
extern char custom_PUSHOVER_USERKEY[custom_PUSHOVER_API_KEY_LEN];
extern char custom_PUSHOVER_API_KEY[custom_PUSHOVER_USERKEY_LEN];

#endif //FEATURE_PUSHOVER

void etask_wifi_mgr (void * parameters);

#endif