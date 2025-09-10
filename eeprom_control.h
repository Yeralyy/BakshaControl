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
#define CHANNELS_SIZE 72



struct Channel {
    /*Modes types: Timer, RTC, Sensor, */
    uint32_t timer {}; // in milliseconds 4 bytes
    uint8_t month {}; // 1 byte
    uint8_t day {}; // 1 byte
    uint8_t hour {}; // 1 byte
    uint8_t minute {}; // 1 byte
    Mode mode {OFF}; // default.  1 byte
    /*size 9 bytes*/
};

struct Channels {
    Channel channel[CHANNELS_COUNT] {}; // 9 * 8 =  72 bytes
};


void initEEPROM(void);
void factoryReset(void);
void updateChannels(Channels& channels);
Channel getChannel(int8_t n);
void putChannel(int8_t n, Channel& channel);
bool isFirstRun(void);




bool isFirstRun(void) {
    return ((EEPROM.read(EEPROM_KEY_ADDRESS) == EEPROM_KEY) ? 0 : 1); // first run?
}

void initEEPROM(void) {
    Channels channels;

    #if LOG
    Serial.print("Channels struct size: ");
    Serial.print(sizeof(Channels));

    Serial.println();
    Serial.print("Channel struct size: ");
    Serial.print(sizeof(Channel));
    #endif

    EEPROM.put(CHANNELS_ADDRESS, channels);
    #if LOG
    Serial.println("EEPROM initilized");
    #endif

    EEPROM.update(EEPROM_KEY_ADDRESS, EEPROM_KEY);
}

void factoryReset(void) {
    Channels channels;

    EEPROM.put(CHANNELS_ADDRESS, channels); // default values

    #if LOG
    Serial.println("EEPROM Factory reset");
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