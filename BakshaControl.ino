/*
BakshaControl - garden, plant, greenhouse control system.
Author: @Yeralyy
*/

#include "CONFIG.h"  

#if nRF

#include "nrf.h"

#if TEST
bool flag = 1;
void setup() {
  Serial.begin(9600);
  radioInit();
}

void loop() {
  sendPackage(flag);
  flag = !flag;
  delay(1000);
}
#endif

#endif

#include "states.h"
#include "lib/encMinim.h"
#include <Wire.h>
#include <RtcDS3231.h>

#include <GyverBME280.h>
#include <LiquidCrystal_I2C.h>

#if SIM800L
#include <SoftwareSerial.h>
#include "string.h"
#define BUF_SIZE 64
#define INTERRUPT_PIN 3
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


#if SIM800L
SoftwareSerial sim800l(6, 5); // 5 - TX, 6 - RX
char buffer[BUF_SIZE];
#endif

#if ESP32
SoftwareSerial esp32(13, 12); // 13 - TX, 12 - RX
#endif


// finite state machine
FSM state {IDLE};
FSM lastState {IDLE};

uint32_t menu_tmr {0}; 
uint32_t time_tmr {0};
uint32_t pid_tmr {0};



#if SIM800L
char buf[BUF_SIZE];
volatile bool ringingFlag {0};
#endif


#if SIM800L
void ringing() {
  ringingFlag = 1;
}
#endif



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

  #if SIM800L
  Serial.begin(9600);
  sim800l.begin(9600);
  //sim800l.println(F("ATE0V0+CMEE=1;&W"));


  pinMode(INTERRUPT_PIN, INPUT_PULLUP);
  
  // attaching 3th pin for interrupt RING
  attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), ringing, RISING);
  #endif

  #if nRF

  radioInit();

  #endif

  lcd.init(); // display
  lcd.backlight();

  rtc.Begin(); // real time clock
  bme.begin(); // bme280

  loadClock(lcd);

  /* real time clock check & init*/

  // --------------- RTC INIT -------------------
  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);

  if (!rtc.IsDateTimeValid()) {
    rtc.SetDateTime(compiled);
  }

  if (!rtc.GetIsRunning()) rtc.SetIsRunning(true);

  #if SET_TIME
  rtc.SetDateTime(compiled);
  #endif

  RtcDateTime now = rtc.GetDateTime();
  
  #if LOG
  Serial.println(now.Hour());
  Serial.println(now.Minute());
  #endif

  if (now < compiled) rtc.SetDateTime(compiled);
  // ---------------- RTC INIT ------------------- 

  state = MAIN_MENU;
  drawMainMenu(lcd, bme.readTemperature(), bme.readHumidity(), bme.readPressure());
  updateTime(lcd, now);


  //resetEEPROM();

  initEEPROM(); // First run

  #if INIT_EEPROM
  initEEPROM();
  #endif
}

void loop() {

  #if SIM800L
  // ATE0V0+CMEE=1;&W no logging
  // ATE1V1+CMEE=2;&W for logging
  // AT+CMGS="+7928xxxxxxx" sending sms
  // ATD+7777xxxxxxxx; call
  

  //AT+CMGDA="DEL ALL" delete all sms
  //AT+CMGL="REC UNREAD",1 list of sms
  //AT+CMGR=7,1 read 7th sms in memmory of SIM
  //AT+CMGS="+7928xxxxxxx" send sms

  
  /*
  if (ringingFlag)  { // Action. SMS/CALL
    if (sim800l.available()) {
      //Serial.write(sim800l.read());
      //buf.strcat
    }
  }
    */
    

  if (sim800l.available() > 0) {
    /*
    uint8_t symbol = sim800l.read();
    Serial.print(symbol);
    if (symbol == 10) Serial.println();
    else Serial.print(' ');
    */
    Serial.write(sim800l.read());
  }

  if (Serial.available()) {
    sim800l.write(Serial.read());
  }
  #endif

  RtcDateTime now = rtc.GetDateTime();
  //scheduelerTick(now);
  enc.tick(); // encoder handler

  if (millis() - pid_tmr >= PID_DT) {
    pid_tmr = millis();
    PIDtick();
  }

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
  }
} 