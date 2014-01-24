#ifndef USBTEST_H
#define USBTEST_H
#include <QThread>
#include <QFile>
#include "novenatest.h"

struct usb_descr;

class USBTest : public NovenaTest
{
public:
    USBTest();
    void runTest();

private:
    int deleteModule(const char *name);
    int initModule(const char *modname);
    int testUSB(struct usb_descr *desc);
    int findUSBDevice(struct usb_descr *desc, const char *product);
};

#endif // USBTEST_H
