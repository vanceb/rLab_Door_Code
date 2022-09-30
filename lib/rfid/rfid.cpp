#include <rfid.h>
#include <Arduino.h>

extern "C" {
#include <pn532.h>
}

bool rfid_open1 = false;
bool rfid_open2 = false;

void rfidTask(void * pvParameters) {
    /* Cast the incoming parameter to a pn532 nfc reader */
    pn532_t * nfc = (pn532_t *) pvParameters;
  
  /*
    pn532_t * nfc = pn532_init(Serial1, 26, 27, 0);
    if (!nfc) {
        Serial.println("Failed to setup PN532 NFC Reader");
        while (1)           // Go no further
            delay(1000);
    } else {
        Serial.println("Successfully setup PN532 NFC Reader");
    }
*/
    int8_t c = 0;
    char id[21];
    for(;;) {
        c = pn532_Cards(nfc);
        if (c > 0)
        {
            log_i("Found %d cards", c);
            pn532_nfcid(nfc, id);
            log_i("Found card id: %s", id);
        } else if (c < 0) {
            log_e("Error reading cards: %s", pn532_err_to_name(pn532_lasterr(nfc)));
        }
        delay(1000);
    }
}