#include <utility>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ThreeWire.h>
#include <RtcDS1302.h>
// #include <BluetoothSerial.h>

using namespace std;

// #if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
// #error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
// #endif

// BluetoothSerial SerialBT;

ThreeWire myWire(5,18,19); // IO, SCLK, CE
RtcDS1302<ThreeWire> Rtc(myWire);

#define OLED_RESET -1

Adafruit_SSD1306 display(OLED_RESET);

#define led_work 13
#define led_rest 14
#define led_plan 12
#define buzzerPin 27
#define btn_start 25
#define btn_next 26
#define btn_co 15

int planTime = 5 * 60;
int workTime = 25 * 60;
int restTime = 5 * 60;
int minuteAdd = 5 * 60;

unsigned long endTime;
unsigned long myTime;

void setup() {
  Serial.begin(115200);
  // SerialBT.begin("ESP32POM");
  pinMode(led_work, OUTPUT);
  pinMode(led_rest, OUTPUT);
  pinMode(led_plan, OUTPUT);
  pinMode(btn_start, INPUT_PULLUP);
  pinMode(btn_next, INPUT_PULLUP);
  pinMode(btn_co, INPUT_PULLUP);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setTextColor(WHITE);

  turn_off_led();
}

int btnStartValue;
int btnNextValue;
int btnCountValue;
char state = 'w';
int startMillis = 0;
unsigned int session = 0;

bool isStart = false;
bool counting = false;
bool wasting = false;
bool reminder_22 = true;
/*
state: w: work
       p: plan
       r: rest
when isStart = false => state = w
*/

unsigned long previousMillis = 0; // will store last time the display was updated
const long updateInterval = 200; // interval at which to refresh the display (milliseconds)
const long debounceDelay = 50; // debounce time for buttons
pair<int, int> t;
// string displayText[] = {};
RtcDateTime now;

#define countof(a) (sizeof(a) / sizeof(a[0]))
char daysConvert[7][4] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};

pair<int, int> printDateTime(const RtcDateTime& dt) {
    char datestring[20];

    snprintf_P(datestring, 
            countof(datestring),
            PSTR("%s %02u/%02u %02u:%02u %02u"),
            daysConvert[dt.DayOfWeek()],
            dt.Day(),
            dt.Month(),
            dt.Hour(),
            dt.Minute(),
            session);

    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print(datestring);

    return make_pair(dt.Hour(), dt.Minute());
}

void playBuzzer() {
  tone(buzzerPin, 1140, 250);
  delay(250*1.30);
  noTone(buzzerPin);
}

void loop() {
  now = Rtc.GetDateTime();

  // if (Serial.available()) {
  //   SerialBT.write(Serial.read());
  // }
  // if (SerialBT.available()) {
  //   Serial.write(SerialBT.read());
  // }
  unsigned long currentMillis = millis();
  btnStartValue = digitalRead(btn_start);
  if (btnStartValue == LOW && currentMillis - previousMillis > 200) {
    previousMillis = currentMillis;  // reset the debouncing timer
    if (isStart || counting) {
      stop_pom();
    } else {
      session = 0;
      start_pom();
    }
  }

  btnNextValue = digitalRead(btn_next);
  if (btnNextValue == LOW && currentMillis - previousMillis > 200) {
    previousMillis = currentMillis; // reset the debouncing timer
    if ((counting || isStart)) {
      if (counting || wasting) {
        counting = false;
        wasting = false;
        start_pom();
      }
      nextState();
    } else {
      if(wasting) {
        wasting = false;
        turn_off_led();
      } else {
        startMillis = millis()/1000; // Reset the start time
        wasting = true;
        counting = false;
        turn_on_all_led();
      }
    }
  }

  btnCountValue = digitalRead(btn_co);
  if (btnCountValue == LOW && currentMillis - previousMillis > 200) {
    previousMillis = currentMillis;
    startMillis = millis()/1000; // Reset the start time
    if (counting) {
      counting = false;
    }else {
      if (isStart)
        isStart = false;
      if (wasting)
        wasting = false;
      counting = true; // Set counting flag
    }
  }

  if(!counting && !wasting && !isStart) {
    display.clearDisplay();
    t = printDateTime(now);
    display.setTextSize(1);
    display.setCursor(0, 32/2);
    switch (t.first) {
      case 0 ... 5:
        display.clearDisplay();
        break;
      case 6 ... 7:
        display.print("TIME TO\nGO TO SCHOOL!");
        break;
      case 12 ... 13:
        display.print("SHORT BREAK <3");
        break;
      case 17 ... 18:
        display.print("WORK OUT\nPLAY PIANO");
        break;
      case 20 ... 21:
        display.print("TIME TO STUDY\nYOUR AIM: AUSTRALIA!");
        break;
      case 22:
        display.print("TURN OFF COMPUTER\nTIME TO READ BOOK!!!");
        if (reminder_22 && t.first == 22 && t.second == 0) {
          playBuzzer();
          reminder_22 = false;
        }
        if (!reminder_22 && t.first == 22 && t.second == 1) {
          reminder_22 = true;
        }
        break;
      case 23 ... 24:
        display.print("PREPARE TO SLEEP!");
        break;
      default:
        display.print("YOUR FUTURE\nIN YOUR HANDS!");
        break;
    }
  }

  if (counting) {
    startcounting();
  }
  
  if (wasting) {
    startwasting();
  }

  if (isStart) {
    myTime = millis()/1000;
    if (myTime >= endTime) {
      nextState();
    }
  }

  if (isStart) {
    displayTime();
  }

  display.display();
}

void displayTime() {
  display.clearDisplay();
  printDateTime(now); // Move this line here
  unsigned long remainingTime = endTime - myTime;
  int minutes = remainingTime / 60;
  int seconds = remainingTime % 60;
  char buffer[5];
  sprintf(buffer, "%02d:%02d", minutes, seconds);
  display.setTextSize(2);
  display.setCursor(128/2, 32/2); //display.width()
  display.print(buffer);
}

void startcounting() {
  if (counting) {
    display.clearDisplay();
    printDateTime(now); // Move this line here
    int currMillis_ALL = millis()/1000 - startMillis;
    display.setTextSize(2);
    display.setCursor(0, 32/2); //display.height()
    display.print(currMillis_ALL);
    if (currMillis_ALL % 900 == 0) playBuzzer();
  }
}

void startwasting() {
  if (wasting) {
    display.clearDisplay();
    printDateTime(now); // Move this line here
    int currMillis_ALL = (millis()/1000 - startMillis);
    display.setTextSize(1);
    display.setCursor(0, 32/2); //display.height()
    display.println("YOU HAVE WASTED");
    if (currMillis_ALL<120) {
      display.print(currMillis_ALL);
      display.print(" seconds");
    } else {
      display.print(currMillis_ALL/60);
      display.print(" minutes");
    }
    if (currMillis_ALL % 300 == 0) {
      turn_on_led(led_work, led_rest, led_plan);
      playBuzzer();
      turn_off_led();
      playBuzzer();
      turn_on_led(led_plan, led_rest, led_work);
      playBuzzer();
      turn_off_led();
      playBuzzer();
      turn_on_led(led_rest, led_work, led_plan);
      playBuzzer();
      turn_on_all_led();
    }
  }
}

void turn_on_led(int led_on, int led_off1, int led_off2) {
  digitalWrite(led_on, HIGH);
  digitalWrite(led_off1, LOW);
  digitalWrite(led_off2, LOW);
}

void turn_on_all_led() {
  digitalWrite(led_work, HIGH);
  digitalWrite(led_rest, HIGH);
  digitalWrite(led_plan, HIGH);
}

void turn_off_led() {
  digitalWrite(led_work, LOW);
  digitalWrite(led_rest, LOW);
  digitalWrite(led_plan, LOW);
  display.clearDisplay();
}

void start_pom() {
  if(isStart) return;
  isStart = true;
  next();
}

void stop_pom() {
  if(!isStart) return;
  isStart = false;
  counting = false;
  wasting = false;
  state = 'w';
  turn_off_led();
}

void nextState() {
  switch (state) {
    case 'w':
      Serial.println("Switching to Plan");
      state = 'p';
      break;
    case 'p':
      Serial.println("Switching to Rest");
      state = 'r';
      break;
    case 'r':
      Serial.println("Switching to Work");
      state = 'w';
      turn_off_led();
      playBuzzer();
      turn_on_led(led_rest, led_work, led_plan);
      playBuzzer();
      turn_off_led();
      playBuzzer();
      turn_on_led(led_rest, led_work, led_plan);
      playBuzzer();
      turn_off_led();
      break;
  }
  next(); // Set up the new state
}

void next() {
  playBuzzer();
  switch (state) {
    case 'w':
      turn_on_led(led_work, led_plan, led_rest);
      myTime = millis()/1000;
      endTime = myTime + workTime;
      break;
    case 'p':
      turn_on_led(led_plan, led_work, led_rest);
      myTime = millis()/1000;
      endTime = myTime + planTime;
      break;
    case 'r':
      turn_on_led(led_rest, led_work, led_plan);
      myTime = millis()/1000;
      endTime = myTime + restTime;
      ++session;
      break;
  }
}