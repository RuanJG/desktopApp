#ifndef DATABASEHELPER_H
#define DATABASEHELPER_H
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
#include "unitsbox.h"

class DataBaseHelper{

private:

    QSqlDatabase mDataBase;
public:
    DataBaseHelper(){
        mDataBase = QSqlDatabase();
    }
    ~DataBaseHelper(){
        close();
    }

    bool openDataBaseFile( QString filename)
    {
        static int DBcounter = 0;

        if( mDataBase.isOpen() ) mDataBase.close();

        mDataBase = QSqlDatabase::addDatabase("QSQLITE",QString("barcodeDB%1").arg(++DBcounter));
        mDataBase.setDatabaseName(filename);
        if( ! mDataBase.open() ){
            qDebug()<< "Error:%s"+ mDataBase.lastError().text();
            mDataBase = QSqlDatabase();
            QSqlDatabase::removeDatabase(QString("barcodeDB%1").arg(DBcounter));
            return false;
        }
        QSqlQuery sql_query(mDataBase);
        return sql_query.exec("create table  if not exists BarcodeTable(Boxcode text primary key, RRCcodeList text, Quantity int, PackDate text, POcode text, DelieverDate text)");
    }

    bool openDataBase(QString dbDriver , QString dbName, QString hostName, QString userName, QString password)
    {

        static int DBcounter = 0;

        if( mDataBase.isOpen() ) mDataBase.close();

        mDataBase = QSqlDatabase::addDatabase(dbDriver, QString("DB%1").arg(++DBcounter));//"QODBC");
        mDataBase.setDatabaseName(dbName);//"DRIVER={SQL SERVER};SERVER=RSERP;DATABASE=AG");
        mDataBase.setHostName(hostName);//"RSERP");
        mDataBase.setUserName(userName);//"AG");
        mDataBase.setPassword(password);//"AG123456");
        mDataBase.setConnectOptions("SQL_ATTR_LOGIN_TIMEOUT=3");
        if ( ! mDataBase.open()) {
            qDebug()<< "Error:"+ mDataBase.lastError().text();
            mDataBase = QSqlDatabase();
            QSqlDatabase::removeDatabase(QString("barcodeDB%1").arg(DBcounter));
            return false;
        }
        QSqlQuery sql_query(mDataBase);
        if( ! sql_query.exec("if OBJECT_ID(N'BarcodeTable',N'U') is null create table BarcodeTable(Boxcode varchar(50) primary key, RRCcodeList varchar(200), Quantity int, PackDate varchar(20), POcode varchar(20), DelieverDate varchar(20))") ){
            qDebug()<< "Error:%s"+ sql_query.lastError().text();
            return false;
        }
        return true;
    }

    QString getDriverName()
    {
        return mDataBase.driverName();
    }

    QString getDataBaseName(){
        return mDataBase.databaseName();
    }

    void close(){
        if( ! mDataBase.isOpen() ) return;

        QString connectName = mDataBase.connectionName();
        mDataBase.close();
        mDataBase = QSqlDatabase();
        QSqlDatabase::removeDatabase(connectName);
    }

    bool isOpen(){
        return mDataBase.isOpen();
    }

    bool sqlCmdExec(QSqlQuery &sql_query,QString cmd ){
        sql_query = QSqlQuery(mDataBase);
        if( ! sql_query.exec( cmd ) ){
            return false;
        }
        return true;
    }

    bool append( UnitsBox box)
    {
        if( mDataBase.isOpen() ){
            return saveToDataBase(box);
        }
        return false;
    }

    bool deleteBoxbyBoxcode( QString boxCode )
    {
        if( ! mDataBase.isOpen() ) return false;

        QString cmd = QString("delete from BarcodeTable where Boxcode = '%1'").arg(boxCode);
        QSqlQuery sql_query(mDataBase);
        qDebug() << "query: "+cmd;
        return sql_query.exec(cmd);
    }

    bool updateBox( UnitsBox box )
    {
        if( ! mDataBase.isOpen() ) return false;

        QString cmd = QString("update BarcodeTable set RRCcodeList = '%1', Quantity = '%2', PackDate = '%3', POcode = '%4', DelieverDate = '%5' where Boxcode = '%6'").arg(RRCcodeListToString(box),QString::number(box.RRCcount),box.packDate,box.poCode,box.delieverDate,box.boxQRcode);
        QSqlQuery sql_query(mDataBase);
        qDebug() << "query: "+cmd;
        return sql_query.exec(cmd);
    }

    QList<UnitsBox> findBoxbyBoxCode(QString boxCode)
    {
        QList<UnitsBox> boxlist;
        boxlist.clear();

        if( ! mDataBase.isOpen() ) return boxlist;

        QString cmd = QString("select * from BarcodeTable where Boxcode = '%1'").arg(boxCode);
        QSqlQuery sql_query(mDataBase);
        sql_query.setForwardOnly(true);
        qDebug() << "query: "+cmd;
        if( sql_query.exec(cmd) ){
            boxlist = SqlRecordToBoxlist(&sql_query);
        }else{
            qDebug() << "sql check failed ";
        }

        return boxlist;
    }

    QList<UnitsBox> findBoxbyUnitCode(QString unitCode)
    {
        QList<UnitsBox> boxlist;
        boxlist.clear();

        if( ! mDataBase.isOpen() ) return boxlist;

        QString cmd = QString("select * from BarcodeTable where RRCcodeList like '\%%1\%'").arg(unitCode);
        QSqlQuery sql_query(mDataBase);
        sql_query.setForwardOnly(true);
        qDebug() << "query: "+cmd;

        if( sql_query.exec(cmd) ){
            boxlist = SqlRecordToBoxlist(&sql_query);
        }else{
            qDebug() << "sql check failed ";
        }

        return boxlist;
    }


    bool exportToTxtfile(QString filename, QString POcode)
    {
        if( !mDataBase.isOpen() ) return false;

        QSqlQuery sql_query(mDataBase);

        QString cmd ;
        if( POcode == "*"){
            cmd = QString("select * from BarcodeTable");
        }else{
            cmd = QString("select * from BarcodeTable where POcode='%1'").arg(POcode);
        }
        qDebug() << cmd;
        if( !sql_query.exec(cmd) ){
            return false;
        }

        QFile file;
        file.setFileName(filename);
        if( !file.open(QIODevice::ReadWrite | QIODevice::Text) ){
            file.close();
            return false;
        }
        file.seek(file.size());

        if( file.size() == 0 ){
            QString title = "Boxcode,QTY,RRCcodeList,PackDate,POcode,DelieverDate\n";
            file.write(title.toLocal8Bit());
        }

        QString data;
        while( sql_query.next()){
            //Boxcode text primary key, RRCcodeList text, Quantity int, PackDate text, POcode text, DelieverDate
            data = sql_query.record().value("Boxcode").toString();
            data += "," + QString::number(sql_query.record().value("Quantity").toInt());
            data += "," + sql_query.record().value("RRCcodeList").toString();
            data += "," + sql_query.record().value("PackDate").toString();
            data += "," + sql_query.record().value("POcode").toString();
            data += "," + sql_query.record().value("DelieverDate").toString();
            data += "\n";
            file.write(data.toLocal8Bit());
        }

        file.flush();
        file.close();

        return true;

    }

    QString RRCcodeListToString(UnitsBox box)
    {
        QString RRCcodeList;
        RRCcodeList = "";
        for( int i=0; i< box.RRCcount; i++){
            if( i> 0) RRCcodeList += ",";
            RRCcodeList+= box.RRCcodeList[i];
        }
        return RRCcodeList;
    }

private:



    bool saveToDataBase(UnitsBox box)
    {
        QString RRCcodeList;
        QSqlQuery sql_query(mDataBase);
        sql_query.setForwardOnly(true);

        if( !mDataBase.isOpen() ) return false;

        RRCcodeList = RRCcodeListToString(box);

        //Boxcode text primary key, RRCcodeList text, Quantity int, PackDate text, POcode text, DelieverDate
        QString cmd = QString("insert into BarcodeTable values('%1','%2',%3,'%4','%5','%6')").arg(box.boxQRcode,RRCcodeList,QString::number(box.RRCcount),box.packDate,box.poCode,box.delieverDate);
        if( !sql_query.exec(cmd)){
            qDebug()<< "sql ERROR : "+sql_query.lastError().text();
            return false;
        }
        return true;
    }


    QList<UnitsBox> SqlRecordToBoxlist(QSqlQuery*sql_query)
    {
        bool ok;
        QList<UnitsBox> boxlist;

        while(sql_query->next()) {
            UnitsBox box;
            box.boxQRcode = sql_query->record().value("Boxcode").toString();
            box.RRCcodeList = sql_query->record().value("RRCcodeList").toString().split(',');
            box.RRCcount = sql_query->record().value("Quantity").toInt(&ok);
            if( !ok ) box.RRCcount = -1;
            box.packDate = sql_query->record().value("PackDate").toString();
            box.poCode =  sql_query->record().value("POcode").toString();
            box.delieverDate =  sql_query->record().value("DelieverDate").toString();
            boxlist.append(box);
        }
        return boxlist;
    }
};


#endif // DATABASEHELPER_H
