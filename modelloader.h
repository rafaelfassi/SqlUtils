#ifndef MODELLOADER_H
#define MODELLOADER_H

#include <QThread>
#include <QSqlTableModel>

class ModelLoader : public QThread
{
    Q_OBJECT
public:
    ModelLoader(QSqlTableModel *model);
    void run();
private:
    QSqlTableModel *m_model;
};

#endif // MODELLOADER_H
