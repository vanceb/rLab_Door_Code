#include <hardware.h>

#include <WiFi.h>

Preferences prefs;

char wifi_ssid[WIFI_SSID_MAX_LEN] = {0};
char wifi_passwd[WIFI_PASSWD_MAX_LEN] = {0};

void setup_gpio() {
    log_i("Setting up GPIO pins");

    #if FEATURE_PI
    pinMode(GPIO_PI_HEARTBEAT,  INPUT);
    pinMode(GPIO_PI_POWERED,    INPUT);
    pinMode(GPIO_PI_OPEN_CMD_1, INPUT);
    pinMode(GPIO_PI_OPEN_CMD_2, INPUT);
    log_i("GPIO Raspberry Pi control configured");
    #endif  // FEATURE_PI

    #if FEATURE_TAMPER
    pinMode(GPIO_TAMPER,        INPUT);
    log_i("GPIO Tamper input enabled");
    #endif  // FEATURE_TAMPER

    #if FEATURE_ADC
    pinMode(GPIO_ADC_VIN,       INPUT);
    pinMode(GPIO_ADC_VBATT,     INPUT);
    pinMode(GPIO_ADC_V5,        INPUT);
    pinMode(GPIO_ADC_V3,        INPUT);
    log_i("GPIO ADC monitoring of system voltages enabled");
    #endif  // FEATURE_ADC

    #if FEATURE_KEYPAD
    pinMode(GPIO_WGD0,          INPUT);
    pinMode(GPIO_WGD1,          INPUT);
    log_i("GPIO Wiegand keypad pins enabled");
    #endif  // FEATURE_KEYPAD
    
    #if FEATURE_NEOPIXELS
    pinMode(GPIO_NPX_1,         OUTPUT);
    pinMode(GPIO_NPX_2,         OUTPUT);
    log_i("GPIO Neopixels enabled");
    #endif  // FEATURE_NEOPIXELS

    pinMode(GPIO_OPEN_1,        OUTPUT);
    pinMode(GPIO_OPEN_2,        OUTPUT);
    log_i("GPIO Open outputs enabled");
}

void load_prefs()
{
    log_i("Loading preferences from flash");
    prefs.begin(PREFS_NS);
    /* Wifi */
    #if FEATURE_WIFI
    if (prefs.isKey(PREFS_WIFI_SSID_KEY) && prefs.isKey(PREFS_WIFI_PWD_KEY)) {
        /* Get the stored ssid and passwd */
        prefs.getString(PREFS_WIFI_SSID_KEY, wifi_ssid, WIFI_SSID_MAX_LEN);
        prefs.getString(PREFS_WIFI_PWD_KEY, wifi_passwd, WIFI_PASSWD_MAX_LEN);
        log_i("Found WiFi credentials in flash for %s", wifi_ssid);
    } else {
        log_w("No WiFi credentials found in flash - please configure");
    }
    #endif  // FEATURE_WIFI

    /* Check pushover */
    #if FEATURE_PUSHOVER
    if (!pushover.configure()) {
        configured &= ~(FEATURE_PUSHOVER);
    }
    #endif  // FEATURE_PUSHOVER
}

#if FEATURE_WIFI
int configure_wifi(char * ssid, char * passwd) {
    WiFi.begin(ssid, passwd);
    int seconds = 10;
    while (seconds > 0 && WiFi.status() != WL_CONNECTED) {
        seconds--;
        delay(1000);
    } 
    if (WiFi.status() == WL_CONNECTED) {
        /* Store SSID and password */
        prefs.putString(PREFS_WIFI_SSID_KEY, ssid);
        prefs.putString(PREFS_WIFI_PWD_KEY,  passwd);
        /* reload preferences from flash */
        load_prefs();
        return 1;
    } else {
        return 0;
    }
}

void start_wifi(uint16_t timeout_secs)
{
    /* Start wifi if configured */
    log_i("Starting WiFi on ssid: %s", wifi_ssid);
    if (strlen(wifi_ssid) > 0 && strlen(wifi_passwd) > 0)
    {
        /* Connect to wifi */
        log_d("Connecting to wifi: %s", wifi_ssid);
        if(!WiFi.begin(wifi_ssid, wifi_passwd)) {
            log_e("Problem starting up wifi - maybe DNS?");
        } 

        /* Give it up to 10 seconds to join then continue */
        while (timeout_secs > 0 && WiFi.status() != WL_CONNECTED)
        {
            timeout_secs--;
            delay(1000);
        }
        if (WiFi.status() == WL_CONNECTED)
        {
            log_i("Connected to wifi ssid: %s", wifi_ssid);
        }
        else
        {
            log_w("Didn't connect to wifi ssid %s", wifi_ssid);
        }
    }
    else
    {
        log_w("Wifi not configured");
    }
}
#endif  // FEATURE_WIFI