#ifndef MODELLOADER_H
#define MODELLOADER_H

#include <QThread>
#include <QSqlQueryModel>

class ModelLoader : public QThread
{
    Q_OBJECT
public:
    ModelLoader(QSqlQueryModel *model);
    void run();
private:
    QSqlQueryModel *m_model;

signals:
    void modelLoaded(QSqlQueryModel *model);
};

#endif // MODELLOADER_H
