#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtSerialPort/QtSerialPort>
#include <QMutex>
#include "libs/uartcoder.h"
#include <libs/chunk.h>
#include <iostream>
#include "libs/iapmaster.h"
#include <QTimer>
#include <QThread>
#include "testerthread.h"

#include <QFile>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();


signals:
    void update_tester_data(int tag, unsigned char *data);
    void debug_step(bool start);
    void testThread_start(bool start);
    void testThread_exit();

private slots:
    void testerThread_sendSerialCmd(int id, unsigned char *data, int len);
    void testerThread_log(QString str);
    void testerThread_result(TesterRecord res);
    void testerThread_error(QString errorStr);

    void on_serialconnectPushButton_clicked();

    void on_serialrescanPushButton_2_clicked();

    void on_NormalradioButton_toggled(bool checked);

    void on_TestMode2radioButton_2_toggled(bool checked);

    void solt_mSerial_ReadReady();
    void on_TestMode3radioButton_3_toggled(bool checked);

    void on_Testmode4radioButton_4_toggled(bool checked);

    void on_V15PoweroncheckBox_2_clicked(bool checked);

    void on_outputtoShavercheckBox_clicked(bool checked);

    void on_outputtoResistorcheckBox_3_clicked(bool checked);

    void on_outputtoVmetercheckBox_4_clicked(bool checked);

    void on_measuretoVmetercheckBox_5_clicked(bool checked);

    void on_airpresscheckBox_clicked(bool checked);

    void on_meausureVDDcheckBox_6_clicked(bool checked);

    void on_measureLEDcheckBox_7_clicked(bool checked);

    void on_LedCapturecheckBox_3_clicked(bool checked);

    void on_measuretoVmetercheckBox_5_clicked();

    void solt_mScanerSerial_ReadReady();
    void on_scanerStartpushButton_clicked();

    void on_VmeterStartcheckBox_clicked(bool checked);

    void on_DebugpushButton_clicked();

    void on_startTestpushButton_clicked();

    void on_AutoTestClearLogpushButton_clicked();

    void on_ClearLogpushButton_2_clicked();

    void on_debugModeCheckBox_clicked(bool checked);

private:
    Ui::MainWindow *ui;
    QSerialPort *mSerialport;
    QSerialPort *mScanerserialport;
    QMutex mSerialMutex;
    UartCoder mDecoder;
    UartCoder mEncoder;
    QTimer  mTimer;
    int mRelayStatus;
    QByteArray QRcodeBytes;
    volatile int mLedBrightness_update;
    int mLedBrightness[12];
    TesterThread *mTesterThread;
    QString mQRcode;
    QFile mTxtfile;


    void update_serial_info();
    bool open_serial();
    void close_serial();
    void handle_Serial_Data(QByteArray &bytes);
    void serial_send_packget(const Chunk &chunk);
    void serial_send_PMSG(unsigned char tag, unsigned char *data, int data_len);
    void scaner_send_cmd(unsigned char *data, int len);
    void sendcmd(int tag, int data);
};

#endif // MAINWINDOW_H
