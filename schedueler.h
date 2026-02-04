#pragma once
#include "CONFIG.h"
#include "eeprom_control.h"
#include <RtcDS3231.h>

#if nRF
#include "nrf.h"
#endif

void scheduelerTick(RtcDateTime& now);


void scheduelerTick(RtcDateTime& now) {
    for (int i = 1; i < 7 + 1; ++i) {
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
                if (digitalRead(channelsPins[i - 1]) == !uint8_t(currentChannel.relayMode) && millis() - timers[i - 1] >= (currentChannel.data.timerMode.periodHour * 3600000) + (currentChannel.data.timerMode.periodMinute * 60000) + (currentChannel.data.timerMode.periodSecond * 1000)) {
                    #if nRF
                    packet.type = CONTROL_PACKAGE;
                    sendPackage(currentChannel.relayMode);
                    digitalWrite(channelsPins[i - 1], currentChannel.relayMode);
                   #else
                    digitalWrite(channelsPins[i - 1], currentChannel.relayMode);
                    #endif
                    timers[i - 1] = millis();
                }

                if (digitalRead(channelsPins[i - 1]) == uint8_t(currentChannel.relayMode) && millis() - timers[i - 1] >= (currentChannel.data.timerMode.workMinute * 60000) + (currentChannel.data.timerMode.workSecond * 1000)) {
                    #if nRF
                    sendPackage(!currentChannel.relayMode);
                    digitalWrite(channelsPins[i - 1], !currentChannel.relayMode);
                    #else
                    digitalWrite(channelsPins[i - 1], !currentChannel.relayMode);
                    #endif
                    timers[i - 1] = millis();
                }
                 
                break;

            case DAY:
                if (digitalRead(channelsPins[i - 1]) == !uint8_t(currentChannel.relayMode)) {
                    if (now.Hour() == currentChannel.data.dayMode.startHour) {
                        if (now.Minute() == currentChannel.data.dayMode.startMinute) {
                            if (now.Second() >= currentChannel.data.dayMode.startSecond) {
                                digitalWrite(channelsPins[i - 1], currentChannel.relayMode);
                            }
                        }
                    }
                    
                }

                if (digitalRead(channelsPins[i - 1]) == uint8_t(currentChannel.relayMode)) {
                    if (now.Hour() == currentChannel.data.dayMode.endHour) {
                        if (now.Minute() == currentChannel.data.dayMode.endMinute) {
                            if (now.Second() >= currentChannel.data.dayMode.endSecond) {
                                digitalWrite(channelsPins[i - 1], !uint8_t(currentChannel.relayMode));
                            }
                        }
                    }
                }
            
                break;

            case SENSOR:
                if (analogRead(channelsPins[i - 1]) >= currentChannel.data.sensorMode.threshold && digitalRead(channelsPins[i - 1]) == !uint8_t(currentChannel.relayMode)) {
                    digitalWrite(channelsPins[i - 1], uint8_t(currentChannel.relayMode));
                    timers[i - 1] = millis();
                }

                if (millis() - timers[i - 1] > currentChannel.data.sensorMode.workMinute * 60000 + currentChannel.data.sensorMode.workSecond * 1000 && digitalRead(channelsPins[i - 1]) == uint8_t(currentChannel.relayMode)) {
                    digitalWrite(channelsPins[i - 1], !uint8_t(currentChannel.relayMode));
                }
            

                break;

            case WEEK:
               uint8_t today = now.DayOfWeek();
               if (today >= 1) --today;
               else if (today == 0) today = 6; // sunday
                
               
               if (currentChannel.data.weekMode.days[today].enabled) {
               
                    if (digitalRead(channelsPins[i - 1]) == !uint8_t(currentChannel.relayMode)) {
                        if (now.Hour() == currentChannel.data.weekMode.days[today].startHour) {
                            if (now.Minute() == currentChannel.data.weekMode.days[today].startMinute) {
                                if (now.Second() >= currentChannel.data.weekMode.days[today].startSecond) {
                                    digitalWrite(channelsPins[i - 1], uint8_t(currentChannel.relayMode));
                                }
                            }
                        }
                    
                    }

                    if (digitalRead(channelsPins[i - 1]) == uint8_t(currentChannel.relayMode)) {
                        if (now.Hour() == currentChannel.data.weekMode.days[today].endHour) {
                            if (now.Minute() == currentChannel.data.weekMode.days[today].endMinute) {
                                if (now.Second() >= currentChannel.data.weekMode.days[today].endSecond) {
                                    digitalWrite(channelsPins[i - 1], !uint8_t(currentChannel.relayMode));
                                }
                            }
                        }
                    }
                }

                break;
               



        }
        
    }

}