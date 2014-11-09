#include "modelloader.h"

ModelLoader::ModelLoader(QSqlTableModel *model)
    : QThread(0)
{
    m_model = model;
}

void ModelLoader::run()
{
    m_model->select();

    while (m_model->canFetchMore()) {
        m_model->fetchMore();
    }
}
