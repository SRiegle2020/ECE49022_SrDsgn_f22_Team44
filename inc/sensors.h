#include "stm32f0xx.h"
#include <stdio.h>
#include "i2c.h"

void accelerometer_write(uint8_t reg, uint8_t val);
uint8_t accelerometer_read(uint8_t reg);
