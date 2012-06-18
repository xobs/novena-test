#include <stdio.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <stdint.h>
#include <unistd.h>
#include "fpga.h"
#include "harness.h"

#define DIG_OV 0x40
#define DIG_OE 0x41
#define DIG_PU 0x42

#define DIG_VALS 0x84

int test_io(void) {
	uint8_t val;

	set_fpga(DIG_PU, 0);
	set_fpga(DIG_OV, 0);
	set_fpga(DIG_OE, 0);
	sync_fpga();

	/*
	 * Set pins 0, 1, 2, and 3 as outputs
	 * Set pins 0, 1, 2, and 3 as low
	 * Set pins 4, 5, 6, and 7 as inputs
	 * Update scan chain
	 * Read input values
	 *
	 * Set pins 0, 1, 2, and 3 as high
	 * Update scan chain
	 * Read input values
	 */

	/* Test the lower four bits */
	set_fpga(DIG_OE, 0x0f);


	set_fpga(DIG_OV, 0);
	sync_fpga();
	val = get_fpga(DIG_VALS);
	if (val != 0x00)
		harness_error(1,
			"Lower-nybble test - Expected 0x00, got 0x%02x",
			val);
	else
		harness_info(1, "Lower-nybble test correct: 0x00");



	set_fpga(DIG_OV, 0x0f);
	sync_fpga();
	val = get_fpga(DIG_VALS);
	if (val != 0xff)
		harness_error(2,
			"Lower-nybble test - Expected 0xff, got 0x%02x",
			val);
	else
		harness_info(2, "Lower-nybble test correct: 0xff");





	/* Test the upper four bits */
	set_fpga(DIG_OE, 0xf0);
	

	set_fpga(DIG_OV, 0);
	sync_fpga();
	val = get_fpga(DIG_VALS);
	if (val != 0x00)
		harness_error(3,
			"Upper-nybble test - Expected 0x00, got 0x%02x",
			val);
	else
		harness_info(3, "Upper-nybble test correct: 0x00");



	set_fpga(DIG_OV, 0xf0);
	sync_fpga();
	val = get_fpga(DIG_VALS);
	if (val != 0xff)
		harness_error(4,
			"Upper-nybble test - Expected 0xff, got 0x%02x",
			val);
	else
		harness_info(4, "Upper-nybble test correct: 0xff");

	return 0;
}
