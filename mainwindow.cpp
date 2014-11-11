#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "modelloader.h"
#include "sqlrelationaldelegate.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow), m_model(0)
{
    ui->setupUi(this);

    m_timerFetch = new QTimer(this);
    connect(m_timerFetch, SIGNAL(timeout()), this, SLOT(fetchMore()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_btnOpen_clicked()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QOCI");
    db.setHostName("localhost");
    db.setPort(1521);
    db.setDatabaseName("ORA11");
    db.setUserName("epccq");
    db.setPassword("epccq");

    if(db.open()) qDebug("Banco de dados aberto");
    else qDebug("Falha oa abrir o banco de dados");
}

void MainWindow::on_btnRefresh_clicked()
{
    if(!m_model)
    {
        m_model = new BaseSqlTableModel(this);
        m_model->setTable("MENSAGEM_SISTEMA_IDIOMA");
        m_model->sort(m_model->record().indexOf("MSID_ID"), Qt::AscendingOrder);

        m_model->setRelation("MSID_MSGS_ID", "MENSAGEM_SISTEMA", "MSGS_ID", "MSGS_CODIGO");
        m_model->setRelation("MSID_IDIO_ID", "IDIOMA", "IDIO_ID", "IDIO_NOME");

        ui->tableView->setModel(m_model);
        ui->tableView->setItemDelegate(new SqlRelationalDelegate(ui->tableView));
    }

    m_model->select();
    m_timerFetch->start(100);
}

void MainWindow::fetchMore()
{
    m_timerFetch->stop();

    if(m_model->canFetchMore())
    {
        m_model->fetchMore();
        m_timerFetch->start();
    }
}
