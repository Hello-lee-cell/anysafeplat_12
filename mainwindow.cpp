﻿#include<QApplication>
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <QDialog>
#include <QFile>
#include <QDebug>
#include "config.h"
#include "file_op.h"
#include "io_op.h"
#include "network/udp.h"
#include "serial.h"
#include "radar_485.h"
#include "mythread.h"
#include "database_op.h"
#include "safty/security.h"
#include "network/main_main.h"
#include "airtightness_test.h"
#include <QElapsedTimer>
#include <timer_thread.h>
#include "oilgas/one_click_sync.h"
#include "ywythread.h"
#include"database_op.h"
#include"ywy_yfy.h"

unsigned char Flag_Draw_Type = 0;       //1-12×8=96对应加油机 101油罐压力  102管线压力  103油罐温度  104油气浓度
unsigned char flag_exchange_history = 0;
unsigned char flag_exchange_connect = 0;        //窗体切换需检测是否建立过窗体，为1则执行close
unsigned int Flag_Timeto_Closescreen = 120;    //120s无动作则关闭屏幕
//**************added for radar***/雷达相关变量
unsigned char Flag_ra_ooc = 0;  //监控时间设置，雷达开关状态位
unsigned char Flag_Onlyonce_radar[6];  //雷达一次报警
unsigned char Which_Area = 1;//默认一号区域
unsigned int Start_time_h = 0;    //监控时间设置  h   开始
unsigned int Start_time_m = 0;    //监控时间设置  m   开始
unsigned int Stop_time_h = 0;     //监控时间设置  h   停止
unsigned int Stop_time_m = 0;     //监控时间设置  m   停止
unsigned int Silent_time_h = 0;    //自动取消静音 h
unsigned int Silent_time_m = 0;    //自动取消静音 m
unsigned int Count_auto_silent = 0;  //自动取消静音  也用作自动取消静音标志位
unsigned char Flag_auto_silent = 0;  //自动取消静音标志位
unsigned int Warn_delay_m = 0;      //报警延长设置
unsigned int Warn_delay_s = 0;
unsigned char Flag_outdoor_warn = 1;    //室外报警器使能
unsigned char Flag_area_ctrl[4] = {0};    //防区开启设置
unsigned char Flag_sensitivity = 1;     //灵敏度设置  1-6
//**********added for radar****/<-
unsigned char Flag_Onlyonce_jingdian[9];//人体静电一次报警
//触摸校准计时器
unsigned int Flag_Touch_Calibration = 0;
//IIE倒计时用
int Time_IIE_oil = 0;
int Time_IIE_peo = 0;
int Flag_IIE_timeoil = 0;
int Flag_IIE_timepeo = 0;
unsigned char Flag_IIE_warn[10] = {2};//用于标记IIE历史记录，仅记录一次
unsigned char Flag_IIE_warn_value[12] = {2};//用于标记IIE电磁阀历史记录，仅记录一次
//油气回收报警记录避免重复记录
unsigned char Flag_reo_uartwrong[48] = {0};

unsigned char Flag_Show_ReoilgasPop = 0;//油气回收检测有问题的弹窗弹出标志位 0 不弹出 1 通信故障  2 报警、预警相关 3 加油机诊断
unsigned char Flag_Reoilgas_DayShow = 0;//弹窗当日不再显示 0显示 1不显示
unsigned char Flag_Reoilgas_NeverShow = 0;//弹窗不在显示 0显示 1不显示

long debug_send = 0;
long debug_read = 0;

/********************* 液位仪   ***********************************/
//进油记录
struct add_oil
{
   QString height_str;
   QString volume_str;

   float start_oil_height;
   float start_water_height;
   float end_oil_height;
   float end_water_height;
   float start_oil_volume;
   float start_water_volume;
   float end_oil_volume;
   float end_water_volume;
   float start_temp;
   float end_temp;
   float netvolume;

   int count = 0;
   uchar average[50] = {0};
   uchar size = 20;
   uchar sum = 0;
   uchar flag_entering = 1;
   bool enable = true;
   bool finish = false;

   QDateTime begin_time;
   QDateTime end_time;
};

struct add_oil add_oil_start;
struct add_oil add_oil_end;
struct add_oil add_oil_all[12];// 0 空

QString start_datetime_str;
QString end_datetime_str;

unsigned char addoil_add_num;




MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->centralWidget->installEventFilter(this);    //主屏幕安装触摸事件唤醒

//    setWindowIcon(QIcon(":/picture/jinggao.png"));

    beep_none();
    err_none();
	system("touch /opt/TouchCalibration");
	QFileInfo fi("/opt/reoilgas");
	if(fi.isDir()) //油气回收存在
	{
		qDebug()<<"reoilgas is haved";
	}
	else //不存在
	{system("mkdir /opt/reoilgas");}

	QFileInfo fifga("/opt/fga");
	if(fifga.isDir()) //可燃气体存在
	{
		qDebug()<<"fga is haved";
	}
	else //不存在
	{system("mkdir /opt/fga");}

	QFileInfo firadar("/opt/radar");
	if(firadar.isDir()) //雷达存在
	{
		qDebug()<<"radar is haved";
	}
	else //不存在
	{system("mkdir /opt/radar");}

	QFileInfo fijingdian("/opt/jingdian");
	if(fijingdian.isDir()) //静电存在
	{
		qDebug()<<"jingdian is haved";
	}
	else //不存在
	{system("mkdir /opt/jingdian");}

	system("sync");
	ui->label_touch_calibration->setHidden(0);

    /****需要创建共享内训，所以放在首位，防止ask690读取不到******/
    mythread *warn;
    warn = new mythread;
    QObject::connect(warn,SIGNAL(warning_oil_basin(int)),this,SLOT(warn_oil_set_basin(int)));
    QObject::connect(warn,SIGNAL(warning_water_basin(int)),this,SLOT(warn_water_set_basin(int)));
    QObject::connect(warn,SIGNAL(warning_sensor_basin(int)),this,SLOT(warn_sensor_set_basin(int)));
    QObject::connect(warn,SIGNAL(warning_uart_basin(int)),this,SLOT(warn_uart_set_basin(int)));
    QObject::connect(warn,SIGNAL(right_basin(int)),this,SLOT(right_set_basin(int)));

    QObject::connect(warn,SIGNAL(warning_oil_tank(int)),this,SLOT(warn_oil_set_tank(int)));
    QObject::connect(warn,SIGNAL(warning_water_tank(int)),this,SLOT(warn_water_set_tank(int)));
    QObject::connect(warn,SIGNAL(warning_high_tank(int)),this,SLOT(warn_high_set_tank(int)));
    QObject::connect(warn,SIGNAL(warning_low_tank(int)),this,SLOT(warn_low_set_tank(int)));
    QObject::connect(warn,SIGNAL(warning_pre_tank(int)),this,SLOT(warn_pre_set_tank(int)));
    QObject::connect(warn,SIGNAL(warning_warn_tank(int)),this,SLOT(warn_warn_set_tank(int)));
    QObject::connect(warn,SIGNAL(warning_sensor_tank(int)),this,SLOT(warn_sensor_set_tank(int)));
    QObject::connect(warn,SIGNAL(warning_uart_tank(int)),this,SLOT(warn_uart_set_tank(int)));
    QObject::connect(warn,SIGNAL(right_tank(int)),this,SLOT(right_set_tank(int)));
    QObject::connect(warn,SIGNAL(warning_oil_pipe(int)),this,SLOT(warn_oil_set_pipe(int)));
    QObject::connect(warn,SIGNAL(warning_water_pipe(int)),this,SLOT(warn_water_set_pipe(int)));
    QObject::connect(warn,SIGNAL(warning_sensor_pipe(int)),this,SLOT(warn_sensor_set_pipe(int)));
    QObject::connect(warn,SIGNAL(warning_uart_pipe(int)),this,SLOT(warn_uart_set_pipe(int)));
    QObject::connect(warn,SIGNAL(right_pipe(int)),this,SLOT(right_set_pipe(int)));
    QObject::connect(warn,SIGNAL(warning_oil_dispener(int)),this,SLOT(warn_oil_set_dispener(int)));
    QObject::connect(warn,SIGNAL(warning_water_dispener(int)),this,SLOT(warn_water_set_dispener(int)));
    QObject::connect(warn,SIGNAL(warning_sensor_dispener(int)),this,SLOT(warn_sensor_set_dispener(int)));
    QObject::connect(warn,SIGNAL(warning_uart_dispener(int)),this,SLOT(warn_uart_set_dispener(int)));
    QObject::connect(warn,SIGNAL(right_dispener(int)),this,SLOT(right_set_dispener(int)));
    QObject::connect(warn,SIGNAL(set_pressure_number()),this,SLOT(Press_number()));
    //人体静电信号
    QObject::connect(warn,SIGNAL(set_renti(unsigned char,unsigned char)),this,SLOT(set_label_jingdian(unsigned char,unsigned char)));
    //IIE信号
    QObject::connect(warn,SIGNAL(IIE_display(unsigned char, int,int,int,int)),this,SLOT(IIE_show(unsigned char, int,int,int,int)));
	//IIE电磁阀信号
	QObject::connect(warn,SIGNAL(IIE_Electromagnetic_Show(unsigned char)),this,SLOT(show_IIE_electromagnetic(unsigned char)));
    warn->start();









	ui->widget_iie_electromagnetic->setHidden(1);
	ui->label_tongxinguzhang->setHidden(1);
    qDebug()<<"my thread is start!";
    /******************涉及共享内存*************************/

    ui->label_delay_jilu->setHidden(1);
	//时间显示
    timer_lcd = new QTimer();
    timer_lcd->setInterval(1000);
	timer_lcd->start();
	connect(timer_lcd,SIGNAL(timeout()),this,SLOT(Timerout_lcd()));
    //防止时间卡顿，增加了一个单独的槽函数处理每秒的函数
    connect(this,SIGNAL(time_out_slotfunction_s(QDateTime)),this,SLOT(time_out_slotfunction(QDateTime)));

	//初始化界面显示，隐藏一部分
    init_tabwidget();
	ui->tabWidget->setTabEnabled(0,Flag_screen_zaixian);
	ui->tabWidget->setTabEnabled(1,Flag_screen_xielou);
	ui->tabWidget->setTabEnabled(2,Flag_screen_radar);
	ui->tabWidget->setTabEnabled(3,Flag_screen_safe);
	ui->tabWidget->setTabEnabled(4,Flag_screen_burngas);
	ui->tabWidget->setTabEnabled(5,Flag_screen_cc);
    ui->tabWidget->setTabEnabled(6,Flag_screen_ywy);
	ui->tabWidget->setStyleSheet("QTabBar::tab:abled {max-height:82px;min-width:147px;background-color: rgb(170,170,255,255);border: 0px solid;border-top-left-radius: 0px;border-top-right-radius: 0px;padding:0px;}\
	                                QTabBar::tab:!selected {margin-top: 0px;background-color:transparent;}\
                                    QTabBar::tab:selected {background-color: white}\
	                                QTabWidget::pane {border-top:0px solid #e8f3f9;background:  transparent;}\
	                             QTabBar::tab:disabled {width: 0; height: 0; color: transparent;padding:0px;border: 0px solid}");

/*********************************************
	gif_right = new QMovie(":/picture/right.gif");
	gif_uart = new QMovie(":/picture/uartwro.gif");
	gif_oil = new QMovie(":/picture/oilwro.gif");
	gif_water = new QMovie(":/picture/waterwro.gif");
	gif_sensor = new QMovie(":/picture/sensorwro.gif");
	gif_nosensor = new QMovie(":/picture/nosensor.gif");
	gif_high = new QMovie(":/picture/highwro.gif");
	gif_low = new QMovie(":/picture/lowwro.gif");
	gif_presurepre = new QMovie(":/picture/presurepre.gif");
	gif_presurewarn = new QMovie(":/picture/presurewarn.gif");


	gif_presurepre_pre = new QMovie(":/picture/presurepre_pre.gif");
	gif_presurewarn_pre = new QMovie(":/picture/presurewarn_pre.gif");
	gif_right_pre = new QMovie(":/picture/right_pre.gif");
	gif_sensor_pre = new QMovie(":/picture/sensorwro_pre.gif");
	gif_uart_pre = new QMovie(":/picture/uartwro_pre.gif");
	gif_crash_warn = new QMovie(":/picture/crashwarn.gif");
	语音文件
			gif_right->start();
	gif_uart->start();
	gif_oil->start();
	gif_water->start();
	gif_sensor->start();
	gif_nosensor->start();
	gif_high->start();
	gif_low->start();
	gif_presurepre->start();
	gif_presurewarn->start();
	gif_crash_warn->start();
	gif_presurepre_pre->start();
	gif_presurewarn_pre->start();
	gif_right_pre->start();
	gif_sensor_pre->start();
	gif_uart_pre->start();
********************************/

	gif_radar = new QMovie(":/picture/radar.gif");
	gif_radar->start();
	ui->label_radar_gif->setMovie(gif_radar);
	gif_radar->stop();

//************************泄漏初始化*begin*****************************//
	int i = 6;  //设备数目初始化使用  //之前是5，改为6加了一个压力法显示模式位
	QFile config("/opt/config.txt");
	if(!config.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qDebug()<<"Can't open the config file!"<<endl;
	}
	QTextStream in(&config);
	QString line;
	while(i)
	{
		line = in.readLine();
		QByteArray num_config = line.toLatin1();
		char *mm = num_config.data();
		unsigned char j;
		switch(i)
		{
		case 1:j = mm[0] - 48;
			if (j == 0 || j == 1)
			{
				Flag_pre_mode = j;
			}
			else
			{
				Flag_pre_mode = 125;
			}
			break;

		case 2: j = mm[0] - 48;
			if(j <= 3)
			{
				Test_Method = mm[0] - 48;
			}
			else
			{
				Test_Method = 0x03;
			}
			break;
		case 6: j = mm[0] - 48;
			if(j <= 8)
			{
				count_tank = mm[0]-48;
			}
			else
			{
				count_tank = 0;
			}

			break;
		case 5: j = mm[0] - 48;
			if(j <= 8)
			{
				count_pipe = mm[0]-48;
			}
			else
			{
				count_pipe = 0;
			}
			switch(count_pipe)
			{
			case 0:
				ui->label_91->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_92->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_93->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_94->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_95->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_96->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_97->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_98->setStyleSheet("border-image: url(:/picture/nosensor.png);");
				break;
			case 1:
				ui->label_91->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_92->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_93->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_94->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_95->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_96->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_97->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_98->setStyleSheet("border-image: url(:/picture/nosensor.png);");
				break;
			case 2:

				ui->label_91->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_92->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_93->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_94->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_95->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_96->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_97->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_98->setStyleSheet("border-image: url(:/picture/nosensor.png);");
				break;
			case 3:

				ui->label_91->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_92->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_93->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_94->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_95->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_96->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_97->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_98->setStyleSheet("border-image: url(:/picture/nosensor.png);");
				break;
			case 4:

				ui->label_91->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_92->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_93->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_94->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_95->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_96->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_97->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_98->setStyleSheet("border-image: url(:/picture/nosensor.png);");
				break;
			case 5:

				ui->label_91->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_92->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_93->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_94->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_95->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_96->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_97->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_98->setStyleSheet("border-image: url(:/picture/nosensor.png);");
				break;
			case 6:

				ui->label_91->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_92->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_93->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_94->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_95->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_96->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_97->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_98->setStyleSheet("border-image: url(:/picture/nosensor.png);");
				break;
			case 7:

				ui->label_91->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_92->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_93->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_94->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_95->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_96->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_97->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_98->setStyleSheet("border-image: url(:/picture/nosensor.png);");
				break;
			case 8:
				ui->label_91->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_92->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_93->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_94->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_95->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_96->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_97->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_98->setStyleSheet("border-image: url(:/picture/right.png);");

			}
			break;
		case 4: j = mm[0] - 48;
			if(j <=8)
			{
				count_dispener = mm[0]-48;
			}
			else
			{
				count_dispener = 0;
			}
			switch(count_dispener)
			{
			case 0:
				ui->label_81->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_82->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_83->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_84->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_85->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_86->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_87->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_88->setStyleSheet("border-image: url(:/picture/nosensor.png);");
				break;
			case 1:
				ui->label_81->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_82->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_83->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_84->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_85->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_86->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_87->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_88->setStyleSheet("border-image: url(:/picture/nosensor.png);");
				break;
			case 2:

				ui->label_81->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_82->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_83->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_84->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_85->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_86->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_87->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_88->setStyleSheet("border-image: url(:/picture/nosensor.png);");
				break;
			case 3:

				ui->label_81->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_82->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_83->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_84->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_85->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_86->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_87->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_88->setStyleSheet("border-image: url(:/picture/nosensor.png);");
				break;
			case 4:

				ui->label_81->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_82->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_83->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_84->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_85->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_86->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_87->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_88->setStyleSheet("border-image: url(:/picture/nosensor.png);");
				break;
			case 5:

				ui->label_81->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_82->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_83->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_84->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_85->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_86->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_87->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_88->setStyleSheet("border-image: url(:/picture/nosensor.png);");
				break;
			case 6:

				ui->label_81->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_82->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_83->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_84->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_85->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_86->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_87->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_88->setStyleSheet("border-image: url(:/picture/nosensor.png);");
				break;
			case 7:

				ui->label_81->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_82->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_83->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_84->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_85->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_86->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_87->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_88->setStyleSheet("border-image: url(:/picture/nosensor.png);");
				break;
			case 8:
				ui->label_81->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_82->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_83->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_84->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_85->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_86->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_87->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_8->setStyleSheet("border-image: url(:/picture/right.png);");

			}
			break;
		case 3: j = mm[0] - 48;
			if(j <= 8)
			{
				count_basin = mm[0]-48;
			}
			else
			{
				count_basin = 0;
			}
			switch(count_basin)
			{
			case 0:
				ui->label->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_2->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_3->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_4->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_5->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_6->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_7->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_8->setStyleSheet("border-image: url(:/picture/nosensor.png);");
				break;
			case 1:
				ui->label->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_2->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_3->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_4->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_5->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_6->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_7->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_8->setStyleSheet("border-image: url(:/picture/nosensor.png);");
				break;
			case 2:

				ui->label->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_2->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_3->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_4->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_5->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_6->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_7->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_8->setStyleSheet("border-image: url(:/picture/nosensor.png);");
				break;
			case 3:

				ui->label->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_2->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_3->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_4->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_5->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_6->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_7->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_8->setStyleSheet("border-image: url(:/picture/nosensor.png);");
				break;
			case 4:

				ui->label->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_2->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_3->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_4->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_5->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_6->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_7->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_8->setStyleSheet("border-image: url(:/picture/nosensor.png);");
				break;
			case 5:

				ui->label->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_2->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_3->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_4->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_5->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_6->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_7->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_8->setStyleSheet("border-image: url(:/picture/nosensor.png);");
				break;
			case 6:

				ui->label->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_2->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_3->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_4->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_5->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_6->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_7->setStyleSheet("border-image: url(:/picture/nosensor.png);");

				ui->label_8->setStyleSheet("border-image: url(:/picture/nosensor.png);");
				break;
			case 7:

				ui->label->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_2->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_3->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_4->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_5->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_6->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_7->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_8->setStyleSheet("border-image: url(:/picture/nosensor.png);");
				break;
			case 8:
				ui->label->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_2->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_3->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_4->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_5->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_6->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_7->setStyleSheet("border-image: url(:/picture/right.png);");

				ui->label_8->setStyleSheet("border-image: url(:/picture/right.png);");

			}
			break;

		}
		i--;
		if(i == 0)
		{
			amount_tank_setted();
		}
	}
	config.close();
//************************泄漏初始化*end*****************************//

//*********************安全防护设备初始化*begin**************************//

	init_jingdian_write();
	init_IIE();
	init_security();

	if(Flag_IIE == 0)
	{
		ui->label_IIE_work->setText("设备关闭");
		ui->label_IIE_tongxinguzhang->setHidden(1);//隐藏设备故障
	}
	else
	{
		ui->label_IIE_work->setText("设备开启");
		ui->label_IIE_tongxinguzhang->setHidden(1);//隐藏设备故障
	}


//*********************安全防护设备初始化*end**************************//

//*********************防撞柱初始化*begin**************************//
	crash_column_reset();//防撞柱数量重绘制
	ui->label_cra1->setStyleSheet("border-image: url(:/picture/right.png);");ui->label_cra2->setStyleSheet("border-image: url(:/picture/right.png);");
	ui->label_cra3->setStyleSheet("border-image: url(:/picture/right.png);");ui->label_cra4->setStyleSheet("border-image: url(:/picture/right.png);");
	ui->label_cra5->setStyleSheet("border-image: url(:/picture/right.png);");ui->label_cra6->setStyleSheet("border-image: url(:/picture/right.png);");
	ui->label_cra7->setStyleSheet("border-image: url(:/picture/right.png);");ui->label_cra8->setStyleSheet("border-image: url(:/picture/right.png);");
	ui->label_cra9->setStyleSheet("border-image: url(:/picture/right.png);");ui->label_cra10->setStyleSheet("border-image: url(:/picture/right.png);");
	ui->label_cra11->setStyleSheet("border-image: url(:/picture/right.png);");ui->label_cra12->setStyleSheet("border-image: url(:/picture/right.png);");
	ui->label_cra13->setStyleSheet("border-image: url(:/picture/right.png);");ui->label_cra14->setStyleSheet("border-image: url(:/picture/right.png);");
	ui->label_cra15->setStyleSheet("border-image: url(:/picture/right.png);");ui->label_cra16->setStyleSheet("border-image: url(:/picture/right.png);");
	ui->label_cra17->setStyleSheet("border-image: url(:/picture/right.png);");ui->label_cra18->setStyleSheet("border-image: url(:/picture/right.png);");
	ui->label_cra19->setStyleSheet("border-image: url(:/picture/right.png);");ui->label_cra20->setStyleSheet("border-image: url(:/picture/right.png);");
	ui->label_cra21->setStyleSheet("border-image: url(:/picture/right.png);");ui->label_cra22->setStyleSheet("border-image: url(:/picture/right.png);");
	ui->label_cra23->setStyleSheet("border-image: url(:/picture/right.png);");ui->label_cra24->setStyleSheet("border-image: url(:/picture/right.png);");
	ui->label_cra25->setStyleSheet("border-image: url(:/picture/right.png);");ui->label_cra26->setStyleSheet("border-image: url(:/picture/right.png);");
	ui->label_cra27->setStyleSheet("border-image: url(:/picture/right.png);");ui->label_cra28->setStyleSheet("border-image: url(:/picture/right.png);");
	ui->label_cra29->setStyleSheet("border-image: url(:/picture/right.png);");ui->label_cra30->setStyleSheet("border-image: url(:/picture/right.png);");
	ui->label_cra31->setStyleSheet("border-image: url(:/picture/right.png);");ui->label_cra32->setStyleSheet("border-image: url(:/picture/right.png);");
//*********************防撞柱初始化*end**************************//

//***********************************added for radar**************************************/
    ui->toolButton_syssetshow->setHidden(1);
    timer_drw = new QTimer();
    timer_drw->setInterval(300);
	connect(timer_drw,SIGNAL(timeout()),this,SLOT(area_pointdrw()));
    ui->paint_area->xAxis->setRange(-25,25);
    ui->paint_area->yAxis->setRange(0,50);

    ui->paint_area->yAxis->setAutoTickStep(false);  //取消自动布长
    ui->paint_area->xAxis->setAutoTickStep(false);
    ui->paint_area->xAxis2->setAutoTickStep(false);
    ui->paint_area->yAxis2->setAutoTickStep(false);
    ui->paint_area->yAxis->setTickStep(5);  //设置步长
    ui->paint_area->xAxis->setTickStep(5);
    ui->paint_area->xAxis2->setTicks(0);    //取消分段小线段显示
    ui->paint_area->yAxis2->setTicks(0);
    ui->paint_area->xAxis->setTickLabels(false);    //x label
    ui->paint_area->yAxis->setTickLabels(false);
    ui->paint_area->xAxis2->setTickLabels(false);    //x label
    ui->paint_area->yAxis2->setTickLabels(false);
    ui->paint_area->xAxis2->setVisible(true);
    ui->paint_area->yAxis2->setVisible(true);

	ui->paint_area->setBackground(QColor(250, 250, 250));    //背景色
	ui->paint_area->xAxis->grid()->setPen(QPen(QColor(152, 203, 254)));   //其他网格线
	ui->paint_area->yAxis->grid()->setPen(QPen(QColor(152, 203, 254)));
	ui->paint_area->xAxis->grid()->setZeroLinePen(QPen(QColor(152, 203, 254)));//零线

    QPen pen1(QColor(255,0,0));
    QPen pen2(QColor(0,104,183));
    QPen pen3(QColor(0,0,255));
    QPen pen4(QColor(0,89,130));
    QPen pen5(QColor(0,86,31));
    QPen pen6(QColor(0,0,0));
    for(uchar i = 0;i<36;i++)   //0-5 6-11 12-17 18-23 24-29 30-35
    {
        ui->paint_area->addGraph();     //添加图层
        if(i<=5)
        {
            ui->paint_area->graph(i)->setPen(pen1);
        }
        if((i<=11)&&(i>=6))
        {
            ui->paint_area->graph(i)->setPen(pen2);
        }
        if((i<=17)&&(i>=12))
        {
            ui->paint_area->graph(i)->setPen(pen3);
        }
        if((i<=23)&&(i>=18))
        {
            ui->paint_area->graph(i)->setPen(pen4);
        }
        if((i<=29)&&(i>=24))
        {
            ui->paint_area->graph(i)->setPen(pen5);
        }
        if((i<=35)&&(i>=30))
        {
            ui->paint_area->graph(i)->setPen(pen6);
        }
    }
    ui->paint_area->addGraph();//描点图层
	//智能设置初始化
	QFile config_radar_zn("/opt/radar/config_zn.txt");
	if(!config_radar_zn.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qDebug()<<"Can't open the config_radar_zn file!"<<endl;
	}
	QTextStream out_radar_zn(&config_radar_zn);
	QString line_radar_zn;
	uchar i_ra = 14;
	while(i_ra)
	{
		line_radar_zn = out_radar_zn.readLine();
		QByteArray radar_time_config = line_radar_zn.toLatin1();
		char *ra = radar_time_config.data();
		switch(i_ra)
		{
		    case 1:Flag_sensitivity = ra[0] - 48;        //灵敏度
			        break;
		    case 2:Flag_area_ctrl[3] = ra[0] -48;        //4#防区使能
			        break;
		    case 3:Flag_area_ctrl[2] = ra[0] -48;        //3#防区使能
			        break;
		    case 4:Flag_area_ctrl[1] = ra[0] -48;        //2#防区使能
			        break;
		    case 5:Flag_area_ctrl[0] = ra[0] -48;       //1#防区使能
			        break;
		    case 6: Flag_outdoor_warn = ra[0] - 48;       //室外报警使能
			        break;
		    case 7: if(!ra[1])  //报警延长设置    s
			        {
				        Warn_delay_s = ra[0] - 48;
			        }
			        else
			        {
				        Warn_delay_s = ((ra[0]-48)*10 + (ra[1]-48));
			        }
			        break;
		    case 8: if(!ra[1])  //报警延长设置    m
			        {
				        Warn_delay_m = ra[0] - 48;
			        }
			        else
			        {
				        Warn_delay_m = ((ra[0]-48)*10 + (ra[1]-48));
			        }
			        break;
		    case 9: if(!ra[1])  //自动取消静音时间  m
			        {
				        Silent_time_m = ra[0] - 48;
			        }
			        else
			        {
				        Silent_time_m = ((ra[0]-48)*10 + (ra[1]-48));
			        }
			        break;
		    case 10: Silent_time_h = ra[0] - 48;     //自动取消静音时间  h
			        break;
		    case 11: if(!ra[1])  //关闭时间  m
			        {
				        Stop_time_m = ra[0] - 48;
			        }
			        else
			        {
				        Stop_time_m = ((ra[0]-48)*10 + (ra[1]-48));
			        }
			        break;
		    case 12: if(!ra[1])      //关闭时间  h
			        {
				        Stop_time_h = ra[0] - 48;
			        }
			        else
			        {
				        Stop_time_h = ((ra[0]-48)*10 + (ra[1]-48));
			        }
			        break;
		    case 13: if(!ra[1])      //开启时间 m
			        {
				        Start_time_m = ra[0] - 48;
			        }
			        else
			        {
				        Start_time_m = ((ra[0]-48)*10 + (ra[1]-48));
			        }
			        break;
		    case 14: if(!ra[1])      //开启时间 h
			        {
				        Start_time_h = ra[0] - 48;
			        }
			        else
			        {
				        Start_time_h = ((ra[0]-48)*10 + (ra[1]-48));
			        }
			        break;
		}
		i_ra--;
	}
	config_radar_zn.close();
	//区域设置初始化
	//区域1
	QFile config_radar_area1("/opt/radar/boundary_machine1_area1.txt");
	if(!config_radar_area1.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qDebug()<<"Can't open the config_radar_area1 file!"<<endl;
	}
	QTextStream out_radar_area1(&config_radar_area1);
	QString line_radar_area1;
	uchar i_area = 12;
	while(i_area)
	{
		line_radar_area1 = out_radar_area1.readLine();
		QByteArray radar_area1_config = line_radar_area1.toLatin1();
		char *ra = radar_area1_config.data();
		switch(i_area)
		{
		    case 12:
			        Master_Boundary_Point_Disp[0][0][0][1] = atoi(ra);
			        break;
		    case 11:
			        Master_Boundary_Point_Disp[0][0][0][0] = atoi(ra);
			        break;
		    case 10:
			        Master_Boundary_Point_Disp[0][0][1][1] = atoi(ra);
			        break;
		    case 9:
			        Master_Boundary_Point_Disp[0][0][1][0] = atoi(ra);
			        break;
		    case 8:
			        Master_Boundary_Point_Disp[0][0][2][1] = atoi(ra);
			        break;
		    case 7:
			        Master_Boundary_Point_Disp[0][0][2][0] = atoi(ra);
			        break;
		    case 6:
			        Master_Boundary_Point_Disp[0][0][3][1] = atoi(ra);
			        break;
		    case 5:
			        Master_Boundary_Point_Disp[0][0][3][0] = atoi(ra);
			        break;
		    case 4:
			        Master_Boundary_Point_Disp[0][0][4][1] = atoi(ra);
			        break;
		    case 3:
			        Master_Boundary_Point_Disp[0][0][4][0] = atoi(ra);
			        break;
		    case 2:
			        Master_Boundary_Point_Disp[0][0][5][1] = atoi(ra);
			        break;
		    case 1:
			        Master_Boundary_Point_Disp[0][0][5][0] = atoi(ra);
			        break;
		}
		i_area--;
	}
	config_radar_area1.close();
	//区域二
	QFile config_radar_area2("/opt/radar/boundary_machine1_area2.txt");
	if(!config_radar_area2.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qDebug()<<"Can't open the config_radar_area2 file!"<<endl;
	}
	QTextStream out_radar_area2(&config_radar_area2);
	QString line_radar_area2;
	uchar i_area2 = 12;
	while(i_area2)
	{
		line_radar_area2 = out_radar_area2.readLine();
		QByteArray radar_area2_config = line_radar_area2.toLatin1();
		char *ra = radar_area2_config.data();
		switch(i_area2)
		{
		    case 12:
			        Master_Boundary_Point_Disp[0][1][0][1] = atoi(ra);
			        break;
		    case 11:
			        Master_Boundary_Point_Disp[0][1][0][0] = atoi(ra);
			        break;
		    case 10:
			        Master_Boundary_Point_Disp[0][1][1][1] = atoi(ra);
			        break;
		    case 9:
			        Master_Boundary_Point_Disp[0][1][1][0] = atoi(ra);
			        break;
		    case 8:
			        Master_Boundary_Point_Disp[0][1][2][1] = atoi(ra);
			        break;
		    case 7:
			        Master_Boundary_Point_Disp[0][1][2][0] = atoi(ra);
			        break;
		    case 6:
			        Master_Boundary_Point_Disp[0][1][3][1] = atoi(ra);
			        break;
		    case 5:
			        Master_Boundary_Point_Disp[0][1][3][0] = atoi(ra);
			        break;
		    case 4:
			        Master_Boundary_Point_Disp[0][1][4][1] = atoi(ra);
			        break;
		    case 3:
			        Master_Boundary_Point_Disp[0][1][4][0] = atoi(ra);
			        break;
		    case 2:
			        Master_Boundary_Point_Disp[0][1][5][1] = atoi(ra);
			        break;
		    case 1:
			        Master_Boundary_Point_Disp[0][1][5][0] = atoi(ra);
			        break;
		}
		i_area2--;
	}
	config_radar_area2.close();

	//区域三
	QFile config_radar_area3("/opt/radar/boundary_machine1_area3.txt");
	if(!config_radar_area3.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qDebug()<<"Can't open the config_radar_area3 file!"<<endl;
	}
	QTextStream out_radar_area3(&config_radar_area3);
	QString line_radar_area3;
	uchar i_area3 = 12;
	while(i_area3)
	{
		line_radar_area3 = out_radar_area3.readLine();
		QByteArray radar_area3_config = line_radar_area3.toLatin1();
		char *ra = radar_area3_config.data();
		switch(i_area3)
		{
		    case 12:
			        Master_Boundary_Point_Disp[0][2][0][1] = atoi(ra);
			        break;
		    case 11:
			        Master_Boundary_Point_Disp[0][2][0][0] = atoi(ra);
			        break;
		    case 10:
			        Master_Boundary_Point_Disp[0][2][1][1] = atoi(ra);
			        break;
		    case 9:
			        Master_Boundary_Point_Disp[0][2][1][0] = atoi(ra);
			        break;
		    case 8:
			        Master_Boundary_Point_Disp[0][2][2][1] = atoi(ra);
			        break;
		    case 7:
			        Master_Boundary_Point_Disp[0][2][2][0] = atoi(ra);
			        break;
		    case 6:
			        Master_Boundary_Point_Disp[0][2][3][1] = atoi(ra);
			        break;
		    case 5:
			        Master_Boundary_Point_Disp[0][2][3][0] = atoi(ra);
			        break;
		    case 4:
			        Master_Boundary_Point_Disp[0][2][4][1] = atoi(ra);
			        break;
		    case 3:
			        Master_Boundary_Point_Disp[0][2][4][0] = atoi(ra);
			        break;
		    case 2:
			        Master_Boundary_Point_Disp[0][2][5][1] = atoi(ra);
			        break;
		    case 1:
			        Master_Boundary_Point_Disp[0][2][5][0] = atoi(ra);
			        break;
		}
		i_area3--;
	}
	config_radar_area3.close();
	//区域四
	QFile config_radar_area4("/opt/radar/boundary_machine1_area4.txt");
	if(!config_radar_area4.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qDebug()<<"Can't open the config_radar_area4 file!"<<endl;
	}
	QTextStream out_radar_area4(&config_radar_area4);
	QString line_radar_area4;
	uchar i_area4 = 12;
	while(i_area4)
	{
		line_radar_area4 = out_radar_area4.readLine();
		QByteArray radar_area4_config = line_radar_area4.toLatin1();
		char *ra = radar_area4_config.data();
		switch(i_area4)
		{
		    case 12:
			        Master_Boundary_Point_Disp[0][3][0][1] = atoi(ra);
			        break;
		    case 11:
			        Master_Boundary_Point_Disp[0][3][0][0] = atoi(ra);
			        break;
		    case 10:
			        Master_Boundary_Point_Disp[0][3][1][1] = atoi(ra);
			        break;
		    case 9:
			        Master_Boundary_Point_Disp[0][3][1][0] = atoi(ra);
			        break;
		    case 8:
			        Master_Boundary_Point_Disp[0][3][2][1] = atoi(ra);
			        break;
		    case 7:
			        Master_Boundary_Point_Disp[0][3][2][0] = atoi(ra);
			        break;
		    case 6:
			        Master_Boundary_Point_Disp[0][3][3][1] = atoi(ra);
			        break;
		    case 5:
			        Master_Boundary_Point_Disp[0][3][3][0] = atoi(ra);
			        break;
		    case 4:
			        Master_Boundary_Point_Disp[0][3][4][1] = atoi(ra);
			        break;
		    case 3:
			        Master_Boundary_Point_Disp[0][3][4][0] = atoi(ra);
			        break;
		    case 2:
			        Master_Boundary_Point_Disp[0][3][5][1] = atoi(ra);
			        break;
		    case 1:
			        Master_Boundary_Point_Disp[0][3][5][0] = atoi(ra);
			        break;
		}
		i_area4--;
	}
	config_radar_area4.close();
	//区域五
	QFile config_radar_area5("/opt/radar/boundary_machine1_area5.txt");
	if(!config_radar_area5.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qDebug()<<"Can't open the config_radar_area5 file!"<<endl;
	}
	QTextStream out_radar_area5(&config_radar_area5);
	QString line_radar_area5;
	uchar i_area5 = 12;
	while(i_area5)
	{
		line_radar_area5 = out_radar_area5.readLine();
		QByteArray radar_area5_config = line_radar_area5.toLatin1();
		char *ra = radar_area5_config.data();
		switch(i_area5)
		{
		    case 12:
			        Master_Boundary_Point_Disp[0][4][0][1] = atoi(ra);
			        break;
		    case 11:
			        Master_Boundary_Point_Disp[0][4][0][0] = atoi(ra);
			        break;
		    case 10:
			        Master_Boundary_Point_Disp[0][4][1][1] = atoi(ra);
			        break;
		    case 9:
			        Master_Boundary_Point_Disp[0][4][1][0] = atoi(ra);
			        break;
		    case 8:
			        Master_Boundary_Point_Disp[0][4][2][1] = atoi(ra);
			        break;
		    case 7:
			        Master_Boundary_Point_Disp[0][4][2][0] = atoi(ra);
			        break;
		    case 6:
			        Master_Boundary_Point_Disp[0][4][3][1] = atoi(ra);
			        break;
		    case 5:
			        Master_Boundary_Point_Disp[0][4][3][0] = atoi(ra);
			        break;
		    case 4:
			        Master_Boundary_Point_Disp[0][4][4][1] = atoi(ra);
			        break;
		    case 3:
			        Master_Boundary_Point_Disp[0][4][4][0] = atoi(ra);
			        break;
		    case 2:
			        Master_Boundary_Point_Disp[0][4][5][1] = atoi(ra);
			        break;
		    case 1:
			        Master_Boundary_Point_Disp[0][4][5][0] = atoi(ra);
			        break;
		}
		i_area5--;
	}
	config_radar_area5.close();
	//区域六
	QFile config_radar_area6("/opt/radar/boundary_machine1_area6.txt");
	if(!config_radar_area6.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qDebug()<<"Can't open the config_radar_area6 file!"<<endl;
	}
	QTextStream out_radar_area6(&config_radar_area6);
	QString line_radar_area6;
	uchar i_area6 = 12;
	while(i_area6)
	{
		line_radar_area6 = out_radar_area6.readLine();
		QByteArray radar_area6_config = line_radar_area6.toLatin1();
		char *ra = radar_area6_config.data();
		switch(i_area6)
		{
		    case 12:
			        Master_Boundary_Point_Disp[0][5][0][1] = atoi(ra);
			        break;
		    case 11:
			        Master_Boundary_Point_Disp[0][5][0][0] = atoi(ra);
			        break;
		    case 10:
			        Master_Boundary_Point_Disp[0][5][1][1] = atoi(ra);
			        break;
		    case 9:
			        Master_Boundary_Point_Disp[0][5][1][0] = atoi(ra);
			        break;
		    case 8:
			        Master_Boundary_Point_Disp[0][5][2][1] = atoi(ra);
			        break;
		    case 7:
			        Master_Boundary_Point_Disp[0][5][2][0] = atoi(ra);
			        break;
		    case 6:
			        Master_Boundary_Point_Disp[0][5][3][1] = atoi(ra);
			        break;
		    case 5:
			        Master_Boundary_Point_Disp[0][5][3][0] = atoi(ra);
			        break;
		    case 4:
			        Master_Boundary_Point_Disp[0][5][4][1] = atoi(ra);
			        break;
		    case 3:
			        Master_Boundary_Point_Disp[0][5][4][0] = atoi(ra);
			        break;
		    case 2:
			        Master_Boundary_Point_Disp[0][5][5][1] = atoi(ra);
			        break;
		    case 1:
			        Master_Boundary_Point_Disp[0][5][5][0] = atoi(ra);
			        break;
		}
		i_area6--;
	}
	config_radar_area6.close();
	area_painted();
	//雷达背景曲线初始化
	QFile config_radar_backgroud("/opt/radar/backgroundvalue_machine1.txt");
	if(!config_radar_backgroud.open(QIODevice::ReadOnly|QIODevice::Text))
	{
		qDebug()<<"Can't open the config_radar_background file!"<<endl;
	}
	QTextStream out_radar_backgroud(&config_radar_backgroud);
	QString line_radar_backgroud;
	unsigned int i_backgroud = 0;
	unsigned char j_backgroud = 0;
	line_radar_backgroud = out_radar_backgroud.readLine();
	while(i_backgroud < 180)
	{
		line_radar_backgroud = out_radar_backgroud.readLine();
		QByteArray radar_backgroud_config = line_radar_backgroud.toLatin1();
		char *ra = radar_backgroud_config.data();
		Master_Back_Groud_Value[0][i_backgroud][j_backgroud] = atoi(ra);


		j_backgroud++;
		if(j_backgroud > 1)
		{
			j_backgroud = 0;
			i_backgroud++;
		}
	}
	config_radar_backgroud.close();

	//Uart线程:雷达
	uart_run = new uartthread();
	connect(uart_run,SIGNAL(warn_to_mainwindowstatelabel()),this,SLOT(label_state_setted()));
    uart_run->start();
	on_toolButton_radar1_clicked(); //待完善，需删除
//************************************added for radar*************************************/<-

//******************************油气回收初始化*begin******************************/
	//初始化油站信息
	StationMessageInit();
	//油气回收初始化数据
    init_Liquid_resistance();//初始化最远端加油机设备号
    init_alset();//初始化气液比相关设置
    init_Pressure_Transmitters_Mode();//初始化压力变送器型号
    init_network_Version();//初始化网络上传版本
    init_isoosi_set();//初始化isoosi协议相关配置
	//init_xielou_network();//初始化泄漏网络上传  转移到main函数tcp创建之前
    init_reoilgas_warnpop();//弹窗设置相关读取
	init_myserver_network();//服务器初始化
	Controller_Version_init();//控制器硬件版本初始化
	PreTemGasSensor_Type_init();//在线监测传感器类型初始化
	//网络相关初始化
	init_post_network();

	QFile if_post("/opt/reoilgas/Post_Enable");
	if(if_post.exists())//如果存在，网络开启
	{
		Flag_Postsend_Enable = 1;
	}
	else
	{
		Flag_Postsend_Enable = 0;
	}
//	Flag_Postsend_Enable = 1;//默认开启

    ui->widget_dispen_details->setHidden(1);
	//*********油气回收界面初始化
	ui->widget_gundetail->setHidden(1);
	ui->label_env_tankpre->setHidden(1);
	ui->label_env_pipepre->setHidden(1);
	ui->label_env_tanktem->setHidden(1);
	ui->label_env_tanknongdu->setHidden(1);
	ui->label_oilgas_tankpre_2->setHidden(1);
	ui->label_oilgas_pipepre_2->setHidden(1);
	ui->label_oilgas_tanktemp_2->setHidden(1);
	ui->label_oilgas_tanknongdu_2->setHidden(1);

	ui->label_gun_1_1u->setHidden(1);
	ui->label_gun_1_2u->setHidden(1);
	ui->label_gun_1_3u->setHidden(1);
	ui->label_gun_1_4u->setHidden(1);
	ui->label_gun_1_5u->setHidden(1);
	ui->label_gun_1_6u->setHidden(1);
	ui->label_gun_2_1u->setHidden(1);
	ui->label_gun_2_2u->setHidden(1);
	ui->label_gun_2_3u->setHidden(1);
	ui->label_gun_2_4u->setHidden(1);
	ui->label_gun_2_5u->setHidden(1);
	ui->label_gun_2_6u->setHidden(1);
	ui->label_gun_3_1u->setHidden(1);
	ui->label_gun_3_2u->setHidden(1);
	ui->label_gun_3_3u->setHidden(1);
	ui->label_gun_3_4u->setHidden(1);
	ui->label_gun_3_5u->setHidden(1);
	ui->label_gun_3_6u->setHidden(1);
	ui->label_gun_4_1u->setHidden(1);
	ui->label_gun_4_2u->setHidden(1);
	ui->label_gun_4_3u->setHidden(1);
	ui->label_gun_4_4u->setHidden(1);
	ui->label_gun_4_5u->setHidden(1);
	ui->label_gun_4_6u->setHidden(1);
	ui->label_gun_5_1u->setHidden(1);
	ui->label_gun_5_2u->setHidden(1);
	ui->label_gun_5_3u->setHidden(1);
	ui->label_gun_5_4u->setHidden(1);
	ui->label_gun_5_5u->setHidden(1);
	ui->label_gun_5_6u->setHidden(1);
	ui->label_gun_6_1u->setHidden(1);
	ui->label_gun_6_2u->setHidden(1);
	ui->label_gun_6_3u->setHidden(1);
	ui->label_gun_6_4u->setHidden(1);
	ui->label_gun_6_5u->setHidden(1);
	ui->label_gun_6_6u->setHidden(1);
	ui->label_gun_7_1u->setHidden(1);
	ui->label_gun_7_2u->setHidden(1);
	ui->label_gun_7_3u->setHidden(1);
	ui->label_gun_7_4u->setHidden(1);
	ui->label_gun_7_5u->setHidden(1);
	ui->label_gun_7_6u->setHidden(1);
	ui->label_gun_8_1u->setHidden(1);
	ui->label_gun_8_2u->setHidden(1);
	ui->label_gun_8_3u->setHidden(1);
	ui->label_gun_8_4u->setHidden(1);
	ui->label_gun_8_5u->setHidden(1);
	ui->label_gun_8_6u->setHidden(1);
	ui->label_gun_9_1u->setHidden(1);
	ui->label_gun_9_2u->setHidden(1);
	ui->label_gun_9_3u->setHidden(1);
	ui->label_gun_9_4u->setHidden(1);
	ui->label_gun_9_5u->setHidden(1);
	ui->label_gun_9_6u->setHidden(1);
	ui->label_gun_10_1u->setHidden(1);
	ui->label_gun_10_2u->setHidden(1);
	ui->label_gun_10_3u->setHidden(1);
	ui->label_gun_10_4u->setHidden(1);
	ui->label_gun_10_5u->setHidden(1);
	ui->label_gun_10_6u->setHidden(1);
	ui->label_gun_11_1u->setHidden(1);
	ui->label_gun_11_2u->setHidden(1);
	ui->label_gun_11_3u->setHidden(1);
	ui->label_gun_11_4u->setHidden(1);
	ui->label_gun_11_5u->setHidden(1);
	ui->label_gun_11_6u->setHidden(1);
	ui->label_gun_12_1u->setHidden(1);
	ui->label_gun_12_2u->setHidden(1);
	ui->label_gun_12_3u->setHidden(1);
	ui->label_gun_12_4u->setHidden(1);
	ui->label_gun_12_5u->setHidden(1);
	ui->label_gun_12_6u->setHidden(1);
    //oilgas 数量初始化
    QFile config_amount_dispenerandgun(CONFIG_REOILGAS);
    if(!config_amount_dispenerandgun.open(QIODevice::ReadOnly|QIODevice::Text))
    {
        qDebug()<<"Can't open the oilgasconfig.ini file!"<<endl;
    }
    QTextStream out_oilgas_amount(&config_amount_dispenerandgun);
    QString line_oilgas_amount;
    //加油机数量
    line_oilgas_amount  = out_oilgas_amount.readLine();
    QByteArray oilgas_amount_config = line_oilgas_amount.toLatin1();
    char *ra_oilgas = oilgas_amount_config.data();
    Amount_Dispener = atoi(ra_oilgas);
    //油枪数量
    for(unsigned char i = 0;i < 12;i++)
    {
        line_oilgas_amount = out_oilgas_amount.readLine();
        oilgas_amount_config = line_oilgas_amount.toLatin1();
        ra_oilgas = oilgas_amount_config.data();
        Amount_Gasgun[i] = atoi(ra_oilgas);
    }
    printf("Amount Of Dispener: %d\n",Amount_Dispener);
    for(int i = 0;i<12;i++)
    {
        printf("Each Dispener's Gun:%d\n",Amount_Gasgun[i]);
    }
    config_amount_dispenerandgun.close();

    //油枪映射编号初始化
    QFile config_mapping(CONFIG_MAPPING);
    if(!config_mapping.open(QIODevice::ReadOnly|QIODevice::Text))
    {
        qDebug()<<"Can't open the mapping table file!"<<endl;
    }
    QTextStream out_mapping(&config_mapping);
    QString line_mapping_table;
    QByteArray qbline_mapping;
    char *read_mapping;
    for(int i = 0; i < 96; i++)
    {
        line_mapping_table = out_mapping.readLine();
        qbline_mapping = line_mapping_table.toLatin1();
        read_mapping = qbline_mapping.data();
        Mapping[i] = atoi(read_mapping);
    }
    config_mapping.close();

	//油枪映射编号初始化
	QFile config_mapping_show(CONFIG_MAPPING_SHOW);
	if(!config_mapping_show.open(QIODevice::ReadOnly|QIODevice::Text))
	{
		qDebug()<<"Can't open the mappingshow table file!"<<endl;
	}
	QTextStream out_mapping_show(&config_mapping_show);
	QString line_mapping_table_show;
	for(int i = 0; i < 96; i++)
	{
		line_mapping_table_show = out_mapping_show.readLine();
		Mapping_Show[i] = line_mapping_table_show;
	}
	config_mapping_show.close();

	//油枪品号初始化
	QFile config_mapping_oil(CONFIG_MAPPING_OILNO);
	if(!config_mapping_oil.open(QIODevice::ReadOnly|QIODevice::Text))
	{
		qDebug()<<"Can't open the mappingoil table file!"<<endl;
	}
	QTextStream out_mapping_oil(&config_mapping_oil);
	QString line_mapping_table_oil;
	for(int i = 0; i < 96; i++)
	{
		line_mapping_table_oil = out_mapping_oil.readLine();
		Mapping_OilNo[i] = line_mapping_table_oil;
	}
	config_mapping_oil.close();


    //oilgas 油因子初始化
    QFile config_factor_fuel(CONFIG_OIL_FACTOR);
    if(!config_factor_fuel.open(QIODevice::ReadOnly|QIODevice::Text))
    {
        qDebug()<<"Can't open the factorfuelconfig.txt file!"<<endl;
    }
    QTextStream out_factor_fuel(&config_factor_fuel);
    out_factor_fuel.readLine();
    for(unsigned char i = 0;i < 12;i++)
    {
        for(unsigned char j = 0;j < 8;j++)
        {
            QString line_factor_fuel = out_factor_fuel.readLine();
            QByteArray config_fact_fuel = line_factor_fuel.toLatin1();
            char *ra_factor = config_fact_fuel.data();
            Fueling_Factor[i][j] = atof(ra_factor);
        }
    }
    config_factor_fuel.close();

    //oilgas 气因子初始化

    QFile config_factor_gas(CONFIG_GAS_FACTOR);
    if(!config_factor_gas.open(QIODevice::ReadOnly|QIODevice::Text))
    {
        qDebug()<<"Can't open the factorgasconfig.txt file!"<<endl;
    }
    QTextStream out_factor_gas(&config_factor_gas);
    out_factor_gas.readLine();
    for(unsigned char i = 0;i < 12;i++)
    {
        for(unsigned char j = 0;j < 8;j++)
        {
            QString line_factor_gas = out_factor_gas.readLine();
            QByteArray config_fact_gas = line_factor_gas.toLatin1();
            char *ra_factor = config_fact_gas.data();
            Gas_Factor[i][j] = atof(ra_factor);
        }
    }
    config_factor_gas.close();
    set_amount_oilgas_dispen(Amount_Dispener);
    set_amount_oilgas_gun();
    //预警天数初始化
    for(unsigned char i = 1;i < 13;i++)
    {
        for(unsigned char j = 1;j < 9;j++)
        {
            QString path_accumto = QString("/opt/reoilgas/info_accum/%1-%2").arg(i).arg(j);
            QFile file_accumto(path_accumto);
            if(!file_accumto.open(QIODevice::ReadOnly|QIODevice::Text))
            {
                qDebug()<<"Can't Open Accountto *-* files!"<<endl;
            }
            QTextStream out_accumto_oilgas(&file_accumto);
            QString line_accumto = out_accumto_oilgas.readLine();
            Flag_Accumto[i-1][j-1] = line_accumto.toInt();
            //printf("%d-%d accum is---------------------- %d\n",i,j,Flag_Accumto[i-1][j-1]);
            file_accumto.close();
        }
    }
	gun_state_show();
    //isoosi添加
    thread_isoosi = new net_isoosi;
	//isoosi添加 重庆
	thread_isoosi_cq = new net_isoosi_cq;
	//合肥协议，暂时使用
	thread_isoosi_hefei = new net_isoosi_hefei;
	//服务器上传泄漏数据
	myserver_thread = new myserver;
	//post 添加 唐山
	post_message = new post_webservice;
	//post 湖南
	post_message_hunan = new post_webservice_hunan;
	//post 佛山
	post_message_foshan = new post_foshan;
	post_message_foshan->moveToThread(post_message_foshan);
    //post 东莞
    post_message_dg = new post_webservice_dongguan;
	//oilgas线程
	uart_reoilgas = new reoilgasthread();
	//可燃气体线程
	uart_fga = new FGA1000_485();

    /********************** 液位仪初始化 ***********************************/
    Initialization_label();
    YWY_Initialization_Data();

    for(unsigned char i = 0;i < Amount_OilTank;i++)
    {
        sum_Tangan_Amount = sum_Tangan_Amount + Tangan_Amount[i];
    }

    draw_OilTank( Amount_OilTank );
    draw_Tangan();

    uart_ywy = new ywythread();
    connect(uart_ywy,SIGNAL(Send_Height_Signal(unsigned char,QString,QString,QString)),SLOT(Display_Height_Data(unsigned char,QString,QString,QString)));
    connect(uart_ywy,SIGNAL(Send_alarm_info(unsigned char,unsigned char)),SLOT(Display_alarm_Tangan_Data(unsigned char,unsigned char)));
    connect(uart_ywy,SIGNAL(Set_tangan_add_success(unsigned char)),SLOT(set_Tangan_add_success_slot(unsigned char)));
    connect(uart_ywy,SIGNAL(Send_compensation_Signal(unsigned char,unsigned char,float)),SLOT(Send_compensation_slot(unsigned char,unsigned char,float)));
    connect(uart_ywy,SIGNAL(compensation_set_result(unsigned char,unsigned char,QString)),SLOT(compensation_set_result_slot(unsigned char,unsigned char,QString)));

    if(!flag_place_hoider){uart_ywy->start();}

    ui->widget_add_oil->setHidden(1);
    ui->widget_add_oil_auto->setHidden(1);

    timer_open_addoil = new QTimer(this);
    timer_open_addoil->setInterval(1000*3);
    connect(timer_open_addoil,SIGNAL(timeout()),SLOT(open_addoil_windows()));

    timer_close_addoil = new QTimer(this);
    timer_close_addoil->setInterval(1000*60);
    connect(timer_close_addoil,SIGNAL(timeout()),SLOT(close_addoil_windows()));
    connect(this,SIGNAL(signal_Display_addoil_data(uchar)),SLOT(slot_Display_addoil_data(uchar)));

    ywy_yfy_thread *yfythread = new ywy_yfy_thread();

    if(!flag_place_hoider){yfythread->start();}


//界面显示的一些数据
	connect(uart_reoilgas,SIGNAL(Version_To_Mainwindow(unsigned char,unsigned char)),this,SLOT(Version_Recv_FromReoilgas(unsigned char,unsigned char)));
	connect(uart_reoilgas,SIGNAL(Setinfo_To_Mainwindow(unsigned char,unsigned char,unsigned char,unsigned char,unsigned char,unsigned char,unsigned char,unsigned char,unsigned char,unsigned char)),this,SLOT(Setinfo_Recv_FromReoilgas(unsigned char,unsigned char,unsigned char,unsigned char,unsigned char,unsigned char,unsigned char,unsigned char,unsigned char,unsigned char)));
	connect(uart_reoilgas,SIGNAL(Warn_UartWrong_Mainwindowdisp(unsigned char,unsigned char)),this,SLOT(Reoilgas_UartWrong_Maindisped(unsigned char,unsigned char)));
	connect(uart_reoilgas,SIGNAL(Reoilgas_Factor_Setover()),this,SLOT(Reoilgas_Factor_SettedtoSys()));
	connect(uart_fga,SIGNAL(data_show()),this,SLOT(Disp_Reoilgas_Env()));
	connect(uart_fga,SIGNAL(normal_fga(int)),this,SLOT(Env_warn_normal_fga(int)));
	connect(uart_fga,SIGNAL(alarm_hig_fga(int)),this,SLOT(Env_warn_hig_fga(int)));
	connect(uart_fga,SIGNAL(alarm_low_fga(int)),this,SLOT(Env_warn_low_fga(int)));
	connect(uart_fga,SIGNAL(alarm_sensor_fga(int)),this,SLOT(Env_warn_sensor_fga(int)));
	connect(uart_fga,SIGNAL(alarm_uart_fga(int)),this,SLOT(Env_warn_uart_fga(int)));
	connect(uart_fga,SIGNAL(alarm_sensor_de_fga(int)),this,SLOT(Env_warn_sensor_de_fga(int)));
	connect(uart_fga,SIGNAL(alarm_uart_pressure(int)),this,SLOT(Env_warn_pre_uart(int)));
	connect(uart_fga,SIGNAL(normal_pressure(int)),this,SLOT(Env_warn_pre_normal(int)));
	connect(uart_fga,SIGNAL(alarm_early_pre(int)),this,SLOT(Env_warn_pre_pre(int)));
	connect(uart_fga,SIGNAL(alarm_warn_pre(int)),this,SLOT(Env_warn_pre_warn(int)));
	connect(uart_fga,SIGNAL(alarm_tem_normal()),this,SLOT(Env_warn_normal_tem()));
	connect(uart_fga,SIGNAL(alarm_tem_warn()),this,SLOT(Env_warn_uart_tem()));
	connect(uart_fga,SIGNAL(init_burngas_setted(int)),this,SLOT(amount_burngas_setted(int)));
	//connect(this,SIGNAL(Time_Fga_1s()),uart_fga,SLOT(time_time()));

//发送泄漏报警数据
	connect(warn,SIGNAL(myserver_send_single(QString,QString,QString,QString,QString)),myserver_thread,SLOT(xielousta(QString,QString,QString,QString,QString)),Qt::DirectConnection);

//发送网络故障数据
	connect(this,SIGNAL(Send_Wrongsdata(QString,QString)),post_message,SLOT(Send_Wrongsdata(QString,QString)));
	connect(this,SIGNAL(Send_Wrongsdat_HuNan(QString,QString)),post_message_hunan,SLOT(Send_Wrongsdata_HuNan(QString,QString)));
	connect(this,SIGNAL(send_wrong_foshan(QString,QString,QString)),post_message_foshan,SLOT(send_wrong(QString,QString,QString)));
    connect(this,SIGNAL(Send_Wrongsdat_dongguan(QString,QString)),post_message_dg,SLOT(Send_Wrongsdata(QString,QString)));
	//isoosi添加 重庆
	connect(this,SIGNAL(refueling_wrongdata_cq(QString)),thread_isoosi_cq,SLOT(refueling_wrongdata(QString)));

//发送网络油枪加油数据
	//post添加 槽函数直连
	connect(uart_reoilgas,SIGNAL(Send_Oilgundata(QString,QString,QString,QString,QString,QString,QString,QString,QString)),post_message,SLOT(Send_Oilgundata(QString,QString,QString,QString,QString,QString,QString,QString,QString)));
	connect(uart_reoilgas,SIGNAL(Send_Oilgundata_HuNan(QString,QString,QString,QString,QString,QString,QString,QString,QString,QString,QString,QString)),post_message_hunan,SLOT(Send_Oilgundata_HuNan(QString,QString,QString,QString,QString,QString,QString,QString,QString,QString,QString,QString)));
	//post佛山
	connect(uart_reoilgas,SIGNAL(send_gundata_foshan(QString,QString,QString,QString,QString,QString,QString,QString,QString,QString,QString,QString,QString)),post_message_foshan,SLOT(send_gundata(QString,QString,QString,QString,QString,QString,QString,QString,QString,QString,QString,QString,QString)));
	//isoosi添加 槽函数直连
	connect(uart_reoilgas,SIGNAL(refueling_gun_data(QString,QString,QString,QString,QString,QString,QString)),thread_isoosi,SLOT(refueling_gun_data(QString,QString,QString,QString,QString,QString,QString)),Qt::DirectConnection);
	//isoosi添加重庆 槽函数直连
	connect(uart_reoilgas,SIGNAL(refueling_gun_data_cq(QString,QString,QString,QString,QString,QString,QString,QString,QString)),thread_isoosi_cq,SLOT(refueling_gun_data(QString,QString,QString,QString,QString,QString,QString,QString,QString)),Qt::DirectConnection);
	//myserver添加 槽函数直连
	connect(uart_reoilgas,SIGNAL(refueling_gun_data_myserver(QString,QString,QString,QString,QString,QString,QString)),myserver_thread,SLOT(refueling_gun_data(QString,QString,QString,QString,QString,QString,QString)),Qt::DirectConnection);
    //isoosi合肥添加
    connect(uart_reoilgas,SIGNAL(refueling_gun_data_hefei(QString)),thread_isoosi_hefei,SLOT(send_gundata(QString)));
    //post东莞
    connect(uart_reoilgas,SIGNAL(Send_Oilgundata_dg(QString,QString,QString,QString,QString,QString,QString,QString,QString,QString,QString)),post_message_dg,SLOT(Send_Oilgundata(QString,QString,QString,QString,QString,QString,QString,QString,QString,QString,QString)));

//post形式发送网络数据
	//post添加 槽函数直连
	connect(uart_fga,SIGNAL(Send_Warndata(QString,QString,QString,QString,QString,QString,QString,QString)),post_message,SLOT(Send_Warndata(QString,QString,QString,QString,QString,QString,QString,QString)));
	connect(uart_fga,SIGNAL(Send_Surroundingsdata(QString,QString,QString,QString)),post_message,SLOT(Send_Surroundingsdata(QString,QString,QString,QString)));
	connect(uart_fga,SIGNAL(Send_Wrongsdata(QString,QString)),post_message,SLOT(Send_Wrongsdata(QString,QString)));
	connect(uart_fga,SIGNAL(Send_Stagundata(QString,QString)),post_message,SLOT(Send_Stagundata(QString,QString)));
	connect(uart_fga,SIGNAL(Send_Closegunsdata(QString,QString,QString,QString,QString)),post_message,SLOT(Send_Closegunsdata(QString,QString,QString,QString,QString)));
	connect(uart_fga,SIGNAL(Send_Configurationdata(QString,QString,QString,QString,QString,QString)),post_message,SLOT(Send_Configurationdata(QString,QString,QString,QString,QString,QString)));

	connect(uart_fga,SIGNAL(Send_Warndata_HuNan(QString,QString,QString,QString,QString,QString,QString,QString,QString,QString,QString)),post_message_hunan,SLOT(Send_Warndata_HuNan(QString,QString,QString,QString,QString,QString,QString,QString,QString,QString,QString)));
	connect(uart_fga,SIGNAL(Send_Surroundingsdata_HuNan(QString,QString,QString,QString,QString,QString,QString,QString)),post_message_hunan,SLOT(Send_Surroundingsdata_HuNan(QString,QString,QString,QString,QString,QString,QString,QString)));
	connect(uart_fga,SIGNAL(Send_Wrongsdata_HuNan(QString,QString)),post_message_hunan,SLOT(Send_Wrongsdata_HuNan(QString,QString)));
	connect(uart_fga,SIGNAL(Send_Stagundata_HuNan(QString,QString)),post_message_hunan,SLOT(Send_Stagundata_HuNan(QString,QString)));
	connect(uart_fga,SIGNAL(Send_Closegunsdata_HuNan(QString,QString,QString,QString,QString)),post_message_hunan,SLOT(Send_Closegunsdata_HuNan(QString,QString,QString,QString,QString)));
	connect(uart_fga,SIGNAL(Send_Configurationdata_HuNan(QString,QString,QString,QString,QString,QString)),post_message_hunan,SLOT(Send_Configurationdata_HuNan(QString,QString,QString,QString,QString,QString)));

    connect(uart_fga,SIGNAL(Send_Configurationdata_dg(QString,QString,QString,QString,QString,QString)),post_message_dg,SLOT(Send_Configurationdata(QString,QString,QString,QString,QString,QString)));
    connect(uart_fga,SIGNAL(Send_Surroundingsdata_dg(QString,QString,QString,QString,QString,QString,QString)),post_message_dg,SLOT(Send_Surroundingsdata(QString,QString,QString,QString,QString,QString,QString)));
    connect(uart_fga,SIGNAL(Send_Wrongsdata_dg(QString,QString)),post_message_dg,SLOT(Send_Wrongsdata(QString,QString)));
    connect(uart_fga,SIGNAL(Send_Stagundata_dg(QString,QString)),post_message_dg,SLOT(Send_Stagundata(QString,QString)));
    connect(uart_fga,SIGNAL(Send_Closegunsdata_dg(QString,QString,QString,QString,QString)),post_message_dg,SLOT(Send_Closegunsdata(QString,QString,QString,QString,QString)));
    connect(uart_fga,SIGNAL(Send_Warndata_dg(QString,QString,QString,QString,QString,QString,QString,QString,QString,QString,QString)),post_message_dg,SLOT(Send_Warndata(QString,QString,QString,QString,QString,QString,QString,QString,QString,QString,QString)));

	//post佛山
	connect(uart_fga,SIGNAL(send_environment_foshan(QString,QString,QString,QString,QString,QString,QString,QString)),post_message_foshan,SLOT(send_environment(QString,QString,QString,QString,QString,QString,QString,QString)));
	connect(uart_fga,SIGNAL(send_gunoperate_foshan(QString,QString,QString,QString,QString,QString)),post_message_foshan,SLOT(send_gunoperate(QString,QString,QString,QString,QString,QString)));
	connect(uart_fga,SIGNAL(send_gunsta_foshan(QString,QString,QString)),post_message_foshan,SLOT(send_gunsta(QString,QString,QString)));
	connect(uart_fga,SIGNAL(send_setinfo_foshan(QString,QString,QString,QString,QString,QString,QString,QString)),post_message_foshan,SLOT(send_setinfo(QString,QString,QString,QString,QString,QString,QString,QString)));
	connect(uart_fga,SIGNAL(send_warninfo_foshan(QString,QString,QString,QString,QString,QString,QString,QString,QString,QString,QString,QString)),post_message_foshan,SLOT(send_warninfo(QString,QString,QString,QString,QString,QString,QString,QString,QString,QString,QString,QString)));
	connect(uart_fga,SIGNAL(send_wrong_foshan(QString,QString,QString)),post_message_foshan,SLOT(send_wrong(QString,QString,QString)));

//isoosi形式发送网络数据
	//isoosi添加  槽函数直连
	connect(uart_fga,SIGNAL(environmental_data(QString,QString,QString,QString,QString,QString)),thread_isoosi,SLOT(environmental_data(QString,QString,QString,QString,QString,QString)),Qt::DirectConnection);
	connect(uart_fga,SIGNAL(gun_warn_data(QString,QString,QString,QString,QString,QString,QString,QString)),thread_isoosi,SLOT(gun_warn_data(QString,QString,QString,QString,QString,QString,QString,QString)),Qt::DirectConnection);
	connect(uart_fga,SIGNAL(refueling_gun_sta(QString)),thread_isoosi,SLOT(refueling_gun_sta(QString)),Qt::DirectConnection);
	connect(uart_fga,SIGNAL(refueling_gun_stop(QString,QString,QString)),thread_isoosi,SLOT(refueling_gun_stop(QString,QString,QString)),Qt::DirectConnection);
	connect(uart_fga,SIGNAL(setup_data(QString,QString,QString,QString)),thread_isoosi,SLOT(setup_data(QString,QString,QString,QString)),Qt::DirectConnection);
	//isoosi添加 重庆 槽函数直连
	connect(uart_fga,SIGNAL(environmental_data_cq(QString,QString,QString,QString,QString,QString)),thread_isoosi_cq,SLOT(environmental_data(QString,QString,QString,QString,QString,QString)),Qt::DirectConnection);
	connect(uart_fga,SIGNAL(gun_warn_data_cq(QString,QString,QString,QString,QString,QString,QString,QString,QString,QString,QString)),thread_isoosi_cq,SLOT(gun_warn_data(QString,QString,QString,QString,QString,QString,QString,QString,QString,QString,QString)),Qt::DirectConnection);
	connect(uart_fga,SIGNAL(refueling_gun_stop_cq(QString,QString,QString)),thread_isoosi_cq,SLOT(refueling_gun_stop(QString,QString,QString)),Qt::DirectConnection);
	connect(uart_fga,SIGNAL(setup_data_cq(QString,QString,QString,QString)),thread_isoosi_cq,SLOT(setup_data(QString,QString,QString,QString)),Qt::DirectConnection);
	connect(uart_fga,SIGNAL(refueling_wrongdata_cq(QString)),thread_isoosi_cq,SLOT(refueling_wrongdata(QString)),Qt::DirectConnection);
	//isoosi合肥添加
	connect(uart_fga,SIGNAL(Send_Surroundingsdata_HeFei(QString,QString,QString)),thread_isoosi_hefei,SLOT(send_surround_message(QString,QString,QString)));
	//myserver添加  槽函数直连
	connect(uart_fga,SIGNAL(environmental_data_myserver(QString,QString,QString,QString,QString,QString)),myserver_thread,SLOT(environmental_data(QString,QString,QString,QString,QString,QString)),Qt::DirectConnection);
	connect(uart_fga,SIGNAL(gun_warn_data_myserver(QString,QString,QString,QString,QString,QString,QString,QString)),myserver_thread,SLOT(gun_warn_data(QString,QString,QString,QString,QString,QString,QString,QString)),Qt::DirectConnection);
	connect(uart_fga,SIGNAL(refueling_gun_sta_myserver(QString)),myserver_thread,SLOT(refueling_gun_sta(QString)),Qt::DirectConnection);
	connect(uart_fga,SIGNAL(refueling_gun_stop_myserver(QString,QString,QString)),myserver_thread,SLOT(refueling_gun_stop(QString,QString,QString)),Qt::DirectConnection);
	connect(uart_fga,SIGNAL(setup_data_myserver(QString,QString,QString,QString)),myserver_thread,SLOT(setup_data(QString,QString,QString,QString)),Qt::DirectConnection);

	connect(myserver_thread,SIGNAL(Myserver_First_Client()),uart_fga,SLOT(Myserver_First_Client()));

	thread_isoosi_cq->start();
	thread_isoosi->start();
	thread_isoosi_hefei->start();
	myserver_thread->start();
    if(flag_place_hoider){uart_reoilgas->start(); }//ywy 暂时占用
//	uart_fga->start();
    if(flag_place_hoider){uart_fga->start(); }//ywy 暂时占用
	post_message_foshan->start();//佛山协议需要启动

	timer_thread *delay_time = new timer_thread;
	connect(delay_time,SIGNAL(delay_1000ms()),uart_fga,SLOT(time_time()),Qt::DirectConnection);
	delay_time->start();
//******************************油气回收初始化*end******************************/

    ui->widget_warn_rom->setHidden(1);//存储空间不足显示
    ui->label_warn_rom->setHidden(1);//存储空间不足显示
    ui->label_delay_data->setHidden(1);//数据分析中
    ui->widget_warn_zaixianjiance->setHidden(1);//弹窗
	add_value_operateinfo("","系统已启动");

//    printf(" Flag_screen_ywy  value:   %d   ",Flag_screen_ywy);fflush(stdout);
}

MainWindow::~MainWindow()
{
    delete ui;
}
bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if(watched == ui->centralWidget)
    {
        if(event->type() == QEvent::MouseButtonPress)
        {
            system("echo '0' > /sys/class/graphics/fb0/blank"); //退出待机
            Flag_Timeto_Closescreen = 120;
            return true;
        }
    }
    return QObject::eventFilter(watched,event);
}

void MainWindow::on_pushButton_clicked()            //login
{
    //system("echo 3 > /proc/sys/vm/drop_caches");//清理内存
    if(flag_exchange_history == 1)
    {
        emit closeing_history();
        ui->pushButton_7->setEnabled(1);
    }
    if(flag_exchange_connect == 1)
    {
        emit closeing_connect();
        ui->pushButton_2->setEnabled(1);
    }

    ui->pushButton->setEnabled(0);
    ui->pushButton_2->setEnabled(0);
    ui->pushButton_7->setEnabled(0);

    Log = new login;
    connect(Log,SIGNAL(login_enter(int)),this,SLOT(login_enter_set(int)));
    connect(Log,SIGNAL(mainwindow_enable()),this,SLOT(mainwindow_enabled()));
    Log->show();
}
void MainWindow::login_enter_set(int t)
{
	systemset_exec = new systemset;
	connect(systemset_exec,SIGNAL(amount_basin_reset(int)),this,SLOT(amount_basin_setted()));
	connect(systemset_exec,SIGNAL(amount_pipe_reset(int)),this,SLOT(amount_pipe_setted()));
	connect(systemset_exec,SIGNAL(amount_dispener_reset(int)),this,SLOT(amount_dispener_setted()));
	connect(systemset_exec,SIGNAL(amount_tank_reset(int)),this,SLOT(amount_tank_setted()));
	connect(systemset_exec,SIGNAL(method_tank_reset(int)),this,SLOT(method_tank_setted()));
	connect(systemset_exec,SIGNAL(mainwindow_enable()),this,SLOT(mainwindow_enabled()));
	connect(Log,SIGNAL(disp_for_managerid(const QString&)),systemset_exec,SLOT(dispset_for_managerid(const QString&)));
	connect(this,SIGNAL(whoareyou(unsigned char)),systemset_exec,SLOT(whoareyou_userset(unsigned char)));
	//added for radar
	connect(this,SIGNAL(systemset_show()),systemset_exec,SLOT(systemset_showed()));
	connect(systemset_exec,SIGNAL(button_sysshow()),this,SLOT(key_syssetshow()));
	connect(systemset_exec,SIGNAL(mainwindow_repainting()),this,SLOT(area_painted()));
	//connect(systemset_exec,SIGNAL(mainwindow_radar_click()),this,SLOT(on_toolButton_radar_clicked()));
	connect(uart_run,SIGNAL(repaint_set_yuzhi()),systemset_exec,SLOT(repaint_seted_yuzhi()));
	//added for radar<-
	//added for screen set
	connect(systemset_exec,SIGNAL(hide_tablewidget(unsigned char,unsigned char)),this,SLOT(hide_tablewidget(unsigned char,unsigned char)));
	//added for screen set<-
	connect(uart_reoilgas,SIGNAL(reflash_setinfo()),systemset_exec,SLOT(tableview_2_replay()));
	//油气回收
	connect(systemset_exec,SIGNAL(amount_oilgas_dispen_set(int)),this,SLOT(set_amount_oilgas_dispen(int)));
	connect(systemset_exec,SIGNAL(amount_oilgas_gun_set()),this,SLOT(set_amount_oilgas_gun()));
	connect(this,SIGNAL(Version_To_SystemSet(unsigned char,unsigned char)),systemset_exec,SLOT(Version_Recv_FromMainwindow(unsigned char,unsigned char)));
	connect(this,SIGNAL(Setinfo_To_SystemSet(unsigned char,unsigned char,unsigned char,unsigned char,unsigned char,unsigned char,unsigned char,unsigned char,unsigned char,unsigned char)),systemset_exec,SLOT(Setinfo_Recv_FromMainwindow(unsigned char,unsigned char,unsigned char,unsigned char,unsigned char,unsigned char,unsigned char,unsigned char,unsigned char,unsigned char)));
	connect(this,SIGNAL(Reoilgas_Factorset_UartWrong(unsigned char,unsigned int)),systemset_exec,SLOT(Reoilgas_FactorUartError_Setted(unsigned char,unsigned int)));
	connect(this,SIGNAL(Reoilgas_Factor_Setted()),systemset_exec,SLOT(Reoilgas_Factor_Setover()));
	connect(systemset_exec,SIGNAL(Pre_Pipe_Close()),this,SLOT(Env_Pre_Pipe_Close()));
	connect(systemset_exec,SIGNAL(Pre_Tank_Close()),this,SLOT(Env_Pre_Tank_Close()));
	connect(systemset_exec,SIGNAL(Fga_Gas_Close()),this,SLOT(Env_FGA_Gas1_Close()));
	connect(systemset_exec,SIGNAL(Tem_Tank_Close()),this,SLOT(Env_Tem_Tank_Close()));
	//可燃气体
	connect(systemset_exec,SIGNAL(amount_burngas_set(int)),this,SLOT(amount_burngas_setted(int)));
	//安全防护
	connect(systemset_exec,SIGNAL(amount_safe_reset()),this,SLOT(reset_safe()));
	//防撞柱
	connect(systemset_exec,SIGNAL(crash_num_reset()),this,SLOT(crash_column_reset()));
	if((t == 2)||(t == 1)||(t == 3))
	{
		emit whoareyou(t);
	}
	//同步带屏气液比采集器脉冲当量
	connect(uart_reoilgas,SIGNAL(signal_sync_data(uint,uint,float,float,float,float)),systemset_exec,SLOT(sync_factor_data(uint,uint,float,float,float,float)));
	//每周更新时间  如果网络好用
	connect(this,SIGNAL(Time_calibration()),systemset_exec,SLOT(on_pushButton_testnet_clicked()));

//网络上传相关
	//post添加
	connect(systemset_exec,SIGNAL(Send_Configurationdata(QString,QString,QString,QString,QString,QString)),post_message,SLOT(Send_Configurationdata(QString,QString,QString,QString,QString,QString)));
	connect(systemset_exec,SIGNAL(Send_Configurationdata_HuNan(QString,QString,QString,QString,QString,QString)),post_message_hunan,SLOT(Send_Configurationdata_HuNan(QString,QString,QString,QString,QString,QString)));
    connect(systemset_exec,SIGNAL(Send_Configurationdata_dg(QString,QString,QString,QString,QString,QString)),post_message_dg,SLOT(Send_Configurationdata(QString,QString,QString,QString,QString,QString)));
	//post佛山
	connect(systemset_exec,SIGNAL(Send_Setinfo_Foshan(QString,QString,QString,QString,QString,QString,QString,QString)),post_message_foshan,SLOT(send_setinfo(QString,QString,QString,QString,QString,QString,QString,QString)));
	connect(systemset_exec,SIGNAL(SendStationFoShan()),post_message_foshan,SLOT(send_station_message()));
	//isoosi添加
	connect(systemset_exec,SIGNAL(setup_data(QString,QString,QString,QString)),thread_isoosi,SLOT(setup_data(QString,QString,QString,QString)));
	//isoosi添加重庆
	connect(systemset_exec,SIGNAL(setup_data_cq(QString,QString,QString,QString)),thread_isoosi_cq,SLOT(setup_data(QString,QString,QString,QString)));
	//服务器上传
	connect(systemset_exec,SIGNAL(setup_data_myserver(QString,QString,QString,QString)),myserver_thread,SLOT(setup_data(QString,QString,QString,QString)));
	connect(systemset_exec,SIGNAL(myserver_xielousetup(QString,QString,QString,QString,QString)),myserver_thread,SLOT(xielousetup(QString,QString,QString,QString,QString)));

//液位仪
    connect(systemset_exec,SIGNAL(amount_Oil_Tank_draw(int)),this,SLOT(draw_OilTank(int)));
    connect(systemset_exec,SIGNAL(amount_Tangan_draw()),this,SLOT(draw_Tangan()));
    connect(this,SIGNAL(set_Tangan_add_success_signal(unsigned char)),systemset_exec,SLOT(set_Tangan_add_success(unsigned char)));
    connect(this,SIGNAL(Send_compensation_Signal(unsigned char,unsigned char,float)),systemset_exec,SLOT(display_compension(unsigned char,unsigned char,float)));
    connect(this,SIGNAL(compensation_set_result_Signal(unsigned char,unsigned char,QString)),systemset_exec,SLOT(display_compensation_set_result(unsigned char,unsigned char,QString)));

	systemset_exec->show();
}
void MainWindow::on_pushButton_7_clicked()      //历史记录
{
    //system("echo 3 > /proc/sys/vm/drop_caches");
    if(flag_exchange_connect == 1)
    {
        emit closeing_connect();
        ui->pushButton_2->setEnabled(1);
    }
    flag_exchange_history = 1;
    ui->pushButton_7->setEnabled(0);
    ui->label_delay_jilu->setHidden(0);
    qApp->processEvents();
    history_exec_creat();
}

void MainWindow::history_exec_creat()
{
    history_exec = new history;
    connect(history_exec,SIGNAL(pushButton_history_enable()),this,SLOT(pushButton_history_enabled()));
    connect(this,SIGNAL(closeing_history()),history_exec,SLOT(on_pushButton_3_clicked()));
    history_exec->show();
    ui->label_delay_jilu->setHidden(1);
}

void MainWindow::on_pushButton_3_clicked()      //静音
{
	if(!flag_silent)
    {
        beep_none();
        ui->pushButton_3->setStyleSheet("border-image: url(:/picture/silent.png)");
        flag_silent = 1;
        Count_auto_silent = Silent_time_h*3600 + Silent_time_m*60;
        Flag_auto_silent = 1;
        if(!Count_auto_silent)
        {
            Flag_auto_silent = 1;
        }
    }
    else
    {
        ui->pushButton_3->setStyleSheet("border-image: url(:/picture/sound.png)");
        flag_silent = 0;
        Flag_auto_silent = 0;
    }

//	Airtightness_Test *airtest;
//	airtest = new Airtightness_Test;
//	airtest->setAttribute(Qt::WA_DeleteOnClose);
//	airtest->show();
//	One_click_sync *sync = new One_click_sync;
//	sync->setAttribute(Qt::WA_DeleteOnClose);
//	sync->show();
}
void MainWindow::on_pushButton_2_clicked()      //联系我们
{
    //system("echo 3 > /proc/sys/vm/drop_caches");
    if(flag_exchange_history == 1)
    {
        emit closeing_history();
        ui->pushButton_7->setEnabled(1);
    }

    flag_exchange_connect = 1;
    ui->pushButton_2->setEnabled(0);
    connectus_show = new connectus;
    connect(connectus_show,SIGNAL(pushButton_connect_enable()),this,SLOT(pushButton_connect_enabled()));
    connect(this,SIGNAL(closeing_connect()),connectus_show,SLOT(on_pushButton_clicked()));
    connectus_show->show();
}

void MainWindow::warn_oil_set_basin(int t)
{
    switch(t)
    {
        case 1 :
		         ui->label->setStyleSheet("border-image: url(:/picture/oilwro.png);");
                 break;
        case 2 :
		         ui->label_2->setStyleSheet("border-image: url(:/picture/oilwro.png);");
                 break;
        case 3 :
		         ui->label_3->setStyleSheet("border-image: url(:/picture/oilwro.png);");
                 break;
        case 4 :
		         ui->label_4->setStyleSheet("border-image: url(:/picture/oilwro.png);");
                 break;
        case 5 :
		         ui->label_5->setStyleSheet("border-image: url(:/picture/oilwro.png);");
                 break;
        case 6 :
		         ui->label_6->setStyleSheet("border-image: url(:/picture/oilwro.png);");
                 break;
        case 7 :
		         ui->label_7->setStyleSheet("border-image: url(:/picture/oilwro.png);");
                 break;
        case 8 :
		         ui->label_8->setStyleSheet("border-image: url(:/picture/oilwro.png);");
                 break;
    }
}
void MainWindow::warn_water_set_basin(int t)
{
    switch(t)
    {
        case 1 :
		        ui->label->setStyleSheet("border-image: url(:/picture/waterwro.png);");
                break;
        case 2 :
		         ui->label_2->setStyleSheet("border-image: url(:/picture/waterwro.png);");
                 break;
        case 3 :
		         ui->label_3->setStyleSheet("border-image: url(:/picture/waterwro.png);");
                 break;
        case 4 :
		         ui->label_4->setStyleSheet("border-image: url(:/picture/waterwro.png);");
                 break;
        case 5 :
		         ui->label_5->setStyleSheet("border-image: url(:/picture/waterwro.png);");
                 break;
        case 6 :
		         ui->label_6->setStyleSheet("border-image: url(:/picture/waterwro.png);");
                 break;
        case 7 :
		         ui->label_7->setStyleSheet("border-image: url(:/picture/waterwro.png);");
                 break;
        case 8 :
		         ui->label_8->setStyleSheet("border-image: url(:/picture/waterwro.png);");
                 break;
    }

}
void MainWindow::warn_sensor_set_basin(int t)
{
    switch(t)
    {
        case 1 :
		         ui->label->setStyleSheet("border-image: url(:/picture/sensorwro.png);");
                 break;
        case 2 :
		         ui->label_2->setStyleSheet("border-image: url(:/picture/sensorwro.png);");
                 break;
        case 3 :
		         ui->label_3->setStyleSheet("border-image: url(:/picture/sensorwro.png);");
                 break;
        case 4 :
		         ui->label_4->setStyleSheet("border-image: url(:/picture/sensorwro.png);");
                 break;
        case 5 :
		         ui->label_5->setStyleSheet("border-image: url(:/picture/sensorwro.png);");
                 break;
        case 6 :
		         ui->label_6->setStyleSheet("border-image: url(:/picture/sensorwro.png);");
                 break;
        case 7 :
		         ui->label_7->setStyleSheet("border-image: url(:/picture/sensorwro.png);");
                 break;
        case 8 :
		         ui->label_8->setStyleSheet("border-image: url(:/picture/sensorwro.png);");
                 break;
    }
}
void MainWindow::warn_uart_set_basin(int t)
{
    switch(t)
    {
        case 1 :
		        ui->label->setStyleSheet("border-image: url(:/picture/uartwro.png);");
                break;
        case 2 :
		         ui->label_2->setStyleSheet("border-image: url(:/picture/uartwro.png);");
                 break;
        case 3 :
		         ui->label_3->setStyleSheet("border-image: url(:/picture/uartwro.png);");
                 break;
        case 4 :
		         ui->label_4->setStyleSheet("border-image: url(:/picture/uartwro.png);");
                 break;
        case 5 :
		         ui->label_5->setStyleSheet("border-image: url(:/picture/uartwro.png);");
                 break;
        case 6 :
		         ui->label_6->setStyleSheet("border-image: url(:/picture/uartwro.png);");
                 break;
        case 7 :
		         ui->label_7->setStyleSheet("border-image: url(:/picture/uartwro.png);");
                 break;
        case 8 :
		         ui->label_8->setStyleSheet("border-image: url(:/picture/uartwro.png);");
                 break;
    }
}
void MainWindow::right_set_basin(int t)
{
    switch(t)
    {
        case 1:
		        ui->label->setStyleSheet("border-image: url(:/picture/right.png);");
                break;
        case 2:
		        ui->label_2->setStyleSheet("border-image: url(:/picture/right.png);");
                break;
        case 3:
		        ui->label_3->setStyleSheet("border-image: url(:/picture/right.png);");
                break;
        case 4:
		        ui->label_4->setStyleSheet("border-image: url(:/picture/right.png);");
                break;
        case 5:
		        ui->label_5->setStyleSheet("border-image: url(:/picture/right.png);");
                break;
        case 6:
		        ui->label_6->setStyleSheet("border-image: url(:/picture/right.png);");
                break;
        case 7:
		        ui->label_7->setStyleSheet("border-image: url(:/picture/right.png);");
                break;
        case 8:
		        ui->label_8->setStyleSheet("border-image: url(:/picture/right.png);");
                break;
    }
}

void MainWindow::warn_oil_set_pipe(int t)
{
    switch(t)
    {
        case 91 :
		         ui->label_91->setStyleSheet("border-image: url(:/picture/oilwro.png);");
                 break;
        case 92 :
		         ui->label_92->setStyleSheet("border-image: url(:/picture/oilwro.png);");
                 break;
        case 93 :
		         ui->label_93->setStyleSheet("border-image: url(:/picture/oilwro.png);");
                 break;
        case 94 :
		         ui->label_94->setStyleSheet("border-image: url(:/picture/oilwro.png);");
                 break;
        case 95 :
		         ui->label_95->setStyleSheet("border-image: url(:/picture/oilwro.png);");
                 break;
        case 96 :
		         ui->label_96->setStyleSheet("border-image: url(:/picture/oilwro.png);");
                 break;
        case 97 :
		         ui->label_97->setStyleSheet("border-image: url(:/picture/oilwro.png);");
                 break;
        case 98 :
		         ui->label_98->setStyleSheet("border-image: url(:/picture/oilwro.png);");
                 break;
    }
}
void MainWindow::warn_water_set_pipe(int t)
{
    switch(t)
    {
        case 91 :
		        ui->label_91->setStyleSheet("border-image: url(:/picture/waterwro.png);");
                break;
        case 92 :
		         ui->label_92->setStyleSheet("border-image: url(:/picture/waterwro.png);");
                 break;
        case 93 :
		         ui->label_93->setStyleSheet("border-image: url(:/picture/waterwro.png);");
                 break;
        case 94 :
		         ui->label_94->setStyleSheet("border-image: url(:/picture/waterwro.png);");
                 break;
        case 95 :
		         ui->label_95->setStyleSheet("border-image: url(:/picture/waterwro.png);");
                 break;
        case 96 :
		         ui->label_96->setStyleSheet("border-image: url(:/picture/waterwro.png);");
                 break;
        case 97 :
		         ui->label_97->setStyleSheet("border-image: url(:/picture/waterwro.png);");
                 break;
        case 98 :
		         ui->label_98->setStyleSheet("border-image: url(:/picture/waterwro.png);");
                 break;
    }
}
void MainWindow::warn_sensor_set_pipe(int t)
{
    switch(t)
    {
        case 91 :
		        ui->label_91->setStyleSheet("border-image: url(:/picture/sensorwro.png);");
                break;
        case 92 :
		         ui->label_92->setStyleSheet("border-image: url(:/picture/sensorwro.png);");
                 break;
        case 93 :
		         ui->label_93->setStyleSheet("border-image: url(:/picture/sensorwro.png);");
                 break;
        case 94 :
		         ui->label_94->setStyleSheet("border-image: url(:/picture/sensorwro.png);");
                 break;
        case 95 :
		         ui->label_95->setStyleSheet("border-image: url(:/picture/sensorwro.png);");
                 break;
        case 96 :
		         ui->label_96->setStyleSheet("border-image: url(:/picture/sensorwro.png);");
                 break;
        case 97 :
		         ui->label_97->setStyleSheet("border-image: url(:/picture/sensorwro.png);");
                 break;
        case 98 :
		         ui->label_98->setStyleSheet("border-image: url(:/picture/sensorwro.png);");
                 break;
    }
}
void MainWindow::warn_uart_set_pipe(int t)
{
    switch(t)
    {
        case 91 :
		        ui->label_91->setStyleSheet("border-image: url(:/picture/uartwro.png);");
                break;
        case 92 :
		         ui->label_92->setStyleSheet("border-image: url(:/picture/uartwro.png);");
                 break;
        case 93 :
		         ui->label_93->setStyleSheet("border-image: url(:/picture/uartwro.png);");
                 break;
        case 94 :
		         ui->label_94->setStyleSheet("border-image: url(:/picture/uartwro.png);");
                 break;
        case 95 :
		         ui->label_95->setStyleSheet("border-image: url(:/picture/uartwro.png);");
                 break;
        case 96 :
		         ui->label_96->setStyleSheet("border-image: url(:/picture/uartwro.png);");
                 break;
        case 97 :
		         ui->label_97->setStyleSheet("border-image: url(:/picture/uartwro.png);");
                 break;
        case 98 :
		         ui->label_98->setStyleSheet("border-image: url(:/picture/uartwro.png);");
                 break;
    }
}
void MainWindow::right_set_pipe(int t)
{
    switch(t)
    {
        case 91 :
		        ui->label_91->setStyleSheet("border-image: url(:/picture/right.png);");
                break;
        case 92 :
		         ui->label_92->setStyleSheet("border-image: url(:/picture/right.png);");
                 break;
        case 93 :
		         ui->label_93->setStyleSheet("border-image: url(:/picture/right.png);");
                 break;
        case 94 :
		         ui->label_94->setStyleSheet("border-image: url(:/picture/right.png);");
                 break;
        case 95 :
		         ui->label_95->setStyleSheet("border-image: url(:/picture/right.png);");
                 break;
        case 96 :
		         ui->label_96->setStyleSheet("border-image: url(:/picture/right.png);");
                 break;
        case 97 :
		         ui->label_97->setStyleSheet("border-image: url(:/picture/right.png);");
                 break;
        case 98 :
		         ui->label_98->setStyleSheet("border-image: url(:/picture/right.png);");
                 break;
    }
}

void MainWindow::warn_oil_set_dispener(int t)
{
    switch(t)
    {
        case 81 :
		         ui->label_81->setStyleSheet("border-image: url(:/picture/oilwro.png);");
                 break;
        case 82 :
		         ui->label_82->setStyleSheet("border-image: url(:/picture/oilwro.png);");
                 break;
        case 83 :
		         ui->label_83->setStyleSheet("border-image: url(:/picture/oilwro.png);");
                 break;
        case 84 :
		         ui->label_84->setStyleSheet("border-image: url(:/picture/oilwro.png);");
                 break;
        case 85 :
		         ui->label_85->setStyleSheet("border-image: url(:/picture/oilwro.png);");
                 break;
        case 86 :
		         ui->label_86->setStyleSheet("border-image: url(:/picture/oilwro.png);");
                 break;
        case 87 :
		         ui->label_87->setStyleSheet("border-image: url(:/picture/oilwro.png);");
                 break;
        case 88 :
		         ui->label_88->setStyleSheet("border-image: url(:/picture/oilwro.png);");
                 break;
    }
}
void MainWindow::warn_water_set_dispener(int t)
{
    switch(t)
    {
        case 81 :
		         ui->label_81->setStyleSheet("border-image: url(:/picture/waterwro.png);");
                 break;
        case 82 :
		         ui->label_82->setStyleSheet("border-image: url(:/picture/waterwro.png);");
                 break;
        case 83 :
		         ui->label_83->setStyleSheet("border-image: url(:/picture/waterwro.png);");
                 break;
        case 84 :
		         ui->label_84->setStyleSheet("border-image: url(:/picture/waterwro.png);");
                 break;
        case 85 :
		         ui->label_85->setStyleSheet("border-image: url(:/picture/waterwro.png);");
                 break;
        case 86 :
		         ui->label_86->setStyleSheet("border-image: url(:/picture/waterwro.png);");
                 break;
        case 87 :
		         ui->label_87->setStyleSheet("border-image: url(:/picture/waterwro.png);");
                 break;
        case 88 :
		         ui->label_88->setStyleSheet("border-image: url(:/picture/waterwro.png);");
                 break;
    }
}
void MainWindow::warn_sensor_set_dispener(int t)
{
    switch(t)
    {
        case 81 :
		         ui->label_81->setStyleSheet("border-image: url(:/picture/sensorwro.png);");
                 break;
        case 82 :
		         ui->label_82->setStyleSheet("border-image: url(:/picture/sensorwro.png);");
                 break;
        case 83 :
		         ui->label_83->setStyleSheet("border-image: url(:/picture/sensorwro.png);");
                 break;
        case 84 :
		         ui->label_84->setStyleSheet("border-image: url(:/picture/sensorwro.png);");
                 break;
        case 85 :
		         ui->label_85->setStyleSheet("border-image: url(:/picture/sensorwro.png);");
                 break;
        case 86 :
		         ui->label_86->setStyleSheet("border-image: url(:/picture/sensorwro.png);");
                 break;
        case 87 :
		         ui->label_87->setStyleSheet("border-image: url(:/picture/sensorwro.png);");
                 break;
        case 88 :
		         ui->label_88->setStyleSheet("border-image: url(:/picture/sensorwro.png);");
                 break;
    }
}
void MainWindow::warn_uart_set_dispener(int t)
{
    switch(t)
    {
        case 81 :
		         ui->label_81->setStyleSheet("border-image: url(:/picture/uartwro.png);");
                 break;
        case 82 :
		         ui->label_82->setStyleSheet("border-image: url(:/picture/uartwro.png);");
                 break;
        case 83 :
		         ui->label_83->setStyleSheet("border-image: url(:/picture/uartwro.png);");
                 break;
        case 84 :
		         ui->label_84->setStyleSheet("border-image: url(:/picture/uartwro.png);");
                 break;
        case 85 :
		         ui->label_85->setStyleSheet("border-image: url(:/picture/uartwro.png);");
                 break;
        case 86 :
		         ui->label_86->setStyleSheet("border-image: url(:/picture/uartwro.png);");
                 break;
        case 87 :
		         ui->label_87->setStyleSheet("border-image: url(:/picture/uartwro.png);");
                 break;
        case 88 :
		         ui->label_88->setStyleSheet("border-image: url(:/picture/uartwro.png);");
                 break;
    }
}
void MainWindow::right_set_dispener(int t)
{
    switch(t)
    {
        case 81 :
		         ui->label_81->setStyleSheet("border-image: url(:/picture/right.png);");
                 break;
        case 82 :
		         ui->label_82->setStyleSheet("border-image: url(:/picture/right.png);");
                 break;
        case 83 :
		         ui->label_83->setStyleSheet("border-image: url(:/picture/right.png);");
                 break;
        case 84 :
		         ui->label_84->setStyleSheet("border-image: url(:/picture/right.png);");
                 break;
        case 85 :
		         ui->label_85->setStyleSheet("border-image: url(:/picture/right.png);");
                 break;
        case 86 :
		         ui->label_86->setStyleSheet("border-image: url(:/picture/right.png);");
                 break;
        case 87 :
		         ui->label_87->setStyleSheet("border-image: url(:/picture/right.png);");
                 break;
        case 88 :
		         ui->label_88->setStyleSheet("border-image: url(:/picture/right.png);");
                 break;
    }
}

void MainWindow::warn_oil_set_tank(int t)
{
    switch(t)
    {
        case 71 :
		         ui->label_71->setStyleSheet("border-image: url(:/picture/oilwro.png);");
                 break;
        case 72 :
		         ui->label_72->setStyleSheet("border-image: url(:/picture/oilwro.png);");
                 break;
        case 73 :
		         ui->label_73->setStyleSheet("border-image: url(:/picture/oilwro.png);");
                 break;
        case 74 :
		         ui->label_74->setStyleSheet("border-image: url(:/picture/oilwro.png);");
                 break;
        case 75 :
		         ui->label_75->setStyleSheet("border-image: url(:/picture/oilwro.png);");
                 break;
        case 76 :
		         ui->label_76->setStyleSheet("border-image: url(:/picture/oilwro.png);");
                 break;
        case 77 :
		         ui->label_77->setStyleSheet("border-image: url(:/picture/oilwro.png);");
                 break;
        case 78 :
		         ui->label_78->setStyleSheet("border-image: url(:/picture/oilwro.png);");
                 break;
    }
}
void MainWindow::warn_water_set_tank(int t)
{
    switch(t)
    {
        case 71 :
		         ui->label_71->setStyleSheet("border-image: url(:/picture/waterwro.png);");
                 break;
        case 72 :
		         ui->label_72->setStyleSheet("border-image: url(:/picture/waterwro.png);");
                 break;
        case 73 :
		         ui->label_73->setStyleSheet("border-image: url(:/picture/waterwro.png);");
                 break;
        case 74 :
		         ui->label_74->setStyleSheet("border-image: url(:/picture/waterwro.png);");
                 break;
        case 75 :
		         ui->label_75->setStyleSheet("border-image: url(:/picture/waterwro.png);");
                 break;
        case 76 :
		         ui->label_76->setStyleSheet("border-image: url(:/picture/waterwro.png);");
                 break;
        case 77 :
		         ui->label_77->setStyleSheet("border-image: url(:/picture/waterwro.png);");
                 break;
        case 78 :
		         ui->label_78->setStyleSheet("border-image: url(:/picture/waterwro.png);");
                 break;
    }
}
void MainWindow::warn_high_set_tank(int t)
{
    switch(t)
    {
        case 71 :
		         ui->label_71->setStyleSheet("border-image: url(:/picture/highwro.png);");
                 break;
        case 72 :
		         ui->label_72->setStyleSheet("border-image: url(:/picture/highwro.png);");
                 break;
        case 73 :
		         ui->label_73->setStyleSheet("border-image: url(:/picture/highwro.png);");
                 break;
        case 74 :
		         ui->label_74->setStyleSheet("border-image: url(:/picture/highwro.png);");
                 break;
        case 75 :
		         ui->label_75->setStyleSheet("border-image: url(:/picture/highwro.png);");
                 break;
        case 76 :
		         ui->label_76->setStyleSheet("border-image: url(:/picture/highwro.png);");
                 break;
        case 77 :
		         ui->label_77->setStyleSheet("border-image: url(:/picture/highwro.png);");
                 break;
        case 78 :
		         ui->label_78->setStyleSheet("border-image: url(:/picture/highwro.png);");
                 break;
    }
}
void MainWindow::warn_low_set_tank(int t)
{
    switch(t)
    {
        case 71 :
		         ui->label_71->setStyleSheet("border-image: url(:/picture/lowwro.png);");
                 break;
        case 72 :
		         ui->label_72->setStyleSheet("border-image: url(:/picture/lowwro.png);");
                 break;
        case 73 :
		         ui->label_73->setStyleSheet("border-image: url(:/picture/lowwro.png);");
                 break;
        case 74 :
		         ui->label_74->setStyleSheet("border-image: url(:/picture/lowwro.png);");
                 break;
        case 75 :
		         ui->label_75->setStyleSheet("border-image: url(:/picture/lowwro.png);");
                 break;
        case 76 :
		         ui->label_76->setStyleSheet("border-image: url(:/picture/lowwro.png);");
                 break;
        case 77 :
		         ui->label_77->setStyleSheet("border-image: url(:/picture/lowwro.png);");
                 break;
        case 78 :
		         ui->label_78->setStyleSheet("border-image: url(:/picture/lowwro.png);");
                 break;
    }
}
void MainWindow::warn_pre_set_tank(int t)
{
    switch(t)
    {
        case 71 :
		         ui->label_71->setStyleSheet("border-image: url(:/picture/pressurepre.png);");
                 break;
        case 72 :
		         ui->label_72->setStyleSheet("border-image: url(:/picture/pressurepre.png);");
                 break;
        case 73 :
		         ui->label_73->setStyleSheet("border-image: url(:/picture/pressurepre.png);");
                 break;
        case 74 :
		         ui->label_74->setStyleSheet("border-image: url(:/picture/pressurepre.png);");
                 break;
        case 75 :
		         ui->label_75->setStyleSheet("border-image: url(:/picture/pressurepre.png);");
                 break;
        case 76 :
		         ui->label_76->setStyleSheet("border-image: url(:/picture/pressurepre.png);");
                 break;
        case 77 :
		         ui->label_77->setStyleSheet("border-image: url(:/picture/pressurepre.png);");
                 break;
        case 78 :
		         ui->label_78->setStyleSheet("border-image: url(:/picture/pressurepre.png);");
                 break;
    }
}
void MainWindow::warn_warn_set_tank(int t)
{
    switch(t)
    {
        case 71 :
		         ui->label_71->setStyleSheet("border-image: url(:/picture/pressurewarn.png);");
                 break;
        case 72 :
		         ui->label_72->setStyleSheet("border-image: url(:/picture/pressurewarn.png);");
                 break;
        case 73 :
		         ui->label_73->setStyleSheet("border-image: url(:/picture/pressurewarn.png);");
                 break;
        case 74 :
		         ui->label_74->setStyleSheet("border-image: url(:/picture/pressurewarn.png);");
                 break;
        case 75 :
		         ui->label_75->setStyleSheet("border-image: url(:/picture/pressurewarn.png);");
                 break;
        case 76 :
		         ui->label_76->setStyleSheet("border-image: url(:/picture/pressurewarn.png);");
                 break;
        case 77 :
		         ui->label_77->setStyleSheet("border-image: url(:/picture/pressurewarn.png);");
                 break;
        case 78 :
		         ui->label_78->setStyleSheet("border-image: url(:/picture/pressurewarn.png);");
                 break;
    }
}
void MainWindow::warn_sensor_set_tank(int t)
{
    switch(t)
    {

        case 71 :
                 if(Test_Method == 1)
                 {
					 ui->label_71->setStyleSheet("border-image: url(:/picture/sensorwro.png);");
                 }
                 else
                 {
					 ui->label_71->setStyleSheet("border-image: url(:/picture/sensorwro.png);");
				 }
                 break;
        case 72 :
                if(Test_Method == 1)
                {
					ui->label_72->setStyleSheet("border-image: url(:/picture/sensorwro.png);");
                }
                else
                {
					ui->label_72->setStyleSheet("border-image: url(:/picture/sensorwro.png);");
                }
                break;
        case 73 :
                if(Test_Method == 1)
                {
					ui->label_73->setStyleSheet("border-image: url(:/picture/sensorwro.png);");
                }
                else
                {
					ui->label_73->setStyleSheet("border-image: url(:/picture/sensorwro.png);");
                }
                break;
        case 74 :
                if(Test_Method == 1)
                {
					ui->label_74->setStyleSheet("border-image: url(:/picture/sensorwro.png);");
                }
                else
                {
					ui->label_74->setStyleSheet("border-image: url(:/picture/sensorwro.png);");
                }
                break;
        case 75 :
                if(Test_Method == 1)
                {
					ui->label_75->setStyleSheet("border-image: url(:/picture/sensorwro.png);");
                }
                else
                {
					ui->label_75->setStyleSheet("border-image: url(:/picture/sensorwro.png);");
                }
                break;
        case 76 :
                if(Test_Method == 1)
                {
					ui->label_76->setStyleSheet("border-image: url(:/picture/sensorwro.png);");
                }
                else
                {
					ui->label_76->setStyleSheet("border-image: url(:/picture/sensorwro.png);");
                }
                break;
        case 77 :
                if(Test_Method == 1)
                {
					ui->label_77->setStyleSheet("border-image: url(:/picture/sensorwro.png);");
                }
        else
                {
					ui->label_77->setStyleSheet("border-image: url(:/picture/sensorwro.png);");
                }
                break;
        case 78 :
                if(Test_Method == 1)
                {
					ui->label_78->setStyleSheet("border-image: url(:/picture/sensorwro.png);");
                }
                else
                {
					ui->label_78->setStyleSheet("border-image: url(:/picture/sensorwro.png);");
                }
                break;
    }
}
void MainWindow::warn_uart_set_tank(int t)
{

    switch(t)
    {
        case 71 :
                if(Test_Method == 1)
                {
					ui->label_71->setStyleSheet("border-image: url(:/picture/uartwro.png);");
                }
                else
                {
					ui->label_71->setStyleSheet("border-image: url(:/picture/uartwro.png);");
                }
                break;
        case 72 :
                if(Test_Method == 1)
                {
					ui->label_72->setStyleSheet("border-image: url(:/picture/uartwro.png);");
                }
                else
                {
					ui->label_72->setStyleSheet("border-image: url(:/picture/uartwro.png);");
                }
                break;
        case 73 :
                if(Test_Method == 1)
                {
					ui->label_73->setStyleSheet("border-image: url(:/picture/uartwro.png);");
                }
                else
                {
					ui->label_73->setStyleSheet("border-image: url(:/picture/uartwro.png);");
               }
                break;
        case 74 :
                if(Test_Method == 1)
                {
					ui->label_74->setStyleSheet("border-image: url(:/picture/uartwro.png);");
                }
                else
                {
					ui->label_74->setStyleSheet("border-image: url(:/picture/uartwro.png);");
                }
                break;
         case 75 :
                if(Test_Method == 1)
                {
					ui->label_75->setStyleSheet("border-image: url(:/picture/uartwro.png);");
                }
                else
                {
					ui->label_75->setStyleSheet("border-image: url(:/picture/uartwro.png);");
                }
                break;
        case 76 :
                if(Test_Method == 1)
                {
					ui->label_76->setStyleSheet("border-image: url(:/picture/uartwro.png);");
                }
                else
                {
					ui->label_76->setStyleSheet("border-image: url(:/picture/uartwro.png);");
                }
                break;
                case 77 :
        if(Test_Method == 1)
                {
			        ui->label_77->setStyleSheet("border-image: url(:/picture/uartwro.png);");
                }
                else
                {
			        ui->label_77->setStyleSheet("border-image: url(:/picture/uartwro.png);");
                }
                break;
        case 78 :
                if(Test_Method == 1)
                {
					ui->label_78->setStyleSheet("border-image: url(:/picture/uartwro.png);");
                }
                else
                {
					ui->label_78->setStyleSheet("border-image: url(:/picture/uartwro.png);");
                }
                break;
    }
}
void MainWindow::right_set_tank(int t)
{
    switch(t)
    {

        case 71 :
                if(Test_Method == 1)
                {
					 ui->label_71->setStyleSheet("border-image: url(:/picture/right.png);");
                }
                else
                {
					ui->label_71->setStyleSheet("border-image: url(:/picture/right.png);");
                }
                 break;
        case 72 :
                if(Test_Method == 1)
                {
					 ui->label_72->setStyleSheet("border-image: url(:/picture/right.png);");
                }
                else
                {
					ui->label_72->setStyleSheet("border-image: url(:/picture/right.png);");
                }
                 break;
        case 73 :
                if(Test_Method == 1)
                {
					 ui->label_73->setStyleSheet("border-image: url(:/picture/right.png);");
                }
                else
                {
					ui->label_73->setStyleSheet("border-image: url(:/picture/right.png);");
                }
                 break;
        case 74 :
                if(Test_Method == 1)
                {
					 ui->label_74->setStyleSheet("border-image: url(:/picture/right.png);");
                }
                else
                {
					ui->label_74->setStyleSheet("border-image: url(:/picture/right.png);");
                }
                 break;
        case 75 :
                if(Test_Method == 1)
                {
					 ui->label_75->setStyleSheet("border-image: url(:/picture/right.png);");
                }
                else
                {
					ui->label_75->setStyleSheet("border-image: url(:/picture/right.png);");
                }
                 break;
        case 76 :
                if(Test_Method == 1)
                {
					 ui->label_76->setStyleSheet("border-image: url(:/picture/right.png);");
                }
                else
                {
					ui->label_76->setStyleSheet("border-image: url(:/picture/right.png);");
                }
                 break;
        case 77 :
                if(Test_Method == 1)
                {
					 ui->label_77->setStyleSheet("border-image: url(:/picture/right.png);");
                }
                else
                {
					ui->label_77->setStyleSheet("border-image: url(:/picture/right.png);");
                }
                 break;
        case 78 :
                if(Test_Method == 1)
                {
					 ui->label_78->setStyleSheet("border-image: url(:/picture/right.png);");
                }
                else
                {
					ui->label_78->setStyleSheet("border-image: url(:/picture/right.png);");
                }
                 break;
    }
}

void MainWindow::amount_basin_setted()       //basin
{

    if(count_basin == 0)                                                                //if(ui->comboBox->currentText()=="1")初始化可用
    {
        config_SensorAmountChanged();
		ui->label->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_2->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_3->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_4->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_5->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_6->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_7->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_8->setStyleSheet("border-image: url(:/picture/nosensor.png);");
    }
    if(count_basin == 1)
    {
        config_SensorAmountChanged();
		ui->label->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_2->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_3->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_4->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_5->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_6->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_7->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_8->setStyleSheet("border-image: url(:/picture/nosensor.png);");
    }
    if(count_basin == 2)
    {
        config_SensorAmountChanged();
		ui->label->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_2->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_3->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_4->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_5->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_6->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_7->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_8->setStyleSheet("border-image: url(:/picture/nosensor.png);");
    }
    if(count_basin == 3)
    {
        config_SensorAmountChanged();
		ui->label->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_2->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_3->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_4->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_5->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_6->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_7->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_8->setStyleSheet("border-image: url(:/picture/nosensor.png);");
    }
    if(count_basin == 4)
    {
        config_SensorAmountChanged();
		ui->label->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_2->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_3->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_4->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_5->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_6->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_7->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_8->setStyleSheet("border-image: url(:/picture/nosensor.png);");
    }
    if(count_basin == 5)
    {
        config_SensorAmountChanged();
		ui->label->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_2->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_3->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_4->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_5->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_6->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_7->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_8->setStyleSheet("border-image: url(:/picture/nosensor.png);");
    }
    if(count_basin == 6)
    {
        config_SensorAmountChanged();
		ui->label->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_2->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_3->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_4->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_5->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_6->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_7->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_8->setStyleSheet("border-image: url(:/picture/nosensor.png);");
    }
    if(count_basin == 7)
    {
        config_SensorAmountChanged();
		ui->label->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_2->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_3->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_4->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_5->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_6->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_7->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_8->setStyleSheet("border-image: url(:/picture/nosensor.png);");
    }
    if(count_basin == 8)
    {
        config_SensorAmountChanged();
		ui->label->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_2->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_3->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_4->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_5->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_6->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_7->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_8->setStyleSheet("border-image: url(:/picture/right.png);");
    }

}

void MainWindow::amount_pipe_setted()        //pipe
{

    if(count_pipe == 0)
    {
        config_SensorAmountChanged();
		ui->label_91->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_92->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_93->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_94->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_95->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_96->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_97->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_98->setStyleSheet("border-image: url(:/picture/nosensor.png);");
    }
    if(count_pipe == 1)
    {
        config_SensorAmountChanged();
		ui->label_91->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_92->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_93->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_94->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_95->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_96->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_97->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_98->setStyleSheet("border-image: url(:/picture/nosensor.png);");
    }
    if(count_pipe == 2)
    {
        config_SensorAmountChanged();
		ui->label_91->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_92->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_93->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_94->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_95->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_96->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_97->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_98->setStyleSheet("border-image: url(:/picture/nosensor.png);");
    }
    if(count_pipe == 3)
    {
        config_SensorAmountChanged();
		ui->label_91->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_92->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_93->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_94->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_95->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_96->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_97->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_98->setStyleSheet("border-image: url(:/picture/nosensor.png);");
    }
    if(count_pipe == 4)
    {
        config_SensorAmountChanged();
		ui->label_91->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_92->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_93->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_94->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_95->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_96->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_97->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_98->setStyleSheet("border-image: url(:/picture/nosensor.png);");
    }
    if(count_pipe == 5)
    {
        config_SensorAmountChanged();
		ui->label_91->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_92->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_93->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_94->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_95->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_96->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_97->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_98->setStyleSheet("border-image: url(:/picture/nosensor.png);");
    }
    if(count_pipe == 6)
    {
        config_SensorAmountChanged();
		ui->label_91->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_92->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_93->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_94->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_95->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_96->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_97->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_98->setStyleSheet("border-image: url(:/picture/nosensor.png);");
    }
    if(count_pipe == 7)
    {
        config_SensorAmountChanged();
		ui->label_91->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_92->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_93->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_94->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_95->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_96->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_97->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_98->setStyleSheet("border-image: url(:/picture/nosensor.png);");
    }
    if(count_pipe == 8)
    {
        config_SensorAmountChanged();
		ui->label_91->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_92->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_93->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_94->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_95->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_96->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_97->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_98->setStyleSheet("border-image: url(:/picture/right.png);");
    }
}

void MainWindow::amount_dispener_setted()        //dispener
{

    if(count_dispener == 0)
    {
        config_SensorAmountChanged();
		ui->label_81->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_82->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_83->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_84->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_85->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_86->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_87->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_88->setStyleSheet("border-image: url(:/picture/nosensor.png);");
    }
    if(count_dispener == 1)
    {
        config_SensorAmountChanged();
		ui->label_81->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_82->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_83->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_84->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_85->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_86->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_87->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_88->setStyleSheet("border-image: url(:/picture/nosensor.png);");
    }
    if(count_dispener == 2)
    {
        config_SensorAmountChanged();
		ui->label_81->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_82->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_83->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_84->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_85->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_86->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_87->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_88->setStyleSheet("border-image: url(:/picture/nosensor.png);");
    }
    if(count_dispener == 3)
    {
        config_SensorAmountChanged();
		ui->label_81->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_82->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_83->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_84->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_85->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_86->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_87->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_88->setStyleSheet("border-image: url(:/picture/nosensor.png);");
    }
    if(count_dispener == 4)
    {
        config_SensorAmountChanged();
		ui->label_81->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_82->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_83->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_84->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_85->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_86->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_87->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_88->setStyleSheet("border-image: url(:/picture/nosensor.png);");
    }
    if(count_dispener == 5)
    {
        config_SensorAmountChanged();
		ui->label_81->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_82->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_83->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_84->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_85->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_86->setStyleSheet("border-image: url(:/picture/nosensor.png);");;
		ui->label_87->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_88->setStyleSheet("border-image: url(:/picture/nosensor.png);");
    }
    if(count_dispener == 6)
    {
        config_SensorAmountChanged();
		ui->label_81->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_82->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_83->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_84->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_85->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_86->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_87->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_88->setStyleSheet("border-image: url(:/picture/nosensor.png);");
    }
    if(count_dispener == 7)
    {
        config_SensorAmountChanged();
		ui->label_81->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_82->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_83->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_84->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_85->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_86->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_87->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_88->setStyleSheet("border-image: url(:/picture/nosensor.png);");
    }
    if(count_dispener == 8)
    {
        config_SensorAmountChanged();
		ui->label_81->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_82->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_83->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_84->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_85->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_86->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_87->setStyleSheet("border-image: url(:/picture/right.png);");
		ui->label_88->setStyleSheet("border-image: url(:/picture/right.png);");
    }
}

void MainWindow::amount_tank_setted()
{

    if(count_tank == 0)
    {
        config_SensorAmountChanged();
		ui->label_71->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_72->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_73->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_74->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_75->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_76->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_77->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_78->setStyleSheet("border-image: url(:/picture/nosensor.png);");

        ui->label_p1->hide();
        ui->label_p2->hide();
        ui->label_p3->hide();
        ui->label_p4->hide();
        ui->label_p5->hide();
        ui->label_p6->hide();
        ui->label_p7->hide();
        ui->label_p8->hide();
    }
    if(count_tank == 1)
    {
        config_SensorAmountChanged();

        if(Test_Method == 1)
        {
			ui->label_71->setStyleSheet("border-image: url(:/picture/right.png);");

            ui->label_p1->show();
            ui->label_p2->hide();
            ui->label_p3->hide();
            ui->label_p4->hide();
            ui->label_p5->hide();
            ui->label_p6->hide();
            ui->label_p7->hide();
            ui->label_p8->hide();

        }
        else
        {
			ui->label_71->setStyleSheet("border-image: url(:/picture/right.png);");

            ui->label_p1->hide();
            ui->label_p2->hide();
            ui->label_p3->hide();
            ui->label_p4->hide();
            ui->label_p5->hide();
            ui->label_p6->hide();
            ui->label_p7->hide();
            ui->label_p8->hide();

        }

		ui->label_72->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_73->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_74->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_75->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_76->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_77->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_78->setStyleSheet("border-image: url(:/picture/nosensor.png);");


    }
    if(count_tank == 2)
    {
        config_SensorAmountChanged();

        if(Test_Method == 1)
        {
			ui->label_71->setStyleSheet("border-image: url(:/picture/right.png);");
			ui->label_72->setStyleSheet("border-image: url(:/picture/right.png);");

            ui->label_p1->show();
            ui->label_p2->show();
            ui->label_p3->hide();
            ui->label_p4->hide();
            ui->label_p5->hide();
            ui->label_p6->hide();
            ui->label_p7->hide();
            ui->label_p8->hide();

        }
       else
        {
			ui->label_71->setStyleSheet("border-image: url(:/picture/right.png);");
			ui->label_72->setStyleSheet("border-image: url(:/picture/right.png);");

            ui->label_p1->hide();
            ui->label_p2->hide();
            ui->label_p3->hide();
            ui->label_p4->hide();
            ui->label_p5->hide();
            ui->label_p6->hide();
            ui->label_p7->hide();
            ui->label_p8->hide();

        }

		ui->label_73->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_74->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_75->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_76->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_77->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_78->setStyleSheet("border-image: url(:/picture/nosensor.png);");


    }
    if(count_tank == 3)
    {
        config_SensorAmountChanged();

        if(Test_Method == 1)
        {
			ui->label_71->setStyleSheet("border-image: url(:/picture/right.png);");
			ui->label_72->setStyleSheet("border-image: url(:/picture/right.png);");
			ui->label_73->setStyleSheet("border-image: url(:/picture/right.png);");

            ui->label_p1->show();
            ui->label_p2->show();
            ui->label_p3->show();
            ui->label_p4->hide();
            ui->label_p5->hide();
            ui->label_p6->hide();
            ui->label_p7->hide();
            ui->label_p8->hide();

        }
       else
        {
			ui->label_71->setStyleSheet("border-image: url(:/picture/right.png);");
			ui->label_72->setStyleSheet("border-image: url(:/picture/right.png);");
			ui->label_73->setStyleSheet("border-image: url(:/picture/right.png);");

            ui->label_p1->hide();
            ui->label_p2->hide();
            ui->label_p3->hide();
            ui->label_p4->hide();
            ui->label_p5->hide();
            ui->label_p6->hide();
            ui->label_p7->hide();
            ui->label_p8->hide();

        }

		ui->label_74->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_75->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_76->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_77->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_78->setStyleSheet("border-image: url(:/picture/nosensor.png);");

    }
    if(count_tank == 4)
    {
        config_SensorAmountChanged();

        if(Test_Method == 1)
        {
			ui->label_71->setStyleSheet("border-image: url(:/picture/right.png);");
			ui->label_72->setStyleSheet("border-image: url(:/picture/right.png);");
			ui->label_73->setStyleSheet("border-image: url(:/picture/right.png);");
			ui->label_74->setStyleSheet("border-image: url(:/picture/right.png);");

            ui->label_p1->show();
            ui->label_p2->show();
            ui->label_p3->show();
            ui->label_p4->show();
            ui->label_p5->hide();
            ui->label_p6->hide();
            ui->label_p7->hide();
            ui->label_p8->hide();

        }
       else
        {
			ui->label_71->setStyleSheet("border-image: url(:/picture/right.png);");
			ui->label_72->setStyleSheet("border-image: url(:/picture/right.png);");
			ui->label_73->setStyleSheet("border-image: url(:/picture/right.png);");
			ui->label_74->setStyleSheet("border-image: url(:/picture/right.png);");

            ui->label_p1->hide();
            ui->label_p2->hide();
            ui->label_p3->hide();
            ui->label_p4->hide();
            ui->label_p5->hide();
            ui->label_p6->hide();
            ui->label_p7->hide();
            ui->label_p8->hide();

        }

		ui->label_75->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_76->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_77->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_78->setStyleSheet("border-image: url(:/picture/nosensor.png);");

    }
    if(count_tank == 5)
    {
        config_SensorAmountChanged();

        if(Test_Method == 1)
        {
			ui->label_71->setStyleSheet("border-image: url(:/picture/right.png);");
			ui->label_72->setStyleSheet("border-image: url(:/picture/right.png);");
			ui->label_73->setStyleSheet("border-image: url(:/picture/right.png);");
			ui->label_74->setStyleSheet("border-image: url(:/picture/right.png);");
			ui->label_75->setStyleSheet("border-image: url(:/picture/right.png);");
            ui->label_p1->show();
            ui->label_p2->show();
            ui->label_p3->show();
            ui->label_p4->show();
            ui->label_p5->show();
            ui->label_p6->hide();
            ui->label_p7->hide();
            ui->label_p8->hide();

        }
       else
        {
			ui->label_71->setStyleSheet("border-image: url(:/picture/right.png);");
			ui->label_72->setStyleSheet("border-image: url(:/picture/right.png);");
			ui->label_73->setStyleSheet("border-image: url(:/picture/right.png);");
			ui->label_74->setStyleSheet("border-image: url(:/picture/right.png);");
			ui->label_75->setStyleSheet("border-image: url(:/picture/right.png);");
            ui->label_p1->hide();
            ui->label_p2->hide();
            ui->label_p3->hide();
            ui->label_p4->hide();
            ui->label_p5->hide();
            ui->label_p6->hide();
            ui->label_p7->hide();
            ui->label_p8->hide();

        }

		ui->label_76->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_77->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_78->setStyleSheet("border-image: url(:/picture/nosensor.png);");


    }
    if(count_tank == 6)
    {
        config_SensorAmountChanged();

        if(Test_Method == 1)
        {
			ui->label_71->setStyleSheet("border-image: url(:/picture/right.png);");
			ui->label_72->setStyleSheet("border-image: url(:/picture/right.png);");
			ui->label_73->setStyleSheet("border-image: url(:/picture/right.png);");
			ui->label_74->setStyleSheet("border-image: url(:/picture/right.png);");
			ui->label_75->setStyleSheet("border-image: url(:/picture/right.png);");
			ui->label_76->setStyleSheet("border-image: url(:/picture/right.png);");
            ui->label_p1->show();
            ui->label_p2->show();
            ui->label_p3->show();
            ui->label_p4->show();
            ui->label_p5->show();
            ui->label_p6->show();
            ui->label_p7->hide();
            ui->label_p8->hide();

        }
       else
        {
			ui->label_71->setStyleSheet("border-image: url(:/picture/right.png);");
			ui->label_72->setStyleSheet("border-image: url(:/picture/right.png);");
			ui->label_73->setStyleSheet("border-image: url(:/picture/right.png);");
			ui->label_74->setStyleSheet("border-image: url(:/picture/right.png);");
			ui->label_75->setStyleSheet("border-image: url(:/picture/right.png);");
			ui->label_76->setStyleSheet("border-image: url(:/picture/right.png);");
            ui->label_p1->hide();
            ui->label_p2->hide();
            ui->label_p3->hide();
            ui->label_p4->hide();
            ui->label_p5->hide();
            ui->label_p6->hide();
            ui->label_p7->hide();
            ui->label_p8->hide();

        }


		ui->label_77->setStyleSheet("border-image: url(:/picture/nosensor.png);");
		ui->label_78->setStyleSheet("border-image: url(:/picture/nosensor.png);");


    }
    if(count_tank == 7)
    {
        config_SensorAmountChanged();

        if(Test_Method == 1)
        {
			ui->label_71->setStyleSheet("border-image: url(:/picture/right.png);");
			ui->label_72->setStyleSheet("border-image: url(:/picture/right.png);");
			ui->label_73->setStyleSheet("border-image: url(:/picture/right.png);");
			ui->label_74->setStyleSheet("border-image: url(:/picture/right.png);");
			ui->label_75->setStyleSheet("border-image: url(:/picture/right.png);");
			ui->label_76->setStyleSheet("border-image: url(:/picture/right.png);");
			ui->label_77->setStyleSheet("border-image: url(:/picture/right.png);");
            ui->label_p1->show();
            ui->label_p2->show();
            ui->label_p3->show();
            ui->label_p4->show();
            ui->label_p5->show();
            ui->label_p6->show();
            ui->label_p7->show();
            ui->label_p8->hide();
        }
       else
        {
			ui->label_71->setStyleSheet("border-image: url(:/picture/right.png);");
			ui->label_72->setStyleSheet("border-image: url(:/picture/right.png);");
			ui->label_73->setStyleSheet("border-image: url(:/picture/right.png);");
			ui->label_74->setStyleSheet("border-image: url(:/picture/right.png);");
			ui->label_75->setStyleSheet("border-image: url(:/picture/right.png);");
			ui->label_76->setStyleSheet("border-image: url(:/picture/right.png);");
			ui->label_77->setStyleSheet("border-image: url(:/picture/right.png);");
            ui->label_p1->hide();
            ui->label_p2->hide();
            ui->label_p3->hide();
            ui->label_p4->hide();
            ui->label_p5->hide();
            ui->label_p6->hide();
            ui->label_p7->hide();
            ui->label_p8->hide();

        }

		ui->label_78->setStyleSheet("border-image: url(:/picture/nosensor.png);");

    }

    if(count_tank == 8)
    {
        config_SensorAmountChanged();

        if(Test_Method == 1)
        {
			ui->label_71->setStyleSheet("border-image: url(:/picture/right.png);");
			ui->label_72->setStyleSheet("border-image: url(:/picture/right.png);");
			ui->label_73->setStyleSheet("border-image: url(:/picture/right.png);");
			ui->label_74->setStyleSheet("border-image: url(:/picture/right.png);");
			ui->label_75->setStyleSheet("border-image: url(:/picture/right.png);");
			ui->label_76->setStyleSheet("border-image: url(:/picture/right.png);");
			ui->label_77->setStyleSheet("border-image: url(:/picture/right.png);");
			ui->label_78->setStyleSheet("border-image: url(:/picture/right.png);");
            ui->label_p1->show();
            ui->label_p2->show();
            ui->label_p3->show();
            ui->label_p4->show();
            ui->label_p5->show();
            ui->label_p6->show();
            ui->label_p7->show();
            ui->label_p8->show();

        }
       else
        {
			ui->label_71->setStyleSheet("border-image: url(:/picture/right.png);");
			ui->label_72->setStyleSheet("border-image: url(:/picture/right.png);");
			ui->label_73->setStyleSheet("border-image: url(:/picture/right.png);");
			ui->label_74->setStyleSheet("border-image: url(:/picture/right.png);");
			ui->label_75->setStyleSheet("border-image: url(:/picture/right.png);");
			ui->label_76->setStyleSheet("border-image: url(:/picture/right.png);");
			ui->label_77->setStyleSheet("border-image: url(:/picture/right.png);");
			ui->label_78->setStyleSheet("border-image: url(:/picture/right.png);");
            ui->label_p1->hide();
            ui->label_p2->hide();
            ui->label_p3->hide();
            ui->label_p4->hide();
            ui->label_p5->hide();
            ui->label_p6->hide();
            ui->label_p7->hide();
            ui->label_p8->hide();

        }
    }
}
void MainWindow::method_tank_setted()
{
    config_SensorAmountChanged();
}

void MainWindow::Timerout_lcd()
{
    QDateTime date_time = QDateTime::currentDateTime();
    ui->systemtime->display(date_time.toString("hh:mm:ss"));
    ui->systemtime_2->display(date_time.toString("yyyy-MM-dd"));
	emit time_out_slotfunction_s(date_time);
	//qDebug()<<date_time.toString("hh:mm:ss");
}

void MainWindow::time_out_slotfunction(QDateTime date_time)
{
	Flag_Touch_Calibration++;
	if(Flag_Touch_Calibration >= 10)
	{
		if(Flag_Touch_Calibration < 25)
		{
			system("rm -r /opt/TouchCalibration");
			system("sync");
			ui->label_touch_calibration->setHidden(1);
			Flag_Touch_Calibration = 30;
		}
		Flag_Touch_Calibration = 30;
	}
	ui->label_debug_send->setText(QString::number(debug_send));
	ui->label_debug_read->setText(QString::number(debug_read));
    //QDateTime date_time = QDateTime::currentDateTime();
	//emit Time_Fga_1s();
//********雷达时间类*********/
    //雷达监控时间判断
    unsigned int time_day = date_time.toString("dd").toInt();
    unsigned int time_h = date_time.toString("hh").toInt();
    unsigned int time_m = date_time.toString("mm").toInt();
    unsigned int time_s = date_time.toString("ss").toInt();
    int a = (Stop_time_h*60 + Stop_time_m) - (Start_time_h*60 + Start_time_m);
    if(a == 0)
    {
        //雷达常开
        if(Flag_area_ctrl[0])
        {
            gif_radar->start();
            ui->label_radaronoff->setStyleSheet("border-image: url(:/picture/radar_zhengchang.png)");
        }
        else
        {
            gif_radar->stop();
            ui->label_radaronoff->setStyleSheet("border-image: url(:/picture/radar_stop.png)");
            ui->label_radarstate->setStyleSheet("border-image: url(:/picture/touming.png)");
        }
    }
    else if(a > 0)   //需要判断是否开启雷达
    {
        if((time_h*60 + time_m) >= (Start_time_h*60 + Start_time_m) && ((time_h*60 + time_m) < (Stop_time_h*60 + Stop_time_m)))
        {
            Flag_area_ctrl[0] = 1;   //改为雷达开启
            gif_radar->start();
            ui->label_radaronoff->setStyleSheet("border-image: url(:/picture/radar_zhengchang.png)");
        }
        else
        {
            Flag_area_ctrl[0] = 0;    //改为雷达关闭
            gif_radar->stop();
            ui->label_radaronoff->setStyleSheet("border-image: url(:/picture/radar_stop.png)");
            ui->label_radarstate->setStyleSheet("border-image: url(:/picture/touming.png)");
        }
    }
    else
    {
        if((time_h*60 + time_m) >= (Stop_time_h*60 + Stop_time_m) && ((time_h*60 + time_m) < (Start_time_h*60 + Start_time_m)))
        {
            Flag_area_ctrl[0] = 0;    //改为雷达关闭
            gif_radar->stop();
            ui->label_radaronoff->setStyleSheet("border-image: url(:/picture/radar_stop.png)");
            ui->label_radarstate->setStyleSheet("border-image: url(:/picture/touming.png)");
        }
        else
        {
            Flag_area_ctrl[0] = 1;   //改为雷达开启
            gif_radar->start();
            ui->label_radaronoff->setStyleSheet("border-image: url(:/picture/radar_zhengchang.png)");
        }
    }
    //自动取消静音
    if(Count_auto_silent)
    {
        Count_auto_silent --;
        if(!Count_auto_silent)
        {
            Flag_auto_silent = 0;
            on_pushButton_3_clicked();
        }
    }
    //报警延长时间
    if(Flag_warn_delay)
    {
        Flag_warn_delay --;
    }
    //********雷达时间类********/<-
    //油气回收相关
    if(date_time.toString("hh:mm:ss") == "23:59:30")
    {
        on_pushButton_4_clicked();//每天数据分析 reoilgas
        Flag_SendMode_Oilgas = 4;//写油气回收设置，关枪
    }
    //网线状态判断
    if(time_s%40 == 0)//每一分钟判断一次
    {
        if(net_detect("eth1") == -1)
        {
            if(net_state != 1)
            {
                add_value_netinfo("网路线缆已拔出");
                //ui->label_9->setHidden(0);
            }
            net_state = 1;
        }
        else
        {
            if(net_state != 0)
            {
                add_value_netinfo("网络线缆已连接");
                //ui->label_9->setHidden(1);
            }
            net_state = 0;
        }
        if(net_state)
        {
            ui->label_9->setHidden(0);
        }
        else
        {
            ui->label_9->setHidden(1);
        }

    }
    //IIE倒计时部分
    if(Flag_IIE_timeoil == 1)
    {
        ui->label_IIE_time_oil->setText(QString::number(Time_IIE_oil).append("S"));
    }
    else
    {
        //ui->label_IIE_time_oil->setText("空");
    }
    if(Flag_IIE_timepeo == 1)
    {
        ui->label_IIE_time_peo->setText(QString::number(Time_IIE_peo).append("S"));
    }
    else
    {
        //ui->label_IIE_time_peo->setText("空");
    }
    if(Time_IIE_oil <= 0)
    {
        Time_IIE_oil = 1;
    }
    if(Time_IIE_peo <= 0)
    {
        Time_IIE_peo = 1;
    }
    Time_IIE_oil--;
    Time_IIE_peo--;

    //查看存储空间是否够用  小于4M
    if((time_h == 6)&&(time_m == 0)&&(time_s == 0))
    {
        //获取剩余内存
        QString mem_available;
        QProcess mem_process;
        mem_process.start("df /opt");
		mem_process.waitForStarted();
        mem_process.waitForFinished();
        QByteArray mem_output = mem_process.readAllStandardOutput();
        QString str_output = mem_output;
        str_output.replace(QRegExp("[\\s]+"), " ");  //把所有的多余的空格转为一个空格
        mem_available = str_output.section(' ', 10, 10);
        qDebug()<< "available is "<<mem_available;
        if(mem_available.toInt() < 4096)
        {
            ui->label_warn_rom->setHidden(0);
            ui->widget_warn_rom->setHidden(0);
        }
		if(mem_available.toInt() < 2048)
		{
			system("rm -r /opt/alptecdata.db");
			system("reboot");
		}
    }
    //时间校准函数
    if(time_day%7 == 0)
    {
        if((time_h == 3)&&(time_m == 0)&&(time_s == 0))//每周凌晨3点
        {
            emit Time_calibration();
        }
    }
    //    息屏相关，已弃用
    //    if(!(Flag_Timeto_CloseNeeded[0]+Flag_Timeto_CloseNeeded[1]+Flag_Timeto_CloseNeeded[2]+Flag_Timeto_CloseNeeded[3]+Flag_Timeto_CloseNeeded[4]+Flag_Timeto_CloseNeeded[5]))
    //    {
    //        if(Flag_Timeto_Closescreen > 0)
    //        {
    //            Flag_Timeto_Closescreen--;

    //        }
    //    }
    //    else
    //    {
    //        Flag_Timeto_Closescreen = 120;
    ////        if(Flag_Timeto_CloseNeeded[3]||Flag_Timeto_CloseNeeded[4]||Flag_Timeto_CloseNeeded[2]||Flag_Timeto_CloseNeeded[0])
    ////        {

    ////        }
    //    }
    //    if(!Flag_Timeto_Closescreen)
    //    {
    //        //system("echo '4' > /sys/class/graphics/fb0/blank"); //息屏  暂时屏蔽息屏
    //    }
    //    else
    //    {
    //        system("echo '0' > /sys/class/graphics/fb0/blank"); //亮屏
    //    }
}
void MainWindow::mainwindow_enabled()
{
    ui->pushButton->setEnabled(1);
    ui->pushButton_2->setEnabled(1);
    ui->pushButton_7->setEnabled(1);
}
void MainWindow::pushButton_history_enabled()
{
    ui->pushButton_7->setEnabled(1);
    flag_exchange_history = 0;
}
void MainWindow::pushButton_connect_enabled()
{
    ui->pushButton_2->setEnabled(1);
    flag_exchange_connect = 0;
}

void MainWindow::area_painted()
{
      //1#区域设置
    {
        //判断有几个点，只要y有0的则不认
        unsigned char count_point = 0;
        for(uchar i = 0;i < 6;i++)
        {
            if(Master_Boundary_Point_Disp[Communication_Machine][0][i][1])
            {
                count_point ++;
            }
            else
            {
                break;
            }
        }
        if(count_point < 3)
        {
            printf("This Area Is Not Exist!\n");
            QVector<double> x(1),y(1);
            x[0] = 0;y[0] = 0;
            ui->paint_area->graph(0)->setData(x,y);
            ui->paint_area->graph(1)->setData(x,y);
            ui->paint_area->graph(2)->setData(x,y);
            ui->paint_area->graph(3)->setData(x,y);
            ui->paint_area->graph(4)->setData(x,y);
            ui->paint_area->graph(5)->setData(x,y);

            ui->paint_area->replot();
     //       return; //可以添加坐标检查不通过的提示
        }

        if(count_point == 3)
        {
            QVector<double> x1(2),y1(2);
            QVector<double> x2(2),y2(2);
            QVector<double> x3(2),y3(2);

            x1[0] = Master_Boundary_Point_Disp[Communication_Machine][0][0][0]/10;y1[0] = Master_Boundary_Point_Disp[Communication_Machine][0][0][1]/10;
            x1[1] = Master_Boundary_Point_Disp[Communication_Machine][0][1][0]/10;y1[1] = Master_Boundary_Point_Disp[Communication_Machine][0][1][1]/10;

            x2[0] = Master_Boundary_Point_Disp[Communication_Machine][0][1][0]/10;y2[0] = Master_Boundary_Point_Disp[Communication_Machine][0][1][1]/10;
            x2[1] = Master_Boundary_Point_Disp[Communication_Machine][0][2][0]/10;y2[1] = Master_Boundary_Point_Disp[Communication_Machine][0][2][1]/10;

            x3[0] = Master_Boundary_Point_Disp[Communication_Machine][0][2][0]/10;y3[0] = Master_Boundary_Point_Disp[Communication_Machine][0][2][1]/10;
            x3[1] = Master_Boundary_Point_Disp[Communication_Machine][0][0][0]/10;y3[1] = Master_Boundary_Point_Disp[Communication_Machine][0][0][1]/10;

            ui->paint_area->graph(0)->setData(x1,y1);
            ui->paint_area->graph(1)->setData(x2,y2);
            ui->paint_area->graph(2)->setData(x3,y3);

        }
        if(count_point == 4)
        {
            QVector<double> x1(2),y1(2);
            QVector<double> x2(2),y2(2);
            QVector<double> x3(2),y3(2);
            QVector<double> x4(2),y4(2);
            x1[0] = Master_Boundary_Point_Disp[Communication_Machine][0][0][0]/10;y1[0] = Master_Boundary_Point_Disp[Communication_Machine][0][0][1]/10;
            x1[1] = Master_Boundary_Point_Disp[Communication_Machine][0][1][0]/10;y1[1] = Master_Boundary_Point_Disp[Communication_Machine][0][1][1]/10;

            x2[0] = Master_Boundary_Point_Disp[Communication_Machine][0][1][0]/10;y2[0] = Master_Boundary_Point_Disp[Communication_Machine][0][1][1]/10;
            x2[1] = Master_Boundary_Point_Disp[Communication_Machine][0][2][0]/10;y2[1] = Master_Boundary_Point_Disp[Communication_Machine][0][2][1]/10;

            x3[0] = Master_Boundary_Point_Disp[Communication_Machine][0][2][0]/10;y3[0] = Master_Boundary_Point_Disp[Communication_Machine][0][2][1]/10;
            x3[1] = Master_Boundary_Point_Disp[Communication_Machine][0][3][0]/10;y3[1] = Master_Boundary_Point_Disp[Communication_Machine][0][3][1]/10;

            x4[0] = Master_Boundary_Point_Disp[Communication_Machine][0][3][0]/10;y4[0] = Master_Boundary_Point_Disp[Communication_Machine][0][3][1]/10;
            x4[1] = Master_Boundary_Point_Disp[Communication_Machine][0][0][0]/10;y4[1] = Master_Boundary_Point_Disp[Communication_Machine][0][0][1]/10;

            ui->paint_area->graph(0)->setData(x1,y1);
            ui->paint_area->graph(1)->setData(x2,y2);
            ui->paint_area->graph(2)->setData(x3,y3);
            ui->paint_area->graph(3)->setData(x4,y4);
        }
        if(count_point == 5)
        {
            QVector<double> x1(2),y1(2);
            QVector<double> x2(2),y2(2);
            QVector<double> x3(2),y3(2);
            QVector<double> x4(2),y4(2);
            QVector<double> x5(2),y5(2);

            x1[0] = Master_Boundary_Point_Disp[Communication_Machine][0][0][0]/10;y1[0] = Master_Boundary_Point_Disp[Communication_Machine][0][0][1]/10;
            x1[1] = Master_Boundary_Point_Disp[Communication_Machine][0][1][0]/10;y1[1] = Master_Boundary_Point_Disp[Communication_Machine][0][1][1]/10;

            x2[0] = Master_Boundary_Point_Disp[Communication_Machine][0][1][0]/10;y2[0] = Master_Boundary_Point_Disp[Communication_Machine][0][1][1]/10;
            x2[1] = Master_Boundary_Point_Disp[Communication_Machine][0][2][0]/10;y2[1] = Master_Boundary_Point_Disp[Communication_Machine][0][2][1]/10;

            x3[0] = Master_Boundary_Point_Disp[Communication_Machine][0][2][0]/10;y3[0] = Master_Boundary_Point_Disp[Communication_Machine][0][2][1]/10;
            x3[1] = Master_Boundary_Point_Disp[Communication_Machine][0][3][0]/10;y3[1] = Master_Boundary_Point_Disp[Communication_Machine][0][3][1]/10;

            x4[0] = Master_Boundary_Point_Disp[Communication_Machine][0][3][0]/10;y4[0] = Master_Boundary_Point_Disp[Communication_Machine][0][3][1]/10;
            x4[1] = Master_Boundary_Point_Disp[Communication_Machine][0][4][0]/10;y4[1] = Master_Boundary_Point_Disp[Communication_Machine][0][4][1]/10;

            x5[0] = Master_Boundary_Point_Disp[Communication_Machine][0][4][0]/10;y5[0] = Master_Boundary_Point_Disp[Communication_Machine][0][4][1]/10;
            x5[1] = Master_Boundary_Point_Disp[Communication_Machine][0][0][0]/10;y5[1] = Master_Boundary_Point_Disp[Communication_Machine][0][0][1]/10;

            ui->paint_area->graph(0)->setData(x1,y1);
            ui->paint_area->graph(1)->setData(x2,y2);
            ui->paint_area->graph(2)->setData(x3,y3);
            ui->paint_area->graph(3)->setData(x4,y4);
            ui->paint_area->graph(4)->setData(x5,y5);
        }
        if(count_point == 6)
        {
            QVector<double> x1(2),y1(2);
            QVector<double> x2(2),y2(2);
            QVector<double> x3(2),y3(2);
            QVector<double> x4(2),y4(2);
            QVector<double> x5(2),y5(2);
            QVector<double> x6(2),y6(2);

            x1[0] = Master_Boundary_Point_Disp[Communication_Machine][0][0][0]/10;y1[0] = Master_Boundary_Point_Disp[Communication_Machine][0][0][1]/10;
            x1[1] = Master_Boundary_Point_Disp[Communication_Machine][0][1][0]/10;y1[1] = Master_Boundary_Point_Disp[Communication_Machine][0][1][1]/10;

            x2[0] = Master_Boundary_Point_Disp[Communication_Machine][0][1][0]/10;y2[0] = Master_Boundary_Point_Disp[Communication_Machine][0][1][1]/10;
            x2[1] = Master_Boundary_Point_Disp[Communication_Machine][0][2][0]/10;y2[1] = Master_Boundary_Point_Disp[Communication_Machine][0][2][1]/10;

            x3[0] = Master_Boundary_Point_Disp[Communication_Machine][0][2][0]/10;y3[0] = Master_Boundary_Point_Disp[Communication_Machine][0][2][1]/10;
            x3[1] = Master_Boundary_Point_Disp[Communication_Machine][0][3][0]/10;y3[1] = Master_Boundary_Point_Disp[Communication_Machine][0][3][1]/10;

            x4[0] = Master_Boundary_Point_Disp[Communication_Machine][0][3][0]/10;y4[0] = Master_Boundary_Point_Disp[Communication_Machine][0][3][1]/10;
            x4[1] = Master_Boundary_Point_Disp[Communication_Machine][0][4][0]/10;y4[1] = Master_Boundary_Point_Disp[Communication_Machine][0][4][1]/10;

            x5[0] = Master_Boundary_Point_Disp[Communication_Machine][0][4][0]/10;y5[0] = Master_Boundary_Point_Disp[Communication_Machine][0][4][1]/10;
            x5[1] = Master_Boundary_Point_Disp[Communication_Machine][0][5][0]/10;y5[1] = Master_Boundary_Point_Disp[Communication_Machine][0][5][1]/10;

            x6[0] = Master_Boundary_Point_Disp[Communication_Machine][0][5][0]/10;y6[0] = Master_Boundary_Point_Disp[Communication_Machine][0][5][1]/10;
            x6[1] = Master_Boundary_Point_Disp[Communication_Machine][0][0][0]/10;y6[1] = Master_Boundary_Point_Disp[Communication_Machine][0][0][1]/10;

            ui->paint_area->graph(0)->setData(x1,y1);
            ui->paint_area->graph(1)->setData(x2,y2);
            ui->paint_area->graph(2)->setData(x3,y3);
            ui->paint_area->graph(3)->setData(x4,y4);
            ui->paint_area->graph(4)->setData(x5,y5);
            ui->paint_area->graph(5)->setData(x6,y6);
        }

        ui->paint_area->replot();
  //      ui->paint_area->removePlottable(0);
    }

    {
       //判断有几个点，只要y有0的则不认
       unsigned char count_point = 0;
       for(uchar i = 0;i < 6;i++)
       {
           if(Master_Boundary_Point_Disp[Communication_Machine][1][i][1])
           {
               count_point ++;
           }
           else
           {
               break;
           }
       }
       if(count_point < 3)
       {
           printf("This Area Is Not Exist!\n");
           QVector<double> x(1),y(1);
           x[0] = 0;y[0] = 0;
           ui->paint_area->graph(6)->setData(x,y);
           ui->paint_area->graph(7)->setData(x,y);
           ui->paint_area->graph(8)->setData(x,y);
           ui->paint_area->graph(9)->setData(x,y);
           ui->paint_area->graph(10)->setData(x,y);
           ui->paint_area->graph(11)->setData(x,y);

           ui->paint_area->replot();
       //    return; //可以添加坐标检查不通过的提示
       }

       if(count_point == 3)
       {
           QVector<double> x1(2),y1(2);
           QVector<double> x2(2),y2(2);
           QVector<double> x3(2),y3(2);

           x1[0] = Master_Boundary_Point_Disp[Communication_Machine][1][0][0]/10;y1[0] = Master_Boundary_Point_Disp[Communication_Machine][1][0][1]/10;
           x1[1] = Master_Boundary_Point_Disp[Communication_Machine][1][1][0]/10;y1[1] = Master_Boundary_Point_Disp[Communication_Machine][1][1][1]/10;

           x2[0] = Master_Boundary_Point_Disp[Communication_Machine][1][1][0]/10;y2[0] = Master_Boundary_Point_Disp[Communication_Machine][1][1][1]/10;
           x2[1] = Master_Boundary_Point_Disp[Communication_Machine][1][2][0]/10;y2[1] = Master_Boundary_Point_Disp[Communication_Machine][1][2][1]/10;

           x3[0] = Master_Boundary_Point_Disp[Communication_Machine][1][2][0]/10;y3[0] = Master_Boundary_Point_Disp[Communication_Machine][1][2][1]/10;
           x3[1] = Master_Boundary_Point_Disp[Communication_Machine][1][0][0]/10;y3[1] = Master_Boundary_Point_Disp[Communication_Machine][1][0][1]/10;

           ui->paint_area->graph(6)->setData(x1,y1);
           ui->paint_area->graph(7)->setData(x2,y2);
           ui->paint_area->graph(8)->setData(x3,y3);
       }

       if(count_point == 4)
       {
           QVector<double> x1(2),y1(2);
           QVector<double> x2(2),y2(2);
           QVector<double> x3(2),y3(2);
           QVector<double> x4(2),y4(2);
           x1[0] = Master_Boundary_Point_Disp[Communication_Machine][1][0][0]/10;y1[0] = Master_Boundary_Point_Disp[Communication_Machine][1][0][1]/10;
           x1[1] = Master_Boundary_Point_Disp[Communication_Machine][1][1][0]/10;y1[1] = Master_Boundary_Point_Disp[Communication_Machine][1][1][1]/10;

           x2[0] = Master_Boundary_Point_Disp[Communication_Machine][1][1][0]/10;y2[0] = Master_Boundary_Point_Disp[Communication_Machine][1][1][1]/10;
           x2[1] = Master_Boundary_Point_Disp[Communication_Machine][1][2][0]/10;y2[1] = Master_Boundary_Point_Disp[Communication_Machine][1][2][1]/10;

           x3[0] = Master_Boundary_Point_Disp[Communication_Machine][1][2][0]/10;y3[0] = Master_Boundary_Point_Disp[Communication_Machine][1][2][1]/10;
           x3[1] = Master_Boundary_Point_Disp[Communication_Machine][1][3][0]/10;y3[1] = Master_Boundary_Point_Disp[Communication_Machine][1][3][1]/10;

           x4[0] = Master_Boundary_Point_Disp[Communication_Machine][1][3][0]/10;y4[0] = Master_Boundary_Point_Disp[Communication_Machine][1][3][1]/10;
           x4[1] = Master_Boundary_Point_Disp[Communication_Machine][1][0][0]/10;y4[1] = Master_Boundary_Point_Disp[Communication_Machine][1][0][1]/10;

           ui->paint_area->graph(6)->setData(x1,y1);
           ui->paint_area->graph(7)->setData(x2,y2);
           ui->paint_area->graph(8)->setData(x3,y3);
           ui->paint_area->graph(9)->setData(x4,y4);
       }
       if(count_point == 5)
       {
           QVector<double> x1(2),y1(2);
           QVector<double> x2(2),y2(2);
           QVector<double> x3(2),y3(2);
           QVector<double> x4(2),y4(2);
           QVector<double> x5(2),y5(2);

           x1[0] = Master_Boundary_Point_Disp[Communication_Machine][1][0][0]/10;y1[0] = Master_Boundary_Point_Disp[Communication_Machine][1][0][1]/10;
           x1[1] = Master_Boundary_Point_Disp[Communication_Machine][1][1][0]/10;y1[1] = Master_Boundary_Point_Disp[Communication_Machine][1][1][1]/10;

           x2[0] = Master_Boundary_Point_Disp[Communication_Machine][1][1][0]/10;y2[0] = Master_Boundary_Point_Disp[Communication_Machine][1][1][1]/10;
           x2[1] = Master_Boundary_Point_Disp[Communication_Machine][1][2][0]/10;y2[1] = Master_Boundary_Point_Disp[Communication_Machine][1][2][1]/10;

           x3[0] = Master_Boundary_Point_Disp[Communication_Machine][1][2][0]/10;y3[0] = Master_Boundary_Point_Disp[Communication_Machine][1][2][1]/10;
           x3[1] = Master_Boundary_Point_Disp[Communication_Machine][1][3][0]/10;y3[1] = Master_Boundary_Point_Disp[Communication_Machine][1][3][1]/10;

           x4[0] = Master_Boundary_Point_Disp[Communication_Machine][1][3][0]/10;y4[0] = Master_Boundary_Point_Disp[Communication_Machine][1][3][1]/10;
           x4[1] = Master_Boundary_Point_Disp[Communication_Machine][1][4][0]/10;y4[1] = Master_Boundary_Point_Disp[Communication_Machine][1][4][1]/10;

           x5[0] = Master_Boundary_Point_Disp[Communication_Machine][1][4][0]/10;y5[0] = Master_Boundary_Point_Disp[Communication_Machine][1][4][1]/10;
           x5[1] = Master_Boundary_Point_Disp[Communication_Machine][1][0][0]/10;y5[1] = Master_Boundary_Point_Disp[Communication_Machine][1][0][1]/10;

           ui->paint_area->graph(6)->setData(x1,y1);
           ui->paint_area->graph(7)->setData(x2,y2);
           ui->paint_area->graph(8)->setData(x3,y3);
           ui->paint_area->graph(9)->setData(x4,y4);
           ui->paint_area->graph(10)->setData(x5,y5);
       }
       if(count_point == 6)
       {
           QVector<double> x1(2),y1(2);
           QVector<double> x2(2),y2(2);
           QVector<double> x3(2),y3(2);
           QVector<double> x4(2),y4(2);
           QVector<double> x5(2),y5(2);
           QVector<double> x6(2),y6(2);

           x1[0] = Master_Boundary_Point_Disp[Communication_Machine][1][0][0]/10;y1[0] = Master_Boundary_Point_Disp[Communication_Machine][1][0][1]/10;
           x1[1] = Master_Boundary_Point_Disp[Communication_Machine][1][1][0]/10;y1[1] = Master_Boundary_Point_Disp[Communication_Machine][1][1][1]/10;

           x2[0] = Master_Boundary_Point_Disp[Communication_Machine][1][1][0]/10;y2[0] = Master_Boundary_Point_Disp[Communication_Machine][1][1][1]/10;
           x2[1] = Master_Boundary_Point_Disp[Communication_Machine][1][2][0]/10;y2[1] = Master_Boundary_Point_Disp[Communication_Machine][1][2][1]/10;

           x3[0] = Master_Boundary_Point_Disp[Communication_Machine][1][2][0]/10;y3[0] = Master_Boundary_Point_Disp[Communication_Machine][1][2][1]/10;
           x3[1] = Master_Boundary_Point_Disp[Communication_Machine][1][3][0]/10;y3[1] = Master_Boundary_Point_Disp[Communication_Machine][1][3][1]/10;

           x4[0] = Master_Boundary_Point_Disp[Communication_Machine][1][3][0]/10;y4[0] = Master_Boundary_Point_Disp[Communication_Machine][1][3][1]/10;
           x4[1] = Master_Boundary_Point_Disp[Communication_Machine][1][4][0]/10;y4[1] = Master_Boundary_Point_Disp[Communication_Machine][1][4][1]/10;

           x5[0] = Master_Boundary_Point_Disp[Communication_Machine][1][4][0]/10;y5[0] = Master_Boundary_Point_Disp[Communication_Machine][1][4][1]/10;
           x5[1] = Master_Boundary_Point_Disp[Communication_Machine][1][5][0]/10;y5[1] = Master_Boundary_Point_Disp[Communication_Machine][1][5][1]/10;

           x6[0] = Master_Boundary_Point_Disp[Communication_Machine][1][5][0]/10;y6[0] = Master_Boundary_Point_Disp[Communication_Machine][1][5][1]/10;
           x6[1] = Master_Boundary_Point_Disp[Communication_Machine][1][0][0]/10;y6[1] = Master_Boundary_Point_Disp[Communication_Machine][1][0][1]/10;

           ui->paint_area->graph(6)->setData(x1,y1);
           ui->paint_area->graph(7)->setData(x2,y2);
           ui->paint_area->graph(8)->setData(x3,y3);
           ui->paint_area->graph(9)->setData(x4,y4);
           ui->paint_area->graph(10)->setData(x5,y5);
           ui->paint_area->graph(11)->setData(x6,y6);
       }

       ui->paint_area->replot();
    }

    {
       //判断有几个点，只要y有0的则不认
       unsigned char count_point = 0;
       for(uchar i = 0;i < 6;i++)
       {
           if(Master_Boundary_Point_Disp[Communication_Machine][2][i][1])
           {
               count_point ++;
           }
           else
           {
               break;
           }
       }
       if(count_point < 3)
       {
           printf("This Area Is Not Exist!\n");

           QVector<double> x(1),y(1);
           x[0] = 0;y[0] = 0;
           ui->paint_area->graph(12)->setData(x,y);
           ui->paint_area->graph(13)->setData(x,y);
           ui->paint_area->graph(14)->setData(x,y);
           ui->paint_area->graph(15)->setData(x,y);
           ui->paint_area->graph(16)->setData(x,y);
           ui->paint_area->graph(17)->setData(x,y);

           ui->paint_area->replot();

     //      return; //可以添加坐标检查不通过的提示
       }

       if(count_point == 3)
       {
           QVector<double> x1(2),y1(2);
           QVector<double> x2(2),y2(2);
           QVector<double> x3(2),y3(2);

           x1[0] = Master_Boundary_Point_Disp[Communication_Machine][2][0][0]/10;y1[0] = Master_Boundary_Point_Disp[Communication_Machine][2][0][1]/10;
           x1[1] = Master_Boundary_Point_Disp[Communication_Machine][2][1][0]/10;y1[1] = Master_Boundary_Point_Disp[Communication_Machine][2][1][1]/10;

           x2[0] = Master_Boundary_Point_Disp[Communication_Machine][2][1][0]/10;y2[0] = Master_Boundary_Point_Disp[Communication_Machine][2][1][1]/10;
           x2[1] = Master_Boundary_Point_Disp[Communication_Machine][2][2][0]/10;y2[1] = Master_Boundary_Point_Disp[Communication_Machine][2][2][1]/10;

           x3[0] = Master_Boundary_Point_Disp[Communication_Machine][2][2][0]/10;y3[0] = Master_Boundary_Point_Disp[Communication_Machine][2][2][1]/10;
           x3[1] = Master_Boundary_Point_Disp[Communication_Machine][2][0][0]/10;y3[1] = Master_Boundary_Point_Disp[Communication_Machine][2][0][1]/10;


           ui->paint_area->graph(12)->setData(x1,y1);
           ui->paint_area->graph(13)->setData(x2,y2);
           ui->paint_area->graph(14)->setData(x3,y3);
       }

       if(count_point == 4)
       {
           QVector<double> x1(2),y1(2);
           QVector<double> x2(2),y2(2);
           QVector<double> x3(2),y3(2);
           QVector<double> x4(2),y4(2);
           x1[0] = Master_Boundary_Point_Disp[Communication_Machine][2][0][0]/10;y1[0] = Master_Boundary_Point_Disp[Communication_Machine][2][0][1]/10;
           x1[1] = Master_Boundary_Point_Disp[Communication_Machine][2][1][0]/10;y1[1] = Master_Boundary_Point_Disp[Communication_Machine][2][1][1]/10;

           x2[0] = Master_Boundary_Point_Disp[Communication_Machine][2][1][0]/10;y2[0] = Master_Boundary_Point_Disp[Communication_Machine][2][1][1]/10;
           x2[1] = Master_Boundary_Point_Disp[Communication_Machine][2][2][0]/10;y2[1] = Master_Boundary_Point_Disp[Communication_Machine][2][2][1]/10;

           x3[0] = Master_Boundary_Point_Disp[Communication_Machine][2][2][0]/10;y3[0] = Master_Boundary_Point_Disp[Communication_Machine][2][2][1]/10;
           x3[1] = Master_Boundary_Point_Disp[Communication_Machine][2][3][0]/10;y3[1] = Master_Boundary_Point_Disp[Communication_Machine][2][3][1]/10;

           x4[0] = Master_Boundary_Point_Disp[Communication_Machine][2][3][0]/10;y4[0] = Master_Boundary_Point_Disp[Communication_Machine][2][3][1]/10;
           x4[1] = Master_Boundary_Point_Disp[Communication_Machine][2][0][0]/10;y4[1] = Master_Boundary_Point_Disp[Communication_Machine][2][0][1]/10;

           ui->paint_area->graph(12)->setData(x1,y1);
           ui->paint_area->graph(13)->setData(x2,y2);
           ui->paint_area->graph(14)->setData(x3,y3);
           ui->paint_area->graph(15)->setData(x4,y4);
       }
       if(count_point == 5)
       {
           QVector<double> x1(2),y1(2);
           QVector<double> x2(2),y2(2);
           QVector<double> x3(2),y3(2);
           QVector<double> x4(2),y4(2);
           QVector<double> x5(2),y5(2);

           x1[0] = Master_Boundary_Point_Disp[Communication_Machine][2][0][0]/10;y1[0] = Master_Boundary_Point_Disp[Communication_Machine][2][0][1]/10;
           x1[1] = Master_Boundary_Point_Disp[Communication_Machine][2][1][0]/10;y1[1] = Master_Boundary_Point_Disp[Communication_Machine][2][1][1]/10;

           x2[0] = Master_Boundary_Point_Disp[Communication_Machine][2][1][0]/10;y2[0] = Master_Boundary_Point_Disp[Communication_Machine][2][1][1]/10;
           x2[1] = Master_Boundary_Point_Disp[Communication_Machine][2][2][0]/10;y2[1] = Master_Boundary_Point_Disp[Communication_Machine][2][2][1]/10;

           x3[0] = Master_Boundary_Point_Disp[Communication_Machine][2][2][0]/10;y3[0] = Master_Boundary_Point_Disp[Communication_Machine][2][2][1]/10;
           x3[1] = Master_Boundary_Point_Disp[Communication_Machine][2][3][0]/10;y3[1] = Master_Boundary_Point_Disp[Communication_Machine][2][3][1]/10;

           x4[0] = Master_Boundary_Point_Disp[Communication_Machine][2][3][0]/10;y4[0] = Master_Boundary_Point_Disp[Communication_Machine][2][3][1]/10;
           x4[1] = Master_Boundary_Point_Disp[Communication_Machine][2][4][0]/10;y4[1] = Master_Boundary_Point_Disp[Communication_Machine][2][4][1]/10;

           x5[0] = Master_Boundary_Point_Disp[Communication_Machine][2][4][0]/10;y5[0] = Master_Boundary_Point_Disp[Communication_Machine][2][4][1]/10;
           x5[1] = Master_Boundary_Point_Disp[Communication_Machine][2][0][0]/10;y5[1] = Master_Boundary_Point_Disp[Communication_Machine][2][0][1]/10;

           ui->paint_area->graph(12)->setData(x1,y1);
           ui->paint_area->graph(13)->setData(x2,y2);
           ui->paint_area->graph(14)->setData(x3,y3);
           ui->paint_area->graph(15)->setData(x4,y4);
           ui->paint_area->graph(16)->setData(x5,y5);
       }
       if(count_point == 6)
       {
           QVector<double> x1(2),y1(2);
           QVector<double> x2(2),y2(2);
           QVector<double> x3(2),y3(2);
           QVector<double> x4(2),y4(2);
           QVector<double> x5(2),y5(2);
           QVector<double> x6(2),y6(2);

           x1[0] = Master_Boundary_Point_Disp[Communication_Machine][2][0][0]/10;y1[0] = Master_Boundary_Point_Disp[Communication_Machine][2][0][1]/10;
           x1[1] = Master_Boundary_Point_Disp[Communication_Machine][2][1][0]/10;y1[1] = Master_Boundary_Point_Disp[Communication_Machine][2][1][1]/10;

           x2[0] = Master_Boundary_Point_Disp[Communication_Machine][2][1][0]/10;y2[0] = Master_Boundary_Point_Disp[Communication_Machine][2][1][1]/10;
           x2[1] = Master_Boundary_Point_Disp[Communication_Machine][2][2][0]/10;y2[1] = Master_Boundary_Point_Disp[Communication_Machine][2][2][1]/10;

           x3[0] = Master_Boundary_Point_Disp[Communication_Machine][2][2][0]/10;y3[0] = Master_Boundary_Point_Disp[Communication_Machine][2][2][1]/10;
           x3[1] = Master_Boundary_Point_Disp[Communication_Machine][2][3][0]/10;y3[1] = Master_Boundary_Point_Disp[Communication_Machine][2][3][1]/10;

           x4[0] = Master_Boundary_Point_Disp[Communication_Machine][2][3][0]/10;y4[0] = Master_Boundary_Point_Disp[Communication_Machine][2][3][1]/10;
           x4[1] = Master_Boundary_Point_Disp[Communication_Machine][2][4][0]/10;y4[1] = Master_Boundary_Point_Disp[Communication_Machine][2][4][1]/10;

           x5[0] = Master_Boundary_Point_Disp[Communication_Machine][2][4][0]/10;y5[0] = Master_Boundary_Point_Disp[Communication_Machine][2][4][1]/10;
           x5[1] = Master_Boundary_Point_Disp[Communication_Machine][2][5][0]/10;y5[1] = Master_Boundary_Point_Disp[Communication_Machine][2][5][1]/10;

           x6[0] = Master_Boundary_Point_Disp[Communication_Machine][2][5][0]/10;y6[0] = Master_Boundary_Point_Disp[Communication_Machine][2][5][1]/10;
           x6[1] = Master_Boundary_Point_Disp[Communication_Machine][2][0][0]/10;y6[1] = Master_Boundary_Point_Disp[Communication_Machine][2][0][1]/10;

           ui->paint_area->graph(12)->setData(x1,y1);
           ui->paint_area->graph(13)->setData(x2,y2);
           ui->paint_area->graph(14)->setData(x3,y3);
           ui->paint_area->graph(15)->setData(x4,y4);
           ui->paint_area->graph(16)->setData(x5,y5);
           ui->paint_area->graph(17)->setData(x6,y6);
       }

       ui->paint_area->replot();
 //      ui->widget->removePlottable(0);
   }

    {
       //判断有几个点，只要y有0的则不认
       unsigned char count_point = 0;
       for(uchar i = 0;i < 6;i++)
       {
           if(Master_Boundary_Point_Disp[Communication_Machine][3][i][1])
           {
               count_point ++;
           }
           else
           {
               break;
           }
       }
       if(count_point < 3)
       {
           printf("This Area Is Not Exist!\n");

           QVector<double> x(1),y(1);
           x[0] = 0;y[0] = 0;
           ui->paint_area->graph(18)->setData(x,y);
           ui->paint_area->graph(19)->setData(x,y);
           ui->paint_area->graph(20)->setData(x,y);
           ui->paint_area->graph(21)->setData(x,y);
           ui->paint_area->graph(22)->setData(x,y);
           ui->paint_area->graph(23)->setData(x,y);

           ui->paint_area->replot();

  //         return; //可以添加坐标检查不通过的提示
       }

       if(count_point == 3)
       {
           QVector<double> x1(2),y1(2);
           QVector<double> x2(2),y2(2);
           QVector<double> x3(2),y3(2);

           x1[0] = Master_Boundary_Point_Disp[Communication_Machine][3][0][0]/10;y1[0] = Master_Boundary_Point_Disp[Communication_Machine][3][0][1]/10;
           x1[1] = Master_Boundary_Point_Disp[Communication_Machine][3][1][0]/10;y1[1] = Master_Boundary_Point_Disp[Communication_Machine][3][1][1]/10;

           x2[0] = Master_Boundary_Point_Disp[Communication_Machine][3][1][0]/10;y2[0] = Master_Boundary_Point_Disp[Communication_Machine][3][1][1]/10;
           x2[1] = Master_Boundary_Point_Disp[Communication_Machine][3][2][0]/10;y2[1] = Master_Boundary_Point_Disp[Communication_Machine][3][2][1]/10;

           x3[0] = Master_Boundary_Point_Disp[Communication_Machine][3][2][0]/10;y3[0] = Master_Boundary_Point_Disp[Communication_Machine][3][2][1]/10;
           x3[1] = Master_Boundary_Point_Disp[Communication_Machine][3][0][0]/10;y3[1] = Master_Boundary_Point_Disp[Communication_Machine][3][0][1]/10;


           ui->paint_area->graph(18)->setData(x1,y1);
           ui->paint_area->graph(19)->setData(x2,y2);
           ui->paint_area->graph(20)->setData(x3,y3);
       }

       if(count_point == 4)
       {
           QVector<double> x1(2),y1(2);
           QVector<double> x2(2),y2(2);
           QVector<double> x3(2),y3(2);
           QVector<double> x4(2),y4(2);
           x1[0] = Master_Boundary_Point_Disp[Communication_Machine][3][0][0]/10;y1[0] = Master_Boundary_Point_Disp[Communication_Machine][3][0][1]/10;
           x1[1] = Master_Boundary_Point_Disp[Communication_Machine][3][1][0]/10;y1[1] = Master_Boundary_Point_Disp[Communication_Machine][3][1][1]/10;

           x2[0] = Master_Boundary_Point_Disp[Communication_Machine][3][1][0]/10;y2[0] = Master_Boundary_Point_Disp[Communication_Machine][3][1][1]/10;
           x2[1] = Master_Boundary_Point_Disp[Communication_Machine][3][2][0]/10;y2[1] = Master_Boundary_Point_Disp[Communication_Machine][3][2][1]/10;

           x3[0] = Master_Boundary_Point_Disp[Communication_Machine][3][2][0]/10;y3[0] = Master_Boundary_Point_Disp[Communication_Machine][3][2][1]/10;
           x3[1] = Master_Boundary_Point_Disp[Communication_Machine][3][3][0]/10;y3[1] = Master_Boundary_Point_Disp[Communication_Machine][3][3][1]/10;

           x4[0] = Master_Boundary_Point_Disp[Communication_Machine][3][3][0]/10;y4[0] = Master_Boundary_Point_Disp[Communication_Machine][3][3][1]/10;
           x4[1] = Master_Boundary_Point_Disp[Communication_Machine][3][0][0]/10;y4[1] = Master_Boundary_Point_Disp[Communication_Machine][3][0][1]/10;

           ui->paint_area->graph(18)->setData(x1,y1);
           ui->paint_area->graph(19)->setData(x2,y2);
           ui->paint_area->graph(20)->setData(x3,y3);
           ui->paint_area->graph(21)->setData(x4,y4);
       }
       if(count_point == 5)
       {
           QVector<double> x1(2),y1(2);
           QVector<double> x2(2),y2(2);
           QVector<double> x3(2),y3(2);
           QVector<double> x4(2),y4(2);
           QVector<double> x5(2),y5(2);

           x1[0] = Master_Boundary_Point_Disp[Communication_Machine][3][0][0]/10;y1[0] = Master_Boundary_Point_Disp[Communication_Machine][3][0][1]/10;
           x1[1] = Master_Boundary_Point_Disp[Communication_Machine][3][1][0]/10;y1[1] = Master_Boundary_Point_Disp[Communication_Machine][3][1][1]/10;

           x2[0] = Master_Boundary_Point_Disp[Communication_Machine][3][1][0]/10;y2[0] = Master_Boundary_Point_Disp[Communication_Machine][3][1][1]/10;
           x2[1] = Master_Boundary_Point_Disp[Communication_Machine][3][2][0]/10;y2[1] = Master_Boundary_Point_Disp[Communication_Machine][3][2][1]/10;

           x3[0] = Master_Boundary_Point_Disp[Communication_Machine][3][2][0]/10;y3[0] = Master_Boundary_Point_Disp[Communication_Machine][3][2][1]/10;
           x3[1] = Master_Boundary_Point_Disp[Communication_Machine][3][3][0]/10;y3[1] = Master_Boundary_Point_Disp[Communication_Machine][3][3][1]/10;

           x4[0] = Master_Boundary_Point_Disp[Communication_Machine][3][3][0]/10;y4[0] = Master_Boundary_Point_Disp[Communication_Machine][3][3][1]/10;
           x4[1] = Master_Boundary_Point_Disp[Communication_Machine][3][4][0]/10;y4[1] = Master_Boundary_Point_Disp[Communication_Machine][3][4][1]/10;

           x5[0] = Master_Boundary_Point_Disp[Communication_Machine][3][4][0]/10;y5[0] = Master_Boundary_Point_Disp[Communication_Machine][3][4][1]/10;
           x5[1] = Master_Boundary_Point_Disp[Communication_Machine][3][0][0]/10;y5[1] = Master_Boundary_Point_Disp[Communication_Machine][3][0][1]/10;

           ui->paint_area->graph(18)->setData(x1,y1);
           ui->paint_area->graph(19)->setData(x2,y2);
           ui->paint_area->graph(20)->setData(x3,y3);
           ui->paint_area->graph(21)->setData(x4,y4);
           ui->paint_area->graph(22)->setData(x5,y5);
       }
       if(count_point == 6)
       {
           QVector<double> x1(2),y1(2);
           QVector<double> x2(2),y2(2);
           QVector<double> x3(2),y3(2);
           QVector<double> x4(2),y4(2);
           QVector<double> x5(2),y5(2);
           QVector<double> x6(2),y6(2);

           x1[0] = Master_Boundary_Point_Disp[Communication_Machine][3][0][0]/10;y1[0] = Master_Boundary_Point_Disp[Communication_Machine][3][0][1]/10;
           x1[1] = Master_Boundary_Point_Disp[Communication_Machine][3][1][0]/10;y1[1] = Master_Boundary_Point_Disp[Communication_Machine][3][1][1]/10;

           x2[0] = Master_Boundary_Point_Disp[Communication_Machine][3][1][0]/10;y2[0] = Master_Boundary_Point_Disp[Communication_Machine][3][1][1]/10;
           x2[1] = Master_Boundary_Point_Disp[Communication_Machine][3][2][0]/10;y2[1] = Master_Boundary_Point_Disp[Communication_Machine][3][2][1]/10;

           x3[0] = Master_Boundary_Point_Disp[Communication_Machine][3][2][0]/10;y3[0] = Master_Boundary_Point_Disp[Communication_Machine][3][2][1]/10;
           x3[1] = Master_Boundary_Point_Disp[Communication_Machine][3][3][0]/10;y3[1] = Master_Boundary_Point_Disp[Communication_Machine][3][3][1]/10;

           x4[0] = Master_Boundary_Point_Disp[Communication_Machine][3][3][0]/10;y4[0] = Master_Boundary_Point_Disp[Communication_Machine][3][3][1]/10;
           x4[1] = Master_Boundary_Point_Disp[Communication_Machine][3][4][0]/10;y4[1] = Master_Boundary_Point_Disp[Communication_Machine][3][4][1]/10;

           x5[0] = Master_Boundary_Point_Disp[Communication_Machine][3][4][0]/10;y5[0] = Master_Boundary_Point_Disp[Communication_Machine][3][4][1]/10;
           x5[1] = Master_Boundary_Point_Disp[Communication_Machine][3][5][0]/10;y5[1] = Master_Boundary_Point_Disp[Communication_Machine][3][5][1]/10;

           x6[0] = Master_Boundary_Point_Disp[Communication_Machine][3][5][0]/10;y6[0] = Master_Boundary_Point_Disp[Communication_Machine][3][5][1]/10;
           x6[1] = Master_Boundary_Point_Disp[Communication_Machine][3][0][0]/10;y6[1] = Master_Boundary_Point_Disp[Communication_Machine][3][0][1]/10;

           ui->paint_area->graph(18)->setData(x1,y1);
           ui->paint_area->graph(19)->setData(x2,y2);
           ui->paint_area->graph(20)->setData(x3,y3);
           ui->paint_area->graph(21)->setData(x4,y4);
           ui->paint_area->graph(22)->setData(x5,y5);
           ui->paint_area->graph(23)->setData(x6,y6);
       }

       ui->paint_area->replot();
 //      ui->widget->removePlottable(0);
   }

    {
       //判断有几个点，只要y有0的则不认
       unsigned char count_point = 0;
       for(uchar i = 0;i < 6;i++)
       {
           if(Master_Boundary_Point_Disp[Communication_Machine][4][i][1])
           {
               count_point ++;
           }
           else
           {
               break;
           }
       }
       if(count_point < 3)
       {
           printf("This Area Is Not Exist!\n");

           QVector<double> x(1),y(1);
           x[0] = 0;y[0] = 0;
           ui->paint_area->graph(24)->setData(x,y);
           ui->paint_area->graph(25)->setData(x,y);
           ui->paint_area->graph(26)->setData(x,y);
           ui->paint_area->graph(27)->setData(x,y);
           ui->paint_area->graph(28)->setData(x,y);
           ui->paint_area->graph(29)->setData(x,y);

           ui->paint_area->replot();

  //         return; //可以添加坐标检查不通过的提示
       }

       if(count_point == 3)
       {
           QVector<double> x1(2),y1(2);
           QVector<double> x2(2),y2(2);
           QVector<double> x3(2),y3(2);

           x1[0] = Master_Boundary_Point_Disp[Communication_Machine][4][0][0]/10;y1[0] = Master_Boundary_Point_Disp[Communication_Machine][4][0][1]/10;
           x1[1] = Master_Boundary_Point_Disp[Communication_Machine][4][1][0]/10;y1[1] = Master_Boundary_Point_Disp[Communication_Machine][4][1][1]/10;

           x2[0] = Master_Boundary_Point_Disp[Communication_Machine][4][1][0]/10;y2[0] = Master_Boundary_Point_Disp[Communication_Machine][4][1][1]/10;
           x2[1] = Master_Boundary_Point_Disp[Communication_Machine][4][2][0]/10;y2[1] = Master_Boundary_Point_Disp[Communication_Machine][4][2][1]/10;

           x3[0] = Master_Boundary_Point_Disp[Communication_Machine][4][2][0]/10;y3[0] = Master_Boundary_Point_Disp[Communication_Machine][4][2][1]/10;
           x3[1] = Master_Boundary_Point_Disp[Communication_Machine][4][0][0]/10;y3[1] = Master_Boundary_Point_Disp[Communication_Machine][4][0][1]/10;


           ui->paint_area->graph(24)->setData(x1,y1);
           ui->paint_area->graph(25)->setData(x2,y2);
           ui->paint_area->graph(26)->setData(x3,y3);
       }

       if(count_point == 4)
       {
           QVector<double> x1(2),y1(2);
           QVector<double> x2(2),y2(2);
           QVector<double> x3(2),y3(2);
           QVector<double> x4(2),y4(2);
           x1[0] = Master_Boundary_Point_Disp[Communication_Machine][4][0][0]/10;y1[0] = Master_Boundary_Point_Disp[Communication_Machine][4][0][1]/10;
           x1[1] = Master_Boundary_Point_Disp[Communication_Machine][4][1][0]/10;y1[1] = Master_Boundary_Point_Disp[Communication_Machine][4][1][1]/10;

           x2[0] = Master_Boundary_Point_Disp[Communication_Machine][4][1][0]/10;y2[0] = Master_Boundary_Point_Disp[Communication_Machine][4][1][1]/10;
           x2[1] = Master_Boundary_Point_Disp[Communication_Machine][4][2][0]/10;y2[1] = Master_Boundary_Point_Disp[Communication_Machine][4][2][1]/10;

           x3[0] = Master_Boundary_Point_Disp[Communication_Machine][4][2][0]/10;y3[0] = Master_Boundary_Point_Disp[Communication_Machine][4][2][1]/10;
           x3[1] = Master_Boundary_Point_Disp[Communication_Machine][4][3][0]/10;y3[1] = Master_Boundary_Point_Disp[Communication_Machine][4][3][1]/10;

           x4[0] = Master_Boundary_Point_Disp[Communication_Machine][4][3][0]/10;y4[0] = Master_Boundary_Point_Disp[Communication_Machine][4][3][1]/10;
           x4[1] = Master_Boundary_Point_Disp[Communication_Machine][4][0][0]/10;y4[1] = Master_Boundary_Point_Disp[Communication_Machine][4][0][1]/10;

           ui->paint_area->graph(24)->setData(x1,y1);
           ui->paint_area->graph(25)->setData(x2,y2);
           ui->paint_area->graph(26)->setData(x3,y3);
           ui->paint_area->graph(27)->setData(x4,y4);
       }
       if(count_point == 5)
       {
           QVector<double> x1(2),y1(2);
           QVector<double> x2(2),y2(2);
           QVector<double> x3(2),y3(2);
           QVector<double> x4(2),y4(2);
           QVector<double> x5(2),y5(2);

           x1[0] = Master_Boundary_Point_Disp[Communication_Machine][4][0][0]/10;y1[0] = Master_Boundary_Point_Disp[Communication_Machine][4][0][1]/10;
           x1[1] = Master_Boundary_Point_Disp[Communication_Machine][4][1][0]/10;y1[1] = Master_Boundary_Point_Disp[Communication_Machine][4][1][1]/10;

           x2[0] = Master_Boundary_Point_Disp[Communication_Machine][4][1][0]/10;y2[0] = Master_Boundary_Point_Disp[Communication_Machine][4][1][1]/10;
           x2[1] = Master_Boundary_Point_Disp[Communication_Machine][4][2][0]/10;y2[1] = Master_Boundary_Point_Disp[Communication_Machine][4][2][1]/10;

           x3[0] = Master_Boundary_Point_Disp[Communication_Machine][4][2][0]/10;y3[0] = Master_Boundary_Point_Disp[Communication_Machine][4][2][1]/10;
           x3[1] = Master_Boundary_Point_Disp[Communication_Machine][4][3][0]/10;y3[1] = Master_Boundary_Point_Disp[Communication_Machine][4][3][1]/10;

           x4[0] = Master_Boundary_Point_Disp[Communication_Machine][4][3][0]/10;y4[0] = Master_Boundary_Point_Disp[Communication_Machine][4][3][1]/10;
           x4[1] = Master_Boundary_Point_Disp[Communication_Machine][4][4][0]/10;y4[1] = Master_Boundary_Point_Disp[Communication_Machine][4][4][1]/10;

           x5[0] = Master_Boundary_Point_Disp[Communication_Machine][4][4][0]/10;y5[0] = Master_Boundary_Point_Disp[Communication_Machine][4][4][1]/10;
           x5[1] = Master_Boundary_Point_Disp[Communication_Machine][4][0][0]/10;y5[1] = Master_Boundary_Point_Disp[Communication_Machine][4][0][1]/10;

           ui->paint_area->graph(24)->setData(x1,y1);
           ui->paint_area->graph(25)->setData(x2,y2);
           ui->paint_area->graph(26)->setData(x3,y3);
           ui->paint_area->graph(27)->setData(x4,y4);
           ui->paint_area->graph(28)->setData(x5,y5);
       }
       if(count_point == 6)
       {
           QVector<double> x1(2),y1(2);
           QVector<double> x2(2),y2(2);
           QVector<double> x3(2),y3(2);
           QVector<double> x4(2),y4(2);
           QVector<double> x5(2),y5(2);
           QVector<double> x6(2),y6(2);

           x1[0] = Master_Boundary_Point_Disp[Communication_Machine][4][0][0]/10;y1[0] = Master_Boundary_Point_Disp[Communication_Machine][4][0][1]/10;
           x1[1] = Master_Boundary_Point_Disp[Communication_Machine][4][1][0]/10;y1[1] = Master_Boundary_Point_Disp[Communication_Machine][4][1][1]/10;

           x2[0] = Master_Boundary_Point_Disp[Communication_Machine][4][1][0]/10;y2[0] = Master_Boundary_Point_Disp[Communication_Machine][4][1][1]/10;
           x2[1] = Master_Boundary_Point_Disp[Communication_Machine][4][2][0]/10;y2[1] = Master_Boundary_Point_Disp[Communication_Machine][4][2][1]/10;

           x3[0] = Master_Boundary_Point_Disp[Communication_Machine][4][2][0]/10;y3[0] = Master_Boundary_Point_Disp[Communication_Machine][4][2][1]/10;
           x3[1] = Master_Boundary_Point_Disp[Communication_Machine][4][3][0]/10;y3[1] = Master_Boundary_Point_Disp[Communication_Machine][4][3][1]/10;

           x4[0] = Master_Boundary_Point_Disp[Communication_Machine][4][3][0]/10;y4[0] = Master_Boundary_Point_Disp[Communication_Machine][4][3][1]/10;
           x4[1] = Master_Boundary_Point_Disp[Communication_Machine][4][4][0]/10;y4[1] = Master_Boundary_Point_Disp[Communication_Machine][4][4][1]/10;

           x5[0] = Master_Boundary_Point_Disp[Communication_Machine][4][4][0]/10;y5[0] = Master_Boundary_Point_Disp[Communication_Machine][4][4][1]/10;
           x5[1] = Master_Boundary_Point_Disp[Communication_Machine][4][5][0]/10;y5[1] = Master_Boundary_Point_Disp[Communication_Machine][4][5][1]/10;

           x6[0] = Master_Boundary_Point_Disp[Communication_Machine][4][5][0]/10;y6[0] = Master_Boundary_Point_Disp[Communication_Machine][4][5][1]/10;
           x6[1] = Master_Boundary_Point_Disp[Communication_Machine][4][0][0]/10;y6[1] = Master_Boundary_Point_Disp[Communication_Machine][4][0][1]/10;

           ui->paint_area->graph(24)->setData(x1,y1);
           ui->paint_area->graph(25)->setData(x2,y2);
           ui->paint_area->graph(26)->setData(x3,y3);
           ui->paint_area->graph(27)->setData(x4,y4);
           ui->paint_area->graph(28)->setData(x5,y5);
           ui->paint_area->graph(29)->setData(x6,y6);
       }

       ui->paint_area->replot();
 //      ui->widget->removePlottable(0);
   }

    {
       //判断有几个点，只要y有0的则不认
       unsigned char count_point = 0;
       for(uchar i = 0;i < 6;i++)
       {
           if(Master_Boundary_Point_Disp[Communication_Machine][5][i][1])
           {
               count_point ++;
           }
           else
           {
               break;
           }
       }
       if(count_point < 3)
       {
           printf("This Area Is Not Exist!\n");

           QVector<double> x(1),y(1);
           x[0] = 0;y[0] = 0;
           ui->paint_area->graph(30)->setData(x,y);
           ui->paint_area->graph(31)->setData(x,y);
           ui->paint_area->graph(32)->setData(x,y);
           ui->paint_area->graph(33)->setData(x,y);
           ui->paint_area->graph(34)->setData(x,y);
           ui->paint_area->graph(35)->setData(x,y);

           ui->paint_area->replot();

    //       return; //可以添加坐标检查不通过的提示
       }

       if(count_point == 3)
       {
           QVector<double> x1(2),y1(2);
           QVector<double> x2(2),y2(2);
           QVector<double> x3(2),y3(2);

           x1[0] = Master_Boundary_Point_Disp[Communication_Machine][5][0][0]/10;y1[0] = Master_Boundary_Point_Disp[Communication_Machine][5][0][1]/10;
           x1[1] = Master_Boundary_Point_Disp[Communication_Machine][5][1][0]/10;y1[1] = Master_Boundary_Point_Disp[Communication_Machine][5][1][1]/10;

           x2[0] = Master_Boundary_Point_Disp[Communication_Machine][5][1][0]/10;y2[0] = Master_Boundary_Point_Disp[Communication_Machine][5][1][1]/10;
           x2[1] = Master_Boundary_Point_Disp[Communication_Machine][5][2][0]/10;y2[1] = Master_Boundary_Point_Disp[Communication_Machine][5][2][1]/10;

           x3[0] = Master_Boundary_Point_Disp[Communication_Machine][5][2][0]/10;y3[0] = Master_Boundary_Point_Disp[Communication_Machine][5][2][1]/10;
           x3[1] = Master_Boundary_Point_Disp[Communication_Machine][5][0][0]/10;y3[1] = Master_Boundary_Point_Disp[Communication_Machine][5][0][1]/10;


           ui->paint_area->graph(30)->setData(x1,y1);
           ui->paint_area->graph(31)->setData(x2,y2);
           ui->paint_area->graph(32)->setData(x3,y3);
       }

       if(count_point == 4)
       {
           QVector<double> x1(2),y1(2);
           QVector<double> x2(2),y2(2);
           QVector<double> x3(2),y3(2);
           QVector<double> x4(2),y4(2);
           x1[0] = Master_Boundary_Point_Disp[Communication_Machine][5][0][0]/10;y1[0] = Master_Boundary_Point_Disp[Communication_Machine][5][0][1]/10;
           x1[1] = Master_Boundary_Point_Disp[Communication_Machine][5][1][0]/10;y1[1] = Master_Boundary_Point_Disp[Communication_Machine][5][1][1]/10;

           x2[0] = Master_Boundary_Point_Disp[Communication_Machine][5][1][0]/10;y2[0] = Master_Boundary_Point_Disp[Communication_Machine][5][1][1]/10;
           x2[1] = Master_Boundary_Point_Disp[Communication_Machine][5][2][0]/10;y2[1] = Master_Boundary_Point_Disp[Communication_Machine][5][2][1]/10;

           x3[0] = Master_Boundary_Point_Disp[Communication_Machine][5][2][0]/10;y3[0] = Master_Boundary_Point_Disp[Communication_Machine][5][2][1]/10;
           x3[1] = Master_Boundary_Point_Disp[Communication_Machine][5][3][0]/10;y3[1] = Master_Boundary_Point_Disp[Communication_Machine][5][3][1]/10;

           x4[0] = Master_Boundary_Point_Disp[Communication_Machine][5][3][0]/10;y4[0] = Master_Boundary_Point_Disp[Communication_Machine][5][3][1]/10;
           x4[1] = Master_Boundary_Point_Disp[Communication_Machine][5][0][0]/10;y4[1] = Master_Boundary_Point_Disp[Communication_Machine][5][0][1]/10;

           ui->paint_area->graph(30)->setData(x1,y1);
           ui->paint_area->graph(31)->setData(x2,y2);
           ui->paint_area->graph(32)->setData(x3,y3);
           ui->paint_area->graph(33)->setData(x4,y4);
       }
       if(count_point == 5)
       {
           QVector<double> x1(2),y1(2);
           QVector<double> x2(2),y2(2);
           QVector<double> x3(2),y3(2);
           QVector<double> x4(2),y4(2);
           QVector<double> x5(2),y5(2);

           x1[0] = Master_Boundary_Point_Disp[Communication_Machine][5][0][0]/10;y1[0] = Master_Boundary_Point_Disp[Communication_Machine][5][0][1]/10;
           x1[1] = Master_Boundary_Point_Disp[Communication_Machine][5][1][0]/10;y1[1] = Master_Boundary_Point_Disp[Communication_Machine][5][1][1]/10;

           x2[0] = Master_Boundary_Point_Disp[Communication_Machine][5][1][0]/10;y2[0] = Master_Boundary_Point_Disp[Communication_Machine][5][1][1]/10;
           x2[1] = Master_Boundary_Point_Disp[Communication_Machine][5][2][0]/10;y2[1] = Master_Boundary_Point_Disp[Communication_Machine][5][2][1]/10;

           x3[0] = Master_Boundary_Point_Disp[Communication_Machine][5][2][0]/10;y3[0] = Master_Boundary_Point_Disp[Communication_Machine][5][2][1]/10;
           x3[1] = Master_Boundary_Point_Disp[Communication_Machine][5][3][0]/10;y3[1] = Master_Boundary_Point_Disp[Communication_Machine][5][3][1]/10;

           x4[0] = Master_Boundary_Point_Disp[Communication_Machine][5][3][0]/10;y4[0] = Master_Boundary_Point_Disp[Communication_Machine][5][3][1]/10;
           x4[1] = Master_Boundary_Point_Disp[Communication_Machine][5][4][0]/10;y4[1] = Master_Boundary_Point_Disp[Communication_Machine][5][4][1]/10;

           x5[0] = Master_Boundary_Point_Disp[Communication_Machine][5][4][0]/10;y5[0] = Master_Boundary_Point_Disp[Communication_Machine][5][4][1]/10;
           x5[1] = Master_Boundary_Point_Disp[Communication_Machine][5][0][0]/10;y5[1] = Master_Boundary_Point_Disp[Communication_Machine][5][0][1]/10;

           ui->paint_area->graph(30)->setData(x1,y1);
           ui->paint_area->graph(31)->setData(x2,y2);
           ui->paint_area->graph(32)->setData(x3,y3);
           ui->paint_area->graph(33)->setData(x4,y4);
           ui->paint_area->graph(34)->setData(x5,y5);
       }
       if(count_point == 6)
       {
           QVector<double> x1(2),y1(2);
           QVector<double> x2(2),y2(2);
           QVector<double> x3(2),y3(2);
           QVector<double> x4(2),y4(2);
           QVector<double> x5(2),y5(2);
           QVector<double> x6(2),y6(2);

           x1[0] = Master_Boundary_Point_Disp[Communication_Machine][5][0][0]/10;y1[0] = Master_Boundary_Point_Disp[Communication_Machine][5][0][1]/10;
           x1[1] = Master_Boundary_Point_Disp[Communication_Machine][5][1][0]/10;y1[1] = Master_Boundary_Point_Disp[Communication_Machine][5][1][1]/10;

           x2[0] = Master_Boundary_Point_Disp[Communication_Machine][5][1][0]/10;y2[0] = Master_Boundary_Point_Disp[Communication_Machine][5][1][1]/10;
           x2[1] = Master_Boundary_Point_Disp[Communication_Machine][5][2][0]/10;y2[1] = Master_Boundary_Point_Disp[Communication_Machine][5][2][1]/10;

           x3[0] = Master_Boundary_Point_Disp[Communication_Machine][5][2][0]/10;y3[0] = Master_Boundary_Point_Disp[Communication_Machine][5][2][1]/10;
           x3[1] = Master_Boundary_Point_Disp[Communication_Machine][5][3][0]/10;y3[1] = Master_Boundary_Point_Disp[Communication_Machine][5][3][1]/10;

           x4[0] = Master_Boundary_Point_Disp[Communication_Machine][5][3][0]/10;y4[0] = Master_Boundary_Point_Disp[Communication_Machine][5][3][1]/10;
           x4[1] = Master_Boundary_Point_Disp[Communication_Machine][5][4][0]/10;y4[1] = Master_Boundary_Point_Disp[Communication_Machine][5][4][1]/10;

           x5[0] = Master_Boundary_Point_Disp[Communication_Machine][5][4][0]/10;y5[0] = Master_Boundary_Point_Disp[Communication_Machine][5][4][1]/10;
           x5[1] = Master_Boundary_Point_Disp[Communication_Machine][5][5][0]/10;y5[1] = Master_Boundary_Point_Disp[Communication_Machine][5][5][1]/10;

           x6[0] = Master_Boundary_Point_Disp[Communication_Machine][5][5][0]/10;y6[0] = Master_Boundary_Point_Disp[Communication_Machine][5][5][1]/10;
           x6[1] = Master_Boundary_Point_Disp[Communication_Machine][5][0][0]/10;y6[1] = Master_Boundary_Point_Disp[Communication_Machine][5][0][1]/10;

           ui->paint_area->graph(30)->setData(x1,y1);
           ui->paint_area->graph(31)->setData(x2,y2);
           ui->paint_area->graph(32)->setData(x3,y3);
           ui->paint_area->graph(33)->setData(x4,y4);
           ui->paint_area->graph(34)->setData(x5,y5);
           ui->paint_area->graph(35)->setData(x6,y6);
       }

       ui->paint_area->replot();
 //      ui->widget->removePlottable(0);
   }
}

void MainWindow::area_pointdrw()
{
    if(Flag_area_ctrl[0])
    {
        QVector<double> x1(6),y1(6);
        QPen pen_dot;
        pen_dot.setWidth(3);
        pen_dot.setColor(QColor(0,0,0));
        unsigned char flag_sound_temp[4][5] = {0};
        for(unsigned char i = 0;i < 5;i++)
        {
            x1[i] = Goal[Communication_Machine][i][0];
            y1[i] = Goal[Communication_Machine][i][1];
            //printf("%f ",x1[i]);
            //printf("%f ----------------------- \n",y1[i]);
            QString x_data = QString("%1").arg(x1[i]);
            QString y_data = QString("%1").arg(y1[i]);

                switch(i+1)
                {
                    case 1:     ui->label_1_x->setText(x_data);
                                ui->label_1_y->setText(y_data);
                                if((Alarm_Re_Flag[Communication_Machine] & 0x01))
                                {
                                    ui->label_1_x->setStyleSheet("color:red;");
                                    ui->label_1_y->setStyleSheet("color:red;");
                                    //入侵报警
                                    flag_sound_temp[0][0] = 1;
                                    Flag_warn_delay = Warn_delay_m*60+Warn_delay_s;

                                    if(Flag_Onlyonce_radar[0] == 0)
                                    {
                                        char point_warn[10] = {0};
                                        sprintf(point_warn,"%.1f,%.1f",Goal[Communication_Machine][i][0],Goal[Communication_Machine][i][1]);
                                        //history_radar_warn_write("1# 入侵报警 坐标:",point_warn);
                                        add_value_radarinfo("1# 入侵报警",point_warn);
                                        Flag_Onlyonce_radar[0] = 1;
                                    }
                                }
                                else
                                {
                                    if(Flag_Onlyonce_radar[0] == 1)
                                    {
                                        ui->label_1_x->setStyleSheet("color:black;");
                                        ui->label_1_y->setStyleSheet("color:black;");
                                        //history_radar_warn_write("1# 入侵报警解除","");
                                        add_value_radarinfo("1# 入侵报警解除","");
                                        Flag_Onlyonce_radar[0] = 0;
                                    }
                                    //入侵解除
                                }
                                break;
                    case 2:     ui->label_2_x->setText(x_data);
                                ui->label_2_y->setText(y_data);
                                if((Alarm_Re_Flag[Communication_Machine] & 0x02))
                                {
                                    ui->label_2_x->setStyleSheet("color:red;");
                                    ui->label_2_y->setStyleSheet("color:red;");
                                    //入侵报警
                                    flag_sound_temp[0][1] = 1;
                                    Flag_warn_delay = Warn_delay_m*60+Warn_delay_s;

                                    if(Flag_Onlyonce_radar[1] == 0)
                                    {
                                        char point_warn[10] = {0};
                                        sprintf(point_warn,"%.1f,%.1f",Goal[Communication_Machine][i][0],Goal[Communication_Machine][i][1]);
                                        //history_radar_warn_write("1# 入侵报警 坐标:",point_warn);
                                        add_value_radarinfo("1# 入侵报警",point_warn);
                                        Flag_Onlyonce_radar[1] = 1;
                                    }
                                }
                                else
                                {
                                    if(Flag_Onlyonce_radar[1] == 1)
                                    {
                                        ui->label_2_x->setStyleSheet("color:black;");
                                        ui->label_2_y->setStyleSheet("color:black;");
                                        //history_radar_warn_write("1# 入侵报警解除","");
                                        add_value_radarinfo("1# 入侵报警解除","");
                                        Flag_Onlyonce_radar[1] = 0;
                                    }
                                    //入侵解除
                                }
                                break;
                    case 3:     ui->label_3_x->setText(x_data);
                                ui->label_3_y->setText(y_data);
                                if((Alarm_Re_Flag[Communication_Machine] & 0x04))
                                {
                                    ui->label_3_x->setStyleSheet("color:red;");
                                    ui->label_3_y->setStyleSheet("color:red;");
                                    //入侵报警
                                    flag_sound_temp[0][2] = 1;
                                    Flag_warn_delay = Warn_delay_m*60+Warn_delay_s;

                                    if(Flag_Onlyonce_radar[2] == 0)
                                    {
                                        char point_warn[10] = {0};
                                        sprintf(point_warn,"%.1f,%.1f",Goal[Communication_Machine][i][0],Goal[Communication_Machine][i][1]);
                                        //history_radar_warn_write("1# 入侵报警 坐标:",point_warn);
                                        add_value_radarinfo("1# 入侵报警",point_warn);
                                        Flag_Onlyonce_radar[2] = 1;
                                    }
                                }
								else
                                {
                                    if(Flag_Onlyonce_radar[2] == 1)
                                    {
                                        ui->label_3_x->setStyleSheet("color:black;");
                                        ui->label_3_y->setStyleSheet("color:black;");
                                        //history_radar_warn_write("1# 入侵报警解除","");
                                        add_value_radarinfo("1# 入侵报警解除","");
                                        Flag_Onlyonce_radar[2] = 0;
                                    }
                                    //入侵解除
                                }
                                break;
                    case 4:     ui->label_4_x->setText(x_data);
                                ui->label_4_y->setText(y_data);
                                if((Alarm_Re_Flag[Communication_Machine] & 0x08))
                                {

                                    ui->label_4_x->setStyleSheet("color:red;");
                                    ui->label_4_y->setStyleSheet("color:red;");
                                    //入侵报警
                                    flag_sound_temp[0][3] = 1;
                                    Flag_warn_delay = Warn_delay_m*60+Warn_delay_s;

                                    if(Flag_Onlyonce_radar[3] == 0)
                                    {
                                        char point_warn[10] = {0};
                                        sprintf(point_warn,"%.1f,%.1f",Goal[Communication_Machine][i][0],Goal[Communication_Machine][i][1]);
                                        //history_radar_warn_write("1# 入侵报警 坐标:",point_warn);
                                        add_value_radarinfo("1# 入侵报警",point_warn);
                                        Flag_Onlyonce_radar[3] = 1;
                                    }
                                }
                                else
                                {
                                    if(Flag_Onlyonce_radar[3] == 1)
                                    {
                                        ui->label_4_x->setStyleSheet("color:black;");
                                        ui->label_4_y->setStyleSheet("color:black;");
                                        //history_radar_warn_write("1# 入侵报警解除","");
                                        add_value_radarinfo("1# 入侵报警解除","");
                                        Flag_Onlyonce_radar[3] = 0;
                                    }
                                    //入侵解除
                                }
                                break;
                    case 5:     ui->label_5_x->setText(x_data);
                                ui->label_5_y->setText(y_data);
                                if((Alarm_Re_Flag[Communication_Machine] & 0x10))
                                {
                                    ui->label_5_x->setStyleSheet("color:red;");
                                    ui->label_5_y->setStyleSheet("color:red;");
                                    //入侵报警
                                    flag_sound_temp[0][4] = 1;
                                    Flag_warn_delay = Warn_delay_m*60+Warn_delay_s;

                                    if(Flag_Onlyonce_radar[4] == 0)
                                    {
                                        char point_warn[10] = {0};
                                        sprintf(point_warn,"%.1f,%.1f",Goal[Communication_Machine][i][0],Goal[Communication_Machine][i][1]);
                                        //history_radar_warn_write("1# 入侵报警 坐标:",point_warn);
                                        add_value_radarinfo("1# 入侵报警",point_warn);
                                        Flag_Onlyonce_radar[4] = 1;
                                    }
                                }
                                else
                                {
                                    if(Flag_Onlyonce_radar[4] == 1)
                                    {
                                        ui->label_5_x->setStyleSheet("color:black;");
                                        ui->label_5_y->setStyleSheet("color:black;");
                                        //history_radar_warn_write("1# 入侵报警解除","");
                                        add_value_radarinfo("1# 入侵报警解除","");
                                        Flag_Onlyonce_radar[4] = 0;
                                    }
                                    //入侵解除
                                }
                                break;
                }
                if(flag_sound_temp[0][0]||flag_sound_temp[0][1]||flag_sound_temp[0][2]||flag_sound_temp[0][3]||flag_sound_temp[0][4])
                {
                    ui->label_radarstate->setStyleSheet("border-image: url(:/picture/radar_ruqin.png)");
                    Flag_Sound_Radar_temp = 1;
                }
                else
                {
                    Flag_Sound_Radar_temp = 0;
                    if(count_radar_uart <= 60)
                    {
                        ui->label_radarstate->setStyleSheet("border-image: url(:/picture/touming.png)");
                    }
                }
        }

        ui->paint_area->graph(36)->setPen(pen_dot);
        ui->paint_area->graph(36)->setLineStyle(QCPGraph::lsNone);
        ui->paint_area->graph(36)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle,3));
        ui->paint_area->graph(36)->setData(x1,y1);
        ui->paint_area->replot();
        if(Flag_autoget_area)
        {
            area_painted();
            config_boundary_machine1_area1();
            ui->toolButton_syssetshow->setHidden(1);
            emit systemset_show();
        }
    }
    else
    {
        Flag_Sound_Radar[1] = 0;
        ui->label_radarstate->setStyleSheet("border-image: url(:/picture/touming.png)");
        ui->paint_area->graph(36)->clearData();
        ui->paint_area->replot();
        ui->label_1_x->clear();
        ui->label_2_x->clear();
        ui->label_3_x->clear();
        ui->label_4_x->clear();
        ui->label_5_x->clear();
        ui->label_1_y->clear();
        ui->label_2_y->clear();
        ui->label_3_y->clear();
        ui->label_4_y->clear();
        ui->label_5_y->clear();
    }
}
void MainWindow::on_toolButton_radar1_clicked() //1号防区
{
    timer_drw->start();
    gif_radar->start();
}

void MainWindow::on_toolButton_syssetshow_clicked() //取消自动设置区域
{
    ui->toolButton_syssetshow->setHidden(1);
    emit systemset_show();
}

void MainWindow::key_syssetshow()
{
    ui->toolButton_syssetshow->setHidden(0);
}

void MainWindow::label_state_setted()
{
    ui->label_radarstate->setStyleSheet("border-image: url(:/picture/radar_tongxin.png)");
}

//************redar<-

//**************安全系统:人体静电
void MainWindow::set_label_jingdian(unsigned char whichbit, unsigned char value)        //人体静电显示
{
    QTextCodec *codec = QTextCodec::codecForName("UTF-8");

    switch(whichbit)    //0 一次  1正常  2异常
    {
        case 8: if(value)       //设备状态
                {
                    QString a = codec->toUnicode("设备开启");
                    ui->label_zhuagntai->setText(a);
                    if((Flag_Onlyonce_jingdian[8] == 0)||(Flag_Onlyonce_jingdian[8] == 2))
                    {
                        //history_jingdian_warn_write("设备开启");
                        //add_value_jingdianinfo("设备开启");
                        Flag_Onlyonce_jingdian[8] = 1;
                    }

                }
                else
                {
                    QString a = codec->toUnicode("设备关闭");
                    ui->label_zhuagntai->setText(a);
                    a = codec->toUnicode("空");
                    ui->label_r_chumo->setText(a);
                    ui->label_r_jiazi->setText(a);
                    ui->label_r_jiedi->setText(a);
                    ui->label_r_jiedi->setStyleSheet("color:black;");
                    ui->label_r_jingdian->setText(a);
                    ui->label_r_jingdian->setStyleSheet("color:black");
                    ui->label_r_lairen->setText(a);
                    ui->label_r_lairening->setText(a);
                    ui->label_r_shengyin->setText(a);

                    ui->label_tongxinguzhang->setHidden(1);
                    if((Flag_Onlyonce_jingdian[8] == 0)||(Flag_Onlyonce_jingdian[8] == 1))
                    {
                        //history_jingdian_warn_write("设备关闭");
                        //add_value_jingdianinfo("设备关闭");
                        Flag_Onlyonce_jingdian[8] = 2;
                    }

                }
                break;
        case 7: //通信故障
                if(value)
                {
                    ui->label_tongxinguzhang->setHidden(0);
                    QString a = codec->toUnicode("空");
                    ui->label_r_chumo->setText(a);
                    ui->label_r_jiazi->setText(a);
                    ui->label_r_jiedi->setText(a);
                    ui->label_r_jiedi->setStyleSheet("color:black;");
                    ui->label_r_jingdian->setText(a);
                    ui->label_r_jingdian->setStyleSheet("color:black");
                    ui->label_r_lairen->setText(a);
                    ui->label_r_lairening->setText(a);
                    ui->label_r_shengyin->setText(a);

                    if((Flag_Onlyonce_jingdian[7] == 0)||(Flag_Onlyonce_jingdian[7] == 1))
                    {
                        //history_jingdian_warn_write("通信故障");
                        add_value_jingdianinfo("通信故障");
                        Flag_Onlyonce_jingdian[7] = 2;
                    }
                }
                else
                {
                    ui->label_tongxinguzhang->setHidden(1);
                    if((Flag_Onlyonce_jingdian[7] == 0)||(Flag_Onlyonce_jingdian[7] == 2))
                    {
                        //history_jingdian_warn_write("通信正常");
                        add_value_jingdianinfo("通信正常");
                        Flag_Onlyonce_jingdian[7] = 1;
                    }
                }
                break;
        case 6: //printf("bit 0 %d\n",value);     //语音
                if(value == 2)
                {
                    QString a = codec->toUnicode("语音");
                    ui->label_r_shengyin->setText(a);
                }
                if(value == 1)
                {
                    QString a = codec->toUnicode("和弦");
                    ui->label_r_shengyin->setText(a);
                }
                if(value == 0)
                {
                    QString a = codec->toUnicode("静音");
                    ui->label_r_shengyin->setText(a);
                }
                break;
        case 5:// printf("bit 1 %d\n",value);     //来人状态
                if(value)
                {
					QString a = codec->toUnicode("来人");
                    ui->label_r_lairening->setText(a);
                }
                else
                {
                    QString a = codec->toUnicode("空闲");
                    ui->label_r_lairening->setText(a);
                }
                break;
        case 4: //printf("bit 2 %d\n",value); //来人检测开启或者关闭
                if(value)
                {
                    QString a = codec->toUnicode("开启");
                    ui->label_r_lairen->setText(a);
             //       ui->label_r_lairen->setStyleSheet("color:red;");
                }
                else
                {
                    QString a = codec->toUnicode("关闭");
                    ui->label_r_lairen->setText(a);
                }
                break;
        case 3: //printf("bit 3 %d\n",value);     //夹子状态
                if(value)
                {
                    QString a = codec->toUnicode("使用中");
                    ui->label_r_jiazi->setText(a);
                    if((Flag_Onlyonce_jingdian[3] == 0)||(Flag_Onlyonce_jingdian[3] == 1))
                    {
                        //history_jingdian_warn_write("夹子使用中");
                        add_value_jingdianinfo("夹子使用中");
                        Flag_Onlyonce_jingdian[3] = 2;
                    }

                }
                else
                {
                    QString a = codec->toUnicode("空闲");
                    ui->label_r_jiazi->setText(a);
                    if((Flag_Onlyonce_jingdian[3] == 0)||(Flag_Onlyonce_jingdian[3] == 2))
                    {
                        //history_jingdian_warn_write("夹子归位");
                        add_value_jingdianinfo("夹子归位");
                        Flag_Onlyonce_jingdian[3] = 1;
                    }

                }
                break;
        case 2:// printf("bit 4 %d\n",value);     //接地状态
                if(value)
                {
                    QString a = codec->toUnicode("报警");
                    ui->label_r_jiedi->setText(a);
                    ui->label_r_jiedi->setStyleSheet("color:red;");
                    if((Flag_Onlyonce_jingdian[2] == 0)||(Flag_Onlyonce_jingdian[2] == 1))
                    {
                        //history_jingdian_warn_write("接地报警");
                        add_value_jingdianinfo("接地报警");
                        Flag_Onlyonce_jingdian[2] = 2;
                    }

                }
                else
                {
                    QString a = codec->toUnicode("正常");
                    ui->label_r_jiedi->setText(a);
                    ui->label_r_jiedi->setStyleSheet("color:black;");
                    if((Flag_Onlyonce_jingdian[2] == 0)||(Flag_Onlyonce_jingdian[2] == 2))
                    {
                        //history_jingdian_warn_write("接地正常");
                        add_value_jingdianinfo("接地正常");
                        Flag_Onlyonce_jingdian[2] = 1;
                    }

                }
                break;
        case 1:// printf("bit 5 %d\n",value);     //静电报警状态
                if((value == 0) || (value == 2))
                {
                    QString a = codec->toUnicode("空闲");

                    ui->label_r_chumo->setText(a);
                    a = codec->toUnicode("无");
                    ui->label_r_jingdian->setText(a);
                    ui->label_r_jingdian->setStyleSheet("color:black;");

                }
                else if(value == 1)
                    {
                        QString a = codec->toUnicode("触摸中");
                        ui->label_r_chumo->setText(a);
                        a = codec->toUnicode("安全");
                        ui->label_r_jingdian->setText(a);
                        ui->label_r_jingdian->setStyleSheet("color:black;");
                        if(Flag_Onlyonce_jingdian[1] == 0)
                        {
                            //history_jingdian_warn_write("触摸静电安全");
                            add_value_jingdianinfo("触摸静电安全");
                            Flag_Onlyonce_jingdian[1]++;
                            if(Flag_Onlyonce_jingdian[1] > 4)
                            {
                                Flag_Onlyonce_jingdian[1] = 0;
                            }
                        }
                    }
                    else if(value == 3)
                    {
                        QString a = codec->toUnicode("触摸中");
                        ui->label_r_chumo->setText(a);
                        a = codec->toUnicode("报警");
                        ui->label_r_jingdian->setText(a);
                        ui->label_r_jingdian->setStyleSheet("color:red;");
                        if(Flag_Onlyonce_jingdian[1] == 0)
                        {
                            //history_jingdian_warn_write("触摸静电报警");
                            add_value_jingdianinfo("触摸静电报警");
                            Flag_Onlyonce_jingdian[1]++;
                            if(Flag_Onlyonce_jingdian[1] > 4)
                            {
                                Flag_Onlyonce_jingdian[1] = 0;
                            }
                        }
                    }
                break;
        case 0:
                break;
    }
}


void MainWindow::Press_number()     //将实际的气压值送到显示
{
    QString n ;
    n = "-----";
    //调试模式
    if(Flag_pre_mode ==1)
    {
        if(count_Pressure[0] > 0 || count_Pressure[0] < -100) //错误数据，显示---
        {
            ui->label_p1->setText(n +"KPa");
        }
        else
        {
            ui->label_p1->setText(QString::number(count_Pressure[0],'f',2).append("KPa"));
        }
        if(count_Pressure[1] > 0 || count_Pressure[1] < -100) //错误数据，显示---
        {
            ui->label_p2->setText(n +"KPa");
        }
        else
        {
            ui->label_p2->setText(QString::number(count_Pressure[1],'f',2).append("KPa"));
        }
        if(count_Pressure[2] > 0 || count_Pressure[2] < -100) //错误数据，显示---
        {
            ui->label_p3->setText(n+"KPa");
        }
        else
        {
            ui->label_p3->setText(QString::number(count_Pressure[2],'f',2).append("KPa"));
        }
        if(count_Pressure[3] > 0 || count_Pressure[3] < -100) //错误数据，显示---
        {
            ui->label_p4->setText(n+"KPa");
        }
        else
        {
            ui->label_p4->setText(QString::number(count_Pressure[3],'f',2).append("KPa"));
        }
        if(count_Pressure[4] > 0 || count_Pressure[4] < -100) //错误数据，显示---
        {
            ui->label_p5->setText(n+"KPa");
        }
        else
        {
            ui->label_p5->setText(QString::number(count_Pressure[4],'f',2).append("KPa"));
        }
        if(count_Pressure[5] > 0 || count_Pressure[5] < -100) //错误数据，显示---
        {
            ui->label_p6->setText(n+"KPa");
        }
        else
        {
            ui->label_p6->setText(QString::number(count_Pressure[5],'f',2).append("KPa"));
        }
        if(count_Pressure[6] > 0 || count_Pressure[6] < -100) //错误数据，显示---
        {
            ui->label_p7->setText(n+"KPa");
        }
        else
        {
            ui->label_p7->setText(QString::number(count_Pressure[6],'f',2).append("KPa"));
        }
        if(count_Pressure[7] > 0 || count_Pressure[7] < -100) //错误数据，显示---
        {
            ui->label_p8->setText(n+"KPa");
        }
        else
        {
            ui->label_p8->setText(QString::number(count_Pressure[7],'f',2).append("KPa"));
        }
    }
    //正常模式
    if(Flag_pre_mode == 0)
    {
        if(count_Pressure[0] > 0 || count_Pressure[0] < -100) //错误数据，显示---
        {
            ui->label_p1->setText( n +"KPa");
        }
        else
        {
            ui->label_p1->setText(QString::number(count_Pressure[0],'f',0).append("KPa"));
        }
        if(count_Pressure[1] > 0 || count_Pressure[1] < -100) //错误数据，显示---
        {
            ui->label_p2->setText(n +"KPa");
        }
        else
        {
            ui->label_p2->setText(QString::number(count_Pressure[1],'f',0).append("KPa"));
        }
        if(count_Pressure[2] > 0 || count_Pressure[2] < -100) //错误数据，显示---
        {
            ui->label_p3->setText(n+"KPa");
        }
        else
        {
            ui->label_p3->setText(QString::number(count_Pressure[2],'f',0).append("KPa"));
        }
        if(count_Pressure[3] > 0 || count_Pressure[3] < -100) //错误数据，显示---
        {
            ui->label_p4->setText(n+"KPa");
        }
        else
        {
            ui->label_p4->setText(QString::number(count_Pressure[3],'f',0).append("KPa"));
        }
        if(count_Pressure[4] > 0 || count_Pressure[4] < -100) //错误数据，显示---
        {
            ui->label_p5->setText(n+"KPa");
        }
        else
        {
            ui->label_p5->setText(QString::number(count_Pressure[4],'f',0).append("KPa"));
        }
        if(count_Pressure[5] > 0 || count_Pressure[5] < -100) //错误数据，显示---
        {
            ui->label_p6->setText(n+"KPa");
        }
        else
        {
            ui->label_p6->setText(QString::number(count_Pressure[5],'f',0).append("KPa"));
        }
        if(count_Pressure[6] > 0 || count_Pressure[6] < -100) //错误数据，显示---
        {
            ui->label_p7->setText(n+"KPa");
        }
        else
        {
            ui->label_p7->setText(QString::number(count_Pressure[6],'f',0).append("KPa"));
        }
        if(count_Pressure[7] > 0 || count_Pressure[7] < -100) //错误数据，显示---
        {
            ui->label_p8->setText(n+"KPa");
        }
        else
        {
            ui->label_p8->setText(QString::number(count_Pressure[7],'f',0).append("KPa"));
        }
    }
}

//油气回收
//加油机个数重绘
void MainWindow::set_amount_oilgas_dispen(int t)
{
    switch(t)
    {
        case 0:
                ui->widget_dispen_1->setHidden(1);
                ui->widget_dispen_2->setHidden(1);
                ui->widget_dispen_3->setHidden(1);
                ui->widget_dispen_4->setHidden(1);
                ui->widget_dispen_5->setHidden(1);
                ui->widget_dispen_6->setHidden(1);
                ui->widget_dispen_7->setHidden(1);
                ui->widget_dispen_8->setHidden(1);
                ui->widget_dispen_9->setHidden(1);
                ui->widget_dispen_10->setHidden(1);
                ui->widget_dispen_11->setHidden(1);
                ui->widget_dispen_12->setHidden(1);
                break;
        case 1:
                ui->widget_dispen_1->setHidden(0);
                ui->widget_dispen_2->setHidden(1);
                ui->widget_dispen_3->setHidden(1);
                ui->widget_dispen_4->setHidden(1);
                ui->widget_dispen_5->setHidden(1);
                ui->widget_dispen_6->setHidden(1);
                ui->widget_dispen_7->setHidden(1);
                ui->widget_dispen_8->setHidden(1);
                ui->widget_dispen_9->setHidden(1);
                ui->widget_dispen_10->setHidden(1);
                ui->widget_dispen_11->setHidden(1);
                ui->widget_dispen_12->setHidden(1);
                break;
        case 2:
                ui->widget_dispen_1->setHidden(0);
                ui->widget_dispen_2->setHidden(0);
                ui->widget_dispen_3->setHidden(1);
                ui->widget_dispen_4->setHidden(1);
                ui->widget_dispen_5->setHidden(1);
                ui->widget_dispen_6->setHidden(1);
                ui->widget_dispen_7->setHidden(1);
                ui->widget_dispen_8->setHidden(1);
                ui->widget_dispen_9->setHidden(1);
                ui->widget_dispen_10->setHidden(1);
                ui->widget_dispen_11->setHidden(1);
                ui->widget_dispen_12->setHidden(1);
                break;
        case 3:
                ui->widget_dispen_1->setHidden(0);
                ui->widget_dispen_2->setHidden(0);
                ui->widget_dispen_3->setHidden(0);
                ui->widget_dispen_4->setHidden(1);
                ui->widget_dispen_5->setHidden(1);
                ui->widget_dispen_6->setHidden(1);
                ui->widget_dispen_7->setHidden(1);
                ui->widget_dispen_8->setHidden(1);
                ui->widget_dispen_9->setHidden(1);
                ui->widget_dispen_10->setHidden(1);
                ui->widget_dispen_11->setHidden(1);
                ui->widget_dispen_12->setHidden(1);
                break;
        case 4:
                ui->widget_dispen_1->setHidden(0);
                ui->widget_dispen_2->setHidden(0);
                ui->widget_dispen_3->setHidden(0);
                ui->widget_dispen_4->setHidden(0);
                ui->widget_dispen_5->setHidden(1);
                ui->widget_dispen_6->setHidden(1);
                ui->widget_dispen_7->setHidden(1);
                ui->widget_dispen_8->setHidden(1);
                ui->widget_dispen_9->setHidden(1);
                ui->widget_dispen_10->setHidden(1);
                ui->widget_dispen_11->setHidden(1);
                ui->widget_dispen_12->setHidden(1);
                break;
        case 5:
                ui->widget_dispen_1->setHidden(0);
                ui->widget_dispen_2->setHidden(0);
                ui->widget_dispen_3->setHidden(0);
                ui->widget_dispen_4->setHidden(0);
                ui->widget_dispen_5->setHidden(0);
                ui->widget_dispen_6->setHidden(1);
                ui->widget_dispen_7->setHidden(1);
                ui->widget_dispen_8->setHidden(1);
                ui->widget_dispen_9->setHidden(1);
                ui->widget_dispen_10->setHidden(1);
                ui->widget_dispen_11->setHidden(1);
                ui->widget_dispen_12->setHidden(1);
                break;
        case 6:
                ui->widget_dispen_1->setHidden(0);
                ui->widget_dispen_2->setHidden(0);
                ui->widget_dispen_3->setHidden(0);
                ui->widget_dispen_4->setHidden(0);
                ui->widget_dispen_5->setHidden(0);
                ui->widget_dispen_6->setHidden(0);
                ui->widget_dispen_7->setHidden(1);
                ui->widget_dispen_8->setHidden(1);
                ui->widget_dispen_9->setHidden(1);
                ui->widget_dispen_10->setHidden(1);
                ui->widget_dispen_11->setHidden(1);
                ui->widget_dispen_12->setHidden(1);
                break;
        case 7:
                ui->widget_dispen_1->setHidden(0);
                ui->widget_dispen_2->setHidden(0);
                ui->widget_dispen_3->setHidden(0);
                ui->widget_dispen_4->setHidden(0);
                ui->widget_dispen_5->setHidden(0);
                ui->widget_dispen_6->setHidden(0);
                ui->widget_dispen_7->setHidden(0);
                ui->widget_dispen_8->setHidden(1);
                ui->widget_dispen_9->setHidden(1);
                ui->widget_dispen_10->setHidden(1);
                ui->widget_dispen_11->setHidden(1);
                ui->widget_dispen_12->setHidden(1);
                break;
        case 8:
                ui->widget_dispen_1->setHidden(0);
                ui->widget_dispen_2->setHidden(0);
                ui->widget_dispen_3->setHidden(0);
                ui->widget_dispen_4->setHidden(0);
                ui->widget_dispen_5->setHidden(0);
                ui->widget_dispen_6->setHidden(0);
                ui->widget_dispen_7->setHidden(0);
                ui->widget_dispen_8->setHidden(0);
                ui->widget_dispen_9->setHidden(1);
                ui->widget_dispen_10->setHidden(1);
                ui->widget_dispen_11->setHidden(1);
                ui->widget_dispen_12->setHidden(1);
                break;
        case 9:
                ui->widget_dispen_1->setHidden(0);
                ui->widget_dispen_2->setHidden(0);
                ui->widget_dispen_3->setHidden(0);
                ui->widget_dispen_4->setHidden(0);
                ui->widget_dispen_5->setHidden(0);
                ui->widget_dispen_6->setHidden(0);
                ui->widget_dispen_7->setHidden(0);
                ui->widget_dispen_8->setHidden(0);
                ui->widget_dispen_9->setHidden(0);
                ui->widget_dispen_10->setHidden(1);
                ui->widget_dispen_11->setHidden(1);
                ui->widget_dispen_12->setHidden(1);
                break;
        case 10:
                ui->widget_dispen_1->setHidden(0);
                ui->widget_dispen_2->setHidden(0);
                ui->widget_dispen_3->setHidden(0);
                ui->widget_dispen_4->setHidden(0);
                ui->widget_dispen_5->setHidden(0);
                ui->widget_dispen_6->setHidden(0);
                ui->widget_dispen_7->setHidden(0);
                ui->widget_dispen_8->setHidden(0);
                ui->widget_dispen_9->setHidden(0);
                ui->widget_dispen_10->setHidden(0);
                ui->widget_dispen_11->setHidden(1);
                ui->widget_dispen_12->setHidden(1);
                break;
        case 11:
                ui->widget_dispen_1->setHidden(0);
                ui->widget_dispen_2->setHidden(0);
                ui->widget_dispen_3->setHidden(0);
                ui->widget_dispen_4->setHidden(0);
                ui->widget_dispen_5->setHidden(0);
                ui->widget_dispen_6->setHidden(0);
                ui->widget_dispen_7->setHidden(0);
                ui->widget_dispen_8->setHidden(0);
                ui->widget_dispen_9->setHidden(0);
                ui->widget_dispen_10->setHidden(0);
                ui->widget_dispen_11->setHidden(0);
                ui->widget_dispen_12->setHidden(1);
                break;
        case 12:
                ui->widget_dispen_1->setHidden(0);
                ui->widget_dispen_2->setHidden(0);
                ui->widget_dispen_3->setHidden(0);
                ui->widget_dispen_4->setHidden(0);
                ui->widget_dispen_5->setHidden(0);
                ui->widget_dispen_6->setHidden(0);
                ui->widget_dispen_7->setHidden(0);
                ui->widget_dispen_8->setHidden(0);
                ui->widget_dispen_9->setHidden(0);
                ui->widget_dispen_10->setHidden(0);
                ui->widget_dispen_11->setHidden(0);
                ui->widget_dispen_12->setHidden(0);
                break;
    }
}
//油枪重绘
void MainWindow::set_amount_oilgas_gun()
{
    unsigned char flag_enable[12][8] = {0};
    unsigned char flag_enable_map[12][8] = {0};
    for(unsigned char i = 0;i < Amount_Dispener;i++)
    {
        for(unsigned char j = 0;j < 8;j++)
        {
            if(Amount_Gasgun[i] > 0)
            {
                if(Amount_Gasgun[i] > j)
                {
                   flag_enable[i][j] = 1;
				   //if(Mapping_Show[i*8+j] != 0)
                   //{
                       flag_enable_map[i][j] = 1;
                   //}
                }
            }

        }
    }
    ui->label_gun_1_1->setVisible(flag_enable[0][0]);
    ui->label_gun_1_1u->setVisible(flag_enable[0][0]);
    ui->label_gun_1_2->setVisible(flag_enable[0][1]);
    ui->label_gun_1_2u->setVisible(flag_enable[0][1]);
    ui->label_gun_1_3->setVisible(flag_enable[0][2]);
    ui->label_gun_1_3u->setVisible(flag_enable[0][2]);
    ui->label_gun_1_4->setVisible(flag_enable[0][3]);
    ui->label_gun_1_4u->setVisible(flag_enable[0][3]);
    ui->label_gun_1_5->setVisible(flag_enable[0][4]);
    ui->label_gun_1_5u->setVisible(flag_enable[0][4]);
    ui->label_gun_1_6->setVisible(flag_enable[0][5]);
    ui->label_gun_1_6u->setVisible(flag_enable[0][5]);
    ui->label_gun_1_7->setVisible(flag_enable[0][6]);
    ui->label_gun_1_7u->setVisible(flag_enable[0][6]);
    ui->label_gun_1_8->setVisible(flag_enable[0][7]);
    ui->label_gun_1_8u->setVisible(flag_enable[0][7]);

    ui->label_gun_2_1->setVisible(flag_enable[1][0]);
    ui->label_gun_2_1u->setVisible(flag_enable[1][0]);
    ui->label_gun_2_2->setVisible(flag_enable[1][1]);
    ui->label_gun_2_2u->setVisible(flag_enable[1][1]);
    ui->label_gun_2_3->setVisible(flag_enable[1][2]);
    ui->label_gun_2_3u->setVisible(flag_enable[1][2]);
    ui->label_gun_2_4->setVisible(flag_enable[1][3]);
    ui->label_gun_2_4u->setVisible(flag_enable[1][3]);
    ui->label_gun_2_5->setVisible(flag_enable[1][4]);
    ui->label_gun_2_5u->setVisible(flag_enable[1][4]);
    ui->label_gun_2_6->setVisible(flag_enable[1][5]);
    ui->label_gun_2_6u->setVisible(flag_enable[1][5]);
    ui->label_gun_2_7->setVisible(flag_enable[1][6]);
    ui->label_gun_2_7u->setVisible(flag_enable[1][6]);
    ui->label_gun_2_8->setVisible(flag_enable[1][7]);
    ui->label_gun_2_8u->setVisible(flag_enable[1][7]);

    ui->label_gun_3_1->setVisible(flag_enable[2][0]);
    ui->label_gun_3_1u->setVisible(flag_enable[2][0]);
    ui->label_gun_3_2->setVisible(flag_enable[2][1]);
    ui->label_gun_3_2u->setVisible(flag_enable[2][1]);
    ui->label_gun_3_3->setVisible(flag_enable[2][2]);
    ui->label_gun_3_3u->setVisible(flag_enable[2][2]);
    ui->label_gun_3_4->setVisible(flag_enable[2][3]);
    ui->label_gun_3_4u->setVisible(flag_enable[2][3]);
    ui->label_gun_3_5->setVisible(flag_enable[2][4]);
    ui->label_gun_3_5u->setVisible(flag_enable[2][4]);
    ui->label_gun_3_6->setVisible(flag_enable[2][5]);
    ui->label_gun_3_6u->setVisible(flag_enable[2][5]);
    ui->label_gun_3_7->setVisible(flag_enable[2][6]);
    ui->label_gun_3_7u->setVisible(flag_enable[2][6]);
    ui->label_gun_3_8->setVisible(flag_enable[2][7]);
    ui->label_gun_3_8u->setVisible(flag_enable[2][7]);

    ui->label_gun_4_1->setVisible(flag_enable[3][0]);
    ui->label_gun_4_1u->setVisible(flag_enable[3][0]);
    ui->label_gun_4_2->setVisible(flag_enable[3][1]);
    ui->label_gun_4_2u->setVisible(flag_enable[3][1]);
    ui->label_gun_4_3->setVisible(flag_enable[3][2]);
    ui->label_gun_4_3u->setVisible(flag_enable[3][2]);
    ui->label_gun_4_4->setVisible(flag_enable[3][3]);
    ui->label_gun_4_4u->setVisible(flag_enable[3][3]);
    ui->label_gun_4_5->setVisible(flag_enable[3][4]);
    ui->label_gun_4_5u->setVisible(flag_enable[3][4]);
    ui->label_gun_4_6->setVisible(flag_enable[3][5]);
    ui->label_gun_4_6u->setVisible(flag_enable[3][5]);
    ui->label_gun_4_7->setVisible(flag_enable[3][6]);
    ui->label_gun_4_7u->setVisible(flag_enable[3][6]);
    ui->label_gun_4_8->setVisible(flag_enable[3][7]);
    ui->label_gun_4_8u->setVisible(flag_enable[3][7]);

    ui->label_gun_5_1->setVisible(flag_enable[4][0]);
    ui->label_gun_5_1u->setVisible(flag_enable[4][0]);
    ui->label_gun_5_2->setVisible(flag_enable[4][1]);
    ui->label_gun_5_2u->setVisible(flag_enable[4][1]);
    ui->label_gun_5_3->setVisible(flag_enable[4][2]);
    ui->label_gun_5_3u->setVisible(flag_enable[4][2]);
    ui->label_gun_5_4->setVisible(flag_enable[4][3]);
    ui->label_gun_5_4u->setVisible(flag_enable[4][3]);
    ui->label_gun_5_5->setVisible(flag_enable[4][4]);
    ui->label_gun_5_5u->setVisible(flag_enable[4][4]);
    ui->label_gun_5_6->setVisible(flag_enable[4][5]);
    ui->label_gun_5_6u->setVisible(flag_enable[4][5]);
    ui->label_gun_5_7->setVisible(flag_enable[4][6]);
    ui->label_gun_5_7u->setVisible(flag_enable[4][6]);
    ui->label_gun_5_8->setVisible(flag_enable[4][7]);
    ui->label_gun_5_8u->setVisible(flag_enable[4][7]);

    ui->label_gun_6_1->setVisible(flag_enable[5][0]);
    ui->label_gun_6_1u->setVisible(flag_enable[5][0]);
    ui->label_gun_6_2->setVisible(flag_enable[5][1]);
    ui->label_gun_6_2u->setVisible(flag_enable[5][1]);
    ui->label_gun_6_3->setVisible(flag_enable[5][2]);
    ui->label_gun_6_3u->setVisible(flag_enable[5][2]);
    ui->label_gun_6_4->setVisible(flag_enable[5][3]);
    ui->label_gun_6_4u->setVisible(flag_enable[5][3]);
    ui->label_gun_6_5->setVisible(flag_enable[5][4]);
    ui->label_gun_6_5u->setVisible(flag_enable[5][4]);
    ui->label_gun_6_6->setVisible(flag_enable[5][5]);
    ui->label_gun_6_6u->setVisible(flag_enable[5][5]);
    ui->label_gun_6_7->setVisible(flag_enable[5][6]);
    ui->label_gun_6_7u->setVisible(flag_enable[5][6]);
    ui->label_gun_6_8->setVisible(flag_enable[5][7]);
    ui->label_gun_6_8u->setVisible(flag_enable[5][7]);

    ui->label_gun_7_1->setVisible(flag_enable[6][0]);
    ui->label_gun_7_1u->setVisible(flag_enable[6][0]);
    ui->label_gun_7_2->setVisible(flag_enable[6][1]);
    ui->label_gun_7_2u->setVisible(flag_enable[6][1]);
    ui->label_gun_7_3->setVisible(flag_enable[6][2]);
    ui->label_gun_7_3u->setVisible(flag_enable[6][2]);
    ui->label_gun_7_4->setVisible(flag_enable[6][3]);
    ui->label_gun_7_4u->setVisible(flag_enable[6][3]);
    ui->label_gun_7_5->setVisible(flag_enable[6][4]);
    ui->label_gun_7_5u->setVisible(flag_enable[6][4]);
    ui->label_gun_7_6->setVisible(flag_enable[6][5]);
    ui->label_gun_7_6u->setVisible(flag_enable[6][5]);
    ui->label_gun_7_7->setVisible(flag_enable[6][6]);
    ui->label_gun_7_7u->setVisible(flag_enable[6][6]);
    ui->label_gun_7_8->setVisible(flag_enable[6][7]);
    ui->label_gun_7_8u->setVisible(flag_enable[6][7]);

    ui->label_gun_8_1->setVisible(flag_enable[7][0]);
    ui->label_gun_8_1u->setVisible(flag_enable[7][0]);
    ui->label_gun_8_2->setVisible(flag_enable[7][1]);
    ui->label_gun_8_2u->setVisible(flag_enable[7][1]);
    ui->label_gun_8_3->setVisible(flag_enable[7][2]);
    ui->label_gun_8_3u->setVisible(flag_enable[7][2]);
    ui->label_gun_8_4->setVisible(flag_enable[7][3]);
    ui->label_gun_8_4u->setVisible(flag_enable[7][3]);
    ui->label_gun_8_5->setVisible(flag_enable[7][4]);
    ui->label_gun_8_5u->setVisible(flag_enable[7][4]);
    ui->label_gun_8_6->setVisible(flag_enable[7][5]);
    ui->label_gun_8_6u->setVisible(flag_enable[7][5]);
    ui->label_gun_8_7->setVisible(flag_enable[7][6]);
    ui->label_gun_8_7u->setVisible(flag_enable[7][6]);
    ui->label_gun_8_8->setVisible(flag_enable[7][7]);
    ui->label_gun_8_8u->setVisible(flag_enable[7][7]);

    ui->label_gun_9_1->setVisible(flag_enable[8][0]);
    ui->label_gun_9_1u->setVisible(flag_enable[8][0]);
    ui->label_gun_9_2->setVisible(flag_enable[8][1]);
    ui->label_gun_9_2u->setVisible(flag_enable[8][1]);
    ui->label_gun_9_3->setVisible(flag_enable[8][2]);
    ui->label_gun_9_3u->setVisible(flag_enable[8][2]);
    ui->label_gun_9_4->setVisible(flag_enable[8][3]);
    ui->label_gun_9_4u->setVisible(flag_enable[8][3]);
    ui->label_gun_9_5->setVisible(flag_enable[8][4]);
    ui->label_gun_9_5u->setVisible(flag_enable[8][4]);
    ui->label_gun_9_6->setVisible(flag_enable[8][5]);
    ui->label_gun_9_6u->setVisible(flag_enable[8][5]);
    ui->label_gun_9_7->setVisible(flag_enable[8][6]);
    ui->label_gun_9_7u->setVisible(flag_enable[8][6]);
    ui->label_gun_9_8->setVisible(flag_enable[8][7]);
    ui->label_gun_9_8u->setVisible(flag_enable[8][7]);

    ui->label_gun_10_1->setVisible(flag_enable[9][0]);
    ui->label_gun_10_1u->setVisible(flag_enable[9][0]);
    ui->label_gun_10_2->setVisible(flag_enable[9][1]);
    ui->label_gun_10_2u->setVisible(flag_enable[9][1]);
    ui->label_gun_10_3->setVisible(flag_enable[9][2]);
    ui->label_gun_10_3u->setVisible(flag_enable[9][2]);
    ui->label_gun_10_4->setVisible(flag_enable[9][3]);
    ui->label_gun_10_4u->setVisible(flag_enable[9][3]);
    ui->label_gun_10_5->setVisible(flag_enable[9][4]);
    ui->label_gun_10_5u->setVisible(flag_enable[9][4]);
    ui->label_gun_10_6->setVisible(flag_enable[9][5]);
    ui->label_gun_10_6u->setVisible(flag_enable[9][5]);
    ui->label_gun_10_7->setVisible(flag_enable[9][6]);
    ui->label_gun_10_7u->setVisible(flag_enable[9][6]);
    ui->label_gun_10_8->setVisible(flag_enable[9][7]);
    ui->label_gun_10_8u->setVisible(flag_enable[9][7]);

    ui->label_gun_11_1->setVisible(flag_enable[10][0]);
    ui->label_gun_11_1u->setVisible(flag_enable[10][0]);
    ui->label_gun_11_2->setVisible(flag_enable[10][1]);
    ui->label_gun_11_2u->setVisible(flag_enable[10][1]);
    ui->label_gun_11_3->setVisible(flag_enable[10][2]);
    ui->label_gun_11_3u->setVisible(flag_enable[10][2]);
    ui->label_gun_11_4->setVisible(flag_enable[10][3]);
    ui->label_gun_11_4u->setVisible(flag_enable[10][3]);
    ui->label_gun_11_5->setVisible(flag_enable[10][4]);
    ui->label_gun_11_5u->setVisible(flag_enable[10][4]);
    ui->label_gun_11_6->setVisible(flag_enable[10][5]);
    ui->label_gun_11_6u->setVisible(flag_enable[10][5]);
    ui->label_gun_11_7->setVisible(flag_enable[10][6]);
    ui->label_gun_11_7u->setVisible(flag_enable[10][6]);
    ui->label_gun_11_8->setVisible(flag_enable[10][7]);
    ui->label_gun_11_8u->setVisible(flag_enable[10][7]);

    ui->label_gun_12_1->setVisible(flag_enable[11][0]);
    ui->label_gun_12_1u->setVisible(flag_enable[11][0]);
    ui->label_gun_12_2->setVisible(flag_enable[11][1]);
    ui->label_gun_12_2u->setVisible(flag_enable[11][1]);
    ui->label_gun_12_3->setVisible(flag_enable[11][2]);
    ui->label_gun_12_3u->setVisible(flag_enable[11][2]);
    ui->label_gun_12_4->setVisible(flag_enable[11][3]);
    ui->label_gun_12_4u->setVisible(flag_enable[11][3]);
    ui->label_gun_12_5->setVisible(flag_enable[11][4]);
    ui->label_gun_12_5u->setVisible(flag_enable[11][4]);
    ui->label_gun_12_6->setVisible(flag_enable[11][5]);
    ui->label_gun_12_6u->setVisible(flag_enable[11][5]);
    ui->label_gun_12_7->setVisible(flag_enable[11][6]);
    ui->label_gun_12_7u->setVisible(flag_enable[11][6]);
    ui->label_gun_12_8->setVisible(flag_enable[11][7]);
    ui->label_gun_12_8u->setVisible(flag_enable[11][7]);

	//油枪编号映射
	ui->label_map_0->setText((Mapping_Show[0]));
	if(Mapping_Show[0] == ""){ui->label_map_0->setText("-1");}
	ui->label_map_1->setText((Mapping_Show[1]));
	if(Mapping_Show[1] == ""){ui->label_map_1->setText("-2");}
	ui->label_map_2->setText((Mapping_Show[2]));
	if(Mapping_Show[2] == ""){ui->label_map_2->setText("-3");}
	ui->label_map_3->setText((Mapping_Show[3]));
	if(Mapping_Show[3] == ""){ui->label_map_3->setText("-4");}
	ui->label_map_4->setText((Mapping_Show[4]));
	if(Mapping_Show[4] == ""){ui->label_map_4->setText("-5");}
	ui->label_map_5->setText((Mapping_Show[5]));
	if(Mapping_Show[5] == ""){ui->label_map_5->setText("-6");}
	ui->label_map_6->setText((Mapping_Show[6]));
	if(Mapping_Show[6] == ""){ui->label_map_6->setText("-7");}
	ui->label_map_7->setText((Mapping_Show[7]));
	if(Mapping_Show[7] == ""){ui->label_map_7->setText("-8");}
	ui->label_map_8->setText((Mapping_Show[8]));
	if(Mapping_Show[8] == ""){ui->label_map_8->setText("-1");}
	ui->label_map_9->setText((Mapping_Show[9]));
	if(Mapping_Show[9] == ""){ui->label_map_9->setText("-2");}
	ui->label_map_10->setText((Mapping_Show[10]));
	if(Mapping_Show[10] == ""){ui->label_map_10->setText("-3");}
	ui->label_map_11->setText((Mapping_Show[11]));
	if(Mapping_Show[11] == ""){ui->label_map_11->setText("-4");}
	ui->label_map_12->setText((Mapping_Show[12]));
	if(Mapping_Show[12] == ""){ui->label_map_12->setText("-5");}
	ui->label_map_13->setText((Mapping_Show[13]));
	if(Mapping_Show[13] == ""){ui->label_map_13->setText("-6");}
	ui->label_map_14->setText((Mapping_Show[14]));
	if(Mapping_Show[14] == ""){ui->label_map_14->setText("-7");}
	ui->label_map_15->setText((Mapping_Show[15]));
	if(Mapping_Show[15] == ""){ui->label_map_15->setText("-8");}
	ui->label_map_16->setText((Mapping_Show[16]));
	if(Mapping_Show[16] == ""){ui->label_map_16->setText("-1");}
	ui->label_map_17->setText((Mapping_Show[17]));
	if(Mapping_Show[17] == ""){ui->label_map_17->setText("-2");}
	ui->label_map_18->setText((Mapping_Show[18]));
	if(Mapping_Show[18] == ""){ui->label_map_18->setText("-3");}
	ui->label_map_19->setText((Mapping_Show[19]));
	if(Mapping_Show[19] == ""){ui->label_map_19->setText("-4");}
	ui->label_map_20->setText((Mapping_Show[20]));
	if(Mapping_Show[20] == ""){ui->label_map_20->setText("-5");}
	ui->label_map_21->setText((Mapping_Show[21]));
	if(Mapping_Show[21] == ""){ui->label_map_21->setText("-6");}
	ui->label_map_22->setText((Mapping_Show[22]));
	if(Mapping_Show[22] == ""){ui->label_map_22->setText("-7");}
	ui->label_map_23->setText((Mapping_Show[23]));
	if(Mapping_Show[23] == ""){ui->label_map_23->setText("-8");}
	ui->label_map_24->setText((Mapping_Show[24]));
	if(Mapping_Show[24] == ""){ui->label_map_24->setText("-1");}
	ui->label_map_25->setText((Mapping_Show[25]));
	if(Mapping_Show[25] == ""){ui->label_map_25->setText("-2");}
	ui->label_map_26->setText((Mapping_Show[26]));
	if(Mapping_Show[26] == ""){ui->label_map_26->setText("-3");}
	ui->label_map_27->setText((Mapping_Show[27]));
	if(Mapping_Show[27] == ""){ui->label_map_27->setText("-4");}
	ui->label_map_28->setText((Mapping_Show[28]));
	if(Mapping_Show[28] == ""){ui->label_map_28->setText("-5");}
	ui->label_map_29->setText((Mapping_Show[29]));
	if(Mapping_Show[29] == ""){ui->label_map_29->setText("-6");}
	ui->label_map_30->setText((Mapping_Show[30]));
	if(Mapping_Show[30] == ""){ui->label_map_30->setText("-7");}
	ui->label_map_31->setText((Mapping_Show[31]));
	if(Mapping_Show[31] == ""){ui->label_map_31->setText("-8");}
	ui->label_map_32->setText((Mapping_Show[32]));
	if(Mapping_Show[32] == ""){ui->label_map_32->setText("-1");}
	ui->label_map_33->setText((Mapping_Show[33]));
	if(Mapping_Show[33] == ""){ui->label_map_33->setText("-2");}
	ui->label_map_34->setText((Mapping_Show[34]));
	if(Mapping_Show[34] == ""){ui->label_map_34->setText("-3");}
	ui->label_map_35->setText((Mapping_Show[35]));
	if(Mapping_Show[35] == ""){ui->label_map_35->setText("-4");}
	ui->label_map_36->setText((Mapping_Show[36]));
	if(Mapping_Show[36] == ""){ui->label_map_36->setText("-5");}
	ui->label_map_37->setText((Mapping_Show[37]));
	if(Mapping_Show[37] == ""){ui->label_map_37->setText("-6");}
	ui->label_map_38->setText((Mapping_Show[38]));
	if(Mapping_Show[38] == ""){ui->label_map_38->setText("-7");}
	ui->label_map_39->setText((Mapping_Show[39]));
	if(Mapping_Show[39] == ""){ui->label_map_39->setText("-8");}
	ui->label_map_40->setText((Mapping_Show[40]));
	if(Mapping_Show[40] == ""){ui->label_map_40->setText("-1");}
	ui->label_map_41->setText((Mapping_Show[41]));
	if(Mapping_Show[41] == ""){ui->label_map_41->setText("-2");}
	ui->label_map_42->setText((Mapping_Show[42]));
	if(Mapping_Show[42] == ""){ui->label_map_42->setText("-3");}
	ui->label_map_43->setText((Mapping_Show[43]));
	if(Mapping_Show[43] == ""){ui->label_map_43->setText("-4");}
	ui->label_map_44->setText((Mapping_Show[44]));
	if(Mapping_Show[44] == ""){ui->label_map_44->setText("-5");}
	ui->label_map_45->setText((Mapping_Show[45]));
	if(Mapping_Show[45] == ""){ui->label_map_45->setText("-6");}
	ui->label_map_46->setText((Mapping_Show[46]));
	if(Mapping_Show[46] == ""){ui->label_map_46->setText("-7");}
	ui->label_map_47->setText((Mapping_Show[47]));
	if(Mapping_Show[47] == ""){ui->label_map_47->setText("-8");}
	ui->label_map_48->setText((Mapping_Show[48]));
	if(Mapping_Show[48] == ""){ui->label_map_48->setText("-1");}
	ui->label_map_49->setText((Mapping_Show[49]));
	if(Mapping_Show[49] == ""){ui->label_map_49->setText("-2");}
	ui->label_map_50->setText((Mapping_Show[50]));
	if(Mapping_Show[50] == ""){ui->label_map_50->setText("-3");}
	ui->label_map_51->setText((Mapping_Show[51]));
	if(Mapping_Show[51] == ""){ui->label_map_51->setText("-4");}
	ui->label_map_52->setText((Mapping_Show[52]));
	if(Mapping_Show[52] == ""){ui->label_map_52->setText("-5");}
	ui->label_map_53->setText((Mapping_Show[53]));
	if(Mapping_Show[53] == ""){ui->label_map_53->setText("-6");}
	ui->label_map_54->setText((Mapping_Show[54]));
	if(Mapping_Show[54] == ""){ui->label_map_54->setText("-7");}
	ui->label_map_55->setText((Mapping_Show[55]));
	if(Mapping_Show[55] == ""){ui->label_map_55->setText("-8");}
	ui->label_map_56->setText((Mapping_Show[56]));
	if(Mapping_Show[56] == ""){ui->label_map_56->setText("-1");}
	ui->label_map_57->setText((Mapping_Show[57]));
	if(Mapping_Show[57] == ""){ui->label_map_57->setText("-2");}
	ui->label_map_58->setText((Mapping_Show[58]));
	if(Mapping_Show[58] == ""){ui->label_map_58->setText("-3");}
	ui->label_map_59->setText((Mapping_Show[59]));
	if(Mapping_Show[59] == ""){ui->label_map_59->setText("-4");}
	ui->label_map_60->setText((Mapping_Show[60]));
	if(Mapping_Show[60] == ""){ui->label_map_60->setText("-5");}
	ui->label_map_61->setText((Mapping_Show[61]));
	if(Mapping_Show[61] == ""){ui->label_map_61->setText("-6");}
	ui->label_map_62->setText((Mapping_Show[62]));
	if(Mapping_Show[62] == ""){ui->label_map_62->setText("-7");}
	ui->label_map_63->setText((Mapping_Show[63]));
	if(Mapping_Show[63] == ""){ui->label_map_63->setText("-8");}
	ui->label_map_64->setText((Mapping_Show[64]));
	if(Mapping_Show[64] == ""){ui->label_map_64->setText("-1");}
	ui->label_map_65->setText((Mapping_Show[65]));
	if(Mapping_Show[65] == ""){ui->label_map_65->setText("-2");}
	ui->label_map_66->setText((Mapping_Show[66]));
	if(Mapping_Show[66] == ""){ui->label_map_66->setText("-3");}
	ui->label_map_67->setText((Mapping_Show[67]));
	if(Mapping_Show[67] == ""){ui->label_map_67->setText("-4");}
	ui->label_map_68->setText((Mapping_Show[68]));
	if(Mapping_Show[68] == ""){ui->label_map_68->setText("-5");}
	ui->label_map_69->setText((Mapping_Show[69]));
	if(Mapping_Show[69] == ""){ui->label_map_69->setText("-6");}
	ui->label_map_70->setText((Mapping_Show[70]));
	if(Mapping_Show[70] == ""){ui->label_map_70->setText("-7");}
	ui->label_map_71->setText((Mapping_Show[71]));
	if(Mapping_Show[71] == ""){ui->label_map_71->setText("-8");}

	ui->label_map_72->setText((Mapping_Show[72]));
	if(Mapping_Show[72] == ""){ui->label_map_72->setText("-1");}
	ui->label_map_73->setText((Mapping_Show[73]));
	if(Mapping_Show[73] == ""){ui->label_map_73->setText("-2");}
	ui->label_map_74->setText((Mapping_Show[74]));
	if(Mapping_Show[74] == ""){ui->label_map_74->setText("-3");}
	ui->label_map_75->setText((Mapping_Show[75]));
	if(Mapping_Show[75] == ""){ui->label_map_75->setText("-4");}
	ui->label_map_76->setText((Mapping_Show[76]));
	if(Mapping_Show[76] == ""){ui->label_map_76->setText("-5");}
	ui->label_map_77->setText((Mapping_Show[77]));
	if(Mapping_Show[77] == ""){ui->label_map_77->setText("-6");}
	ui->label_map_78->setText((Mapping_Show[78]));
	if(Mapping_Show[78] == ""){ui->label_map_78->setText("-7");}
	ui->label_map_79->setText((Mapping_Show[79]));
	if(Mapping_Show[79] == ""){ui->label_map_79->setText("-8");}
	ui->label_map_80->setText((Mapping_Show[80]));
	if(Mapping_Show[80] == ""){ui->label_map_80->setText("-1");}
	ui->label_map_81->setText((Mapping_Show[81]));
	if(Mapping_Show[81] == ""){ui->label_map_81->setText("-2");}
	ui->label_map_82->setText((Mapping_Show[82]));
	if(Mapping_Show[82] == ""){ui->label_map_82->setText("-3");}
	ui->label_map_83->setText((Mapping_Show[83]));
	if(Mapping_Show[83] == ""){ui->label_map_83->setText("-4");}
	ui->label_map_84->setText((Mapping_Show[84]));
	if(Mapping_Show[84] == ""){ui->label_map_84->setText("-5");}
	ui->label_map_85->setText((Mapping_Show[85]));
	if(Mapping_Show[85] == ""){ui->label_map_85->setText("-6");}
	ui->label_map_86->setText((Mapping_Show[86]));
	if(Mapping_Show[86] == ""){ui->label_map_86->setText("-7");}
	ui->label_map_87->setText((Mapping_Show[87]));
	if(Mapping_Show[87] == ""){ui->label_map_87->setText("-8");}
	ui->label_map_88->setText((Mapping_Show[88]));
	if(Mapping_Show[88] == ""){ui->label_map_88->setText("-1");}
	ui->label_map_89->setText((Mapping_Show[89]));
	if(Mapping_Show[89] == ""){ui->label_map_89->setText("-2");}
	ui->label_map_90->setText((Mapping_Show[90]));
	if(Mapping_Show[90] == ""){ui->label_map_90->setText("-3");}
	ui->label_map_91->setText((Mapping_Show[91]));
	if(Mapping_Show[91] == ""){ui->label_map_91->setText("-4");}
	ui->label_map_92->setText((Mapping_Show[92]));
	if(Mapping_Show[92] == ""){ui->label_map_92->setText("-5");}
	ui->label_map_93->setText((Mapping_Show[93]));
	if(Mapping_Show[93] == ""){ui->label_map_93->setText("-6");}
	ui->label_map_94->setText((Mapping_Show[94]));
	if(Mapping_Show[94] == ""){ui->label_map_94->setText("-7");}
	ui->label_map_95->setText((Mapping_Show[95]));
	if(Mapping_Show[95] == ""){ui->label_map_95->setText("-8");}

	//油品号映射
	ui->label_OilNo_1->setText((Mapping_OilNo[0]));
	if(Mapping_OilNo[0] == ""){ui->label_OilNo_1->setText("");}
	ui->label_OilNo_2->setText((Mapping_OilNo[1]));
	if(Mapping_OilNo[1] == ""){ui->label_OilNo_2->setText("");}
	ui->label_OilNo_3->setText((Mapping_OilNo[2]));
	if(Mapping_OilNo[2] == ""){ui->label_OilNo_3->setText("");}
	ui->label_OilNo_4->setText((Mapping_OilNo[3]));
	if(Mapping_OilNo[3] == ""){ui->label_OilNo_4->setText("");}
	ui->label_OilNo_5->setText((Mapping_OilNo[4]));
	if(Mapping_OilNo[4] == ""){ui->label_OilNo_5->setText("");}
	ui->label_OilNo_6->setText((Mapping_OilNo[5]));
	if(Mapping_OilNo[5] == ""){ui->label_OilNo_6->setText("");}
	ui->label_OilNo_7->setText((Mapping_OilNo[6]));
	if(Mapping_OilNo[6] == ""){ui->label_OilNo_7->setText("");}
	ui->label_OilNo_8->setText((Mapping_OilNo[7]));
	if(Mapping_OilNo[7] == ""){ui->label_OilNo_8->setText("");}

	ui->label_OilNo_9->setText((Mapping_OilNo[8]));
	if(Mapping_OilNo[8] == ""){ui->label_OilNo_9->setText("");}
	ui->label_OilNo_10->setText((Mapping_OilNo[9]));
	if(Mapping_OilNo[9] == ""){ui->label_OilNo_10->setText("");}
	ui->label_OilNo_11->setText((Mapping_OilNo[10]));
	if(Mapping_OilNo[10] == ""){ui->label_OilNo_11->setText("");}
	ui->label_OilNo_12->setText((Mapping_OilNo[11]));
	if(Mapping_OilNo[11] == ""){ui->label_OilNo_12->setText("");}
	ui->label_OilNo_13->setText((Mapping_OilNo[12]));
	if(Mapping_OilNo[12] == ""){ui->label_OilNo_13->setText("");}
	ui->label_OilNo_14->setText((Mapping_OilNo[13]));
	if(Mapping_OilNo[13] == ""){ui->label_OilNo_14->setText("");}
	ui->label_OilNo_15->setText((Mapping_OilNo[14]));
	if(Mapping_OilNo[14] == ""){ui->label_OilNo_15->setText("");}
	ui->label_OilNo_16->setText((Mapping_OilNo[15]));
	if(Mapping_OilNo[15] == ""){ui->label_OilNo_16->setText("");}

	ui->label_OilNo_17->setText((Mapping_OilNo[16]));
	if(Mapping_OilNo[16] == ""){ui->label_OilNo_17->setText("");}
	ui->label_OilNo_18->setText((Mapping_OilNo[17]));
	if(Mapping_OilNo[17] == ""){ui->label_OilNo_18->setText("");}
	ui->label_OilNo_19->setText((Mapping_OilNo[18]));
	if(Mapping_OilNo[18] == ""){ui->label_OilNo_19->setText("");}
	ui->label_OilNo_20->setText((Mapping_OilNo[19]));
	if(Mapping_OilNo[19] == ""){ui->label_OilNo_20->setText("");}
	ui->label_OilNo_21->setText((Mapping_OilNo[20]));
	if(Mapping_OilNo[20] == ""){ui->label_OilNo_21->setText("");}
	ui->label_OilNo_22->setText((Mapping_OilNo[21]));
	if(Mapping_OilNo[21] == ""){ui->label_OilNo_22->setText("");}
	ui->label_OilNo_23->setText((Mapping_OilNo[22]));
	if(Mapping_OilNo[22] == ""){ui->label_OilNo_23->setText("");}
	ui->label_OilNo_24->setText((Mapping_OilNo[23]));
	if(Mapping_OilNo[23] == ""){ui->label_OilNo_24->setText("");}

	ui->label_OilNo_25->setText((Mapping_OilNo[24]));
	if(Mapping_OilNo[24] == ""){ui->label_OilNo_25->setText("");}
	ui->label_OilNo_26->setText((Mapping_OilNo[25]));
	if(Mapping_OilNo[25] == ""){ui->label_OilNo_26->setText("");}
	ui->label_OilNo_27->setText((Mapping_OilNo[26]));
	if(Mapping_OilNo[26] == ""){ui->label_OilNo_27->setText("");}
	ui->label_OilNo_28->setText((Mapping_OilNo[27]));
	if(Mapping_OilNo[27] == ""){ui->label_OilNo_28->setText("");}
	ui->label_OilNo_29->setText((Mapping_OilNo[28]));
	if(Mapping_OilNo[28] == ""){ui->label_OilNo_29->setText("");}
	ui->label_OilNo_30->setText((Mapping_OilNo[29]));
	if(Mapping_OilNo[29] == ""){ui->label_OilNo_30->setText("");}
	ui->label_OilNo_31->setText((Mapping_OilNo[30]));
	if(Mapping_OilNo[30] == ""){ui->label_OilNo_31->setText("");}
	ui->label_OilNo_32->setText((Mapping_OilNo[31]));
	if(Mapping_OilNo[31] == ""){ui->label_OilNo_32->setText("");}

	ui->label_OilNo_33->setText((Mapping_OilNo[32]));
	if(Mapping_OilNo[32] == ""){ui->label_OilNo_33->setText("");}
	ui->label_OilNo_34->setText((Mapping_OilNo[33]));
	if(Mapping_OilNo[33] == ""){ui->label_OilNo_34->setText("");}
	ui->label_OilNo_35->setText((Mapping_OilNo[34]));
	if(Mapping_OilNo[34] == ""){ui->label_OilNo_35->setText("");}
	ui->label_OilNo_36->setText((Mapping_OilNo[35]));
	if(Mapping_OilNo[35] == ""){ui->label_OilNo_36->setText("");}
	ui->label_OilNo_37->setText((Mapping_OilNo[36]));
	if(Mapping_OilNo[36] == ""){ui->label_OilNo_37->setText("");}
	ui->label_OilNo_38->setText((Mapping_OilNo[37]));
	if(Mapping_OilNo[37] == ""){ui->label_OilNo_38->setText("");}
	ui->label_OilNo_39->setText((Mapping_OilNo[38]));
	if(Mapping_OilNo[38] == ""){ui->label_OilNo_39->setText("");}
	ui->label_OilNo_40->setText((Mapping_OilNo[39]));
	if(Mapping_OilNo[39] == ""){ui->label_OilNo_40->setText("");}

	ui->label_OilNo_41->setText((Mapping_OilNo[40]));
	if(Mapping_OilNo[40] == ""){ui->label_OilNo_41->setText("");}
	ui->label_OilNo_42->setText((Mapping_OilNo[41]));
	if(Mapping_OilNo[41] == ""){ui->label_OilNo_42->setText("");}
	ui->label_OilNo_43->setText((Mapping_OilNo[42]));
	if(Mapping_OilNo[42] == ""){ui->label_OilNo_43->setText("");}
	ui->label_OilNo_44->setText((Mapping_OilNo[43]));
	if(Mapping_OilNo[43] == ""){ui->label_OilNo_44->setText("");}
	ui->label_OilNo_45->setText((Mapping_OilNo[44]));
	if(Mapping_OilNo[44] == ""){ui->label_OilNo_45->setText("");}
	ui->label_OilNo_46->setText((Mapping_OilNo[45]));
	if(Mapping_OilNo[45] == ""){ui->label_OilNo_46->setText("");}
	ui->label_OilNo_47->setText((Mapping_OilNo[46]));
	if(Mapping_OilNo[46] == ""){ui->label_OilNo_47->setText("");}
	ui->label_OilNo_48->setText((Mapping_OilNo[47]));
	if(Mapping_OilNo[47] == ""){ui->label_OilNo_48->setText("");}

	ui->label_OilNo_49->setText((Mapping_OilNo[48]));
	if(Mapping_OilNo[48] == ""){ui->label_OilNo_49->setText("");}
	ui->label_OilNo_50->setText((Mapping_OilNo[49]));
	if(Mapping_OilNo[49] == ""){ui->label_OilNo_50->setText("");}
	ui->label_OilNo_51->setText((Mapping_OilNo[50]));
	if(Mapping_OilNo[50] == ""){ui->label_OilNo_51->setText("");}
	ui->label_OilNo_52->setText((Mapping_OilNo[51]));
	if(Mapping_OilNo[51] == ""){ui->label_OilNo_52->setText("");}
	ui->label_OilNo_53->setText((Mapping_OilNo[52]));
	if(Mapping_OilNo[52] == ""){ui->label_OilNo_53->setText("");}
	ui->label_OilNo_54->setText((Mapping_OilNo[53]));
	if(Mapping_OilNo[53] == ""){ui->label_OilNo_54->setText("");}
	ui->label_OilNo_55->setText((Mapping_OilNo[54]));
	if(Mapping_OilNo[54] == ""){ui->label_OilNo_55->setText("");}
	ui->label_OilNo_56->setText((Mapping_OilNo[55]));
	if(Mapping_OilNo[55] == ""){ui->label_OilNo_56->setText("");}

	ui->label_OilNo_57->setText((Mapping_OilNo[56]));
	if(Mapping_OilNo[56] == ""){ui->label_OilNo_57->setText("");}
	ui->label_OilNo_58->setText((Mapping_OilNo[57]));
	if(Mapping_OilNo[57] == ""){ui->label_OilNo_58->setText("");}
	ui->label_OilNo_59->setText((Mapping_OilNo[58]));
	if(Mapping_OilNo[58] == ""){ui->label_OilNo_59->setText("");}
	ui->label_OilNo_60->setText((Mapping_OilNo[59]));
	if(Mapping_OilNo[59] == ""){ui->label_OilNo_60->setText("");}
	ui->label_OilNo_61->setText((Mapping_OilNo[60]));
	if(Mapping_OilNo[60] == ""){ui->label_OilNo_61->setText("");}
	ui->label_OilNo_62->setText((Mapping_OilNo[61]));
	if(Mapping_OilNo[61] == ""){ui->label_OilNo_62->setText("");}
	ui->label_OilNo_63->setText((Mapping_OilNo[62]));
	if(Mapping_OilNo[62] == ""){ui->label_OilNo_63->setText("");}
	ui->label_OilNo_64->setText((Mapping_OilNo[63]));
	if(Mapping_OilNo[63] == ""){ui->label_OilNo_64->setText("");}

	ui->label_OilNo_65->setText((Mapping_OilNo[64]));
	if(Mapping_OilNo[64] == ""){ui->label_OilNo_65->setText("");}
	ui->label_OilNo_66->setText((Mapping_OilNo[65]));
	if(Mapping_OilNo[65] == ""){ui->label_OilNo_66->setText("");}
	ui->label_OilNo_67->setText((Mapping_OilNo[66]));
	if(Mapping_OilNo[66] == ""){ui->label_OilNo_67->setText("");}
	ui->label_OilNo_68->setText((Mapping_OilNo[67]));
	if(Mapping_OilNo[67] == ""){ui->label_OilNo_68->setText("");}
	ui->label_OilNo_69->setText((Mapping_OilNo[68]));
	if(Mapping_OilNo[68] == ""){ui->label_OilNo_69->setText("");}
	ui->label_OilNo_70->setText((Mapping_OilNo[69]));
	if(Mapping_OilNo[69] == ""){ui->label_OilNo_70->setText("");}
	ui->label_OilNo_71->setText((Mapping_OilNo[70]));
	if(Mapping_OilNo[70] == ""){ui->label_OilNo_71->setText("");}
	ui->label_OilNo_72->setText((Mapping_OilNo[71]));
	if(Mapping_OilNo[71] == ""){ui->label_OilNo_72->setText("");}

	ui->label_OilNo_73->setText((Mapping_OilNo[72]));
	if(Mapping_OilNo[72] == ""){ui->label_OilNo_73->setText("");}
	ui->label_OilNo_74->setText((Mapping_OilNo[73]));
	if(Mapping_OilNo[73] == ""){ui->label_OilNo_74->setText("");}
	ui->label_OilNo_75->setText((Mapping_OilNo[74]));
	if(Mapping_OilNo[74] == ""){ui->label_OilNo_75->setText("");}
	ui->label_OilNo_76->setText((Mapping_OilNo[75]));
	if(Mapping_OilNo[75] == ""){ui->label_OilNo_76->setText("");}
	ui->label_OilNo_77->setText((Mapping_OilNo[76]));
	if(Mapping_OilNo[76] == ""){ui->label_OilNo_77->setText("");}
	ui->label_OilNo_78->setText((Mapping_OilNo[77]));
	if(Mapping_OilNo[77] == ""){ui->label_OilNo_78->setText("");}
	ui->label_OilNo_79->setText((Mapping_OilNo[78]));
	if(Mapping_OilNo[78] == ""){ui->label_OilNo_79->setText("");}
	ui->label_OilNo_80->setText((Mapping_OilNo[79]));
	if(Mapping_OilNo[79] == ""){ui->label_OilNo_80->setText("");}

	ui->label_OilNo_81->setText((Mapping_OilNo[80]));
	if(Mapping_OilNo[80] == ""){ui->label_OilNo_81->setText("");}
	ui->label_OilNo_82->setText((Mapping_OilNo[81]));
	if(Mapping_OilNo[81] == ""){ui->label_OilNo_82->setText("");}
	ui->label_OilNo_83->setText((Mapping_OilNo[82]));
	if(Mapping_OilNo[82] == ""){ui->label_OilNo_83->setText("");}
	ui->label_OilNo_84->setText((Mapping_OilNo[83]));
	if(Mapping_OilNo[83] == ""){ui->label_OilNo_84->setText("");}
	ui->label_OilNo_85->setText((Mapping_OilNo[84]));
	if(Mapping_OilNo[84] == ""){ui->label_OilNo_85->setText("");}
	ui->label_OilNo_86->setText((Mapping_OilNo[85]));
	if(Mapping_OilNo[85] == ""){ui->label_OilNo_86->setText("");}
	ui->label_OilNo_87->setText((Mapping_OilNo[86]));
	if(Mapping_OilNo[86] == ""){ui->label_OilNo_87->setText("");}
	ui->label_OilNo_88->setText((Mapping_OilNo[87]));
	if(Mapping_OilNo[87] == ""){ui->label_OilNo_88->setText("");}

	ui->label_OilNo_89->setText((Mapping_OilNo[88]));
	if(Mapping_OilNo[88] == ""){ui->label_OilNo_89->setText("");}
	ui->label_OilNo_90->setText((Mapping_OilNo[89]));
	if(Mapping_OilNo[89] == ""){ui->label_OilNo_90->setText("");}
	ui->label_OilNo_91->setText((Mapping_OilNo[90]));
	if(Mapping_OilNo[90] == ""){ui->label_OilNo_91->setText("");}
	ui->label_OilNo_92->setText((Mapping_OilNo[91]));
	if(Mapping_OilNo[91] == ""){ui->label_OilNo_92->setText("");}
	ui->label_OilNo_93->setText((Mapping_OilNo[92]));
	if(Mapping_OilNo[92] == ""){ui->label_OilNo_93->setText("");}
	ui->label_OilNo_94->setText((Mapping_OilNo[93]));
	if(Mapping_OilNo[93] == ""){ui->label_OilNo_94->setText("");}
	ui->label_OilNo_95->setText((Mapping_OilNo[94]));
	if(Mapping_OilNo[94] == ""){ui->label_OilNo_95->setText("");}
	ui->label_OilNo_96->setText((Mapping_OilNo[95]));
	if(Mapping_OilNo[95] == ""){ui->label_OilNo_96->setText("");}

	//油枪编号显示与否
    ui->label_map_0->setVisible(flag_enable_map[0][0]);
    ui->label_map_1->setVisible(flag_enable_map[0][1]);
    ui->label_map_2->setVisible(flag_enable_map[0][2]);
    ui->label_map_3->setVisible(flag_enable_map[0][3]);
    ui->label_map_4->setVisible(flag_enable_map[0][4]);
    ui->label_map_5->setVisible(flag_enable_map[0][5]);
    ui->label_map_6->setVisible(flag_enable_map[0][6]);
    ui->label_map_7->setVisible(flag_enable_map[0][7]);

    ui->label_map_8->setVisible(flag_enable_map[1][0]);
    ui->label_map_9->setVisible(flag_enable_map[1][1]);
    ui->label_map_10->setVisible(flag_enable_map[1][2]);
    ui->label_map_11->setVisible(flag_enable_map[1][3]);
    ui->label_map_12->setVisible(flag_enable_map[1][4]);
    ui->label_map_13->setVisible(flag_enable_map[1][5]);
    ui->label_map_14->setVisible(flag_enable_map[1][6]);
    ui->label_map_15->setVisible(flag_enable_map[1][7]);

    ui->label_map_16->setVisible(flag_enable_map[2][0]);
    ui->label_map_17->setVisible(flag_enable_map[2][1]);
    ui->label_map_18->setVisible(flag_enable_map[2][2]);
    ui->label_map_19->setVisible(flag_enable_map[2][3]);
    ui->label_map_20->setVisible(flag_enable_map[2][4]);
    ui->label_map_21->setVisible(flag_enable_map[2][5]);
    ui->label_map_22->setVisible(flag_enable_map[2][6]);
    ui->label_map_23->setVisible(flag_enable_map[2][7]);

    ui->label_map_24->setVisible(flag_enable_map[3][0]);
    ui->label_map_25->setVisible(flag_enable_map[3][1]);
    ui->label_map_26->setVisible(flag_enable_map[3][2]);
    ui->label_map_27->setVisible(flag_enable_map[3][3]);
    ui->label_map_28->setVisible(flag_enable_map[3][4]);
    ui->label_map_29->setVisible(flag_enable_map[3][5]);
    ui->label_map_30->setVisible(flag_enable_map[3][6]);
    ui->label_map_31->setVisible(flag_enable_map[3][7]);

    ui->label_map_32->setVisible(flag_enable_map[4][0]);
    ui->label_map_33->setVisible(flag_enable_map[4][1]);
    ui->label_map_34->setVisible(flag_enable_map[4][2]);
    ui->label_map_35->setVisible(flag_enable_map[4][3]);
    ui->label_map_36->setVisible(flag_enable_map[4][4]);
    ui->label_map_37->setVisible(flag_enable_map[4][5]);
    ui->label_map_38->setVisible(flag_enable_map[4][6]);
    ui->label_map_39->setVisible(flag_enable_map[4][7]);

    ui->label_map_40->setVisible(flag_enable_map[5][0]);
    ui->label_map_41->setVisible(flag_enable_map[5][1]);
    ui->label_map_42->setVisible(flag_enable_map[5][2]);
    ui->label_map_43->setVisible(flag_enable_map[5][3]);
    ui->label_map_44->setVisible(flag_enable_map[5][4]);
    ui->label_map_45->setVisible(flag_enable_map[5][5]);
    ui->label_map_46->setVisible(flag_enable_map[5][6]);
    ui->label_map_47->setVisible(flag_enable_map[5][7]);

    ui->label_map_48->setVisible(flag_enable_map[6][0]);
    ui->label_map_49->setVisible(flag_enable_map[6][1]);
    ui->label_map_50->setVisible(flag_enable_map[6][2]);
    ui->label_map_51->setVisible(flag_enable_map[6][3]);
    ui->label_map_52->setVisible(flag_enable_map[6][4]);
    ui->label_map_53->setVisible(flag_enable_map[6][5]);
    ui->label_map_54->setVisible(flag_enable_map[6][6]);
    ui->label_map_55->setVisible(flag_enable_map[6][7]);

    ui->label_map_56->setVisible(flag_enable_map[7][0]);
    ui->label_map_57->setVisible(flag_enable_map[7][1]);
    ui->label_map_58->setVisible(flag_enable_map[7][2]);
    ui->label_map_59->setVisible(flag_enable_map[7][3]);
    ui->label_map_60->setVisible(flag_enable_map[7][4]);
    ui->label_map_61->setVisible(flag_enable_map[7][5]);
    ui->label_map_62->setVisible(flag_enable_map[7][6]);
    ui->label_map_63->setVisible(flag_enable_map[7][7]);

    ui->label_map_64->setVisible(flag_enable_map[8][0]);
    ui->label_map_65->setVisible(flag_enable_map[8][1]);
    ui->label_map_66->setVisible(flag_enable_map[8][2]);
    ui->label_map_67->setVisible(flag_enable_map[8][3]);
    ui->label_map_68->setVisible(flag_enable_map[8][4]);
    ui->label_map_69->setVisible(flag_enable_map[8][5]);
    ui->label_map_70->setVisible(flag_enable_map[8][6]);
    ui->label_map_71->setVisible(flag_enable_map[8][7]);

    ui->label_map_72->setVisible(flag_enable_map[9][0]);
    ui->label_map_73->setVisible(flag_enable_map[9][1]);
    ui->label_map_74->setVisible(flag_enable_map[9][2]);
    ui->label_map_75->setVisible(flag_enable_map[9][3]);
    ui->label_map_76->setVisible(flag_enable_map[9][4]);
    ui->label_map_77->setVisible(flag_enable_map[9][5]);
    ui->label_map_78->setVisible(flag_enable_map[9][6]);
    ui->label_map_79->setVisible(flag_enable_map[9][7]);

    ui->label_map_80->setVisible(flag_enable_map[10][0]);
    ui->label_map_81->setVisible(flag_enable_map[10][1]);
    ui->label_map_82->setVisible(flag_enable_map[10][2]);
    ui->label_map_83->setVisible(flag_enable_map[10][3]);
    ui->label_map_84->setVisible(flag_enable_map[10][4]);
    ui->label_map_85->setVisible(flag_enable_map[10][5]);
    ui->label_map_86->setVisible(flag_enable_map[10][6]);
    ui->label_map_87->setVisible(flag_enable_map[10][7]);

    ui->label_map_88->setVisible(flag_enable_map[11][0]);
    ui->label_map_89->setVisible(flag_enable_map[11][1]);
    ui->label_map_90->setVisible(flag_enable_map[11][2]);
    ui->label_map_91->setVisible(flag_enable_map[11][3]);
    ui->label_map_92->setVisible(flag_enable_map[11][4]);
    ui->label_map_93->setVisible(flag_enable_map[11][5]);
    ui->label_map_94->setVisible(flag_enable_map[11][6]);
    ui->label_map_95->setVisible(flag_enable_map[11][7]);

	//油枪品号显示与否
	ui->label_OilNo_1->setVisible(flag_enable_map[0][0]);
	ui->label_OilNo_2->setVisible(flag_enable_map[0][1]);
	ui->label_OilNo_3->setVisible(flag_enable_map[0][2]);
	ui->label_OilNo_4->setVisible(flag_enable_map[0][3]);
	ui->label_OilNo_5->setVisible(flag_enable_map[0][4]);
	ui->label_OilNo_6->setVisible(flag_enable_map[0][5]);
	ui->label_OilNo_7->setVisible(flag_enable_map[0][6]);
	ui->label_OilNo_8->setVisible(flag_enable_map[0][7]);

	ui->label_OilNo_9->setVisible(flag_enable_map[1][0]);
	ui->label_OilNo_10->setVisible(flag_enable_map[1][1]);
	ui->label_OilNo_11->setVisible(flag_enable_map[1][2]);
	ui->label_OilNo_12->setVisible(flag_enable_map[1][3]);
	ui->label_OilNo_13->setVisible(flag_enable_map[1][4]);
	ui->label_OilNo_14->setVisible(flag_enable_map[1][5]);
	ui->label_OilNo_15->setVisible(flag_enable_map[1][6]);
	ui->label_OilNo_16->setVisible(flag_enable_map[1][7]);

	ui->label_OilNo_17->setVisible(flag_enable_map[2][0]);
	ui->label_OilNo_18->setVisible(flag_enable_map[2][1]);
	ui->label_OilNo_19->setVisible(flag_enable_map[2][2]);
	ui->label_OilNo_20->setVisible(flag_enable_map[2][3]);
	ui->label_OilNo_21->setVisible(flag_enable_map[2][4]);
	ui->label_OilNo_22->setVisible(flag_enable_map[2][5]);
	ui->label_OilNo_23->setVisible(flag_enable_map[2][6]);
	ui->label_OilNo_24->setVisible(flag_enable_map[2][7]);

	ui->label_OilNo_25->setVisible(flag_enable_map[3][0]);
	ui->label_OilNo_26->setVisible(flag_enable_map[3][1]);
	ui->label_OilNo_27->setVisible(flag_enable_map[3][2]);
	ui->label_OilNo_28->setVisible(flag_enable_map[3][3]);
	ui->label_OilNo_29->setVisible(flag_enable_map[3][4]);
	ui->label_OilNo_30->setVisible(flag_enable_map[3][5]);
	ui->label_OilNo_31->setVisible(flag_enable_map[3][6]);
	ui->label_OilNo_32->setVisible(flag_enable_map[3][7]);

	ui->label_OilNo_33->setVisible(flag_enable_map[4][0]);
	ui->label_OilNo_34->setVisible(flag_enable_map[4][1]);
	ui->label_OilNo_35->setVisible(flag_enable_map[4][2]);
	ui->label_OilNo_36->setVisible(flag_enable_map[4][3]);
	ui->label_OilNo_37->setVisible(flag_enable_map[4][4]);
	ui->label_OilNo_38->setVisible(flag_enable_map[4][5]);
	ui->label_OilNo_39->setVisible(flag_enable_map[4][6]);
	ui->label_OilNo_40->setVisible(flag_enable_map[4][7]);

	ui->label_OilNo_41->setVisible(flag_enable_map[5][0]);
	ui->label_OilNo_42->setVisible(flag_enable_map[5][1]);
	ui->label_OilNo_43->setVisible(flag_enable_map[5][2]);
	ui->label_OilNo_44->setVisible(flag_enable_map[5][3]);
	ui->label_OilNo_45->setVisible(flag_enable_map[5][4]);
	ui->label_OilNo_46->setVisible(flag_enable_map[5][5]);
	ui->label_OilNo_47->setVisible(flag_enable_map[5][6]);
	ui->label_OilNo_48->setVisible(flag_enable_map[5][7]);

	ui->label_OilNo_49->setVisible(flag_enable_map[6][0]);
	ui->label_OilNo_59->setVisible(flag_enable_map[6][1]);
	ui->label_OilNo_51->setVisible(flag_enable_map[6][2]);
	ui->label_OilNo_52->setVisible(flag_enable_map[6][3]);
	ui->label_OilNo_53->setVisible(flag_enable_map[6][4]);
	ui->label_OilNo_54->setVisible(flag_enable_map[6][5]);
	ui->label_OilNo_55->setVisible(flag_enable_map[6][6]);
	ui->label_OilNo_56->setVisible(flag_enable_map[6][7]);

	ui->label_OilNo_57->setVisible(flag_enable_map[7][0]);
	ui->label_OilNo_58->setVisible(flag_enable_map[7][1]);
	ui->label_OilNo_59->setVisible(flag_enable_map[7][2]);
	ui->label_OilNo_60->setVisible(flag_enable_map[7][3]);
	ui->label_OilNo_61->setVisible(flag_enable_map[7][4]);
	ui->label_OilNo_62->setVisible(flag_enable_map[7][5]);
	ui->label_OilNo_63->setVisible(flag_enable_map[7][6]);
	ui->label_OilNo_64->setVisible(flag_enable_map[7][7]);

	ui->label_OilNo_65->setVisible(flag_enable_map[8][0]);
	ui->label_OilNo_66->setVisible(flag_enable_map[8][1]);
	ui->label_OilNo_67->setVisible(flag_enable_map[8][2]);
	ui->label_OilNo_68->setVisible(flag_enable_map[8][3]);
	ui->label_OilNo_69->setVisible(flag_enable_map[8][4]);
	ui->label_OilNo_70->setVisible(flag_enable_map[8][5]);
	ui->label_OilNo_71->setVisible(flag_enable_map[8][6]);
	ui->label_OilNo_72->setVisible(flag_enable_map[8][7]);

	ui->label_OilNo_73->setVisible(flag_enable_map[9][0]);
	ui->label_OilNo_74->setVisible(flag_enable_map[9][1]);
	ui->label_OilNo_75->setVisible(flag_enable_map[9][2]);
	ui->label_OilNo_76->setVisible(flag_enable_map[9][3]);
	ui->label_OilNo_77->setVisible(flag_enable_map[9][4]);
	ui->label_OilNo_78->setVisible(flag_enable_map[9][5]);
	ui->label_OilNo_79->setVisible(flag_enable_map[9][6]);
	ui->label_OilNo_80->setVisible(flag_enable_map[9][7]);

	ui->label_OilNo_81->setVisible(flag_enable_map[10][0]);
	ui->label_OilNo_82->setVisible(flag_enable_map[10][1]);
	ui->label_OilNo_83->setVisible(flag_enable_map[10][2]);
	ui->label_OilNo_84->setVisible(flag_enable_map[10][3]);
	ui->label_OilNo_85->setVisible(flag_enable_map[10][4]);
	ui->label_OilNo_86->setVisible(flag_enable_map[10][5]);
	ui->label_OilNo_87->setVisible(flag_enable_map[10][6]);
	ui->label_OilNo_88->setVisible(flag_enable_map[10][7]);

	ui->label_OilNo_89->setVisible(flag_enable_map[11][0]);
	ui->label_OilNo_90->setVisible(flag_enable_map[11][1]);
	ui->label_OilNo_91->setVisible(flag_enable_map[11][2]);
	ui->label_OilNo_92->setVisible(flag_enable_map[11][3]);
	ui->label_OilNo_93->setVisible(flag_enable_map[11][4]);
	ui->label_OilNo_94->setVisible(flag_enable_map[11][5]);
	ui->label_OilNo_95->setVisible(flag_enable_map[11][6]);
	ui->label_OilNo_96->setVisible(flag_enable_map[11][7]);

}


void MainWindow::on_toolButton_dispen_1_clicked()
{
	QString settext = "";
    ui->label_delay_data->setHidden(0);
    qApp->processEvents();
    ui->widget_dispen_details->setHidden(0);
    ui->label_detail_whichdispen->setText("1");
    oneday_analy(1,0);
    ui->label_detail_1_day->setText(QString("%1").arg(Flag_Accumto[0][0]));
    ui->label_detail_1_al->setText(QString("%1").arg(PerDay_AL[0]));
    ui->label_detail_1_right->setText(QString("%1").arg(PerDay_Percent[0]));
    ui->label_detail_1_big->setText(QString("%1").arg(PerDay_Al_Big[0]));
    ui->label_detail_1_small->setText(QString("%1").arg(PerDay_Al_Smal[0]));

    ui->label_detail_2_day->setText(QString("%1").arg(Flag_Accumto[0][1]));
    ui->label_detail_2_al->setText(QString("%1").arg(PerDay_AL[1]));
    ui->label_detail_2_right->setText(QString("%1").arg(PerDay_Percent[1]));
    ui->label_detail_2_big->setText(QString("%1").arg(PerDay_Al_Big[1]));
    ui->label_detail_2_small->setText(QString("%1").arg(PerDay_Al_Smal[1]));

    ui->label_detail_3_day->setText(QString("%1").arg(Flag_Accumto[0][2]));
    ui->label_detail_3_al->setText(QString("%1").arg(PerDay_AL[2]));
    ui->label_detail_3_right->setText(QString("%1").arg(PerDay_Percent[2]));
    ui->label_detail_3_big->setText(QString("%1").arg(PerDay_Al_Big[2]));
    ui->label_detail_3_small->setText(QString("%1").arg(PerDay_Al_Smal[2]));

    ui->label_detail_4_day->setText(QString("%1").arg(Flag_Accumto[0][3]));
    ui->label_detail_4_al->setText(QString("%1").arg(PerDay_AL[3]));
    ui->label_detail_4_right->setText(QString("%1").arg(PerDay_Percent[3]));
    ui->label_detail_4_big->setText(QString("%1").arg(PerDay_Al_Big[3]));
    ui->label_detail_4_small->setText(QString("%1").arg(PerDay_Al_Smal[3]));

    ui->label_detail_5_day->setText(QString("%1").arg(Flag_Accumto[0][4]));
    ui->label_detail_5_al->setText(QString("%1").arg(PerDay_AL[4]));
    ui->label_detail_5_right->setText(QString("%1").arg(PerDay_Percent[4]));
    ui->label_detail_5_big->setText(QString("%1").arg(PerDay_Al_Big[4]));
    ui->label_detail_5_small->setText(QString("%1").arg(PerDay_Al_Smal[4]));

    ui->label_detail_6_day->setText(QString("%1").arg(Flag_Accumto[0][5]));
    ui->label_detail_6_al->setText(QString("%1").arg(PerDay_AL[5]));
    ui->label_detail_6_right->setText(QString("%1").arg(PerDay_Percent[5]));
    ui->label_detail_6_big->setText(QString("%1").arg(PerDay_Al_Big[5]));
    ui->label_detail_6_small->setText(QString("%1").arg(PerDay_Al_Smal[5]));

    ui->label_detail_7_day->setText(QString("%1").arg(Flag_Accumto[0][6]));
    ui->label_detail_7_al->setText(QString("%1").arg(PerDay_AL[6]));
    ui->label_detail_7_right->setText(QString("%1").arg(PerDay_Percent[6]));
    ui->label_detail_7_big->setText(QString("%1").arg(PerDay_Al_Big[6]));
    ui->label_detail_7_small->setText(QString("%1").arg(PerDay_Al_Smal[6]));

    ui->label_detail_8_day->setText(QString("%1").arg(Flag_Accumto[0][7]));
    ui->label_detail_8_al->setText(QString("%1").arg(PerDay_AL[7]));
    ui->label_detail_8_right->setText(QString("%1").arg(PerDay_Percent[7]));
    ui->label_detail_8_big->setText(QString("%1").arg(PerDay_Al_Big[7]));
    ui->label_detail_8_small->setText(QString("%1").arg(PerDay_Al_Smal[7]));

	if(Mapping_Show[0] != "")
    {
		settext = Mapping_Show[0];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_1->setText(settext);
    }
    else{ui->toolButton_gundetail_1->setText("1-1加油枪详情");}
	if(Mapping_Show[1] != "")
    {
		settext = Mapping_Show[1];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_2->setText(settext);
    }
    else{ui->toolButton_gundetail_2->setText("1-2加油枪详情");}
	if(Mapping_Show[2] != "")
    {
		settext = Mapping_Show[2];
		settext.append("#加油枪详情");
		 ui->toolButton_gundetail_3->setText(settext);
    }
    else{ui->toolButton_gundetail_3->setText("1-3加油枪详情");}
	if(Mapping_Show[3] != "")
    {
		settext = Mapping_Show[3];
		settext.append("#加油枪详情");
		 ui->toolButton_gundetail_4->setText(settext);
    }
    else{ui->toolButton_gundetail_4->setText("1-4加油枪详情");}
	if(Mapping_Show[4] != "")
    {
		settext = Mapping_Show[4];
		settext.append("#加油枪详情");
		 ui->toolButton_gundetail_5->setText(settext);
    }
    else{ui->toolButton_gundetail_5->setText("1-5加油枪详情");}
	if(Mapping_Show[5] != "")
    {
		settext = Mapping_Show[5];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_6->setText(settext);
    }
    else{ui->toolButton_gundetail_6->setText("1-6加油枪详情");}
	if(Mapping_Show[6] != "")
    {
		settext = Mapping_Show[6];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_7->setText(settext);
    }
    else{ui->toolButton_gundetail_7->setText("1-7加油枪详情");}
	if(Mapping_Show[7] != "")
    {
		settext = Mapping_Show[7];
		settext.append("#加油枪详情");
		 ui->toolButton_gundetail_8->setText(settext);
    }
    else{ui->toolButton_gundetail_8->setText("1-8加油枪详情");}
    ui->label_delay_data->setHidden(1);
}

void MainWindow::on_toolButton_dispen_2_clicked()
{
	QString settext = "";
    ui->label_delay_data->setHidden(0);
    qApp->processEvents();
    ui->widget_dispen_details->setHidden(0);
    ui->label_detail_whichdispen->setText("2");
    oneday_analy(2,0);
	ui->label_detail_1_day->setText(QString("%1").arg(Flag_Accumto[1][0]));
    ui->label_detail_1_al->setText(QString("%1").arg(PerDay_AL[0]));
    ui->label_detail_1_right->setText(QString("%1").arg(PerDay_Percent[0]));
    ui->label_detail_1_big->setText(QString("%1").arg(PerDay_Al_Big[0]));
    ui->label_detail_1_small->setText(QString("%1").arg(PerDay_Al_Smal[0]));

	ui->label_detail_2_day->setText(QString("%1").arg(Flag_Accumto[1][1]));
    ui->label_detail_2_al->setText(QString("%1").arg(PerDay_AL[1]));
    ui->label_detail_2_right->setText(QString("%1").arg(PerDay_Percent[1]));
    ui->label_detail_2_big->setText(QString("%1").arg(PerDay_Al_Big[1]));
    ui->label_detail_2_small->setText(QString("%1").arg(PerDay_Al_Smal[1]));

	ui->label_detail_3_day->setText(QString("%1").arg(Flag_Accumto[1][2]));
    ui->label_detail_3_al->setText(QString("%1").arg(PerDay_AL[2]));
    ui->label_detail_3_right->setText(QString("%1").arg(PerDay_Percent[2]));
    ui->label_detail_3_big->setText(QString("%1").arg(PerDay_Al_Big[2]));
    ui->label_detail_3_small->setText(QString("%1").arg(PerDay_Al_Smal[2]));

	ui->label_detail_4_day->setText(QString("%1").arg(Flag_Accumto[1][3]));
    ui->label_detail_4_al->setText(QString("%1").arg(PerDay_AL[3]));
    ui->label_detail_4_right->setText(QString("%1").arg(PerDay_Percent[3]));
    ui->label_detail_4_big->setText(QString("%1").arg(PerDay_Al_Big[3]));
    ui->label_detail_4_small->setText(QString("%1").arg(PerDay_Al_Smal[3]));

	ui->label_detail_5_day->setText(QString("%1").arg(Flag_Accumto[1][4]));
    ui->label_detail_5_al->setText(QString("%1").arg(PerDay_AL[4]));
    ui->label_detail_5_right->setText(QString("%1").arg(PerDay_Percent[4]));
    ui->label_detail_5_big->setText(QString("%1").arg(PerDay_Al_Big[4]));
    ui->label_detail_5_small->setText(QString("%1").arg(PerDay_Al_Smal[4]));

	ui->label_detail_6_day->setText(QString("%1").arg(Flag_Accumto[1][5]));
    ui->label_detail_6_al->setText(QString("%1").arg(PerDay_AL[5]));
    ui->label_detail_6_right->setText(QString("%1").arg(PerDay_Percent[5]));
    ui->label_detail_6_big->setText(QString("%1").arg(PerDay_Al_Big[5]));
    ui->label_detail_6_small->setText(QString("%1").arg(PerDay_Al_Smal[5]));

	ui->label_detail_7_day->setText(QString("%1").arg(Flag_Accumto[1][6]));
    ui->label_detail_7_al->setText(QString("%1").arg(PerDay_AL[6]));
    ui->label_detail_7_right->setText(QString("%1").arg(PerDay_Percent[6]));
    ui->label_detail_7_big->setText(QString("%1").arg(PerDay_Al_Big[6]));
    ui->label_detail_7_small->setText(QString("%1").arg(PerDay_Al_Smal[6]));

	ui->label_detail_8_day->setText(QString("%1").arg(Flag_Accumto[1][7]));
    ui->label_detail_8_al->setText(QString("%1").arg(PerDay_AL[7]));
    ui->label_detail_8_right->setText(QString("%1").arg(PerDay_Percent[7]));
    ui->label_detail_8_big->setText(QString("%1").arg(PerDay_Al_Big[7]));
    ui->label_detail_8_small->setText(QString("%1").arg(PerDay_Al_Smal[7]));

	if(Mapping_Show[8] != "")
    {
		settext = Mapping_Show[8];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_1->setText((settext));
    }
    else{ui->toolButton_gundetail_1->setText("2-1加油枪详情");}
	if(Mapping_Show[9] != "")
    {
		settext = Mapping_Show[9];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_2->setText((settext));
    }
    else{ui->toolButton_gundetail_2->setText("2-2加油枪详情");}
	if(Mapping_Show[10] != "")
    {
		settext = Mapping_Show[10];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_3->setText((settext));
    }
    else{ui->toolButton_gundetail_3->setText("2-3加油枪详情");}
	if(Mapping_Show[11] != "")
    {
		settext = Mapping_Show[11];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_4->setText((settext));
    }
    else{ui->toolButton_gundetail_4->setText("2-4加油枪详情");}
	if(Mapping_Show[12] != "")
    {
		settext = Mapping_Show[12];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_5->setText((settext));
    }
    else{ui->toolButton_gundetail_5->setText("2-5加油枪详情");}
	if(Mapping_Show[13] != "")
    {
		settext = Mapping_Show[13];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_6->setText((settext));
    }
    else{ui->toolButton_gundetail_6->setText("2-6加油枪详情");}
	if(Mapping_Show[14] != "")
    {
		settext = Mapping_Show[14];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_7->setText((settext));
    }
    else{ui->toolButton_gundetail_7->setText("2-7加油枪详情");}
	if(Mapping_Show[15] != "")
    {
		settext = Mapping_Show[15];
		settext.append("#加油枪详情");
		 ui->toolButton_gundetail_8->setText((settext));
    }
    else{ui->toolButton_gundetail_8->setText("2-8加油枪详情");}
    ui->label_delay_data->setHidden(1);
}

void MainWindow::on_toolButton_dispen_3_clicked()
{
	QString settext = "";
    ui->label_delay_data->setHidden(0);
    qApp->processEvents();
    ui->widget_dispen_details->setHidden(0);
    ui->label_detail_whichdispen->setText("3");
    oneday_analy(3,0);
	ui->label_detail_1_day->setText(QString("%1").arg(Flag_Accumto[2][0]));
    ui->label_detail_1_al->setText(QString("%1").arg(PerDay_AL[0]));
    ui->label_detail_1_right->setText(QString("%1").arg(PerDay_Percent[0]));
    ui->label_detail_1_big->setText(QString("%1").arg(PerDay_Al_Big[0]));
    ui->label_detail_1_small->setText(QString("%1").arg(PerDay_Al_Smal[0]));

	ui->label_detail_2_day->setText(QString("%1").arg(Flag_Accumto[2][1]));
    ui->label_detail_2_al->setText(QString("%1").arg(PerDay_AL[1]));
    ui->label_detail_2_right->setText(QString("%1").arg(PerDay_Percent[1]));
    ui->label_detail_2_big->setText(QString("%1").arg(PerDay_Al_Big[1]));
    ui->label_detail_2_small->setText(QString("%1").arg(PerDay_Al_Smal[1]));

	ui->label_detail_3_day->setText(QString("%1").arg(Flag_Accumto[2][2]));
    ui->label_detail_3_al->setText(QString("%1").arg(PerDay_AL[2]));
    ui->label_detail_3_right->setText(QString("%1").arg(PerDay_Percent[2]));
    ui->label_detail_3_big->setText(QString("%1").arg(PerDay_Al_Big[2]));
    ui->label_detail_3_small->setText(QString("%1").arg(PerDay_Al_Smal[2]));

	ui->label_detail_4_day->setText(QString("%1").arg(Flag_Accumto[2][3]));
    ui->label_detail_4_al->setText(QString("%1").arg(PerDay_AL[3]));
    ui->label_detail_4_right->setText(QString("%1").arg(PerDay_Percent[3]));
    ui->label_detail_4_big->setText(QString("%1").arg(PerDay_Al_Big[3]));
    ui->label_detail_4_small->setText(QString("%1").arg(PerDay_Al_Smal[3]));

	ui->label_detail_5_day->setText(QString("%1").arg(Flag_Accumto[2][4]));
    ui->label_detail_5_al->setText(QString("%1").arg(PerDay_AL[4]));
    ui->label_detail_5_right->setText(QString("%1").arg(PerDay_Percent[4]));
    ui->label_detail_5_big->setText(QString("%1").arg(PerDay_Al_Big[4]));
    ui->label_detail_5_small->setText(QString("%1").arg(PerDay_Al_Smal[4]));

	ui->label_detail_6_day->setText(QString("%1").arg(Flag_Accumto[2][5]));
    ui->label_detail_6_al->setText(QString("%1").arg(PerDay_AL[5]));
    ui->label_detail_6_right->setText(QString("%1").arg(PerDay_Percent[5]));
    ui->label_detail_6_big->setText(QString("%1").arg(PerDay_Al_Big[5]));
    ui->label_detail_6_small->setText(QString("%1").arg(PerDay_Al_Smal[5]));

	ui->label_detail_7_day->setText(QString("%1").arg(Flag_Accumto[2][6]));
    ui->label_detail_7_al->setText(QString("%1").arg(PerDay_AL[6]));
    ui->label_detail_7_right->setText(QString("%1").arg(PerDay_Percent[6]));
    ui->label_detail_7_big->setText(QString("%1").arg(PerDay_Al_Big[6]));
    ui->label_detail_7_small->setText(QString("%1").arg(PerDay_Al_Smal[6]));

	ui->label_detail_8_day->setText(QString("%1").arg(Flag_Accumto[2][7]));
    ui->label_detail_8_al->setText(QString("%1").arg(PerDay_AL[7]));
    ui->label_detail_8_right->setText(QString("%1").arg(PerDay_Percent[7]));
    ui->label_detail_8_big->setText(QString("%1").arg(PerDay_Al_Big[7]));
    ui->label_detail_8_small->setText(QString("%1").arg(PerDay_Al_Smal[7]));

	if(Mapping_Show[16] != "")
    {
		settext = Mapping_Show[16];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_1->setText(settext);
    }
    else{ui->toolButton_gundetail_1->setText("3-1加油枪详情");}
	if(Mapping_Show[17] != "")
    {
		settext = Mapping_Show[17];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_2->setText(settext);
    }
    else{ui->toolButton_gundetail_2->setText("3-2加油枪详情");}
	if(Mapping_Show[18] != "")
    {
		settext = Mapping_Show[18];
		settext.append("#加油枪详情");
		 ui->toolButton_gundetail_3->setText(settext);
    }
    else{ui->toolButton_gundetail_3->setText("3-3加油枪详情");}
	if(Mapping_Show[19] != "")
    {
		settext = Mapping_Show[19];
		settext.append("#加油枪详情");
		 ui->toolButton_gundetail_4->setText(settext);
    }
    else{ui->toolButton_gundetail_4->setText("3-4加油枪详情");}
	if(Mapping_Show[20] != "")
    {
		settext = Mapping_Show[20];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_5->setText(settext);
    }
    else{ui->toolButton_gundetail_5->setText("3-5加油枪详情");}
	if(Mapping_Show[21] != "")
    {
		settext = Mapping_Show[21];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_6->setText(settext);
    }
    else{ui->toolButton_gundetail_6->setText("3-6加油枪详情");}
	if(Mapping_Show[22] != "")
    {
		settext = Mapping_Show[22];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_7->setText(settext);
    }
    else{ui->toolButton_gundetail_7->setText("3-7加油枪详情");}
	if(Mapping_Show[23] != "")
    {
		settext = Mapping_Show[23];
		settext.append("#加油枪详情");
		 ui->toolButton_gundetail_8->setText(settext);
    }
    else{ui->toolButton_gundetail_8->setText("3-8加油枪详情");}
    ui->label_delay_data->setHidden(1);
}

void MainWindow::on_toolButton_dispen_4_clicked()
{
	QString settext = "";
    ui->label_delay_data->setHidden(0);
    qApp->processEvents();
    ui->widget_dispen_details->setHidden(0);
    ui->label_detail_whichdispen->setText("4");
    oneday_analy(4,0);
	ui->label_detail_1_day->setText(QString("%1").arg(Flag_Accumto[3][0]));
    ui->label_detail_1_al->setText(QString("%1").arg(PerDay_AL[0]));
    ui->label_detail_1_right->setText(QString("%1").arg(PerDay_Percent[0]));
    ui->label_detail_1_big->setText(QString("%1").arg(PerDay_Al_Big[0]));
    ui->label_detail_1_small->setText(QString("%1").arg(PerDay_Al_Smal[0]));

	ui->label_detail_2_day->setText(QString("%1").arg(Flag_Accumto[3][1]));
    ui->label_detail_2_al->setText(QString("%1").arg(PerDay_AL[1]));
    ui->label_detail_2_right->setText(QString("%1").arg(PerDay_Percent[1]));
    ui->label_detail_2_big->setText(QString("%1").arg(PerDay_Al_Big[1]));
    ui->label_detail_2_small->setText(QString("%1").arg(PerDay_Al_Smal[1]));

	ui->label_detail_3_day->setText(QString("%1").arg(Flag_Accumto[3][2]));
    ui->label_detail_3_al->setText(QString("%1").arg(PerDay_AL[2]));
    ui->label_detail_3_right->setText(QString("%1").arg(PerDay_Percent[2]));
    ui->label_detail_3_big->setText(QString("%1").arg(PerDay_Al_Big[2]));
    ui->label_detail_3_small->setText(QString("%1").arg(PerDay_Al_Smal[2]));

	ui->label_detail_4_day->setText(QString("%1").arg(Flag_Accumto[3][3]));
    ui->label_detail_4_al->setText(QString("%1").arg(PerDay_AL[3]));
    ui->label_detail_4_right->setText(QString("%1").arg(PerDay_Percent[3]));
    ui->label_detail_4_big->setText(QString("%1").arg(PerDay_Al_Big[3]));
    ui->label_detail_4_small->setText(QString("%1").arg(PerDay_Al_Smal[3]));

	ui->label_detail_5_day->setText(QString("%1").arg(Flag_Accumto[3][4]));
    ui->label_detail_5_al->setText(QString("%1").arg(PerDay_AL[4]));
    ui->label_detail_5_right->setText(QString("%1").arg(PerDay_Percent[4]));
    ui->label_detail_5_big->setText(QString("%1").arg(PerDay_Al_Big[4]));
    ui->label_detail_5_small->setText(QString("%1").arg(PerDay_Al_Smal[4]));

	ui->label_detail_6_day->setText(QString("%1").arg(Flag_Accumto[3][5]));
    ui->label_detail_6_al->setText(QString("%1").arg(PerDay_AL[5]));
    ui->label_detail_6_right->setText(QString("%1").arg(PerDay_Percent[5]));
    ui->label_detail_6_big->setText(QString("%1").arg(PerDay_Al_Big[5]));
    ui->label_detail_6_small->setText(QString("%1").arg(PerDay_Al_Smal[5]));

	ui->label_detail_7_day->setText(QString("%1").arg(Flag_Accumto[3][6]));
    ui->label_detail_7_al->setText(QString("%1").arg(PerDay_AL[6]));
    ui->label_detail_7_right->setText(QString("%1").arg(PerDay_Percent[6]));
    ui->label_detail_7_big->setText(QString("%1").arg(PerDay_Al_Big[6]));
    ui->label_detail_7_small->setText(QString("%1").arg(PerDay_Al_Smal[6]));

	ui->label_detail_8_day->setText(QString("%1").arg(Flag_Accumto[3][7]));
    ui->label_detail_8_al->setText(QString("%1").arg(PerDay_AL[7]));
    ui->label_detail_8_right->setText(QString("%1").arg(PerDay_Percent[7]));
    ui->label_detail_8_big->setText(QString("%1").arg(PerDay_Al_Big[7]));
    ui->label_detail_8_small->setText(QString("%1").arg(PerDay_Al_Smal[7]));

	if(Mapping_Show[24] != "")
    {
		settext = Mapping_Show[24];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_1->setText(settext);
    }
    else{ui->toolButton_gundetail_1->setText("4-1加油枪详情");}
	if(Mapping_Show[25] != "")
    {
		settext = Mapping_Show[25];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_2->setText(settext);
    }
    else{ui->toolButton_gundetail_2->setText("4-2加油枪详情");}
	if(Mapping_Show[26] != "")
    {
		settext = Mapping_Show[26];
		settext.append("#加油枪详情");
		 ui->toolButton_gundetail_3->setText(settext);
    }
    else{ui->toolButton_gundetail_3->setText("4-3加油枪详情");}
	if(Mapping_Show[27] != "")
    {
		settext = Mapping_Show[27];
		settext.append("#加油枪详情");
		 ui->toolButton_gundetail_4->setText(settext);
    }
    else{ui->toolButton_gundetail_4->setText("4-4加油枪详情");}
	if(Mapping_Show[28] != "")
    {
		settext = Mapping_Show[28];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_5->setText(settext);
    }
    else{ui->toolButton_gundetail_5->setText("4-5加油枪详情");}
	if(Mapping_Show[29] != "")
    {
		settext = Mapping_Show[29];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_6->setText(settext);
    }
    else{ui->toolButton_gundetail_6->setText("4-6加油枪详情");}
	if(Mapping_Show[30] != "")
    {
		settext = Mapping_Show[30];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_7->setText(settext);
    }
    else{ui->toolButton_gundetail_7->setText("4-7加油枪详情");}
	if(Mapping_Show[31] != "")
    {
		settext = Mapping_Show[31];
		settext.append("#加油枪详情");
		 ui->toolButton_gundetail_8->setText(settext);
    }
    else{ui->toolButton_gundetail_8->setText("4-8加油枪详情");}
    ui->label_delay_data->setHidden(1);
}

void MainWindow::on_toolButton_dispen_5_clicked()
{
	QString settext = "";
    ui->label_delay_data->setHidden(0);
    qApp->processEvents();
    ui->widget_dispen_details->setHidden(0);
    ui->label_detail_whichdispen->setText("5");
    oneday_analy(5,0);
	ui->label_detail_1_day->setText(QString("%1").arg(Flag_Accumto[4][0]));
    ui->label_detail_1_al->setText(QString("%1").arg(PerDay_AL[0]));
    ui->label_detail_1_right->setText(QString("%1").arg(PerDay_Percent[0]));
    ui->label_detail_1_big->setText(QString("%1").arg(PerDay_Al_Big[0]));
    ui->label_detail_1_small->setText(QString("%1").arg(PerDay_Al_Smal[0]));

	ui->label_detail_2_day->setText(QString("%1").arg(Flag_Accumto[4][1]));
    ui->label_detail_2_al->setText(QString("%1").arg(PerDay_AL[1]));
    ui->label_detail_2_right->setText(QString("%1").arg(PerDay_Percent[1]));
    ui->label_detail_2_big->setText(QString("%1").arg(PerDay_Al_Big[1]));
    ui->label_detail_2_small->setText(QString("%1").arg(PerDay_Al_Smal[1]));

	ui->label_detail_3_day->setText(QString("%1").arg(Flag_Accumto[4][2]));
    ui->label_detail_3_al->setText(QString("%1").arg(PerDay_AL[2]));
    ui->label_detail_3_right->setText(QString("%1").arg(PerDay_Percent[2]));
    ui->label_detail_3_big->setText(QString("%1").arg(PerDay_Al_Big[2]));
    ui->label_detail_3_small->setText(QString("%1").arg(PerDay_Al_Smal[2]));

	ui->label_detail_4_day->setText(QString("%1").arg(Flag_Accumto[4][3]));
    ui->label_detail_4_al->setText(QString("%1").arg(PerDay_AL[3]));
    ui->label_detail_4_right->setText(QString("%1").arg(PerDay_Percent[3]));
    ui->label_detail_4_big->setText(QString("%1").arg(PerDay_Al_Big[3]));
    ui->label_detail_4_small->setText(QString("%1").arg(PerDay_Al_Smal[3]));

	ui->label_detail_5_day->setText(QString("%1").arg(Flag_Accumto[4][4]));
    ui->label_detail_5_al->setText(QString("%1").arg(PerDay_AL[4]));
    ui->label_detail_5_right->setText(QString("%1").arg(PerDay_Percent[4]));
    ui->label_detail_5_big->setText(QString("%1").arg(PerDay_Al_Big[4]));
    ui->label_detail_5_small->setText(QString("%1").arg(PerDay_Al_Smal[4]));

	ui->label_detail_6_day->setText(QString("%1").arg(Flag_Accumto[4][5]));
    ui->label_detail_6_al->setText(QString("%1").arg(PerDay_AL[5]));
    ui->label_detail_6_right->setText(QString("%1").arg(PerDay_Percent[5]));
    ui->label_detail_6_big->setText(QString("%1").arg(PerDay_Al_Big[5]));
    ui->label_detail_6_small->setText(QString("%1").arg(PerDay_Al_Smal[5]));

	ui->label_detail_7_day->setText(QString("%1").arg(Flag_Accumto[4][6]));
    ui->label_detail_7_al->setText(QString("%1").arg(PerDay_AL[6]));
    ui->label_detail_7_right->setText(QString("%1").arg(PerDay_Percent[6]));
    ui->label_detail_7_big->setText(QString("%1").arg(PerDay_Al_Big[6]));
    ui->label_detail_7_small->setText(QString("%1").arg(PerDay_Al_Smal[6]));

	ui->label_detail_8_day->setText(QString("%1").arg(Flag_Accumto[4][7]));
    ui->label_detail_8_al->setText(QString("%1").arg(PerDay_AL[7]));
    ui->label_detail_8_right->setText(QString("%1").arg(PerDay_Percent[7]));
    ui->label_detail_8_big->setText(QString("%1").arg(PerDay_Al_Big[7]));
    ui->label_detail_8_small->setText(QString("%1").arg(PerDay_Al_Smal[7]));

	if(Mapping_Show[32] != "")
    {
		settext = Mapping_Show[32];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_1->setText(settext);
    }
    else{ui->toolButton_gundetail_1->setText("5-1加油枪详情");}
	if(Mapping_Show[33] != "")
    {
		settext = Mapping_Show[33];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_2->setText(settext);
    }
    else{ui->toolButton_gundetail_2->setText("5-2加油枪详情");}
	if(Mapping_Show[34] != "")
    {
		settext = Mapping_Show[34];
		settext.append("#加油枪详情");
		 ui->toolButton_gundetail_3->setText(settext);
    }
    else{ui->toolButton_gundetail_3->setText("5-3加油枪详情");}
	if(Mapping_Show[35] != "")
    {
		settext = Mapping_Show[35];
		settext.append("#加油枪详情");
		 ui->toolButton_gundetail_4->setText(settext);
    }
    else{ui->toolButton_gundetail_4->setText("5-4加油枪详情");}
	if(Mapping_Show[36] != "")
    {
		settext = Mapping_Show[36];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_5->setText(settext);
    }
    else{ui->toolButton_gundetail_5->setText("5-5加油枪详情");}
	if(Mapping_Show[37] != "")
    {
		settext = Mapping_Show[37];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_6->setText(settext);
    }
    else{ui->toolButton_gundetail_6->setText("5-6加油枪详情");}
	if(Mapping_Show[38] != "")
    {
		settext = Mapping_Show[38];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_7->setText(settext);
    }
    else{ui->toolButton_gundetail_7->setText("5-7加油枪详情");}
	if(Mapping_Show[39] != "")
    {
		settext = Mapping_Show[39];
		settext.append("#加油枪详情");
		 ui->toolButton_gundetail_8->setText(settext);
    }
    else{ui->toolButton_gundetail_8->setText("5-8加油枪详情");}
    ui->label_delay_data->setHidden(1);
}

void MainWindow::on_toolButton_dispen_6_clicked()
{
	QString settext = "";
    ui->label_delay_data->setHidden(0);
    qApp->processEvents();
    ui->widget_dispen_details->setHidden(0);
    ui->label_detail_whichdispen->setText("6");
    oneday_analy(6,0);
	ui->label_detail_1_day->setText(QString("%1").arg(Flag_Accumto[5][0]));
    ui->label_detail_1_al->setText(QString("%1").arg(PerDay_AL[0]));
    ui->label_detail_1_right->setText(QString("%1").arg(PerDay_Percent[0]));
    ui->label_detail_1_big->setText(QString("%1").arg(PerDay_Al_Big[0]));
    ui->label_detail_1_small->setText(QString("%1").arg(PerDay_Al_Smal[0]));

	ui->label_detail_2_day->setText(QString("%1").arg(Flag_Accumto[5][1]));
    ui->label_detail_2_al->setText(QString("%1").arg(PerDay_AL[1]));
    ui->label_detail_2_right->setText(QString("%1").arg(PerDay_Percent[1]));
    ui->label_detail_2_big->setText(QString("%1").arg(PerDay_Al_Big[1]));
    ui->label_detail_2_small->setText(QString("%1").arg(PerDay_Al_Smal[1]));

	ui->label_detail_3_day->setText(QString("%1").arg(Flag_Accumto[5][2]));
    ui->label_detail_3_al->setText(QString("%1").arg(PerDay_AL[2]));
    ui->label_detail_3_right->setText(QString("%1").arg(PerDay_Percent[2]));
    ui->label_detail_3_big->setText(QString("%1").arg(PerDay_Al_Big[2]));
    ui->label_detail_3_small->setText(QString("%1").arg(PerDay_Al_Smal[2]));

	ui->label_detail_4_day->setText(QString("%1").arg(Flag_Accumto[5][3]));
    ui->label_detail_4_al->setText(QString("%1").arg(PerDay_AL[3]));
    ui->label_detail_4_right->setText(QString("%1").arg(PerDay_Percent[3]));
    ui->label_detail_4_big->setText(QString("%1").arg(PerDay_Al_Big[3]));
    ui->label_detail_4_small->setText(QString("%1").arg(PerDay_Al_Smal[3]));

	ui->label_detail_5_day->setText(QString("%1").arg(Flag_Accumto[5][4]));
    ui->label_detail_5_al->setText(QString("%1").arg(PerDay_AL[4]));
    ui->label_detail_5_right->setText(QString("%1").arg(PerDay_Percent[4]));
    ui->label_detail_5_big->setText(QString("%1").arg(PerDay_Al_Big[4]));
    ui->label_detail_5_small->setText(QString("%1").arg(PerDay_Al_Smal[4]));

	ui->label_detail_6_day->setText(QString("%1").arg(Flag_Accumto[5][5]));
    ui->label_detail_6_al->setText(QString("%1").arg(PerDay_AL[5]));
    ui->label_detail_6_right->setText(QString("%1").arg(PerDay_Percent[5]));
    ui->label_detail_6_big->setText(QString("%1").arg(PerDay_Al_Big[5]));
    ui->label_detail_6_small->setText(QString("%1").arg(PerDay_Al_Smal[5]));

	ui->label_detail_7_day->setText(QString("%1").arg(Flag_Accumto[5][6]));
    ui->label_detail_7_al->setText(QString("%1").arg(PerDay_AL[6]));
    ui->label_detail_7_right->setText(QString("%1").arg(PerDay_Percent[6]));
    ui->label_detail_7_big->setText(QString("%1").arg(PerDay_Al_Big[6]));
    ui->label_detail_7_small->setText(QString("%1").arg(PerDay_Al_Smal[6]));

	ui->label_detail_8_day->setText(QString("%1").arg(Flag_Accumto[5][7]));
    ui->label_detail_8_al->setText(QString("%1").arg(PerDay_AL[7]));
    ui->label_detail_8_right->setText(QString("%1").arg(PerDay_Percent[7]));
    ui->label_detail_8_big->setText(QString("%1").arg(PerDay_Al_Big[7]));
    ui->label_detail_8_small->setText(QString("%1").arg(PerDay_Al_Smal[7]));

	if(Mapping_Show[40] != "")
    {
		settext = Mapping_Show[40];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_1->setText(settext);
    }
    else{ui->toolButton_gundetail_1->setText("6-1加油枪详情");}
	if(Mapping_Show[41] != "")
    {
		settext = Mapping_Show[41];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_2->setText(settext);
    }
    else{ui->toolButton_gundetail_2->setText("6-2加油枪详情");}
	if(Mapping_Show[42] != "")
    {
		settext = Mapping_Show[42];
		settext.append("#加油枪详情");
		 ui->toolButton_gundetail_3->setText(settext);
    }
    else{ui->toolButton_gundetail_3->setText("6-3加油枪详情");}
	if(Mapping_Show[43] != "")
    {
		settext = Mapping_Show[43];
		settext.append("#加油枪详情");
		 ui->toolButton_gundetail_4->setText(settext);
    }
    else{ui->toolButton_gundetail_4->setText("6-4加油枪详情");}
	if(Mapping_Show[44] != "")
    {
		settext = Mapping_Show[44];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_5->setText(settext);
    }
    else{ui->toolButton_gundetail_5->setText("6-5加油枪详情");}
	if(Mapping_Show[45] != "")
    {
		settext = Mapping_Show[45];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_6->setText(settext);
    }
    else{ui->toolButton_gundetail_6->setText("6-6加油枪详情");}
	if(Mapping_Show[46] != "")
    {
		settext = Mapping_Show[46];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_7->setText(settext);
    }
    else{ui->toolButton_gundetail_7->setText("6-7加油枪详情");}
	if(Mapping_Show[47] != "")
    {
		settext = Mapping_Show[47];
		settext.append("#加油枪详情");
		 ui->toolButton_gundetail_8->setText(settext);
    }
    else{ui->toolButton_gundetail_8->setText("6-8加油枪详情");}
    ui->label_delay_data->setHidden(1);
}

void MainWindow::on_toolButton_dispen_7_clicked()
{
	QString settext = "";
    ui->label_delay_data->setHidden(0);
    qApp->processEvents();
    ui->widget_dispen_details->setHidden(0);
    ui->label_detail_whichdispen->setText("7");
    oneday_analy(7,0);
	ui->label_detail_1_day->setText(QString("%1").arg(Flag_Accumto[6][0]));
    ui->label_detail_1_al->setText(QString("%1").arg(PerDay_AL[0]));
    ui->label_detail_1_right->setText(QString("%1").arg(PerDay_Percent[0]));
    ui->label_detail_1_big->setText(QString("%1").arg(PerDay_Al_Big[0]));
    ui->label_detail_1_small->setText(QString("%1").arg(PerDay_Al_Smal[0]));

	ui->label_detail_2_day->setText(QString("%1").arg(Flag_Accumto[6][1]));
    ui->label_detail_2_al->setText(QString("%1").arg(PerDay_AL[1]));
    ui->label_detail_2_right->setText(QString("%1").arg(PerDay_Percent[1]));
    ui->label_detail_2_big->setText(QString("%1").arg(PerDay_Al_Big[1]));
    ui->label_detail_2_small->setText(QString("%1").arg(PerDay_Al_Smal[1]));

	ui->label_detail_3_day->setText(QString("%1").arg(Flag_Accumto[6][2]));
    ui->label_detail_3_al->setText(QString("%1").arg(PerDay_AL[2]));
    ui->label_detail_3_right->setText(QString("%1").arg(PerDay_Percent[2]));
    ui->label_detail_3_big->setText(QString("%1").arg(PerDay_Al_Big[2]));
    ui->label_detail_3_small->setText(QString("%1").arg(PerDay_Al_Smal[2]));

	ui->label_detail_4_day->setText(QString("%1").arg(Flag_Accumto[6][3]));
    ui->label_detail_4_al->setText(QString("%1").arg(PerDay_AL[3]));
    ui->label_detail_4_right->setText(QString("%1").arg(PerDay_Percent[3]));
    ui->label_detail_4_big->setText(QString("%1").arg(PerDay_Al_Big[3]));
    ui->label_detail_4_small->setText(QString("%1").arg(PerDay_Al_Smal[3]));

	ui->label_detail_5_day->setText(QString("%1").arg(Flag_Accumto[6][4]));
    ui->label_detail_5_al->setText(QString("%1").arg(PerDay_AL[4]));
    ui->label_detail_5_right->setText(QString("%1").arg(PerDay_Percent[4]));
    ui->label_detail_5_big->setText(QString("%1").arg(PerDay_Al_Big[4]));
    ui->label_detail_5_small->setText(QString("%1").arg(PerDay_Al_Smal[4]));

	ui->label_detail_6_day->setText(QString("%1").arg(Flag_Accumto[6][5]));
    ui->label_detail_6_al->setText(QString("%1").arg(PerDay_AL[5]));
    ui->label_detail_6_right->setText(QString("%1").arg(PerDay_Percent[5]));
    ui->label_detail_6_big->setText(QString("%1").arg(PerDay_Al_Big[5]));
    ui->label_detail_6_small->setText(QString("%1").arg(PerDay_Al_Smal[5]));

	ui->label_detail_7_day->setText(QString("%1").arg(Flag_Accumto[6][6]));
    ui->label_detail_7_al->setText(QString("%1").arg(PerDay_AL[6]));
    ui->label_detail_7_right->setText(QString("%1").arg(PerDay_Percent[6]));
    ui->label_detail_7_big->setText(QString("%1").arg(PerDay_Al_Big[6]));
    ui->label_detail_7_small->setText(QString("%1").arg(PerDay_Al_Smal[6]));

	ui->label_detail_8_day->setText(QString("%1").arg(Flag_Accumto[6][7]));
    ui->label_detail_8_al->setText(QString("%1").arg(PerDay_AL[7]));
    ui->label_detail_8_right->setText(QString("%1").arg(PerDay_Percent[7]));
    ui->label_detail_8_big->setText(QString("%1").arg(PerDay_Al_Big[7]));
    ui->label_detail_8_small->setText(QString("%1").arg(PerDay_Al_Smal[7]));

	if(Mapping_Show[48] != "")
    {
		settext = Mapping_Show[48];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_1->setText(settext);
    }
    else{ui->toolButton_gundetail_1->setText("7-1加油枪详情");}
	if(Mapping_Show[49] != "")
    {
		settext = Mapping_Show[49];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_2->setText(settext);
    }
    else{ui->toolButton_gundetail_2->setText("7-2加油枪详情");}
	if(Mapping_Show[50] != "")
    {
		settext = Mapping_Show[50];
		settext.append("#加油枪详情");
		 ui->toolButton_gundetail_3->setText(settext);
    }
    else{ui->toolButton_gundetail_3->setText("7-3加油枪详情");}
	if(Mapping_Show[51] != "")
    {
		settext = Mapping_Show[51];
		settext.append("#加油枪详情");
		 ui->toolButton_gundetail_4->setText(settext);
    }
    else{ui->toolButton_gundetail_4->setText("7-4加油枪详情");}
	if(Mapping_Show[52] != "")
    {
		settext = Mapping_Show[52];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_5->setText(settext);
    }
    else{ui->toolButton_gundetail_5->setText("7-5加油枪详情");}
	if(Mapping_Show[53] != "")
    {
		settext = Mapping_Show[53];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_6->setText(settext);
    }
    else{ui->toolButton_gundetail_6->setText("7-6加油枪详情");}
	if(Mapping_Show[54] != "")
    {
		settext = Mapping_Show[54];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_7->setText(settext);
    }
    else{ui->toolButton_gundetail_7->setText("7-7加油枪详情");}
	if(Mapping_Show[55] != "")
    {
		settext = Mapping_Show[55];
		settext.append("#加油枪详情");
		 ui->toolButton_gundetail_8->setText(settext);
    }
    else{ui->toolButton_gundetail_8->setText("7-8加油枪详情");}
    ui->label_delay_data->setHidden(1);
}

void MainWindow::on_toolButton_dispen_8_clicked()
{
	QString settext = "";
    ui->label_delay_data->setHidden(0);
    qApp->processEvents();
    ui->widget_dispen_details->setHidden(0);
    ui->label_detail_whichdispen->setText("8");
    oneday_analy(8,0);
	ui->label_detail_1_day->setText(QString("%1").arg(Flag_Accumto[7][0]));
    ui->label_detail_1_al->setText(QString("%1").arg(PerDay_AL[0]));
    ui->label_detail_1_right->setText(QString("%1").arg(PerDay_Percent[0]));
    ui->label_detail_1_big->setText(QString("%1").arg(PerDay_Al_Big[0]));
    ui->label_detail_1_small->setText(QString("%1").arg(PerDay_Al_Smal[0]));

	ui->label_detail_2_day->setText(QString("%1").arg(Flag_Accumto[7][1]));
    ui->label_detail_2_al->setText(QString("%1").arg(PerDay_AL[1]));
    ui->label_detail_2_right->setText(QString("%1").arg(PerDay_Percent[1]));
    ui->label_detail_2_big->setText(QString("%1").arg(PerDay_Al_Big[1]));
    ui->label_detail_2_small->setText(QString("%1").arg(PerDay_Al_Smal[1]));

	ui->label_detail_3_day->setText(QString("%1").arg(Flag_Accumto[7][2]));
    ui->label_detail_3_al->setText(QString("%1").arg(PerDay_AL[2]));
    ui->label_detail_3_right->setText(QString("%1").arg(PerDay_Percent[2]));
    ui->label_detail_3_big->setText(QString("%1").arg(PerDay_Al_Big[2]));
    ui->label_detail_3_small->setText(QString("%1").arg(PerDay_Al_Smal[2]));

	ui->label_detail_4_day->setText(QString("%1").arg(Flag_Accumto[7][3]));
    ui->label_detail_4_al->setText(QString("%1").arg(PerDay_AL[3]));
    ui->label_detail_4_right->setText(QString("%1").arg(PerDay_Percent[3]));
    ui->label_detail_4_big->setText(QString("%1").arg(PerDay_Al_Big[3]));
    ui->label_detail_4_small->setText(QString("%1").arg(PerDay_Al_Smal[3]));

	ui->label_detail_5_day->setText(QString("%1").arg(Flag_Accumto[7][4]));
    ui->label_detail_5_al->setText(QString("%1").arg(PerDay_AL[4]));
    ui->label_detail_5_right->setText(QString("%1").arg(PerDay_Percent[4]));
    ui->label_detail_5_big->setText(QString("%1").arg(PerDay_Al_Big[4]));
    ui->label_detail_5_small->setText(QString("%1").arg(PerDay_Al_Smal[4]));

	ui->label_detail_6_day->setText(QString("%1").arg(Flag_Accumto[7][5]));
    ui->label_detail_6_al->setText(QString("%1").arg(PerDay_AL[5]));
    ui->label_detail_6_right->setText(QString("%1").arg(PerDay_Percent[5]));
    ui->label_detail_6_big->setText(QString("%1").arg(PerDay_Al_Big[5]));
    ui->label_detail_6_small->setText(QString("%1").arg(PerDay_Al_Smal[5]));

	ui->label_detail_7_day->setText(QString("%1").arg(Flag_Accumto[7][6]));
    ui->label_detail_7_al->setText(QString("%1").arg(PerDay_AL[6]));
    ui->label_detail_7_right->setText(QString("%1").arg(PerDay_Percent[6]));
    ui->label_detail_7_big->setText(QString("%1").arg(PerDay_Al_Big[6]));
    ui->label_detail_7_small->setText(QString("%1").arg(PerDay_Al_Smal[6]));

	ui->label_detail_8_day->setText(QString("%1").arg(Flag_Accumto[7][7]));
    ui->label_detail_8_al->setText(QString("%1").arg(PerDay_AL[7]));
    ui->label_detail_8_right->setText(QString("%1").arg(PerDay_Percent[7]));
    ui->label_detail_8_big->setText(QString("%1").arg(PerDay_Al_Big[7]));
    ui->label_detail_8_small->setText(QString("%1").arg(PerDay_Al_Smal[7]));

	if(Mapping_Show[56] != "")
    {
		settext = Mapping_Show[56];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_1->setText(settext);
    }
    else{ui->toolButton_gundetail_1->setText("8-1加油枪详情");}
	if(Mapping_Show[57] != "")
    {
		settext = Mapping_Show[57];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_2->setText(settext);
    }
    else{ui->toolButton_gundetail_2->setText("8-2加油枪详情");}
	if(Mapping_Show[58] != "")
    {
		settext = Mapping_Show[58];
		settext.append("#加油枪详情");
		 ui->toolButton_gundetail_3->setText(settext);
    }
    else{ui->toolButton_gundetail_3->setText("8-3加油枪详情");}
	if(Mapping_Show[59] != "")
    {
		settext = Mapping_Show[59];
		settext.append("#加油枪详情");
		 ui->toolButton_gundetail_4->setText(settext);
    }
    else{ui->toolButton_gundetail_4->setText("8-4加油枪详情");}
	if(Mapping_Show[60] != "")
    {
		settext = Mapping_Show[60];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_5->setText(settext);
    }
    else{ui->toolButton_gundetail_5->setText("8-5加油枪详情");}
	if(Mapping_Show[61] != "")
    {
		settext = Mapping_Show[61];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_6->setText(settext);
    }
    else{ui->toolButton_gundetail_6->setText("8-6加油枪详情");}
	if(Mapping_Show[62] != "")
    {
		settext = Mapping_Show[62];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_7->setText(settext);
    }
    else{ui->toolButton_gundetail_7->setText("8-7加油枪详情");}
	if(Mapping_Show[63] != "")
    {
		settext = Mapping_Show[63];
		settext.append("#加油枪详情");
		 ui->toolButton_gundetail_8->setText(settext);
    }
    else{ui->toolButton_gundetail_8->setText("8-8加油枪详情");}
    ui->label_delay_data->setHidden(1);
}

void MainWindow::on_toolButton_dispen_9_clicked()
{
	QString settext = "";
    ui->label_delay_data->setHidden(0);
    qApp->processEvents();
    ui->widget_dispen_details->setHidden(0);
    ui->label_detail_whichdispen->setText("9");
    oneday_analy(9,0);
	ui->label_detail_1_day->setText(QString("%1").arg(Flag_Accumto[8][0]));
    ui->label_detail_1_al->setText(QString("%1").arg(PerDay_AL[0]));
    ui->label_detail_1_right->setText(QString("%1").arg(PerDay_Percent[0]));
    ui->label_detail_1_big->setText(QString("%1").arg(PerDay_Al_Big[0]));
    ui->label_detail_1_small->setText(QString("%1").arg(PerDay_Al_Smal[0]));

	ui->label_detail_2_day->setText(QString("%1").arg(Flag_Accumto[8][1]));
    ui->label_detail_2_al->setText(QString("%1").arg(PerDay_AL[1]));
    ui->label_detail_2_right->setText(QString("%1").arg(PerDay_Percent[1]));
    ui->label_detail_2_big->setText(QString("%1").arg(PerDay_Al_Big[1]));
    ui->label_detail_2_small->setText(QString("%1").arg(PerDay_Al_Smal[1]));

	ui->label_detail_3_day->setText(QString("%1").arg(Flag_Accumto[8][2]));
    ui->label_detail_3_al->setText(QString("%1").arg(PerDay_AL[2]));
    ui->label_detail_3_right->setText(QString("%1").arg(PerDay_Percent[2]));
    ui->label_detail_3_big->setText(QString("%1").arg(PerDay_Al_Big[2]));
    ui->label_detail_3_small->setText(QString("%1").arg(PerDay_Al_Smal[2]));

	ui->label_detail_4_day->setText(QString("%1").arg(Flag_Accumto[8][3]));
    ui->label_detail_4_al->setText(QString("%1").arg(PerDay_AL[3]));
    ui->label_detail_4_right->setText(QString("%1").arg(PerDay_Percent[3]));
    ui->label_detail_4_big->setText(QString("%1").arg(PerDay_Al_Big[3]));
    ui->label_detail_4_small->setText(QString("%1").arg(PerDay_Al_Smal[3]));

	ui->label_detail_5_day->setText(QString("%1").arg(Flag_Accumto[8][4]));
    ui->label_detail_5_al->setText(QString("%1").arg(PerDay_AL[4]));
    ui->label_detail_5_right->setText(QString("%1").arg(PerDay_Percent[4]));
    ui->label_detail_5_big->setText(QString("%1").arg(PerDay_Al_Big[4]));
    ui->label_detail_5_small->setText(QString("%1").arg(PerDay_Al_Smal[4]));

	ui->label_detail_6_day->setText(QString("%1").arg(Flag_Accumto[8][5]));
    ui->label_detail_6_al->setText(QString("%1").arg(PerDay_AL[5]));
    ui->label_detail_6_right->setText(QString("%1").arg(PerDay_Percent[5]));
    ui->label_detail_6_big->setText(QString("%1").arg(PerDay_Al_Big[5]));
    ui->label_detail_6_small->setText(QString("%1").arg(PerDay_Al_Smal[5]));

	ui->label_detail_7_day->setText(QString("%1").arg(Flag_Accumto[8][6]));
    ui->label_detail_7_al->setText(QString("%1").arg(PerDay_AL[6]));
    ui->label_detail_7_right->setText(QString("%1").arg(PerDay_Percent[6]));
    ui->label_detail_7_big->setText(QString("%1").arg(PerDay_Al_Big[6]));
    ui->label_detail_7_small->setText(QString("%1").arg(PerDay_Al_Smal[6]));

	ui->label_detail_8_day->setText(QString("%1").arg(Flag_Accumto[8][7]));
    ui->label_detail_8_al->setText(QString("%1").arg(PerDay_AL[7]));
    ui->label_detail_8_right->setText(QString("%1").arg(PerDay_Percent[7]));
    ui->label_detail_8_big->setText(QString("%1").arg(PerDay_Al_Big[7]));
    ui->label_detail_8_small->setText(QString("%1").arg(PerDay_Al_Smal[7]));
	if(Mapping_Show[64] != "")
    {
		settext = Mapping_Show[64];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_1->setText(settext);
    }
    else{ui->toolButton_gundetail_1->setText("9-1加油枪详情");}
	if(Mapping_Show[65] != "")
    {
		settext = Mapping_Show[65];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_2->setText(settext);
    }
    else{ui->toolButton_gundetail_2->setText("9-2加油枪详情");}
	if(Mapping_Show[66] != "")
    {
		settext = Mapping_Show[66];
		settext.append("#加油枪详情");
		 ui->toolButton_gundetail_3->setText(settext);
    }
    else{ui->toolButton_gundetail_3->setText("9-3加油枪详情");}
	if(Mapping_Show[67] != "")
    {
		settext = Mapping_Show[67];
		settext.append("#加油枪详情");
		 ui->toolButton_gundetail_4->setText(settext);
    }
    else{ui->toolButton_gundetail_4->setText("9-4加油枪详情");}
	if(Mapping_Show[68] != "")
    {
		settext = Mapping_Show[68];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_5->setText(settext);
    }
    else{ui->toolButton_gundetail_5->setText("9-5加油枪详情");}
	if(Mapping_Show[69] != "")
    {
		settext = Mapping_Show[69];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_6->setText(settext);
    }
    else{ui->toolButton_gundetail_6->setText("9-6加油枪详情");}
	if(Mapping_Show[70] != "")
    {
		settext = Mapping_Show[70];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_7->setText(settext);
    }
    else{ui->toolButton_gundetail_7->setText("9-7加油枪详情");}
	if(Mapping_Show[71] != "")
    {
		settext = Mapping_Show[71];
		settext.append("#加油枪详情");
		 ui->toolButton_gundetail_8->setText(settext);
    }
    else{ui->toolButton_gundetail_8->setText("9-8加油枪详情");}
    ui->label_delay_data->setHidden(1);
}

void MainWindow::on_toolButton_dispen_10_clicked()
{
	QString settext = "";
    ui->label_delay_data->setHidden(0);
    qApp->processEvents();
    ui->widget_dispen_details->setHidden(0);
    ui->label_detail_whichdispen->setText("10");
    oneday_analy(10,0);
	ui->label_detail_1_day->setText(QString("%1").arg(Flag_Accumto[9][0]));
    ui->label_detail_1_al->setText(QString("%1").arg(PerDay_AL[0]));
    ui->label_detail_1_right->setText(QString("%1").arg(PerDay_Percent[0]));
    ui->label_detail_1_big->setText(QString("%1").arg(PerDay_Al_Big[0]));
    ui->label_detail_1_small->setText(QString("%1").arg(PerDay_Al_Smal[0]));

	ui->label_detail_2_day->setText(QString("%1").arg(Flag_Accumto[9][1]));
    ui->label_detail_2_al->setText(QString("%1").arg(PerDay_AL[1]));
    ui->label_detail_2_right->setText(QString("%1").arg(PerDay_Percent[1]));
    ui->label_detail_2_big->setText(QString("%1").arg(PerDay_Al_Big[1]));
    ui->label_detail_2_small->setText(QString("%1").arg(PerDay_Al_Smal[1]));

	ui->label_detail_3_day->setText(QString("%1").arg(Flag_Accumto[9][2]));
    ui->label_detail_3_al->setText(QString("%1").arg(PerDay_AL[2]));
    ui->label_detail_3_right->setText(QString("%1").arg(PerDay_Percent[2]));
    ui->label_detail_3_big->setText(QString("%1").arg(PerDay_Al_Big[2]));
    ui->label_detail_3_small->setText(QString("%1").arg(PerDay_Al_Smal[2]));

	ui->label_detail_4_day->setText(QString("%1").arg(Flag_Accumto[9][3]));
    ui->label_detail_4_al->setText(QString("%1").arg(PerDay_AL[3]));
    ui->label_detail_4_right->setText(QString("%1").arg(PerDay_Percent[3]));
    ui->label_detail_4_big->setText(QString("%1").arg(PerDay_Al_Big[3]));
    ui->label_detail_4_small->setText(QString("%1").arg(PerDay_Al_Smal[3]));

	ui->label_detail_5_day->setText(QString("%1").arg(Flag_Accumto[9][4]));
    ui->label_detail_5_al->setText(QString("%1").arg(PerDay_AL[4]));
    ui->label_detail_5_right->setText(QString("%1").arg(PerDay_Percent[4]));
    ui->label_detail_5_big->setText(QString("%1").arg(PerDay_Al_Big[4]));
    ui->label_detail_5_small->setText(QString("%1").arg(PerDay_Al_Smal[4]));

	ui->label_detail_6_day->setText(QString("%1").arg(Flag_Accumto[9][5]));
    ui->label_detail_6_al->setText(QString("%1").arg(PerDay_AL[5]));
    ui->label_detail_6_right->setText(QString("%1").arg(PerDay_Percent[5]));
    ui->label_detail_6_big->setText(QString("%1").arg(PerDay_Al_Big[5]));
    ui->label_detail_6_small->setText(QString("%1").arg(PerDay_Al_Smal[5]));

	ui->label_detail_7_day->setText(QString("%1").arg(Flag_Accumto[9][6]));
    ui->label_detail_7_al->setText(QString("%1").arg(PerDay_AL[6]));
    ui->label_detail_7_right->setText(QString("%1").arg(PerDay_Percent[6]));
    ui->label_detail_7_big->setText(QString("%1").arg(PerDay_Al_Big[6]));
    ui->label_detail_7_small->setText(QString("%1").arg(PerDay_Al_Smal[6]));

	ui->label_detail_8_day->setText(QString("%1").arg(Flag_Accumto[9][7]));
    ui->label_detail_8_al->setText(QString("%1").arg(PerDay_AL[7]));
    ui->label_detail_8_right->setText(QString("%1").arg(PerDay_Percent[7]));
    ui->label_detail_8_big->setText(QString("%1").arg(PerDay_Al_Big[7]));
    ui->label_detail_8_small->setText(QString("%1").arg(PerDay_Al_Smal[7]));

	if(Mapping_Show[72] != "")
    {
		settext = Mapping_Show[72];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_1->setText(settext);
    }
    else{ui->toolButton_gundetail_1->setText("10-1加油枪详情");}
	if(Mapping_Show[73] != "")
    {
		settext = Mapping_Show[73];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_2->setText(settext);
    }
    else{ui->toolButton_gundetail_2->setText("10-2加油枪详情");}
	if(Mapping_Show[74] != "")
    {
		settext = Mapping_Show[74];
		settext.append("#加油枪详情");
		 ui->toolButton_gundetail_3->setText(settext);
    }
    else{ui->toolButton_gundetail_3->setText("10-3加油枪详情");}
	if(Mapping_Show[75] != "")
    {
		settext = Mapping_Show[75];
		settext.append("#加油枪详情");
		 ui->toolButton_gundetail_4->setText(settext);
    }
    else{ui->toolButton_gundetail_4->setText("10-4加油枪详情");}
	if(Mapping_Show[76] != "")
    {
		settext = Mapping_Show[76];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_5->setText(settext);
    }
    else{ui->toolButton_gundetail_5->setText("10-5加油枪详情");}
	if(Mapping_Show[77] != "")
    {
		settext = Mapping_Show[77];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_6->setText(settext);
    }
    else{ui->toolButton_gundetail_6->setText("10-6加油枪详情");}
	if(Mapping_Show[78] != "")
    {
		settext = Mapping_Show[78];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_7->setText(settext);
    }
    else{ui->toolButton_gundetail_7->setText("10-7加油枪详情");}
	if(Mapping_Show[79] != "")
    {
		settext = Mapping_Show[79];
		settext.append("#加油枪详情");
		 ui->toolButton_gundetail_8->setText(settext);
    }
    else{ui->toolButton_gundetail_8->setText("10-8加油枪详情");}
    ui->label_delay_data->setHidden(1);
}

void MainWindow::on_toolButton_dispen_11_clicked()
{
	QString settext = "";
    ui->label_delay_data->setHidden(0);
    qApp->processEvents();
    ui->widget_dispen_details->setHidden(0);
    ui->label_detail_whichdispen->setText("11");
    oneday_analy(11,0);
	ui->label_detail_1_day->setText(QString("%1").arg(Flag_Accumto[10][0]));
    ui->label_detail_1_al->setText(QString("%1").arg(PerDay_AL[0]));
    ui->label_detail_1_right->setText(QString("%1").arg(PerDay_Percent[0]));
    ui->label_detail_1_big->setText(QString("%1").arg(PerDay_Al_Big[0]));
    ui->label_detail_1_small->setText(QString("%1").arg(PerDay_Al_Smal[0]));

	ui->label_detail_2_day->setText(QString("%1").arg(Flag_Accumto[10][1]));
    ui->label_detail_2_al->setText(QString("%1").arg(PerDay_AL[1]));
    ui->label_detail_2_right->setText(QString("%1").arg(PerDay_Percent[1]));
    ui->label_detail_2_big->setText(QString("%1").arg(PerDay_Al_Big[1]));
    ui->label_detail_2_small->setText(QString("%1").arg(PerDay_Al_Smal[1]));

	ui->label_detail_3_day->setText(QString("%1").arg(Flag_Accumto[10][2]));
    ui->label_detail_3_al->setText(QString("%1").arg(PerDay_AL[2]));
    ui->label_detail_3_right->setText(QString("%1").arg(PerDay_Percent[2]));
    ui->label_detail_3_big->setText(QString("%1").arg(PerDay_Al_Big[2]));
    ui->label_detail_3_small->setText(QString("%1").arg(PerDay_Al_Smal[2]));

	ui->label_detail_4_day->setText(QString("%1").arg(Flag_Accumto[10][3]));
    ui->label_detail_4_al->setText(QString("%1").arg(PerDay_AL[3]));
    ui->label_detail_4_right->setText(QString("%1").arg(PerDay_Percent[3]));
    ui->label_detail_4_big->setText(QString("%1").arg(PerDay_Al_Big[3]));
    ui->label_detail_4_small->setText(QString("%1").arg(PerDay_Al_Smal[3]));

	ui->label_detail_5_day->setText(QString("%1").arg(Flag_Accumto[10][4]));
    ui->label_detail_5_al->setText(QString("%1").arg(PerDay_AL[4]));
    ui->label_detail_5_right->setText(QString("%1").arg(PerDay_Percent[4]));
    ui->label_detail_5_big->setText(QString("%1").arg(PerDay_Al_Big[4]));
    ui->label_detail_5_small->setText(QString("%1").arg(PerDay_Al_Smal[4]));

	ui->label_detail_6_day->setText(QString("%1").arg(Flag_Accumto[10][5]));
    ui->label_detail_6_al->setText(QString("%1").arg(PerDay_AL[5]));
    ui->label_detail_6_right->setText(QString("%1").arg(PerDay_Percent[5]));
    ui->label_detail_6_big->setText(QString("%1").arg(PerDay_Al_Big[5]));
    ui->label_detail_6_small->setText(QString("%1").arg(PerDay_Al_Smal[5]));

	ui->label_detail_7_day->setText(QString("%1").arg(Flag_Accumto[10][6]));
    ui->label_detail_7_al->setText(QString("%1").arg(PerDay_AL[6]));
    ui->label_detail_7_right->setText(QString("%1").arg(PerDay_Percent[6]));
    ui->label_detail_7_big->setText(QString("%1").arg(PerDay_Al_Big[6]));
    ui->label_detail_7_small->setText(QString("%1").arg(PerDay_Al_Smal[6]));

	ui->label_detail_8_day->setText(QString("%1").arg(Flag_Accumto[10][7]));
    ui->label_detail_8_al->setText(QString("%1").arg(PerDay_AL[7]));
    ui->label_detail_8_right->setText(QString("%1").arg(PerDay_Percent[7]));
    ui->label_detail_8_big->setText(QString("%1").arg(PerDay_Al_Big[7]));
    ui->label_detail_8_small->setText(QString("%1").arg(PerDay_Al_Smal[7]));

	if(Mapping_Show[80] != "")
    {
		settext = Mapping_Show[80];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_1->setText(settext);
    }
    else{ui->toolButton_gundetail_1->setText("11-1加油枪详情");}
	if(Mapping_Show[81] != "")
    {
		settext = Mapping_Show[81];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_2->setText(settext);
    }
    else{ui->toolButton_gundetail_2->setText("11-2加油枪详情");}
	if(Mapping_Show[82] != "")
    {
		settext = Mapping_Show[82];
		settext.append("#加油枪详情");
		 ui->toolButton_gundetail_3->setText(settext);
    }
    else{ui->toolButton_gundetail_3->setText("11-3加油枪详情");}
	if(Mapping_Show[83] != "")
    {
		settext = Mapping_Show[83];
		settext.append("#加油枪详情");
		 ui->toolButton_gundetail_4->setText(settext);
    }
    else{ui->toolButton_gundetail_4->setText("11-4加油枪详情");}
	if(Mapping_Show[84] != "")
    {
		settext = Mapping_Show[84];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_5->setText(settext);
    }
    else{ui->toolButton_gundetail_5->setText("11-5加油枪详情");}
	if(Mapping_Show[85] != "")
    {
		settext = Mapping_Show[85];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_6->setText(settext);
    }
    else{ui->toolButton_gundetail_6->setText("11-6加油枪详情");}
	if(Mapping_Show[86] != "")
    {
		settext = Mapping_Show[86];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_7->setText(settext);
    }
    else{ui->toolButton_gundetail_7->setText("11-7加油枪详情");}
	if(Mapping_Show[87] != "")
    {
		settext = Mapping_Show[87];
		settext.append("#加油枪详情");
		 ui->toolButton_gundetail_8->setText(settext);
    }
    else{ui->toolButton_gundetail_8->setText("11-8加油枪详情");}
    ui->label_delay_data->setHidden(1);
}

void MainWindow::on_toolButton_dispen_12_clicked()
{
	QString settext = "";
    ui->label_delay_data->setHidden(0);
    qApp->processEvents();
    ui->widget_dispen_details->setHidden(0);
    ui->label_detail_whichdispen->setText("12");
    oneday_analy(12,0);
	ui->label_detail_1_day->setText(QString("%1").arg(Flag_Accumto[11][0]));
    ui->label_detail_1_al->setText(QString("%1").arg(PerDay_AL[0]));
    ui->label_detail_1_right->setText(QString("%1").arg(PerDay_Percent[0]));
    ui->label_detail_1_big->setText(QString("%1").arg(PerDay_Al_Big[0]));
    ui->label_detail_1_small->setText(QString("%1").arg(PerDay_Al_Smal[0]));

	ui->label_detail_2_day->setText(QString("%1").arg(Flag_Accumto[11][1]));
    ui->label_detail_2_al->setText(QString("%1").arg(PerDay_AL[1]));
    ui->label_detail_2_right->setText(QString("%1").arg(PerDay_Percent[1]));
    ui->label_detail_2_big->setText(QString("%1").arg(PerDay_Al_Big[1]));
    ui->label_detail_2_small->setText(QString("%1").arg(PerDay_Al_Smal[1]));

	ui->label_detail_3_day->setText(QString("%1").arg(Flag_Accumto[11][2]));
    ui->label_detail_3_al->setText(QString("%1").arg(PerDay_AL[2]));
    ui->label_detail_3_right->setText(QString("%1").arg(PerDay_Percent[2]));
    ui->label_detail_3_big->setText(QString("%1").arg(PerDay_Al_Big[2]));
    ui->label_detail_3_small->setText(QString("%1").arg(PerDay_Al_Smal[2]));

	ui->label_detail_4_day->setText(QString("%1").arg(Flag_Accumto[11][3]));
    ui->label_detail_4_al->setText(QString("%1").arg(PerDay_AL[3]));
    ui->label_detail_4_right->setText(QString("%1").arg(PerDay_Percent[3]));
    ui->label_detail_4_big->setText(QString("%1").arg(PerDay_Al_Big[3]));
    ui->label_detail_4_small->setText(QString("%1").arg(PerDay_Al_Smal[3]));

	ui->label_detail_5_day->setText(QString("%1").arg(Flag_Accumto[11][4]));
    ui->label_detail_5_al->setText(QString("%1").arg(PerDay_AL[4]));
    ui->label_detail_5_right->setText(QString("%1").arg(PerDay_Percent[4]));
    ui->label_detail_5_big->setText(QString("%1").arg(PerDay_Al_Big[4]));
    ui->label_detail_5_small->setText(QString("%1").arg(PerDay_Al_Smal[4]));

	ui->label_detail_6_day->setText(QString("%1").arg(Flag_Accumto[11][5]));
    ui->label_detail_6_al->setText(QString("%1").arg(PerDay_AL[5]));
    ui->label_detail_6_right->setText(QString("%1").arg(PerDay_Percent[5]));
    ui->label_detail_6_big->setText(QString("%1").arg(PerDay_Al_Big[5]));
    ui->label_detail_6_small->setText(QString("%1").arg(PerDay_Al_Smal[5]));

	ui->label_detail_7_day->setText(QString("%1").arg(Flag_Accumto[11][6]));
    ui->label_detail_7_al->setText(QString("%1").arg(PerDay_AL[6]));
    ui->label_detail_7_right->setText(QString("%1").arg(PerDay_Percent[6]));
    ui->label_detail_7_big->setText(QString("%1").arg(PerDay_Al_Big[6]));
    ui->label_detail_7_small->setText(QString("%1").arg(PerDay_Al_Smal[6]));

	ui->label_detail_8_day->setText(QString("%1").arg(Flag_Accumto[11][7]));
    ui->label_detail_8_al->setText(QString("%1").arg(PerDay_AL[7]));
    ui->label_detail_8_right->setText(QString("%1").arg(PerDay_Percent[7]));
    ui->label_detail_8_big->setText(QString("%1").arg(PerDay_Al_Big[7]));
    ui->label_detail_8_small->setText(QString("%1").arg(PerDay_Al_Smal[7]));

	if(Mapping_Show[88] != "")
    {
		settext = Mapping_Show[88];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_1->setText(settext);
    }
    else{ui->toolButton_gundetail_1->setText("12-1加油枪详情");}
	if(Mapping_Show[89] != "")
    {
		settext = Mapping_Show[89];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_2->setText(settext);
    }
    else{ui->toolButton_gundetail_2->setText("12-2加油枪详情");}
	if(Mapping_Show[90] != "")
    {
		settext = Mapping_Show[90];
		settext.append("#加油枪详情");
		 ui->toolButton_gundetail_3->setText(settext);
    }
    else{ui->toolButton_gundetail_3->setText("12-3加油枪详情");}
	if(Mapping_Show[91] != "")
    {
		settext = Mapping_Show[91];
		settext.append("#加油枪详情");
		 ui->toolButton_gundetail_4->setText(settext);
    }
    else{ui->toolButton_gundetail_4->setText("12-4加油枪详情");}
	if(Mapping_Show[92] != "")
    {
		settext = Mapping_Show[92];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_5->setText(settext);
    }
    else{ui->toolButton_gundetail_5->setText("12-5加油枪详情");}
	if(Mapping_Show[93] != "")
    {
		settext = Mapping_Show[93];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_6->setText(settext);
    }
    else{ui->toolButton_gundetail_6->setText("12-6加油枪详情");}
	if(Mapping_Show[94] != "")
    {
		settext = Mapping_Show[94];
		settext.append("#加油枪详情");
		ui->toolButton_gundetail_7->setText(settext);
    }
    else{ui->toolButton_gundetail_7->setText("12-7加油枪详情");}
	if(Mapping_Show[95] != "")
    {
		settext = Mapping_Show[95];
		settext.append("#加油枪详情");
		 ui->toolButton_gundetail_8->setText(settext);
    }
    else{ui->toolButton_gundetail_8->setText("12-8加油枪详情");}
    ui->label_delay_data->setHidden(1);
}
void MainWindow::on_toolButton_close_oilgas_details_clicked()
{
    //ui->label_delay_data->setHidden(1);
    qApp->processEvents();
    ui->widget_dispen_details->setHidden(1);
}

void MainWindow::on_pushButton_4_clicked()      //屏幕test按键
{
    oneday_analy(10,1);
    gun_state_show();

    Lock_Mode_Reoilgas.lock();
    Flag_SendMode_Oilgas = 4;
    Lock_Mode_Reoilgas.unlock();
}

void MainWindow::gun_state_show()
{
    if(Flag_Show_ReoilgasPop == 2){Flag_Show_ReoilgasPop = 0;}
    for(unsigned int i=0;i<12;i++)
    {
        for(unsigned int j=0;j<8;j++)
        {
            if(Flag_Accumto[i][j]>0)
            {
                Flag_Show_ReoilgasPop = 2;//弹窗，预警报警类。
                if(Flag_Accumto[i][j]< WarnAL_Days)//预警
                {
					if(ReoilgasPop_GunSta[i*8+j] <= 10){ReoilgasPop_GunSta[i*8+j] = 1;}
                    add_value_gunwarn_details(QString::number(i*8+j),GUN_EARLY_WARNING);
                }
                else                            //报警
                {
					if(ReoilgasPop_GunSta[i*8+j] <= 10){ReoilgasPop_GunSta[i*8+j] = 2;}
                    add_value_gunwarn_details(QString::number(i*8+j),GUN_WARNING);
                }
            }
            else
            {
				if((ReoilgasPop_GunSta[i*8+j] == 1)||(ReoilgasPop_GunSta[i*8+j] == 2)) //不是故障
				{
					ReoilgasPop_GunSta[i*8+j] = 0;
					add_value_gunwarn_details(QString::number(i*8+j),GUN_NORMAL);
					//正常
				}
            }
        }
    }
    //1-1
    if(Flag_Accumto[0][0] > 0)
    {

        if(Flag_Accumto[0][0] >= WarnAL_Days)
        {
            ui->label_gun_1_1->setStyleSheet("border-image: url(:/picture/gasgun_red_L.png);");
            if(Flag_Delay_State[0][0] != 1)
            {
                add_value_reoilgaswarn("1-1","油枪报警");
            }
            Flag_Delay_State[0][0] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_1_1->setStyleSheet("border-image: url(:/picture/gasgun_yello_L.png);");
            if(Flag_Delay_State[0][0] != 2)
            {
                add_value_reoilgaswarn("1-1","油枪预警");
            }
            Flag_Delay_State[0][0] = 2;
        }
    }
    else
    {
        ui->label_gun_1_1->setStyleSheet("border-image: url(:/picture/gasgun_green_L.png);");
        if(Flag_Delay_State[0][0] != 0)
        {
            add_value_reoilgaswarn("1-1","油枪正常");
        }
        Flag_Delay_State[0][0] = 0;
    }
    //1-2
    if(Flag_Accumto[0][1] > 0)
    {

        if(Flag_Accumto[0][1] >= WarnAL_Days)
        {
            ui->label_gun_1_2->setStyleSheet("border-image: url(:/picture/gasgun_red_R.png);");
            if(Flag_Delay_State[0][1] != 1)
            {
                add_value_reoilgaswarn("1-2","油枪报警");
            }
            Flag_Delay_State[0][1] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_1_2->setStyleSheet("border-image: url(:/picture/gasgun_yello_R.png);");
            if(Flag_Delay_State[0][1] != 2)
            {
                add_value_reoilgaswarn("1-2","油枪预警");
            }
            Flag_Delay_State[0][1] = 2;
        }
    }
    else
    {
        ui->label_gun_1_2->setStyleSheet("border-image: url(:/picture/gasgun_green_R.png);");
        if(Flag_Delay_State[0][1] != 0)
        {
            add_value_reoilgaswarn("1-2","油枪正常");
        }
        Flag_Delay_State[0][1] = 0;
    }
    //1-3
    if(Flag_Accumto[0][2] > 0)
    {

        if(Flag_Accumto[0][2] >= WarnAL_Days)
        {
            ui->label_gun_1_3->setStyleSheet("border-image: url(:/picture/gasgun_red_L.png);");
            if(Flag_Delay_State[0][2] != 1)
            {
                add_value_reoilgaswarn("1-3","油枪报警");
            }
            Flag_Delay_State[0][2] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_1_3->setStyleSheet("border-image: url(:/picture/gasgun_yello_L.png);");
            if(Flag_Delay_State[0][2] != 2)
            {
                add_value_reoilgaswarn("1-3","油枪预警");
            }
            Flag_Delay_State[0][2] = 2;
        }
    }
    else
    {
        ui->label_gun_1_3->setStyleSheet("border-image: url(:/picture/gasgun_green_L.png);");
        if(Flag_Delay_State[0][2] != 0)
        {
            add_value_reoilgaswarn("1-3","油枪正常");
        }
        Flag_Delay_State[0][2] = 0;
    }
    //1-4
    if(Flag_Accumto[0][3] > 0)
    {

        if(Flag_Accumto[0][3] >= WarnAL_Days)
        {
            ui->label_gun_1_4->setStyleSheet("border-image: url(:/picture/gasgun_red_R.png);");
            if(Flag_Delay_State[0][3] != 1)
            {
                add_value_reoilgaswarn("1-4","油枪报警");
            }
            Flag_Delay_State[0][3] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_1_4->setStyleSheet("border-image: url(:/picture/gasgun_yello_R.png);");
            if(Flag_Delay_State[0][3] != 2)
            {
                add_value_reoilgaswarn("1-4","油枪预警");
            }
            Flag_Delay_State[0][3] = 2;
        }
    }
    else
    {
        ui->label_gun_1_4->setStyleSheet("border-image: url(:/picture/gasgun_green_R.png);");
        if(Flag_Delay_State[0][3] != 0)
        {
            add_value_reoilgaswarn("1-4","油枪正常");
        }
        Flag_Delay_State[0][3] = 0;
    }
    //1-5
    if(Flag_Accumto[0][4] > 0)
    {

        if(Flag_Accumto[0][4] >= WarnAL_Days)
        {
            ui->label_gun_1_5->setStyleSheet("border-image: url(:/picture/gasgun_red_L.png);");
            if(Flag_Delay_State[0][4] != 1)
            {
                add_value_reoilgaswarn("1-5","油枪报警");
            }
            Flag_Delay_State[0][4] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_1_5->setStyleSheet("border-image: url(:/picture/gasgun_yello_L.png);");
            if(Flag_Delay_State[0][4] != 2)
            {
                add_value_reoilgaswarn("1-5","油枪预警");
            }
            Flag_Delay_State[0][4] = 2;
        }
    }
    else
    {
        ui->label_gun_1_5->setStyleSheet("border-image: url(:/picture/gasgun_green_L.png);");
        if(Flag_Delay_State[0][4] != 0)
        {
            add_value_reoilgaswarn("1-5","油枪正常");
        }
        Flag_Delay_State[0][4] = 0;
    }
    //1-6
    if(Flag_Accumto[0][5] > 0)
    {

        if(Flag_Accumto[0][5] >= WarnAL_Days)
        {
            ui->label_gun_1_6->setStyleSheet("border-image: url(:/picture/gasgun_red_R.png);");
            if(Flag_Delay_State[0][5] != 1)
            {
                add_value_reoilgaswarn("1-6","油枪报警");
            }
            Flag_Delay_State[0][5] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_1_6->setStyleSheet("border-image: url(:/picture/gasgun_yello_R.png);");
            if(Flag_Delay_State[0][5] != 2)
            {
                add_value_reoilgaswarn("1-6","油枪预警");
            }
            Flag_Delay_State[0][5] = 2;
        }
    }
    else
    {
        ui->label_gun_1_6->setStyleSheet("border-image: url(:/picture/gasgun_green_R.png);");
        if(Flag_Delay_State[0][5] != 0)
        {
            add_value_reoilgaswarn("1-6","油枪正常");
        }
        Flag_Delay_State[0][5] = 0;
    }
    //1-7
    if(Flag_Accumto[0][6] > 0)
    {

        if(Flag_Accumto[0][6] >= WarnAL_Days)
        {
            ui->label_gun_1_7->setStyleSheet("border-image: url(:/picture/gasgun_red_L.png);");
            if(Flag_Delay_State[0][6] != 1)
            {
                add_value_reoilgaswarn("1-7","油枪报警");
            }
            Flag_Delay_State[0][6] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_1_7->setStyleSheet("border-image: url(:/picture/gasgun_yello_L.png);");
            if(Flag_Delay_State[0][6] != 2)
            {
                add_value_reoilgaswarn("1-7","油枪预警");
            }
            Flag_Delay_State[0][6] = 2;
        }
    }
    else
    {
        ui->label_gun_1_7->setStyleSheet("border-image: url(:/picture/gasgun_green_L.png);");
        if(Flag_Delay_State[0][6] != 0)
        {
            add_value_reoilgaswarn("1-7","油枪正常");
        }
        Flag_Delay_State[0][6] = 0;
    }
    //1-8
    if(Flag_Accumto[0][7] > 0)
    {

        if(Flag_Accumto[0][7] >= WarnAL_Days)
        {
            ui->label_gun_1_8->setStyleSheet("border-image: url(:/picture/gasgun_red_R.png);");
            if(Flag_Delay_State[0][7] != 1)
            {
                add_value_reoilgaswarn("1-8","油枪报警");
            }
            Flag_Delay_State[0][7] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_1_8->setStyleSheet("border-image: url(:/picture/gasgun_yello_R.png);");
            if(Flag_Delay_State[0][7] != 2)
            {
                add_value_reoilgaswarn("1-8","油枪预警");
            }
            Flag_Delay_State[0][7] = 2;
        }
    }
    else
    {
        ui->label_gun_1_8->setStyleSheet("border-image: url(:/picture/gasgun_green_R.png);");
        if(Flag_Delay_State[0][7] != 0)
        {
            add_value_reoilgaswarn("1-8","油枪正常");
        }
        Flag_Delay_State[0][7] = 0;
    }
    //2-1
    if(Flag_Accumto[1][0] > 0)
    {

        if(Flag_Accumto[1][0] >= WarnAL_Days)
        {
            ui->label_gun_2_1->setStyleSheet("border-image: url(:/picture/gasgun_red_L.png);");
            if(Flag_Delay_State[1][0] != 1)
            {
                add_value_reoilgaswarn("2-1","油枪报警");
            }
            Flag_Delay_State[1][0] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_2_1->setStyleSheet("border-image: url(:/picture/gasgun_yello_L.png);");
            if(Flag_Delay_State[1][0] != 2)
            {
                add_value_reoilgaswarn("2-1","油枪预警");
            }
            Flag_Delay_State[1][0] = 2;
        }
    }
    else
    {
        ui->label_gun_2_1->setStyleSheet("border-image: url(:/picture/gasgun_green_L.png);");
        if(Flag_Delay_State[1][0] != 0)
        {
            add_value_reoilgaswarn("2-1","油枪正常");
        }
        Flag_Delay_State[1][0] = 0;
    }
    //2-2
    if(Flag_Accumto[1][1] > 0)
    {

        if(Flag_Accumto[1][1] >= WarnAL_Days)
        {
            ui->label_gun_2_2->setStyleSheet("border-image: url(:/picture/gasgun_red_R.png);");
            if(Flag_Delay_State[1][1] != 1)
            {
                add_value_reoilgaswarn("2-2","油枪报警");
            }
            Flag_Delay_State[1][1] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_2_2->setStyleSheet("border-image: url(:/picture/gasgun_yello_R.png);");
            if(Flag_Delay_State[1][1] != 2)
            {
                add_value_reoilgaswarn("2-2","油枪预警");
            }
            Flag_Delay_State[1][1] = 2;
        }
    }
    else
    {
        ui->label_gun_2_2->setStyleSheet("border-image: url(:/picture/gasgun_green_R.png);");
        if(Flag_Delay_State[1][1] != 0)
        {
            add_value_reoilgaswarn("2-2","油枪正常");
        }
        Flag_Delay_State[1][1] = 0;
    }
    //2-3
    if(Flag_Accumto[1][2] > 0)
    {

        if(Flag_Accumto[1][2] >= WarnAL_Days)
        {
            ui->label_gun_2_3->setStyleSheet("border-image: url(:/picture/gasgun_red_L.png);");
            if(Flag_Delay_State[1][2] != 1)
            {
                add_value_reoilgaswarn("2-3","油枪报警");
            }
            Flag_Delay_State[1][2] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_2_3->setStyleSheet("border-image: url(:/picture/gasgun_yello_L.png);");
            if(Flag_Delay_State[1][2] != 2)
            {
                add_value_reoilgaswarn("2-3","油枪预警");
            }
            Flag_Delay_State[1][2] = 2;
        }
    }
    else
    {
        ui->label_gun_2_3->setStyleSheet("border-image: url(:/picture/gasgun_green_L.png);");
        if(Flag_Delay_State[1][2] != 0)
        {
            add_value_reoilgaswarn("2-3","油枪正常");
        }
        Flag_Delay_State[1][2] = 0;
    }
    //2-4
    if(Flag_Accumto[1][3] > 0)
    {

        if(Flag_Accumto[1][3] >= WarnAL_Days)
        {
            ui->label_gun_2_4->setStyleSheet("border-image: url(:/picture/gasgun_red_R.png);");
            if(Flag_Delay_State[1][3] != 1)
            {
                add_value_reoilgaswarn("2-4","油枪报警");
            }
            Flag_Delay_State[1][3] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_2_4->setStyleSheet("border-image: url(:/picture/gasgun_yello_R.png);");
            if(Flag_Delay_State[1][3] != 2)
            {
                add_value_reoilgaswarn("2-4","油枪预警");
            }
            Flag_Delay_State[1][3] = 2;
        }
    }
    else
    {
        ui->label_gun_2_4->setStyleSheet("border-image: url(:/picture/gasgun_green_R.png);");
        if(Flag_Delay_State[1][3] != 0)
        {
            add_value_reoilgaswarn("2-4","油枪正常");
        }
        Flag_Delay_State[1][3] = 0;
    }
    //2-5
    if(Flag_Accumto[1][4] > 0)
    {

        if(Flag_Accumto[1][4] >= WarnAL_Days)
        {
            ui->label_gun_2_5->setStyleSheet("border-image: url(:/picture/gasgun_red_L.png);");
            if(Flag_Delay_State[1][4] != 1)
            {
                add_value_reoilgaswarn("2-5","油枪报警");
            }
            Flag_Delay_State[1][4] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_2_5->setStyleSheet("border-image: url(:/picture/gasgun_yello_L.png);");
            if(Flag_Delay_State[1][4] != 2)
            {
                add_value_reoilgaswarn("2-5","油枪预警");
            }
            Flag_Delay_State[1][4] = 2;
        }
    }
    else
    {
        ui->label_gun_2_5->setStyleSheet("border-image: url(:/picture/gasgun_green_L.png);");
        if(Flag_Delay_State[1][4] != 0)
        {
            add_value_reoilgaswarn("2-5","油枪正常");
        }
        Flag_Delay_State[1][4] = 0;
    }
    //2-6
    if(Flag_Accumto[1][5] > 0)
    {

        if(Flag_Accumto[1][5] >= WarnAL_Days)
        {
            ui->label_gun_2_6->setStyleSheet("border-image: url(:/picture/gasgun_red_R.png);");
            if(Flag_Delay_State[1][5] != 1)
            {
                add_value_reoilgaswarn("2-6","油枪报警");
            }
            Flag_Delay_State[1][5] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_2_6->setStyleSheet("border-image: url(:/picture/gasgun_yello_R.png);");
            if(Flag_Delay_State[1][5] != 2)
            {
                add_value_reoilgaswarn("2-6","油枪预警");
            }
            Flag_Delay_State[1][5] = 2;
        }
    }
    else
    {
        ui->label_gun_2_6->setStyleSheet("border-image: url(:/picture/gasgun_green_R.png);");
        if(Flag_Delay_State[1][5] != 0)
        {
            add_value_reoilgaswarn("2-6","油枪正常");
        }
        Flag_Delay_State[1][5] = 0;
    }
    //2-7
    if(Flag_Accumto[1][6] > 0)
    {

        if(Flag_Accumto[1][6] >= WarnAL_Days)
        {
            ui->label_gun_2_7->setStyleSheet("border-image: url(:/picture/gasgun_red_L.png);");
            if(Flag_Delay_State[1][6] != 1)
            {
                add_value_reoilgaswarn("2-7","油枪报警");
            }
            Flag_Delay_State[1][6] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_2_7->setStyleSheet("border-image: url(:/picture/gasgun_yello_L.png);");
            if(Flag_Delay_State[1][6] != 2)
            {
                add_value_reoilgaswarn("2-7","油枪预警");
            }
            Flag_Delay_State[1][6] = 2;
        }
    }
    else
    {
        ui->label_gun_2_7->setStyleSheet("border-image: url(:/picture/gasgun_green_L.png);");
        if(Flag_Delay_State[1][6] != 0)
        {
            add_value_reoilgaswarn("2-7","油枪正常");
        }
        Flag_Delay_State[1][6] = 0;
    }
    //2-8
    if(Flag_Accumto[1][7] > 0)
    {

        if(Flag_Accumto[1][7] >= WarnAL_Days)
        {
            ui->label_gun_2_8->setStyleSheet("border-image: url(:/picture/gasgun_red_R.png);");
            if(Flag_Delay_State[1][7] != 1)
            {
                add_value_reoilgaswarn("2-8","油枪报警");
            }
            Flag_Delay_State[1][7] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_2_8->setStyleSheet("border-image: url(:/picture/gasgun_yello_R.png);");
            if(Flag_Delay_State[1][7] != 2)
            {
                add_value_reoilgaswarn("2-8","油枪预警");
            }
            Flag_Delay_State[1][7] = 2;
        }
    }
    else
    {
        ui->label_gun_2_8->setStyleSheet("border-image: url(:/picture/gasgun_green_R.png);");
        if(Flag_Delay_State[1][7] != 0)
        {
            add_value_reoilgaswarn("2-8","油枪正常");
        }
        Flag_Delay_State[1][7] = 0;
    }
    //3-1
    if(Flag_Accumto[2][0] > 0)
    {

        if(Flag_Accumto[2][0] >= WarnAL_Days)
        {
            ui->label_gun_3_1->setStyleSheet("border-image: url(:/picture/gasgun_red_L.png);");
            if(Flag_Delay_State[2][0] != 1)
            {
                add_value_reoilgaswarn("3-1","油枪报警");
            }
            Flag_Delay_State[2][0] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_3_1->setStyleSheet("border-image: url(:/picture/gasgun_yello_L.png);");
            if(Flag_Delay_State[2][0] != 2)
            {
                add_value_reoilgaswarn("3-1","油枪预警");
            }
            Flag_Delay_State[2][0] = 2;
        }
    }
    else
    {
        ui->label_gun_3_1->setStyleSheet("border-image: url(:/picture/gasgun_green_L.png);");
        if(Flag_Delay_State[2][0] != 0)
        {
            add_value_reoilgaswarn("3-1","油枪正常");
        }
        Flag_Delay_State[2][0] = 0;
    }
    //3-2
    if(Flag_Accumto[2][1] > 0)
    {

        if(Flag_Accumto[2][1] >= WarnAL_Days)
        {
            ui->label_gun_3_2->setStyleSheet("border-image: url(:/picture/gasgun_red_R.png);");
            if(Flag_Delay_State[2][1] != 1)
            {
                add_value_reoilgaswarn("3-2","油枪报警");
            }
            Flag_Delay_State[2][1] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_3_2->setStyleSheet("border-image: url(:/picture/gasgun_yello_R.png);");
            if(Flag_Delay_State[2][1] != 2)
            {
                add_value_reoilgaswarn("3-2","油枪预警");
            }
            Flag_Delay_State[2][1] = 2;
        }
    }
    else
    {
        ui->label_gun_3_2->setStyleSheet("border-image: url(:/picture/gasgun_green_R.png);");
        if(Flag_Delay_State[2][1] != 0)
        {
            add_value_reoilgaswarn("3-2","油枪正常");
        }
        Flag_Delay_State[2][1] = 0;
    }
    //3-3
    if(Flag_Accumto[2][2] > 0)
    {

        if(Flag_Accumto[2][2] >= WarnAL_Days)
        {
            ui->label_gun_3_3->setStyleSheet("border-image: url(:/picture/gasgun_red_L.png);");
            if(Flag_Delay_State[2][2] != 1)
            {
                add_value_reoilgaswarn("3-3","油枪报警");
            }
            Flag_Delay_State[2][2] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_3_3->setStyleSheet("border-image: url(:/picture/gasgun_yello_L.png);");
            if(Flag_Delay_State[2][2] != 2)
            {
                add_value_reoilgaswarn("3-3","油枪预警");
            }
            Flag_Delay_State[2][2] = 2;
        }
    }
    else
    {
        ui->label_gun_3_3->setStyleSheet("border-image: url(:/picture/gasgun_green_L.png);");
        if(Flag_Delay_State[2][2] != 0)
        {
            add_value_reoilgaswarn("3-3","油枪正常");
        }
        Flag_Delay_State[2][2] = 0;
    }
    //3-4
    if(Flag_Accumto[2][3] > 0)
    {

        if(Flag_Accumto[2][3] >= WarnAL_Days)
        {
            ui->label_gun_3_4->setStyleSheet("border-image: url(:/picture/gasgun_red_R.png);");
            if(Flag_Delay_State[2][3] != 1)
            {
                add_value_reoilgaswarn("3-4","油枪报警");
            }
            Flag_Delay_State[2][3] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_3_4->setStyleSheet("border-image: url(:/picture/gasgun_yello_R.png);");
            if(Flag_Delay_State[2][3] != 2)
            {
                add_value_reoilgaswarn("3-4","油枪预警");
            }
            Flag_Delay_State[2][3] = 2;
        }
    }
    else
    {
        ui->label_gun_3_4->setStyleSheet("border-image: url(:/picture/gasgun_green_R.png);");
        if(Flag_Delay_State[2][3] != 0)
        {
            add_value_reoilgaswarn("3-4","油枪正常");
        }
        Flag_Delay_State[2][3] = 0;
    }
    //3-5
    if(Flag_Accumto[2][4] > 0)
    {

        if(Flag_Accumto[2][4] >= WarnAL_Days)
        {
            ui->label_gun_3_5->setStyleSheet("border-image: url(:/picture/gasgun_red_L.png);");
            if(Flag_Delay_State[2][4] != 1)
            {
                add_value_reoilgaswarn("3-5","油枪报警");
            }
            Flag_Delay_State[2][4] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_3_5->setStyleSheet("border-image: url(:/picture/gasgun_yello_L.png);");
            if(Flag_Delay_State[2][4] != 2)
            {
                add_value_reoilgaswarn("3-5","油枪预警");
            }
            Flag_Delay_State[2][4] = 2;
        }
    }
    else
    {
        ui->label_gun_3_5->setStyleSheet("border-image: url(:/picture/gasgun_green_L.png);");
        if(Flag_Delay_State[2][4] != 0)
        {
            add_value_reoilgaswarn("3-5","油枪正常");
        }
        Flag_Delay_State[2][4] = 0;
    }
    //3-6
    if(Flag_Accumto[2][5] > 0)
    {

        if(Flag_Accumto[2][5] >= WarnAL_Days)
        {
            ui->label_gun_3_6->setStyleSheet("border-image: url(:/picture/gasgun_red_R.png);");
            if(Flag_Delay_State[2][5] != 1)
            {
                add_value_reoilgaswarn("3-6","油枪报警");
            }
            Flag_Delay_State[2][5] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_3_6->setStyleSheet("border-image: url(:/picture/gasgun_yello_R.png);");
            if(Flag_Delay_State[2][5] != 2)
            {
                add_value_reoilgaswarn("3-6","油枪预警");
            }
            Flag_Delay_State[2][5] = 2;
        }
    }
    else
    {
        ui->label_gun_3_6->setStyleSheet("border-image: url(:/picture/gasgun_green_R.png);");
        if(Flag_Delay_State[2][5] != 0)
        {
            add_value_reoilgaswarn("3-6","油枪正常");
        }
        Flag_Delay_State[2][5] = 0;
    }
    //3-7
    if(Flag_Accumto[2][6] > 0)
    {

        if(Flag_Accumto[2][6] >= WarnAL_Days)
        {
            ui->label_gun_3_7->setStyleSheet("border-image: url(:/picture/gasgun_red_L.png);");
            if(Flag_Delay_State[2][6] != 1)
            {
                add_value_reoilgaswarn("3-7","油枪报警");
            }
            Flag_Delay_State[2][6] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_3_7->setStyleSheet("border-image: url(:/picture/gasgun_yello_L.png);");
            if(Flag_Delay_State[2][6] != 2)
            {
                add_value_reoilgaswarn("3-7","油枪预警");
            }
            Flag_Delay_State[2][6] = 2;
        }
    }
    else
    {
        ui->label_gun_3_7->setStyleSheet("border-image: url(:/picture/gasgun_green_L.png);");
        if(Flag_Delay_State[2][6] != 0)
        {
            add_value_reoilgaswarn("3-7","油枪正常");
        }
        Flag_Delay_State[2][6] = 0;
    }
    //3-8
    if(Flag_Accumto[2][7] > 0)
    {

        if(Flag_Accumto[2][7] >= WarnAL_Days)
        {
            ui->label_gun_3_8->setStyleSheet("border-image: url(:/picture/gasgun_red_R.png);");
            if(Flag_Delay_State[2][7] != 1)
            {
                add_value_reoilgaswarn("3-8","油枪报警");
            }
            Flag_Delay_State[2][7] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_3_8->setStyleSheet("border-image: url(:/picture/gasgun_yello_R.png);");
            if(Flag_Delay_State[2][7] != 2)
            {
                add_value_reoilgaswarn("3-8","油枪预警");
            }
            Flag_Delay_State[2][7] = 2;
        }
    }
    else
    {
        ui->label_gun_3_8->setStyleSheet("border-image: url(:/picture/gasgun_green_R.png);");
        if(Flag_Delay_State[2][7] != 0)
        {
            add_value_reoilgaswarn("3-8","油枪正常");
        }
        Flag_Delay_State[2][7] = 0;
    }
    //4-1
    if(Flag_Accumto[3][0] > 0)
    {

        if(Flag_Accumto[3][0] >= WarnAL_Days)
        {
            ui->label_gun_4_1->setStyleSheet("border-image: url(:/picture/gasgun_red_L.png);");
            if(Flag_Delay_State[3][0] != 1)
            {
                add_value_reoilgaswarn("4-1","油枪报警");
            }
            Flag_Delay_State[3][0] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_4_1->setStyleSheet("border-image: url(:/picture/gasgun_yello_L.png);");
            if(Flag_Delay_State[3][0] != 2)
            {
                add_value_reoilgaswarn("4-1","油枪预警");
            }
            Flag_Delay_State[3][0] = 2;
        }
    }
    else
    {
        ui->label_gun_4_1->setStyleSheet("border-image: url(:/picture/gasgun_green_L.png);");
        if(Flag_Delay_State[3][0] != 0)
        {
            add_value_reoilgaswarn("4-1","油枪正常");
        }
        Flag_Delay_State[3][0] = 0;
    }
    //4-2
    if(Flag_Accumto[3][1] > 0)
    {

        if(Flag_Accumto[3][1] >= WarnAL_Days)
        {
            ui->label_gun_4_2->setStyleSheet("border-image: url(:/picture/gasgun_red_R.png);");
            if(Flag_Delay_State[3][1] != 1)
            {
                add_value_reoilgaswarn("4-2","油枪报警");
            }
            Flag_Delay_State[3][1] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_4_2->setStyleSheet("border-image: url(:/picture/gasgun_yello_R.png);");
            if(Flag_Delay_State[3][1] != 2)
            {
                add_value_reoilgaswarn("4-2","油枪预警");
            }
            Flag_Delay_State[3][1] = 2;
        }
    }
    else
    {
        ui->label_gun_4_2->setStyleSheet("border-image: url(:/picture/gasgun_green_R.png);");
        if(Flag_Delay_State[3][1] != 0)
        {
            add_value_reoilgaswarn("4-2","油枪正常");
        }
        Flag_Delay_State[3][1] = 0;
    }
    //4-3
    if(Flag_Accumto[3][2] > 0)
    {

        if(Flag_Accumto[3][2] >= WarnAL_Days)
        {
            ui->label_gun_4_3->setStyleSheet("border-image: url(:/picture/gasgun_red_L.png);");
            if(Flag_Delay_State[3][2] != 1)
            {
                add_value_reoilgaswarn("4-3","油枪报警");
            }
            Flag_Delay_State[3][2] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_4_3->setStyleSheet("border-image: url(:/picture/gasgun_yello_L.png);");
            if(Flag_Delay_State[3][2] != 2)
            {
                add_value_reoilgaswarn("4-3","油枪预警");
            }
            Flag_Delay_State[3][2] = 2;
        }
    }
    else
    {
        ui->label_gun_4_3->setStyleSheet("border-image: url(:/picture/gasgun_green_L.png);");
        if(Flag_Delay_State[3][2] != 0)
        {
            add_value_reoilgaswarn("4-3","油枪正常");
        }
        Flag_Delay_State[3][2] = 0;
    }
    //4-4
    if(Flag_Accumto[3][3] > 0)
    {

        if(Flag_Accumto[3][3] >= WarnAL_Days)
        {
            ui->label_gun_4_4->setStyleSheet("border-image: url(:/picture/gasgun_red_R.png);");
            if(Flag_Delay_State[3][3] != 1)
            {
                add_value_reoilgaswarn("4-4","油枪报警");
            }
            Flag_Delay_State[3][3] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_4_4->setStyleSheet("border-image: url(:/picture/gasgun_yello_R.png);");
            if(Flag_Delay_State[3][3] != 2)
            {
                add_value_reoilgaswarn("4-4","油枪预警");
            }
            Flag_Delay_State[3][3] = 2;
        }
    }
    else
    {
        ui->label_gun_4_4->setStyleSheet("border-image: url(:/picture/gasgun_green_R.png);");
        if(Flag_Delay_State[3][3] != 0)
        {
            add_value_reoilgaswarn("4-4","油枪正常");
        }
        Flag_Delay_State[3][3] = 0;
    }
    //4-5
    if(Flag_Accumto[3][4] > 0)
    {

        if(Flag_Accumto[3][4] >= WarnAL_Days)
        {
            ui->label_gun_4_5->setStyleSheet("border-image: url(:/picture/gasgun_red_L.png);");
            if(Flag_Delay_State[3][4] != 1)
            {
                add_value_reoilgaswarn("4-5","油枪报警");
            }
            Flag_Delay_State[3][4] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_4_5->setStyleSheet("border-image: url(:/picture/gasgun_yello_L.png);");
            if(Flag_Delay_State[3][4] != 2)
            {
                add_value_reoilgaswarn("4-5","油枪预警");
            }
            Flag_Delay_State[3][4] = 2;
        }
    }
    else
    {
        ui->label_gun_4_5->setStyleSheet("border-image: url(:/picture/gasgun_green_L.png);");
        if(Flag_Delay_State[3][4] != 0)
        {
            add_value_reoilgaswarn("4-5","油枪正常");
        }
        Flag_Delay_State[3][4] = 0;
    }
    //4-6
    if(Flag_Accumto[3][5] > 0)
    {

        if(Flag_Accumto[3][5] >= WarnAL_Days)
        {
            ui->label_gun_4_6->setStyleSheet("border-image: url(:/picture/gasgun_red_R.png);");
            if(Flag_Delay_State[3][5] != 1)
            {
                add_value_reoilgaswarn("4-6","油枪报警");
            }
            Flag_Delay_State[3][5] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_4_6->setStyleSheet("border-image: url(:/picture/gasgun_yello_R.png);");
            if(Flag_Delay_State[3][5] != 2)
            {
                add_value_reoilgaswarn("4-6","油枪预警");
            }
            Flag_Delay_State[3][5] = 2;
        }
    }
    else
    {
        ui->label_gun_4_6->setStyleSheet("border-image: url(:/picture/gasgun_green_R.png);");
        if(Flag_Delay_State[3][5] != 0)
        {
            add_value_reoilgaswarn("4-6","油枪正常");
        }
        Flag_Delay_State[3][5] = 0;
    }
    //4-7
    if(Flag_Accumto[3][6] > 0)
    {

        if(Flag_Accumto[3][6] >= WarnAL_Days)
        {
            ui->label_gun_4_7->setStyleSheet("border-image: url(:/picture/gasgun_red_L.png);");
            if(Flag_Delay_State[3][6] != 1)
            {
                add_value_reoilgaswarn("4-7","油枪报警");
            }
            Flag_Delay_State[3][6] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_4_7->setStyleSheet("border-image: url(:/picture/gasgun_yello_L.png);");
            if(Flag_Delay_State[3][6] != 2)
            {
                add_value_reoilgaswarn("4-7","油枪预警");
            }
            Flag_Delay_State[3][6] = 2;
        }
    }
    else
    {
        ui->label_gun_4_7->setStyleSheet("border-image: url(:/picture/gasgun_green_L.png);");
        if(Flag_Delay_State[3][6] != 0)
        {
            add_value_reoilgaswarn("4-7","油枪正常");
        }
        Flag_Delay_State[3][6] = 0;
    }
    //4-8
    if(Flag_Accumto[3][7] > 0)
    {

        if(Flag_Accumto[3][7] >= WarnAL_Days)
        {
            ui->label_gun_4_8->setStyleSheet("border-image: url(:/picture/gasgun_red_R.png);");
            if(Flag_Delay_State[3][7] != 1)
            {
                add_value_reoilgaswarn("4-8","油枪报警");
            }
            Flag_Delay_State[3][7] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_4_8->setStyleSheet("border-image: url(:/picture/gasgun_yello_R.png);");
            if(Flag_Delay_State[3][7] != 2)
            {
                add_value_reoilgaswarn("4-8","油枪预警");
            }
            Flag_Delay_State[3][7] = 2;
        }
    }
    else
    {
        ui->label_gun_4_8->setStyleSheet("border-image: url(:/picture/gasgun_green_R.png);");
        if(Flag_Delay_State[3][7] != 0)
        {
            add_value_reoilgaswarn("4-8","油枪正常");
        }
        Flag_Delay_State[3][7] = 0;
    }
    //5-1
    if(Flag_Accumto[4][0] > 0)
    {

        if(Flag_Accumto[4][0] >= WarnAL_Days)
        {
            ui->label_gun_5_1->setStyleSheet("border-image: url(:/picture/gasgun_red_L.png);");
            if(Flag_Delay_State[4][0] != 1)
            {
                add_value_reoilgaswarn("5-1","油枪报警");
            }
            Flag_Delay_State[4][0] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_5_1->setStyleSheet("border-image: url(:/picture/gasgun_yello_L.png);");
            if(Flag_Delay_State[4][0] != 2)
            {
                add_value_reoilgaswarn("5-1","油枪预警");
            }
            Flag_Delay_State[4][0] = 2;
        }
    }
    else
    {
        ui->label_gun_5_1->setStyleSheet("border-image: url(:/picture/gasgun_green_L.png);");
        if(Flag_Delay_State[4][0] != 0)
        {
            add_value_reoilgaswarn("5-1","油枪正常");
        }
        Flag_Delay_State[4][0] = 0;
    }
    //5-2
    if(Flag_Accumto[4][1] > 0)
    {

        if(Flag_Accumto[4][1] >= WarnAL_Days)
        {
            ui->label_gun_5_2->setStyleSheet("border-image: url(:/picture/gasgun_red_R.png);");
            if(Flag_Delay_State[4][1] != 1)
            {
                add_value_reoilgaswarn("5-2","油枪报警");
            }
            Flag_Delay_State[4][1] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_5_2->setStyleSheet("border-image: url(:/picture/gasgun_yello_R.png);");
            if(Flag_Delay_State[4][1] != 2)
            {
                add_value_reoilgaswarn("5-2","油枪预警");
            }
            Flag_Delay_State[4][1] = 2;
        }
    }
    else
    {
        ui->label_gun_5_2->setStyleSheet("border-image: url(:/picture/gasgun_green_R.png);");
        if(Flag_Delay_State[4][1] != 0)
        {
            add_value_reoilgaswarn("5-2","油枪正常");
        }
        Flag_Delay_State[4][1] = 0;
    }
    //5-3
    if(Flag_Accumto[4][2] > 0)
    {

        if(Flag_Accumto[4][2] >= WarnAL_Days)
        {
            ui->label_gun_5_3->setStyleSheet("border-image: url(:/picture/gasgun_red_L.png);");
            if(Flag_Delay_State[4][2] != 1)
            {
                add_value_reoilgaswarn("5-3","油枪报警");
            }
            Flag_Delay_State[4][2] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_5_3->setStyleSheet("border-image: url(:/picture/gasgun_yello_L.png);");
            if(Flag_Delay_State[4][2] != 2)
            {
                add_value_reoilgaswarn("5-3","油枪预警");
            }
            Flag_Delay_State[4][2] = 2;
        }
    }
    else
    {
        ui->label_gun_5_3->setStyleSheet("border-image: url(:/picture/gasgun_green_L.png);");
        if(Flag_Delay_State[4][2] != 0)
        {
            add_value_reoilgaswarn("5-3","油枪正常");
        }
        Flag_Delay_State[4][2] = 0;
    }
    //5-4
    if(Flag_Accumto[4][3] > 0)
    {

        if(Flag_Accumto[4][3] >= WarnAL_Days)
        {
            ui->label_gun_5_4->setStyleSheet("border-image: url(:/picture/gasgun_red_R.png);");
            if(Flag_Delay_State[4][3] != 1)
            {
                add_value_reoilgaswarn("5-4","油枪报警");
            }
            Flag_Delay_State[4][3] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_5_4->setStyleSheet("border-image: url(:/picture/gasgun_yello_R.png);");
            if(Flag_Delay_State[4][3] != 2)
            {
                add_value_reoilgaswarn("5-4","油枪预警");
            }
            Flag_Delay_State[4][3] = 2;
        }
    }
    else
    {
        ui->label_gun_5_4->setStyleSheet("border-image: url(:/picture/gasgun_green_R.png);");
        if(Flag_Delay_State[4][3] != 0)
        {
            add_value_reoilgaswarn("5-4","油枪正常");
        }
        Flag_Delay_State[4][3] = 0;
    }
    //5-5
    if(Flag_Accumto[4][4] > 0)
    {

        if(Flag_Accumto[4][4] >= WarnAL_Days)
        {
            ui->label_gun_5_5->setStyleSheet("border-image: url(:/picture/gasgun_red_L.png);");
            if(Flag_Delay_State[4][4] != 1)
            {
                add_value_reoilgaswarn("5-5","油枪报警");
            }
            Flag_Delay_State[4][4] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_5_5->setStyleSheet("border-image: url(:/picture/gasgun_yello_L.png);");
            if(Flag_Delay_State[4][4] != 2)
            {
                add_value_reoilgaswarn("5-5","油枪预警");
            }
            Flag_Delay_State[4][4] = 2;
        }
    }
    else
    {
        ui->label_gun_5_5->setStyleSheet("border-image: url(:/picture/gasgun_green_L.png);");
        if(Flag_Delay_State[4][4] != 0)
        {
            add_value_reoilgaswarn("5-5","油枪正常");
        }
        Flag_Delay_State[4][4] = 0;
    }
    //5-6
    if(Flag_Accumto[4][5] > 0)
    {

        if(Flag_Accumto[4][5] >= WarnAL_Days)
        {
            ui->label_gun_5_6->setStyleSheet("border-image: url(:/picture/gasgun_red_R.png);");
            if(Flag_Delay_State[4][5] != 1)
            {
                add_value_reoilgaswarn("5-6","油枪报警");
            }
            Flag_Delay_State[4][5] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_5_6->setStyleSheet("border-image: url(:/picture/gasgun_yello_R.png);");
            if(Flag_Delay_State[4][5] != 2)
            {
                add_value_reoilgaswarn("5-6","油枪预警");
            }
            Flag_Delay_State[4][5] = 2;
        }
    }
    else
    {
        ui->label_gun_5_6->setStyleSheet("border-image: url(:/picture/gasgun_green_R.png);");
        if(Flag_Delay_State[4][5] != 0)
        {
            add_value_reoilgaswarn("5-6","油枪正常");
        }
        Flag_Delay_State[4][5] = 0;
    }
    //5-7
    if(Flag_Accumto[4][6] > 0)
    {

        if(Flag_Accumto[4][6] >= WarnAL_Days)
        {
            ui->label_gun_5_7->setStyleSheet("border-image: url(:/picture/gasgun_red_L.png);");
            if(Flag_Delay_State[4][6] != 1)
            {
                add_value_reoilgaswarn("5-7","油枪报警");
            }
            Flag_Delay_State[4][6] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_5_7->setStyleSheet("border-image: url(:/picture/gasgun_yello_L.png);");
            if(Flag_Delay_State[4][6] != 2)
            {
                add_value_reoilgaswarn("5-7","油枪预警");
            }
            Flag_Delay_State[4][6] = 2;
        }
    }
    else
    {
        ui->label_gun_5_7->setStyleSheet("border-image: url(:/picture/gasgun_green_L.png);");
        if(Flag_Delay_State[4][6] != 0)
        {
            add_value_reoilgaswarn("5-7","油枪正常");
        }
        Flag_Delay_State[4][6] = 0;
    }
    //5-8
    if(Flag_Accumto[4][7] > 0)
    {

        if(Flag_Accumto[4][7] >= WarnAL_Days)
        {
            ui->label_gun_5_8->setStyleSheet("border-image: url(:/picture/gasgun_red_R.png);");
            if(Flag_Delay_State[4][7] != 1)
            {
                add_value_reoilgaswarn("5-8","油枪报警");
            }
            Flag_Delay_State[4][7] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_5_8->setStyleSheet("border-image: url(:/picture/gasgun_yello_R.png);");
            if(Flag_Delay_State[4][7] != 2)
            {
                add_value_reoilgaswarn("5-8","油枪预警");
            }
            Flag_Delay_State[4][7] = 2;
        }
    }
    else
    {
        ui->label_gun_5_8->setStyleSheet("border-image: url(:/picture/gasgun_green_R.png);");
        if(Flag_Delay_State[4][7] != 0)
        {
            add_value_reoilgaswarn("5-8","油枪正常");
        }
        Flag_Delay_State[4][7] = 0;
    }

    //6-1
    if(Flag_Accumto[5][0] > 0)
    {

        if(Flag_Accumto[5][0] >= WarnAL_Days)
        {
            ui->label_gun_6_1->setStyleSheet("border-image: url(:/picture/gasgun_red_L.png);");
            if(Flag_Delay_State[5][0] != 1)
            {
                add_value_reoilgaswarn("6-1","油枪报警");
            }
            Flag_Delay_State[5][0] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_6_1->setStyleSheet("border-image: url(:/picture/gasgun_yello_L.png);");
            if(Flag_Delay_State[5][0] != 2)
            {
                add_value_reoilgaswarn("6-1","油枪预警");
            }
            Flag_Delay_State[5][0] = 2;
        }
    }
    else
    {
        ui->label_gun_6_1->setStyleSheet("border-image: url(:/picture/gasgun_green_L.png);");
        if(Flag_Delay_State[5][0] != 0)
        {
            add_value_reoilgaswarn("6-1","油枪正常");
        }
        Flag_Delay_State[5][0] = 0;
    }
    //6-2
    if(Flag_Accumto[5][1] > 0)
    {

        if(Flag_Accumto[5][1] >= WarnAL_Days)
        {
            ui->label_gun_6_2->setStyleSheet("border-image: url(:/picture/gasgun_red_R.png);");
            if(Flag_Delay_State[5][1] != 1)
            {
                add_value_reoilgaswarn("6-2","油枪报警");
            }
            Flag_Delay_State[5][1] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_6_2->setStyleSheet("border-image: url(:/picture/gasgun_yello_R.png);");
            if(Flag_Delay_State[5][1] != 2)
            {
                add_value_reoilgaswarn("6-2","油枪预警");
            }
            Flag_Delay_State[5][1] = 2;
        }
    }
    else
    {
        ui->label_gun_6_2->setStyleSheet("border-image: url(:/picture/gasgun_green_R.png);");
        if(Flag_Delay_State[5][1] != 0)
        {
            add_value_reoilgaswarn("6-2","油枪正常");
        }
        Flag_Delay_State[5][1] = 0;
    }
    //6-3
    if(Flag_Accumto[5][2] > 0)
    {

        if(Flag_Accumto[5][2] >= WarnAL_Days)
        {
            ui->label_gun_6_3->setStyleSheet("border-image: url(:/picture/gasgun_red_L.png);");
            if(Flag_Delay_State[5][2] != 1)
            {
                add_value_reoilgaswarn("6-3","油枪报警");
            }
            Flag_Delay_State[5][2] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_6_3->setStyleSheet("border-image: url(:/picture/gasgun_yello_L.png);");
            if(Flag_Delay_State[5][2] != 2)
            {
                add_value_reoilgaswarn("6-3","油枪预警");
            }
            Flag_Delay_State[5][2] = 2;
        }
    }
    else
    {
        ui->label_gun_6_3->setStyleSheet("border-image: url(:/picture/gasgun_green_L.png);");
        if(Flag_Delay_State[5][2] != 0)
        {
            add_value_reoilgaswarn("6-3","油枪正常");
        }
        Flag_Delay_State[5][2] = 0;
    }
    //6-4
    if(Flag_Accumto[5][3] > 0)
    {

        if(Flag_Accumto[5][3] >= WarnAL_Days)
        {
            ui->label_gun_6_4->setStyleSheet("border-image: url(:/picture/gasgun_red_R.png);");
            if(Flag_Delay_State[5][3] != 1)
            {
                add_value_reoilgaswarn("6-4","油枪报警");
            }
            Flag_Delay_State[5][3] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_6_4->setStyleSheet("border-image: url(:/picture/gasgun_yello_R.png);");
            if(Flag_Delay_State[5][3] != 2)
            {
                add_value_reoilgaswarn("6-4","油枪预警");
            }
            Flag_Delay_State[5][3] = 2;
        }
    }
    else
    {
        ui->label_gun_6_4->setStyleSheet("border-image: url(:/picture/gasgun_green_R.png);");
        if(Flag_Delay_State[5][3] != 0)
        {
            add_value_reoilgaswarn("6-4","油枪正常");
        }
        Flag_Delay_State[5][3] = 0;
    }
    //6-5
    if(Flag_Accumto[5][4] > 0)
    {

        if(Flag_Accumto[5][4] >= WarnAL_Days)
        {
            ui->label_gun_6_5->setStyleSheet("border-image: url(:/picture/gasgun_red_L.png);");
            if(Flag_Delay_State[5][4] != 1)
            {
                add_value_reoilgaswarn("6-5","油枪报警");
            }
            Flag_Delay_State[5][4] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_6_5->setStyleSheet("border-image: url(:/picture/gasgun_yello_L.png);");
            if(Flag_Delay_State[5][4] != 2)
            {
                add_value_reoilgaswarn("6-5","油枪预警");
            }
            Flag_Delay_State[5][4] = 2;
        }
    }
    else
    {
        ui->label_gun_6_5->setStyleSheet("border-image: url(:/picture/gasgun_green_L.png);");
        if(Flag_Delay_State[5][4] != 0)
        {
            add_value_reoilgaswarn("6-5","油枪正常");
        }
        Flag_Delay_State[5][4] = 0;
    }
    //6-6
    if(Flag_Accumto[5][5] > 0)
    {

        if(Flag_Accumto[5][5] >= WarnAL_Days)
        {
            ui->label_gun_6_6->setStyleSheet("border-image: url(:/picture/gasgun_red_R.png);");
            if(Flag_Delay_State[5][5] != 1)
            {
                add_value_reoilgaswarn("6-6","油枪报警");
            }
            Flag_Delay_State[5][5] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_6_6->setStyleSheet("border-image: url(:/picture/gasgun_yello_R.png);");
            if(Flag_Delay_State[5][5] != 2)
            {
                add_value_reoilgaswarn("6-6","油枪预警");
            }
            Flag_Delay_State[5][5] = 2;
        }
    }
    else
    {
        ui->label_gun_6_6->setStyleSheet("border-image: url(:/picture/gasgun_green_R.png);");
        if(Flag_Delay_State[5][5] != 0)
        {
            add_value_reoilgaswarn("6-6","油枪正常");
        }
        Flag_Delay_State[5][5] = 0;
    }
    //6-7
    if(Flag_Accumto[5][6] > 0)
    {

        if(Flag_Accumto[5][6] >= WarnAL_Days)
        {
            ui->label_gun_6_7->setStyleSheet("border-image: url(:/picture/gasgun_red_L.png);");
            if(Flag_Delay_State[5][6] != 1)
            {
                add_value_reoilgaswarn("6-7","油枪报警");
            }
            Flag_Delay_State[5][6] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_6_7->setStyleSheet("border-image: url(:/picture/gasgun_yello_L.png);");
            if(Flag_Delay_State[5][6] != 2)
            {
                add_value_reoilgaswarn("6-7","油枪预警");
            }
            Flag_Delay_State[5][6] = 2;
        }
    }
    else
    {
        ui->label_gun_6_7->setStyleSheet("border-image: url(:/picture/gasgun_green_L.png);");
        if(Flag_Delay_State[5][6] != 0)
        {
            add_value_reoilgaswarn("6-7","油枪正常");
        }
        Flag_Delay_State[5][6] = 0;
    }
    //6-8
    if(Flag_Accumto[5][7] > 0)
    {

        if(Flag_Accumto[5][7] >= WarnAL_Days)
        {
            ui->label_gun_6_8->setStyleSheet("border-image: url(:/picture/gasgun_red_R.png);");
            if(Flag_Delay_State[5][7] != 1)
            {
                add_value_reoilgaswarn("6-8","油枪报警");
            }
            Flag_Delay_State[5][7] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_6_8->setStyleSheet("border-image: url(:/picture/gasgun_yello_R.png);");
            if(Flag_Delay_State[5][7] != 2)
            {
                add_value_reoilgaswarn("6-8","油枪预警");
            }
            Flag_Delay_State[5][7] = 2;
        }
    }
    else
    {
        ui->label_gun_6_8->setStyleSheet("border-image: url(:/picture/gasgun_green_R.png);");
        if(Flag_Delay_State[5][7] != 0)
        {
            add_value_reoilgaswarn("6-8","油枪正常");
        }
        Flag_Delay_State[5][7] = 0;
    }
    //7-1
    if(Flag_Accumto[6][0] > 0)
    {

        if(Flag_Accumto[6][0] >= WarnAL_Days)
        {
            ui->label_gun_7_1->setStyleSheet("border-image: url(:/picture/gasgun_red_L.png);");
            if(Flag_Delay_State[6][0] != 1)
            {
                add_value_reoilgaswarn("7-1","油枪报警");
            }
            Flag_Delay_State[6][0] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_7_1->setStyleSheet("border-image: url(:/picture/gasgun_yello_L.png);");
            if(Flag_Delay_State[6][0] != 2)
            {
                add_value_reoilgaswarn("7-1","油枪预警");
            }
            Flag_Delay_State[6][0] = 2;
        }
    }
    else
    {
        ui->label_gun_7_1->setStyleSheet("border-image: url(:/picture/gasgun_green_L.png);");
        if(Flag_Delay_State[6][0] != 0)
        {
            add_value_reoilgaswarn("7-1","油枪正常");
        }
        Flag_Delay_State[6][0] = 0;
    }
    //7-2
    if(Flag_Accumto[6][1] > 0)
    {

        if(Flag_Accumto[6][1] >= WarnAL_Days)
        {
            ui->label_gun_7_2->setStyleSheet("border-image: url(:/picture/gasgun_red_R.png);");
            if(Flag_Delay_State[6][1] != 1)
            {
                add_value_reoilgaswarn("7-2","油枪报警");
            }
            Flag_Delay_State[6][1] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_7_2->setStyleSheet("border-image: url(:/picture/gasgun_yello_R.png);");
            if(Flag_Delay_State[6][1] != 2)
            {
                add_value_reoilgaswarn("7-2","油枪预警");
            }
            Flag_Delay_State[6][1] = 2;
        }
    }
    else
    {
        ui->label_gun_7_2->setStyleSheet("border-image: url(:/picture/gasgun_green_R.png);");
        if(Flag_Delay_State[6][1] != 0)
        {
            add_value_reoilgaswarn("7-2","油枪正常");
        }
        Flag_Delay_State[6][1] = 0;
    }
    //7-3
    if(Flag_Accumto[6][2] > 0)
    {

        if(Flag_Accumto[6][2] >= WarnAL_Days)
        {
            ui->label_gun_7_3->setStyleSheet("border-image: url(:/picture/gasgun_red_L.png);");
            if(Flag_Delay_State[6][2] != 1)
            {
                add_value_reoilgaswarn("7-3","油枪报警");
            }
            Flag_Delay_State[6][2] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_7_3->setStyleSheet("border-image: url(:/picture/gasgun_yello_L.png);");
            if(Flag_Delay_State[6][2] != 2)
            {
                add_value_reoilgaswarn("7-3","油枪预警");
            }
            Flag_Delay_State[6][2] = 2;
        }
    }
    else
    {
        ui->label_gun_7_3->setStyleSheet("border-image: url(:/picture/gasgun_green_L.png);");
        if(Flag_Delay_State[6][2] != 0)
        {
            add_value_reoilgaswarn("7-3","油枪正常");
        }
        Flag_Delay_State[6][2] = 0;
    }
    //7-4
    if(Flag_Accumto[6][3] > 0)
    {

        if(Flag_Accumto[6][3] >= WarnAL_Days)
        {
            ui->label_gun_7_4->setStyleSheet("border-image: url(:/picture/gasgun_red_R.png);");
            if(Flag_Delay_State[6][3] != 1)
            {
                add_value_reoilgaswarn("7-4","油枪报警");
            }
            Flag_Delay_State[6][3] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_7_4->setStyleSheet("border-image: url(:/picture/gasgun_yello_R.png);");
            if(Flag_Delay_State[6][3] != 2)
            {
                add_value_reoilgaswarn("7-4","油枪预警");
            }
            Flag_Delay_State[6][3] = 2;
        }
    }
    else
    {
        ui->label_gun_7_4->setStyleSheet("border-image: url(:/picture/gasgun_green_R.png);");
        if(Flag_Delay_State[6][3] != 0)
        {
            add_value_reoilgaswarn("7-4","油枪正常");
        }
        Flag_Delay_State[6][3] = 0;
    }
    //7-5
    if(Flag_Accumto[6][4] > 0)
    {

        if(Flag_Accumto[6][4] >= WarnAL_Days)
        {
            ui->label_gun_7_5->setStyleSheet("border-image: url(:/picture/gasgun_red_L.png);");
            if(Flag_Delay_State[6][4] != 1)
            {
                add_value_reoilgaswarn("7-5","油枪报警");
            }
            Flag_Delay_State[6][4] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_7_5->setStyleSheet("border-image: url(:/picture/gasgun_yello_L.png);");
            if(Flag_Delay_State[6][4] != 2)
            {
                add_value_reoilgaswarn("7-5","油枪预警");
            }
            Flag_Delay_State[6][4] = 2;
        }
    }
    else
    {
        ui->label_gun_7_5->setStyleSheet("border-image: url(:/picture/gasgun_green_L.png);");
        if(Flag_Delay_State[6][4] != 0)
        {
            add_value_reoilgaswarn("7-5","油枪正常");
        }
        Flag_Delay_State[6][4] = 0;
    }
    //7-6
    if(Flag_Accumto[6][5] > 0)
    {

        if(Flag_Accumto[6][5] >= WarnAL_Days)
        {
            ui->label_gun_7_6->setStyleSheet("border-image: url(:/picture/gasgun_red_R.png);");
            if(Flag_Delay_State[6][5] != 1)
            {
                add_value_reoilgaswarn("7-6","油枪报警");
            }
            Flag_Delay_State[6][5] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_7_6->setStyleSheet("border-image: url(:/picture/gasgun_yello_R.png);");
            if(Flag_Delay_State[6][5] != 2)
            {
                add_value_reoilgaswarn("7-6","油枪预警");
            }
            Flag_Delay_State[6][5] = 2;
        }
    }
    else
    {
        ui->label_gun_7_6->setStyleSheet("border-image: url(:/picture/gasgun_green_R.png);");
        if(Flag_Delay_State[6][5] != 0)
        {
            add_value_reoilgaswarn("7-6","油枪正常");
        }
        Flag_Delay_State[6][5] = 0;
    }
    //7-7
    if(Flag_Accumto[6][6] > 0)
    {

        if(Flag_Accumto[6][6] >= WarnAL_Days)
        {
            ui->label_gun_7_7->setStyleSheet("border-image: url(:/picture/gasgun_red_L.png);");
            if(Flag_Delay_State[6][6] != 1)
            {
                add_value_reoilgaswarn("7-7","油枪报警");
            }
            Flag_Delay_State[6][6] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_7_7->setStyleSheet("border-image: url(:/picture/gasgun_yello_L.png);");
            if(Flag_Delay_State[6][6] != 2)
            {
                add_value_reoilgaswarn("7-7","油枪预警");
            }
            Flag_Delay_State[6][6] = 2;
        }
    }
    else
    {
        ui->label_gun_7_7->setStyleSheet("border-image: url(:/picture/gasgun_green_L.png);");
        if(Flag_Delay_State[6][6] != 0)
        {
            add_value_reoilgaswarn("7-7","油枪正常");
        }
        Flag_Delay_State[6][6] = 0;
    }
    //7-8
    if(Flag_Accumto[6][7] > 0)
    {

        if(Flag_Accumto[6][7] >= WarnAL_Days)
        {
            ui->label_gun_7_8->setStyleSheet("border-image: url(:/picture/gasgun_red_R.png);");
            if(Flag_Delay_State[6][7] != 1)
            {
                add_value_reoilgaswarn("7-8","油枪报警");
            }
            Flag_Delay_State[6][7] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_7_8->setStyleSheet("border-image: url(:/picture/gasgun_yello_R.png);");
            if(Flag_Delay_State[6][7] != 2)
            {
                add_value_reoilgaswarn("7-8","油枪预警");
            }
            Flag_Delay_State[6][7] = 2;
        }
    }
    else
    {
        ui->label_gun_7_8->setStyleSheet("border-image: url(:/picture/gasgun_green_R.png);");
        if(Flag_Delay_State[6][7] != 0)
        {
            add_value_reoilgaswarn("7-8","油枪正常");
        }
        Flag_Delay_State[6][7] = 0;
    }
    //8-1
    if(Flag_Accumto[7][0] > 0)
    {

        if(Flag_Accumto[7][0] >= WarnAL_Days)
        {
            ui->label_gun_8_1->setStyleSheet("border-image: url(:/picture/gasgun_red_L.png);");
            if(Flag_Delay_State[7][0] != 1)
            {
                add_value_reoilgaswarn("8-1","油枪报警");
            }
            Flag_Delay_State[7][0] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_8_1->setStyleSheet("border-image: url(:/picture/gasgun_yello_L.png);");
            if(Flag_Delay_State[7][0] != 2)
            {
                add_value_reoilgaswarn("8-1","油枪预警");
            }
            Flag_Delay_State[7][0] = 2;
        }
    }
    else
    {
        ui->label_gun_8_1->setStyleSheet("border-image: url(:/picture/gasgun_green_L.png);");
        if(Flag_Delay_State[7][0] != 0)
        {
            add_value_reoilgaswarn("8-1","油枪正常");
        }
        Flag_Delay_State[7][0] = 0;
    }
    //8-2
    if(Flag_Accumto[7][1] > 0)
    {

        if(Flag_Accumto[7][1] >= WarnAL_Days)
        {
            ui->label_gun_8_2->setStyleSheet("border-image: url(:/picture/gasgun_red_R.png);");
            if(Flag_Delay_State[7][1] != 1)
            {
                add_value_reoilgaswarn("8-2","油枪报警");
            }
            Flag_Delay_State[7][1] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_8_2->setStyleSheet("border-image: url(:/picture/gasgun_yello_R.png);");
            if(Flag_Delay_State[7][1] != 2)
            {
                add_value_reoilgaswarn("8-2","油枪预警");
            }
            Flag_Delay_State[7][1] = 2;
        }
    }
    else
    {
        ui->label_gun_8_2->setStyleSheet("border-image: url(:/picture/gasgun_green_R.png);");
        if(Flag_Delay_State[7][1] != 0)
        {
            add_value_reoilgaswarn("8-2","油枪正常");
        }
        Flag_Delay_State[7][1] = 0;
    }
    //8-3
    if(Flag_Accumto[7][2] > 0)
    {

        if(Flag_Accumto[7][2] >= WarnAL_Days)
        {
            ui->label_gun_8_3->setStyleSheet("border-image: url(:/picture/gasgun_red_L.png);");
            if(Flag_Delay_State[7][2] != 1)
            {
                add_value_reoilgaswarn("8-3","油枪报警");
            }
            Flag_Delay_State[7][2] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_8_3->setStyleSheet("border-image: url(:/picture/gasgun_yello_L.png);");
            if(Flag_Delay_State[7][2] != 2)
            {
                add_value_reoilgaswarn("8-3","油枪预警");
            }
            Flag_Delay_State[7][2] = 2;
        }
    }
    else
    {
        ui->label_gun_8_3->setStyleSheet("border-image: url(:/picture/gasgun_green_L.png);");
        if(Flag_Delay_State[7][2] != 0)
        {
            add_value_reoilgaswarn("8-3","油枪正常");
        }
        Flag_Delay_State[7][2] = 0;
    }
    //8-4
    if(Flag_Accumto[7][3] > 0)
    {

        if(Flag_Accumto[7][3] >= WarnAL_Days)
        {
            ui->label_gun_8_4->setStyleSheet("border-image: url(:/picture/gasgun_red_R.png);");
            if(Flag_Delay_State[7][3] != 1)
            {
                add_value_reoilgaswarn("8-4","油枪报警");
            }
            Flag_Delay_State[7][3] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_8_4->setStyleSheet("border-image: url(:/picture/gasgun_yello_R.png);");
            if(Flag_Delay_State[7][3] != 2)
            {
                add_value_reoilgaswarn("8-4","油枪预警");
            }
            Flag_Delay_State[7][3] = 2;
        }
    }
    else
    {
        ui->label_gun_8_4->setStyleSheet("border-image: url(:/picture/gasgun_green_R.png);");
        if(Flag_Delay_State[7][3] != 0)
        {
            add_value_reoilgaswarn("8-4","油枪正常");
        }
        Flag_Delay_State[7][3] = 0;
    }
    //8-5
    if(Flag_Accumto[7][4] > 0)
    {

        if(Flag_Accumto[7][4] >= WarnAL_Days)
        {
            ui->label_gun_8_5->setStyleSheet("border-image: url(:/picture/gasgun_red_L.png);");
            if(Flag_Delay_State[7][4] != 1)
            {
                add_value_reoilgaswarn("8-5","油枪报警");
            }
            Flag_Delay_State[7][4] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_8_5->setStyleSheet("border-image: url(:/picture/gasgun_yello_L.png);");
            if(Flag_Delay_State[7][4] != 2)
            {
                add_value_reoilgaswarn("8-5","油枪预警");
            }
            Flag_Delay_State[7][4] = 2;
        }
    }
    else
    {
        ui->label_gun_8_5->setStyleSheet("border-image: url(:/picture/gasgun_green_L.png);");
        if(Flag_Delay_State[7][4] != 0)
        {
            add_value_reoilgaswarn("8-5","油枪正常");
        }
        Flag_Delay_State[7][4] = 0;
    }
    //8-6
    if(Flag_Accumto[7][5] > 0)
    {

        if(Flag_Accumto[7][5] >= WarnAL_Days)
        {
            ui->label_gun_8_6->setStyleSheet("border-image: url(:/picture/gasgun_red_R.png);");
            if(Flag_Delay_State[7][5] != 1)
            {
                add_value_reoilgaswarn("8-6","油枪报警");
            }
            Flag_Delay_State[7][5] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_8_6->setStyleSheet("border-image: url(:/picture/gasgun_yello_R.png);");
            if(Flag_Delay_State[7][5] != 2)
            {
                add_value_reoilgaswarn("8-6","油枪预警");
            }
            Flag_Delay_State[7][5] = 2;
        }
    }
    else
    {
        ui->label_gun_8_6->setStyleSheet("border-image: url(:/picture/gasgun_green_R.png);");
        if(Flag_Delay_State[7][5] != 0)
        {
            add_value_reoilgaswarn("8-6","油枪正常");
        }
        Flag_Delay_State[7][5] = 0;
    }
    //8-7
    if(Flag_Accumto[7][6] > 0)
    {

        if(Flag_Accumto[7][6] >= WarnAL_Days)
        {
            ui->label_gun_8_7->setStyleSheet("border-image: url(:/picture/gasgun_red_L.png);");
            if(Flag_Delay_State[7][6] != 1)
            {
                add_value_reoilgaswarn("8-7","油枪报警");
            }
            Flag_Delay_State[7][6] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_8_7->setStyleSheet("border-image: url(:/picture/gasgun_yello_L.png);");
            if(Flag_Delay_State[7][6] != 2)
            {
                add_value_reoilgaswarn("8-7","油枪预警");
            }
            Flag_Delay_State[7][6] = 2;
        }
    }
    else
    {
        ui->label_gun_8_7->setStyleSheet("border-image: url(:/picture/gasgun_green_L.png);");
        if(Flag_Delay_State[7][6] != 0)
        {
            add_value_reoilgaswarn("8-7","油枪正常");
        }
        Flag_Delay_State[7][6] = 0;
    }
    //8-8
    if(Flag_Accumto[7][7] > 0)
    {

        if(Flag_Accumto[7][7] >= WarnAL_Days)
        {
            ui->label_gun_8_8->setStyleSheet("border-image: url(:/picture/gasgun_red_R.png);");
            if(Flag_Delay_State[7][7] != 1)
            {
                add_value_reoilgaswarn("8-8","油枪报警");
            }
            Flag_Delay_State[7][7] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_8_8->setStyleSheet("border-image: url(:/picture/gasgun_yello_R.png);");
            if(Flag_Delay_State[7][7] != 2)
            {
                add_value_reoilgaswarn("8-8","油枪预警");
            }
            Flag_Delay_State[7][7] = 2;
        }
    }
    else
    {
        ui->label_gun_8_8->setStyleSheet("border-image: url(:/picture/gasgun_green_R.png);");
        if(Flag_Delay_State[7][7] != 0)
        {
            add_value_reoilgaswarn("8-8","油枪正常");
        }
        Flag_Delay_State[7][7] = 0;
    }
    //9-1
    if(Flag_Accumto[8][0] > 0)
    {

        if(Flag_Accumto[8][0] >= WarnAL_Days)
        {
            ui->label_gun_9_1->setStyleSheet("border-image: url(:/picture/gasgun_red_L.png);");
            if(Flag_Delay_State[8][0] != 1)
            {
                add_value_reoilgaswarn("9-1","油枪报警");
            }
            Flag_Delay_State[8][0] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_9_1->setStyleSheet("border-image: url(:/picture/gasgun_yello_L.png);");
            if(Flag_Delay_State[8][0] != 2)
            {
                add_value_reoilgaswarn("9-1","油枪预警");
            }
            Flag_Delay_State[8][0] = 2;
        }
    }
    else
    {
        ui->label_gun_9_1->setStyleSheet("border-image: url(:/picture/gasgun_green_L.png);");
        if(Flag_Delay_State[8][0] != 0)
        {
            add_value_reoilgaswarn("9-1","油枪正常");
        }
        Flag_Delay_State[8][0] = 0;
    }
    //9-2
    if(Flag_Accumto[8][1] > 0)
    {

        if(Flag_Accumto[8][1] >= WarnAL_Days)
        {
            ui->label_gun_9_2->setStyleSheet("border-image: url(:/picture/gasgun_red_R.png);");
            if(Flag_Delay_State[8][1] != 1)
            {
                add_value_reoilgaswarn("9-2","油枪报警");
            }
            Flag_Delay_State[8][1] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_9_2->setStyleSheet("border-image: url(:/picture/gasgun_yello_R.png);");
            if(Flag_Delay_State[8][1] != 2)
            {
                add_value_reoilgaswarn("9-2","油枪预警");
            }
            Flag_Delay_State[8][1] = 2;
        }
    }
    else
    {
        ui->label_gun_9_2->setStyleSheet("border-image: url(:/picture/gasgun_green_R.png);");
        if(Flag_Delay_State[8][1] != 0)
        {
            add_value_reoilgaswarn("9-2","油枪正常");
        }
        Flag_Delay_State[8][1] = 0;
    }
    //9-3
    if(Flag_Accumto[8][2] > 0)
    {

        if(Flag_Accumto[8][2] >= WarnAL_Days)
        {
            ui->label_gun_9_3->setStyleSheet("border-image: url(:/picture/gasgun_red_L.png);");
            if(Flag_Delay_State[8][2] != 1)
            {
                add_value_reoilgaswarn("9-3","油枪报警");
            }
            Flag_Delay_State[8][2] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_9_3->setStyleSheet("border-image: url(:/picture/gasgun_yello_L.png);");
            if(Flag_Delay_State[8][2] != 2)
            {
                add_value_reoilgaswarn("9-3","油枪预警");
            }
            Flag_Delay_State[8][2] = 2;
        }
    }
    else
    {
        ui->label_gun_9_3->setStyleSheet("border-image: url(:/picture/gasgun_green_L.png);");
        if(Flag_Delay_State[8][2] != 0)
        {
            add_value_reoilgaswarn("9-3","油枪正常");
        }
        Flag_Delay_State[8][2] = 0;
    }
    //9-4
    if(Flag_Accumto[8][3] > 0)
    {

        if(Flag_Accumto[8][3] >= WarnAL_Days)
        {
            ui->label_gun_9_4->setStyleSheet("border-image: url(:/picture/gasgun_red_R.png);");
            if(Flag_Delay_State[8][3] != 1)
            {
                add_value_reoilgaswarn("9-4","油枪报警");
            }
            Flag_Delay_State[8][3] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_9_4->setStyleSheet("border-image: url(:/picture/gasgun_yello_R.png);");
            if(Flag_Delay_State[8][3] != 2)
            {
                add_value_reoilgaswarn("9-4","油枪预警");
            }
            Flag_Delay_State[8][3] = 2;
        }
    }
    else
    {
        ui->label_gun_9_4->setStyleSheet("border-image: url(:/picture/gasgun_green_R.png);");
        if(Flag_Delay_State[8][3] != 0)
        {
            add_value_reoilgaswarn("9-4","油枪正常");
        }
        Flag_Delay_State[8][3] = 0;
    }
    //9-5
    if(Flag_Accumto[8][4] > 0)
    {

        if(Flag_Accumto[8][4] >= WarnAL_Days)
        {
            ui->label_gun_9_5->setStyleSheet("border-image: url(:/picture/gasgun_red_L.png);");
            if(Flag_Delay_State[8][4] != 1)
            {
                add_value_reoilgaswarn("9-5","油枪报警");
            }
            Flag_Delay_State[8][4] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_9_5->setStyleSheet("border-image: url(:/picture/gasgun_yello_L.png);");
            if(Flag_Delay_State[8][4] != 2)
            {
                add_value_reoilgaswarn("9-5","油枪预警");
            }
            Flag_Delay_State[8][4] = 2;
        }
    }
    else
    {
        ui->label_gun_9_5->setStyleSheet("border-image: url(:/picture/gasgun_green_L.png);");
        if(Flag_Delay_State[8][4] != 0)
        {
            add_value_reoilgaswarn("9-5","油枪正常");
        }
        Flag_Delay_State[8][4] = 0;
    }
    //9-6
    if(Flag_Accumto[8][5] > 0)
    {

        if(Flag_Accumto[8][5] >= WarnAL_Days)
        {
            ui->label_gun_9_6->setStyleSheet("border-image: url(:/picture/gasgun_red_R.png);");
            if(Flag_Delay_State[8][5] != 1)
            {
                add_value_reoilgaswarn("9-6","油枪报警");
            }
            Flag_Delay_State[8][5] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_9_6->setStyleSheet("border-image: url(:/picture/gasgun_yello_R.png);");
            if(Flag_Delay_State[8][5] != 2)
            {
                add_value_reoilgaswarn("9-6","油枪预警");
            }
            Flag_Delay_State[8][5] = 2;
        }
    }
    else
    {
        ui->label_gun_9_6->setStyleSheet("border-image: url(:/picture/gasgun_green_R.png);");
        if(Flag_Delay_State[8][5] != 0)
        {
            add_value_reoilgaswarn("9-6","油枪正常");
        }
        Flag_Delay_State[8][5] = 0;
    }
    //9-7
    if(Flag_Accumto[8][6] > 0)
    {

        if(Flag_Accumto[8][6] >= WarnAL_Days)
        {
            ui->label_gun_9_7->setStyleSheet("border-image: url(:/picture/gasgun_red_L.png);");
            if(Flag_Delay_State[8][6] != 1)
            {
                add_value_reoilgaswarn("9-7","油枪报警");
            }
            Flag_Delay_State[8][6] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_9_7->setStyleSheet("border-image: url(:/picture/gasgun_yello_L.png);");
            if(Flag_Delay_State[8][6] != 2)
            {
                add_value_reoilgaswarn("9-7","油枪预警");
            }
            Flag_Delay_State[8][6] = 2;
        }
    }
    else
    {
        ui->label_gun_9_7->setStyleSheet("border-image: url(:/picture/gasgun_green_L.png);");
        if(Flag_Delay_State[8][6] != 0)
        {
            add_value_reoilgaswarn("9-7","油枪正常");
        }
        Flag_Delay_State[8][6] = 0;
    }
    //9-8
    if(Flag_Accumto[8][7] > 0)
    {

        if(Flag_Accumto[8][7] >= WarnAL_Days)
        {
            ui->label_gun_9_8->setStyleSheet("border-image: url(:/picture/gasgun_red_R.png);");
            if(Flag_Delay_State[8][7] != 1)
            {
                add_value_reoilgaswarn("9-8","油枪报警");
            }
            Flag_Delay_State[8][7] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_9_8->setStyleSheet("border-image: url(:/picture/gasgun_yello_R.png);");
            if(Flag_Delay_State[8][7] != 2)
            {
                add_value_reoilgaswarn("9-8","油枪预警");
            }
            Flag_Delay_State[8][7] = 2;
        }
    }
    else
    {
        ui->label_gun_9_8->setStyleSheet("border-image: url(:/picture/gasgun_green_R.png);");
        if(Flag_Delay_State[8][7] != 0)
        {
            add_value_reoilgaswarn("9-8","油枪正常");
        }
        Flag_Delay_State[8][7] = 0;
    }
    //10-1
    if(Flag_Accumto[9][0] > 0)
    {

        if(Flag_Accumto[9][0] >= WarnAL_Days)
        {
            ui->label_gun_10_1->setStyleSheet("border-image: url(:/picture/gasgun_red_L.png);");
            if(Flag_Delay_State[9][0] != 1)
            {
                add_value_reoilgaswarn("10-1","油枪报警");
            }
            Flag_Delay_State[9][0] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_10_1->setStyleSheet("border-image: url(:/picture/gasgun_yello_L.png);");
            if(Flag_Delay_State[9][0] != 2)
            {
                add_value_reoilgaswarn("10-1","油枪预警");
            }
            Flag_Delay_State[9][0] = 2;
        }
    }
    else
    {
        ui->label_gun_10_1->setStyleSheet("border-image: url(:/picture/gasgun_green_L.png);");
        if(Flag_Delay_State[9][0] != 0)
        {
            add_value_reoilgaswarn("10-1","油枪正常");
        }
        Flag_Delay_State[9][0] = 0;
    }
    //10-2
    if(Flag_Accumto[9][1] > 0)
    {

        if(Flag_Accumto[9][1] >= WarnAL_Days)
        {
            ui->label_gun_10_2->setStyleSheet("border-image: url(:/picture/gasgun_red_R.png);");
            if(Flag_Delay_State[9][1] != 1)
            {
                add_value_reoilgaswarn("10-2","油枪报警");
            }
            Flag_Delay_State[9][1] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_10_2->setStyleSheet("border-image: url(:/picture/gasgun_yello_R.png);");
            if(Flag_Delay_State[9][1] != 2)
            {
                add_value_reoilgaswarn("10-2","油枪预警");
            }
            Flag_Delay_State[9][1] = 2;
        }
    }
    else
    {
        ui->label_gun_10_2->setStyleSheet("border-image: url(:/picture/gasgun_green_R.png);");
        if(Flag_Delay_State[9][1] != 0)
        {
            add_value_reoilgaswarn("10-2","油枪正常");
        }
        Flag_Delay_State[9][1] = 0;
    }
    //10-3
    if(Flag_Accumto[9][2] > 0)
    {

        if(Flag_Accumto[9][2] >= WarnAL_Days)
        {
            ui->label_gun_10_3->setStyleSheet("border-image: url(:/picture/gasgun_red_L.png);");
            if(Flag_Delay_State[9][2] != 1)
            {
                add_value_reoilgaswarn("10-3","油枪报警");
            }
            Flag_Delay_State[9][2] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_10_3->setStyleSheet("border-image: url(:/picture/gasgun_yello_L.png);");
            if(Flag_Delay_State[9][2] != 2)
            {
                add_value_reoilgaswarn("10-3","油枪预警");
            }
            Flag_Delay_State[9][2] = 2;
        }
    }
    else
    {
        ui->label_gun_10_3->setStyleSheet("border-image: url(:/picture/gasgun_green_L.png);");
        if(Flag_Delay_State[9][2] != 0)
        {
            add_value_reoilgaswarn("10-3","油枪正常");
        }
        Flag_Delay_State[9][2] = 0;
    }
    //10-4
    if(Flag_Accumto[9][3] > 0)
    {

        if(Flag_Accumto[9][3] >= WarnAL_Days)
        {
            ui->label_gun_10_4->setStyleSheet("border-image: url(:/picture/gasgun_red_R.png);");
            if(Flag_Delay_State[9][3] != 1)
            {
                add_value_reoilgaswarn("10-4","油枪报警");
            }
            Flag_Delay_State[9][3] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_10_4->setStyleSheet("border-image: url(:/picture/gasgun_yello_R.png);");
            if(Flag_Delay_State[9][3] != 2)
            {
                add_value_reoilgaswarn("10-4","油枪预警");
            }
            Flag_Delay_State[9][3] = 2;
        }
    }
    else
    {
        ui->label_gun_10_4->setStyleSheet("border-image: url(:/picture/gasgun_green_R.png);");
        if(Flag_Delay_State[9][3] != 0)
        {
            add_value_reoilgaswarn("10-4","油枪正常");
        }
        Flag_Delay_State[9][3] = 0;
    }
    //10-5
    if(Flag_Accumto[9][4] > 0)
    {

        if(Flag_Accumto[9][4] >= WarnAL_Days)
        {
            ui->label_gun_10_5->setStyleSheet("border-image: url(:/picture/gasgun_red_L.png);");
            if(Flag_Delay_State[9][4] != 1)
            {
                add_value_reoilgaswarn("10-5","油枪报警");
            }
            Flag_Delay_State[9][4] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_10_5->setStyleSheet("border-image: url(:/picture/gasgun_yello_L.png);");
            if(Flag_Delay_State[9][4] != 2)
            {
                add_value_reoilgaswarn("10-5","油枪预警");
            }
            Flag_Delay_State[9][4] = 2;
        }
    }
    else
    {
        ui->label_gun_10_5->setStyleSheet("border-image: url(:/picture/gasgun_green_L.png);");
        if(Flag_Delay_State[9][4] != 0)
        {
            add_value_reoilgaswarn("10-5","油枪正常");
        }
        Flag_Delay_State[9][4] = 0;
    }
    //10-6
    if(Flag_Accumto[9][5] > 0)
    {

        if(Flag_Accumto[9][5] >= WarnAL_Days)
        {
            ui->label_gun_10_6->setStyleSheet("border-image: url(:/picture/gasgun_red_R.png);");
            if(Flag_Delay_State[9][5] != 1)
            {
                add_value_reoilgaswarn("10-6","油枪报警");
            }
            Flag_Delay_State[9][5] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_10_6->setStyleSheet("border-image: url(:/picture/gasgun_yello_R.png);");
            if(Flag_Delay_State[9][5] != 2)
            {
                add_value_reoilgaswarn("10-6","油枪预警");
            }
            Flag_Delay_State[9][5] = 2;
        }
    }
    else
    {
        ui->label_gun_10_6->setStyleSheet("border-image: url(:/picture/gasgun_green_R.png);");
        if(Flag_Delay_State[9][5] != 0)
        {
            add_value_reoilgaswarn("10-6","油枪正常");
        }
        Flag_Delay_State[9][5] = 0;
    }
    //10-7
    if(Flag_Accumto[9][6] > 0)
    {

        if(Flag_Accumto[9][6] >= WarnAL_Days)
        {
            ui->label_gun_10_7->setStyleSheet("border-image: url(:/picture/gasgun_red_L.png);");
            if(Flag_Delay_State[9][6] != 1)
            {
                add_value_reoilgaswarn("10-7","油枪报警");
            }
            Flag_Delay_State[9][6] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_10_7->setStyleSheet("border-image: url(:/picture/gasgun_yello_L.png);");
            if(Flag_Delay_State[9][6] != 2)
            {
                add_value_reoilgaswarn("10-7","油枪预警");
            }
            Flag_Delay_State[9][6] = 2;
        }
    }
    else
    {
        ui->label_gun_10_7->setStyleSheet("border-image: url(:/picture/gasgun_green_L.png);");
        if(Flag_Delay_State[9][6] != 0)
        {
            add_value_reoilgaswarn("10-7","油枪正常");
        }
        Flag_Delay_State[9][6] = 0;
    }
    //10-8
    if(Flag_Accumto[9][7] > 0)
    {

        if(Flag_Accumto[9][7] >= WarnAL_Days)
        {
            ui->label_gun_10_8->setStyleSheet("border-image: url(:/picture/gasgun_red_R.png);");
            if(Flag_Delay_State[9][7] != 1)
            {
                add_value_reoilgaswarn("10-7","油枪报警");
            }
            Flag_Delay_State[9][7] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_10_8->setStyleSheet("border-image: url(:/picture/gasgun_yello_R.png);");
            if(Flag_Delay_State[9][7] != 2)
            {
                add_value_reoilgaswarn("10-8","油枪预警");
            }
            Flag_Delay_State[9][7] = 2;
        }
    }
    else
    {
        ui->label_gun_10_8->setStyleSheet("border-image: url(:/picture/gasgun_green_R.png);");
        if(Flag_Delay_State[9][7] != 0)
        {
            add_value_reoilgaswarn("10-8","油枪正常");
        }
        Flag_Delay_State[9][7] = 0;
    }
    //11-1
    if(Flag_Accumto[10][0] > 0)
    {

        if(Flag_Accumto[10][0] >= WarnAL_Days)
        {
            ui->label_gun_11_1->setStyleSheet("border-image: url(:/picture/gasgun_red_L.png);");
            if(Flag_Delay_State[10][0] != 1)
            {
                add_value_reoilgaswarn("11-1","油枪报警");
            }
            Flag_Delay_State[10][0] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_11_1->setStyleSheet("border-image: url(:/picture/gasgun_yello_L.png);");
            if(Flag_Delay_State[10][0] != 2)
            {
                add_value_reoilgaswarn("11-1","油枪预警");
            }
            Flag_Delay_State[10][0] = 2;
        }
    }
    else
    {
        ui->label_gun_11_1->setStyleSheet("border-image: url(:/picture/gasgun_green_L.png);");
        if(Flag_Delay_State[10][0] != 0)
        {
            add_value_reoilgaswarn("11-1","油枪正常");
        }
        Flag_Delay_State[10][0] = 0;
    }
    //11-2
    if(Flag_Accumto[10][1] > 0)
    {

        if(Flag_Accumto[10][1] >= WarnAL_Days)
        {
            ui->label_gun_11_2->setStyleSheet("border-image: url(:/picture/gasgun_red_R.png);");
            if(Flag_Delay_State[10][1] != 1)
            {
                add_value_reoilgaswarn("11-2","油枪报警");
            }
            Flag_Delay_State[10][1] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_11_2->setStyleSheet("border-image: url(:/picture/gasgun_yello_R.png);");
            if(Flag_Delay_State[10][1] != 2)
            {
                add_value_reoilgaswarn("11-2","油枪预警");
            }
            Flag_Delay_State[10][1] = 2;
        }
    }
    else
    {
        ui->label_gun_11_2->setStyleSheet("border-image: url(:/picture/gasgun_green_R.png);");
        if(Flag_Delay_State[10][1] != 0)
        {
            add_value_reoilgaswarn("11-2","油枪正常");
        }
        Flag_Delay_State[10][1] = 0;
    }
    //11-3
    if(Flag_Accumto[10][2] > 0)
    {

        if(Flag_Accumto[10][2] >= WarnAL_Days)
        {
            ui->label_gun_11_3->setStyleSheet("border-image: url(:/picture/gasgun_red_L.png);");
            if(Flag_Delay_State[10][2] != 1)
            {
                add_value_reoilgaswarn("11-3","油枪报警");
            }
            Flag_Delay_State[10][2] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_11_3->setStyleSheet("border-image: url(:/picture/gasgun_yello_L.png);");
            if(Flag_Delay_State[10][2] != 2)
            {
                add_value_reoilgaswarn("11-3","油枪预警");
            }
            Flag_Delay_State[10][2] = 2;
        }
    }
    else
    {
        ui->label_gun_11_3->setStyleSheet("border-image: url(:/picture/gasgun_green_L.png);");
        if(Flag_Delay_State[10][2] != 0)
        {
            add_value_reoilgaswarn("11-3","油枪正常");
        }
        Flag_Delay_State[10][2] = 0;
    }
    //11-4
    if(Flag_Accumto[10][3] > 0)
    {

        if(Flag_Accumto[10][3] >= WarnAL_Days)
        {
            ui->label_gun_11_4->setStyleSheet("border-image: url(:/picture/gasgun_red_R.png);");
            if(Flag_Delay_State[10][3] != 1)
            {
                add_value_reoilgaswarn("11-4","油枪报警");
            }
            Flag_Delay_State[10][3] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_11_4->setStyleSheet("border-image: url(:/picture/gasgun_yello_R.png);");
            if(Flag_Delay_State[10][3] != 2)
            {
                add_value_reoilgaswarn("11-4","油枪预警");
            }
            Flag_Delay_State[10][3] = 2;
        }
    }
    else
    {
        ui->label_gun_11_4->setStyleSheet("border-image: url(:/picture/gasgun_green_R.png);");
        if(Flag_Delay_State[10][3] != 0)
        {
            add_value_reoilgaswarn("11-4","油枪正常");
        }
        Flag_Delay_State[10][3] = 0;
    }
    //11-5
    if(Flag_Accumto[10][4] > 0)
    {

        if(Flag_Accumto[10][4] >= WarnAL_Days)
        {
            ui->label_gun_11_5->setStyleSheet("border-image: url(:/picture/gasgun_red_L.png);");
            if(Flag_Delay_State[10][4] != 1)
            {
                add_value_reoilgaswarn("11-5","油枪报警");
            }
            Flag_Delay_State[10][4] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_11_5->setStyleSheet("border-image: url(:/picture/gasgun_yello_L.png);");
            if(Flag_Delay_State[10][4] != 2)
            {
                add_value_reoilgaswarn("11-5","油枪预警");
            }
            Flag_Delay_State[10][4] = 2;
        }
    }
    else
    {
        ui->label_gun_11_5->setStyleSheet("border-image: url(:/picture/gasgun_green_L.png);");
        if(Flag_Delay_State[10][4] != 0)
        {
            add_value_reoilgaswarn("11-5","油枪正常");
        }
        Flag_Delay_State[10][4] = 0;
    }
    //11-6
    if(Flag_Accumto[10][5] > 0)
    {

        if(Flag_Accumto[10][5] >= WarnAL_Days)
        {
            ui->label_gun_11_6->setStyleSheet("border-image: url(:/picture/gasgun_red_R.png);");
            if(Flag_Delay_State[10][5] != 1)
            {
                add_value_reoilgaswarn("11-6","油枪报警");
            }
            Flag_Delay_State[10][5] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_11_6->setStyleSheet("border-image: url(:/picture/gasgun_yello_R.png);");
            if(Flag_Delay_State[10][5] != 2)
            {
                add_value_reoilgaswarn("11-6","油枪预警");
            }
            Flag_Delay_State[10][5] = 2;
        }
    }
    else
    {
        ui->label_gun_11_6->setStyleSheet("border-image: url(:/picture/gasgun_green_R.png);");
        if(Flag_Delay_State[10][5] != 0)
        {
            add_value_reoilgaswarn("11-6","油枪正常");
        }
        Flag_Delay_State[10][5] = 0;
    }
    //11-7
    if(Flag_Accumto[10][6] > 0)
    {

        if(Flag_Accumto[10][6] >= WarnAL_Days)
        {
            ui->label_gun_11_7->setStyleSheet("border-image: url(:/picture/gasgun_red_L.png);");
            if(Flag_Delay_State[10][6] != 1)
            {
                add_value_reoilgaswarn("11-7","油枪报警");
            }
            Flag_Delay_State[10][6] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_11_7->setStyleSheet("border-image: url(:/picture/gasgun_yello_L.png);");
            if(Flag_Delay_State[10][6] != 2)
            {
                add_value_reoilgaswarn("11-7","油枪预警");
            }
            Flag_Delay_State[10][6] = 2;
        }
    }
    else
    {
        ui->label_gun_11_7->setStyleSheet("border-image: url(:/picture/gasgun_green_L.png);");
        if(Flag_Delay_State[10][6] != 0)
        {
            add_value_reoilgaswarn("11-7","油枪正常");
        }
        Flag_Delay_State[10][6] = 0;
    }
    //11-8
    if(Flag_Accumto[10][7] > 0)
    {

        if(Flag_Accumto[10][7] >= WarnAL_Days)
        {
            ui->label_gun_11_8->setStyleSheet("border-image: url(:/picture/gasgun_red_R.png);");
            if(Flag_Delay_State[10][7] != 1)
            {
                add_value_reoilgaswarn("11-8","油枪报警");
            }
            Flag_Delay_State[10][7] = 8;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_11_8->setStyleSheet("border-image: url(:/picture/gasgun_yello_R.png);");
            if(Flag_Delay_State[10][7] != 2)
            {
                add_value_reoilgaswarn("11-8","油枪预警");
            }
            Flag_Delay_State[10][7] = 2;
        }
    }
    else
    {
        ui->label_gun_11_8->setStyleSheet("border-image: url(:/picture/gasgun_green_R.png);");
        if(Flag_Delay_State[10][7] != 0)
        {
            add_value_reoilgaswarn("11-8","油枪正常");
        }
        Flag_Delay_State[10][7] = 0;
    }
    //12-1
    if(Flag_Accumto[11][0] > 0)
    {

        if(Flag_Accumto[11][0] >= WarnAL_Days)
        {
            ui->label_gun_12_1->setStyleSheet("border-image: url(:/picture/gasgun_red_L.png);");
            if(Flag_Delay_State[11][0] != 1)
            {
                add_value_reoilgaswarn("12-1","油枪报警");
            }
            Flag_Delay_State[11][0] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_12_1->setStyleSheet("border-image: url(:/picture/gasgun_yello_L.png);");
            if(Flag_Delay_State[11][0] != 2)
            {
                add_value_reoilgaswarn("12-1","油枪预警");
            }
            Flag_Delay_State[11][0] = 2;
        }
    }
    else
    {
        ui->label_gun_12_1->setStyleSheet("border-image: url(:/picture/gasgun_green_L.png);");
        if(Flag_Delay_State[11][0] != 0)
        {
            add_value_reoilgaswarn("12-1","油枪正常");
        }
        Flag_Delay_State[11][0] = 0;
    }
    //12-2
    if(Flag_Accumto[11][1] > 0)
    {

        if(Flag_Accumto[11][1] >= WarnAL_Days)
        {
            ui->label_gun_12_2->setStyleSheet("border-image: url(:/picture/gasgun_red_R.png);");
            if(Flag_Delay_State[11][1] != 1)
            {
                add_value_reoilgaswarn("12-2","油枪报警");
            }
            Flag_Delay_State[11][1] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_12_2->setStyleSheet("border-image: url(:/picture/gasgun_yello_R.png);");
            if(Flag_Delay_State[11][1] != 2)
            {
                add_value_reoilgaswarn("12-2","油枪预警");
            }
            Flag_Delay_State[11][1] = 2;
        }
    }
    else
    {
        ui->label_gun_12_2->setStyleSheet("border-image: url(:/picture/gasgun_green_R.png);");
        if(Flag_Delay_State[11][1] != 0)
        {
            add_value_reoilgaswarn("12-2","油枪正常");
        }
        Flag_Delay_State[11][1] = 0;
    }
    //12-3
    if(Flag_Accumto[11][2] > 0)
    {

        if(Flag_Accumto[11][2] >= WarnAL_Days)
        {
            ui->label_gun_12_3->setStyleSheet("border-image: url(:/picture/gasgun_red_L.png);");
            if(Flag_Delay_State[11][2] != 1)
            {
                add_value_reoilgaswarn("12-3","油枪报警");
            }
            Flag_Delay_State[11][2] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_12_3->setStyleSheet("border-image: url(:/picture/gasgun_yello_L.png);");
            if(Flag_Delay_State[11][2] != 2)
            {
                add_value_reoilgaswarn("12-3","油枪预警");
            }
            Flag_Delay_State[11][2] = 2;
        }
    }
    else
    {
        ui->label_gun_12_3->setStyleSheet("border-image: url(:/picture/gasgun_green_L.png);");
        if(Flag_Delay_State[11][2] != 0)
        {
            add_value_reoilgaswarn("12-3","油枪正常");
        }
        Flag_Delay_State[11][2] = 0;
    }
    //12-4
    if(Flag_Accumto[11][3] > 0)
    {

        if(Flag_Accumto[11][3] >= WarnAL_Days)
        {
            ui->label_gun_12_4->setStyleSheet("border-image: url(:/picture/gasgun_red_R.png);");
            if(Flag_Delay_State[11][3] != 1)
            {
                add_value_reoilgaswarn("12-4","油枪报警");
            }
            Flag_Delay_State[11][3] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_12_4->setStyleSheet("border-image: url(:/picture/gasgun_yello_R.png);");
            if(Flag_Delay_State[11][3] != 2)
            {
                add_value_reoilgaswarn("12-4","油枪预警");
            }
            Flag_Delay_State[11][3] = 2;
        }
    }
    else
    {
        ui->label_gun_12_4->setStyleSheet("border-image: url(:/picture/gasgun_green_R.png);");
        if(Flag_Delay_State[11][3] != 0)
        {
            add_value_reoilgaswarn("12-4","油枪正常");
        }
        Flag_Delay_State[11][3] = 0;
    }
    //12-5
    if(Flag_Accumto[11][4] > 0)
    {

        if(Flag_Accumto[11][4] >= WarnAL_Days)
        {
            ui->label_gun_12_5->setStyleSheet("border-image: url(:/picture/gasgun_red_L.png);");
            if(Flag_Delay_State[11][4] != 1)
            {
                add_value_reoilgaswarn("12-5","油枪报警");
            }
            Flag_Delay_State[11][4] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_12_5->setStyleSheet("border-image: url(:/picture/gasgun_yello_L.png);");
            if(Flag_Delay_State[11][4] != 2)
            {
                add_value_reoilgaswarn("12-5","油枪预警");
            }
            Flag_Delay_State[11][4] = 2;
        }
    }
    else
    {
        ui->label_gun_12_5->setStyleSheet("border-image: url(:/picture/gasgun_green_L.png);");
        if(Flag_Delay_State[11][4] != 0)
        {
            add_value_reoilgaswarn("12-5","油枪正常");
        }
        Flag_Delay_State[11][4] = 0;
    }
    //12-6
    if(Flag_Accumto[11][5] > 0)
    {

        if(Flag_Accumto[11][5] >= WarnAL_Days)
        {
            ui->label_gun_12_6->setStyleSheet("border-image: url(:/picture/gasgun_red_R.png);");
            if(Flag_Delay_State[11][5] != 1)
            {
                add_value_reoilgaswarn("12-6","油枪报警");
            }
            Flag_Delay_State[11][5] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_12_6->setStyleSheet("border-image: url(:/picture/gasgun_yello_R.png);");
            if(Flag_Delay_State[11][5] != 2)
            {
                add_value_reoilgaswarn("12-6","油枪预警");
            }
            Flag_Delay_State[11][5] = 2;
        }
    }
    else
    {
        ui->label_gun_12_6->setStyleSheet("border-image: url(:/picture/gasgun_green_R.png);");
        if(Flag_Delay_State[11][5] != 0)
        {
            add_value_reoilgaswarn("12-6","油枪正常");
        }
        Flag_Delay_State[11][5] = 0;
    }
    //12-7
    if(Flag_Accumto[11][6] > 0)
    {

        if(Flag_Accumto[11][6] >= WarnAL_Days)
        {
            ui->label_gun_12_7->setStyleSheet("border-image: url(:/picture/gasgun_red_L.png);");
            if(Flag_Delay_State[11][6] != 1)
            {
                add_value_reoilgaswarn("12-7","油枪报警");
            }
            Flag_Delay_State[11][6] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_12_7->setStyleSheet("border-image: url(:/picture/gasgun_yello_L.png);");
            if(Flag_Delay_State[11][6] != 2)
            {
                add_value_reoilgaswarn("12-7","油枪预警");
            }
            Flag_Delay_State[11][6] = 2;
        }
    }
    else
    {
        ui->label_gun_12_7->setStyleSheet("border-image: url(:/picture/gasgun_green_L.png);");
        if(Flag_Delay_State[11][6] != 0)
        {
            add_value_reoilgaswarn("12-7","油枪正常");
        }
        Flag_Delay_State[11][6] = 0;
    }
    //12-8
    if(Flag_Accumto[11][7] > 0)
    {

        if(Flag_Accumto[11][7] >= WarnAL_Days)
        {
            ui->label_gun_12_8->setStyleSheet("border-image: url(:/picture/gasgun_red_R.png);");
            if(Flag_Delay_State[11][7] != 1)
            {
                add_value_reoilgaswarn("12-8","油枪报警");
            }
            Flag_Delay_State[11][7] = 1;//关枪继电器使能，再次用来作不重复做报警记录的判断
        }
        else
        {
            ui->label_gun_12_8->setStyleSheet("border-image: url(:/picture/gasgun_yello_R.png);");
            if(Flag_Delay_State[11][7] != 2)
            {
                add_value_reoilgaswarn("12-8","油枪预警");
            }
            Flag_Delay_State[11][7] = 2;
        }
    }
    else
    {
        ui->label_gun_12_8->setStyleSheet("border-image: url(:/picture/gasgun_green_R.png);");
        if(Flag_Delay_State[11][7] != 0)
        {
            add_value_reoilgaswarn("12-8","油枪正常");
        }
        Flag_Delay_State[11][7] = 0;
    }

    unsigned char flag_timeto_5_temp = 0;
    for(unsigned char i = 0;i<Amount_Dispener;i++)
    {
        for(unsigned char j = 0;j<Amount_Gasgun[i];j++)
        {
           flag_timeto_5_temp = flag_timeto_5_temp + Flag_Delay_State[i][j];
        }
    }
    Flag_Timeto_CloseNeeded[5] = flag_timeto_5_temp;
}
void MainWindow::Reoilgas_UartWrong_Maindisped(unsigned char whichone,unsigned char state)
{


    if(Flag_reo_uartwrong[whichone] == state)//0正常，1故障
    {

    }
    else
    {
        //做记录并重新赋值
        Flag_reo_uartwrong[whichone] = state;
        if(state == 1)
        {
            Flag_Show_ReoilgasPop = 1;//弹窗显示，通信故障类
            ReoilgasPop_GunSta[whichone*2] = 10;//通信故障状态
            ReoilgasPop_GunSta[whichone*2+1] = 10;//通信故障状态
			add_value_gunwarn_details(QString::number(whichone*2),GUN_UARTWRONG);
            //post添加
            QString str= QString::number(whichone+1);
            network_Wrongsdata("N",str);//网络数据发送

            switch(whichone)
            {
            case 0: add_value_reoilgaswarn("1-1","通信故障");
                add_value_reoilgaswarn("1-2","通信故障");
                break;
            case 1: add_value_reoilgaswarn("1-3","通信故障");
                add_value_reoilgaswarn("1-4","通信故障");
                break;
            case 2: add_value_reoilgaswarn("1-5","通信故障");
                add_value_reoilgaswarn("1-6","通信故障");
                break;
            case 3: add_value_reoilgaswarn("1-7","通信故障");
                add_value_reoilgaswarn("1-8","通信故障");
                break;
            case 4: add_value_reoilgaswarn("2-1","通信故障");
                add_value_reoilgaswarn("2-2","通信故障");
                break;
            case 5: add_value_reoilgaswarn("2-3","通信故障");
                add_value_reoilgaswarn("2-4","通信故障");
                break;
            case 6: add_value_reoilgaswarn("2-5","通信故障");
                add_value_reoilgaswarn("2-6","通信故障");
                break;
            case 7: add_value_reoilgaswarn("2-7","通信故障");
                add_value_reoilgaswarn("2-8","通信故障");
                break;
            case 8: add_value_reoilgaswarn("3-1","通信故障");
                add_value_reoilgaswarn("3-2","通信故障");
                break;
            case 9: add_value_reoilgaswarn("3-3","通信故障");
                add_value_reoilgaswarn("3-4","通信故障");
                break;
            case 10: add_value_reoilgaswarn("3-5","通信故障");
                add_value_reoilgaswarn("3-6","通信故障");
                break;
            case 11: add_value_reoilgaswarn("3-7","通信故障");
                add_value_reoilgaswarn("3-8","通信故障");
                break;
            case 12: add_value_reoilgaswarn("4-1","通信故障");
                add_value_reoilgaswarn("4-2","通信故障");
                break;
            case 13: add_value_reoilgaswarn("4-3","通信故障");
                add_value_reoilgaswarn("4-4","通信故障");
                break;
            case 14: add_value_reoilgaswarn("4-5","通信故障");
                add_value_reoilgaswarn("4-6","通信故障");
                break;
            case 15: add_value_reoilgaswarn("4-7","通信故障");
                add_value_reoilgaswarn("4-8","通信故障");
                break;
            case 16: add_value_reoilgaswarn("5-1","通信故障");
                add_value_reoilgaswarn("5-2","通信故障");
                break;
            case 17: add_value_reoilgaswarn("5-3","通信故障");
                add_value_reoilgaswarn("5-4","通信故障");
                break;
            case 18: add_value_reoilgaswarn("5-5","通信故障");
                add_value_reoilgaswarn("5-6","通信故障");
                break;
            case 19: add_value_reoilgaswarn("5-7","通信故障");
                add_value_reoilgaswarn("5-8","通信故障");
                break;
            case 20: add_value_reoilgaswarn("6-1","通信故障");
                add_value_reoilgaswarn("6-2","通信故障");
                break;
            case 21: add_value_reoilgaswarn("6-3","通信故障");
                add_value_reoilgaswarn("6-4","通信故障");
                break;
            case 22: add_value_reoilgaswarn("6-5","通信故障");
                add_value_reoilgaswarn("6-6","通信故障");
                break;
            case 23: add_value_reoilgaswarn("6-7","通信故障");
                add_value_reoilgaswarn("6-8","通信故障");
                break;
            case 24: add_value_reoilgaswarn("7-1","通信故障");
                add_value_reoilgaswarn("7-2","通信故障");
                break;
            case 25: add_value_reoilgaswarn("7-3","通信故障");
                add_value_reoilgaswarn("7-4","通信故障");
                break;
            case 26: add_value_reoilgaswarn("7-5","通信故障");
                add_value_reoilgaswarn("7-6","通信故障");
                break;
            case 27: add_value_reoilgaswarn("7-7","通信故障");
                add_value_reoilgaswarn("7-8","通信故障");
                break;
            case 28: add_value_reoilgaswarn("8-1","通信故障");
                add_value_reoilgaswarn("8-2","通信故障");
                break;
            case 29: add_value_reoilgaswarn("8-3","通信故障");
                add_value_reoilgaswarn("8-4","通信故障");
                break;
            case 30: add_value_reoilgaswarn("8-5","通信故障");
                add_value_reoilgaswarn("8-6","通信故障");
                break;
            case 31: add_value_reoilgaswarn("8-7","通信故障");
                add_value_reoilgaswarn("8-8","通信故障");
                break;
            case 32: add_value_reoilgaswarn("9-1","通信故障");
                add_value_reoilgaswarn("9-2","通信故障");
                break;
            case 33: add_value_reoilgaswarn("9-3","通信故障");
                add_value_reoilgaswarn("9-4","通信故障");
                break;
            case 34: add_value_reoilgaswarn("9-5","通信故障");
                add_value_reoilgaswarn("9-6","通信故障");
                break;
            case 35: add_value_reoilgaswarn("9-7","通信故障");
                add_value_reoilgaswarn("9-8","通信故障");
                break;



            case 36: add_value_reoilgaswarn("10-1","通信故障");
                add_value_reoilgaswarn("10-2","通信故障");
                break;
            case 37: add_value_reoilgaswarn("10-3","通信故障");
                add_value_reoilgaswarn("10-4","通信故障");
                break;
            case 38: add_value_reoilgaswarn("10-5","通信故障");
                add_value_reoilgaswarn("10-6","通信故障");
                break;
            case 39: add_value_reoilgaswarn("10-7","通信故障");
                add_value_reoilgaswarn("10-8","通信故障");
                break;
            case 40: add_value_reoilgaswarn("11-1","通信故障");
                add_value_reoilgaswarn("11-2","通信故障");
                break;
            case 41: add_value_reoilgaswarn("11-3","通信故障");
                add_value_reoilgaswarn("11-4","通信故障");
                break;
            case 42: add_value_reoilgaswarn("11-5","通信故障");
                add_value_reoilgaswarn("11-6","通信故障");
                break;
            case 43: add_value_reoilgaswarn("11-7","通信故障");
                add_value_reoilgaswarn("11-8","通信故障");
                break;
            case 44: add_value_reoilgaswarn("12-1","通信故障");
                add_value_reoilgaswarn("12-2","通信故障");
                break;
            case 45: add_value_reoilgaswarn("12-3","通信故障");
                add_value_reoilgaswarn("12-4","通信故障");
                break;
            case 46: add_value_reoilgaswarn("12-5","通信故障");
                add_value_reoilgaswarn("12-6","通信故障");
                break;
            case 47: add_value_reoilgaswarn("12-7","通信故障");
                add_value_reoilgaswarn("12-8","通信故障");
                break;


            }
        }
        else
        {
            if(Flag_Show_ReoilgasPop == 1){Flag_Show_ReoilgasPop = 0;}
			if(ReoilgasPop_GunSta[whichone*2] >= 10)
			{
				ReoilgasPop_GunSta[whichone*2] = 0;//通信正常状态
				ReoilgasPop_GunSta[whichone*2+1] = 0;//通信正常状态
				add_value_gunwarn_details(QString::number(whichone*2),GUN_NORMAL);
			}
            switch(whichone)
            {
            case 0: add_value_reoilgaswarn("1-1","通信正常");
                add_value_reoilgaswarn("1-2","通信正常");
                break;
            case 1: add_value_reoilgaswarn("1-3","通信正常");
                add_value_reoilgaswarn("1-4","通信正常");
                break;
            case 2: add_value_reoilgaswarn("1-5","通信正常");
                add_value_reoilgaswarn("1-6","通信正常");
                break;
            case 3: add_value_reoilgaswarn("1-7","通信正常");
                add_value_reoilgaswarn("1-8","通信正常");
                break;
            case 4: add_value_reoilgaswarn("2-1","通信正常");
                add_value_reoilgaswarn("2-2","通信正常");
                break;
            case 5: add_value_reoilgaswarn("2-3","通信正常");
                add_value_reoilgaswarn("2-4","通信正常");
                break;
            case 6: add_value_reoilgaswarn("2-5","通信正常");
                add_value_reoilgaswarn("2-6","通信正常");
                break;
            case 7: add_value_reoilgaswarn("2-7","通信正常");
                add_value_reoilgaswarn("2-8","通信正常");
                break;
            case 8: add_value_reoilgaswarn("3-1","通信正常");
                add_value_reoilgaswarn("3-2","通信正常");
                break;
            case 9: add_value_reoilgaswarn("3-3","通信正常");
                add_value_reoilgaswarn("3-4","通信正常");
                break;
            case 10: add_value_reoilgaswarn("3-5","通信正常");
                add_value_reoilgaswarn("3-6","通信正常");
                break;
            case 11: add_value_reoilgaswarn("3-7","通信正常");
                add_value_reoilgaswarn("3-8","通信正常");
                break;
            case 12: add_value_reoilgaswarn("4-1","通信正常");
                add_value_reoilgaswarn("4-2","通信正常");
                break;
            case 13: add_value_reoilgaswarn("4-3","通信正常");
                add_value_reoilgaswarn("4-4","通信正常");
                break;
            case 14: add_value_reoilgaswarn("4-5","通信正常");
                add_value_reoilgaswarn("4-6","通信正常");
                break;
            case 15: add_value_reoilgaswarn("4-7","通信正常");
                add_value_reoilgaswarn("4-8","通信正常");
                break;
            case 16: add_value_reoilgaswarn("5-1","通信正常");
                add_value_reoilgaswarn("5-2","通信正常");
                break;
            case 17: add_value_reoilgaswarn("5-3","通信正常");
                add_value_reoilgaswarn("5-4","通信正常");
                break;
            case 18: add_value_reoilgaswarn("5-5","通信正常");
                add_value_reoilgaswarn("5-6","通信正常");
                break;
            case 19: add_value_reoilgaswarn("5-7","通信正常");
                add_value_reoilgaswarn("5-8","通信正常");
                break;
            case 20: add_value_reoilgaswarn("6-1","通信正常");
                add_value_reoilgaswarn("6-2","通信正常");
                break;
            case 21: add_value_reoilgaswarn("6-3","通信正常");
                add_value_reoilgaswarn("6-4","通信正常");
                break;
            case 22: add_value_reoilgaswarn("6-5","通信正常");
                add_value_reoilgaswarn("6-6","通信正常");
                break;
            case 23: add_value_reoilgaswarn("6-7","通信正常");
                add_value_reoilgaswarn("6-8","通信正常");
                break;
            case 24: add_value_reoilgaswarn("7-1","通信正常");
                add_value_reoilgaswarn("7-2","通信正常");
                break;
            case 25: add_value_reoilgaswarn("7-3","通信正常");
                add_value_reoilgaswarn("7-4","通信正常");
                break;
            case 26: add_value_reoilgaswarn("7-5","通信正常");
                add_value_reoilgaswarn("7-6","通信正常");
                break;
            case 27: add_value_reoilgaswarn("7-7","通信正常");
                add_value_reoilgaswarn("7-8","通信正常");
                break;
            case 28: add_value_reoilgaswarn("8-1","通信正常");
                add_value_reoilgaswarn("8-2","通信正常");
                break;
            case 29: add_value_reoilgaswarn("8-3","通信正常");
                add_value_reoilgaswarn("8-4","通信正常");
                break;
            case 30: add_value_reoilgaswarn("8-5","通信正常");
                add_value_reoilgaswarn("8-6","通信正常");
                break;
            case 31: add_value_reoilgaswarn("8-7","通信正常");
                add_value_reoilgaswarn("8-8","通信正常");
                break;
            case 32: add_value_reoilgaswarn("9-1","通信正常");
                add_value_reoilgaswarn("9-2","通信正常");
                break;
            case 33: add_value_reoilgaswarn("9-3","通信正常");
                add_value_reoilgaswarn("9-4","通信正常");
                break;
            case 34: add_value_reoilgaswarn("9-5","通信正常");
                add_value_reoilgaswarn("9-6","通信正常");
                break;
            case 35: add_value_reoilgaswarn("9-7","通信正常");
                add_value_reoilgaswarn("9-8","通信正常");
                break;

            case 36: add_value_reoilgaswarn("10-1","通信正常");
                add_value_reoilgaswarn("10-2","通信正常");
                break;
            case 37: add_value_reoilgaswarn("10-3","通信正常");
                add_value_reoilgaswarn("10-4","通信正常");
                break;
            case 38: add_value_reoilgaswarn("10-5","通信正常");
                add_value_reoilgaswarn("10-6","通信正常");
                break;
            case 39: add_value_reoilgaswarn("10-7","通信正常");
                add_value_reoilgaswarn("10-8","通信正常");
                break;
            case 40: add_value_reoilgaswarn("11-1","通信正常");
                add_value_reoilgaswarn("11-2","通信正常");
                break;
            case 41: add_value_reoilgaswarn("11-3","通信正常");
                add_value_reoilgaswarn("11-4","通信正常");
                break;
            case 42: add_value_reoilgaswarn("11-5","通信正常");
                add_value_reoilgaswarn("11-6","通信正常");
                break;
            case 43: add_value_reoilgaswarn("11-7","通信正常");
                add_value_reoilgaswarn("11-8","通信正常");
                break;
            case 44: add_value_reoilgaswarn("12-1","通信正常");
                add_value_reoilgaswarn("12-2","通信正常");
                break;
            case 45: add_value_reoilgaswarn("12-3","通信正常");
                add_value_reoilgaswarn("12-4","通信正常");
                break;
            case 46: add_value_reoilgaswarn("12-5","通信正常");
                add_value_reoilgaswarn("12-6","通信正常");
                break;
            case 47: add_value_reoilgaswarn("12-7","通信正常");
                add_value_reoilgaswarn("12-8","通信正常");
                break;

            }
        }

    }


    if(state)
    {
        Flag_Timeto_CloseNeeded[2] = 1;
        emit Reoilgas_Factorset_UartWrong(1,whichone);//factor设置肯定会卡在通信故障的地方
        switch(whichone)
        {
            case 0: ui->label_gun_1_1u->setHidden(0);
                    if(Amount_Gasgun[0]>=2)
                    {
                        ui->label_gun_1_2u->setHidden(0);
                    }
                    break;
            case 1: ui->label_gun_1_3u->setHidden(0);
                    if(Amount_Gasgun[0]>=4)
                    {
                        ui->label_gun_1_4u->setHidden(0);
                    }
                    break;
            case 2: ui->label_gun_1_5u->setHidden(0);
                    if(Amount_Gasgun[0]>=6)
                    {
                        ui->label_gun_1_6u->setHidden(0);
                    }
                    break;
            case 3: ui->label_gun_1_7u->setHidden(0);
                    if(Amount_Gasgun[0]>=8)
                    {
                        ui->label_gun_1_8u->setHidden(0);
                    }
                    break;
            case 4: ui->label_gun_2_1u->setHidden(0);
                    if(Amount_Gasgun[1]>=2)
                    {
                        ui->label_gun_2_2u->setHidden(0);
                    }
                    break;
            case 5: ui->label_gun_2_3u->setHidden(0);
                    if(Amount_Gasgun[1]>=4)
                    {
                        ui->label_gun_2_4u->setHidden(0);
                    }
                    break;
            case 6: ui->label_gun_2_5u->setHidden(0);
                    if(Amount_Gasgun[1]>=6)
                    {
                        ui->label_gun_2_6u->setHidden(0);
                    }
                    break;
            case 7: ui->label_gun_2_7u->setHidden(0);
                    if(Amount_Gasgun[1]>=8)
                    {
                        ui->label_gun_2_8u->setHidden(0);
                    }
                    break;
            case 8: ui->label_gun_3_1u->setHidden(0);
                    if(Amount_Gasgun[2]>=2)
                    {
                        ui->label_gun_3_2u->setHidden(0);
                    }

                    break;
            case 9: ui->label_gun_3_3u->setHidden(0);
                    if(Amount_Gasgun[2]>=4)
                    {
                         ui->label_gun_3_4u->setHidden(0);
                    }
                    break;
            case 10: ui->label_gun_3_5u->setHidden(0);
                    if(Amount_Gasgun[2]>=6)
                    {
                        ui->label_gun_3_6u->setHidden(0);
                    }
                    break;
            case 11: ui->label_gun_3_7u->setHidden(0);
                    if(Amount_Gasgun[2]>=8)
                    {
                       ui->label_gun_3_8u->setHidden(0);
                    }
                    break;
            case 12: ui->label_gun_4_1u->setHidden(0);
                    if(Amount_Gasgun[3]>=2)
                    {
                        ui->label_gun_4_2u->setHidden(0);
                    }
                    break;
            case 13: ui->label_gun_4_3u->setHidden(0);
                    if(Amount_Gasgun[3]>=4)
                    {
                        ui->label_gun_4_4u->setHidden(0);
                    }
                    break;
            case 14: ui->label_gun_4_5u->setHidden(0);
                    if(Amount_Gasgun[3]>=6)
                    {
                        ui->label_gun_4_6u->setHidden(0);
                    }
                    break;
            case 15: ui->label_gun_4_7u->setHidden(0);
                    if(Amount_Gasgun[3]>=8)
                    {
                         ui->label_gun_4_8u->setHidden(0);
                    }
                    break;
            case 16: ui->label_gun_5_1u->setHidden(0);
                    if(Amount_Gasgun[4]>=2)
                    {
                       ui->label_gun_5_2u->setHidden(0);
                    }
                    break;
            case 17: ui->label_gun_5_3u->setHidden(0);
                    if(Amount_Gasgun[4]>=4)
                    {
                        ui->label_gun_5_4u->setHidden(0);
                    }
                    break;
            case 18: ui->label_gun_5_5u->setHidden(0);
                    if(Amount_Gasgun[4]>=6)
                    {
                        ui->label_gun_5_6u->setHidden(0);
                    }
                    break;
            case 19: ui->label_gun_5_7u->setHidden(0);
                    if(Amount_Gasgun[4]>=8)
                    {
                       ui->label_gun_5_8u->setHidden(0);
                    }
                    break;
            case 20: ui->label_gun_6_1u->setHidden(0);
                    if(Amount_Gasgun[5]>=2)
                    {
                         ui->label_gun_6_2u->setHidden(0);
                    }
                    break;
            case 21: ui->label_gun_6_3u->setHidden(0);
                    if(Amount_Gasgun[5]>=4)
                    {
                        ui->label_gun_6_4u->setHidden(0);
                    }
                    break;
            case 22: ui->label_gun_6_5u->setHidden(0);
                    if(Amount_Gasgun[5]>=6)
                    {
                        ui->label_gun_6_6u->setHidden(0);
                    }
                    break;
            case 23: ui->label_gun_6_7u->setHidden(0);
                    if(Amount_Gasgun[5]>=8)
                    {
                        ui->label_gun_6_8u->setHidden(0);
                    }
                    break;
            case 24: ui->label_gun_7_1u->setHidden(0);
                    if(Amount_Gasgun[6]>=2)
                    {
                        ui->label_gun_7_2u->setHidden(0);
                    }
                    break;
            case 25: ui->label_gun_7_3u->setHidden(0);
                    if(Amount_Gasgun[6]>=4)
                    {
                        ui->label_gun_7_4u->setHidden(0);
                    }
                    break;
            case 26: ui->label_gun_7_5u->setHidden(0);
                    if(Amount_Gasgun[6]>=6)
                    {
                        ui->label_gun_7_6u->setHidden(0);
                    }
                    break;
            case 27: ui->label_gun_7_7u->setHidden(0);
                    if(Amount_Gasgun[6]>=8)
                    {
                        ui->label_gun_7_8u->setHidden(0);
                    }
                    break;
            case 28: ui->label_gun_8_1u->setHidden(0);
                    if(Amount_Gasgun[7]>=2)
                    {
                        ui->label_gun_8_2u->setHidden(0);
                    }
                    break;
            case 29: ui->label_gun_8_3u->setHidden(0);
                    if(Amount_Gasgun[7]>=4)
                    {
                        ui->label_gun_8_4u->setHidden(0);
                    }
                    break;
            case 30: ui->label_gun_8_5u->setHidden(0);
                    if(Amount_Gasgun[7]>=6)
                    {
                        ui->label_gun_8_6u->setHidden(0);
                    }
                    break;
            case 31: ui->label_gun_8_7u->setHidden(0);
                    if(Amount_Gasgun[7]>=8)
                    {
                        ui->label_gun_8_8u->setHidden(0);
                    }
                    break;
            case 32: ui->label_gun_9_1u->setHidden(0);
                    if(Amount_Gasgun[8]>=2)
                    {
                        ui->label_gun_9_2u->setHidden(0);
                    }
                    break;
            case 33: ui->label_gun_9_3u->setHidden(0);
            if(Amount_Gasgun[8]>=4)
                    {
                        ui->label_gun_9_4u->setHidden(0);
                    }
                    break;
            case 34: ui->label_gun_9_5u->setHidden(0);
                    if(Amount_Gasgun[8]>=6)
                    {
                        ui->label_gun_9_6u->setHidden(0);
                    }
                    break;
            case 35: ui->label_gun_9_7u->setHidden(0);
                    if(Amount_Gasgun[8]>=8)
                    {
                        ui->label_gun_9_8u->setHidden(0);
                    }
                    break;
            case 36: ui->label_gun_10_1u->setHidden(0);
                    if(Amount_Gasgun[9]>=2)
                    {
                        ui->label_gun_10_2u->setHidden(0);
                    }
                    break;
            case 37: ui->label_gun_10_3u->setHidden(0);
                    if(Amount_Gasgun[9]>=4)
                    {
                        ui->label_gun_10_4u->setHidden(0);
                    }
                    break;
            case 38: ui->label_gun_10_5u->setHidden(0);
                    if(Amount_Gasgun[9]>=6)
                    {
                        ui->label_gun_10_6u->setHidden(0);
                    }
                    break;
            case 39: ui->label_gun_10_7u->setHidden(0);
                    if(Amount_Gasgun[9]>=8)
                    {
                        ui->label_gun_10_8u->setHidden(0);
                    }
                    break;
            case 40: ui->label_gun_11_1u->setHidden(0);
                    if(Amount_Gasgun[10]>=2)
                    {
                        ui->label_gun_11_2u->setHidden(0);
                    }
                    break;
            case 41: ui->label_gun_11_3u->setHidden(0);
                    if(Amount_Gasgun[10]>=4)
                    {
                        ui->label_gun_11_4u->setHidden(0);
                    }
                    break;
            case 42: ui->label_gun_11_5u->setHidden(0);
                    if(Amount_Gasgun[10]>=6)
                    {
                        ui->label_gun_11_6u->setHidden(0);
                    }
                    break;
            case 43: ui->label_gun_11_7u->setHidden(0);
                    if(Amount_Gasgun[10]>=8)
                    {
                       ui->label_gun_11_8u->setHidden(0);
                    }
                    break;
            case 44: ui->label_gun_12_1u->setHidden(0);
                    if(Amount_Gasgun[11]>=2)
                    {
                         ui->label_gun_12_2u->setHidden(0);
                    }
                    break;
            case 45: ui->label_gun_12_3u->setHidden(0);
                    if(Amount_Gasgun[11]>=4)
                    {
                         ui->label_gun_12_4u->setHidden(0);
                    }
                    break;
            case 46: ui->label_gun_12_5u->setHidden(0);
                    if(Amount_Gasgun[11]>=6)
                    {
                        ui->label_gun_12_6u->setHidden(0);
                    }
                    break;
            case 47: ui->label_gun_12_7u->setHidden(0);
                    if(Amount_Gasgun[11]>=8)
                    {
                        ui->label_gun_12_8u->setHidden(0);
                    }
                    break;
        }
    }
    else
    {
        Flag_Timeto_CloseNeeded[2] = 0;
        emit Reoilgas_Factorset_UartWrong(0,whichone);//factor设置通信正常后label显示设置中
        switch(whichone)
        {
            case 0: ui->label_gun_1_1u->setHidden(1);
                    ui->label_gun_1_2u->setHidden(1);
                    break;
            case 1: ui->label_gun_1_3u->setHidden(1);
                    ui->label_gun_1_4u->setHidden(1);
                    break;
            case 2: ui->label_gun_1_5u->setHidden(1);
                    ui->label_gun_1_6u->setHidden(1);
                    break;
            case 3: ui->label_gun_1_7u->setHidden(1);
                    ui->label_gun_1_8u->setHidden(1);
                    break;
            case 4: ui->label_gun_2_1u->setHidden(1);
                    ui->label_gun_2_2u->setHidden(1);
                    break;
            case 5: ui->label_gun_2_3u->setHidden(1);
                    ui->label_gun_2_4u->setHidden(1);
                    break;
            case 6: ui->label_gun_2_5u->setHidden(1);
                    ui->label_gun_2_6u->setHidden(1);
                    break;
            case 7: ui->label_gun_2_7u->setHidden(1);
                    ui->label_gun_2_8u->setHidden(1);
                    break;
            case 8: ui->label_gun_3_1u->setHidden(1);
                    ui->label_gun_3_2u->setHidden(1);
                    break;
            case 9: ui->label_gun_3_3u->setHidden(1);
                    ui->label_gun_3_4u->setHidden(1);
                    break;
            case 10: ui->label_gun_3_5u->setHidden(1);
                    ui->label_gun_3_6u->setHidden(1);
                    break;
            case 11: ui->label_gun_3_7u->setHidden(1);
                    ui->label_gun_3_8u->setHidden(1);
                    break;
            case 12: ui->label_gun_4_1u->setHidden(1);
                    ui->label_gun_4_2u->setHidden(1);
                    break;
            case 13: ui->label_gun_4_3u->setHidden(1);
                    ui->label_gun_4_4u->setHidden(1);
                    break;
            case 14: ui->label_gun_4_5u->setHidden(1);
                    ui->label_gun_4_6u->setHidden(1);
                    break;
            case 15: ui->label_gun_4_7u->setHidden(1);
                    ui->label_gun_4_8u->setHidden(1);
                    break;
            case 16: ui->label_gun_5_1u->setHidden(1);
                    ui->label_gun_5_2u->setHidden(1);
                    break;
            case 17: ui->label_gun_5_3u->setHidden(1);
                    ui->label_gun_5_4u->setHidden(1);
                    break;
            case 18: ui->label_gun_5_5u->setHidden(1);
                    ui->label_gun_5_6u->setHidden(1);
                    break;
            case 19: ui->label_gun_5_7u->setHidden(1);
                    ui->label_gun_5_8u->setHidden(1);
                    break;
            case 20: ui->label_gun_6_1u->setHidden(1);
                    ui->label_gun_6_2u->setHidden(1);
                    break;
            case 21: ui->label_gun_6_3u->setHidden(1);
                    ui->label_gun_6_4u->setHidden(1);
                    break;
            case 22: ui->label_gun_6_5u->setHidden(1);
                    ui->label_gun_6_6u->setHidden(1);
                    break;
            case 23: ui->label_gun_6_7u->setHidden(1);
                    ui->label_gun_6_8u->setHidden(1);
                    break;
            case 24: ui->label_gun_7_1u->setHidden(1);
                    ui->label_gun_7_2u->setHidden(1);
                    break;
            case 25: ui->label_gun_7_3u->setHidden(1);
                    ui->label_gun_7_4u->setHidden(1);
                    break;
            case 26: ui->label_gun_7_5u->setHidden(1);
                    ui->label_gun_7_6u->setHidden(1);
                    break;
            case 27: ui->label_gun_7_7u->setHidden(1);
                    ui->label_gun_7_8u->setHidden(1);
                    break;
            case 28: ui->label_gun_8_1u->setHidden(1);
                    ui->label_gun_8_2u->setHidden(1);
                    break;
            case 29: ui->label_gun_8_3u->setHidden(1);
                    ui->label_gun_8_4u->setHidden(1);
                    break;
            case 30: ui->label_gun_8_5u->setHidden(1);
                    ui->label_gun_8_6u->setHidden(1);
                    break;
            case 31: ui->label_gun_8_7u->setHidden(1);
                    ui->label_gun_8_8u->setHidden(1);
                    break;
            case 32: ui->label_gun_9_1u->setHidden(1);
                    ui->label_gun_9_2u->setHidden(1);
                    break;
            case 33: ui->label_gun_9_3u->setHidden(1);
                    ui->label_gun_9_4u->setHidden(1);
                    break;
            case 34: ui->label_gun_9_5u->setHidden(1);
                    ui->label_gun_9_6u->setHidden(1);
                    break;
            case 35: ui->label_gun_9_7u->setHidden(1);
                    ui->label_gun_9_8u->setHidden(1);
                    break;
            case 36: ui->label_gun_10_1u->setHidden(1);
                    ui->label_gun_10_2u->setHidden(1);
                    break;
            case 37: ui->label_gun_10_3u->setHidden(1);
                    ui->label_gun_10_4u->setHidden(1);
                    break;
            case 38: ui->label_gun_10_5u->setHidden(1);
                    ui->label_gun_10_6u->setHidden(1);
                    break;
            case 39: ui->label_gun_10_7u->setHidden(1);
                    ui->label_gun_10_8u->setHidden(1);
                    break;
            case 40: ui->label_gun_11_1u->setHidden(1);
                    ui->label_gun_11_2u->setHidden(1);
                    break;
            case 41: ui->label_gun_11_3u->setHidden(1);
                    ui->label_gun_11_4u->setHidden(1);
                    break;
            case 42: ui->label_gun_11_5u->setHidden(1);
                    ui->label_gun_11_6u->setHidden(1);
                    break;
            case 43: ui->label_gun_11_7u->setHidden(1);
                    ui->label_gun_11_8u->setHidden(1);
                    break;
            case 44: ui->label_gun_12_1u->setHidden(1);
                    ui->label_gun_12_2u->setHidden(1);
                    break;
            case 45: ui->label_gun_12_3u->setHidden(1);
                    ui->label_gun_12_4u->setHidden(1);
                    break;
            case 46: ui->label_gun_12_5u->setHidden(1);
                    ui->label_gun_12_6u->setHidden(1);
                    break;
            case 47: ui->label_gun_12_7u->setHidden(1);
                    ui->label_gun_12_8u->setHidden(1);
                    break;
        }
    }
}
void MainWindow::Reoilgas_Factor_SettedtoSys()
{
    emit Reoilgas_Factor_Setted();
}
void MainWindow::Version_Recv_FromReoilgas(unsigned char high, unsigned char low)
{
    emit Version_To_SystemSet(high,low);
}
void MainWindow::Setinfo_Recv_FromReoilgas(unsigned char factoroil11, unsigned char factoroil12, unsigned char factorgas11, unsigned char factorgas12, unsigned char delay1, unsigned char factoroil21, unsigned char factoroil22, unsigned char factorgas21, unsigned char factorgas22, unsigned char delay2)
{
    emit Setinfo_To_SystemSet(factoroil11,factoroil12,factorgas11,factorgas12,delay1,factoroil21,factoroil22,factorgas21,factorgas22,delay2);
}
//系统密闭性
void MainWindow::Disp_Reoilgas_Env()
{
    if(Pre_tank_en)
    {
        ui->label_oilgas_tankpre->setText(QString::number(Pre[0],'f',2).append("KPa"));
    }
    else
    {
        ui->label_oilgas_tankpre->setText("");
    }
    if(Pre_pipe_en)
    {
        ui->label_oilgas_pipepre->setText(QString::number(Pre[1],'f',2).append("KPa"));
    }
    else
    {
        ui->label_oilgas_pipepre->setText("");
    }
    if(Env_Gas_en)
    {
        ui->label_oilgas_tanknongdu->setText(QString::number(Gas_Concentration_Fga[1]/2.0,'f',2).append("g/m³"));
    }
    else
    {
        ui->label_oilgas_tanknongdu->setText("");
    }
    if(Tem_tank_en)
    {
        ui->label_oilgas_tanktemp->setText(QString::number(Tem[0],'f',2).append("℃"));
    }
    else
    {
        ui->label_oilgas_tanktemp->setText("");
    }
    if(Num_Fga>2)
    {
        ui->label_gasval_1->setText("浓度:"+QString::number(Gas_Concentration_Fga[3],'f',2)+" LEL");
    }
    if(Num_Fga>3)
    {
        ui->label_gasval_2->setText("浓度:"+QString::number(Gas_Concentration_Fga[4],'f',2)+" LEL");
    }
    if(Num_Fga>4)
    {
        ui->label_gasval_3->setText("浓度:"+QString::number(Gas_Concentration_Fga[5],'f',2)+" LEL");
    }
    if(Num_Fga>5)
    {
        ui->label_gasval_4->setText("浓度:"+QString::number(Gas_Concentration_Fga[6],'f',2)+" LEL");
    }


}
void MainWindow::Env_warn_normal_fga(int t)   //正常
{
    switch(t)
    {
        case 1: ui->label_env_tanknongdu->setHidden(1);
                ui->label_oilgas_tanknongdu_2->setHidden(1);
                break;
        case 3: ui->label_burngas_1->setStyleSheet("border-image: url(:/picture/right.png);");
                break;
        case 4: ui->label_burngas_2->setStyleSheet("border-image: url(:/picture/right.png);");
                break;
        case 5: ui->label_burngas_3->setStyleSheet("border-image: url(:/picture/right.png);");
                break;
        case 6: ui->label_burngas_4->setStyleSheet("border-image: url(:/picture/right.png);");
                break;
    }
}
void MainWindow::Env_warn_hig_fga(int t)    //报警
{
    switch(t)
    {
        case 1: ui->label_env_tanknongdu->setHidden(0);
                ui->label_oilgas_tanknongdu_2->setHidden(1);
                ui->label_env_tanknongdu->setStyleSheet("border-image: url(:/picture/jinggao_red.png);");
                break;
        case 3: ui->label_burngas_1->setStyleSheet("border-image: url(:/picture/gashighwarn.png);");
                break;
        case 4: ui->label_burngas_2->setStyleSheet("border-image: url(:/picture/gashighwarn.png);");
                break;
        case 5: ui->label_burngas_3->setStyleSheet("border-image: url(:/picture/gashighwarn.png);");
                break;
        case 6: ui->label_burngas_4->setStyleSheet("border-image: url(:/picture/gashighwarn.png);");
                break;
    }
}
void MainWindow::Env_warn_low_fga(int t)    //预警
{
    switch(t)
    {
        case 1: ui->label_env_tanknongdu->setHidden(0);
                ui->label_oilgas_tanknongdu_2->setHidden(1);
                ui->label_env_tanknongdu->setStyleSheet("border-image: url(:/picture/jinggao.png);");
                break;
        case 3: ui->label_burngas_1->setStyleSheet("border-image: url(:/picture/gaslowwarn.png);");
                break;
        case 4: ui->label_burngas_2->setStyleSheet("border-image: url(:/picture/gaslowwarn.png);");
                break;
        case 5: ui->label_burngas_3->setStyleSheet("border-image: url(:/picture/gaslowwarn.png);");
                break;
        case 6: ui->label_burngas_4->setStyleSheet("border-image: url(:/picture/gaslowwarn.png);");
                break;
    }
}
void MainWindow::Env_warn_sensor_fga(int t)     //传感器故障
{
    switch(t)
    {
        case 1: ui->label_env_tanknongdu->setHidden(0);
                ui->label_oilgas_tanknongdu_2->setHidden(0);
                ui->label_env_tanknongdu->setStyleSheet("border-image: url(:/picture/jinggao_red.png);");
                ui->label_oilgas_tanknongdu_2->setText(QString("%1").arg("传感器故障"));
                break;
	    case 3: ui->label_burngas_1->setStyleSheet("border-image: url(:/picture/sensorwro.png);");
                break;
	    case 4: ui->label_burngas_2->setStyleSheet("border-image: url(:/picture/sensorwro.png);");
                break;
	    case 5: ui->label_burngas_3->setStyleSheet("border-image: url(:/picture/sensorwro.png);");
                break;
	    case 6: ui->label_burngas_4->setStyleSheet("border-image: url(:/picture/sensorwro.png);");
                break;
    }
}
void MainWindow::Env_warn_uart_fga(int t)   //通信故障
{
    switch(t)
    {
        case 1: ui->label_env_tanknongdu->setHidden(0);
                ui->label_oilgas_tanknongdu_2->setHidden(0);
                ui->label_env_tanknongdu->setStyleSheet("border-image: url(:/picture/jinggao_red.png);");
                ui->label_oilgas_tanknongdu_2->setText(QString("%1").arg("通信故障"));
                break;
	    case 3: ui->label_burngas_1->setStyleSheet("border-image: url(:/picture/uartwro.png);");
                break;
	    case 4: ui->label_burngas_2->setStyleSheet("border-image: url(:/picture/uartwro.png);");
                break;
	    case 5: ui->label_burngas_3->setStyleSheet("border-image: url(:/picture/uartwro.png);");
                break;
	    case 6: ui->label_burngas_4->setStyleSheet("border-image: url(:/picture/uartwro.png);");
                break;
    }
}
void MainWindow::Env_warn_sensor_de_fga(int t)  //探测器传感器故障
{
    switch(t)
    {
        case 1: ui->label_env_tanknongdu->setHidden(0);
                ui->label_oilgas_tanknongdu_2->setHidden(0);
                ui->label_env_tanknongdu->setStyleSheet("border-image: url(:/picture/jinggao_red.png);");
                ui->label_oilgas_tanknongdu_2->setText(QString("%1").arg("T传感器故障"));
                break;
	    case 3: ui->label_burngas_1->setStyleSheet("border-image: url(:/picture/sensorwro.png);");
                break;
	    case 4: ui->label_burngas_2->setStyleSheet("border-image: url(:/picture/sensorwro.png);");
                break;
	    case 5: ui->label_burngas_3->setStyleSheet("border-image: url(:/picture/sensorwro.png);");
                break;
	    case 6: ui->label_burngas_4->setStyleSheet("border-image: url(:/picture/sensorwro.png);");
                break;
    }
}

void MainWindow::Env_warn_pre_normal(int t)  //压力正常
{
    if(t==1)
    {
        ui->label_env_tankpre->setHidden(1);
        ui->label_oilgas_tankpre_2->setHidden(1);
		//温度通信正常
        if((Flag_Pressure_Transmitters_Mode == 0)&&(Tem_tank_en == 1))
        {
            ui->label_env_tanktem->setHidden(1);
            ui->label_oilgas_tanktemp_2->setHidden(1);
        }
    }
    if(t==2)
    {
        ui->label_env_pipepre->setHidden(1);
        ui->label_oilgas_pipepre_2->setHidden(1);
    }
}
void MainWindow::Env_warn_pre_pre(int t)    //压力预警
{
    if(t==1)
    {
        ui->label_env_tankpre->setHidden(0);
        ui->label_oilgas_tankpre_2->setHidden(1);
        ui->label_env_tankpre->setStyleSheet("border-image: url(:/picture/jinggao.png);");
		//温度通信正常
		if((Flag_Pressure_Transmitters_Mode == 0)&&(Tem_tank_en == 1))
		{
			ui->label_env_tanktem->setHidden(1);
			ui->label_oilgas_tanktemp_2->setHidden(1);
		}
    }
    if(t==2)
    {
        ui->label_env_pipepre->setHidden(0);
        ui->label_oilgas_pipepre_2->setHidden(1);
        ui->label_env_pipepre->setStyleSheet("border-image: url(:/picture/jinggao.png);");
    }
}
void MainWindow::Env_warn_pre_warn(int t)  //压力报警
{
    if(t==1)
    {
        ui->label_env_tankpre->setHidden(0);
        ui->label_oilgas_tankpre_2->setHidden(1);
        ui->label_env_tankpre->setStyleSheet("border-image: url(:/picture/jinggao_red.png);");
		//温度通信正常
		if((Flag_Pressure_Transmitters_Mode == 0)&&(Tem_tank_en == 1))
		{
			ui->label_env_tanktem->setHidden(1);
			ui->label_oilgas_tanktemp_2->setHidden(1);
		}
    }
    if(t==2)
    {
        ui->label_env_pipepre->setHidden(0);
        ui->label_oilgas_pipepre_2->setHidden(1);
        ui->label_env_pipepre->setStyleSheet("border-image: url(:/picture/jinggao_red.png);");
    }
}
void MainWindow::Env_warn_pre_uart(int t)  //压力通信故障
{
    if(t==1)
    {
        ui->label_env_tankpre->setHidden(0);
        ui->label_oilgas_tankpre_2->setHidden(0);
        ui->label_env_tankpre->setStyleSheet("border-image: url(:/picture/jinggao_red.png);");
        ui->label_oilgas_tankpre_2->setText(QString("%1").arg("通信故障"));
		//温度通信故障
        if((Flag_Pressure_Transmitters_Mode == 0)&&(Tem_tank_en == 1))
        {
            ui->label_env_tanktem->setHidden(0);
            ui->label_env_tanktem->setStyleSheet("border-image: url(:/picture/jinggao_red.png);");
            ui->label_oilgas_tanktemp_2->setHidden(0);
            ui->label_oilgas_tanktemp_2->setText(QString("%1").arg("通信故障"));
        }
    }
    if(t==2)
    {
        ui->label_env_pipepre->setHidden(0);
        ui->label_oilgas_pipepre_2->setHidden(0);
        ui->label_env_pipepre->setStyleSheet("border-image: url(:/picture/jinggao_red.png);");
        ui->label_oilgas_pipepre_2->setText(QString("%1").arg("通信故障"));
    }
}
void MainWindow::Env_warn_uart_tem()//温度通信故障
{
    ui->label_env_tanktem->setHidden(0);
    ui->label_env_tanktem->setStyleSheet("border-image: url(:/picture/jinggao_red.png);");
    ui->label_oilgas_tanktemp_2->setHidden(0);
    ui->label_oilgas_tanktemp_2->setText(QString("%1").arg("通信故障"));
}
void MainWindow::Env_warn_normal_tem()//温度正常
{
    ui->label_env_tanktem->setHidden(1);
    ui->label_oilgas_tanktemp_2->setHidden(1);
}
void MainWindow::Env_Pre_Tank_Close() //油罐压力表关闭
{
    ui->label_env_tankpre->setHidden(1);
    ui->label_oilgas_tankpre->setText("");
    ui->label_oilgas_tankpre_2->setHidden(1);
}
void MainWindow::Env_Pre_Pipe_Close() //管线压力关闭
{
    ui->label_env_pipepre->setHidden(1);
    ui->label_oilgas_pipepre->clear();
    ui->label_oilgas_pipepre_2->setHidden(1);
}
void MainWindow::Env_FGA_Gas1_Close() //油气浓度关闭
{
    ui->label_env_tanknongdu->setHidden(1);
    ui->label_oilgas_tanknongdu->clear();
    ui->label_oilgas_tanknongdu_2->setHidden(1);
}
void MainWindow::Env_Tem_Tank_Close() //温度传感器关闭
{
    ui->label_env_tanktem->setHidden(1);
    ui->label_oilgas_tanktemp->setText("");
    ui->label_oilgas_tanktemp_2->setHidden(1);
}
//可燃气体传感器数量设置
void MainWindow::amount_burngas_setted(int t)
{
    Num_Fga = t + 2;
    if(t>0)
    {
        ui->label_burngas_1->setStyleSheet("border-image: url(:/picture/right.png);");
        ui->label_gasval_1->setHidden(0);
    }
    else
    {
        ui->label_burngas_1->setStyleSheet("border-image: url(:/picture/nosensor.png);");
        ui->label_gasval_1->setHidden(1);
    }
    if(t>1)
    {
        ui->label_burngas_2->setStyleSheet("border-image: url(:/picture/right.png);");
        ui->label_gasval_2->setHidden(0);
    }
    else
    {
        ui->label_burngas_2->setStyleSheet("border-image: url(:/picture/nosensor.png);");
        ui->label_gasval_2->setHidden(1);
    }
    if(t>2)
    {
        ui->label_burngas_3->setStyleSheet("border-image: url(:/picture/right.png);");
        ui->label_gasval_3->setHidden(0);
    }
    else
    {
        ui->label_burngas_3->setStyleSheet("border-image: url(:/picture/nosensor.png);");
        ui->label_gasval_3->setHidden(1);
    }
    if(t>3)
    {
        ui->label_burngas_4->setStyleSheet("border-image: url(:/picture/right.png);");
        ui->label_gasval_4->setHidden(0);
    }
    else
    {
       ui->label_burngas_4->setStyleSheet("border-image: url(:/picture/nosensor.png);");
       ui->label_gasval_4->setHidden(1);
    }
}


//曲线图绘制与导出
void MainWindow::on_toolButton_close_draw_clicked()     //关闭图表（隐藏）
{
    ui->widget_gundetail->setHidden(1);
    ui->label_delay_data->setHidden(1);
}

void MainWindow::on_toolButton_detail_tankpre_clicked()     //油罐压力曲线
{
    //默认显示日曲线
    ui->label_delay_data->setHidden(0);
    qApp->processEvents();
    Flag_Draw_Type = 101;
    on_toolButton_daydata_clicked();
}
void MainWindow::on_toolButton_detail_pipepre_clicked()
{
    //默认显示日曲线
    ui->label_delay_data->setHidden(0);
    qApp->processEvents();
    Flag_Draw_Type = 102;
    on_toolButton_daydata_clicked();
}

void MainWindow::on_toolButton_detail_tanktempr_clicked()   //油罐温度曲线
{
    //默认显示日曲线
    ui->label_delay_data->setHidden(0);
    qApp->processEvents();
    Flag_Draw_Type = 103;
    on_toolButton_daydata_clicked();
}
void MainWindow::on_toolButton_detail_gasvalue_clicked()    //油气浓度曲线
{
    //默认显示日曲线
    ui->label_delay_data->setHidden(0);
    qApp->processEvents();
    Flag_Draw_Type = 104;
    on_toolButton_daydata_clicked();
}

void MainWindow::on_toolButton_gundetail_1_clicked()
{
    int t = ui->label_detail_whichdispen->text().toInt();
    Flag_Draw_Type = (t-1)*8 + 1;
    on_toolButton_daydata_clicked();
}

void MainWindow::on_toolButton_gundetail_2_clicked()
{
    int t = ui->label_detail_whichdispen->text().toInt();
    Flag_Draw_Type = (t-1)*8 + 2;
    on_toolButton_daydata_clicked();
}

void MainWindow::on_toolButton_gundetail_3_clicked()
{
    int t = ui->label_detail_whichdispen->text().toInt();
    Flag_Draw_Type = (t-1)*8 + 3;
    on_toolButton_daydata_clicked();
}

void MainWindow::on_toolButton_gundetail_4_clicked()
{
    int t = ui->label_detail_whichdispen->text().toInt();
    Flag_Draw_Type = (t-1)*8 + 4;
    on_toolButton_daydata_clicked();
}

void MainWindow::on_toolButton_gundetail_5_clicked()
{
    int t = ui->label_detail_whichdispen->text().toInt();
    Flag_Draw_Type = (t-1)*8 + 5;
    on_toolButton_daydata_clicked();
}

void MainWindow::on_toolButton_gundetail_6_clicked()
{
    int t = ui->label_detail_whichdispen->text().toInt();
    Flag_Draw_Type = (t-1)*8 + 6;
    on_toolButton_daydata_clicked();
}
void MainWindow::on_toolButton_gundetail_7_clicked()
{
    int t = ui->label_detail_whichdispen->text().toInt();
    Flag_Draw_Type = (t-1)*8 + 7;
    on_toolButton_daydata_clicked();
}

void MainWindow::on_toolButton_gundetail_8_clicked()
{
    int t = ui->label_detail_whichdispen->text().toInt();
    Flag_Draw_Type = (t-1)*8 + 8;
    on_toolButton_daydata_clicked();
}


void MainWindow::on_toolButton_daydata_clicked()
{
    ui->label_delay_data->setHidden(0);
    qApp->processEvents();
    switch (Flag_Draw_Type)
    {
        case 101:
                Draw_Tank_Pre_Day();
                ui->label_delay_data->setHidden(1);
                break;
        case 102:
                Draw_Pipe_Pre_Day();
                ui->label_delay_data->setHidden(1);
                break;
        case 103:
                Draw_Tank_Tempr_Day();
                ui->label_delay_data->setHidden(1);
                break;
        case 104:
                Draw_Tank_Nongdu_Day();
                ui->label_delay_data->setHidden(1);
                break;
        default:
                printf("flag_draw_type:-----%d\n",Flag_Draw_Type);
                Draw_Oilgun_Al_Day(Flag_Draw_Type);
                ui->label_delay_data->setHidden(1);
    }
}

void MainWindow::on_toolButton_monthdata_clicked()
{
    ui->label_delay_data->setHidden(0);
    qApp->processEvents();
    switch (Flag_Draw_Type)
    {
        case 101:
                Draw_Tank_Pre_Month();
                ui->label_delay_data->setHidden(1);
                break;
        case 102:
                Draw_Pipe_Pre_Month();
                ui->label_delay_data->setHidden(1);
                break;
        case 103:
                Draw_Tank_Tempr_Month();
                ui->label_delay_data->setHidden(1);
                break;
        case 104:
                Draw_Tank_Nongdu_Month();
                ui->label_delay_data->setHidden(1);
                break;
        default:
                printf("flag_draw_type:-----%d\n",Flag_Draw_Type);
                Draw_Oilgun_Al_Month(Flag_Draw_Type);
                ui->label_delay_data->setHidden(1);
    }
}

void MainWindow::Draw_Tank_Tempr_Day()      //油罐温度日曲线
{
    ui->widget_customplot_reoilgasdetail->clearGraphs();
    select_data_oneday_tanktempr();

    unsigned int n = 0;
    unsigned int n_total = 0;
    unsigned int i_graph = 0;
    float y_range_max = Temperature_Day[1][1];
    float y_range_min = Temperature_Day[1][1];
    while(Temperature_Day[n+1][0])
    {
        if(Temperature_Day[n+1][0] > (Temperature_Day[n][0] + 8))
        {
            ui->widget_customplot_reoilgasdetail->addGraph();
            QVector<double> x1(n-n_total),y1(n-n_total);
            for(unsigned int i = 0;i<(n-n_total);i++)
            {
                x1[i] = Temperature_Day[i+1+n_total][0]/60;
                y1[i] = Temperature_Day[i+1+n_total][1];
            }
            ui->widget_customplot_reoilgasdetail->graph(i_graph)->setData(x1,y1);
            i_graph++;
            n_total = n;
        }
        y_range_max = qMax(y_range_max,Temperature_Day[n+1][1]);
        y_range_min = qMin(y_range_min,Temperature_Day[n+1][1]);
        n++;
    }
    ui->widget_customplot_reoilgasdetail->addGraph();
    QVector<double> x1(n-n_total),y1(n-n_total);
    for(unsigned int i = 0;i<(n-n_total);i++)
    {
        x1[i] = Temperature_Day[i+1+n_total][0]/60;
        y1[i] = Temperature_Day[i+1+n_total][1];
        printf("---%f--%f\n",Temperature_Day[i+1+n_total][0],Temperature_Day[i+1+n_total][1]);
    }
    ui->widget_customplot_reoilgasdetail->legend->setVisible(0);
    ui->widget_customplot_reoilgasdetail->graph(i_graph)->setData(x1,y1);
    ui->widget_customplot_reoilgasdetail->xAxis->setLabel(QString("时间/h"));
    ui->widget_customplot_reoilgasdetail->yAxis->setLabel(QString("温度/℃"));
    ui->widget_customplot_reoilgasdetail->xAxis->setRange(0,24);
    ui->widget_customplot_reoilgasdetail->xAxis->setAutoTickStep(false);
    ui->widget_customplot_reoilgasdetail->xAxis->setTickStep(1);
    ui->widget_customplot_reoilgasdetail->yAxis->setRange(y_range_min-5,y_range_max+5);

    ui->widget_customplot_reoilgasdetail->replot();
    ui->widget_gundetail->setHidden(0);
}
void MainWindow::Draw_Tank_Pre_Day()        //油罐压力日曲线
{
    ui->widget_customplot_reoilgasdetail->clearGraphs();
    select_data_oneday_tankpre();

    unsigned int n = 0;
    unsigned int n_total = 0;
    unsigned int i_graph = 0;
    float y_range_max = Temperature_Day[1][1];
    float y_range_min = Temperature_Day[1][1];
    while(Temperature_Day[n+1][0])
    {
        if(Temperature_Day[n+1][0] > (Temperature_Day[n][0] + 8))
        {
            ui->widget_customplot_reoilgasdetail->addGraph();
            QVector<double> x1(n-n_total),y1(n-n_total);
            for(unsigned int i = 0;i<(n-n_total);i++)
            {
                x1[i] = Temperature_Day[i+1+n_total][0]/60;
                y1[i] = Temperature_Day[i+1+n_total][1];
            }
            ui->widget_customplot_reoilgasdetail->graph(i_graph)->setData(x1,y1);
            i_graph++;
            n_total = n;
            printf("in while >\n");
        }
        y_range_max = qMax(y_range_max,Temperature_Day[n+1][1]);
        y_range_min = qMin(y_range_min,Temperature_Day[n+1][1]);
        n++;
    }
    ui->widget_customplot_reoilgasdetail->addGraph();
    QVector<double> x1(n-n_total),y1(n-n_total);
    for(unsigned int i = 0;i<(n-n_total);i++)
    {
        x1[i] = Temperature_Day[i+1+n_total][0]/60;
        y1[i] = Temperature_Day[i+1+n_total][1];
    }
    ui->widget_customplot_reoilgasdetail->legend->setVisible(0);
    ui->widget_customplot_reoilgasdetail->graph(i_graph)->setData(x1,y1);
    ui->widget_customplot_reoilgasdetail->xAxis->setLabel(QString("时间/h"));
    ui->widget_customplot_reoilgasdetail->yAxis->setLabel(QString("压力/KPa"));
    ui->widget_customplot_reoilgasdetail->xAxis->setRange(0,24);
    ui->widget_customplot_reoilgasdetail->xAxis->setAutoTickStep(false);
    ui->widget_customplot_reoilgasdetail->xAxis->setTickStep(1);
    ui->widget_customplot_reoilgasdetail->yAxis->setRange(y_range_min-3,y_range_max+3);

    ui->widget_customplot_reoilgasdetail->replot();
    ui->widget_gundetail->setHidden(0);
}
void MainWindow::Draw_Pipe_Pre_Day()        //管道压力日曲线
{
    ui->widget_customplot_reoilgasdetail->clearGraphs();
    select_data_oneday_pipepre();

    unsigned int n = 0;
    unsigned int n_total = 0;
    unsigned int i_graph = 0;
    float y_range_max = Temperature_Day[1][1];
    float y_range_min = Temperature_Day[1][1];
    while(Temperature_Day[n+1][0])
    {
        if(Temperature_Day[n+1][0] > (Temperature_Day[n][0] + 8))
        {
            ui->widget_customplot_reoilgasdetail->addGraph();
            QVector<double> x1(n-n_total),y1(n-n_total);
            for(unsigned int i = 0;i<(n-n_total);i++)
            {
                x1[i] = Temperature_Day[i+1+n_total][0]/60;
                y1[i] = Temperature_Day[i+1+n_total][1];
            }
            ui->widget_customplot_reoilgasdetail->graph(i_graph)->setData(x1,y1);
            i_graph++;
            n_total = n;
            printf("in while >\n");
        }
        y_range_max = qMax(y_range_max,Temperature_Day[n+1][1]);
        y_range_min = qMin(y_range_min,Temperature_Day[n+1][1]);
        n++;
    }
    ui->widget_customplot_reoilgasdetail->addGraph();
    QVector<double> x1(n-n_total),y1(n-n_total);
    for(unsigned int i = 0;i<(n-n_total);i++)
    {
        x1[i] = Temperature_Day[i+1+n_total][0]/60;
        y1[i] = Temperature_Day[i+1+n_total][1];
    }
    ui->widget_customplot_reoilgasdetail->legend->setVisible(0);
    ui->widget_customplot_reoilgasdetail->graph(i_graph)->setData(x1,y1);
    ui->widget_customplot_reoilgasdetail->xAxis->setLabel(QString("时间/h"));
    ui->widget_customplot_reoilgasdetail->yAxis->setLabel(QString("压力/KPa"));
    ui->widget_customplot_reoilgasdetail->xAxis->setRange(0,24);
    ui->widget_customplot_reoilgasdetail->xAxis->setAutoTickStep(false);
    ui->widget_customplot_reoilgasdetail->xAxis->setTickStep(1);
    ui->widget_customplot_reoilgasdetail->yAxis->setRange(y_range_min-3,y_range_max+3);

    ui->widget_customplot_reoilgasdetail->replot();
    ui->widget_gundetail->setHidden(0);
}
void MainWindow::Draw_Tank_Nongdu_Day()
{
    ui->widget_customplot_reoilgasdetail->clearGraphs();
    select_data_oneday_tanknongdu();

    unsigned int n = 0;
    unsigned int n_total = 0;
    unsigned int i_graph = 0;
    float y_range_max = Temperature_Day[1][1];
    while(Temperature_Day[n+1][0])
    {
        if(Temperature_Day[n+1][0] > (Temperature_Day[n][0] + 8))
        {
            ui->widget_customplot_reoilgasdetail->addGraph();
            QVector<double> x1(n-n_total),y1(n-n_total);
            for(unsigned int i = 0;i<(n-n_total);i++)
            {
                x1[i] = Temperature_Day[i+1+n_total][0]/60;
                y1[i] = Temperature_Day[i+1+n_total][1];
            }
            ui->widget_customplot_reoilgasdetail->graph(i_graph)->setData(x1,y1);
            i_graph++;
            n_total = n;
            printf("in while >\n");
        }
        y_range_max = qMax(y_range_max,Temperature_Day[n+1][1]);
        n++;
    }
    ui->widget_customplot_reoilgasdetail->addGraph();
    QVector<double> x1(n-n_total),y1(n-n_total);
    for(unsigned int i = 0;i<(n-n_total);i++)
    {
        x1[i] = Temperature_Day[i+1+n_total][0]/60;
        y1[i] = Temperature_Day[i+1+n_total][1];
    }
    ui->widget_customplot_reoilgasdetail->legend->setVisible(0);
    ui->widget_customplot_reoilgasdetail->graph(i_graph)->setData(x1,y1);
    ui->widget_customplot_reoilgasdetail->xAxis->setLabel(QString("时间/h"));
    ui->widget_customplot_reoilgasdetail->yAxis->setLabel(QString("浓度/g/m³"));
    ui->widget_customplot_reoilgasdetail->xAxis->setRange(0,24);
    ui->widget_customplot_reoilgasdetail->xAxis->setAutoTickStep(false);
    ui->widget_customplot_reoilgasdetail->xAxis->setTickStep(1);
    ui->widget_customplot_reoilgasdetail->yAxis->setRange(-0.1,y_range_max+5);

    ui->widget_customplot_reoilgasdetail->replot();
    ui->widget_gundetail->setHidden(0);
}

void MainWindow::Draw_Oilgun_Al_Day(unsigned char t)
{
    ui->widget_customplot_reoilgasdetail->clearGraphs();
    select_data_oneday_oilgun_al(t);
    unsigned int n_max = 0;
    unsigned int i_graph = 0;
    //al最大值曲线
    while(AL_Day[n_max][0])
    {
        n_max++;
    }
    if((AL_Day[0][0]==0) && (AL_Day[0][1]>0))
    {
        AL_Day[0][1]-=2;
        n_max++;
    }
    if(n_max)
    {
        ui->widget_customplot_reoilgasdetail->addGraph();
        QVector<double> x1(n_max),y1(n_max);
        for(unsigned int i = 0;i<(n_max);i++)
        {
            x1[i] = AL_Day[i][0]/60;
            y1[i] = AL_Day[i][1];
            printf("x:%f-y:%f\n",x1[i],y1[i]);
        }

        ui->widget_customplot_reoilgasdetail->graph(i_graph)->setData(x1,y1);
    }

    ui->widget_customplot_reoilgasdetail->legend->setVisible(0);
    ui->widget_customplot_reoilgasdetail->xAxis->setRange(0,24);
    ui->widget_customplot_reoilgasdetail->xAxis->setLabel(QString("时间/h"));
    ui->widget_customplot_reoilgasdetail->yAxis->setLabel(QString("A/L"));
    ui->widget_customplot_reoilgasdetail->xAxis->setAutoTickStep(false);
    ui->widget_customplot_reoilgasdetail->xAxis->setTickStep(1);
    ui->widget_customplot_reoilgasdetail->yAxis->setRange(-0.1,2);
    ui->widget_customplot_reoilgasdetail->replot();
    ui->widget_gundetail->setHidden(0);
}

void MainWindow::Draw_Tank_Tempr_Month()    //油罐温度月曲线
{
    ui->widget_customplot_reoilgasdetail->clearGraphs();
    select_data_onemonth_tanktempr();

    unsigned int n= 0;
    unsigned int n_total = 0;
    unsigned int i_graph = 0;
    float y_range_max = Temperature_Month[1][1];
    float y_range_min = Temperature_Month[1][1];
    while(Temperature_Month[n+1][0])
    {
        if(Temperature_Month[n+1][0]>(Temperature_Month[n][0]+1))
        {
            ui->widget_customplot_reoilgasdetail->addGraph();
            QVector<double> x(n-n_total),y(n-n_total);
            for(unsigned int i = 0;i<(n-n_total);i++)
            {
                x[i] = Temperature_Month[i+1+n_total][0]/24;
                y[i] = Temperature_Month[i+1+n_total][1];
            }
            ui->widget_customplot_reoilgasdetail->graph(i_graph)->setData(x,y);
            i_graph++;
            n_total = n;
        }
        y_range_max = qMax(y_range_max,Temperature_Month[n+1][1]);
        y_range_min = qMin(y_range_min,Temperature_Month[n+1][1]);
        n++;
    }
    ui->widget_customplot_reoilgasdetail->addGraph();
    QVector<double> x(n-n_total),y(n-n_total);
    for(unsigned int i = 0;i<(n-n_total);i++)
    {
        x[i] = Temperature_Month[i+1+n_total][0]/24;
        y[i] = Temperature_Month[i+1+n_total][1];
    }
    ui->widget_customplot_reoilgasdetail->legend->setVisible(0);
    ui->widget_customplot_reoilgasdetail->graph(i_graph)->setData(x,y);
    ui->widget_customplot_reoilgasdetail->xAxis->setLabel(QString("时间/天"));
    ui->widget_customplot_reoilgasdetail->yAxis->setLabel(QString("温度/℃"));
    ui->widget_customplot_reoilgasdetail->xAxis->setRange(0,31);
    ui->widget_customplot_reoilgasdetail->xAxis->setAutoTickStep(false);
    ui->widget_customplot_reoilgasdetail->xAxis->setTickStep(1);
    ui->widget_customplot_reoilgasdetail->yAxis->setRange(y_range_min-5,y_range_max+5);

    ui->widget_customplot_reoilgasdetail->replot();
    ui->widget_gundetail->setHidden(0);

}

void MainWindow::Draw_Tank_Pre_Month()  //油罐压力月曲线
{
    ui->widget_customplot_reoilgasdetail->clearGraphs();
    select_data_onemonth_tankpre();

    unsigned int n = 0;
    unsigned int n_total = 0;
    unsigned int i_graph = 0;
    float y_range_max = Temperature_Month[1][1];
    float y_range_min = Temperature_Month[1][1];
    while(Temperature_Month[n+1][0])
    {
        if(Temperature_Month[n+1][0]>(Temperature_Month[n][0]+1))
        {
            ui->widget_customplot_reoilgasdetail->addGraph();
            QVector<double> x(n-n_total),y(n-n_total);
            for(unsigned int i = 0;i<(n-n_total);i++)
            {
                x[i] = Temperature_Month[i+1+n_total][0]/24;
                y[i] = Temperature_Month[i+1+n_total][1];
            }
            ui->widget_customplot_reoilgasdetail->graph(i_graph)->setData(x,y);
            i_graph++;
            n_total = n;
        }
        y_range_max = qMax(y_range_max,Temperature_Month[n+1][1]);
        y_range_min = qMin(y_range_min,Temperature_Month[n+1][1]);
        n++;
    }
    ui->widget_customplot_reoilgasdetail->addGraph();
    QVector<double> x(n-n_total),y(n-n_total);
    for(unsigned int i = 0;i<(n-n_total);i++)
    {
        x[i] = Temperature_Month[i+1+n_total][0]/24;
        y[i] = Temperature_Month[i+1+n_total][1];
    }
    ui->widget_customplot_reoilgasdetail->legend->setVisible(0);
    ui->widget_customplot_reoilgasdetail->graph(i_graph)->setData(x,y);
    ui->widget_customplot_reoilgasdetail->xAxis->setLabel(QString("时间/天"));
    ui->widget_customplot_reoilgasdetail->yAxis->setLabel(QString("压力/KPa"));
    ui->widget_customplot_reoilgasdetail->xAxis->setRange(0,31);
    ui->widget_customplot_reoilgasdetail->xAxis->setAutoTickStep(false);
    ui->widget_customplot_reoilgasdetail->xAxis->setTickStep(1);
    ui->widget_customplot_reoilgasdetail->yAxis->setRange(y_range_min-3,y_range_max+3);

    ui->widget_customplot_reoilgasdetail->replot();
    ui->widget_gundetail->setHidden(0);
}
void MainWindow::Draw_Pipe_Pre_Month()
{
    ui->widget_customplot_reoilgasdetail->clearGraphs();
    select_data_onemonth_pipepre();

    unsigned int n = 0;
    unsigned int n_total = 0;
    unsigned int i_graph = 0;
    float y_range_max = Temperature_Month[1][1];
    float y_range_min = Temperature_Month[1][1];
    while(Temperature_Month[n+1][0])
    {
        if(Temperature_Month[n+1][0]>(Temperature_Month[n][0]+1))
        {
            ui->widget_customplot_reoilgasdetail->addGraph();
            QVector<double> x(n-n_total),y(n-n_total);
            for(unsigned int i = 0;i<(n-n_total);i++)
            {
                x[i] = Temperature_Month[i+1+n_total][0]/24;
                y[i] = Temperature_Month[i+1+n_total][1];
            }
            ui->widget_customplot_reoilgasdetail->graph(i_graph)->setData(x,y);
            i_graph++;
            n_total = n;
        }
        y_range_max = qMax(y_range_max,Temperature_Month[n+1][1]);
        y_range_min = qMin(y_range_min,Temperature_Month[n+1][1]);
        n++;
    }
    ui->widget_customplot_reoilgasdetail->addGraph();
    QVector<double> x(n-n_total),y(n-n_total);
    for(unsigned int i = 0;i<(n-n_total);i++)
    {
        x[i] = Temperature_Month[i+1+n_total][0]/24;
        y[i] = Temperature_Month[i+1+n_total][1];
    }
    ui->widget_customplot_reoilgasdetail->legend->setVisible(0);
    ui->widget_customplot_reoilgasdetail->graph(i_graph)->setData(x,y);
    ui->widget_customplot_reoilgasdetail->xAxis->setLabel(QString("时间/天"));
    ui->widget_customplot_reoilgasdetail->yAxis->setLabel(QString("压力/KPa"));
    ui->widget_customplot_reoilgasdetail->xAxis->setRange(0,31);
    ui->widget_customplot_reoilgasdetail->xAxis->setAutoTickStep(false);
    ui->widget_customplot_reoilgasdetail->xAxis->setTickStep(1);
    ui->widget_customplot_reoilgasdetail->yAxis->setRange(y_range_min-3,y_range_max+3);

    ui->widget_customplot_reoilgasdetail->replot();
    ui->widget_gundetail->setHidden(0);
}
void MainWindow::Draw_Tank_Nongdu_Month()   //油气浓度曲线
{
    ui->widget_customplot_reoilgasdetail->clearGraphs();
    select_data_onemonth_tanknongdu();

    unsigned int n = 0;
    unsigned int n_total = 0;
    unsigned int i_graph = 0;
    float y_range_max = Temperature_Month[1][1];
    while(Temperature_Month[n+1][0])
    {
        if(Temperature_Month[n+1][0]>(Temperature_Month[n][0]+1))
        {
            ui->widget_customplot_reoilgasdetail->addGraph();
            QVector<double> x(n-n_total),y(n-n_total);
            for(unsigned int i = 0;i<(n-n_total);i++)
            {
                x[i] = Temperature_Month[i+1+n_total][0]/24;
                y[i] = Temperature_Month[i+1+n_total][1];
            }
            ui->widget_customplot_reoilgasdetail->graph(i_graph)->setData(x,y);
            i_graph++;
            n_total = n;
        }
        y_range_max = qMax(y_range_max,Temperature_Month[n+1][1]);
        n++;
    }
    ui->widget_customplot_reoilgasdetail->addGraph();
    QVector<double> x(n-n_total),y(n-n_total);
    for(unsigned int i = 0;i<(n-n_total);i++)
    {
        x[i] = Temperature_Month[i+1+n_total][0]/24;
        y[i] = Temperature_Month[i+1+n_total][1];
    }
    ui->widget_customplot_reoilgasdetail->legend->setVisible(0);
    ui->widget_customplot_reoilgasdetail->graph(i_graph)->setData(x,y);
    ui->widget_customplot_reoilgasdetail->xAxis->setLabel(QString("时间/天"));
    ui->widget_customplot_reoilgasdetail->yAxis->setLabel(QString("浓度/g/m³"));
    ui->widget_customplot_reoilgasdetail->xAxis->setRange(0,31);
    ui->widget_customplot_reoilgasdetail->xAxis->setAutoTickStep(false);
    ui->widget_customplot_reoilgasdetail->xAxis->setTickStep(1);
    ui->widget_customplot_reoilgasdetail->yAxis->setRange(-0.1,y_range_max+5);

    ui->widget_customplot_reoilgasdetail->replot();
    ui->widget_gundetail->setHidden(0);
}
void MainWindow::Draw_Oilgun_Al_Month(unsigned char t)
{
    ui->widget_customplot_reoilgasdetail->clearGraphs();
    select_data_onemonth_oilgun_al(t);

    unsigned int n_max = 0;
    unsigned int n_min = 0;
    unsigned int i_graph = 0;
    //al最大值曲线
    while(Temperature_Month[n_max][0])
    {
        n_max++;
    }
    if(n_max)
    {
        ui->widget_customplot_reoilgasdetail->addGraph();
        QVector<double> x1(n_max),y1(n_max);
        for(unsigned int i = 0;i<n_max;i++)
        {
            x1[i] = Temperature_Month[i][0]/24;
            y1[i] = Temperature_Month[i][1];
            printf("%f--y:%f-\n",x1[i],y1[i]);
        }
        ui->widget_customplot_reoilgasdetail->graph(i_graph)->setData(x1,y1);
        ui->widget_customplot_reoilgasdetail->graph(i_graph)->setName("A/L Max");
        i_graph++;
    }
    //al最小值曲线
    while(Temperature_Month_Min[n_min][0])
    {
        n_min++;
    }
    if(n_min)
    {
        ui->widget_customplot_reoilgasdetail->addGraph();
        QVector<double> x2(n_min),y2(n_min);
        for(unsigned int i = 0;i<n_min;i++)
        {
            x2[i] = Temperature_Month_Min[i][0]/24;
            y2[i] = Temperature_Month_Min[i][1];
            printf("%f--y:%f-\n",x2[i],y2[i]);
        }
        ui->widget_customplot_reoilgasdetail->graph(i_graph)->setData(x2,y2);
        ui->widget_customplot_reoilgasdetail->graph(i_graph)->setName("A/L Min");
        ui->widget_customplot_reoilgasdetail->graph(i_graph)->setPen(QPen(Qt::red));
    }
    ui->widget_customplot_reoilgasdetail->legend->setVisible(1);
    ui->widget_customplot_reoilgasdetail->xAxis->setRange(0,31);
    ui->widget_customplot_reoilgasdetail->xAxis->setLabel(QString("时间/天"));
    ui->widget_customplot_reoilgasdetail->yAxis->setLabel(QString("A/L"));
    ui->widget_customplot_reoilgasdetail->xAxis->setAutoTickStep(false);
    ui->widget_customplot_reoilgasdetail->xAxis->setTickStep(1);
    ui->widget_customplot_reoilgasdetail->yAxis->setRange(-0.1,2);
    ui->widget_customplot_reoilgasdetail->replot();
    ui->widget_gundetail->setHidden(0);
}

void MainWindow::IIE_show(unsigned char IIE_uart_m,int IIE_R_m, int IIE_V_m, int IIE_oil_time_m, int IIE_people_time_m)
{
    printf("\n");
    //printf("receive IIE data  ");
    printf("IIE V is %d  ",IIE_V_m);
    printf("IIE R is %d  ",IIE_R_m);
    printf("IIE oil time is %d  ",IIE_oil_time_m);
    printf("IIE people time is %d\n",IIE_people_time_m);
    for(int i = 0; i < 8; i++)
    {
        printf("  %x ",IIE_sta[i]);
    }
    printf("\n");
    for(int i = 0; i < 8; i++)
    {
        printf("  %x ",IIE_set[i]);
    }
    printf("\n");

    if(Flag_IIE == 1)//如果设置了IIE
    {
        if(IIE_uart_m == 0xff) //通信故障
        {
            ui->label_IIE_tongxinguzhang->setHidden(0);//显示设备故障
            ui->label_IIE_work->setText("设备开启"); //工作状态
            ui->label_IIE_liquid->setText("空"); //高液位报警
            ui->label_IIE_sele->setText("空");   //静电状态
            ui->label_IIE_touch->setText("空");  //触摸状态
            ui->label_IIE_clip->setText("空");   //夹子状态
            ui->label_IIE_groung->setText("空"); //接地状态
            ui->label_IIE_oiltime->setText("空");   //稳油计时状态
            ui->label_IIE_peotime->setText("空"); //人员值守计时状态

            ui->label_IIE_waring_s->setText("空"); //持续报警设置
            ui->label_IIE_sele_s->setText("空"); //静电功能设置
            ui->label_IIE_liquid_s->setText("空"); //高液位报警功能设置
            ui->label_IIE_people_s->setText("空"); //来人检测设置
            ui->label_IIE_people_sele_s->setText("空"); //人体静电设置
            ui->label_IIE_key_s->setText("空"); //钥匙管理器设置
            ui->label_IIE_watch_s->setText("空"); //人员值守设置
            ui->label_IIE_time_s->setText("空"); //计时功能设置

            ui->label_IIE_V->setText("空"); //电压值
            ui->label_IIE_R->setText("空"); //电阻值
            ui->label_IIE_time_oil->setText("空"); //稳油倒计时
            ui->label_IIE_time_peo->setText("空"); //人员值倒计时

            Flag_IIE_timeoil = 0;//稳油计时结束
            Flag_IIE_timepeo = 0;//值守计时结束
        }
        else//通信正常
        {
            ui->label_IIE_tongxinguzhang->setHidden(1);//隐藏设备故障
            //设置部分
            if(IIE_set[0] == 1){ui->label_IIE_waring_s->setText("屏蔽");}
            else{ui->label_IIE_waring_s->setText("开启");}
            if(IIE_set[1] == 1){ui->label_IIE_sele_s->setText("屏蔽");ui->label_IIE_clip->setText("已屏蔽");}
            else{ui->label_IIE_sele_s->setText("开启");}
            if(IIE_set[2] == 1){ui->label_IIE_liquid_s->setText("屏蔽");ui->label_IIE_liquid->setText("已屏蔽");}
            else{ui->label_IIE_liquid_s->setText("开启");}
            if(IIE_set[3] == 1){ui->label_IIE_people_s->setText("屏蔽");}
            else{ui->label_IIE_people_s->setText("开启");}
            if(IIE_set[4] == 1){ui->label_IIE_people_sele_s->setText("屏蔽");}
            else{ui->label_IIE_people_sele_s->setText("开启");}
            if(IIE_set[5] == 1){ui->label_IIE_key_s->setText("屏蔽");}
            else{ui->label_IIE_key_s->setText("开启");}
            if(IIE_set[6] == 1){ui->label_IIE_watch_s->setText("屏蔽");ui->label_IIE_peotime->setText("已屏蔽");ui->label_IIE_time_peo->setText("空");}
            else{ui->label_IIE_watch_s->setText("开启");}
            if(IIE_set[7] == 1){ui->label_IIE_time_s->setText("屏蔽");}
            else{ui->label_IIE_time_s->setText("开启");}

            //液位报警部分  带屏蔽功能
			if((IIE_sta[0] == 1))
            {
                ui->label_IIE_liquid->setText("高液位报警"); //高液位报警
            }
			if((IIE_sta[0] == 0))
            {
                ui->label_IIE_liquid->setText("高液位正常"); //高液位报警
            }

            if(IIE_sta[3] == 0)//开始使用
            {
                ui->label_IIE_work->setText("工作中");
                ui->label_IIE_V->setText(QString::number(IIE_V_m).append("V"));
                ui->label_IIE_R->setText(QString::number(IIE_R_m).append("Ω"));

                if(IIE_sta[1] == 1)
                {
                    ui->label_IIE_sele->setText("静电危险");
                }else
                {
                    ui->label_IIE_sele->setText("静电安全");
                }
                if(IIE_sta[2] == 1)
                {
                    ui->label_IIE_touch->setText("已触摸");
                }else
                {
                    ui->label_IIE_touch->setText("未触摸");
                }
                //带屏蔽功能
                if((IIE_sta[4] == 0)&&(IIE_set[1] == 0))
                {
                    ui->label_IIE_clip->setText("夹子正常");
                }
                if((IIE_sta[4] == 1)&&(IIE_set[1] == 0))
                {
                    ui->label_IIE_clip->setText("夹子报警");
                }

                if(IIE_sta[5] == 1)
                {
                    ui->label_IIE_groung->setText("接地故障");
                }else
                {
                    ui->label_IIE_groung->setText("接地正常");
                }
                if(IIE_sta[6] == 1)
                {
                    Flag_IIE_timeoil = 1;//稳油计时使能
                    ui->label_IIE_oiltime->setText("稳油计时中");
                    if((IIE_oil_time_m - Time_IIE_oil) >= 3)
                    {
                        Time_IIE_oil = IIE_oil_time_m;//误差超过三秒重新赋值
                    }
                    //ui->label_IIE_time_oil->setText(QString::number(Time_IIE_oil).append("S"));
                }else
                {
                    Flag_IIE_timeoil = 0;//稳油计时结束
                    ui->label_IIE_oiltime->setText("待机");
                    ui->label_IIE_time_oil->setText("空");
                }
                //带屏蔽功能
                if((IIE_sta[7] == 1)&&(IIE_set[6] == 0))
                {
                    Flag_IIE_timepeo = 1;//值守计时使能
                    ui->label_IIE_peotime->setText("值守计时中");
                    if((IIE_people_time_m - Time_IIE_peo) >= 3)
                    {
                        Time_IIE_peo = IIE_people_time_m;//误差超过三秒重新赋值
                    }
                    //ui->label_IIE_time_peo->setText(QString::number(IIE_people_time_m).append("S"));
                }
                if((IIE_sta[7] == 0)&&(IIE_set[6] == 0))
                {
                    Flag_IIE_timepeo = 0;//值守计时结束
                    ui->label_IIE_peotime->setText("待机");
                    ui->label_IIE_time_peo->setText("空");
                }



            }
            else
            {
                ui->label_IIE_work->setText("待机中");
                ui->label_IIE_liquid->setText("空"); //高液位报警
                ui->label_IIE_sele->setText("空");   //静电状态
                ui->label_IIE_touch->setText("空");  //触摸状态
                ui->label_IIE_clip->setText("空");   //夹子状态
                ui->label_IIE_groung->setText("空"); //接地状态
                ui->label_IIE_oiltime->setText("空");   //稳油计时状态
                ui->label_IIE_peotime->setText("空"); //人员值守计时状态
                ui->label_IIE_V->setText("空");
                ui->label_IIE_R->setText("空");
                Flag_IIE_timeoil = 0;//稳油计时结束
                Flag_IIE_timepeo = 0;//值守计时结束
                ui->label_IIE_time_oil->setText("空");
                ui->label_IIE_time_peo->setText("空");

                ui->label_IIE_waring_s->setText("空"); //持续报警设置
                ui->label_IIE_sele_s->setText("空"); //静电功能设置
                ui->label_IIE_liquid_s->setText("空"); //高液位报警功能设置
                ui->label_IIE_people_s->setText("空"); //来人检测设置
                ui->label_IIE_people_sele_s->setText("空"); //人体静电设置
                ui->label_IIE_key_s->setText("空"); //钥匙管理器设置
                ui->label_IIE_watch_s->setText("空"); //人员值守设置
                ui->label_IIE_time_s->setText("空"); //计时功能设置
            }

        }


        if((IIE_uart_m == 0xff)&&(Flag_IIE_warn[1] != 1)) //通信故障
        {
            Flag_IIE_warn[1] = 1;
			add_value_IIE("卸油流程控制器：通信故障");
        }
        if((IIE_uart_m == 0x00)&&(Flag_IIE_warn[1] != 0)) //通信正常
        {
            Flag_IIE_warn[1] = 0;
			add_value_IIE("卸油流程控制器：通信正常");
        }
        if((IIE_uart_m == 0x00)&&(IIE_sta[0] == 1)&&(IIE_set[2] == 0)&&(Flag_IIE_warn[2] != 1))
        {
            Flag_IIE_warn[2] = 1;
			add_value_IIE("卸油流程控制器：高液位报警");
        }
        if((IIE_uart_m == 0x00)&&(IIE_sta[0] == 0)&&(IIE_set[2] == 0)&&(Flag_IIE_warn[2] != 0))
        {
            Flag_IIE_warn[2] = 0;
			add_value_IIE("卸油流程控制器：高液位正常");
        }
        if((IIE_uart_m == 0x00)&&(IIE_sta[3] == 1)&&(Flag_IIE_warn[3] != 1))
        {
            Flag_IIE_warn[3] = 1;
			add_value_IIE("卸油流程控制器：待机模式");
        }
        if((IIE_uart_m == 0x00)&&(IIE_sta[3] == 0)&&(Flag_IIE_warn[3] != 0))
        {
            Flag_IIE_warn[3] = 0;
			add_value_IIE("卸油流程控制器：工作模式");
        }
		if((IIE_uart_m == 0x00)&&(IIE_sta[1] == 1)&&(Flag_IIE_warn[4] != 1))
		{
			Flag_IIE_warn[4] = 1;
			add_value_IIE("卸油流程控制器：静电报警");
		}
		if((IIE_uart_m == 0x00)&&(IIE_sta[1] == 0)&&(Flag_IIE_warn[4] != 0))
		{
			Flag_IIE_warn[4] = 0;
			add_value_IIE("卸油流程控制器：静电正常");
		}
		if((IIE_uart_m == 0x00)&&(IIE_sta[4] == 1)&&(Flag_IIE_warn[5] != 1))
		{
			Flag_IIE_warn[5] = 1;
			add_value_IIE("卸油流程控制器：夹子报警");
		}
		if((IIE_uart_m == 0x00)&&(IIE_sta[4] == 0)&&(Flag_IIE_warn[5] != 0))
		{
			Flag_IIE_warn[5] = 0;
			add_value_IIE("卸油流程控制器：夹子正常");
		}
		if((IIE_uart_m == 0x00)&&(IIE_sta[5] == 1)&&(Flag_IIE_warn[6] != 1))
		{
			Flag_IIE_warn[6] = 1;
			add_value_IIE("卸油流程控制器：接地故障");
		}
		if((IIE_uart_m == 0x00)&&(IIE_sta[5] == 0)&&(Flag_IIE_warn[6] != 0))
		{
			Flag_IIE_warn[6] = 0;
			add_value_IIE("卸油流程控制器：接地正常");
		}

    }
    else//设备关闭
    {
        ui->label_IIE_tongxinguzhang->setHidden(1);//隐藏设备故障

        ui->label_IIE_work->setText("设备关闭"); //工作状态
        ui->label_IIE_liquid->setText("空"); //高液位报警
        ui->label_IIE_sele->setText("空");   //静电状态
        ui->label_IIE_touch->setText("空");  //触摸状态
        ui->label_IIE_clip->setText("空");   //夹子状态
        ui->label_IIE_groung->setText("空"); //接地状态
        ui->label_IIE_oiltime->setText("空");   //稳油计时状态
        ui->label_IIE_peotime->setText("空"); //人员值守计时状态

        ui->label_IIE_waring_s->setText("空"); //持续报警设置
        ui->label_IIE_sele_s->setText("空"); //静电功能设置
        ui->label_IIE_liquid_s->setText("空"); //高液位报警功能设置
        ui->label_IIE_people_s->setText("空"); //来人检测设置
        ui->label_IIE_people_sele_s->setText("空"); //人体静电设置
        ui->label_IIE_key_s->setText("空"); //钥匙管理器设置
        ui->label_IIE_watch_s->setText("空"); //人员值守设置
        ui->label_IIE_time_s->setText("空"); //计时功能设置

        ui->label_IIE_V->setText("空"); //电压值
        ui->label_IIE_R->setText("空"); //电阻值
        Flag_IIE_timeoil = 0;//稳油计时结束
        Flag_IIE_timepeo = 0;//值守计时结束
        ui->label_IIE_time_oil->setText("空"); //稳油倒计时
        ui->label_IIE_time_peo->setText("空"); //人员值倒计时

        Flag_IIE_warn[0] = 2;Flag_IIE_warn[1] = 2;Flag_IIE_warn[2] = 2;
        Flag_IIE_warn[3] = 2;Flag_IIE_warn[4] = 2;Flag_IIE_warn[5] = 2;
        Flag_IIE_warn[6] = 2;Flag_IIE_warn[7] = 2;Flag_IIE_warn[8] = 2;
        Flag_IIE_warn[9] = 2;
    }
}
//历史记录防止多次写入标志位，每个序号代表什么
//Flag_IIE_warn
//0    开启关闭
//1    通信
//2    高液位

void MainWindow::reset_safe()
{
    if(Flag_IIE == 0)
    {
        ui->label_IIE_tongxinguzhang->setHidden(1);//隐藏设备故障

        ui->label_IIE_work->setText("设备关闭"); //工作状态
        ui->label_IIE_liquid->setText("空"); //高液位报警
        ui->label_IIE_sele->setText("空");   //静电状态
        ui->label_IIE_touch->setText("空");  //触摸状态
        ui->label_IIE_clip->setText("空");   //夹子状态
        ui->label_IIE_groung->setText("空"); //接地状态
        ui->label_IIE_oiltime->setText("空");   //稳油计时状态
        ui->label_IIE_peotime->setText("空"); //人员值守计时状态

        ui->label_IIE_waring_s->setText("空"); //持续报警设置
        ui->label_IIE_sele_s->setText("空"); //静电功能设置
        ui->label_IIE_liquid_s->setText("空"); //高液位报警功能设置
        ui->label_IIE_people_s->setText("空"); //来人检测设置
        ui->label_IIE_people_sele_s->setText("空"); //人体静电设置
        ui->label_IIE_key_s->setText("空"); //钥匙管理器设置
        ui->label_IIE_watch_s->setText("空"); //人员值守设置
        ui->label_IIE_time_s->setText("空"); //计时功能设置

        ui->label_IIE_V->setText("空"); //电压值
        ui->label_IIE_R->setText("空"); //电阻值
        Flag_IIE_timeoil = 0;//稳油计时结束
        Flag_IIE_timepeo = 0;//值守计时结束
        ui->label_IIE_time_oil->setText("空"); //稳油倒计时
        ui->label_IIE_time_peo->setText("空"); //人员值倒计时
    }
    else
    {
        ui->label_IIE_work->setText("设备开启"); //工作状态
        ui->label_IIE_tongxinguzhang->setHidden(1);//隐藏设备故障
    }

}
void MainWindow::show_IIE_electromagnetic(unsigned char sta)
{
	if(Flag_IIE == 1)
	{
		if(sta == 0xff)
		{
			ui->pushButton_IIE_electromagnetic->setText("通信故障");
			if(Flag_IIE_warn_value[0] != 1)
			{
				Flag_IIE_warn_value[0] = 1;
				add_value_IIE("电磁阀控制器：通信故障");
			}
		}
		if(sta == 0x00)
		{
			ui->pushButton_IIE_electromagnetic->setText("设备正常");
			if(Flag_IIE_warn_value[0] != 0)
			{
				Flag_IIE_warn_value[0] = 0;
				add_value_IIE("电磁阀控制器：通信正常");
			}
		}
		if(ui->widget_iie_electromagnetic->isVisible())
		{
			on_pushButton_IIE_electromagnetic_clicked();
		}
	}
	else
	{
		ui->pushButton_IIE_electromagnetic->setText("设备关闭");
	}
}

void MainWindow::on_pushButton_IIE_electromagnetic_clicked()
{
	ui->frame_value1->setHidden(1);
	ui->frame_value2->setHidden(1);
	ui->frame_value3->setHidden(1);
	ui->frame_value4->setHidden(1);
	ui->frame_value5->setHidden(1);
	if(IIE_Value_Num>=1)
	{
		ui-> frame_value1->setHidden(0);
	}
	if(IIE_Value_Num>=2)
	{
		ui-> frame_value2->setHidden(0);
	}
	if(IIE_Value_Num>=3)
	{
		ui-> frame_value3->setHidden(0);
	}
	if(IIE_Value_Num>=4)
	{
		ui-> frame_value4->setHidden(0);
	}
	if(IIE_Value_Num>=5)
	{
		ui-> frame_value5->setHidden(0);
	}
	ui->widget_iie_electromagnetic->setHidden(0);
	ui->widget_iie_electromagnetic-> setWindowFlags(Qt::Widget);
	if(Flag_IIE)
	{
		if(IIE_Electromagnetic_Sta[0][0] == 0x0f)//单个故障
		{
			ui->label_IIE2_uart->setText("通信故障");
			ui->label_IIE2_far->setText("空");
			ui->label_IIE2_open->setText("空");
			if(Flag_IIE_warn_value[1] != 0)
			{
				Flag_IIE_warn_value[1] = 0;
				add_value_IIE("1号电磁阀：通信故障");
			}
		}
		else
		{
			if(IIE_Electromagnetic_Sta[0][0] == 1)
			{
				ui->label_IIE2_uart->setText("故障");
				if(Flag_IIE_warn_value[1] != 1)
				{
					Flag_IIE_warn_value[1] = 1;
					add_value_IIE("1号电磁阀：故障");
				}
			}
			else
			{
				ui->label_IIE2_uart->setText("正常");
				if(Flag_IIE_warn_value[1] != 2)
				{
					Flag_IIE_warn_value[1] = 2;
					add_value_IIE("1号电磁阀：正常");
				}
			}
			if(IIE_Electromagnetic_Sta[0][1] == 1)
			    {ui->label_IIE2_far->setText("自动");}else{ui->label_IIE2_far->setText("手动");}
			if(IIE_Electromagnetic_Sta[0][2] == 1)
			{
				ui->label_IIE2_open->setText("全关");
				if(Flag_IIE_warn_value[2] != 0)
				{
					Flag_IIE_warn_value[2] = 0;
					add_value_IIE("1号电磁阀：全关");
				}
			}
			else if(IIE_Electromagnetic_Sta[0][3] == 1)
			{
				ui->label_IIE2_open->setText("全开");
				if(Flag_IIE_warn_value[2] != 1)
				{
					Flag_IIE_warn_value[2] = 1;
					add_value_IIE("1号电磁阀：全开");
				}
			}
			else if((IIE_Electromagnetic_Sta[0][2] == 0)&&(IIE_Electromagnetic_Sta[0][3] == 0))
			{
				ui->label_IIE2_open->setText("动作");
			}
		}

		if(IIE_Electromagnetic_Sta[1][0] == 0x0f)
		{
			ui->label_IIE2_uart_2->setText("通信故障");
			ui->label_IIE2_far_2->setText("空");
			ui->label_IIE2_open_2->setText("空");
			if(Flag_IIE_warn_value[3] != 0)
			{
				Flag_IIE_warn_value[3] = 0;
				add_value_IIE("2号电磁阀：通信故障");
			}
		}
		else
		{
			if(IIE_Electromagnetic_Sta[1][0] == 1)
			{
				ui->label_IIE2_uart_2->setText("故障");
				if(Flag_IIE_warn_value[3] != 1)
				{
					Flag_IIE_warn_value[3] = 1;
					add_value_IIE("2号电磁阀：故障");
				}
			}else
			{
				ui->label_IIE2_uart_2->setText("正常");
				if(Flag_IIE_warn_value[3] != 2)
				{
					Flag_IIE_warn_value[3] = 2;
					add_value_IIE("2号电磁阀：正常");
				}
			}
			if(IIE_Electromagnetic_Sta[1][1] == 1)
			    {ui->label_IIE2_far_2->setText("自动");}else{ui->label_IIE2_far_2->setText("手动");}
			if(IIE_Electromagnetic_Sta[1][2] == 1)
			{
				ui->label_IIE2_open_2->setText("全关");
				if(Flag_IIE_warn_value[4] != 0)
				{
					Flag_IIE_warn_value[4] = 0;
					add_value_IIE("2号电磁阀：全关");
				}
			}
			else if(IIE_Electromagnetic_Sta[1][3] == 1)
			{
				ui->label_IIE2_open_2->setText("全开");
				if(Flag_IIE_warn_value[4] != 1)
				{
					Flag_IIE_warn_value[4] = 1;
					add_value_IIE("2号电磁阀：全开");
				}
			}
			else if((IIE_Electromagnetic_Sta[1][2] == 0)&&(IIE_Electromagnetic_Sta[0][3] == 0))
			{
				ui->label_IIE2_open_2->setText("动作");
			}
		}

		if(IIE_Electromagnetic_Sta[2][0] == 0x0f)
		{
			ui->label_IIE2_uart_3->setText("通信故障");
			ui->label_IIE2_far_3->setText("空");
			ui->label_IIE2_open_3->setText("空");
			if(Flag_IIE_warn_value[5] != 0)
			{
				Flag_IIE_warn_value[5] = 0;
				add_value_IIE("3号电磁阀：通信故障");
			}
		}
		else
		{
			if(IIE_Electromagnetic_Sta[2][0] == 1)
			{
				ui->label_IIE2_uart_3->setText("故障");
				if(Flag_IIE_warn_value[5] != 1)
				{
					Flag_IIE_warn_value[5] = 1;
					add_value_IIE("3号电磁阀：故障");
				}
			}else
			{
				ui->label_IIE2_uart_3->setText("正常");
				if(Flag_IIE_warn_value[5] != 2)
				{
					Flag_IIE_warn_value[5] = 2;
					add_value_IIE("3号电磁阀：正常");
				}
			}
			if(IIE_Electromagnetic_Sta[2][1] == 1)
			    {ui->label_IIE2_far_3->setText("自动");}else{ui->label_IIE2_far_3->setText("手动");}
			if(IIE_Electromagnetic_Sta[2][2] == 1)
			{
				ui->label_IIE2_open_3->setText("全关");
				if(Flag_IIE_warn_value[6] != 0)
				{
					Flag_IIE_warn_value[6] = 0;
					add_value_IIE("3号电磁阀：全关");
				}
			}
			else if(IIE_Electromagnetic_Sta[2][3] == 1)
			{
				ui->label_IIE2_open_3->setText("全开");
				if(Flag_IIE_warn_value[6] != 1)
				{
					Flag_IIE_warn_value[6] = 1;
					add_value_IIE("3号电磁阀：全开");
				}
			}
			else if((IIE_Electromagnetic_Sta[2][2] == 0)&&(IIE_Electromagnetic_Sta[0][3] == 0))
			{
				ui->label_IIE2_open_3->setText("动作");
			}
		}

		if(IIE_Electromagnetic_Sta[3][0] = 0x0f)
		{
			ui->label_IIE2_uart_4->setText("通信故障");
			ui->label_IIE2_far_4->setText("空");
			ui->label_IIE2_open_4->setText("空");
			if(Flag_IIE_warn_value[7] != 0)
			{
				Flag_IIE_warn_value[7] = 0;
				add_value_IIE("4号电磁阀：通信故障");
			}
		}
		else
		{
			if(IIE_Electromagnetic_Sta[3][0] == 1)
			{
				ui->label_IIE2_uart_4->setText("故障");
				if(Flag_IIE_warn_value[7] != 1)
				{
					Flag_IIE_warn_value[7] = 1;
					add_value_IIE("4号电磁阀：故障");
				}
			}else
			{
				ui->label_IIE2_uart_4->setText("正常");
				if(Flag_IIE_warn_value[7] != 2)
				{
					Flag_IIE_warn_value[7] = 2;
					add_value_IIE("4号电磁阀：正常");
				}
			}
			if(IIE_Electromagnetic_Sta[3][1] == 1)
			    {ui->label_IIE2_far_4->setText("自动");}else{ui->label_IIE2_far_4->setText("手动");}
			if(IIE_Electromagnetic_Sta[3][2] == 1)
			{
				ui->label_IIE2_open_4->setText("全关");
				if(Flag_IIE_warn_value[8] != 0)
				{
					Flag_IIE_warn_value[8] = 0;
					add_value_IIE("4号电磁阀：全关");
				}
			}
			else if(IIE_Electromagnetic_Sta[3][3] == 1)
			{
				ui->label_IIE2_open_4->setText("全开");
				if(Flag_IIE_warn_value[8] != 1)
				{
					Flag_IIE_warn_value[8] = 1;
					add_value_IIE("4号电磁阀：全开");
				}
			}
			else if((IIE_Electromagnetic_Sta[3][2] == 0)&&(IIE_Electromagnetic_Sta[0][3] == 0))
			{
				ui->label_IIE2_open_4->setText("动作");
			}
		}

		if(IIE_Electromagnetic_Sta[4][0] = 0x0f)
		{
			ui->label_IIE2_uart_5->setText("通信故障");
			ui->label_IIE2_far_5->setText("空");
			ui->label_IIE2_open_5->setText("空");
			if(Flag_IIE_warn_value[9] != 0)
			{
				Flag_IIE_warn_value[9] = 0;
				add_value_IIE("5号电磁阀：通信故障");
			}
		}
		else
		{
			if(IIE_Electromagnetic_Sta[4][0] == 1)
			{
				ui->label_IIE2_uart_5->setText("故障");
				if(Flag_IIE_warn_value[9] != 1)
				{
					Flag_IIE_warn_value[9] = 1;
					add_value_IIE("5号电磁阀：故障");
				}
			}
			else
			{
				ui->label_IIE2_uart_5->setText("正常");
				if(Flag_IIE_warn_value[9] != 2)
				{
					Flag_IIE_warn_value[9] = 2;
					add_value_IIE("5号电磁阀：正常");
				}
			}
			if(IIE_Electromagnetic_Sta[4][1] == 1)
			{ui->label_IIE2_far_5->setText("自动");}else{ui->label_IIE2_far_5->setText("手动");}
			if(IIE_Electromagnetic_Sta[0][2] == 1)
			{
				ui->label_IIE2_open_5->setText("全关");
				if(Flag_IIE_warn_value[10] != 0)
				{
					Flag_IIE_warn_value[10] = 0;
					add_value_IIE("5号电磁阀：全关");
				}
			}
			else if(IIE_Electromagnetic_Sta[0][3] == 1)
			{
				ui->label_IIE2_open_5->setText("全开");
				if(Flag_IIE_warn_value[10] != 1)
				{
					Flag_IIE_warn_value[10] = 1;
					add_value_IIE("5号电磁阀：全开");
				}
			}
			else if((IIE_Electromagnetic_Sta[0][2] == 0)&&(IIE_Electromagnetic_Sta[0][3] == 0))
			{
				ui->label_IIE2_open_5->setText("动作");
			}
		}
	}
	else
	{
		ui->label_IIE2_uart->setText("空");
		ui->label_IIE2_far->setText("空");
		ui->label_IIE2_open->setText("空");
		ui->label_IIE2_uart_2->setText("空");
		ui->label_IIE2_far_2->setText("空");
		ui->label_IIE2_open_2->setText("空");
		ui->label_IIE2_uart_3->setText("空");
		ui->label_IIE2_far_3->setText("空");
		ui->label_IIE2_open_3->setText("空");
		ui->label_IIE2_uart_4->setText("空");
		ui->label_IIE2_far_4->setText("空");
		ui->label_IIE2_open_4->setText("空");
		ui->label_IIE2_uart_5->setText("空");
		ui->label_IIE2_far_5->setText("空");
		ui->label_IIE2_open_5->setText("空");
	}

}
void MainWindow::on_pushButton_IIEDetails_close_clicked()
{
	ui->widget_iie_electromagnetic->setHidden(1);
}
void MainWindow::liquid_nomal_s()
{
    ui->label_yewei_sta->setText("液位正常");
}
void MainWindow::liquid_close_s()
{
    ui->label_yewei_sta->setText("设备监测关闭");
}
void MainWindow::liquid_warn_s()
{
    ui->label_yewei_sta->setText("高液位报警");
}

void MainWindow::pump_close_s()
{
    ui->label_youbeng_sta->setText("设备监测关闭");
}
void MainWindow::pump_run_s()
{
    ui->label_youbeng_sta->setText("设备启动");
}
void MainWindow::pump_stop_s()
{
    ui->label_youbeng_sta->setText("设备停止");
}

//防撞柱数量重绘
void MainWindow::crash_column_reset()
{
    ui->label_cra1->setHidden(1);ui->label_cra2->setHidden(1);ui->label_cra3->setHidden(1);
    ui->label_cra4->setHidden(1);ui->label_cra5->setHidden(1);ui->label_cra6->setHidden(1);
    ui->label_cra7->setHidden(1);ui->label_cra8->setHidden(1);ui->label_cra9->setHidden(1);
    ui->label_cra10->setHidden(1);ui->label_cra11->setHidden(1);ui->label_cra12->setHidden(1);
    ui->label_cra13->setHidden(1);ui->label_cra14->setHidden(1);ui->label_cra15->setHidden(1);
    ui->label_cra16->setHidden(1);ui->label_cra17->setHidden(1);ui->label_cra18->setHidden(1);
    ui->label_cra19->setHidden(1);ui->label_cra20->setHidden(1);ui->label_cra21->setHidden(1);
    ui->label_cra22->setHidden(1);ui->label_cra23->setHidden(1);ui->label_cra24->setHidden(1);
    ui->label_cra25->setHidden(1);ui->label_cra26->setHidden(1);ui->label_cra27->setHidden(1);
    ui->label_cra28->setHidden(1);ui->label_cra29->setHidden(1);ui->label_cra30->setHidden(1);
    ui->label_cra31->setHidden(1);ui->label_cra32->setHidden(1);
    ui->label_nc1->setHidden(1);ui->label_nc2->setHidden(1);ui->label_nc3->setHidden(1);ui->label_nc4->setHidden(1);
    ui->label_nc5->setHidden(1);ui->label_nc6->setHidden(1);ui->label_nc7->setHidden(1);ui->label_nc8->setHidden(1);
    ui->label_nc9->setHidden(1);ui->label_nc10->setHidden(1);ui->label_nc11->setHidden(1);ui->label_nc12->setHidden(1);
    ui->label_nc13->setHidden(1);ui->label_nc14->setHidden(1);ui->label_nc15->setHidden(1);ui->label_nc16->setHidden(1);
    ui->label_nc17->setHidden(1);ui->label_nc18->setHidden(1);ui->label_nc19->setHidden(1);ui->label_nc20->setHidden(1);
    ui->label_nc21->setHidden(1);ui->label_nc22->setHidden(1);ui->label_nc23->setHidden(1);ui->label_nc24->setHidden(1);
    ui->label_nc25->setHidden(1);ui->label_nc26->setHidden(1);ui->label_nc27->setHidden(1);ui->label_nc28->setHidden(1);
    ui->label_nc29->setHidden(1);ui->label_nc30->setHidden(1);ui->label_nc31->setHidden(1);ui->label_nc32->setHidden(1);

    if(Num_Crash_Column >= 1) {ui->label_cra1->setHidden(0);ui->label_nc1->setHidden(0);}
    if(Num_Crash_Column >= 2) {ui->label_cra2->setHidden(0);ui->label_nc2->setHidden(0);}
    if(Num_Crash_Column >= 3) {ui->label_cra3->setHidden(0);ui->label_nc3->setHidden(0);}
    if(Num_Crash_Column >= 4) {ui->label_cra4->setHidden(0);ui->label_nc4->setHidden(0);}
    if(Num_Crash_Column >= 5) {ui->label_cra5->setHidden(0);ui->label_nc5->setHidden(0);}
    if(Num_Crash_Column >= 6) {ui->label_cra6->setHidden(0);ui->label_nc6->setHidden(0);}
    if(Num_Crash_Column >= 7) {ui->label_cra7->setHidden(0);ui->label_nc7->setHidden(0);}
    if(Num_Crash_Column >= 8) {ui->label_cra8->setHidden(0);ui->label_nc8->setHidden(0);}
    if(Num_Crash_Column >= 9) {ui->label_cra9->setHidden(0);ui->label_nc9->setHidden(0);}
    if(Num_Crash_Column >= 10) {ui->label_cra10->setHidden(0);ui->label_nc10->setHidden(0);}
    if(Num_Crash_Column >= 11) {ui->label_cra11->setHidden(0);ui->label_nc11->setHidden(0);}
    if(Num_Crash_Column >= 12) {ui->label_cra12->setHidden(0);ui->label_nc12->setHidden(0);}
    if(Num_Crash_Column >= 13) {ui->label_cra13->setHidden(0);ui->label_nc13->setHidden(0);}
    if(Num_Crash_Column >= 14) {ui->label_cra14->setHidden(0);ui->label_nc14->setHidden(0);}
    if(Num_Crash_Column >= 15) {ui->label_cra15->setHidden(0);ui->label_nc15->setHidden(0);}
    if(Num_Crash_Column >= 16) {ui->label_cra16->setHidden(0);ui->label_nc16->setHidden(0);}
    if(Num_Crash_Column >= 17) {ui->label_cra17->setHidden(0);ui->label_nc17->setHidden(0);}
    if(Num_Crash_Column >= 18) {ui->label_cra18->setHidden(0);ui->label_nc18->setHidden(0);}
    if(Num_Crash_Column >= 19) {ui->label_cra19->setHidden(0);ui->label_nc19->setHidden(0);}
    if(Num_Crash_Column >= 20) {ui->label_cra20->setHidden(0);ui->label_nc20->setHidden(0);}
    if(Num_Crash_Column >= 21) {ui->label_cra21->setHidden(0);ui->label_nc21->setHidden(0);}
    if(Num_Crash_Column >= 22) {ui->label_cra22->setHidden(0);ui->label_nc22->setHidden(0);}
    if(Num_Crash_Column >= 23) {ui->label_cra23->setHidden(0);ui->label_nc23->setHidden(0);}
    if(Num_Crash_Column >= 24) {ui->label_cra24->setHidden(0);ui->label_nc24->setHidden(0);}
    if(Num_Crash_Column >= 25) {ui->label_cra25->setHidden(0);ui->label_nc25->setHidden(0);}
    if(Num_Crash_Column >= 26) {ui->label_cra26->setHidden(0);ui->label_nc26->setHidden(0);}
    if(Num_Crash_Column >= 27) {ui->label_cra27->setHidden(0);ui->label_nc27->setHidden(0);}
    if(Num_Crash_Column >= 28) {ui->label_cra28->setHidden(0);ui->label_nc28->setHidden(0);}
    if(Num_Crash_Column >= 29) {ui->label_cra29->setHidden(0);ui->label_nc29->setHidden(0);}
    if(Num_Crash_Column >= 30) {ui->label_cra30->setHidden(0);ui->label_nc30->setHidden(0);}
    if(Num_Crash_Column >= 31) {ui->label_cra31->setHidden(0);ui->label_nc31->setHidden(0);}
    if(Num_Crash_Column >= 32) {ui->label_cra32->setHidden(0);ui->label_nc32->setHidden(0);}
}

void MainWindow::crash_column_stashow(unsigned char whichone,unsigned char sta)
{
    switch (whichone)
    {
    case 1:
		if(sta == 0) ui->label_cra1->setStyleSheet("border-image: url(:/picture/right.png);");//正常
		if(sta == 1) ui->label_cra1->setStyleSheet("border-image: url(:/picture/crashwarn.png);");//报警
		if(sta == 0xff) ui->label_cra1->setStyleSheet("border-image: url(:/picture/uartwro.png);");//通信故障
        break;
    case 2:
		if(sta == 0) ui->label_cra2->setStyleSheet("border-image: url(:/picture/right.png);");//正常
		if(sta == 1) ui->label_cra2->setStyleSheet("border-image: url(:/picture/crashwarn.png);");//报警
		if(sta == 0xff) ui->label_cra2->setStyleSheet("border-image: url(:/picture/uartwro.png);");//通信故障
        break;
    case 3:
		if(sta == 0) ui->label_cra3->setStyleSheet("border-image: url(:/picture/right.png);");//正常
		if(sta == 1) ui->label_cra3->setStyleSheet("border-image: url(:/picture/crashwarn.png);");//报警
		if(sta == 0xff) ui->label_cra3->setStyleSheet("border-image: url(:/picture/uartwro.png);");//通信故障
        break;
    case 4:
		if(sta == 0) ui->label_cra4->setStyleSheet("border-image: url(:/picture/right.png);");//正常
		if(sta == 1) ui->label_cra4->setStyleSheet("border-image: url(:/picture/crashwarn.png);");//报警
		if(sta == 0xff) ui->label_cra4->setStyleSheet("border-image: url(:/picture/uartwro.png);");//通信故障
        break;
    case 5:
		if(sta == 0) ui->label_cra5->setStyleSheet("border-image: url(:/picture/right.png);");//正常
		if(sta == 1) ui->label_cra5->setStyleSheet("border-image: url(:/picture/crashwarn.png);");//报警
		if(sta == 0xff) ui->label_cra5->setStyleSheet("border-image: url(:/picture/uartwro.png);");//通信故障
        break;
    case 6:
		if(sta == 0) ui->label_cra6->setStyleSheet("border-image: url(:/picture/right.png);");//正常
		if(sta == 1) ui->label_cra6->setStyleSheet("border-image: url(:/picture/crashwarn.png);");//报警
		if(sta == 0xff) ui->label_cra6->setStyleSheet("border-image: url(:/picture/uartwro.png);");//通信故障
        break;
    case 7:
		if(sta == 0) ui->label_cra7->setStyleSheet("border-image: url(:/picture/right.png);");//正常
		if(sta == 1) ui->label_cra7->setStyleSheet("border-image: url(:/picture/crashwarn.png);");//报警
		if(sta == 0xff) ui->label_cra7->setStyleSheet("border-image: url(:/picture/uartwro.png);");//通信故障
        break;
    case 8:
		if(sta == 0) ui->label_cra8->setStyleSheet("border-image: url(:/picture/right.png);");//正常
		if(sta == 1) ui->label_cra8->setStyleSheet("border-image: url(:/picture/crashwarn.png);");//报警
		if(sta == 0xff) ui->label_cra8->setStyleSheet("border-image: url(:/picture/uartwro.png);");//通信故障
        break;
    case 9:
		if(sta == 0) ui->label_cra9->setStyleSheet("border-image: url(:/picture/right.png);");//正常
		if(sta == 1) ui->label_cra9->setStyleSheet("border-image: url(:/picture/crashwarn.png);");//报警
		if(sta == 0xff) ui->label_cra9->setStyleSheet("border-image: url(:/picture/uartwro.png);");//通信故障
        break;
    case 10:
		if(sta == 0) ui->label_cra10->setStyleSheet("border-image: url(:/picture/right.png);");//正常
		if(sta == 1) ui->label_cra10->setStyleSheet("border-image: url(:/picture/crashwarn.png);");//报警
		if(sta == 0xff) ui->label_cra10->setStyleSheet("border-image: url(:/picture/uartwro.png);");//通信故障
        break;
    case 11:
		if(sta == 0) ui->label_cra11->setStyleSheet("border-image: url(:/picture/right.png);");//正常
		if(sta == 1) ui->label_cra11->setStyleSheet("border-image: url(:/picture/crashwarn.png);");//报警
		if(sta == 0xff) ui->label_cra11->setStyleSheet("border-image: url(:/picture/uartwro.png);");//通信故障
        break;
    case 12:
		if(sta == 0) ui->label_cra12->setStyleSheet("border-image: url(:/picture/right.png);");//正常
		if(sta == 1) ui->label_cra12->setStyleSheet("border-image: url(:/picture/crashwarn.png);");//报警
		if(sta == 0xff) ui->label_cra12->setStyleSheet("border-image: url(:/picture/uartwro.png);");//通信故障
        break;
    case 13:
		if(sta == 0) ui->label_cra13->setStyleSheet("border-image: url(:/picture/right.png);");//正常
		if(sta == 1) ui->label_cra13->setStyleSheet("border-image: url(:/picture/crashwarn.png);");//报警
		if(sta == 0xff) ui->label_cra13->setStyleSheet("border-image: url(:/picture/uartwro.png);");//通信故障
        break;
    case 14:
		if(sta == 0) ui->label_cra14->setStyleSheet("border-image: url(:/picture/right.png);");//正常
		if(sta == 1) ui->label_cra14->setStyleSheet("border-image: url(:/picture/crashwarn.png);");//报警
		if(sta == 0xff) ui->label_cra14->setStyleSheet("border-image: url(:/picture/uartwro.png);");//通信故障
        break;
    case 15:
		if(sta == 0) ui->label_cra15->setStyleSheet("border-image: url(:/picture/right.png);");//正常
		if(sta == 1) ui->label_cra15->setStyleSheet("border-image: url(:/picture/crashwarn.png);");//报警
		if(sta == 0xff) ui->label_cra15->setStyleSheet("border-image: url(:/picture/uartwro.png);");//通信故障
        break;
    case 16:
		if(sta == 0) ui->label_cra16->setStyleSheet("border-image: url(:/picture/right.png);");//正常
		if(sta == 1) ui->label_cra16->setStyleSheet("border-image: url(:/picture/crashwarn.png);");//报警
		if(sta == 0xff) ui->label_cra16->setStyleSheet("border-image: url(:/picture/uartwro.png);");//通信故障
        break;
    case 17:
		if(sta == 0) ui->label_cra17->setStyleSheet("border-image: url(:/picture/right.png);");//正常
		if(sta == 1) ui->label_cra17->setStyleSheet("border-image: url(:/picture/crashwarn.png);");//报警
		if(sta == 0xff) ui->label_cra17->setStyleSheet("border-image: url(:/picture/uartwro.png);");//通信故障
        break;
    case 18:
		if(sta == 0) ui->label_cra18->setStyleSheet("border-image: url(:/picture/right.png);");//正常
		if(sta == 1) ui->label_cra18->setStyleSheet("border-image: url(:/picture/crashwarn.png);");//报警
		if(sta == 0xff) ui->label_cra18->setStyleSheet("border-image: url(:/picture/uartwro.png);");//通信故障
        break;
    case 19:
		if(sta == 0) ui->label_cra19->setStyleSheet("border-image: url(:/picture/right.png);");//正常
		if(sta == 1) ui->label_cra19->setStyleSheet("border-image: url(:/picture/crashwarn.png);");//报警
		if(sta == 0xff) ui->label_cra19->setStyleSheet("border-image: url(:/picture/uartwro.png);");//通信故障
        break;
    case 20:
		if(sta == 0) ui->label_cra20->setStyleSheet("border-image: url(:/picture/right.png);");//正常
		if(sta == 1) ui->label_cra20->setStyleSheet("border-image: url(:/picture/crashwarn.png);");//报警
		if(sta == 0xff) ui->label_cra20->setStyleSheet("border-image: url(:/picture/uartwro.png);");//通信故障
        break;
    case 21:
		if(sta == 0) ui->label_cra21->setStyleSheet("border-image: url(:/picture/right.png);");//正常
		if(sta == 1) ui->label_cra21->setStyleSheet("border-image: url(:/picture/crashwarn.png);");//报警
		if(sta == 0xff) ui->label_cra21->setStyleSheet("border-image: url(:/picture/uartwro.png);");//通信故障
        break;
    case 22:
		if(sta == 0) ui->label_cra22->setStyleSheet("border-image: url(:/picture/right.png);");//正常
		if(sta == 1) ui->label_cra22->setStyleSheet("border-image: url(:/picture/crashwarn.png);");//报警
		if(sta == 0xff) ui->label_cra22->setStyleSheet("border-image: url(:/picture/uartwro.png);");//通信故障
        break;
    case 23:
		if(sta == 0) ui->label_cra23->setStyleSheet("border-image: url(:/picture/right.png);");//正常
		if(sta == 1) ui->label_cra23->setStyleSheet("border-image: url(:/picture/crashwarn.png);");//报警
		if(sta == 0xff) ui->label_cra23->setStyleSheet("border-image: url(:/picture/uartwro.png);");//通信故障
        break;
    case 24:
		if(sta == 0) ui->label_cra24->setStyleSheet("border-image: url(:/picture/right.png);");//正常
		if(sta == 1) ui->label_cra24->setStyleSheet("border-image: url(:/picture/crashwarn.png);");//报警
		if(sta == 0xff) ui->label_cra24->setStyleSheet("border-image: url(:/picture/uartwro.png);");//通信故障
        break;
    case 25:
		if(sta == 0) ui->label_cra25->setStyleSheet("border-image: url(:/picture/right.png);");//正常
		if(sta == 1) ui->label_cra25->setStyleSheet("border-image: url(:/picture/crashwarn.png);");//报警
		if(sta == 0xff) ui->label_cra25->setStyleSheet("border-image: url(:/picture/uartwro.png);");//通信故障
        break;
    case 26:
		if(sta == 0) ui->label_cra26->setStyleSheet("border-image: url(:/picture/right.png);");//正常
		if(sta == 1) ui->label_cra26->setStyleSheet("border-image: url(:/picture/crashwarn.png);");//报警
		if(sta == 0xff) ui->label_cra26->setStyleSheet("border-image: url(:/picture/uartwro.png);");//通信故障
        break;
    case 27:
		if(sta == 0) ui->label_cra27->setStyleSheet("border-image: url(:/picture/right.png);");//正常
		if(sta == 1) ui->label_cra27->setStyleSheet("border-image: url(:/picture/crashwarn.png);");//报警
		if(sta == 0xff) ui->label_cra27->setStyleSheet("border-image: url(:/picture/uartwro.png);");//通信故障
        break;
    case 28:
		if(sta == 0) ui->label_cra28->setStyleSheet("border-image: url(:/picture/right.png);");//正常
		if(sta == 1) ui->label_cra28->setStyleSheet("border-image: url(:/picture/crashwarn.png);");//报警
		if(sta == 0xff) ui->label_cra28->setStyleSheet("border-image: url(:/picture/uartwro.png);");//通信故障
        break;
    case 29:
		if(sta == 0) ui->label_cra29->setStyleSheet("border-image: url(:/picture/right.png);");//正常
		if(sta == 1) ui->label_cra29->setStyleSheet("border-image: url(:/picture/crashwarn.png);");//报警
		if(sta == 0xff) ui->label_cra29->setStyleSheet("border-image: url(:/picture/uartwro.png);");//通信故障
        break;
    case 30:
		if(sta == 0) ui->label_cra30->setStyleSheet("border-image: url(:/picture/right.png);");//正常
		if(sta == 1) ui->label_cra30->setStyleSheet("border-image: url(:/picture/crashwarn.png);");//报警
		if(sta == 0xff) ui->label_cra30->setStyleSheet("border-image: url(:/picture/uartwro.png);");//通信故障
        break;
    case 31:
		if(sta == 0) ui->label_cra31->setStyleSheet("border-image: url(:/picture/right.png);");//正常
		if(sta == 1) ui->label_cra31->setStyleSheet("border-image: url(:/picture/crashwarn.png);");//报警
		if(sta == 0xff) ui->label_cra31->setStyleSheet("border-image: url(:/picture/uartwro.png);");//通信故障
        break;
    case 32:
		if(sta == 0) ui->label_cra32->setStyleSheet("border-image: url(:/picture/right.png);");//正常
		if(sta == 1) ui->label_cra32->setStyleSheet("border-image: url(:/picture/crashwarn.png);");//报警
		if(sta == 0xff) ui->label_cra32->setStyleSheet("border-image: url(:/picture/uartwro.png);");//通信故障
        break;
    }
}
//高液位报警和潜油泵的相关函数

/*关于报警亮屏的数组
 *     正常是0，息屏，不正常是1，不会息屏
 *     Flag_Timeto_CloseNeeded[0]   按钮相关，登录，历史记录，系统设置等等等
 *     Flag_Timeto_CloseNeeded[1]
 *     Flag_Timeto_CloseNeeded[2]   加油机通信故障
 *     Flag_Timeto_CloseNeeded[3]   压力变送器状态
 *     Flag_Timeto_CloseNeeded[4]   可燃气体状态检测
 *     Flag_Timeto_CloseNeeded[5]   加油机预警
 *     //Flag_Timeto_CloseNeeded[6]
 *
 *     可燃气体监测部分有蜂鸣器报警
*/

void MainWindow::on_pushButton_close_warn_rom_clicked()//存储空间不足提醒
{
    ui->widget_warn_rom->setHidden(1);
}
void MainWindow::hide_tablewidget(unsigned char which, unsigned char sta)
{
    qDebug()<<"screen hide"<<which<<sta;
	ui->tabWidget->setTabEnabled(0,Flag_screen_zaixian);
	ui->tabWidget->setTabEnabled(1,Flag_screen_xielou);
	ui->tabWidget->setTabEnabled(2,Flag_screen_radar);
	ui->tabWidget->setTabEnabled(3,Flag_screen_safe);
	ui->tabWidget->setTabEnabled(4,Flag_screen_burngas);
    ui->tabWidget->setTabEnabled(5,Flag_screen_cc);
    ui->tabWidget->setTabEnabled(6,Flag_screen_ywy);
	ui->tabWidget->setStyleSheet("QTabBar::tab:abled {max-height:82px;min-width:147px;background-color: rgb(170,170,255,255);border: 0px solid;border-top-left-radius: 0px;border-top-right-radius: 0px;padding:0px;}\
	                                QTabBar::tab:!selected {margin-top: 0px;background-color:transparent;}\
	                                QTabBar::tab:selected {background-color: white}\
	                                QTabWidget::pane {border-top:0px solid #e8f3f9;background:  transparent;}\
	                                QTabBar::tab:disabled {width: 0;height: 0; color: transparent;padding:0px;border: 0px solid}");
}

//关闭报警弹窗
void MainWindow::on_pushButton_close_zaixianjiance_clicked()
{
    ui->widget_warn_zaixianjiance->setHidden(1);
    if(ui->checkBox_cancel_show->isChecked())//选中则不再显示，
    {
        Flag_Reoilgas_DayShow = 1;  //今天不再显示
    }
    else
    {
        Flag_Reoilgas_DayShow = 0;
    }
}
//显示油气回收报警的详细信息，创建新窗体
void MainWindow::on_pushButton_hiswarn_zaixianjiance_clicked()
{
    ui->widget_warn_zaixianjiance->setHidden(1);
    reoilgas_pop_exec = new reoilgas_pop;
    connect(this,SIGNAL(refresh_dispener_data_signal()),reoilgas_pop_exec,SLOT(refresh_dispener_data()));
    reoilgas_pop_exec->show();
}

void MainWindow::show_pop_ups(int sta)//显示在线监测弹窗
{
    if(sta == 1)
    {
        ui->widget_warn_zaixianjiance->setHidden(0);
    }
    if(sta == 0)
    {
        ui->widget_warn_zaixianjiance->setHidden(1);
    }

}

void MainWindow::on_toolButton_clicked()
{
    on_pushButton_hiswarn_zaixianjiance_clicked();
}
void MainWindow::refresh_dispener_data_slot()
{
    emit refresh_dispener_data_signal();
}

/**************故障数据网络上传******************
 * id         没有用到 预留
 * whichone   哪个枪报警
 * ******************************/
void MainWindow::network_Wrongsdata(QString id ,QString whichone)//报警网络数据上传
{
	if(1)
	{

		id = id;
		qDebug()<<"network send Wrongsdata!" << Flag_Network_Send_Version;
		if(Flag_Network_Send_Version == 0)//福建协议
		{
			QString wrongdata_post = "0111";//post添加
			emit Send_Wrongsdata(DATAID_POST,wrongdata_post.append(QString("%1").arg(Mapping[2*(whichone.toInt())-2], 2, 10, QLatin1Char('0')))); //只发送采集器第一把枪
		}
		if(Flag_Network_Send_Version == 1)//广州协议
		{

		}
		if(Flag_Network_Send_Version == 2)//重庆协议
		{
			QString wrongdata_post = "0111";//post添加
			refueling_wrongdata_cq(wrongdata_post.append(QString("%1").arg(whichone.toInt(), 2, 10, QLatin1Char('0'))));//只发送采集器第一把枪
		}
		if(Flag_Network_Send_Version == 3)//唐山协议，与福建相同
		{
			QString wrongdata_post = "0111";//post添加
			emit Send_Wrongsdata(DATAID_POST,wrongdata_post.append(QString("%1").arg(whichone.toInt(), 2, 10, QLatin1Char('0')))); //只发送采集器第一把枪
		}
		if(Flag_Network_Send_Version == 4)//湖南协议，与福建类似
		{
			QString wrongdata_post = "0111";//post添加
			emit Send_Wrongsdat_HuNan(DATAID_POST,wrongdata_post.append(QString("%1").arg(whichone.toInt(), 2, 10, QLatin1Char('0'))));//只发送采集器第一把枪
		}
		if(Flag_Network_Send_Version == 5)//江门协议，与唐山协议，与福建相同
		{
			QString wrongdata_post = "0111";//post添加
			emit Send_Wrongsdata(DATAID_POST,wrongdata_post.append(QString("%1").arg(whichone.toInt(), 2, 10, QLatin1Char('0')))); //只发送采集器第一把枪
		}
		if(Flag_Network_Send_Version == 6)//佛山协议
		{
			QString wrongdata_post = "0111";//post添加
			emit send_wrong_foshan(DATAID_POST,"date",wrongdata_post.append(QString("%1").arg(whichone.toInt(), 2, 10, QLatin1Char('0')))); //只发送采集器第一把枪
		}
        if(Flag_Network_Send_Version == 8)//东莞协议
        {
            QString wrongdata_post = "0111";//post添加
            emit Send_Wrongsdat_dongguan(DATAID_POST,wrongdata_post.append(QString("%1").arg(whichone.toInt(), 2, 10, QLatin1Char('0'))));
        }

	}
}

/*****************************************************************************
 * 液位仪
*******************************************************************************/
//与MainWindow相关

void MainWindow::Initialization_label()
{
   ui->label_alarm_11->setHidden(1);
   ui->label_alarm_21->setHidden(1);
   ui->label_alarm_31->setHidden(1);
   ui->label_alarm_41->setHidden(1);
   ui->label_alarm_51->setHidden(1);
   ui->label_alarm_61->setHidden(1);
}

void MainWindow::Display_Height_Data(unsigned char add, QString str1,QString str2,QString str3) //1 油 2 水 3 温度
{
    float volume,volumeused;
    unsigned int i_oil,i_water,r_table,g_diameter;
    QDateTime start_add_oil_time  = QDateTime::currentDateTime();

    if(((int)(str1.toFloat())%10) >= 5)
    {
        i_oil = str1.toFloat()/10 + 1;
    }
    else
    {
        i_oil = str1.toFloat()/10;
    }
    if(((int)(str2.toFloat())%10) >= 5)
    {
        i_water = str2.toFloat()/10 + 1;
    }
    else
    {
        i_water = str2.toFloat()/10;
    }

    if(i_oil>299 || i_water>299)  //屏蔽水位异常
    {
        return;
    }

    r_table = OilTank_Set[add-1][4];    //对应罐表
    g_diameter = OilTank_Set[add-1][0] /10;    //对应罐直径  CM
    volume = (Oil_Tank_Table[r_table][i_oil] - Oil_Tank_Table[r_table][i_water]) *1000;
    volumeused = (Oil_Tank_Table[r_table][g_diameter] - Oil_Tank_Table[r_table][i_oil]) *1000;


    //自动进油相关
    if(add_oil_all[add].enable) //重新上电时获取一下高度值
    {
        add_oil_all[add].start_oil_height = str1.toFloat();
        add_oil_all[add].enable = false;
    }
    if(str1.toFloat() > add_oil_all[add].start_oil_height +2)// 2为波动值 add_oil_all[add].begin_time.secsTo(end_add_oil_time)> 20)  //自动进油　高度单位:ｍｍ
    {
        add_oil_all[add].average[add_oil_all[add].count++] = 1;
    }
    else
    {
       add_oil_all[add].average[add_oil_all[add].count++] = 0;
    }


    if(add_oil_all[add].count > add_oil_all[add].size)
    {
      for(int i=0;i<add_oil_all[add].size;i++)
      {
         add_oil_all[add].sum =  add_oil_all[add].average[i] + add_oil_all[add].sum;
      }
      if(add_oil_all[add].sum > 15) //正在卸油
      {
         if(add_oil_all[add].flag_entering)
         {
            add_oil_all[add].flag_entering = 0;
            add_oil_all[add].begin_time =  start_add_oil_time;

         }
      }
      else if(add_oil_all[add].sum < 5) //液面稳定
      {
          if(add_oil_all[add].flag_entering == 0) //卸油结束后的稳定
          {
              add_oil_all[add].flag_entering = 1;
              if(str1.toFloat() - add_oil_all[add].start_oil_height >10)  //屏蔽最小卸油量 10mm
              {
                 add_oil_all[add].end_oil_height = str1.toFloat();
                 add_oil_all[add].end_water_height = str2.toFloat();
                 add_oil_all[add].end_oil_volume = volume;
                 add_oil_all[add].end_water_volume = Oil_Tank_Table[r_table][i_water] *1000;
                 add_oil_all[add].end_temp = str3.toFloat();
                 add_oil_all[add].end_time = QDateTime::currentDateTime();
                 emit signal_Display_addoil_data(add);
                 timer_open_addoil->start();
              }

          }
          else  //下降或者非卸油后的稳定
          {
              add_oil_all[add].start_oil_height = str1.toFloat();
              add_oil_all[add].start_water_height = str2.toFloat();
              add_oil_all[add].start_oil_volume = volume;
              add_oil_all[add].start_temp = str3.toFloat();
              add_oil_all[add].start_water_volume = Oil_Tank_Table[r_table][i_water] *1000;
              start_add_oil_time  = QDateTime::currentDateTime();
          }

      }
      qDebug()<<"油罐　"<<add <<" sum "<<add_oil_all[add].sum;
      qDebug()<<"begin_time　"<<add_oil_all[add].begin_time.toString("yyyy-MM-dd  hh:mm:ss")<<"  start_oil_height "<<add_oil_all[add].start_oil_height;
      qDebug()<<"end_time　  "<<add_oil_all[add].end_time.toString("yyyy-MM-dd  hh:mm:ss")<<  "  end_oil_height   "<<add_oil_all[add].end_oil_height;

      add_oil_all[add].sum = 0;
      add_oil_all[add].count = 0;
    }




    //云飞扬上传相关
    Reply_Data_OT[add][1].f = volume;
    Reply_Data_OT[add][2].f = volume * 0.98;  //TC容积 温度补偿后的体积  0.98不准 需要改
    Reply_Data_OT[add][3].f = volumeused;
    Reply_Data_OT[add][7].f = Oil_Tank_Table[r_table][i_water] *1000 ;

    add_Oil_array[add][0] = str1.toFloat();

//    QString strvolume = QString::number(volume, 'f', 1);//限制 float 小数位数
//    QString strvolume = QString("%1").arg(volume);
//    printf("%s \n",strvolume);fflush(stdout);

    switch (add)
    {
        case 1:
//            r_table = OilTank_Set[0][4];    //对应罐表
//            g_diameter = OilTank_Set[0][0] /10;    //对应罐直径  CM
//            volume = (Oil_Tank_Table[r_table][i_oil] - Oil_Tank_Table[r_table][i_water]) *1000;
//            volumeused = (Oil_Tank_Table[r_table][g_diameter] - Oil_Tank_Table[r_table][i_oil]) *1000;

            add_Oil_array[add][1] = Oil_Tank_Table[r_table][i_oil] * 1000;

            ui->drawHeight_11->setValue(g_diameter,0,i_oil,i_water);

            ui->label_volume_11->setText(QString(" 油品体积：%1%2").arg(volume).arg(" L"));
            ui->label_OilHeight_11->setText(QString(" 油高：%1%2").arg(str1.toStdString().data()).arg(" mm"));
            ui->label_WaterHeight_11->setText(QString(" 水高：%1%2").arg(str2.toStdString().data()).arg(" mm"));
            ui->label_Temp_11->setText(QString(" 温度：%1%2").arg(str3.toStdString().data()).arg(" ℃"));
            ui->label_volumeused_11->setText(QString(" 可卸量：%1%2").arg(volumeused).arg(" L"));
            break;
        case 2:
//            r_table = OilTank_Set[1][4];    //对应罐表
//            g_diameter = OilTank_Set[1][0] /10;    //对应罐直径  CM
//            volume = (Oil_Tank_Table[r_table][i_oil] - Oil_Tank_Table[r_table][i_water]) *1000;
//            volumeused = (Oil_Tank_Table[r_table][g_diameter] - Oil_Tank_Table[r_table][i_oil]) *1000;

            add_Oil_array[add][1] = Oil_Tank_Table[r_table][i_oil] * 1000;

            ui->drawHeight_21->setValue(g_diameter,0,i_oil,i_water);

            ui->label_volume_21->setText(QString(" 油品体积：%1%2").arg(volume).arg(" L"));
            ui->label_OilHeight_21->setText(QString(" 油高：%1%2").arg(str1.toStdString().data()).arg(" mm"));
            ui->label_WaterHeight_21->setText(QString(" 水高：%1%2").arg(str2.toStdString().data()).arg(" mm"));
            ui->label_Temp_21->setText(QString(" 温度：%1%2").arg(str3.toStdString().data()).arg(" ℃"));
            ui->label_volumeused_21->setText(QString(" 可卸量：%1%2").arg(volumeused).arg(" L"));
            break;
        case 3:
//            r_table = OilTank_Set[2][4];    //对应罐表
//            g_diameter = OilTank_Set[2][0] /10;    //对应罐直径  CM
//            volume = (Oil_Tank_Table[r_table][i_oil] - Oil_Tank_Table[r_table][i_water]) *1000;
//            volumeused = (Oil_Tank_Table[r_table][g_diameter] - Oil_Tank_Table[r_table][i_oil]) *1000;

            add_Oil_array[add][1] = Oil_Tank_Table[r_table][i_oil] * 1000;

            ui->drawHeight_31->setValue(g_diameter,0,i_oil,i_water);

            ui->label_volume_31->setText(QString(" 油品体积：%1%2").arg(volume).arg(" L"));
            ui->label_OilHeight_31->setText(QString(" 油高：%1%2").arg(str1.toStdString().data()).arg(" mm"));
            ui->label_WaterHeight_31->setText(QString(" 水高：%1%2").arg(str2.toStdString().data()).arg(" mm"));
            ui->label_Temp_31->setText(QString(" 温度：%1%2").arg(str3.toStdString().data()).arg(" ℃"));
            ui->label_volumeused_31->setText(QString(" 可卸量：%1%2").arg(volumeused).arg(" L"));
            break;
        case 4:
//            r_table = OilTank_Set[3][4];    //对应罐表
//            g_diameter = OilTank_Set[3][0] /10;    //对应罐直径  CM
//            volume = (Oil_Tank_Table[r_table][i_oil] - Oil_Tank_Table[r_table][i_water]) *1000;
//            volumeused = (Oil_Tank_Table[r_table][g_diameter] - Oil_Tank_Table[r_table][i_oil]) *1000;

            add_Oil_array[add][1] = Oil_Tank_Table[r_table][i_oil] * 1000;

            ui->drawHeight_41->setValue(g_diameter,0,i_oil,i_water);

            ui->label_volume_41->setText(QString(" 油品体积：%1%2").arg(volume).arg(" L"));
            ui->label_OilHeight_41->setText(QString(" 油高：%1%2").arg(str1.toStdString().data()).arg(" mm"));
            ui->label_WaterHeight_41->setText(QString(" 水高：%1%2").arg(str2.toStdString().data()).arg(" mm"));
            ui->label_Temp_41->setText(QString(" 温度：%1%2").arg(str3.toStdString().data()).arg(" ℃"));
            ui->label_volumeused_41->setText(QString(" 可卸量：%1%2").arg(volumeused).arg(" L"));
            break;
        case 5:
//            r_table = OilTank_Set[4][4];    //对应罐表
//            g_diameter = OilTank_Set[4][0] /10;    //对应罐直径  CM
//            volume = (Oil_Tank_Table[r_table][i_oil] - Oil_Tank_Table[r_table][i_water]) *1000;
//            volumeused = (Oil_Tank_Table[r_table][g_diameter] - Oil_Tank_Table[r_table][i_oil]) *1000;

            add_Oil_array[add][1] = Oil_Tank_Table[r_table][i_oil] * 1000;

            ui->drawHeight_51->setValue(g_diameter,0,i_oil,i_water);

            ui->label_volume_51->setText(QString(" 油品体积：%1%2").arg(volume).arg(" L"));
            ui->label_OilHeight_51->setText(QString(" 油高：%1%2").arg(str1.toStdString().data()).arg(" mm"));
            ui->label_WaterHeight_51->setText(QString(" 水高：%1%2").arg(str2.toStdString().data()).arg(" mm"));
            ui->label_Temp_51->setText(QString(" 温度：%1%2").arg(str3.toStdString().data()).arg(" ℃"));
            ui->label_volumeused_51->setText(QString(" 可卸量：%1%2").arg(volumeused).arg(" L"));
            break;
        case 6:
//            r_table = OilTank_Set[5][4];    //对应罐表
//            g_diameter = OilTank_Set[5][0] /10;    //对应罐直径  CM
//            volume = (Oil_Tank_Table[r_table][i_oil] - Oil_Tank_Table[r_table][i_water]) *1000;
//            volumeused = (Oil_Tank_Table[r_table][g_diameter] - Oil_Tank_Table[r_table][i_oil]) *1000;

            add_Oil_array[add][1] = Oil_Tank_Table[r_table][i_oil] * 1000;

            ui->drawHeight_61->setValue(g_diameter,0,i_oil,i_water);

            ui->label_volume_61->setText(QString(" 油品体积：%1%2").arg(volume).arg(" L"));
            ui->label_OilHeight_61->setText(QString(" 油高：%1%2").arg(str1.toStdString().data()).arg(" mm"));
            ui->label_WaterHeight_61->setText(QString(" 水高：%1%2").arg(str2.toStdString().data()).arg(" mm"));
            ui->label_Temp_61->setText(QString(" 温度：%1%2").arg(str3.toStdString().data()).arg(" ℃"));
            ui->label_volumeused_61->setText(QString(" 可卸量：%1%2").arg(volumeused).arg(" L"));
            break;
        default:
            break;
    }
}

void MainWindow::Display_alarm_Tangan_Data(unsigned char add, unsigned char flag)   //0 通讯正常 1 通讯故障  2 油溢出  3 水过高  4 浮子故障  5 温度传感器故障
{
    if(add == 1)
    {
        switch (flag)
        {
            case 0:
                ui->label_alarm_11->setHidden(0);
                ui->label_alarm_11->setText(" 通讯故障");

                ui->label_volume_11->setText(QString(" 油品体积：%1%2").arg("0.0").arg(" L"));
                ui->label_OilHeight_11->setText(QString(" 油高：%1%2").arg("0.0").arg(" mm"));
                ui->label_WaterHeight_11->setText(QString(" 水高：%1%2").arg("0.0").arg(" mm"));
                ui->label_Temp_11->setText(QString(" 温度：%1%2").arg("0.0").arg(" ℃"));
                ui->label_volumeused_11->setText(QString(" 可卸量：%1%2").arg("0.0").arg(" L"));
                break;
            case 1:
                ui->label_alarm_11->setHidden(0);
                ui->label_alarm_11->setText(" 油位过高");
                break;
            case 2:
                ui->label_alarm_11->setHidden(0);
                ui->label_alarm_11->setText(" 油位过低");
                break;
            case 3:
                ui->label_alarm_11->setHidden(0);
                ui->label_alarm_11->setText(" 浮子故障");
                break;
            case 4:
                ui->label_alarm_11->setHidden(0);
                ui->label_alarm_11->setText(" 温度传感器故障");
                break;
            case 5:
                ui->label_alarm_11->setHidden(0);
                ui->label_alarm_11->setText(" 高水位报警");
                break;
            case 6:
                ui->label_alarm_11->setText("正常");
                ui->label_alarm_11->setHidden(1);
                break;
            default:
                break;
        }
    }
    else if(add ==2)
    {
        switch (flag)
        {
            case 0:
                ui->label_alarm_21->setHidden(0);
                ui->label_alarm_21->setText(" 通讯故障");

                ui->label_volume_21->setText(QString(" 油品体积：%1%2").arg("0.0").arg(" L"));
                ui->label_OilHeight_21->setText(QString(" 油高：%1%2").arg("0.0").arg(" mm"));
                ui->label_WaterHeight_21->setText(QString(" 水高：%1%2").arg("0.0").arg(" mm"));
                ui->label_Temp_21->setText(QString(" 温度：%1%2").arg("0.0").arg(" ℃"));
                ui->label_volumeused_21->setText(QString(" 可卸量：%1%2").arg("0.0").arg(" L"));
                break;
            case 1:
                ui->label_alarm_21->setHidden(0);
                ui->label_alarm_21->setText(" 油位过高");
                break;
            case 2:
                ui->label_alarm_21->setHidden(0);
                ui->label_alarm_21->setText(" 油位过低");
                break;
            case 3:
                ui->label_alarm_21->setHidden(0);
                ui->label_alarm_21->setText(" 浮子故障");
                break;
            case 4:
                ui->label_alarm_21->setHidden(0);
                ui->label_alarm_21->setText(" 温度传感器故障");
                break;
            case 5:
                ui->label_alarm_21->setHidden(0);
                ui->label_alarm_21->setText(" 高水位报警");
                break;
            case 6:
                ui->label_alarm_21->setText("正常");
                ui->label_alarm_21->setHidden(1);
                break;
                default:
                    break;
        }
    }
    else if(add ==3)
    {
        switch (flag)
        {
            case 0:
                ui->label_alarm_31->setHidden(0);
                ui->label_alarm_31->setText(" 通讯故障");

                ui->label_volume_31->setText(QString(" 油品体积：%1%2").arg("0.0").arg(" L"));
                ui->label_OilHeight_31->setText(QString(" 油高：%1%2").arg("0.0").arg(" mm"));
                ui->label_WaterHeight_31->setText(QString(" 水高：%1%2").arg("0.0").arg(" mm"));
                ui->label_Temp_31->setText(QString(" 温度：%1%2").arg("0.0").arg(" ℃"));
                ui->label_volumeused_31->setText(QString(" 可卸量：%1%2").arg("0.0").arg(" L"));
                break;
            case 1:
                ui->label_alarm_31->setHidden(0);
                ui->label_alarm_31->setText(" 油位过高");
                break;
            case 2:
                ui->label_alarm_31->setHidden(0);
                ui->label_alarm_31->setText(" 油位过低");
                break;
            case 3:
                ui->label_alarm_31->setHidden(0);
                ui->label_alarm_31->setText(" 浮子故障");
                break;
            case 4:
                ui->label_alarm_31->setHidden(0);
                ui->label_alarm_31->setText(" 温度传感器故障");
                break;
            case 5:
                ui->label_alarm_31->setHidden(0);
                ui->label_alarm_31->setText(" 高水位报警");
                break;
            case 6:
                ui->label_alarm_31->setText("正常");
                ui->label_alarm_31->setHidden(1);
                break;
                default:
                    break;
        }
    }
    else if(add ==4)
    {
        switch (flag)
        {
            case 0:
                ui->label_alarm_41->setHidden(0);
                ui->label_alarm_41->setText(" 通讯故障");

                ui->label_volume_41->setText(QString(" 油品体积：%1%2").arg("0.0").arg(" L"));
                ui->label_OilHeight_41->setText(QString(" 油高：%1%2").arg("0.0").arg(" mm"));
                ui->label_WaterHeight_41->setText(QString(" 水高：%1%2").arg("0.0").arg(" mm"));
                ui->label_Temp_41->setText(QString(" 温度：%1%2").arg("0.0").arg(" ℃"));
                ui->label_volumeused_41->setText(QString(" 可卸量：%1%2").arg("0.0").arg(" L"));
                break;
            case 1:
                ui->label_alarm_41->setHidden(0);
                ui->label_alarm_41->setText(" 油位过高");
                break;
            case 2:
                ui->label_alarm_41->setHidden(0);
                ui->label_alarm_41->setText(" 油位过低");
                break;
            case 3:
                ui->label_alarm_41->setHidden(0);
                ui->label_alarm_41->setText(" 浮子故障");
                break;
            case 4:
                ui->label_alarm_41->setHidden(0);
                ui->label_alarm_41->setText(" 温度传感器故障");
                break;
            case 5:
                ui->label_alarm_41->setHidden(0);
                ui->label_alarm_41->setText(" 高水位报警");
                break;
            case 6:
                ui->label_alarm_41->setText("正常");
                ui->label_alarm_41->setHidden(1);
                break;
                default:
                    break;
        }
    }
    else if(add ==5)
    {
        switch (flag)
        {
            case 0:
                ui->label_alarm_51->setHidden(0);
                ui->label_alarm_51->setText(" 通讯故障");

                ui->label_volume_51->setText(QString(" 油品体积：%1%2").arg("0.0").arg(" L"));
                ui->label_OilHeight_51->setText(QString(" 油高：%1%2").arg("0.0").arg(" mm"));
                ui->label_WaterHeight_51->setText(QString(" 水高：%1%2").arg("0.0").arg(" mm"));
                ui->label_Temp_51->setText(QString(" 温度：%1%2").arg("0.0").arg(" ℃"));
                ui->label_volumeused_51->setText(QString(" 可卸量：%1%2").arg("0.0").arg(" L"));
                break;
            case 1:
                ui->label_alarm_51->setHidden(0);
                ui->label_alarm_51->setText(" 油位过高");
                break;
            case 2:
                ui->label_alarm_51->setHidden(0);
                ui->label_alarm_51->setText(" 油位过低");
                break;
            case 3:
                ui->label_alarm_51->setHidden(0);
                ui->label_alarm_51->setText(" 浮子故障");
                break;
            case 4:
                ui->label_alarm_51->setHidden(0);
                ui->label_alarm_51->setText(" 温度传感器故障");
                break;
            case 5:
                ui->label_alarm_51->setHidden(0);
                ui->label_alarm_51->setText(" 高水位报警");
                break;
            case 6:
                ui->label_alarm_51->setText("正常");
                ui->label_alarm_51->setHidden(1);
                break;
                default:
                    break;
        }
    }
    else if(add ==6)
    {
        switch (flag)
        {
            case 0:
                ui->label_alarm_61->setHidden(0);
                ui->label_alarm_61->setText(" 通讯故障");

                ui->label_volume_61->setText(QString(" 油品体积：%1%2").arg("0.0").arg(" L"));
                ui->label_OilHeight_61->setText(QString(" 油高：%1%2").arg("0.0").arg(" mm"));
                ui->label_WaterHeight_61->setText(QString(" 水高：%1%2").arg("0.0").arg(" mm"));
                ui->label_Temp_61->setText(QString(" 温度：%1%2").arg("0.0").arg(" ℃"));
                ui->label_volumeused_61->setText(QString(" 可卸量：%1%2").arg("0.0").arg(" L"));
                break;
            case 1:
                ui->label_alarm_61->setHidden(0);
                ui->label_alarm_61->setText(" 油位过高");
                break;
            case 2:
                ui->label_alarm_61->setHidden(0);
                ui->label_alarm_61->setText(" 油位过低");
                break;
            case 3:
                ui->label_alarm_61->setHidden(0);
                ui->label_alarm_61->setText(" 浮子故障");
                break;
            case 4:
                ui->label_alarm_61->setHidden(0);
                ui->label_alarm_61->setText(" 温度传感器故障");
                break;
            case 5:
                ui->label_alarm_61->setHidden(0);
                ui->label_alarm_61->setText(" 高水位报警");
                break;
            case 6:
                ui->label_alarm_61->setText("正常");
                ui->label_alarm_61->setHidden(1);
                break;
                default:
                    break;
        }
    }
}

//与systemset相关
void MainWindow::draw_OilTank(int t)
{
    switch(t)
    {
       case 0:
            ui->widget_OilTank_1->setHidden(1);
            ui->widget_OilTank_2->setHidden(1);
            ui->widget_OilTank_3->setHidden(1);
            ui->widget_OilTank_4->setHidden(1);
            ui->widget_OilTank_5->setHidden(1);
            ui->widget_OilTank_6->setHidden(1);
            break;
        case 1:
             ui->widget_OilTank_1->setHidden(0);
             ui->widget_OilTank_2->setHidden(1);
             ui->widget_OilTank_3->setHidden(1);
             ui->widget_OilTank_4->setHidden(1);
             ui->widget_OilTank_5->setHidden(1);
             ui->widget_OilTank_6->setHidden(1);
             break;
        case 2:
             ui->widget_OilTank_1->setHidden(0);
             ui->widget_OilTank_2->setHidden(0);
             ui->widget_OilTank_3->setHidden(1);
             ui->widget_OilTank_4->setHidden(1);
             ui->widget_OilTank_5->setHidden(1);
             ui->widget_OilTank_6->setHidden(1);
             break;
        case 3:
             ui->widget_OilTank_1->setHidden(0);
             ui->widget_OilTank_2->setHidden(0);
             ui->widget_OilTank_3->setHidden(0);
             ui->widget_OilTank_4->setHidden(1);
             ui->widget_OilTank_5->setHidden(1);
             ui->widget_OilTank_6->setHidden(1);
             break;
        case 4:
             ui->widget_OilTank_1->setHidden(0);
             ui->widget_OilTank_2->setHidden(0);
             ui->widget_OilTank_3->setHidden(0);
             ui->widget_OilTank_4->setHidden(0);
             ui->widget_OilTank_5->setHidden(1);
             ui->widget_OilTank_6->setHidden(1);
             break;
        case 5:
             ui->widget_OilTank_1->setHidden(0);
             ui->widget_OilTank_2->setHidden(0);
             ui->widget_OilTank_3->setHidden(0);
             ui->widget_OilTank_4->setHidden(0);
             ui->widget_OilTank_5->setHidden(0);
             ui->widget_OilTank_6->setHidden(1);
             break;
        case 6:
             ui->widget_OilTank_1->setHidden(0);
             ui->widget_OilTank_2->setHidden(0);
             ui->widget_OilTank_3->setHidden(0);
             ui->widget_OilTank_4->setHidden(0);
             ui->widget_OilTank_5->setHidden(0);
             ui->widget_OilTank_6->setHidden(0);
             break;
    }
}

void MainWindow::draw_Tangan()
{
    unsigned char flag_enable[12][3] = {0};
    for(unsigned char i = 0;i < Amount_OilTank;i++)
    {
        for(unsigned char j = 0;j < 3;j++)
        {
            if(Tangan_Amount[i] > 0)
            {
                if(Tangan_Amount[i] > j)
                {
                    flag_enable[i][j] = 1;
                }
            }
        }
    }

    ui->frame_11->setVisible(flag_enable[0][0]);

    ui->frame_21->setVisible(flag_enable[1][0]);

    ui->frame_31->setVisible(flag_enable[2][0]);

    ui->frame_41->setVisible(flag_enable[3][0]);

    ui->frame_51->setVisible(flag_enable[4][0]);

    ui->frame_61->setVisible(flag_enable[5][0]);

    if(flag_enable[0][0]){ui->label_OilTank_1->setText(QString(" 1#油罐: %1%2").arg(Oil_Kind[0][0]).arg("#"));}//ui->label_OilTank_1->adjustSize();
    if(flag_enable[1][0]){ui->label_OilTank_2->setText(QString(" 2#油罐: %1%2").arg(Oil_Kind[1][0]).arg("#"));}
    if(flag_enable[2][0]){ui->label_OilTank_3->setText(QString(" 3#油罐: %1%2").arg(Oil_Kind[2][0]).arg("#"));}
    if(flag_enable[3][0]){ui->label_OilTank_4->setText(QString(" 4#油罐: %1%2").arg(Oil_Kind[3][0]).arg("#"));}
    if(flag_enable[4][0]){ui->label_OilTank_5->setText(QString(" 5#油罐: %1%2").arg(Oil_Kind[4][0]).arg("#"));}
    if(flag_enable[5][0]){ui->label_OilTank_6->setText(QString(" 6#油罐: %1%2").arg(Oil_Kind[5][0]).arg("#"));}

}

void MainWindow::set_Tangan_add_success_slot(unsigned char add)
{
    emit set_Tangan_add_success_signal(add);
}

void MainWindow::Send_compensation_slot(unsigned char command,unsigned char hang,float data)
{
    emit Send_compensation_Signal(command,hang,data);
}

void MainWindow::compensation_set_result_slot(unsigned char command,unsigned char add,QString str)
{
    emit compensation_set_result_Signal(command,add,str);
}

//进油管理
void MainWindow::on_btn_enter_add_Oil_clicked()
{
    ui->btn_enter_add_Oil->setDisabled(1);
    ui->comboBox_addoil->clear();

    for(int i=0; i<=Amount_OilTank;i++)
    {
        if(i_alarm_record[i][0] == 0)
        {
           ui->comboBox_addoil->addItem(QString("%1").arg(i));
        }
    }
    ui->widget_add_oil->setHidden(0);
}

void MainWindow::on_comboBox_addoil_currentIndexChanged(int index)
{
    addoil_add_num = index;
    model =  new QStandardItemModel(); //进油记录
    model->setColumnCount(3);
    ui->tableView_addoil->verticalHeader()->setHidden(1);//隐藏行号
    ui->tableView_addoil->horizontalHeader()->setDefaultSectionSize(150);

    model->setHeaderData(0,Qt::Horizontal,QObject::tr("%1").arg(""));
    model->setHeaderData(1,Qt::Horizontal,QObject::tr("%1").arg("开始"));
    model->setHeaderData(2,Qt::Horizontal,QObject::tr("%1").arg("结束"));
    model->setItem(0,0,new QStandardItem(QString("时间")));
    model->item(0,0)->setTextAlignment(Qt::AlignCenter);
    model->setItem(1,0,new QStandardItem(QString("高度")));
    model->item(1,0)->setTextAlignment(Qt::AlignCenter);
    model->setItem(2,0,new QStandardItem(QString("体积")));
    model->item(2,0)->setTextAlignment(Qt::AlignCenter);
    model->setItem(3,0,new QStandardItem(QString("进油量")));
    model->item(3,0)->setTextAlignment(Qt::AlignCenter);

    ui->tableView_addoil->setSpan(3,1,3,2);

    ui->tableView_addoil->setModel(model);

    ui->tableView_addoil->setColumnWidth(0,100);
    ui->tableView_addoil->setColumnWidth(1,220);
    ui->tableView_addoil->setColumnWidth(2,220);
}

void MainWindow::on_btn_addoil_start_clicked()
{
    QDateTime current_datetime = QDateTime::currentDateTime();
    start_datetime_str = current_datetime.toString("yyyy-MM-dd  hh:mm:ss");

    add_oil_start.height_str = QString::number(add_Oil_array[addoil_add_num][0], 'f', 1);
    add_oil_start.volume_str = QString::number(add_Oil_array[addoil_add_num][1], 'f', 1);

    model->setItem(0,1,new QStandardItem(start_datetime_str));
    model->item(0,1)->setTextAlignment(Qt::AlignCenter);
    model->setItem(1,1,new QStandardItem(add_oil_start.height_str));
    model->item(1,1)->setTextAlignment(Qt::AlignCenter);
    model->setItem(2,1,new QStandardItem(add_oil_start.volume_str));
    model->item(2,1)->setTextAlignment(Qt::AlignCenter);
}


void MainWindow::on_btn_addoil_end_clicked()
{
    QDateTime current_datetime = QDateTime::currentDateTime();
    end_datetime_str = current_datetime.toString("yyyy-MM-dd  hh:mm:ss");

    add_oil_end.height_str = QString::number(add_Oil_array[addoil_add_num][0], 'f', 1);
    add_oil_end.volume_str = QString::number(add_Oil_array[addoil_add_num][1], 'f', 1);
    float volume = add_oil_end.volume_str.toFloat() - add_oil_start.volume_str.toFloat();

    model->setItem(0,2,new QStandardItem(end_datetime_str));
    model->item(0,2)->setTextAlignment(Qt::AlignCenter);
    model->setItem(1,2,new QStandardItem(add_oil_end.height_str));
    model->item(1,2)->setTextAlignment(Qt::AlignCenter);
    model->setItem(2,2,new QStandardItem(add_oil_end.volume_str));
    model->item(2,2)->setTextAlignment(Qt::AlignCenter);
    model->setItem(3,1,new QStandardItem(QString::number(volume, 'f', 1)));
    model->item(3,1)->setTextAlignment(Qt::AlignCenter);

    add_yeweiyi_addOil_Record_Write(start_datetime_str,QString("%1").arg(addoil_add_num),QString::number(volume, 'f', 1));
}

void MainWindow::on_toolbtn_addoil_close_clicked()
{
    ui->widget_add_oil->setHidden(1);
    ui->btn_enter_add_Oil->setEnabled(1);
}

//自动进油
void MainWindow::open_addoil_windows()
{
    timer_open_addoil->stop();
    timer_close_addoil->start();

}

void MainWindow::close_addoil_windows()
{
    char add;
    QString addstr = ui->label_ao_num_2->text();
    add_yeweiyi_addOil_Auto_Record_Write(add_oil_all[addstr.toInt()].begin_time.toString(),add_oil_all[addstr.toInt()].end_time.toString(),addstr,QString::number(add_oil_all[addstr.toInt()].netvolume, 'f', 1));
    add_oil_all[addstr.toInt()].finish = false;
    for(int i=0;i<6;i++)
    {
        if(add_oil_all[i+1].finish)
        {
           add = i+1;
           i=10;
        }
    }

    timer_close_addoil->stop();

    if(add)
    {
        emit signal_Display_addoil_data(add);
        timer_close_addoil->start();
    }
    else
    {
        ui->widget_add_oil_auto->setHidden(1);
    }
}

void MainWindow::slot_Display_addoil_data(uchar add)
{
    ui->widget_add_oil_auto->setHidden(0);

    ui->label_ao_num_2->setText(QString("%1").arg(add));
    ui->label_ao_ok->setText(QString("油品：%1＃").arg(Oil_Kind[add-1][0]));

    ui->label_aos_time->setText(add_oil_all[add].begin_time.toString());
    ui->label_aos_oh->setText("油位高度："+QString::number(add_oil_all[add].start_oil_height, 'f', 1)+" mm");
    ui->label_aos_ov->setText("油品体积："+QString::number(add_oil_all[add].start_oil_volume, 'f', 1)+" L");
    ui->label_aos_wh->setText("水位高度："+QString::number(add_oil_all[add].start_water_height, 'f', 1)+" mm");
    ui->label_aos_wv->setText("水体积：  "+QString::number(add_oil_all[add].start_water_volume, 'f', 1)+" L");
    ui->label_aos_tempure->setText("油品温度："+QString::number(add_oil_all[add].start_temp, 'f', 1)+"       ℃");

    ui->label_aoe_time->setText(add_oil_all[add].begin_time.toString());
    ui->label_aoe_oh->setText("油位高度："+QString::number(add_oil_all[add].end_oil_height, 'f', 1)+" mm");
    ui->label_aoe_ov->setText("油品体积："+QString::number(add_oil_all[add].end_oil_volume, 'f', 1)+" L");
    ui->label_aoe_wh->setText("水位高度："+QString::number(add_oil_all[add].end_water_height, 'f', 1)+" mm");
    ui->label_aoe_wv->setText("水体积：  "+QString::number(add_oil_all[add].end_water_volume, 'f', 1)+" L");
    ui->label_aoe_tempure->setText("油品温度："+QString::number(add_oil_all[add].end_temp, 'f', 1)+"       ℃");

    add_oil_all[add].netvolume = add_oil_all[add].end_oil_volume - add_oil_all[add].start_oil_volume;
    ui->label_ao_auto_av->setText(QString::number(add_oil_all[add].netvolume, 'f', 1));

    add_oil_all[add].finish = true;
}

void MainWindow::on_btn_addoil_auto_confirm_clicked()
{
    char add;
    QString addstr = ui->label_ao_num_2->text();
    add_yeweiyi_addOil_Auto_Record_Write(add_oil_all[addstr.toInt()].begin_time.toString(),add_oil_all[addstr.toInt()].end_time.toString(),addstr,QString::number(add_oil_all[addstr.toInt()].netvolume, 'f', 1));
    add_oil_all[addstr.toInt()].finish = false;
    for(int i=0;i<6;i++)
    {
        if(add_oil_all[i+1].finish)
        {
           add = i+1;
           i=10;
        }
    }

    timer_close_addoil->stop();

    if(add)
    {
        emit signal_Display_addoil_data(add);
        timer_close_addoil->start();
    }
    else
    {
        ui->widget_add_oil_auto->setHidden(1);
    }
}

void MainWindow::on_btn_addoil_auto_quit_clicked()
{
    char add;
    QString addstr = ui->label_ao_num_2->text();
    add_oil_all[addstr.toInt()].finish = false;
    for(int i=0;i<6;i++)
    {
        if(add_oil_all[i+1].finish)
        {
           add = i+1;
           i=10;
        }
    }

    timer_close_addoil->stop();

    if(add)
    {
        emit signal_Display_addoil_data(add);
        timer_close_addoil->start();
    }
    else
    {
        ui->widget_add_oil_auto->setHidden(1);
    }
}
