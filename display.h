#pragma once
#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include "lib/rtc/RtcDateTime.h"

#define countof(a) (sizeof((a)) / sizeof((a)[0]))

void drawMainMenu(LiquidCrystal_I2C& lcd, float temperature, float humidity) {


    lcd.setCursor(0, 1);
    lcd.print("    ");
    lcd.setCursor(0, 1);
    lcd.print(temperature);


    lcd.setCursor(0, 2);
    lcd.print("      ");
    lcd.setCursor(0, 2);
    lcd.print(humidity);
    lcd.print('%');
}

void updateTime(LiquidCrystal_I2C& lcd, RtcDateTime& time) {

    #ifdef LOG
    Serial.print("Size of RtcDateTime: ");
    Serial.print(sizeof(time));
    Serial.println();
    #endif

    char timestring[6];
    snprintf_P(timestring,
        countof(timestring),
        PSTR("%02u:%02u"),
        time.Hour(),
        time.Minute() );

    lcd.setCursor(0, 0);
    lcd.print("    ");
    lcd.setCursor(0, 0);
    lcd.print(timestring);
}


