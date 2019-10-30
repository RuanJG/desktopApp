#ifndef UNITSBOX_H
#define UNITSBOX_H
#include <QMainWindow>
#include <QEvent>
#include <QKeyEvent>
#include <QApplication>
#include <QDebug>
#include <QStringList>
#include <QList>
#include <QFile>
#include <QMessageBox>
#include <QtSql/QSql>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlError>

#define POCODE_LENGTH  6 //TODO confirm ?
#define BOXCODE_LENGTH 20
#define DELIEVERY_DATE_LENGTH 8
#define BARCODE_LENGTH 13

class UnitsBox{
public:
    QString boxQRcode;
    QStringList RRCcodeList;
    int RRCcount;
    QString packDate;
    QString poCode;
    QString delieverDate;

    UnitsBox(){
        reset();
    }
    bool isEmpty()
    {
        return( RRCcount == 0);
    }
    void reset(){
        boxQRcode = "";
        RRCcount = 0;
        RRCcodeList.clear();
        packDate="";
        poCode="";
        delieverDate="";
    }

};


#endif // UNITSBOX_H
