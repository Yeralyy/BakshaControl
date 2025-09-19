#pragma once
#include <Arduino.h>
#include <EEPROM.h>
#include "states.h"
#include "CONFIG.h"

#define EEPROM_KEY_ADDRESS 1023
#define EEPROM_KEY 22

#define CHANNELS_COUNT 8 
#define SERVOS 4 // later

#define CHANNELS_ADDRESS 0
#define CHANNELS_SIZE 104


struct Channel {
    union {
        // you can notice that all timer variables is int32_t and not uint32_t. So in future if you want to compare these variables with milli() which is uin32_t you should explicitly convert in32_t to uint32_t, undefined behavier otherwise
        struct {
            uint32_t timer;

            uint8_t periodHour;
            uint8_t periodMinute;
            uint8_t periodSecond;

            uint8_t workMinute;
            uint8_t workSecond;
            // 5 + 4 = 9 bytes
        } timerMode;

        struct {
            int32_t timer;
            int threshold;

            uint8_t workMinute;
            uint8_t workSecond;
            // 4 + 2 + 2 = 8bytes
        } sensorMode;

        struct {
            uint32_t timer;
            // start
            uint8_t startHour;
            uint8_t startMinute;
            uint8_t startSecond;

            // end
            uint8_t endHour;
            uint8_t endMinute;
            uint8_t endSecond;

            // 4 + 1 + 1 + 1 + 1 + 1 + 1 = 10 bytes

        } dayMode;

        struct {
            // empty for now
            uint32_t timer;
        } rtcMode;
    } data; // 10 bytes

    Mode mode {OFF}; // 1 byte
}; // 11 bytes


struct Channels {
    Channel channel[CHANNELS_COUNT]; // 11 * 8 =   88 bytes 
};


void initEEPROM(void);
void factoryReset(void);
void updateChannels(Channels& channels);
Channel getChannel(int8_t n);
void putChannel(int8_t n, Channel& channel);
bool isFirstRun(void);
void resetEEPROM(void);




bool isFirstRun(void) {
    #if LOG
    Serial.print(F("EP key: "));
    Serial.print(EEPROM.read(EEPROM_KEY_ADDRESS));
    #endif
    return ((EEPROM.read(EEPROM_KEY_ADDRESS) == EEPROM_KEY) ? 0 : 1); // first run?
}

void initEEPROM(void) {
    Channels channels;

    #if LOG
    Serial.println(F("Chls size: "));
    Serial.print(sizeof(Channels));

    Serial.println(F("Channel struct size: "));
    Serial.print(sizeof(Channel));
    #endif

    for (int i; i < 8; ++i) {
        putChannel(i, channels.channel[i]);
        delay(10);
    }

    #if LOG
    Serial.println(F("EEPROM initilized"));
    #endif

    EEPROM.update(EEPROM_KEY_ADDRESS, EEPROM_KEY);
}

void factoryReset(void) {
    for (int8_t i = 0; i < 8; i++) {
        Channel channel {};
        putChannel(i, channel);
    }

    #if LOG
    Serial.println(F("EEPROM Factory reset"));
    #endif
}

void updateChannels(Channels& channels) {
    EEPROM.put(CHANNELS_ADDRESS, channels);
}

Channel getChannel(int8_t n) { // return get N'th struct in Channels
    if (n >= 0 && n <= CHANNELS_COUNT) {
        Channel channel;
        EEPROM.get(CHANNELS_ADDRESS + (n-1) * sizeof(Channel), channel);
        return channel;
    } 
}

void putChannel(int8_t n, Channel& channel) { // put N'th struct in Channels
    if (n >= 0 && n <= CHANNELS_COUNT) EEPROM.put(CHANNELS_ADDRESS + (n - 1) * sizeof(Channel), channel);
}

void resetEEPROM(void) {
    #if LOG
    Serial.println(F("Factory reset of EEPROM"));
    #endif
    for (int i; i < 1024; ++i) {
        EEPROM.update(i, 255);
        delay(10);
    }
}