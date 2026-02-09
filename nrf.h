#pragma once

#include "CONFIG.h"
#include <Arduino.h>
#include <SPI.h>
#include <RF24.h>

#define CE_PIN 9
#define CSN_PIN 10

typedef enum : uint8_t {
    SLAVE_REQUEST = 0, // salve - master
    PAIRING_REQUEST,  // master - slave
    CONTROL_PACKAGE, // master - slave
    STATUS_REQUEST, // master - slave, slave - master
} PACKAGE_TYPE; 

typedef enum : uint8_t {
    SLAVE = 0,
    MASTER,
} NodeType;

struct Package {
    uint32_t deviceID;
    bool state;
    PACKAGE_TYPE type;
};

struct Node {
    uint32_t deviceId;
    NodeType deviceType;
};

class Radio : public RF24 {
    public:
        Radio(int ce_pin, int csn_pin) : RF24(ce_pin, csn_pin) {
        }

        void radioInit(uint32_t deviceid, bool first_init = 0);
        bool sendPackage();
        bool readPipe();

        uint16_t scanRadio(uint16_t timeout = 1000);
        void scanDelete();

        Node& operator [](int idx) {
            return scanResult[idx];
        }
        
    private:

        //void _rxMxode(uint8_t readingPipe);
        //void _txMode(uint8_t readingPipe);

        const uint8_t address[5][6] = {"1Node", "2Node", "3Node", "4Node", "5Node"}; // pipe 1 will be for pairing
        const uint8_t pair_pipe = 0;
        const uint8_t master_tx = 1;
        const uint8_t slave_tx = 2;

        #if ROLE
        const bool master = true;
        #else
        const bool master = false;
        #endif

        typedef enum : uint8_t {
        IDLE = 0,
        PAIR
        } RADIO_STATE;

        Package send_packet;
        Package recieve_packet;

        Node* scanResult = nullptr; // array pointer

        uint32_t radio_tmr = 0;
    
};


/*
void Radio::_rxMode(uint8_t readingPipe) {
    if (master) {
        this->openWritingPipe(master_tx);
        this->openReadingPipe(1, address[readingPipe]);
        this->startListening();
    } else {
        this->openWritingPipe(address[readingPipe]);
        this->openReadingPip
    }
}

void radio::_txMode(uint8_t readingPipe) {
    
}
*/

void Radio::radioInit(uint32_t deviceId, bool first_init = 0) {
    if (!this->begin()) {
        while (1) {}
    }

    send_packet.deviceID = deviceId;

    this->setPALevel(RF24_PA_LOW);
    this->setPayloadSize(sizeof(send_packet));

    if (first_init) { // in first run slave will send his id and master will be listening one
        if (master) {
            this->openWritingPipe(master_tx);
            this->openReadingPipe(1, address[pair_pipe]);
            this->startListening();
        } else {
            this->openWritingPipe(address[pair_pipe]);
            this->openReadingPipe(1, address[master_tx]);
            this->stopListening();
        }
    } else {
        if (master) { // if its not pairing mode, master will tx, slave will rx
            this->openWritingPipe(address[master_tx]); 
            this->openReadingPipe(1, address[slave_tx]);
            this->stopListening();
        } else {
            this->openWritingPipe(address[slave_tx]);
            this->openReadingPipe(1, address[master_tx]);
            this->startListening();
        }
    }
}

uint16_t Radio::scanRadio(uint16_t timeout) {
    if (master) {
        radio_tmr = millis();
        uint16_t count = 0;
        uint16_t buff_size = 2;
        scanDelete();
        scanResult = new Node[buff_size];
        while (millis() - radio_tmr < timeout) {
            if (readPipe() && recieve_packet.type == SLAVE_REQUEST) {
                count++;
                Node slave {recieve_packet.deviceID, SLAVE};
                if (count > buff_size) {
                    buff_size = buff_size * 2;
                    Node* newScanResult = new Node[buff_size]; // double the size of array
                    memcpy(newScanResult, scanResult, size_t(count * sizeof(Node))); // copy elements to new array
                    scanDelete();
                    scanResult = newScanResult; // assign new to the main pointer
                }

                scanResult[count - 1] = slave;
            }
        }

        return count;

    } else return 0;
    
}

void Radio::scanDelete() {
    delete[] scanResult;
    scanResult = nullptr;
}

bool Radio::sendPackage() {
   unsigned long start_timer = micros(); // timeout count
   bool report = this->write(&send_packet, sizeof(send_packet));
   unsigned long end_timer = micros();


   if (report) {
    #if LOG
    Serial.print(F("Transmission successful! "));  // payload was delivered
    Serial.print(F("Time to transmit = "));
    Serial.print(end_timer - start_timer);  // print the timer result
    Serial.println(F(" us. Sent: "));
    #endif
    return true;

   } else {
    #if LOG 
    Serial.println(F("Transmission failed or timed out"));  // payload was not delivered
    #endif
    return false;
   }
}

bool Radio::readPipe() {
    uint8_t pipe;
    if (this->available(&pipe)) {
        uint8_t bytes = this->getPayloadSize();
        this->read(&recieve_packet, bytes);
        return true;
    } else return false;
}