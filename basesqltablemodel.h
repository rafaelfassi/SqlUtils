#ifndef BASESQLTABLEMODEL_H
#define BASESQLTABLEMODEL_H

#include "sqlutil.h"

#include <QSqlTableModel>
#include <QSqlRecord>
#include <QDebug>

class BaseSqlTableModel : public QSqlTableModel
{
    Q_OBJECT
public:

    explicit BaseSqlTableModel(QObject *parent = 0, QSqlDatabase db = QSqlDatabase());

    QVariant data(const QModelIndex &item, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex &item, const QVariant &value, int role = Qt::EditRole);
    virtual bool select();
    virtual bool selectRow(int row);
    virtual void setTable(const QString &tableName);
    void setRelation(const QString &relationColumn,
                     const QString &tableName,
                     const QString &indexColumn,
                     const QString &displayColumn,
                     const QString &sortColumn = QString());

    QSqlQueryModel *relationModel(int column) const;
    SqlRelation relation(int column) const;


protected:
    virtual QString selectStatement() const;
    virtual bool updateRowInTable(int row, const QSqlRecord &values);

private:

    mutable QVector<SqlRelation> m_relations;
    mutable QHash<int, QHash<int, QVariant>> m_dictionaryCache;
    QSqlRecord m_baseRec;






};



#endif // BASESQLTABLEMODEL_H
