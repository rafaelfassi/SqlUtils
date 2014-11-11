#include "sqlutil.h"
#include <QDebug>

bool SqlRelation::isValid()
{
    return (m_parent != NULL && !(m_tableName.isNull() || m_indexColumn.isNull() || m_displayColumn.isNull()));
}

void SqlRelation::populateModel()
{
    if (!isValid())
        return;
    Q_ASSERT(m_parent != NULL);

    if (!model)
    {
        QString query = Sql::concat(
                        Sql::concat(
                            Sql::select(Sql::comma(m_indexColumn, m_displayColumn)),
                            Sql::from(m_tableName)),
                            Sql::orderBy(m_displayColumn));

        qDebug() << query;

        model = new QSqlQueryModel(m_parent);
        model->setQuery(query, m_parent->database());
    }
}
