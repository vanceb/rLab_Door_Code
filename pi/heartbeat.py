import RPi.GPIO as GPIO
import time

hb_pin = 7
o1_pin = 16
o2_pin = 18
GPIO.setmode(GPIO.BOARD)
GPIO.setup(hb_pin, GPIO.OUT)
GPIO.setup(o1_pin, GPIO.OUT)
GPIO.setup(o2_pin, GPIO.OUT)

loopCounter = 0
state = False;

while True:
    # Output the heartbeat
    GPIO.output(hb_pin, state)
    state = not(state)
    # Have the door open for 5 in every 30 seconds
    if (loopCounter % 60) < 10:
        GPIO.output(o1_pin, True)
        GPIO.output(o2_pin, True)
    else:
        GPIO.output(o1_pin, False)
        GPIO.output(o2_pin, False)
    loopCounter += 1
    print(loopCounter)
    time.sleep(0.5)

