#-------------------------------------------------
#
# Project created by QtCreator 2019-08-26T22:22:50
#
#-------------------------------------------------

QT       += core gui network
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
    testerthread.h \
    D:/Qt/Qt5.6.2/5.6/mingw49_32/include/QtCore/qt_windows.h

FORMS    += mainwindow.ui
