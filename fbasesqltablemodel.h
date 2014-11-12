#ifndef FBASESQLTABLEMODEL_H
#define FBASESQLTABLEMODEL_H

#include "fsqlrelation.h"

#include <QSqlTableModel>
#include <QSqlRecord>
#include <QDebug>

class QTimer;

class FBaseSqlTableModel : public QSqlTableModel
{
    Q_OBJECT
public:

    enum FetchMode {
        AllFetch,
        LazyFetch,
        ManualFetch
    };

    explicit FBaseSqlTableModel(QObject *parent = 0, QSqlDatabase db = QSqlDatabase());

    QVariant data(const QModelIndex &item, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex &item, const QVariant &value, int role = Qt::EditRole);
    virtual bool select(FetchMode fetchMode = AllFetch);
    virtual bool selectRow(int row);
    virtual void setTable(const QString &tableName);
    void setRelation(const QString &relationColumn,
                     const QString &tableName,
                     const QString &indexColumn,
                     const QString &displayColumn);

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
};



#endif // FBASESQLTABLEMODEL_H
