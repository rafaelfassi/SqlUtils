#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "modelloader.h"
#include "fsqlrelationaldelegate.h"
#include "sqlutil.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow), m_model(0)
{
    ui->setupUi(this);
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

    Sql::addGlobalFilter("_CNTR_CODIGO", "Qualidade");

    if(db.open()) qDebug("Banco de dados aberto");
    else qDebug("Falha oa abrir o banco de dados");
}

void MainWindow::on_btnRefresh_clicked()
{
    if(!m_model)
    {
        m_model = new FBaseSqlTableModel(this);

        m_model->setTable("DOCUMENTO");
        m_model->addField("DCMN_CNTR_CODIGO");
        m_model->addField("DCMN_ID");
        m_model->addField("DCMN_NUMERO");
        m_model->addField("DCMN_TITULO");

        m_model->addJoin("DCMN_ID", "LISTA_SPOOL", "LISP_DCMN_ID", Sql::InnerJoin);
        m_model->addField("LISP_CNTR_CODIGO");
        m_model->addField("LISP_DCMN_ID");
        m_model->addField("LISP_PESO");
        m_model->addField("LISP_DT_RECEBIMENTO");
        m_model->addField("LISP_LOTE");

        m_model->addJoin("LISP_DMTR_ID", "DIAMETRO", "DMTR_ID");
        m_model->addField("DMTR_CNTR_CODIGO");
        m_model->addField("DMTR_ID");
        m_model->addField("DMTR_CODIGO");

        m_model->addExtraField("DCMN_NUMERO || ' - ' || LISP_LOTE", "Numero e Lote");



        //m_model->setRelation("FOSM_UNME_ID", "UNIDADE_MEDIDA", "UNME_ID", "UNME_NOME");
        //m_model->setRelation("FOSM_FCES_ID", "folha_criterio_estrutura", "FCES_ID", "FCES_SIGLA");

//        m_model->setTable("MENSAGEM_SISTEMA_IDIOMA");
//        m_model->sort(m_model->record().indexOf("MSID_ID"), Qt::AscendingOrder);

        //m_model->setRelation("MSID_MSGS_ID", "MENSAGEM_SISTEMA", "MSGS_ID", "MSGS_CODIGO");
        //m_model->setRelation("MSID_IDIO_ID", "IDIOMA", "IDIO_ID", "IDIO_NOME");

        ui->tableView->setModel(m_model);
        //ui->tableView->setItemDelegate(new FSqlRelationalDelegate(ui->tableView));
    }

    m_model->select(FBaseSqlTableModel::ManualFetch);



    //qDebug() << Sql::getGlobalFilter(m_model->tableName(), m_model->database());
}

