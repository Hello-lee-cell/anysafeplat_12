#include "systemset.h"
#include "ui_systemset.h"
#include<stdint.h>
#include<stdio.h>
#include<sys/shm.h>
#include<sys/ipc.h>
#include<errno.h>
#include<fcntl.h>
#include<unistd.h>

#include"serial.h"
#include"mainwindow.h"
#include"main_main.h"
#include"config.h"
#include"file_op.h"
#include"ip_op.h"
#include"mythread.h"
#include"warn.h"
#include"keyboard.h"
#include"database_op.h"
//************radar*********/
#include"radar_485.h"
#include"security.h"
//add for 检查是否能连外网
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <QTcpSocket>
#include "airtightness_test.h"

unsigned char hang = 0;
unsigned char lie = 0;

unsigned char hang_gun = 0;
unsigned char lie_gun = 0;

unsigned char hang_mapping = 0;
unsigned char lie_mapping = 0;

unsigned char flag_mov_x = 20;//移动竖线使用
unsigned int max_value_y = 0;//Y轴最大值
unsigned char Temp_Auto_Backgroud[90][2];//阈值自动设置临时数组
unsigned char flag_hide_red = 1;

//unsigned char Flag_Set_yuzhi = 0;
//unsigned char Flag_pre_mode;  //1 调试模式 0正常模式


//**************added for radar*************/<-
unsigned int ip[4];        //四位ip地址中的每一位
char IP_DES[32] = {0};   //系统设置地址 system所需的字符串

unsigned char flag_mythread_temp = 0;   //一次写入，每个选项的检测

unsigned char Mapping[96] = {0};//加油枪编号映射数组，全局变量yignshe，用于数据上传编号
QString Mapping_Show[96] = {""};//加油枪编号映射数组，全局变量yignshe,用于显示
//post添加
unsigned char flag_post_Configuration = 0;//如果置1，则在设置退出时上传油气回收设置信息

systemset::systemset(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::systemset)
{
    ui->setupUi(this);
    this->setAttribute(Qt::WA_DeleteOnClose,true);
    Flag_Timeto_CloseNeeded[0] = 1;
    move(0,78);
    ui->tabWidget_all->setStyleSheet("QTabBar::tab{max-height:33px;min-width:80px;background-color: rgb(170,170,255,255);border: 2px solid;padding:9px;}\
                                     QTabBar::tab:selected {background-color: white}\
                                     QTabWidget::pane {border-top:0px solid #e8f3f9;background:  transparent;}");
    touchkey = new keyboard;
    setAttribute(Qt::WA_TranslucentBackground,true);    //窗体透明
    setWindowFlags(Qt::Tool|Qt::WindowStaysOnTopHint|Qt::FramelessWindowHint);

    delay10s = new QTimer;
    delay10s->setInterval(1500);
    connect(delay10s,SIGNAL(timeout()),this,SLOT(setok_delay10sclose()));
    ui->label_2->setHidden(1);//设置中
    ui->label_managerid->setText("");
    //警告窗口设置
    ui->widget_warn_man_id->setHidden(1);
    ui->widget_warn_man_pw->setHidden(1);
    ui->widget_warn_user_add->setHidden(1);
    ui->widget_warn_user_del->setHidden(1);
    ui->widget_warn_user_pw->setHidden(1);
    ui->widget_warn_inputerror->setHidden(1);
    ui->widget_warn_user_noexist->setHidden(1);
    ui->widget_warn_poweroff->setHidden(1);
    ui->widget_warn_update->setHidden(1);   //系统升级二次确认界面
    ui->widget_warn_histclr->setHidden(1);  //历史记录清除二次确认界面
    ui->widget_warn_gujian->setHidden(1);   //固件升级二次确认界面
    ui->widget_warn_allclean->setHidden(1);//清除报警状态确认界面

    //********************传感器设置********************//
    ui->comboBox->setStyleSheet("QScrollBar{ background: #F0F0F0; width:20px ;margin-top:0px;margin-bottom:0px }"
                                "QScrollBar::handle:vertical{ background: #6c65c8; min-height: 80px ;width:18px }");
    ui->comboBox_2->setStyleSheet("QScrollBar{ background: #F0F0F0; width:20px ;margin-top:0px;margin-bottom:0px }"
                                "QScrollBar::handle:vertical{ background: #6c65c8; min-height: 80px ;width:18px }");
    ui->comboBox_3->setStyleSheet("QScrollBar{ background: #F0F0F0; width:20px ;margin-top:0px;margin-bottom:0px }"
                                "QScrollBar::handle:vertical{ background: #6c65c8; min-height: 80px ;width:18px }");
    ui->comboBox_4->setStyleSheet("QScrollBar{ background: #F0F0F0; width:20px ;margin-top:0px;margin-bottom:0px }"
                                "QScrollBar::handle:vertical{ background: #6c65c8; min-height: 80px ;width:18px }");
    ui->comboBox_5->setStyleSheet("QScrollBar{ background: #F0F0F0; width:20px ;margin-top:0px;margin-bottom:0px }"
                                "QScrollBar::handle:vertical{ background: #6c65c8; min-height: 80px ;width:18px }");
    ui->comboBox->setCurrentIndex(count_basin);
    ui->comboBox_2->setCurrentIndex(count_pipe);
    ui->comboBox_3->setCurrentIndex(count_dispener);
    ui->comboBox_4->setCurrentIndex(count_tank);
    ui->comboBox_5->setCurrentIndex(Test_Method);
    if(Test_Method == 1) //如果是压力法才显示显示模式按钮
    {
        ui->pushButton_pre_mode->show();
        if(Flag_pre_mode == 1)
        {
            ui->pushButton_pre_mode->setText( QString::fromUtf8("调试模式"));
        }
        else
        {
            ui->pushButton_pre_mode->setText( QString::fromUtf8("正常模式"));
        }
    }
    else
    {
        ui->pushButton_pre_mode->hide();
    }
    //********************传感器设置********************//


    //********************雷达设置********************//
    ui->lineEdit_x1->installEventFilter(this);
    ui->lineEdit_x2->installEventFilter(this);
    ui->lineEdit_x3->installEventFilter(this);
    ui->lineEdit_x4->installEventFilter(this);
    ui->lineEdit_x5->installEventFilter(this);
    ui->lineEdit_x6->installEventFilter(this);
    ui->lineEdit_y1->installEventFilter(this);
    ui->lineEdit_y2->installEventFilter(this);
    ui->lineEdit_y3->installEventFilter(this);
    ui->lineEdit_y4->installEventFilter(this);
    ui->lineEdit_y5->installEventFilter(this);
    ui->lineEdit_y6->installEventFilter(this);
    ui->lineEdit_yuzhi_handinput->installEventFilter(this);
    area_point_disp(0);
    //智能设置当前状态显示
    ui->comboBox_starttime_h->setStyleSheet("QScrollBar{ background: #F0F0F0; width:20px ;margin-top:0px;margin-bottom:0px }"
                                "QScrollBar::handle:vertical{ background: #6c65c8; min-height: 80px ;width:18px }");
    ui->comboBox_starttime_m->setStyleSheet("QScrollBar{ background: #F0F0F0; width:20px ;margin-top:0px;margin-bottom:0px }"
                                "QScrollBar::handle:vertical{ background: #6c65c8; min-height: 80px ;width:18px }");
    ui->comboBox_stoptime_h->setStyleSheet("QScrollBar{ background: #F0F0F0; width:20px ;margin-top:0px;margin-bottom:0px }"
                                "QScrollBar::handle:vertical{ background: #6c65c8; min-height: 80px ;width:18px }");
    ui->comboBox_stoptime_m->setStyleSheet("QScrollBar{ background: #F0F0F0; width:20px ;margin-top:0px;margin-bottom:0px }"
                                "QScrollBar::handle:vertical{ background: #6c65c8; min-height: 80px ;width:18px }");
    ui->comboBox_silent_h->setStyleSheet("QScrollBar{ background: #F0F0F0; width:20px ;margin-top:0px;margin-bottom:0px }"
                                "QScrollBar::handle:vertical{ background: #6c65c8; min-height: 80px ;width:18px }");
    ui->comboBox_silent_m->setStyleSheet("QScrollBar{ background: #F0F0F0; width:20px ;margin-top:0px;margin-bottom:0px }"
                                "QScrollBar::handle:vertical{ background: #6c65c8; min-height: 80px ;width:18px }");
    ui->comboBox_warndelay_m->setStyleSheet("QScrollBar{ background: #F0F0F0; width:20px ;margin-top:0px;margin-bottom:0px }"
                                "QScrollBar::handle:vertical{ background: #6c65c8; min-height: 80px ;width:18px }");
    ui->comboBox_warndelay_s->setStyleSheet("QScrollBar{ background: #F0F0F0; width:20px ;margin-top:0px;margin-bottom:0px }"
                                "QScrollBar::handle:vertical{ background: #6c65c8; min-height: 80px ;width:18px }");

    ui->comboBox_sensitivity->setStyleSheet("QScrollBar{ background: #F0F0F0; width:20px ;margin-top:0px;margin-bottom:0px }"
                                "QScrollBar::handle:vertical{ background: #6c65c8; min-height: 80px ;width:18px }");

    ui->comboBox_starttime_h->setCurrentIndex(Start_time_h);    //1
    ui->comboBox_starttime_m->setCurrentIndex(Start_time_m);    //2
    ui->comboBox_stoptime_h->setCurrentIndex(Stop_time_h);      //3
    ui->comboBox_stoptime_m->setCurrentIndex(Stop_time_m);      //4
    ui->comboBox_silent_h->setCurrentIndex(Silent_time_h);      //5
    ui->comboBox_silent_m->setCurrentIndex(Silent_time_m);      //6
    ui->comboBox_warndelay_m->setCurrentIndex(Warn_delay_m);    //7
    ui->comboBox_warndelay_s->setCurrentIndex(Warn_delay_s);    //8

    ui->comboBox_sensitivity->setCurrentIndex(Flag_sensitivity);
    if(Flag_outdoor_warn)
    {
        ui->toolButton_outdor_warnopen->setEnabled(0);
    }
    else
    {
        ui->toolButton_outdor_warnclo->setEnabled(0);
    }
    if(Flag_area_ctrl[0])
    {
        ui->toolButton_areaopen_1->setEnabled(0);
    }
    else
    {
        ui->toolButton_areaclo_1->setEnabled(0);
    }
    if(Flag_area_ctrl[1])
    {
        ui->toolButton_areaopen_2->setEnabled(0);
    }
    else
    {
        ui->toolButton_areaclo_2->setEnabled(0);
    }
    if(Flag_area_ctrl[2])
    {
        ui->toolButton_areaopen_3->setEnabled(0);
    }
    else
    {
        ui->toolButton_areaclo_3->setEnabled(0);
    }
    if(Flag_area_ctrl[3])
    {
        ui->toolButton_areaopen_4->setEnabled(0);
    }
    else
    {
        ui->toolButton_areaclo_4->setEnabled(0);
    }
    timer_Check_temp = new QTimer;
    timer_Check_temp->setInterval(500);
    connect(timer_Check_temp,SIGNAL(timeout()),this,SLOT(Check_Temp()));
    timer_doublhand_pointdrw = new QTimer;
    timer_doublhand_pointdrw->setInterval(500);
    connect(timer_doublhand_pointdrw,SIGNAL(timeout()),this,SLOT(doublhand_pointdrw()));
    timer_doublauto_linedrw = new QTimer;
    timer_doublauto_linedrw->setInterval(500);
    connect(timer_doublauto_linedrw,SIGNAL(timeout()),this,SLOT(doublanto_linedrw()));

    ui->lineEdit_yuzhi_handinput->setHidden(1);
    ui->widget_colletc->addGraph(); //添加图层  白 前九十
    ui->widget_colletc->addGraph(); //添加图层  红       graph(2)
    ui->widget_colletc->addGraph(); //添加图层，坐标指示竖线
    ui->widget_colletc->addGraph(); //添加图层  双路手动设置  3
    ui->widget_colletc->addGraph(); //添加图层  双路自动设置  4
    //********************雷达设置********************//


    //********************安全防护设置********************//
    ui->comboBox_num_cc->setStyleSheet("QScrollBar{ background: #F0F0F0; width:20px ;margin-top:0px;margin-bottom:0px }"
                                "QScrollBar::handle:vertical{ background: #6c65c8; min-height: 50px ;width:18px }");
    if(Flag_xieyou)
    {
        ui->toolButton_kaiqi->setEnabled(0);
        ui->toolButton_guanbi->setEnabled(1);
    }
    else
    {
        ui->toolButton_kaiqi->setEnabled(1);
        ui->toolButton_guanbi->setEnabled(0);
    }
    if(Flag_IIE)
    {
        ui->toolButton_kaiqi_IIE->setEnabled(0);
        ui->toolButton_guanbi_IIE->setEnabled(1);
    }
    else
    {
        ui->toolButton_kaiqi_IIE->setEnabled(1);
        ui->toolButton_guanbi_IIE->setEnabled(0);
    }

    if(Flag_Enable_liqiud == 1)
    {
        ui->toolButton_kaiqi_yewei->setEnabled(0);
        ui->toolButton_guanbi_yewei->setEnabled(1);
    }
    else
    {
        ui->toolButton_kaiqi_yewei->setEnabled(1);
        ui->toolButton_guanbi_yewei->setEnabled(0);
    }
    if(Flag_Enable_pump == 1)
    {
        ui->toolButton_kaiqi_beng->setEnabled(0);
        ui->toolButton_guanbi_beng->setEnabled(1);
    }
    else
    {
        ui->toolButton_kaiqi_beng->setEnabled(1);
        ui->toolButton_guanbi_beng->setEnabled(0);
    }
    if(Pre_pipe_en)
    {
        ui->toolButton_reoilgas_pipepre_open->setEnabled(0);
        ui->toolButton_reoilgas_pipepre_open->setText("已开启");
        ui->toolButton_reoilgas_pipepre_close->setText("点击关闭");
    }
    else
    {
        ui->toolButton_reoilgas_pipepre_close->setEnabled(0);
        ui->toolButton_reoilgas_pipepre_open->setText("点击开启");
        ui->toolButton_reoilgas_pipepre_close->setText("已关闭");
    }
    if(Pre_tank_en)
    {
        ui->toolButton_reoilgas_tankpre_open->setEnabled(0);
        ui->toolButton_reoilgas_tankpre_open->setText("已开启");
        ui->toolButton_reoilgas_tankpre_close->setText("点击关闭");
    }
    else
    {
        ui->toolButton_reoilgas_tankpre_close->setEnabled(0);
        ui->toolButton_reoilgas_tankpre_open->setText("点击开启");
        ui->toolButton_reoilgas_tankpre_close->setText("已关闭");
    }
    if(Env_Gas_en)
    {
        ui->toolButton_gas1_open->setEnabled(0);
        ui->toolButton_gas1_open->setText("已开启");
        ui->toolButton_gas1_close->setText("点击关闭");
    }
    else
    {
        ui->toolButton_gas1_close->setEnabled(0);
        ui->toolButton_gas1_open->setText("点击开启");
        ui->toolButton_gas1_close->setText("已关闭");
    }
    //温度传感器
    if(Tem_tank_en)
    {
        ui->toolButton_reoilgas_tem_open->setEnabled(0);
        ui->toolButton_reoilgas_tem_open->setText("已开启");
        ui->toolButton_reoilgas_tem_close->setText("点击关闭");
    }
    else
    {
        ui->toolButton_reoilgas_tem_close->setEnabled(0);
        ui->toolButton_reoilgas_tem_open->setText("点击开启");
        ui->toolButton_reoilgas_tem_close->setText("已关闭");
    }
    ui->comboBox_num_cc->setCurrentIndex(Num_Crash_Column);//防撞柱显示
    //********************安全防护设置********************//


    //********************其他设置********************//
    ui->lineEdit_manid->installEventFilter(this);
    ui->lineEdit_manpw->installEventFilter(this);
    ui->lineEdit_username->installEventFilter(this);
    ui->lineEdit_userpw->installEventFilter(this);
    ui->lineEdit_adduser->installEventFilter(this);
    ui->lineEdit_adduser_2->installEventFilter(this);
    ui->lineEdit_deluser->installEventFilter(this);
    ui->scrollArea_other->setStyleSheet("QScrollArea#scrollArea_other, QWidget#scrollAreaWidgetContents_other{background-color:rgb(172,214,248);}");
    ui->scrollArea_other->verticalScrollBar()->setStyleSheet("QScrollBar{ background: #F0F0F0; width:25px ;margin-top:0px;margin-bottom:0px }"
                                                       "QScrollBar::handle:vertical{ background: #6c65c8; min-height: 80px ;width:21px }");
    ui->comboBox_year->setStyleSheet("QScrollBar{ background: #F0F0F0; width:20px ;margin-top:0px;margin-bottom:0px }"
                                "QScrollBar::handle:vertical{ background: #6c65c8; min-height: 50px ;width:18px }");
    ui->comboBox_month->setStyleSheet("QScrollBar{ background: #F0F0F0; width:20px ;margin-top:0px;margin-bottom:0px }"
                                "QScrollBar::handle:vertical{ background: #6c65c8; min-height: 50px ;width:18px }");
    ui->comboBox_day->setStyleSheet("QScrollBar{ background: #F0F0F0; width:20px ;margin-top:0px;margin-bottom:0px }"
                                "QScrollBar::handle:vertical{ background: #6c65c8; min-height: 50px ;width:18px }");
    ui->comboBox_hour->setStyleSheet("QScrollBar{ background: #F0F0F0; width:20px ;margin-top:0px;margin-bottom:0px }"
                                "QScrollBar::handle:vertical{ background: #6c65c8; min-height: 50px ;width:18px }");
    ui->comboBox_min->setStyleSheet("QScrollBar{ background: #F0F0F0; width:20px ;margin-top:0px;margin-bottom:0px }"
                                "QScrollBar::handle:vertical{ background: #6c65c8; min-height: 50px ;width:18px }");
    ui->comboBox_sec->setStyleSheet("QScrollBar{ background: #F0F0F0; width:20px ;margin-top:0px;margin-bottom:0px }"
                                "QScrollBar::handle:vertical{ background: #6c65c8; min-height: 50px ;width:18px }");
    //qlineedit指示文字设置 用户管理
    QTextCodec *codec = QTextCodec::codecForName("UTF-8");
    QString text_temp;
    QByteArray manager_id = "点击输入管理员ID";
    QByteArray manager_passwd = "点击输入新密码";
    QByteArray user_name = "点击输入用户名";
    QByteArray user_passwd = "点击输入新用户密码";
    QByteArray user_add_name = "点击输入新用户名";
    QByteArray user_add_passwd = "点击输入新密码";
    QByteArray user_del = "点击输入用户名";
    text_temp = codec->toUnicode(manager_id);
    ui->lineEdit_manid->setPlaceholderText(text_temp);
    ui->lineEdit_manid->setMaxLength(18);

    text_temp = codec->toUnicode(manager_passwd);
    ui->lineEdit_manpw->setPlaceholderText(text_temp);
    ui->lineEdit_manpw->setMaxLength(6);
    ui->lineEdit_manpw->setEchoMode(QLineEdit::Password);

    text_temp = codec->toUnicode(user_name);
    ui->lineEdit_username->setPlaceholderText(text_temp);
    ui->lineEdit_username->setMaxLength(18);

    text_temp = codec->toUnicode(user_passwd);
    ui->lineEdit_userpw->setPlaceholderText(text_temp);
    ui->lineEdit_userpw->setMaxLength(6);

    text_temp = codec->toUnicode(user_add_name);
    ui->lineEdit_adduser->setPlaceholderText(text_temp);
    ui->lineEdit_adduser->setMaxLength(18);


    text_temp = codec->toUnicode(user_add_passwd);
    ui->lineEdit_adduser_2->setPlaceholderText(text_temp);
    ui->lineEdit_adduser_2->setMaxLength(6);

    text_temp = codec->toUnicode(user_del);
    ui->lineEdit_deluser->setPlaceholderText(text_temp);
    ui->lineEdit_deluser->setMaxLength(18);
    //时间设置
    QDateTime date_time = QDateTime::currentDateTime();
    int year;
    int month;
    int day;
    int hour;
    int min;
    int sec;
    year = date_time.toString("yyyy").toInt();
    month = date_time.toString("MM").toInt();
    day = date_time.toString("dd").toInt();
    hour = date_time.toString("hh").toInt();
    min = date_time.toString("mm").toInt();
    sec = date_time.toString("ss").toInt();
    ui->comboBox_year->setCurrentIndex(year-2010);
    ui->comboBox_month->setCurrentIndex(month-1);
    ui->comboBox_day->setCurrentIndex(day-1);
    ui->comboBox_hour->setCurrentIndex(hour);
    ui->comboBox_min->setCurrentIndex(min);
    ui->comboBox_sec->setCurrentIndex(sec);
    //********************其他设置********************//

    //********************油气回收设置********************//
    ui->lineEdit_pv->installEventFilter(this);
    ui->lineEdit_pv_nega->installEventFilter(this);

    ui->comboBox_dispen_sumset->setStyleSheet("QScrollBar{ background: #F0F0F0; width:20px ;margin-top:0px;margin-bottom:0px }"
                                "QScrollBar::handle:vertical{ background: #6c65c8; min-height: 80px ;width:18px }");
    ui->tableView_dispenerset->verticalScrollBar()->setStyleSheet( "QScrollBar{ background: #F0F0F0; width:25px ;margin-top:0px;margin-bottom:0px }"
                                                       "QScrollBar::handle:vertical{ background: #6c65c8; min-height: 80px ;width:21px }");
    ui->tableView_gundetailset->verticalScrollBar()->setStyleSheet("QScrollBar{ background: #F0F0F0; width:25px ;margin-top:0px;margin-bottom:0px }"
                                                                   "QScrollBar::handle:vertical{ background: #6c65c8; min-height: 80px ;width:21px }");
    ui->tableView_yingshe->verticalScrollBar()->setStyleSheet( "QScrollBar{ background: #F0F0F0; width:25px ;margin-top:0px;margin-bottom:0px }"
                                                       "QScrollBar::handle:vertical{ background: #6c65c8; min-height: 80px ;width:21px }");
	ui->tableView_dispenerset->verticalHeader()->setHidden(1);//隐藏行号
	ui->tableView_gundetailset->verticalHeader()->setHidden(1);
	ui->tableView_yingshe->verticalHeader()->setHidden(1);
	ui->tableView_dispenerset->verticalHeader()->setDefaultSectionSize(26);        //设置行间距
	ui->tableView_dispenerset->verticalHeader()->setMinimumSectionSize(26);        //设置行间距
	ui->tableView_gundetailset->verticalHeader()->setDefaultSectionSize(26);        //设置行间距
	ui->tableView_gundetailset->verticalHeader()->setMinimumSectionSize(26);        //设置行间距
	ui->tableView_yingshe->verticalHeader()->setDefaultSectionSize(26);        //设置行间距
	ui->tableView_yingshe->verticalHeader()->setMinimumSectionSize(26);        //设置行间距
	ui->tableView_dispenerset-> setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);// 纵向 平滑滑动，因为太卡，看不太出来效果
	ui->tableView_gundetailset-> setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);// 纵向 平滑滑动，因为太卡，看不太出来效果
	ui->tableView_yingshe-> setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);// 纵向 平滑滑动，因为太卡，看不太出来效果

	ui->scrollArea->setStyleSheet("QScrollArea#scrollArea, QWidget#scrollAreaWidgetContents{background-color:rgb(172,214,248);}");
    ui->scrollArea->verticalScrollBar()->setStyleSheet("QScrollBar{ background: #F0F0F0; width:25px ;margin-top:0px;margin-bottom:0px }"
                                                       "QScrollBar::handle:vertical{ background: #6c65c8; min-height: 80px ;width:21px }");
    ui->comboBox_far_dispener->setStyleSheet("QScrollBar{ background: #F0F0F0; width:20px ;margin-top:0px;margin-bottom:0px }"
                                "QScrollBar::handle:vertical{ background: #6c65c8; min-height: 50px ;width:18px }");
    ui->comboBox_dispen_sumset->setCurrentIndex(Amount_Dispener);
    tableview_1_replay(Amount_Dispener);
    tableview_2_replay();
    tableView_yingshe_replay();

    QIntValidator *onlynum = new QIntValidator(this);
    onlynum->setRange(0,255);
    QIntValidator *onlynum_pv = new QIntValidator(this);
    onlynum->setRange(0,1000);
    ui->lineEdit_pv->setValidator(onlynum_pv);
    ui->lineEdit_pv_nega->setValidator(onlynum_pv);
    ui->lineEdit_pv->setText(QString("%1").arg(Positive_Pres));
    ui->lineEdit_pv_nega->setText(QString("%1").arg(Negative_Pres));

    ui->comboBox_far_dispener->setCurrentIndex(Far_Dispener);//默认显示之前设置的
	qDebug()<< "al_low is"<<NormalAL_Low;
	if(int(NormalAL_Low*100)-80 == 0){ui->comboBox_al_low->setCurrentIndex(0);qDebug()<<"al low****  0";} //!!!!!浮点数不能直接判断是否相等，后多少位可能会有偏差
	if(int(NormalAL_Low*100)-85 == 0){ui->comboBox_al_low->setCurrentIndex(1);qDebug()<<"al low****  1";}
	if(int(NormalAL_Low*100)-90 == 0){ui->comboBox_al_low->setCurrentIndex(2);qDebug()<<"al low****  2";}
	if(int(NormalAL_Low*100)-95 == 0){ui->comboBox_al_low->setCurrentIndex(3);qDebug()<<"al low****  3";}
	if(int(NormalAL_Low*100)-100 == 0){ui->comboBox_al_low->setCurrentIndex(4);qDebug()<<"al low****  4";}
	qDebug()<< "al_hign is"<<NormalAL_High;
	if(int(NormalAL_High*100)-120 == 0){ui->comboBox_al_high->setCurrentIndex(0);qDebug()<<"al high****  0";}
	if(int(NormalAL_High*100)-125 == 0){ui->comboBox_al_high->setCurrentIndex(1);qDebug()<<"al high****  1";}
	if(int(NormalAL_High*100)-130 == 0){ui->comboBox_al_high->setCurrentIndex(2);qDebug()<<"al high****  2";}
	if(int(NormalAL_High*100)-135 == 0){ui->comboBox_al_high->setCurrentIndex(3);qDebug()<<"al high****  3";}
	if(int(NormalAL_High*100)-140 == 0){ui->comboBox_al_high->setCurrentIndex(4);qDebug()<<"al high****  4";}

	if(WarnAL_Days == 5){ui->comboBox_alwarndays->setCurrentIndex(0);}
	if(WarnAL_Days == 7){ui->comboBox_alwarndays->setCurrentIndex(1);}

    ui->label_far->setHidden(1);//最远端加油机未设置的提示文本
    if(Far_Dispener>Amount_Dispener)//大于设置的加油机数量
    {
        ui->label_far->setHidden(0);//显示错误文本
    }
    else
    {
        ui->label_far->setHidden(1);//不显示
    }
    ui->label_reoilgas_factorset_state->setHidden(1);
    ui->label_reoilgas_count_dispen->setHidden(1);
    if(Flag_Gun_off == 1)
    {
        ui->toolButton_gun_off_guanbi->setEnabled(1);
        ui->toolButton_gun_off_kaiqi->setEnabled(0);
    }
    else
    {
        ui->toolButton_gun_off_guanbi->setEnabled(0);
        ui->toolButton_gun_off_kaiqi->setEnabled(1);
    }
    //********************油气回收设置********************//

    //********************网络设置********************//
    ui->lineEdit_ifisport_udp->installEventFilter(this);
    ui->lineEdit_ifisport_tcp->installEventFilter(this);
    ui->lineEdit_ip->installEventFilter(this);
    ui->lineEdit_mask->installEventFilter(this);
    ui->lineEdit_bcast->installEventFilter(this);
    //post添加
	ui->lineEdit_postaddress->installEventFilter(this);
    ui->lineEdit_postdataid->installEventFilter(this);
    ui->lineEdit_postuserid->installEventFilter(this);
    ui->lineEdit_postversion->installEventFilter(this);
	ui->lineEdit_postusername->installEventFilter(this);
	ui->lineEdit_postpassword->installEventFilter(this);
    //isoosi
	ui->lineEdit_isoosi_UrlIp->installEventFilter(this);
	ui->lineEdit_isoosi_UrlPort->installEventFilter(this);
    ui->lineEdit_isoosi_mn->installEventFilter(this);
	ui->lineEdit_isoosi_pw->installEventFilter(this);
	//isoosi 重庆
	ui->lineEdit_isoosi_cqid->installEventFilter(this);
    //泄漏
    ui->lineEdit_hubei_station_id->installEventFilter(this);
	//服务器
	ui->lineEdit_MyServerId->installEventFilter(this);
	ui->lineEdit_MyServerPW->installEventFilter(this);
	ui->lineEdit_MyServerIp->installEventFilter(this);
	ui->lineEdit_MyServerPort->installEventFilter(this);

	ui->scrollArea_network->setStyleSheet("QScrollArea#scrollArea_network, QWidget#scrollAreaWidgetContents_network{background-color:rgb(172,214,248);}");
	ui->scrollArea_network->verticalScrollBar()->setStyleSheet("QScrollBar{ background: #F0F0F0; width:25px ;margin-top:0px;margin-bottom:0px }"
	                                                           "QScrollBar::handle:vertical{ background: #6c65c8; min-height: 80px ;width:21px }");

	//ui->scrollArea_network->setWidgetResizable(true);
	//ui->scrollAreaWidgetContents_network->setGeometry(-1,-1,959,1000);
	//ui->scrollAreaWidgetContents_network->size().setWidth(945);

    ui->comboBox_dispen_x_select->setStyleSheet("QScrollBar{ background: #F0F0F0; width:20px ;margin-top:0px;margin-bottom:0px }"
                                                "QScrollBar::handle:vertical{ background: #6c65c8; min-height: 80px ;width:18px }");
    ui->lineEdit_ip->setAlignment(Qt::AlignRight);     //qlineEdit 对齐方式
    ui->lineEdit_mask->setAlignment(Qt::AlignRight);
    ui->lineEdit_bcast->setAlignment(Qt::AlignRight);


    ui->widget_warn_ip_wrong->setHidden(1);
    //post添加
	ui->lineEdit_postaddress->setText(Post_Address.toUtf8());
	ui->lineEdit_postdataid->setText(DATAID_POST.toUtf8());
	ui->lineEdit_postuserid->setText(USERID_POST.toUtf8());
	QString data = VERSION_POST.right(VERSION_POST.size() - 1);  /* 取data左边size - 1长度的数据 */
	ui->lineEdit_postversion->setText(data);
    //isoosi添加
	ui->lineEdit_isoosi_UrlIp->setText(IsoOis_UrlIp);
	ui->lineEdit_isoosi_UrlPort->setText(IsoOis_UrlPort);
	ui->lineEdit_isoosi_mn->setText(IsoOis_MN);
	ui->lineEdit_isoosi_pw->setText(IsoOis_PW);
	//isoosi重庆
	ui->lineEdit_isoosi_cqid->setText(IsoOis_StationId_Cq);
    qDebug()<< DATAID_POST << USERID_POST << VERSION_POST << "???";
    if(Flag_Shield_Network == 0)
    {
        ui->pushButton_shield_network->setText("点击屏蔽");
        ui->toolButton_isoosi_pb->setText("点击屏蔽");
    }

    if(Flag_Shield_Network == 1)
    {
        ui->pushButton_shield_network->setText("已屏蔽");
        ui->toolButton_isoosi_pb->setText("已屏蔽");
    }

    if(Flag_Postsend_Enable == 1)
    {
        ui->toolButton_post_guanbi->setEnabled(1);
        ui->toolButton_post_kaiqi->setEnabled(0);
    }
    else
    {
        ui->toolButton_post_guanbi->setEnabled(0);
        ui->toolButton_post_kaiqi->setEnabled(1);
    }
	ui->lineEdit_ifisport_udp->setText(QString::number(PORT_UDP));
	ui->lineEdit_ifisport_tcp->setText(QString::number(PORT_TCP));
	//服务器设置
	ui->lineEdit_MyServerId->setText(MyStationId);
	ui->lineEdit_MyServerPW->setText(MyStationPW);
	ui->lineEdit_MyServerIp->setText(MyServerIp);
	ui->lineEdit_MyServerPort->setText(QString::number(MyServerPort));
	if(Flag_MyServerEn == 1)
	{
		ui->toolButton_MyServerSwitch->setText("已开启");
	}
	else
	{
		ui->toolButton_MyServerSwitch->setText("点击开启");
	}
    //********************网络设置********************//


    //其他
    on_tabWidget_all_currentChanged(0);
}

systemset::~systemset()
{
    delete ui;
}


// 整体导航
void systemset::on_tabWidget_all_currentChanged(int index)
{
    //index = 0     传感器设置
    //index = 1     雷达设置
    //index = 2     人体静电设置  安全防护
    //index = 3     可燃气体
    //index = 4     油气回收
    //index = 5     网络设置
    //index = 6     其他设置：用户管理  升级 时间设置
    emit closeing_touchkey();
    if(index == 0)
    {
        char IP[32] = {0};
        get_local_ip(if_name,IP);
        ui->label_14->setText(IP);
    }
    if(index == 1)
    {

    }
    if(index == 2)
    {

    }
    if(index == 3)
    {
        //可燃气体数量初始化
        ui->comboBox_burngas->setStyleSheet("QScrollBar{ background: #F0F0F0; width:20px ;margin-top:0px;margin-bottom:0px }"
                                    "QScrollBar::handle:vertical{ background: #6c65c8; min-height: 80px ;width:18px }");
        ui->comboBox_burngas->setCurrentIndex(Num_Fga-2);
    }
    if(index == 4) //post添加
    {
        //post
        flag_post_Configuration = 1;
        printf("we will send oilgun set to webservice!!\n");
        ui->comboBox_reoilgas_ver->setCurrentIndex(Flag_Reoilgas_Version-1);
        ui->comboBox_pressure_transmitters_mode_mode->setCurrentIndex(Flag_Pressure_Transmitters_Mode);
        if(Flag_Reoilgas_NeverShow == 0)
        {
            ui->toolButton_pop_show->setText("已开启");
        }
		if(Flag_Reoilgas_NeverShow == 1)
        {
            ui->toolButton_pop_show->setText("点击开启");
        }
        //关枪使能
        if(Flag_Gun_off == 0)
        {
            ui->toolButton_gun_off_guanbi->setEnabled(0);
            ui->toolButton_gun_off_guanbi->setText("已关闭");
            ui->toolButton_gun_off_kaiqi->setEnabled(1);
            ui->toolButton_gun_off_kaiqi->setText("点击开启");
        }
        if(Flag_Gun_off == 1)
        {
            ui->toolButton_gun_off_guanbi->setEnabled(1);
            ui->toolButton_gun_off_guanbi->setText("点击关闭");
            ui->toolButton_gun_off_kaiqi->setEnabled(0);
            ui->toolButton_gun_off_kaiqi->setText("已开启");
        }

    }
    if(index == 5)
    {
		if(Flag_Network_Send_Version<=4)
        {
            ui->comboBox_network_version->setCurrentIndex(Flag_Network_Send_Version);
        }
        else {
			ui->comboBox_network_version->setCurrentIndex(8);
        }

        ui->comboBox_reoilgas_ver->setCurrentIndex(Flag_Reoilgas_Version-1);
        ui->comboBox_pressure_transmitters_mode_mode->setCurrentIndex(Flag_Pressure_Transmitters_Mode);
        //选择显示油气回收的网络上传目标地址
		if(Flag_Network_Send_Version == 0) //福州
        {
            ui->frame_fujian->setHidden(0);
            ui->frame_guangzhou->setHidden(1);
            ui->frame_network_verselect->setHidden(1);
			ui->frame_hunan_login->setHidden(1);//湖南的登录信息
			ui->label_post_name->setText("广州油气回收上传");
        }
		if(Flag_Network_Send_Version == 1) //广州
        {
            ui->frame_fujian->setHidden(1);
            ui->frame_guangzhou->setHidden(0);
			ui->frame_isoosi_chongqing->setHidden(1);
            ui->frame_network_verselect->setHidden(1);
			ui->label_isoosi_name->setText("广州油气回收上传");
        }
		if(Flag_Network_Send_Version == 2) //重庆  与广州同一个界面
        {
            ui->frame_fujian->setHidden(1);
			ui->frame_guangzhou->setHidden(0);
			ui->frame_isoosi_chongqing->setHidden(0);
			ui->frame_network_verselect->setHidden(1);
			ui->label_isoosi_name->setText("重庆油气回收上传");
        }
		if(Flag_Network_Send_Version == 3) //唐山  与福州相同
		{
			ui->frame_fujian->setHidden(0);
			ui->frame_guangzhou->setHidden(1);
			ui->frame_network_verselect->setHidden(1);
			ui->frame_hunan_login->setHidden(1);//湖南的登录信息
			ui->label_post_name->setText("唐山油气回收上传");
		}
		if(Flag_Network_Send_Version == 4) //湖南协议 与福州类似
		{
			ui->frame_fujian->setHidden(0);
			ui->frame_guangzhou->setHidden(1);
			ui->frame_network_verselect->setHidden(1);
			ui->frame_hunan_login->setHidden(0);//湖南的登录信息
			ui->lineEdit_postusername->setText(POSTUSERNAME_HUNAN);
			ui->lineEdit_postpassword->setText(POSTPASSWORD_HUNAN);
			ui->label_post_name->setText("湖南油气回收上传");
		}
		if(Flag_Network_Send_Version >= 5)//其他
		{
			ui->frame_fujian->setHidden(1);
			ui->frame_guangzhou->setHidden(1);
			ui->frame_network_verselect->setHidden(0);
		}

        //泄漏网络
        if(Flag_HuBeitcp_Enable == 0)
        {
            ui->toolButton_hubei_kaiguan->setText("点击开启");
        }
        if(Flag_HuBeitcp_Enable == 1)
        {
            ui->toolButton_hubei_kaiguan->setText("已开启");
        }
		if(Flag_XieLou_Version == 0)
		{
			ui->frame_hubei->setHidden(1);
			ui->frame_IFSF->setHidden(0);
			ui->label_114->setText("广州中石化端口设置");
		}
		if(Flag_XieLou_Version == 1)
		{
			ui->frame_hubei->setHidden(0);
			ui->frame_IFSF->setHidden(1);
		}
		if(Flag_XieLou_Version == 2)
		{
			ui->frame_hubei->setHidden(1);
			ui->frame_IFSF->setHidden(0);
			ui->label_114->setText("中国石油端口设置");
		}
        ui->comboBox_xielou_net_version->setCurrentIndex(Flag_XieLou_Version);
        ui->lineEdit_hubei_station_id->setPlaceholderText(QString::number(Station_ID_HB[0]*256+Station_ID_HB[1]));
		ui->widget_warn_ip_wrong->setHidden(1);

        //当前ip地址显示
        QString ip_add;
        QString bcast;
        QString mask;
        QProcess mem_process;
        mem_process.start("ifconfig eth1");
        mem_process.waitForFinished();
        QByteArray mem_output = mem_process.readAllStandardOutput();
        QString str_output = mem_output;
        str_output.replace(QRegExp("[\\s]+"), " ");  //把所有的多余的空格转为一个空格
        ip_add = str_output.section(' ', 6, 6);
        bcast = str_output.section(' ', 7, 7);
        mask = str_output.section(' ', 8, 8);
        ip_add = ip_add.right(ip_add.length()-5);
        bcast = bcast.right(bcast.length()-6);
         mask = mask.right(mask.length()-5);
        qDebug()<< ip_add<<bcast<<mask;

        ui->lineEdit_ip->setPlaceholderText(ip_add);
        ui->lineEdit_mask->setPlaceholderText(mask);
        ui->lineEdit_bcast->setPlaceholderText(bcast);
		//获取mac地址
		QFile devicd_id("/sys/class/net/eth1/address");
		if(!devicd_id.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			printf("read device MAC file\n");
		}
		QTextStream in(&devicd_id);
		QString mac_id;
		mac_id = in.readLine();
		ui->label_mac->setText(mac_id);
		qDebug()<<mac_id;
    }
    if(index == 6)
    {
        ui->textBrowser->clear();
        unsigned char i = 0;
        QString Line[20];
        QDir *dir = new QDir(USER_USER);
        foreach(QFileInfo list_file,dir->entryInfoList())
        {
            if(list_file.isFile())
            {
                Line[i] = list_file.fileName();

                if(!(Line[i].isEmpty()))
                {
                    ui->textBrowser->append(Line[i]);
                    i++;
                    if(i > 19)
                    {
                        i = 0;
                        ui->toolButton_useradd->setEnabled(0);
                    }
                    else
                    {
                        ui->toolButton_useradd->setEnabled(1);
                    }
                }
            }
        }


        if(Flag_screen_xielou == 0)
        {
            ui->toolButton_screen_xielou->setText("点击开启");
        }
        else
        {
            ui->toolButton_screen_xielou->setText("已开启");
        }

        if(Flag_screen_radar == 0)
        {
            ui->toolButton_screen_radar->setText("点击开启");
        }
        else
        {
            ui->toolButton_screen_radar->setText("已开启");
        }

        if(Flag_screen_safe == 0)
        {
            ui->toolButton_screen_safe->setText("点击开启");
        }
        else
        {
            ui->toolButton_screen_safe->setText("已开启");
        }

        if(Flag_screen_burngas == 0)
        {
            ui->toolButton_screen_burngas->setText("点击开启");
        }
        else
        {
            ui->toolButton_screen_burngas->setText("已开启");
        }


        if(Flag_screen_zaixian == 0)
        {
            ui->toolButton_screen_zaixian->setText("点击开启");
        }
        else
        {
            ui->toolButton_screen_zaixian->setText("已开启");
        }


        if(Flag_screen_cc == 0)
        {
            ui->toolButton_screen_cc->setText("点击开启");
        }
        else
        {
            ui->toolButton_screen_cc->setText("已开启");
        }

    }
}

void systemset::on_pushButton_3_clicked()       //保存 退出
{
    closeing_touchkey();
    Flag_Timeto_CloseNeeded[0] = 0;
    config_backgroundvalue_machine1();
    toolButton_areaset_enter_clicked();
    if(flag_waitsetup)
    {
        delay10s->start();
    }
    else
    {
        delay10s->setInterval(500);
        delay10s->start();
    }
    ui->label_2->setHidden(0);
    this->setEnabled(0);
    if(flag_mythread_temp)
    {
       flag_mythread = 1;      //一次写入
       flag_mythread_temp = 0;
    }
    //post添加
    if(flag_post_Configuration == 1)
    {
        printf("send net work !!!\n");
        unsigned int num_oilgun = Amount_Gasgun[0]+Amount_Gasgun[1]+Amount_Gasgun[2]+Amount_Gasgun[3]+Amount_Gasgun[4]+Amount_Gasgun[5]+
                Amount_Gasgun[6]+Amount_Gasgun[7]+Amount_Gasgun[8]+Amount_Gasgun[9]+Amount_Gasgun[10]+Amount_Gasgun[11];
        flag_post_Configuration = 0;
        emit network_onfigurationdata(DATAID_POST,QString::number(num_oilgun),QString::number(Positive_Pres,'f',1),QString::number(Negative_Pres,'f',1),
                                    "600.0",QString::number(Far_Dispener));//后处理装置压力值没有，设置为600，
    }

    config_tabwidget();//写入主界面显示配置
    emit hide_tablewidget(0,0);//显示主界面
    config_network_Version_write();//为了写入网络上传是否屏蔽不合格数据
	//my服务器配置参数
	MyStationId = ui->lineEdit_MyServerId->text();
	MyStationPW = ui->lineEdit_MyServerPW->text();
	MyServerIp = ui->lineEdit_MyServerIp->text();
	MyServerPort = (ui->lineEdit_MyServerPort->text()).toInt();
	config_MyServer_network();

}
void systemset::setok_delay10sclose()
{
    emit mainwindow_enable();
    flag_waitsetup = 0;
    delay10s->stop();
    close();
}

//泄漏部分
void systemset::on_comboBox_4_currentIndexChanged(int index)      //油罐
{
    if(index != count_tank)
    {
        flag_waitsetup = 1;
           flag_mythread_temp = 1;      //一次写入
        count_tank = index;
        emit amount_tank_reset(1);

        char history_detail[40] = {0};
        sprintf(history_detail,"油罐传感器数量设置为%d",count_tank);
        add_value_operateinfo(ui->label_managerid->text(),history_detail);
    }
}

void systemset::on_comboBox_2_currentIndexChanged(int index)      //管线
{
    if(index != count_pipe)
    {
        flag_waitsetup = 1;
           flag_mythread_temp = 1;      //一次写入
        count_pipe = index;
        emit amount_pipe_reset(1);

        char history_detail[40] = {0};
        sprintf(history_detail,"管线传感器数量设置为%d",count_pipe);
        add_value_operateinfo(ui->label_managerid->text(),history_detail);
    }
}

void systemset::on_comboBox_3_currentIndexChanged(int index)    //加油机
{
    if(index != count_dispener)
    {
        flag_waitsetup = 1;
           flag_mythread_temp = 1;      //一次写入
        count_dispener = index;
        emit amount_dispener_reset(1);

        char history_detail[40] = {0};
        sprintf(history_detail,"加油机传感器数量设置为%d",count_dispener);
        add_value_operateinfo(ui->label_managerid->text(),history_detail);
    }
}

void systemset::on_comboBox_currentIndexChanged(int index)      //防渗池
{
    if(index != count_basin)
    {
        flag_waitsetup = 1;
           flag_mythread_temp = 1;      //一次写入
        count_basin = index;
        emit amount_basin_reset(1);

        char history_detail[40] = {0};
        sprintf(history_detail,"防渗池传感器数量设置为%d",count_basin);
        add_value_operateinfo(ui->label_managerid->text(),history_detail);
    }
}

void systemset::on_comboBox_5_currentIndexChanged(int index)    //方法
{
    if(index != Test_Method )  //其他法
    {
        flag_waitsetup = 1;
            flag_mythread_temp = 1;      //一次写入
        Test_Method = index;
        emit method_tank_reset(1);

        emit amount_tank_reset(1);//因为加入压力显示而后加的


        if(Test_Method == 0)
        {
            add_value_operateinfo(ui->label_managerid->text(),"检测方式设置为其他方法");
            ui->pushButton_pre_mode->hide();
        }
        if(Test_Method == 1)
        {
            add_value_operateinfo(ui->label_managerid->text(),"检测方式设置为压力法");
            ui->pushButton_pre_mode->show();
        }
        if(Test_Method == 2)
        {
            add_value_operateinfo(ui->label_managerid->text(),"检测方式设置为液媒法");
            ui->pushButton_pre_mode->hide();
        }
        if(Test_Method == 3)
        {
            add_value_operateinfo(ui->label_managerid->text(),"检测方式设置为传感器法");
            ui->pushButton_pre_mode->hide();
        }
    }
  /*
        Test_Method = 0x01;//压力法
        Test_Method = 0x02;//液媒法法
        Test_Method = 0x03;//传感器法
  */
}

void systemset::on_pushButton_autoip_clicked()
{
    emit closeing_touchkey();
    warn_Scr = new warn;
    connect(this,SIGNAL(hide_warn_button(int)),warn_Scr,SLOT(hide_warn_button_set(int)));
    connect(warn_Scr,SIGNAL(history_ipautoset()),this,SLOT(history_autoip()));
    connect(warn_Scr,SIGNAL(history_ipset()),this,SLOT(history_ipset()));
    emit hide_warn_button(1);
    warn_Scr->exec();
}
void systemset::on_pushButton_ipset_clicked()
{
    emit closeing_touchkey();

    if(((ui->lineEdit_ip->text()).isEmpty())||((ui->lineEdit_mask->text()).isEmpty())||((ui->lineEdit_bcast->text()).isEmpty()))
    {
        ui->widget_warn_ip_wrong->setHidden(0);
		ui->label_ipwrong->setText("请输入完整的IP配置信息！");
    }
    else
    {
        QString valuestr1=ui->lineEdit_ip->text();
        QString valuestr2=ui->lineEdit_mask->text();
        QString valuestr3=ui->lineEdit_bcast->text();
		QString network = QString("ifconfig eth1 %1 netmask %2\nroute add default gw %3\n").arg(valuestr1).arg(valuestr2).arg(valuestr3);

        qDebug() << network;
        QByteArray ba = network.toLatin1(); // must
        ipstr=ba.data();

        warn_Scr = new warn;
        connect(this,SIGNAL(hide_warn_button(int)),warn_Scr,SLOT(hide_warn_button_set(int)));
        emit hide_warn_button(2);
        warn_Scr->exec();
    }
}

bool systemset::eventFilter(QObject *watched, QEvent *event)
{
    //网络IP设置
    if(watched==ui->lineEdit_ip)
    {
        if(event->type()==QEvent::MouseButtonPress)
        {
            emit closeing_touchkey();
            touchkey = new keyboard;
            connect(touchkey->signalMapper,SIGNAL(mapped(const QString&)),this,SLOT(setText_ip(const QString&)));
            connect(touchkey,SIGNAL(display_backspace()),this,SLOT(setBackspace_ip()));
            connect(this,SIGNAL(closeing_touchkey()),touchkey,SLOT(onEnter()));
            touchkey->show();
            return true;
        }

    }
    if(watched==ui->lineEdit_mask)
    {
        if(event->type()==QEvent::MouseButtonPress)
        {
            emit closeing_touchkey();
            touchkey = new keyboard;
            connect(touchkey->signalMapper,SIGNAL(mapped(const QString&)),this,SLOT(setText_mask(const QString&)));
            connect(touchkey,SIGNAL(display_backspace()),this,SLOT(setBackspace_mask()));
            connect(this,SIGNAL(closeing_touchkey()),touchkey,SLOT(onEnter()));
            touchkey->show();
            return true;
        }

    }
    if(watched==ui->lineEdit_bcast)
    {
        if(event->type()==QEvent::MouseButtonPress)
        {
            emit closeing_touchkey();
            touchkey = new keyboard;
            connect(touchkey->signalMapper,SIGNAL(mapped(const QString&)),this,SLOT(setText_bcast(const QString&)));
            connect(touchkey,SIGNAL(display_backspace()),this,SLOT(setBackspace_bcast()));
            connect(this,SIGNAL(closeing_touchkey()),touchkey,SLOT(onEnter()));
            touchkey->show();
            return true;
        }
    }
    if(watched==ui->lineEdit_ifisport_udp)
    {
        if(event->type()==QEvent::MouseButtonPress)
        {
            emit closeing_touchkey();
            touchkey = new keyboard;
            connect(touchkey->signalMapper,SIGNAL(mapped(const QString&)),this,SLOT(setText_ifisport_udp(const QString&)));
            connect(touchkey,SIGNAL(display_backspace()),this,SLOT(setBackspace_ifisport_udp()));
            connect(this,SIGNAL(closeing_touchkey()),touchkey,SLOT(onEnter()));
            touchkey->show();
            return true;
        }
    }
    if(watched==ui->lineEdit_ifisport_tcp)
    {
        if(event->type()==QEvent::MouseButtonPress)
        {
            emit closeing_touchkey();
            touchkey = new keyboard;
            connect(touchkey->signalMapper,SIGNAL(mapped(const QString&)),this,SLOT(setText_ifisport_tcp(const QString&)));
            connect(touchkey,SIGNAL(display_backspace()),this,SLOT(setBackspace_ifisport_tcp()));
            connect(this,SIGNAL(closeing_touchkey()),touchkey,SLOT(onEnter()));
            touchkey->show();
            return true;
        }
    }
    if(watched==ui->lineEdit_hubei_station_id)
    {
        if(event->type()==QEvent::MouseButtonPress)
        {
            emit closeing_touchkey();
            touchkey = new keyboard;
            connect(touchkey->signalMapper,SIGNAL(mapped(const QString&)),this,SLOT(setText_hubei_station_id(const QString&)));
            connect(touchkey,SIGNAL(display_backspace()),this,SLOT(setBackspace_hubei_station_id()));
            connect(this,SIGNAL(closeing_touchkey()),touchkey,SLOT(onEnter()));
            touchkey->show();
            return true;
        }
    }

    //用户管理输入
    if(watched == ui->lineEdit_manid)
    {
        if(event->type() == QEvent::MouseButtonPress)
        {
            emit closeing_touchkey();
            touchkey = new keyboard;
            connect(touchkey->signalMapper,SIGNAL(mapped(const QString&)),this,SLOT(setText_manid(const QString&)));
            connect(touchkey,SIGNAL(display_backspace()),this,SLOT(setBackspace_mainid()));
            connect(this,SIGNAL(closeing_touchkey()),touchkey,SLOT(onEnter()));
            touchkey->show();
            return true;
        }
    }
    if(watched == ui->lineEdit_manpw)
    {
        if(event->type() == QEvent::MouseButtonPress)
        {
            emit closeing_touchkey();
            touchkey = new keyboard;
            connect(touchkey->signalMapper,SIGNAL(mapped(const QString&)),this,SLOT(setText_manpw(const QString&)));
            connect(touchkey,SIGNAL(display_backspace()),this,SLOT(setBackspace_manpw()));
            connect(this,SIGNAL(closeing_touchkey()),touchkey,SLOT(onEnter()));
            touchkey->show();
            return true;
        }
    }
    if(watched == ui->lineEdit_username)
    {
        if(event->type() == QEvent::MouseButtonPress)
        {
            emit closeing_touchkey();
            touchkey = new keyboard;
            connect(touchkey->signalMapper,SIGNAL(mapped(const QString&)),this,SLOT(setText_username(const QString&)));
            connect(touchkey,SIGNAL(display_backspace()),this,SLOT(setBackspace_username()));
            connect(this,SIGNAL(closeing_touchkey()),touchkey,SLOT(onEnter()));
            touchkey->show();
            return true;
        }
    }
    if(watched == ui->lineEdit_userpw)
    {
        if(event->type() == QEvent::MouseButtonPress)
        {
            emit closeing_touchkey();
            touchkey = new keyboard;
            connect(touchkey->signalMapper,SIGNAL(mapped(const QString&)),this,SLOT(setText_userpw(const QString&)));
            connect(touchkey,SIGNAL(display_backspace()),this,SLOT(setBackspace_userpw()));
            connect(this,SIGNAL(closeing_touchkey()),touchkey,SLOT(onEnter()));
            touchkey->show();
            return true;
        }
    }
    if(watched == ui->lineEdit_adduser)
    {
        if(event->type() == QEvent::MouseButtonPress)
        {
            emit closeing_touchkey();
            touchkey = new keyboard;
            connect(touchkey->signalMapper,SIGNAL(mapped(const QString&)),this,SLOT(setText_adduser(const QString&)));
            connect(touchkey,SIGNAL(display_backspace()),this,SLOT(setBackspace_adduser()));
            connect(this,SIGNAL(closeing_touchkey()),touchkey,SLOT(onEnter()));
            touchkey->show();
            return true;
        }
    }
    if(watched == ui->lineEdit_adduser_2)
    {
        if(event->type() == QEvent::MouseButtonPress)
        {
            emit closeing_touchkey();
            touchkey = new keyboard;
            connect(touchkey->signalMapper,SIGNAL(mapped(const QString&)),this,SLOT(setText_adduser_2(const QString&)));
            connect(touchkey,SIGNAL(display_backspace()),this,SLOT(setBackspace_adduser_2()));
            connect(this,SIGNAL(closeing_touchkey()),touchkey,SLOT(onEnter()));
            touchkey->show();
            return true;
        }
    }
    if(watched == ui->lineEdit_deluser)
    {
        if(event->type() == QEvent::MouseButtonPress)
        {
            emit closeing_touchkey();
            touchkey = new keyboard;
            connect(touchkey->signalMapper,SIGNAL(mapped(const QString&)),this,SLOT(setText_deluser(const QString&)));
            connect(touchkey,SIGNAL(display_backspace()),this,SLOT(setBackspace_deluser()));
            connect(this,SIGNAL(closeing_touchkey()),touchkey,SLOT(onEnter()));
            touchkey->show();
            return true;
        }
    }
    //post添加

	if(watched == ui->lineEdit_postaddress)
	{
		if(event->type() == QEvent::MouseButtonPress)
		{
			emit closeing_touchkey();
			touchkey = new keyboard;
			connect(touchkey->signalMapper,SIGNAL(mapped(const QString&)),this,SLOT(setText_postaddress(QString)));
			connect(touchkey,SIGNAL(display_backspace()),this,SLOT(setBackspace_postaddress()));
			connect(this,SIGNAL(closeing_touchkey()),touchkey,SLOT(onEnter()));
			touchkey->show();
			return true;
		}
	}
    if(watched == ui->lineEdit_postdataid)
    {
        if(event->type() == QEvent::MouseButtonPress)
        {
            emit closeing_touchkey();
            touchkey = new keyboard;
            connect(touchkey->signalMapper,SIGNAL(mapped(const QString&)),this,SLOT(setText_postdataid(const QString&)));
            connect(touchkey,SIGNAL(display_backspace()),this,SLOT(setBackspace_postdataid()));
            connect(this,SIGNAL(closeing_touchkey()),touchkey,SLOT(onEnter()));
            touchkey->show();
            return true;
        }
    }
    if(watched == ui->lineEdit_postuserid)
    {
        if(event->type() == QEvent::MouseButtonPress)
        {
            emit closeing_touchkey();
            touchkey = new keyboard;
            connect(touchkey->signalMapper,SIGNAL(mapped(const QString&)),this,SLOT(setText_postuserid(const QString&)));
            connect(touchkey,SIGNAL(display_backspace()),this,SLOT(setBackspace_postuserid()));
            connect(this,SIGNAL(closeing_touchkey()),touchkey,SLOT(onEnter()));
            touchkey->show();
            return true;
        }
    }
    if(watched == ui->lineEdit_postversion)
    {
        if(event->type() == QEvent::MouseButtonPress)
        {
            emit closeing_touchkey();
            touchkey = new keyboard;
            connect(touchkey->signalMapper,SIGNAL(mapped(const QString&)),this,SLOT(setText_postversion(const QString&)));
            connect(touchkey,SIGNAL(display_backspace()),this,SLOT(setBackspace_postversion()));
            connect(this,SIGNAL(closeing_touchkey()),touchkey,SLOT(onEnter()));
            touchkey->show();
            return true;
        }
    }
	if(watched == ui->lineEdit_postusername)
	{
		if(event->type() == QEvent::MouseButtonPress)
		{
			emit closeing_touchkey();
			touchkey = new keyboard;
			connect(touchkey->signalMapper,SIGNAL(mapped(const QString&)),this,SLOT(setText_postusername(const QString&)));
			connect(touchkey,SIGNAL(display_backspace()),this,SLOT(setBackspace_postusername()));
			connect(this,SIGNAL(closeing_touchkey()),touchkey,SLOT(onEnter()));
			touchkey->show();
			return true;
		}
	}
	if(watched == ui->lineEdit_postpassword)
	{
		if(event->type() == QEvent::MouseButtonPress)
		{
			emit closeing_touchkey();
			touchkey = new keyboard;
			connect(touchkey->signalMapper,SIGNAL(mapped(const QString&)),this,SLOT(setText_postpassword(const QString&)));
			connect(touchkey,SIGNAL(display_backspace()),this,SLOT(setBackspace_postpassword()));
			connect(this,SIGNAL(closeing_touchkey()),touchkey,SLOT(onEnter()));
			touchkey->show();
			return true;
		}
	}
    //isoosi添加
	if(watched == ui->lineEdit_isoosi_UrlIp)
	{
		if(event->type() == QEvent::MouseButtonPress)
		{
			emit closeing_touchkey();
			touchkey = new keyboard;
			connect(touchkey->signalMapper,SIGNAL(mapped(const QString&)),this,SLOT(setText_isoosi_urlip(const QString&)));
			connect(touchkey,SIGNAL(display_backspace()),this,SLOT(setBackspace_isoosi_urlip()));
			connect(this,SIGNAL(closeing_touchkey()),touchkey,SLOT(onEnter()));
			touchkey->show();
			return true;
		}
	}
	if(watched == ui->lineEdit_isoosi_UrlPort)
	{
		if(event->type() == QEvent::MouseButtonPress)
		{
			emit closeing_touchkey();
			touchkey = new keyboard;
			connect(touchkey->signalMapper,SIGNAL(mapped(const QString&)),this,SLOT(setText_isoosi_urlport(const QString&)));
			connect(touchkey,SIGNAL(display_backspace()),this,SLOT(setBackspace_isoosi_urlport()));
			connect(this,SIGNAL(closeing_touchkey()),touchkey,SLOT(onEnter()));
			touchkey->show();
			return true;
		}
	}
    if(watched == ui->lineEdit_isoosi_mn)
    {
        if(event->type() == QEvent::MouseButtonPress)
        {
            emit closeing_touchkey();
            touchkey = new keyboard;
            connect(touchkey->signalMapper,SIGNAL(mapped(const QString&)),this,SLOT(setText_isoosi_mn(const QString&)));
            connect(touchkey,SIGNAL(display_backspace()),this,SLOT(setBackspace_isoosi_mn()));
            connect(this,SIGNAL(closeing_touchkey()),touchkey,SLOT(onEnter()));
            touchkey->show();
            return true;
        }
    }
    if(watched == ui->lineEdit_isoosi_pw)
    {
        if(event->type() == QEvent::MouseButtonPress)
        {
            emit closeing_touchkey();
            touchkey = new keyboard;
            connect(touchkey->signalMapper,SIGNAL(mapped(const QString&)),this,SLOT(setText_isoosi_pw(const QString&)));
            connect(touchkey,SIGNAL(display_backspace()),this,SLOT(setBackspace_isoosi_pw()));
            connect(this,SIGNAL(closeing_touchkey()),touchkey,SLOT(onEnter()));
            touchkey->show();
            return true;
        }
    }
	//isoosi添加重庆
	if(watched == ui->lineEdit_isoosi_cqid)
	{
		if(event->type() == QEvent::MouseButtonPress)
		{
			emit closeing_touchkey();
			touchkey = new keyboard;
			connect(touchkey->signalMapper,SIGNAL(mapped(const QString&)),this,SLOT(setText_isoosi_cqid(const QString&)));
			connect(touchkey,SIGNAL(display_backspace()),this,SLOT(setBackspace_isoosi_cqid()));
			connect(this,SIGNAL(closeing_touchkey()),touchkey,SLOT(onEnter()));
			touchkey->show();
			return true;
		}
	}
	//服务器添加
	if(watched == ui->lineEdit_MyServerId)
	{
		if(event->type() == QEvent::MouseButtonPress)
		{
			emit closeing_touchkey();
			touchkey = new keyboard;
			connect(touchkey->signalMapper,SIGNAL(mapped(const QString&)),this,SLOT(setText_myserverid(const QString&)));
			connect(touchkey,SIGNAL(display_backspace()),this,SLOT(setBackspace_myserverid()));
			connect(this,SIGNAL(closeing_touchkey()),touchkey,SLOT(onEnter()));
			touchkey->show();
			return true;
		}
	}
	if(watched == ui->lineEdit_MyServerPW)
	{
		if(event->type() == QEvent::MouseButtonPress)
		{
			emit closeing_touchkey();
			touchkey = new keyboard;
			connect(touchkey->signalMapper,SIGNAL(mapped(const QString&)),this,SLOT(setText_myserverpw(const QString&)));
			connect(touchkey,SIGNAL(display_backspace()),this,SLOT(setBackspace_myserverpw()));
			connect(this,SIGNAL(closeing_touchkey()),touchkey,SLOT(onEnter()));
			touchkey->show();
			return true;
		}
	}
	if(watched == ui->lineEdit_MyServerIp)
	{
		if(event->type() == QEvent::MouseButtonPress)
		{
			emit closeing_touchkey();
			touchkey = new keyboard;
			connect(touchkey->signalMapper,SIGNAL(mapped(const QString&)),this,SLOT(setText_myserverip(const QString&)));
			connect(touchkey,SIGNAL(display_backspace()),this,SLOT(setBackspace_myserverip()));
			connect(this,SIGNAL(closeing_touchkey()),touchkey,SLOT(onEnter()));
			touchkey->show();
			return true;
		}
	}
	if(watched == ui->lineEdit_MyServerPort)
	{
		if(event->type() == QEvent::MouseButtonPress)
		{
			emit closeing_touchkey();
			touchkey = new keyboard;
			connect(touchkey->signalMapper,SIGNAL(mapped(const QString&)),this,SLOT(setText_myserverport(const QString&)));
			connect(touchkey,SIGNAL(display_backspace()),this,SLOT(setBackspace_myserverport()));
			connect(this,SIGNAL(closeing_touchkey()),touchkey,SLOT(onEnter()));
			touchkey->show();
			return true;
		}
	}
    //**************added for radar*************/->
    if(watched == ui->lineEdit_x1)
    {
        if(event->type() == QEvent::MouseButtonPress)
        {
            emit closeing_touchkey();
            touchkey = new keyboard;
            connect(touchkey->signalMapper,SIGNAL(mapped(const QString&)),this,SLOT(setText_area_x1(const QString&)));
            connect(touchkey,SIGNAL(display_backspace()),this,SLOT(setBackspace_area_x1()));
            connect(this,SIGNAL(closeing_touchkey()),touchkey,SLOT(onEnter()));
            touchkey->show();
            return true;
        }
    }
    if(watched == ui->lineEdit_x2)
    {
        if(event->type() == QEvent::MouseButtonPress)
        {
            emit closeing_touchkey();
            touchkey = new keyboard;
            connect(touchkey->signalMapper,SIGNAL(mapped(const QString&)),this,SLOT(setText_area_x2(const QString&)));
            connect(touchkey,SIGNAL(display_backspace()),this,SLOT(setBackspace_area_x2()));
            connect(this,SIGNAL(closeing_touchkey()),touchkey,SLOT(onEnter()));
            touchkey->show();
            return true;
        }
    }
    if(watched == ui->lineEdit_x3)
    {
        if(event->type() == QEvent::MouseButtonPress)
        {
            emit closeing_touchkey();
            touchkey = new keyboard;
            connect(touchkey->signalMapper,SIGNAL(mapped(const QString&)),this,SLOT(setText_area_x3(const QString&)));
            connect(touchkey,SIGNAL(display_backspace()),this,SLOT(setBackspace_area_x3()));
            connect(this,SIGNAL(closeing_touchkey()),touchkey,SLOT(onEnter()));
            touchkey->show();
            return true;
        }
    }
    if(watched == ui->lineEdit_x4)
    {
        if(event->type() == QEvent::MouseButtonPress)
        {
            emit closeing_touchkey();
            touchkey = new keyboard;
            connect(touchkey->signalMapper,SIGNAL(mapped(const QString&)),this,SLOT(setText_area_x4(const QString&)));
            connect(touchkey,SIGNAL(display_backspace()),this,SLOT(setBackspace_area_x4()));
            connect(this,SIGNAL(closeing_touchkey()),touchkey,SLOT(onEnter()));
            touchkey->show();
            return true;
        }
    }
    if(watched == ui->lineEdit_x5)
    {
        if(event->type() == QEvent::MouseButtonPress)
        {
            emit closeing_touchkey();
            touchkey = new keyboard;
            connect(touchkey->signalMapper,SIGNAL(mapped(const QString&)),this,SLOT(setText_area_x5(const QString&)));
            connect(touchkey,SIGNAL(display_backspace()),this,SLOT(setBackspace_area_x5()));
            connect(this,SIGNAL(closeing_touchkey()),touchkey,SLOT(onEnter()));
            touchkey->show();
            return true;
        }
    }
    if(watched == ui->lineEdit_x6)
    {
        if(event->type() == QEvent::MouseButtonPress)
        {
            emit closeing_touchkey();
            touchkey = new keyboard;
            connect(touchkey->signalMapper,SIGNAL(mapped(const QString&)),this,SLOT(setText_area_x6(const QString&)));
            connect(touchkey,SIGNAL(display_backspace()),this,SLOT(setBackspace_area_x6()));
            connect(this,SIGNAL(closeing_touchkey()),touchkey,SLOT(onEnter()));
            touchkey->show();
            return true;
        }
    }
    if(watched == ui->lineEdit_y1)
    {
        if(event->type() == QEvent::MouseButtonPress)
        {
            emit closeing_touchkey();
            touchkey = new keyboard;
            connect(touchkey->signalMapper,SIGNAL(mapped(const QString&)),this,SLOT(setText_area_y1(const QString&)));
            connect(touchkey,SIGNAL(display_backspace()),this,SLOT(setBackspace_area_y1()));
            connect(this,SIGNAL(closeing_touchkey()),touchkey,SLOT(onEnter()));
            touchkey->show();
            return true;
        }
    }
    if(watched == ui->lineEdit_y2)
    {
        if(event->type() == QEvent::MouseButtonPress)
        {
            emit closeing_touchkey();
            touchkey = new keyboard;
            connect(touchkey->signalMapper,SIGNAL(mapped(const QString&)),this,SLOT(setText_area_y2(const QString&)));
            connect(touchkey,SIGNAL(display_backspace()),this,SLOT(setBackspace_area_y2()));
            connect(this,SIGNAL(closeing_touchkey()),touchkey,SLOT(onEnter()));
            touchkey->show();
            return true;
        }
    }
    if(watched == ui->lineEdit_y3)
    {
        if(event->type() == QEvent::MouseButtonPress)
        {
            emit closeing_touchkey();
            touchkey = new keyboard;
            connect(touchkey->signalMapper,SIGNAL(mapped(const QString&)),this,SLOT(setText_area_y3(const QString&)));
            connect(touchkey,SIGNAL(display_backspace()),this,SLOT(setBackspace_area_y3()));
            connect(this,SIGNAL(closeing_touchkey()),touchkey,SLOT(onEnter()));
            touchkey->show();
            return true;
        }
    }
    if(watched == ui->lineEdit_y4)
    {
        if(event->type() == QEvent::MouseButtonPress)
        {
            emit closeing_touchkey();
            touchkey = new keyboard;
            connect(touchkey->signalMapper,SIGNAL(mapped(const QString&)),this,SLOT(setText_area_y4(const QString&)));
            connect(touchkey,SIGNAL(display_backspace()),this,SLOT(setBackspace_area_y4()));
            connect(this,SIGNAL(closeing_touchkey()),touchkey,SLOT(onEnter()));
            touchkey->show();
            return true;
        }
    }
    if(watched == ui->lineEdit_y5)
    {
        if(event->type() == QEvent::MouseButtonPress)
        {
            emit closeing_touchkey();
            touchkey = new keyboard;
            connect(touchkey->signalMapper,SIGNAL(mapped(const QString&)),this,SLOT(setText_area_y5(const QString&)));
            connect(touchkey,SIGNAL(display_backspace()),this,SLOT(setBackspace_area_y5()));
            connect(this,SIGNAL(closeing_touchkey()),touchkey,SLOT(onEnter()));
            touchkey->show();
            return true;
        }
    }
    if(watched == ui->lineEdit_y6)
    {
        if(event->type() == QEvent::MouseButtonPress)
        {
            emit closeing_touchkey();
            touchkey = new keyboard;
            connect(touchkey->signalMapper,SIGNAL(mapped(const QString&)),this,SLOT(setText_area_y6(const QString&)));
            connect(touchkey,SIGNAL(display_backspace()),this,SLOT(setBackspace_area_y6()));
            connect(this,SIGNAL(closeing_touchkey()),touchkey,SLOT(onEnter()));
            touchkey->show();
            return true;
        }
    }
    if(watched == ui->lineEdit_yuzhi_handinput)
    {
        if(event->type() == QEvent::MouseButtonPress)
        {
            emit closeing_touchkey();
            touchkey = new keyboard;
            connect(touchkey->signalMapper,SIGNAL(mapped(const QString&)),this,SLOT(setText_yuzhi_handinput(const QString&)));
            connect(touchkey,SIGNAL(display_backspace()),this,SLOT(setBackspace_yuzhi_handinput()));
            connect(this,SIGNAL(closeing_touchkey()),touchkey,SLOT(onEnter()));
            touchkey->show();
            return true;
        }
    }
    //**************added for radar*************/<-
    //**************added for reoilgas*********/
    if(watched == ui->lineEdit_pv)
    {
        if(event->type()==QEvent::MouseButtonPress)
        {
            emit closeing_touchkey();
            touchkey = new keyboard;
            connect(touchkey->signalMapper,SIGNAL(mapped(const QString&)),this,SLOT(setText_pv(const QString&)));
            connect(touchkey,SIGNAL(display_backspace()),this,SLOT(setBackspace_pv()));
            connect(this,SIGNAL(closeing_touchkey()),touchkey,SLOT(onEnter()));
            touchkey->show();
            return true;
        }
    }
    if(watched == ui->lineEdit_pv_nega)
    {
        if(event->type()==QEvent::MouseButtonPress)
        {
            emit closeing_touchkey();
            touchkey = new keyboard;
            connect(touchkey->signalMapper,SIGNAL(mapped(const QString&)),this,SLOT(setText_pv_nega(const QString&)));
            connect(touchkey,SIGNAL(display_backspace()),this,SLOT(setBackspace_pv_nega()));
            connect(this,SIGNAL(closeing_touchkey()),touchkey,SLOT(onEnter()));
            touchkey->show();
            return true;
        }
    }
    return QWidget::eventFilter(watched,event);

}
//网络设置
void systemset::setText_ip(const QString &text)
{
    ui->lineEdit_ip->insert(text);
}
void systemset::setText_mask(const QString &text)
{
    ui->lineEdit_mask->insert(text);
}
void systemset::setText_bcast(const QString &text)
{
    ui->lineEdit_bcast->insert(text);
}
void systemset::setText_ifisport_udp(const QString &text)
{
    ui->lineEdit_ifisport_udp->insert(text);
}
void systemset::setText_ifisport_tcp(const QString &text)
{
    ui->lineEdit_ifisport_tcp->insert(text);
}
void systemset::setText_hubei_station_id(const QString &text)
{
    ui->lineEdit_hubei_station_id->insert(text);
}
void systemset::setText_myserverid(const QString &text)
{
	ui->lineEdit_MyServerId->insert(text);
}
void systemset::setText_myserverpw(const QString &text)
{
	ui->lineEdit_MyServerPW->insert(text);
}
void systemset::setText_myserverip(const QString &text)
{
	ui->lineEdit_MyServerIp->insert(text);
}
void systemset::setText_myserverport(const QString &text)
{
	ui->lineEdit_MyServerPort->insert(text);
}



void systemset::setBackspace_ip()
{
    ui->lineEdit_ip->backspace();
}
void systemset::setBackspace_mask()
{
    ui->lineEdit_mask->backspace();
}
void systemset::setBackspace_bcast()
{
    ui->lineEdit_bcast->backspace();
}
void systemset::setBackspace_ifisport_udp()
{
    ui->lineEdit_ifisport_udp->backspace();
}
void systemset::setBackspace_ifisport_tcp()
{
    ui->lineEdit_ifisport_tcp->backspace();
}
void systemset::setBackspace_hubei_station_id()
{
    ui->lineEdit_hubei_station_id->backspace();
}
void systemset::setBackspace_myserverid()
{
	ui->lineEdit_MyServerId->backspace();
}
void systemset::setBackspace_myserverpw()
{
	ui->lineEdit_MyServerPW->backspace();
}
void systemset::setBackspace_myserverip()
{
	ui->lineEdit_MyServerIp->backspace();
}
void systemset::setBackspace_myserverport()
{
	ui->lineEdit_MyServerPort->backspace();
}

//用户管理输入
void systemset::setText_manid(const QString &text)
{
    ui->lineEdit_manid->insert(text);
}

void systemset::setText_manpw(const QString& text)
{
    ui->lineEdit_manpw->insert(text);
}
void systemset::setText_username(const QString &text)
{
    ui->lineEdit_username->insert(text);
}
void systemset::setText_userpw(const QString& text)
{
    ui->lineEdit_userpw->insert(text);
}
void systemset::setText_adduser(const QString& text)
{
    ui->lineEdit_adduser->insert(text);
}
void systemset::setText_adduser_2(const QString &text)
{
    ui->lineEdit_adduser_2->insert(text);
}
void systemset::setText_deluser(const QString &text)
{
    ui->lineEdit_deluser->insert(text);
}
void systemset::setBackspace_mainid()
{
    ui->lineEdit_manid->backspace();
}
void systemset::setBackspace_manpw()
{
    ui->lineEdit_manpw->backspace();
}
void systemset::setBackspace_username()
{
    ui->lineEdit_username->backspace();
}
void systemset::setBackspace_userpw()
{
    ui->lineEdit_userpw->backspace();
}
void systemset::setBackspace_adduser()
{
    ui->lineEdit_adduser->backspace();
}
void systemset::setBackspace_adduser_2()
{
    ui->lineEdit_adduser_2->backspace();
}
void systemset::setBackspace_deluser()
{
    ui->lineEdit_deluser->backspace();
}
//post添加
void systemset::setText_postaddress(const QString& text)
{
	ui->lineEdit_postaddress->insert(text);
}
void systemset::setText_postdataid(const QString& text)
{
    ui->lineEdit_postdataid->insert(text);
}
void systemset::setText_postuserid(const QString &text)
{
    ui->lineEdit_postuserid->insert(text);
}
void systemset::setText_postversion(const QString &text)
{
    ui->lineEdit_postversion->insert(text);
}
void systemset::setText_isoosi_urlip(const QString &text)
{
	ui->lineEdit_isoosi_UrlIp->insert(text);
}
void systemset::setText_isoosi_urlport(const QString &text)
{
	ui->lineEdit_isoosi_UrlPort->insert(text);
}
void systemset::setText_isoosi_mn(const QString &text)
{
    ui->lineEdit_isoosi_mn->insert(text);
}
void systemset::setText_isoosi_pw(const QString &text)
{
    ui->lineEdit_isoosi_pw->insert(text);
}
void systemset::setText_isoosi_cqid(const QString &text)
{
	ui->lineEdit_isoosi_cqid->insert(text);
}
void systemset::setText_postusername(const QString &text)
{
	ui->lineEdit_postusername->insert(text);
}
void systemset::setText_postpassword(const QString &text)
{
	ui->lineEdit_postpassword->insert(text);
}
void systemset::setBackspace_postaddress()
{
	ui->lineEdit_postaddress->backspace();
}
void systemset::setBackspace_postdataid()
{
    ui->lineEdit_postdataid->backspace();
}
void systemset::setBackspace_postuserid()
{
    ui->lineEdit_postuserid->backspace();
}
void systemset::setBackspace_postversion()
{
    ui->lineEdit_postversion->backspace();
}
void systemset::setBackspace_isoosi_urlip()
{
	ui->lineEdit_isoosi_UrlIp->backspace();
}
void systemset::setBackspace_isoosi_urlport()
{
	ui->lineEdit_isoosi_UrlPort->backspace();
}
void systemset::setBackspace_isoosi_mn()
{
    ui->lineEdit_isoosi_mn->backspace();
}
void systemset::setBackspace_isoosi_pw()
{
    ui->lineEdit_isoosi_pw->backspace();
}
void systemset::setBackspace_isoosi_cqid()
{
	ui->lineEdit_isoosi_cqid->backspace();
}
void systemset::setBackspace_postusername()
{
	ui->lineEdit_postusername->backspace();
}
void systemset::setBackspace_postpassword()
{
	ui->lineEdit_postpassword->backspace();
}


void systemset::on_toolButton_manid_clicked()       //对管理员帐号进行重命名
{
    //用户名格式验证
    char *manager_name;
    QByteArray Q_manager_name = ui->lineEdit_manid->text().toLatin1();
    manager_name = Q_manager_name.data();
    if(strlen(manager_name) > 0)
    {
        warn_manid_showed();
    }
    else
    {
        ui->widget_warn_inputerror->setHidden(0);
    }
}

void systemset::on_toolButton_manpw_clicked()       //管理员密码修改
{
    //密码格式验证
    char *manager_passwd;
    QByteArray Q_manager_passwd = ui->lineEdit_manpw->text().toLatin1();
    manager_passwd = Q_manager_passwd.data();

    if(strlen(manager_passwd) == 6)
    {
        warn_manpw_showed();
    }
    else
    {
        ui->widget_warn_inputerror->setHidden(0);
    }
}

void systemset::on_toolButton_userid_clicked()      //用户密码修改：S1用户名输入
{
    //用户名格式验证
    char *user_name;
    char user_name_[40] = {0};
    QByteArray Q_username = ui->lineEdit_username->text().toLatin1();
    user_name = Q_username.data();
    if(strlen(user_name) > 0)
    {
        //用户存在验证
        sprintf(user_name_,"%s%s",USER_USER,user_name);
        int fd;
        fd = ::open(user_name_,O_RDONLY);           //若路径不存在，fd返回-1
        if(fd != -1)
        {
            ui->lineEdit_username->setHidden(1);
            ui->toolButton_userid->setHidden(1);
            emit closeing_touchkey();
        }
        else
        {
            ui->widget_warn_user_noexist->setHidden(0);
            emit closeing_touchkey();
        }
        ::close(fd);
    }
    else
    {
        ui->widget_warn_inputerror->setHidden(0);
    }
}
void systemset::on_toolButton_userpw_clicked()      //用户密码修改：S2密码输入
{
    //密码格式验证
    char *user_passwd;
    QByteArray Q_user_passwd = ui->lineEdit_userpw->text().toLatin1();
    user_passwd = Q_user_passwd.data();
    if(strlen(user_passwd) == 6)
    {
        warn_userpw_showed();
    }
    else
    {
        ui->widget_warn_inputerror->setHidden(0);
    }
}
void systemset::on_toolButton_useradd_clicked()         //新用户名添加:S1用户名输入
{
    //用户名格式验证
    char *newuser_name;
    QByteArray Q_newuser_name = ui->lineEdit_adduser->text().toLatin1();
    newuser_name = Q_newuser_name.data();
    if(strlen(newuser_name) > 0)
    {
        ui->lineEdit_adduser->setHidden(1);
        ui->toolButton_useradd->setHidden(1);
        emit closeing_touchkey();
    }
    else
    {
        ui->widget_warn_inputerror->setHidden(0);
    }
}
void systemset::on_toolButton_useradd_2_clicked()       //新用户添加：S2密码输入
{
    //密码格式验证
    char *newuser_passwd;
    QByteArray Q_newuser_passwd = ui->lineEdit_adduser_2->text().toLatin1();
    newuser_passwd = Q_newuser_passwd.data();
    if(strlen(newuser_passwd) == 6)
    {
        warn_add_showed();
    }
    else
    {
        ui->widget_warn_inputerror->setHidden(0);
    }
}
void systemset::on_toolButton_userdel_clicked()         //用户删除
{
    //用户名格式验证
    char *user_name;
    char user_name_[40] = {0};
    QByteArray Q_username = ui->lineEdit_deluser->text().toLatin1();
    user_name = Q_username.data();
    if(strlen(user_name) > 0)
    {
        //用户存在验证
        sprintf(user_name_,"%s%s",USER_USER,user_name);
        int fd;
        fd = ::open(user_name_,O_RDONLY);
        if(fd != -1)
        {
            warn_del_showed();
        }
        else
        {
            ui->widget_warn_user_noexist->setHidden(0);
            emit closeing_touchkey();
        }
    }
    else
    {
        ui->widget_warn_inputerror->setHidden(0);
    }
}
void systemset::warn_manid_showed()   //管理员ID重命名
{
    ui->widget_warn_man_id->setHidden(0);
    emit closeing_touchkey();
}
void systemset::warn_manpw_showed()   //管理员密码修改
{
    ui->widget_warn_man_pw->setHidden(0);
    emit closeing_touchkey();
}
void systemset::warn_userpw_showed()  //用户密码修改
{
    ui->widget_warn_user_pw->setHidden(0);
    emit closeing_touchkey();
}
void systemset::warn_add_showed()     //新用户添加
{
    ui->widget_warn_user_add->setHidden(0);
    emit closeing_touchkey();
}
void systemset::warn_del_showed()     //用户删除
{
    ui->widget_warn_user_del->setHidden(0);
    emit closeing_touchkey();
}

void systemset::on_toolButton_warn_manid_enter_clicked()     //警告界面：管理员重命名
{
    char *oldname_temp;
    char *newname_temp;
    char oldname[40];
    char newname[40];

    unsigned char i = 0;
    QDir *dir = new QDir(USER_MANAGER);
    foreach(QFileInfo file_all,dir->entryInfoList())
    {
        if(file_all.isFile())
        {
            QByteArray Q_oldname = file_all.fileName().toLatin1();
            oldname_temp = Q_oldname.data();
            i++;
        }
    }

    sprintf(oldname,"%s%s",USER_MANAGER,oldname_temp);

    QByteArray Q_newname = ui->lineEdit_manid->text().toLatin1();
    newname_temp = Q_newname.data();
    sprintf(newname,"%s%s",USER_MANAGER,newname_temp);
    printf("%s----%s \n",oldname,newname);
    rename(oldname,newname);
    system("sync");

    on_toolButton_warn_manid_cancel_clicked();
}

void systemset::on_toolButton_warn_manpw_enter_clicked()    //警告界面：管理员密码重置
{
    //管理员id提取
    char *manager_name_temp;
    char manager[40];
    unsigned char i = 0;
    QDir *dir = new QDir(USER_MANAGER);
    foreach(QFileInfo file_name,dir->entryInfoList())
    {
        if(file_name.isFile())
        {
            QByteArray Q_manager = file_name.fileName().toLatin1();
            manager_name_temp = Q_manager.data();
            i++;
        }
    }
    sprintf(manager,"%s%s",USER_MANAGER,manager_name_temp);
    printf("%s---\n",manager);

    //新密码输入提取
    char *manager_passwd;
    QByteArray Q_passwd = ui->lineEdit_manpw->text().toLatin1();
    manager_passwd = Q_passwd.data();

    int fd;
    fd = ::open(manager,O_WRONLY);
    write(fd,manager_passwd,6);
    ::close(fd);
    system("sync");

    on_toolButton_warn_manpw_cancel_clicked();
}

void systemset::on_toolButton_warn_userpw_enter_clicked()   //警告界面：用户密码重置
{
    //用户名提取
    char *user_name_temp;
    char user_name[40];
    QByteArray Q_username = ui->lineEdit_username->text().toLatin1();
    user_name_temp = Q_username.data();
    sprintf(user_name,"%s%s",USER_USER,user_name_temp);

    //用户新密码提取
    char *user_passwd;
    QByteArray Q_passwd = ui->lineEdit_userpw->text().toLatin1();
    user_passwd = Q_passwd.data();

    int fd;
    fd = ::open(user_name,O_WRONLY);
    write(fd,user_passwd,6);
    ::close(fd);
    system("sync");

    on_toolButton_warn_userpw_cancel_clicked();
}

void systemset::on_toolButton_warn_add_enter_clicked()      //警告界面：添加新用户
{
    //新用户名提取
    char *new_username_temp;
    char new_username[40];
    QByteArray Q_new_username = ui->lineEdit_adduser->text().toLatin1();
    new_username_temp = Q_new_username.data();
    sprintf(new_username,"%s%s",USER_USER,new_username_temp);

    //新用户密码提取
    char *new_userpasswd;
    QByteArray Q_new_userpasswd = ui->lineEdit_adduser_2->text().toLatin1();
    new_userpasswd = Q_new_userpasswd.data();

    int fd;
    fd = ::open(new_username,O_RDWR|O_CREAT);
    write(fd,new_userpasswd,6);
    ::close(fd);
    system("sync");

    on_toolButton_warn_add_cancel_clicked();

    //用户列表刷新
    ui->textBrowser->clear();
    unsigned char i = 0;
    QString Line[20];               //用户数量限制&显示
    QDir *dir = new QDir(USER_USER);
    foreach(QFileInfo list_file,dir->entryInfoList())
    {
        if(list_file.isFile())
        {
            Line[i] = list_file.fileName();

            if(!(Line[i].isEmpty()))
            {
                ui->textBrowser->append(Line[i]);
                i++;
                if(i > 19)
                {
                    i = 0;
                    ui->toolButton_useradd->setEnabled(0);
                }
                else
                {
                    ui->toolButton_useradd->setEnabled(1);
                }
            }
        }
    }
}

void systemset::on_toolButton_warn_del_enter_clicked()      //警告界面：删除用户
{
    char *username_temp;
    char username[40];
    QByteArray Q_username = ui->lineEdit_deluser->text().toLatin1();
    username_temp = Q_username.data();
    sprintf(username,"%s%s",USER_USER,username_temp);
    printf("%s",username);
    remove(username);

    on_toolButton_warn_del_cancel_clicked();

    //用户列表刷新
    ui->textBrowser->clear();
    unsigned char i = 0;
    QString Line[20];
    QDir *dir = new QDir(USER_USER);
    foreach(QFileInfo list_file,dir->entryInfoList())
    {
        if(list_file.isFile())
        {
            Line[i] = list_file.fileName();

            if(!(Line[i].isEmpty()))
            {
                ui->textBrowser->append(Line[i]);
                i++;
                if(i > 19)
                {
                    i = 0;
                    ui->toolButton_useradd->setEnabled(0);
                }
                else
                {
                    ui->toolButton_useradd->setEnabled(1);
                }
            }
        }
    }
}

void systemset::on_toolButton_warn_manid_cancel_clicked()   //警告界面取消：管理员ID修改
{
    ui->lineEdit_manid->clear();
    ui->widget_warn_man_id->setHidden(1);
}

void systemset::on_toolButton_warn_manpw_cancel_clicked()   //警告界面取消：管理员密码修改
{
    ui->lineEdit_manpw->clear();
    ui->widget_warn_man_pw->setHidden(1);
}

void systemset::on_toolButton_warn_userpw_cancel_clicked()  //警告界面取消：用户密码修改
{
    ui->widget_warn_user_pw->setHidden(1);
    ui->lineEdit_username->clear();
    ui->lineEdit_userpw->clear();
    ui->lineEdit_username->setHidden(0);
    ui->toolButton_userid->setHidden(0);
}

void systemset::on_toolButton_warn_add_cancel_clicked() //警告界面取消：添加新用户
{
    ui->widget_warn_user_add->setHidden(1);
    ui->lineEdit_adduser->clear();
    ui->lineEdit_adduser_2->clear();
    ui->lineEdit_adduser->setHidden(0);
    ui->toolButton_useradd->setHidden(0);
}

void systemset::on_toolButton_warn_del_cancel_clicked() //警告界面取消：删除用户
{
    ui->lineEdit_deluser->clear();
    ui->widget_warn_user_del->setHidden(1);
}

void systemset::on_toolButton_warn_inputerror_enter_clicked()   //密码格式输入错误弹窗
{
    ui->toolButton_userid->setHidden(0);
    ui->toolButton_useradd->setHidden(0);
    ui->lineEdit_adduser->setHidden(0);
    ui->lineEdit_username->setHidden(0);

    ui->widget_warn_man_id->setHidden(1);
    ui->widget_warn_man_pw->setHidden(1);
    ui->widget_warn_user_add->setHidden(1);
    ui->widget_warn_user_del->setHidden(1);
    ui->widget_warn_user_pw->setHidden(1);
    ui->widget_warn_inputerror->setHidden(1);

    ui->lineEdit_adduser->clear();
    ui->lineEdit_adduser_2->clear();
    ui->lineEdit_deluser->clear();
    ui->lineEdit_manid->clear();
    ui->lineEdit_manpw->clear();
    ui->lineEdit_username->clear();
    ui->lineEdit_userpw->clear();

    emit closeing_touchkey();
}

void systemset::on_toolButton_warn_usernoexist_enter_clicked()  //用户名不存在弹窗
{
    ui->toolButton_userid->setHidden(0);
    ui->lineEdit_username->setHidden(0);
    ui->widget_warn_user_noexist->setHidden(1);
    ui->lineEdit_username->clear();
    ui->lineEdit_deluser->clear();
}

void systemset::whoareyou_userset(unsigned char t)         //判断为非管理员帐号的设置
{
  //  ui->toolButton_manager->setHidden(1);
    if(t == 1)
    {
        ui->tab_yuzhi->setEnabled(0);
        ui->pushButton_allclean->setHidden(1);//清除报警状态
		ui->pushButton_U_clear->setHidden(1);//删除优盘盘符
        ui->pushButton_shield_network->setHidden(1);//网络上传报警数据修正
        ui->toolButton_isoosi_pb->setHidden(1);
        ui->widget_alset->setHidden(1);//气液比相关设置
    }
    if(t == 2)
    {
        ui->tab_yuzhi->setEnabled(0);
        ui->frame_user_guanli->setHidden(1);//用户管理隐藏
        ui->pushButton_allclean->setHidden(1);
		ui->pushButton_U_clear->setHidden(1);//删除优盘盘符
        ui->pushButton_shield_network->setHidden(1);//网络上传报警数据修正
        ui->toolButton_isoosi_pb->setHidden(1);
        ui->widget_alset->setHidden(1);//气液比相关设置
    }
    if(t == 3)
    {
        ui->pushButton_allclean->setHidden(0);
		ui->pushButton_U_clear->setHidden(0);//删除优盘盘符
        ui->pushButton_shield_network->setHidden(0);//网络上传报警数据修正
        ui->toolButton_isoosi_pb->setHidden(0);
        ui->widget_alset->setHidden(0);//气液比相关设置
    }
}
void systemset::dispset_for_managerid(const QString &text)
{
    ui->label_managerid->setText(text);
}
void systemset::on_toolButton_poweroff_enter_clicked()  //确认关机
{
    add_value_operateinfo(ui->label_managerid->text(),"关闭系统");
    sleep(1);
    system("poweroff");
}
void systemset::on_toolButton_poweroff_cancl_clicked()  //取消关机
{
    ui->widget_warn_poweroff->setHidden(1);
}
void systemset::on_toolButton_poweroff_clicked()    //关机按钮
{
    ui->widget_warn_poweroff->setHidden(0);
}

void systemset::on_toolButton_clicked()     //升级按钮
{
    ui->widget_warn_update->setHidden(0);
}
void systemset::on_toolButton_warn_update_enter_clicked()   //确认升级
{
    add_value_operateinfo(ui->label_managerid->text(),"进行系统升级");
    system("chmod +x /etc/init.d/S94update");
    system("sync");
    system("reboot");
}
void systemset::on_toolButton_warn_update_cancl_clicked()   //取消升级
{
    ui->widget_warn_update->setHidden(1);
}

void systemset::on_toolButton_histoclr_clicked()    //历史记录清除
{
    ui->widget_warn_histclr->setHidden(0);
}
void systemset::on_toolButton_warn_histclr_enter_clicked()  //确认进行历史记录清除
{
    ui->widget_warn_histclr->setHidden(1);
    history_clearall();
}
void systemset::on_toolButton_warn_histclr_cancl_clicked()  //取消历史记录清除
{
    ui->widget_warn_histclr->setHidden(1);
}

void systemset::on_toolButton_gujian_clicked()      //固件升级按键
{
    ui->widget_warn_gujian->setHidden(0);
}

void systemset::on_toolButton_warn_gujian_enter_clicked()   //确认固件升级
{
    add_value_operateinfo(ui->label_managerid->text(),"进行固件升级");
    system("chmod +x /etc/init.d/S95sysupdate");
    system("sync");
    system("reboot");
}

void systemset::on_toolButton_warn_gujian_cancl_clicked()   //取消固件升级
{
    ui->widget_warn_gujian->setHidden(1);
}

void systemset::history_autoip()
{
    add_value_operateinfo(ui->label_managerid->text(),"DHCP动态获取IP");
}
void systemset::history_ipset()
{
    add_value_operateinfo(ui->label_managerid->text(),"网段手动设置");
}
//*******************added for radar*********/
//区域设置-》手动设置-》输入
void systemset::setText_area_x1(const QString &text)
{
    if((text == "0")||(text == "1")||(text == "2")||(text == "3")||(text == "4")||(text == "5")||
            (text == "6")||(text == "7")||(text == "8")||(text == "9")||(text == ".")||(text == "-"))
    {
        if(ui->lineEdit_x1->text() == "0")
        {
            ui->lineEdit_x1->clear();
            ui->lineEdit_x1->insert(text);
        }
        else
        {
            ui->lineEdit_x1->insert(text);
        }
        //判定输入是否在0-50内
        float area_limit = ui->lineEdit_x1->text().toFloat();
        if((area_limit > 25.0) ||(area_limit < (-25.0)))
        {
            ui->lineEdit_x1->setText("0");
        }

    }
    uchar which_one = ui->comboBox_whicharea->currentIndex();

    Master_Boundary_Point_Disp[Communication_Machine][which_one][0][0] = (ui->lineEdit_x1->text().toFloat())*10;
}
void systemset::setText_area_x2(const QString &text)
{
    if((text == "0")||(text == "1")||(text == "2")||(text == "3")||(text == "4")||(text == "5")||
            (text == "6")||(text == "7")||(text == "8")||(text == "9")||(text == ".")||(text == "-"))
    {
        if(ui->lineEdit_x2->text() == "0")
        {
            ui->lineEdit_x2->clear();
            ui->lineEdit_x2->insert(text);
        }
        else
        {
            ui->lineEdit_x2->insert(text);
        }

        //判定输入是否在0-50内
        float area_limit = ui->lineEdit_x2->text().toFloat();
        if((area_limit > 25.0) ||(area_limit < (-25.0)))
        {
            ui->lineEdit_x2->setText("0");
        }

    }
    uchar which_one = ui->comboBox_whicharea->currentIndex();
    Master_Boundary_Point_Disp[Communication_Machine][which_one][1][0] = (ui->lineEdit_x2->text().toFloat())*10;
}
void systemset::setText_area_x3(const QString &text)
{
    if((text == "0")||(text == "1")||(text == "2")||(text == "3")||(text == "4")||(text == "5")||
            (text == "6")||(text == "7")||(text == "8")||(text == "9")||(text == ".")||(text == "-"))
    {
        if(ui->lineEdit_x3->text() == "0")
        {
            ui->lineEdit_x3->clear();
            ui->lineEdit_x3->insert(text);
        }
        else
        {
            ui->lineEdit_x3->insert(text);
        }

        //判定输入是否在0-50内
        float area_limit = ui->lineEdit_x3->text().toFloat();
        if((area_limit > 25.0) ||(area_limit < (-25.0)))
        {
            ui->lineEdit_x3->setText("0");
        }

    }
    uchar which_one = ui->comboBox_whicharea->currentIndex();
    Master_Boundary_Point_Disp[Communication_Machine][which_one][2][0] = (ui->lineEdit_x3->text().toFloat())*10;
}
void systemset::setText_area_x4(const QString &text)
{
    if((text == "0")||(text == "1")||(text == "2")||(text == "3")||(text == "4")||(text == "5")||
            (text == "6")||(text == "7")||(text == "8")||(text == "9")||(text == ".")||(text == "-"))
    {
        if(ui->lineEdit_x4->text() == "0")
        {
            ui->lineEdit_x4->clear();
            ui->lineEdit_x4->insert(text);
        }
        else
        {
            ui->lineEdit_x4->insert(text);
        }

        //判定输入是否在0-50内
        float area_limit = ui->lineEdit_x4->text().toFloat();
        if((area_limit > 25.0) ||(area_limit < (-25.0)))
        {
            ui->lineEdit_x4->setText("0");
        }

    }
    uchar which_one = ui->comboBox_whicharea->currentIndex();
    Master_Boundary_Point_Disp[Communication_Machine][which_one][3][0] = (ui->lineEdit_x4->text().toFloat())*10;
}
void systemset::setText_area_x5(const QString &text)
{
    if((text == "0")||(text == "1")||(text == "2")||(text == "3")||(text == "4")||(text == "5")||
            (text == "6")||(text == "7")||(text == "8")||(text == "9")||(text == ".")||(text == "-"))
    {
        if(ui->lineEdit_x5->text() == "0")
        {
            ui->lineEdit_x5->clear();
            ui->lineEdit_x5->insert(text);
        }
        else
        {
            ui->lineEdit_x5->insert(text);
        }

        //判定输入是否在0-50内
        float area_limit = ui->lineEdit_x5->text().toFloat();
        if((area_limit > 25.0) ||(area_limit < (-25.0)))
        {
            ui->lineEdit_x5->setText("0");
        }

    }
    uchar which_one = ui->comboBox_whicharea->currentIndex();
    Master_Boundary_Point_Disp[Communication_Machine][which_one][4][0] = (ui->lineEdit_x5->text().toFloat())*10;
}
void systemset::setText_area_x6(const QString &text)
{
    if((text == "0")||(text == "1")||(text == "2")||(text == "3")||(text == "4")||(text == "5")||
            (text == "6")||(text == "7")||(text == "8")||(text == "9")||(text == ".")||(text == "-"))
    {
        if(ui->lineEdit_x6->text() == "0")
        {
            ui->lineEdit_x6->clear();
            ui->lineEdit_x6->insert(text);
        }
        else
        {
            ui->lineEdit_x6->insert(text);
        }

        //判定输入是否在0-50内
        float area_limit = ui->lineEdit_x6->text().toFloat();
        if((area_limit > 25.0) ||(area_limit < (-25.0)))
        {
            ui->lineEdit_x6->setText("0");
        }

    }
    uchar which_one = ui->comboBox_whicharea->currentIndex();
    Master_Boundary_Point_Disp[Communication_Machine][which_one][5][0] = (ui->lineEdit_x6->text().toFloat())*10;
}
void systemset::setText_area_y1(const QString &text)
{
    if((text == "0")||(text == "1")||(text == "2")||(text == "3")||(text == "4")||(text == "5")||
            (text == "6")||(text == "7")||(text == "8")||(text == "9")||(text == ".")||(text == "-"))
    {
        if(ui->lineEdit_y1->text() == "0")
        {
            ui->lineEdit_y1->clear();
            ui->lineEdit_y1->insert(text);
        }
        else
        {
            ui->lineEdit_y1->insert(text);
        }

        //判定输入是否在0-50内
        float area_limit = ui->lineEdit_y1->text().toFloat();
        if((area_limit < 0) ||(area_limit >= 50))
        {
            ui->lineEdit_y1->setText("0");
        }

    }
    uchar which_one = ui->comboBox_whicharea->currentIndex();
    Master_Boundary_Point_Disp[Communication_Machine][which_one][0][1] = (ui->lineEdit_y1->text().toFloat())*10;
}
void systemset::setText_area_y2(const QString &text)
{
    if((text == "0")||(text == "1")||(text == "2")||(text == "3")||(text == "4")||(text == "5")||
            (text == "6")||(text == "7")||(text == "8")||(text == "9")||(text == ".")||(text == "-"))
    {
        if(ui->lineEdit_y2->text() == "0")
        {
            ui->lineEdit_y2->clear();
            ui->lineEdit_y2->insert(text);
        }
        else
        {
            ui->lineEdit_y2->insert(text);
        }

        //判定输入是否在0-50内
        float area_limit = ui->lineEdit_y2->text().toFloat();
        if((area_limit < 0) ||(area_limit >= 50))
        {
            ui->lineEdit_y2->setText("0");
        }

    }
    uchar which_one = ui->comboBox_whicharea->currentIndex();
    Master_Boundary_Point_Disp[Communication_Machine][which_one][1][1] = (ui->lineEdit_y2->text().toFloat())*10;
}
void systemset::setText_area_y3(const QString &text)
{
    if((text == "0")||(text == "1")||(text == "2")||(text == "3")||(text == "4")||(text == "5")||
            (text == "6")||(text == "7")||(text == "8")||(text == "9")||(text == ".")||(text == "-"))
    {
        if(ui->lineEdit_y3->text() == "0")
        {
            ui->lineEdit_y3->clear();
            ui->lineEdit_y3->insert(text);
        }
        else
        {
            ui->lineEdit_y3->insert(text);
        }

        //判定输入是否在0-50内
        float area_limit = ui->lineEdit_y3->text().toFloat();
        if((area_limit < 0) ||(area_limit >= 50))
        {
            ui->lineEdit_y3->setText("0");
        }

    }
    uchar which_one = ui->comboBox_whicharea->currentIndex();
    Master_Boundary_Point_Disp[Communication_Machine][which_one][2][1] = (ui->lineEdit_y3->text().toFloat())*10;
}
void systemset::setText_area_y4(const QString &text)
{
    if((text == "0")||(text == "1")||(text == "2")||(text == "3")||(text == "4")||(text == "5")||
            (text == "6")||(text == "7")||(text == "8")||(text == "9")||(text == ".")||(text == "-"))
    {
        if(ui->lineEdit_y4->text() == "0")
        {
            ui->lineEdit_y4->clear();
            ui->lineEdit_y4->insert(text);
        }
        else
        {
            ui->lineEdit_y4->insert(text);
        }

        //判定输入是否在0-50内
        float area_limit = ui->lineEdit_y4->text().toFloat();
        if((area_limit < 0) ||(area_limit >= 50))
        {
            ui->lineEdit_y4->setText("0");
        }

    }
    uchar which_one = ui->comboBox_whicharea->currentIndex();
    Master_Boundary_Point_Disp[Communication_Machine][which_one][3][1] = (ui->lineEdit_y4->text().toFloat())*10;
}
void systemset::setText_area_y5(const QString &text)
{
    if((text == "0")||(text == "1")||(text == "2")||(text == "3")||(text == "4")||(text == "5")||
            (text == "6")||(text == "7")||(text == "8")||(text == "9")||(text == ".")||(text == "-"))
    {
        if(ui->lineEdit_y5->text() == "0")
        {
            ui->lineEdit_y5->clear();
            ui->lineEdit_y5->insert(text);
        }
        else
        {
            ui->lineEdit_y5->insert(text);
        }

        //判定输入是否在0-50内
        float area_limit = ui->lineEdit_y5->text().toFloat();
        if((area_limit < 0) ||(area_limit >= 50))
        {
            ui->lineEdit_y5->setText("0");
        }

    }
    uchar which_one = ui->comboBox_whicharea->currentIndex();
    Master_Boundary_Point_Disp[Communication_Machine][which_one][4][1] = (ui->lineEdit_y5->text().toFloat())*10;
}
void systemset::setText_area_y6(const QString &text)
{
    if((text == "0")||(text == "1")||(text == "2")||(text == "3")||(text == "4")||(text == "5")||
            (text == "6")||(text == "7")||(text == "8")||(text == "9")||(text == ".")||(text == "-"))
    {
        if(ui->lineEdit_y6->text() == "0")
        {
            ui->lineEdit_y6->clear();
            ui->lineEdit_y6->insert(text);
        }
        else
        {
            ui->lineEdit_y6->insert(text);
        }

        //判定输入是否在0-50内
        float area_limit = ui->lineEdit_y6->text().toFloat();
        if((area_limit < 0) ||(area_limit >= 50))
        {
            ui->lineEdit_y6->setText("0");
        }

    }
    uchar which_one = ui->comboBox_whicharea->currentIndex();
    Master_Boundary_Point_Disp[Communication_Machine][which_one][5][1] = (ui->lineEdit_y6->text().toFloat())*10;
}
void systemset::setText_yuzhi_handinput(const QString &text)
{
    ui->lineEdit_yuzhi_handinput->insert(text);
}
//区域设置->手动设置->退格
void systemset::setBackspace_area_x1()
{
    ui->lineEdit_x1->backspace();
    if(ui->lineEdit_x1->text().isEmpty())
    {
        ui->lineEdit_x1->insert("0");
    }
    uchar which_one = ui->comboBox_whicharea->currentIndex();
    Master_Boundary_Point_Disp[Communication_Machine][which_one][0][0] = 0;
}
void systemset::setBackspace_area_x2()
{
    ui->lineEdit_x2->backspace();
    if(ui->lineEdit_x2->text().isEmpty())
    {
        ui->lineEdit_x2->insert("0");
    }
    uchar which_one = ui->comboBox_whicharea->currentIndex();
    Master_Boundary_Point_Disp[Communication_Machine][which_one][1][0] = 0;
}
void systemset::setBackspace_area_x3()
{
    ui->lineEdit_x3->backspace();
    if(ui->lineEdit_x3->text().isEmpty())
    {
        ui->lineEdit_x3->insert("0");
    }
    uchar which_one = ui->comboBox_whicharea->currentIndex();
    Master_Boundary_Point_Disp[Communication_Machine][which_one][2][0] = 0;
}
void systemset::setBackspace_area_x4()
{
    ui->lineEdit_x4->backspace();
    if(ui->lineEdit_x4->text().isEmpty())
    {
        ui->lineEdit_x4->insert("0");
    }
    uchar which_one = ui->comboBox_whicharea->currentIndex();
    Master_Boundary_Point_Disp[Communication_Machine][which_one][3][0] = 0;
}
void systemset::setBackspace_area_x5()
{
    ui->lineEdit_x5->backspace();
    if(ui->lineEdit_x5->text().isEmpty())
    {
        ui->lineEdit_x5->insert("0");
    }
    uchar which_one = ui->comboBox_whicharea->currentIndex();
    Master_Boundary_Point_Disp[Communication_Machine][which_one][4][0] = 0;
}
void systemset::setBackspace_area_x6()
{
    ui->lineEdit_x6->backspace();
    if(ui->lineEdit_x6->text().isEmpty())
    {
        ui->lineEdit_x6->insert("0");
    }
    uchar which_one = ui->comboBox_whicharea->currentIndex();
    Master_Boundary_Point_Disp[Communication_Machine][which_one][5][0] = 0;
}
void systemset::setBackspace_area_y1()
{
    ui->lineEdit_y1->backspace();
    if(ui->lineEdit_y1->text().isEmpty())
    {
        ui->lineEdit_y1->insert("0");
    }
    uchar which_one = ui->comboBox_whicharea->currentIndex();
    Master_Boundary_Point_Disp[Communication_Machine][which_one][0][1] = 0;
}
void systemset::setBackspace_area_y2()
{
    ui->lineEdit_y2->backspace();
    if(ui->lineEdit_y2->text().isEmpty())
    {
        ui->lineEdit_y2->insert("0");
    }
    uchar which_one = ui->comboBox_whicharea->currentIndex();
    Master_Boundary_Point_Disp[Communication_Machine][which_one][1][1] = 0;
}
void systemset::setBackspace_area_y3()
{
    ui->lineEdit_y3->backspace();
    if(ui->lineEdit_y3->text().isEmpty())
    {
        ui->lineEdit_y3->insert("0");
    }
    uchar which_one = ui->comboBox_whicharea->currentIndex();
    Master_Boundary_Point_Disp[Communication_Machine][which_one][2][1] = 0;
}
void systemset::setBackspace_area_y4()
{
    ui->lineEdit_y4->backspace();

    if(ui->lineEdit_y4->text().isEmpty())
    {
        ui->lineEdit_y4->insert("0");
    }
    uchar which_one = ui->comboBox_whicharea->currentIndex();
    Master_Boundary_Point_Disp[Communication_Machine][which_one][3][1] = 0;
}
void systemset::setBackspace_area_y5()
{
    ui->lineEdit_y5->backspace();
    if(ui->lineEdit_y5->text().isEmpty())
    {
        ui->lineEdit_y5->insert("0");
    }
    uchar which_one = ui->comboBox_whicharea->currentIndex();
    Master_Boundary_Point_Disp[Communication_Machine][which_one][4][1] = 0;
}
void systemset::setBackspace_area_y6()
{
    ui->lineEdit_y6->backspace();
    if(ui->lineEdit_y6->text().isEmpty())
    {
        ui->lineEdit_y6->insert("0");
    }
    uchar which_one = ui->comboBox_whicharea->currentIndex();
    Master_Boundary_Point_Disp[Communication_Machine][which_one][5][1] = 0;
}
void systemset::setBackspace_yuzhi_handinput()
{
    ui->lineEdit_yuzhi_handinput->backspace();
}
//给主窗体发送重绘信号
void systemset::toolButton_areaset_enter_clicked()       //区域设置重绘 与保存
{
    emit mainwindow_repainting();
    Flag_Set_SendMode = 5;
    config_boundary_machine1_area1();
    config_boundary_machine1_area2();
    config_boundary_machine1_area3();
    config_boundary_machine1_area4();
    config_boundary_machine1_area5();
    config_boundary_machine1_area6();
}

void systemset::area_point_disp(unsigned char t)      //区域手动设置 坐标数值显示
{
    QString temp;
    float temp_flo;

    temp_flo = Master_Boundary_Point_Disp[Communication_Machine][t][0][0]/10 + 0.1*(Master_Boundary_Point_Disp[Communication_Machine][t][0][0]%10);
    temp = QString("%1").arg(temp_flo);
    ui->lineEdit_x1->setText(temp);

    temp_flo = Master_Boundary_Point_Disp[Communication_Machine][t][1][0]/10 + 0.1*(Master_Boundary_Point_Disp[Communication_Machine][t][1][0]%10);
    temp = QString("%1").arg(temp_flo);
    ui->lineEdit_x2->setText(temp);

    temp_flo = Master_Boundary_Point_Disp[Communication_Machine][t][2][0]/10 + 0.1*(Master_Boundary_Point_Disp[Communication_Machine][t][2][0]%10);
    temp = QString("%1").arg(temp_flo);
    ui->lineEdit_x3->setText(temp);

    temp_flo = Master_Boundary_Point_Disp[Communication_Machine][t][3][0]/10 + 0.1*(Master_Boundary_Point_Disp[Communication_Machine][t][3][0]%10);
    temp = QString("%1").arg(temp_flo);
    ui->lineEdit_x4->setText(temp);

    temp_flo = Master_Boundary_Point_Disp[Communication_Machine][t][4][0]/10 + 0.1*(Master_Boundary_Point_Disp[Communication_Machine][t][4][0]%10);
    temp = QString("%1").arg(temp_flo);
    ui->lineEdit_x5->setText(temp);

    temp_flo = Master_Boundary_Point_Disp[Communication_Machine][t][5][0]/10 + 0.1*(Master_Boundary_Point_Disp[Communication_Machine][t][5][0]%10);
    temp = QString("%1").arg(temp_flo);
    ui->lineEdit_x6->setText(temp);

    temp_flo = Master_Boundary_Point_Disp[Communication_Machine][t][0][1]/10 + 0.1*(Master_Boundary_Point_Disp[Communication_Machine][t][0][1]%10);
    temp = QString("%1").arg(temp_flo);
    ui->lineEdit_y1->setText(temp);

    temp_flo = Master_Boundary_Point_Disp[Communication_Machine][t][1][1]/10 + 0.1*(Master_Boundary_Point_Disp[Communication_Machine][t][1][1]%10);
    temp = QString("%1").arg(temp_flo);
    ui->lineEdit_y2->setText(temp);

    temp_flo = Master_Boundary_Point_Disp[Communication_Machine][t][2][1]/10 + 0.1*(Master_Boundary_Point_Disp[Communication_Machine][t][2][1]%10);
    temp = QString("%1").arg(temp_flo);
    ui->lineEdit_y3->setText(temp);

    temp_flo = Master_Boundary_Point_Disp[Communication_Machine][t][3][1]/10 + 0.1*(Master_Boundary_Point_Disp[Communication_Machine][t][3][1]%10);
    temp = QString("%1").arg(temp_flo);
    ui->lineEdit_y4->setText(temp);

    temp_flo = Master_Boundary_Point_Disp[Communication_Machine][t][4][1]/10 + 0.1*(Master_Boundary_Point_Disp[Communication_Machine][t][4][1]%10);
    temp = QString("%1").arg(temp_flo);
    ui->lineEdit_y5->setText(temp);

    temp_flo = Master_Boundary_Point_Disp[Communication_Machine][t][5][1]/10 + 0.1*(Master_Boundary_Point_Disp[Communication_Machine][t][5][1]%10);
    temp = QString("%1").arg(temp_flo);
    ui->lineEdit_y6->setText(temp);
}

void systemset::on_comboBox_whicharea_currentIndexChanged(int index)    //区域选择combox
{
    area_point_disp(index);
}

void systemset::on_comboBox_starttime_h_currentIndexChanged(int index)  //起始时间  h
{
    Start_time_h = index;
    config_radar_znwrite();
}

void systemset::on_comboBox_starttime_m_currentIndexChanged(int index)  //起始时间  m
{
     Start_time_m = index;
     config_radar_znwrite();
}

void systemset::on_comboBox_stoptime_h_currentIndexChanged(int index)   //关闭时间  h
{
     Stop_time_h = index;
     config_radar_znwrite();
}

void systemset::on_comboBox_stoptime_m_currentIndexChanged(int index)   //关闭时间  m
{
     Stop_time_m = index;
     config_radar_znwrite();
}

void systemset::on_comboBox_silent_h_currentIndexChanged(int index)     //自动取消静音时间  h
{
    Silent_time_h = index;
    config_radar_znwrite();
}

void systemset::on_comboBox_silent_m_currentIndexChanged(int index)     //自动取消静音时间  m
{
    Silent_time_m = index;
    config_radar_znwrite();
}

void systemset::on_comboBox_warndelay_m_currentIndexChanged(int index)  //报警延长时间    m
{
    Warn_delay_m = index;
    config_radar_znwrite();
}

void systemset::on_comboBox_warndelay_s_currentIndexChanged(int index)  //报警延长时间    s
{
    Warn_delay_s = index;
    config_radar_znwrite();
}

void systemset::on_toolButton_outdor_warnopen_clicked()     //室外报警使能    开
{
    Flag_outdoor_warn = 1;
    ui->toolButton_outdor_warnclo->setEnabled(1);
    ui->toolButton_outdor_warnopen->setEnabled(0);
    config_radar_znwrite();
}

void systemset::on_toolButton_outdor_warnclo_clicked()      //室外报警使能    关
{
    Flag_outdoor_warn = 0;
    ui->toolButton_outdor_warnclo->setEnabled(0);
    ui->toolButton_outdor_warnopen->setEnabled(1);
    config_radar_znwrite();
}

void systemset::on_toolButton_areaopen_1_clicked()      //防区设置    开
{
    Flag_area_ctrl[0] = 1;
    ui->toolButton_areaclo_1->setEnabled(1);
    ui->toolButton_areaopen_1->setEnabled(0);
    config_radar_znwrite();
}

void systemset::on_toolButton_areaopen_2_clicked()      //防区设置    开
{
    Flag_area_ctrl[1] = 1;
    ui->toolButton_areaclo_2->setEnabled(1);
    ui->toolButton_areaopen_2->setEnabled(0);
    config_radar_znwrite();
}

void systemset::on_toolButton_areaopen_3_clicked()      //防区设置    开
{
    Flag_area_ctrl[2] = 1;
    ui->toolButton_areaclo_3->setEnabled(1);
    ui->toolButton_areaopen_3->setEnabled(0);
    config_radar_znwrite();
}

void systemset::on_toolButton_areaopen_4_clicked()      //防区设置    开
{
    Flag_area_ctrl[3] = 1;
    ui->toolButton_areaclo_4->setEnabled(1);
    ui->toolButton_areaopen_4->setEnabled(0);
    config_radar_znwrite();
}

void systemset::on_toolButton_areaclo_1_clicked()       //防区设置      关
{
    Flag_area_ctrl[0] = 0;
    ui->toolButton_areaclo_1->setEnabled(0);
    ui->toolButton_areaopen_1->setEnabled(1);
    config_radar_znwrite();
}
void systemset::on_toolButton_areaclo_2_clicked()       //防区设置      关
{
    Flag_area_ctrl[1] = 0;
    ui->toolButton_areaclo_2->setEnabled(0);
    ui->toolButton_areaopen_2->setEnabled(1);
    config_radar_znwrite();
}
void systemset::on_toolButton_areaclo_3_clicked()       //防区设置      关
{
    Flag_area_ctrl[2] = 0;
    ui->toolButton_areaclo_3->setEnabled(0);
    ui->toolButton_areaopen_3->setEnabled(1);
    config_radar_znwrite();
}

void systemset::on_toolButton_areaclo_4_clicked()       //防区设置      关
{
    Flag_area_ctrl[3] = 0;
    ui->toolButton_areaclo_4->setEnabled(0);
    ui->toolButton_areaopen_4->setEnabled(1);
    config_radar_znwrite();
}

void systemset::on_comboBox_sensitivity_currentIndexChanged(int index)  //灵敏度设置
{
    Flag_sensitivity = index;
    config_radar_znwrite();
    Flag_Set_SendMode = 7;
}

void systemset::paint_yuzhi_collect()
{
    QVector<double> x1(90),y1(90);
    QVector<double> x2(90),y2(90);
    for(uchar i = 0;i<90;i++)
    {
        x1[i] = i;
        y1[i] = ((Master_Back_Groud_Value[Communication_Machine][i][0])) + ((Master_Back_Groud_Value[Communication_Machine][i][1]<<8));
        printf("%f   ",x1[i]);
        printf("%f   \n",y1[i]);
        if(y1[i] > max_value_y)
        {
            max_value_y = y1[i];
        }
    }
    for(uchar i = 90;i<180;i++)
    {
        x2[i-90] = i-90;
        y2[i-90] = ((Master_Back_Groud_Value[Communication_Machine][i][0])) + ((Master_Back_Groud_Value[Communication_Machine][i][1]<<8));
        printf("%f   ",x2[i-90]);
        printf("%f   \n",y2[i-90]);
        if(y2[i-90] > max_value_y)
        {
            max_value_y = y2[i-90];
        }
    }

    QPen pen_red(QColor(255,0,0));
    ui->widget_colletc->graph(1)->setPen(pen_red);

    ui->widget_colletc->yAxis->setRange(0,max_value_y + 300);
    ui->widget_colletc->xAxis->setRange(0,90);

    ui->widget_colletc->graph(0)->setData(x1,y1);   //白
    ui->widget_colletc->graph(1)->clearData();
    if(!flag_hide_red)
    {
        ui->widget_colletc->graph(1)->setData(x2,y2);   //红
    }
    ui->widget_colletc->replot();
}

int systemset::Check_Temp()
{
    if(Get_Back_Flag[0] == 0x38)
    {
        unsigned char err_flag;
        int i;
        int j;

        unsigned char com_sn = 0;   //现在只支持一个，该值固定，修改个数之后，需改为全局变量

        err_flag = 0;
        for(i=0;i<180;i++)
        {
            if (
                (Back_Groud_Temp[com_sn][i][0]==0)&& (Back_Groud_Temp[com_sn][i][1]==0)
                )
                {
                    err_flag = 1;
                }
        }
        err_flag = 0;
        if(err_flag==0)
        {
            for(i=0;i<180;i++)
            {
                for(j=0;j<2;j++)
                {
                    Master_Back_Groud_Value[com_sn][i][j] = Back_Groud_Temp[com_sn][i][j];  //？？？？j = 4？？
                }
            }
        }
        timer_Check_temp->stop();
        Get_Back_Flag[0] = 0x00;
        paint_yuzhi_collect();

        QTextCodec *codec = QTextCodec::codecForName("UTF-8");
        QByteArray a = "采集";
        ui->toolButton_yuzhicollect->setText(codec->toUnicode(a));
        ui->toolButton_yuzhicollect->setEnabled(1);
        return(err_flag);
    }
    return 0;
}

void systemset::on_toolButton_mov_left_clicked()        //阈值设置坐标左移按键
{
    QVector<double> x1(2),y1(2);

    if(flag_mov_x > 0)
    {
        flag_mov_x--;
    }
    else
    {
        flag_mov_x = 0;
    }

    if(Temp_Auto_Backgroud[flag_mov_x][1] ||Temp_Auto_Backgroud[flag_mov_x][0])
    {
        QString temp_line = QString::number((Temp_Auto_Backgroud[flag_mov_x][1]<<8)+(Temp_Auto_Backgroud[flag_mov_x][0]));
        ui->lineEdit_yuzhi_handinput->setText(temp_line);
    }
    else
    {
        ui->lineEdit_yuzhi_handinput->clear();
    }

    x1[0] = flag_mov_x;
    y1[0] = 100000;//(Master_Back_Groud_Value[Communication_Machine][flag_mov_x][0]) + (Back_Groud_Temp[Communication_Machine][flag_mov_x][1]<<8);
    x1[1] = flag_mov_x;
    y1[1] = 0;
    QString temp;
    temp = QString::number(flag_mov_x,10);
    ui->label_x->setText(temp);
    temp = QString::number((Master_Back_Groud_Value[Communication_Machine][flag_mov_x][0]) + (Master_Back_Groud_Value[Communication_Machine][flag_mov_x][1]<<8),10);
    ui->label_y->setText(temp);
    ui->widget_colletc->graph(2)->setData(x1,y1);
    ui->widget_colletc->replot();
}
void systemset::on_toolButton_mov_righ_clicked()        //阈值设置坐标右移按键
{
   QVector<double> x1(2),y1(2);
   if(flag_mov_x < 89)
   {
       flag_mov_x++;
   }
   else
   {
       flag_mov_x = 89;
   }
   if(Temp_Auto_Backgroud[flag_mov_x][1] ||Temp_Auto_Backgroud[flag_mov_x][0])
   {
       QString temp_line = QString::number((Temp_Auto_Backgroud[flag_mov_x][1]<<8)+(Temp_Auto_Backgroud[flag_mov_x][0]));
       ui->lineEdit_yuzhi_handinput->setText(temp_line);
   }
   else
   {
       ui->lineEdit_yuzhi_handinput->clear();
   }

   x1[0] = flag_mov_x;
   y1[0] = 100000;//(Master_Back_Groud_Value[Communication_Machine][flag_mov_x][0]) + (Back_Groud_Temp[Communication_Machine][flag_mov_x][1]<<8);
   x1[1] = flag_mov_x;
   y1[1] = 0;
   QString temp;
   temp = QString::number(flag_mov_x,10);
   ui->label_x->setText(temp);
   temp = QString::number((Master_Back_Groud_Value[Communication_Machine][flag_mov_x][0]) + (Master_Back_Groud_Value[Communication_Machine][flag_mov_x][1]<<8),10);
   ui->label_y->setText(temp);
   ui->widget_colletc->graph(2)->setData(x1,y1);
   ui->widget_colletc->replot();

}

void systemset::on_toolButton_yuzhicollect_clicked()  //阈值采集    “采集”按钮
{
    flag_hide_red = 0;
    Flag_Set_SendMode = 1;

    QTextCodec *codec = QTextCodec::codecForName("UTF-8");
    QByteArray a = "请等待...";
    ui->toolButton_yuzhicollect->setEnabled(0);
    ui->toolButton_yuzhicollect->setText(codec->toUnicode(a));

    timer_Check_temp->start();
}

void systemset::on_toolButton_yuzhi_collect_clicked()   //安全设置。阈值采集
{
    on_toolButton_setyuzhi_refresh_clicked();
    ui->toolButton_yuzhi_doublauto->setStyleSheet("background-color: rgb(0, 170, 255)");
    ui->toolButton_yuzhi_doublhand->setStyleSheet("background-color: rgb(0, 170, 255)");
    ui->toolButton_yuzhi_collect->setStyleSheet("background-color: rgb(189, 223, 255)");

    ui->toolButton_yuzhihecheng->setHidden(1);
    ui->toolButton_yuzhicollect->setHidden(0);
    ui->toolButton_yuzhigengxin->setHidden(1);
}

void systemset::on_toolButton_yuzhi_doublauto_clicked() //双路自动设置
{
    on_toolButton_setyuzhi_refresh_clicked();
    ui->toolButton_yuzhi_doublauto->setStyleSheet("background-color: rgb(189, 223, 255)");
    ui->toolButton_yuzhi_doublhand->setStyleSheet("background-color: rgb(0, 170, 255)");
    ui->toolButton_yuzhi_collect->setStyleSheet("background-color: rgb(0, 170, 255)");

    Flag_Set_SendMode = 2;
    for(unsigned char i = 0;i<90;i++)
    {
        Temp_Auto_Backgroud[i][0] = 0;
        Temp_Auto_Backgroud[i][1] = 0;
    }
    ui->toolButton_yuzhicollect->setHidden(1);
    ui->toolButton_yuzhigengxin->setHidden(1);
    ui->toolButton_yuzhihecheng->setHidden(0);
    timer_doublhand_pointdrw->stop();
    timer_doublauto_linedrw->start();
    ui->widget_colletc->graph(3)->clearData();
    ui->widget_colletc->replot();
}
void systemset::doublanto_linedrw()     //双路自动设置图线绘制
{
    QVector<double> x1(1),y1(1);

    for(unsigned char i = 0;i < 10;i++)
    {
        if((Temp_Auto_Backgroud[Amp_R_Date[i*3]][1]<<8 |Temp_Auto_Backgroud[Amp_R_Date[i*3]][0]) < ((Amp_R_Date[i*3+1]<<8)|Amp_R_Date[i*3+2]))
        {
            Temp_Auto_Backgroud[Amp_R_Date[i*3]][1] = Amp_R_Date[i*3+1];
            Temp_Auto_Backgroud[Amp_R_Date[i*3]][0] = Amp_R_Date[i*3+2];

            x1[0] = Amp_R_Date[i*3];
            y1[0] = ((Amp_R_Date[i*3+1]<<8)|Amp_R_Date[i*3+2]);

            if(max_value_y < y1[0])
            {
                max_value_y = y1[0];
                ui->widget_colletc->yAxis->setRange(0,max_value_y);
            }
            QPen pen_line;
            pen_line.setColor(QColor(128,128,0));

            ui->widget_colletc->graph(4)->setPen(pen_line);
            ui->widget_colletc->graph(4)->addData(x1,y1);
            ui->widget_colletc->replot();
        }
    }

    QPen pen_line;
    pen_line.setColor(QColor(128,128,0));

    ui->widget_colletc->graph(4)->setPen(pen_line);
    ui->widget_colletc->graph(4)->addData(x1,y1);
    ui->widget_colletc->replot();
}

void systemset::on_toolButton_yuzhihecheng_clicked()        //双路自动设置 阈值合成
{
    Flag_Set_yuzhi = 1;
    QTextCodec *codec = QTextCodec::codecForName("UTF-8");
    QByteArray a = "请等待...";
    ui->toolButton_yuzhihecheng->setEnabled(0);
    ui->toolButton_yuzhihecheng->setText(codec->toUnicode(a));
    int i;
    int j;
    float max_b;
    float max_a;
    unsigned char flag_a = 0;
    unsigned char flag_b = 0;
    for(i = 1;i<89;i++)
    {
        max_a = (Temp_Auto_Backgroud[i][1]<<8) + Temp_Auto_Backgroud[i][0];
        for(j = i-1;j>=0;j--)
        {
            if(max_a<((Temp_Auto_Backgroud[j][1]<<8) + Temp_Auto_Backgroud[j][0]))
            {
                max_a = (Temp_Auto_Backgroud[j][1]<<8) + Temp_Auto_Backgroud[j][0];
                flag_a = j;
            }
        }
        max_b = (Temp_Auto_Backgroud[i][1]<<8) + Temp_Auto_Backgroud[i][0];
        for(j = i+1;j<90;j++)
        {
            if(max_b<((Temp_Auto_Backgroud[j][1]<<8) + Temp_Auto_Backgroud[j][0]))
            {
                max_b = (Temp_Auto_Backgroud[j][1]<<8) + Temp_Auto_Backgroud[j][0];
                flag_b = j;
            }
        }

        if(max_a>max_b)
        {
            if(((Temp_Auto_Backgroud[i][1]<<8) + Temp_Auto_Backgroud[i][0])<max_b)
            {
                Temp_Auto_Backgroud[i][1] = Temp_Auto_Backgroud[flag_b][1];
                Temp_Auto_Backgroud[i][0] = Temp_Auto_Backgroud[flag_b][0];
            }
        }
        else
        {
            if(((Temp_Auto_Backgroud[i][1]<<8) + Temp_Auto_Backgroud[i][0])<max_a)
            {
                Temp_Auto_Backgroud[i][1] = Temp_Auto_Backgroud[flag_a][1];
                Temp_Auto_Backgroud[i][0] = Temp_Auto_Backgroud[flag_a][0];
            }
        }
    }
    for(unsigned char i = 0;i < 90;i++)
    {
        if(((Master_Back_Groud_Value[Communication_Machine][i][1]<<8) + Master_Back_Groud_Value[Communication_Machine][i][0]) <
                ((((Temp_Auto_Backgroud[i][1]<<8) + Temp_Auto_Backgroud[i][0])/5)))
        {
            Master_Back_Groud_Value[Communication_Machine][i][1] = ((((Temp_Auto_Backgroud[i][1]<<8)+Temp_Auto_Backgroud[i][0])/5) & 0xff00)>>8;
            Master_Back_Groud_Value[Communication_Machine][i][0] = (((Temp_Auto_Backgroud[i][1]<<8)+Temp_Auto_Backgroud[i][0])/5) & 0xff;
        }
    }
    Flag_Set_SendMode = 4;
}

void systemset::repaint_seted_yuzhi()
{
    ui->lineEdit_yuzhi_handinput->setHidden(1);
    ui->widget_colletc->graph(2)->clearData();

    on_toolButton_setyuzhi_refresh_clicked();
}
void systemset::on_toolButton_yuzhi_doublhand_clicked() //双路手动设置
{
    on_toolButton_setyuzhi_refresh_clicked();

    ui->lineEdit_yuzhi_handinput->setHidden(0);
    ui->lineEdit_yuzhi_handinput->clear();
    ui->toolButton_yuzhi_doublauto->setStyleSheet("background-color: rgb(0, 170, 255)");
    ui->toolButton_yuzhi_doublhand->setStyleSheet("background-color: rgb(189, 223, 255)");
    ui->toolButton_yuzhi_collect->setStyleSheet("background-color: rgb(0, 170, 255)");

    Flag_Set_SendMode = 2;

    for(unsigned char i = 0;i<90;i++)
    {
        Temp_Auto_Backgroud[i][0] = 0;
        Temp_Auto_Backgroud[i][1] = 0;
    }

    ui->toolButton_yuzhicollect->setHidden(1);
    ui->toolButton_yuzhihecheng->setHidden(1);
    ui->toolButton_yuzhigengxin->setHidden(0);
    timer_doublauto_linedrw->stop();
    timer_doublhand_pointdrw->start();
    ui->widget_colletc->graph(4)->clearData();
    ui->widget_colletc->replot();
}

void systemset::on_lineEdit_yuzhi_handinput_textChanged(const QString &arg1)    //双路手动设置，点输入
{
    Temp_Auto_Backgroud[flag_mov_x][1] = (arg1.toInt()&0xff00) >>8;
    Temp_Auto_Backgroud[flag_mov_x][0] = arg1.toInt()&0xff;
}

void systemset::on_toolButton_yuzhigengxin_clicked()        //双路手动设置 阈值更新
{
    Flag_Set_yuzhi = 1;

    QTextCodec *codec = QTextCodec::codecForName("UTF-8");
    QByteArray a = "请等待...";
    ui->toolButton_yuzhigengxin->setEnabled(0);
    ui->toolButton_yuzhigengxin->setText(codec->toUnicode(a));
    for(unsigned char i = 0;i < 90;i++)
    {
        if(Temp_Auto_Backgroud[i][1] || Temp_Auto_Backgroud[i][0])
        {
            Master_Back_Groud_Value[Communication_Machine][i][1] = ((((Temp_Auto_Backgroud[i][1]<<8)+Temp_Auto_Backgroud[i][0])/5) & 0xff00)>>8;
            Master_Back_Groud_Value[Communication_Machine][i][0] = (((Temp_Auto_Backgroud[i][1]<<8)+Temp_Auto_Backgroud[i][0])/5) & 0xff;
        }
    }
    Flag_Set_SendMode = 4;
}

void systemset::doublhand_pointdrw()    //双路手动设置，点绘制
{
    QVector<double> x1(10),y1(10);

    for(unsigned char i = 0;i < 10;i++)
    {
        x1[i] = Amp_R_Date[i*3];
        y1[i] = ((Amp_R_Date[i*3+1]<<8)|Amp_R_Date[i*3+2]);
        if(max_value_y< y1[i])
        {
            max_value_y = y1[i];
        }
    }

    QPen pen_point;
    pen_point.setWidth(3);  //圆圈直径
    pen_point.setColor(QColor(128,128,0));
    ui->widget_colletc->yAxis->setRange(0,max_value_y);
    ui->widget_colletc->xAxis->setRange(0,90);

    ui->widget_colletc->graph(3)->setPen(pen_point);
    ui->widget_colletc->graph(3)->setLineStyle(QCPGraph::lsNone);
    ui->widget_colletc->graph(3)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle,2));
    ui->widget_colletc->graph(3)->setData(x1,y1);

    ui->widget_colletc->replot();
}

void systemset::on_toolButton_areaautoset_clicked()
{
    Get_Are_Flag[Communication_Machine] = 1;
    this->setHidden(1);
    emit button_sysshow();
}

void systemset::systemset_showed()  //取消自动设置区域
{
    if(Flag_autoget_area)
    {
        Flag_Set_SendMode = 5;
        Flag_autoget_area = 0;
    }
    else
    {
        Flag_Set_SendMode = 3;
    }                                           //？？？？？？？
    this->setHidden(0);
}
void systemset::on_tabWidget_radar_currentChanged(int index)    //阈值设置 tab
{
    if(index == 2)
    {
        on_toolButton_setyuzhi_refresh_clicked();
    }
}

void systemset::on_toolButton_setyuzhi_refresh_clicked()    //刷新按钮
{
    flag_hide_red = 1;
    max_value_y = 0;

    paint_yuzhi_collect();

    timer_doublauto_linedrw->stop();
    timer_doublhand_pointdrw->stop();
    ui->widget_colletc->graph(3)->clearData();
    ui->widget_colletc->graph(4)->clearData();
    ui->widget_colletc->replot();

    ui->toolButton_yuzhicollect->setHidden(1);
    ui->toolButton_yuzhigengxin->setHidden(1);
    ui->toolButton_yuzhihecheng->setHidden(1);

    ui->lineEdit_yuzhi_handinput->setHidden(1);

    QTextCodec *codec = QTextCodec::codecForName("UTF-8");
    QByteArray a = "阈值合成";
    ui->toolButton_yuzhihecheng->setEnabled(1);
    ui->toolButton_yuzhihecheng->setText(codec->toUnicode(a));
    a = "采集";
    ui->toolButton_yuzhicollect->setEnabled(1);
    ui->toolButton_yuzhicollect->setText(codec->toUnicode(a));
    a = "阈值更新";
    ui->toolButton_yuzhigengxin->setEnabled(1);
    ui->toolButton_yuzhigengxin->setText(codec->toUnicode(a));

    ui->toolButton_yuzhi_doublauto->setStyleSheet("background-color: rgb(0, 170, 255)");
    ui->toolButton_yuzhi_doublhand->setStyleSheet("background-color: rgb(0, 170, 255)");
    ui->toolButton_yuzhi_collect->setStyleSheet("background-color: rgb(0, 170, 255)");
    Flag_Set_SendMode = 6;
}


//*******************added for radar*********/<-

void systemset::on_pushButton_time_clicked()    //时间设置
{
    QString m_data ;
    QString year ;
    QString month ;
    QString day ;
    QString hour ;
    QString minute ;
    QString second ;

    year = ui->comboBox_year->currentText();
    month = ui->comboBox_month->currentText();
    day = ui->comboBox_day->currentText();
    hour = ui->comboBox_hour->currentText();
    minute = ui->comboBox_min->currentText();
    second = ui->comboBox_sec->currentText();

    m_data = "date -s \""+year+"-"+month+"-"+day+" "+hour+":"+minute+":"+second+"\"";
    QProcess::startDetached(m_data);
    QProcess::startDetached("hwclock -w");
    QProcess::startDetached("sync");
}

void systemset::on_toolButton_setanquan_clicked()
{
    if(Flag_moni_warn)
    {
        Flag_moni_warn = 0;
        ui->toolButton_setanquan->setStyleSheet("background-color: rgb(170, 0, 255)");
        Flag_Sound_Radar[2] = 0;
    }
    else
    {
        Flag_moni_warn = 1;
        ui->toolButton_setanquan->setStyleSheet("background-color: rgb(255, 255, 255)");
        Flag_Sound_Radar[2] = 1;
    }
}
//***设置压力法数据的显示模式，两位小数（调试模式）或者不要小数（正常模式）************//
void systemset::on_pushButton_pre_mode_clicked()   //1调试模式  0正常模式
{
    if(Flag_pre_mode == 1)
    {
        ui->pushButton_pre_mode->setText( QString::fromUtf8("正常模式"));
        Flag_pre_mode = 0;
    }
    else
    {
        ui->pushButton_pre_mode->setText( QString::fromUtf8("调试模式"));
        Flag_pre_mode = 1;
    }
    config_SensorAmountChanged(); //将Flag_pre_mode写入文件config.txt中。
}

//在线油气回收

//加油机数量选择
void systemset::on_comboBox_dispen_sumset_currentIndexChanged(const QString &arg1)
{
    tableview_1_replay(arg1.toInt());
}

//tableview 加油机油枪数量设置
void systemset::tableview_1_replay(int t)
{
    model = new QStandardItemModel();

    model->setColumnCount(2);
    model->setHeaderData(0,Qt::Horizontal,QObject::tr("%1").arg("加油机编号"));
    model->setHeaderData(1,Qt::Horizontal,QObject::tr("%1").arg("油枪数目"));

//    model->removeColumn(0);
    int i = 0;
    while(i < t)
    {
        model->setItem(i,0,new QStandardItem(QString("%1#加油机").arg(i+1)));
        model->item(i,0)->setTextAlignment(Qt::AlignCenter);
        model->setItem(i,1,new QStandardItem(QString("%1").arg(Amount_Gasgun[i])));
        model->item(i,1)->setTextAlignment(Qt::AlignCenter);
        i++;
    }

    ui->tableView_dispenerset->setModel(model);
    ui->tableView_dispenerset->setColumnWidth(0,180);
    ui->tableView_dispenerset->setColumnWidth(1,150);
    on_toolButton_gunamount_enter_clicked();
}
void systemset::tableview_2_replay()//油量校准因子显示
{
    model_gundetail = new QStandardItemModel();

    model_gundetail->setColumnCount(3);
    model_gundetail->setHeaderData(0,Qt::Horizontal,QObject::tr("%1").arg("油枪编号"));
    model_gundetail->setHeaderData(1,Qt::Horizontal,QObject::tr("%1").arg("油量脉冲当量"));
    model_gundetail->setHeaderData(2,Qt::Horizontal,QObject::tr("%1").arg("气量脉冲当量"));

    int i = 0;
    unsigned char i_al = 0;
    while(i < Amount_Dispener)
    {
        for(unsigned char j = 0;j < Amount_Gasgun[i];j++)
        {
            model_gundetail->setItem(i_al,0,new QStandardItem(QString("%1-%2#油枪").arg(i+1).arg(j+1))); //1-1油枪
            model_gundetail->item(i_al,0)->setTextAlignment(Qt::AlignCenter);
            model_gundetail->setItem(i_al,1,new QStandardItem(QString("%1").arg(Fueling_Factor[i][j]))); //油量校正因子
            model_gundetail->item(i_al,1)->setTextAlignment(Qt::AlignCenter);
            model_gundetail->setItem(i_al,2,new QStandardItem(QString("%1").arg(Gas_Factor[i][j]))); //气量校正因子
            model_gundetail->item(i_al,2)->setTextAlignment(Qt::AlignCenter);
            i_al++;
        }
        i++;
    }
    ui->tableView_gundetailset->setModel(model_gundetail);
    ui->tableView_gundetailset->setColumnWidth(0,150);
    ui->tableView_gundetailset->setColumnWidth(1,150);
    ui->tableView_gundetailset->setColumnWidth(2,150);
}
void systemset::on_tableView_dispenerset_clicked(const QModelIndex &index)
{
    //获取是第几行第几列
    hang = index.row();
    lie = index.column();
    emit closeing_touchkey();
    if(lie == 1)
    {
        touchkey = new keyboard;
        connect(touchkey->signalMapper,SIGNAL(mapped(const QString&)),this,SLOT(set_amount_gun(const QString&)));

        connect(this,SIGNAL(closeing_touchkey()),touchkey,SLOT(onEnter()));
        touchkey->show();
    }
}

void systemset::set_amount_gun(const QString &text)
{
    std::string temp = text.toStdString();
    if((temp[0] >= '0') && (temp[0] < '9'))
    {
        printf("%d\n",text.toInt());
        model->setItem(hang,lie,new QStandardItem(text));
        model->item(hang,lie)->setTextAlignment(Qt::AlignCenter);
        this->clearFocus();
        on_toolButton_gunamount_enter_clicked();
    }
}

void systemset::on_toolButton_gunamount_enter_clicked()
{
    QModelIndex index;
    QVariant data;
    for(unsigned char i = 0;i < 12;i++)
    {
        index = model->index(i,1);
        data = model->data(index);
        Amount_Gasgun[i] = data.toInt();
        qDebug()<<data.toString();
    }
    Amount_Dispener = ui->comboBox_dispen_sumset->currentIndex();
    qDebug() <<"!!!!!!!dispener is " << Amount_Dispener;
    //重绘显示界面
    emit amount_oilgas_dispen_set(Amount_Dispener);
    emit amount_oilgas_gun_set();
    //写入配置文本
    config_reoilgas_write();
    tableview_2_replay();
    tableView_yingshe_replay();
}
//油气量校正因子设置
void systemset::on_tableView_gundetailset_clicked(const QModelIndex &index)
{
    hang_gun = index.row();
    lie_gun = index.column();
    emit closeing_touchkey();
    if((lie_gun == 1) || (lie_gun == 2))
    {
        touchkey = new keyboard;
        connect(touchkey->signalMapper,SIGNAL(mapped(const QString&)),this,SLOT(set_detailof_gun(const QString&)));
        connect(touchkey,SIGNAL(display_backspace()),this,SLOT(set_detailof_gun_backspace()));
        connect(this,SIGNAL(closeing_touchkey()),touchkey,SLOT(onEnter()));
        touchkey->show();
    }
}
void systemset::set_detailof_gun(const QString &text)
{
    //判断是否是合法输入：数字
    std::string temp = text.toStdString();
    if(((temp[0] >= '0') && (temp[0] <= '9'))||(text == "."))
    {
        //获取当前单元格中的数字
        QModelIndex index = model_gundetail->index(hang_gun,lie_gun);
        QVariant data = model_gundetail->data(index);
        QString Q_temp = data.toString();

        if(Q_temp == "0")
        {
            if(text == ".")
            {
                Q_temp = Q_temp.append(text);
                model_gundetail->setItem(hang_gun,lie_gun,new QStandardItem(Q_temp));
                model_gundetail->item(hang_gun,lie_gun)->setTextAlignment(Qt::AlignCenter);
                this->clearFocus();
            }
            else
            {
                Q_temp = text;
                model_gundetail->setItem(hang_gun,lie_gun,new QStandardItem(Q_temp));
                model_gundetail->item(hang_gun,lie_gun)->setTextAlignment(Qt::AlignCenter);
                this->clearFocus();
            }
        }
        else
        {
            unsigned char flag_dot = 9;
            for(unsigned char i = 0;i<5;i++)
            {
                if(Q_temp[i] == '.')
                {
                    flag_dot = i;
                }
            }
            if(flag_dot == 9)
            {
                Q_temp = Q_temp.append(text);
                int a = Q_temp.toInt();
                if(a < 100)
                {
                    model_gundetail->setItem(hang_gun,lie_gun,new QStandardItem(Q_temp));
                    model_gundetail->item(hang_gun,lie_gun)->setTextAlignment(Qt::AlignCenter);
                    this->clearFocus();
                }
            }
            else
            {
                if(Q_temp[flag_dot+1].isNull())
                {
                    Q_temp.append(text);
                    model_gundetail->setItem(hang_gun,lie_gun,new QStandardItem(Q_temp));
                    model_gundetail->item(hang_gun,lie_gun)->setTextAlignment(Qt::AlignCenter);
                    this->clearFocus();
                }
                else
                {
                    if(Q_temp[flag_dot+2].isNull())
                    {
                        Q_temp.append(text);
                        model_gundetail->setItem(hang_gun,lie_gun,new QStandardItem(Q_temp));
                        model_gundetail->item(hang_gun,lie_gun)->setTextAlignment(Qt::AlignCenter);
                        this->clearFocus();
                    }
                }
            }

 /*
            int a = Q_temp.toInt();
            if(a<100)
            {
                unsigned char flag_dot = 9;
                for(unsigned char i = 0;i < 5;i++)
                {
                    if(Q_temp[i] == '.')
                    {
                        flag_dot = i;
                    }
                }
                Q_temp = Q_temp.append(text);
                qDebug()<<"----------------"+Q_temp[0]+Q_temp[1]+Q_temp[2]+Q_temp[3]+Q_temp[4]+Q_temp[5]<<endl;

                model_gundetail->setItem(hang_gun,lie_gun,new QStandardItem(Q_temp));
                model_gundetail->item(hang_gun,lie_gun)->setTextAlignment(Qt::AlignCenter);
                this->clearFocus();
            }
 */
        }
    }
}
void systemset::set_detailof_gun_backspace()
{
    model_gundetail->setItem(hang_gun,lie_gun,new QStandardItem("0"));
    model_gundetail->item(hang_gun,lie_gun)->setTextAlignment(Qt::AlignCenter);
}

void systemset::on_toolButton_gundetail_enter_clicked()
{
    if((Amount_Dispener>0) && (Amount_Gasgun[Amount_Dispener-1]==0))
    {
        ui->label_reoilgas_count_dispen->setHidden(0);
    }
    else
    {
        ui->label_reoilgas_count_dispen->setHidden(1);
        ui->toolButton_gundetail_enter->setEnabled(0);
        ui->label_reoilgas_factorset_state->setText(QString("%1").arg("设置中，请等待..."));
        ui->label_reoilgas_factorset_state->setHidden(0);
        QModelIndex index;
        QVariant data;
        unsigned char count_temp = 0;
        for(unsigned char i = 0;i < 12;i++)
        {
            for(unsigned char j = 0;j < Amount_Gasgun[i];j++)
            {
                index = model_gundetail->index(count_temp,1);    //oil
                data = model_gundetail->data(index);
                Fueling_Factor[i][j] = data.toFloat();

                index = model_gundetail->index(count_temp,2);    //gas
                data = model_gundetail->data(index);
                Gas_Factor[i][j] = data.toFloat();
                count_temp++;
            }
        }

        config_reoilgas_gasdetail_write();
        config_reoilgas_oildetail_write();

        Lock_Mode_Reoilgas.lock();
        Flag_SendMode_Oilgas = 4;
        Lock_Mode_Reoilgas.unlock();
    }
}

void systemset::on_toolButton_reoilgas_versionask_clicked()
{
    Lock_Mode_Reoilgas.lock();

    Reoilgas_Version_Set_Whichone = ui->comboBox_dispen_x_select->currentIndex();
    qDebug()<<ui->comboBox_dispen_x_select->currentText();
    printf("--%d\n",Reoilgas_Version_Set_Whichone);

    Flag_SendMode_Oilgas = 2;
    Lock_Mode_Reoilgas.unlock();
    ui->label_reoilgas_factorset_state->setHidden(1);
    ui->toolButton_gundetail_enter->setEnabled(1);
}

void systemset::Version_Recv_FromMainwindow(unsigned char high, unsigned char low)
{
    ui->label_reoilgas_versiondisp->setText(QString("%1.%2").arg(high).arg(low));
}

void systemset::on_toolButton_reoilgas_setask_clicked()
{
    Lock_Mode_Reoilgas.lock();

    Reoilgas_Version_Set_Whichone = ui->comboBox_dispen_x_select->currentIndex();
    qDebug()<<ui->comboBox_dispen_x_select->currentText();
    printf("--%d\n",Reoilgas_Version_Set_Whichone);

    Flag_SendMode_Oilgas = 5;
    Lock_Mode_Reoilgas.unlock();
}

void systemset::Setinfo_Recv_FromMainwindow(unsigned char a, unsigned char b, unsigned char c, unsigned char d, unsigned char e, unsigned char f, unsigned char g, unsigned char h, unsigned char i, unsigned char j)
{
    ui->label_reoilgas_setdisp->setText(QString("%1.%2-%3.%4-%5  %6.%7-%8.%9-%10").arg(a).arg(b).arg(c).arg(d).arg(e).arg(f).arg(g).arg(h).arg(i).arg(j));
}

void systemset::setText_pv(const QString &text) //pv正压开启压力设置
{
    ui->lineEdit_pv->insert(text);
}
void systemset::setBackspace_pv()
{
    ui->lineEdit_pv->backspace();
}

void systemset::on_toolButton_pv_clicked()
{
    //qlock
    Positive_Pres = ui->lineEdit_pv->text().toFloat();
    config_pv_positive_write();
}
void systemset::setText_pv_nega(const QString &text) //pv负压开启压力设置
{
    ui->lineEdit_pv_nega->insert(text);
}
void systemset::setBackspace_pv_nega()
{
    ui->lineEdit_pv_nega->backspace();
}

void systemset::on_toolButton_pv_nega_clicked()
{
    //qlock
    float temp = ui->lineEdit_pv_nega->text().toFloat();
    if(temp>0)
    {
        temp = -temp;
    }
    else
    {
        temp = temp;
    }
    Negative_Pres = temp;
    config_pv_negative_write();
}
void systemset::Reoilgas_FactorUartError_Setted(unsigned char state,unsigned int whichone)
{
    whichone = whichone+1;
    int dis = 0;
    int gun = 0;
    if((whichone>=1)&&(whichone<=4))
    {
        dis = 1;
        gun = whichone;
    }
    if((whichone>=5)&&(whichone<=8))
    {
        dis = 2;
        gun = whichone%4;
        if(whichone==8)
        {
            gun = 4;
        }
    }
    if((whichone>=9)&&(whichone<=12))
    {
        dis = 3;
        gun = whichone%8;
    }
    if((whichone>=13)&&(whichone<=16))
    {
        dis = 4;
        gun = whichone%12;
    }
    if((whichone>=17)&&(whichone<=20))
    {
        dis = 5;
        gun = whichone%16;
    }
    if((whichone>=21)&&(whichone<=24))
    {
        dis = 6;
        gun = whichone%20;
    }
    if((whichone>=25)&&(whichone<=28))
    {
        dis = 7;
        gun = whichone%24;
    }
    if((whichone>=29)&&(whichone<=32))
    {
        dis = 8;
        gun = whichone%28;
    }
    if((whichone>=33)&&(whichone<=36))
    {
        dis = 9;
        gun = whichone%32;
    }
    if((whichone>=37)&&(whichone<=40))
    {
        dis = 10;
        gun = whichone%36;
    }
    if((whichone>=41)&&(whichone<=44))
    {
        dis = 11;
        gun = whichone%40;
    }
    if((whichone>=45)&&(whichone<=48))
    {
        dis = 12;
        gun = whichone%44;
    }

    if(state)
    {
        if((dis == 0)&&(gun == 0))
        {
            ui->label_reoilgas_factorset_state->setText(QString("%1").arg("设置中，请等待..."));
        }
        else
        {
            ui->label_reoilgas_factorset_state->setText(QString::number(dis).append("-").append(QString::number(gun)).append(" 设置失败，请确认设备通信正常"));
        }
    }
    else
    {
        if((dis == 0)&&(gun == 0))
        {
            ui->label_reoilgas_factorset_state->setText(QString("%1").arg("设置中，请等待..."));
        }
        else
        {
            ui->label_reoilgas_factorset_state->setText(QString::number(dis).append("-").append(QString::number(gun)).append(" 设置中，请等待..."));
        }
    }
}
void systemset::on_comboBox_far_dispener_currentIndexChanged(int index)
{
    if(index>Amount_Dispener)//大于设置的加油机数量
    {
        ui->label_far->setHidden(0);
        //错误
    }
    else
    {
        ui->label_far->setHidden(1);
        Far_Dispener = index;
        Speed_fargas = 0;  //重置油气流速
        config_Liquid_resistance();//写入记录
        printf("****************************%d\n",Far_Dispener);
    }

}
void systemset::on_toolButton_gun_off_kaiqi_clicked()
{
    Flag_Gun_off = 1;
    ui->toolButton_gun_off_guanbi->setEnabled(1);
    ui->toolButton_gun_off_guanbi->setText("点击关闭");
    ui->toolButton_gun_off_kaiqi->setEnabled(0);
    ui->toolButton_gun_off_kaiqi->setText("已开启");
    config_alset();
}

void systemset::on_toolButton_gun_off_guanbi_clicked()
{
    Flag_Gun_off = 0;
    ui->toolButton_gun_off_guanbi->setEnabled(0);
    ui->toolButton_gun_off_guanbi->setText("已关闭");
    ui->toolButton_gun_off_kaiqi->setEnabled(1);
    ui->toolButton_gun_off_kaiqi->setText("点击开启");
    config_alset();
}
void systemset::on_tableView_yingshe_clicked(const QModelIndex &index)//映射表
{
    //获取是第几行第几列
    hang_mapping = index.row();
    lie_mapping = index.column();
    emit closeing_touchkey();
	if(lie_mapping == 1 || lie_mapping == 2)
    {
        touchkey = new keyboard;
        connect(touchkey->signalMapper,SIGNAL(mapped(const QString&)),this,SLOT(set_yingshe_gun(const QString&)));
        connect(touchkey,SIGNAL(display_backspace()),this,SLOT(set_yingshe_gun_backspace()));
        connect(this,SIGNAL(closeing_touchkey()),touchkey,SLOT(onEnter()));
        touchkey->show();
    }
}
void systemset::tableView_yingshe_replay()//映射表绘制
{
    model_yingshe = new QStandardItemModel();

	model_yingshe->setColumnCount(3);
    model_yingshe->setHeaderData(0,Qt::Horizontal,QObject::tr("%1").arg("本地油枪编号"));
	model_yingshe->setHeaderData(1,Qt::Horizontal,QObject::tr("%1").arg("上传油枪编号"));
	model_yingshe->setHeaderData(2,Qt::Horizontal,QObject::tr("%1").arg("显示油枪编号"));

//    model->removeColumn(0);
    int i = 0;
    unsigned char i_al = 0;
    while(i < Amount_Dispener)
    {
        for(unsigned char j = 0;j < Amount_Gasgun[i];j++)
        {
            model_yingshe->setItem(i_al,0,new QStandardItem(QString("%1-%2").arg(i+1).arg(j+1))); //1-1油枪
            model_yingshe->item(i_al,0)->setTextAlignment(Qt::AlignCenter);
            model_yingshe->setItem(i_al,1,new QStandardItem(QString("%1").arg(Mapping[i*8+j]))); //映射后的加油站油枪编号
            model_yingshe->item(i_al,1)->setTextAlignment(Qt::AlignCenter);
			model_yingshe->setItem(i_al,2,new QStandardItem(Mapping_Show[i*8+j])); //映射后的加油站油枪编号
			model_yingshe->item(i_al,2)->setTextAlignment(Qt::AlignCenter);
            i_al++;
        }
        i++;
    }

    ui->tableView_yingshe->setModel(model_yingshe);
	ui->tableView_yingshe->setColumnWidth(0,110);
	ui->tableView_yingshe->setColumnWidth(1,110);
	ui->tableView_yingshe->setColumnWidth(2,110);
    //on_toolButton_gunamount_enter_clicked();
}
void systemset::set_yingshe_gun(const QString &text)
{
    //获取当前单元格中的数字
    QModelIndex index = model_yingshe->index(hang_mapping,lie_mapping);
    QVariant data = model_yingshe->data(index);
    QString Q_temp = data.toString();

	if(lie_mapping == 1)
	{
		if(Q_temp == "0")
		{
			if(text == ".")
			{
				Q_temp = Q_temp.append(text);
				model_yingshe->setItem(hang_mapping,lie_mapping,new QStandardItem(Q_temp));
				model_yingshe->item(hang_mapping,lie_mapping)->setTextAlignment(Qt::AlignCenter);
				this->clearFocus();
			}
			else
			{
				Q_temp = text;
				model_yingshe->setItem(hang_mapping,lie_mapping,new QStandardItem(Q_temp));
				model_yingshe->item(hang_mapping,lie_mapping)->setTextAlignment(Qt::AlignCenter);
				this->clearFocus();
			}
		}
		else
		{
			unsigned char flag_dot = 9;
			for(unsigned char i = 0;i<5;i++)
			{
				if(Q_temp[i] == '.')
				{
					flag_dot = i;
				}
			}
			if(flag_dot == 9)
			{
				Q_temp = Q_temp.append(text);
				int a = Q_temp.toInt();
				if(a < 100)
				{
					model_yingshe->setItem(hang_mapping,lie_mapping,new QStandardItem(Q_temp));
					model_yingshe->item(hang_mapping,lie_mapping)->setTextAlignment(Qt::AlignCenter);
					this->clearFocus();
				}
			}
			else
			{
				if(Q_temp[flag_dot+1].isNull())
				{
					Q_temp.append(text);
					model_yingshe->setItem(hang_mapping,lie_mapping,new QStandardItem(Q_temp));
					model_yingshe->item(hang_mapping,lie_mapping)->setTextAlignment(Qt::AlignCenter);
					this->clearFocus();
				}
				else
				{
					if(Q_temp[flag_dot+2].isNull())
					{
						Q_temp.append(text);
						model_yingshe->setItem(hang_mapping,lie_mapping,new QStandardItem(Q_temp));
						model_yingshe->item(hang_mapping,lie_mapping)->setTextAlignment(Qt::AlignCenter);
						this->clearFocus();
					}
				}
			}
		}
	}
	if(lie_mapping == 2)
	{
		if(Q_temp == "0")
		{
			if(text == "." || text == "-")
			{
				Q_temp = Q_temp.append(text);
				model_yingshe->setItem(hang_mapping,lie_mapping,new QStandardItem(Q_temp));
				model_yingshe->item(hang_mapping,lie_mapping)->setTextAlignment(Qt::AlignCenter);
				this->clearFocus();
			}
			else
			{
				Q_temp = text;
				model_yingshe->setItem(hang_mapping,lie_mapping,new QStandardItem(Q_temp));
				model_yingshe->item(hang_mapping,lie_mapping)->setTextAlignment(Qt::AlignCenter);
				this->clearFocus();
			}
		}
		else
		{
			Q_temp.append(text);
			model_yingshe->setItem(hang_mapping,lie_mapping,new QStandardItem(Q_temp));
			model_yingshe->item(hang_mapping,lie_mapping)->setTextAlignment(Qt::AlignCenter);
			this->clearFocus();
		}
	}

}


void systemset::on_pushButtonsave_yingshe_copy_clicked()
{
	unsigned int hang = 0;
	for(unsigned char i = 0;i < Amount_Dispener;i++)
	{
		for(unsigned char j = 0;j < Amount_Gasgun[i];j++)
		{
			//获取当前单元格中的数字
			QModelIndex index = model_yingshe->index(hang,1);
			QVariant data = model_yingshe->data(index);
			QString Q_temp = data.toString();

			model_yingshe->setItem(hang,2,new QStandardItem(Q_temp)); //映射后的加油站油枪编号
			model_yingshe->item(hang,2)->setTextAlignment(Qt::AlignCenter);
			hang++;
		}
	}
}

void systemset::on_pushButtonsave_yingshe_clicked()
{
    QModelIndex index;
	QModelIndex index_show;
    QVariant data;
	QVariant data_show;
    unsigned char count_temp = 0;
    for(unsigned char i = 0;i < 12;i++)
    {
        for(unsigned char j = 0;j < Amount_Gasgun[i];j++)
        {
            index = model_yingshe->index(count_temp,1);    //oil
            data = model_yingshe->data(index);
            Mapping[i*8+j] = data.toInt();

			index_show = model_yingshe->index(count_temp,2);    //oil
			data_show = model_yingshe->data(index_show);
			Mapping_Show[i*8+j] = data_show.toString();

			count_temp++;
        }
        for(unsigned char j = Amount_Gasgun[i];j < 8;j++)
        {
            Mapping[i*8+j] = 0;
			Mapping_Show[i*8+j] = "";
        }
    }
    emit amount_oilgas_gun_set();//重绘油枪
    config_mapping_write();//映射表记录在文本中。
    for(int i = 0;i<96;i++)
    {
        //printf("* %d*",Mapping[i]);
    }
    printf("\n");

}

void systemset::on_comboBox_al_low_activated(const QString &arg1)
{
    //qDebug()<< arg1;
    NormalAL_Low = arg1.toFloat();
    config_alset();
}

void systemset::on_comboBox_al_high_activated(const QString &arg1)
{
    NormalAL_High = arg1.toFloat();
    config_alset();
}

void systemset::on_comboBox_alwarndays_activated(const QString &arg1)
{
    WarnAL_Days = arg1.toInt();
    config_alset();
}


void systemset::Reoilgas_Factor_Setover()
{
    ui->label_reoilgas_factorset_state->setHidden(1);
    ui->toolButton_gundetail_enter->setEnabled(1);
}
//可燃气体
void systemset::on_comboBox_burngas_currentIndexChanged(const QString &arg1)
{
    emit amount_burngas_set(arg1.toInt());
    Num_Fga = arg1.toInt()+2;
    add_value_fgainfo("设置数量",arg1);
    config_num_fga();
}
//通讯位于可燃气体，但是设置属于油气回收
//油气回收油罐压力开启
void systemset::on_toolButton_reoilgas_tankpre_open_clicked()
{
    Pre_tank_en = 1;
    ui->toolButton_reoilgas_tankpre_open->setEnabled(0);
    ui->toolButton_reoilgas_tankpre_open->setText("已开启");
    ui->toolButton_reoilgas_tankpre_close->setEnabled(1);
    ui->toolButton_reoilgas_tankpre_close->setText("点击关闭");
    config_reoilgas_pre_en_write();
}
//油气回收油罐压力关闭
void systemset::on_toolButton_reoilgas_tankpre_close_clicked()
{
    Pre_tank_en = 0;
    ui->toolButton_reoilgas_tankpre_open->setEnabled(1);
    ui->toolButton_reoilgas_tankpre_open->setText("点击开启");
    ui->toolButton_reoilgas_tankpre_close->setEnabled(0);
    ui->toolButton_reoilgas_tankpre_close->setText("已关闭");
    emit Pre_Tank_Close();
    config_reoilgas_pre_en_write();
}
//油气回收管线压力开启
void systemset::on_toolButton_reoilgas_pipepre_open_clicked()
{
    Pre_pipe_en = 1;
    ui->toolButton_reoilgas_pipepre_open->setEnabled(0);
    ui->toolButton_reoilgas_pipepre_open->setText("已开启");
    ui->toolButton_reoilgas_pipepre_close->setEnabled(1);
    ui->toolButton_reoilgas_pipepre_close->setText("点击关闭");
    config_reoilgas_pre_en_write();
}
//油气回收管线压力关闭
void systemset::on_toolButton_reoilgas_pipepre_close_clicked()
{
    Pre_pipe_en = 0;
    ui->toolButton_reoilgas_pipepre_open->setEnabled(1);
	ui->toolButton_reoilgas_pipepre_open->setText("点击开启");
    ui->toolButton_reoilgas_pipepre_close->setEnabled(0);
    ui->toolButton_reoilgas_pipepre_close->setText("已开启");
    emit Pre_Pipe_Close();
    config_reoilgas_pre_en_write();
}
//油气回收气体浓度开启
void systemset::on_toolButton_gas1_open_clicked()
{
    Env_Gas_en = 1;
    ui->toolButton_gas1_open->setEnabled(0);
    ui->toolButton_gas1_open->setText("已开启");
    ui->toolButton_gas1_close->setEnabled(1);
    ui->toolButton_gas1_close->setText("点击关闭");
    config_reoilgas_pre_en_write();
}
//油气回收气体浓度关闭
void systemset::on_toolButton_gas1_close_clicked()
{
    Env_Gas_en = 0;
    emit Fga_Gas_Close();
    ui->toolButton_gas1_close->setEnabled(0);
    ui->toolButton_gas1_open->setText("点击开启");
    ui->toolButton_gas1_open->setEnabled(1);
    ui->toolButton_gas1_close->setText("已关闭");
    config_reoilgas_pre_en_write();
}
//油气回收气体温度开启
void systemset::on_toolButton_reoilgas_tem_open_clicked()
{
    Tem_tank_en = 1;
    ui->toolButton_reoilgas_tem_open->setEnabled(0);
    ui->toolButton_reoilgas_tem_open->setText("已开启");
    ui->toolButton_reoilgas_tem_close->setEnabled(1);
    ui->toolButton_reoilgas_tem_close->setText("点击关闭");
    config_reoilgas_pre_en_write();
}
//油气回收气体温度关闭
void systemset::on_toolButton_reoilgas_tem_close_clicked()
{
    Tem_tank_en = 0;
    emit Tem_Tank_Close();
    ui->toolButton_reoilgas_tem_open->setEnabled(1);
    ui->toolButton_reoilgas_tem_open->setText("点击开启");
    ui->toolButton_reoilgas_tem_close->setEnabled(0);
    ui->toolButton_reoilgas_tem_close->setText("已关闭");
    config_reoilgas_pre_en_write();
}


void systemset::on_toolButton_kaiqi_clicked()   //人体静电开启
{
    add_value_jingdianinfo("设备开启");
    Flag_xieyou = 1;
    ui->toolButton_kaiqi->setEnabled(0);
    ui->toolButton_guanbi->setEnabled(1);
    config_jingdian_write();
    emit amount_safe_reset();
}

void systemset::on_toolButton_guanbi_clicked()  //人体静电关闭
{
    Flag_xieyou = 0;
    add_value_jingdianinfo("设备关闭");
    ui->toolButton_kaiqi->setEnabled(1);
    ui->toolButton_guanbi->setEnabled(0);
    config_jingdian_write();
    emit amount_safe_reset();
}

void systemset::on_toolButton_kaiqi_IIE_clicked() //IIE开启
{
    Flag_IIE = 1;
    add_value_IIE("设备开启");
    ui->toolButton_kaiqi_IIE->setEnabled(0);
    ui->toolButton_guanbi_IIE->setEnabled(1);
    config_IIE_write();
    emit amount_safe_reset();
}

void systemset::on_toolButton_guanbi_IIE_clicked()//IIE关闭
{
    add_value_IIE("设备关闭");
    Flag_IIE = 0;
    ui->toolButton_kaiqi_IIE->setEnabled(1);
    ui->toolButton_guanbi_IIE->setEnabled(0);
    config_IIE_write();
    emit amount_safe_reset();
}

void systemset::on_pushButton_allclean_clicked()
{
    ui->widget_warn_allclean->setHidden(0);
}

void systemset::on_toolButton_warn_all_clean_queren_clicked()
{
    ui->widget_warn_allclean->setHidden(1);
    all_sta_clean();
}

void systemset::on_toolButton_warn_allclean_quxiao_clicked()
{
    ui->widget_warn_allclean->setHidden(1);
}

void systemset::on_toolButton_kaiqi_yewei_clicked()
{
    Flag_Enable_liqiud = 1;
    config_security();
    add_value_liquid("设备监测开启");
    ui->toolButton_kaiqi_yewei->setEnabled(0);
    ui->toolButton_guanbi_yewei->setEnabled(1);
}

void systemset::on_toolButton_guanbi_yewei_clicked()
{
    Flag_Enable_liqiud = 0;
    config_security();
    ui->toolButton_kaiqi_yewei->setEnabled(1);
    ui->toolButton_guanbi_yewei->setEnabled(0);
}


void systemset::on_toolButton_kaiqi_beng_clicked()
{
    Flag_Enable_pump = 1;
    config_security();
    add_value_pump("设备监测开启");
    ui->toolButton_kaiqi_beng->setEnabled(0);
    ui->toolButton_guanbi_beng->setEnabled(1);
}

void systemset::on_toolButton_guanbi_beng_clicked()
{
    Flag_Enable_pump = 0;
    config_security();
    ui->toolButton_kaiqi_beng->setEnabled(1);
    ui->toolButton_guanbi_beng->setEnabled(0);
}

void systemset::on_toolButton_warn_ip_wrong_enter_clicked()
{
    ui->widget_warn_ip_wrong->setHidden(1);
}

void systemset::on_toolButton_warn_ip_wrong_cancl_clicked()
{
    ui->widget_warn_ip_wrong->setHidden(1);
}

void systemset::set_yingshe_gun_backspace()
{
    model_yingshe->setItem(hang_mapping,lie_mapping,new QStandardItem("0"));
    model_yingshe->item(hang_mapping,lie_mapping)->setTextAlignment(Qt::AlignCenter);
}

void systemset::on_comboBox_num_cc_currentIndexChanged(int index)//防撞柱数量设置
{
    Num_Crash_Column = index;
}

void systemset::on_pushButton_enter_cc_clicked()//防撞柱数量确认
{
    config_security();
    emit crash_num_reset();
}


void systemset::on_comboBox_burngas_currentIndexChanged(int index)
{
	index = index;
}

//post添加
void systemset::on_toolButton_postdataid_clicked()
{
	Post_Address = ui->lineEdit_postaddress->text();
	USERID_POST = ui->lineEdit_postuserid->text();
	QString ver = "V";
	QString verdata = ui->lineEdit_postversion->text();
	VERSION_POST = ver.append(verdata);
    DATAID_POST = ui->lineEdit_postdataid->text();

	POSTPASSWORD_HUNAN = ui->lineEdit_postpassword->text();
	POSTUSERNAME_HUNAN = ui->lineEdit_postusername->text();
    config_post_network();
}

void systemset::on_pushButton_shield_network_clicked()
{
    if(Flag_Shield_Network == 1)
    {
        Flag_Shield_Network = 0;
        ui->pushButton_shield_network->setText("点击屏蔽");
    }
    else if(Flag_Shield_Network == 0)
    {
        Flag_Shield_Network = 1;
        ui->pushButton_shield_network->setText("已屏蔽");
    }
    qDebug()<< Flag_Shield_Network;
    config_network_Version_write();//油气回收网络上传版本写入,屏蔽状态写入
}

void systemset::on_toolButton_post_kaiqi_clicked()
{
    Flag_Postsend_Enable = 1;
    ui->toolButton_post_guanbi->setEnabled(1);
    ui->toolButton_post_kaiqi->setEnabled(0);
    system("touch /opt/reoilgas/Post_Enable");
    system("sync");
    qDebug() << "Postsend_Enable is "<< Flag_Postsend_Enable;
}

void systemset::on_toolButton_post_guanbi_clicked()
{
    Flag_Postsend_Enable = 0;
    ui->toolButton_post_guanbi->setEnabled(0);
    ui->toolButton_post_kaiqi->setEnabled(1);
    system("rm -r /opt/reoilgas/Post_Enable");
    system("sync");
    qDebug() << "Postsend_Enable is "<< Flag_Postsend_Enable;
}

void systemset::on_toolButton_ifisport_tcp_clicked()
{
	PORT_UDP = ui->lineEdit_ifisport_udp->text().toInt();
    PORT_TCP = ui->lineEdit_ifisport_tcp->text().toInt();
    //config_post_network();//改到泄漏相关设置
    config_xielou_network();//泄漏网络上传相关参数写入
}

void systemset::on_toolButton_screen_xielou_clicked()
{
    if(Flag_screen_xielou == 1)
    {
        Flag_screen_xielou = 0;
        //emit hide_tablewidget(0,0);
        ui->toolButton_screen_xielou->setText("点击开启");
    }
    else
    {
        Flag_screen_xielou = 1;
        //emit hide_tablewidget(0,1);
        ui->toolButton_screen_xielou->setText("已开启");
    }
}
void systemset::on_toolButton_screen_radar_clicked()
{
    if(Flag_screen_radar == 1)
    {
        Flag_screen_radar = 0;
        //emit hide_tablewidget(1,0);
        ui->toolButton_screen_radar->setText("点击开启");
    }
    else
    {
        Flag_screen_radar = 1;
        //emit hide_tablewidget(1,1);
        ui->toolButton_screen_radar->setText("已开启");
    }
}
void systemset::on_toolButton_screen_safe_clicked()
{
    if(Flag_screen_safe == 1)
    {
        Flag_screen_safe = 0;
        //emit hide_tablewidget(2,0);
        ui->toolButton_screen_safe->setText("点击开启");
    }
    else
    {
        Flag_screen_safe = 1;
        //emit hide_tablewidget(2,1);
        ui->toolButton_screen_safe->setText("已开启");
    }
}
void systemset::on_toolButton_screen_burngas_clicked()
{
    if(Flag_screen_burngas == 1)
    {
        Flag_screen_burngas = 0;
        //emit hide_tablewidget(3,0);
        ui->toolButton_screen_burngas->setText("点击开启");
    }
    else
    {
        Flag_screen_burngas = 1;
        //emit hide_tablewidget(3,1);
        ui->toolButton_screen_burngas->setText("已开启");
    }
}
void systemset::on_toolButton_screen_zaixian_clicked()
{
    if(Flag_screen_zaixian == 1)
    {
        Flag_screen_zaixian = 0;
        //emit hide_tablewidget(4,0);
        ui->toolButton_screen_zaixian->setText("点击开启");
    }
    else
    {
        Flag_screen_zaixian = 1;
        //emit hide_tablewidget(4,1);
        ui->toolButton_screen_zaixian->setText("已开启");
    }
}
void systemset::on_toolButton_screen_cc_clicked()
{
    if(Flag_screen_cc == 1)
    {
        Flag_screen_cc = 0;
        //emit hide_tablewidget(5,0);
        ui->toolButton_screen_cc->setText("点击开启");
    }
    else
    {
        Flag_screen_cc = 1;
        //emit hide_tablewidget(5,1);
        ui->toolButton_screen_cc->setText("已开启");
    }
}

void systemset::on_comboBox_pressure_transmitters_mode_mode_currentIndexChanged(int index)
{
    Flag_Pressure_Transmitters_Mode = index;
    qDebug() << "Pressure_Transmitters is "<<index;
    config_Pressure_Transmitters_Mode_write();//压力变送器模式设置写入
}
//isoosi协议相关参数 重庆
void systemset::on_toolButton_isoosi_up_clicked()
{

}

void systemset::on_toolButton_isoosi_down_clicked()
{

}
void systemset::on_toolButton_isoosi_pw_clicked()
{
	IsoOis_MN = ui->lineEdit_isoosi_mn->text();
    IsoOis_PW = ui->lineEdit_isoosi_pw->text();
	IsoOis_UrlIp = ui->lineEdit_isoosi_UrlIp->text();
	IsoOis_UrlPort = ui->lineEdit_isoosi_UrlPort->text();
	IsoOis_StationId_Cq = ui->lineEdit_isoosi_cqid->text();
    config_isoosi_write();
}


void systemset::on_comboBox_network_version_currentIndexChanged(int index)
{
    Flag_Network_Send_Version = index;
    config_network_Version_write();//写入配置信息
    qDebug() << "network send model is "<< index;
	if(Flag_Network_Send_Version == 0) //福州
	{
		ui->frame_fujian->setHidden(0);
		ui->frame_guangzhou->setHidden(1);
		ui->frame_network_verselect->setHidden(1);
		ui->frame_hunan_login->setHidden(1);//湖南的登录信息
		ui->label_post_name->setText("广州油气回收上传");
	}
	if(Flag_Network_Send_Version == 1) //广州
	{
		ui->frame_fujian->setHidden(1);
		ui->frame_guangzhou->setHidden(0);
		ui->frame_isoosi_chongqing->setHidden(1);
		ui->frame_network_verselect->setHidden(1);
		ui->label_isoosi_name->setText("广州油气回收上传");
	}
	if(Flag_Network_Send_Version == 2) //重庆  与广州同一个界面
	{
		ui->frame_fujian->setHidden(1);
		ui->frame_guangzhou->setHidden(0);
		ui->frame_isoosi_chongqing->setHidden(0);
		ui->frame_network_verselect->setHidden(1);
		ui->label_isoosi_name->setText("重庆油气回收上传");
	}
	if(Flag_Network_Send_Version == 3) //唐山  与福州相同
	{
		ui->frame_fujian->setHidden(0);
		ui->frame_guangzhou->setHidden(1);
		ui->frame_network_verselect->setHidden(1);
		ui->frame_hunan_login->setHidden(1);//湖南的登录信息
		ui->label_post_name->setText("唐山油气回收上传");
	}
	if(Flag_Network_Send_Version == 4) //湖南协议 与福州类似
	{
		ui->frame_fujian->setHidden(0);
		ui->frame_guangzhou->setHidden(1);
		ui->frame_network_verselect->setHidden(1);
		ui->frame_hunan_login->setHidden(0);//湖南的登录信息
		ui->lineEdit_postusername->setText(POSTUSERNAME_HUNAN);
		ui->lineEdit_postpassword->setText(POSTPASSWORD_HUNAN);
		ui->label_post_name->setText("湖南油气回收上传");
	}
	if(Flag_Network_Send_Version >= 5)//其他
	{
		ui->frame_fujian->setHidden(1);
		ui->frame_guangzhou->setHidden(1);
		ui->frame_network_verselect->setHidden(0);
	}
}
//isoosi屏蔽气液比不合格
void systemset::on_toolButton_isoosi_pb_clicked()
{
    if(Flag_Shield_Network == 1)
    {
        Flag_Shield_Network = 0;
        ui->toolButton_isoosi_pb->setText("点击屏蔽");
    }
    else if(Flag_Shield_Network == 0)
    {
        Flag_Shield_Network = 1;
        ui->toolButton_isoosi_pb->setText("已屏蔽");
    }
    qDebug()<< Flag_Shield_Network;
    config_network_Version_write();//油气回收网络上传版本写入,屏蔽状态写入
}
/******************下面的函数用来判断是否能联通外网*************************
 *
 * *******************************************/
bool systemset::IPLive(QString ip, int port)
{
    QTcpSocket tcpClient;
    tcpClient.abort();
    tcpClient.connectToHost(ip, port);
    tcpClient.close();//关闭之后仍然能正常使用？？？此处用法有疑问
    //500毫秒没有连接上则判断不在线
    return tcpClient.waitForConnected(500);
}

void systemset::on_pushButton_testnet_clicked()
{
    //能接通百度IP说明可以通外网
    bool ok = IPLive("202.108.22.5", 80);

    if(ok == true)
    {
        ui->label_networ_test->setText("网络已连接");
        qApp->processEvents();
        QProcess::startDetached("ntpdate time.windows.com");
        QProcess::startDetached("hwclock -w");
    }
    else
    {
        ui->label_networ_test->setText("网络连接失败");
    }
    qDebug()<<ok;

}


void systemset::on_comboBox_reoilgas_ver_currentIndexChanged(int index)
{
     Flag_Reoilgas_Version = index+1;
     config_Pressure_Transmitters_Mode_write();//压力变送器,气液比采集器模式设置写入
}

void systemset::on_toolButton_station_id_queren_clicked()
{
    QString id = ui->lineEdit_hubei_station_id->text();
    Station_ID_HB[0] = id.toInt()/256;
    Station_ID_HB[1] = id.toInt()%256;
    config_xielou_network();//泄漏网络上传相关参数写入
}

void systemset::on_toolButton_hubei_kaiguan_clicked()
{
    if(Flag_HuBeitcp_Enable == 1)
    {
        Flag_HuBeitcp_Enable = 0;
        ui->toolButton_hubei_kaiguan->setText("点击开启");
    }
    else if(Flag_HuBeitcp_Enable == 0)
    {
        Flag_HuBeitcp_Enable = 1;
        ui->toolButton_hubei_kaiguan->setText("已开启");
    }
    qDebug()<<"Flag_HuBeitcp_Enable"<<Flag_HuBeitcp_Enable;
    config_xielou_network();//泄漏网络上传相关参数写入
}

void systemset::on_comboBox_xielou_net_version_currentIndexChanged(int index)
{
    Flag_XieLou_Version = index;
    config_xielou_network();//泄漏网络上传相关参数写入
    qDebug()<< "Flag_XieLou_Version" << Flag_XieLou_Version;
	if(Flag_XieLou_Version == 0)
    {
        ui->frame_hubei->setHidden(1);
        ui->frame_IFSF->setHidden(0);
		ui->label_114->setText("广州中石化端口设置");
    }
    if(Flag_XieLou_Version == 1)
    {
        ui->frame_hubei->setHidden(0);
        ui->frame_IFSF->setHidden(1);
    }
	if(Flag_XieLou_Version == 2)
	{
		ui->frame_hubei->setHidden(1);
		ui->frame_IFSF->setHidden(0);
		ui->label_114->setText("中国石油端口设置");
	}

	ui->widget_warn_ip_wrong->setHidden(0);//借用ip错误的弹窗
	ui->label_ipwrong->setText("设置完成后请重启控制器以使设置生效！");
}

void systemset::on_toolButton_gun_off_kaiqi_2_clicked()//手动解除关枪
{
    add_value_reoilgaswarn("关枪使能","手动开启");
    Flag_Gun_off = 0;
    Flag_SendMode_Oilgas = 4;//写油气回收设置，屏蔽关枪
    config_alset();
}

void systemset::on_toolButton_pop_show_clicked()
{
    if(Flag_Reoilgas_NeverShow == 0)
    {
        Flag_Reoilgas_NeverShow = 1;
        ui->toolButton_pop_show->setText("点击开启");
    }
    else if(Flag_Reoilgas_NeverShow == 1)
    {
        Flag_Reoilgas_NeverShow = 0;
        ui->toolButton_pop_show->setText("已开启");
    }
    config_reoilgas_warnpop();
}

void systemset::on_toolButton_MyServerSwitch_clicked()
{
	if(Flag_MyServerEn == 1)
	{
		ui->toolButton_MyServerSwitch->setText("点击开启");
		Flag_MyServerEn = 0;
	}
	else
	{
		ui->toolButton_MyServerSwitch->setText("已开启");
		Flag_MyServerEn = 1;
	}

}

void systemset::on_pushButton_U_clear_clicked()
{
	system("rm -r /media/sda*");
	system("rm -r /media/sdb*");
	system("sync");
}
//气密性测试流程
void systemset::on_pushButton_Airtighness_Test_clicked()
{
	Airtightness_Test *airtest;
	airtest = new Airtightness_Test;
	airtest->setAttribute(Qt::WA_DeleteOnClose);
	airtest->show();
}


/************配置信息网络上传*****************
 * id     没有用
 * jyqs   加油枪数量
 * pvz    pv阀正压
 * pvf    pv阀负压
 * hclk   后处理装置开启压力值
 * yzqh   最远端装液阻压力表的加油机编号
 * ***********************××××××*********/
void systemset::network_onfigurationdata(QString id, QString jyqs, QString pvz, QString pvf, QString hclk, QString yzqh)
{
	if(net_state == 0) //有网线连接
	{

		id = id;
		qDebug()<<"network send onfigurationdata!" << Flag_Network_Send_Version;
		if(Flag_Network_Send_Version == 0) //福建协议
		{
			Send_Configurationdata(DATAID_POST,jyqs,pvz,pvf,hclk,yzqh);
		}
		if(Flag_Network_Send_Version == 1) //广州协议
		{
			//isoosi添加
			emit setup_data(pvz,QString::number(-(pvf.toFloat())),"0.00","0.00");
		}
		if(Flag_Network_Send_Version == 2) //重庆协议
		{
			emit setup_data_cq(pvz,QString::number((pvf.toFloat())),"0","0");
		}
		if(Flag_Network_Send_Version == 3) //唐山协议，与福建相同
		{
			Send_Configurationdata(DATAID_POST,jyqs,pvz,pvf,hclk,yzqh);
		}
		if(Flag_Network_Send_Version == 4) //湖南协议，与福建类似
		{
			Send_Configurationdata_HuNan(DATAID_POST,jyqs,pvz,pvf,"NULL",yzqh);
		}
	}
}

