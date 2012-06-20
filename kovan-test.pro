#-------------------------------------------------
#
# Project created by QtCreator 2012-05-31T15:23:00
#
#-------------------------------------------------

QT       += core gui network

TARGET = kovan-test
TEMPLATE = app


SOURCES += main.cpp\
        kovantestwindow.cpp \
    kovantest.cpp \
    externaltest.cpp \
    kovantestengine.cpp \
    delayedtextprinttest.cpp \
    audiotest.cpp \
    motortest.cpp \
    switchtest.cpp \
    test-accel.c \
    test-audio.c \
    test-io.c \
    test-serial.c \
    test-servo.c \
    test-usb.c \
    batterytest.cpp \
    fpga.c gpio.c \
    wifitest.cpp


linux-gnueabi-oe-g++ {
    CONFIG += link_pkgconfig
    PKGCONFIG += alsa
}



HEADERS  += kovantestwindow.h \
    kovantest.h \
    externaltest.h \
    kovantestengine.h \
    delayedtextprinttest.h \
    batterytest.h \
    audiotest.h \
    motortest.h \
    switchtest.h \
    wifitest.h

FORMS    += kovantestwindow.ui
