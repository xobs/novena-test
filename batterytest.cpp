#include "batterytest.h"
#include "fpga.h"

BatteryTestStart::BatteryTestStart()
{
    name = new QString("Battery test start");
}

void BatteryTestStart::runTest() {
        uint32_t battery_level;
	QString *str;

	str = new QString("Waiting for AC to be unplugged...");
	emit testStateUpdated(TEST_INFO, 0, str);

        for (battery_level = read_battery();
             battery_level > 12000;
             battery_level = read_battery()) {
                usleep(100000);
        }

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

        for (battery_level = read_battery();
             battery_level < 12000;
             battery_level = read_battery()) {
                usleep(100000);
        }

	str = new QString();
	str->sprintf("Battery level: %d mV", battery_level);
	emit testStateUpdated(TEST_INFO, 0, str);

	return;
}

