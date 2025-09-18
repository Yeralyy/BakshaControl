#pragma once
#include <LiquidCrystal_I2C.h>

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





