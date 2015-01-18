#include "batterychargetest.h"
#include "timertest.h"

#include <QFile>
#include <QTime>

#include <errno.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <ctype.h>

#define SENOKO_I2C_FILE "/dev/i2c-0"
#define SENOKO_I2C_ADDR 0x20
#define GG_I2C_FILE "/dev/i2c-0"
#define GG_I2C_ADDR 0x0b

#define MAX_TRIES 10

#define CELL_TOLERANCE_MV 50
#define TEMPERATURE_MIN_C 18
#define TEMPERATURE_MAX_C 35
#define TEMP_KELVIN_TO_CELSIUS 2731

/* When charging, the current should be at least this much */
#define CHARGE_CURRENT_MIN_MA 100

/* This is the voltage at which the battery is "conditioned" */
#define BATTERY_FINISHED_MV 11600

/* If the battery gets here, it's gone too far and is no longer considered "good" */
#define BATTERY_ABORT_MV 10500

/* How often to print test state */
#define REPORT_INTERVAL_SECONDS 10

static int senoko_fd;
static int gg_fd;

static int senokoShutdown(void)
{
    struct i2c_rdwr_ioctl_data session;
    struct i2c_msg messages[1];

    char power_off_sequence[] = { 0x0f, (2 << 6) | (1 << 0) };

    if (senoko_fd <= 0)
        senoko_fd = open(SENOKO_I2C_FILE, O_RDWR);

    messages[0].addr = SENOKO_I2C_ADDR;
    messages[0].flags = 0;
    messages[0].len = sizeof(power_off_sequence);
    messages[0].buf = power_off_sequence;

    session.msgs = messages;
    session.nmsgs = 1;

    while(ioctl(senoko_fd, I2C_RDWR, &session) < 0)
        ;

    return 0;
}

static int getGasGaugeDataReal(int reg, void *data, quint32 len)
{
    struct i2c_rdwr_ioctl_data session;
    struct i2c_msg messages[2];
    char set_addr_buf[1];

    if (gg_fd <= 0)
        gg_fd = open(GG_I2C_FILE, O_RDWR);

    memset(set_addr_buf, 0, sizeof(set_addr_buf));
    memset(data, 0, len);

    set_addr_buf[0] = reg;

    messages[0].addr = GG_I2C_ADDR;
    messages[0].flags = 0;
    messages[0].len = sizeof(set_addr_buf);
    messages[0].buf = set_addr_buf;

    messages[1].addr = GG_I2C_ADDR;
    messages[1].flags = I2C_M_RD;
    messages[1].len = len;
    messages[1].buf = (char *)data;

    session.msgs = messages;
    session.nmsgs = 2;

    if(ioctl(gg_fd, I2C_RDWR, &session) < 0)
        return -1;

    return 0;
}

static int getGasGaugeData(int reg, void *data, quint32 len)
{
    int tries;
    bool success;

    success = false;
    for (tries = 0; !success && (tries < MAX_TRIES); tries++) {
        if (getGasGaugeDataReal(reg, data, len))
            sleep(1);
        else
            success = true;
    }
    if (!success)
        return -1;
    return 0;
}

BatteryChargeMonitor::BatteryChargeMonitor(void)
{
    stop_run = false;
}

void BatteryChargeMonitor::run(void)
{
    while (!stop_run) {
        quint16 voltage;

        if (getGasGaugeData(0x09, (void *)&voltage, sizeof(voltage)))
            continue;

        if (voltage < BATTERY_ABORT_MV)
            senokoShutdown();

        msleep(2500);
    }
}

BatteryChargeTestStart::BatteryChargeTestStart(void)
{
    name = "Battery Charge Test (Start)";
    senoko_fd = 0;
    chargeMonitor = new BatteryChargeMonitor();
}

void BatteryChargeTestStart::runTest()
{
    quint16 voltage;

    testInfo("Verifying the voltage is high enough to start");
    if (getGasGaugeData(0x09, (void *)&voltage, sizeof(voltage))) {
        testError("Unable to communicate with gas gauge");
        return;
    }

    if (voltage < 9500) {
        testError(QString("Battery is defective.  Starting voltage is %1, should be at least 9500."));
        return;
    }

    if (voltage < 10900) {
        testInfo("Waiting for battery voltage to be at least 10.9V");
        /* Wait for the votlage to be at least 10.9V */
        do {
            sleep(1);
            if (getGasGaugeData(0x09, (void *)&voltage, sizeof(voltage))) {
                testError("Unable to communicate with gas gauge");
                return;
            }
        } while (voltage < 10900);
        testInfo("Battery is charged");
    }

    /* Ensure the cells are within 50 mV of each other */
    quint16 cell1;
    quint16 cell2;
    quint16 cell3;
    if (getGasGaugeData(0x3d, (void *)&cell1, sizeof(cell1))) {
        testError("Unable to get cell 1 voltage");
        return;
    }

    if (getGasGaugeData(0x3e, (void *)&cell2, sizeof(cell2))) {
        testError("Unable to get cell 2 voltage");
        return;
    }

    if (getGasGaugeData(0x3f, (void *)&cell3, sizeof(cell3))) {
        testError("Unable to get cell 3 voltage");
        return;
    }

    testDebug(QString("Cell voltages: %1  %2  %3").arg(QString::number(cell1)).arg(QString::number(cell2)).arg(QString::number(cell3)));

    int diff12 = abs((int)cell1 - (int)cell2);
    int diff13 = abs((int)cell1 - (int)cell3);
    int diff23 = abs((int)cell2 - (int)cell3);

    if (diff12 > CELL_TOLERANCE_MV) {
        testError(QString("Cell 1 and cell 2 differ by %1 mV (max: %2 mV)").arg(QString::number(diff12).arg(QString::number(CELL_TOLERANCE_MV))));
        return;
    }

    if (diff13 > CELL_TOLERANCE_MV) {
        testError(QString("Cell 1 and cell 3 differ by %1 mV (max: %2 mV)").arg(QString::number(diff13).arg(QString::number(CELL_TOLERANCE_MV))));
        return;
    }

    if (diff23 > CELL_TOLERANCE_MV) {
        testError(QString("Cell 2 and cell 3 differ by %1 mV (max: %2 mV)").arg(QString::number(diff23).arg(QString::number(CELL_TOLERANCE_MV))));
        return;
    }

    quint16 temp;
    if (getGasGaugeData(0x08, (void *)&temp, sizeof(temp))) {
        testError("Unable to get gas gauge temperature");
        return;
    }

    int temp_c = (((double)temp) - TEMP_KELVIN_TO_CELSIUS) / 10.0;
    if (temp_c < TEMPERATURE_MIN_C) {
        testError(QString("Temperature is too low (wanted %1 C, got %2 C").arg(QString::number(TEMPERATURE_MIN_C)).arg(QString::number(temp_c)));
        return;
    }
    else if (temp_c > TEMPERATURE_MAX_C) {
        testError(QString("Temperature is too high (wanted %1 C, got %2 C").arg(QString::number(TEMPERATURE_MAX_C)).arg(QString::number(temp_c)));
        return;
    }
    else
        testInfo(QString("Temperature is OK: %1 C").arg(QString::number(temp_c)));

    chargeMonitor->start();
}

BatteryChargeTestRate::BatteryChargeTestRate(void)
{
    name = "Battery Charge Test Rate";
    senoko_fd = 0;
}

void BatteryChargeTestRate::runTest()
{
    /* Verify there is current flowing (the charger should be enabled already) */
    sleep(10);

    qint16 current;
    if (getGasGaugeData(0x0a, (void *)&current, sizeof(current))) {
        testError("Unable to get current level");
        return;
    }

    if (current < CHARGE_CURRENT_MIN_MA) {
        testError(QString("Current should be %1 mA, but observed %2 mA").arg(QString::number(CHARGE_CURRENT_MIN_MA)).arg(QString::number(current)));
        return;
    }
    testInfo(QString("Charger was observed to be driving %1 mA (threshold: %2 mA)").arg(QString::number(current)).arg(QString::number(CHARGE_CURRENT_MIN_MA)));

    testInfo("Battery test passed");
}

BatteryChargeTestCondition::BatteryChargeTestCondition(void)
{
    name = "Battery Charge Test (Conditioning)";
    senoko_fd = 0;
}

void BatteryChargeTestCondition::runTest()
{
    quint16 voltage;
    qint16 current;
    int seconds_counter = 0;

    testInfo(QString("Waiting for battery voltage to drop below %1 mV").arg(QString::number(BATTERY_FINISHED_MV)));
    /* Wait for the votlage to drop below conditioning value */
    do {
        sleep(1);
        seconds_counter++;

        if (getGasGaugeData(0x09, (void *)&voltage, sizeof(voltage))) {
            testError("Unable to communicate with gas gauge");
            return;
        }

        if (getGasGaugeData(0x0a, (void *)&current, sizeof(current))) {
            testError("Unable to get current level");
            return;
        }

        if (!(seconds_counter % REPORT_INTERVAL_SECONDS)) {
            QTime elapsed;
            elapsed.addMSecs(testElapsedMs());

            testInfo(QString("Test duration: %1  Voltage: %2 mV  Current: %3 mA")
                     .arg(elapsed.toString("H:mm:ss"))
                     .arg(QString::number(voltage))
                     .arg(QString::number(current)));
        }
    } while (voltage > BATTERY_FINISHED_MV);

    if (getGasGaugeData(0x09, (void *)&voltage, sizeof(voltage))) {
        testError("Unable to communicate with gas gauge");
        return;
    }
    if (voltage < BATTERY_ABORT_MV) {
        testError(QString("Battery voltage is too low.  Wanted: %1 mV, got %2 mV").arg(QString::number(BATTERY_ABORT_MV)).arg(QString::number(voltage)));
        senokoShutdown();
        return;
    }

    testInfo("Battery conditioned");
    sync();
    sleep(10);
    while (1)
        senokoShutdown();
}
