#include <stdint.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#ifdef linux
#include <linux/input.h>
#endif


#include "hdmitest.h"
#include "gpio.h"

static const uint8_t zerobytes_6slx9csg324[] = 
#include "zerobytes-6slx9csg324.h"
;

static const uint8_t zerobytes_6slx45csg324[] = 
#include "zerobytes-6slx45csg324.h"
;

#define LX9_FIRMWARE "/lib/firmware/kovan-lx9.bit"
#define LX45_FIRMWARE "/lib/firmware/kovan-lx45.bit"

#define LX45_JTAG 0x34008093
#define LX9_JTAG 0x24001093

#define TDO_GPIO 16
#define TMS_GPIO 18
#define TCK_GPIO 20
#define TDI_GPIO 34

#define GPIO_PATH "/sys/class/gpio"
#define EXPORT_PATH GPIO_PATH "/export"
#define UNEXPORT_PATH GPIO_PATH "/unexport"

#define FPGA_IOC_MAGIC  'c'

#define FPGA_IOCWTEST    _IOW(FPGA_IOC_MAGIC,  1, int)
#define FPGA_IOCRTEST    _IOR(FPGA_IOC_MAGIC,  2, int)
#define FPGA_IOCRESET    _IOW(FPGA_IOC_MAGIC,  3, int)
#define FPGA_IOCLED0     _IOW(FPGA_IOC_MAGIC,  4, int)
#define FPGA_IOCLED1     _IOW(FPGA_IOC_MAGIC,  5, int)
#define FPGA_IOCDONE     _IOR(FPGA_IOC_MAGIC,  6, int)
#define FPGA_IOCINIT     _IOR(FPGA_IOC_MAGIC,  7, int)


struct jtag_state {
	int tdi;
	int tms;
	int tck;
	int tdo;
};


/* Wiggle the TCK like, moving JTAG one step further along its state machine */
static int jtag_tick(struct jtag_state *state) {
	gpio_set_value(state->tck, 0);
	gpio_set_value(state->tck, 1);
	gpio_set_value(state->tck, 0);
	return 0;
}


/* Send five 1s through JTAG, which will bring it into reset state */
static int jtag_reset(struct jtag_state *state) {
	int i;
	for (i=0; i<5; i++) {
		gpio_set_value(state->tms, 1);
		jtag_tick(state);
	}

	return 0;
}


static int jtag_open(struct jtag_state *state) {

	gpio_export(TDI_GPIO);
	gpio_export(TMS_GPIO);
	gpio_export(TCK_GPIO);
	gpio_export(TDO_GPIO);

	gpio_set_direction(TDI_GPIO, 0);
	gpio_set_direction(TMS_GPIO, 1);
	gpio_set_direction(TCK_GPIO, 1);
	gpio_set_direction(TDO_GPIO, 1);

	gpio_set_value(TDO_GPIO, 0);
	gpio_set_value(TMS_GPIO, 0);
	gpio_set_value(TCK_GPIO, 0);

	state->tdi = TDI_GPIO;
	state->tms = TMS_GPIO;
	state->tck = TCK_GPIO;
	state->tdo = TDO_GPIO;

	jtag_reset(state);

	return 0;
}

/* Reads the ID CODE out of the FPGA
 * When the state machine is reset, the sequence 0, 1, 0, 0 will move
 * it to a point where continually reading the TDO line will yield the
 * ID code.
 *
 * This is because by default, the reset command loads the chip's ID
 * into the data register, so all we have to do is read it out.
 */
static uint32_t jtag_idcode(struct jtag_state *state) {
	int i;
	uint32_t val = 0;

	/* Reset the state machine */
	jtag_reset(state);

	/* Get into "Run-Test/ Idle" state */
	gpio_set_value(state->tms, 0);
	jtag_tick(state);

	/* Get into "Select DR-Scan" state */
	gpio_set_value(state->tms, 1);
	jtag_tick(state);

	/* Get into "Capture DR" state */
	gpio_set_value(state->tms, 0);
	jtag_tick(state);

	/* Get into "Shift-DR" state */
	gpio_set_value(state->tms, 0);
	jtag_tick(state);

	/* Read the code out */
	for (i=0; i<32; i++) {
		int ret = gpio_get_value(state->tdi);
		val |= (ret<<i);
		jtag_tick(state);
	}

	return val;
}

/* Close GPIOs and return everything to how it was */
static void jtag_cleanup(struct jtag_state *state) {
	gpio_unexport(state->tdi);
	gpio_unexport(state->tms);
	gpio_unexport(state->tck);
	gpio_unexport(state->tdo);
}


#define SWAP16(x) ((uint16_t)((((uint16_t)(x) & 0xFF00) >> 8) | \
				  (((uint16_t)(x) & 0x00FF) << 8)))

void HDMITest::loadFpgaFirmware(const uint8_t *bfr, ssize_t size) {
	int i;
	uint16_t length;

	// Skip first three sections
	for (i=0; i<3; i++) {
		memcpy(&length, bfr, sizeof(length));
		length = SWAP16(length);
		bfr += sizeof(length);
		size -= sizeof(length);
		printf("Section %d size: %d\n", i, length);

		bfr += length;
		size -= length;
	}

	if (*bfr != 'b') {
		QString *str = new QString();
		str->sprintf("Unexpected key: wanted 'b', got '%c' (0x%02x)", *bfr, *bfr);
		emit testStateUpdated(TEST_ERROR, 0, str);
		return;
	}
	bfr++;
	size--;
	memcpy(&length, bfr, sizeof(length));
	length = SWAP16(length);
	bfr += sizeof(length);
	size -= sizeof(length);
	char fpga_name[length+1];
	bzero(fpga_name, sizeof(fpga_name));
	memcpy(fpga_name, bfr, length);
	bfr += length;
	size -= length;
	
	if (*bfr != 'c') {
		QString *str = new QString();
		str->sprintf("Unexpected key: wanted 'c', got '%c' (0x%02x)", *bfr, *bfr);
		emit testStateUpdated(TEST_ERROR, 0, str);
		return;
	}
	bfr++;
	size--;
	memcpy(&length, bfr, sizeof(length));
	length = SWAP16(length);
	bfr += sizeof(length);
	size -= sizeof(length);
	char date_code[length+1];
	bzero(date_code, sizeof(date_code));
	memcpy(date_code, bfr, length);
	bfr += length;
	size -= length;
	
	if (*bfr != 'd') {
		QString *str = new QString();
		str->sprintf("Unexpected key: wanted 'd', got '%c' (0x%02x)", *bfr, *bfr);
		emit testStateUpdated(TEST_ERROR, 0, str);
		return;
	}
	bfr++;
	size--;
	memcpy(&length, bfr, sizeof(length));
	length = SWAP16(length);
	bfr += sizeof(length);
	size -= sizeof(length);
	char time_code[length+1];
	bzero(time_code, sizeof(time_code));
	memcpy(time_code, bfr, length);
	bfr += length;
	size -= length;
	
	if (*bfr != 'e') {
		QString *str = new QString();
		str->sprintf("Unexpected key: wanted 'e', got '%c' (0x%02x)", *bfr, *bfr);
		emit testStateUpdated(TEST_ERROR, 0, str);
		return;
	}
	bfr++;
	size--;
	memcpy(&length, bfr, sizeof(length));
	length = SWAP16(length);
	bfr += sizeof(length);
	size -= sizeof(length);
	

	int fd = open("/dev/fpga", O_RDWR);
	if (-1 == fd) {
		emit testStateUpdated(TEST_ERROR, 0, new QString("Unable to open /dev/fpga"));
		return;
	}

	if (ioctl(fd, FPGA_IOCRESET, NULL) < 0) {
		emit testStateUpdated(TEST_ERROR, 0, new QString("Unable to reset FPGA"));
		close(fd);
		return;
	}

	if (write(fd, bfr, size) != size) {
		emit testStateUpdated(TEST_ERROR, 0, new QString("Unable to write firmware"));
		close(fd);
		return;
	}
	close(fd);

	emit testStateUpdated(TEST_INFO, 0, new QString("HDMI FPGA firmware loaded"));

	return;
}


HDMITest::HDMITest()
{
    name = new QString("HDMI Test");
}

void HDMITest::runTest() {
    struct jtag_state state;
    uint32_t idcode;
    QString *str;

    jtag_open(&state);
    idcode = jtag_idcode(&state);
    jtag_cleanup(&state);

    if (idcode == LX9_JTAG)
        loadFpgaFirmware(zerobytes_6slx9csg324, sizeof(zerobytes_6slx9csg324));
    else if (idcode == LX45_JTAG)
        loadFpgaFirmware(zerobytes_6slx45csg324, sizeof(zerobytes_6slx45csg324));
    else {
        QString *str = new QString();
        str->sprintf("Unrecognized FPGA JTAG: 0x%08x", idcode);
        emit testStateUpdated(TEST_ERROR, 0, str);
        return;
    }

#ifdef linux
    int fd = open("/dev/input/event0", O_RDONLY);
    struct input_event e;

    if (-1 == fd) {
        str = new QString("Unable to open switch: ");
        str->append(strerror(errno));
        emit testStateUpdated(TEST_ERROR, 0, str);
        return;
    }

    str = new QString("Please press the side switch if the pattern appears");
    emit testStateUpdated(TEST_INFO, 0, str);

    if (read(fd, &e, sizeof(e)) != sizeof(e)) {
        str = new QString("Unable to read switch: ");
        str->append(strerror(errno));
        emit testStateUpdated(TEST_ERROR, 0, str);
        close(fd);
        return;
    }

    close(fd);


    str = new QString("Side switch OK");
    emit testStateUpdated(TEST_INFO, 0, str);
#else
    str = new QString("Switch test skipped on this platform");
    emit testStateUpdated(TEST_INFO, 0, str);
#endif

    
    if (idcode == LX9_JTAG) {
        uint8_t bytes[sizeof(zerobytes_6slx9csg324)];
        fd = open(LX9_FIRMWARE, O_RDONLY);
        if (-1 == fd) {
            str = new QString("Unable to open LX9 firmware");
            emit testStateUpdated(TEST_ERROR, 0, str);
            return;
        }
        if (read(fd, bytes, sizeof(bytes)) != sizeof(bytes)) {
            str = new QString("Unable to read LX9 firmware");
            emit testStateUpdated(TEST_ERROR, 0, str);
            close(fd);
            return;
        }
        close(fd);
        loadFpgaFirmware(bytes, sizeof(bytes));
    }
    else if (idcode == LX45_JTAG) {
        uint8_t bytes[sizeof(zerobytes_6slx45csg324)];
        fd = open(LX45_FIRMWARE, O_RDONLY);
        if (-1 == fd) {
            str = new QString("Unable to open LX45 firmware");
            emit testStateUpdated(TEST_ERROR, 0, str);
            return;
        }
        if (read(fd, bytes, sizeof(bytes)) != sizeof(bytes)) {
            str = new QString("Unable to read LX45 firmware");
            emit testStateUpdated(TEST_ERROR, 0, str);
            close(fd);
            return;
        }
        close(fd);
        loadFpgaFirmware(bytes, sizeof(bytes));
    }

    return;
}
