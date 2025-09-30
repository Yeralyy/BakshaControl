#pragma once
#include <Arduino.h>
#include <EEPROM.h>
#include "states.h"
#include "CONFIG.h"

#define EEPROM_KEY_ADDRESS 1023
#define EEPROM_KEY 22

#define CHANNELS_COUNT 4 
#define SERVOS 4 // later

#define CHANNELS_ADDRESS 0
#define CHANNELS_SIZE 104


const uint8_t channelsPins[CHANNELS_COUNT] {8, 9, 10, 11}; // A6 - 20
uint32_t timers[CHANNELS_COUNT] {0, 0, 0, 0};

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

    Mode mode {OFF}; // 1 byte
}; // 50  bytes


struct Channels {
    Channel channel[CHANNELS_COUNT]; // 4 * 50 = 200 bytes 
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

    EEPROM.put(0, channels);
    delay(40);

    #if LOG
    Serial.println(F("EEPROM initilized"));
    #endif

    EEPROM.update(EEPROM_KEY_ADDRESS, EEPROM_KEY);
}

void factoryReset(void) {
    for (int8_t i = 0; i < CHANNELS_COUNT; i++) {
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
        EEPROM.get(CHANNELS_ADDRESS + (n-1) * sizeof(Channel), channel);
        return channel;
    } 
}

void putChannel(int8_t n, Channel& channel) { // put N'th struct in Channels
    if (n > 0 && n <= CHANNELS_COUNT) EEPROM.put(CHANNELS_ADDRESS + (n - 1) * sizeof(Channel), channel);
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