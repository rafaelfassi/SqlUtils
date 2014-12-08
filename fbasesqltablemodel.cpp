#include "fbasesqltablemodel.h"

#include <QSqlDriver>
#include <QSqlField>
#include <QTimer>
#include <QThread>
#include <QSqlError>
#include <QSqlResult>

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
    : QSqlQueryModel(parent),m_db(db), m_timerFetch(0), m_busyInsertingRows(false),
      m_strategy(OnRowChange)
{
}

void FBaseSqlTableModel::initRecordAndPrimaryIndex(int tableId)
{
    SqlTableJoin &table = m_tables[tableId];
    table.baseRec = m_db.record(table.tableName);
    table.primaryIndex = m_db.primaryIndex(table.tableName);
}

QSqlRecord FBaseSqlTableModel::primaryValues(int tableId, int row) const
{
    const SqlTableJoin &table = m_tables[tableId];

    const QSqlRecord &pIndex = table.primaryIndex.isEmpty() ? table.baseRec : table.primaryIndex;

    ModifiedRow mr = m_cache.value(row);
    if (mr.op() != None)
        return mr.primaryValues(pIndex);
    else
        return QSqlQueryModel::record(row).keyValues(pIndex);
}

QVariant FBaseSqlTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || (role != Qt::DisplayRole && role != Qt::EditRole))
        return QVariant();

    const ModifiedRow mrow = m_cache.value(index.row());
    if (mrow.op() != None)
        return mrow.rec().value(index.column());

    return QSqlQueryModel::data(index, role);

    /*
    if (role == Qt::DisplayRole && index.column() >= 0 && index.column() < m_relations.count() &&
            m_relations.value(index.column()).isValid())
    {
        if(m_displayCache.contains(index.row()) && m_displayCache[index.row()].contains(index.column()))
            return m_displayCache[index.row()][index.column()];
    }

    return QSqlQueryModel::data(index, role);
    */
}

bool FBaseSqlTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (m_busyInsertingRows)
        return false;

    if (role != Qt::EditRole)
        return QSqlQueryModel::setData(index, value, role);

    if (!index.isValid() || /*index.column() >= d->rec.count() ||*/ index.row() >= rowCount())
        return false;

    if (!(flags(index) & Qt::ItemIsEditable))
        return false;

    const QVariant oldValue = data(index, role);
    if (value == oldValue
        && value.isNull() == oldValue.isNull()
        && m_cache.value(index.row()).op() != Insert)
        return true;

    ModifiedRow &row = m_cache[index.row()];

    if (row.op() == None)
        row = ModifiedRow(Update, QSqlQueryModel::record(index.row()));

    row.setValue(index.column(), value);
    emit dataChanged(index, index);

    if (m_strategy == OnFieldChange && row.op() != Insert)
        return submit();

    return true;


//    QMutexLocker locker(&m_mutex);

//    if (role == Qt::DisplayRole && index.column() > 0 && index.column() < m_relations.count()
//            && m_relations.value(index.column()).isValid())
//    {
//         m_displayCache[index.row()][index.column()] = value;
//    }

//    return QSqlQueryModel::setData(index, value, role);
}

bool FBaseSqlTableModel::submit()
{
    if (m_strategy == OnRowChange || m_strategy == OnFieldChange)
        return submitAll();
    return true;
}

bool FBaseSqlTableModel::submitAll()
{
    bool success = true;

    foreach (int row, m_cache.keys()) {
        // be sure cache *still* contains the row since overridden selectRow() could have called select()
        CacheMap::iterator it = m_cache.find(row);
        if (it == m_cache.end())
            continue;

        ModifiedRow &mrow = it.value();
        if (mrow.submitted())
            continue;

        switch (mrow.op()) {
        case Insert:
            //success = insertRowIntoTable(mrow.rec());
            break;
        case Update:
            success = updateRowInTable(row, mrow.rec());
            break;
        case Delete:
            //success = deleteRowFromTable(row);
            break;
        case None:
            Q_ASSERT_X(false, "FBaseSqlTableModel::submitAll()", "Invalid cache operation");
            break;
        }

        if (success) {
            if (m_strategy != OnManualSubmit && mrow.op() == Insert) {
                int c = mrow.rec().indexOf(m_autoColumn);
                if (c != -1 && !mrow.rec().isGenerated(c))
                    mrow.setValue(c, m_editQuery.lastInsertId());
            }
            mrow.setSubmitted();
            if (m_strategy != OnManualSubmit)
                success = selectRow(row);
        }

        if (!success)
            break;
    }

    if (success) {
        if (m_strategy == OnManualSubmit)
            success = select();
    }

    return success;
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
    const int tabId(0);

    SqlTableJoin join;
    join.tableName = table;
    join.tableAlias = Sql::relTablePrefix(m_tables.size());

    if(m_tables.isEmpty())
        m_tables.append(join);
    else
        m_tables.replace(tabId, join);

    initRecordAndPrimaryIndex(tabId);

    return tabId;
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

int FBaseSqlTableModel::addJoin(int relationTableId, const QString &relationColumn,
             const QString &tableName, const QString &indexColumn,
             Sql::JoinMode joinMode)
{
    Q_ASSERT_X(m_tables.size(), "FBaseSqlTableModel::addJoin()",
               "Necessario setar uma tabela antes de adicionar um join");

    SqlTableJoin join;
    join.relationTableId = relationTableId;
    join.relationColumn = relationColumn;
    join.tableName = tableName;
    join.tableAlias = Sql::relTablePrefix(m_tables.size());
    join.indexColumn = indexColumn;
    join.joinMode = joinMode;

    m_tables.append(join);

    const int tabId = m_tables.size() - 1;

    initRecordAndPrimaryIndex(tabId);

    return tabId;
}

int FBaseSqlTableModel::addJoin(const QString &relationColumn,
             const QString &tableName, const QString &indexColumn,
             Sql::JoinMode joinMode)
{
    int tabId = getTableIdByField(relationColumn);

    if(tabId >= 0)
        return addJoin(tabId, relationColumn, tableName, indexColumn, joinMode);

    return -1;
}

void FBaseSqlTableModel::addField(int tableId, const QString &fieldName, const QString &fieldAlias)
{
    if(tableId < m_tables.size())
        m_tables[tableId].fields.append(SqlField(fieldName, fieldAlias));
}

void FBaseSqlTableModel::addField(const QString &fieldName, const QString &fieldAlias)
{
    int tabId = getTableIdByField(fieldName);
    if(tabId >= 0) addField(tabId, fieldName, fieldAlias);
}

void FBaseSqlTableModel::addExtraField(const QString &fieldName, const QString &fieldAlias)
{
    m_extrafields.append(SqlField(fieldName, fieldAlias));
}

int FBaseSqlTableModel::getTableIdByField(const QString &fieldName)
{
    for(int i = 0; i < m_tables.size(); ++i)
    {
        const QSqlRecord &rec = m_tables.at(i).baseRec;
        for (int r = 0; r < rec.count(); ++r)
        {
            if(rec.fieldName(r) == fieldName)
            {
                return i;
            }
        }
    }
    return -1;
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
        const SqlTableJoin &joinRelTable = m_tables.at(joinTable.relationTableId);
        QString join = Sql::getJoinTables(joinRelTable.tableName, joinTable.tableName,
                                          joinRelTable.tableAlias, joinTable.tableAlias,
                                          joinTable.relationColumn, joinTable.indexColumn,
                                          joinTable.joinMode, m_db);
        from = Sql::concat(from, join);
    }

    for(int i = 0; i < m_tables.size(); ++i)
    {
        const SqlTableJoin &joinTable = m_tables.at(i);

        foreach (const SqlField &field, joinTable.fields)
        {
            const QString tableField = Sql::as(field.name,
                                               m_db.driver()->escapeIdentifier(field.alias, QSqlDriver::FieldName));

             fList = Sql::comma(fList, Sql::fullyQualifiedFieldName(joinTable.tableAlias, tableField));
        }
    }

    foreach (const SqlField &field, m_extrafields)
    {
        const QString tableField = Sql::as(field.name,
                                           m_db.driver()->escapeIdentifier(field.alias, QSqlDriver::FieldName));

         fList = Sql::comma(fList, tableField);
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
    bool result(true);
    emit beforeUpdate(row, QSqlRecord(values));

    for(int t = 0; t < m_tables.size(); ++t)
    {
        SqlTableJoin &table = m_tables[t];
        QSqlRecord rec(table.baseRec);



//        for(int r = 0; r < table.baseRec.count(); ++r)
//        {
//            qDebug() << "baseRec: " << table.baseRec.fieldName(r);
//        }

//        for(int r = 0; r < values.count(); ++r)
//        {
//            qDebug() << "values: " << values.fieldName(r);
//        }

        for(int f = 0; f < rec.count(); ++f)
        {
            rec.setGenerated(f, false);
        }

        int fieldsCount(0);
        for(int f = 0; f < values.count(); ++f)
        {
            QString name = values.fieldName(f);
            int idx = table.baseRec.indexOf(name);
            if(idx >= 0)
            {
                rec.setGenerated(idx, values.isGenerated(f));
                rec.setValue(idx, values.value(f));
                fieldsCount++;
            }

        }

        if(!fieldsCount) continue;

        const QSqlRecord whereValues = primaryValues(t, row);
        const bool prepStatement = false; //m_db.driver()->hasFeature(QSqlDriver::PreparedQueries);
        const QString stmt = m_db.driver()->sqlStatement(QSqlDriver::UpdateStatement, table.tableName,
                                                         rec, prepStatement);
        const QString where = m_db.driver()->sqlStatement(QSqlDriver::WhereStatement, table.tableName,
                                                           whereValues, prepStatement);

        if (stmt.isEmpty() || where.isEmpty() || row < 0 || row >= rowCount()) {
            setLastError(QSqlError(QLatin1String("No Fields to update"), QString(),
                                     QSqlError::StatementError));
            continue;
        }

        bool
        result = exec(Sql::concat(stmt, where), prepStatement, rec, whereValues);
        if(!result) return result;
    }

    return result;






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

bool FBaseSqlTableModel::exec(const QString &stmt, bool prepStatement,
                              const QSqlRecord &rec, const QSqlRecord &whereValues)
{
    qDebug() << "exec stmt: " << stmt;

    if (stmt.isEmpty())
        return false;

    // lazy initialization of editQuery
    if (m_editQuery.driver() != m_db.driver())
        m_editQuery = QSqlQuery(m_db);

    // workaround for In-Process databases - remove all read locks
    // from the table to make sure the editQuery succeeds
//    if (m_db.driver()->hasFeature(QSqlDriver::SimpleLocking)) TODO
//        const_cast<QSqlResult *>(query().result())->detachFromResultSet();

    if (prepStatement) {
        if (m_editQuery.lastQuery() != stmt) {
            if (!m_editQuery.prepare(stmt)) {
                setLastError(m_editQuery.lastError());
                return false;
            }
        }
        int i;
        for (i = 0; i < rec.count(); ++i)
            if (rec.isGenerated(i))
                m_editQuery.addBindValue(rec.value(i));
        for (i = 0; i < whereValues.count(); ++i)
            if (whereValues.isGenerated(i) && !whereValues.isNull(i))
                m_editQuery.addBindValue(whereValues.value(i));

        if (!m_editQuery.exec()) {
            setLastError(m_editQuery.lastError());
            return false;
        }
    } else {
        if (!m_editQuery.exec(stmt)) {
            setLastError(m_editQuery.lastError());
            return false;
        }
    }
    return true;
}

Qt::ItemFlags FBaseSqlTableModel::flags(const QModelIndex &index) const
{
    return QSqlQueryModel::flags(index) | Qt::ItemIsEditable;
}
