#include "fbasesqltablemodel.h"

#include <QSqlDriver>
#include <QSqlField>
#include <QSqlQuery>
#include <QTimer>
#include <QThread>
#include <QSqlError>

class ModelLoader : public QThread
{
public:
    ModelLoader(FBaseSqlTableModel *model) :
        QThread(0), m_model(model) {}
    void run()
    {
        while(m_model->canFetchMore())
        {
            m_model->fetchMore();
            sleep(1);
        }
        this->deleteLater();
    }

private:
    FBaseSqlTableModel *m_model;
};

FBaseSqlTableModel::FBaseSqlTableModel(QObject *parent, QSqlDatabase db)
    : QSqlQueryModel(parent),m_db(db), m_timerFetch(0)
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

    return QSqlQueryModel::data(index, role);
}

bool FBaseSqlTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    QMutexLocker locker(&m_mutex);

    if (role == Qt::DisplayRole && index.column() > 0 && index.column() < m_relations.count()
            && m_relations.value(index.column()).isValid())
    {
         m_displayCache[index.row()][index.column()] = value;
    }

    return QSqlQueryModel::setData(index, value, role);
}

bool FBaseSqlTableModel::select(FetchMode fetchMode)
{
    m_displayCache.clear();

    const QString query = selectStatement();

    if (query.isEmpty())
        return false;

    bool ok(true);

    beginResetModel();

    //d->clearCache();

    QSqlQuery qu(query, m_db);
    setQuery(qu);

    if (!qu.isActive() || lastError().isValid()) {
        // something went wrong - revert to non-select state
        //d->initRecordAndPrimaryIndex();
        ok = false;
    }
    endResetModel();

    if(ok)
    {
        switch (fetchMode)
        {
        case ImmediateFetch:
            {
                while(canFetchMore())
                    fetchMore();
            }
            break;
        case LazyFetch:
            {
                if(!m_timerFetch)
                {
                    m_timerFetch = new QTimer(this);
                    connect(m_timerFetch, SIGNAL(timeout()), this, SLOT(doFetchMore()));
                }
                m_timerFetch->start(50);
            }
            break;
        case ParallelFetch:
            {
                ModelLoader *modelLoader = new ModelLoader(this);
                modelLoader->start();
            }
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
   return false;
//    m_displayCache.remove(row);
//    return QSqlQueryModel::selectRow(row);
}

int FBaseSqlTableModel::setTable(const QString &table)
{  
    SqlTableJoin join;
    join.tableName = table;
    join.baseRec = m_db.record(table);
    join.tableAlias = Sql::relTablePrefix(m_tables.size());

    if(m_tables.isEmpty())
        m_tables.append(join);
    else
        m_tables.replace(0, join);

    return 0;
}

bool FBaseSqlTableModel::canFetchMore(const QModelIndex &parent) const
{
    return QSqlQueryModel::canFetchMore(parent);
}

void FBaseSqlTableModel::fetchMore(const QModelIndex &parent)
{
    QMutexLocker locker(&m_mutex);
    QSqlQueryModel::fetchMore(parent);
}

void FBaseSqlTableModel::setRelation(const QString &relationColumn,
                                                 const QString &tableName,
                                                 const QString &indexColumn,
                                                 const QString &displayColumn)
{
//    int column = record().indexOf(relationColumn);

//    if (column < 0)
//        return;


//    FSqlRelation sqlRelation;
//    sqlRelation.setParent(this);
//    sqlRelation.setRelationColumn(relationColumn);
//    sqlRelation.setTableName(tableName);
//    sqlRelation.setIndexColumn(indexColumn);
//    sqlRelation.setDisplayColumn(displayColumn);

//    if (m_relations.size() <= column)
//        m_relations.resize(column + 1);

//    m_relations[column] = sqlRelation;
}

int FBaseSqlTableModel::addJoin(const QString &relationColumn,
             const QString &tableName,
             const QString &indexColumn,
             Sql::JoinMode joinMode)
{
    Q_ASSERT_X(m_tables.size(), "FBaseSqlTableModel::addJoin()",
               "Necessario setar uma tabela antes de adicionar um join");

    SqlTableJoin join;
    join.relationColumn = relationColumn;
    join.tableName = tableName;
    join.tableAlias = Sql::relTablePrefix(m_tables.size());
    join.indexColumn = indexColumn;
    join.joinMode = joinMode;
    join.baseRec = m_db.record(tableName);

    m_tables.append(join);
    return m_tables.size() - 1;
}

void FBaseSqlTableModel::addField(int tableId, const QString &fieldName)
{
    if(tableId < m_tables.size())
        m_tables[tableId].fields.append(fieldName);
}

void FBaseSqlTableModel::addField(const QString &fieldName)
{
    for(int i = 0; i < m_tables.size(); ++i)
    {
        const QSqlRecord &rec = m_tables.at(i).baseRec;
        for (int r = 0; r < rec.count(); ++r)
        {
            if(rec.fieldName(r) == fieldName)
            {
                addField(i, fieldName);
                return;
            }
        }
    }
}

QString FBaseSqlTableModel::filter() const
{
    return QString();
}

QString FBaseSqlTableModel::orderByClause() const
{
    return QString();
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
//    int column = item.column();
//    if (column < 0 || column >= m_relations.count())
//        return 0;

//    if(m_relations.value(item.column()).isValid())
//    {
//        FSqlRelation &relation = m_relations.value(column);

//        QString strFilter = m_db.driver()->sqlStatement(QSqlDriver::WhereStatement,
//                                                                          tableName(),
//                                                                          primaryValues(item.row()),
//                                                                          false);

//        QString strQuery = Sql::concat(
//                           Sql::concat(
//                                Sql::select(relation.relationColumn()),
//                                Sql::from(tableName())),
//                                strFilter);

//        QSqlQuery query(strQuery);
//        if(query.next())
//            return query.value(0).toInt();
//    }
    return 0;
}

QString FBaseSqlTableModel::selectStatement() const
{
    if (m_tables.isEmpty())
        return QString();

    const SqlTableJoin &mainTable = m_tables.at(0);

    QString fList;
    QString conditions;
    QString from = Sql::concat(Sql::from(mainTable.tableName), mainTable.tableAlias);

    for(int i = 1; i < m_tables.size(); ++i)
    {
        const SqlTableJoin &joinTable = m_tables.at(i);
        QString join = Sql::getJoinTables(mainTable.tableName, joinTable.tableName,
                                          mainTable.tableAlias, joinTable.tableAlias,
                                          joinTable.relationColumn, joinTable.indexColumn,
                                          joinTable.joinMode, m_db);
        from = Sql::concat(from, join);
    }

    for(int i = 0; i < m_tables.size(); ++i)
    {
        const SqlTableJoin &joinTable = m_tables.at(i);

        foreach (const QString &name, joinTable.fields)
        {
            const QString tableField = m_db.driver()->escapeIdentifier(name, QSqlDriver::FieldName);

             fList = Sql::comma(fList, Sql::fullyQualifiedFieldName(joinTable.tableAlias, tableField));
        }
    }


    if (fList.isEmpty())
        return QString();

    conditions = Sql::getGlobalFilter(mainTable.tableName, mainTable.tableAlias, m_db);
    const QString stmt = Sql::concat(Sql::select(fList), from);
    const QString where = Sql::where(Sql::et(Sql::paren(conditions), Sql::paren(filter())));

    QString strQueryResult = Sql::concat(Sql::concat(stmt, where), orderByClause());

    qDebug() << strQueryResult;



    return strQueryResult;
}

bool FBaseSqlTableModel::updateRowInTable(int row, const QSqlRecord &values)
{
    return false;
//    QSqlRecord rec = values;

//    for (int i = 0; i < rec.count(); ++i)
//    {
//        if (m_relations.value(i).isValid())
//        {
//            QVariant v = rec.value(i);
//            bool gen = rec.isGenerated(i);
//            rec.replace(i, m_baseRec.field(i));
//            rec.setValue(i, v);
//            rec.setGenerated(i, gen);
//        }
//    }

//    return QSqlTableModel::updateRowInTable(row, rec);
}
