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

typedef struct _UnitBox{
    QString boxQRcode;
    int unitsCnt;
    QStringList unitsQRcodeList;
}UnitsBox_t;

class UnitBox{
public:
    UnitBox(){
        mBoxList.clear();
    }
    ~UnitBox(){
        mBoxList.clear();
        if( mStoreFile.isOpen()){
            mStoreFile.flush();
            mStoreFile.close();
        }
    }

    int findIndex(QString boxQRcode){
        for( int i=0 ; i< mBoxList.size(); i++){
            if( mBoxList[i].boxQRcode == boxQRcode ){
                return i;
            }
        }
        return -1;
    }

    bool append( QString boxQR, QStringList unitsQR){
        UnitsBox_t box;
        box.boxQRcode = boxQR;
        box.unitsQRcodeList.clear();
        box.unitsQRcodeList = unitsQR;
        box.unitsCnt = unitsQR.size();
        return append(box);
    }

    bool append( UnitsBox_t box){
        //if( box.unitsCnt == box.unitsQRcodeList.size()){
            if( flush(box) ){
                mBoxList.append(box);
                return true;
            }else{
                return false;
            }
        //}
    }

    bool flush(UnitsBox_t box){
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



