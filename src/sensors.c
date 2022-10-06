#include "stm32f0xx.h"
#include <stdio.h>
#include "i2c.h"
#include "sensors.h"

#include "uart.h"

//============================================================================
// DEFINITIONS FOR DEVICE ADDRESSES
//  * This just includes the definitions for the device addresses.
//============================================================================
#define ACCELEROMETER_ADDR 	0x69 //Since MPU6050 has the same address as the PCF8523, we have to set the A0 pin on the MPU6050 HIGH so both can be on the same I2C bus
#define PULSEOX_ADDR 		0x57
#define HDC_ADDR 			0x40

//============================================================================
// TEMP_WRITE
//  * Send data to the temp sensor
//  * Used mainly for configuration
//============================================================================
void temp_write(uint8_t reg, uint8_t val) {
	uint8_t temp_writedata[2] = {reg, val};
	i2c_senddata(HDC_ADDR, temp_writedata,2);
}

//============================================================================
// TEMP_SIMPLE_READ
//  * Read a single byte from the temp sensor.
//============================================================================
uint8_t temp_simple_read(uint8_t reg) {
    uint8_t temp_data[1] = {reg};
    i2c_recvdata_noP(HDC_ADDR, temp_data, 1);
    return I2C1->RXDR & 0xff;
}

//============================================================================
// TEMP_READ_ARRAY
//  * Read individual data from the temp sensor.
//  * Result is stored into the data buffer
//============================================================================
void temp_read_array(uint8_t loc, char data[], uint8_t len) {
    uint8_t reg[1] = { loc };
    i2c_recvdata_noP_array(HDC_ADDR,data,len,reg);              //Read data
}

//============================================================================
// PULSEOX_WRITE
//  * Send data to the pulse-oximeter
//  * Used mainly for configuration
//============================================================================
void pulseox_write(uint8_t reg, uint8_t val) {
	uint8_t pulseox_writedata[2] = {reg, val};
	i2c_senddata(PULSEOX_ADDR, pulseox_writedata,2);
}

//============================================================================
// PULSEOX_SIMPLE_READ
//  * Read a single byte from the pulse oximeter.
//============================================================================
uint8_t pulseox_simple_read(uint8_t reg) {
    uint8_t pulseox_data[1] = {reg};
    i2c_recvdata_noP(PULSEOX_ADDR, pulseox_data, 1);
    return I2C1->RXDR & 0xff;
}

//============================================================================
// PULSEOX_READ_ARRAY
//  * Read individual data from the pulse oximeter.
//  * Result is stored into the data buffer
//============================================================================
void pulseox_read_array(uint8_t loc, char data[], uint8_t len) {
    uint8_t reg[1] = { loc };
    i2c_recvdata_noP_array(PULSEOX_ADDR,data,len,reg);              //Read data
}

//============================================================================
// PULSEOX_SETUP
//  * Resets pulse ox
//  * Sets to average 32 samples per FIFO sample, roll-over data
//============================================================================
void pulseox_setup(void) {
	pulseox_write(0x09,0x40); 	//Reset
	pulseox_write(0x08,0x70); 	//Average 8 samples per FIFO sample, roll-over data
	pulseox_write(0x09,0x03); 	//Set to SpO2 mode (RED and IR) -> 2 active LEDs
	pulseox_write(0x0a,0x27); 	//Set to 4096nA range, 100 samples/second, and 411us width
	pulseox_write(0x0c,0x1f);	//Set LEDs to 6.2mA power (about ~4" detection)
	pulseox_write(0x0d,0x1f);

	pulseox_write(0x11,0x21);   //Set Time Slots (Slot 1 -> Red; Slot 2 -> Infrared)

	//Clear FIFO
	pulseox_write(0x04,0x00);
	pulseox_write(0x05,0x00);
	pulseox_write(0x06,0x00);
}

int led_arr[30*5*2];
float r;

//============================================================================
// PULSEOX_CHECK
//  * THEORETICALLY checks data from the FIFO.
//  * Adapted from https://github.com/sparkfun/SparkFun_MAX3010x_Sensor_Library
//    (SPARKFUN's OPEN SOURCE ARDUINO LIBRARY)
//============================================================================
void pulseox_check(void)
{
    //Read register FIDO_DATA in (3-byte * number of active LED) chunks
    //Until FIFO_RD_PTR = FIFO_WR_PTR
    uint8_t readPointer  = pulseox_simple_read(0x06);
    uint8_t writePointer = pulseox_simple_read(0x04);

    int numberOfSamples = 0;

    //Do we have new data?
    //Calculate the number of readings we need to get from sensor
    numberOfSamples = writePointer - readPointer;
    if (numberOfSamples < 0) numberOfSamples += 32; //Wrap condition

    //We now have the number of readings, now calc bytes to read
    //For this example we are just doing Red and IR (3 bytes each)
    //3 bytes per sample, 2 LEDs
    char pulseox_buf[6];
    pulseox_read_array(0x07, pulseox_buf,6);

    int IR = (pulseox_buf[0] << 16) | (pulseox_buf[1] << 8) | (pulseox_buf[2]);
    IR &= 0x3ffff;
    int Rd = (pulseox_buf[3] << 16) | (pulseox_buf[4] << 8) | (pulseox_buf[5]);
    Rd &= 0x3ffff;
    //printf("R%6d  I%6d\n",Rd,IR);

    for(int i = 300; i > 0; i -= 2) {
    	led_arr[i]   = led_arr[i-2];
    	led_arr[i-1] = led_arr[i-3];
    }
    led_arr[0] = Rd;
    led_arr[1] = IR;
}

int red_avg;
int red_min;
//============================================================================
// GET_SPO2
//	* Gives user SpO2 values from a regression curve calibrated to a
//	  commercial pulse ox.
//============================================================================
int get_spo2(void) {
    //Calculate R Value for SpO2
    int ir_min = 16777216;
    int ir_max = 0;
    red_min = 16777216;
    int red_max = 0;
    red_avg = 0;
    //red & ir min and max
    for(int i = 0; i < 300; i++) {
    	if(i%2 == 0){
    		red_avg += led_arr[i];
    		if(led_arr[i] < red_min)
    			red_min = led_arr[i];
    		if(led_arr[i] > red_max)
    			red_max = led_arr[i];
    	}
    	else{
            if(led_arr[i] < ir_min)
                ir_min = led_arr[i];
            if(led_arr[i] > ir_max)
                ir_max = led_arr[i];
    	}
    }
    red_avg /= 150;

    if(ir_min < 1500 || red_min < 1500) {
        //printf("Wrist Not Detected\n");
        return(-1);
    }

    //printf("%d %d %d\n",red_min,red_avg,red_max);

    float r_AC = ((float)(ir_max - ir_min) / (float)(red_max - red_min));
    float r_DC = ((float)red_min / (float)(ir_min));
    r = r_AC * r_DC;
    int spo2 = (float)(0.5493*r + 95.105);
    //printf("%.4f\n",r);
    //printf("%d\n",spo2);
    return(spo2);
    //printf("%.4f\n",spo2);
}

//
int count = 0;
int sampl_hr[7] = {70,70,70,70,70,70,70};
int hr_avg;
//============================================================================
// GET_HR
//	* Gives user HR. Works by looking for a peak (one value surrounded by
//	  several smaller values. Then, takes average of last few samples to
//	  consider noise and bad measurements.
//============================================================================
int get_HR(void) {
	//Check if at a peak
	if(led_arr[8] > led_arr[0] && led_arr[8] > led_arr[2]
	  && led_arr[8] > led_arr[4]
	  && led_arr[8] > led_arr[6]
	  && led_arr[8] > led_arr[14]
	  && led_arr[8] > led_arr[16]
	  && led_arr[8] > red_avg) {

		//Check for realistic pulses
		// [36BPM to 120BPM]
		// Realistically, people are unlikely to have values that surpass
		// these without being in the hospital.
		if(count > 15 && count < 50 && red_min > 1500) {

			//Find average HR for last 7 samples
			hr_avg = 0;
			for(int i = 6; i > 0; i--) {
				hr_avg += sampl_hr[i];
				sampl_hr[i] = sampl_hr[i-1];
			}
			sampl_hr[0] = 1800/count;
			hr_avg += sampl_hr[0];
			//printf("Heart Rate: %d\n",hr_avg/10);
		}
	    //printf("Time: %d\n",count);
   	count = 0;
    }

	//Reset the counter and return the final heartrate
    count++;
	return(hr_avg/7);
}

//============================================================================
// ACCELEROMETER_WRITE
//  * Send data to the accelerometer
//  * Used mainly for configuration
//============================================================================
void accelerometer_write(uint8_t reg, uint8_t val) {
    uint8_t accelerometer_writedata[2] = {reg, val};
    i2c_senddata(ACCELEROMETER_ADDR, accelerometer_writedata,2);
}

//============================================================================
// ACCELEROMETER_READ
//  * Read data from the accelerometer
//============================================================================
uint8_t accelerometer_read(uint8_t reg) {
    uint8_t accelerometer_data[1] = {reg};
    //i2c_senddata(ACCELEROMETER_ADDR, accelerometer_data, 1);
    i2c_recvdata_noP(ACCELEROMETER_ADDR, accelerometer_data, 1);
    return I2C1->RXDR & 0xff;
}

//============================================================================
// INIT_ACCELEROMETER
//  * Configures the accelerometer
//      (1) Reset
//      (2) Set to +-8g sensitivity. This was chosen since the algorithms used
//          by Kim Dongwoo and Hee Chan Kim [Energy Expenditure, IEEE] and by
//          by Malmo University [Step Counter] were using +-6g and +-8g
//          accelerometers, respectively.
//      (3) Set to a 20Hz DLPF. This is because the same algorithms implement
//          a 5Hz [Step Counter, Malmo] and 20Hz [Energy Expenditure, IEEE]
//          LPF for calculations.
//============================================================================
void init_accelerometer(void) {
    accelerometer_write(0x6b,0x10); //RESET EVERYTHING
    accelerometer_write(0x68,0x02); //RESET SENSORS
    accelerometer_write(0x1c,0x10); //+-8g sensitivity
    accelerometer_write(0x1a,0x06); //Set to 20Hz DLPF
}

//============================================================================
// ACCELEROMETER_X
//  * Read X-axis data from the accelerometer
//============================================================================
short accelerometer_X(void) {
    return((accelerometer_read(0x3b) << 8) | accelerometer_read(0x3c));
}

//============================================================================
// ACCELEROMETER_Y
//  * Read Y-axis data from the accelerometer
//  * NOTE: This reads a default of 1g
//============================================================================
short accelerometer_Y(void) {
    return((accelerometer_read(0x3d) << 8) | accelerometer_read(0x3e));
}

//============================================================================
// ACCELEROMETER_Z
//  * Read Z-axis data from the accelerometer
//============================================================================
short accelerometer_Z(void) {
    return((accelerometer_read(0x3f) << 8) | accelerometer_read(0x40));
}
