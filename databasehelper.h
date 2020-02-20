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

class DataBaseHelper{

private:
    QSqlDatabase mDataBase;
    QStringList mTableItems;
    QString mTableName;


public:
    DataBaseHelper(){
        mDataBase = QSqlDatabase();
        mTableItems = QStringList();
    }
    ~DataBaseHelper(){
        close();
    }

    enum ERROR_TYPE{
        OK = 0,
        FILE_ERROR  = -1,
        SQL_CMD_ERROR = -2
    };

    //TableItems , all item is text , the first item is the key
    bool openSqliteDataBase( QString filename, QString tableName, QStringList tableItems)
    {
        static int DBcounter = 0;

        if( mDataBase.isOpen() ) mDataBase.close();

        mDataBase = QSqlDatabase::addDatabase("QSQLITE",QString("sqliteDB%1").arg(++DBcounter));
        mDataBase.setDatabaseName(filename);
        if( ! mDataBase.open() ){
            qDebug()<< "Error:%s"+ mDataBase.lastError().text();
            mDataBase = QSqlDatabase();
            QSqlDatabase::removeDatabase(QString("sqliteDB%1").arg(DBcounter));
            return false;
        }

        mTableName = tableName;
        mTableItems = tableItems;

        QSqlQuery sql_query;
        //"create table  if not exists BarcodeTable(Boxcode text primary key, RRCcodeList text, Quantity int, PackDate text, POcode text, DelieverDate text)"
        QString cmd = "create table  if not exists "+mTableName+"(";
        for( int i=0; i<tableItems.size(); i++){
            if( i == 0 ){
                cmd += QString(tableItems[i]+" text primary key");
            }else{
                cmd += QString(", "+tableItems[i]+" text");
            }
        }
        cmd += ")";
        qDebug() << cmd;
        if( ! execSqlcmd(sql_query,cmd) ){
            close();
            return false;
        }
        return true;
    }

    bool openODBCDataBase(QString dbName, QString hostName, QString userName, QString password, QString tableName, QStringList tableItems, QStringList tableItemsStringSize)
    {

        static int DBcounter = 0;

        if( mDataBase.isOpen() ) mDataBase.close();

        mDataBase = QSqlDatabase::addDatabase("QODBC", QString("ODBCDB%1").arg(++DBcounter));//"QODBC");
        mDataBase.setDatabaseName(dbName);//"DRIVER={SQL SERVER};SERVER=RSERP;DATABASE=AG");
        mDataBase.setHostName(hostName);//"RSERP");
        mDataBase.setUserName(userName);//"AG");
        mDataBase.setPassword(password);//"AG123456");
        mDataBase.setConnectOptions("SQL_ATTR_LOGIN_TIMEOUT=5");
        if ( ! mDataBase.open()) {
            qDebug()<< "Error:"+ mDataBase.lastError().text();
            mDataBase = QSqlDatabase();
            QSqlDatabase::removeDatabase(QString("ODBCDB%1").arg(DBcounter));
            return false;
        }
        //QSqlQuery sql_query(mDataBase);
        //if( ! sql_query.exec("if OBJECT_ID(N'BarcodeTable',N'U') is null create table BarcodeTable(Boxcode varchar(50) primary key, RRCcodeList varchar(200), Quantity int, PackDate varchar(20), POcode varchar(20), DelieverDate varchar(20))") ){
        //    qDebug()<< "Error:%s"+ sql_query.lastError().text();
        //    return false;
        //}

        mTableName = tableName;
        mTableItems = tableItems;

        if( mTableItems.size() != tableItemsStringSize.size())
            return false;

        QSqlQuery sql_query;
        //"if OBJECT_ID(N'BarcodeTable',N'U') is null create table BarcodeTable(Boxcode varchar(50) primary key, RRCcodeList varchar(200), Quantity int, PackDate varchar(20), POcode varchar(20), DelieverDate varchar(20))"
        QString cmd = QString( "if OBJECT_ID(N'%1',N'U') is null create table %2(").arg(mTableName).arg(mTableName);
        for( int i=0; i<mTableItems.size(); i++){
            if( i == 0 ){
                cmd += QString("%1 varchar(%2) primary key").arg(mTableItems[i]).arg(tableItemsStringSize[i]);
            }else{
                cmd += QString(", %1 varchar(%2)").arg(mTableItems[i]).arg(tableItemsStringSize[i]);
            }
        }
        cmd += ")";
        qDebug() << cmd;
        if( ! execSqlcmd(sql_query,cmd) ){
            close();
            return false;
        }
        return true;
    }

    void close(){
        if( ! mDataBase.isOpen() ) return;
        QString connectName = mDataBase.connectionName();
        mDataBase.close();
        mDataBase = QSqlDatabase();
        QSqlDatabase::removeDatabase(connectName);
    }

    bool execSqlcmd(QSqlQuery &sql_query,QString cmd ){
        sql_query = QSqlQuery(mDataBase);
        return sql_query.exec( cmd );
    }

    QString getDriverName(){
        return mDataBase.driverName();
    }

    QString getDataBaseName(){
        return mDataBase.databaseName();
    }

    QStringList getTableItems(){
        return mTableItems;
    }

    QString getTableName(){
        return mTableName;
    }

    bool isOpen(){
        return mDataBase.isOpen();
    }

    bool isRecordExist( QString keyValue ){
        if( ! mDataBase.isOpen() ) return false;

        QString cmd = QString("select %1 from %2 where %3 = '%4'").arg(mTableItems[0]).arg(mTableName).arg(mTableItems[0]).arg(keyValue);

        QSqlQuery sql_query(mDataBase);
        //sql_query.setForwardOnly(true);
        qDebug() << "isRecordExist: "+cmd;
        if( sql_query.exec(cmd) ){
            if( sql_query.next()){
                return true;
            }
        }else{
            qDebug() << "isRecordExist: sql exec failed ";
        }
        return false;
    }

    QStringList getRecord( QString keyValue){
        QStringList record;

        if( ! mDataBase.isOpen() ) return record;

        QString cmd = QString("select * from %1 where %2 = '%3'").arg(mTableName).arg(mTableItems[0]).arg(keyValue);

        QSqlQuery sql_query(mDataBase);
        sql_query.setForwardOnly(true);
        qDebug() << "getRecord: "+cmd;
        if( sql_query.exec(cmd) ){
            if( sql_query.next()){
                for( int i=0; i< mTableItems.size(); i++){
                    record.append(sql_query.record().value(mTableItems[i]).toString());
                }
            }
        }else{
            qDebug() << "getRecord: sql exec failed ";
        }
        return record;
    }



    QList<QStringList> getRecord( QString key  , QString value){
        QList<QStringList> recordList;
        QStringList record;

        if( ! mDataBase.isOpen() ) return recordList;

        QString cmd = QString("select * from %1 where %2 = '%3'").arg(mTableName).arg(key).arg(value);

        QSqlQuery sql_query(mDataBase);
        sql_query.setForwardOnly(true);
        qDebug() << "getRecord: "+cmd;
        if( sql_query.exec(cmd) ){
            while( sql_query.next()){
                record = QStringList();
                for( int i=0; i< mTableItems.size(); i++){
                    record.append(sql_query.record().value(mTableItems[i]).toString());
                }
                recordList.append(record);
            }
        }else{
            qDebug() << "getRecord: sql exec failed ";
        }
        return recordList;
    }



    QList<QStringList> getRecord()
    {
        QList<QStringList> recordList;

        if( ! mDataBase.isOpen() ) return recordList;

        QString cmd = QString("select * from %1").arg(mTableName);

        QSqlQuery sql_query(mDataBase);
        sql_query.setForwardOnly(true);
        qDebug() << "getRecords: "+cmd;
        if( sql_query.exec(cmd) ){
            while( sql_query.next() ){
                QStringList record;
                for( int i=0; i< mTableItems.size(); i++){
                    record.append(sql_query.record().value(mTableItems[i]).toString());
                }
                recordList.append(record);
            }
        }else{
            qDebug() << "getRecords: sql exec failed ";
        }
        return recordList;
    }

    bool appendRecord(QStringList record){

        if( ! mDataBase.isOpen() ) return false;

        QSqlQuery sql_query(mDataBase);
        //sql_query.setForwardOnly(true);

        //"insert into BarcodeTable values('%1','%2',%3,'%4','%5','%6')"
        QString cmd = QString("insert into %1 values(").arg(mTableName);
        for( int i=0; i<mTableItems.size(); i++){
            if( i == 0 ){
                cmd += QString("'%1'").arg(record[i]);
            }else{
                cmd += QString(",'%1'").arg(record[i]);
            }
        }
        cmd += ")";

        if( !sql_query.exec(cmd)){
            qDebug()<< "sql ERROR : "+sql_query.lastError().text();
            return false;
        }
        return true;
    }

    bool updateRecord( QStringList record ){
        if( ! mDataBase.isOpen() ) return false;

        QSqlQuery sql_query(mDataBase);
        //sql_query.setForwardOnly(true);

        //"update BarcodeTable set RRCcodeList = '%1', Quantity = '%2', PackDate = '%3', POcode = '%4', DelieverDate = '%5' where Boxcode = '%6'"
        QString cmd = QString("update %1 set ").arg(mTableName);
        for( int i=1; i<mTableItems.size(); i++){
            if( i == 1 ){
                cmd += QString("%1 = '%2'").arg(mTableItems[i]).arg(record[i]);
            }else{
                cmd += QString(", %1 = '%2'").arg(mTableItems[i]).arg(record[i]);
            }
        }
        cmd += QString(" where %1 = '%2'").arg(mTableItems[0]).arg(record[0]);

        if( !sql_query.exec(cmd)){
            qDebug()<< "sql ERROR : "+sql_query.lastError().text();
            return false;
        }
        return true;
    }

    bool flushRecord(QStringList record)
    {
        if( record.size() != mTableItems.size()){
            return false;
        }

        if(isRecordExist(record[0])){
            return updateRecord(record);
        }else{
            return appendRecord(record);
        }
    }

    bool deleteRecord( QString key , QString value)
    {
        if( ! mDataBase.isOpen() ) return false;

        //delete all data :  "delete from 'table'"
        QString cmd = QString("delete from %1 where %2 = '%3'").arg(mTableName).arg(key).arg(value);
        QSqlQuery sql_query(mDataBase);
        qDebug() << "query: "+cmd;
        return sql_query.exec(cmd);
    }


    bool exportToFile(QString filename, QString splitStr)
    {
        return exportToFile( filename,splitStr, QString("select * from %1").arg(mTableName));
    }


    bool exportToFile(QString filename, QString splitStr, QString sqlcmd)
    {
        if( !mDataBase.isOpen() ) return false;

        QSqlQuery sql_query(mDataBase);
        sql_query.setForwardOnly(true);
        if( !sql_query.exec(sqlcmd) ){
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
            //Boxcode;QTY;RRCcodeList;PackDate;POcode;DelieverDate\n
            QString title = "";
            for( int i=0; i< mTableItems.size(); i++){
                title += QString("%1%2").arg(mTableItems[i]).arg(splitStr);
            }
            title += "\n";
            file.write(title.toLocal8Bit());
        }

        QString data;
        while( sql_query.next()){
            data = "";
            for( int i=0; i< mTableItems.size(); i++){
                data += QString("%1%2").arg(sql_query.record().value(mTableItems[i]).toString()).arg(splitStr);
            }
            data += "\n";
            file.write(data.toLocal8Bit());
        }

        file.flush();
        file.close();

        return true;

    }

    ERROR_TYPE importFromFile(QString filename, QString splitStr)
    {
        QFile file;
        file.setFileName(filename);
        if( !file.open(QIODevice::ReadWrite | QIODevice::Text) ){
            file.close();
            return FILE_ERROR;
        }
        file.seek(0);

        QStringList record;
        QList<QStringList> recordList;

        while( file.pos() < file.size() )
        {
            record = QString(file.readLine()).split(QRegExp("["+splitStr+"\n\r]"),QString::SkipEmptyParts);
            if( record.size() <= 0){
                continue;
            }
            if( record[0] == mTableItems[0]){
                //this is title
                continue;
            }
            if( record.size() == mTableItems.size()){
                recordList.append(record);
            }else{
                //unknow data , let user confirm
                recordList.clear();
                file.close();
                return FILE_ERROR;
            }
        }

        foreach (QStringList r , recordList) {
            if( ! flushRecord(r)){
                file.close();
                return SQL_CMD_ERROR;
            }
        }
        recordList.clear();

        return OK;
    }










};


#endif // DATABASEHELPER_H
