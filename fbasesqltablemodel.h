#ifndef FBASESQLTABLEMODEL_H
#define FBASESQLTABLEMODEL_H

#include "fsqlrelation.h"
#include "sqlutil.h"

#include <QSqlQueryModel>
#include <QSqlRecord>
#include <QDebug>

class QTimer;

struct SqlTableJoin {

    SqlTableJoin()
        : joinMode(Sql::LeftJoin){}

    QString tableName;
    QString tableAlias;
    QStringList fields;
    QSqlRecord baseRec;
    QString relationColumn;
    QString indexColumn;
    Sql::JoinMode joinMode;
};


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

    explicit FBaseSqlTableModel(QObject *parent = 0, QSqlDatabase db = QSqlDatabase::database());

    QVariant data(const QModelIndex &item, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex &item, const QVariant &value, int role = Qt::EditRole);
    virtual bool select(FetchMode fetchMode = ImmediateFetch);
    virtual bool selectRow(int row);
    virtual int setTable(const QString &tableName);
    virtual bool canFetchMore(const QModelIndex &parent = QModelIndex()) const;
    virtual void fetchMore(const QModelIndex &parent = QModelIndex());
    void setRelation(const QString &relationColumn,
                     const QString &tableName,
                     const QString &indexColumn,
                     const QString &displayColumn);

    int addJoin(const QString &relationColumn,
                 const QString &tableName,
                 const QString &indexColumn,
                 Sql::JoinMode joinMode = Sql::LeftJoin);

    void addField(int tableId, const QString &fieldName);
    void addField(const QString &fieldName);

    QString filter() const;
    virtual QString orderByClause() const;

    QSqlQueryModel *relationModel(int column) const;
    FSqlRelation relation(int column) const;
    int getRelationalId(const QModelIndex &item) const;

private slots:
    void doFetchMore();

protected:
    virtual QString selectStatement() const;
    virtual bool updateRowInTable(int row, const QSqlRecord &values);

private:
    mutable QVector<FSqlRelation> m_relations;
    mutable QHash<int, QHash<int, QVariant>> m_displayCache;
    QSqlRecord m_baseRec;
    QTimer *m_timerFetch;
    mutable QMutex m_mutex;
    QList<SqlTableJoin> m_tables;
    QSqlDatabase m_db;
};



#endif // FBASESQLTABLEMODEL_H
