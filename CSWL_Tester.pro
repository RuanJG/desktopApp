#-------------------------------------------------
#
# Project created by QtCreator 2019-08-26T22:22:50
#
#-------------------------------------------------

QT       += core gui
QT      += serialport
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = CSWL_Tester
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    libs/chunk.cpp \
    libs/iapmaster.cpp \
    libs/uartcoder.cpp \
    libs/util.cpp \
    testerthread.cpp

HEADERS  += mainwindow.h \
    libs/chunk.h \
    libs/iapmaster.h \
    libs/uartcoder.h \
    libs/util.h \
    testerthread.h

FORMS    += mainwindow.ui
