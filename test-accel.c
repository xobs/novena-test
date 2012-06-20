#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/ioctl.h>
#include "harness.h"


#define ACCEL_ADDR 0x1d
#define FAILURE_COUNT 10

#define THR_STOPPED 0
#define THR_RUNNING 1
#define THR_ERROR -1
#define THR_SUCCESS 2

static pthread_t i2c_thread;
static char i2c_return_message[256];
static int i2c_return_code;
static int should_quit;
static int is_running = THR_STOPPED;


#ifdef linux
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
static int i2c_get(uint8_t i2c_addr, uint8_t reg_addr, char *bfr, int sz) {
	struct i2c_rdwr_ioctl_data packets;
	struct i2c_msg messages[2];

	static int fd = 0;

	if (!fd) {
		fd = open("/dev/i2c-1", O_RDWR);
		if (-1 == fd)
			return 1;
	}



	messages[0].addr = i2c_addr;
	messages[0].flags = 0;
	messages[0].len = sizeof(reg_addr);
	messages[0].buf = &reg_addr;

	messages[1].addr = i2c_addr;
	messages[1].flags = I2C_M_RD;
	messages[1].len = sz;
	messages[1].buf = (void *)bfr;

	packets.msgs = messages;
	packets.nmsgs = 2;
	
	return ioctl(fd, I2C_RDWR, &packets) < 0;
}
#else
static int i2c_get(uint8_t i2c_addr, uint8_t reg_addr, char *bfr, int sz) {
	return 0;
}
#endif //linux


static void *i2c_background(void *_ignored) {
	int failures_in_a_row = 0;

	is_running = THR_RUNNING;

	while (!should_quit) {
		char bfr[8];

		if (i2c_get(ACCEL_ADDR, 0, bfr, sizeof(bfr))) {
			/* Other things are using the accelerometer,
			 * so allow a few failures to happen.
			 */
			if (failures_in_a_row < FAILURE_COUNT) {
				failures_in_a_row++;
				continue;
			}
			i2c_return_code = 1;
			snprintf(i2c_return_message, sizeof(i2c_return_message)-1,
				"I2C failed %d times: %s", failures_in_a_row, strerror(errno));
			is_running = THR_ERROR;
			pthread_exit(NULL);
		}
		failures_in_a_row = 0;
	}

	is_running = THR_SUCCESS;
	return NULL;
}

int test_accel_start(void) {

	i2c_return_code = 0;
	strcpy(i2c_return_message, "I2C stable");
	should_quit = 0;

	if (0 != pthread_create(&i2c_thread, NULL, i2c_background, NULL)) {
		harness_error(0, "Unable to create I2C thread");
		return 1;
	}
	harness_info(0, "Created I2C background thread");
	return 0;
}


int test_accel_finish(void) {
	char bfr[3];
	uint8_t addr;

	harness_info(0, "Checking the status of background thread...");
	if (is_running == THR_ERROR) {
		harness_error(0, "I2C failed at some point: %s",
			i2c_return_message);
		return 1;
	}
	should_quit = 1;


	harness_info(1, "Querying accelerometer for address register...");
	addr = 0x0d;
	if (i2c_get(ACCEL_ADDR, addr, bfr, sizeof(bfr))) {
		harness_error(1, "I2C failed to get register 0x1d: %s", strerror(errno));
		return 1;
	}


	if (bfr[0] != 0x1d) {
		harness_error(2, "Accelerometer address register incorrect.  Expected 0x1d, got 0x%02x", bfr[0]);
		return 1;
	}


	return 0;
}
