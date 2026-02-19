#pragma once
#include "CONFIG.h"
#include "eeprom_control.h"
#include <RtcDS3231.h>

#if nRF
#include "nrf.h"
#endif

bool channels_state[5] {0};

void scheduelerTick(RtcDateTime& now, Radio& radio);

int8_t i = 1;

void scheduelerTick(RtcDateTime& now, Radio& radio) {
    int8_t channels_n = channelsCount();

    if (i < channels_n + 1) {
        Channel slaveChannel {getChannel(i)};

        #if LOG
        Serial.print("Channel ");
        Serial.print(i);
        Serial.print(" mode: ");
        #endif
        

        switch (slaveChannel.mode) {
            case OFF:
                Serial.println("OFF");
                break;
            case TIMER:
                #if LOG
                Serial.println("TIMER");
                #endif
                if (channels_state[i - 1] == !uint8_t(slaveChannel.relayMode) && millis() - timers[i - 1] >= (slaveChannel.data.timerMode.periodHour * 3600000) + (slaveChannel.data.timerMode.periodMinute * 60000) + (slaveChannel.data.timerMode.periodSecond * 1000)) {
                    channels_state[i - 1] = uint8_t(slaveChannel.relayMode);
                    radio.setPackageType(CONTROL_PACKAGE);
                    radio.sendPackage(slaveChannel.relayMode);
                    timers[i - 1] = millis();
                }

                if (channels_state[i - 1] == uint8_t(slaveChannel.relayMode) && millis() - timers[i - 1] >= (slaveChannel.data.timerMode.workMinute * 60000) + (slaveChannel.data.timerMode.workSecond * 1000)) {
                    channels_state[i - 1] = !uint8_t(slaveChannel.relayMode);
                    radio.setPackageType(CONTROL_PACKAGE);
                    radio.sendPackage(!slaveChannel.relayMode);
                    timers[i - 1] = millis();
                }
                 
                break;

            case DAY:
                if (channels_state[i - 1] == !uint8_t(slaveChannel.relayMode)) {
                    if (now.Hour() == slaveChannel.data.dayMode.startHour) {
                        if (now.Minute() == slaveChannel.data.dayMode.startMinute) {
                            if (now.Second() >= slaveChannel.data.dayMode.startSecond) {
                                //digitalWrite(channelsPins[i - 1], currentChannel.relayMode);
                                channels_state[i - 1] = uint8_t(slaveChannel.relayMode);
                                radio.setPackageType(CONTROL_PACKAGE);
                                radio.sendPackage(slaveChannel.relayMode);
                            }
                        }
                    }
                    
                }

                if (channels_state[i - 1] == uint8_t(slaveChannel.relayMode)) {
                    if (now.Hour() == slaveChannel.data.dayMode.endHour) {
                        if (now.Minute() == slaveChannel.data.dayMode.endMinute) {
                            if (now.Second() >= slaveChannel.data.dayMode.endSecond) {
                                //digitalWrite(channelsPins[i - 1], !uint8_t(currentChannel.relayMode));
                                channels_state[i - 1] = !uint8_t(slaveChannel.relayMode);
                                radio.setPackageType(CONTROL_PACKAGE);
                                radio.sendPackage(!slaveChannel.relayMode);
                            }
                        }
                    }
                }
            
                break;

            case SENSOR:
                if (analogRead(channelsPins[i - 1]) >= slaveChannel.data.sensorMode.threshold && channels_state[i - 1] == !uint8_t(slaveChannel.relayMode)) {
                    //digitalWrite(channelsPins[i - 1], uint8_t(currentChannel.relayMode));
                    channels_state[i - 1] = uint8_t(slaveChannel.relayMode);
                    radio.setPackageType(CONTROL_PACKAGE);
                    radio.sendPackage(slaveChannel.relayMode);
                    timers[i - 1] = millis();
                }

                if (millis() - timers[i - 1] > slaveChannel.data.sensorMode.workMinute * 60000 + slaveChannel.data.sensorMode.workSecond * 1000 && channels_state[i - 1] == uint8_t(slaveChannel.relayMode)) {
                    //digitalWrite(channelsPins[i - 1], !uint8_t(currentChannel.relayMode));
                    channels_state[i - 1] = !uint8_t(slaveChannel.relayMode);
                    radio.setPackageType(CONTROL_PACKAGE);
                    radio.sendPackage(!slaveChannel.relayMode);
                }
            

                break;

            case WEEK:
               uint8_t today = now.DayOfWeek();
               if (today >= 1) --today;
               else if (today == 0) today = 6; // sunday
                
               
               if (slaveChannel.data.weekMode.days[today].enabled) {
               
                    if (channels_state[i - 1] == !uint8_t(slaveChannel.relayMode)) {
                        if (now.Hour() == slaveChannel.data.weekMode.days[today].startHour) {
                            if (now.Minute() == slaveChannel.data.weekMode.days[today].startMinute) {
                                if (now.Second() >= slaveChannel.data.weekMode.days[today].startSecond) {
                                    //digitalWrite(channelsPins[i - 1], uint8_t(currentChannel.relayMode));
                                    channels_state[i - 1] = uint8_t(slaveChannel.relayMode);
                                    radio.setPackageType(CONTROL_PACKAGE);
                                    radio.sendPackage(slaveChannel.relayMode);
                                }
                            }
                        }
                    
                    }

                    if (channels_state[i - 1] == uint8_t(slaveChannel.relayMode)) {
                        if (now.Hour() == slaveChannel.data.weekMode.days[today].endHour) {
                            if (now.Minute() == slaveChannel.data.weekMode.days[today].endMinute) {
                                if (now.Second() >= slaveChannel.data.weekMode.days[today].endSecond) {
                                    //digitalWrite(slavePins[i - 1], !uint8_t(slaveChannel.relayMode));
                                    channels_state[i - 1] = !uint8_t(slaveChannel.relayMode);
                                    radio.setPackageType(CONTROL_PACKAGE);
                                    radio.sendPackage(!slaveChannel.relayMode);
                                }
                            }
                        }
                    }
                }

                break;
            }

            ++i;

    } else {
        i = 1;
    }
}
