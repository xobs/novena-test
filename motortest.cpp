#include <stdint.h>
#include <unistd.h>
#include <QString>
#include "motortest.h"
#include "fpga.h"

#define BASECLOCK 208000000 / 4096	// 208MHz base clock for motor
#define TARGET_RATE 10000 		// 10 kHz target PWM period
#define FPGA_MDIV 0x4b
#define FPGA_MDUTY 0x49
#define FPGA_MDIR 0x48
#define FPGA_MPANIC 0x47

#define MOT_FORWARD 2
#define MOT_REVERSE 1
#define MOT_STOP 0
#define MOT_BRAKE 3

MotorTest::MotorTest()
{
    name = new QString("Motor test");
    pwm_divider = (BASECLOCK - 2 * TARGET_RATE) / TARGET_RATE;
}

void MotorTest::runTest()
{
    int motor;
    uint8_t panic_value;

    // Set the PWM divider
    write_fpga(FPGA_MDIV, &pwm_divider, sizeof(pwm_divider));

    // Ungate motors
    panic_value = 0;
    write_fpga(FPGA_MPANIC, &panic_value, sizeof(panic_value));

    for (motor=0; motor<4; motor++)
        testMotorNumber(motor);

    // Stop all motors
    panic_value = 1;
    write_fpga(FPGA_MPANIC, &panic_value, sizeof(panic_value));
}


int MotorTest::dutyFromPercent(int percentage)
{
    return (percentage/100.0)*4096;
}


int MotorTest::setMotorDirection(int motor, int direction)
{
    uint8_t byte;
    byte = (direction<<(motor*2));
    return write_fpga(FPGA_MDIR, &byte, sizeof(byte));
}

int MotorTest::setDutyCycle(uint16_t cycle)
{
    return write_fpga(FPGA_MDUTY, &cycle, sizeof(cycle));
}


int MotorTest::testMotorNumber(int motor)
{
    QString *str;
    int level;
    uint16_t emf1, emf2;

    setDutyCycle(0);

    setMotorDirection(motor, MOT_FORWARD);
    for (level=0; level<100; level+=10) {
        setDutyCycle(dutyFromPercent(level));
        usleep(100000);
    }


    emf1 = read_adc(motor*2);
    emf2 = read_adc(motor*2+1);
    emf1 = read_adc(motor*2);
    emf2 = read_adc(motor*2+1);
    str = new QString();
    if ( (emf1 < 470 || emf1 > 560) && (emf2 < 470 || emf2 > 560) ) {
        str->sprintf("Motor %d forwards nominal: %d/%d", motor, emf1, emf2);
        emit testStateUpdated(TEST_INFO, motor, str);
    }
    else {
        str->sprintf("Motor %d forwards out of range: %d/%d", motor, emf1, emf2);
        emit testStateUpdated(TEST_ERROR, motor, str);
    }


    for (level=100; level>=0; level-=10) {
        setDutyCycle(dutyFromPercent(level));
        usleep(100000);
    }



    // Drive backwards
    setMotorDirection(motor, MOT_REVERSE);

    for (level=0; level<100; level+=10) {
        setDutyCycle(dutyFromPercent(level));
        usleep(100000);
    }


    emf1 = read_adc(motor*2);
    emf2 = read_adc(motor*2+1);
    str = new QString();
    if ( (emf1 < 470 || emf1 > 560) && (emf2 < 470 || emf2 > 560) ) {
        str->sprintf("Motor %d reverse nominal: %d/%d", motor, emf1, emf2);
        emit testStateUpdated(TEST_INFO, motor, str);
    }
    else {
        str->sprintf("Motor %d reverse out of range: %d/%d", motor, emf1, emf2);
        emit testStateUpdated(TEST_ERROR, motor, str);
    }


    for (level=100; level>=0; level-=10) {
        setDutyCycle(dutyFromPercent(level));
        usleep(100000);
    }


    // All-stop
    setMotorDirection(motor, MOT_STOP);
    setDutyCycle(0);

    return 0;
}
