#ifndef DATABASE_SET
#define DATABASE_SET

#include<QSqlDatabase>
//#include<QSqlQuery>
static bool creatConnection()
{
    //创建链接
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");//第二个参数可以设置连接名字，这里为default
    db.setDatabaseName("./alptecdata.db");
    if(!db.open())
    {
        qDebug("Failed to connect database!");
        exit(1);
    }
    else
    {
        qDebug("Connected database.");
    }
	//db.transaction();//开启事务
	//db.commit();//提交
    return true;
}

#endif // DATABASE_SET

