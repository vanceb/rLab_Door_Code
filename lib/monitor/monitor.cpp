#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#include <monitor.h>
#include <hardware.h>
#include <pushover.h>
#include <rfid.h>

#if FEATURE_WIFI
#include <WiFi.h>
#endif

#if FEATURE_PI
#include <pi_control.h>
#endif  // FEATURE_PI


/* Global state variables */
static bool open1_state = true;
static bool open2_state = true;
static unsigned long open1_changed = 0;
static unsigned long open2_changed = 0;
static bool pi_reject = false;
static unsigned long pi_reject_indicated = 0;
static bool power_lost = false;
static bool battery_low = false;
static bool tamper = false;
static uint16_t num_open = 0;
static uint16_t num_reject = 0;
static uint16_t num_tamper = 0;
static uint16_t num_power_loss = 0;
static uint16_t num_pi_fails = 0;


#if FEATURE_DISPLAY
/* Display */
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "uptime.h"
LiquidCrystal_I2C display = LiquidCrystal_I2C(DISP_ADDR, DISP_COLS, DISP_ROWS);

byte WiFi_Char[8] = {
    B00000,
    B00001,
    B00001,
    B00101,
    B00101,
    B10101,
    B10101,
    B00000
};
/*
byte WiFi_AP_Char[8] = {
    B00001,
    B00001,
    B00101,
    B00101,
    B10101,
    B10101,
    B11111,
    B11111
};
*/
byte WiFi_AP_Char[8] = {
    B10001,
    B10001,
    B01010,
    B01010,
    B00100,
    B11111,
    B11111,
    B00000
};

byte Pi_Char[8] = {
    B00000,
    B00000,
    B11101,
    B10100,
    B11101,
    B10001,
    B10001,
    B00000
};

byte Plug_Char[8] = {
    B00000,
    B01110,
    B11011,
    B11111,
    B10101,
    B11111,
    B00100,
    B00000
};

byte Batt_Full_Char[8] = {
    B00100,
    B01110,
    B01110,
    B01110,
    B01110,
    B01110,
    B01110,
    B00000
};

byte Batt_Empty_Char[8] = {
    B00100,
    B01010,
    B01010,
    B01010,
    B01010,
    B01010,
    B01110,
    B00000
};

int setup_i2c_disp() {
    Wire.begin(GPIO_SDA_DISP, GPIO_SCL_DISP);
    /* Check to see that we have a display */
    Wire.beginTransmission(DISP_ADDR);
    if (Wire.endTransmission() == 0) {
        log_i("Character display found at i2c 0x%02X", DISP_ADDR);
        
        /* Create Custom Characters */
        display.createChar(0, WiFi_Char);
        display.createChar(1, WiFi_AP_Char);
        display.createChar(2, Pi_Char);
        display.createChar(3, Plug_Char);
        display.createChar(4, Batt_Full_Char);
        display.createChar(5, Batt_Empty_Char);

        display.init();
        display.clear();
        display.backlight();
        return true;
    } else {
        log_w("Character display NOT found at i2c 0x%02X", DISP_ADDR);
        log_i("Scanning...");
        int error;
        int found = 0;
        for (int address = 1; address < 127; address++ )
        {
            // The i2c_scanner uses the return value of
            // the Write.endTransmisstion to see if
            // a device did acknowledge to the address.
            Wire.beginTransmission(address);
            error = Wire.endTransmission();

            if (error == 0) {
                log_i("I2C device found at address 0x%02X", address);
                found++;
            } else if (error == 4) {
                log_w("Unknown error at address 0x%02X", address);
            }
        }
        log_i("Found %d devices on the I2C bus", found);
    }
    return false;
}
#endif  // FEATURE_DISPLAY

/* check_door_state()
 * 
 * Check and control the state of the door.
 * Looks at the command variables coming from the Pi and the RFID reader
 * and opens or closes the door based on these commands.
 * It also has a safety timeout which closes the door 
 * in case it is left in an open state for too long.
 */
void check_door_state() {
    bool open1_request = false;
    bool open2_request = false;

    #if FEATURE_PI
    #if FEATURE_PI_OPEN2_REJECT
    if (pi_open2) {
        log_w("Card Rejected by Pi - doors locked");
        num_reject++;
        pi_reject = true;
        pi_reject_indicated = millis() + REJECT_INDICATION;
        pi_open1 = false;  // Reject trumps an open command
        pi_open2 = false;
    } else {
        pi_reject = false;
    }
    open2_request |= pi_open1;  // Use the open1 command to open both doors
    #else
    open2_request |= pi_open2;
    #endif  // FEATURE_PI_OPEN2_REJECT
    open1_request |= pi_open1;
    #endif  // FEATURE_PI
    
    #if FEATURE_NFC
    open1_request |= rfid_open1;
    open2_request |= rfid_open2;
    #endif  // FEATURE_NFC

    /* Change the state of the door if requested */

    if (open1_state != open1_request) {
        digitalWrite(GPIO_OPEN_1, open1_request);
        open1_state = open1_request;
        open1_changed = millis();
        if (open1_state) {
            log_i("Door 1 opened by %s", rfid_open1 ? "RFID" : "Pi");
            num_open++;
        } else {
            log_i("Door 1 closed");
        }
    }

    if (open2_state != open2_request) {
        digitalWrite(GPIO_OPEN_2, open2_request);
        open2_state = open2_request;
        open2_changed = millis();
        if (open2_state) {
            log_i("Door 2 opened by %s", rfid_open2 ? "RFID" : "Pi");
        } else {
            log_i("Door 2 closed");
        }
    }
   
    /* Check to see whether we should close the door based on timeout */
    if (millis() - open1_changed > DOOR_OPEN_MAX_MS && digitalRead(GPIO_OPEN_1)) {
        digitalWrite(GPIO_OPEN_1, false);
#if FEATURE_PI
        pi_open1 = false;
#endif  // FEATURE_PI
#if FEATURE_NFC
        rfid_open1 = false;
#endif  // FEATURE_NFC
        log_i("Door 1 closed because of timeout");
    }

    if (millis() - open2_changed > DOOR_OPEN_MAX_MS && digitalRead(GPIO_OPEN_2)) {
        digitalWrite(GPIO_OPEN_2, false);
#if FEATURE_PI
        pi_open2 = false;
#endif  // FEATURE_PI
#if FEATURE_NFC
        rfid_open2 = false;
#endif  // FEATURE_NFC
        log_i("Door 2 closed because of timeout");
    }
}

/* check_voltages()
 * 
 * Uses the ADC to check the various system rails.
 */
uint8_t check_voltages() {
    uint8_t errors = 0;
#if FEATURE_ADC
    uint16_t v_adc;
    float v;
    v_adc = analogRead(GPIO_ADC_V3);
    v = (float) v_adc / V3_FACTOR;
    log_d("V3:    %0.1fV [%d]", v, v_adc);
    if (v < V3_LOW) {
        log_d("3.3V supply low: %0.1fV ", v);
        errors |= ERR_V3_LOW;
    } else if (v > V3_HI) {
        log_d("3.3V supply high: %0.1fV ", v);
        errors |= ERR_V3_HI;
    }
    v_adc = analogRead(GPIO_ADC_V5);
    v = (float) v_adc / V5_FACTOR;
    log_d("V5:    %0.1fV [%d]", v, v_adc);
    if (v < V5_LOW) {
        log_d("5V supply low: %0.1fV ", v);
        errors |= ERR_V5_LOW;
    } else if (v > V5_HI) {
        log_d("5V high: %0.1fV ", v);
        errors |= ERR_V5_HI;
    }
    v_adc = analogRead(GPIO_ADC_VBATT);
    v = ((float) v_adc / VBATT_FACTOR) + 0.5;  // 0.5 accounts for voltage drop across diode
    log_d("VBATT: %0.1fV [%d]", v, v_adc);
    if (v < VBATT_LOW) {
        log_d("Battery voltage low: %0.1fV ", v);
        errors |= ERR_VBATT_LOW;
    } else if (v > VBATT_HI) {
        log_d("Battery voltage high: %0.1fV ", v);
        errors |= ERR_VBATT_HI;
    }
    v_adc = analogRead(GPIO_ADC_VIN);
    v = ((float) v_adc / VIN_FACTOR) + 0.5;  // 0.5 accounts for voltage drop across diode
    log_d("VIN:   %0.1fV [%d]", v, v_adc);
    if (v < VIN_LOW) {
        log_d("Input voltage low: %0.1fV ", v);
        errors |= ERR_VIN_LOW;
    } else if (v > VIN_HI) {
        log_d("Input voltage high: %0.1fV ", v);
        errors |= ERR_VIN_HI;
    }
    #endif  // FEATURE_ADC
    return errors;
} 

void monitorTask(void * pvParameters) {
    unsigned long loop_counter = 0;
    const long loop_delay = 1000 / LOOP_FREQ;
    unsigned long loop_start = millis();
    long delay_for;
    uint8_t errors = 0;
    uint8_t prev_errors = 0;
    int pi_connected = 0;
    int pi_connected_prev = 0;
    int pi_ok = 0;
    int pi_ok_prev = 0;
    static time_t send_weekly_status = 0;
    
#if FEATURE_DISPLAY
    setup_i2c_disp();
#endif  // FEATURE_DISPLAY

#if FEATURE_NEOPIXELS
    /* Neopixels */
    Adafruit_NeoPixel npx1(NPX_NUM_LEDS_1, GPIO_NPX_1, NEO_GRB + NEO_KHZ800);
    Adafruit_NeoPixel npx2(NPX_NUM_LEDS_2, GPIO_NPX_2, NEO_GRB + NEO_KHZ800);
    npx1.begin();
    npx2.begin();
#endif  // FEATURE_NEOPIXELS

  #if FEATURE_PI
  /* Check the Raspberry Pi */
  Pi rpi;
  rpi.begin(
    GPIO_PI_POWERED,
    GPIO_PI_HEARTBEAT,
    GPIO_PI_OPEN_CMD_1,
    GPIO_PI_OPEN_CMD_2
  );
#endif  // FEATURE_PI

    /* Main loop */
    for(;;) {
        /* Run every loop */
        loop_start = millis();

        /* Do the checks on the hardware */

#if FEATURE_PI
        /* Check that the Pi is alive */
        pi_connected = rpi.is_connected();
        pi_ok        = rpi.is_alive();
        if (pi_connected) {
            /* We can see the physical connection */
            if (pi_ok) {
                /* All good! */
                if (!pi_ok_prev) {
                    /* Heartbeat just come up */
                    log_i("Raspberry Pi detected with heartbeat, all OK");
#if FEATURE_PUSHOVER
                    pushover.send("rLabDoor Pi", "The rLabDoor Pi service detected. Pi door control is enabled", -1);
#endif  // FEATURE_PUSHOVER
                } else {
                    /*Been good for a while, nothing to say */
                }
            } else {
                /* No recent heartbeat from the Raspberry Pi */
                if (pi_ok_prev) {
                    /* Was OK before so alert */
                    log_e("No recent heartbeat from Raspberry Pi - Check the service on the Pi");
                    num_pi_fails++;
#if FEATURE_PUSHOVER
                    pushover.send("rLabDoor Pi", "No heartbeat detected from the Pi - Door will not operate", -1);
#endif  // FEATURE_PUSHOVER
                } else {
                    /* Startup or not seen in a while, stay silent */
                }
            }
        } else {
            if (pi_connected_prev) {
                /* We have seen it up a moment ago */
                log_e("Can't detect Raspberry Pi any more");
                num_pi_fails++;
#if FEATURE_PUSHOVER
                pushover.send("rLabDoor Pi", "Raspberry Pi connection physical lost", -1);
#endif  // FEATURE_PUSHOVER
            } else {
                log_e("No Raspberry Pi detected on startup");
#if FEATURE_PUSHOVER
                pushover.send("rLabDoor Pi", "Raspberry Pi physical connection not detected", -1);
#endif  // FEATURE_PUSHOVER
            }
        }

        pi_ok_prev = pi_ok;
        pi_connected_prev = pi_connected;
#endif  // FEATURE_PI

        /* Door opening */
        check_door_state();

        /* Check the Tamper input */
#if FEATURE_TAMPER
        if (digitalRead(GPIO_TAMPER)) {
            if (!tamper) {
                /* This has just happened */
                tamper = true;
                log_w("TAMPER - The enclosure is open!");
                num_tamper++;
#if FEATURE_PUSHOVER
                pushover.send("rLabDoor Tamper!", "The rLabDoor enclosure has been opened", 0);
#endif  // FEATURE_PUSHOVER
            } 
        } else {
            if (tamper) {
                tamper = false;
                log_i("TAMPER - The enclosure is closed");
#if FEATURE_PUSHOVER
                pushover.send("rLabDoor Tamper!", "The rLabDoor enclosure has been closed", -1);
#endif  // FEATURE_PUSHOVER
            }
        }
#endif  // FEATURE_TAMPER

        /* Once per second */
        if (loop_counter % LOOP_FREQ == 0) {
            /* Check the system voltages */
            errors = check_voltages();
            if(errors != prev_errors) {
                prev_errors = errors;
                /* Something has changed! */
                if (errors) {
                    log_w("At least one of the system voltages is out of limits");
                    if ((errors & ERR_VIN_LOW) && !power_lost) {
                        power_lost = true;
                        log_e("Input power lost or low");
                        num_power_loss++;
    #if FEATURE_PUSHOVER
                        pushover.send("rLabDoor Power!", "Input power lost", 1);
    #endif  // FEATURE_PUSHOVER
                    }
                    if ((errors & ERR_VBATT_LOW) && !battery_low) {
                        battery_low = true;
                        log_w("Battery voltage is low, door may go offline shortly...");
    #if FEATURE_PUSHOVER
                        pushover.send("rLabDoor Power!", "Battery voltage low, door may not operate correctly", 2);
    #endif  // FEATURE_PUSHOVER
                    }
                } else {
                    log_i("All voltages are within expected limits");
                    if (power_lost || battery_low) {
                        log_i("System power is OK");
    #if FEATURE_PUSHOVER
                        pushover.send("rLabDoor Power!", "Input power restored", 0);
    #endif  // FEATURE_PUSHOVER
                        power_lost = false;
                        battery_low = false;
                    }
                } 
            }
        }

#if FEATURE_NEOPIXELS
        /* Neopixels */
        int i = 0;
        uint8_t brightness = 255;
        npx1.clear();
        if (open1_state || open2_state) {
            for (i=0; i<NPX_NUM_LEDS_1; i++) {
                npx1.setPixelColor(i, npx1.Color(0,255,0));
            }
        } else if (millis() < pi_reject_indicated) {
            for (i=0; i<NPX_NUM_LEDS_1; i++) {
                npx1.setPixelColor(i, npx1.Color(255,0,0));   
            }
        } else {
            brightness = loop_counter % 255;
            brightness = brightness > 128 ? 256 - brightness : brightness;
            for (i=0; i<NPX_NUM_LEDS_1; i++) {
                npx1.setPixelColor(i, npx1.Color(255,255,255));
            }
        }
        npx1.setBrightness(brightness);
        npx1.show();
#endif  // FEATURE_NEOPIXELS        

#if FEATURE_DISPLAY
        /* Update display once per second */
        if (loop_counter % LOOP_FREQ == 0) {
            /* Line 1 - Status Icons */
            display.setCursor(0,0);
            display.print("rLab Door      ");

            /* Show WiFi status*/
            display.setCursor(19,0);
            if (WiFi.status() == WL_CONNECTED) {
                display.write(byte(0));
            } else if(WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
                display.write(byte(1));
            } else {
                display.print(" ");
            }

            /* Show Pi Connection Status */
            display.setCursor(15,0);
            if (pi_ok) {
                display.write(byte(2));
            } else {
                display.print(" ");
            }

            /* Show Power status */
            display.setCursor(17,0);
            if (battery_low) {
                /* Display Power message */
//                display.print("Power Off - Batt Low");
                /* Show Batt Low Status*/
                display.write(byte(5));
            } else if (power_lost) {
                /* Display Power message */
//                display.print("Power Off - Batt OK ");
                /* Show Batt Full Status*/
                display.write(byte(4));
            } else {
                /* Display Power OK message */
//                display.print("      Power OK      ");
                /* Show Mains Power Status*/
                display.write(byte(3));
            }

            /* Line 2 - Card swipe status */
            display.setCursor(0,1);
            if (open1_state || open2_state) {
                display.print("   Card Accepted    ");
            } else if (millis() < pi_reject_indicated) {
                display.print("   Card Rejected    ");
            } else {
                display.print("                    ");
            }

            /* Line 3 - Door Status */
            display.setCursor(0,2);
            display.printf("Door 1: %s 2: %s ",
                (open1_state ? "open" : "lock"),
                (open2_state ? "open" : "lock")
            );
            
            /* Line 4 - Show uptime */
            display.setCursor(0,3);
            uptime::calculateUptime();
            display.printf("Up: %03lud %02luh %02lum %02lus",
                            uptime::getDays(),
                            uptime::getHours(),
                            uptime::getMinutes(),
                            uptime::getSeconds()
            );
        }
#endif  // FEATURE_DISPLAY


#if FEATURE_PUSHOVER
        /* Set the time to send the weekly status report */
        /* 08:00:00 on Sunday morning */
        time_t now;
        struct tm timeinfo;
        int days_away;
        getLocalTime(&timeinfo, NTP_TIMEOUT_MS);
        //getLocalTime(&timeinfo, 1);
        now = mktime(&timeinfo);
        if (timeinfo.tm_year > 100 && send_weekly_status == 0) {  // The clock has been set and we have just booted
            timeinfo.tm_hour = 8;
            timeinfo.tm_min = 0;
            timeinfo.tm_sec = 0;
            days_away = 7 - timeinfo.tm_wday;
            send_weekly_status = mktime(&timeinfo) + days_away*24*60*60;
            if (now > send_weekly_status) {
                send_weekly_status += 7*24*60*60;
            }
            log_i("Status report will be sent %s", asctime(localtime(&send_weekly_status)));
        }

        /* Check whether we should send the weekly status report */
        getLocalTime(&timeinfo, NTP_TIMEOUT_MS);
        //getLocalTime(&timeinfo, 1);
        now = mktime(&timeinfo);
        if (send_weekly_status && now > send_weekly_status) {
            // We should send the report
            char msg[PUSHOVER_MAX_BODY_LEN];
            snprintf(msg, PUSHOVER_MAX_PAYLOAD_LEN, "Up: %d days, %d hours\nDoor Open: %d\nCard Reject: %d\nPi Fails: %d\nPower Loss: %d\nTamper: %d", 
                    uptime::getDays, 
                    uptime::getHours, 
                    num_reject, 
                    num_pi_fails, 
                    num_power_loss, 
                    num_tamper);
            pushover.send("rLabDoor weekly status", msg, -1);

            // Reset the counters and the next status time
            num_open = 0;
            num_reject = 0;
            num_pi_fails = 0;
            num_power_loss = 0;
            num_tamper = 0;
            timeinfo.tm_hour = 8;
            timeinfo.tm_min = 0;
            timeinfo.tm_sec = 0;
            send_weekly_status = mktime(&timeinfo) + 7*24*60*60;
            log_i("Next status report will be sent %s", asctime(localtime(&send_weekly_status)));
        }
#endif  // FEATURE_PUSHOVER

        /* Work out how long we have to sleep for */
        delay_for = (loop_delay - (millis() - loop_start));
        delay_for = delay_for < 0 ? 0 : delay_for;
        //log_d("Monitor loop sleeping for %dms", delay_for);
        delay(delay_for);
        loop_counter++;
    }
}