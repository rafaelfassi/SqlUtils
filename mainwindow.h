#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSqlRelationalTableModel>
#include <QTimer>
#include <QDebug>
#include "fbasesqltablemodel.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_btnOpen_clicked();
    void on_btnRefresh_clicked();

private:
    Ui::MainWindow *ui;
    FBaseSqlTableModel *m_model;
};

#endif // MAINWINDOW_H
