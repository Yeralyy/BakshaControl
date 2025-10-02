#pragma once
#include "eeprom_control.h"

void PIDtick();
int computePID(float input, float setPoint, float kp, float ki, float kd, float dt);

void PIDtick() {
    for (uint8_t i = 1; i < CHANNELS_COUNT; ++i) {
        Channel currentChannel = getChannel(i);
        if (currentChannel.mode == PID) {
            analogWrite(channelsPins[i - 1], computePID(analogRead(currentChannel.data.PidMode.pin), float(currentChannel.data.PidMode.setPoint), currentChannel.data.PidMode.Kp, currentChannel.data.PidMode.Ki, currentChannel.data.PidMode.Kd, 100));
        } else continue;
    }
}


int computePID(float input, float setPoint, float kp, float ki, float kd, float dt) {
    float err = setPoint - input;
    static float integral = 0;
    static float previousError = 0;
    integral += err * dt; // I
    float D = (err - previousError) / dt;
    previousError = err;
    return (err * kp + integral * ki + D * kd);
}