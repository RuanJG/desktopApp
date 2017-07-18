#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtSerialPort/QtSerialPort>
#include <QMutex>
#include <uartcoder.h>
#include <chunk.h>
#include "excelengine.h"


// packget  head tag
#define USER_DATA_TAG	1
#define USER_LOG_TAG	2
#define USER_START_TAG  3
#define USER_CMD_CHMOD_TAG  4
#define USER_CMD_CURRENT_MAXMIN_TAG 5
#define USER_CMD_VOICE_MAXMIN_TAG   6

//packget body : result error value
#define USER_RES_CURRENT_FALSE_FLAG 1
#define USER_RES_VOICE_FALSE_FLAG 2
#define USER_RES_ERROR_FLAG 4

//work_mode value
#define USER_MODE_ONLINE 1
#define USER_MODE_OFFLINE 2


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void closeEvent(QCloseEvent *event);

private slots:
    void on_save_close_Button_clicked();

    void on_pushButton_3_clicked();

    void on_SerialButton_clicked();

    void solt_mSerial_ReadReady();

    void on_setcurrentButton_clicked();

    void on_setVolumeButton_clicked();

    void on_ClearTextBrowButton_clicked();

    void on_restartButton_clicked();

private:
    Ui::MainWindow *ui;
    QSerialPort *mSerialport;
    QMutex mSerialMutex;
    UartCoder mDecoder;
    UartCoder mEncoder;
    ExcelEngine mExcel; //创建excl对象
    unsigned int mExcelCurrentRow;

    void update_serial_info();
    void close_serial();
    bool open_serial();
    bool open_serial_dev(QString portname);
    void handle_Serial_Data(QByteArray &bytes);
    void handle_device_message(const unsigned char *data, int len);
    void serial_send_packget(const Chunk &chunk);
    void saveRecordToExcel(int db, QString current, int count, int error);
};

#endif // MAINWINDOW_H
