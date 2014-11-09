#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSqlRelationalTableModel>
#include <QTimer>
#include <QDebug>
#include "basesqltablemodel.h"

namespace Ui {
class MainWindow;
}

class SqlRelationalTableModel : public QSqlRelationalTableModel
{
public:
    SqlRelationalTableModel(QObject *parent = 0, QSqlDatabase db = QSqlDatabase())
        :QSqlRelationalTableModel(parent, db){}


    QString selectStatement() const
    {
        QString statement = QSqlRelationalTableModel::selectStatement();
        qDebug() << statement;
        return statement;
    }

    virtual void fetchMore(const QModelIndex &parent = QModelIndex())
    {
        //qDebug("fetchMore");
        QSqlRelationalTableModel::fetchMore(parent);
    }

};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_btnOpen_clicked();
    void on_btnRefresh_clicked();
    void fetchMore();

private:
    Ui::MainWindow *ui;
    BaseSqlTableModel *m_model;
    QTimer *m_timerFetch;
};

#endif // MAINWINDOW_H
