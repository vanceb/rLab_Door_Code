#ifndef HARDWARE_H
#define HARDWARE_H

#include <Preferences.h>

/* Enable Features */
#define FEATURE_WIFI        1   // Enable wifi
#define FEATURE_PI          1   // Enable control by Pi (GPIO)
#define FEATURE_PI_SERIAL   0   // Enable serial communication with Pi (Not Implemented)
#define FEATURE_NFC         0   // Enable NFC card reader using PN532 (Not Implemented)
#define FEATURE_TAMPER      1   // Enable tamper detection
#define FEATURE_ADC         1   // Enable voltage monitoring using the ADC
#define FEATURE_NEOPIXELS   1   // Enable neopixel display
#define FEATURE_DISPLAY     1   // Enable character display
#define FEATURE_KEYPAD      0   // Enable Wiegand keypad for PIN entry
#define FEATURE_PUSHOVER    1   // Enable Pushover messages (Requires WiFi)


/* GPIO config */

/* UARTs */
/* Serial - Standard pinouts for programming/logging */
#define GPIO_TXDO_PROG       1
#define GPIO_RXDI_PROG       3

#if FEATURE_PI
/* Raspberry Pi */
#define GPIO_PI_HEARTBEAT    16
#define GPIO_PI_POWERED      17
#define GPIO_PI_OPEN_CMD_1   23
#define GPIO_PI_OPEN_CMD_2   25

/* Serial1 - Raspberry Pi Serial Interface */
#if FEATURE_PI_SERIAL
#define GPIO_TXDO_PI         18
#define GPIO_RXDI_PI         19
#endif  // FEATURE_PI_SERIAL
#endif  // FEATURE_PI

/* Serial2 - NFC Serial Interface */
#if FEATURE_NFC
#define GPIO_TXDO_NFC        26
#define GPIO_RXDI_NFC        27
#endif  // FEATURE_NFC

/* Monitoring */
#if FEATURE_TAMPER 
#define GPIO_TAMPER          15
#endif  // FEATURE_TAMPER
#if FEATURE_ADC
#define GPIO_ADC_VIN         34
#define GPIO_ADC_VBATT       35
#define GPIO_ADC_V5          32
#define GPIO_ADC_V3          33
#endif  // FEATURE_ADC

/* Display */
#if FEATURE_NEOPIXELS
/* Neopixels */
#define GPIO_NPX_1           5
#define GPIO_NPX_2           4
#define NPX_NUM_LEDS_1      6
#define NPX_NUM_LEDS_2      0
#endif  // FEATURE_NEOPIXELS

#if FEATURE_DISPLAY
/* I2C Character Display */
#define GPIO_SDA_DISP        21
#define GPIO_SCL_DISP        22
/* I2C Character Display */
#define DISP_ADDR           0x27
#define DISP_ROWS           4
#define DISP_COLS           20
#define SSD_ADDR            0x3C
#define SSD_ROWS            64
#define SSD_COLS            128
#endif  // FEATURE_DISPLAY

/* Control */
#define GPIO_OPEN_1          12
#define GPIO_OPEN_2          2

/* Keypad */
#if FEATURE_KEYPAD
#define GPIO_WGD0            14
#define GPIO_WGD1            13
#endif  // FEATURE_KEYPAD

/* Wifi */
#if FEATURE_WIFI
#define WIFI_SSID_MAX_LEN   32
#define WIFI_PASSWD_MAX_LEN 64
#endif

/* Preferences stored in Flash */
extern Preferences prefs;
#define PREFS_NS            "rlabDoor"      // Preferences namespace
#define PREFS_WIFI_SSID_KEY "wifi_ssid"     // Key for storing ssid
#define PREFS_WIFI_PWD_KEY  "wifi_passwd"   // Key for storing wifi passwd


void setup_gpio();
void load_prefs();
int configure_wifi(char * ssid, char * passwd);
void start_wifi(uint16_t timeout_secs = 10);

#endif