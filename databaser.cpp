#include "databaser.h"
#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>

DataBaser::DataBaser()
{
    mDataBase = QSqlDatabase();
}

DataBaser::~DataBaser()
{
    mDataBase.close();
}

bool DataBaser::connectDataBase(QString filename)
{
    if( mDataBase.isOpen() ) mDataBase.close();

    mDataBase = QSqlDatabase::addDatabase("QSQLITE","barcodeDB");
    mDataBase.setDatabaseName(filename);
    if( ! mDataBase.open() ){
        qDebug()<< "Error:%s"+ mDataBase.lastError().text();
        return false;
    }
    return true;
}

bool DataBaser::startQuery(QString cmd){
    QSqlQuery sql_query(mDataBase);
    return sql_query.exec(cmd);
}

bool DataBaser::isOpen()
{
    return mDataBase.isOpen();
}
