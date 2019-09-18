#ifndef DATABASER_H
#define DATABASER_H


#include <QtSql/QSql>
#include <QtSql/QSqlDatabase>


class DataBaser
{
public:
    DataBaser();
    ~DataBaser();

    bool connectDataBase(QString filename);
    bool isOpen();
    bool startQuery(QString cmd);
private:
    QSqlDatabase mDataBase;

};

#endif // DATABASER_H
