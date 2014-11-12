#include "fsqlrelation.h"
#include "sqlutil.h"
#include <QSqlTableModel>
#include <QDebug>

void FSqlRelation::populateModel()
{
    if (!isValid())
        return;
    Q_ASSERT(m_parent != NULL);

    if (!m_model)
    {
        QString query = Sql::concat(
                        Sql::concat(
                            Sql::select(Sql::comma(m_indexColumn, m_displayColumn)),
                            Sql::from(m_tableName)),
                            Sql::orderBy(m_displayColumn));

        qDebug() << query;

        m_model = new QSqlQueryModel(m_parent);
        m_model->setQuery(query, m_parent->database());
    }
}

void FSqlRelation::clear()
{
    m_model->deleteLater();
    m_model = NULL;
}

bool FSqlRelation::isValid()
{
    return (m_parent != NULL && !(m_tableName.isNull() || m_indexColumn.isNull() || m_displayColumn.isNull()));
}
