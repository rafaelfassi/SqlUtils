#include "fbasesqltablemodel.h"
#include "sqlutil.h"

#include <QSqlDriver>
#include <QSqlField>
#include <QSqlQuery>
#include <QTimer>

FBaseSqlTableModel::FBaseSqlTableModel(QObject *parent, QSqlDatabase db)
    : QSqlTableModel(parent, db), m_timerFetch(0)
{
}

QVariant FBaseSqlTableModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole && index.column() >= 0 && index.column() < m_relations.count() &&
            m_relations.value(index.column()).isValid())
    {
        if(m_displayCache.contains(index.row()) && m_displayCache[index.row()].contains(index.column()))
            return m_displayCache[index.row()][index.column()];
    }
    return QSqlTableModel::data(index, role);
}

bool FBaseSqlTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role == Qt::DisplayRole && index.column() > 0 && index.column() < m_relations.count()
            && m_relations.value(index.column()).isValid())
    {
         m_displayCache[index.row()][index.column()] = value;
    }
    return QSqlTableModel::setData(index, value, role);
}

bool FBaseSqlTableModel::select(FetchMode fetchMode)
{
    m_displayCache.clear();

    bool ok = QSqlTableModel::select();

    if(ok)
    {
        switch (fetchMode)
        {
        case ImmediateFetch:
            while(canFetchMore())
                fetchMore();
            break;
        case LazyFetch:
            if(!m_timerFetch)
            {
                m_timerFetch = new QTimer(this);
                connect(m_timerFetch, SIGNAL(timeout()), this, SLOT(doFetchMore()));
            }
            m_timerFetch->start(50);
        }
    }

    return ok;
}

void FBaseSqlTableModel::doFetchMore()
{
    m_timerFetch->stop();

    if(canFetchMore())
    {
        fetchMore();
        m_timerFetch->start();
    }
}

bool FBaseSqlTableModel::selectRow(int row)
{
    m_displayCache.remove(row);
    return QSqlTableModel::selectRow(row);
}

void FBaseSqlTableModel::setTable(const QString &table)
{
    // memorize the table before applying the relations
    m_baseRec = database().record(table);
    QSqlTableModel::setTable(table);
}

void FBaseSqlTableModel::setRelation(const QString &relationColumn,
                                                 const QString &tableName,
                                                 const QString &indexColumn,
                                                 const QString &displayColumn)
{
    int column = record().indexOf(relationColumn);

    if (column < 0)
        return;


    FSqlRelation sqlRelation;
    sqlRelation.setParent(this);
    sqlRelation.setRelationColumn(relationColumn);
    sqlRelation.setTableName(tableName);
    sqlRelation.setIndexColumn(indexColumn);
    sqlRelation.setDisplayColumn(displayColumn);

    if (m_relations.size() <= column)
        m_relations.resize(column + 1);

    m_relations[column] = sqlRelation;
}

QSqlQueryModel *FBaseSqlTableModel::relationModel(int column) const
{
    if (column < 0 || column >= m_relations.count())
        return 0;

    FSqlRelation &relation = m_relations[column];
    if (!relation.isValid())
        return 0;

    if (!relation.model())
        relation.populateModel();
    return relation.model();
}

FSqlRelation FBaseSqlTableModel::relation(int column) const
{
    if (column < 0 || column >= m_relations.count())
        return FSqlRelation();
    return m_relations.value(column);
}

int FBaseSqlTableModel::getRelationalId(const QModelIndex &item) const
{
    int column = item.column();
    if (column < 0 || column >= m_relations.count())
        return 0;

    if(m_relations.value(item.column()).isValid())
    {
        FSqlRelation &relation = m_relations.value(column);

        QString strFilter = database().driver()->sqlStatement(QSqlDriver::WhereStatement,
                                                                          tableName(),
                                                                          primaryValues(item.row()),
                                                                          false);

        QString strQuery = Sql::concat(
                           Sql::concat(
                                Sql::select(relation.relationColumn()),
                                Sql::from(tableName())),
                                strFilter);

        QSqlQuery query(strQuery);
        if(query.next())
            return query.value(0).toInt();
    }
    return 0;
}

QString FBaseSqlTableModel::selectStatement() const
{
    if (tableName().isEmpty())
        return QString();
    if (m_relations.isEmpty())
        return QSqlTableModel::selectStatement();

    // Count how many times each field name occurs in the record
    QHash<QString, int> fieldNames;
    QStringList fieldList;
    for (int i = 0; i < m_baseRec.count(); ++i)
    {
        FSqlRelation relation = m_relations.value(i);
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
            name = m_baseRec.fieldName(i);
        }
        fieldNames[name] = fieldNames.value(name, 0) + 1;
        fieldList.append(name);
    }

    QString fList;
    QString conditions;
    QString from = Sql::from(tableName());
    for (int i = 0; i < m_baseRec.count(); ++i)
    {
        FSqlRelation relation = m_relations.value(i);
        const QString tableField = Sql::fullyQualifiedFieldName(tableName(), database().driver()->escapeIdentifier(m_baseRec.fieldName(i), QSqlDriver::FieldName));
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
            const QString gLikeTables = Sql::getLikeTables(tableName(), relation.tableName(), database());
            const QString cond = Sql::et(Sql::eq(tableField, relTableField), gLikeTables);

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

    conditions = Sql::getGlobalFilter(tableName(), database());
    const QString stmt = Sql::concat(Sql::select(fList), from);
    const QString where = Sql::where(Sql::et(Sql::paren(conditions), Sql::paren(filter())));

    QString strQueryResult = Sql::concat(Sql::concat(stmt, where), orderByClause());
    qDebug() << strQueryResult;
    return strQueryResult;
}

bool FBaseSqlTableModel::updateRowInTable(int row, const QSqlRecord &values)
{
    QSqlRecord rec = values;

    for (int i = 0; i < rec.count(); ++i)
    {
        if (m_relations.value(i).isValid())
        {
            QVariant v = rec.value(i);
            bool gen = rec.isGenerated(i);
            rec.replace(i, m_baseRec.field(i));
            rec.setValue(i, v);
            rec.setGenerated(i, gen);
        }
    }

    return QSqlTableModel::updateRowInTable(row, rec);
}
