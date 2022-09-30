#include <pi_control.h>
#include <hardware.h>

volatile unsigned long pi_heartbeat;
volatile bool pi_open1 = false;
volatile bool pi_open2 = false;

/* ISRs */
void IRAM_ATTR Pi_Heartbeat_ISR() {
    pi_heartbeat = millis();
}

void IRAM_ATTR PiOpen1_ISR() {
    if ((pi_heartbeat > (millis() - MAX_HEARTBEAT_DELAY))) {
        pi_open1 = digitalRead(GPIO_PI_OPEN_CMD_1);
    }
}

void IRAM_ATTR PiOpen2_ISR() {
    if ((pi_heartbeat > (millis() - MAX_HEARTBEAT_DELAY))) {
        pi_open2 = digitalRead(GPIO_PI_OPEN_CMD_2);
    }
}
    
Pi::Pi() {
    _powered_pin    = 0;
    _heartbeat_pin  = 0;
    _open1_pin      = 0;
    _open2_pin      = 0;
}

Pi::~Pi() {

}

void Pi::begin(
    uint8_t powered_pin,
    uint8_t heartbeat_pin,
    uint8_t open1_pin,
    uint8_t open2_pin) {

    _powered_pin    = powered_pin;
    _heartbeat_pin  = heartbeat_pin;
    _open1_pin      = open1_pin;
    _open2_pin      = open2_pin;

    /* Attach the interrupts */
    pinMode(_powered_pin,   INPUT);
    pinMode(_heartbeat_pin, INPUT);
    pinMode(_open1_pin,     INPUT);
    pinMode(_open2_pin,     INPUT);
    attachInterrupt(_heartbeat_pin, Pi_Heartbeat_ISR, RISING);
    attachInterrupt(_open1_pin, PiOpen1_ISR, CHANGE);
    attachInterrupt(_open2_pin, PiOpen2_ISR, CHANGE);
}

bool Pi::is_connected() {
    return digitalRead(_powered_pin);
}

bool Pi::is_alive() {
    return (pi_heartbeat > (millis() - MAX_HEARTBEAT_DELAY));
}