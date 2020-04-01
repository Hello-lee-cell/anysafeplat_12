#include <QFile>
#include <QDebug>
#include <QTextCodec>
#include <QDir>
#include <fcntl.h>
#include <unistd.h>
#include <QSqlTableModel>
#include <qstyle.h>
#include <QScrollBar>
#include <QProcess>
#include "history.h"
#include "ui_history.h"
#include "config.h"

unsigned char Flag_AL_ENV = 0;  //条件查询 表选择   0：油枪数据  1：环境数据

history::history(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::history)
{
    ui->setupUi(this);
    Flag_Timeto_CloseNeeded[0] = 1;
    this->setAttribute(Qt::WA_DeleteOnClose,true);
  //  setAttribute(Qt::WA_TranslucentBackground,true);    //窗体透明
    setWindowFlags(Qt::Tool|Qt::WindowStaysOnTopHint|Qt::FramelessWindowHint);

//	if(Flag_screen_zaixian == 1){on_pushButton_gasre_oilgun_clicked();}
//	else if(Flag_screen_xielou == 1){on_pushButton_xielou_warn_clicked();}
//	else if(Flag_screen_radar == 1){on_pushButton_radar_clicked();}
//	else if((Flag_screen_safe==1)||(Flag_screen_burngas==1)){on_pushButton_jingdian_clicked();}
//	else if(Flag_screen_burngas == 1){on_pushButton_gaswarn_clicked();}

	ui->tabWidget->setTabEnabled(0,Flag_screen_zaixian);
	ui->tabWidget->setTabEnabled(1,Flag_screen_xielou);
	ui->tabWidget->setTabEnabled(2,Flag_screen_radar);
	if((Flag_screen_safe==0)&&(Flag_screen_burngas==0))
	{
		ui->tabWidget->setTabEnabled(3,0);
	}
	else
	{
		ui->tabWidget->setTabEnabled(3,1);
	}
	ui->tabWidget->setTabEnabled(4,Flag_screen_burngas);
	ui->tabWidget->setStyleSheet("QTabBar::tab:abled {min-height:40px;max-width:150px;background-color: rgb(170,170,255,255);border: 1px solid;padding:4px;}\
	                             QTabBar::tab:!selected {margin-top: 0px;background-color:transparent;}\
	                             QTabBar::tab:selected {background-color: white}\
	                             QTabWidget::pane {border-top:0px solid #e8f3f9;background:  transparent;}\
	                             QTabBar::tab:disabled {width: 0; height: 0; color: transparent;padding:0px;border: 0px solid}");


	ui->tableView->verticalScrollBar()->setStyleSheet( "QScrollBar:vertical{ background: #F0F0F0; width:30px ;margin-top:0px;margin-bottom:0px }"
                                                       "QScrollBar::handle:vertical{ background: #6c65c8; border-radius:3px; min-height: 80px ;width:30px }");
	ui->tableView->horizontalScrollBar()->setStyleSheet("QScrollBar:horizontal{ background: #F0F0F0; height:20px ;margin-top:0px;margin-bottom:0px }"
                                                        "QScrollBar::handle:horizontal{ background: #6c65c8; border-radius:3px; min-width: 80px ;height:20px }");
	//ui->tableView-> setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);// 纵向 平滑滑动，因为太卡，看不太出来效果
	ui->tableView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);//水平 平滑滑动 可防止单元格太宽跳动
	QHeaderView *header = ui->tableView->verticalHeader();
	header->setHidden(true);// 隐藏行号
	ui->tableView->verticalHeader()->setDefaultSectionSize(20);        //设置行间距
	ui->tableView->verticalHeader()->setMinimumSectionSize(20);        //设置行间距
	move(0,85);


    //数据库表格初始化
    model = new MyQSqlRelationModel_Centerdisp;

    label_hide = new QTimer;
    label_hide->setInterval(5000);
    connect(label_hide,SIGNAL(timeout()),this,SLOT(disp_over()));
	connect(this,SIGNAL(export_ing()),this,SLOT(disp_ing()));
    connect(this,SIGNAL(export_noU()),this,SLOT(disp_noU()));

    //筛选时间控件初始化
	ui->dateEdit_from->setDate(QDate::currentDate().addDays(-1));
	ui->dateEdit_to->setDate(QDate::currentDate());
//    ui->timeEdit_from->setTime(QTime::currentTime());
	ui->timeEdit_to->setTime(QTime::currentTime());
	ui->widget_conditional_query->setHidden(1);

	//获取剩余内存
	QString mem_used;
	QString mem_all;
	QString mem_available;
	QString mem_percentage;
	QProcess mem_process;
	mem_process.start("df /opt");
	mem_process.waitForFinished();
	QByteArray mem_output = mem_process.readAllStandardOutput();

	QString str_output = mem_output;
	qDebug() << str_output.toUtf8();
	str_output.replace(QRegExp("[\\s]+"), " ");  //把所有的多余的空格转为一个空格
	mem_all = str_output.section(' ', 8, 8);       //
	mem_used = str_output.section(' ', 9, 9);
	mem_available = str_output.section(' ', 10, 10);
	mem_percentage = str_output.section(' ', 11, 11);
	mem_used = mem_used.left(mem_used.length() - 3);//变为M字节
	mem_available = mem_available.left(mem_available.length() - 3);//变为M字节
	mem_used.append("M");
	mem_available.append("M");
	qDebug()<<mem_all<<mem_used<<mem_available<<mem_percentage;
	QString mem_sta = "已用: "+mem_used+"\r\n"+"可用: "+mem_available+"\n"+"使用率: "+mem_percentage ;
	ui->label_mem->setText(mem_sta);

}

history::~history()
{
    model->deleteLater();
    delete ui;
}

void history::on_pushButton_3_clicked()     //关闭
{
    Flag_Timeto_CloseNeeded[0] = 0;
    emit pushButton_history_enable();
    close();
}

void history::on_pushButton_xielou_warn_clicked()     //报警记录
{
	model->clear();
	Filter.clear();
    model->setTable("controlinfo");
    model->setSort(0,Qt::DescendingOrder);  //倒序
    model->setEditStrategy(QSqlTableModel::OnManualSubmit);
//    model->setRelation(4,QSqlRelation("id","time","whichone","state"));
    model->setHeaderData(0,Qt::Horizontal,QObject::tr("%1").arg("时间"));
    model->setHeaderData(1,Qt::Horizontal,QObject::tr("%1").arg("设备号"));
    model->setHeaderData(2,Qt::Horizontal,QObject::tr("%1").arg("状态"));
    model->removeColumn(0);
    model->select();
	ui->tableView->setModel(model);
	ui->tableView->setColumnWidth(0,300);
	ui->tableView->setColumnWidth(1,140);
	ui->tableView->setColumnWidth(2,180);

    ui->pushButton_xielou_warn->setStyleSheet("background-color: rgb(250,250,250)");
    ui->pushButton_xielou_op->setStyleSheet("background-color: rgb(60,164,252)");
    ui->pushButton_xielou_net->setStyleSheet("background-color: rgb(60,164,252)");
}

void history::on_pushButton_xielou_op_clicked()     //操作日志
{
	model->clear();
	Filter.clear();
    model->setTable("operateinfo");
    model->setSort(0,Qt::DescendingOrder);  //倒序
    model->setEditStrategy(QSqlTableModel::OnManualSubmit);
//    model->setRelation(4,QSqlRelation("id","time","whichone","state"));
    model->setHeaderData(0,Qt::Horizontal,QObject::tr("%1").arg("时间"));
    model->setHeaderData(1,Qt::Horizontal,QObject::tr("%1").arg("用户名"));
    model->setHeaderData(2,Qt::Horizontal,QObject::tr("%1").arg("执行操作"));
    model->removeColumn(0);
    model->select();
	ui->tableView->setModel(model);
	ui->tableView->setColumnWidth(0,300);
	ui->tableView->setColumnWidth(1,140);
	ui->tableView->setColumnWidth(2,250);

    ui->pushButton_xielou_warn->setStyleSheet("background-color: rgb(60,164,252)");
    ui->pushButton_xielou_op->setStyleSheet("background-color: rgb(250,250,250)");
    ui->pushButton_xielou_net->setStyleSheet("background-color: rgb(60,164,252)");
}

void history::on_pushButton_xielou_net_clicked()     //通信日志
{
	model->clear();
	Filter.clear();
    model->setTable("netinfo");
    model->setSort(0,Qt::DescendingOrder);  //倒序
    model->setEditStrategy(QSqlTableModel::OnManualSubmit);
//    model->setRelation(4,QSqlRelation("id","time","whichone","state"));
    model->setHeaderData(0,Qt::Horizontal,QObject::tr("%1").arg("时间"));
    model->setHeaderData(1,Qt::Horizontal,QObject::tr("%1").arg("网络状态"));
    model->removeColumn(0);
    model->select();
	ui->tableView->setModel(model);
	ui->tableView->setColumnWidth(0,300);
	ui->tableView->setColumnWidth(1,400);

    ui->pushButton_xielou_warn->setStyleSheet("background-color: rgb(60,164,252)");
    ui->pushButton_xielou_op->setStyleSheet("background-color: rgb(60,164,252)");
    ui->pushButton_xielou_net->setStyleSheet("background-color: rgb(250,250,250)");
}


void history::on_pushButton_copy_clicked()
{
    ui->pushButton_copy->setEnabled(0);
    printf("i am in copy\n");
    int fd_des;
    //取消sda1的限制，盘符通用
    char Udisk_fullname[20] = {"sda1"};
    //定义浏览的文件夹及所含文件
    QString U_Disk;
    QDir *dir = new QDir("/media/");
    foreach(QFileInfo list_file,dir->entryInfoList())
    {
        if(list_file.isDir())
        {
            U_Disk = list_file.fileName();
            QByteArray Q_lastname = U_Disk.toLatin1();
            char *U_lastname = Q_lastname.data();
            if((strcmp(".",U_lastname)) && (strcmp("..",U_lastname)))
            {
                sprintf(Udisk_fullname,"/media/%s",U_lastname);
            }
        }
    }

    //打开合成之后的路径,验证是否存在
    fd_des = ::open(Udisk_fullname,O_RDONLY);

    if(fd_des < 0)
    {
        printf("no U!!\n");
        fflush(stdout);
        ::close(fd_des);
        emit export_noU();
        return;
    }
    ::close(fd_des);
    emit export_ing();

    QSqlTableModel *outputModel = new QSqlTableModel();
    QStringList strList;//记录数据库中的一行报警数据
    QString strString;

    //导出表controlinfo
    outputModel->setTable("controlinfo");
    outputModel->select();
    while(outputModel->canFetchMore())
    {
        outputModel->fetchMore();
    }
    QFile csvFile(QString("%1/His_controlInfo.csv").arg(Udisk_fullname));
    csvFile.open(QIODevice::ReadWrite);
    strString = QString("ID,时间,设备编号,状态\n");
    csvFile.write(strString.toUtf8());
    for(int i = 0;i < outputModel->rowCount();i++)
    {
        for(int j = 0;j < outputModel->columnCount();j++)
        {
            strList.insert(j,outputModel->data(outputModel->index(i,j)).toString());//把每一行的每一列数据读取到strList中
        }
        strString = strList.join(",")+"\n";//给两个列数据之前加“,”号，一行数据末尾加回车
        strList.clear();
        csvFile.write(strString.toUtf8());
    }
    csvFile.close();

    //导出表operateinfo
    outputModel->setTable("operateinfo");
    outputModel->select();
    while(outputModel->canFetchMore())
    {
        outputModel->fetchMore();
    }
    QFile csvFile_Operate(QString("%1/His_operateInfo.csv").arg(Udisk_fullname));
    csvFile_Operate.open(QIODevice::ReadWrite);
    strString = QString("ID,时间,用户名,执行操作\n");
    csvFile_Operate.write(strString.toUtf8());
    for(int i = 0;i < outputModel->rowCount();i++)
    {
        for(int j = 0;j < outputModel->columnCount();j++)
        {
            strList.insert(j,outputModel->data(outputModel->index(i,j)).toString());
        }
        strString = strList.join(",") + "\n";
        strList.clear();
        csvFile_Operate.write(strString.toUtf8());
    }
    csvFile_Operate.close();

    //导出表netinfo
    outputModel->setTable("netinfo");
    outputModel->select();
    while(outputModel->canFetchMore())
    {
        outputModel->fetchMore();
    }
    QFile csvFile_Net(QString("%1/His_netInfo.csv").arg(Udisk_fullname));
    csvFile_Net.open(QIODevice::ReadWrite);
    strString = QString("ID,时间,网络状态\n");
    csvFile_Net.write(strString.toUtf8());
    for(int i = 0;i < outputModel->rowCount();i++)
    {
        for(int j = 0;j < outputModel->columnCount();j++)
        {
            strList.insert(j,outputModel->data(outputModel->index(i,j)).toString());
        }
        strString = strList.join(",") + "\n";
        strList.clear();
        csvFile_Net.write(strString.toUtf8());
    }
    csvFile_Net.close();

    //导出表history_radarinfo
    outputModel->setTable("history_radarinfo");
    outputModel->select();
    while(outputModel->canFetchMore())
    {
        outputModel->fetchMore();
    }
    QFile csvFile_Radar(QString("%1/His_radarInfo.csv").arg(Udisk_fullname));
    csvFile_Radar.open(QIODevice::ReadWrite);
    strString = QString("ID,时间,设备状态,报警坐标\n");
    csvFile_Radar.write(strString.toUtf8());
    for(int i = 0;i < outputModel->rowCount();i++)
    {
        for(int j = 0;j < outputModel->columnCount();j++)
        {
            strList.insert(j,outputModel->data(outputModel->index(i,j)).toString());
        }
        strString = strList.join(",") + "\n";
        strList.clear();
        csvFile_Radar.write(strString.toUtf8());
    }
    csvFile_Radar.close();

    //导出表history_jingdianinfo
    outputModel->setTable("history_jingdianinfo");
    outputModel->select();
    while(outputModel->canFetchMore())
    {
        outputModel->fetchMore();
    }
    QFile csvFile_Jingdian(QString("%1/His_jingdianInfo.csv").arg(Udisk_fullname));
    csvFile_Jingdian.open(QIODevice::ReadWrite);
    strString = QString("ID,时间,设备编号，状态\n");
    csvFile_Jingdian.write(strString.toUtf8());
    for(int i = 0;i < outputModel->rowCount();i++)
    {
        for(int j = 0;j < outputModel->columnCount();j++)
        {
            strList.insert(j,outputModel->data(outputModel->index(i,j)).toString());
        }
        strString = strList.join(",") + "\n";
        strList.clear();
        csvFile_Jingdian.write(strString.toUtf8());
    }
    csvFile_Jingdian.close();

    //导出表history_IIE
    outputModel->setTable("history_IIE");
    outputModel->select();
    while(outputModel->canFetchMore())
    {
        outputModel->fetchMore();
    }
    QFile csvFile_IIE(QString("%1/His_IIE.csv").arg(Udisk_fullname));
    csvFile_IIE.open(QIODevice::ReadWrite);
    strString = QString("ID,时间,状态\n");
    csvFile_IIE.write(strString.toUtf8());
    for(int i = 0;i < outputModel->rowCount();i++)
    {
        for(int j = 0;j < outputModel->columnCount();j++)
        {
            strList.insert(j,outputModel->data(outputModel->index(i,j)).toString());
        }
        strString = strList.join(",") + "\n";
        strList.clear();
        csvFile_IIE.write(strString.toUtf8());
    }
    csvFile_IIE.close();
    //导出表reoilgasinfo
    outputModel->setTable("reoilgasinfo");
    outputModel->select();
    while(outputModel->canFetchMore())
    {
        outputModel->fetchMore();
    }
    QFile csvFile_reoilgas(QString("%1/His_ReoilgasInfo.csv").arg(Udisk_fullname));
    csvFile_reoilgas.open(QIODevice::ReadWrite);
    strString = QString("ID,设备编号,油枪编号,时间,加油时长(s),A/L(%),油气流量(L),燃油流量(L),油气流速(L/min),燃油流速(L/min),15L,A/L状态\n");
    csvFile_reoilgas.write(strString.toUtf8());
    for(int i = 0;i < outputModel->rowCount();i++)
    {
        for(int j = 0;j < outputModel->columnCount();j++)
        {
            strList.insert(j,outputModel->data(outputModel->index(i,j)).toString());
        }
        strString = strList.join(",") + "\n";
        strList.clear();
        csvFile_reoilgas.write(strString.toUtf8());
    }
    csvFile_reoilgas.close();
    //导出表reoilgaswarn
    outputModel->setTable("reoilgaswarn");
    outputModel->select();
    while(outputModel->canFetchMore())
    {
        outputModel->fetchMore();
    }
    QFile csvFile_reoilgaswarn(QString("%1/His_reoilgaswarn.csv").arg(Udisk_fullname));
    csvFile_reoilgaswarn.open(QIODevice::ReadWrite);
    strString = QString("ID,设备编号,状态\n");
    csvFile_reoilgaswarn.write(strString.toUtf8());
    for(int i = 0;i < outputModel->rowCount();i++)
    {
        for(int j = 0;j < outputModel->columnCount();j++)
        {
            strList.insert(j,outputModel->data(outputModel->index(i,j)).toString());
        }
        strString = strList.join(",") + "\n";
        strList.clear();
        csvFile_reoilgaswarn.write(strString.toUtf8());
    }
    csvFile_reoilgaswarn.close();
    //导出表fgainfo
    outputModel->setTable("fgainfo");
    outputModel->select();
    while(outputModel->canFetchMore())
    {
        outputModel->fetchMore();
    }
    QFile csvFile_fga(QString("%1/His_FgaInfo.csv").arg(Udisk_fullname));
    csvFile_fga.open(QIODevice::ReadWrite);
    strString = QString("ID,时间,传感器编号,状态\n");
    csvFile_fga.write(strString.toUtf8());
    for(int i = 0;i < outputModel->rowCount();i++)
    {
        for(int j = 0;j < outputModel->columnCount();j++)
        {
            strList.insert(j,outputModel->data(outputModel->index(i,j)).toString());
        }
        strString = strList.join(",") + "\n";
        strList.clear();
        csvFile_fga.write(strString.toUtf8());
    }
    csvFile_fga.close();
    //导出表envinfo
    outputModel->setTable("envinfo");
    outputModel->select();
    while(outputModel->canFetchMore())
    {
        outputModel->fetchMore();
    }
    QFile csvFile_env(QString("%1/His_Envinfo.csv").arg(Udisk_fullname));
    csvFile_env.open(QIODevice::ReadWrite);
    strString = QString("ID,时间,油罐压力,管道压力,油罐温度,油气浓度\n");
    csvFile_env.write(strString.toUtf8());
    for(int i = 0;i < outputModel->rowCount();i++)
    {
        for(int j = 0;j < outputModel->columnCount();j++)
        {
            strList.insert(j,outputModel->data(outputModel->index(i,j)).toString());
        }
        strString = strList.join(",") + "\n";
        strList.clear();
        csvFile_env.write(strString.toUtf8());
    }
    csvFile_env.close();

    //导出表history_liquid
    outputModel->setTable("history_liquid");
    outputModel->select();
    while(outputModel->canFetchMore())
    {
        outputModel->fetchMore();
    }
    QFile history_liquid(QString("%1/His_Liquid.csv").arg(Udisk_fullname));
    history_liquid.open(QIODevice::ReadWrite);
    strString = QString("ID,时间,状态\n");
    history_liquid.write(strString.toUtf8());
    for(int i = 0;i < outputModel->rowCount();i++)
    {
        for(int j = 0;j < outputModel->columnCount();j++)
        {
            strList.insert(j,outputModel->data(outputModel->index(i,j)).toString());
        }
        strString = strList.join(",") + "\n";
        strList.clear();
        history_liquid.write(strString.toUtf8());
    }
    history_liquid.close();

    //导出表history_pump
    outputModel->setTable("history_pump");
    outputModel->select();
    while(outputModel->canFetchMore())
    {
        outputModel->fetchMore();
    }
    QFile csvFile_history_pump(QString("%1/His_Pump.csv").arg(Udisk_fullname));
    csvFile_history_pump.open(QIODevice::ReadWrite);
    strString = QString("ID,时间,状态\n");
    csvFile_history_pump.write(strString.toUtf8());
    for(int i = 0;i < outputModel->rowCount();i++)
    {
        for(int j = 0;j < outputModel->columnCount();j++)
        {
            strList.insert(j,outputModel->data(outputModel->index(i,j)).toString());
        }
        strString = strList.join(",") + "\n";
        strList.clear();
        csvFile_history_pump.write(strString.toUtf8());
    }
    csvFile_history_pump.close();

    //导出表history_crash
    outputModel->setTable("history_crash");
    outputModel->select();
    while(outputModel->canFetchMore())
    {
        outputModel->fetchMore();
    }
    QFile csvFile_history_crash(QString("%1/His_Crash.csv").arg(Udisk_fullname));
    csvFile_history_crash.open(QIODevice::ReadWrite);
    strString = QString("ID,时间,状态\n");
    csvFile_history_crash.write(strString.toUtf8());
    for(int i = 0;i < outputModel->rowCount();i++)
    {
        for(int j = 0;j < outputModel->columnCount();j++)
        {
            strList.insert(j,outputModel->data(outputModel->index(i,j)).toString());
        }
        strString = strList.join(",") + "\n";
        strList.clear();
        csvFile_history_crash.write(strString.toUtf8());
    }
    csvFile_history_crash.close();
	//导出表加油机详情
	outputModel->setTable("gunwarn_details");
	outputModel->select();
	while(outputModel->canFetchMore())
	{
		outputModel->fetchMore();
	}
	QFile csvFile_gunwarn_details(QString("%1/His_GunWarn.csv").arg(Udisk_fullname));
	csvFile_gunwarn_details.open(QIODevice::ReadWrite);
	strString = QString("ID,时间,设备ID,枪号，详细报警信息\n");
	csvFile_gunwarn_details.write(strString.toUtf8());
	for(int i = 0;i < outputModel->rowCount();i++)
	{
		for(int j = 0;j < outputModel->columnCount();j++)
		{
			strList.insert(j,outputModel->data(outputModel->index(i,j)).toString());
		}
		strString = strList.join(",") + "\n";
		strList.clear();
		csvFile_gunwarn_details.write(strString.toUtf8());
	}
	csvFile_gunwarn_details.close();

    //
    system("sync");
    outputModel->deleteLater();
}
void history::disp_ing()
{
    ui->label_copying->setText(QString("%1").arg("导出中..."));
    qApp->processEvents();
    label_hide->start();
}
void history::disp_noU()
{
    ui->label_copying->setText(QString("%1").arg("请插入U盘"));
    label_hide->start();
}
void history::disp_over()
{
    ui->pushButton_copy->setEnabled(1);
    ui->label_copying->setText("");
    label_hide->stop();
}



void history::on_pushButton_radar_clicked()     //雷达入侵记录
{
	model->clear();
	Filter.clear();
    model->setTable("history_radarinfo");
    model->setSort(0,Qt::DescendingOrder);  //倒序
    model->setEditStrategy(QSqlTableModel::OnManualSubmit);
//    model->setRelation(4,QSqlRelation("id","time","whichone","state"));
    model->setHeaderData(0,Qt::Horizontal,QObject::tr("%1").arg("时间"));
    model->setHeaderData(1,Qt::Horizontal,QObject::tr("%1").arg("设备状态"));
    model->setHeaderData(2,Qt::Horizontal,QObject::tr("%1").arg("报警坐标"));
    model->removeColumn(0);
    model->select();
	ui->tableView->setModel(model);
	ui->tableView->setColumnWidth(0,300);
	ui->tableView->setColumnWidth(1,230);
	ui->tableView->setColumnWidth(2,150);

    //ui->pushButton_radar->setStyleSheet("border-image: url(:/picture/history_warn_c.png)");
}

void history::on_tabWidget_currentChanged(int index)        //导航
{
//	ui->widget_conditional_query->setHidden(1);//隐藏加油枪条件查询信息
//    switch (index)
//    {
//        case 0: on_pushButton_xielou_warn_clicked();
//                break;
//        case 1: on_pushButton_radar_clicked();
//                break;
//        case 2: on_pushButton_jingdian_clicked();
//                break;
//        case 3: on_pushButton_gaswarn_clicked();
//                break;
//        case 4: on_pushButton_gasre_oilgun_clicked();
//                break;
//    }
}

void history::on_pushButton_gaswarn_clicked()       //可燃气体
{
	model->clear();
	Filter.clear();
    model->setTable("fgainfo");
    model->setSort(0,Qt::DescendingOrder);
    model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    model->setHeaderData(0,Qt::Horizontal,QObject::tr("%1").arg("时间"));
    model->setHeaderData(1,Qt::Horizontal,QObject::tr("%1").arg("传感器编号"));
    model->setHeaderData(2,Qt::Horizontal,QObject::tr("%1").arg("状态"));
    model->removeColumn(0);
    model->select();

	ui->tableView->setModel(model);
	ui->tableView->setColumnWidth(0,290);
	ui->tableView->setColumnWidth(1,200);
	ui->tableView->setColumnWidth(2,200);
}

void history::on_pushButton_gasre_huanjing_clicked()    //在线油气回收   环境数据
{
	model->clear();
	Filter.clear();
    Flag_AL_ENV = 1;
	ui->scrollArea->setEnabled(0); //条件查询框 更改后有没有无所谓
    model->setTable("envinfo");
    model->setSort(0,Qt::DescendingOrder);
    model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    model->setHeaderData(0,Qt::Horizontal,QObject::tr("%1").arg("ID"));
    model->setHeaderData(1,Qt::Horizontal,QObject::tr("%1").arg("时间"));
    model->setHeaderData(2,Qt::Horizontal,QObject::tr("%1").arg("油罐压力/KPa"));
    model->setHeaderData(3,Qt::Horizontal,QObject::tr("%1").arg("管道压力/KPa"));
    model->setHeaderData(4,Qt::Horizontal,QObject::tr("%1").arg("油罐温度/℃"));
    model->setHeaderData(5,Qt::Horizontal,QObject::tr("%1").arg("气体浓度/g/m³"));
    //model->removeColumn(0);
    model->select();

	ui->tableView->setModel(model);
	ui->tableView->setColumnWidth(0,90);    //id
	ui->tableView->setColumnWidth(1,290);   //时间
	ui->tableView->setColumnWidth(2,90);    //油罐压力
	ui->tableView->setColumnWidth(3,90);    //管道压力
	ui->tableView->setColumnWidth(4,90);    //油罐温度
	ui->tableView->setColumnWidth(5,90);    //气体浓度

    ui->pushButton_gasre_huanjing->setStyleSheet("background-color: rgb(250,250,250)");
    ui->pushButton_gasre_oilgun->setStyleSheet("background-color: rgb(60,164,252)");
    ui->pushButton_gasre_warn->setStyleSheet("background-color: rgb(60,164,252)");
    ui->pushButton_conditional_query->setStyleSheet("background-color: rgb(60,164,252)");
    ui->widget_conditional_query->setHidden(1);//隐藏加油枪条件查询信息
}

void history::on_pushButton_gasre_oilgun_clicked()      //在线油气回收    油枪数据
{
	model->clear();
	Filter.clear();
    Flag_AL_ENV = 0;
	ui->scrollArea->setEnabled(1);//条件查询框 更改后有没有无所谓
    model->setTable("reoilgasinfo");
    model->setSort(0,Qt::DescendingOrder);  //倒序
    model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    model->setHeaderData(0,Qt::Horizontal,QObject::tr("%1").arg("ID"));
    model->setHeaderData(1,Qt::Horizontal,QObject::tr("%1").arg("设备编号"));
    model->setHeaderData(2,Qt::Horizontal,QObject::tr("%1").arg("油枪编号"));
    model->setHeaderData(3,Qt::Horizontal,QObject::tr("%1").arg("时间"));
    model->setHeaderData(4,Qt::Horizontal,QObject::tr("%1").arg("加油时长(s)"));
    model->setHeaderData(5,Qt::Horizontal,QObject::tr("%1").arg("A/L %"));
    model->setHeaderData(6,Qt::Horizontal,QObject::tr("%1").arg("油气流量(L)"));
    model->setHeaderData(7,Qt::Horizontal,QObject::tr("%1").arg("燃油流量(L)"));
    model->setHeaderData(8,Qt::Horizontal,QObject::tr("%1").arg("油气流速(L/min)"));
    model->setHeaderData(9,Qt::Horizontal,QObject::tr("%1").arg("燃油流速(L/min)"));
//    model->setHeaderData(8,Qt::Horizontal,QObject::tr("%1").arg("油气浓度(%)"));
    model->setHeaderData(10,Qt::Horizontal,QObject::tr("%1").arg("状态"));
    //model->removeColumn(0);
    model->removeColumn(10);
    model->select();

	ui->tableView->setModel(model);
	ui->tableView->setColumnWidth(0,70);   //ID
	ui->tableView->setColumnWidth(1,70);   //设备编号
	ui->tableView->setColumnWidth(2,70);   //油枪编号
	ui->tableView->setColumnWidth(3,200);   //时间
	ui->tableView->setColumnWidth(4,90);    //加油时长
	ui->tableView->setColumnWidth(5,70);    //al
	ui->tableView->setColumnWidth(6,90);    //气流量
	ui->tableView->setColumnWidth(7,90);   //油流量
	ui->tableView->setColumnWidth(8,110);    //气流速
	ui->tableView->setColumnWidth(9,110);   //油流速
	ui->tableView->setColumnWidth(10,70);   //状态


    ui->pushButton_gasre_huanjing->setStyleSheet("background-color: rgb(60,164,252)");
    ui->pushButton_gasre_oilgun->setStyleSheet("background-color: rgb(250,250,250)");
    ui->pushButton_gasre_warn->setStyleSheet("background-color: rgb(60,164,252)");
    ui->pushButton_conditional_query->setStyleSheet("background-color: rgb(60,164,252)");
    ui->widget_conditional_query->setHidden(1);//隐藏加油枪条件查询信息
}

void history::on_pushButton_gasre_warn_clicked() //报警信息
{
	model->clear();
	Filter.clear();
    Flag_AL_ENV = 2;
	ui->scrollArea->setEnabled(1);//条件查询框 更改后有没有无所谓
    model->setTable("reoilgaswarn");
    model->setSort(0,Qt::DescendingOrder);
    model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    model->setHeaderData(0,Qt::Horizontal,QObject::tr("%1").arg("ID"));
    model->setHeaderData(1,Qt::Horizontal,QObject::tr("%1").arg("时间"));
    model->setHeaderData(2,Qt::Horizontal,QObject::tr("%1").arg("设备编号"));
    model->setHeaderData(3,Qt::Horizontal,QObject::tr("%1").arg("状态"));
    //model->removeColumn(0);
    model->select();

	ui->tableView->setModel(model);
	ui->tableView->setColumnWidth(0,90);
	ui->tableView->setColumnWidth(1,290);
	ui->tableView->setColumnWidth(2,90);
	ui->tableView->setColumnWidth(3,150);

    ui->pushButton_gasre_huanjing->setStyleSheet("background-color: rgb(60,164,252)");
    ui->pushButton_gasre_oilgun->setStyleSheet("background-color: rgb(60,164,252)");
    ui->pushButton_gasre_warn->setStyleSheet("background-color: rgb(250,250,250)");
    ui->pushButton_conditional_query->setStyleSheet("background-color: rgb(60,164,252)");
    ui->widget_conditional_query->setHidden(1);//隐藏加油枪条件查询信息
}
//条件查询按钮
void history::on_pushButton_conditional_query_clicked()
{
	which_guncheckbox_show();
    ui->widget_conditional_query->setHidden(0);
    ui->pushButton_conditional_query->setStyleSheet("background-color: rgb(250,250,250)");
    ui->pushButton_gasre_huanjing->setStyleSheet("background-color: rgb(60,164,252)");
    ui->pushButton_gasre_oilgun->setStyleSheet("background-color: rgb(60,164,252)");
    ui->pushButton_gasre_warn->setStyleSheet("background-color: rgb(60,164,252)");
}
//条件查询
void history::on_toolButton_reoilgas_detail_clicked()
{
	model->clear();
	Filter.clear();
    unsigned char flag_filter[12][8] = {0};     //筛选
    unsigned int sum_flag_filter = 0;           //筛选
    unsigned int sum_flag_filter_temp = 0;      //筛选
    switch(Flag_AL_ENV)
    {
        case 0:
                model->setTable("reoilgasinfo");
                model->setSort(0,Qt::DescendingOrder);  //倒序
                model->setEditStrategy(QSqlTableModel::OnManualSubmit);
                model->setHeaderData(0,Qt::Horizontal,QObject::tr("%1").arg("ID"));
                model->setHeaderData(1,Qt::Horizontal,QObject::tr("%1").arg("设备编号"));
                model->setHeaderData(2,Qt::Horizontal,QObject::tr("%1").arg("油枪编号"));
                model->setHeaderData(3,Qt::Horizontal,QObject::tr("%1").arg("时间"));
                model->setHeaderData(4,Qt::Horizontal,QObject::tr("%1").arg("加油时长(s)"));
                model->setHeaderData(5,Qt::Horizontal,QObject::tr("%1").arg("A/L %"));
                model->setHeaderData(6,Qt::Horizontal,QObject::tr("%1").arg("油气流量(L)"));
                model->setHeaderData(7,Qt::Horizontal,QObject::tr("%1").arg("燃油流量(L)"));
                model->setHeaderData(8,Qt::Horizontal,QObject::tr("%1").arg("油气流速(L/min)"));
                model->setHeaderData(9,Qt::Horizontal,QObject::tr("%1").arg("燃油流速(L/min)"));
            //    model->setHeaderData(8,Qt::Horizontal,QObject::tr("%1").arg("油气浓度(%)"));
                model->setHeaderData(10,Qt::Horizontal,QObject::tr("%1").arg("状态"));
                //model->removeColumn(0);
                model->removeColumn(10);

                if(ui->checkBox_1_1->isChecked())
                {
                    flag_filter[0][0] = 1;
                }
                if(ui->checkBox_1_2->isChecked())
                {
                    flag_filter[0][1] = 1;
                }
                if(ui->checkBox_1_3->isChecked())
                {
                    flag_filter[0][2] = 1;
                }
                if(ui->checkBox_1_4->isChecked())
                {
                    flag_filter[0][3] = 1;
                }
                if(ui->checkBox_1_5->isChecked())
                {
                    flag_filter[0][4] = 1;
                }
                if(ui->checkBox_1_6->isChecked())
                {
                    flag_filter[0][5] = 1;
                }
                if(ui->checkBox_1_7->isChecked())
                {
                    flag_filter[0][6] = 1;
                }
                if(ui->checkBox_1_8->isChecked())
                {
                    flag_filter[0][7] = 1;
                }
                if(ui->checkBox_2_1->isChecked())
                {
                    flag_filter[1][0] = 1;
                }
                if(ui->checkBox_2_2->isChecked())
                {
                    flag_filter[1][1] = 1;
                }
                if(ui->checkBox_2_3->isChecked())
                {
                    flag_filter[1][2] = 1;
                }
                if(ui->checkBox_2_4->isChecked())
                {
                    flag_filter[1][3] = 1;
                }
                if(ui->checkBox_2_5->isChecked())
                {
                    flag_filter[1][4] = 1;
                }
                if(ui->checkBox_2_6->isChecked())
                {
                    flag_filter[1][5] = 1;
                }
                if(ui->checkBox_2_7->isChecked())
                {
                    flag_filter[1][6] = 1;
                }
                if(ui->checkBox_2_8->isChecked())
                {
                    flag_filter[1][7] = 1;
                }
                if(ui->checkBox_3_1->isChecked())
                {
                    flag_filter[2][0] = 1;
                }
                if(ui->checkBox_3_2->isChecked())
                {
                    flag_filter[2][1] = 1;
                }
                if(ui->checkBox_3_3->isChecked())
                {
                    flag_filter[2][2] = 1;
                }
                if(ui->checkBox_3_4->isChecked())
                {
                    flag_filter[2][3] = 1;
                }
                if(ui->checkBox_3_5->isChecked())
                {
                    flag_filter[2][4] = 1;
                }
                if(ui->checkBox_3_6->isChecked())
                {
                    flag_filter[2][5] = 1;
                }
                if(ui->checkBox_3_7->isChecked())
                {
                    flag_filter[2][6] = 1;
                }
                if(ui->checkBox_3_8->isChecked())
                {
                    flag_filter[2][7] = 1;
                }
                if(ui->checkBox_4_1->isChecked())
                {
                    flag_filter[3][0] = 1;
                }
                if(ui->checkBox_4_2->isChecked())
                {
                    flag_filter[3][1] = 1;
                }
                if(ui->checkBox_4_3->isChecked())
                {
                    flag_filter[3][2] = 1;
                }
                if(ui->checkBox_4_4->isChecked())
                {
                    flag_filter[3][3] = 1;
                }
                if(ui->checkBox_4_5->isChecked())
                {
                    flag_filter[3][4] = 1;
                }
                if(ui->checkBox_4_6->isChecked())
                {
                    flag_filter[3][5] = 1;
                }
                if(ui->checkBox_4_7->isChecked())
                {
                    flag_filter[3][6] = 1;
                }
                if(ui->checkBox_4_8->isChecked())
                {
                    flag_filter[3][7] = 1;
                }
                if(ui->checkBox_5_1->isChecked())
                {
                    flag_filter[4][0] = 1;
                }
                if(ui->checkBox_5_2->isChecked())
                {
                    flag_filter[4][1] = 1;
                }
                if(ui->checkBox_5_3->isChecked())
                {
                    flag_filter[4][2] = 1;
                }
                if(ui->checkBox_5_4->isChecked())
                {
                    flag_filter[4][3] = 1;
                }
                if(ui->checkBox_5_5->isChecked())
                {
                    flag_filter[4][4] = 1;
                }
                if(ui->checkBox_5_6->isChecked())
                {
                    flag_filter[4][5] = 1;
                }
                if(ui->checkBox_5_7->isChecked())
                {
                    flag_filter[4][6] = 1;
                }
                if(ui->checkBox_5_8->isChecked())
                {
                    flag_filter[4][7] = 1;
                }
                if(ui->checkBox_6_1->isChecked())
                {
                    flag_filter[5][0] = 1;
                }
                if(ui->checkBox_6_2->isChecked())
                {
                    flag_filter[5][1] = 1;
                }
                if(ui->checkBox_6_3->isChecked())
                {
                    flag_filter[5][2] = 1;
                }
                if(ui->checkBox_6_4->isChecked())
                {
                    flag_filter[5][3] = 1;
                }
                if(ui->checkBox_6_5->isChecked())
                {
                    flag_filter[5][4] = 1;
                }
                if(ui->checkBox_6_6->isChecked())
                {
                    flag_filter[5][5] = 1;
                }
                if(ui->checkBox_6_7->isChecked())
                {
                    flag_filter[5][6] = 1;
                }
                if(ui->checkBox_6_8->isChecked())
                {
                    flag_filter[5][7] = 1;
                }
                if(ui->checkBox_7_1->isChecked())
                {
                    flag_filter[6][0] = 1;
                }
                if(ui->checkBox_7_2->isChecked())
                {
                    flag_filter[6][1] = 1;
                }
                if(ui->checkBox_7_3->isChecked())
                {
                    flag_filter[6][2] = 1;
                }
                if(ui->checkBox_7_4->isChecked())
                {
                    flag_filter[6][3] = 1;
                }
                if(ui->checkBox_7_5->isChecked())
                {
                    flag_filter[6][4] = 1;
                }
                if(ui->checkBox_7_6->isChecked())
                {
                    flag_filter[6][5] = 1;
                }
                if(ui->checkBox_7_7->isChecked())
                {
                    flag_filter[6][6] = 1;
                }
                if(ui->checkBox_7_8->isChecked())
                {
                    flag_filter[6][7] = 1;
                }
                if(ui->checkBox_8_1->isChecked())
                {
                    flag_filter[7][0] = 1;
                }
                if(ui->checkBox_8_2->isChecked())
                {
                    flag_filter[7][1] = 1;
                }
                if(ui->checkBox_8_3->isChecked())
                {
                    flag_filter[7][2] = 1;
                }
                if(ui->checkBox_8_4->isChecked())
                {
                    flag_filter[7][3] = 1;
                }
                if(ui->checkBox_8_5->isChecked())
                {
                    flag_filter[7][4] = 1;
                }
                if(ui->checkBox_8_6->isChecked())
                {
                    flag_filter[7][5] = 1;
                }
                if(ui->checkBox_8_7->isChecked())
                {
                    flag_filter[7][6] = 1;
                }
                if(ui->checkBox_8_8->isChecked())
                {
                    flag_filter[7][7] = 1;
                }
                if(ui->checkBox_9_1->isChecked())
                {
                    flag_filter[8][0] = 1;
                }
                if(ui->checkBox_9_2->isChecked())
                {
                    flag_filter[8][1] = 1;
                }
                if(ui->checkBox_9_3->isChecked())
                {
                    flag_filter[8][2] = 1;
                }
                if(ui->checkBox_9_4->isChecked())
                {
                    flag_filter[8][3] = 1;
                }
                if(ui->checkBox_9_5->isChecked())
                {
                    flag_filter[8][4] = 1;
                }
                if(ui->checkBox_9_6->isChecked())
                {
                    flag_filter[8][5] = 1;
                }
                if(ui->checkBox_9_7->isChecked())
                {
                    flag_filter[8][6] = 1;
                }
                if(ui->checkBox_9_8->isChecked())
                {
                    flag_filter[8][7] = 1;
                }
                if(ui->checkBox_10_1->isChecked())
                {
                    flag_filter[9][0] = 1;
                }
                if(ui->checkBox_10_2->isChecked())
                {
                    flag_filter[9][1] = 1;
                }
                if(ui->checkBox_10_3->isChecked())
                {
                    flag_filter[9][2] = 1;
                }
                if(ui->checkBox_10_4->isChecked())
                {
                    flag_filter[9][3] = 1;
                }
                if(ui->checkBox_10_5->isChecked())
                {
                    flag_filter[9][4] = 1;
                }
                if(ui->checkBox_10_6->isChecked())
                {
                    flag_filter[9][5] = 1;
                }
                if(ui->checkBox_10_7->isChecked())
                {
                    flag_filter[9][6] = 1;
                }
                if(ui->checkBox_10_8->isChecked())
                {
                    flag_filter[9][7] = 1;
                }
                if(ui->checkBox_11_1->isChecked())
                {
                    flag_filter[10][0] = 1;
                }
                if(ui->checkBox_11_2->isChecked())
                {
                    flag_filter[10][1] = 1;
                }
                if(ui->checkBox_11_3->isChecked())
                {
                    flag_filter[10][2] = 1;
                }
                if(ui->checkBox_11_4->isChecked())
                {
                    flag_filter[10][3] = 1;
                }
                if(ui->checkBox_11_5->isChecked())
                {
                    flag_filter[10][4] = 1;
                }
                if(ui->checkBox_11_6->isChecked())
                {
                    flag_filter[10][5] = 1;
                }
                if(ui->checkBox_11_7->isChecked())
                {
                    flag_filter[10][6] = 1;
                }
                if(ui->checkBox_11_8->isChecked())
                {
                    flag_filter[10][7] = 1;
                }
                if(ui->checkBox_12_1->isChecked())
                {
                    flag_filter[11][0] = 1;
                }
                if(ui->checkBox_12_2->isChecked())
                {
                    flag_filter[11][1] = 1;
                }
                if(ui->checkBox_12_3->isChecked())
                {
                    flag_filter[11][2] = 1;
                }
                if(ui->checkBox_12_4->isChecked())
                {
                    flag_filter[11][3] = 1;
                }
                if(ui->checkBox_12_5->isChecked())
                {
                    flag_filter[11][4] = 1;
                }
                if(ui->checkBox_12_6->isChecked())
                {
                    flag_filter[11][5] = 1;
                }
                if(ui->checkBox_12_7->isChecked())
                {
                    flag_filter[11][6] = 1;
                }
                if(ui->checkBox_12_8->isChecked())
                {
                    flag_filter[11][7] = 1;
                }
                for(uchar i = 0;i<12;i++)
                {
                    for(uchar j = 0;j<6;j++)
                    {
                        sum_flag_filter+=flag_filter[i][j];
                        if(sum_flag_filter > sum_flag_filter_temp)
                        {
                            if(sum_flag_filter_temp > 0)
                            {
                                Filter.append(QString("or whichone = '%1-%2' ").arg(i+1).arg(j+1));
 //                               Filter.append(QString("and time between '20%1  %2' and '20%3  %4' ").arg(ui->dateEdit_from->text()).arg(ui->timeEdit_from->text()).arg(ui->dateEdit_to->text()).arg(ui->timeEdit_to->text()));
                            }
                            else
                            {
                                Filter.append(QString("(whichone = '%1-%2' ").arg(i+1).arg(j+1));
 //                               Filter.append(QString("and time between '20%1  %2' and '20%3  %4' ").arg(ui->dateEdit_from->text()).arg(ui->timeEdit_from->text()).arg(ui->dateEdit_to->text()).arg(ui->timeEdit_to->text()));
                            }
                            sum_flag_filter_temp = sum_flag_filter;
                        }
                    }
                }
                if(sum_flag_filter_temp > 0)
                {
                    Filter.append(QString(") and time between '20%1  %2' and '20%3  %4' ").arg(ui->dateEdit_from->text()).arg(ui->timeEdit_from->text()).arg(ui->dateEdit_to->text()).arg(ui->timeEdit_to->text()));
                }
                else
                {
                    Filter.append(QString("time between '20%1  %2' and '20%3  %4' ").arg(ui->dateEdit_from->text()).arg(ui->timeEdit_from->text()).arg(ui->dateEdit_to->text()).arg(ui->timeEdit_to->text()));
                }
             //   filter.append(QString("and time between '20%1  %2' and '20%3  %4'").arg(ui->dateEdit_from->text()).arg(ui->timeEdit_from->text()).arg(ui->dateEdit_to->text()).arg(ui->timeEdit_to->text()));
                qDebug()<<Filter<<endl;
                model->setFilter(Filter);
                model->select();

				ui->tableView->setModel(model);
				ui->tableView->setColumnWidth(0,70);   //ID
				ui->tableView->setColumnWidth(1,70);   //设备编号
				ui->tableView->setColumnWidth(2,70);   //油枪编号
				ui->tableView->setColumnWidth(3,200);   //时间
				ui->tableView->setColumnWidth(4,90);    //加油时长
				ui->tableView->setColumnWidth(5,70);    //al
				ui->tableView->setColumnWidth(6,90);    //气流量
				ui->tableView->setColumnWidth(7,90);   //油流量
				ui->tableView->setColumnWidth(8,110);    //气流速
				ui->tableView->setColumnWidth(9,110);   //油流速
				ui->tableView->setColumnWidth(10,70);   //状态
                break;
        case 1:
                model->setTable("envinfo");
                model->setSort(0,Qt::DescendingOrder);
                model->setEditStrategy(QSqlTableModel::OnManualSubmit);
                model->setHeaderData(0,Qt::Horizontal,QObject::tr("%1").arg("ID"));
                model->setHeaderData(1,Qt::Horizontal,QObject::tr("%1").arg("时间"));
                model->setHeaderData(2,Qt::Horizontal,QObject::tr("%1").arg("油罐压力/KPa"));
                model->setHeaderData(3,Qt::Horizontal,QObject::tr("%1").arg("管道压力/KPa"));
                model->setHeaderData(4,Qt::Horizontal,QObject::tr("%1").arg("油罐温度/℃"));
                model->setHeaderData(5,Qt::Horizontal,QObject::tr("%1").arg("气体浓度/g/m³"));
                //model->removeColumn(0);
                Filter.append(QString("time between '20%1  %2' and '20%3  %4' ").arg(ui->dateEdit_from->text()).arg(ui->timeEdit_from->text()).arg(ui->dateEdit_to->text()).arg(ui->timeEdit_to->text()));
                qDebug()<<Filter<<endl;
                model->setFilter(Filter);
                model->select();

				ui->tableView->setModel(model);
				ui->tableView->setColumnWidth(0,90);    //id
				ui->tableView->setColumnWidth(1,290);   //时间
				ui->tableView->setColumnWidth(2,90);    //油罐压力
				ui->tableView->setColumnWidth(3,90);    //管道压力
				ui->tableView->setColumnWidth(4,90);    //油罐温度
				ui->tableView->setColumnWidth(5,90);    //气体浓度
                break;
        case 2:
                model->setTable("reoilgaswarn");
                model->setSort(0,Qt::DescendingOrder);
                model->setEditStrategy(QSqlTableModel::OnManualSubmit);
                model->setHeaderData(0,Qt::Horizontal,QObject::tr("%1").arg("ID"));
                model->setHeaderData(1,Qt::Horizontal,QObject::tr("%1").arg("时间"));
                model->setHeaderData(2,Qt::Horizontal,QObject::tr("%1").arg("设备编号"));
                model->setHeaderData(3,Qt::Horizontal,QObject::tr("%1").arg("状态"));
                //model->removeColumn(0);
                if(ui->checkBox_1_1->isChecked())
                {
                    flag_filter[0][0] = 1;
                }
                if(ui->checkBox_1_2->isChecked())
                {
                    flag_filter[0][1] = 1;
                }
                if(ui->checkBox_1_3->isChecked())
                {
                    flag_filter[0][2] = 1;
                }
                if(ui->checkBox_1_4->isChecked())
                {
                    flag_filter[0][3] = 1;
                }
                if(ui->checkBox_1_5->isChecked())
                {
                    flag_filter[0][4] = 1;
                }
                if(ui->checkBox_1_6->isChecked())
                {
                    flag_filter[0][5] = 1;
                }
                if(ui->checkBox_1_7->isChecked())
                {
                    flag_filter[0][6] = 1;
                }
                if(ui->checkBox_1_8->isChecked())
                {
                    flag_filter[0][7] = 1;
                }
                if(ui->checkBox_2_1->isChecked())
                {
                    flag_filter[1][0] = 1;
                }
                if(ui->checkBox_2_2->isChecked())
                {
                    flag_filter[1][1] = 1;
                }
                if(ui->checkBox_2_3->isChecked())
                {
                    flag_filter[1][2] = 1;
                }
                if(ui->checkBox_2_4->isChecked())
                {
                    flag_filter[1][3] = 1;
                }
                if(ui->checkBox_2_5->isChecked())
                {
                    flag_filter[1][4] = 1;
                }
                if(ui->checkBox_2_6->isChecked())
                {
                    flag_filter[1][5] = 1;
                }
                if(ui->checkBox_2_7->isChecked())
                {
                    flag_filter[1][6] = 1;
                }
                if(ui->checkBox_2_8->isChecked())
                {
                    flag_filter[1][7] = 1;
                }
                if(ui->checkBox_3_1->isChecked())
                {
                    flag_filter[2][0] = 1;
                }
                if(ui->checkBox_3_2->isChecked())
                {
                    flag_filter[2][1] = 1;
                }
                if(ui->checkBox_3_3->isChecked())
                {
                    flag_filter[2][2] = 1;
                }
                if(ui->checkBox_3_4->isChecked())
                {
                    flag_filter[2][3] = 1;
                }
                if(ui->checkBox_3_5->isChecked())
                {
                    flag_filter[2][4] = 1;
                }
                if(ui->checkBox_3_6->isChecked())
                {
                    flag_filter[2][5] = 1;
                }
                if(ui->checkBox_3_7->isChecked())
                {
                    flag_filter[2][6] = 1;
                }
                if(ui->checkBox_3_8->isChecked())
                {
                    flag_filter[2][7] = 1;
                }
                if(ui->checkBox_4_1->isChecked())
                {
                    flag_filter[3][0] = 1;
                }
                if(ui->checkBox_4_2->isChecked())
                {
                    flag_filter[3][1] = 1;
                }
                if(ui->checkBox_4_3->isChecked())
                {
                    flag_filter[3][2] = 1;
                }
                if(ui->checkBox_4_4->isChecked())
                {
                    flag_filter[3][3] = 1;
                }
                if(ui->checkBox_4_5->isChecked())
                {
                    flag_filter[3][4] = 1;
                }
                if(ui->checkBox_4_6->isChecked())
                {
                    flag_filter[3][5] = 1;
                }
                if(ui->checkBox_4_7->isChecked())
                {
                    flag_filter[3][6] = 1;
                }
                if(ui->checkBox_4_8->isChecked())
                {
                    flag_filter[3][7] = 1;
                }
                if(ui->checkBox_5_1->isChecked())
                {
                    flag_filter[4][0] = 1;
                }
                if(ui->checkBox_5_2->isChecked())
                {
                    flag_filter[4][1] = 1;
                }
                if(ui->checkBox_5_3->isChecked())
                {
                    flag_filter[4][2] = 1;
                }
                if(ui->checkBox_5_4->isChecked())
                {
                    flag_filter[4][3] = 1;
                }
                if(ui->checkBox_5_5->isChecked())
                {
                    flag_filter[4][4] = 1;
                }
                if(ui->checkBox_5_6->isChecked())
                {
                    flag_filter[4][5] = 1;
                }
                if(ui->checkBox_5_7->isChecked())
                {
                    flag_filter[4][6] = 1;
                }
                if(ui->checkBox_5_8->isChecked())
                {
                    flag_filter[4][7] = 1;
                }
                if(ui->checkBox_6_1->isChecked())
                {
                    flag_filter[5][0] = 1;
                }
                if(ui->checkBox_6_2->isChecked())
                {
                    flag_filter[5][1] = 1;
                }
                if(ui->checkBox_6_3->isChecked())
                {
                    flag_filter[5][2] = 1;
                }
                if(ui->checkBox_6_4->isChecked())
                {
                    flag_filter[5][3] = 1;
                }
                if(ui->checkBox_6_5->isChecked())
                {
                    flag_filter[5][4] = 1;
                }
                if(ui->checkBox_6_6->isChecked())
                {
                    flag_filter[5][5] = 1;
                }
                if(ui->checkBox_6_7->isChecked())
                {
                    flag_filter[5][6] = 1;
                }
                if(ui->checkBox_6_8->isChecked())
                {
                    flag_filter[5][7] = 1;
                }
                if(ui->checkBox_7_1->isChecked())
                {
                    flag_filter[6][0] = 1;
                }
                if(ui->checkBox_7_2->isChecked())
                {
                    flag_filter[6][1] = 1;
                }
                if(ui->checkBox_7_3->isChecked())
                {
                    flag_filter[6][2] = 1;
                }
                if(ui->checkBox_7_4->isChecked())
                {
                    flag_filter[6][3] = 1;
                }
                if(ui->checkBox_7_5->isChecked())
                {
                    flag_filter[6][4] = 1;
                }
                if(ui->checkBox_7_6->isChecked())
                {
                    flag_filter[6][5] = 1;
                }
                if(ui->checkBox_7_7->isChecked())
                {
                    flag_filter[6][6] = 1;
                }
                if(ui->checkBox_7_8->isChecked())
                {
                    flag_filter[6][7] = 1;
                }
                if(ui->checkBox_8_1->isChecked())
                {
                    flag_filter[7][0] = 1;
                }
                if(ui->checkBox_8_2->isChecked())
                {
                    flag_filter[7][1] = 1;
                }
                if(ui->checkBox_8_3->isChecked())
                {
                    flag_filter[7][2] = 1;
                }
                if(ui->checkBox_8_4->isChecked())
                {
                    flag_filter[7][3] = 1;
                }
                if(ui->checkBox_8_5->isChecked())
                {
                    flag_filter[7][4] = 1;
                }
                if(ui->checkBox_8_6->isChecked())
                {
                    flag_filter[7][5] = 1;
                }
                if(ui->checkBox_8_7->isChecked())
                {
                    flag_filter[7][6] = 1;
                }
                if(ui->checkBox_8_8->isChecked())
                {
                    flag_filter[7][7] = 1;
                }
                if(ui->checkBox_9_1->isChecked())
                {
                    flag_filter[8][0] = 1;
                }
                if(ui->checkBox_9_2->isChecked())
                {
                    flag_filter[8][1] = 1;
                }
                if(ui->checkBox_9_3->isChecked())
                {
                    flag_filter[8][2] = 1;
                }
                if(ui->checkBox_9_4->isChecked())
                {
                    flag_filter[8][3] = 1;
                }
                if(ui->checkBox_9_5->isChecked())
                {
                    flag_filter[8][4] = 1;
                }
                if(ui->checkBox_9_6->isChecked())
                {
                    flag_filter[8][5] = 1;
                }
                if(ui->checkBox_9_7->isChecked())
                {
                    flag_filter[8][6] = 1;
                }
                if(ui->checkBox_9_8->isChecked())
                {
                    flag_filter[8][7] = 1;
                }
                if(ui->checkBox_10_1->isChecked())
                {
                    flag_filter[9][0] = 1;
                }
                if(ui->checkBox_10_2->isChecked())
                {
                    flag_filter[9][1] = 1;
                }
                if(ui->checkBox_10_3->isChecked())
                {
                    flag_filter[9][2] = 1;
                }
                if(ui->checkBox_10_4->isChecked())
                {
                    flag_filter[9][3] = 1;
                }
                if(ui->checkBox_10_5->isChecked())
                {
                    flag_filter[9][4] = 1;
                }
                if(ui->checkBox_10_6->isChecked())
                {
                    flag_filter[9][5] = 1;
                }
                if(ui->checkBox_10_7->isChecked())
                {
                    flag_filter[9][6] = 1;
                }
                if(ui->checkBox_10_8->isChecked())
                {
                    flag_filter[9][7] = 1;
                }
                if(ui->checkBox_11_1->isChecked())
                {
                    flag_filter[10][0] = 1;
                }
                if(ui->checkBox_11_2->isChecked())
                {
                    flag_filter[10][1] = 1;
                }
                if(ui->checkBox_11_3->isChecked())
                {
                    flag_filter[10][2] = 1;
                }
                if(ui->checkBox_11_4->isChecked())
                {
                    flag_filter[10][3] = 1;
                }
                if(ui->checkBox_11_5->isChecked())
                {
                    flag_filter[10][4] = 1;
                }
                if(ui->checkBox_11_6->isChecked())
                {
                    flag_filter[10][5] = 1;
                }
                if(ui->checkBox_11_7->isChecked())
                {
                    flag_filter[10][6] = 1;
                }
                if(ui->checkBox_11_8->isChecked())
                {
                    flag_filter[10][7] = 1;
                }
                if(ui->checkBox_12_1->isChecked())
                {
                    flag_filter[11][0] = 1;
                }
                if(ui->checkBox_12_2->isChecked())
                {
                    flag_filter[11][1] = 1;
                }
                if(ui->checkBox_12_3->isChecked())
                {
                    flag_filter[11][2] = 1;
                }
                if(ui->checkBox_12_4->isChecked())
                {
                    flag_filter[11][3] = 1;
                }
                if(ui->checkBox_12_5->isChecked())
                {
                    flag_filter[11][4] = 1;
                }
                if(ui->checkBox_12_6->isChecked())
                {
                    flag_filter[11][5] = 1;
                }
                if(ui->checkBox_12_7->isChecked())
                {
                    flag_filter[11][6] = 1;
                }
                if(ui->checkBox_12_8->isChecked())
                {
                    flag_filter[11][7] = 1;
                }
                for(uchar i = 0;i<12;i++)
                {
                    for(uchar j = 0;j<8;j++)
                    {
                        sum_flag_filter+=flag_filter[i][j];
                        if(sum_flag_filter > sum_flag_filter_temp)
                        {
                            if(sum_flag_filter_temp > 0)
                            {
                                Filter.append(QString("or whichone = '%1-%2' ").arg(i+1).arg(j+1));
 //                               Filter.append(QString("and time between '20%1  %2' and '20%3  %4' ").arg(ui->dateEdit_from->text()).arg(ui->timeEdit_from->text()).arg(ui->dateEdit_to->text()).arg(ui->timeEdit_to->text()));
                            }
                            else
                            {
                                Filter.append(QString("(whichone = '%1-%2' ").arg(i+1).arg(j+1));
 //                               Filter.append(QString("and time between '20%1  %2' and '20%3  %4' ").arg(ui->dateEdit_from->text()).arg(ui->timeEdit_from->text()).arg(ui->dateEdit_to->text()).arg(ui->timeEdit_to->text()));
                            }
                            sum_flag_filter_temp = sum_flag_filter;
                        }
                    }
                }
                if(sum_flag_filter_temp > 0)
                {
                    Filter.append(QString(") and time between '20%1  %2' and '20%3  %4' ").arg(ui->dateEdit_from->text()).arg(ui->timeEdit_from->text()).arg(ui->dateEdit_to->text()).arg(ui->timeEdit_to->text()));
                }
                else
                {
                    Filter.append(QString("time between '20%1  %2' and '20%3  %4' ").arg(ui->dateEdit_from->text()).arg(ui->timeEdit_from->text()).arg(ui->dateEdit_to->text()).arg(ui->timeEdit_to->text()));
                }
             //   filter.append(QString("and time between '20%1  %2' and '20%3  %4'").arg(ui->dateEdit_from->text()).arg(ui->timeEdit_from->text()).arg(ui->dateEdit_to->text()).arg(ui->timeEdit_to->text()));
                qDebug()<<Filter<<endl;
                model->setFilter(Filter);
                model->select();


				ui->tableView->setModel(model);
				ui->tableView->setColumnWidth(0,90);
				ui->tableView->setColumnWidth(1,290);
				ui->tableView->setColumnWidth(2,90);
				ui->tableView->setColumnWidth(3,150);
        break;
    }
    ui->widget_conditional_query->setHidden(1);
}
//条件导出
void history::on_toolButton_reoilgas_detail_output_clicked()
{
    ui->pushButton_copy->setEnabled(0);
    printf("i am in copy\n");
    int fd_des;
    //取消sda1的限制，盘符通用
    char Udisk_fullname[20] = {"sda1"};
    //定义浏览的文件夹及所含文件
    QString U_Disk;
    QDir *dir = new QDir("/media/");
    foreach(QFileInfo list_file,dir->entryInfoList())
    {
        if(list_file.isDir())
        {
            U_Disk = list_file.fileName();
            QByteArray Q_lastname = U_Disk.toLatin1();
            char *U_lastname = Q_lastname.data();
            if((strcmp(".",U_lastname)) && (strcmp("..",U_lastname)))
            {
                sprintf(Udisk_fullname,"/media/%s",U_lastname);
            }
        }
    }

    //打开合成之后的路径,验证是否存在
    fd_des = ::open(Udisk_fullname,O_RDONLY);

    if(fd_des < 0)
    {
        printf("no U!!\n");
        fflush(stdout);
        ::close(fd_des);
        emit export_noU();
        return;
    }
    ::close(fd_des);
    emit export_ing();

    QSqlTableModel *outputModel = new QSqlTableModel();
    QStringList strList;//记录数据库中的一行报警数据
    QString strString;

    QFile csvFile_reoilgas(QString("%1/His_ReoilgasInfo_%2.csv").arg(Udisk_fullname).arg((QDateTime::currentDateTime().toString("dd-mm-ss"))));
    QFile csvFile_env(QString("%1/His_Envinfo_%2.csv").arg(Udisk_fullname).arg((QDateTime::currentDateTime().toString("dd-mm-ss"))));
    switch (Flag_AL_ENV)
    {
        case 0:
                //导出表reoilgasinfo
                outputModel->setTable("reoilgasinfo");
                outputModel->setFilter(Filter);
                outputModel->select();
                while(outputModel->canFetchMore())
                {
                    outputModel->fetchMore();
                }

                csvFile_reoilgas.open(QIODevice::ReadWrite);
                strString = QString("ID,设备编号,油枪编号，时间,加油时长(s),A/L(%),油气流量(L),燃油流量(L),油气流速(L/min),燃油流速(L/min),15L,A/L状态\n");
                csvFile_reoilgas.write(strString.toUtf8());
                for(int i = 0;i < outputModel->rowCount();i++)
                {
                    for(int j = 0;j < outputModel->columnCount();j++)
                    {
                        strList.insert(j,outputModel->data(outputModel->index(i,j)).toString());
                    }
                    strString = strList.join(",") + "\n";
                    strList.clear();
                    csvFile_reoilgas.write(strString.toUtf8());
                }
                csvFile_reoilgas.close();
                break;
        case 1:
                //导出表envinfo
                outputModel->setTable("envinfo");
                outputModel->setFilter(Filter);
                outputModel->select();
                while(outputModel->canFetchMore())
                {
                    outputModel->fetchMore();
                }

                csvFile_env.open(QIODevice::ReadWrite);
                strString = QString("ID,时间,油罐压力,管道压力,油罐温度,油气浓度\n");
                csvFile_env.write(strString.toUtf8());
                for(int i = 0;i < outputModel->rowCount();i++)
                {
                    for(int j = 0;j < outputModel->columnCount();j++)
                    {
                        strList.insert(j,outputModel->data(outputModel->index(i,j)).toString());
                    }
                    strString = strList.join(",") + "\n";
                    strList.clear();
                    csvFile_env.write(strString.toUtf8());
                }
                csvFile_env.close();
                break;
        case 2:
                //导出表reoilgaswarn
                outputModel->setTable("reoilgaswarn");
                outputModel->setFilter(Filter);
                outputModel->select();
                while(outputModel->canFetchMore())
                {
                    outputModel->fetchMore();
                }

                csvFile_env.open(QIODevice::ReadWrite);
                strString = QString("ID,时间,设备编号,状态\n");
                csvFile_env.write(strString.toUtf8());
                for(int i = 0;i < outputModel->rowCount();i++)
                {
                    for(int j = 0;j < outputModel->columnCount();j++)
                    {
                        strList.insert(j,outputModel->data(outputModel->index(i,j)).toString());
                    }
                    strString = strList.join(",") + "\n";
                    strList.clear();
                    csvFile_env.write(strString.toUtf8());
                }
        csvFile_env.close();
        break;
    }
    system("sync");
    outputModel->deleteLater();
    ui->widget_conditional_query->setHidden(1);
}
void history::which_guncheckbox_show()
{
    unsigned char flag_checkbox[12][8] = {0};
    for(unsigned char i = 0;i<Amount_Dispener;i++)
    {
        for(unsigned char j = 0;j<Amount_Gasgun[i];j++)
        {
            flag_checkbox[i][j] = 1;
        }
    }
    ui->checkBox_1_1->setEnabled(flag_checkbox[0][0]);
    ui->checkBox_1_2->setEnabled(flag_checkbox[0][1]);
    ui->checkBox_1_3->setEnabled(flag_checkbox[0][2]);
    ui->checkBox_1_4->setEnabled(flag_checkbox[0][3]);
    ui->checkBox_1_5->setEnabled(flag_checkbox[0][4]);
    ui->checkBox_1_6->setEnabled(flag_checkbox[0][5]);
    ui->checkBox_1_7->setEnabled(flag_checkbox[0][6]);
    ui->checkBox_1_8->setEnabled(flag_checkbox[0][7]);

    ui->checkBox_2_1->setEnabled(flag_checkbox[1][0]);
    ui->checkBox_2_2->setEnabled(flag_checkbox[1][1]);
    ui->checkBox_2_3->setEnabled(flag_checkbox[1][2]);
    ui->checkBox_2_4->setEnabled(flag_checkbox[1][3]);
    ui->checkBox_2_5->setEnabled(flag_checkbox[1][4]);
    ui->checkBox_2_6->setEnabled(flag_checkbox[1][5]);
    ui->checkBox_2_7->setEnabled(flag_checkbox[1][6]);
    ui->checkBox_2_8->setEnabled(flag_checkbox[1][7]);

    ui->checkBox_3_1->setEnabled(flag_checkbox[2][0]);
    ui->checkBox_3_2->setEnabled(flag_checkbox[2][1]);
    ui->checkBox_3_3->setEnabled(flag_checkbox[2][2]);
    ui->checkBox_3_4->setEnabled(flag_checkbox[2][3]);
    ui->checkBox_3_5->setEnabled(flag_checkbox[2][4]);
    ui->checkBox_3_6->setEnabled(flag_checkbox[2][5]);
    ui->checkBox_3_7->setEnabled(flag_checkbox[2][6]);
    ui->checkBox_3_8->setEnabled(flag_checkbox[2][7]);

    ui->checkBox_4_1->setEnabled(flag_checkbox[3][0]);
    ui->checkBox_4_2->setEnabled(flag_checkbox[3][1]);
    ui->checkBox_4_3->setEnabled(flag_checkbox[3][2]);
    ui->checkBox_4_4->setEnabled(flag_checkbox[3][3]);
    ui->checkBox_4_5->setEnabled(flag_checkbox[3][4]);
    ui->checkBox_4_6->setEnabled(flag_checkbox[3][5]);
    ui->checkBox_4_7->setEnabled(flag_checkbox[3][6]);
    ui->checkBox_4_8->setEnabled(flag_checkbox[3][7]);

    ui->checkBox_5_1->setEnabled(flag_checkbox[4][0]);
    ui->checkBox_5_2->setEnabled(flag_checkbox[4][1]);
    ui->checkBox_5_3->setEnabled(flag_checkbox[4][2]);
    ui->checkBox_5_4->setEnabled(flag_checkbox[4][3]);
    ui->checkBox_5_5->setEnabled(flag_checkbox[4][4]);
    ui->checkBox_5_6->setEnabled(flag_checkbox[4][5]);
    ui->checkBox_5_7->setEnabled(flag_checkbox[4][6]);
    ui->checkBox_5_8->setEnabled(flag_checkbox[4][7]);

    ui->checkBox_6_1->setEnabled(flag_checkbox[5][0]);
    ui->checkBox_6_2->setEnabled(flag_checkbox[5][1]);
    ui->checkBox_6_3->setEnabled(flag_checkbox[5][2]);
    ui->checkBox_6_4->setEnabled(flag_checkbox[5][3]);
    ui->checkBox_6_5->setEnabled(flag_checkbox[5][4]);
    ui->checkBox_6_6->setEnabled(flag_checkbox[5][5]);
    ui->checkBox_6_7->setEnabled(flag_checkbox[5][6]);
    ui->checkBox_6_8->setEnabled(flag_checkbox[5][7]);

    ui->checkBox_7_1->setEnabled(flag_checkbox[6][0]);
    ui->checkBox_7_2->setEnabled(flag_checkbox[6][1]);
    ui->checkBox_7_3->setEnabled(flag_checkbox[6][2]);
    ui->checkBox_7_4->setEnabled(flag_checkbox[6][3]);
    ui->checkBox_7_5->setEnabled(flag_checkbox[6][4]);
    ui->checkBox_7_6->setEnabled(flag_checkbox[6][5]);
    ui->checkBox_7_7->setEnabled(flag_checkbox[6][6]);
    ui->checkBox_7_8->setEnabled(flag_checkbox[6][7]);

    ui->checkBox_8_1->setEnabled(flag_checkbox[7][0]);
    ui->checkBox_8_2->setEnabled(flag_checkbox[7][1]);
    ui->checkBox_8_3->setEnabled(flag_checkbox[7][2]);
    ui->checkBox_8_4->setEnabled(flag_checkbox[7][3]);
    ui->checkBox_8_5->setEnabled(flag_checkbox[7][4]);
    ui->checkBox_8_6->setEnabled(flag_checkbox[7][5]);
    ui->checkBox_8_7->setEnabled(flag_checkbox[7][6]);
    ui->checkBox_8_8->setEnabled(flag_checkbox[7][7]);

    ui->checkBox_9_1->setEnabled(flag_checkbox[8][0]);
    ui->checkBox_9_2->setEnabled(flag_checkbox[8][1]);
    ui->checkBox_9_3->setEnabled(flag_checkbox[8][2]);
    ui->checkBox_9_4->setEnabled(flag_checkbox[8][3]);
    ui->checkBox_9_5->setEnabled(flag_checkbox[8][4]);
    ui->checkBox_9_6->setEnabled(flag_checkbox[8][5]);
    ui->checkBox_9_7->setEnabled(flag_checkbox[8][6]);
    ui->checkBox_9_8->setEnabled(flag_checkbox[8][7]);

    ui->checkBox_10_1->setEnabled(flag_checkbox[9][0]);
    ui->checkBox_10_2->setEnabled(flag_checkbox[9][1]);
    ui->checkBox_10_3->setEnabled(flag_checkbox[9][2]);
    ui->checkBox_10_4->setEnabled(flag_checkbox[9][3]);
    ui->checkBox_10_5->setEnabled(flag_checkbox[9][4]);
    ui->checkBox_10_6->setEnabled(flag_checkbox[9][5]);
    ui->checkBox_10_7->setEnabled(flag_checkbox[9][6]);
    ui->checkBox_10_8->setEnabled(flag_checkbox[9][7]);

    ui->checkBox_11_1->setEnabled(flag_checkbox[10][0]);
    ui->checkBox_11_2->setEnabled(flag_checkbox[10][1]);
    ui->checkBox_11_3->setEnabled(flag_checkbox[10][2]);
    ui->checkBox_11_4->setEnabled(flag_checkbox[10][3]);
    ui->checkBox_11_5->setEnabled(flag_checkbox[10][4]);
    ui->checkBox_11_6->setEnabled(flag_checkbox[10][5]);
    ui->checkBox_11_7->setEnabled(flag_checkbox[10][6]);
    ui->checkBox_11_8->setEnabled(flag_checkbox[10][7]);

    ui->checkBox_12_1->setEnabled(flag_checkbox[11][0]);
    ui->checkBox_12_2->setEnabled(flag_checkbox[11][1]);
    ui->checkBox_12_3->setEnabled(flag_checkbox[11][2]);
    ui->checkBox_12_4->setEnabled(flag_checkbox[11][3]);
    ui->checkBox_12_5->setEnabled(flag_checkbox[11][4]);
    ui->checkBox_12_6->setEnabled(flag_checkbox[11][5]);
    ui->checkBox_12_7->setEnabled(flag_checkbox[11][6]);
    ui->checkBox_12_8->setEnabled(flag_checkbox[11][7]);
}

void history::on_pushButton_jingdian_clicked()      //人体静电
{
	model->clear();
	Filter.clear();
    model->setTable("history_jingdianinfo");
    model->setSort(0,Qt::DescendingOrder);  //倒序
    model->setEditStrategy(QSqlTableModel::OnManualSubmit);
//    model->setRelation(4,QSqlRelation("id","time","whichone","state"));
    model->setHeaderData(0,Qt::Horizontal,QObject::tr("%1").arg("时间"));
    model->setHeaderData(1,Qt::Horizontal,QObject::tr("%1").arg("状态"));
    model->removeColumn(0);
    model->select();
	ui->tableView->setModel(model);
	ui->tableView->setColumnWidth(0,300);
	ui->tableView->setColumnWidth(1,200);

    ui->pushButton_jingdian->setStyleSheet("background-color: rgb(250,250,250)");
    ui->pushButton_IIE->setStyleSheet("background-color: rgb(60,164,252)");
    ui->pushButton_liquid->setStyleSheet("background-color: rgb(60,164,252)");
    ui->pushButton_pump->setStyleSheet("background-color: rgb(60,164,252)");
    ui->pushButton_crash->setStyleSheet("background-color: rgb(60,164,252)");

}

void history::on_pushButton_IIE_clicked() //IIE
{
	model->clear();
	Filter.clear();
    model->setTable("history_IIE");
    model->setSort(0,Qt::DescendingOrder);  //倒序
    model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    model->setHeaderData(0,Qt::Horizontal,QObject::tr("%1").arg("时间"));
    model->setHeaderData(1,Qt::Horizontal,QObject::tr("%1").arg("状态"));
    model->removeColumn(0);
    model->select();
	ui->tableView->setModel(model);
	ui->tableView->setColumnWidth(0,300);
	ui->tableView->setColumnWidth(1,200);

    ui->pushButton_jingdian->setStyleSheet("background-color: rgb(60,164,252)");
    ui->pushButton_IIE->setStyleSheet("background-color: rgb(250,250,250)");
    ui->pushButton_liquid->setStyleSheet("background-color: rgb(60,164,252)");
    ui->pushButton_pump->setStyleSheet("background-color: rgb(60,164,252)");
    ui->pushButton_crash->setStyleSheet("background-color: rgb(60,164,252)");
}

void history::on_pushButton_pump_clicked()//潜油泵
{
	model->clear();
	Filter.clear();
    model->setTable("history_pump");
    model->setSort(0,Qt::DescendingOrder);  //倒序
    model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    model->setHeaderData(0,Qt::Horizontal,QObject::tr("%1").arg("时间"));
    model->setHeaderData(1,Qt::Horizontal,QObject::tr("%1").arg("状态"));
    model->removeColumn(0);
    model->select();
	ui->tableView->setModel(model);
	ui->tableView->setColumnWidth(0,300);
	ui->tableView->setColumnWidth(1,200);

    ui->pushButton_jingdian->setStyleSheet("background-color: rgb(60,164,252)");
    ui->pushButton_IIE->setStyleSheet("background-color: rgb(60,164,252)");
    ui->pushButton_liquid->setStyleSheet("background-color: rgb(60,164,252)");
    ui->pushButton_pump->setStyleSheet("background-color: rgb(250,250,250)");
    ui->pushButton_crash->setStyleSheet("background-color: rgb(60,164,252)");
}

void history::on_pushButton_liquid_clicked()//液位仪
{
	model->clear();
	Filter.clear();
    model->setTable("history_liquid");
    model->setSort(0,Qt::DescendingOrder);  //倒序
    model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    model->setHeaderData(0,Qt::Horizontal,QObject::tr("%1").arg("时间"));
    model->setHeaderData(1,Qt::Horizontal,QObject::tr("%1").arg("状态"));
    model->removeColumn(0);
    model->select();
	ui->tableView->setModel(model);
	ui->tableView->setColumnWidth(0,300);
	ui->tableView->setColumnWidth(1,200);
    ui->pushButton_jingdian->setStyleSheet("background-color: rgb(60,164,252)");
    ui->pushButton_IIE->setStyleSheet("background-color: rgb(60,164,252)");
    ui->pushButton_liquid->setStyleSheet("background-color: rgb(250,250,250)");
    ui->pushButton_pump->setStyleSheet("background-color: rgb(60,164,252)");
    ui->pushButton_crash->setStyleSheet("background-color: rgb(60,164,252)");
}



void history::on_pushButton_crash_clicked()//防撞柱
{
	model->clear();
	Filter.clear();
    model->setTable("history_crash");
    model->setSort(0,Qt::DescendingOrder);  //倒序
    model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    model->setHeaderData(0,Qt::Horizontal,QObject::tr("%1").arg("时间"));
    model->setHeaderData(1,Qt::Horizontal,QObject::tr("%1").arg("状态"));
    model->removeColumn(0);
    model->select();
	ui->tableView->setModel(model);
	ui->tableView->setColumnWidth(0,300);
	ui->tableView->setColumnWidth(1,400);
    ui->pushButton_jingdian->setStyleSheet("background-color: rgb(60,164,252)");
    ui->pushButton_IIE->setStyleSheet("background-color: rgb(60,164,252)");
    ui->pushButton_liquid->setStyleSheet("background-color: rgb(60,164,252)");
    ui->pushButton_pump->setStyleSheet("background-color: rgb(60,164,252)");
    ui->pushButton_crash->setStyleSheet("background-color: rgb(250,250,250)");
}




void history::on_toolButton_reoilgas_detail_2_clicked()
{
    ui->widget_conditional_query->setHidden(1);
}
