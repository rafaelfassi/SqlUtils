#ifndef BASESQLTABLEMODEL_H
#define BASESQLTABLEMODEL_H

#include "sqlutil.h"

#include <QSqlTableModel>
#include <QDebug>

class BaseSqlTableModel : public QSqlTableModel
{
    Q_OBJECT
public:

    explicit BaseSqlTableModel(QObject *parent = 0, QSqlDatabase db = QSqlDatabase());

    void setRelation(const QString &relationColumn,
                     const QString &tableName,
                     const QString &indexColumn,
                     const QString &displayColumn,
                     const QString &sortColumn = QString());

    QSqlQueryModel *relationModel(int column) const;
    SqlRelation relation(int column) const;


protected:
    virtual QString selectStatement() const;

private:

    mutable QVector<SqlRelation> m_relations;






};



#endif // BASESQLTABLEMODEL_H
