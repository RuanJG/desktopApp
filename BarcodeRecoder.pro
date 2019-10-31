#-------------------------------------------------
#
# Project created by QtCreator 2019-09-11T15:29:47
#
#-------------------------------------------------

QT       += core gui sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = BarcodeRecoder
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h \
    unitsbox.h \
    databasehelper.h

FORMS    += mainwindow.ui

RC_ICONS = icon.ico
