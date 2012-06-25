#include <stdio.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "gpio.h"


#define DIG_SCAN 0x45

#define ADC_SAMPLE 0x46
#define ADC_VAL 0x81

#define SERIAL_BYTE_0 0x38

/* This GPIO needs to go high to measure battery voltage */
#define ADC8_GPIO 79


static int i2c_fd;
static char *i2c_device = "/dev/i2c-0";
static uint8_t fpga_addr = 0x1e;


#ifdef linux
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
int write_fpga(uint8_t start_reg, void *buffer, uint32_t bytes)
{
	uint8_t data[bytes+1];
	struct i2c_rdwr_ioctl_data packets;
	struct i2c_msg messages[1];

	if(!i2c_fd) {
		if ((i2c_fd = open(i2c_device, O_RDWR))==-1) {
			perror("Unable to open fpga device");
			i2c_fd = 0;
			return 1;
		}
	}

	// Set the address we'll read to the start address.
	data[0] = start_reg;
	memcpy(data+1, buffer, bytes);

	messages[0].addr = fpga_addr;
	messages[0].flags = 0;
	messages[0].len = bytes+1;
	messages[0].buf = data;

	packets.msgs = messages;
	packets.nmsgs = 1;

	if(ioctl(i2c_fd, I2C_RDWR, &packets) < 0) {
		perror("Unable to communicate with i2c device");
		return 1;
	}

	return 0;
}

int read_fpga(uint8_t start_reg, void *buffer, int bytes)
{
	struct i2c_rdwr_ioctl_data packets;
	struct i2c_msg messages[2];

	bzero(buffer, bytes);
	if (!i2c_fd) {
		if ((i2c_fd = open(i2c_device, O_RDWR))==-1) {
			perror("Unable to open fpga device");
			i2c_fd = 0;
			return 1;
		}
	}

	messages[0].addr = fpga_addr;
	messages[0].flags = 0;
	messages[0].len = sizeof(start_reg);
	messages[0].buf = &start_reg;

	messages[1].addr = fpga_addr;
	messages[1].flags = I2C_M_RD;
	messages[1].len = bytes;
	messages[1].buf = (void *)buffer;

	packets.msgs = messages;
	packets.nmsgs = 2;

	if(ioctl(i2c_fd, I2C_RDWR, &packets) < 0) {
		perror("Unable to communicate with i2c device");
		return 1;
	}

	return 0;
}

#else

int write_fpga(uint8_t start_reg, void *buffer, uint32_t bytes) {
    return 0;
}

int read_fpga(uint8_t start_reg, void *buffer, int bytes) {
    bzero(buffer, bytes);
    return 0;
}

#endif //linux

int set_fpga(uint8_t reg, uint8_t val) {
	return write_fpga(reg, &val, sizeof(val));
}

uint8_t get_fpga(uint8_t reg) {
	uint8_t val;
	read_fpga(reg, &val, sizeof(val));
	return val;
}

int sync_fpga(void) {
	int t, i;

	/* Refresh the scan chain twice.  Once to write out the new values,
	and a second time to read them back. */
        if(set_fpga(DIG_SCAN, 0))
		return -1;
        if(set_fpga(DIG_SCAN, 2))
		return -1;
        if(set_fpga(DIG_SCAN, 0))
		return -1;
        if(set_fpga(DIG_SCAN, 1))
		return -1;

        if(set_fpga(DIG_SCAN, 0))
		return -1;
        if(set_fpga(DIG_SCAN, 2))
		return -1;
        if(set_fpga(DIG_SCAN, 0))
		return -1;
        if(set_fpga(DIG_SCAN, 1))
		return -1;
        if(set_fpga(DIG_SCAN, 0))
		return -1;

	/* Wait for the FPGA to go ready */
	for (t=0, i=0; t!=2 && i<100; t=get_fpga(0x80), i++)
		usleep(1000);

	if (t != 2)
		return -1;
	return 0;
}



#define PWM_PERIOD_LO 0x4d
#define PWM_PERIOD_MD 0x4e
#define PWM_PERIOD_HI 0x4f

#define TIMEDIV (1.0 / 13000000) // 13 MHz clock
#define PWM_PERIOD_RAW 0.02F
#define PWM_PERIOD (PWM_PERIOD_RAW / TIMEDIV)
static void set_pwm_period(uint32_t period) {
        uint8_t *val = (uint8_t *)&period;
	write_fpga(PWM_PERIOD_LO, val, 3);
        return;
}


static uint32_t read_adc_internal(uint32_t channel, uint8_t is_battery) {
        uint16_t result;
	int t, i;

	/* For some reason this has to be set for ADC to work */
	set_pwm_period(PWM_PERIOD);

	if (channel == 8) {
		gpio_export(ADC8_GPIO);
		gpio_set_direction(ADC8_GPIO, 1);
		gpio_set_value(ADC8_GPIO, is_battery);
	}
        set_fpga(ADC_SAMPLE, channel);
        set_fpga(ADC_SAMPLE, channel | 0x10);
        set_fpga(ADC_SAMPLE, channel);

	/* Wait for ADC to go ready */
	for (t=0, i=0; !(t&1) && i<100; t=get_fpga(0x83), i++)
		usleep(1000);

        read_fpga(ADC_VAL, &result, sizeof(result));
        return result;
}

uint32_t read_battery(void) {
	read_adc_internal(8, 1);
	return (read_adc_internal(8, 1))*4.086*3.3/1024*1000;
}

uint32_t read_adc(uint32_t channel) {
	read_adc_internal(channel, 0);
	return read_adc_internal(channel, 0);
}


uint32_t read_fpga_serial(uint8_t serial[7]) {
	return read_fpga(SERIAL_BYTE_0, serial, 7);
}
