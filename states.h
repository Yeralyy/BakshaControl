#pragma once

// current user state
enum FSM : uint8_t {
 IDLE = 0, // idle state
 MAIN_MENU,
 SERVICE,
 CHANNELS,
 SENSORS,
 MODES,
};


enum Mode : uint8_t {
    OFF = 0, // idle state
    TIMER,
    RTC,
    DAY,  // every day
    SENSOR,
    // SCHEDUEL 
    // WEEK
    // in future
};

/*
Mode operator++(Mode& mode) {
    if (static_cast<uint8_t>(mode) < 4) mode = Mode(static_cast<uint8_t>(mode) + 1);
    else mode = TIMER;
}

Mode operator--(Mode& mode) {
    if (static_cast<uint8_t>(mode) > 1) mode = Mode(static_cast<uint8_t>(mode) - 1);
    else mode = SENSOR; // overflow
}
    */

inline Mode operator++(Mode& mode) {
    if (mode < SENSOR) mode = static_cast<Mode>(static_cast<uint8_t>(mode) + 1);
    else mode = TIMER;
    
    return mode;
}

inline Mode operator--(Mode& mode) {
    if (mode > TIMER) mode = static_cast<Mode>(static_cast<uint8_t>(mode) - 1);
    else mode = SENSOR;

    return mode;
}