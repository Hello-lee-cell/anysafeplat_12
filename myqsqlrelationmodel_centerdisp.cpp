#include "myqsqlrelationmodel_centerdisp.h"

MyQSqlRelationModel_Centerdisp::MyQSqlRelationModel_Centerdisp()
{

}

QVariant MyQSqlRelationModel_Centerdisp::data(const QModelIndex &item, int role) const
{
    QVariant value = QSqlRelationalTableModel::data(item,role);
    if(role == Qt::TextAlignmentRole)
    {
        value = (Qt::AlignCenter);
        return value;
    }
    return value;
}
