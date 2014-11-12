#include "modelloader.h"

ModelLoader::ModelLoader(QSqlQueryModel *model)
    : QThread(0)
{
    m_model = model;
}

void ModelLoader::run()
{
    while (m_model->canFetchMore()) {
        m_model->fetchMore();
    }

    emit modelLoaded(m_model);
}
