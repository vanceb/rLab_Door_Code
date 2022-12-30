import RPi.GPIO as GPIO
import time

hb_pin = 7
pw_pin = 11
o1_pin = 16
o2_pin = 18

GPIO.setmode(GPIO.BOARD)
GPIO.setup(hb_pin, GPIO.OUT)
GPIO.setup(pw_pin, GPIO.OUT)
GPIO.setup(o1_pin, GPIO.OUT)
GPIO.setup(o2_pin, GPIO.OUT)

# Turn on the power to the card reader
GPIO.output(pw_pin, True)

loopCounter = 0
state = False;

while True:
    # Output the heartbeat
    GPIO.output(hb_pin, state)
    state = not(state)
    # Have the door open for 5 in every 30 seconds
    if (loopCounter % 60) < 10:
        GPIO.output(o1_pin, True)
    else:
        GPIO.output(o1_pin, False)

    # Occasionally reject a card
    if (loopCounter % 37) == 0:
        GPIO.output(o2_pin, True)
    else:
        GPIO.output(o2_pin, False)

    # Power cycle the card reader every minute
    if (loopCounter % 120) == 0:
        GPIO.output(pw_pin, False)
        time.sleep(1)
        GPIO.output(pw_pin, True)

    loopCounter += 1
    print(loopCounter)
    time.sleep(0.5)