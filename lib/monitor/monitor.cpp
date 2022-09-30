#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#include <monitor.h>
#include <hardware.h>

#if FEATURE_PI
#include <pi_control.h>
#endif  // FEATURE_PI

/* Global state variables */
static bool open1_state = true;
static bool open2_state = true;
static unsigned long open1_changed = 0;
static unsigned long open2_changed = 0;
static bool input_power_lost = false;
static bool battery_low = false;
static bool tamper = false;

#if FEATURE_DISPLAY
/* Display */
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "uptime.h"
LiquidCrystal_I2C display = LiquidCrystal_I2C(DISP_ADDR, DISP_COLS, DISP_ROWS);

int setup_i2c_disp() {
    Wire.begin(GPIO_SDA_DISP, GPIO_SCL_DISP);
    /* Check to see that we have a display */
    Wire.beginTransmission(DISP_ADDR);
    if (Wire.endTransmission() == 0) {
        log_i("Character display found at i2c 0x%02X", DISP_ADDR);
        display.init();
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
    open1_request |= pi_open1;
    open2_request |= pi_open2;
    #endif  // FEATURE_PI
    
    #if FEATURE_NFC
    open1_request |= pi_open1;
    open2_request |= pi_open2;
    #endif  // FEATURE_NFC

    /* Change the state of the door if requested */

    if (open1_state != open1_request) {
        digitalWrite(GPIO_OPEN_1, open1_request);
        open1_state = open1_request;
        open1_changed = millis();
        if (open1_state) {
            log_i("Door 1 opened by %s", pi_open1 ? "Pi" : "RFID");
        } else {
            log_i("Door 1 closed");
        }
    }

    if (open2_state != open2_request) {
        digitalWrite(GPIO_OPEN_2, open2_request);
        open2_state = open2_request;
        open2_changed = millis();
        if (open2_state) {
            log_i("Door 2 opened by %s", pi_open2 ? "Pi" : "RFID");
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
        log_w("3.3V supply low: %0.1fV ", v);
        errors |= ERR_V3_LOW;
    } else if (v > V3_HI) {
        log_w("3.3V supply high: %0.1fV ", v);
        errors |= ERR_V3_HI;
    }
    v_adc = analogRead(GPIO_ADC_V5);
    v = (float) v_adc / V5_FACTOR;
    log_d("V5:    %0.1fV [%d]", v, v_adc);
    if (v < V5_LOW) {
        log_w("5V supply low: %0.1fV ", v);
        errors |= ERR_V5_LOW;
    } else if (v > V5_HI) {
        log_w("5V high: %0.1fV ", v);
        errors |= ERR_V5_HI;
    }
    v_adc = analogRead(GPIO_ADC_VBATT);
    v = ((float) v_adc / VBATT_FACTOR) + 0.5;  // 0.5 accounts for voltage drop across diode
    log_d("VBATT: %0.1fV [%d]", v, v_adc);
    if (v < VBATT_LOW) {
        log_w("Battery voltage low: %0.1fV ", v);
        errors |= ERR_VBATT_LOW;
    } else if (v > VBATT_HI) {
        log_w("Battery voltage high: %0.1fV ", v);
        errors |= ERR_VBATT_HI;
    }
    v_adc = analogRead(GPIO_ADC_VIN);
    v = ((float) v_adc / VIN_FACTOR) + 0.5;  // 0.5 accounts for voltage drop across diode
    log_d("VIN:   %0.1fV [%d]", v, v_adc);
    if (v < VIN_LOW) {
        log_w("Input voltage low: %0.1fV ", v);
        errors |= ERR_VIN_LOW;
    } else if (v > VIN_HI) {
        log_w("Input voltage high: %0.1fV ", v);
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
    uint8_t errors;
    
#if FEATURE_NEOPIXELS
    /* Neopixels */
    Adafruit_NeoPixel npx1(NPX_NUM_LEDS_1, GPIO_NPX_1, NEO_GRB + NEO_KHZ800);
    Adafruit_NeoPixel npx2(NPX_NUM_LEDS_2, GPIO_NPX_2, NEO_GRB + NEO_KHZ800);
    npx1.begin();
    npx2.begin();
#endif  // FEATURE_NEOPIXELS

#if FEATURE_DISPLAY
    /* Configure I2C Character Display */
    setup_i2c_disp();

//    display.init();
//    display.backlight();
    display.setCursor(0,0);
    display.print("rLab Door Controller");
#endif  // FEATURE_DISPLAY

    /* Main loop */
    for(;;) {
        /* Run every loop */
        loop_start = millis();

        /* Door opening */
        check_door_state();

#if FEATURE_NEOPIXELS
        /* Neopixels */
        int i = 0;
        uint8_t brightness = 255;
        npx1.clear();
        if (open1_state || open2_state) {
            for (i=0; i<NPX_NUM_LEDS_1; i++) {
                npx1.setPixelColor(i, npx1.Color(0,255,0));
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


        /* Run every 10 loops */
        if (loop_counter % 10 == 0) {
#if FEATURE_TAMPER
            /* Check the Tamper input */
            if (digitalRead(GPIO_TAMPER)) {
                if (!tamper) {
                    /* This has just happened */
                    tamper = true;
                    log_w("TAMPER - The enclosure is open!");
                } 
            } else {
                if (tamper) {
                    tamper = false;
                    log_i("TAMPER - The enclosure is closed");
                }
            }
#endif  // FEATURE_TAMPER
        }

        /* Run once per second */
        if (loop_counter % LOOP_FREQ == 0) {
#if FEATURE_DISPLAY
            /* Display */
            display.setCursor(0,0);
            if (tamper) {
                display.print("       TAMPER       ");
            } else {
                display.print("rLab Door Controller");
            }

            /* Update Door Status on display*/
            display.setCursor(0,2);
            display.printf("Door 1: %s 2: %s ",
                (open1_state ? "open" : "lock"),
                (open2_state ? "open" : "lock")
            );
#endif  // FEATURE_DISPLAY
            /* Check the system voltages */
            errors = check_voltages();
            if(errors) {
                /* One of the voltages is out of limits */
                log_i("At least one of the system voltages is out of limits");
                if ((errors & ERR_VIN_LOW) && !input_power_lost) {
                    input_power_lost = true;
                    log_e("Input power lost or low");
#if FEATURE_DISPLAY
                    /* Display Power message */
                    display.setCursor(0,1);
                    display.print("Power Off - Batt OK ");
#endif  // FEATURE_DISPLAY
                }
                if ((errors & ERR_VBATT_LOW) && !battery_low) {
                    battery_low = true;
                    log_w("Battery voltage is low, door may go offline shortly...");
#if FEATURE_DISPLAY
                    /* Display Power message */
                    display.setCursor(0,1);
                    display.print("Power Off - Batt Low");
#endif  // FEATURE_DISPLAY
                }
            } else {
                /* No errors, so clear the flags */
                if (input_power_lost || battery_low) {
                    log_i("System power is OK");
                    input_power_lost = false;
                    battery_low = false;
                } 
#if FEATURE_DISPLAY
                /* Display Power OK message */
                display.setCursor(0,1);
                display.print("      Power OK      ");
#endif  // FEATURE_DISPLAY
            }

#if FEATURE_DISPLAY
            /* Show uptime */
            display.setCursor(0,3);
            uptime::calculateUptime();
            display.printf("Up: %03lud %02luh %02lum %02lus",
                            uptime::getDays(),
                            uptime::getHours(),
                            uptime::getMinutes(),
                            uptime::getSeconds()
            );
#endif  // FEATURE_DISPLAY
        }

        /* Run once per hour */
        if (loop_counter % (LOOP_FREQ * 3600) == 0) {

        }


        /* Work out how long we have to sleep for */
        delay_for = (loop_delay - (millis() - loop_start));
        delay_for = delay_for < 0 ? 0 : delay_for;
        //log_d("Monitor loop sleeping for %dms", delay_for);
        delay(delay_for);
        loop_counter++;
    }
}