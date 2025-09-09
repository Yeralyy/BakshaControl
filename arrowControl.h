#pragma once
#include "lib/encMinim.h"
#include "states.h"
#include <LiquidCrystal_I2C.h>
#include "eeprom_control.h"


class ArrowControl {
    public:
        void setRange(uint8_t start_index, uint8_t end_index);
        void menuTick(encMinim& enc, LiquidCrystal_I2C& lcd, FSM& state);
        void channelsTick(encMinim& enc, LiquidCrystal_I2C& lcd, FSM& state);
    private:
        uint8_t _start_index {0};
        uint8_t _end_index;
        int8_t _count;
        bool _changedFlag {0};
        bool _first {1};


};


void ArrowControl::setRange(uint8_t start_index, uint8_t end_index) {
    _start_index = start_index; _end_index = end_index; }

void ArrowControl::menuTick(encMinim& enc, LiquidCrystal_I2C& lcd, FSM& state)  {
    if (_first) {
        /*  Service
            Channels
            Sensors*/
        lcd.setCursor(11, 0); 
        lcd.print(">Channels"); // arrow (11, 0)
        lcd.setCursor(13, 1);
        lcd.print("Service"); // arrow (11, 1);
        lcd.setCursor(13, 2);
        lcd.print("Sensors"); // arrow (11, 2);


        _count = 0;
        _first = 0;
    }

    if (enc.isTurn()) {
        if (enc.isRight()) ++_count;
        if (enc.isLeft()) --_count;
        _changedFlag = 1;  
    }

    if (enc.isClick()) {
        switch (_count) {
            case 0:
                state = CHANNELS;
                _count = 1;
                break;
            case 1:
                state = SERVICE;
                break;
            case 2:
                state = SENSORS;
                break;
        }
        lcd.clear(); // clean display
        _first = 1;
    }

    if (_count < 0) _count = 2;
    if (_count > 2) _count = 0;

    if (_changedFlag) { // 
        lcd.setCursor(11, 0);
        lcd.print(" ");
        lcd.setCursor(12, 1);
        lcd.print(" ");
        lcd.setCursor(12, 2);
        lcd.print(" ");

        switch (_count) {
            case 0:
                lcd.setCursor(11, 0);
                lcd.print('>');
                break;
            case 1:
                lcd.setCursor(12, 1);
                lcd.print('>');
                break;
            case 2:
                lcd.setCursor(12, 2);
                lcd.print('>');
                break;
        }

        _changedFlag = 0;
    }
}

void ArrowControl::channelsTick(encMinim& enc, LiquidCrystal_I2C& lcd, FSM& state) {
    if (_first) {

        if (_count < 1) _count = 1;
        if (_count > CHANNELS) _count = CHANNELS;

        lcd.setCursor(0, 0);
        lcd.print(">");

        lcd.setCursor(1, 0);
        lcd.print("Channel ");

        lcd.print(_count);


        // getting N channel 

        Channel currentChannel {getChannel(_count)};
        #if TEST
        currentChannel.mode = TIMER;
        #endif
        switch (currentChannel.mode) {
            case OFF:
                lcd.setCursor(17, 0);
                lcd.print("Off");
                break;
            case TIMER:
                lcd.setCursor(18, 0);
                lcd.print("On");

                lcd.setCursor(0, 1);
                lcd.print("Mode: ");
                lcd.print("<Timer>");

                //lcd.setCursor(0, 2);
                //lcd.print("Uptime: ");
            
                //lcd.print(currentChannel.timer * 60000);  // in minutes
                //lcd.print("min");

                lcd.setCursor(16, 3);
                lcd.print("Back");
                break;

            case RTC:
                lcd.setCursor(18, 0);
                lcd.print("On");
                
                lcd.setCursor(0, 1);
                lcd.print("Mode: ");
                lcd.print("<RTC>");

                lcd.setCursor(16, 3);
                lcd.print("Back");

                //lcd.print("00.00 00:00");
                break;

            case SENSOR:
                lcd.setCursor(18, 0);
                lcd.print("On");

                lcd.setCursor(0, 1);
                lcd.print("Mode: ");
                lcd.print("<Sensor>");

                lcd.setCursor(16, 3);
                lcd.print("Back");
        }

        _first = 0; }

        if (enc.isTurn()) {
            if (enc.isRightH()) {++_count; _first = 1;}
            if (enc.isLeftH()) {--_count; _first = 1;}
            _changedFlag = 1;
        }

        if (enc.isClick()) { state = MAIN_MENU; lcd.clear(); _first = 1;}

        

    

}