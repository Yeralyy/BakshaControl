#pragma once
#include "eeprom_control.h"
#include "lib/rtc/RtcDS1302.h"


void scheduelerTick(RtcDateTime& now);



void scheduelerTick(RtcDateTime& now) {
    for (int i = 1; i < 9; ++i) {
        Channel currentChannel {getChannel(i)};
        /*
        #if LOG
        Serial.print(i);
        Serial.print(F(" Channel mode: "));
        Serial.print(currentChannel.mode);
        Serial.println();
        #endif
        */

        switch (currentChannel.mode) {
            case OFF:
                continue;
            case TIMER:
                if (digitalRead(channelsPins[i - 1]) == LOW && millis() - timers[i - 1] >= (currentChannel.data.timerMode.periodHour * 3600000) + (currentChannel.data.timerMode.periodMinute * 60000) + (currentChannel.data.timerMode.periodSecond * 1000)) {
                    digitalWrite(channelsPins[i - 1], HIGH);
                    timers[i - 1] = millis();
                }

                if (digitalRead(channelsPins[i - 1]) == HIGH && millis() - timers[i - 1] >= (currentChannel.data.timerMode.workMinute * 60000) + (currentChannel.data.timerMode.workSecond * 1000)) {
                    digitalWrite(channelsPins[i - 1], LOW);
                    timers[i - 1] = millis();
                }
                 
                break;

            case DAY:
                if (digitalRead(channelsPins[i - 1]) == LOW) {
                    if (now.Hour() == currentChannel.data.dayMode.startHour) {
                        if (now.Minute() == currentChannel.data.dayMode.startMinute) {
                            if (now.Second() >= currentChannel.data.dayMode.startSecond) {
                                digitalWrite(channelsPins[i - 1], HIGH);
                            }
                        }
                    }
                    
                }

                if (digitalRead(channelsPins[i - 1]) == HIGH) {
                    if (now.Hour() == currentChannel.data.dayMode.endHour) {
                        if (now.Minute() == currentChannel.data.dayMode.endMinute) {
                            if (now.Second() >= currentChannel.data.dayMode.endSecond) {
                                digitalWrite(channelsPins[i - 1], LOW);
                            }
                        }
                    }
                }
            
                break;

            case SENSOR:
                if (analogRead(channelsPins[i - 1]) >= currentChannel.data.sensorMode.threshold && !digitalRead(channelsPins[i - 1])) {
                    digitalWrite(channelsPins[i - 1], HIGH);
                    timers[i - 1] = millis();
                }

                if (millis() - timers[i - 1] > currentChannel.data.sensorMode.workMinute * 60000 + currentChannel.data.sensorMode.workSecond * 1000 && digitalRead(channelsPins[i - 1])) {
                    digitalWrite(channelsPins[i - 1], LOW);
                }
            

                break;
        }
        
    }

}