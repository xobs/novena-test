#-------------------------------------------------
#
# Project created by QtCreator 2012-05-31T15:23:00
#
#-------------------------------------------------

QT       += core gui network multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
lessThan(QT_MAJOR_VERSION, 5): LIBS += -lqjson

TARGET = novena-test
TEMPLATE = app


SOURCES += main.cpp\
    delayedtextprinttest.cpp \
    audiotest.cpp \
    novenatestwindow.cpp \
    novenatestengine.cpp \
    novenatest.cpp \
    mmctest.cpp \
    eepromtest.cpp \
    netperftest.cpp \
    usbtest.cpp \
    timertest.cpp \
    fpgatest.cpp \
    waitfornetwork.cpp \
    hwclocktest.cpp \
    acceltest.cpp \
    keyboardmousetest.cpp \
    stmpetest.cpp \
    gpbbtest.cpp \
    destructivedisktest.cpp \
    programsenoko.cpp \
    buttontest.cpp \
    capacitytest.cpp \
    packageinstaller.cpp


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
    mmctest.h \
    eepromtest.h \
    netperftest.h \
    usbtest.h \
    timertest.h \
    fpgatest.h \
    waitfornetwork.h \
    hwclocktest.h \
    novena-eeprom.h \
    acceltest.h \
    keyboardmousetest.h \
    stmpetest.h \
    gpbbtest.h \
    destructivedisktest.h \
    programsenoko.h \
    buttontest.h \
    capacitytest.h \
    packageinstaller.h

FORMS    += novenatestwindow.ui

