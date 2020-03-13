#ifndef MYQSQLRELATIONMODEL_CENTERDISP_H
#define MYQSQLRELATIONMODEL_CENTERDISP_H
#include<QSqlRelationalTableModel>

class MyQSqlRelationModel_Centerdisp:public QSqlRelationalTableModel
{
public:
    explicit MyQSqlRelationModel_Centerdisp();
public:
    QVariant data(const QModelIndex &item, int role) const;
};

#endif // MYQSQLRELATIONMODEL_CENTERDISP_H
