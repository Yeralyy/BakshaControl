#pragma once
#include "lib/encMinim.h"
#include "states.h"
#include <LiquidCrystal_I2C.h>
#include "eeprom_control.h"


// MUSTAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAARD

class ArrowControl {
    public:
        void setRange(uint8_t start_index, uint8_t end_index);
        void menuTick(encMinim& enc, LiquidCrystal_I2C& lcd, FSM& state);
        void channelsTick(encMinim& enc, LiquidCrystal_I2C& lcd, FSM& state);
    private:
        Channel currentChannel;

        uint8_t _start_index {0};
        uint8_t _end_index;
        int8_t _count;
        int8_t _index; // arrow index
        int8_t _indexFlag;
        bool _changedFlag {0};
        bool _first {1};
        bool _inChannelFlag {0};
        bool _newReadFlag {1}; // Flag for EEPROM read

        void updateDisplay(LiquidCrystal_I2C& lcd, FSM& state);


};


void ArrowControl::setRange(uint8_t start_index, uint8_t end_index) {
    _start_index = start_index; _end_index = end_index; }

void ArrowControl::menuTick(encMinim& enc, LiquidCrystal_I2C& lcd, FSM& state)  {
    updateDisplay(lcd, state);

    if (enc.isTurn()) {
        if (enc.isRight()) ++_count;
        if (enc.isLeft()) --_count;
        _changedFlag = 1;  
        #if LOG
        Serial.println("MAIN_MENU: Encoder turn");
        #endif
    }

    if (enc.isClick()) {
        switch (_count) {
            case 0:
                state = CHANNELS;
                _count = 1;
                #if LOG
                Serial.println("State changed to CHANNELS");
                #endif

                break;
            case 1:
                state = SERVICE;
                #if LOG
                Serial.println("State changed to SERVICE");
                #endif

                break;
            case 2:
                state = SENSORS;
                #if LOG
                Serial.println("State changed to SENSORS");
                #endif

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
    updateDisplay(lcd, state);
        // ==================================== ARROW TRACKING ==============================

        if (_inChannelFlag) {
            
            //Channel currentChannel {getChannel(_count)}; // getting channel information from EEPROM
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
                else if (_index > 3) {_index = 3; _changedFlag = 0; } // constraining
            } else {
                if (_index < 0) { _index = 0; _changedFlag = 0; } // if mode is OFF
                else if (_index == 2 && _indexFlag != 3) {_index = 3; }
                else if (_index == 2) {_index = 1; }
                else if (_index > 3) {_index = 3; _changedFlag = 0; }
            }

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
                                lcd.print("              ");
                                lcd.setCursor(0, 1);
                                lcd.print("Mode: ");
                                lcd.print("<Timer>");
                                break;
                                    
                            case RTC:
                                lcd.setCursor(0, 1);
                                lcd.print("          ");
                                lcd.setCursor(0, 1);
                                lcd.print("Mode: ");
                                lcd.print("<RTC>");
                                break;
                                   
                            case SENSOR:
                                lcd.setCursor(0, 1);
                                lcd.print("              ");
                                lcd.setCursor(0, 1);
                                lcd.print("Mode: ");
                                lcd.print("<Sensor>");
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
                    Serial.println("Removing old arrow");
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
                                lcd.print(">Mode: ");
                                lcd.print("<Timer>");
                                break;
                                    
                            case RTC:
                                lcd.setCursor(0, 1);
                                lcd.print("          ");
                                lcd.setCursor(0, 1);
                                lcd.print(">Mode: ");
                                lcd.print("<RTC>");
                                break;
                                   
                            case SENSOR:
                                lcd.setCursor(0, 1);
                                lcd.print("              ");
                                lcd.setCursor(0, 1);
                                lcd.print(">Mode: ");
                                lcd.print("<Sensor>");
                                break;
                            }
                        break;

                    case 3: // Back
                        lcd.setCursor(15, 3);
                        lcd.print('>');
                        break;
                    
                    #if LOG
                    Serial.println("Drawing new arrow");
                    #endif
                // ====================================== ARROW TRACKING END ===========================
                }
                _changedFlag = 0;
            }

            if (enc.isClick() && _index == 3) {
                #if LOG
                Serial.println("Changing state to MAIN_MENU");
                #endif
                lcd.clear();
                state = MAIN_MENU; // MAIN_MENU
                _first = 1;
                _inChannelFlag = 0;
            }
                
            /* Detecting enc actions */
            if (enc.isRightH()) {
                switch (_index) {
                    case 0: // 
                        /* Saving Channel settings*/
                        putChannel(_count, currentChannel);

                        ++_count; // saving settings of current Channel and moving to the next
                        _first = 1; 
                        _inChannelFlag = 0; 

                        lcd.setCursor(0, 1);
                        lcd.print("                   ");

                        #if LOG
                        Serial.println("Next Channel, Previous saved into EEPROM");
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
                            
                            case SENSOR:
                                lcd.setCursor(0, 1);
                                lcd.print("Mode: <Sensor>");
                                break;
                        }
                        

                        break;
                    
                    case 2:
                        if (currentChannel.mode != OFF) currentChannel.mode++; // Next Mode

                        lcd.setCursor(0, 1); 
                        lcd.print("                   "); // clearing Mode

                        switch (currentChannel.mode) {
                            case TIMER:
                                lcd.setCursor(0, 1);
                                lcd.print("Mode: <Timer>");
                                break;
                            
                            case RTC:
                                lcd.setCursor(0, 1);
                                lcd.print("Mode: <RTC>");
                                break;
                            
                            case SENSOR:
                                lcd.setCursor(0, 1);
                                lcd.print("Mode: <Sensor>");
                                break;
                        }

                        break;


                    }
                }

            if (enc.isLeftH()) {
                switch (_index) {
                    case 0:
                        /* Saving channel settings in EEPROM */
                        putChannel(_count, currentChannel);

                        --_count;
                        _first = 1;
                        _inChannelFlag = 0;
                        _changedFlag = 1;

                        lcd.setCursor(0, 1);
                        lcd.print("                   ");

                        #if LOG
                        Serial.println("Previous Channel. Current one settings saved");
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


                        break;

                    case 2:
                        if (currentChannel.mode != OFF) currentChannel.mode--; // Decrement Mode

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
                            
                            case SENSOR:
                                lcd.setCursor(0, 1);
                                lcd.print("Mode: <Sensor>");
                                break;
                        }
                        break;



                }
            }

            /*
            if (enc.isLeftH()) {
                switch (_index) {
                    case 0:
                        --_count;
                        _first = 1;
                        break;
                        
                }
            }
                */


            
            } 

        if (enc.isRightH() && !_inChannelFlag) {++_count; _first = 1; _changedFlag = 1; }
        if (enc.isLeftH() && !_inChannelFlag) {--_count; _first = 1; _changedFlag = 1; }

        /*
        #if LOG
        Serial.println("Channel change");
        #endif
        */
        
        if (enc.isClick()) { 
            _inChannelFlag = 1;
            #if LOG
            Serial.println("In Channel settings");
            #endif
        }

        }


void ArrowControl::updateDisplay(LiquidCrystal_I2C& lcd, FSM& state) {
    if (_first) {
        switch (state) {
            case MAIN_MENU:
                #if LOG
                Serial.println("Drawing MAIN_MENU _first = 1");
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
                Serial.println("Drawing CHANNELS _first = 1");
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
                Serial.println("Channel fetched from EEPROM");
                #endif
                }
                #if TEST
                currentChannel.mode = TIMER;
                #endif

                lcd.setCursor(0, 1);
                lcd.print("                   ");

                lcd.setCursor(15, 0);
                lcd.print("     ");

                switch (currentChannel.mode) {
                    case OFF:
                        lcd.setCursor(17, 0);
                        lcd.print("Off");

                        lcd.setCursor(16, 3);
                        lcd.print("Back");
                        break;
                    case TIMER:
                        lcd.setCursor(18, 0);
                        lcd.print("On");

                        lcd.setCursor(0, 1);
                        lcd.print("Mode: ");
                        lcd.print("<Timer>");

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

                        break;

                    case SENSOR:
                        lcd.setCursor(18, 0);
                        lcd.print("On");

                        lcd.setCursor(0, 1);
                        lcd.print("Mode: ");
                        lcd.print("<Sensor>");

                        lcd.setCursor(16, 3);
                        lcd.print("Back");
                        break;

                    }
                    break;

        }
        _first = 0;
    }
}