#include "basesqltablemodel.h"
#include <QSqlDriver>
#include <QSqlRecord>


BaseSqlTableModel::BaseSqlTableModel(QObject *parent, QSqlDatabase db)
    : QSqlTableModel(parent, db)
{
}

void BaseSqlTableModel::setRelation(const QString &relationColumn,
                                                 const QString &tableName,
                                                 const QString &indexColumn,
                                                 const QString &displayColumn,
                                                 const QString &sortColumn)
{
    int column = record().indexOf(relationColumn);

    if (column < 0)
        return;


    SqlRelation sqlRelation;
    sqlRelation.setParent(this);
    sqlRelation.setRelationColumn(relationColumn);
    sqlRelation.setTableName(tableName);
    sqlRelation.setIndexColumn(indexColumn);
    sqlRelation.setDisplayColumn(displayColumn);
    sqlRelation.setSortColumn(sortColumn);

    if (m_relations.size() <= column)
        m_relations.resize(column + 1);

    m_relations[column] = sqlRelation;
}

QSqlQueryModel *BaseSqlTableModel::relationModel(int column) const
{
    if ( column < 0 || column >= m_relations.count())
        return 0;

    SqlRelation &relation = m_relations[column];
    if (!relation.isValid())
        return 0;

    if (!relation.model)
        relation.populateModel();
    return relation.model;
}

SqlRelation BaseSqlTableModel::relation(int column) const
{
    return m_relations.value(column);
}


QString BaseSqlTableModel::selectStatement() const
{
    if (tableName().isEmpty())
        return QString();
    if (m_relations.isEmpty())
        return QSqlTableModel::selectStatement();

    // Count how many times each field name occurs in the record
    QHash<QString, int> fieldNames;
    QStringList fieldList;
    for (int i = 0; i < record().count(); ++i)
    {
        SqlRelation relation = m_relations.value(i);
        QString name;
        if (relation.isValid())
        {
            // Count the display column name, not the original foreign key
            name = relation.displayColumn();
            if (database().driver()->isIdentifierEscaped(name, QSqlDriver::FieldName))
                name = database().driver()->stripDelimiters(name, QSqlDriver::FieldName);

            const QSqlRecord rec = database().record(relation.tableName());
            for (int i = 0; i < rec.count(); ++i) {
                if (name.compare(rec.fieldName(i), Qt::CaseInsensitive) == 0) {
                    name = rec.fieldName(i);
                    break;
                }
            }
        }
        else {
            name = record().fieldName(i);
        }
        fieldNames[name] = fieldNames.value(name, 0) + 1;
        fieldList.append(name);
    }

    QString fList;
    QString conditions;
    QString from = Sql::from(tableName());
    for (int i = 0; i < record().count(); ++i)
    {
        SqlRelation relation = m_relations.value(i);
        const QString tableField = Sql::fullyQualifiedFieldName(tableName(), database().driver()->escapeIdentifier(record().fieldName(i), QSqlDriver::FieldName));
        if (relation.isValid())
        {
            const QString relTableAlias = Sql::relTablePrefix(i);
            QString displayTableField = Sql::fullyQualifiedFieldName(relTableAlias, relation.displayColumn());

            // Duplicate field names must be aliased
            if (fieldNames.value(fieldList[i]) > 1)
            {
                QString relTableName = relation.tableName().section(QChar::fromLatin1('.'), -1, -1);
                if (database().driver()->isIdentifierEscaped(relTableName, QSqlDriver::TableName))
                    relTableName = database().driver()->stripDelimiters(relTableName, QSqlDriver::TableName);
                QString displayColumn = relation.displayColumn();
                if (database().driver()->isIdentifierEscaped(displayColumn, QSqlDriver::FieldName))
                    displayColumn = database().driver()->stripDelimiters(displayColumn, QSqlDriver::FieldName);
                const QString alias = QString::fromLatin1("%1_%2_%3").arg(relTableName).arg(displayColumn).arg(fieldNames.value(fieldList[i]));
                displayTableField = Sql::as(displayTableField, alias);
                --fieldNames[fieldList[i]];
            }

            fList = Sql::comma(fList, displayTableField);

            // Join related table
            const QString tblexpr = Sql::concat(relation.tableName(), relTableAlias);
            const QString relTableField = Sql::fullyQualifiedFieldName(relTableAlias, relation.indexColumn());
            const QString cond = Sql::eq(tableField, relTableField);

            from = Sql::concat(from, Sql::leftJoin(tblexpr));
            from = Sql::concat(from, Sql::on(cond));
        }
        else
        {
            fList = Sql::comma(fList, tableField);
        }
    }

    if (fList.isEmpty())
        return QString();

    const QString stmt = Sql::concat(Sql::select(fList), from);
    const QString where = Sql::where(Sql::et(Sql::paren(conditions), Sql::paren(filter())));

    QString strQueryResult = Sql::concat(Sql::concat(stmt, where), orderByClause());
    qDebug() << strQueryResult;
    return strQueryResult;
}
