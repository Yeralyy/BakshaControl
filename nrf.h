#pragma once

#include "CONFIG.h"
#include <Arduino.h>
#include <SPI.h>
#include <RF24.h>

#define CE_PIN 9
#define CSN_PIN 10

RF24 radio(CE_PIN, CSN_PIN);

uint8_t address[][6] = {"1Node", "2Node", "3Node", "4Node", "5Node"}; // pipe 1 will be for pairing

bool radioNumber = 0;

bool role = true;

typedef enum {
    SLAVE_REQUEST = 0, // salve - master
    PAIRING_REQUEST,  // master - slave
    CONTROL_PACKAGE, // master - slave
    STATUS_REQUEST, // master - slave, slave - master
} PACKAGE_TYPE; 

struct Package {
    uint32_t deviceID;
    bool state;
    PACKAGE_TYPE type;
};

Package packet;

void radioInit() {
    if (!radio.begin()) {
        while (1) {}
    }

    radio.setPALevel(RF24_PA_LOW);
    radio.setPayloadSize(sizeof(packet));
    radio.stopListening(address[radioNumber]);
    radio.openReadingPipe(1, address[!radioNumber]);

    if (!role) {
        radio.startListening();
    }
}

void sendPackage(bool state) {
    packet.state = state;

    #if LOG
    Serial.print("State: ");
    Serial.print(state);
    Serial.println();
    #endif

   unsigned long start_timer = micros();
   bool report = radio.write(&packet, sizeof(packet));
   unsigned long end_timer = micros();


   if (report) {
    #if LOG
    Serial.print(F("Transmission successful! "));  // payload was delivered
    Serial.print(F("Time to transmit = "));
    Serial.print(end_timer - start_timer);  // print the timer result
    Serial.println(F(" us. Sent: "));
    #endif
   } else {
    #if LOG 
    Serial.println(F("Transmission failed or timed out"));  // payload was not delivered
    #endif
   }
}