#include "sqlutil.h"
#include <QDebug>

bool SqlRelation::isValid()
{
    return (m_parent != NULL); //TODO condicoes para a relacao ser valida
}

void SqlRelation::populateModel()
{
    if (!isValid())
        return;
    Q_ASSERT(m_parent != NULL);

    if (!model)
    {
        QString query = Sql::concat(Sql::select(Sql::comma(m_indexColumn, m_displayColumn)), Sql::from(m_tableName));

        qDebug() << query;

        model = new QSqlQueryModel(m_parent);
        model->setQuery(query, m_parent->database());
    }
}
