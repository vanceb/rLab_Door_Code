#include <hardware.h>

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