from machine import Pin, PWM
import time
import sys

# SETUP COMPONENTS
led01 = Pin(12, Pin.OUT)
led02 = Pin(26, Pin.OUT)
btn = Pin(14, Pin.IN, Pin.PULL_UP)
bre = Pin(32, Pin.IN, Pin.PULL_UP)
beeper = PWM(Pin(25, Pin.OUT), freq=440, duty=0)
led01.on()
led02.on()

# SETUP VARIABLES
work = 30 * 60
rest = 5 * 60
whatImDoing = 'stop'
long_press_threshold = 1  # Adjust this value for your desired long press duration

def beep(value, led_pin):
    beeper.duty(0)
    time.sleep(0.6)
    beeper.freq(value)
    for _ in range(3):
        led_pin.on()
        beeper.duty(512)
        time.sleep(0.2)
        led_pin.off()
        beeper.duty(0)
        time.sleep(0.2)

def start_or_break_session(minutes, led_pin):
    global whatImDoing
    whatImDoing = 'start' if led_pin == led01 else 'stop'
    led01.off()
    led02.off()
    if minutes == 300 or minutes == 120:
        beep(1140, led_pin)
    else:
        beep(440, led_pin)
    print(whatImDoing, led_pin)
    led_pin.on()
    s = time.time()
    end = s + minutes
    while time.time() < end:
        if btn.value() == 0:
            start_or_break_session(rest, led02 if led_pin == led01 else led01)
            break
        checkBreak()
    beeper.freq(3140) # 3140: Buzzer normal Hz
    beeper.duty(512)
    if btn.value() == 0:
        start_or_break_session(rest, led02 if led_pin == led01 else led01)

def plus_5_minutes():
    if whatImDoing == 'start':
        start_or_break_session(300, led01)
    else:
        start_or_break_session(120, led02)

def checkBreak():
    if bre.value() == 0:
        beeper.duty(0)
        led01.off()
        led02.off()
        sys.exit(0)

def main():
    button_pressed_time = None
    while True:
        if btn.value() == 0:
            if button_pressed_time is None:
                button_pressed_time = time.time()
        else:
            if button_pressed_time is not None:
                press_duration = time.time() - button_pressed_time
                button_pressed_time = None
                if press_duration >= long_press_threshold:
                    plus_5_minutes()
                else:
                    if whatImDoing == 'start':
                        start_or_break_session(rest, led02)
                    else:
                        start_or_break_session(work, led01)
            time.sleep(0.1)  # Debounce
        checkBreak()

main()
