#include "sqlutil.h"
#include <QSqlRecord>

QList<Sql::Filters> Sql::m_filters;

QString Sql::getGlobalFilter(const QString &table, QSqlDatabase db)
{
    QSqlRecord rec = db.record(table);
    QString filter;

    QList<Sql::Filters>::const_iterator i;
    for (i = m_filters.constBegin(); i != m_filters.constEnd(); ++i)
    {
        for (int x = 0; x < rec.count(); ++x)
        {
            QString &fieldName = rec.fieldName(x);

            if(fieldName.contains((*i).fieldText))
            {
                QString strValue = (*i).value.toString();

                switch((*i).value.type())
                {
                    case QVariant::Char:
                    case QVariant::String:
                    strValue = QString("'%1'").arg(strValue);
                }

                filter = et(filter, eq(fieldName, strValue));
                break;
            }
        }

    }

    return filter;

}

QString Sql::getLikeTables(const QString &table1, const QString &table2,
                           const QString &qualif1, const QString &qualif2, QSqlDatabase db)
{
    QSqlRecord rec1 = db.record(table1);
    QSqlRecord rec2 = db.record(table2);

    QString like;

    QList<Sql::Filters>::const_iterator i;
    for (i = m_filters.constBegin(); i != m_filters.constEnd(); ++i)
    {
        const QString &fieldText = (*i).fieldText;

        for (int x = 0; x < rec1.count(); ++x)
        {
            QString &fieldName1 = rec1.fieldName(x);

            if(fieldName1.contains(fieldText))
            {
                for (int y = 0; y < rec2.count(); ++y)
                {
                    QString &fieldName2 = rec2.fieldName(y);

                    if(fieldName2.contains(fieldText))
                    {
                        fieldName1 = fullyQualifiedFieldName(qualif1, fieldName1);
                        fieldName2 = fullyQualifiedFieldName(qualif2, fieldName2);
                        like = et(like, eq(fieldName1, fieldName2));
                    }
                }
            }
        }

    }

    return like;
}

QString Sql::getJoinTables(const QString &table1, const QString &table2,
                           const QString &qualif1, const QString &qualif2,
                           const QString &field1, const QString &field2,
                           JoinMode joinMode, QSqlDatabase db)
{
    QSqlRecord rec1 = db.record(table1);
    QSqlRecord rec2 = db.record(table2);

    QString like = eq(fullyQualifiedFieldName(qualif1, field1), fullyQualifiedFieldName(qualif2, field2));
    QString join;

    QList<Sql::Filters>::const_iterator i;
    for (i = m_filters.constBegin(); i != m_filters.constEnd(); ++i)
    {
        const QString &fieldText = (*i).fieldText;

        for (int x = 0; x < rec1.count(); ++x)
        {
            QString fieldName1 = rec1.fieldName(x);

            if(fieldName1.contains(fieldText))
            {
                for (int y = 0; y < rec2.count(); ++y)
                {
                    QString fieldName2 = rec2.fieldName(y);

                    if(fieldName2.contains(fieldText))
                    {
                        fieldName1 = fullyQualifiedFieldName(qualif1, fieldName1);
                        fieldName2 = fullyQualifiedFieldName(qualif2, fieldName2);
                        like = et(like, eq(fieldName1, fieldName2));
                    }
                }
            }
        }

    }

    switch (joinMode) {
    case InnerJoin:
        join = innerJoin(concat(table2, qualif2));
        break;
    case LeftJoin:
        join = leftJoin(concat(table2, qualif2));
        break;
    case RightJoin:
        join = rightJoin(concat(table2, qualif2));
        break;
    case FullJoin:
        join = fullJoin(concat(table2, qualif2));
        break;
    default:
        return QString();
    }

    join = concat(join, on(paren(like)));

    return join;
}
