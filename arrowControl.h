#pragma once
#include "lib/encMinim.h"
#include <LiquidCrystal_I2C.h>
#include "eeprom_control.h"
#include "lib/rtc/RtcDateTime.h"
#include "utils.h"


#include "states.h"


// MUSTAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAARD

class ArrowControl {
    public:
        void menuTick(encMinim& enc, LiquidCrystal_I2C& lcd, FSM& state);
        void channelsTick(encMinim& enc, LiquidCrystal_I2C& lcd, FSM& state);
        void modesTick(encMinim& enc, LiquidCrystal_I2C& lcd, FSM& state);
    private:
        Channel currentChannel;

        uint32_t _tmr {0};

        int8_t _count; int8_t _index; // arrow index
        int8_t _indexFlag;
        int8_t _dayIndex {0};
        int8_t _dayIndexFlag {0};

        // for modes preferenes
        bool _changedFlag {0}; bool _first {1};
        bool _inChannelFlag {0};
        bool _newReadFlag {1}; // Flag for EEPROM read
        bool _channelFlag {1};
	    bool _settingsChanged {0};
        bool _modesFlag = {1};

        bool _scrollFlag = {0};

        uint8_t _oneByte {0};
        uint8_t _switchByte {0};
        Mode _lastMode;

        void redrawDisplay(LiquidCrystal_I2C& lcd, FSM& state);
        void updateDisplay(LiquidCrystal_I2C& lcd, FSM& state);
        void constrainHelper();
        void switchHelper(encMinim& enc,  const bool isRight);
        //void calculateLeft(LiquidCrystal_I2C& lcd, FSM& state);
        //void constrainModes(void);
};


// ====================================== CODE ======================================

void ArrowControl::menuTick(encMinim& enc, LiquidCrystal_I2C& lcd, FSM& state)  {
    redrawDisplay(lcd, state);

    if (enc.isTurn()) {
        if (enc.isRight()) ++_count;
        if (enc.isLeft()) --_count;
        _changedFlag = 1;  
        #if LOG
        Serial.println(F("MAIN_MENU:Enc t"));
        #endif
    }

    if (enc.isClick()) {
        switch (_count) {
            case 0:
                state = CHANNELS;
                _count = 1;
                #if LOG
                Serial.println(F("State->CHANNELS"));
                #endif

                break;
            case 1:
                state = SERVICE;
                #if LOG
                Serial.println(F("State->SERVICE"));
                #endif

                break;
            case 2:
                state = SENSORS; // FSM& 
                #if LOG
                Serial.println(F("State->DAY"));
                #endif

                break;
        }
        lcd.clear(); // clean display
        _first = 1;
    }

    if (_count < 0) _count = 2;
    if (_count > 2) _count = 0;

    updateDisplay(lcd, state); // arrows

}

void ArrowControl::constrainHelper() {
        if (currentChannel.mode != OFF) {
            if (_index < 0) { _index = 0; _changedFlag = 0; }
            else if (_index > 4) {_index = 4; _changedFlag = 0; } // constraining
        } else {
            if (_index < 0) { _index = 0; _changedFlag = 0; } // if mode is OFF
            else if (_index == 2 && _indexFlag != 4) {_index = 4; }
            else if (_index == 3) {_index = 1; }
            else if (_index > 4) {_index = 4; _changedFlag = 0; }
        }

}

void ArrowControl::switchHelper(encMinim& enc, const bool isRight = 1) {
    switch (currentChannel.mode) {
        case TIMER:
            if (_switchByte & (1 << 0)) {
                if (currentChannel.data.timerMode.periodHour > 23) currentChannel.data.timerMode.periodHour = 0;
                if (currentChannel.data.timerMode.periodMinute > 59) currentChannel.data.timerMode.periodMinute = 0;
                if (currentChannel.data.timerMode.periodSecond > 59) currentChannel.data.timerMode.periodSecond = 0;

                if (currentChannel.data.timerMode.workMinute > 99) currentChannel.data.timerMode.workMinute = 0;
                if (currentChannel.data.timerMode.workSecond > 59) currentChannel.data.timerMode.workSecond = 0;
                _switchByte &= ~(1 << 0);
            }

            if (_switchByte & (1 << 1)) {
                switch (_index) {
                    case 1: // period hours
                        settingsHandle(enc, currentChannel.data.timerMode.periodHour, 0, 23, 3, isRight);
                        
                        _oneByte |= 1 << 0; // 0th bit for periodHour
                        break;
                    case 2:
                        settingsHandle(enc, currentChannel.data.timerMode.periodMinute, 0, 59, 5, isRight);

                        _oneByte |= 1 << 1; // 1th bit for periodMinute
                        break;
                    case 3:
                        settingsHandle(enc, currentChannel.data.timerMode.periodSecond, 0, 59, 5, isRight);

                        _oneByte |= 1 << 2; // 2th bit for periodSecond
                        break;
                    
                    case 4:
                        settingsHandle(enc, currentChannel.data.timerMode.workMinute, 0, 99, 5, isRight);
                        
                        _oneByte |= 1 << 3; // 3th bit for workMinute
                        break;
                    case 5:
                        settingsHandle(enc, currentChannel.data.timerMode.workSecond, 0, 59, 5, isRight);
                        _oneByte |= 1 << 4; // 4th bit for workSecond
                        break;
                    }

                _switchByte &= ~(1 << 1);
            }

            break;
        case SENSOR:
            if (_switchByte & (1 << 0)) {
                if (currentChannel.data.sensorMode.threshold < 0 || currentChannel.data.sensorMode.threshold > 1023) currentChannel.data.sensorMode.threshold = 1023;
                if (currentChannel.data.sensorMode.workMinute > 99) currentChannel.data.sensorMode.workMinute = 0;
                if (currentChannel.data.sensorMode.workSecond > 59) currentChannel.data.sensorMode.workSecond = 0;
                currentChannel.data.sensorMode.pin = sensorsPins[0]; // A0 default
                _switchByte &= ~(1 << 0);
            }

            if (_switchByte & (1 << 1)) {
                switch (_index) {
                    case 1:
                        settingsHandle(enc, currentChannel.data.sensorMode.threshold, 0, 1023, 50, isRight);
                        _oneByte |= 1 << 0;
                        break;
                    case 2:
                        settingsHandle(enc, currentChannel.data.sensorMode.workMinute, 0, 99, 10, isRight);
                        _oneByte |= 1 << 1;
                        break;
                    case 3:
                        settingsHandle(enc, currentChannel.data.sensorMode.workSecond, 0, 59, 5, isRight);
                        _oneByte |= 1 << 2;
                        break;
                    case 4:
                        if (isRight) ++currentChannel.data.sensorMode.pin;
                        else --currentChannel.data.sensorMode.pin;
                        pinConstrain(currentChannel.data.sensorMode.pin, isRight);
                        _oneByte |= 1 << 3;
                        break;
                }
               _switchByte &= ~(1 << 1);
            }

            break;
        case WEEK:
            if (_switchByte & (1 << 0)) {
                currentChannel.data.weekMode.days[0] = {0, 0, 0, 0, 0, 0, 1}; // by default at least 1 day is enabled
                currentChannel.data.weekMode.days[1] = {0, 0, 0, 0, 0, 0, 0}; // by default at least 1 day is enabled
                currentChannel.data.weekMode.days[2] = {0, 0, 0, 0, 0, 0, 0}; // by default at least 1 day is enabled
                currentChannel.data.weekMode.days[3] = {0, 0, 0, 0, 0, 0, 0}; // by default at least 1 day is enabled
                currentChannel.data.weekMode.days[4] = {0, 0, 0, 0, 0, 0, 0}; // by default at least 1 day is enabled
                currentChannel.data.weekMode.days[5] = {0, 0, 0, 0, 0, 0, 0}; // by default at least 1 day is enabled
                currentChannel.data.weekMode.days[6] = {0, 0, 0, 0, 0, 0, 0}; // by default at least 1 day is enabled
                _switchByte &= ~(1 << 0);
            }

            if (_switchByte & (1 << 1)) {
                switch (_index) {
                     case 1:
                        if (isRight) ++_dayIndex;
                        else --_dayIndex;

                        if (_dayIndex > 6) _dayIndex = 0;
                        else if (_dayIndex < 0) _dayIndex = 6;

                        _changedFlag = 1;
                        break;
                    
                    case 2:
                        if (isRight) {
                            if (!currentChannel.data.weekMode.days[_dayIndex].enabled) { _changedFlag = 1; currentChannel.data.weekMode.days[_dayIndex].enabled = 1; _oneByte |= (1 << 7); }
                            else {_changedFlag = 0; }
                        } else {
                            if (currentChannel.data.weekMode.days[_dayIndex].enabled) { _changedFlag = 1; currentChannel.data.weekMode.days[_dayIndex].enabled = 0; _oneByte |= (1 << 7); }
                            else {_changedFlag = 0; }
                        }

                        break;
                    case 3:
                        settingsHandle(enc, currentChannel.data.weekMode.days[_dayIndex].startHour, 0, 23, 3, isRight);
                        _oneByte |= 1 << 0 ; // 0b01 

                        break;
                    case 4:
                        settingsHandle(enc, currentChannel.data.weekMode.days[_dayIndex].startMinute, 0, 59, 5, isRight);
                        _oneByte |= 1 << 1;
                        
                        break;
                    case 5:
                        settingsHandle(enc, currentChannel.data.weekMode.days[_dayIndex].startSecond, 0, 59, 5, isRight);
                        _oneByte |= 1 << 2; 

                        break;
                    case 6:
                        settingsHandle(enc, currentChannel.data.weekMode.days[_dayIndex].endHour, 0, 23, 3, isRight);
                        _oneByte |= 1 << 3; // 3th bit for endHour 
                        
                        break;
                    case 7:
                        settingsHandle(enc, currentChannel.data.weekMode.days[_dayIndex].endMinute, 0, 59, 5, isRight);
                        _oneByte |= 1 << 4; 
                        
                        break;
                    case 8:
                        settingsHandle(enc, currentChannel.data.weekMode.days[_dayIndex].endSecond, 0, 59, 5, isRight);
                        _oneByte |= 1 << 5; 

                        break;
                }
                _switchByte &= ~(1 << 1);
            }
            
            break;        

        case DAY:
            if (_switchByte & (1 << 0)) {
                if (currentChannel.data.dayMode.startHour > 23) currentChannel.data.dayMode.startHour = 0;
                if (currentChannel.data.dayMode.startMinute > 59) currentChannel.data.dayMode.startMinute = 0;
                if (currentChannel.data.dayMode.startSecond > 59) currentChannel.data.dayMode.startSecond = 0;
                if (currentChannel.data.dayMode.endHour > 23) currentChannel.data.dayMode.endHour = 0;
                if (currentChannel.data.dayMode.endMinute > 59) currentChannel.data.dayMode.endMinute = 0;
                if (currentChannel.data.dayMode.endSecond > 59) currentChannel.data.dayMode.endSecond = 0;
                _switchByte &= ~(1 << 0);
            }

            if (_switchByte & (1 << 1)) {
                switch (_index) {
                     case 1:
                        settingsHandle(enc, currentChannel.data.dayMode.startHour, 0, 23, 3, isRight);
                        _oneByte |= 1 << 0 ; // 0b01 
                        //0th bit for startHour

                        break;
                    case 2:
                        settingsHandle(enc, currentChannel.data.dayMode.startMinute, 0, 59, 5, isRight);
                        _oneByte |= 1 << 1;
                        
                        break;
                    case 3:
                        settingsHandle(enc, currentChannel.data.dayMode.startSecond, 0, 59, 5, isRight);
                        _oneByte |= 1 << 2; 

                        break;
                    case 4:
                        settingsHandle(enc, currentChannel.data.dayMode.endHour, 0, 23, 3, isRight);
                        _oneByte |= 1 << 3;// 3th bit for endHour 
                        break;
                    case 5:
                        settingsHandle(enc, currentChannel.data.dayMode.endMinute, 0, 59, 5, isRight);
                        _oneByte |= 1 << 4; 
                        break;
                    case 6:
                        settingsHandle(enc, currentChannel.data.dayMode.endSecond, 0, 59, 5, isRight);
                        _oneByte |= 1 << 5; 
                        break;                
                }
                _switchByte &= ~(1 << 1);
            }

        case PID:
            if (_switchByte & (1 << 0)) {
                currentChannel.data.PidMode.Kp = 0.0f;
                currentChannel.data.PidMode.Ki = 0.0f;
                currentChannel.data.PidMode.Kd = 0.0f;

                currentChannel.data.PidMode.setPoint = 0;
                currentChannel.data.PidMode.pin = 16; // A2 default
                _switchByte &= ~(1 << 0);
            }

            if (_switchByte & (1 << 1)) {
                switch (_index) {
                    case 1:
                        if (isRight) {
                            if (enc.isFast()) currentChannel.data.PidMode.Kp += 5;
                            else ++currentChannel.data.PidMode.Kp;
                        } else {
                            if (enc.isFast()) currentChannel.data.PidMode.Kp -= 5;
                            else --currentChannel.data.PidMode.Kp;
                        }
                        _oneByte |= (1 << 0);
                        break;
                    case 2:
                        if (isRight) {
                            if (enc.isFast()) currentChannel.data.PidMode.Kp += 0.05f;
                            else currentChannel.data.PidMode.Kp += 0.01f;
                        } else {
                            if (enc.isFast()) currentChannel.data.PidMode.Kp -= 0.05f;
                            else currentChannel.data.PidMode.Kp -= 0.01f;
                        }
                        _oneByte |= (1 << 1);
                        break;
                    case 3:
                        if (isRight) {
                            if (enc.isFast()) currentChannel.data.PidMode.Ki += 5;
                            else ++currentChannel.data.PidMode.Ki;
                        } else {
                            if (enc.isFast()) currentChannel.data.PidMode.Ki -= 5;
                            else --currentChannel.data.PidMode.Ki;
                        }
                        _oneByte |= (1 << 2);
                        break;
                    case 4:
                        if (isRight) {
                            if (enc.isFast()) currentChannel.data.PidMode.Ki += 0.05f;
                            else currentChannel.data.PidMode.Ki += 0.01f;
                        } else {
                            if (enc.isFast()) currentChannel.data.PidMode.Ki -= 0.05f;
                            else currentChannel.data.PidMode.Ki -= 0.01f;
                        }
                        _oneByte |= (1 << 3);
                        break;
                    case 5:
                        if (isRight) {
                            if (enc.isFast()) currentChannel.data.PidMode.Kd += 5;
                            else ++currentChannel.data.PidMode.Kd;
                        } else {
                            if (enc.isFast()) currentChannel.data.PidMode.Kd -= 5;
                            else --currentChannel.data.PidMode.Kd;
                        }
                        _oneByte |= (1 << 4);
                        break;
                    case 6:
                        if (isRight) {
                            if (enc.isFast()) currentChannel.data.PidMode.Kd += 0.05f;
                            else currentChannel.data.PidMode.Kd += 0.01f;
                        } else {
                            if (enc.isFast()) currentChannel.data.PidMode.Kd -= 0.05f;
                            else currentChannel.data.PidMode.Kd -= 0.01f;
                        }
                        _oneByte |= (1 << 5);
                        break;
                    case 7:
                        if (isRight) {
                            if (enc.isFast()) currentChannel.data.PidMode.setPoint += 50;
                            else ++currentChannel.data.PidMode.setPoint;
                        } else {
                            if (enc.isFast()) currentChannel.data.PidMode.setPoint -= 50;
                            else --currentChannel.data.PidMode.setPoint;
                        }
                        _oneByte |= (1 << 6);

                        break;
                    case 8:
                        if (isRight) ++currentChannel.data.PidMode.pin;
                        else --currentChannel.data.PidMode.pin;

                        pinConstrain(currentChannel.data.PidMode.pin, isRight);

                        _oneByte |= 1 << 7; // 3th bit for analog pin change
                        break;
                }
                
                _switchByte &= ~(1 << 1);
            }
            
            break;    
    }
}

void ArrowControl::channelsTick(encMinim& enc, LiquidCrystal_I2C& lcd, FSM& state) {
    redrawDisplay(lcd, state);


        // ==================================== ARROW TRACKING ==============================

        if (_inChannelFlag) {
            // arrow pos tracking
            if (enc.isRight()) {
                _indexFlag = _index;
                ++_index;
                _changedFlag = 1; }
                    
            if (enc.isLeft()) {
                _indexFlag = _index;
                --_index;
                _changedFlag = 1; }


            constrainHelper();
            
            if (_changedFlag) updateDisplay(lcd, state);    

            if (_index == 4 && enc.isClick()) {
                #if LOG
                Serial.println(F("state->MAIN_MENU"));
                #endif
                lcd.clear();
                state = MAIN_MENU; // MAIN_MENU
                _first = 1;
                _inChannelFlag = 0;
                _index = 0;


                if (currentChannel.mode == OFF) digitalWrite(channelsPins[_count - 1], !uint8_t(currentChannel.relayMode));
                // saving current channel settings
                if (_channelFlag) {
                    putChannel(_count, currentChannel);
                    _channelFlag = 0;
                }
            }

            if (_index == 2 && enc.isClick()) {
                #if LOG
                Serial.println(F("state->MODES"));
                #endif
                lcd.clear();

                if (_channelFlag) {
                    putChannel(_count, currentChannel);
                    _channelFlag = 0;
                }
                
                state = MODES;
                _first = 1;
                _inChannelFlag = 0;
                _index = 0;
                if (_modesFlag && _lastMode != currentChannel.mode) {
                    _switchByte |= (1 << 0);
                    switchHelper(enc);
                }
            }
                
            /* Detecting enc actions */
            if (enc.isRightH()) {
                switch (_index) {
                    case 0: // 
                        /* Saving Channel settings*/

                        if (currentChannel.mode == OFF) digitalWrite(channelsPins[_count - 1], !uint8_t(currentChannel.relayMode));

                        if (_channelFlag) {
                            putChannel(_count, currentChannel);
                            _channelFlag = 0;
                        }

                        ++_count; // saving settings of current Channel and moving to the next
                        _first = 1; 
                        _inChannelFlag = 0; 

                        lcd.setCursor(0, 1);
                        lcd.print("                   ");
                        lcd.setCursor(0, 2);
                        lcd.print("                   ");

                        _channelFlag = 1;

                        #if LOG
                        Serial.println(F("++Channel Prev saved"));
                        #endif

                        break;


                    case 1:
                        // ON/OFF
                        if (currentChannel.mode == OFF) currentChannel.mode = TIMER;  // default
                        lcd.setCursor(16, 0);
                        lcd.print("    ");
                        lcd.setCursor(17, 0);
                        lcd.print(">On");

                        lcd.setCursor(0, 1);
                        switch (currentChannel.mode) {
                            case TIMER:
                                lcd.setCursor(0, 1);
                                lcd.print("Mode: <Timer>");
                                break;
                            
                            case PID:
                                lcd.setCursor(0, 1);
                                lcd.print("Mode: <PID>");
                                break;
                            
                            case DAY:
                                lcd.setCursor(0, 1);
                                lcd.print("Mode: <Day>");
                                break;
                            case SENSOR:
                                lcd.setCursor(0, 1);
                                lcd.print("Mode: <Sensor>");
                        }

                        lcd.setCursor(0, 2);
                        if (currentChannel.relayMode == TO_ON) lcd.print("Direction: Off-On");
                        else lcd.print("Direction: On-Off");
                        
                        _changedFlag = 1;

                        break;
                    
                    case 2:
                        #if LOG
                        Serial.println(F("Changing mode++"));            
                        #endif

                        if (currentChannel.mode != OFF) currentChannel.mode++; // Next Mode

                        lcd.setCursor(0, 1); 
                        lcd.print("                   "); // clearing Mode
                        #if LOG
                        Serial.println(F("Clearing mode"));
                        #endif

                        switch (currentChannel.mode) {
                            case TIMER:
                                lcd.setCursor(0, 1);
                                lcd.print("Mode: <Timer>");
                                break;
                            
                            case PID:
                                lcd.setCursor(0, 1);
                                lcd.print("Mode: <PID>");
                                break;
                            
                            case DAY:
                                lcd.setCursor(0, 1);
                                lcd.print("Mode: <Day>");
                                break;
                            
                            case SENSOR:
                                lcd.setCursor(0, 1);
                                lcd.print("Mode: <Sensor>");
                                break;
                            
                            case WEEK:
                                lcd.setCursor(0, 1);
                                lcd.print("Mode: <Week>");
                                break;

                            default:
                                #if LOG
                                Serial.println("Unxecpected mode");
                                #endif
                                break;
                        }

                        _channelFlag = 1;
                        
                        break;

                    case 3:
                        if (currentChannel.relayMode == TO_OFF) {
                            currentChannel.relayMode = TO_ON; 
                            lcd.setCursor(10, 2);
                            lcd.print("          ");
                            lcd.setCursor(10, 2);
                            lcd.print(">Off-On");

                            _channelFlag = 1;
                        }
                        break;

                    }
                }

            if (enc.isLeftH()) {
                switch (_index) {
                    case 0:
                        /* Saving channel settings in EEPROM */
                        if (currentChannel.mode == OFF) digitalWrite(channelsPins[_count - 1], !uint8_t(currentChannel.relayMode));

                        if (_channelFlag) { 
                            putChannel(_count, currentChannel);
                            _channelFlag = 0;
                        }

                        --_count;
                        _first = 1;
                        _inChannelFlag = 0;
                        _changedFlag = 1;

                        lcd.setCursor(0, 1);
                        lcd.print("                   ");
                        lcd.setCursor(0, 2);
                        lcd.print("                   ");

                        _channelFlag = 1;

                        #if LOG
                        Serial.println(F("-Channel Current saved"));
                        #endif

                        break;
                    
                    case 1:

                        if (currentChannel.mode != OFF) currentChannel.mode = OFF;

                        lcd.setCursor(17, 0);
                        lcd.print("   ");
                        lcd.setCursor(16, 0);
                        lcd.print(">Off");

                        lcd.setCursor(0, 1);
                        lcd.print("                   "); // clearing Mode

                        lcd.setCursor(0, 2);
                        lcd.print("                   ");

                        _channelFlag = 1;
                        
                        break;

                    case 2:
                        if (currentChannel.mode != OFF) currentChannel.mode--; // Decrement Mode

                        #if LOG
                        Serial.println(F("Changing mode--"));            
                        #endif

                        lcd.setCursor(0, 1);
                        lcd.print("                   ");

                        switch (currentChannel.mode) {
                            case TIMER:
                                lcd.setCursor(0, 1);
                                lcd.print(">Mode: <Timer>");
                                break;
                            
                            case PID: 
                                lcd.setCursor(0, 1);
                                lcd.print(">Mode: <PID>");
                                break;
                            
                            case DAY:
                                lcd.setCursor(0, 1);
                                lcd.print(">Mode: <Day>");
                                break;
                            
                            case SENSOR:
                                lcd.setCursor(0, 1);
                                lcd.print(">Mode: <Sensor>"); 
                                break;

                            case WEEK:
                                lcd.setCursor(0, 1);
                                lcd.print(">Mode: <Week>");
                                break;

                            default:
                                #if LOG
                                Serial.println(F("Unexptected Mode"));
                                #endif
                                break;
                        }

                        _channelFlag = 1;
                        
                        break;
                    
                        case 3:
                        if (currentChannel.relayMode == TO_ON) {
                            currentChannel.relayMode = TO_OFF; 
                            lcd.setCursor(10, 2);
                            lcd.print("          ");
                            lcd.setCursor(10, 2);
                            lcd.print(">On-Off");
                            _channelFlag = 1;
                        }
                        break;
                }
            }
        } 

        if (enc.isClick()) { 
            _inChannelFlag = 1;
            #if LOG
            Serial.println(F("!In Channel"));
            #endif
        }


        if (enc.isRightH() && !_inChannelFlag) {++_count; _first = 1; _changedFlag = 1; }
        if (enc.isLeftH() && !_inChannelFlag) {--_count; _first = 1; _changedFlag = 1; }


        updateDisplay(lcd, state);

        }


void ArrowControl::modesTick(encMinim& enc, LiquidCrystal_I2C& lcd, FSM& state) { // modesfunc
    redrawDisplay(lcd, state);

    if (_index != 0 && enc.isClick())  {
        _scrollFlag = !_scrollFlag;      
    }

    if (!_scrollFlag) {

        if (enc.isRight()) {
            _indexFlag = _index;
            ++_index;
            _changedFlag = 1;
        }

        if (enc.isLeft()) {
            _indexFlag = _index;
            --_index;
            _changedFlag = 1;
        }
    }


    if (_index < 0 && _indexFlag == 0) {_changedFlag = 0; _index = 0; }
   switch (currentChannel.mode) {
        case TIMER:
            if (_index > 5 && _indexFlag == 5) {_changedFlag = 0; _index = 5; }
            break;
        case DAY:
            if (_index > 6 && _indexFlag == 6) {_changedFlag = 0; _index = 6; }
            break;
        case PID:
            if (_index > 8 && _indexFlag == 8) {_changedFlag = 0; _index = 8; }
            break;

        case SENSOR:
            if (_index > 4 && _indexFlag == 4) {_changedFlag = 0; _index = 4; }
            break;    
        
        case WEEK:
            if (currentChannel.data.weekMode.days[_dayIndex].enabled) {
                if (_index > 8 && _indexFlag == 8) {_changedFlag = 0; _index = 8; }
            } else {
                if (_index > 2 && _indexFlag == 2) {_changedFlag = 0; _index = 2; }
            }
            break;
   }

    if (_changedFlag || _settingsChanged || (millis() - _tmr > 500)) updateDisplay(lcd, state);



    if (_scrollFlag && enc.isRight()) {
        _switchByte |= (1 << 1);
        _settingsChanged = 1;
        switchHelper(enc, 1);
    }

    if (_scrollFlag && enc.isLeft()) {
        _switchByte |= (1 << 1);
        _settingsChanged = 1;
        switchHelper(enc, 0);
    }

   if (_index == 0 && enc.isClick()) {
    state = CHANNELS;
    _index = 0;
    _inChannelFlag = 1;
    lcd.clear();
    _first = 1;
    _oneByte = 0;
    _lastMode = currentChannel.mode;
    _modesFlag = 1;

    // saving channel settings
    putChannel(_count, currentChannel);
   }


}






void ArrowControl::redrawDisplay(LiquidCrystal_I2C& lcd, FSM& state) { // redrawfunc
    if (_first) {
        switch (state) {
            case MAIN_MENU:
                #if LOG
                Serial.println(F("Draw MAIN_MENU"));
                #endif
        
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

                break;
            case CHANNELS:
                #if LOG
                Serial.println(F("Drawing CHANNELS _first = 1"));
                #endif

                if (_count < 1) _count = 1;
                if (_count > CHANNELS_COUNT) _count = CHANNELS_COUNT;

                lcd.setCursor(0, 0);
                lcd.print(">");

                lcd.setCursor(1, 0);
                lcd.print("Channel ");

                lcd.print(_count);


                // getting N channel 

                if (_newReadFlag) {
                currentChannel = {getChannel(_count)};
                #if LOG
                Serial.println(F("new Channel"));
                #endif
                }
                #if TEST
                currentChannel.mode = TIMER;
                #endif

                lcd.setCursor(0, 1);
                lcd.print("                   ");

                lcd.setCursor(0, 2);
                lcd.print("                   ");

                lcd.setCursor(15, 0);
                lcd.print("     ");

                if (currentChannel.mode == OFF) {
                    lcd.setCursor(17, 0);
                    lcd.print("Off");
                } else {
                    lcd.setCursor(18, 0);
                    lcd.print("On");
                    lcd.setCursor(0, 2);
                    if (currentChannel.relayMode == TO_ON) lcd.print("Direction: Off-On");
                    else lcd.print("Direction: On-Off");

                } 
                

                switch (currentChannel.mode) {
                    case OFF:
                        lcd.setCursor(16, 3);
                        lcd.print("Back");
                        break;
                    case TIMER:
                        lcd.setCursor(0, 1);
                        lcd.print("Mode: <Timer>");

                        break;

                    case PID:
                        lcd.setCursor(0, 1);
                        lcd.print("Mode: <PID>");

                        break;

                    case DAY:
                        lcd.setCursor(0, 1);
                        lcd.print("Mode: <Day>");

                        break;
                    
                    case SENSOR:
                        lcd.setCursor(0, 1);
                        lcd.print("Mode: <Sensor>");

                        break;

                    case WEEK:
                        lcd.setCursor(0, 1);
                        lcd.print("Mode: <Week>");

                        break;

                    }


                lcd.setCursor(16, 3);
                lcd.print("Back");
            break;

            case MODES:
                switch (currentChannel.mode) {
                    case TIMER:
                        lcd.setCursor(0, 0);
                        lcd.print("<TIMER>");


                        lcd.setCursor(0, 1);
                        lcd.print("Period: "); // period. e.g: every 5hours, every 5 mins
                        //target
                        print2digits(currentChannel.data.timerMode.periodHour, lcd, 8, 1);
                        lcd.print("h ");
                        print2digits(currentChannel.data.timerMode.periodMinute, lcd, 12, 1);
                        lcd.print("m ");
                        print2digits(currentChannel.data.timerMode.periodSecond, lcd, 16, 1);
                        lcd.print('s');
                        

                        lcd.setCursor(0, 2);
                        lcd.print("Work: "); // work time 
                        print2digits(currentChannel.data.timerMode.workMinute, lcd, 6, 2);
                        lcd.print("m ");
                        print2digits(currentChannel.data.timerMode.workSecond, lcd, 10, 2);
                        lcd.print('s');
                        
                        lcd.setCursor(0, 3);
                        lcd.print("Left: 00h 00m 00s");
                        break;

                    case PID: 

                        lcd.setCursor(0, 0);
                        lcd.print("<PID>");

                        lcd.setCursor(0, 1);
                        lcd.print("Kp: 00.00");
                        lcd.setCursor(0, 2);
                        lcd.print("Ki: 00.00");
                        lcd.setCursor(0, 3);
                        lcd.print("Kd: 00.00");

                        lcd.setCursor(11, 1);
                        lcd.print("Set: ");
                        lcd.print(currentChannel.data.PidMode.setPoint);
                        
                        lcd.setCursor(11, 2);
                        lcd.print("Pin: A");
                        
                        switch (currentChannel.data.PidMode.pin) {
                            case 16:
                                lcd.print('2');
                                break;
                            case 17:
                                lcd.print('3');
                                break;
                            case 20:
                                lcd.print('6');
                                break;
                            case 21:
                                lcd.print('7');
                                break;
                            default:
                                lcd.print('2');
                        }
                        

                        lcd.setCursor(11, 3);
                        lcd.print("Val: ");
                        lcd.print(analogRead(currentChannel.data.PidMode.pin));

                        
                        break;

                    case DAY:
                        lcd.setCursor(0, 0);
                        lcd.print("<Day>");

                        lcd.setCursor(0, 1);
                        lcd.print("Start: ");
                        print2digits(currentChannel.data.dayMode.startHour, lcd, 7, 1);
                        lcd.print(':');
                        print2digits(currentChannel.data.dayMode.startMinute, lcd, 10, 1);
                        lcd.print('.');
                        print2digits(currentChannel.data.dayMode.startSecond, lcd, 13, 1);
                        lcd.print('s');

                        lcd.setCursor(0, 2);
                        lcd.print("End: ");
                        print2digits(currentChannel.data.dayMode.endHour, lcd, 5, 2);
                        lcd.print(':');
                        print2digits(currentChannel.data.dayMode.endMinute, lcd, 8, 2);
                        lcd.print('.');
                        print2digits(currentChannel.data.dayMode.endSecond, lcd, 11, 2);
                        lcd.print('s');

                        lcd.setCursor(0, 3);
                        lcd.print("Left: 00:00.00s");
                        break;
                    
                    case SENSOR:
                        lcd.setCursor(0, 0);
                        lcd.print("<Sensor>");

                        lcd.setCursor(0, 1);
                        lcd.print("Threshold: ");
                        lcd.print(currentChannel.data.sensorMode.threshold);

                        lcd.setCursor(0, 2);
                        lcd.print("Work: ");
                        print2digits(currentChannel.data.sensorMode.workMinute, lcd, 6, 2);
                        lcd.print('m');
                        print2digits(currentChannel.data.sensorMode.workSecond, lcd, 10, 2);
                        lcd.print('s');

                        lcd.setCursor(0, 3);
                        lcd.print("Pin: A");
                        
                        switch (currentChannel.data.sensorMode.pin) {
                            case 16:
                                lcd.print('2');
                                break;
                            case 17:
                                lcd.print('3');
                                break;
                            case 20:
                                lcd.print('6');
                                break;
                            case 21:
                                lcd.print('7');
                                break;
                            default:
                                lcd.print('0');
                                break;
                        }
                        

                        lcd.setCursor(9, 3);
                        lcd.print("Val: ");
                        lcd.print(analogRead(currentChannel.data.sensorMode.pin));
                       break;
                    
                    case WEEK:
                        lcd.setCursor(0, 0);
                        lcd.print("<WEEK>");

                        lcd.setCursor(0, 1);
                        //lcd.print("Day: Monday       On");
                        switch (_dayIndex) {
                            case 0:
                                lcd.print("Day: Monday");
                                break;
                            case 1:
                                lcd.print("Day: Tuesday");
                                break;
                            case 2:
                                lcd.print("Day: Wednesday");
                                break;
                            case 3:
                                lcd.print("Day: Thursdsay");
                                break;
                            case 4:
                                lcd.print("Day: Friday");
                                break;
                            case 5:
                                lcd.print("Day: Saturday");
                                break;
                            case 6:
                                lcd.print("Day: Sunday");
                                break;
                        }

                        if (currentChannel.data.weekMode.days[_dayIndex].enabled) {
                        lcd.setCursor(18, 1);
                        lcd.print("On");

                        lcd.setCursor(0, 2);
                        lcd.print("Start: ");
                        //_dayIndex = 0;
                        print2digits(currentChannel.data.weekMode.days[_dayIndex].startHour, lcd, 7, 2);
                        lcd.print(':');
                        print2digits(currentChannel.data.weekMode.days[_dayIndex].startMinute, lcd, 10, 2);
                        lcd.print('.');
                        print2digits(currentChannel.data.weekMode.days[_dayIndex].startSecond, lcd, 13, 2);
                        lcd.print('s');

                        lcd.setCursor(0, 3);
                        lcd.print("End: ");
                        print2digits(currentChannel.data.weekMode.days[_dayIndex].endHour, lcd, 5, 3);
                        lcd.print(':');
                        print2digits(currentChannel.data.weekMode.days[_dayIndex].endMinute, lcd, 8, 3);
                        lcd.print('.');
                        print2digits(currentChannel.data.weekMode.days[_dayIndex].endSecond, lcd, 11, 3);
                        lcd.print('s');
                        } else {
                            lcd.setCursor(17, 1);
                            lcd.print("Off");
                        }

                        break;
                }
            lcd.setCursor(15, 0);
            lcd.print(">Back");
            break;

        }
        _first = 0;
    }
}

void ArrowControl::updateDisplay(LiquidCrystal_I2C& lcd, FSM& state) { // updatefunc

    switch (state) {
        case MAIN_MENU:
         
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
                    break; }
           

            _changedFlag = 0; }

            break;

        case CHANNELS:
            if (_changedFlag) {
                // deleting old arrow

                switch (_indexFlag) { // Switch to detect last arrow pos and clearing it
                    case 0:
                        lcd.setCursor(0, 0);
                        lcd.print("          ");
                        lcd.setCursor(0, 0);
                        lcd.print("Channel ");
                        lcd.print(_count);
                        break;
                    case 1:
                        if (currentChannel.mode != OFF) { // On/Off selector
                            lcd.setCursor(16, 0);
                            lcd.print("   ");
                            lcd.setCursor(18, 0);
                            lcd.print("On");
                        } else {
                            lcd.setCursor(16, 0);
                            lcd.print("    ");
                            lcd.setCursor(17, 0);
                            lcd.print("Off"); }
                        

                        break;

                    case 2: // Mode selector
                        switch (currentChannel.mode) {
                            case TIMER:
                                lcd.setCursor(0, 1);
                                lcd.print("                    ");
                                lcd.setCursor(0, 1);
                                lcd.print("Mode: <Timer>");
                                break;
                                    
                            case PID:
                                lcd.setCursor(0, 1);
                                lcd.print("                    ");
                                lcd.setCursor(0, 1);
                                lcd.print("Mode: <PID>");
                                break;
                                   
                            case DAY:
                                lcd.setCursor(0, 1);
                                lcd.print("                    ");
                                lcd.setCursor(0, 1);
                                lcd.print("Mode: <Day>");
                                break;

                            case SENSOR:
                                lcd.setCursor(0, 1);
                                lcd.print("                    ");
                                lcd.setCursor(0, 1);
                                lcd.print("Mode: <Sensor>");
                                break;
                            
                            case WEEK:
                                lcd.setCursor(0, 1);
                                lcd.print("                    ");
                                lcd.setCursor(0, 1);
                                lcd.print("Mode: <Week>");
                                break;
                            }
                        break;

                    case 3:
                        lcd.setCursor(0, 2);
                        lcd.print("                   ");
                        lcd.setCursor(0, 2);
                        if (currentChannel.relayMode == TO_ON) {
                            lcd.print("Direction: Off-On");
                        }
                        else {
                            lcd.print("Direction: On-Off");
                        }
                        break;

                    case 4: // Back
                        lcd.setCursor(15, 3);
                        lcd.print("     ");
                        lcd.setCursor(16, 3);
                        lcd.print("Back");
                        break;
                    
                    #if LOG
                    Serial.println(F("Removing old arrow"));
                    #endif
                        }
                
                /* Inserting new arrow in new position */

                switch (_index) {
                    case 0:
                        lcd.setCursor(0, 0);
                        lcd.print("         ");
                        lcd.setCursor(0, 0);
                        lcd.print(">Channel ");
                        lcd.print(_count);
                        break;
                    case 1:
                        if (currentChannel.mode != OFF) { // On/Off selector
                            lcd.setCursor(17, 0);
                            lcd.print('>');
                        } else {
                            lcd.setCursor(16, 0);
                            lcd.print('>');
                        }

                        break;

                    case 2: // Mode selector
                        switch (currentChannel.mode) {
                            case TIMER:
                                lcd.setCursor(0, 1);
                                lcd.print("             ");
                                lcd.setCursor(0, 1);
                                lcd.print(">Mode: <Timer>");
                                break;
                                    
                            case PID:
                                lcd.setCursor(0, 1);
                                lcd.print("          ");
                                lcd.setCursor(0, 1);
                                lcd.print(">Mode: <PID>");
                                break;
                                   
                            case DAY:
                                lcd.setCursor(0, 1);
                                lcd.print("              ");
                                lcd.setCursor(0, 1);
                                lcd.print(">Mode: <Day>");
                                break;
                            
                            case SENSOR:
                                lcd.setCursor(0, 1);
                                lcd.print("              ");
                                lcd.setCursor(0, 1);
                                lcd.print(">Mode: <Sensor>");
                                break;

                            case WEEK:
                                lcd.setCursor(0, 1);                            
                                lcd.print("              ");
                                lcd.setCursor(0, 1);                            
                                lcd.print(">Mode: <Week>");
                                break;

                            }
                        break;

                    case 3:
                        lcd.setCursor(0, 2);
                        lcd.print("                   ");
                        lcd.setCursor(0, 2);
                        if (currentChannel.relayMode == TO_ON) {
                            lcd.print("Direction:>Off-On");
                        }
                        else {
                            lcd.print("Direction:>On-Off");
                        }
                        break;


                    case 4: // Back
                        lcd.setCursor(15, 3);
                        lcd.print('>');
                        break;
                    
                    #if LOG
                    Serial.println(F("Drawing new arrow"));
                    #endif
                // ====================================== ARROW TRACKING END ===========================
                }
                _changedFlag = 0;
            }
            break;
            
        // ==================================== MODES =======================================
        
        case MODES:
            if (millis() - _tmr > 300 && (currentChannel.mode == SENSOR || currentChannel.mode == PID)) {
                if (currentChannel.mode == SENSOR) {
                lcd.setCursor(14, 3);
                lcd.print("      ");
                lcd.setCursor(14, 3);
                } else if (currentChannel.mode == PID) {
                lcd.setCursor(16, 3);
                lcd.print("    ");
                lcd.setCursor(16, 3);
                }

                lcd.print(analogRead(currentChannel.data.sensorMode.pin));
                _tmr = millis();
            }

            if (_changedFlag) {
                switch (currentChannel.mode) {
                    case DAY:

                        switch (_indexFlag) {
                            case 0:
                                lcd.setCursor(15, 0);
                                lcd.print(' ');
                                break;
                            case 1:
                                lcd.setCursor(6, 1);
                                lcd.print(' ');
                                break;
                            case 2:
                                lcd.setCursor(9, 1);
                                lcd.print(':');
                                break;
                            case 3:
                                lcd.setCursor(12, 1);
                                lcd.print('.');
                                break;
                            case 4:
                                lcd.setCursor(4, 2);
                                lcd.print(' ');
                                break;
                            case 5:
                                lcd.setCursor(7, 2);
                                lcd.print(':');
                                break;
                            case 6:
                                lcd.setCursor(10, 2);
                                lcd.print('.');
                                break;
                        } // clearing after arrow shift

                        switch (_index) {
                            case 0:
                                lcd.setCursor(15, 0);
                                break;
                            case 1:
                                lcd.setCursor(6, 1);
                                break;
                            case 2:
                                lcd.setCursor(9, 1);
                                break;
                            case 3:
                                lcd.setCursor(12, 1);
                                break;
                            case 4:
                                lcd.setCursor(4, 2);
                                break;
                            case 5:
                                lcd.setCursor(7, 2);
                                break;
                            case 6:
                                lcd.setCursor(10, 2);
                                break; }
                        lcd.print('>');
                        break;

                    case TIMER:
                        switch (_indexFlag) { // deleting old arrow
                            case 0:
                                lcd.setCursor(15, 0);
                                break; 
                            case 1:
                                lcd.setCursor(7, 1);
                                break;
                            case 2:
                                lcd.setCursor(11, 1);
                                break;
                            case 3:
                                lcd.setCursor(15, 1);
                                break;
                            case 4:
                                lcd.setCursor(5, 2);
                                break;
                            case 5:
                                lcd.setCursor(9, 2);
                                break;
                        }
                        lcd.print(' ');

                        switch (_index) { // printing new one
                            case 0:
                                lcd.setCursor(15, 0);
                                break;
                            case 1:
                                lcd.setCursor(7, 1);
                                break;
                            case 2:
                                lcd.setCursor(11, 1);
                                break;
                            case 3:
                                lcd.setCursor(15, 1);
                                break;
                            case 4:
                                lcd.setCursor(5, 2);
                                break;
                            case 5:
                                lcd.setCursor(9, 2);
                                break;
                        }
                        lcd.print('>');

                        break;
                    
                    case SENSOR:
                        switch (_indexFlag) { // deleting old arrow
                            case 0:
                                lcd.setCursor(15, 0);
                                break;
                            case 1:
                                lcd.setCursor(10, 1);
                                break;
                            case 2:
                                lcd.setCursor(5, 2);
                                break;
                            case 3:
                                lcd.setCursor(9, 2);
                                break;
                            case 4:
                                lcd.setCursor(4, 3);
                                break;
                        }
                        lcd.print(' ');

                        switch (_index) { // printing new one
                            case 0:
                                lcd.setCursor(15, 0);
                                break;
                            case 1:
                                lcd.setCursor(10, 1);
                                break;
                            case 2:
                                lcd.setCursor(5, 2);
                                break;
                            case 3:
                                lcd.setCursor(9, 2);
                                break;
                            case 4:
                                lcd.setCursor(4, 3);
                                break;
                        }
                        lcd.print('>');


                        break;

                    case WEEK:

                        if (_oneByte & (1 << 7)) {
                            if (currentChannel.data.weekMode.days[_dayIndex].enabled) { // if its on
                                lcd.setCursor(16, 1);
                                lcd.print("    "); // clearing

                                lcd.setCursor(17, 1);
                                lcd.print(">On");

                                if (currentChannel.data.weekMode.days[_dayIndex].startHour > 23) currentChannel.data.weekMode.days[_dayIndex].startHour = 0;
                                if (currentChannel.data.weekMode.days[_dayIndex].startMinute > 59) currentChannel.data.weekMode.days[_dayIndex].startMinute = 0;
                                if (currentChannel.data.weekMode.days[_dayIndex].startSecond > 23) currentChannel.data.weekMode.days[_dayIndex].startSecond = 0;

                                if (currentChannel.data.weekMode.days[_dayIndex].endHour > 23) currentChannel.data.weekMode.days[_dayIndex].endHour = 0;
                                if (currentChannel.data.weekMode.days[_dayIndex].endMinute > 59) currentChannel.data.weekMode.days[_dayIndex].endMinute = 0;
                                if (currentChannel.data.weekMode.days[_dayIndex].endSecond > 23) currentChannel.data.weekMode.days[_dayIndex].endSecond = 0;

                                lcd.setCursor(0, 2);
                                lcd.print("Start: ");
                                print2digits(currentChannel.data.weekMode.days[_dayIndex].startHour, lcd, 7, 2);
                                lcd.print(':');
                                print2digits(currentChannel.data.weekMode.days[_dayIndex].startMinute, lcd, 10, 2);
                                lcd.print('.');
                                print2digits(currentChannel.data.weekMode.days[_dayIndex].startSecond, lcd, 13, 2);
                                lcd.print('s');

                                lcd.setCursor(0, 3);
                                lcd.print("End: ");
                                print2digits(currentChannel.data.weekMode.days[_dayIndex].endHour, lcd, 5, 3);
                                lcd.print(':');
                                print2digits(currentChannel.data.weekMode.days[_dayIndex].endMinute, lcd, 8, 3);
                                lcd.print('.');
                                print2digits(currentChannel.data.weekMode.days[_dayIndex].endSecond, lcd, 11, 3);
                                lcd.print('s');


                            } else {
                                lcd.setCursor(16, 1);
                                lcd.print("    "); // clearing

                                lcd.setCursor(16, 1);
                                lcd.print(">Off");

                                lcd.setCursor(0, 2);
                                lcd.print("                   ");
                                lcd.setCursor(0, 3);
                                lcd.print("                   ");

                            }

                            _oneByte &= 127;
                            _changedFlag = 0;
                        }


                        switch (_indexFlag) {
                            case 0:
                                lcd.setCursor(15, 0);
                                lcd.print(' ');
                                break;
                            case 1:
                                lcd.setCursor(4, 1);
                                lcd.print(' ');
                                break;
                            case 2:
                                if (currentChannel.data.weekMode.days[_dayIndex].enabled) lcd.setCursor(17, 1);
                                else lcd.setCursor(16, 1);

                                lcd.print(' ');
                                break;
                            case 3:
                                lcd.setCursor(6, 2);
                                lcd.print(' ');
                                break;
                            case 4:
                                lcd.setCursor(9, 2);
                                lcd.print(':');
                                break;
                            case 5:
                                lcd.setCursor(12, 2);
                                lcd.print('.');
                                break;
                            case 6:
                                lcd.setCursor(4, 3);
                                lcd.print(' ');
                                break;
                            case 7:
                                lcd.setCursor(7, 3);
                                lcd.print(':');
                                break;
                            case 8:
                                lcd.setCursor(10, 3);
                                lcd.print('.');
                                break;
                        }

                        switch (_index) {
                            case 0:
                                lcd.setCursor(14, 0);
                                lcd.print(' ');
                                break;
                            case 1:
                                lcd.setCursor(4, 1);
                                break;
                            case 2:
                                if (currentChannel.data.weekMode.days[_dayIndex].enabled) lcd.setCursor(17, 1);
                                else lcd.setCursor(16, 1);

                                break;
                            case 3:
                                lcd.setCursor(6, 2);
                                break;
                            case 4:
                                lcd.setCursor(9, 2);
                                break;
                            case 5:
                                lcd.setCursor(12, 2);
                                break;
                            case 6:
                                lcd.setCursor(4, 3);
                                break;
                            case 7:
                                lcd.setCursor(7, 3);
                                break;
                            case 8:
                                lcd.setCursor(10, 3);
                                break;
                        }


                        lcd.print('>');

                        if (_dayIndex != _dayIndexFlag) {
                            // cleaning
                            lcd.setCursor(0, 1);
                            lcd.print("                   ");
                            lcd.setCursor(0, 2);
                            lcd.print("                   ");
                            lcd.setCursor(0, 3);
                            lcd.print("                   ");
                            
                            lcd.setCursor(0, 1);
                            switch (_dayIndex) {
                                case 0: // monday
                                    lcd.print("Day:>Monday");
                                    break;
                                case 1: // tuesday 
                                    lcd.print("Day:>Tuesday");
                                    break;
                                case 2: // Wednesday
                                    lcd.print("Day:>Wednesday");
                                    break;
                                case 3: // Thursday
                                    lcd.print("Day:>Thursday");
                                    break;
                                case 4: // Friday
                                    lcd.print("Day:>Friday");
                                    break;
                                case 5: // Satruday
                                    lcd.print("Day:>Satruday");
                                    break;
                                case 6: // Sunday
                                    lcd.print("Day:>Sunday");
                                    break;
                            }

                            if (currentChannel.data.weekMode.days[_dayIndex].enabled) {
                                lcd.setCursor(18, 1);
                                lcd.print("On");

                                lcd.setCursor(0, 2);
                                lcd.print("Start: ");
                                print2digits(currentChannel.data.weekMode.days[_dayIndex].startHour, lcd, 7, 2);
                                lcd.print(':');
                                print2digits(currentChannel.data.weekMode.days[_dayIndex].startMinute, lcd, 10, 2);
                                lcd.print('.');
                                print2digits(currentChannel.data.weekMode.days[_dayIndex].startSecond, lcd, 13, 2);
                                lcd.print('s');

                                lcd.setCursor(0, 3);
                                lcd.print("End: ");
                                print2digits(currentChannel.data.weekMode.days[_dayIndex].endHour, lcd, 5, 3);
                                lcd.print(':');
                                print2digits(currentChannel.data.weekMode.days[_dayIndex].endMinute, lcd, 8, 3);
                                lcd.print('.');
                                print2digits(currentChannel.data.weekMode.days[_dayIndex].endSecond, lcd, 11, 3);
                                lcd.print('s');

                            } else {
                                lcd.setCursor(17, 1);
                                lcd.print("Off");
                            }

                            _dayIndexFlag = _dayIndex;
                        }

                        break;
                        
                    
                    case PID:
                        switch (_indexFlag) {
                            case 0:
                                lcd.setCursor(15, 0);
                                lcd.print(' ');
                                break;
                            case 1:
                                lcd.setCursor(3, 1);
                                lcd.print(' ');
                                break;
                            case 2:
                                lcd.setCursor(6, 1);
                                lcd.print('.');
                                break;
                            case 3:
                                lcd.setCursor(3, 2);
                                lcd.print(' ');
                                break;
                            case 4:
                                lcd.setCursor(6, 2);
                                lcd.print('.');
                                break;
                            case 5:
                                lcd.setCursor(3, 3);
                                lcd.print(' ');
                                break;
                            case 6:
                                lcd.setCursor(6, 3);
                                lcd.print('.');
                                break;
                            case 7:
                                lcd.setCursor(15, 1);
                                lcd.print(' ');
                                break;
                            case 8:
                                lcd.setCursor(15, 2);
                                lcd.print(' ');
                                break;
                        }

                        switch (_index) {
                            case 0:
                                lcd.setCursor(15, 0);
                                break;
                            case 1:
                                lcd.setCursor(3, 1);
                                break;
                            case 2:
                                lcd.setCursor(6, 1);
                                break;
                            case 3:
                                lcd.setCursor(3, 2);
                                break;
                            case 4:
                                lcd.setCursor(6, 2);
                                break;
                            case 5:
                                lcd.setCursor(3, 3);
                                break;
                            case 6:
                                lcd.setCursor(6, 3);
                                break;
                            case 7:
                                lcd.setCursor(15, 1);
                                break;
                            case 8:
                                lcd.setCursor(15, 2);
                                break;
                        }
                        lcd.print('>');

                        break;
                        
                    }



            }
	_changedFlag = 0;

	if (_settingsChanged) {
		switch (currentChannel.mode) {

		case TIMER:
            if (_oneByte & (1 << 0)) {
                lcd.setCursor(8, 1);
                lcd.print("  ");

                print2digits(currentChannel.data.timerMode.periodHour, lcd, 8, 1);
                _oneByte &= 254;
            }

            if (_oneByte & 1 << 1) {
                lcd.setCursor(12, 1);
                lcd.print("  ");

                print2digits(currentChannel.data.timerMode.periodMinute, lcd, 12, 1);
                _oneByte &= 253;
            }

            if (_oneByte & 1 << 2) {
                lcd.setCursor(16, 1);
                lcd.print("  ");

                print2digits(currentChannel.data.timerMode.periodSecond, lcd, 16, 1);
                _oneByte &= 251;
            }

            if (_oneByte & (1 << 3)) {
                lcd.setCursor(6, 2);
                lcd.print("  ");
                
                print2digits(currentChannel.data.timerMode.workMinute, lcd, 6, 2);
                _oneByte &= 247;
            }

            if (_oneByte & (1 << 4)) {
                lcd.setCursor(10, 2);
                lcd.print("  ");

                print2digits(currentChannel.data.timerMode.workSecond, lcd, 10, 2);
                _oneByte &= 239;
            }
			break;

		case DAY:

            if (_oneByte & (1 << 0))  {
			    lcd.setCursor(7, 1);
			    lcd.print("  ");

                print2digits(currentChannel.data.dayMode.startHour, lcd, 7, 1);
                _oneByte &= 254;
            }

            if (_oneByte & (1 << 1)) {
			    lcd.setCursor(10, 1);
			    lcd.print("  ");

			    print2digits(currentChannel.data.dayMode.startMinute, lcd, 10, 1);
                _oneByte &= 253;
            }

            if (_oneByte & (1 << 2)) {
			    lcd.setCursor(13, 1);
			    lcd.print("  ");

			    print2digits(currentChannel.data.dayMode.startSecond, lcd, 13, 1);
                _oneByte &= 251;
            }

            if (_oneByte & (1 << 3)) {
			    lcd.setCursor(5, 2);
			    lcd.print("  ");

			    print2digits(currentChannel.data.dayMode.endHour, lcd, 5, 2);
                _oneByte &= 247;
            }

            if (_oneByte & (1 << 4)) {
			    lcd.setCursor(8, 2);
			    lcd.print("  ");

			    print2digits(currentChannel.data.dayMode.endMinute, lcd, 8, 2);
                _oneByte &= 239;
            }

            if (_oneByte & (1 << 5)) {
			    lcd.setCursor(11, 2);
			    lcd.print("  ");

			    print2digits(currentChannel.data.dayMode.endSecond, lcd, 11, 2);
                _oneByte &= 223;
            }

			break;

		case SENSOR:
            if (_oneByte & (1 << 0)) {
                lcd.setCursor(11, 1);
                lcd.print("         ");

                lcd.setCursor(11, 1);
                lcd.print(currentChannel.data.sensorMode.threshold);
            
                _oneByte &= 254;
            }

            if (_oneByte & (1 << 1)) {
                lcd.setCursor(6, 2);
                lcd.print("  ");

                print2digits(currentChannel.data.sensorMode.workMinute, lcd, 6, 2);
                _oneByte &= 253;
            }

            if (_oneByte & (1 << 2)) {
                lcd.setCursor(10, 2);
                lcd.print("  ");

                print2digits(currentChannel.data.sensorMode.workSecond, lcd, 10, 2);
                _oneByte &= 251;
            }

            if (_oneByte & (1 << 3)) {
                lcd.setCursor(6, 3);
                lcd.print(' ');
                lcd.setCursor(6, 3);
                
                switch (currentChannel.data.sensorMode.pin) {
                    case 14:
                        lcd.print('0');
                        break;
                    case 15:
                        lcd.print('1');
                        break;
                    case 16:
                        lcd.print('2');
                        break;
                    case 17:
                        lcd.print('3');
                        break;
                    case 20:
                        lcd.print('6');
                        break;
                    case 21:
                        lcd.print('7');
                        break;
                }
                lcd.setCursor(14, 3);
                lcd.print("      ");
                lcd.setCursor(14, 3);
                lcd.print(analogRead(currentChannel.data.sensorMode.pin));
                _oneByte &= 247;
            }

			break;

        case WEEK:
            if (_oneByte & (1 << 0))  {
			    lcd.setCursor(7, 2);
			    lcd.print("  ");

                print2digits(currentChannel.data.weekMode.days[_dayIndex].startHour, lcd, 7, 2);
                _oneByte &= 254;
            }

            if (_oneByte & (1 << 1)) {
			    lcd.setCursor(10, 2);
			    lcd.print("  ");

			    print2digits(currentChannel.data.weekMode.days[_dayIndex].startMinute, lcd, 10, 2);
                _oneByte &= 253;
            }

            if (_oneByte & (1 << 2)) {
			    lcd.setCursor(13, 2);
			    lcd.print("  ");

			    print2digits(currentChannel.data.weekMode.days[_dayIndex].startSecond, lcd, 13, 2);
                _oneByte &= 251;
            }

            if (_oneByte & (1 << 3)) {
			    lcd.setCursor(5, 3);
			    lcd.print("  ");

			    print2digits(currentChannel.data.weekMode.days[_dayIndex].endHour, lcd, 5, 3);
                _oneByte &= 247;
            }

            if (_oneByte & (1 << 4)) {
			    lcd.setCursor(8, 3);
			    lcd.print("  ");

			    print2digits(currentChannel.data.weekMode.days[_dayIndex].endMinute, lcd, 8, 3);
                _oneByte &= 239;
            }

            if (_oneByte & (1 << 5)) {
			    lcd.setCursor(11, 3);
			    lcd.print("  ");

			    print2digits(currentChannel.data.weekMode.days[_dayIndex].endSecond, lcd, 11, 3);
                _oneByte &= 223;
            }

            break;

		case PID:
            if (_oneByte & (1 << 0) || _oneByte & (1 << 1)) {
                lcd.setCursor(4, 1);
                lcd.print("     ");
                //lcd.setCursor(4, 1);
                printFloat(currentChannel.data.PidMode.Kp, lcd, 4, 1);

                _oneByte &= ~((1 << 0) | (1 << 1));
            }

            if (_oneByte & (1 << 2) || _oneByte & (1 << 3)) {
                lcd.setCursor(4, 2);
                lcd.print("     ");
                //lcd.setCursor(4, 2);
                printFloat(currentChannel.data.PidMode.Ki, lcd, 4, 2);

                _oneByte &= ~((1 << 2) | (1 << 3));
            }

            if (_oneByte & (1 << 4) || _oneByte & (1 << 5)) {
                lcd.setCursor(4, 3);
                lcd.print("     ");
                //lcd.setCursor(4, 3);
                printFloat(currentChannel.data.PidMode.Kd, lcd, 4, 3);

                _oneByte &= ~((1 << 4) | (1 << 5));
            }
            
            if (_oneByte & (1 << 6)) {
                lcd.setCursor(16, 1);
                lcd.print("    ");
                lcd.setCursor(16, 1);
                lcd.print(currentChannel.data.PidMode.setPoint);

                _oneByte &= ~(1 << 6);
            }

            if (_oneByte & (1 << 7)) {
                lcd.setCursor(17, 2);
                lcd.print(' ');
                lcd.setCursor(17, 2);
                
                switch (currentChannel.data.PidMode.pin) {
                    case 14:
                        lcd.print('0');
                        break;
                    case 15:
                        lcd.print('1');
                        break;
                    case 16:
                        lcd.print('2');
                        break;
                    case 17:
                        lcd.print('3');
                        break;
                    case 20:
                        lcd.print('6');
                        break;
                    case 21:
                        lcd.print('7');
                        break;
                    default:
                        lcd.print('0');
                        break;
                }
                lcd.setCursor(16, 3);
                lcd.print("    ");
                lcd.setCursor(16, 3);
                lcd.print(analogRead(currentChannel.data.PidMode.pin));
                _oneByte &= ~(1 << 7);                    
                
            }


			break;
		
		}		
	_settingsChanged = 0;
	}
       break;
    }
}