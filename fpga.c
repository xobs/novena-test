#include <stdio.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#define DIG_SCAN 0x45

#define ADC_SAMPLE 0x46
#define ADC_VAL 0x81


static int i2c_fd;
static char *i2c_device = "/dev/i2c-0";
static uint8_t fpga_addr = 0x1e;

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

uint32_t read_adc(uint32_t channel) {
        uint16_t result;

        set_fpga(ADC_SAMPLE, channel);
        set_fpga(ADC_SAMPLE, channel | 0x10);
        set_fpga(ADC_SAMPLE, channel);
        read_fpga(ADC_VAL, &result, sizeof(result));
        return result;
}
