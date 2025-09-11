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

Mode operator++(Mode& mode) {
    if (uint8_t(mode) < 3) mode = Mode((uint8_t)mode + 1);
    else mode = TIMER;

}

Mode operator--(Mode& mode) {
    if (uint8_t(mode) > 1) mode = Mode((uint8_t)mode - 1);
    else mode = SENSOR; // overflow
}