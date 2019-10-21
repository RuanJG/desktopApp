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
#include "databasehelper.h"



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

    void on_dbFileChoisepushButton_2_clicked();

    void on_dbImportpushButton_clicked();

    void on_boxlistfilechoisepushButton_3_clicked();

    void on_boxlisttxtfileimportpushButton_4_clicked();

    void on_boxcheckpushButton_clicked();

private:
    Ui::MainWindow *ui;
    volatile int mStepIndex;
    DataBaseHelper mBarcodeDataBase;
    UnitsBox mCurrentBox;
    QByteArray mStringData;
    QMessageBox mMsgbox;
    void updateTable();
    void updateStep();
};










#endif // MAINWINDOW_H



