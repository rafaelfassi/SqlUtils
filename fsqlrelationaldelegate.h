#ifndef FSQLRELATIONALDELEGATE_H
#define FSQLRELATIONALDELEGATE_H

#include "fbasesqltablemodel.h"

#include <QtWidgets/qitemdelegate.h>
#include <QtWidgets/qlistview.h>
#include <QtWidgets/qcombobox.h>
#include <QSqlRecord>
#include <QDebug>


class FSqlRelationalDelegate: public QItemDelegate
{
public:

explicit FSqlRelationalDelegate(QObject *aParent = 0)
    : QItemDelegate(aParent)
{}

~FSqlRelationalDelegate()
{}

QWidget *createEditor(QWidget *aParent,
                      const QStyleOptionViewItem &option,
                      const QModelIndex &index) const
{
    qDebug("createEditor");
    const FBaseSqlTableModel *sqlModel = qobject_cast<const FBaseSqlTableModel *>(index.model());
    QSqlQueryModel *childModel = sqlModel ? sqlModel->relationModel(index.column()) : 0;
    if (!childModel)
        return QItemDelegate::createEditor(aParent, option, index);

    while(childModel->canFetchMore())
        childModel->fetchMore();

    QComboBox *combo = new QComboBox(aParent);
    combo->setModel(childModel);
    //combo->setEditable(true);
    combo->setModelColumn(childModel->record().indexOf(sqlModel->relation(index.column()).displayColumn()));
    combo->installEventFilter(const_cast<FSqlRelationalDelegate *>(this));

    return combo;
}

void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    qDebug("setModelData");
    if (!index.isValid())
        return;

    FBaseSqlTableModel *sqlModel = qobject_cast<FBaseSqlTableModel *>(model);
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

#endif // FSQLRELATIONALDELEGATE_H
