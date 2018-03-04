#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtSerialPort/QtSerialPort>
#include <QMutex>
#include <uartcoder.h>
#include <chunk.h>
#include <iostream>
#include "iapmaster.h"
#include <QTimer>


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow,IapMaster
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_pushButton_3_clicked();

    void on_SerialButton_clicked();

    void solt_mSerial_ReadReady();

    void on_ClearTextBrowButton_clicked();

    void iapTickHandler();

    void on_iapButton_clicked();

    void on_fileChooseButton_clicked();

    void on_sendpushButton_clicked();

private:
    Ui::MainWindow *ui;
    QSerialPort *mSerialport;
    QMutex mSerialMutex;
    UartCoder mDecoder;
    UartCoder mEncoder;
    QTimer  mTimer;
    QTimer  mtestTimer;

    void update_serial_info();
    void close_serial();
    bool open_serial();
    bool open_serial_dev(QString portname);
    void handle_Serial_Data(QByteArray &bytes);
    void handle_device_message(const unsigned char *data, int len);
    void serial_send_packget(const Chunk &chunk);
    void iapSendBytes(unsigned char *data, size_t len);
    void iapEvent(int EventType, std::string value);
    void iapResetDevice();
    void stopIap();
    bool startIap();
    void initIap();

    QString settingRead();
    void settingWrite(QString data);
};














#endif // MAINWINDOW_H
