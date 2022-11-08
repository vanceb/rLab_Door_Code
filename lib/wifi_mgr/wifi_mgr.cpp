
/****************************************************************************************************************************
  ConfigOnSwitch.ino
  For ESP8266 / ESP32 boards

  ESP_WiFiManager is a library for the ESP8266/ESP32 platform (https://github.com/esp8266/Arduino) to enable easy
  configuration and reconfiguration of WiFi credentials using a Captive Portal. Inspired by:
  http://www.esp8266.com/viewtopic.php?f=29&t=2520
  https://github.com/chriscook8/esp-arduino-apboot
  https://github.com/esp8266/Arduino/blob/master/libraries/DNSServer/examples/CaptivePortalAdvanced/

  Modified from Tzapu https://github.com/tzapu/WiFiManager
  and from Ken Taylor https://github.com/kentaylor

  Built by Khoi Hoang https://github.com/khoih-prog/ESP_WiFiManager
  Licensed under MIT license
 *****************************************************************************************************************************/
/****************************************************************************************************************************
   This example will open a configuration portal when no WiFi configuration has been previously entered or when a button is pushed.
   It is the easiest scenario for configuration but requires a pin and a button on the ESP8266 device.
   The Flash button is convenient for this on NodeMCU devices.
   Also in this example a password is required to connect to the configuration portal
   network. This is inconvenient but means that only those who know the password or those
   already connected to the target WiFi network can access the configuration portal and
   the WiFi network credentials will be sent from the browser over an encrypted connection and
   can not be read by observers.
 *****************************************************************************************************************************/

#include <hardware.h>
#include <wifi_mgr.h>

#include <esp_wifi.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>

// From v1.1.0
#include <WiFiMulti.h>
WiFiMulti wifiMulti;

#define ESP_getChipId() (ESP.getEfuseMac())

/* Use SPIFFS for storing config data */
#include <SPIFFS.h>
FS *filesystem = &SPIFFS;
#define FileFS SPIFFS
#define FS_Name "SPIFFS"

#if FEATURE_PUSHOVER
/* Pushover - Variables to be populated from config settings */
char custom_PUSHOVER_API_URL[custom_PUSHOVER_API_URL_LEN] = { 0 };
char custom_PUSHOVER_USERKEY[custom_PUSHOVER_USERKEY_LEN] = { 0 };
char custom_PUSHOVER_API_KEY[custom_PUSHOVER_API_KEY_LEN] = { 0 };
#endif  // FEATURE_PUSHOVER

typedef struct
{
    char wifi_ssid[SSID_MAX_LEN];
    char wifi_pw[PASS_MAX_LEN];
} WiFi_Credentials;

typedef struct
{
    String wifi_ssid;
    String wifi_pw;
} WiFi_Credentials_String;

typedef struct
{
    WiFi_Credentials WiFi_Creds[NUM_WIFI_CREDENTIALS];
    char TZ_Name[TZNAME_MAX_LEN]; // "America/Toronto"
    char TZ[TIMEZONE_MAX_LEN];    // "EST5EDT,M3.2.0,M11.1.0"
    uint16_t checksum;
} WM_Config;

WM_Config WM_config;

#define CONFIG_FILENAME F("/wifi_cred.dat")
//////

// Should we start the config portal?
bool start_portal = false;

////////////////////////////////////////////

IPAddress stationIP = IPAddress(0, 0, 0, 0);
IPAddress gatewayIP = IPAddress(192, 168, 2, 1);
IPAddress netMask = IPAddress(255, 255, 255, 0);

////////////////////////////////////////////

IPAddress dns1IP = gatewayIP;
IPAddress dns2IP = IPAddress(8, 8, 8, 8);

// New in v1.4.0
IPAddress APStaticIP = IPAddress(192, 168, 100, 1);
IPAddress APStaticGW = IPAddress(192, 168, 100, 1);
IPAddress APStaticSN = IPAddress(255, 255, 255, 0);

// Must be placed before #include <ESP_WiFiManager.h>, or default port 80 will be used
//#define HTTP_PORT     8080

#include <ESP_WiFiManager.h> //https://github.com/khoih-prog/ESP_WiFiManager

// SSID and PW for Config Portal
String ssid = "ESP_" + String(ESP_getChipId(), HEX);
String password;

// SSID and PW for your Router
String Router_SSID;
String Router_Pass;

// Function Prototypes
uint8_t connectMultiWiFi();

WiFi_AP_IPConfig WM_AP_IPconfig;
WiFi_STA_IPConfig WM_STA_IPconfig;

void initAPIPConfigStruct(WiFi_AP_IPConfig &in_WM_AP_IPconfig)
{
    in_WM_AP_IPconfig._ap_static_ip = APStaticIP;
    in_WM_AP_IPconfig._ap_static_gw = APStaticGW;
    in_WM_AP_IPconfig._ap_static_sn = APStaticSN;
}

void initSTAIPConfigStruct(WiFi_STA_IPConfig &in_WM_STA_IPconfig)
{
    in_WM_STA_IPconfig._sta_static_ip = stationIP;
    in_WM_STA_IPconfig._sta_static_gw = gatewayIP;
    in_WM_STA_IPconfig._sta_static_sn = netMask;
}

void displayIPConfigStruct(WiFi_STA_IPConfig in_WM_STA_IPconfig)
{
    LOGERROR3(F("stationIP ="), in_WM_STA_IPconfig._sta_static_ip, ", gatewayIP =", in_WM_STA_IPconfig._sta_static_gw);
    LOGERROR1(F("netMask ="), in_WM_STA_IPconfig._sta_static_sn);
}

void configWiFi(WiFi_STA_IPConfig in_WM_STA_IPconfig)
{
    // Set static IP, Gateway, Subnetmask, Use auto DNS1 and DNS2.
    WiFi.config(in_WM_STA_IPconfig._sta_static_ip, in_WM_STA_IPconfig._sta_static_gw, in_WM_STA_IPconfig._sta_static_sn);
}

///////////////////////////////////////////

uint8_t connectMultiWiFi()
{
// For ESP32, this better be 0 to shorten the connect time.
// For ESP32-S2/C3, must be > 500
#if (USING_ESP32_S2 || USING_ESP32_C3)
#define WIFI_MULTI_1ST_CONNECT_WAITING_MS 500L
#else
// For ESP32 core v1.0.6, must be >= 500
#define WIFI_MULTI_1ST_CONNECT_WAITING_MS 800L
#endif

#define WIFI_MULTI_CONNECT_WAITING_MS 500L

    uint8_t status;

    log_i("ConnectMultiWiFi with :");
    /*
        if ((Router_SSID != "") && (Router_Pass != ""))
        {
            log_i("Flash credentials found: SSID: %s", Router_SSID);
            wifiMulti.addAP(Router_SSID.c_str(), Router_Pass.c_str());
        }
    */
    for (uint8_t i = 0; i < NUM_WIFI_CREDENTIALS; i++)
    {
        // Don't permit NULL SSID and password len < MIN_AP_PASSWORD_SIZE (8)
        if ((String(WM_config.WiFi_Creds[i].wifi_ssid) != "") && (strlen(WM_config.WiFi_Creds[i].wifi_pw) >= MIN_AP_PASSWORD_SIZE))
        {
            log_i("Config credentials found: SSID: %s", WM_config.WiFi_Creds[i].wifi_ssid);
            wifiMulti.addAP(WM_config.WiFi_Creds[i].wifi_ssid, WM_config.WiFi_Creds[i].wifi_pw);
        }
    }

    int i = 0;
    status = wifiMulti.run();
    delay(WIFI_MULTI_1ST_CONNECT_WAITING_MS);

    while ((i++ < 20) && (status != WL_CONNECTED))
    {
        status = WiFi.status();

        if (status == WL_CONNECTED)
            break;
        else
            delay(WIFI_MULTI_CONNECT_WAITING_MS);
    }

    if (status == WL_CONNECTED)
    {
        log_i("WiFi connected: ");
        log_i("SSID: %s, RSSI: %d", WiFi.SSID(), WiFi.RSSI());
        log_i("Channel: %d, IP: %s", WiFi.channel(), WiFi.localIP().toString().c_str());
    }
    else
    {
        log_e("WiFi not connected");
        //        ESP.restart();
    }
    return status;
}

#if USE_ESP_WIFIMANAGER_NTP

void printLocalTime()
{
    struct tm timeinfo;

    getLocalTime(&timeinfo, NTP_TIMEOUT_MS);
    //getLocalTime(&timeinfo, 1);

    // Valid only if year > 2000.
    // You can get from timeinfo : tm_year, tm_mon, tm_mday, tm_hour, tm_min, tm_sec
    if (timeinfo.tm_year > 100)
    {
        log_i("Local Date/Time: %s", asctime(&timeinfo));
    }
    else
    {
        log_e("NTP time not set");
    }
}

#endif

void heartBeatPrint()
{
#if USE_ESP_WIFIMANAGER_NTP
    printLocalTime();
#else
    static int num = 1;

    if (WiFi.status() == WL_CONNECTED)
        Serial.print(F("H")); // H means connected to WiFi
    else
        Serial.print(F("F")); // F means not connected to WiFi

    if (num == 80)
    {
        Serial.println();
        num = 1;
    }
    else if (num++ % 10 == 0)
    {
        Serial.print(F(" "));
    }
#endif
}

void check_WiFi()
{
    if ((WiFi.status() != WL_CONNECTED))
    {
        log_e("WiFi lost. Attempting to reconnect...");
        connectMultiWiFi();
    }
}

void check_status()
{
    static ulong checkstatus_timeout = 0;
    static ulong checkwifi_timeout = 0;

    static ulong current_millis;

#define WIFICHECK_INTERVAL 1000L

#if USE_ESP_WIFIMANAGER_NTP
#define HEARTBEAT_INTERVAL 60000L
#else
#define HEARTBEAT_INTERVAL 10000L
#endif

    current_millis = millis();

    // Check WiFi every WIFICHECK_INTERVAL (1) seconds.
    if ((current_millis > checkwifi_timeout) || (checkwifi_timeout == 0))
    {
        check_WiFi();
        checkwifi_timeout = current_millis + WIFICHECK_INTERVAL;
    }

    // Print hearbeat every HEARTBEAT_INTERVAL (10) seconds.
    if ((current_millis > checkstatus_timeout) || (checkstatus_timeout == 0))
    {
        heartBeatPrint();
        checkstatus_timeout = current_millis + HEARTBEAT_INTERVAL;
    }
}

int calcChecksum(uint8_t *address, uint16_t sizeToCalc)
{
    uint16_t checkSum = 0;

    for (uint16_t index = 0; index < sizeToCalc; index++)
    {
        checkSum += *(((byte *)address) + index);
    }

    return checkSum;
}

bool loadWifiConfig()
{
    log_i("Loading config file from SPIFFS");
    File file = FileFS.open(CONFIG_FILENAME, "r");

    memset((void *)&WM_config, 0, sizeof(WM_config));

    // New in v1.4.0
    memset((void *)&WM_STA_IPconfig, 0, sizeof(WM_STA_IPconfig));
    //////

    if (file)
    {
        file.readBytes((char *)&WM_config, sizeof(WM_config));

        // New in v1.4.0
        file.readBytes((char *)&WM_STA_IPconfig, sizeof(WM_STA_IPconfig));
        //////

        file.close();
        log_i("Loaded config data from file");

        if (WM_config.checksum != calcChecksum((uint8_t *)&WM_config, sizeof(WM_config) - sizeof(WM_config.checksum)))
        {
            log_e("Config file checksum incorrect");
            return false;
        }
        displayIPConfigStruct(WM_STA_IPconfig);
        return true;
    }
    else
    {
        log_e("Unable to load config file from SPIFFS");
        return false;
    }
}

void saveWifiConfig()
{
    File file = FileFS.open(CONFIG_FILENAME, "w");
    log_i("Saving config file to SPIFFS");

    if (file)
    {
        WM_config.checksum = calcChecksum((uint8_t *)&WM_config, sizeof(WM_config) - sizeof(WM_config.checksum));

        file.write((uint8_t *)&WM_config, sizeof(WM_config));

        displayIPConfigStruct(WM_STA_IPconfig);

        // New in v1.4.0
        file.write((uint8_t *)&WM_STA_IPconfig, sizeof(WM_STA_IPconfig));
        //////

        file.close();
        log_i("Saved config data to file");
    }
    else
    {
        log_e("Unable to save config file to SPIFFS");
    }
}

#if FEATURE_PUSHOVER
bool loadPushoverConfig() 
{
  // this opens the config file in read-mode
  File f = FileFS.open(PUSHOVER_CONFIG_FILE, "r");

  if (!f)
  {
    log_e("Config File not found");
    return false;
  }
  else
  {
    // we could open the file
    size_t size = f.size();
    // Allocate a buffer to store contents of the file.
    std::unique_ptr<char[]> buf(new char[size + 1]);

    // Read and store file contents in buf
    f.readBytes(buf.get(), size);
    // Closing file
    f.close();
    // Using dynamic JSON buffer which is not the recommended memory model, but anyway
    // See https://github.com/bblanchon/ArduinoJson/wiki/Memory%20model

#if (ARDUINOJSON_VERSION_MAJOR >= 6)

    DynamicJsonDocument json(1024);
    auto deserializeError = deserializeJson(json, buf.get());
    
    if ( deserializeError )
    {
      log_e("JSON parseObject() failed");
      return false;
    }
    
//    serializeJson(json, Serial);
    
#else

    DynamicJsonBuffer jsonBuffer;
    // Parse JSON string
    JsonObject& json = jsonBuffer.parseObject(buf.get());
    
    // Test if parsing succeeds.
    if (!json.success())
    {
      Serial.println(F("JSON parseObject() failed"));
      return false;
    }
    
//    json.printTo(Serial);
    
#endif

    // Parse all config file parameters, override
    // local config variables with parsed values
    if (json.containsKey(PUSHOVER_API_URL_Label))
    {
      strcpy(custom_PUSHOVER_API_URL, json[PUSHOVER_API_URL_Label]);
    }

    if (json.containsKey(PUSHOVER_USERKEY_Label))
    {
      strcpy(custom_PUSHOVER_USERKEY, json[PUSHOVER_USERKEY_Label]);
    }

    if (json.containsKey(PUSHOVER_API_KEY_Label))
    {
      strcpy(custom_PUSHOVER_API_KEY, json[PUSHOVER_API_KEY_Label]);
    }
  }
  
  log_i("Pushover config:\n %s %s %s", custom_PUSHOVER_API_URL, custom_PUSHOVER_USERKEY, custom_PUSHOVER_API_KEY);
  
  return true;
}

bool savePushoverConfig() 
{
  log_i("Saving Pushover Config File");

#if (ARDUINOJSON_VERSION_MAJOR >= 6)
  DynamicJsonDocument json(1024);
#else
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
#endif

  // JSONify local configuration parameters
  json[PUSHOVER_API_URL_Label] = custom_PUSHOVER_API_URL;
  json[PUSHOVER_USERKEY_Label] = custom_PUSHOVER_USERKEY;
  json[PUSHOVER_API_KEY_Label] = custom_PUSHOVER_API_KEY;

  // Open file for writing
  File f = FileFS.open(PUSHOVER_CONFIG_FILE, "w");

  if (!f)
  {
    log_e("Failed to open Config File for writing");
    return false;
  }

#if (ARDUINOJSON_VERSION_MAJOR >= 6)
  serializeJsonPretty(json, Serial);
  // Write data to file and close it
  serializeJson(json, f);
#else
  json.prettyPrintTo(Serial);
  // Write data to file and close it
  json.printTo(f);
#endif

  f.close();

  log_i("Pushover Config File successfully saved");
  return true;
}
#endif //FEATURE_PUSHOVER

void etask_wifi_mgr(void *parameters)
{
#if FEATURE_WIFI
    log_i("Starting SPIFFS");
    if (FORMAT_FILESYSTEM)
    {
        log_w("Forced formatting of SPIFFS filesystem!");
        FileFS.format();
    }

    // Format FileFS if not yet
    if (!FileFS.begin(true))
    {
        if (!FileFS.begin())
        {
            // prevents debug info from the library to hide err message.
            delay(100);
            log_e("SPIFFS failed!. Continuing without WiFi!");
            while (true)
            {
                delay(1000);
            }
        }
    }
    else
    {
        log_i("Successfully mounted SPIFFS");
    }

    // New in v1.4.0
    initAPIPConfigStruct(WM_AP_IPconfig);
    initSTAIPConfigStruct(WM_STA_IPconfig);
    //////

    // Local intialization. Once its business is done, there is no need to keep it around
    //  Use this to personalize DHCP hostname (RFC952 conformed)
    ESP_WiFiManager ESP_wifiManager("rLabDoor");

#if WIFI_MGR_DEBUG
    ESP_wifiManager.setDebugOutput(true);
#endif

#if WIFI_MGR_RESET
    ESP_wifiManager.resetSettings();
#endif

    ESP_wifiManager.setMinimumSignalQuality(-1);

    // Set config portal channel, default = 1. Use 0 => random channel from 1-13
    ESP_wifiManager.setConfigPortalChannel(0);

#if USING_CORS_FEATURE
    ESP_wifiManager.setCORSHeader("Your Access-Control-Allow-Origin");
#endif

    // We can't use WiFi.SSID() in ESP32as it's only valid after connected.
    // SSID and Password stored in ESP32 wifi_ap_record_t and wifi_config_t are also cleared in reboot
    // Have to create a new function to store in EEPROM/SPIFFS for this purpose
    Router_SSID = ESP_wifiManager.WiFi_SSID();
    Router_Pass = ESP_wifiManager.WiFi_Pass();

    password = "My" + ssid;

    bool WifiConfigLoaded = false;
    bool POConfigLoaded = false;

    // Load stored data, the addAP ready for MultiWiFi reconnection
    if(loadWifiConfig()) {
        log_i("Got WiFi credentials from SPIFFS");
        WifiConfigLoaded = true;
    }
#if FEATURE_PUSHOVER
    /* Load config for Pushover.net */
    if(loadPushoverConfig()) {
        log_i("Got Pushover credentials from SPIFFS");
    }
#endif //FEATURE_PUSHOVER

    /* Decide whether to start the config portal */
    if (digitalRead(GPIO_TAMPER))
    {
        /* Case is open so start portal */
        log_w("Case (TAMPER) open on reboot - Starting config portal!");
        start_portal = true;
    }
    
    if(!WifiConfigLoaded && !start_portal)
    {
        log_w("No WiFi creds found - Continuing without WiFi");
        log_w("Set WiFi SSID and password through config portal, enabled by rebooting with case (TAMPER) open");
        while (true) {
            /* End task here - No WiFi Creds and no portal */
            delay(1000);
        }
    }

#if FEATURE_PUSHOVER
    // Extra parameters to be configured for Pushover configuration
    // After connecting, parameter.getValue() will get you the configured value
    // Format: <ID> <Placeholder text> <default value> <length> <custom HTML> <label placement>
    // (*** we are not using <custom HTML> and <label placement> ***)

    // PUSHOVER_API_URL
    ESP_WMParameter PUSHOVER_API_URL_FIELD(PUSHOVER_API_URL_Label, "Pushover API URL", custom_PUSHOVER_API_URL, custom_PUSHOVER_API_URL_LEN);

    // PUSHOVER_USERKEY
    ESP_WMParameter PUSHOVER_USERKEY_FIELD(PUSHOVER_USERKEY_Label, "Pushover User Key", custom_PUSHOVER_USERKEY, custom_PUSHOVER_USERKEY_LEN);

    // PUSHOVER_APIKEY
    ESP_WMParameter PUSHOVER_API_KEY_FIELD(PUSHOVER_API_KEY_Label, "Pushover API Key", custom_PUSHOVER_API_KEY, custom_PUSHOVER_API_KEY_LEN);

    ESP_wifiManager.addParameter(&PUSHOVER_API_URL_FIELD);
    ESP_wifiManager.addParameter(&PUSHOVER_USERKEY_FIELD);
    ESP_wifiManager.addParameter(&PUSHOVER_API_KEY_FIELD);
#endif // FEATURE_PUSHOVER

    if (start_portal)
    {
        log_i("Starting config portal:  SSID: %s PASS: %s", ssid, password);
        log_i("%s:%d", "192.168.4.1", HTTP_PORT_TO_USE);

        // Starts an access point BLOCKING
        if (!ESP_wifiManager.startConfigPortal((const char *)ssid.c_str(), password.c_str()))
            log_e("Unable to start config portal");

        // Stored  for later usage, from v1.1.0, but clear first
        memset(&WM_config, 0, sizeof(WM_config));

        for (uint8_t i = 0; i < NUM_WIFI_CREDENTIALS; i++)
        {
            String tempSSID = ESP_wifiManager.getSSID(i);
            String tempPW = ESP_wifiManager.getPW(i);

            if (strlen(tempSSID.c_str()) < sizeof(WM_config.WiFi_Creds[i].wifi_ssid) - 1)
                strcpy(WM_config.WiFi_Creds[i].wifi_ssid, tempSSID.c_str());
            else
                strncpy(WM_config.WiFi_Creds[i].wifi_ssid, tempSSID.c_str(), sizeof(WM_config.WiFi_Creds[i].wifi_ssid) - 1);

            if (strlen(tempPW.c_str()) < sizeof(WM_config.WiFi_Creds[i].wifi_pw) - 1)
                strcpy(WM_config.WiFi_Creds[i].wifi_pw, tempPW.c_str());
            else
                strncpy(WM_config.WiFi_Creds[i].wifi_pw, tempPW.c_str(), sizeof(WM_config.WiFi_Creds[i].wifi_pw) - 1);

            // Don't permit NULL SSID and password len < MIN_AP_PASSWORD_SIZE (8)
            if ((String(WM_config.WiFi_Creds[i].wifi_ssid) != "") && (strlen(WM_config.WiFi_Creds[i].wifi_pw) >= MIN_AP_PASSWORD_SIZE))
            {
                log_i("Add SSID: %s; PWD: %s", Router_SSID.c_str(), Router_Pass.c_str());
                wifiMulti.addAP(WM_config.WiFi_Creds[i].wifi_ssid, WM_config.WiFi_Creds[i].wifi_pw);
            }
        }

#if USE_ESP_WIFIMANAGER_NTP
        String tempTZ = ESP_wifiManager.getTimezoneName();

        if (strlen(tempTZ.c_str()) < sizeof(WM_config.TZ_Name) - 1)
            strcpy(WM_config.TZ_Name, tempTZ.c_str());
        else
            strncpy(WM_config.TZ_Name, tempTZ.c_str(), sizeof(WM_config.TZ_Name) - 1);

        const char *TZ_Result = ESP_wifiManager.getTZ(WM_config.TZ_Name);

        if (strlen(TZ_Result) < sizeof(WM_config.TZ) - 1)
            strcpy(WM_config.TZ, TZ_Result);
        else
            strncpy(WM_config.TZ, TZ_Result, sizeof(WM_config.TZ_Name) - 1);

        if (strlen(WM_config.TZ_Name) > 0)
        {
            log_i("Saving current TZ_Name = %s, TZ = %s", WM_config.TZ_Name, WM_config.TZ);
            configTzTime(WM_config.TZ, "pool.ntp.org", "0.pool.ntp.org", "1.pool.ntp.org");
        }
        else
        {
            log_e("Current Timezone Name is not set. Enter Config Portal to set.");
        }
#endif

        ESP_wifiManager.getSTAStaticIPConfig(WM_STA_IPconfig);
        saveWifiConfig();

#if FEATURE_PUSHOVER
        // Getting posted form values and overriding local variables parameters
        // Config file is written regardless the connection state
        strcpy(custom_PUSHOVER_API_URL, PUSHOVER_API_URL_FIELD.getValue());
        strcpy(custom_PUSHOVER_USERKEY, PUSHOVER_USERKEY_FIELD.getValue());
        strcpy(custom_PUSHOVER_API_KEY, PUSHOVER_API_KEY_FIELD.getValue());

        // Writing Pushover to JSON config file on flash for next boot
        savePushoverConfig();
#endif // FEATURE_PUSHOVER

    }
    else
    {
        for (uint8_t i = 0; i < NUM_WIFI_CREDENTIALS; i++)
        {
            // Don't permit NULL SSID and password len < MIN_AP_PASSWORD_SIZE (8)
            if ((String(WM_config.WiFi_Creds[i].wifi_ssid) != "") && (strlen(WM_config.WiFi_Creds[i].wifi_pw) >= MIN_AP_PASSWORD_SIZE))
            {
                log_i("Add SSID: %s; PWD: %s", Router_SSID.c_str(), Router_Pass.c_str());
                wifiMulti.addAP(WM_config.WiFi_Creds[i].wifi_ssid, WM_config.WiFi_Creds[i].wifi_pw);
            }
        }

        if (WiFi.status() != WL_CONNECTED)
        {
            log_i("Attempting to connect to WiFi");
            connectMultiWiFi();
        }

#if USE_ESP_WIFIMANAGER_NTP
        if (strlen(WM_config.TZ_Name) > 0)
        {
            log_i("Current TZ_Name = %s, TZ = %s", WM_config.TZ_Name, WM_config.TZ);
            configTzTime(WM_config.TZ, "pool.ntp.org", "0.pool.ntp.org", "1.pool.ntp.org");
        }
        else
        {
            log_e("Current Timezone Name is not set. Enter Config Portal to set.");
        }
#endif
    }

    if (WiFi.status() == WL_CONNECTED)
        log_i("Connected. Local IP: %s", WiFi.localIP().toString().c_str());
    else
        log_e("Not connected to WiFi!");
#endif  // FEATURE_WIFI
    for (;;)
    {
        // put your main code here, to run repeatedly
#if FEATURE_WIFI
        check_status();
#endif  // FEATURE_WIFI
        delay(100);
    }
}