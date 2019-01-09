#-------------------------------------------------
#
# Project created by QtCreator 2017-06-30T07:42:31
#
#-------------------------------------------------

QT       += core gui network

#use serial
QT      += serialport

#use console for debug

#CONFIG  += console
#use execl
 QT +=  axcontainer

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

TARGET = T-REX
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    chunk.cpp \
    uartcoder.cpp \
    util.cpp \
    excelengine.cpp \
    iapmaster.cpp \
    qcustomplot.cpp \
    serialcoder.cpp

HEADERS  += mainwindow.h \
    chunk.h \
    uartcoder.h \
    util.h \
    coder.h \
    excelengine.h \
    iapmaster.h \
    qcustomplot.h \
    serialcoder.h

FORMS    += mainwindow.ui

DISTFILES += \
    t-rex.rc

RC_FILE += t-rex.rc
