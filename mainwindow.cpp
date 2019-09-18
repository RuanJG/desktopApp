#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QEvent>
#include <QDir>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    mStepIndex(0),
    mBoxes()
{
    ui->setupUi(this);
    ui->indicatelabel_3->setFont(QFont("Microsoft YaHei", 20, 75));
    ui->textBrowser->setFont(QFont("Microsoft YaHei", 10, 50));
    QPalette pa;
    pa.setColor(QPalette::WindowText,Qt::red);
    ui->indicatelabel_3->setPalette(pa);

    mCurrentBox.boxQRcode="";
    mCurrentBox.unitsQRcodeList.clear();
    mCurrentBox.unitsCnt = 0;
    updateTable();
    updateStep();

    QDir mdir;
    QString filedirPath = QDir::currentPath()+"/Data";
    QString filename =QDir::toNativeSeparators( filedirPath+"/"+"Barcode_Record.txt" );
    QString dbfilename =QDir::toNativeSeparators( filedirPath+"/"+"Barcode_Record.db" );
    mdir.mkpath(filedirPath);

    if( !mBoxes.setFilename(filename) ){
        QMessageBox::warning(this,"Error",tr("打开数据文档失败"));
    }
    if( !mBoxes.setDataBaseFile(dbfilename) ){
        QMessageBox::warning(this,"Error",tr("打开数据库失败"));
    }

}

MainWindow::~MainWindow()
{
    delete ui;
}

MainWindow::updateTable()
{
    ui->textBrowser->clear();
    if( mCurrentBox.boxQRcode.length()>0 )
        ui->textBrowser->append(tr("包装箱条形码：")+mCurrentBox.boxQRcode+"\n");
    for( int i=0 ;i< mCurrentBox.unitsQRcodeList.size(); i++){
        ui->textBrowser->append(tr("成品")+QString::number(i+1)+"  条形码:  "+mCurrentBox.unitsQRcodeList[i]+"\n");
    }
}

MainWindow::updateStep()
{
    switch( mStepIndex ){
    case 0:
        ui->indicatelabel_3->setText(tr("扫描包装箱的条形码"));
        break;

    case 1:
        ui->indicatelabel_3->setText(tr("扫描第")+QString::number(mCurrentBox.unitsQRcodeList.size()+1)+"个产品的条形码");
        break;
    case 2:
        ui->indicatelabel_3->setText(tr("将以上")+QString::number(mCurrentBox.unitsCnt)+tr("个产品打包"));
        break;
    case 3:
        ui->indicatelabel_3->setText(tr("保存数据失败，重启软件，重新扫描"));
        break;
    }
}

bool MainWindow::eventFilter(QObject *watched, QEvent *e)
{
    if(e->type() == QEvent::KeyPress)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(e);
        mStringData.append(keyEvent->text().toLocal8Bit());
        //qDebug()<< mStringData.toHex();
        if( mStepIndex == 0){
            // 0x2 [20] 0x4
            int index = mStringData.indexOf(0x2);
            if( index > 0 ) mStringData.remove(0,index);
            if ( index == -1 ) mStringData.clear();

            if( mStringData.length()>= 22){
                if( mStringData.at(21)==0x04){
                    mStringData.remove(0,1);
                    mStringData.remove(20,mStringData.length()-20);
                    mCurrentBox.boxQRcode = QString::fromLocal8Bit(mStringData);
                    mCurrentBox.unitsCnt = 0;
                    mCurrentBox.unitsQRcodeList.clear();
                    mStepIndex++;
                    updateStep();
                    updateTable();
                }
                mStringData.clear();
            }
        }else if( mStepIndex == 1){
            // 0x2 [13] 0x4
            int index = mStringData.indexOf(0x2);
            if( index > 0 ) mStringData.remove(0,index);
            if ( index == -1 ) mStringData.clear();

            if( mStringData.length() >= 15 ){
                if( mStringData.at(14)==0x04){
                    mStringData.remove(0,1);
                    mStringData.remove(13,mStringData.length()-13);
                    QString barcode = QString::fromLocal8Bit(mStringData);
                    if( 0 > mCurrentBox.unitsQRcodeList.indexOf(barcode) ){
                        mCurrentBox.unitsQRcodeList.append( QString::fromLocal8Bit(mStringData) );
                        mCurrentBox.unitsCnt++;
                    }
                    updateTable();
                    updateStep();
                    mStringData.clear();
                    if( mCurrentBox.unitsQRcodeList.size() >= ui->unitsCountspinBox->value()){
                        mCurrentBox.unitsCnt = mCurrentBox.unitsQRcodeList.size();
                        if( ! mBoxes.append(mCurrentBox) ){
                            QMessageBox::warning(this,"Error",tr("保存数据失败，重启软件，重新扫描"));
                            mStepIndex = 3;
                            updateStep();
                        }else{
                            mStepIndex = 2;
                            updateStep();
                            QTimer::singleShot(5000, this, SLOT(slot_Start_Packing()));
                            //mMsgbox.setText(tr("将以上")+QString::number(mCurrentBox.unitsCnt)+tr("个产品打包, 再继续扫码"));
                            //mMsgbox.setWindowTitle(tr("提示"));
                            //mMsgbox.show();
                        }
                    }
                }else{
                    mStringData.clear();
                }
            }
        }

        return true;
    }else{
        return QMainWindow::eventFilter(watched,e);
    }
}

void MainWindow::slot_Start_Packing()
{
    //mMsgbox.close();
    mStepIndex = 0;
    updateStep();

    mCurrentBox.boxQRcode.clear();
    mCurrentBox.unitsQRcodeList.clear();
    updateTable();
}

void MainWindow::on_reducepushButton_clicked()
{
    int value = ui->unitsCountspinBox->value();
    if( value > 1)
        ui->unitsCountspinBox->setValue(value-1);
    updateTable();
}

void MainWindow::on_AddpushButton_2_clicked()
{
    int value = ui->unitsCountspinBox->value();
    ui->unitsCountspinBox->setValue(value+1);
    updateTable();
}
