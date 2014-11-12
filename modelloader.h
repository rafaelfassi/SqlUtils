#ifndef MODELLOADER_H
#define MODELLOADER_H

#include <QThread>
#include <QSqlQueryModel>

class ModelLoader : public QThread
{
    Q_OBJECT
public:
    ModelLoader(QSqlQueryModel *model) :
        QThread(0), m_model(model) {}
    void run()
    {
        while(m_model->canFetchMore())
        {
            m_model->fetchMore();
            sleep(1);
        }
        this->deleteLater();
    }

signals:
    void modelLoaded(QSqlQueryModel *model);

private:
    QSqlQueryModel *m_model;
};

#endif // MODELLOADER_H
