#-------------------------------------------------
#
# Project created by QtCreator 2012-05-31T15:23:00
#
#-------------------------------------------------

QT       += core gui

TARGET = kovan-test
TEMPLATE = app


SOURCES += main.cpp\
        kovantestwindow.cpp \
    kovantest.cpp \
    externaltest.cpp \
    kovantestengine.cpp \
    delayedtextprinttest.cpp \
    audiotest.cpp \
    test-accel.c \
    test-audio.c \
    test-io.c \
    test-serial.c \
    test-servo.c \
    test-usb.c \
    test-wifi.c \
    fpga.c \
    harness.c

CONFIG += link_pkgconfig
PKGCONFIG += alsa libcurl



HEADERS  += kovantestwindow.h \
    kovantest.h \
    externaltest.h \
    kovantestengine.h \
    delayedtextprinttest.h \
    audiotest.h

FORMS    += kovantestwindow.ui
