#ifndef FSQLRELATION_H
#define FSQLRELATION_H

#include <QString>

class QSqlTableModel;
class QSqlQueryModel;

class FSqlRelation
{
public:
    FSqlRelation(): m_model(0), m_parent(0) {}

    void populateModel();
    void clear();
    bool isValid();

    QString relationColumn() const { return m_relationColumn; }
    QString tableName() const { return m_tableName; }
    QString indexColumn() const { return m_indexColumn; }
    QString displayColumn() const { return m_displayColumn; }
    QSqlQueryModel *model() { return m_model; }

    void setParent(QSqlTableModel *parent) { m_parent = parent; }
    void setRelationColumn(const QString &relationColumn) { m_relationColumn = relationColumn; }
    void setTableName(const QString &tableName) { m_tableName = tableName; }
    void setIndexColumn(const QString &indexColumn) { m_indexColumn = indexColumn; }
    void setDisplayColumn(const QString &displayColumn) { m_displayColumn = displayColumn; }

private:
    QSqlTableModel *m_parent;
    QSqlQueryModel *m_model;
    QString m_relationColumn;
    QString m_tableName;
    QString m_indexColumn;
    QString m_displayColumn;
};

#endif // FSQLRELATION_H
