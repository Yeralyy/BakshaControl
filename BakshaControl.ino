/*
BakshaControl - garden, plant, greenhouse control system.
Author: @Yeralyy

Credits: Thanks to all libraries authors which i used in this project
*/
#include "CONFIG.h"
#include "states.h"
#include "lib/encMinim.h"
#include "lib/rtc/RtcDS1302.h" //#include "lib/Timer.h"
#include <GyverBME280.h>
#include <LiquidCrystal_I2C.h>


#include "eeprom_control.h"
#include "display.h"
#include "arrowControl.h"
#include "schedueler.h"

#define ONE_SECOND 1000
#define HALF_MINUTE 30000
#define ONE_MINUTE 60000 // 1 minute = 60 sec = 60 000 milliseconds
#define ONE_HOUR  36000000UL // 1hour = 60 minute = 3600 sec = 36 000 000 milliseconds


#define SW 6



/*
#define s1 7
#define s2 6
#define sw 8
*/


encMinim enc(1, 7, 6, 0);
LiquidCrystal_I2C lcd(0x27, 20, 4);
ThreeWire myWire(4, 5, 2);
RtcDS1302<ThreeWire> rtc(myWire);
GyverBME280 bme;
ArrowControl arrow;


// finite state machine
FSM state {IDLE};
FSM lastState {IDLE};

uint32_t menu_tmr {0}; 
uint32_t time_tmr {0};


void setup() {
  #if LOG
  Serial.begin(9600);
  #endif

  #if RESET_EEPROM
  resetEEPROM();
  #endif

  lcd.init(); // display
  lcd.backlight();

  rtc.Begin(); // real time clock
  bme.begin(); // bme280


  /* real time clock check & init*/

  // --------------- RTC INIT -------------------
  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);

  if (!rtc.IsDateTimeValid()) {
    #if LOG
    //Serial.println("Rtc lost confidence in the DateTime!");
    #endif
    rtc.SetDateTime(compiled);
  }

  if (rtc.GetIsWriteProtected()) rtc.SetIsWriteProtected(false);
  if (!rtc.GetIsRunning()) rtc.SetIsRunning(true);

  #if SET_TIME
  rtc.SetDateTime(compiled);
  #endif

  RtcDateTime now = rtc.GetDateTime();
  

  if (now < compiled) rtc.SetDateTime(compiled);
  // ---------------- RTC INIT ------------------- 

  for (int i = 0; i < 8; ++i) {
    pinMode(channelsPins[i], OUTPUT); 
  }

  pinMode(14, INPUT);
  pinMode(15, INPUT);
  pinMode(16, INPUT);
  pinMode(17, INPUT);
  pinMode(20, INPUT);
  pinMode(21, INPUT);


  state = MAIN_MENU;
  drawMainMenu(lcd, bme.readTemperature(), bme.readHumidity());
  updateTime(lcd, now);

  if (!digitalRead(SW)) {
    factoryReset();
  }

  if (isFirstRun()) {
    initEEPROM(); // First run
    #if LOG
    Serial.println(F("fr:eeprm init"));
    #endif
  }

  #if INIT_EEPROM
  initEEPROM();
  #endif

 
  /*
  #if LOG
  
  for (int i = 1; i <= 8; ++i) {
    Channel channel {getChannel(i)};
    
    Serial.print("Channel: ");
    Serial.print(i);
    Serial.print(" Mode: ");
    
    switch (channel.mode) {
      case OFF:
        Serial.print("OFF");
        break;
      case TIMER:
        Serial.print("Timer");
        break;
      case RTC:
        Serial.print("RTC");
        break;
    }
    Serial.println();
  }
  #endif
  */
}




void loop() {

  /*
  #if LOG
  Serial.println(state);
  #endif
  */

  #if LOG
  Serial.println(analogRead(14));
  #endif

  enc.tick(); // encoder handler
  RtcDateTime now = rtc.GetDateTime();
  scheduelerTick(now);


  switch (state)
  {
    case MAIN_MENU:

      arrow.menuTick(enc, lcd, state);
      if (millis() - time_tmr >= HALF_MINUTE) {
        time_tmr = millis(); // genius code huh?
        #if LOG
        Serial.println(F("Updating Time"));
        #endif
        //RtcDateTime now = rtc.GetDateTime();
        updateTime(lcd, now);
      } else if (lastState != MAIN_MENU) {
        //RtcDateTime now = rtc.GetDateTime();
        updateTime(lcd, now);
      }

      if (millis() - menu_tmr >= ONE_SECOND) {
        menu_tmr = millis();
        drawMainMenu(lcd, bme.readTemperature(), bme.readHumidity());
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