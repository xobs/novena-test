#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/ioctl.h>

#include <sys/utsname.h>

#include "harness.h"

#ifdef linux
#include <linux/usbdevice_fs.h>
#include <syscall.h>
#define init_module(mod, len, opts) syscall(__NR_init_module, mod, len, opts)
#define delete_module(mod) syscall(__NR_delete_module, mod)

#define G_ZERO_PATH "kernel/drivers/usb/gadget/g_zero.ko"
#define USBTEST_PATH "kernel/drivers/usb/misc/usbtest.ko"
#define USB_PATH "/sys/bus/usb/devices"

#define CABLE_TRIES 5

struct usb_descr {
	int busnum;
	int devnum;
};


struct usbtest_param {
	// inputs
	unsigned		test_num;	/* 0..(TEST_CASES-1) */
	unsigned		iterations;
	unsigned		length;
	unsigned		vary;
	unsigned		sglen;

	// outputs
	struct timeval		duration;
};
#define USBTEST_REQUEST	_IOWR('U', 100, struct usbtest_param)



static int find_usb_device(struct usb_descr *desc, char *product) {
	DIR *d;
	struct dirent *de;
	
	d = opendir(USB_PATH);

	if (!d) {
		harness_error(4, "Unable to open USB path: %s", strerror(errno));
		return 0;
	}

	while ((de = readdir(d))) {
		char prodfile[2048];
		char read_product[256];
		FILE *prod;
		int i;
		snprintf(prodfile, sizeof(prodfile)-1, "%s/%s/product", USB_PATH, de->d_name);
		prod = fopen(prodfile, "r");
		if (!prod) {
			continue;
		}

		bzero(read_product, sizeof(read_product));
		fread(read_product, sizeof(read_product)-1, 1, prod);
		fclose(prod);
		for (i=0; i<sizeof(read_product); i++)
			if (read_product[i] == '\n')
				read_product[i] = '\0';

		if (strcmp(product, read_product))
			continue;


		snprintf(prodfile, sizeof(prodfile)-1, "%s/%s/busnum", USB_PATH, de->d_name);
		prod = fopen(prodfile, "r");
		if (!prod) {
			harness_debug(5, "Unable to open busnum %s: %s", prodfile, strerror(errno));
			continue;
		}
		fscanf(prod, "%d", &desc->busnum);
		fclose(prod);


		snprintf(prodfile, sizeof(prodfile)-1, "%s/%s/devnum", USB_PATH, de->d_name);
		prod = fopen(prodfile, "r");
		if (!prod) {
			harness_debug(5, "Unable to open devnum %s: %s", prodfile, strerror(errno));
			continue;
		}
		fscanf(prod, "%d", &desc->devnum);
		fclose(prod);


		closedir(d);
		return 1;
	}
	closedir(d);
	return 0;
}


static int do_test_usb(struct usb_descr *desc) {
	char dev_name[256];
	int fd;
	int ret = 0;
	int testnum;

	snprintf(dev_name, sizeof(dev_name)-1, "/dev/bus/usb/%03d/%03d", desc->busnum, desc->devnum);

	fd = open(dev_name, O_RDWR);
	if (fd < 0) {
		harness_error(8, "Unable to open USB device %s: %s", dev_name, strerror(errno));
		ret = 1;
		goto out;
	}

	/* Run tests */
	for (testnum=0; testnum<=10; testnum++) {
		struct usbdevfs_ioctl params;
		struct usbtest_param p;

		params.ifno = 0;
		params.ioctl_code = USBTEST_REQUEST;
		p.test_num = testnum;
		p.iterations = 1000;
		p.length = 512;
		p.vary = 512;
		p.sglen = 32;
		params.data = &p;

		harness_info(10+testnum, "Running USB test %d/11", testnum+1);
		if (ioctl(fd, USBDEVFS_IOCTL, &params) < 0) {
			harness_error(9, "Unable to run test %d: %s", testnum, strerror(errno));
			ret = 2;
			goto out;
		}
	}
	harness_info(20, "All USB tests completed");

out:

	if (fd >= 0) {
		close(fd);
	}
	return ret;
}

static int my_init_module(char *modname) {
	int fd;
	struct stat st;
	int ret;
	char path[2048];
	struct utsname ver;
	uname(&ver);

	snprintf(path, sizeof(path)-1, "/lib/modules/%s/%s", ver.release, modname);

	fd = open(path, O_RDONLY);
	if (fd == -1) {
		harness_error(1, "Unable to open %s module: %s", modname, strerror(errno));
		return -1;
	}

	fstat(fd, &st);

	char dat[st.st_size];
	if (read(fd, dat, sizeof(dat)) != sizeof(dat)) {
		harness_error(2, "Couldn't read %s module off disk: %s", modname, strerror(errno));
		close(fd);
		return -2;
	}
	close(fd);

	ret = init_module(dat, sizeof(dat), "");
	if (ret && errno == EEXIST)
		ret = errno = 0;
	if (ret)
		harness_error(3, "Couldn't insert %s module: %s", modname, strerror(errno));

	return ret;
}

static int my_delete_module(const char *name) {
	return delete_module(name);
}


int test_usb(void) {
	struct usb_descr descr;
	int ret = 0;
	int tries;

	system("systemctl stop network-gadget-init.service 2> /dev/null > /dev/null");
	my_delete_module("g_ether");
	my_init_module(G_ZERO_PATH);
	my_init_module(USBTEST_PATH);

	bzero(&descr, sizeof(descr));
	for (tries=0; tries<CABLE_TRIES; tries++) {
		harness_info(3, "Looking for loopback cable (try %d/%d)...", tries+1, CABLE_TRIES);
		if (find_usb_device(&descr, "Gadget Zero"))
			break;
		sleep(1);
	}

	if (!descr.busnum && !descr.devnum) {
		harness_error(4, "Unable to find connected cable");
		ret = 1;
		goto out;
	}

	if (!do_test_usb(&descr)) {
		ret = 2;
		goto out;
	}

out:
	return ret;
}
#else
int test_usb(void) {
	return 0;
}
#endif //linux
