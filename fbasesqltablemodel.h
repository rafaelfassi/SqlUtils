#ifndef FBASESQLTABLEMODEL_H
#define FBASESQLTABLEMODEL_H

#include "fsqlrelation.h"
#include "sqlutil.h"

#include <QSqlQueryModel>
#include <QSqlRecord>
#include <QSqlIndex>
#include <QDebug>
#include <QSqlQuery>

class QTimer;




class FBaseSqlTableModel : public QSqlQueryModel
{
    Q_OBJECT
public:

    enum FetchMode {
        ImmediateFetch,
        LazyFetch,
        ParallelFetch,
        ManualFetch
    };

    enum Op { None, Insert, Update, Delete };

    enum EditStrategy {OnFieldChange, OnRowChange, OnManualSubmit};

    struct SqlField {
        SqlField() : flags(0){}
        SqlField(const QString &_name, const QString &_alias, int _flags = 0)
            : name(_name), alias(_alias), flags(_flags){}

        QString name;
        QString alias;
        int flags;
    };

    struct SqlTableJoin {
        SqlTableJoin() : joinMode(Sql::LeftJoin){}

        QString tableName;
        QString tableAlias;
        QList<SqlField> fields;
        QSqlRecord baseRec;
        QString relationColumn;
        QString indexColumn;
        Sql::JoinMode joinMode;
        int relationTableId;
        QSqlIndex primaryIndex;
    };

    class ModifiedRow
    {
    public:
        inline ModifiedRow(Op o = None, const QSqlRecord &r = QSqlRecord())
            : m_op(None), m_db_values(r), m_insert(o == Insert)
        { setOp(o); }
        inline Op op() const { return m_op; }
        inline void setOp(Op o)
        {
            if (o == None)
                m_submitted = true;
            if (o == m_op)
                return;
            m_submitted = (o != Insert && o != Delete);
            m_op = o;
            m_rec = m_db_values;
            setGenerated(m_rec, m_op == Delete);
        }
        inline QSqlRecord rec() const { return m_rec; }
        inline QSqlRecord& recRef() { return m_rec; }
        inline void setValue(int c, const QVariant &v)
        {
            m_submitted = false;
            m_rec.setValue(c, v);
            m_rec.setGenerated(c, true);
        }
        inline bool submitted() const { return m_submitted; }
        inline void setSubmitted()
        {
            m_submitted = true;
            setGenerated(m_rec, false);
            if (m_op == Delete) {
                m_rec.clearValues();
            }
            else {
                m_op = Update;
                m_db_values = m_rec;
                setGenerated(m_db_values, true);
            }
        }
        inline void refresh(bool exists, const QSqlRecord& newvals)
        {
            m_submitted = true;
            if (exists) {
                m_op = Update;
                m_db_values = newvals;
                m_rec = newvals;
                setGenerated(m_rec, false);
            } else {
                m_op = Delete;
                m_rec.clear();
                m_db_values.clear();
            }
        }
        inline bool insert() const { return m_insert; }
        inline void revert()
        {
            if (m_submitted)
                return;
            if (m_op == Delete)
                m_op = Update;
            m_rec = m_db_values;
            setGenerated(m_rec, false);
            m_submitted = true;
        }
        inline QSqlRecord primaryValues(const QSqlRecord& pi) const
        {
            if (m_op == None || m_op == Insert)
                return QSqlRecord();

            return m_db_values.keyValues(pi);
        }
    private:
        inline static void setGenerated(QSqlRecord& r, bool g)
        {
            for (int i = r.count() - 1; i >= 0; --i)
                r.setGenerated(i, g);
        }
        Op m_op;
        QSqlRecord m_rec;
        QSqlRecord m_db_values;
        bool m_submitted;
        bool m_insert;
    };

    typedef QMap<int, ModifiedRow> CacheMap;


    explicit FBaseSqlTableModel(QObject *parent = 0, QSqlDatabase db = QSqlDatabase::database());

    QVariant data(const QModelIndex &item, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex &item, const QVariant &value, int role = Qt::EditRole);
    virtual bool submit();
    virtual bool submitAll();
    virtual bool select(FetchMode fetchMode = ImmediateFetch);
    virtual bool selectRow(int row);
    virtual int setTable(const QString &tableName);
    virtual bool canFetchMore(const QModelIndex &parent = QModelIndex()) const;
    virtual void fetchMore(const QModelIndex &parent = QModelIndex());
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
    void setRelation(const QString &relationColumn,
                     const QString &tableName,
                     const QString &indexColumn,
                     const QString &displayColumn);

    int addJoin(int relationTableId,
                const QString &relationColumn,
                const QString &tableName,
                const QString &indexColumn,
                Sql::JoinMode joinMode = Sql::LeftJoin);

    int addJoin(const QString &relationColumn,
                const QString &tableName,
                const QString &indexColumn,
                Sql::JoinMode joinMode = Sql::LeftJoin);

    void addField(int tableId, const QString &fieldName, const QString &fieldAlias = QString());
    void addField(const QString &fieldName, const QString &fieldAlias = QString());
    void addExtraField(const QString &fieldName, const QString &fieldAlias = QString());
    QSqlRecord primaryValues(int tableId, int row) const;

    QString filter() const;
    virtual QString orderByClause() const;

    QSqlQueryModel *relationModel(int column) const;
    FSqlRelation relation(int column) const;
    int getRelationalId(const QModelIndex &item) const;

private slots:
    void doFetchMore();

Q_SIGNALS:
    void primeInsert(int row, QSqlRecord &record);
    void beforeInsert(QSqlRecord &record);
    void beforeUpdate(int row, QSqlRecord &record);
    void beforeDelete(int row);

protected:
    virtual QString selectStatement() const;
    virtual bool updateRowInTable(int row, const QSqlRecord &values);
    int getTableIdByField(const QString &fieldName);

private:
    void initRecordAndPrimaryIndex(int tableId);
    bool exec(const QString &stmt, bool prepStatement,
              const QSqlRecord &rec, const QSqlRecord &whereValues);

    mutable QVector<FSqlRelation> m_relations;
    mutable QHash<int, QHash<int, QVariant>> m_displayCache;
    QSqlRecord m_baseRec;
    QTimer *m_timerFetch;
    mutable QMutex m_mutex;
    QList<SqlTableJoin> m_tables;
    QSqlDatabase m_db;
    QList<SqlField> m_extrafields;
    CacheMap m_cache;
    bool m_busyInsertingRows;
    EditStrategy m_strategy;
    QString m_autoColumn;
    QSqlQuery m_editQuery;
};



#endif // FBASESQLTABLEMODEL_H
