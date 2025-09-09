#pragma once

// current user state
enum FSM : uint8_t {
 IDLE = 0, // idle state
 MAIN_MENU,
 SERVICE,
 CHANNELS,
 SENSORS,
 // sub states of [SERVICE, CHANNELS, SENSORS]
};


enum Mode : uint8_t {
    OFF = 0, // idle state
    TIMER,
    RTC,
    SENSOR
};