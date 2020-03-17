#include "mainwindow.h"
#include <QApplication>
#include <QWSServer>
#include <unistd.h>
#include <stdio.h>
#include "net_tcpclient_hb.h"
#include "keyboard.h"
#include "mythread.h"
#include "main_main.h"
#include "uartthread.h"
#include "warn_sound_thread.h"
#include "security.h"
#include "timer_pop.h"
#include "file_op.h"
//数据库头文件
#include<database_set.h>
#include<database_op.h>
#include"myserver/myserver.h"
//char IP_DES[20] = {0};
//main_main
char IP_BRO[32] = {0};
unsigned char Flag_TcpClose_FromUdp = 0;            //三次心跳断开TCP标志位/客户端要求断开
unsigned char Flag_TcpClose_FromTcp = 0;            //客户端主动断开链接标志位
unsigned char OIL_BASIN[16]={0xc0,0x00,0xc0,0x00,0xc0,0x00,0xc0,0x00,0xc0,0x00,0xc0,0x00,0xc0,0x00,0xc0,0x00};      //人井油盆状态
unsigned char OIL_PIPE[16]={0xc0,0x00,0xc0,0x00,0xc0,0x00,0xc0,0x00,0xc0,0x00,0xc0,0x00,0xc0,0x00,0xc0,0x00};       //管线测漏状态数据
unsigned char OIL_TANK[16]={0xc0,0x00,0xc0,0x00,0xc0,0x00,0xc0,0x00,0xc0,0x00,0xc0,0x00,0xc0,0x00,0xc0,0x00};       //油罐测漏状态数据
unsigned char OIL_DISPENER[16]={0xc0,0x00,0xc0,0x00,0xc0,0x00,0xc0,0x00,0xc0,0x00,0xc0,0x00,0xc0,0x00,0xc0,0x00};   //加油机底槽测漏状态数据
unsigned char Mythread_basin[9] = {0};      //给tcp主动信号
unsigned char Mythread_pipe[9] = {0};
unsigned char Mythread_dispener[9] = {0};
unsigned char Mythread_tank[9] = {0};
char *ipstr;
unsigned int ipa,ipb,ipc,ipd;                                       //a:192  b:168  c:1     d:106
unsigned int ID_M = 0x01;                                             //机器编号，逻辑地址低位
unsigned int heartbeat_time = 10;                                   //UDP心跳时间
        //uart
unsigned int Timer_Counter = 5;
int fd_uart;
int len_uart, ret_uart;
//mainwindow
unsigned char Flag_Sound_Radar_temp = 0;
unsigned int Flag_warn_delay = 0;                   //报警延长时间  也当标志位使用
unsigned char Flag_Timeto_CloseNeeded[6] = {0}; //==0计时定时息屏  ==1取消计时 Flag_Timeto_closeMainwindow = 300;
                                              //[0]窗体创建[1]测漏报警[2]气液比通信故障[3]环境检测：pre_pipe/tank[4]环境检测报警:gas1[5]气液比报警
unsigned char Flag_Network_Send_Version = 100;//油气回收网络上传版本 0-福州 1-广州 2 3 4 5其他
float PerDay_Al_Big[8] = {0};
float PerDay_Al_Smal[8] = {0};

QMutex Lock_Flag_Timeto;
extern QMutex Lock_Flag_xielou;//泄漏网络信息主动上传数组
extern QMutex Lock_Flag_youqi;//油气回收网络信息主动上传
//mythread.h
unsigned char flag_waitsetup = 0;                   //卡一下mythread
unsigned char flag_mythread = 0;                    //一次写入
unsigned char Flag_Sound_Xielou[20] = {0};          //2223   //4tankoil  5tankwater  6tanksensor 7tankuart
//20radar_uart 21 radar_warn  22 radar_monibaojing

//radar_485.h
int fd_uart_radar;
int ret_uart_radar;
int len_uart_radar;
//uartthread.h
unsigned char Flag_Sound_Radar[5] = {0};
unsigned char count_radar_uart = 0; //雷达通信故障计数

unsigned char Flag_Set_SendMode = 0;
unsigned char Flag_autoget_area = 0;                //是否完成区域一数组标志位
unsigned char Amp_R_Date[30];                       //从前端读回的幅值
unsigned char Get_Are_Flag[4];
int Master_Boundary_Point_Disp[4][6][6][2];
unsigned char Send_MODE[4] = {11,11,11,11};            //通信传输的消息模式
unsigned char Master_Back_Groud_Value[4][200][2];
unsigned char Back_Groud_Temp[4][200][2];           //定义一个200×4的二维数组背景值，现在只使用180个
unsigned char Get_Back_Flag[4];                     //取得阈值标志位
float Goal[4][20][2];                               //存储目标位置
unsigned char Flag_moni_warn = 0;
unsigned char Alarm_Re_Flag[4];
unsigned char Communication_Machine = 0;    //通信时的从机

//serial.h
unsigned char Data_Buf_Sencor[45] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
                                     0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
                                     0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};  //19字节数组
unsigned char Test_Method = 0x03;                   //检测方法标志位，00其他方法，01压力法，02液媒法，03传感器法
//systemset.h
unsigned char Flag_Set_yuzhi = 0;
unsigned char Flag_pre_mode;  //1 调试模式 0正常模式
unsigned char Flag_screen_xielou = 1;
unsigned char Flag_screen_radar = 1;
unsigned char Flag_screen_safe = 1;
unsigned char Flag_screen_burngas = 1;
unsigned char Flag_screen_zaixian = 1;
unsigned char Flag_screen_cc = 1;

//udp.h
unsigned char net_state = 0;
//泄漏网络上传
unsigned char Flag_XieLou_Version = 0;//泄漏上传版本

//传感器数量设置
unsigned char count_basin=0;
unsigned char count_tank=0;
unsigned char count_pipe=0;
unsigned char count_dispener=0;
float count_Pressure[8] = {0};//存储压力值
unsigned char flag_silent = 0;
//人体静电
unsigned char Flag_xieyou = 1;  //人体静电使能
//IIE
unsigned char Flag_IIE = 1; //IIE使能
//在线油气回收
unsigned char Flag_Accumto[12][8] = {0};//油枪连续报警天数
unsigned char Flag_Delay_State[12][8] = {0};//关枪继电器使能
double PerDay_Percent[8] = {0};//四把枪的al正常占比
float PerDay_AL[8] = {0};//四把枪的al均值
float Gas_Factor[12][8] = {0};
float Fueling_Factor[12][8] = {0};
unsigned char Amount_Dispener = 0;      //加油机数量
int Far_Dispener = 0;  //最远端加油机编号
unsigned char Speed_fargas = 0;  //最远端油气流速
unsigned char Amount_Gasgun[12] = {0};    //加油机挂载的油枪数量
unsigned char Flag_SendMode_Oilgas = 1;     //模式选择位
unsigned char Reoilgas_Version_Set_Whichone = 0;    //选择询问哪个变送器
QMutex Lock_Mode_Reoilgas;
float Temperature_Day[290][2] = {0};//图标日数据
float Temperature_Month[750][2] = {0};//月数据
float Temperature_Month_Min[750][2] = {0};//月数据 al最小值
float AL_Day[1440][2] = {0};
unsigned char flag_morethan5[96] = {0};//记录连续几天没有到达5次加油
float NormalAL_Low = 1.0;
float NormalAL_High = 1.2;
unsigned char WarnAL_Days = 5;
unsigned char Flag_Gun_off = 0;
unsigned char Flag_Reoilgas_Version = 2;//气液比采集器版本选择
QString Time_Reoilgas;//第二版本气液比采集器发上来的时间
//油气回收网络上传
QString Post_Address = "http://218.5.4.107:60000/webservice/gasStation?wsdl";
QString VERSION_POST = "V1.1";   // 通信协议版本
QString DATAID_POST = "000001";  //数据序号（6 位）
QString USERID_POST = "3501040007";    // 区域代码标识（6 位）+ 加油站标识（4 位）
QString TIME_POST = "";                //在线监控设备当前时间（年月日时分 14 位）
QString SEC_POST = "0";                //加密标识（1 表示业务数据为密文传输，0 表示明文）
QString POSTUSERNAME_HUNAN = "admin";
QString POSTPASSWORD_HUNAN = "123456";
unsigned char Flag_Shield_Network = 0;//屏蔽网络上传的报警信息，只上传正常数据 1屏蔽 0正常
unsigned char Flag_Postsend_Enable = 1;//网络上传使能位，1 上传  0不上传
//isoosi
QString IsoOis_MN = "440111301A52TWUF73000001";//"440111301A52TWUF73000001";
QString IsoOis_PW = "758534";//"758534";
QString IsoOis_UrlIp = "";  //网络上传ip
QString IsoOis_UrlPort = ""; //网络上传端口
QString IsoOis_StationId_Cq = "5001130001";//加油站ID
//fga1000
unsigned char Env_Gas_en = 0; //浓度传感器使能
unsigned char Pre_tank_en = 0;//油罐压力传感器使能
unsigned char Pre_pipe_en = 0;//管线液阻压使能
unsigned char Tem_tank_en = 0;//油罐、气体温度使能
float Tem[2] = {0};//温度qqqqqqqqqqqqqqqqqlock
float Pre[2] = {0};//压力qqqqqqqqqqlock
float Positive_Pres = 1000;//正压开启压力
float Negative_Pres = -1000;//负压开启压力
unsigned char Num_Fga = 0;//fga传感器数目，暂时默认为1个，最多6个
unsigned char Gas_Concentration_Fga[7] ={0x00,0x00,0x00,0x00,0x00,0x00,0x00};//fga浓度  [0]没用qqqqqqqqqlock
unsigned char Flag_Pressure_Transmitters_Mode = 1;//压力变送器模式 0 485  1 4-20ma  2 无线模式

int main(int argc, char *argv[])
{
	setvbuf(stdout, 0, _IONBF, 0);
 //   QWSServer::setBackground(QColor(0,0,0,0));  //去掉绿屏
    QApplication a(argc, argv,QApplication::GuiServer);
//    QWSServer::setCursorVisible(false);//取消鼠标显   位置qapplication实例化之后
    //数据库初始化
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
    creatConnection();
    creat_table();
    MainWindow w;
    w.setWindowFlags(Qt::FramelessWindowHint);


	init_xielou_network();//初始化泄漏网络上传
	if(Flag_XieLou_Version == 0)
	{
		pthread_t id_main;                     //主程序若放到主窗体建立前会导致网络套接字绑定错误，原因？？
		long t_main = 0;
		int ret_main = pthread_create(&id_main,NULL,main_main,(void*)t_main);
		if(ret_main!=0)
		{
			printf("Can not create main thread!\n");
			exit(1);
		}
		qDebug()<<"ZhongShiHua XieLou";
	}
	if(Flag_XieLou_Version == 2)
	{
		main_main_zhongyou *main_main_zy = new main_main_zhongyou;
		main_main_zy->start();

		mytcpclient_zhongyou *zhongyoutcp = new mytcpclient_zhongyou;
		QObject::connect(main_main_zy,SIGNAL(net_reply(int)),zhongyoutcp,SLOT(net_reply(int)),Qt::DirectConnection);
		zhongyoutcp->start();
		qDebug()<<"ZhongShiYou XieLou";
	}
	if(Flag_XieLou_Version == 1)
	{
		net_tcpclient_hb *hbtcp = new net_tcpclient_hb;
		hbtcp->start();
		qDebug()<<"HuBei XieLou";
	}

    //结束进程
//    hbtcp->quit();
//    hbtcp->wait();
//    delete hbtcp;

    warn_sound_thread *write_sound = new warn_sound_thread;
	write_sound->start();

	myserver *myserver_thread = new myserver;
	myserver_thread->start();

    security *securitys = new security;
    QObject::connect(securitys,SIGNAL(liquid_close()),&w,SLOT(liquid_close_s()));
    QObject::connect(securitys,SIGNAL(liquid_nomal()),&w,SLOT(liquid_nomal_s()));
    QObject::connect(securitys,SIGNAL(liquid_warn()),&w,SLOT(liquid_warn_s()));
    QObject::connect(securitys,SIGNAL(pump_close()),&w,SLOT(pump_close_s()));
    QObject::connect(securitys,SIGNAL(pump_run()),&w,SLOT(pump_run_s()));
    QObject::connect(securitys,SIGNAL(pump_stop()),&w,SLOT(pump_stop_s()));
    QObject::connect(securitys,SIGNAL(crash_column_stashow(unsigned char,unsigned char)),&w,SLOT(crash_column_stashow(unsigned char,unsigned char)));
    securitys->start();

    timer_pop *pop_ups = new timer_pop;
    QObject::connect(pop_ups,SIGNAL(show_reoilgas_pop(int)),&w,SLOT(show_pop_ups(int)));
    QObject::connect(pop_ups,SIGNAL(refresh_dispener_data()),&w,SLOT(refresh_dispener_data_slot()));
    pop_ups->start();

    w.show();
    return a.exec();
}
