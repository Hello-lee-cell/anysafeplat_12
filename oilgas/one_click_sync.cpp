#include "one_click_sync.h"
#include "ui_one_click_sync.h"
#include <QStandardItemModel>
#include <QDebug>
#include <QScrollBar>
#include "myqsqlrelationmodel_centerdisp.h"
#include "config.h"
#include <QTimer>
unsigned char Flag_WaitSync = 0;//等待数据同步，阻断正常的问询进程，全局变量
unsigned int data_hang = 0;//数据表的行数
unsigned int sum_gun_num = 0;//油枪总数
unsigned char flag_flicker = 0;//同步中闪烁标志
QStandardItemModel *model_sync;

QTimer *lcd_shot;

One_click_sync::One_click_sync(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::One_click_sync)
{
	ui->setupUi(this);
	this->setWindowFlags(windowFlags()|Qt::FramelessWindowHint|Qt::WindowTitleHint|Qt::WindowStaysOnTopHint);
	this->setAttribute(Qt::WA_DeleteOnClose,true);//立即释放缓存

	model_sync = new QStandardItemModel();
	model_sync->clear();
	QStringList labels = QObject::trUtf8("设备编号,同步油量脉冲当量1,同步气量脉冲当量1,同步油量脉冲当量2,同步气量脉冲当量2").simplified().split(",");
	model_sync->setHorizontalHeaderLabels(labels);
	ui->tableView->setShowGrid(true);
	ui->tableView->verticalHeader()->setHidden(1);//隐藏行号
	ui->tableView->setModel(model_sync);
	ui->tableView->setColumnWidth(0,80);
	ui->tableView->setColumnWidth(1,152);
	ui->tableView->setColumnWidth(2,152);
	ui->tableView->setColumnWidth(3,152);
	ui->tableView->setColumnWidth(4,152);
	ui->tableView->show();

	ui->label_sync_sta_show->setText("未同步");

	gun_num_calc();//计算气液比采集器数目

	move(0,85);

	lcd_shot = new QTimer(this);
	lcd_shot->setInterval(500);
	connect(lcd_shot,SIGNAL(timeout()),this,SLOT(flicker()));
	lcd_shot->start();

}

One_click_sync::~One_click_sync()
{
	delete lcd_shot;//删除qtimer
	model_sync->deleteLater();
	delete ui;
}
void One_click_sync::show_data(unsigned int idi,unsigned int idj,float oil_factor1,float gas_factor1,float oil_factor2,float gas_factor2)
{
	ui->label_sync_sta_show->setText("同步中……");
	qApp->processEvents();
	if(oil_factor1<256)
	{
		model_sync->setItem(data_hang,0,new QStandardItem(QString::number(idi+1).append("-").append(QString::number(idj+1))));
		model_sync->setItem(data_hang,1,new QStandardItem(QString::number(oil_factor1)));
		model_sync->setItem(data_hang,2,new QStandardItem(QString::number(gas_factor1)));
		model_sync->setItem(data_hang,3,new QStandardItem(QString::number(oil_factor2)));
		model_sync->setItem(data_hang,4,new QStandardItem(QString::number(gas_factor2)));

		model_sync->item(data_hang,0)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
		model_sync->item(data_hang,1)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
		model_sync->item(data_hang,2)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
		model_sync->item(data_hang,3)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
		model_sync->item(data_hang,4)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);

		model_sync->item(data_hang,1)->setForeground(QBrush(QColor(0, 0, 0)));
		model_sync->item(data_hang,2)->setForeground(QBrush(QColor(0, 0, 0)));
		model_sync->item(data_hang,3)->setForeground(QBrush(QColor(0, 0, 0)));
		model_sync->item(data_hang,4)->setForeground(QBrush(QColor(0, 0, 0)));

	}
	else
	{
		model_sync->setItem(data_hang,0,new QStandardItem(QString::number(idi+1).append("-").append(QString::number(idj+1))));
		model_sync->setItem(data_hang,1,new QStandardItem("同步失败"));
		model_sync->setItem(data_hang,2,new QStandardItem("同步失败"));
		model_sync->setItem(data_hang,3,new QStandardItem("同步失败"));
		model_sync->setItem(data_hang,4,new QStandardItem("同步失败"));

		model_sync->item(data_hang,0)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
		model_sync->item(data_hang,1)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
		model_sync->item(data_hang,2)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
		model_sync->item(data_hang,3)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
		model_sync->item(data_hang,4)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);

		model_sync->item(data_hang,1)->setForeground(QBrush(QColor(255, 0, 0)));
		model_sync->item(data_hang,2)->setForeground(QBrush(QColor(255, 0, 0)));
		model_sync->item(data_hang,3)->setForeground(QBrush(QColor(255, 0, 0)));
		model_sync->item(data_hang,4)->setForeground(QBrush(QColor(255, 0, 0)));
	}
	data_hang++;
	ui->tableView->setModel(model_sync);
	ui->tableView->show();
	if(data_hang >= sum_gun_num)
	{
		ui->label_sync_sta_show->setText("同步完成");
		ui->label_sync_sta_show->setHidden(0);
		ui->pushButton->setEnabled(1);
		ui->pushButton_2->setEnabled(1);
	}
}

void One_click_sync::on_pushButton_clicked()
{
	ui->label_sync_sta_show->setText("正在准备同步");
	ui->label_sync_sta_show->setHidden(0);
	ui->pushButton->setEnabled(0);
	ui->pushButton_2->setEnabled(0);
	qApp->processEvents();
	Flag_WaitSync = 1;
	data_hang = 0;
	model_sync->clear();//清空列表先
	QStringList labels = QObject::trUtf8("设备编号,同步油量脉冲当量1,同步气量脉冲当量1,同步油量脉冲当量2,同步气量脉冲当量2").simplified().split(",");
	model_sync->setHorizontalHeaderLabels(labels);
	ui->tableView->setModel(model_sync);
	ui->tableView->setColumnWidth(0,80);
	ui->tableView->setColumnWidth(1,152);
	ui->tableView->setColumnWidth(2,152);
	ui->tableView->setColumnWidth(3,152);
	ui->tableView->setColumnWidth(4,152);
	ui->tableView->show();
}
void One_click_sync::flicker()
{
	flag_flicker++;
	if(flag_flicker%2)
	{
		if((ui->label_sync_sta_show->text()=="同步中……")||(ui->label_sync_sta_show->text()=="正在准备同步"))
		{
			ui->label_sync_sta_show->setHidden(1);
		}
	}
	else
	{
		if((ui->label_sync_sta_show->text()=="同步中……")||(ui->label_sync_sta_show->text()=="正在准备同步"))
		{
			ui->label_sync_sta_show->setHidden(0);
		}
	}
	if(flag_flicker>=1000)
	{
		flag_flicker = 0;
	}
}
void One_click_sync::ask_data()
{
	//显示等待问询中
	qApp->processEvents();
	//问询开始
	//开始流程
	//接收返回数据
}
void One_click_sync::receive_factor_data(unsigned int idi,unsigned int idj,float oil_factor1,float gas_factor1,float oil_factor2,float gas_factor2)
{
	qDebug()<<idi<<idj<<oil_factor1<<gas_factor1<<oil_factor2<<gas_factor2;
	show_data(idi,idj,oil_factor1,gas_factor1,oil_factor2,gas_factor2);
}
//计算气液比采集器数量
void One_click_sync::gun_num_calc()
{
	sum_gun_num = 0;
	for(unsigned int i=0;i<Amount_Dispener;i++)
	{
		if(Amount_Gasgun[i]%2==0)
		{
			sum_gun_num = sum_gun_num+Amount_Gasgun[i]/2;
		}
		else
		{
			sum_gun_num = sum_gun_num+Amount_Gasgun[i]/2+1;
		}
	}
}

void One_click_sync::on_pushButton_2_clicked()
{
	//delete lcd_shot;//删除qtimer
	//model_sync->deleteLater();
	//delete ui;
	this->close();
}
