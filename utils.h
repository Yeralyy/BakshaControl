#pragma once
#include <LiquidCrystal_I2C.h>
#include "lib/encMinim.h"

void print2digits(uint8_t val, LiquidCrystal_I2C& lcd, uint8_t col, uint8_t row) { 
	if (val > 9) {
		lcd.setCursor(col, row);
		lcd.print(val);
	} else {
		lcd.setCursor(col, row);
		lcd.print('0');
		lcd.print(val);
	}
}

void printFloat(float val, LiquidCrystal_I2C& lcd, uint8_t col, uint8_t row) {
	lcd.setCursor(col, row);
	if (val > 9) {
		lcd.print(val);
	} else {
		lcd.print('0');
		lcd.print(val);
	}
}


template <typename T, typename K>
void constraining(T& x, K a, K b) {
	if (x < a) x = a;
	if (x > b) x = b;
}

template<typename T>
void pinConstrain(T& pin, bool dir = 1) {
	if (pin > 17 && pin < 20) {
		if (dir) pin = 20;
		else pin = 17;
	} else constraining(pin, 16, 21);
}

template <typename T, typename K>
void settingsHandle(encMinim& enc, T& data, K minValue, K maxValue, K fastValue, bool dir = 1) {
	if (dir) {
		if (enc.isFast() && data + fastValue < maxValue) data += fastValue;
		else ++data;
	} else {
		if (enc.isFast() && data - fastValue > minValue) data -= fastValue;
		else if (data > minValue) --data;
	}

	constraining(data, minValue, maxValue);
}