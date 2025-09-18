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


/*
struct Channel {
    
    uint32_t timer {}; // in milliseconds 4 bytes
    uint32_t work {}; // work time in milliseconds 4 bytes
    int threshold {1023}; // 2 bytes
    uint8_t month {}; // 1 byte
    uint8_t day {}; // 1 byte
    uint8_t hour {}; // 1 byte
    uint8_t minute {}; // 1 byte
    Mode mode {OFF}; // default.  1 byte
    
    
};
*/

struct Channel {
    union {
        // you can notice that all timer variables is in32_t and not uint32_t. So in future if you want to compare these variables with milli() which is uin32_t you should explicitly convert in32_t to uint32_t, undefined behavier otherwise
        struct {
            int32_t period; 
            int32_t timer;
            int32_t work;
            // 4 + 4 + 4 = 12 bytes
        } timerMode;

        struct {
            int32_t work;
            int threshold;
            // 4 + 2 = 6 bytes
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
    } data; // 12 bytes

    Mode mode {OFF}; // 1 byte
}; // 13 bytes


struct Channels {
    Channel channel[CHANNELS_COUNT]; // 13 * 8 =   104 bytes 
};


void initEEPROM(void);
void factoryReset(void);
void updateChannels(Channels& channels);
Channel getChannel(int8_t n);
void putChannel(int8_t n, Channel& channel);
bool isFirstRun(void);




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
    Serial.print(F("Chls size: "));
    Serial.print(sizeof(Channels));

    Serial.println();
    Serial.print(F("Channel struct size: "));
    Serial.print(sizeof(Channel));
    #endif

    EEPROM.put(CHANNELS_ADDRESS, channels);
    #if LOG
    Serial.println(F("EEPROM initilized"));
    #endif

    EEPROM.update(EEPROM_KEY_ADDRESS, EEPROM_KEY);
}

void factoryReset(void) {
    for (int8_t i = 1; i <= 8; i++) {
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
    if (n > 0 && n <= CHANNELS_COUNT) {
        Channel channel;
        EEPROM.get(CHANNELS_ADDRESS + n * sizeof(Channel), channel);
        return channel;
    } 
}

void putChannel(int8_t n, Channel& channel) { // put N'th struct in Channels
    if (n > 0 && n <= CHANNELS_COUNT) EEPROM.put(CHANNELS_ADDRESS + n * sizeof(Channel), channel);
}
