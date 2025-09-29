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

        int8_t _count;
        int8_t _index; // arrow index
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
        uint8_t _oneByte {0};
        Mode _lastMode;

        void redrawDisplay(LiquidCrystal_I2C& lcd, FSM& state);
        void updateDisplay(LiquidCrystal_I2C& lcd, FSM& state);
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

            if (currentChannel.mode != OFF) {
                if (_index < 0) { _index = 0; _changedFlag = 0; }
                else if (_index > 4) {_index = 4; _changedFlag = 0; } // constraining
            } else {
                if (_index < 0) { _index = 0; _changedFlag = 0; } // if mode is OFF
                else if (_index == 2 && _indexFlag != 3) {_index = 3; }
                else if (_index == 2) {_index = 1; }
                else if (_index > 3) {_index = 3; _changedFlag = 0; }
            }

            if (_changedFlag) updateDisplay(lcd, state);    

            if (_index == 3 && enc.isClick()) {
                #if LOG
                Serial.println(F("state->MAIN_MENU"));
                #endif
                lcd.clear();
                state = MAIN_MENU; // MAIN_MENU
                _first = 1;
                _inChannelFlag = 0;
                _index = 0;


                if (currentChannel.mode == OFF) digitalWrite(channelsPins[_count - 1], LOW);
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
                
                state = MODES;
                _first = 1;
                _inChannelFlag = 0;
                _index = 0;
                if (_modesFlag && _lastMode != currentChannel.mode) {
                    switch (currentChannel.mode) {
                        case TIMER:
                            currentChannel.data.timerMode.periodHour = 0;
                            currentChannel.data.timerMode.periodMinute = 0;
                            currentChannel.data.timerMode.periodSecond = 0;

                            currentChannel.data.timerMode.workMinute = 0;
                            currentChannel.data.timerMode.workSecond = 0;
                            break;

                        case SENSOR:
                            currentChannel.data.sensorMode.threshold = 1023;
                            currentChannel.data.sensorMode.workMinute = 0;
                            currentChannel.data.sensorMode.workSecond = 0;
                            currentChannel.data.sensorMode.pin = 14; // A0 default
                            break;

                        case DAY:
                            currentChannel.data.dayMode.startHour = 0;
                            currentChannel.data.dayMode.startMinute = 0;
                            currentChannel.data.dayMode.startSecond = 0;
                            currentChannel.data.dayMode.endHour = 0;
                            currentChannel.data.dayMode.endMinute = 0;
                            currentChannel.data.dayMode.endSecond = 0;
                            break;
                        case WEEK:
                            currentChannel.data.weekMode.days[0] = {0, 0, 0, 0, 0, 0, 1}; // by default at least 1 day is enabled
                            currentChannel.data.weekMode.days[1] = {0, 0, 0, 0, 0, 0, 0};
                            currentChannel.data.weekMode.days[2] = {0, 0, 0, 0, 0, 0, 0};
                            currentChannel.data.weekMode.days[3] = {0, 0, 0, 0, 0, 0, 0};
                            currentChannel.data.weekMode.days[4] = {0, 0, 0, 0, 0, 0, 0};

                            currentChannel.data.weekMode.days[5] = {0, 0, 0, 0, 0, 0, 0};
                            currentChannel.data.weekMode.days[6] = {0, 0, 0, 0, 0, 0, 0};
                            break;

                    }
                }
            }
                
            /* Detecting enc actions */
            if (enc.isRightH()) {
                switch (_index) {
                    case 0: // 
                        /* Saving Channel settings*/

                        if (currentChannel.mode == OFF) digitalWrite(channelsPins[_count - 1], LOW);

                        if (_channelFlag) {
                            putChannel(_count, currentChannel);
                            _channelFlag = 0;
                        }

                        ++_count; // saving settings of current Channel and moving to the next
                        _first = 1; 
                        _inChannelFlag = 0; 

                        lcd.setCursor(0, 1);
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
                            
                            case RTC:
                                lcd.setCursor(0, 1);
                                lcd.print("Mode: <RTC>");
                                break;
                            
                            case DAY:
                                lcd.setCursor(0, 1);
                                lcd.print("Mode: <Day>");
                                break;
                            case SENSOR:
                                lcd.setCursor(0, 1);
                                lcd.print("Mode: <Sensor>");
                        }
                        
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
                            
                            case RTC:
                                lcd.setCursor(0, 1);
                                lcd.print("Mode: <RTC>");
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


                    }
                }

            if (enc.isLeftH()) {
                switch (_index) {
                    case 0:
                        /* Saving channel settings in EEPROM */
                        if (currentChannel.mode == OFF) digitalWrite(channelsPins[_count - 1], LOW);

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
                                lcd.print("Mode: <Timer>");
                                break;
                            
                            case RTC:
                                lcd.setCursor(0, 1);
                                lcd.print("Mode: <RTC>");
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
                                Serial.println(F("Unexptected Mode"));
                                #endif
                                break;
                        }

                        _channelFlag = 1;
                        
                        break;



                }
            }
        } 

        if (enc.isRightH() && !_inChannelFlag) {++_count; _first = 1; _changedFlag = 1; }
        if (enc.isLeftH() && !_inChannelFlag) {--_count; _first = 1; _changedFlag = 1; }

        if (enc.isClick()) { 
            _inChannelFlag = 1;
            #if LOG
            Serial.println(F("!In Channel"));
            #endif
        }

        updateDisplay(lcd, state);

        }


void ArrowControl::modesTick(encMinim& enc, LiquidCrystal_I2C& lcd, FSM& state) { // modesfunc
    redrawDisplay(lcd, state);
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


    if (_index < 0 && _indexFlag == 0) {_changedFlag = 0; _index = 0; }
   switch (currentChannel.mode) {
        case TIMER:
            if (_index > 5 && _indexFlag == 5) {_changedFlag = 0; _index = 5; }
            break;
        case DAY:
            if (_index > 6 && _indexFlag == 6) {_changedFlag = 0; _index = 6; }
            break;
        case RTC:
            _changedFlag = 0;
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


    if (_index != 0 && enc.isRightH()) {
        switch (currentChannel.mode) {
            case TIMER:
                switch (_index) {
                    case 1: // period hours
                        if (currentChannel.data.timerMode.periodHour < 99) {
                            if (enc.isFast() && currentChannel.data.timerMode.periodHour + 3 < 99) currentChannel.data.timerMode.periodHour += 3;
                            else ++currentChannel.data.timerMode.periodHour;
                            _oneByte |= 1 << 0; // 0th bit for periodHour
                        } else currentChannel.data.timerMode.periodHour = 99;
                        break;
                    case 2:
                        if (currentChannel.data.timerMode.periodMinute < 59) {
                            if (enc.isFast() && currentChannel.data.timerMode.periodMinute + 5 < 59) currentChannel.data.timerMode.periodMinute += 5;
                            else ++currentChannel.data.timerMode.periodMinute;
                            _oneByte |= 1 << 1; // 1th bit for periodMinute
                        } else currentChannel.data.timerMode.periodMinute = 59;
                        break;
                    case 3:
                        if (currentChannel.data.timerMode.periodSecond < 59) {
                            if (enc.isFast() && currentChannel.data.timerMode.periodSecond + 5 < 59) currentChannel.data.timerMode.periodSecond += 5;
                            else ++currentChannel.data.timerMode.periodSecond;
                            _oneByte |= 1 << 2; // 2th bit for periodSecond
                        } else currentChannel.data.timerMode.periodSecond = 59;
                        break;
                    
                    case 4:
                        if (currentChannel.data.timerMode.workMinute < 59) {
                            if (enc.isFast() && currentChannel.data.timerMode.workMinute + 5 < 59) currentChannel.data.timerMode.workMinute += 5;
                            else ++currentChannel.data.timerMode.workMinute;
                            _oneByte |= 1 << 3; // 3th bit for workMinute
                        } else currentChannel.data.timerMode.workMinute = 59;
                        break;
                    case 5:
                        if (currentChannel.data.timerMode.workSecond < 59) {
                            if (enc.isFast() && currentChannel.data.timerMode.workSecond + 5 < 59) currentChannel.data.timerMode.workSecond += 5;
                            else ++currentChannel.data.timerMode.workSecond;
                            _oneByte |= 1 << 4; // 4th bit for workSecond
                        } else currentChannel.data.timerMode.workSecond = 59;
                        break;
                }

                _settingsChanged = 1;    
                
                break;
            
            case DAY: // _oneByte as flags for changed sets 
                switch (_index) {
                    case 1:
                        if (currentChannel.data.dayMode.startHour < 23) {
                        if (enc.isFast() && currentChannel.data.dayMode.startHour + 3 < 23) currentChannel.data.dayMode.startHour += 3;
                        else ++currentChannel.data.dayMode.startHour;
                        _oneByte |= 1 << 0 ;} // 0b01 
                        //0th bit for startHour

                        else currentChannel.data.dayMode.startHour = 23; // 0th hour
                        break;
                    case 2:
                        if (currentChannel.data.dayMode.startMinute >= 59 && currentChannel.data.dayMode.startHour < 23) {
                        currentChannel.data.dayMode.startMinute = 0;
                        ++currentChannel.data.dayMode.startHour; 
                        _oneByte |= (1 << 0) | (1 << 1) ;} // 1th bit for startMinute
                        

                        else if (currentChannel.data.dayMode.startMinute < 59) {
                        
                        if (enc.isFast() && currentChannel.data.dayMode.startMinute + 5 < 59) currentChannel.data.dayMode.startMinute += 5;
                        else ++currentChannel.data.dayMode.startMinute;
                        _oneByte |= 1 << 1;
                        } else currentChannel.data.dayMode.startMinute = 59;
                        
                        break;
                    case 3:
                        if (currentChannel.data.dayMode.startSecond >= 59 && currentChannel.data.dayMode.startHour < 23 && currentChannel.data.dayMode.startMinute < 59) {
                            currentChannel.data.dayMode.startSecond = 0;
                            ++currentChannel.data.dayMode.startMinute;
                            _oneByte |= (1 << 2) | (1 << 1); // 2th bit for startSecond
                        } 

                        else if (currentChannel.data.dayMode.startSecond < 59) {
                            if (enc.isFast() && currentChannel.data.dayMode.startSecond + 5 < 59) currentChannel.data.dayMode.startSecond += 5;
                            else ++currentChannel.data.dayMode.startSecond; 
                            _oneByte |= 1 << 2; 
                        }
                        else currentChannel.data.dayMode.startSecond = 59;

                        break;
                    case 4:
                        if (currentChannel.data.dayMode.endHour < 23) {
                            if (enc.isFast() && currentChannel.data.dayMode.endHour + 3 < 23) currentChannel.data.dayMode.endHour += 3;
                            else ++currentChannel.data.dayMode.endHour;
                            _oneByte |= 1 << 3; }// 3th bit for endHour 
                        
                        else currentChannel.data.dayMode.endHour = 23;
                        break;
                    case 5:
                        if (currentChannel.data.dayMode.endMinute >= 59 && currentChannel.data.dayMode.endHour != 23) {
                            currentChannel.data.dayMode.endMinute = 0;
                            ++currentChannel.data.dayMode.endHour; 
                            _oneByte |= (1 << 3) | (1 << 4); // 4th bit for endMinute
                        }

                        else if (currentChannel.data.dayMode.endMinute < 59) {
                            if (enc.isFast() && currentChannel.data.dayMode.endMinute + 5 < 59) currentChannel.data.dayMode.endMinute += 5;
                            else ++currentChannel.data.dayMode.endMinute; 
                            _oneByte |= 1 << 4; 
                        }
                        
                        break;
                    case 6:
                        if (currentChannel.data.dayMode.endSecond >= 59 && currentChannel.data.dayMode.endHour != 23 && currentChannel.data.dayMode.endMinute < 59) {
                                currentChannel.data.dayMode.endSecond = 0;
                                ++currentChannel.data.dayMode.endMinute;
                                _oneByte |= (1 << 4) | (1 << 5); // 5th bit for endSecond
                        }
                        else if (currentChannel.data.dayMode.endSecond < 59) {
                            if (enc.isFast() && currentChannel.data.dayMode.endSecond + 5 < 59) currentChannel.data.dayMode.endSecond += 5;
                            else ++currentChannel.data.dayMode.endSecond; 
                            _oneByte |= 1 << 5; 
                        }
                        else currentChannel.data.dayMode.endSecond = 0;

                        break;
                }

                _settingsChanged = 1;
                break;
            
            case SENSOR:
                switch (_index) {
                    case 1:
                        if (currentChannel.data.sensorMode.threshold < 1023) {
                            if (enc.isFast() && currentChannel.data.sensorMode.threshold + 50 < 1023) currentChannel.data.sensorMode.threshold += 50;
                            else ++currentChannel.data.sensorMode.threshold;
                            _oneByte |= 1 << 0;
                        } else currentChannel.data.sensorMode.threshold = 1023;
                        break;
                    case 2:
                        if (currentChannel.data.sensorMode.workMinute < 99) {
                            if (enc.isFast() && currentChannel.data.sensorMode.workMinute + 5 < 99) currentChannel.data.sensorMode.workMinute += 5;
                            else ++currentChannel.data.sensorMode.workMinute;
                            _oneByte |= 1 << 1;
                        } else currentChannel.data.sensorMode.workMinute = 99;
                        break;
                    case 3:
                        if (currentChannel.data.sensorMode.workSecond >= 59 && currentChannel.data.sensorMode.workMinute < 99) {
                            currentChannel.data.sensorMode.workSecond = 0;
                            ++currentChannel.data.sensorMode.workMinute;
                            _oneByte |= 1 << 2; // 2th bit forworkMinute
                            _oneByte |= 1 << 1;
                            break;
                        } 
                        if (currentChannel.data.sensorMode.workSecond < 59) {
                            if (enc.isFast() && currentChannel.data.sensorMode.workSecond + 5 < 59) currentChannel.data.sensorMode.workSecond += 5;
                            else ++currentChannel.data.sensorMode.workSecond;
                            _oneByte |= 1 << 2;
                        } else currentChannel.data.sensorMode.workSecond = 59;

                        break;
                    
                    case 4:
                        ++currentChannel.data.sensorMode.pin;
                        if (currentChannel.data.sensorMode.pin < 20 && currentChannel.data.sensorMode.pin > 17) {
                            currentChannel.data.sensorMode.pin = 20;
                        }

                        if (currentChannel.data.sensorMode.pin > 21) {
                            currentChannel.data.sensorMode.pin = 21;
                        }
                        _oneByte |= 1 << 3; // 3th bit for analog pin change

                        break;

                }

		        _settingsChanged = 1;	
                break;

            case WEEK:
                switch (_index) {
                    case 1:
                        if (_dayIndex < 6) {_dayIndexFlag = _dayIndex; ++_dayIndex; }
                        else {_dayIndexFlag = _dayIndex; _dayIndex = 0; } // overflow
                        _changedFlag = 1;
                        break;
                    
                    case 2:
                        if (!currentChannel.data.weekMode.days[_dayIndex].enabled) { _changedFlag = 1; currentChannel.data.weekMode.days[_dayIndex].enabled = 1; _oneByte |= (1 << 7); }
                        else {_changedFlag = 0; }

                        break;
                    case 3:
                        if (currentChannel.data.weekMode.days[_dayIndex].startHour < 23) {
                            if (enc.isFast() && currentChannel.data.weekMode.days[_dayIndex].startHour + 3 < 23) currentChannel.data.weekMode.days[_dayIndex].startHour += 3;
                            else ++currentChannel.data.weekMode.days[_dayIndex].startHour;
                            _oneByte |= 1 << 0 ; // 0b01 
                        } else currentChannel.data.dayMode.startHour = 23; // 0th hour

                        break;
                    case 4:
                        if (currentChannel.data.weekMode.days[_dayIndex].startMinute >= 59 && currentChannel.data.weekMode.days[_dayIndex].startHour < 23) {
                        currentChannel.data.weekMode.days[_dayIndex].startMinute = 0;
                        ++currentChannel.data.weekMode.days[_dayIndex].startHour; 
                        _oneByte |= (1 << 0) | (1 << 1); // 1th bit for startMinute
                        } else if (currentChannel.data.weekMode.days[_dayIndex].startMinute < 59) {
                        
                        if (enc.isFast() && currentChannel.data.weekMode.days[_dayIndex].startMinute + 5 < 59) currentChannel.data.weekMode.days[_dayIndex].startMinute += 5;
                        else ++currentChannel.data.weekMode.days[_dayIndex].startMinute;
                        _oneByte |= 1 << 1;
                        } else currentChannel.data.weekMode.days[_dayIndex].startMinute = 59;
                        
                        break;
                    case 5:
                        if (currentChannel.data.weekMode.days[_dayIndex].startSecond >= 59 && currentChannel.data.weekMode.days[_dayIndex].startHour < 23 && currentChannel.data.weekMode.days[_dayIndex].startMinute < 59) {
                            currentChannel.data.weekMode.days[_dayIndex].startSecond = 0;
                            ++currentChannel.data.weekMode.days[_dayIndex].startMinute;
                            _oneByte |= (1 << 2) | (1 << 1); // 2th bit for startSecond
                        } 

                        else if (currentChannel.data.weekMode.days[_dayIndex].startSecond < 59) {
                            if (enc.isFast() && currentChannel.data.weekMode.days[_dayIndex].startSecond + 5 < 59) currentChannel.data.weekMode.days[_dayIndex].startSecond += 5;
                            else ++currentChannel.data.weekMode.days[_dayIndex].startSecond; 
                            _oneByte |= 1 << 2; 
                        }
                        else currentChannel.data.weekMode.days[_dayIndex].startSecond = 59;

                        break;
                    case 6:
                        if (currentChannel.data.weekMode.days[_dayIndex].endHour < 23) {
                            if (enc.isFast() && currentChannel.data.weekMode.days[_dayIndex].endHour + 5 < 23) currentChannel.data.weekMode.days[_dayIndex].endHour += 3;
                            else ++currentChannel.data.weekMode.days[_dayIndex].endHour;
                            _oneByte |= 1 << 3; }// 3th bit for endHour 
                        
                        else currentChannel.data.weekMode.days[_dayIndex].endHour = 23;
                        break;
                    case 7:
                        if (currentChannel.data.weekMode.days[_dayIndex].endMinute >= 59 && currentChannel.data.weekMode.days[_dayIndex].endHour != 23) {
                            currentChannel.data.weekMode.days[_dayIndex].endMinute = 0;
                            ++currentChannel.data.weekMode.days[_dayIndex].endHour; 
                            _oneByte |= (1 << 3) | (1 << 4); // 4th bit for endMinute
                        }

                        else if (currentChannel.data.weekMode.days[_dayIndex].endMinute < 59) {
                            if (enc.isFast() && currentChannel.data.weekMode.days[_dayIndex].endMinute + 5 < 59) currentChannel.data.weekMode.days[_dayIndex].endMinute += 5;
                            else ++currentChannel.data.weekMode.days[_dayIndex].endMinute; 
                            _oneByte |= 1 << 4; 
                        }
                        
                        break;
                    case 8:
                        if (currentChannel.data.weekMode.days[_dayIndex].endSecond >= 59 && currentChannel.data.weekMode.days[_dayIndex].endHour != 23 && currentChannel.data.weekMode.days[_dayIndex].endMinute < 59) {
                                currentChannel.data.weekMode.days[_dayIndex].endSecond = 0;
                                ++currentChannel.data.weekMode.days[_dayIndex].endMinute;
                                _oneByte |= (1 << 4) | (1 << 5); // 5th bit for endSecond
                        }
                        else if (currentChannel.data.weekMode.days[_dayIndex].endSecond < 59) {
                            if (enc.isFast() && currentChannel.data.weekMode.days[_dayIndex].endSecond + 5 < 59) currentChannel.data.weekMode.days[_dayIndex].endSecond += 5;
                            else ++currentChannel.data.weekMode.days[_dayIndex].endSecond; 
                            _oneByte |= 1 << 5; 
                        }
                        else currentChannel.data.weekMode.days[_dayIndex].endSecond = 0;

                        break;

                }

                if (_index > 2 && _index < 9) _settingsChanged = 1;
                
                break;
            
            case RTC:
                switch (_index) {
                    // pass
                }
                break;
       }
    }
        
    if (_index != 0 && enc.isLeftH()) {
        switch (currentChannel.mode) {
            case TIMER:
                switch (_index) {
                    case 1: // period hours
                        if (currentChannel.data.timerMode.periodHour > 0) {
                            if (enc.isFast() && currentChannel.data.timerMode.periodHour - 3 > 0) currentChannel.data.timerMode.periodHour -= 3;
                            else --currentChannel.data.timerMode.periodHour;
                            _oneByte |= 1 << 0; // 0th bit for periodHour
                        } else currentChannel.data.timerMode.periodHour = 0;
                        break;
                    case 2:
                        if (currentChannel.data.timerMode.periodMinute > 0) {
                            if (enc.isFast() && currentChannel.data.timerMode.periodMinute - 5 > 0) currentChannel.data.timerMode.periodMinute -= 5;
                            else --currentChannel.data.timerMode.periodMinute;
                            _oneByte |= 1 << 1; // 1th bit for periodMinute
                        } else currentChannel.data.timerMode.periodMinute = 0;
                        break;
                    case 3:
                        if (currentChannel.data.timerMode.periodSecond > 0) {
                            if (enc.isFast() && currentChannel.data.timerMode.periodSecond - 5 > 0) currentChannel.data.timerMode.periodSecond -= 5;
                            else --currentChannel.data.timerMode.periodSecond;
                            _oneByte |= 1 << 2; // 2th bit for periodSecond
                        } else currentChannel.data.timerMode.periodSecond = 0;
                        break;
                    
                    case 4:
                        if (currentChannel.data.timerMode.workMinute > 0) {
                            if (enc.isFast() && currentChannel.data.timerMode.workMinute - 5 > 0) currentChannel.data.timerMode.workMinute -= 5;
                            else --currentChannel.data.timerMode.workMinute;
                            _oneByte |= 1 << 3; // 3th bit for workMinute
                        } else currentChannel.data.timerMode.workMinute = 0;
                        break;
                    case 5:
                        if (currentChannel.data.timerMode.workSecond > 0) {
                            if (enc.isFast() && currentChannel.data.timerMode.workSecond - 5 > 0) currentChannel.data.timerMode.workSecond -= 5;
                            else --currentChannel.data.timerMode.workSecond;
                            _oneByte |= 1 << 4; // 4th bit for workSecond
                        } else currentChannel.data.timerMode.workSecond = 0;
                        break;
                }

                _settingsChanged = 1;
                break;
            
            case DAY: // _oneByte as flags for changed sets
                switch (_index) {
                    case 1:
                        if (currentChannel.data.dayMode.startHour > 0) {
                            if (enc.isFast() && currentChannel.data.dayMode.startHour - 3 > 0) currentChannel.data.dayMode.startHour -= 3;
                            else --currentChannel.data.dayMode.startHour;
                            _oneByte |= 1 << 0; //0th bit for startHour
                        }

                        else currentChannel.data.dayMode.startHour = 0;
                        break;
                    case 2:
                        if (currentChannel.data.dayMode.startMinute > 0) {
                            if (enc.isFast() && currentChannel.data.dayMode.startMinute - 5 > 0) currentChannel.data.dayMode.startMinute -= 5;
                            else --currentChannel.data.dayMode.startMinute;
                            _oneByte |= 1 << 1; // 1th bit for startMinute
                        }
                        else currentChannel.data.dayMode.startMinute = 0; 
                        break;
                    case 3:
                        if (currentChannel.data.dayMode.startSecond > 0) {
                             if (enc.isFast() && currentChannel.data.dayMode.startSecond - 5 > 0) currentChannel.data.dayMode.startSecond -= 5;
                            else --currentChannel.data.dayMode.startSecond;
                            _oneByte |= 1 << 2; // 2th bit for startSecond
                        }
                        else currentChannel.data.dayMode.startSecond = 0;
                        break;
                    case 4:
                        if (currentChannel.data.dayMode.endHour > 0) {
                            if (enc.isFast() && currentChannel.data.dayMode.endHour - 3 > 0) currentChannel.data.dayMode.endHour -= 3;
                            else --currentChannel.data.dayMode.endHour;
                            _oneByte |= 1 << 3; // 3th bit for endHour
                        }
                        else currentChannel.data.dayMode.endHour = 0;
                        break;
                    case 5:
                        if (currentChannel.data.dayMode.endMinute > 0) {
                            if (enc.isFast() && currentChannel.data.dayMode.endMinute - 5 > 0) currentChannel.data.dayMode.endMinute -= 5;
                            else --currentChannel.data.dayMode.endMinute;
                            _oneByte |= 1 << 4; // 4th bit for endMinute
                        }
                        else currentChannel.data.dayMode.endMinute = 0;
                        break;
                    case 6:
                        if (currentChannel.data.dayMode.endSecond > 0) {
                            if (enc.isFast() && currentChannel.data.dayMode.endSecond - 5 > 0) currentChannel.data.dayMode.endSecond -= 5;
                            else --currentChannel.data.dayMode.endSecond;
                            _oneByte |= 1 << 5; // 5th bit for endSecond
                        }
                        else currentChannel.data.dayMode.endSecond = 0;
                        break;
                }


                _settingsChanged = 1;

                break;
            
            case SENSOR:
                switch (_index) {
                    case 1:
                        if (currentChannel.data.sensorMode.threshold > 0) {
                            if (enc.isFast() && currentChannel.data.sensorMode.threshold - 50 > 0) currentChannel.data.sensorMode.threshold -= 50;
                            else --currentChannel.data.sensorMode.threshold;
                        } else currentChannel.data.sensorMode.threshold = 0; 
                        _oneByte |= 1 << 0; // 0th bit for threshold
                        break;
                    case 2:
                        if (currentChannel.data.sensorMode.workMinute > 0) {
                            if (enc.isFast() && currentChannel.data.sensorMode.threshold - 5 > 0) currentChannel.data.sensorMode.workMinute -= 5;
                            else --currentChannel.data.sensorMode.workMinute;
                        } else currentChannel.data.sensorMode.workMinute = 0;
                        _oneByte |= 1 << 1; // 1th bit for threshold
                        break;
                    case 3:
                        if (currentChannel.data.sensorMode.workSecond > 0) {
                            if (enc.isFast() && currentChannel.data.sensorMode.workSecond - 5 > 0) currentChannel.data.sensorMode.workSecond -= 5;
                            else --currentChannel.data.sensorMode.workSecond;
                        } else currentChannel.data.sensorMode.workSecond = 0;
                        _oneByte |= 1 << 2; // 2th bit for threshold
                        break;
                    case 4:
                        --currentChannel.data.sensorMode.pin;
                        if (currentChannel.data.sensorMode.pin < 20 && currentChannel.data.sensorMode.pin > 17) {
                            currentChannel.data.sensorMode.pin = 17;
                        }

                        if (currentChannel.data.sensorMode.pin < 14) {
                            currentChannel.data.sensorMode.pin = 14;
                        }
                        _oneByte |= 1 << 3; // 3th bit for analog pin change

                        break;
                }


                _settingsChanged = 1;
                break;

            case WEEK:
                switch (_index) {
                    case 1:
                        if (_dayIndex > 0) {_dayIndexFlag = _dayIndex; --_dayIndex; }
                        else {_dayIndexFlag = _dayIndex; _dayIndex = 6; } // overflow
                        _changedFlag = 1;
                        break;
                    
                    case 2:
                        if (currentChannel.data.weekMode.days[_dayIndex].enabled) { _changedFlag = 1; currentChannel.data.weekMode.days[_dayIndex].enabled = 0; _oneByte |= (1 << 7); }
                        else {_changedFlag = 0; }

                        break;

                    case 3:
                        if (currentChannel.data.weekMode.days[_dayIndex].startHour > 0) {
                            if (enc.isFast() && currentChannel.data.weekMode.days[_dayIndex].startHour - 3 > 0) currentChannel.data.weekMode.days[_dayIndex].startHour -= 3;
                            else --currentChannel.data.weekMode.days[_dayIndex].startHour;
                            _oneByte |= 1 << 0; //0th bit for startHour
                        }

                        else currentChannel.data.weekMode.days[_dayIndex].startHour = 0;
                        break;

                    case 4:
                        if (currentChannel.data.weekMode.days[_dayIndex].startMinute > 0) {
                            if (enc.isFast() && currentChannel.data.weekMode.days[_dayIndex].startMinute - 5 > 0) currentChannel.data.weekMode.days[_dayIndex].startMinute -= 5;
                            else --currentChannel.data.weekMode.days[_dayIndex].startMinute;
                            _oneByte |= 1 << 1; // 1th bit for startMinute
                        }
                        else currentChannel.data.weekMode.days[_dayIndex].startMinute = 0; 
                        break;
                    case 5:
                        if (currentChannel.data.weekMode.days[_dayIndex].startSecond > 0) {
                             if (enc.isFast() && currentChannel.data.weekMode.days[_dayIndex].startSecond - 5 > 0) currentChannel.data.weekMode.days[_dayIndex].startSecond -= 5;
                            else --currentChannel.data.weekMode.days[_dayIndex].startSecond;
                            _oneByte |= 1 << 2; // 2th bit for startSecond
                        }
                        else currentChannel.data.weekMode.days[_dayIndex].startSecond = 0;
                        break;
                    case 6:
                        if (currentChannel.data.weekMode.days[_dayIndex].endHour > 0) {
                            if (enc.isFast() && currentChannel.data.weekMode.days[_dayIndex].endHour - 3 > 0) currentChannel.data.weekMode.days[_dayIndex].endHour -= 3;
                            else --currentChannel.data.weekMode.days[_dayIndex].endHour;
                            _oneByte |= 1 << 3; // 3th bit for endHour
                        }
                        else currentChannel.data.weekMode.days[_dayIndex].endHour = 0;
                        break;
                    case 7:
                        if (currentChannel.data.weekMode.days[_dayIndex].endMinute > 0) {
                            if (enc.isFast() && currentChannel.data.weekMode.days[_dayIndex].endMinute - 5 > 0) currentChannel.data.weekMode.days[_dayIndex].endMinute -= 5;
                            else --currentChannel.data.weekMode.days[_dayIndex].endMinute;
                            _oneByte |= 1 << 4; // 4th bit for endMinute
                        }
                        else currentChannel.data.weekMode.days[_dayIndex].endMinute = 0;
                        break;
                    case 8:
                        if (currentChannel.data.weekMode.days[_dayIndex].endSecond > 0) {
                            if (enc.isFast() && currentChannel.data.weekMode.days[_dayIndex].endSecond - 5 > 0) currentChannel.data.weekMode.days[_dayIndex].endSecond -= 5;
                            else --currentChannel.data.weekMode.days[_dayIndex].endSecond;
                            _oneByte |= 1 << 5; // 5th bit for endSecond
                        }
                        else currentChannel.data.weekMode.days[_dayIndex].endSecond = 0;
                        break;
                }

                if (_index > 2 && _index < 9) _settingsChanged = 1;

                break;
            
            case RTC:
                switch (_index) {
                    // pass
                }
                break;
       }
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

                lcd.setCursor(15, 0);
                lcd.print("     ");

                if (currentChannel.mode == OFF) {
                    lcd.setCursor(17, 0);
                    lcd.print("Off");
                } else {
                    lcd.setCursor(18, 0);
                    lcd.print("On");
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

                    case RTC:
                        lcd.setCursor(0, 1);
                        lcd.print("Mode: <RTC>");

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

                    case RTC:
                        lcd.setCursor(0, 0);
                        lcd.print("<RTC>");

                        lcd.setCursor(0, 1);
                        lcd.print("Start: 00:00.00");

                        lcd.setCursor(0, 2);
                        lcd.print("End: 00:00.00s");
                        
                        lcd.setCursor(0, 3);
                        lcd.print("Left: 00:00.00s"); // hours:minutes.seconds
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
                        

                        lcd.setCursor(9, 3);
                        lcd.print("Val: ");
                        lcd.print(analogRead(currentChannel.data.sensorMode.pin));
                       break;
                    
                    case WEEK:
                        lcd.setCursor(0, 0);
                        lcd.print("<WEEK>");

                        lcd.setCursor(0, 1);
                        lcd.print("Day: Monday       On");

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
                                    
                            case RTC:
                                lcd.setCursor(0, 1);
                                lcd.print("                    ");
                                lcd.setCursor(0, 1);
                                lcd.print("Mode: <RTC>");
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

                    case 3: // Back
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
                                    
                            case RTC:
                                lcd.setCursor(0, 1);
                                lcd.print("          ");
                                lcd.setCursor(0, 1);
                                lcd.print(">Mode: <RTC>");
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

                    case 3: // Back
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
            if (currentChannel.mode == SENSOR && millis() - _tmr > 300) {
                _tmr = millis();
                lcd.setCursor(14, 3);
                lcd.print("      ");
                lcd.setCursor(14, 3);
                lcd.print(analogRead(currentChannel.data.sensorMode.pin));
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
                        
                    
                    case RTC:
                        //
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

                print2digits(currentChannel.data.dayMode.startHour, lcd, 7, 2);
                _oneByte &= 254;
            }

            if (_oneByte & (1 << 1)) {
			    lcd.setCursor(10, 2);
			    lcd.print("  ");

			    print2digits(currentChannel.data.dayMode.startMinute, lcd, 10, 2);
                _oneByte &= 253;
            }

            if (_oneByte & (1 << 2)) {
			    lcd.setCursor(13, 2);
			    lcd.print("  ");

			    print2digits(currentChannel.data.dayMode.startSecond, lcd, 13, 2);
                _oneByte &= 251;
            }

            if (_oneByte & (1 << 3)) {
			    lcd.setCursor(5, 3);
			    lcd.print("  ");

			    print2digits(currentChannel.data.dayMode.endHour, lcd, 5, 3);
                _oneByte &= 247;
            }

            if (_oneByte & (1 << 4)) {
			    lcd.setCursor(8, 3);
			    lcd.print("  ");

			    print2digits(currentChannel.data.dayMode.endMinute, lcd, 8, 3);
                _oneByte &= 239;
            }

            if (_oneByte & (1 << 5)) {
			    lcd.setCursor(11, 3);
			    lcd.print("  ");

			    print2digits(currentChannel.data.dayMode.endSecond, lcd, 11, 3);
                _oneByte &= 223;
            }

            break;

		case RTC:
			break;
		
		}		
	_settingsChanged = 0;
	}
       break;
    }
}
