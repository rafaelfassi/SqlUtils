#ifndef SQLUTIL_H
#define SQLUTIL_H

#include <QSqlTableModel>

class Sql
{
public:
    // SQL keywords
    inline const static QLatin1String as() { return QLatin1String("AS"); }
    inline const static QLatin1String asc() { return QLatin1String("ASC"); }
    inline const static QLatin1String comma() { return QLatin1String(","); }
    inline const static QLatin1String desc() { return QLatin1String("DESC"); }
    inline const static QLatin1String eq() { return QLatin1String("="); }
    // "and" is a C++ keyword
    inline const static QLatin1String et() { return QLatin1String("AND"); }
    inline const static QLatin1String from() { return QLatin1String("FROM"); }
    inline const static QLatin1String leftJoin() { return QLatin1String("LEFT JOIN"); }
    inline const static QLatin1String on() { return QLatin1String("ON"); }
    inline const static QLatin1String orderBy() { return QLatin1String("ORDER BY"); }
    inline const static QLatin1String parenClose() { return QLatin1String(")"); }
    inline const static QLatin1String parenOpen() { return QLatin1String("("); }
    inline const static QLatin1String select() { return QLatin1String("SELECT"); }
    inline const static QLatin1String sp() { return QLatin1String(" "); }
    inline const static QLatin1String where() { return QLatin1String("WHERE"); }

    // Build expressions based on key words
    inline const static QString as(const QString &a, const QString &b) { return b.isEmpty() ? a : concat(concat(a, as()), b); }
    inline const static QString asc(const QString &s) { return concat(s, asc()); }
    inline const static QString comma(const QString &a, const QString &b) { return a.isEmpty() ? b : b.isEmpty() ? a : QString(a).append(comma()).append(b); }
    inline const static QString concat(const QString &a, const QString &b) { return a.isEmpty() ? b : b.isEmpty() ? a : QString(a).append(sp()).append(b); }
    inline const static QString desc(const QString &s) { return concat(s, desc()); }
    inline const static QString eq(const QString &a, const QString &b) { return QString(a).append(eq()).append(b); }
    inline const static QString et(const QString &a, const QString &b) { return a.isEmpty() ? b : b.isEmpty() ? a : concat(concat(a, et()), b); }
    inline const static QString from(const QString &s) { return concat(from(), s); }
    inline const static QString leftJoin(const QString &s) { return concat(leftJoin(), s); }
    inline const static QString on(const QString &s) { return concat(on(), s); }
    inline const static QString orderBy(const QString &s) { return s.isEmpty() ? s : concat(orderBy(), s); }
    inline const static QString paren(const QString &s) { return s.isEmpty() ? s : parenOpen() + s + parenClose(); }
    inline const static QString select(const QString &s) { return concat(select(), s); }
    inline const static QString where(const QString &s) { return s.isEmpty() ? s : concat(where(), s); }

    inline const static QString relTablePrefix(int i) { return QString::number(i).prepend(QLatin1String("relTblAl_")); }

    const static QString fullyQualifiedFieldName(const QString &tableName, const QString &fieldName)
    {
        QString ret;
        ret.reserve(tableName.size() + fieldName.size() + 1);
        ret.append(tableName).append(QLatin1Char('.')).append(fieldName);
        return ret;
    }
};

class SqlRelation
{
    public:
        SqlRelation(): model(0), m_parent(0) {}

        void populateModel();
        //void clearDictionary();

        void clear();
        bool isValid();

        QString relationColumn() const { return m_relationColumn; }
        QString tableName() const { return m_tableName; }
        QString indexColumn() const { return m_indexColumn; }
        QString displayColumn() const { return m_displayColumn; }
        QString sortColumn() const { return m_sortColumn; }

        void setParent(QSqlTableModel *parent) { m_parent = parent; }
        void setRelationColumn(const QString &relationColumn) { m_relationColumn = relationColumn; }
        void setTableName(const QString &tableName) { m_tableName = tableName; }
        void setIndexColumn(const QString &indexColumn) { m_indexColumn = indexColumn; }
        void setDisplayColumn(const QString &displayColumn) { m_displayColumn = displayColumn; }
        void setSortColumn(const QString &sortColumn) { m_sortColumn = sortColumn; }

        QSqlQueryModel *model;

    private:
        QSqlTableModel *m_parent;
        QString m_relationColumn;
        QString m_tableName;
        QString m_indexColumn;
        QString m_displayColumn;
        QString m_sortColumn;
};

#endif // SQLUTIL_H
