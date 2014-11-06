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
#include <linux/usbdevice_fs.h>
#include <syscall.h>
#define init_module(mod, len, opts) syscall(__NR_init_module, mod, len, opts)
#define delete_module(mod) syscall(__NR_delete_module, mod)

#define CONFIGFS_PATH "kernel/fs/configfs/configfs.ko"
#define LIBCOMPOSITE_PATH "kernel/drivers/usb/gadget/libcomposite.ko"
#define USB_F_SS_LB_PATH "kernel/drivers/usb/gadget/function/usb_f_ss_lb.ko"
#define G_ZERO_PATH "kernel/drivers/usb/gadget/legacy/g_zero.ko"
#define USBTEST_PATH "kernel/drivers/usb/misc/usbtest.ko"
#define USB_PATH "/sys/bus/usb/devices"

#define CABLE_TRIES 5

struct usb_descr {
    int busnum;
    int devnum;
};


struct usbtest_param {
    // inputs
    unsigned        test_num;   /* 0..(TEST_CASES-1) */
    unsigned        iterations;
    unsigned        length;
    unsigned        vary;
    unsigned        sglen;

    // outputs
    struct timeval      duration;
};
#define USBTEST_REQUEST _IOWR('U', 100, struct usbtest_param)

#include "usbtest.h"


int USBTest::findUSBDevice(struct usb_descr *desc, const char *product)
{
    DIR *d;
    struct dirent *de;
    
    d = opendir(USB_PATH);

    if (!d) {
        testError(QString("Unable to open USB path: %1").arg(strerror(errno)));
        return 0;
    }

    while ((de = readdir(d))) {
        char prodfile[2048];
        char read_product[256];
        FILE *prod;
        unsigned int i;
        snprintf(prodfile, sizeof(prodfile)-1, "%s/%s/product", USB_PATH, de->d_name);
        prod = fopen(prodfile, "r");
        if (!prod) {
            continue;
        }

        memset(read_product, 0, sizeof(read_product));
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
            testDebug(QString("Unable to open busnum %1: %2").arg(prodfile).arg(strerror(errno)));
            continue;
        }
        fscanf(prod, "%d", &desc->busnum);
        fclose(prod);


        snprintf(prodfile, sizeof(prodfile)-1, "%s/%s/devnum", USB_PATH, de->d_name);
        prod = fopen(prodfile, "r");
        if (!prod) {
            testDebug(QString("Unable to open devnum %1: %2").arg(prodfile).arg(strerror(errno)));
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


int USBTest::testUSB(struct usb_descr *desc)
{
    char dev_name[256];
    int fd;
    int ret = 0;
    int testnum;

    snprintf(dev_name, sizeof(dev_name)-1, "/dev/bus/usb/%03d/%03d", desc->busnum, desc->devnum);

    fd = open(dev_name, O_RDWR);
    if (fd < 0) {
        testError(QString("Unable to open USB device %1: %2").arg(dev_name).arg(strerror(errno)));
        ret = 1;
        goto out;
    }

    /* Run tests */
    for (testnum = 0; testnum < 10; testnum++) {
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

        testInfo(QString("Running USB test %1/10").arg(testnum + 1));
        if (ioctl(fd, USBDEVFS_IOCTL, &params) < 0) {
            testError(QString("Unable to run test %1: %2").arg(testnum).arg(strerror(errno)));
            ret = 2;
            goto out;
        }
    }
    testInfo("All USB tests completed");

out:

    if (fd >= 0)
        close(fd);
    return ret;
}

int USBTest::initModule(const char *modname)
{
    int fd;
    struct stat st;
    int ret;
    struct utsname ver;
    QString path;
    uname(&ver);

    testDebug(QString("Kernel release: %1  Module: %2").arg(ver.release).arg(modname));
    path = QString("/lib/modules/%1/%2").arg(ver.release).arg(modname);

    fd = open(path.toUtf8(), O_RDONLY);
    if (fd == -1) {
        testError(QString("Unable to open %1 module: %2").arg(path).arg(strerror(errno)));
        return -1;
    }

    fstat(fd, &st);

    char dat[st.st_size];
    if (read(fd, dat, sizeof(dat)) != (int)sizeof(dat)) {
        testError(QString("Couldn't read %1 module off disk: %2").arg(path).arg(strerror(errno)));
        close(fd);
        return -2;
    }
    close(fd);

    ret = init_module(dat, sizeof(dat), "");
    if (ret && errno == EEXIST)
        ret = errno = 0;
    if (ret)
        testError(QString("Couldn't insert %1 module: %2").arg(path).arg(strerror(errno)));

    return ret;
}

int USBTest::deleteModule(const char *name)
{
    return delete_module(name);
}


USBTest::USBTest()
{
    name = "USB Loopback Test";
}

void USBTest::runTest()
{
    struct usb_descr descr;
    int tries;

    initModule(CONFIGFS_PATH);
    initModule(LIBCOMPOSITE_PATH);
    initModule(USB_F_SS_LB_PATH);
    initModule(G_ZERO_PATH);
    initModule(USBTEST_PATH);

    memset(&descr, 0, sizeof(descr));
    for (tries = 0; tries < CABLE_TRIES; tries++) {
        testInfo(QString("Looking for loopback cable (try %1/%2)...").arg(tries+1).arg(CABLE_TRIES));
        if (findUSBDevice(&descr, "Gadget Zero"))
            break;
        sleep(1);
    }

    if (!descr.busnum && !descr.devnum) {
        testError("Unable to find connected cable");
        return;
    }

    testUSB(&descr);
}
