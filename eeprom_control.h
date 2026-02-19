#pragma once
#include <Arduino.h>
#include <EEPROM.h>
#include "states.h"
#include "CONFIG.h"

#if nRF
#include "nrf.h"
#endif

#define EEPROM_KEY_ADDRESS 1023
#define EEPROM_KEY 69 

#define CHANNEL_POINTER_ADRESS 974
#define CHANNELS_ADDRESS 0

uint16_t CHANNEL_POINTER = 0; 
int8_t channels_count;


/*
EEPROM MAP

Channels_addres start 0
Each channel is 54bytes long

1023 / 54 = 18 possible channels

0 - 972 for channels

973 - 1022 free

1023 key address


*/


const uint8_t sensorsPins[4] {16, 17, 20, 21};
const uint8_t channelsPins[7] {0, 1, 9, 10, 13, 14, 15}; // A6 - 20
uint32_t timers[7] {0, 0, 0, 0, 0, 0, 0};

struct Day {
    // start
    uint8_t startHour;
    uint8_t startMinute;
    uint8_t startSecond;

    // end
    uint8_t endHour;
    uint8_t endMinute;
    uint8_t endSecond;

    bool enabled;

}; // 7 bytes

struct Channel {
    union {
        struct {
            uint8_t periodHour;
            uint8_t periodMinute;
            uint8_t periodSecond;

            uint8_t workMinute;
            uint8_t workSecond;
            //  5 bytes
        } timerMode;

        struct {
            int threshold;

            uint8_t workMinute;
            uint8_t workSecond;
            uint8_t pin;

            // 4bytes
        } sensorMode;

        struct {
            // start
            uint8_t startHour;
            uint8_t startMinute;
            uint8_t startSecond;

            // end
            uint8_t endHour;
            uint8_t endMinute;
            uint8_t endSecond;

            // 6 bytes

        } dayMode;

        struct {
            Day days[7];
            /*
            Day monday; // 7 bytes
            Day tuesday; 
            Day wednesday;
            Day thursday;
            Day friday;

            Day saturday;
            Day sunday;
            */


        } weekMode; // 49

        struct {

            float Kp; // P 
            float Ki; // I
            float Kd; // D

            int setPoint; // system set point

            uint8_t pin; // analog pin/feedback pin
        } PidMode; // 15 bytes

    } data; // 49 bytes

    uint32_t deviceID;
    Mode mode {OFF}; // 1 byte
    RelayMode relayMode {TO_ON};
    
}; // 54  bytes

void initEEPROM(FSM& state);
Channel getChannel(int8_t n);
void putChannel(int8_t n, Channel& channel);
  void putChannel(int8_t n, Node& node);
bool isFirstRun(void);
void resetEEPROM(void);

#if nRF
int8_t channelsCount() {
    CHANNEL_POINTER = EEPROM.read(CHANNEL_POINTER_ADRESS);
    channels_count = int8_t(CHANNEL_POINTER / sizeof(Channel));
    return channels_count;
}
#endif


bool isFirstRun(void) {
    #if LOG
    Serial.print(F("EP key: "));
    Serial.println(EEPROM.read(EEPROM_KEY_ADDRESS));
    #endif
    return ((EEPROM.read(EEPROM_KEY_ADDRESS) == EEPROM_KEY) ? 0 : 1); // first run?
}

void initEEPROM(FSM& state) {
    if (isFirstRun()) {
        EEPROM.update(EEPROM_KEY_ADDRESS, EEPROM_KEY);
        EEPROM.update(CHANNEL_POINTER_ADRESS, CHANNEL_POINTER);
        state = FIRST_RUN;
    } else 
        state = MAIN_MENU;

    channelsCount();

    #if LOG
    Serial.println(F("EEPROM initilized"));

    Serial.print("Channel pointer: ");
    Serial.println(CHANNEL_POINTER);
    Serial.print("Channel count: ");
    Serial.println(channels_count);

    #endif

}


Channel getChannel(int8_t n) { // return get N'th struct in Channels
    if (n > 0 && n <= channels_count) {
        Channel channel;
        EEPROM.get(CHANNELS_ADDRESS + (n-1) * sizeof(Channel), channel);
        return channel;
    } 
}

void putChannel(int8_t n, Channel& channel) { // put N'th struct in Channels
    if (n > 0 && n <= channels_count) EEPROM.put(CHANNELS_ADDRESS + (n - 1) * sizeof(Channel), channel);
}

void putChannel(Node& node) {
    Channel new_channel {};
    //EEPROM.put(CHANNELS_ADDRESS + (channels_count - 1) * sizeof(Channel), new_channel);
    EEPROM.put(CHANNEL_POINTER, new_channel);
    channels_count++;
    CHANNEL_POINTER += sizeof(Channel);
    EEPROM.update(CHANNEL_POINTER_ADRESS, CHANNEL_POINTER);
}


void resetEEPROM(void) {
    #if LOG
    Serial.println(F("Factory reset of EEPROM"));
    #endif
    for (int i = 0; i < 1024 ; ++i) {
        EEPROM.update(i, 255);
        delay(10);
    }
}
