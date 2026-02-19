/*
BakshaControl - garden, plant, greenhouse control system.
Author: @Yeralyy
*/

#include "CONFIG.h"  

#if nRF

#include "nrf.h"

#endif

#if TEST
/*
int num = 0;
bool flag = 1;

#define CE_PIN 9
#define CSN_PIN 10

Radio radio(CE_PIN, CSN_PIN);


void setup() {
  Serial.begin(9600);
  radio.radioInit(0, 1);
}

void loop() {
  uint16_t n = radio.scanRadio();
  //radio.readPipe();
  Serial.println(n);
  if (n) {
    for (int i = 0; i < n; ++i) {
      Serial.print("device id: ");
      Serial.println(radio[i].deviceId);
    }
  }
}
//#endif
*/

#include <SoftwareSerial.h>

const byte rxPin = 2;
const byte txPin = 3;

SoftwareSerial sim800l(rxPin, txPin);

void setup() {
  Serial.begin(9600);
  sim800l.begin(9600);
}

void loop() {
  if (Serial.available()) {
    sim800l.write(Serial.read());
  }

  if (sim800l.available()) {
    Serial.write(sim800l.read());
  }
}

#else

#include "states.h"
#include "lib/encMinim.h"
#include <Wire.h>
#include <RtcDS3231.h>

#include <GyverBME280.h>
#include <LiquidCrystal_I2C.h>

#if SIM800L
#include <SoftwareSerial.h>
#endif

#if ESP32
#include <SoftwareSerial.h>
#endif

#include "eeprom_control.h"
#include "display.h"
#include "arrowControl.h"
#include "schedueler.h"
#include "pid_control.h"


#define ONE_SECOND 1000
#define HALF_MINUTE 30000
#define ONE_MINUTE 60000 // 1 minute = 60 sec = 60 000 milliseconds
#define ONE_HOUR  36000000UL // 1hour = 60 minute = 3600 sec = 36 000 000 milliseconds

#define PID_DT 100 // 100 milliseconds

#define SW 7

RtcDS3231<TwoWire> rtc(Wire);

encMinim enc(3, 4, 7, 0);
LiquidCrystal_I2C lcd(0x27, 20, 4);
GyverBME280 bme;
ArrowControl arrow;

/*
int iter = 0;
uint8_t dot = 9;
uint32_t pairTmr = 0;
uint32_t searchTmr = 0;
*/

#if ESP32
SoftwareSerial esp32(13, 12); // 13 - TX, 12 - RX
#endif

#if nRF

#define CE_PIN 9
#define CSN_PIN 10

Radio radio(CE_PIN, CSN_PIN);

uint16_t n; // scanned networks

#endif

// finite state machine
FSM state {IDLE};
FSM lastState {IDLE};

uint32_t menu_tmr {0}; 
uint32_t time_tmr {0};
uint32_t pid_tmr {0};
bool gstate = 0;



void setup() {
  #if LOG
  Serial.begin(9600);
  #endif

  #if ESP32
  esp32.begin(115200);
  Serial.begin(115200);
  #endif

  #if RESET_EEPROM
  resetEEPROM();
  #endif

  #if nRF

  //Serial.begin(9600);
  radio.radioInit(0, 0);

  #endif

  lcd.init(); // display
  lcd.backlight();

  rtc.Begin(); // real time clock
  bme.begin(); // bme280

  loadClock(lcd);

  /* real time clock check & init*/

  // --------------- RTC INIT -------------------
  #if SET_TIME
  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);

  if (!rtc.IsDateTimeValid()) {
    rtc.SetDateTime(compiled);
  }

  if (!rtc.GetIsRunning()) rtc.SetIsRunning(true);

  rtc.SetDateTime(compiled);
  #endif

  RtcDateTime now = rtc.GetDateTime();
  
  #if LOG
  Serial.println(now.Hour());
  Serial.println(now.Minute());
  #endif

  // ---------------- EEPROM INIT -------------------------

  initEEPROM(state); // First run

  //state = FIRST_RUN;
  
  if (state == MAIN_MENU) {
    drawMainMenu(lcd, bme.readTemperature(), bme.readHumidity(), bme.readPressure());
    updateTime(lcd, now);
  }


}

void loop() {
  enc.tick(); // encoder handler
  RtcDateTime now = rtc.GetDateTime();
  scheduelerTick(now, radio);

  /*
  if (millis() - pid_tmr >= PID_DT) {
    pid_tmr = millis();
    PIDtick();
  }
    */

    /*
    if (millis() - timers[0] >= 3000) {
      radio.setPackageType(CONTROL_PACKAGE);
      radio.sendPackage(channels_state[i - 1]);
      channels_state[i - 1] = !channels_state[i - 1];
      timers[0] = millis();
    }
      */

  switch (state)
  {
    case MAIN_MENU:

      arrow.menuTick(enc, lcd, state);
      if (millis() - time_tmr >= 5000) {
        time_tmr = millis(); 
        #if LOG
        Serial.println(F("Updating Time"));
        #endif
        updateTime(lcd, now);
      } else if (lastState != MAIN_MENU) {
        updateTime(lcd, now);
      }

      if (millis() - menu_tmr >= ONE_SECOND) {
        menu_tmr = millis();
        drawMainMenu(lcd, bme.readTemperature(), bme.readHumidity(), bme.readPressure());
      }
      lastState = MAIN_MENU;
      break;

    
    case CHANNELS:
      arrow.channelsTick(enc, lcd, state);
      lastState = CHANNELS;
      break;

    case MODES:
      arrow.modesTick(enc, lcd, state);
      break;

    case FIRST_RUN:
      arrow.welcomeTick(enc, lcd, state);
      break;
    
    case PAIRING:
      arrow.pairingTick(enc, lcd, radio, state);
      break;
    
  }
} 

#endif
