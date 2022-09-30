#ifndef PI_CONTROL_H
#define PI_CONTROL_H

#include <Arduino.h>

#define MAX_HEARTBEAT_DELAY     (2000)  // milliseconds
#define OPEN_QUEUE_LEN          (5)

extern volatile bool pi_open1;
extern volatile bool pi_open2;

/* ISRs */
void IRAM_ATTR Pi_Heartbeat_ISR();
void IRAM_ATTR Pi_Open1_ISR();
void IRAM_ATTR Pi_Open2_ISR();

class Pi
{   
private:
    /* data */
    uint8_t _powered_pin;
    uint8_t _heartbeat_pin;
    uint8_t _open1_pin;
    uint8_t _open2_pin;

public:
    Pi();
    ~Pi();

    void begin( 
        uint8_t powered_pin,
        uint8_t heartbeat_pin,
        uint8_t open1_pin,
        uint8_t open2_pin
      );
    bool is_connected();
    bool is_alive();
};

#endif