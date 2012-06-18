#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>

#include "harness.h"

#define EXT_DEV_NAME "/dev/ttyS2"
#define INT_DEV_NAME "/dev/ttyS0"

int test_serial(void) {
	int ext_fd = 0;
	int int_fd = 0;
	int ret = 0;
	char bfr[64];
	int offset;

	struct termios old_ext, old_int, new_ext, new_int;

	/* Quick hack to stop /dev/ttyS0 service */
	system("systemctl stop serial-getty@ttyS0.service 2> /dev/null > /dev/null");

	ext_fd = open(EXT_DEV_NAME, O_RDWR | O_NOCTTY);
	if (-1 == ext_fd) {
		harness_error(0, "Unable to open external serial port: %s", strerror(errno));
		ret = 1;
		goto out;
	}


	int_fd = open(INT_DEV_NAME, O_RDWR | O_NOCTTY);
	if (-1 == int_fd) {
		harness_error(1, "Unable to open internal serial port: %s", strerror(errno));
		ret = 1;
		goto out;
	}


	/* Set baudrates */
	tcgetattr(ext_fd, &old_ext);
	tcgetattr(int_fd, &old_int);

	bzero(&new_ext, sizeof(new_ext));
	bzero(&new_int, sizeof(new_int));

	new_ext.c_cflag = B115200 | CS8 | CLOCAL | CREAD;
	new_ext.c_iflag = IGNPAR;
	new_ext.c_oflag = 0;
	new_ext.c_lflag = 0;

	new_ext.c_cc[VTIME] = 1; /* Time out if no chars read after 1/10 sec*/
	new_ext.c_cc[VMIN]  = 0; /* 0 bytes, so VTIME will timeout */

	memcpy(&new_int, &new_ext, sizeof(new_int));

	tcflush(ext_fd, TCIFLUSH);
	tcflush(int_fd, TCIFLUSH);

	tcsetattr(ext_fd, TCSANOW, &new_ext);
	tcsetattr(int_fd, TCSANOW, &new_int);

	for (offset=0; offset<256; offset+=sizeof(bfr)) {
		int i;
		int err = 0;
		int readval;
		char bfr2[sizeof(bfr)];
		for (i=0; i<sizeof(bfr); i++)
			bfr[i] = i+offset;
		if (write(int_fd, bfr, sizeof(bfr)) != sizeof(bfr)) {
			harness_error(2, "Unable to write bytes [%d .. %d] out internal serial port: %s", offset, offset+sizeof(bfr), strerror(errno));
			ret++;
			err++;
		}


		bzero(bfr2, sizeof(bfr2));
		readval = read(ext_fd, bfr2, sizeof(bfr2));
		if (readval != sizeof(bfr2)) {
			if (!readval)
				errno = ENODATA;
			harness_error(3, "Unable to read bytes [%d .. %d] in external serial port: %s", offset, offset+sizeof(bfr2), strerror(errno));
			ret++;
			err++;
		}

		if (!err) {
			for (i=0; i<sizeof(bfr); i++) {
				if (bfr[i] != bfr2[i]) {
					harness_error(4, "Data consistency error: Byte %d doesn't match int -> ext (sent: %d  read: %d)", i+offset, bfr[i], bfr2[i]);
					ret++;
					err++;
				}
			}
		}

		if (!err)
			harness_info(4, "Serial int -> ext bytes (%d .. %d) OK",
				offset, offset+sizeof(bfr));
	}


	for (offset=0; offset<256; offset+=sizeof(bfr)) {
		int i;
		int err = 0;
		int readval;
		char bfr2[sizeof(bfr)];
		for (i=0; i<sizeof(bfr); i++)
			bfr[i] = i+offset;
		if (write(ext_fd, bfr, sizeof(bfr)) != sizeof(bfr)) {
			harness_error(2, "Unable to write bytes [%d .. %d] out external serial port: %s", offset, offset+sizeof(bfr), strerror(errno));
			ret++;
			err++;
		}


		bzero(bfr2, sizeof(bfr2));
		readval = read(int_fd, bfr2, sizeof(bfr2));
		if (readval != sizeof(bfr2)) {
			if (!readval)
				errno = ENODATA;
			harness_error(3, "Unable to read bytes [%d .. %d] in internal serial port: %s", offset, offset+sizeof(bfr2), strerror(errno));
			ret++;
			err++;
		}

		if (!err) {
			for (i=0; i<sizeof(bfr); i++) {
				if (bfr[i] != bfr2[i]) {
					harness_error(4, "Data consistency error: Byte %d doesn't match ext -> int (sent: %d  read: %d)", i+offset, bfr[i], bfr2[i]);
					ret++;
					err++;
				}
			}
		}

		if (!err)
			harness_info(4, "Serial ext -> int bytes (%d .. %d) OK",
				offset, offset+sizeof(bfr));
	}


	if (!ret)
		harness_info(5, "Serial test passed");

out:
	if (ext_fd) {
		tcsetattr(ext_fd, TCSANOW, &old_ext);
		close(ext_fd);
	}
	if (int_fd) {
		tcsetattr(int_fd, TCSANOW, &old_int);
		close(int_fd);
	}

	return ret;
}
