#include "batterytest.h"
#include "fpga.h"
#include "gpio.h"

#define AC_PRESENT_GPIO 43

BatteryTestStart::BatteryTestStart()
{
    name = new QString("Battery test start");
	gpio_export(AC_PRESENT_GPIO);
	gpio_set_direction(AC_PRESENT_GPIO, 0);
}

void BatteryTestStart::runTest() {
	uint32_t battery_level = 0;
	QString *str;

	str = new QString("Waiting for AC to be unplugged...");
	emit testStateUpdated(TEST_INFO, 0, str);

#ifdef linux
	/* Wait for AC to be unplugged */
	while (gpio_get_value(AC_PRESENT_GPIO))
		usleep(10000);
	battery_level = read_battery();
#endif

	str = new QString();
	str->sprintf("Battery level: %d mV", battery_level);
	emit testStateUpdated(TEST_INFO, 0, str);

	return;
}

BatteryTestStop::BatteryTestStop()
{
	name = new QString("Battery test finish");
}


void BatteryTestStop::runTest() {
        uint32_t battery_level;
	QString *str;

	str = new QString("Waiting for AC to be attached...");
	emit testStateUpdated(TEST_INFO, 0, str);

#ifdef linux
	/* Wait for AC to get plugged in again */
	while (!gpio_get_value(AC_PRESENT_GPIO))
		usleep(10000);

	battery_level = read_battery();
#endif

	str = new QString();
	str->sprintf("Battery level: %d mV", battery_level);
	emit testStateUpdated(TEST_INFO, 0, str);

	return;
}

