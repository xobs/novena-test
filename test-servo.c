#include <stdio.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <unistd.h>
#include "fpga.h"
#include "harness.h"

#define PWM0_LO 0x50
#define PWM0_MD 0x51
#define PWM0_HI 0x52

#define PWM1_LO 0x53
#define PWM1_MD 0x54
#define PWM1_HI 0x55

#define PWM2_LO 0x56
#define PWM2_MD 0x57
#define PWM2_HI 0x58

#define PWM3_LO 0x59
#define PWM3_MD 0x5a
#define PWM3_HI 0x5b

#define PWM_PERIOD_LO 0x4d
#define PWM_PERIOD_MD 0x4e
#define PWM_PERIOD_HI 0x4f

#define TIMEDIV (1.0 / 13000000) // 13 MHz clock
#define PWM_PERIOD_RAW 0.02F
#define SERVO_MAX_RAW 0.002f
#define SERVO_MIN_RAW 0.001f
#define PWM_PERIOD (PWM_PERIOD_RAW / TIMEDIV)
#define SERVO_MAX (SERVO_MAX_RAW / TIMEDIV)
#define SERVO_MIN (SERVO_MIN_RAW / TIMEDIV)

static void set_pwm_period(uint32_t period) {
	uint8_t *val = (uint8_t *)&period;
	set_fpga(PWM_PERIOD_LO, val[0]);
	set_fpga(PWM_PERIOD_MD, val[1]);
	set_fpga(PWM_PERIOD_HI, val[2]);
	return;
}

static void set_pwm_degrees(uint32_t pwm, uint32_t deg) {
	uint8_t *val = (uint8_t *)&deg;
	deg = ((SERVO_MAX - SERVO_MIN) * (deg / 360.0)) + SERVO_MIN;
	set_fpga(PWM0_LO+(pwm*3), val[0]);
	set_fpga(PWM0_MD+(pwm*3), val[1]);
	set_fpga(PWM0_HI+(pwm*3), val[2]);
	return;
}

int test_servo(void) {
	uint32_t val;
	int i;
	set_pwm_period(PWM_PERIOD);

	for (i=0; i<4; i++) {
		int pwm = i;
		int adc = i+12;

		set_pwm_degrees(pwm, 0);
		usleep(200000);
		val = read_adc(adc);

		if (val < 50 || val > 110)
			harness_error(pwm+1, "PWM %d, ADC %d, value expected to be between 50 and 110, read as %d", pwm, adc, val);
		else
			harness_info(pwm+1, "PWM %d, ADC %d, value nominal at %d", pwm, adc, val);


		set_pwm_degrees(pwm, 360);
		usleep(200000);
		val = read_adc(adc);

		if (val < 120 || val > 170)
			harness_error(pwm+1, "PWM %d, ADC %d, value expected to be between 120 and 180, read as %d", pwm, adc, val);
		else
			harness_info(pwm+1, "PWM %d, ADC %d, value nominal at %d", pwm, adc, val);
	}

	return 0;
}
