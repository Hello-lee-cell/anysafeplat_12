#include "reoilgas_pop.h"
#include "ui_reoilgas_pop.h"
#include <QStandardItemModel>
#include <QDebug>
#include <QScrollBar>
#include "myqsqlrelationmodel_centerdisp.h"
#include "config.h"
#include "systemset.h"

MyQSqlRelationModel_Centerdisp *model_list;
QStandardItemModel* model1;QStandardItemModel* model2;
QStandardItemModel* model3;QStandardItemModel* model4;
QStandardItemModel* model5;QStandardItemModel* model6;
QStandardItemModel* model7;QStandardItemModel* model8;
QStandardItemModel* model9;QStandardItemModel* model10;
QStandardItemModel* model11;QStandardItemModel* model12;
unsigned char ReoilgasPop_GunSta[96] = {0};//96把枪的状态 0正常 1预警 2报警  3真空泵故障 4气路堵塞  5燃油流速慢 10通信故障
unsigned char flag_which_page = 0;
unsigned char Flag_Reoilgas_Pop_Sta = 0;//在线油气回收详情页面打开状态 0 未打开 1打开
reoilgas_pop::reoilgas_pop(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::reoilgas_pop)
{
    ui->setupUi(this);
    this->setAttribute(Qt::WA_DeleteOnClose,true);//立即释放资源
    setWindowFlags(Qt::Tool|Qt::WindowStaysOnTopHint|Qt::FramelessWindowHint);
    ui->tabWidget->setStyleSheet("QTabBar::tab{max-height:33px;min-width:130px;background-color: rgb(170,170,255,255);border: 2px solid;padding:9px;}\
                                     QTabBar::tab:selected {background-color: white}\
                                     QTabWidget::pane {border-top:0px solid #e8f3f9;background:  transparent;}");

	move(0,85);
    Flag_Reoilgas_Pop_Sta = 1;//界面打开
    model12 = new QStandardItemModel();model1 = new QStandardItemModel();model2 = new QStandardItemModel();model3 = new QStandardItemModel();
    model4 = new QStandardItemModel();model5 = new QStandardItemModel();model6 = new QStandardItemModel();model7 = new QStandardItemModel();
    model8 = new QStandardItemModel();model9 = new QStandardItemModel();model10 = new QStandardItemModel();model11 = new QStandardItemModel();
    ui->tableView_gun_sta->setShowGrid(true);ui->tableView_gun_sta_2->setShowGrid(true);ui->tableView_gun_sta_3->setShowGrid(true);
    ui->tableView_gun_sta_4->setShowGrid(true);ui->tableView_gun_sta_5->setShowGrid(true);ui->tableView_gun_sta_6->setShowGrid(true);
    ui->tableView_gun_sta_7->setShowGrid(true);ui->tableView_gun_sta_8->setShowGrid(true);ui->tableView_gun_sta_9->setShowGrid(true);
    ui->tableView_gun_sta_10->setShowGrid(true);ui->tableView_gun_sta_11->setShowGrid(true);ui->tableView_gun_sta_12->setShowGrid(true);
    model_list = new MyQSqlRelationModel_Centerdisp;
    ui->tableView_gun_warn->verticalScrollBar()->setStyleSheet( "QScrollBar{ background: #F0F0F0; width:30px ;margin-top:0px;margin-bottom:0px }"
                                                       "QScrollBar::handle:vertical{ background: #6c65c8; min-height: 80px ;width:30px }");

    show_dispener();
    select_gun_sta();
}

reoilgas_pop::~reoilgas_pop()
{
	model1->deleteLater();model2->deleteLater();model3->deleteLater();model4->deleteLater();
	model5->deleteLater();model6->deleteLater();model7->deleteLater();model8->deleteLater();
	model9->deleteLater();model10->deleteLater();model11->deleteLater();model12->deleteLater();
	model_list->deleteLater();
    delete ui;
}

void reoilgas_pop::on_toolButton_close_clicked()
{
    Flag_Reoilgas_Pop_Sta = 0;//界面关闭

    close();
}

void reoilgas_pop::on_tabWidget_currentChanged(int index)
{
    flag_which_page = index;
    if(index == 0)
    {
        select_gun_sta();
    }
    if(index == 1)
    {
        select_gun_wrong();
    }
}
/*******************
 * 选择加油机状态页
 * *****************/
void reoilgas_pop::select_gun_sta()
{
    QString gun_id;
    QString gun_num;
    QString gun_sta = "设备正常";
    unsigned char second_column = 0;
    unsigned int row_num = 0;
    unsigned i = 0;
	model1->clear();model2->clear();model3->clear();model4->clear();model5->clear();model6->clear();
	model7->clear();model8->clear();model9->clear();model10->clear();model11->clear();model12->clear();
    // 隐藏行号
    ui->tableView_gun_sta->verticalHeader()->setHidden(1);ui->tableView_gun_sta_2->verticalHeader()->setHidden(1);
    ui->tableView_gun_sta_3->verticalHeader()->setHidden(1);ui->tableView_gun_sta_4->verticalHeader()->setHidden(1);
    ui->tableView_gun_sta_5->verticalHeader()->setHidden(1);ui->tableView_gun_sta_6->verticalHeader()->setHidden(1);
    ui->tableView_gun_sta_7->verticalHeader()->setHidden(1);ui->tableView_gun_sta_8->verticalHeader()->setHidden(1);
    ui->tableView_gun_sta_9->verticalHeader()->setHidden(1);ui->tableView_gun_sta_10->verticalHeader()->setHidden(1);
    ui->tableView_gun_sta_11->verticalHeader()->setHidden(1);ui->tableView_gun_sta_12->verticalHeader()->setHidden(1);
    //单元格不可选
    ui->tableView_gun_sta->setSelectionMode(QAbstractItemView::NoSelection);
    ui->tableView_gun_sta_2->setSelectionMode(QAbstractItemView::NoSelection);
    ui->tableView_gun_sta_3->setSelectionMode(QAbstractItemView::NoSelection);
    ui->tableView_gun_sta_4->setSelectionMode(QAbstractItemView::NoSelection);
    ui->tableView_gun_sta_5->setSelectionMode(QAbstractItemView::NoSelection);
    ui->tableView_gun_sta_6->setSelectionMode(QAbstractItemView::NoSelection);
    ui->tableView_gun_sta_7->setSelectionMode(QAbstractItemView::NoSelection);
    ui->tableView_gun_sta_8->setSelectionMode(QAbstractItemView::NoSelection);
    ui->tableView_gun_sta_9->setSelectionMode(QAbstractItemView::NoSelection);
    ui->tableView_gun_sta_10->setSelectionMode(QAbstractItemView::NoSelection);
    ui->tableView_gun_sta_11->setSelectionMode(QAbstractItemView::NoSelection);
    ui->tableView_gun_sta_12->setSelectionMode(QAbstractItemView::NoSelection);
    QStringList labels = QObject::trUtf8("枪号,状态,枪号,状态").simplified().split(",");
    model1->setHorizontalHeaderLabels(labels);
    model2->setHorizontalHeaderLabels(labels);model3->setHorizontalHeaderLabels(labels);
    model4->setHorizontalHeaderLabels(labels);model5->setHorizontalHeaderLabels(labels);model6->setHorizontalHeaderLabels(labels);
    model7->setHorizontalHeaderLabels(labels);model8->setHorizontalHeaderLabels(labels);model9->setHorizontalHeaderLabels(labels);
    model10->setHorizontalHeaderLabels(labels);model11->setHorizontalHeaderLabels(labels);model12->setHorizontalHeaderLabels(labels);


    i = 0;
    row_num = 0;
    second_column = 0;
    for(unsigned int j=0;j<Amount_Gasgun[i];j++)
    {
        if(ReoilgasPop_GunSta[i*8+j] == 0){gun_sta = "设备正常";}
        else if(ReoilgasPop_GunSta[i*8+j] == 1){gun_sta = "油枪预警";}
        else if(ReoilgasPop_GunSta[i*8+j] == 2){gun_sta = "油枪报警";}
        else if(ReoilgasPop_GunSta[i*8+j] == 3){gun_sta = "真空泵故障";}
        else if(ReoilgasPop_GunSta[i*8+j] == 4){gun_sta = "气路堵塞";}
        else if(ReoilgasPop_GunSta[i*8+j] == 5){gun_sta = "燃油流速慢";}
        else if(ReoilgasPop_GunSta[i*8+j] == 10){gun_sta = "通信故障";}
        gun_id = QString::number(i+1).append("-").append(QString::number(j+1));
        //qDebug()<<gun_id
		gun_num = Mapping_Show[i*8+j];
        if(second_column == 0)
        {
            model1->setItem(row_num, 0, new QStandardItem(gun_num));
            model1->setItem(row_num, 1, new QStandardItem(gun_sta));
            model1->item(row_num,0)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
            model1->item(row_num,1)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
            if(ReoilgasPop_GunSta[i*8+j] != 0){model1->item(row_num,1)->setForeground(QBrush(QColor(255, 0, 0)));}
            else {model1->item(row_num,1)->setForeground(QBrush(QColor(0, 0, 0)));}
            second_column = 1;
        }
        else if(second_column == 1)
        {
            model1->setItem(row_num, 2, new QStandardItem(gun_num));
            model1->setItem(row_num, 3, new QStandardItem(gun_sta));
            model1->item(row_num,2)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
            model1->item(row_num,3)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
            if(ReoilgasPop_GunSta[i*8+j] != 0){model1->item(row_num,3)->setForeground(QBrush(QColor(255, 0, 0)));}
            else {model1->item(row_num,3)->setForeground(QBrush(QColor(0, 0, 0)));}
            second_column = 0;
            row_num++;
        }

    }
    ui->tableView_gun_sta->setModel(model1);
    ui->tableView_gun_sta->setColumnWidth(0,38);
    ui->tableView_gun_sta->setColumnWidth(1,85);
    ui->tableView_gun_sta->setColumnWidth(2,38);
    ui->tableView_gun_sta->setColumnWidth(3,85);
    ui->tableView_gun_sta->show();

    i = 1;
    row_num = 0;
    second_column = 0;
    for(unsigned int j=0;j<Amount_Gasgun[i];j++)
    {
        if(ReoilgasPop_GunSta[i*8+j] == 0){gun_sta = "设备正常";}
        else if(ReoilgasPop_GunSta[i*8+j] == 1){gun_sta = "油枪预警";}
        else if(ReoilgasPop_GunSta[i*8+j] == 2){gun_sta = "油枪报警";}
        else if(ReoilgasPop_GunSta[i*8+j] == 3){gun_sta = "真空泵故障";}
        else if(ReoilgasPop_GunSta[i*8+j] == 4){gun_sta = "气路堵塞";}
        else if(ReoilgasPop_GunSta[i*8+j] == 5){gun_sta = "燃油流速慢";}
        else if(ReoilgasPop_GunSta[i*8+j] == 10){gun_sta = "通信故障";}
        gun_id = QString::number(i+1).append("-").append(QString::number(j+1));
        //qDebug()<<gun_id
		gun_num = Mapping_Show[i*8+j];
        if(second_column == 0)
        {
            model2->setItem(row_num, 0, new QStandardItem(gun_num));
            model2->setItem(row_num, 1, new QStandardItem(gun_sta));
            model2->item(row_num,0)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
            model2->item(row_num,1)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
            if(ReoilgasPop_GunSta[i*8+j] != 0){model2->item(row_num,1)->setForeground(QBrush(QColor(255, 0, 0)));}
            else {model2->item(row_num,1)->setForeground(QBrush(QColor(0, 0, 0)));}
            second_column = 1;
        }
        else if(second_column == 1)
        {
            model2->setItem(row_num, 2, new QStandardItem(gun_num));
            model2->setItem(row_num, 3, new QStandardItem(gun_sta));
            model2->item(row_num,2)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
            model2->item(row_num,3)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
            if(ReoilgasPop_GunSta[i*8+j] != 0){model2->item(row_num,3)->setForeground(QBrush(QColor(255, 0, 0)));}
            else {model2->item(row_num,3)->setForeground(QBrush(QColor(0, 0, 0)));}
            second_column = 0;
            row_num++;
        }
    }
    ui->tableView_gun_sta_2->setModel(model2);
    ui->tableView_gun_sta_2->setColumnWidth(0,38);
    ui->tableView_gun_sta_2->setColumnWidth(1,85);
    ui->tableView_gun_sta_2->setColumnWidth(2,38);
    ui->tableView_gun_sta_2->setColumnWidth(3,85);
    ui->tableView_gun_sta_2->show();

    i = 2;
    row_num = 0;
    second_column = 0;
    for(unsigned int j=0;j<Amount_Gasgun[i];j++)
    {
        if(ReoilgasPop_GunSta[i*8+j] == 0){gun_sta = "设备正常";}
        else if(ReoilgasPop_GunSta[i*8+j] == 1){gun_sta = "油枪预警";}
        else if(ReoilgasPop_GunSta[i*8+j] == 2){gun_sta = "油枪报警";}
        else if(ReoilgasPop_GunSta[i*8+j] == 3){gun_sta = "真空泵故障";}
        else if(ReoilgasPop_GunSta[i*8+j] == 4){gun_sta = "气路堵塞";}
        else if(ReoilgasPop_GunSta[i*8+j] == 5){gun_sta = "燃油流速慢";}
        else if(ReoilgasPop_GunSta[i*8+j] == 10){gun_sta = "通信故障";}
        gun_id = QString::number(i+1).append("-").append(QString::number(j+1));
        //qDebug()<<gun_id
		gun_num = Mapping_Show[i*8+j];
        if(second_column == 0)
        {
            model3->setItem(row_num, 0, new QStandardItem(gun_num));
            model3->setItem(row_num, 1, new QStandardItem(gun_sta));
            model3->item(row_num,0)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
            model3->item(row_num,1)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
            if(ReoilgasPop_GunSta[i*8+j] != 0){model3->item(row_num,1)->setForeground(QBrush(QColor(255, 0, 0)));}
            else {model3->item(row_num,1)->setForeground(QBrush(QColor(0, 0, 0)));}
            second_column = 1;
        }
        else if(second_column == 1)
        {
            model3->setItem(row_num, 2, new QStandardItem(gun_num));
            model3->setItem(row_num, 3, new QStandardItem(gun_sta));
            model3->item(row_num,2)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
            model3->item(row_num,3)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
            if(ReoilgasPop_GunSta[i*8+j] != 0){model3->item(row_num,3)->setForeground(QBrush(QColor(255, 0, 0)));}
            else {model3->item(row_num,3)->setForeground(QBrush(QColor(0, 0, 0)));}
            second_column = 0;
            row_num++;
        }
    }
    ui->tableView_gun_sta_3->setModel(model3);
    ui->tableView_gun_sta_3->setColumnWidth(0,38);
    ui->tableView_gun_sta_3->setColumnWidth(1,85);
    ui->tableView_gun_sta_3->setColumnWidth(2,38);
    ui->tableView_gun_sta_3->setColumnWidth(3,85);
    ui->tableView_gun_sta_3->show();

    i = 3;
    row_num = 0;
    second_column = 0;
    for(unsigned int j=0;j<Amount_Gasgun[i];j++)
    {
        if(ReoilgasPop_GunSta[i*8+j] == 0){gun_sta = "设备正常";}
        else if(ReoilgasPop_GunSta[i*8+j] == 1){gun_sta = "油枪预警";}
        else if(ReoilgasPop_GunSta[i*8+j] == 2){gun_sta = "油枪报警";}
        else if(ReoilgasPop_GunSta[i*8+j] == 3){gun_sta = "真空泵故障";}
        else if(ReoilgasPop_GunSta[i*8+j] == 4){gun_sta = "气路堵塞";}
        else if(ReoilgasPop_GunSta[i*8+j] == 5){gun_sta = "燃油流速慢";}
        else if(ReoilgasPop_GunSta[i*8+j] == 10){gun_sta = "通信故障";}
        gun_id = QString::number(i+1).append("-").append(QString::number(j+1));
        //qDebug()<<gun_id
		gun_num = Mapping_Show[i*8+j];
        if(second_column == 0)
        {
            model4->setItem(row_num, 0, new QStandardItem(gun_num));
            model4->setItem(row_num, 1, new QStandardItem(gun_sta));
            model4->item(row_num,0)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
            model4->item(row_num,1)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
            if(ReoilgasPop_GunSta[i*8+j] != 0){model4->item(row_num,1)->setForeground(QBrush(QColor(255, 0, 0)));}
            else {model4->item(row_num,1)->setForeground(QBrush(QColor(0, 0, 0)));}
            second_column = 1;
        }
        else if(second_column == 1)
        {
            model4->setItem(row_num, 2, new QStandardItem(gun_num));
            model4->setItem(row_num, 3, new QStandardItem(gun_sta));
            model4->item(row_num,2)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
            model4->item(row_num,3)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
            if(ReoilgasPop_GunSta[i*8+j] != 0){model4->item(row_num,3)->setForeground(QBrush(QColor(255, 0, 0)));}
            else {model4->item(row_num,3)->setForeground(QBrush(QColor(0, 0, 0)));}
            second_column = 0;
            row_num++;
        }
    }
    ui->tableView_gun_sta_4->setModel(model4);
    ui->tableView_gun_sta_4->setColumnWidth(0,38);
    ui->tableView_gun_sta_4->setColumnWidth(1,85);
    ui->tableView_gun_sta_4->setColumnWidth(2,38);
    ui->tableView_gun_sta_4->setColumnWidth(3,85);
    ui->tableView_gun_sta_4->show();

    i = 4;
    row_num = 0;
    second_column = 0;
    for(unsigned int j=0;j<Amount_Gasgun[i];j++)
    {
        if(ReoilgasPop_GunSta[i*8+j] == 0){gun_sta = "设备正常";}
        else if(ReoilgasPop_GunSta[i*8+j] == 1){gun_sta = "油枪预警";}
        else if(ReoilgasPop_GunSta[i*8+j] == 2){gun_sta = "油枪报警";}
        else if(ReoilgasPop_GunSta[i*8+j] == 3){gun_sta = "真空泵故障";}
        else if(ReoilgasPop_GunSta[i*8+j] == 4){gun_sta = "气路堵塞";}
        else if(ReoilgasPop_GunSta[i*8+j] == 5){gun_sta = "燃油流速慢";}
        else if(ReoilgasPop_GunSta[i*8+j] == 10){gun_sta = "通信故障";}
        gun_id = QString::number(i+1).append("-").append(QString::number(j+1));
        //qDebug()<<gun_id
		gun_num = Mapping_Show[i*8+j];
        if(second_column == 0)
        {
            model5->setItem(row_num, 0, new QStandardItem(gun_num));
            model5->setItem(row_num, 1, new QStandardItem(gun_sta));
            model5->item(row_num,0)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
            model5->item(row_num,1)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
            if(ReoilgasPop_GunSta[i*8+j] != 0){model5->item(row_num,1)->setForeground(QBrush(QColor(255, 0, 0)));}
            else {model5->item(row_num,1)->setForeground(QBrush(QColor(0, 0, 0)));}
            second_column = 1;
        }
        else if(second_column == 1)
        {
            model5->setItem(row_num, 2, new QStandardItem(gun_num));
            model5->setItem(row_num, 3, new QStandardItem(gun_sta));
            model5->item(row_num,2)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
            model5->item(row_num,3)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
            if(ReoilgasPop_GunSta[i*8+j] != 0){model5->item(row_num,3)->setForeground(QBrush(QColor(255, 0, 0)));}
            else {model5->item(row_num,3)->setForeground(QBrush(QColor(0, 0, 0)));}
            second_column = 0;
            row_num++;
        }
    }
    ui->tableView_gun_sta_5->setModel(model5);
    ui->tableView_gun_sta_5->setColumnWidth(0,38);
    ui->tableView_gun_sta_5->setColumnWidth(1,85);
    ui->tableView_gun_sta_5->setColumnWidth(2,38);
    ui->tableView_gun_sta_5->setColumnWidth(3,85);
    ui->tableView_gun_sta_5->show();

    i = 5;
    row_num = 0;
    second_column = 0;
    for(unsigned int j=0;j<Amount_Gasgun[i];j++)
    {
        if(ReoilgasPop_GunSta[i*8+j] == 0){gun_sta = "设备正常";}
        else if(ReoilgasPop_GunSta[i*8+j] == 1){gun_sta = "油枪预警";}
        else if(ReoilgasPop_GunSta[i*8+j] == 2){gun_sta = "油枪报警";}
        else if(ReoilgasPop_GunSta[i*8+j] == 3){gun_sta = "真空泵故障";}
        else if(ReoilgasPop_GunSta[i*8+j] == 4){gun_sta = "气路堵塞";}
        else if(ReoilgasPop_GunSta[i*8+j] == 5){gun_sta = "燃油流速慢";}
        else if(ReoilgasPop_GunSta[i*8+j] == 10){gun_sta = "通信故障";}
        gun_id = QString::number(i+1).append("-").append(QString::number(j+1));
        //qDebug()<<gun_id
		gun_num = Mapping_Show[i*8+j];
        if(second_column == 0)
        {
            model6->setItem(row_num, 0, new QStandardItem(gun_num));
            model6->setItem(row_num, 1, new QStandardItem(gun_sta));
            model6->item(row_num,0)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
            model6->item(row_num,1)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
            if(ReoilgasPop_GunSta[i*8+j] != 0){model6->item(row_num,1)->setForeground(QBrush(QColor(255, 0, 0)));}
            else {model6->item(row_num,1)->setForeground(QBrush(QColor(0, 0, 0)));}
            second_column = 1;
        }
        else if(second_column == 1)
        {
            model6->setItem(row_num, 2, new QStandardItem(gun_num));
            model6->setItem(row_num, 3, new QStandardItem(gun_sta));
            model6->item(row_num,2)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
            model6->item(row_num,3)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
            if(ReoilgasPop_GunSta[i*8+j] != 0){model6->item(row_num,3)->setForeground(QBrush(QColor(255, 0, 0)));}
            else {model6->item(row_num,3)->setForeground(QBrush(QColor(0, 0, 0)));}
            second_column = 0;
            row_num++;
        }
    }
    ui->tableView_gun_sta_6->setModel(model6);
    ui->tableView_gun_sta_6->setColumnWidth(0,38);
    ui->tableView_gun_sta_6->setColumnWidth(1,85);
    ui->tableView_gun_sta_6->setColumnWidth(2,38);
    ui->tableView_gun_sta_6->setColumnWidth(3,85);
    ui->tableView_gun_sta_6->show();

    i = 6;
    row_num = 0;
    second_column = 0;
    for(unsigned int j=0;j<Amount_Gasgun[i];j++)
    {
        if(ReoilgasPop_GunSta[i*8+j] == 0){gun_sta = "设备正常";}
        else if(ReoilgasPop_GunSta[i*8+j] == 1){gun_sta = "油枪预警";}
        else if(ReoilgasPop_GunSta[i*8+j] == 2){gun_sta = "油枪报警";}
        else if(ReoilgasPop_GunSta[i*8+j] == 3){gun_sta = "真空泵故障";}
        else if(ReoilgasPop_GunSta[i*8+j] == 4){gun_sta = "气路堵塞";}
        else if(ReoilgasPop_GunSta[i*8+j] == 5){gun_sta = "燃油流速慢";}
        else if(ReoilgasPop_GunSta[i*8+j] == 10){gun_sta = "通信故障";}
        gun_id = QString::number(i+1).append("-").append(QString::number(j+1));
        //qDebug()<<gun_id
		gun_num = Mapping_Show[i*8+j];
        if(second_column == 0)
        {
            model7->setItem(row_num, 0, new QStandardItem(gun_num));
            model7->setItem(row_num, 1, new QStandardItem(gun_sta));
            model7->item(row_num,0)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
            model7->item(row_num,1)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
            if(ReoilgasPop_GunSta[i*8+j] != 0){model7->item(row_num,1)->setForeground(QBrush(QColor(255, 0, 0)));}
            else {model7->item(row_num,1)->setForeground(QBrush(QColor(0, 0, 0)));}
            second_column = 1;
        }
        else if(second_column == 1)
        {
            model7->setItem(row_num, 2, new QStandardItem(gun_num));
            model7->setItem(row_num, 3, new QStandardItem(gun_sta));
            model7->item(row_num,2)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
            model7->item(row_num,3)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
            if(ReoilgasPop_GunSta[i*8+j] != 0){model7->item(row_num,3)->setForeground(QBrush(QColor(255, 0, 0)));}
            else {model7->item(row_num,3)->setForeground(QBrush(QColor(0, 0, 0)));}
            second_column = 0;
            row_num++;
        }
    }
    ui->tableView_gun_sta_7->setModel(model7);
    ui->tableView_gun_sta_7->setColumnWidth(0,38);
    ui->tableView_gun_sta_7->setColumnWidth(1,85);
    ui->tableView_gun_sta_7->setColumnWidth(2,38);
    ui->tableView_gun_sta_7->setColumnWidth(3,85);
    ui->tableView_gun_sta_7->show();

    i = 7;
    row_num = 0;
    second_column = 0;
    for(unsigned int j=0;j<Amount_Gasgun[i];j++)
    {
        if(ReoilgasPop_GunSta[i*8+j] == 0){gun_sta = "设备正常";}
        else if(ReoilgasPop_GunSta[i*8+j] == 1){gun_sta = "油枪预警";}
        else if(ReoilgasPop_GunSta[i*8+j] == 2){gun_sta = "油枪报警";}
        else if(ReoilgasPop_GunSta[i*8+j] == 3){gun_sta = "真空泵故障";}
        else if(ReoilgasPop_GunSta[i*8+j] == 4){gun_sta = "气路堵塞";}
        else if(ReoilgasPop_GunSta[i*8+j] == 5){gun_sta = "燃油流速慢";}
        else if(ReoilgasPop_GunSta[i*8+j] == 10){gun_sta = "通信故障";}
        gun_id = QString::number(i+1).append("-").append(QString::number(j+1));
        //qDebug()<<gun_id
		gun_num = Mapping_Show[i*8+j];
        if(second_column == 0)
        {
            model8->setItem(row_num, 0, new QStandardItem(gun_num));
            model8->setItem(row_num, 1, new QStandardItem(gun_sta));
            model8->item(row_num,0)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
            model8->item(row_num,1)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
            if(ReoilgasPop_GunSta[i*8+j] != 0){model8->item(row_num,1)->setForeground(QBrush(QColor(255, 0, 0)));}
            else {model8->item(row_num,1)->setForeground(QBrush(QColor(0, 0, 0)));}
            second_column = 1;
        }
        else if(second_column == 1)
        {
            model8->setItem(row_num, 2, new QStandardItem(gun_num));
            model8->setItem(row_num, 3, new QStandardItem(gun_sta));
            model8->item(row_num,2)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
            model8->item(row_num,3)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
            if(ReoilgasPop_GunSta[i*8+j] != 0){model8->item(row_num,3)->setForeground(QBrush(QColor(255, 0, 0)));}
            else {model8->item(row_num,3)->setForeground(QBrush(QColor(0, 0, 0)));}
            second_column = 0;
            row_num++;
        }
    }
    ui->tableView_gun_sta_8->setModel(model8);
    ui->tableView_gun_sta_8->setColumnWidth(0,38);
    ui->tableView_gun_sta_8->setColumnWidth(1,85);
    ui->tableView_gun_sta_8->setColumnWidth(2,38);
    ui->tableView_gun_sta_8->setColumnWidth(3,85);
    ui->tableView_gun_sta_8->show();

    i = 8;
    row_num = 0;
    second_column = 0;
    for(unsigned int j=0;j<Amount_Gasgun[i];j++)
    {
        if(ReoilgasPop_GunSta[i*8+j] == 0){gun_sta = "设备正常";}
        else if(ReoilgasPop_GunSta[i*8+j] == 1){gun_sta = "油枪预警";}
        else if(ReoilgasPop_GunSta[i*8+j] == 2){gun_sta = "油枪报警";}
        else if(ReoilgasPop_GunSta[i*8+j] == 3){gun_sta = "真空泵故障";}
        else if(ReoilgasPop_GunSta[i*8+j] == 4){gun_sta = "气路堵塞";}
        else if(ReoilgasPop_GunSta[i*8+j] == 5){gun_sta = "燃油流速慢";}
        else if(ReoilgasPop_GunSta[i*8+j] == 10){gun_sta = "通信故障";}
        gun_id = QString::number(i+1).append("-").append(QString::number(j+1));
        //qDebug()<<gun_id
		gun_num = Mapping_Show[i*8+j];
        if(second_column == 0)
        {
            model9->setItem(row_num, 0, new QStandardItem(gun_num));
            model9->setItem(row_num, 1, new QStandardItem(gun_sta));
            model9->item(row_num,0)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
            model9->item(row_num,1)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
            if(ReoilgasPop_GunSta[i*8+j] != 0){model9->item(row_num,1)->setForeground(QBrush(QColor(255, 0, 0)));}
            else {model9->item(row_num,1)->setForeground(QBrush(QColor(0, 0, 0)));}
            second_column = 1;
        }
        else if(second_column == 1)
        {
            model9->setItem(row_num, 2, new QStandardItem(gun_num));
            model9->setItem(row_num, 3, new QStandardItem(gun_sta));
            model9->item(row_num,2)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
            model9->item(row_num,3)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
            if(ReoilgasPop_GunSta[i*8+j] != 0){model9->item(row_num,3)->setForeground(QBrush(QColor(255, 0, 0)));}
            else {model9->item(row_num,3)->setForeground(QBrush(QColor(0, 0, 0)));}
            second_column = 0;
            row_num++;
        }
    }
    ui->tableView_gun_sta_9->setModel(model9);
    ui->tableView_gun_sta_9->setColumnWidth(0,38);
    ui->tableView_gun_sta_9->setColumnWidth(1,85);
    ui->tableView_gun_sta_9->setColumnWidth(2,38);
    ui->tableView_gun_sta_9->setColumnWidth(3,85);
    ui->tableView_gun_sta_9->show();

    i = 9;
    row_num = 0;
    second_column = 0;
    for(unsigned int j=0;j<Amount_Gasgun[i];j++)
    {
        if(ReoilgasPop_GunSta[i*8+j] == 0){gun_sta = "设备正常";}
        else if(ReoilgasPop_GunSta[i*8+j] == 1){gun_sta = "油枪预警";}
        else if(ReoilgasPop_GunSta[i*8+j] == 2){gun_sta = "油枪报警";}
        else if(ReoilgasPop_GunSta[i*8+j] == 3){gun_sta = "真空泵故障";}
        else if(ReoilgasPop_GunSta[i*8+j] == 4){gun_sta = "气路堵塞";}
        else if(ReoilgasPop_GunSta[i*8+j] == 5){gun_sta = "燃油流速慢";}
        else if(ReoilgasPop_GunSta[i*8+j] == 10){gun_sta = "通信故障";}
        gun_id = QString::number(i+1).append("-").append(QString::number(j+1));
        //qDebug()<<gun_id
		gun_num = Mapping_Show[i*8+j];
        if(second_column == 0)
        {
            model10->setItem(row_num, 0, new QStandardItem(gun_num));
            model10->setItem(row_num, 1, new QStandardItem(gun_sta));
            model10->item(row_num,0)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
            model10->item(row_num,1)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
            if(ReoilgasPop_GunSta[i*8+j] != 0){model10->item(row_num,1)->setForeground(QBrush(QColor(255, 0, 0)));}
            else {model10->item(row_num,1)->setForeground(QBrush(QColor(0, 0, 0)));}
            second_column = 1;
        }
        else if(second_column == 1)
        {
            model10->setItem(row_num, 2, new QStandardItem(gun_num));
            model10->setItem(row_num, 3, new QStandardItem(gun_sta));
            model10->item(row_num,2)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
            model10->item(row_num,3)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
            if(ReoilgasPop_GunSta[i*8+j] != 0){model10->item(row_num,3)->setForeground(QBrush(QColor(255, 0, 0)));}
            else {model10->item(row_num,3)->setForeground(QBrush(QColor(0, 0, 0)));}
            second_column = 0;
            row_num++;
        }
    }
    ui->tableView_gun_sta_10->setModel(model10);
    ui->tableView_gun_sta_10->setColumnWidth(0,38);
    ui->tableView_gun_sta_10->setColumnWidth(1,85);
    ui->tableView_gun_sta_10->setColumnWidth(2,38);
    ui->tableView_gun_sta_10->setColumnWidth(3,85);
    ui->tableView_gun_sta_10->show();

    i = 10;
    row_num = 0;
    second_column = 0;
    for(unsigned int j=0;j<Amount_Gasgun[i];j++)
    {
        if(ReoilgasPop_GunSta[i*8+j] == 0){gun_sta = "设备正常";}
        else if(ReoilgasPop_GunSta[i*8+j] == 1){gun_sta = "油枪预警";}
        else if(ReoilgasPop_GunSta[i*8+j] == 2){gun_sta = "油枪报警";}
        else if(ReoilgasPop_GunSta[i*8+j] == 3){gun_sta = "真空泵故障";}
        else if(ReoilgasPop_GunSta[i*8+j] == 4){gun_sta = "气路堵塞";}
        else if(ReoilgasPop_GunSta[i*8+j] == 5){gun_sta = "燃油流速慢";}
        else if(ReoilgasPop_GunSta[i*8+j] == 10){gun_sta = "通信故障";}
        gun_id = QString::number(i+1).append("-").append(QString::number(j+1));
        //qDebug()<<gun_id
		gun_num = Mapping_Show[i*8+j];
        if(second_column == 0)
        {
            model11->setItem(row_num, 0, new QStandardItem(gun_num));
            model11->setItem(row_num, 1, new QStandardItem(gun_sta));
            model11->item(row_num,0)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
            model11->item(row_num,1)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
            if(ReoilgasPop_GunSta[i*8+j] != 0){model11->item(row_num,1)->setForeground(QBrush(QColor(255, 0, 0)));}
            else {model11->item(row_num,1)->setForeground(QBrush(QColor(0, 0, 0)));}
            second_column = 1;
        }
        else if(second_column == 1)
        {
            model11->setItem(row_num, 2, new QStandardItem(gun_num));
            model11->setItem(row_num, 3, new QStandardItem(gun_sta));
            model11->item(row_num,2)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
            model11->item(row_num,3)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
            if(ReoilgasPop_GunSta[i*8+j] != 0){model11->item(row_num,3)->setForeground(QBrush(QColor(255, 0, 0)));}
            else {model11->item(row_num,3)->setForeground(QBrush(QColor(0, 0, 0)));}
            second_column = 0;
            row_num++;
        }
    }
    ui->tableView_gun_sta_11->setModel(model11);
    ui->tableView_gun_sta_11->setColumnWidth(0,38);
    ui->tableView_gun_sta_11->setColumnWidth(1,85);
    ui->tableView_gun_sta_11->setColumnWidth(2,38);
    ui->tableView_gun_sta_11->setColumnWidth(3,85);
    ui->tableView_gun_sta_11->show();

    i = 11;
    row_num = 0;
    second_column = 0;
    for(unsigned int j=0;j<Amount_Gasgun[i];j++)
    {
        if(ReoilgasPop_GunSta[i*8+j] == 0){gun_sta = "设备正常";}
        else if(ReoilgasPop_GunSta[i*8+j] == 1){gun_sta = "油枪预警";}
        else if(ReoilgasPop_GunSta[i*8+j] == 2){gun_sta = "油枪报警";}
        else if(ReoilgasPop_GunSta[i*8+j] == 3){gun_sta = "真空泵故障";}
        else if(ReoilgasPop_GunSta[i*8+j] == 4){gun_sta = "气路堵塞";}
        else if(ReoilgasPop_GunSta[i*8+j] == 5){gun_sta = "燃油流速慢";}
        else if(ReoilgasPop_GunSta[i*8+j] == 10){gun_sta = "通信故障";}
        gun_id = QString::number(i+1).append("-").append(QString::number(j+1));
        //qDebug()<<gun_id
		gun_num = Mapping_Show[i*8+j];
        if(second_column == 0)
        {
            model12->setItem(row_num, 0, new QStandardItem(gun_num));
            model12->setItem(row_num, 1, new QStandardItem(gun_sta));
            model12->item(row_num,0)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
            model12->item(row_num,1)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
            if(ReoilgasPop_GunSta[i*8+j] != 0){model12->item(row_num,1)->setForeground(QBrush(QColor(255, 0, 0)));}
            else {model12->item(row_num,1)->setForeground(QBrush(QColor(0, 0, 0)));}
            second_column = 1;
        }
        else if(second_column == 1)
        {
            model12->setItem(row_num, 2, new QStandardItem(gun_num));
            model12->setItem(row_num, 3, new QStandardItem(gun_sta));
            model12->item(row_num,2)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
            model12->item(row_num,3)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
            if(ReoilgasPop_GunSta[i*8+j] != 0){model12->item(row_num,3)->setForeground(QBrush(QColor(255, 0, 0)));}
            else {model12->item(row_num,3)->setForeground(QBrush(QColor(0, 0, 0)));}
            second_column = 0;
            row_num++;
        }
    }
    ui->tableView_gun_sta_12->setModel(model12);
    ui->tableView_gun_sta_12->setColumnWidth(0,38);
    ui->tableView_gun_sta_12->setColumnWidth(1,85);
    ui->tableView_gun_sta_12->setColumnWidth(2,38);
    ui->tableView_gun_sta_12->setColumnWidth(3,85);
    ui->tableView_gun_sta_12->show();
}
/********************
 * 选择加油机故障信息页
 * *****************/
void reoilgas_pop::select_gun_wrong()
{
    QHeaderView *header = ui->tableView_gun_warn->verticalHeader();
    header->setHidden(true);// 隐藏行号
    model_list->setTable("gunwarn_details");
    model_list->setSort(0,Qt::DescendingOrder);  //倒序
    model_list->setEditStrategy(QSqlTableModel::OnManualSubmit);
    model_list->setHeaderData(0,Qt::Horizontal,QObject::tr("%1").arg("ID"));
    model_list->setHeaderData(1,Qt::Horizontal,QObject::tr("%1").arg("时间"));
    model_list->setHeaderData(2,Qt::Horizontal,QObject::tr("%1").arg("设备ID"));
    model_list->setHeaderData(3,Qt::Horizontal,QObject::tr("%1").arg("枪号"));
    model_list->setHeaderData(4,Qt::Horizontal,QObject::tr("%1").arg("详细报警信息"));

    model_list->select();

    ui->tableView_gun_warn->setModel(model_list);
    ui->tableView_gun_warn->setColumnWidth(0,70);
    ui->tableView_gun_warn->setColumnWidth(1,180);
    ui->tableView_gun_warn->setColumnWidth(2,70);
    ui->tableView_gun_warn->setColumnWidth(3,40);
    ui->tableView_gun_warn->setColumnWidth(4,630);
}
/********************
 * 只显示设置的加油机的数据
 * *****************/
void reoilgas_pop::show_dispener()
{
    ui->frame_12->setHidden(1);ui->frame_1->setHidden(1);ui->frame_2->setHidden(1);
    ui->frame_3->setHidden(1);ui->frame_4->setHidden(1);ui->frame_5->setHidden(1);
    ui->frame_6->setHidden(1);ui->frame_7->setHidden(1);ui->frame_8->setHidden(1);
    ui->frame_9->setHidden(1);ui->frame_10->setHidden(1);ui->frame_11->setHidden(1);
    if(Amount_Dispener >= 1) {ui->frame_1->setHidden(0);}
    if(Amount_Dispener >= 2) {ui->frame_2->setHidden(0);}
    if(Amount_Dispener >= 3) {ui->frame_3->setHidden(0);}
    if(Amount_Dispener >= 4) {ui->frame_4->setHidden(0);}
    if(Amount_Dispener >= 5) {ui->frame_5->setHidden(0);}
    if(Amount_Dispener >= 6) {ui->frame_6->setHidden(0);}
    if(Amount_Dispener >= 7) {ui->frame_7->setHidden(0);}
    if(Amount_Dispener >= 8) {ui->frame_8->setHidden(0);}
    if(Amount_Dispener >= 9) {ui->frame_9->setHidden(0);}
    if(Amount_Dispener >= 10) {ui->frame_10->setHidden(0);}
    if(Amount_Dispener >= 11) {ui->frame_11->setHidden(0);}
    if(Amount_Dispener >= 12) {ui->frame_12->setHidden(0);}
}
/********************
 * 刷新界面
 * *****************/
void reoilgas_pop::refresh_dispener_data()
{
    //qDebug()<<"shauxin  jiemian ";
	//if(flag_which_page == 0)
	//{
        select_gun_sta();
		//}
		//if(flag_which_page == 1)
		//{
        //select_gun_wrong();  //报警记录不刷新
		//}
}
