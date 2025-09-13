#pragma once
#include "lib/encMinim.h"
#include <LiquidCrystal_I2C.h>
#include "eeprom_control.h"
#include "lib/rtc/RtcDateTime.h"

#include "states.h"


// MUSTAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAARD

class ArrowControl {
    public:
        void menuTick(encMinim& enc, LiquidCrystal_I2C& lcd, FSM& state);
        void channelsTick(encMinim& enc, LiquidCrystal_I2C& lcd, FSM& state);
        void modesTick(encMinim& enc, LiquidCrystal_I2C& lcd, FSM& state);
    private:
        Channel currentChannel;

        int8_t _count;
        int8_t _index; // arrow index
        int8_t _indexFlag;

        // for modes preferenes
        uint8_t _minutes {0};
        uint8_t _hours {0};
        uint8_t _seconds {0};
        bool _changedFlag {0}; bool _first {1};
        bool _inChannelFlag {0};
        bool _newReadFlag {1}; // Flag for EEPROM read
        bool _channelFlag {0};

        void redrawDisplay(LiquidCrystal_I2C& lcd, FSM& state);
        void updateDisplay(LiquidCrystal_I2C& lcd, FSM& state);
};


// ====================================== CODE ======================================

void ArrowControl::menuTick(encMinim& enc, LiquidCrystal_I2C& lcd, FSM& state)  {
    redrawDisplay(lcd, state);

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
                state = SENSORS; // FSM& 
                #if LOG
                Serial.println("State changed to DAY");
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
                Serial.println("Changing state to MAIN_MENU");
                #endif
                lcd.clear();
                state = MAIN_MENU; // MAIN_MENU
                _first = 1;
                _inChannelFlag = 0;
                _index = 0;

                // saving current channel settings
                if (_channelFlag) {
                    putChannel(_count, currentChannel);
                    _channelFlag = 0;
                }
            }

            if (_index == 2 && enc.isClick()) {
                #if LOG
                Serial.println("Changing state to MODES");
                #endif
                lcd.clear();
                
                state = MODES;
                _first = 1;
                _inChannelFlag = 0;
                _index = 0;
            }
                
            /* Detecting enc actions */
            if (enc.isRightH()) {
                switch (_index) {
                    case 0: // 
                        /* Saving Channel settings*/
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
                            
                            case DAY:
                                lcd.setCursor(0, 1);
                                lcd.print("Mode: <Day>");
                                break;
                        }
                        
                        _channelFlag = 1;

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
                            
                            case DAY:
                                lcd.setCursor(0, 1);
                                lcd.print("Mode: <Day>");
                                break;
                            
                            case SENSOR:
                                lcd.setCursor(0, 1);
                                lcd.print("Mode: <Sensor>");
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

                        _channelFlag = 1;
                        
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
                            
                            case DAY:
                                lcd.setCursor(0, 1);
                                lcd.print("Mode: <Day>");
                                break;
                            
                            case SENSOR:
                                lcd.setCursor(0, 1);
                                lcd.print("Mode: <Sensor>");
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
            Serial.println("In Channel settings");
            #endif
        }

        }


void ArrowControl::modesTick(encMinim& enc, LiquidCrystal_I2C& lcd, FSM& state) {
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
            if (_index > 3 && _indexFlag == 3) {_changedFlag = 0; _index = 3; }
            break;    
   }

    if (_changedFlag) updateDisplay(lcd, state);


   /*
   if (_index != 0 && enc.isRightH()) {
        switch (_index) {
            case 1:
                ++currentChannel.hour;
                //updateDisplay(lcd, state);
                break;
            case 2:
                ++currentChannel.minute;   
                break;
            case 3:
                ++currentChannel.day;
                break;
            case 4:
                ++currentChannel.month;
                break;
            case 5:
                ++_hours;
                break;
            case 6:
                ++_minutes;
                break;
            case 7:
                ++_seconds;
                break;
        } 
   }
   */ 
  

   /*
   if (_index != 0 && enc.isLeftH()) {
        switch (_index) {
            case 1:
                --currentChannel.hour;    
                break;
            case 2:
                --currentChannel.minute;
                break;
            case 3:
                --currentChannel.day;
                break;
            case 4:
                --currentChannel.month;
                break;
            case 5:
                --_hours;
                break;
            case 6:
            --_minutes;
                break;
            case 7:
                --_seconds;
                break;
        }
    */

   


   if (_index == 0 && enc.isClick()) {
    state = CHANNELS;
    lcd.clear();
    _first = 1;
   }


}







void ArrowControl::redrawDisplay(LiquidCrystal_I2C& lcd, FSM& state) {
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
                        lcd.print("Mode: <Timer>");

                        lcd.setCursor(16, 3);
                        lcd.print("Back");
                        break;

                    case RTC:
                        lcd.setCursor(18, 0);
                        lcd.print("On");
                
                        lcd.setCursor(0, 1);
                        lcd.print("Mode: <RTC>");

                        lcd.setCursor(16, 3);
                        lcd.print("Back");

                        break;

                    case DAY:
                        lcd.setCursor(18, 0);
                        lcd.print("On");

                        lcd.setCursor(0, 1);
                        lcd.print("Mode: <Day>");

                        lcd.setCursor(16, 3);
                        lcd.print("Back");
                        break;
                    
                    case SENSOR:
                        lcd.setCursor(18, 0);
                        lcd.print("On");

                        lcd.setCursor(0, 1);
                        lcd.print("Mode: <Sensor>");

                        lcd.setCursor(16, 3);
                        lcd.print("Back");
                        break;
                    }
            break;

            case MODES:
                switch (currentChannel.mode) {
                    case TIMER:
                        lcd.setCursor(0, 0);
                        lcd.print("<TIMER>");
                        lcd.setCursor(15, 0);
                        lcd.print(">Back");

                        lcd.setCursor(0, 1);
                        lcd.print("Period: 00h 00m 00s"); // period. e.g: every 5hours, every 5 mins

                        lcd.setCursor(0, 2);
                        lcd.print("Work: 00m 00s"); // work time 
                        
                        lcd.setCursor(0, 3);
                        lcd.print("Left: 00h 00m 00s");
                        break;
                    case RTC:
                        lcd.setCursor(0, 0);
                        lcd.print("<RTC>");
                        lcd.setCursor(15, 0);
                        lcd.print(">Back");

                        lcd.setCursor(0, 1);
                        lcd.print("Start: 00:00.00");
                        //lcd.print("00:00 00.00"); // hour:minute day:month

                        lcd.setCursor(0, 2);
                        lcd.print("End: 00:00.00s");
                        
                        lcd.setCursor(0, 3);
                        lcd.print("Left: 00:00.00s"); // hours:minutes.seconds

                        break;
                    case DAY:
                        lcd.setCursor(0, 0);
                        lcd.print("<Day>");
                        lcd.setCursor(15, 0);
                        lcd.print(">Back");

                        lcd.setCursor(0, 1);
                        lcd.print("Start: 00:00.00s");
                        lcd.setCursor(0, 2);
                        lcd.print("End: 00:00.00s");

                        lcd.setCursor(0, 3);
                        lcd.print("Left: 00:00.00s");

                        break;
                    
                    case SENSOR:
                        lcd.setCursor(0, 0);
                        lcd.print("<Sensor>");
                        lcd.setCursor(15, 0);
                        lcd.print(">Back");

                        lcd.setCursor(0, 1);
                        lcd.print("Treshold: 1023");

                        lcd.setCursor(0, 2);
                        lcd.print("Work: 00m 00s");

                        /*
                        lcd.setCursor(0, 3);
                        lcd.print("Value: ");
                        lcd.print()
                        int sensorValue = analogRead(currentChannel.pin);
                        */
                       break;
                }
            break;

        }
        _first = 0;
    }
}

void ArrowControl::updateDisplay(LiquidCrystal_I2C& lcd, FSM& state) {

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
            break;
            
        // ==================================== MODES =======================================
        
        case MODES:
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
                                lcd.setCursor(9, 1);
                                break;
                            case 2:
                                lcd.setCursor(5, 2);
                                break;
                            case 3:
                                lcd.setCursor(9, 2);
                                break;
                        }
                        lcd.print(' ');

                        switch (_index) { // printing new one
                            case 0:
                                lcd.setCursor(15, 0);
                                break;
                            case 1:
                                lcd.setCursor(9, 1);
                                break;
                            case 2:
                                lcd.setCursor(5, 2);
                                break;
                            case 3:
                                lcd.setCursor(9, 2);
                                break;
                        }
                        lcd.print('>');


                        break;
                    
                    case RTC:
                        //
                        break;
                        

                }



            } _changedFlag = 0;
        break;
    }
        
}