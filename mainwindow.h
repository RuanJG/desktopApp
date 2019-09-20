#ifndef MAINWINDOW_H
#define MAINWINDOW_H

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

typedef struct _UnitBox{
    QString boxQRcode;
    int unitsCnt;
    QStringList unitsQRcodeList;
}UnitsBox_t;

class UnitBox{

private:
    QSqlDatabase mDataBase;
public:
    UnitBox(){
        mBoxList.clear();
        mDataBase = QSqlDatabase();
    }
    ~UnitBox(){
        mBoxList.clear();
        if( mStoreFile.isOpen()){
            mStoreFile.flush();
            mStoreFile.close();
        }
    }

    bool findbox(QString boxQRcode , QString &record)
    {
        if( mDataBase.isOpen() ){
            QSqlQuery sql_query(mDataBase);
            sql_query.setForwardOnly(true);

            QString cmd = QString("select unitsBarcode from barcode where boxBarcode = \"%1\"").arg(boxQRcode);
            qDebug() << "query: "+cmd;
            if( sql_query.exec(cmd) ){
                record = "";
                if (sql_query.next()) {
                    record = sql_query.record().value("unitsBarcode").toString();
                }
                return true;
            }else{
                qDebug() << "sql check failed ";
                return false;
            }
        }
        return false;
    }

    bool append( UnitsBox_t box)
    {
        if( mStoreFile.fileName().length() > 0){
            mBoxList.append(box);
            return saveToFile(box);
        }
        if( mDataBase.isOpen() ){
            return saveToDataBase(box);
        }
        return false;
    }
    bool setFilename( QString filename )
    {
        if( mStoreFile.isOpen()){
            mStoreFile.flush();
            mStoreFile.close();
        }
        mStoreFile.setFileName(filename);
        if( !mStoreFile.open(QIODevice::ReadWrite | QIODevice::Text) ){
            //QMessageBox::warning(this,"Error","打开数据文档失败");
            mStoreFile.close();
            return false;
        }
        mStoreFile.close();
        return true;
    }

    bool setDataBaseFile( QString filename)
    {
        if( mDataBase.isOpen() ) mDataBase.close();

        mDataBase = QSqlDatabase::addDatabase("QSQLITE","barcodeDB");
        mDataBase.setDatabaseName(filename);
        if( ! mDataBase.open() ){
            qDebug()<< "Error:%s"+ mDataBase.lastError().text();
            return false;
        }
        QSqlQuery sql_query(mDataBase);
        sql_query.exec("create table barcode(boxBarcode text primary key, unitsBarcode text)");
        return true;
    }

    bool exportToTxtfile(QString filename)
    {
        if( !mDataBase.isOpen() ) return false;

        QSqlQuery sql_query(mDataBase);
        if( !sql_query.exec("select * from barcode") ){
            return false;
        }

        QFile file;
        file.setFileName(filename);
        if( !file.open(QIODevice::ReadWrite | QIODevice::Text) ){
            file.close();
            return false;
        }
        file.seek(file.size());

        QString data;
        while( sql_query.next()){
            data = sql_query.record().value("boxBarcode").toString()+"," + sql_query.record().value("unitsBarcode").toString()+"\n";
            file.write(data.toLocal8Bit());
        }

        file.close();

        return true;

    }


private:
    bool saveToFile(UnitsBox_t box)
    {
        QString record;

        record = box.boxQRcode;
        record += (","+QString::number(box.unitsCnt));
        for( int i=0; i< box.unitsCnt; i++){
            record+= (","+box.unitsQRcodeList[i] );
        }
        record+="\n";

        if( !mStoreFile.open(QIODevice::ReadWrite | QIODevice::Text) ){
            mStoreFile.close();
            return false;
        }
        mStoreFile.seek(mStoreFile.size());
        mStoreFile.write(record.toLocal8Bit());
        mStoreFile.flush();
        mStoreFile.close();
        return true;
    }
    bool saveToDataBase(UnitsBox_t box)
    {
        QString record;
        QSqlQuery sql_query(mDataBase);
        sql_query.setForwardOnly(true);

        if( !mDataBase.isOpen() ) return false;

        record = QString::number(box.unitsCnt);
        for( int i=0; i< box.unitsCnt; i++){
            record+= (","+box.unitsQRcodeList[i] );
        }

        QString cmd = QString("select unitsBarcode from barcode where boxBarcode = \"%1\"").arg(box.boxQRcode);
        qDebug() << "query: "+cmd;
        if( sql_query.exec(cmd) ){
            if (sql_query.next()) {
                qDebug() << "sql check has record :"+ sql_query.record().value("unitsBarcode").toString();
                //TODO ask user
                if( sql_query.exec(QString("delete from barcode where boxBarcode = \"%1\"").arg(box.boxQRcode)) ){
                    qDebug() << "sql delete ok";
                }else{
                    qDebug() << "sql delete false";
                    return false;
                }
            }else{
                qDebug() << "sql check no record ";
            }
        }else{
            qDebug() << "sql check failed ";
            return false;
        }


        cmd = QString("insert into barcode values(\"%1\",\"%2\")").arg(box.boxQRcode,record);
        qDebug() << "query: "+cmd;
        if( !sql_query.exec(cmd )){
            qDebug()<< "sql ERROR : "+sql_query.lastError().text();
            return false;
        }

        return true;
    }
private:
    QFile mStoreFile;
    QList<UnitsBox_t> mBoxList;

};

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();


    bool eventFilter(QObject *watched, QEvent *event);

public slots:
    void slot_Start_Packing();
private slots:
    void on_reducepushButton_clicked();

    void on_AddpushButton_2_clicked();

    void on_exportpushButton_clicked();

private:
    Ui::MainWindow *ui;
    volatile int mStepIndex;
    UnitBox mBoxes;
    UnitsBox_t mCurrentBox;
    QByteArray mStringData;
    QMessageBox mMsgbox;
    updateTable();
    updateStep();
};










#endif // MAINWINDOW_H



