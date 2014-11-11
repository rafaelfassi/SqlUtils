#ifndef SQLRELATIONALDELEGATE_H
#define SQLRELATIONALDELEGATE_H

#include "basesqltablemodel.h"

#include <QtWidgets/qitemdelegate.h>
#include <QtWidgets/qlistview.h>
#include <QtWidgets/qcombobox.h>
#include <QSqlRecord>


class SqlRelationalDelegate: public QItemDelegate
{
public:

explicit SqlRelationalDelegate(QObject *aParent = 0)
    : QItemDelegate(aParent)
{}

~SqlRelationalDelegate()
{}

QWidget *createEditor(QWidget *aParent,
                      const QStyleOptionViewItem &option,
                      const QModelIndex &index) const
{
    qDebug("createEditor");
    const BaseSqlTableModel *sqlModel = qobject_cast<const BaseSqlTableModel *>(index.model());
    QSqlQueryModel *childModel = sqlModel ? sqlModel->relationModel(index.column()) : 0;
    if (!childModel)
        return QItemDelegate::createEditor(aParent, option, index);

    while(childModel->canFetchMore())
        childModel->fetchMore();

    QComboBox *combo = new QComboBox(aParent);
    combo->setModel(childModel);
    combo->setModelColumn(childModel->record().indexOf(sqlModel->relation(index.column()).displayColumn()));
    combo->installEventFilter(const_cast<SqlRelationalDelegate *>(this));

    return combo;
}

void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    qDebug("setModelData");
    if (!index.isValid())
        return;

    BaseSqlTableModel *sqlModel = qobject_cast<BaseSqlTableModel *>(model);
    QSqlQueryModel *childModel = sqlModel ? sqlModel->relationModel(index.column()) : 0;
    QComboBox *combo = qobject_cast<QComboBox *>(editor);
    if (!sqlModel || !childModel || !combo) {
        QItemDelegate::setModelData(editor, model, index);
        return;
    }

    int currentItem = combo->currentIndex();
    int childColIndex = childModel->record().indexOf(sqlModel->relation(index.column()).displayColumn());
    int childEditIndex = childModel->record().indexOf(sqlModel->relation(index.column()).indexColumn());
    sqlModel->setData(index,
            childModel->data(childModel->index(currentItem, childColIndex), Qt::DisplayRole),
            Qt::DisplayRole);
    sqlModel->setData(index,
            childModel->data(childModel->index(currentItem, childEditIndex), Qt::EditRole),
            Qt::EditRole);
}

};

#endif // SQLRELATIONALDELEGATE_H
