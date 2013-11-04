#-------------------------------------------------
#
# Project created by QtCreator 2012-05-31T15:23:00
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = novena-test
TEMPLATE = app


SOURCES += main.cpp\
    delayedtextprinttest.cpp \
    audiotest.cpp \
    switchtest.cpp \
    hdmitest.cpp \
    fpga.c gpio.c \
    wifitest.cpp \
    novenatestwindow.cpp \
    novenatestengine.cpp \
    novenatest.cpp \
    mmctest.cpp


linux-gnueabi-oe-g++ {
    CONFIG += link_pkgconfig
    PKGCONFIG += alsa
    QMAKE_CXXFLAGS += -Wno-psabi
}



HEADERS  += novenatestwindow.h \
    novenatest.h \
    novenatestengine.h \
    delayedtextprinttest.h \
    audiotest.h \
    switchtest.h \
    hdmitest.h \
    wifitest.h \
    fpga.h \
    mmctest.h

FORMS    += novenatestwindow.ui

