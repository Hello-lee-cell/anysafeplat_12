#include <QtDebug>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <asm/termios.h>
#include <signal.h>
#include <sys/time.h>
#include <QTimer>
#include <serial.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <errno.h>
#include <sys/shm.h>
#include "systemset.h"
#include "mythread.h"
#include "config.h"
#include "io_op.h"
#include "file_op.h"
#include "database_op.h"
#include "fga1000_485.h"
#include "mainwindow.h"
#include <QMutex>
#include "reoilgasthread.h"
#define EXPORT_PATH_FGA "/sys/class/gpio/export"
#define RADAR_CON_26 "26"
#define DIRECT_PATH_26 "/sys/class/gpio/gpio26/direction"
#define VALUE_PATH_26  "/sys/class/gpio/gpio26/value"
#define RADAR_CON_27 "27"
#define DIRECT_PATH_27 "/sys/class/gpio/gpio27/direction"
#define VALUE_PATH_27  "/sys/class/gpio/gpio27/value"
#define IO_DIRECTOUT    "out"
#define VALUE_HIGH   "1"
#define VALUE_LOW   "0"

QMutex wait_fgaread_over;
unsigned char ask_fga[8] = {0x01,0x03,0x00,0x00,0x00,0x02,0x00,0x00};  //问可燃气体浓度
//unsigned char ask_pre_sensor[8] = {0xa1,0x30,0x00,0xb5,0x00,0x01,0x00,0x00};//问压力传感器压力值，温度值
unsigned char ask_pre_sensor[8] = {0xa1,0x03,0x20,0x02,0x00,0x01,0x00,0x00};//问压力传感器压力值，温度值
unsigned char ask_zeroing[8] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}; //FGA校零
unsigned char read_fga[9] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};  //读可燃气体浓度
unsigned char read_pre_sensor[10] = {0};//读取压力变送器返回数值
unsigned char flag_uart_wro_fga[7] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00}; //通讯故障标志位,0位没用*如果增加可燃气体检测器数需量增大这个数组
unsigned char sta_fga[7] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00};//fga状态,0位没用*如果增加可燃气体检测器数需量增大这个数组qqqqqqqqqqlock
unsigned char Flag_StaFga_Temp[7] = {0x09,0x09,0x09,0x09,0x09,0x09,0x09};     //可燃气体状态一次写入
unsigned char Flag_StaPre_Temp[7] = {0x09,0x09,0x09,0x09,0x09,0x09,0x09};     //压力表状态一次写入
unsigned char num_fga_this = 0;//本地变量来代替全局变量，避免多次读取全局变量
unsigned char add_num = 1;//问的传感器地址
unsigned char flag_zeroing = 0;//校零标志位     校零未完成不会问压力传感器
unsigned char add_zeroing = 0; //校零传感器地址
unsigned char flag_zeroing_send = 0;//发送校零数据后的标志位
unsigned char flag_zeroing_wro = 0;//校零失败计数
unsigned char num_fga_pre;//存储一次读取的传感器数量
unsigned char flag_fga_prwar[7] = {0};//油气浓度预警三次后设为预警
int flag_fga_warn[7] = {0};//5分钟 300个数后报警

//压力变送器 1号地址0xa1，对应罐压；2号地址0xa2，对应管线压力
unsigned char flag_fga_over = 0;//问完fga传感器标志位
unsigned char flag_ask_fre = 0;//问压力变送器的次数，一个数据传感器需要问多次
unsigned char temperature[8] = {0};//温度
unsigned char pressure[8] = {0};//气压

unsigned char flag_uart_wro_pressure[3] = {0}; //压力变送器通讯故障计数 [2]当做4-20ma温度传感器的状态
unsigned char sta_pre[3] = {0};//压力变送器状态标志位qqqqqqqqqlock  sta_pre[2] 当做4-20ma温度传感器的状态
unsigned char add_pre = 0;//压力变送器标号
unsigned char Flag_Ifsend = 0;//判断是否重发数据  全局变量，发送失败的时候置0在重新发送

int time_line_pressure = 0;//管线压力预警次数


int fd_uart_fga; //全局变量？
int ret_usrt_fga;
int len_uart_fga;

unsigned int testtime = 0; //int 的最大值2147483647
unsigned int day_over = 0;//每夜零点清零  调试使用
//第一种报警
unsigned int time_sec_tankpre = 0;//第一种判断预报警秒数
unsigned int time_day_tankpre = 0;//第一种判断预报警天数
unsigned char flag_warnpre_day = 0;//第一种判断预报警标志位
//第二种报警
unsigned int time_sec_tankpre2 = 0; //第二种判断预报警秒数
//第三种报警
unsigned int time_sec_tankpre3 = 0;      //第三种判断每天报警次数
unsigned char flag_warnpre_day3 = 0; //第三种判断预报警标志位
unsigned char time_day_tankpre3 = 0;   //第三种判断报警天数

//线程同步标志 time_time
unsigned char Flag_Sync = 0;
unsigned char Flag_Count = 0;   //5s 30s计数


int Gas_Concentration_Threshold = 40;//油气浓度报警阈值，20 g/m3, ×××全局变量×××

//post添加 用于区分罐压报警类型
unsigned char Sta_Tank_Postsend = 0;
//0正常；1零压预警；2零压报警；3真空阀临界压力预警；4真空阀临界压力报警；5真空阀预警；6真空阀报警
QString STA_YZ = "0";
QString STA_YGLY = "0";
QString STA_PVZT = "0";
QString STA_PVLJZT = "0";
QString STA_ND = "0";

unsigned char Pre_tank_en_change = 0;//如果设置改变  要及时发信号标志位
unsigned char Pre_pipe_en_change = 0;
unsigned char Env_Gas_en_change = 0;
unsigned char Tem_tank_en_change = 0;
unsigned char flag_pre_version = 0;//压力表版本
unsigned char Flag_TankPre_Type = 1;//0有线版本 1无线版本  默认无线版本
unsigned char Flag_PipePre_Type = 1;
unsigned char Flag_TankTem_Type = 1;
unsigned char Flag_Gas_Type = 1;

unsigned char Flag_MyserverFirstSend = 0;
QString his_tem = "";//历史记录缓存.压力报警类型

unsigned char Flag_SendOnceGunCloseOperate = 0;

FGA1000_485::FGA1000_485(QObject *parent):
    QThread(parent)
{
//	QTimer *time_timeout = new QTimer;
//	time_timeout->setInterval(1000);
//	time_timeout->start();
//	time_timeout->moveToThread(this);
//	connect(time_timeout,SIGNAL(timeout()),this,SLOT(time_time()));
}

void FGA1000_485::run()
{
	sleep(5);
	init_fga_num();//从文本中读取数目,压力变送器和可燃气体检测传感器的数目
	num_fga_pre = Num_Fga;
	sleep(2);
	fd_uart_fga = open(FGA_SERI,O_RDWR|O_NOCTTY);
	ret_usrt_fga = set_port_attr(fd_uart_fga,B9600,8,"1",'N',0,0);
	Pre_tank_en_change = Pre_tank_en;
	Pre_pipe_en_change = Pre_pipe_en;
	Env_Gas_en_change = Env_Gas_en;
	Tem_tank_en_change = Tem_tank_en;
	flag_pre_version = Flag_Pressure_Transmitters_Mode;

	while (1)
	{
		if( (Pre_tank_en != Pre_tank_en_change) || (Pre_pipe_en != Pre_pipe_en_change)||(flag_pre_version != Flag_Pressure_Transmitters_Mode))
		{
			Flag_StaPre_Temp[0] = 0x09;Flag_StaPre_Temp[1] = 0x09;Flag_StaPre_Temp[2] = 0x09;Flag_StaPre_Temp[3] = 0x09;
			Pre_tank_en_change = Pre_tank_en;
			Pre_pipe_en_change = Pre_pipe_en;
			flag_pre_version = Flag_Pressure_Transmitters_Mode;
		}
		if(Env_Gas_en != Env_Gas_en_change)
		{
			Flag_StaFga_Temp[0] = 0x09;Flag_StaFga_Temp[1] = 0x09;Flag_StaFga_Temp[2] = 0x09;Flag_StaFga_Temp[3] = 0x09;
			Flag_StaFga_Temp[4] = 0x09;Flag_StaFga_Temp[5] = 0x09;Flag_StaFga_Temp[6] = 0x09;
			Env_Gas_en_change = Env_Gas_en;
		}
		if((Tem_tank_en_change != Tem_tank_en)||(flag_pre_version != Flag_Pressure_Transmitters_Mode))
		{
			Flag_StaPre_Temp[0] = 0x09;Flag_StaPre_Temp[1] = 0x09;Flag_StaPre_Temp[2] = 0x09;Flag_StaPre_Temp[2] = 0x09;
			Tem_tank_en_change = Tem_tank_en;
			flag_pre_version = Flag_Pressure_Transmitters_Mode;
		}
		if(Flag_Controller_Version == 0)
		{
			SendDataFGA();
			msleep(100);
		}
		else if(Flag_Controller_Version == 1)//新版本控制器模式，所有传感器在一个口上  无线模式 新压力表模式
		{
			//浓度同步
			sta_fga[1] = ReoilGasFgaSta[0];sta_fga[2] = ReoilGasFgaSta[1];
			sta_fga[3] = ReoilGasFgaSta[2];sta_fga[4] = ReoilGasFgaSta[3];
			sta_fga[5] = ReoilGasFgaSta[4];sta_fga[6] = ReoilGasFgaSta[5];

			Gas_Concentration_Fga[1] = Fga1000_Value[0];Gas_Concentration_Fga[2] = Fga1000_Value[1];
			Gas_Concentration_Fga[3] = Fga1000_Value[2];Gas_Concentration_Fga[4] = Fga1000_Value[3];
			Gas_Concentration_Fga[5] = Fga1000_Value[4];Gas_Concentration_Fga[6] = Fga1000_Value[5];
			//压力同步
			sta_pre[0] = ReoilgasPreSta[0];
			sta_pre[1] = ReoilgasPreSta[1];
			//温度同步
			sta_pre[2] = ReoilgasTemSta[0];
			//数据分析
			msleep(500);
			num_fga_this = Num_Fga;
			if(num_fga_pre != num_fga_this)//传感器数目发生改变
			{
				empty_array();
				num_fga_pre = num_fga_this;
			}
			data_processing();
			sta_pressure();
			//
		}
		//this->exec();
	}
}


//问FGA传感器
void FGA1000_485::SendDataFGA()
{
	num_fga_this = Num_Fga;
	if(num_fga_pre != num_fga_this)//传感器数目发生改变
	{
		empty_array();
		num_fga_pre = num_fga_this;
	}
	if((num_fga_this > 0) && (flag_fga_over == 0)&&((Env_Gas_en == 1) || (num_fga_this > 2))) //如果传感器数目不为零  而且轮到问可燃气体检测器了
	{
		if(Env_Gas_en == 1)
		{

		}
		if((Env_Gas_en == 0)&&(num_fga_this > 2))
		{
			if(add_num<=2)
			{
				add_num = 3;
			}
		}
		gpio_26_low(); gpio_27_high();
		unsigned char *sucArray_fga;
		unsigned int SCRC = 0;

		if(flag_zeroing == 0) //如果是正常模式
		{
			ask_fga[0] = add_num; //要问的传感器地址
			sucArray_fga = ask_fga;//指向正常数组
			flag_zeroing_send = 0;

			SCRC = CRC_Test(sucArray_fga,8);
			*(sucArray_fga + 6) = ((SCRC & 0xff00) >> 8);
			*(sucArray_fga + 7) = (SCRC & 0x00ff);
			len_uart_fga = write(fd_uart_fga,ask_fga,8);
			msleep(10);
			gpio_26_high();gpio_27_low();
		}
		else  //校零模式
		{
			ask_zeroing[0]=add_zeroing;//要校零的传感器地址
			sucArray_fga = ask_zeroing;//指向清零数组
			flag_zeroing_send = 1;

			SCRC = CRC_Test(sucArray_fga,8);
			*(sucArray_fga + 6) = ((SCRC & 0xff00) >> 8);
			*(sucArray_fga + 7) = (SCRC & 0x00ff);
			len_uart_fga = write(fd_uart_fga,ask_zeroing,8);
			msleep(10);
			gpio_26_high();gpio_27_low();
		}
		msleep(200);
		wait_fgaread_over.lock();
		ReadDataFGA();//读可燃气体
		wait_fgaread_over.unlock();
	}
	else//问压力变送器
	{
		msleep(1);
		flag_fga_over = 1;//fga传感器问完了，接下来问压力变送器
		if(Flag_Pressure_Transmitters_Mode == 1)
		{
			send_Pressure_Transmitters();//问压力变送器函数
		}
		if(Flag_Pressure_Transmitters_Mode == 0)
		{
			send_Pressure_Transmitters_485();//485压力变送器
		}
	}
}

//读取FGA发送回来的数据
void FGA1000_485::ReadDataFGA()
{
	unsigned char *sucArray_fga = read_fga;
	int SCRC = 0;
	unsigned char CRCHi;
	unsigned char CRCLi;
	len_uart_fga = read(fd_uart_fga, read_fga, sizeof(read_fga));
	if(flag_zeroing_send == 0)//是正常返回值
	{
		if(len_uart_fga <= 0)  //无数据，通讯故障
		{
			flag_uart_wro_fga[add_num]++;
		}
		else
		{
			flag_uart_wro_fga[add_num] = 0;
			SCRC = CRC_Test(sucArray_fga,9);
			CRCHi = ((SCRC & 0xff00) >> 8);
			CRCLi = (SCRC & 0x00ff);
			if((CRCHi == read_fga[7])&&(CRCLi == read_fga[8]))//数据校验通过
			{
				//printf("CRC RIGHT\n");
				if(add_num <= 2) //前两个油气浓度
				{
					if((read_fga[4] == 0x01)||(read_fga[4] == 0x02)||(read_fga[4] == 0x00))//如果是正常的报警状态，则舍弃，自行判断。
					{
						if((sta_fga[add_num] == 0x04)||(sta_fga[add_num] == 0x03)||(sta_fga[add_num] == 0x05))
						{
							read_fga[4] = 0x00;//如果之前是通讯故障/传感器故障/探测器故障，改变之后暂设为正常
						}
						else
						{
							read_fga[4] = sta_fga[add_num];
						}
					}
					sta_fga[add_num] = read_fga[4];//状态
					Gas_Concentration_Fga[add_num] = read_fga[6]*2;//浓度  ***因为后来的传感器单位是克每立方米，所以对数据乘上2
					if(read_fga[4] == 0xc0)
					{
						sta_fga[add_num] = 0x05;//探测器传感器故障
						Gas_Concentration_Fga[add_num] = 0;
					}
					if(read_fga[4] == 0x03)
					{
						sta_fga[add_num] = 0x03;//传感器故障
						Gas_Concentration_Fga[add_num] = 0;
					}
				}
				else
				{
					sta_fga[add_num] = read_fga[4];//状态
					qDebug()<< add_num<<read_fga[4]<<"!!!!!!!!!!!!!!";
					Gas_Concentration_Fga[add_num] = read_fga[6];//浓度  卸油口传感器单位是LEL，不用乘2
					if(read_fga[4] == 0xc0)
					{
						sta_fga[add_num] = 0x05;//探测器传感器故障
						Gas_Concentration_Fga[add_num] = 0;
					}
					if(read_fga[4] == 0x03)
					{
						sta_fga[add_num] = 0x03;//传感器故障
						Gas_Concentration_Fga[add_num] = 0;
					}
				}

			}
		}
		if(flag_uart_wro_fga[add_num] >= 2)//连续3次没有数据返回判定通讯故障
		{
			for(uchar i=0;i<9;i++)//对读取数据数组清零
			{
				read_fga[i] = 0;
			}
			sta_fga[add_num] = 0x04;
			Gas_Concentration_Fga[add_num] = 0;//判定为通讯故障时浓度设为0
			flag_uart_wro_fga[add_num] = 0;
		}


		add_num++;
		if(add_num > num_fga_this)
		{
			data_processing();//数据分析，
			add_num = 1;
			flag_fga_over = 1;//fga传感器问完了，接下来问压力变送器
		}

//        /******************************/
//        for(uchar i = 0; i < 9; i ++)
//        {
//            printf("  %02x",read_fga[i]);
//        }
//        printf("---\n");
	}
	else //是清零返回值
	{
		if(len_uart_fga <= 0)  //无数据，通讯故障
		{
			flag_zeroing_wro++;
		}
		else
		{
			SCRC = CRC_Test(sucArray_fga,9);
			CRCHi = ((SCRC & 0xff00) >> 8);
			CRCLi = (SCRC & 0x00ff);
			if((CRCHi == read_fga[7])&&(CRCLi == read_fga[8]))//数据校验通过
			{
				 //对返回数据进行判定，具体判断根据探头返回的数据在进行设置
				emit Zeroing_success(add_zeroing);//校零成功
				flag_zeroing = 0; //校零结束
				flag_zeroing_wro = 0;
			}
		}
		if(flag_zeroing_wro > 5) //如果一次不成功则最多连续发送5次校准命令
		{
			emit Zeroing_fail(add_zeroing); //校零失败
			flag_zeroing_wro = 0;
			flag_zeroing = 0; //校零结束
		}
	}
}

//FGA状态检测
void FGA1000_485::data_processing()
{
	for(int i=1; i<=num_fga_this; i++)
	{
		if(i <= 2)
		{
			if((sta_fga[i]!=0x03)&&(sta_fga[i]!=0x04)&&(sta_fga[i]!=0x05))
			{
				if(Gas_Concentration_Fga[i] > Gas_Concentration_Threshold)//预警
				{
					flag_fga_prwar[i]++;
					if(flag_fga_prwar[i]>=3)//三次预警
					{
						sta_fga[i] = 0x01;//预警
						flag_fga_prwar[i] = 3;
					}
				}
				if(Gas_Concentration_Fga[i] <= Gas_Concentration_Threshold)
				{
					sta_fga[i] = 0x00;//正常
					flag_fga_prwar[i] = 0;
				}
				if((Gas_Concentration_Fga[i] > Gas_Concentration_Threshold)&&(flag_fga_prwar[i] < 3))//防止变化时出现01  02
				{
					sta_fga[i] = 0x00;//正常
				}
			}
		}
		else
		{

		}
	}
}

//可燃气体检测校零槽函数，暂时还未与设置中的某个信号关联，
void FGA1000_485::Zeroing(int n)
{
	add_zeroing = n;
	flag_zeroing = 1;//校零标志位
}

//从文本中读取可燃气体检测器和压力变送器的数目，开机运行一次
void FGA1000_485::init_fga_num()
{
	QFile config_fga("/opt/fga/config_fga.txt");
	if(!config_fga.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qDebug() <<"Can't open config_fga file!"<<endl;
	}
	QTextStream in(&config_fga);
	QString line = in.readLine();
	QByteArray read_config = line.toLatin1();
	char *read_data_fga = read_config.data();
	Num_Fga = atoi(read_data_fga);
	config_fga.close();

	emit init_burngas_setted(Num_Fga-2);//桌面初始化

	QFile config_pre("/opt/reoilgas/config_pre.txt");
	if(!config_pre.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qDebug() <<"Can't open config_pre file!"<<endl;
	}
	QTextStream in2(&config_pre);
	QString line2;
	for(uchar i = 0; i < 4; i ++)
	{
		line2 = in2.readLine();
		QByteArray read_config_pre = line2.toLatin1();
		char *read_data_pre = read_config_pre.data();
		if(i == 0)
		{
			Pre_tank_en = atoi(read_data_pre);
		}
		if(i == 1)
		{
			Pre_pipe_en = atoi(read_data_pre);
		}
		if(i == 2)
		{
			Env_Gas_en = atoi(read_data_pre);
		}
		if(i == 3)
		{
			Tem_tank_en = atoi(read_data_pre);
		}
	}
	config_pre.close();

	QFile config_50_50("/opt/reoilgas/info_accum/50-50");
	if(!config_50_50.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qDebug() <<"Can't open config_50_50 file!"<<endl;
	}
	QTextStream out(&config_50_50);
	QString line_50;
	line_50 = out.readLine();
	time_day_tankpre = line_50.toInt();
	config_50_50.close();

	QFile config_600_1000("/opt/reoilgas/info_accum/600-1000");
	if(!config_600_1000.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qDebug() <<"Can't open config_600_1000 file!"<<endl;
	}
	QTextStream out2(&config_600_1000);
	QString line_600;
	line_600 = out2.readLine();
	time_day_tankpre3 = line_600.toInt();
	config_600_1000.close();
	printf("FGA Inited,Value Of 50 600 Is:%d/%d\n",time_day_tankpre,time_day_tankpre3);

	//读PV阀的正压开启压力
	QFile config_pv_positive("/opt/reoilgas/config_pv_positive.txt");
	if(!config_pv_positive.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qDebug() <<"Can't open config_pv file!"<<endl;
	}
	QTextStream in_pv_positive(&config_pv_positive);
	QString line_pv_positive;
	line_pv_positive = in_pv_positive.readLine();
	QByteArray read_config_pv_positive = line_pv_positive.toLatin1();
	char *read_data_pv_positive = read_config_pv_positive.data();
	Positive_Pres = atoi(read_data_pv_positive);//正压
	config_pv_positive.close();
	printf("**************************Positive is %f*************************\n",Positive_Pres);

	//读PV阀的负压开启压力
	QFile config_pv_negative("/opt/reoilgas/config_pv_negative.txt");
	if(!config_pv_negative.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qDebug() <<"Can't open config_pv file!"<<endl;
	}
	QTextStream in_pv_negative(&config_pv_negative);
	QString line_pv_negative;
	line_pv_negative = in_pv_negative.readLine();
	QByteArray read_config_pv_negative = line_pv_negative.toLatin1();
	char *read_data_pv_negative = read_config_pv_negative.data();
	Negative_Pres = atoi(read_data_pv_negative);//负压
	config_pv_negative.close();
	printf("**************************Positive is %f*************************\n",Negative_Pres);
}

//清空未使用到的数组数据，fga传感器
void FGA1000_485::empty_array() //清零未设置的数组位
{
	for(uchar i = num_fga_this + 1; i < 7; i++)
	{
		flag_uart_wro_fga[i] = 0;
		sta_fga[i] = 0;
		Gas_Concentration_Fga[i] = 0;
	}
	for(uchar i = 0;i<7;i++)
	{
		Flag_StaFga_Temp[i] = 0x09;
	}
}

//问压力变送器 4-20ma
void FGA1000_485::send_Pressure_Transmitters()
{
	if((Pre_pipe_en == 0)&&(Pre_tank_en == 0)&&(Tem_tank_en == 0))
	{
		flag_fga_over = 0;//清零，接下来问可燃气体检测器
		flag_ask_fre = 0;
		sta_pressure();
		//emit data_show();//显示数值
		//SendDataFGA();  //2018.10.16
	}
	else
	{
		ask_pre_sensor[0] = 0xa1;

		if(flag_ask_fre == 0)//问温度
		{
			ask_pre_sensor[1] = 0x03;
			ask_pre_sensor[2] = 0x20;
			ask_pre_sensor[3] = 0x06;
			ask_pre_sensor[4] = 0x00;
			ask_pre_sensor[5] = 0x01;
			add_pre = 2;
		}
		if(flag_ask_fre == 1)//问温度
		{
			ask_pre_sensor[1] = 0x03;
			ask_pre_sensor[2] = 0x20;
			ask_pre_sensor[3] = 0x07;
			ask_pre_sensor[4] = 0x00;
			ask_pre_sensor[5] = 0x01;
			add_pre = 2;
		}
		if(flag_ask_fre == 2)//问压力1
		{
			ask_pre_sensor[1] = 0x03;
			ask_pre_sensor[2] = 0x20;
			ask_pre_sensor[3] = 0x02;
			ask_pre_sensor[4] = 0x00;
			ask_pre_sensor[5] = 0x01;
			add_pre = 0;
		}
		if(flag_ask_fre == 3)//问压力1
		{
			ask_pre_sensor[1] = 0x03;
			ask_pre_sensor[2] = 0x20;
			ask_pre_sensor[3] = 0x03;
			ask_pre_sensor[4] = 0x00;
			ask_pre_sensor[5] = 0x01;
			add_pre = 0;
		}
		if(flag_ask_fre == 6)//问压力2
		{
			ask_pre_sensor[1] = 0x03;
			ask_pre_sensor[2] = 0x20;
			ask_pre_sensor[3] = 0x04;
			ask_pre_sensor[4] = 0x00;
			ask_pre_sensor[5] = 0x01;
			add_pre = 1;
		}
		if(flag_ask_fre == 7)//问压力2
		{
			ask_pre_sensor[1] = 0x03;
			ask_pre_sensor[2] = 0x20;
			ask_pre_sensor[3] = 0x05;
			ask_pre_sensor[4] = 0x00;
			ask_pre_sensor[5] = 0x01;
			add_pre = 1;
		}

		gpio_26_low(); gpio_27_high();
		unsigned char *sucArray_pre;
		unsigned int SCRC = 0;
		sucArray_pre = ask_pre_sensor;

		SCRC = CRC_Test(sucArray_pre,8);
		*(sucArray_pre + 6) = ((SCRC & 0xff00) >> 8);
		*(sucArray_pre + 7) = (SCRC & 0x00ff);
		len_uart_fga = write(fd_uart_fga,ask_pre_sensor,8);
		msleep(10);
		gpio_26_high();gpio_27_low();
		//QTimer::singleShot(300, this, SLOT(read_pressure_transmitters()));
		msleep(200);
		//qDebug()<< "ask pre add "<<flag_ask_fre;
		wait_fgaread_over.lock();
		read_pressure_transmitters();
		wait_fgaread_over.unlock();
	}
}
//问压力变送器 485
void FGA1000_485::send_Pressure_Transmitters_485()
{
	if((Pre_pipe_en == 0)&&(Pre_tank_en == 0))
	{
		flag_fga_over = 0;//清零，接下来问可燃气体检测器
		flag_ask_fre = 0;
		sta_pressure();
		//emit data_show();//显示数值
		//SendDataFGA();  //2018.10.16
	}
	else
	{
		if((Pre_tank_en == 1)&&(Pre_pipe_en == 0))//如果只有罐
		{
			ask_pre_sensor[0] = 0xa1;
			add_pre = 0;//当前询问1号压力变送器
		}
		if((Pre_pipe_en == 1)&&(Pre_tank_en == 0))//如果只有管线液阻
		{
			ask_pre_sensor[0] = 0xa2;
			add_pre = 1;//当前询问2号压力变送器
		}
		if((Pre_tank_en == 1)&&(Pre_pipe_en == 1))//如果二者都有
		{
			if(flag_ask_fre <= 3)
			{
				ask_pre_sensor[0] = 0xa1;
				add_pre = 0;//当前询问1号压力变送器
			}
			else
			{
				ask_pre_sensor[0] = 0xa2;
				add_pre = 1;//当前询问2号压力变送器
			}
		}
		if((flag_ask_fre == 0) || (flag_ask_fre == 4))//问温度1
		{
			ask_pre_sensor[1] = 0x03;
			ask_pre_sensor[2] = 0x00;
			ask_pre_sensor[3] = 0xb5;
			ask_pre_sensor[4] = 0x00;
			ask_pre_sensor[5] = 0x01;
		}
		if((flag_ask_fre == 1) || (flag_ask_fre == 5))//问温度2
		{
			ask_pre_sensor[1] = 0x03;
			ask_pre_sensor[2] = 0x00;
			ask_pre_sensor[3] = 0xb6;
			ask_pre_sensor[4] = 0x00;
			ask_pre_sensor[5] = 0x01;
		}
		if((flag_ask_fre == 2) || (flag_ask_fre == 6))//问压力1
		{
			ask_pre_sensor[1] = 0x03;
			ask_pre_sensor[2] = 0x00;
			ask_pre_sensor[3] = 0x16;
			ask_pre_sensor[4] = 0x00;
			ask_pre_sensor[5] = 0x01;
		}
		if((flag_ask_fre == 3) || (flag_ask_fre == 7))//问压力2
		{
			ask_pre_sensor[1] = 0x03;
			ask_pre_sensor[2] = 0x00;
			ask_pre_sensor[3] = 0x17;
			ask_pre_sensor[4] = 0x00;
			ask_pre_sensor[5] = 0x01;
		}
		gpio_26_low(); gpio_27_high();
		unsigned char *sucArray_pre;
		unsigned int SCRC = 0;
		sucArray_pre = ask_pre_sensor;

		SCRC = CRC_Test(sucArray_pre,8);
		*(sucArray_pre + 6) = ((SCRC & 0xff00) >> 8);
		*(sucArray_pre + 7) = (SCRC & 0x00ff);
		len_uart_fga = write(fd_uart_fga,ask_pre_sensor,8);
		msleep(10);
		gpio_26_high();gpio_27_low();

  //      QTimer::singleShot(300, this, SLOT(read_pressure_transmitters_485()));
		msleep(200);
		wait_fgaread_over.lock();
		read_pressure_transmitters_485();
		wait_fgaread_over.unlock();
	}

}
//读取压力变送器返回的数据 4-20ma
void FGA1000_485::read_pressure_transmitters()
{
	unsigned char *sucArray_pre = read_pre_sensor;
	int SCRC = 0;
	unsigned char CRCHi;
	unsigned char CRCLi;
	len_uart_fga = read(fd_uart_fga, read_pre_sensor, sizeof(read_pre_sensor));
	if(len_uart_fga > 0)//有数据返回，开始校验和数据处理
	{
		//qDebug()<<"read pre add "<<flag_ask_fre;
		SCRC = CRC_Test(sucArray_pre,7);
		CRCHi = ((SCRC & 0xff00) >> 8);
		CRCLi = (SCRC & 0x00ff);
		if((CRCHi == read_pre_sensor[5])&&(CRCLi == read_pre_sensor[6]))
		{
			//数据处理
			if(flag_ask_fre == 0)
			{
				temperature[0] = read_pre_sensor[3];
				temperature[1] = read_pre_sensor[4];
			}
			if(flag_ask_fre == 1)
			{
				temperature[2] = read_pre_sensor[3];
				temperature[3] = read_pre_sensor[4];
			}
			if(flag_ask_fre == 2)
			{
				pressure[0] = read_pre_sensor[3];
				pressure[1] = read_pre_sensor[4];
			}
			if(flag_ask_fre == 3)
			{
				pressure[2] = read_pre_sensor[3];
				pressure[3] = read_pre_sensor[4];
			}
			if(flag_ask_fre == 4)
			{
				temperature[4] = read_pre_sensor[3];
				temperature[5] = read_pre_sensor[4];
			}
			if(flag_ask_fre == 5)
			{
				temperature[6] = read_pre_sensor[3];
				temperature[7] = read_pre_sensor[4];
			}
			if(flag_ask_fre == 6)
			{
				pressure[4] = read_pre_sensor[3];
				pressure[5] = read_pre_sensor[4];
			}
			if(flag_ask_fre == 7)
			{
				pressure[6] = read_pre_sensor[3];
				pressure[7] = read_pre_sensor[4];
			}

			flag_ask_fre++;
			sta_pre[add_pre] = 0x00;//有数据返回则暂时判定为状态正常
			flag_uart_wro_pressure[add_pre] = 0;
		}
		else
		{
			flag_uart_wro_pressure[add_pre]++;//通讯故障
		}
	}
	else
	{
		flag_uart_wro_pressure[add_pre]++;//通讯故障

	}
	if(flag_uart_wro_pressure[add_pre] >= 3)//判断为通讯故障
	{
		sta_pre[add_pre] = 0x04;//标记为通讯故障
		flag_uart_wro_pressure[add_pre] = 0;
		flag_ask_fre++;
	}

	if((Pre_pipe_en == 0)&&(Pre_tank_en == 0)&&(Tem_tank_en == 1))
	{
		if(flag_ask_fre >= 2)
		{
			flag_fga_over = 0;//清零，接下来问可燃气体检测器
			flag_ask_fre = 0;
			floating_point_conversion();
			sta_pressure();
		}
	}
	if((Pre_pipe_en == 0)&&(Pre_tank_en == 1)&&(Tem_tank_en == 1))
	{
		if(flag_ask_fre >= 4)
		{
			flag_fga_over = 0;//清零，接下来问可燃气体检测器
			flag_ask_fre = 0;
			floating_point_conversion();
			sta_pressure();
		}
	}
	if((Pre_pipe_en == 0)&&(Pre_tank_en == 1)&&(Tem_tank_en == 0))
	{
		if(flag_ask_fre >= 4)
		{
			flag_fga_over = 0;//清零，接下来问可燃气体检测器
			flag_ask_fre = 2;
			floating_point_conversion();
			sta_pressure();
		}
	}
	if((Pre_pipe_en == 1)&&(Pre_tank_en == 0)&&(Tem_tank_en == 0))
	{
		if(flag_ask_fre >= 8)
		{
			flag_fga_over = 0;//清零，接下来问可燃气体检测器
			flag_ask_fre = 6;
			floating_point_conversion();
			sta_pressure();
		}
	}

	if((Pre_pipe_en == 1)&&(Pre_tank_en == 0)&&(Tem_tank_en == 1))
	{
		if(flag_ask_fre >= 8)
		{
			flag_fga_over = 0;//清零，接下来问可燃气体检测器
			flag_ask_fre = 0;
			floating_point_conversion();
			sta_pressure();
		}
		else if(flag_ask_fre == 2)
		{
			flag_ask_fre = 6;
		}

	}
	if((Pre_tank_en == 1)&&(Pre_pipe_en == 1)&&(Tem_tank_en == 0)) //如果是两个都有
	{
		if(flag_ask_fre >= 8)
		{
			flag_fga_over = 0;//清零，接下来问可燃气体检测器
			flag_ask_fre = 2;
			floating_point_conversion();
			sta_pressure();
		}
		else if(flag_ask_fre == 4)
		{
			flag_ask_fre = 6;
		}
	}
	if((Pre_tank_en == 1)&&(Pre_pipe_en == 1)&&(Tem_tank_en == 1)) //如果是两个都有
	{
		if(flag_ask_fre >= 8)
		{
			flag_fga_over = 0;//清零，接下来问可燃气体检测器
			flag_ask_fre = 0;
			floating_point_conversion();
			sta_pressure();
		}
		else if(flag_ask_fre == 4)
		{
			flag_ask_fre = 6;
		}
	}
}
//读取压力变送器返回的数据485
void FGA1000_485::read_pressure_transmitters_485()
{
	unsigned char *sucArray_pre = read_pre_sensor;
	int SCRC = 0;
	unsigned char CRCHi;
	unsigned char CRCLi;
	len_uart_fga = read(fd_uart_fga, read_pre_sensor, sizeof(read_pre_sensor));
	if(len_uart_fga > 0)//有数据返回，开始校验和数据处理
	{
		SCRC = CRC_Test(sucArray_pre,7);
		CRCHi = ((SCRC & 0xff00) >> 8);
		CRCLi = (SCRC & 0x00ff);
		if((CRCHi == read_pre_sensor[5])&&(CRCLi == read_pre_sensor[6]))
		{

			//数据处理
			if((Pre_tank_en == 1)&&(Pre_pipe_en == 1))
			{
				if(flag_ask_fre == 0)
				{
					temperature[0] = read_pre_sensor[3];
					temperature[1] = read_pre_sensor[4];
				}
				if(flag_ask_fre == 1)
				{
					temperature[2] = read_pre_sensor[3];
					temperature[3] = read_pre_sensor[4];
				}
				if(flag_ask_fre == 2)
				{
					pressure[0] = read_pre_sensor[3];
					pressure[1] = read_pre_sensor[4];
				}
				if(flag_ask_fre == 3)
				{
					pressure[2] = read_pre_sensor[3];
					pressure[3] = read_pre_sensor[4];
				}
				if(flag_ask_fre == 4)
				{
					temperature[4] = read_pre_sensor[3];
					temperature[5] = read_pre_sensor[4];
				}
				if(flag_ask_fre == 5)
				{
					temperature[6] = read_pre_sensor[3];
					temperature[7] = read_pre_sensor[4];
				}
				if(flag_ask_fre == 6)
				{
					pressure[4] = read_pre_sensor[3];
					pressure[5] = read_pre_sensor[4];
				}
				if(flag_ask_fre == 7)
				{
					pressure[6] = read_pre_sensor[3];
					pressure[7] = read_pre_sensor[4];
				}
			}
			if((Pre_tank_en == 1)&&(Pre_pipe_en == 0))
			{
				if(flag_ask_fre == 0)
				{
					temperature[0] = read_pre_sensor[3];
					temperature[1] = read_pre_sensor[4];
				}
				if(flag_ask_fre == 1)
				{
					temperature[2] = read_pre_sensor[3];
					temperature[3] = read_pre_sensor[4];
				}
				if(flag_ask_fre == 2)
				{
					pressure[0] = read_pre_sensor[3];
					pressure[1] = read_pre_sensor[4];
				}
				if(flag_ask_fre == 3)
				{
					pressure[2] = read_pre_sensor[3];
					pressure[3] = read_pre_sensor[4];
				}
			}
			if((Pre_tank_en == 0)&&(Pre_pipe_en == 1))
			{
				if(flag_ask_fre == 0)
				{
					temperature[4] = read_pre_sensor[3];
					temperature[5] = read_pre_sensor[4];
				}
				if(flag_ask_fre == 1)
				{
					temperature[6] = read_pre_sensor[3];
					temperature[7] = read_pre_sensor[4];
				}
				if(flag_ask_fre == 2)
				{
					pressure[4] = read_pre_sensor[3];
					pressure[5] = read_pre_sensor[4];
				}
				if(flag_ask_fre == 3)
				{
					pressure[6] = read_pre_sensor[3];
					pressure[7] = read_pre_sensor[4];
				}
			}
			flag_ask_fre++;
			sta_pre[add_pre] = 0x00;//有数据返回则暂时判定为状态正常
			flag_uart_wro_pressure[add_pre] = 0;
		}
		else
		{
			//flag_ask_fre++;
			flag_uart_wro_pressure[add_pre] ++;
		}

	}
	else
	{
		flag_uart_wro_pressure[add_pre]++;//通讯故障
	}
	if(flag_uart_wro_pressure[add_pre] >= 3)//判断为通讯故障
	{
		sta_pre[add_pre] = 0x04;//标记为通讯故障
		flag_uart_wro_pressure[add_pre] = 0;
		flag_ask_fre++;
	}

	if((flag_ask_fre >= 4)&&( ( (Pre_tank_en == 1)&&(Pre_pipe_en == 0) )||( (Pre_pipe_en == 1)&&(Pre_tank_en == 0) ) ))
	{
		flag_fga_over = 0;//清零，接下来问可燃气体检测器
		flag_ask_fre = 0;
		floating_point_conversion_485();
		sta_pressure();

		//emit data_show();//显示数值
	}

	if((flag_ask_fre >= 8)&&((Pre_tank_en == 1)&&(Pre_pipe_en == 1))) //如果是两个都有
	{
		flag_fga_over = 0;//清零，接下来问可燃气体检测器
		flag_ask_fre = 0;
		floating_point_conversion_485();
		sta_pressure();

		//emit data_show();//显示数值
	}
}

//浮点数转换4-20ma
void FGA1000_485::floating_point_conversion()
{
	if(Pre_tank_en == 1)
	{
		float pre_ma1 ;
		if((pressure[1]&0x80) != 0x00)//负数
		{
			pre_ma1 = (float((pressure[1]*65536+pressure[2]*256+pressure[3])-16777216)/8388607)*20*1.2;
			Pre[0] = (-pre_ma1 - 12)*625/1000;
		}
		else//正数
		{
			pre_ma1 = (float(pressure[1]*65536+pressure[2]*256+pressure[3])/8388607)*20*1.2;
			Pre[0] = (pre_ma1 - 12)*625/1000;
		}
		//Pre[0] = (pre_ma1 - 12)*625/1000;
		//qDebug()<<"youguan"<<pre_ma1<<pressure[1]<<pressure[2]<<pressure[3]<<Pre[0];
		if((Pre[0] < -20) || (Pre[0] > 20))
		{
			Pre[0] = 88.88;
		}
	}
	else
	{
		Pre[0] = 0.0000;
		if(Flag_Pressure_Transmitters_Mode == 0)
		{
			temperature[0] = 0;
			temperature[1] = 0;
			temperature[2] = 0;
			temperature[3] = 0;
			Tem[0] = 0.0000;//没有设置则数据清零
		}
		pressure[0] = 0;
		pressure[1] = 0;
		pressure[2] = 0;
		pressure[3] = 0;
	}
	if(Pre_pipe_en == 1)
	{
		float pre_ma2 ;
		if((pressure[5]&0x80) != 0x00)//负数
		{
			pre_ma2 = (float((pressure[5]*65536+pressure[6]*256+pressure[7])-16777216)/8388607)*20*1.2;
			Pre[1] = (-pre_ma2 - 12)*625/1000;
		}
		else
		{
			pre_ma2 = (float(pressure[5]*65536+pressure[6]*256+pressure[7])/8388607)*20*1.2;
			Pre[1] = (pre_ma2 - 12)*625/1000;
		}
		if((Pre[1] < -20) || (Pre[1] > 20))
		{
			Pre[1] = 88.88;
		}
	}
	else
	{
		pressure[4] = 0;
		pressure[5] = 0;
		pressure[6] = 0;
		pressure[7] = 0;
		Pre[1] = 0.0000;
		if(Flag_Pressure_Transmitters_Mode == 0)
		{
			Tem[1] = 0.0000;//没有设置则数据清零
			temperature[4] = 0;
			temperature[5] = 0;
			temperature[6] = 0;
			temperature[7] = 0;
		}
	}
	if(Tem_tank_en == 1)
	{
		float tem_ma ;
		if((temperature[1]&0x80) != 0x00)//负数
		{
			tem_ma = (float((temperature[1]*65536+temperature[2]*256+temperature[3])-16777216)/8388607)*20*1.2;
			Tem[0] = (((-tem_ma - 4)*9.375)-50);//-50到100℃   4-20ma
		}
		else
		{
			tem_ma = (float(temperature[1]*65536+temperature[2]*256+temperature[3])/8388607)*20*1.2;
			Tem[0] = (((tem_ma - 4)*9.375)-50);//-50到100℃   4-20ma
		}
		if((Tem[0] < -50) || (Tem[0] > 100))
		{
			Tem[0] = 88.88;
		}
	}
	else
	{
		temperature[0] = 0;
		temperature[1] = 0;
		temperature[2] = 0;
		temperature[3] = 0;
		Tem[0] = 0.0000;//没有设置则数据清零
	}
}
//浮点数转换 485
void FGA1000_485::floating_point_conversion_485()
{
	union{
		float f;
		char buf[4];
	}data;
	if(Pre_tank_en == 1)
	{
		data.buf[0] = temperature[3];
		data.buf[1] = temperature[2];
		data.buf[2] = temperature[1];
		data.buf[3] = temperature[0];
		if((data.f<200)&&(data.f>-200))//解决偶尔出现乱码
		{
			Tem[0] = data.f;  // 1号温度
		}
		//printf("****1  %.4f****\n",Tem[0]);
		data.buf[0] = pressure[3];
		data.buf[1] = pressure[2];
		data.buf[2] = pressure[1];
		data.buf[3] = pressure[0];
		if((data.f<200)&&(data.f>-200))//解决偶尔出现乱码
		{
			Pre[0] = data.f;  // 1号压力
		}
		//printf("****1  %.4f****\n",Pre[0]);
	}
	else
	{
		Tem[0] = 0.0000;//没有设置则数据清零
		Pre[0] = 0.0000;
		temperature[0] = 0;
		temperature[1] = 0;
		temperature[2] = 0;
		temperature[3] = 0;
		pressure[0] = 0;
		pressure[1] = 0;
		pressure[2] = 0;
		pressure[3] = 0;
	}
	if(Pre_pipe_en == 1)
	{
		data.buf[0] = temperature[7];
		data.buf[1] = temperature[6];
		data.buf[2] = temperature[5];
		data.buf[3] = temperature[4];
		if((data.f<200)&&(data.f>-200))//解决偶尔出现乱码
		{
			Tem[1] = data.f;  // 2号温度
		}
		//printf("****2  %.4f****\n",Tem[1]);
		data.buf[0] = pressure[7];
		data.buf[1] = pressure[6];
		data.buf[2] = pressure[5];
		data.buf[3] = pressure[4];
		if((data.f<200)&&(data.f>-200))//解决偶尔出现乱码
		{
			Pre[1] = data.f;  // 2号压力
		}
		//printf("****2  %.4f****\n",Pre[1]);
	}
	else
	{
		temperature[4] = 0;
		temperature[5] = 0;
		temperature[6] = 0;
		temperature[7] = 0;
		pressure[4] = 0;
		pressure[5] = 0;
		pressure[6] = 0;
		pressure[7] = 0;
		Tem[1] = 0.0000;//没有设置则数据清零
		Pre[1] = 0.0000;
	}

}

//压力变送器的状态发送（通信故障和正常）
void FGA1000_485::sta_pressure()
{
	if(Pre_tank_en == 1)
	{
		if(sta_pre[0] == 0x04)
		{
			if(Flag_Pressure_Transmitters_Mode == 0)//485
			{
				temperature[0] = 0;
				temperature[1] = 0;
				temperature[2] = 0;
				temperature[3] = 0;
				Tem[0] = 0.0000;//通讯故障则数据清零
			}
			Pre[0] = 0.0000;
			pressure[0] = 0;
			pressure[1] = 0;
			pressure[2] = 0;
			pressure[3] = 0;
//            printf("****************1 wrong**********\n");
		}
	}

	if(Pre_pipe_en == 1)
	{
		if(sta_pre[1] == 0x04)
		{

			pressure[4] = 0;
			pressure[5] = 0;
			pressure[6] = 0;
			pressure[7] = 0;
			Pre[1] = 0.0000;
			if(Flag_Pressure_Transmitters_Mode == 0)//485
			{
				temperature[4] = 0;
				temperature[5] = 0;
				temperature[6] = 0;
				temperature[7] = 0;
				Tem[0] = 0.0000;//通讯故障则数据清零
			}
//            printf("****************2 wrong**********\n");
		}
	}
	if(Tem_tank_en == 1)
	{

	}
}


void FGA1000_485::time_time()
{
	QDateTime date_time = QDateTime::currentDateTime();//当前时间
	unsigned int time_h = date_time.toString("hh").toInt();
	unsigned int time_m = date_time.toString("mm").toInt();
	unsigned int time_s = date_time.toString("ss").toInt();
	unsigned char flag_day_over = 0;
	//qDebug()<<time_m<<time_s<<"^^^^^^^^^^^^^^^^^^^^^^^^";
	//post添加
	QString postdata_tank;
	QString postdata_pipe;
	QString postdata_xieyou;//卸油浓度
	QString postdata_hcnd;//后处理浓度
	if((time_h == 0)&&(time_m == 30)&&(time_s == 0))//每天早上一点
	{
		flag_day_over = 1;
		day_over = 1;//调试用
	}
	//post添加
	if((time_h == 22))
	{
		Flag_Ifsend = 0;
	}
	if(Flag_MyserverFirstSend == 1)//我的服务器第一次连接发送数据
	{
		First_Send();
	}
	//下面用来发送各类信息，因为卡顿造成发送不成功在发送一次
	if((time_h == 0)&&(time_m == 0)&&(time_s == 0))//每天凌晨0点  有时候在这个函数进不来，所有后面又加了标志位，再发两次
	{
		Flag_Ifsend = 1;
		network_Warndata("1","N","N","N");//发送日报警信息
		network_Stagundata("0","N");      //日油枪状态
		network_Wrongsdata("1","N");      //日故障信息
		network_Configurationdata("N");   //日设置信息
		network_Closegunsdata("0","N","N","N","N");//关枪数据
		//Fga_StaPostSend();
	}
	if((time_h == 0)&&(time_m == 5)&&(time_s == 0))//每天凌晨0点  发送两次
	{
		if(Flag_Ifsend == 0)
		{
			Flag_Ifsend = 1;
			network_Warndata("1","N","N","N");//发送日报警信息
			network_Stagundata("0","N");      //日油枪状态
			network_Wrongsdata("1","N");      //日故障信息
			network_Configurationdata("N");   //日设置信息
			network_Closegunsdata("0","N","N","N","N");//关枪数据
			//Fga_StaPostSend();
		}
	}
	if((time_h == 0)&&(time_m == 10)&&(time_s == 0))//每天凌晨0点  发送两次
	{
		if(Flag_Ifsend == 0)
		{
			Flag_Ifsend = 1;
			network_Warndata("1","N","N","N");//发送日报警信息
			network_Stagundata("0","N");      //日油枪状态
			network_Wrongsdata("1","N");      //日故障信息
			network_Configurationdata("N");   //日设置信息
			network_Closegunsdata("0","N","N","N","N");//关枪数据
			//Fga_StaPostSend();
		}
	}
	if((time_h == 0)&&(time_m == 20)&&(time_s == 0))//每天凌晨0点  发送两次
	{
		if(Flag_Ifsend == 0)
		{
			Flag_Ifsend = 1;
			network_Warndata("1","N","N","N");//发送日报警信息
			network_Stagundata("0","N");      //日油枪状态
			network_Wrongsdata("1","N");      //日故障信息
			network_Configurationdata("N");   //日设置信息
			network_Closegunsdata("0","N","N","N","N");//关枪数据
			//Fga_StaPostSend();
		}
	}
	if((time_h == 0)&&(time_m == 30)&&(time_s == 0))//每天凌晨0点  发送两次
	{
		if(Flag_Ifsend == 0)
		{
			Flag_Ifsend = 1;
			network_Warndata("1","N","N","N");//发送日报警信息
			network_Stagundata("0","N");      //日油枪状态
			network_Wrongsdata("1","N");      //日故障信息
			network_Configurationdata("N");   //日设置信息
			network_Closegunsdata("0","N","N","N","N");//关枪数据
			//Fga_StaPostSend();
		}
	}
	if((time_h == 0)&&(time_m == 50)&&(time_s == 0))//每天凌晨0点  发送两次
	{
		if(Flag_Ifsend == 0)
		{
			Flag_Ifsend = 1;
			network_Warndata("1","N","N","N");//发送日报警信息
			network_Stagundata("0","N");      //日油枪状态
			network_Wrongsdata("1","N");      //日故障信息
			network_Configurationdata("N");   //日设置信息
			network_Closegunsdata("0","N","N","N","N");//关枪数据
			//Fga_StaPostSend();
		}
	}
	if((time_h == 0)&&(time_m == 59)&&(time_s == 0))//每天凌晨0点  发送两次
	{
		if(Flag_Ifsend == 0)
		{
			Flag_Ifsend = 1;
			network_Warndata("1","N","N","N");//发送日报警信息
			network_Stagundata("0","N");      //日油枪状态
			network_Wrongsdata("1","N");      //日故障信息
			network_Configurationdata("N");   //日设置信息
			network_Closegunsdata("0","N","N","N","N");//关枪数据
			//Fga_StaPostSend();
		}
	}
	if((time_h == 1)&&(time_m == 30)&&(time_s == 0))//每天凌晨0点  发送两次
	{
		if(Flag_Ifsend == 0)
		{
			Flag_Ifsend = 1;
			network_Warndata("1","N","N","N");//发送日报警信息
			network_Stagundata("0","N");      //日油枪状态
			network_Wrongsdata("1","N");      //日故障信息
			network_Configurationdata("N");   //日设置信息
			network_Closegunsdata("0","N","N","N","N");//关枪数据
			//Fga_StaPostSend();
		}
	}
	if((time_h == 2)&&(time_m == 0)&&(time_s == 0))//每天凌晨0点  发送两次
	{
		if(Flag_Ifsend == 0)
		{
			Flag_Ifsend = 1;
			network_Warndata("1","N","N","N");//发送日报警信息
			network_Stagundata("0","N");      //日油枪状态
			network_Wrongsdata("1","N");      //日故障信息
			network_Configurationdata("N");   //日设置信息
			network_Closegunsdata("0","N","N","N","N");//关枪数据
			//Fga_StaPostSend();
		}
	}
	Flag_Count++;
	if(!(Flag_Count%5))
	{
		//printf("5s\n");
		while(Flag_Sync)
		{
			printf("FGA UART BUSY!\n");
		}
		if( day_over == 1 ){flag_day_over = 1;}//按钮调试过一天，后期屏蔽掉

		//可燃气体检测部分
		unsigned char flag_beeptimer = 0;
		for(int i = 1; i<=num_fga_this; i++)
		{
			if(i <= 2)
			{
				if((sta_fga[i] == 0x00)||(Env_Gas_en == 0))
				{

					if((sta_fga[i] != Flag_StaFga_Temp[i])&&(Flag_StaFga_Temp[i]!=0x09))
					{
						Flag_StaFga_Temp[i] = sta_fga[i];
						emit normal_fga(i);//正常
						add_value_reoilgaswarn("油气浓度","设备正常");
						network_Warndata("0","N","N","0");//发送报警信息
					}

					flag_fga_warn[i] = 0;
					Flag_Timeto_CloseNeeded[4] = 0;
				}
				else
				{
					if(sta_fga[i] == 0x01)
					{
						flag_fga_warn[i]++;
						if(flag_fga_warn[i] >= 60)//5分钟
						{
							sta_fga[i] = 0x02;//报警
							flag_fga_warn[i] = 61;

							if(sta_fga[i] != Flag_StaFga_Temp[i])
							{
								Flag_StaFga_Temp[i] = sta_fga[i];
								emit alarm_hig_fga(i);//报警
								add_value_reoilgaswarn("油气浓度","浓度报警");
								network_Warndata("0","N","N","2");//发送报警信息
							}

							Flag_Timeto_CloseNeeded[4] = 1;
						}
						else
						{
							if(sta_fga[i] != Flag_StaFga_Temp[i])
							{
								Flag_StaFga_Temp[i] = sta_fga[i];
								emit alarm_low_fga(i);//预警
								add_value_reoilgaswarn("油气浓度","浓度预警");
								network_Warndata("0","N","N","1");//发送报警信息
							}
						}
					}
					if(sta_fga[i] == 0x03)
					{

						if(sta_fga[i] != Flag_StaFga_Temp[i])
						{
							Flag_StaFga_Temp[i] = sta_fga[i];
							emit alarm_sensor_fga(i);//传感器故障
							add_value_reoilgaswarn("油气浓度","传感器故障");
						}

						flag_fga_warn[i] = 0;
						Flag_Timeto_CloseNeeded[4] = 1;
					}
					if(sta_fga[i] == 0x04)
					{

						if(sta_fga[i] != Flag_StaFga_Temp[i])
						{
							Flag_StaFga_Temp[i] = sta_fga[i];
							emit alarm_uart_fga(i);//通讯故障
							add_value_reoilgaswarn("油气浓度","通信故障");
							network_Wrongsdata("0","041000");//网络上传
						}

						flag_fga_warn[i] = 0;
						Flag_Timeto_CloseNeeded[4] = 1;
					}
					if(sta_fga[i] == 0x05)
					{

						if(sta_fga[i] != Flag_StaFga_Temp[i])
						{
							Flag_StaFga_Temp[i] = sta_fga[i];
							emit alarm_sensor_de_fga(i);//探测传感器故障
							add_value_reoilgaswarn("油气浓度","探测传感器故障");
						}

						flag_fga_warn[i] = 0;
						Flag_Timeto_CloseNeeded[4] = 1;
					}
				}
				i++;//空过2号油气浓度传感器
			}
			else
			{
				if((sta_fga[i] == 0x00) && (sta_fga[i] != Flag_StaFga_Temp[i]))
				{
					emit normal_fga(i);//正常
					add_value_fgainfo(QString("%1# 传感器").arg(i-2),"设备正常");
					Flag_StaFga_Temp[i] = sta_fga[i];
				}
				if((sta_fga[i] == 0x01) && (sta_fga[i] != Flag_StaFga_Temp[i]))
				{
					emit alarm_low_fga(i);//预警   低限位报警
					add_value_fgainfo(QString("%1# 传感器").arg(i-2),"低限报警");
					Flag_StaFga_Temp[i] = sta_fga[i];
				}
				if((sta_fga[i] == 0x02) && (sta_fga[i] != Flag_StaFga_Temp[i]))
				{
					emit alarm_hig_fga(i);//报警    高限位报警
					add_value_fgainfo(QString("%1# 传感器").arg(i-2),"高限报警");
					Flag_StaFga_Temp[i] = sta_fga[i];
				}
				if((sta_fga[i] == 0x03) && (sta_fga[i] != Flag_StaFga_Temp[i]))
				{
					emit alarm_sensor_fga(i);//传感器故障
					add_value_fgainfo(QString("%1# 传感器").arg(i-2),"传感器故障");
					Flag_StaFga_Temp[i] = sta_fga[i];
				}
				if((sta_fga[i] == 0x04) && (sta_fga[i] != Flag_StaFga_Temp[i]))
				{
					emit alarm_uart_fga(i);//通讯故障
					add_value_fgainfo(QString("%1# 传感器").arg(i-2),"通信故障");
					if(i == 3){network_Wrongsdata("0","041000");}//卸油口油气浓度
					Flag_StaFga_Temp[i] = sta_fga[i];
				}
				if((sta_fga[i] == 0x05) && (sta_fga[i] != Flag_StaFga_Temp[i]))
				{
					emit alarm_sensor_de_fga(i);//探测传感器故障
					add_value_fgainfo(QString("%1# 传感器").arg(i-2),"通信故障");
					Flag_StaFga_Temp[i] = sta_fga[i];
				}
				flag_beeptimer+=sta_fga[i];
			}
			//        printf("   GAS%d 0x%02x",i,sta_fga[i]);
		}
		if(flag_beeptimer)//屏蔽了蜂鸣器滴滴滴滴报警
		{
			//beep_timer();
		}
		else
		{
			//beep_none();
		}
		//压力变送器 部分 罐压力
//        if(Pre[0] >= 88)
//        {
//            sta_pre[0] = 0x04;//通信故障
//        }
		if(sta_pre[0] == 0x04)//通讯故障
		{

		}
		else
		{

			if((Pre[0] > -0.05)&&(Pre[0] < 0.05))    //第一种报警
			{
				time_sec_tankpre++;
				if(time_sec_tankpre >= 8640) //12 hours  43200秒
				{
					time_sec_tankpre = 8640;
					sta_pre[0] = 0x01;
					Sta_Tank_Postsend = 1;//零压预警
					flag_warnpre_day = 1;//预报警标志位
					if(time_day_tankpre >= 4)
					{
						sta_pre[0] = 0x02;
						Sta_Tank_Postsend = 2;//零压报警
					}

				}
			}
			else
			{
				time_sec_tankpre = 0;

				if((Pre[0]*1000 > Positive_Pres+200)||(Pre[0]*1000 < Negative_Pres-200))//第二种报警
				{
					//qDebug()<<"PV----wrong";
					sta_pre[0] = 0x01;
					Sta_Tank_Postsend = 3;//压力/真空阀极限状态预警
					time_sec_tankpre2++;
					if(time_sec_tankpre2 >= 60)
					{
						sta_pre[0] = 0x02;
						time_sec_tankpre2 = 60;
						Sta_Tank_Postsend = 4;//压力/真空阀极限状态报警
					}
				}
				else
				{
					sta_pre[0] = 0x00;
					Sta_Tank_Postsend = 0;//油罐压力正常
					time_sec_tankpre2 = 0;
				}
			}
			//2020-01-08 根据报批稿取消此类报警，如果恢复直接取消注释即可
//            if((Pre[0]*1000 > 600)||(Pre[0]*1000 > Positive_Pres*0.8)||(Pre[0]*1000 < -1000))//第三种报警
//            {
//                time_sec_tankpre3++;
//                if((time_sec_tankpre3 > 4320)&&(sta_pre[0] != 0x02)) //一天的25%,而且前两次判断不是报警状态
//                {
//                    sta_pre[0] = 0x01;
//                    Sta_Tank_Postsend = 5;//压力/真空阀状态预警
//                    flag_warnpre_day3 = 1; //预报警标志位
//                    if(time_day_tankpre3 >= 4)//第五天仍然是预警则报警
//                    {
//                        sta_pre[0] = 0x02;
//                        Sta_Tank_Postsend = 6;//压力/真空阀状态报警
//                    }
//                }
//            }
		}
		//管线压力，液阻相关
		if(sta_pre[1] == 0x04)//通讯故障
		{
			time_line_pressure = 0;
		}
		else
		{
			float Maximum_line_pressure = 0;//最高管线压力
			float line_pressure = (Pre[1]-Pre[0])*1000;//比较用的压力是管线压力减去罐压
			if(Speed_fargas > 20)//流速大于20L/Min才能用
			{
				Maximum_line_pressure = (Speed_fargas*5.75)-66;//计算得到最大压力，单位pa
			}
			else
			{
				Maximum_line_pressure = 4000;//如果是0或者不合格，则将其设置为一个比较大的值，起到屏蔽作用
			}
			if(line_pressure > Maximum_line_pressure)
			{
				time_line_pressure++;
				if(time_line_pressure>=3600)//一个小时
				{
					sta_pre[1] = 0x02;//报警
					time_line_pressure = 3602;
				}
				else
				{
					sta_pre[1] = 0x01;//预警
				}
			}
			else
			{
				sta_pre[1] = 0x00;//正常
				time_line_pressure = 0;
			}
		}
		//一天结束
		if(flag_day_over == 1)
		{
			//第一种报警跨天
			if(flag_warnpre_day == 1)
			{
				time_day_tankpre++;
				flag_warnpre_day = 0;
				time_sec_tankpre = 0;
			}
			else
			{
				time_day_tankpre = 0;
				flag_warnpre_day = 0;
				time_sec_tankpre = 0;
			}
			//第三种报警跨天
			if(flag_warnpre_day3 == 1)
			{
				flag_warnpre_day3 = 0;//报警标志位
				time_sec_tankpre3 = 0;//每天报警次数
				time_day_tankpre3++;//报警天数
			}
			else
			{
				flag_warnpre_day3 = 0;//报警标志位
				time_sec_tankpre3 = 0;//每天报警次数
				time_day_tankpre3 = 0;//报警天数
			}

			config_fga_accum_write(time_day_tankpre,time_day_tankpre3);
		}
		unsigned char flag_timeto_temp = 0;
		for(uchar i = 0; i < 3; i++)
		{
			if((Pre_tank_en == 1)&&(i == 0))
			{
				if(sta_pre[i] == 0x00)//正常
				{
					if((sta_pre[i] != Flag_StaPre_Temp[i])&&(Flag_StaPre_Temp[i]!=0x09))
					{
						emit normal_pressure(1);
						Flag_StaPre_Temp[i] = sta_pre[i];
						add_value_reoilgaswarn("油罐压力","设备正常");
						//add_value_reoilgaswarn("油罐温度","设备正常");
						//post添加
						network_Warndata("0","0","N","N");
					}
				}
				if(sta_pre[i] == 0x01)//预警
				{
					if(sta_pre[i] != Flag_StaPre_Temp[i])
					{
						emit alarm_early_pre(1);
						Flag_StaPre_Temp[i] = sta_pre[i];
						if(Sta_Tank_Postsend == 1)
						{
							his_tem = "零压预警，油罐压力处于-50～+50Pa时间大于12小时";
						}
						else if(Sta_Tank_Postsend == 3)
						{
							his_tem = "压力预警，油罐压力大于泄压阀开启压力+200Pa";
						}
						else if(Sta_Tank_Postsend == 5)
						{
							his_tem = "压力预警，在24小时内，油罐压力数据在-1000Pa～+600Pa(或-1000Pa～压力/真空阀正压开启压力的80%)之外的次数超过总次数的25%";
						}
						add_value_reoilgaswarn("油罐压力",his_tem);
						//post添加
						network_Warndata("0",QString::number(Sta_Tank_Postsend),"N","N");
					}
					flag_timeto_temp+=sta_pre[i];
				}
				if(sta_pre[i] == 0x02)//报警
				{
					if(sta_pre[i] != Flag_StaPre_Temp[i])
					{
						emit alarm_warn_pre(1);
						Flag_StaPre_Temp[i] = sta_pre[i];
						if(Sta_Tank_Postsend == 2)
						{
							his_tem = "零压报警，连续5天零压预警";
						}
						else if(Sta_Tank_Postsend == 4)
						{
							his_tem = "压力报警，连续5分钟油罐压力大于泄压阀开启压力+200Pa";
						}
						else if(Sta_Tank_Postsend == 6)
						{
							his_tem = "压力报警，连续7天在24小时内，油罐压力数据在-1000Pa～+600Pa(或-1000Pa～压力/真空阀正压开启压力的80%)之外的次数超过总次数的25%";
						}
						add_value_reoilgaswarn("油罐压力",his_tem);
						network_Warndata("0",QString::number(Sta_Tank_Postsend),"N","N");
					}
					flag_timeto_temp+=sta_pre[i];
				}
				if(sta_pre[i] == 0x04)//通讯故障
				{
					if(sta_pre[i] != Flag_StaPre_Temp[i])
					{
						emit alarm_uart_pressure(1);
						Flag_StaPre_Temp[i] = sta_pre[i];
						add_value_reoilgaswarn("油罐压力","通信故障");
						//post添加
						network_Wrongsdata("0","031001");
					}
					flag_timeto_temp+=sta_pre[i];
				}
				//            printf("   PRE%d 0x%02x",i,sta_pre[i]);
			}
			if((Pre_pipe_en == 1)&&(i == 1))
			{
				if(sta_pre[i] == 0x00)//正常
				{
					if((sta_pre[i] != Flag_StaPre_Temp[i])&&(Flag_StaPre_Temp[i]!=0x09))
					{
						emit normal_pressure(2);
						Flag_StaPre_Temp[i] = sta_pre[i];
						add_value_reoilgaswarn("管线压力","设备正常");
						network_Warndata("0","N","0","N");
						//post添加
					}
				}
				if(sta_pre[i] == 0x01)//预警
				{
					if(sta_pre[i] != Flag_StaPre_Temp[i])
					{
						emit alarm_early_pre(2);
						Flag_StaPre_Temp[i] = sta_pre[i];
						add_value_reoilgaswarn("管线压力","压力预警");
						network_Warndata("0","N","1","N");
						//post添加
					}
					flag_timeto_temp+=sta_pre[i];
				}
				if(sta_pre[i] == 0x02)//报警
				{
					if(sta_pre[i] != Flag_StaPre_Temp[i])
					{
						emit alarm_warn_pre(2);
						Flag_StaPre_Temp[i] = sta_pre[i];
						add_value_reoilgaswarn("管线压力","压力报警");
						network_Warndata("0","N","2","N");
						//post添加
					}
					flag_timeto_temp+=sta_pre[i];
				}
				if(sta_pre[i] == 0x04)//通讯故障
				{
					if(sta_pre[i] != Flag_StaPre_Temp[i])
					{
						emit alarm_uart_pressure(2);
						Flag_StaPre_Temp[i] = sta_pre[i];
						add_value_reoilgaswarn("管线压力","通信故障");
						//post添加
						network_Wrongsdata("0","031002");
					}
					flag_timeto_temp+=sta_pre[i];
				}
			}
			if((Tem_tank_en == 1)&&((Flag_Pressure_Transmitters_Mode == 1)||(Flag_Pressure_Transmitters_Mode == 2))&&(i == 2))
			{
				if(sta_pre[i] == 0x04)
				{
					if(sta_pre[i] != Flag_StaPre_Temp[i])
					{
						Flag_StaPre_Temp[i] = sta_pre[i];
						emit alarm_tem_warn();
						add_value_reoilgaswarn("油气温度传感器","通信故障");
                        //post添加
                        network_Wrongsdata("0","051000");
					}
					flag_timeto_temp+=sta_pre[i];

				}
				else
				{
					if((sta_pre[i] != Flag_StaPre_Temp[i])&&(Flag_StaPre_Temp[i]!=0x09))
					{
						emit alarm_tem_normal();
						Flag_StaPre_Temp[i] = sta_pre[i];
						add_value_reoilgaswarn("油气温度传感器","设备正常");
					}
				}
			}
		}
		Flag_Timeto_CloseNeeded[3] = flag_timeto_temp;
		//显示信号
		emit data_show();

		flag_day_over = 0;
		day_over = 0; //调试用
	}
	if(Flag_Count > 30)
	{
		Flag_Count = 0;
	}
	//30S记录一次
	if(!(time_s%30))
	{
		if(Pre_pipe_en||Pre_tank_en||Env_Gas_en)
		{
			add_value_envinfo(Pre[0],Pre[1],Tem[0],Gas_Concentration_Fga[1]);
		}
		postdata_tank = "NULL";
		postdata_pipe = "NULL";
		postdata_xieyou = "NULL";//卸油浓度
		postdata_hcnd = "NULL";//后处理浓度
		if(Pre_pipe_en == 0)
		{
			postdata_tank = "NULL";
		}
		else
		{
			postdata_tank = QString::number(Pre[0]*1000,'f',1);
		}
		if(Pre_pipe_en == 0)
		{
			postdata_pipe = "NULL";
		}
		else
		{
			postdata_pipe = QString::number(Pre[1]*1000,'f',1);
		}
		if(Num_Fga >= 3)
		{
			postdata_xieyou = QString::number(Gas_Concentration_Fga[3],'f',1);
		}
		else
		{
			postdata_xieyou = "NULL";
		}
		if(Env_Gas_en == 1)
		{
			postdata_hcnd = QString::number(Gas_Concentration_Fga[1],'f',1);
		}
		else
		{
			postdata_hcnd = "NULL";
		}
		network_Surroundingsdata("0",postdata_tank,postdata_pipe,"NULL",postdata_xieyou,postdata_hcnd,"NULL");
	}
}




void FGA1000_485::gpio_26_high()
{
	//printf("high\n");
	int fd_export = open(EXPORT_PATH_FGA,O_WRONLY);   //打开gpio设备导出设备
	if(fd_export < 0)
	{
		perror("open export error");
	}
	write(fd_export,RADAR_CON_26,strlen(RADAR_CON_26));   //向export文件写入gpio排列序号字符串
	int fd_dir;
	int ret;
	fd_dir = open(DIRECT_PATH_26,O_RDWR);
	if(fd_dir < 0)
	{
		perror("open io error!");
	}
	ret = write(fd_dir,IO_DIRECTOUT,sizeof(IO_DIRECTOUT));
	if(ret < 0)
	{
		perror("write direct error!");
	}
	int fd_val;
	fd_val = open(VALUE_PATH_26,O_RDWR);
	if(fd_val < 0)
	{
		perror("value set error!");
	}
	ret = write(fd_val,VALUE_HIGH,strlen(VALUE_HIGH));
	if(ret < 0)
	{
		perror("write value error!");
	}
	close(fd_export);
	close(fd_dir);
	close(fd_val);
}
void FGA1000_485::gpio_26_low()
{
	int fd_export = open(EXPORT_PATH_FGA,O_WRONLY);   //打开gpio设备导出设备
	if(fd_export < 0)
	{
		perror("open export error");
	}
	write(fd_export,RADAR_CON_26,strlen(RADAR_CON_26));   //向export文件写入gpio排列序号字符串
	int fd_dir;
	int ret;
	fd_dir = open(DIRECT_PATH_26,O_RDWR);
	if(fd_dir < 0)
	{
		perror("open io error!");
	}
	ret = write(fd_dir,IO_DIRECTOUT,sizeof(IO_DIRECTOUT));
	if(ret < 0)
	{
		perror("write direct error!");
	}
	int fd_val;
	fd_val = open(VALUE_PATH_26,O_RDWR);
	if(fd_val < 0)
	{
		perror("value set error!");
	}
	ret = write(fd_val,VALUE_LOW,strlen(VALUE_LOW));
	if(ret < 0)
	{
		perror("write value error!");
	}
	close(fd_export);
	close(fd_dir);
	close(fd_val);
}

//点亮指示灯，数据读取时亮一下
void FGA1000_485::gpio_27_high()
{
	//printf("high\n");
	int fd_export = open(EXPORT_PATH_FGA,O_WRONLY);   //打开gpio设备导出设备
	if(fd_export < 0)
	{
		perror("open export error");
	}
	write(fd_export,RADAR_CON_27,strlen(RADAR_CON_27));   //向export文件写入gpio排列序号字符串
	int fd_dir;
	int ret;
	fd_dir = open(DIRECT_PATH_27,O_RDWR);
	if(fd_dir < 0)
	{
		perror("open io error!");
	}
	ret = write(fd_dir,IO_DIRECTOUT,sizeof(IO_DIRECTOUT));
	if(ret < 0)
	{
		perror("write direct error!");
	}
	int fd_val;
	fd_val = open(VALUE_PATH_27,O_RDWR);
	if(fd_val < 0)
	{
		perror("value set error!");
	}
	ret = write(fd_val,VALUE_HIGH,strlen(VALUE_HIGH));
	if(ret < 0)
	{
		perror("write value error!");
	}
	close(fd_export);
	close(fd_dir);
	close(fd_val);
}
void FGA1000_485::gpio_27_low()
{
	//printf("low\n");
	int fd_export = open(EXPORT_PATH_FGA,O_WRONLY);   //打开gpio设备导出设备
	if(fd_export < 0)
	{
		perror("open export error");
	}
	write(fd_export,RADAR_CON_27,strlen(RADAR_CON_27));   //向export文件写入gpio排列序号字符串
	int fd_dir;
	int ret;
	fd_dir = open(DIRECT_PATH_27,O_RDWR);
	if(fd_dir < 0)
	{
		perror("open io error!");
	}
	ret = write(fd_dir,IO_DIRECTOUT,sizeof(IO_DIRECTOUT));
	if(ret < 0)
	{
		perror("write direct error!");
	}
	int fd_val;
	fd_val = open(VALUE_PATH_27,O_RDWR);
	if(fd_val < 0)
	{
		perror("value set error!");
	}
	ret = write(fd_val,VALUE_LOW,strlen(VALUE_LOW));
	if(ret < 0)
	{
		perror("write value error!");
	}
	close(fd_export);
	close(fd_dir);
	close(fd_val);
}

void FGA1000_485::testday()
{
	day_over = 1;
	printf("*************************************\n");
}

//post添加  20191021弃用
void FGA1000_485::Fga_WarnPostSend(QString which, QString sta)
{
	which = which;
	sta = sta;
}
//20191021弃用
void FGA1000_485::Fga_StaPostSend()
{

}
/****************网络发送油气回收报警状态*********************
 * id     0 单次报警数据  1全天报警数据
 * sta_yg 油罐状态  0-正常 1-零压预警 2-零压报警 3-真空阀极限预警 4-真空阀极限报警 5-真空阀预警 6-真空阀报警
 * sta_yz 液阻状态  0-正常 1-预警 2-报警
 * hclzt  预留
 * ***********************************/
void FGA1000_485:: network_Warndata(QString id,QString sta_yg,QString sta_yz,QString hclzt) //发送报警数据报文
{
	if(1)//如果网线连接
	{
		qDebug()<<"network send warndata!"<< Flag_Network_Send_Version;
		QString al_post;
		QString sta_gun;
		QString gun_num;
		QString send_gun_sta = "";
		unsigned int num_gun = 0;

		if(Flag_Network_Send_Version == 0) //福建协议
		{
			if(id.toInt() == 0)//单次报警数据
			{
				if(sta_yg != "N")//发送油罐相关的报警
				{
					if(sta_yg == "0")
					{
						Send_Warndata(DATAID_POST,"N","0","N","0","0","0","N");
					}
					if(sta_yg == "1")//油罐零压预警
					{
						Send_Warndata(DATAID_POST,"N","0","N","1","0","0","N");
					}
					if(sta_yg == "2")//油罐零压报警
					{
						Send_Warndata(DATAID_POST,"N","0","N","2","0","0","N");
					}
					if(sta_yg == "3")//压力真空阀临界预警
					{
						Send_Warndata(DATAID_POST,"N","0","N","0","0","1","N");
					}
					if(sta_yg == "4")//压力真空阀临界报警
					{
						Send_Warndata(DATAID_POST,"N","0","N","0","0","2","N");
					}
					if(sta_yg == "5")//压力真空阀预警
					{
						Send_Warndata(DATAID_POST,"N","0","N","0","1","0","N");
					}
					if(sta_yg == "6")//压力真空阀报警
					{
						Send_Warndata(DATAID_POST,"N","0","N","0","2","0","N");
					}
				}
				if(sta_yz != "N")//发送液阻报警
				{
					if(sta_yz == "0")
					{
						Send_Warndata(DATAID_POST,"N","0","0","N","N","N","N");
					}
					if(sta_yz == "1")
					{
						Send_Warndata(DATAID_POST,"N","0","1","N","N","N","N");
					}
					if(sta_yz == "2")
					{
						Send_Warndata(DATAID_POST,"N","0","2","N","N","N","N");
					}
				}
				if(hclzt != "N")//浓度报警
				{
					if(hclzt == "0")
					{
						Send_Warndata(DATAID_POST,"N","N","N","N","N","N","0");
					}
					if(hclzt == "1")
					{
						Send_Warndata(DATAID_POST,"N","N","N","N","N","N","1");
					}
					if(hclzt == "2")
					{
						Send_Warndata(DATAID_POST,"N","N","N","N","N","N","2");
					}
				}
			}
			if(id.toInt() == 1)//当天报警数据
			{
				/**************begin***网络报警数据*******************/
				printf("ready send one day data !!\n");
				STA_YGLY = "0";
				STA_PVLJZT = "0";
				STA_PVZT = "0";
				STA_YZ = "0";
				if(Pre_tank_en == 1)
				{

					if(Sta_Tank_Postsend == 0)  //正常
					{
						STA_YGLY = "0";
						STA_PVLJZT = "0";
						STA_PVZT = "0";
					}
					if(Sta_Tank_Postsend == 1)//零压预警
					{
						STA_YGLY = "1";
					}
					if(Sta_Tank_Postsend == 2)//零压报警
					{
						STA_YGLY = "2";
					}
					if(Sta_Tank_Postsend == 3)//泄压阀极限压力状态预警
					{
						STA_PVLJZT = "1";
					}
					if(Sta_Tank_Postsend == 4)//泄压阀极限压力状态报警
					{
						STA_PVLJZT = "2";
					}
					if(Sta_Tank_Postsend == 5)//压力真空阀状态预警
					{
						STA_PVZT = "1";
					}
					if(Sta_Tank_Postsend == 9)//压力真空阀状态报警
					{
						STA_PVZT = "2";
					}
				}
				else
				{
					STA_YGLY = "N";
					STA_PVLJZT = "N";
					STA_PVZT = "N";
				}
				if(Pre_pipe_en == 1)
				{
					if(sta_pre[1] == 0)
					{
						STA_YZ = "0";
					}
					if(sta_pre[1] == 1)
					{
						STA_YZ = "1";
					}
					if(sta_pre[1] == 2)
					{
						STA_YZ = "2";
					}
				}
				else
				{
					STA_YZ = "N";
				}
				if(Env_Gas_en == 1)
				{
					if(Flag_StaFga_Temp[1] == 0)
					{
						STA_ND = "0";
					}
					if(Flag_StaFga_Temp[1] == 1)
					{
						STA_ND = "1";
					}
					if(Flag_StaFga_Temp[1] == 2)
					{
						STA_ND = "2";
					}
				}
				else
				{
					STA_ND = "N";
				}

				for(unsigned int i = 0;i < Amount_Dispener;i++)
				{
					printf("!!!!!!!!!%d",Amount_Gasgun[i]);
					for(unsigned int j = 0;j < Amount_Gasgun[i];j++)
					{
						num_gun++;
						if(Flag_Accumto[i][j] == 0)
						{
							sta_gun = "0";
						}
						if((Flag_Accumto[i][j] > 0)&&(Flag_Accumto[i][j] <= 5))
						{
							sta_gun = "1";
						}
						if(Flag_Accumto[i][j] > 5)
						{
							sta_gun = "2";
						}
						al_post.append(QString::number(num_gun).append(":").append(sta_gun).append(";"));
					}
				}
				num_gun = 0;
				Send_Warndata(DATAID_POST,al_post,"0",STA_YZ,STA_YGLY,STA_PVZT,STA_PVLJZT,"0");
				/**************end***网络报警数据*******************/
			}
		}
		if(Flag_Network_Send_Version == 1) //广州协议
		{
			if(id.toInt() == 0)//单次报警数据
			{
				if(sta_yg != "N")//发送油罐相关的报警
				{
					if(sta_yg == "0")
					{
						emit gun_warn_data("q0000=N","0","N","0","N","0","N","N");
					}
					if(sta_yg == "1")//油罐零压预警
					{
						//Send_Warndata(DATAID_POST,"N","0","N","1","0","0","0");
					}
					if(sta_yg == "2")//油罐零压报警
					{
						//Send_Warndata(DATAID_POST,"N","0","N","2","0","0","0");
					}
					if(sta_yg == "3")//压力真空阀临界预警
					{
						emit gun_warn_data("q0000=N","0","N","0","N","1","N","N");
					}
					if(sta_yg == "4")//压力真空阀临界报警
					{
						emit gun_warn_data("q0000=N","0","N","0","N","2","N","N");
					}
					if(sta_yg == "5")//压力真空阀预警
					{
						emit gun_warn_data("q0000=N","0","N","1","N","0","N","N");
					}
					if(sta_yg == "6")//压力真空阀报警
					{
						emit gun_warn_data("q0000=N","0","N","2","N","0","N","N");
					}
				}
				if(sta_yz != "N")//发送液阻报警
				{
					if(sta_yz == "0")
					{
						emit gun_warn_data("q0000=N","0","0","N","N","N","N","N");
					}
					if(sta_yz == "1")
					{
						emit gun_warn_data("q0000=N","0","1","N","N","N","N","N");
					}
					if(sta_yz == "2")
					{
						emit gun_warn_data("q0000=N","0","2","N","N","N","N","N");
					}
				}
				if(hclzt != "N")
				{
					if(hclzt == "0")
					{
						emit gun_warn_data("q0000=N","0","N","N","0","N","N","N");
					}
					if(hclzt == "1")
					{
						emit gun_warn_data("q0000=N","0","N","N","1","N","N","N");
					}
					if(hclzt == "2")
					{
						emit gun_warn_data("q0000=N","0","N","N","2","N","N","N");
					}
				}
			}
			if(id.toInt() == 1)//当天报警数据
			{
				/**************begin***网络报警数据*******************/
				printf("ready send one day data !!\n");

				STA_YGLY = "0";
				STA_PVLJZT = "0";
				STA_PVZT = "0";
				STA_YZ = "0";
				STA_ND = "0";
				if(Sta_Tank_Postsend == 0)  //正常
				{
					STA_YGLY = "0";
					STA_PVLJZT = "0";
					STA_PVZT = "0";
				}
				if(Sta_Tank_Postsend == 1)//零压预警
				{
					STA_YGLY = "1";
				}
				if(Sta_Tank_Postsend == 2)//零压报警
				{
					STA_YGLY = "2";
				}
				if(Sta_Tank_Postsend == 3)//泄压阀极限压力状态预警
				{
					STA_PVLJZT = "1";
				}
				if(Sta_Tank_Postsend == 4)//泄压阀极限压力状态报警
				{
					STA_PVLJZT = "2";
				}
				if(Sta_Tank_Postsend == 5)//压力真空阀状态预警
				{
					STA_PVZT = "1";
				}
				if(Sta_Tank_Postsend == 9)//压力真空阀状态报警
				{
					STA_PVZT = "2";
				}

				if(sta_pre[1] == 0)
				{
					STA_YZ = "0";
				}
				if(sta_pre[1] == 1)
				{
					STA_YZ = "1";
				}
				if(sta_pre[1] == 2)
				{
					STA_YZ = "2";
				}

				if(Pre_tank_en == 0)
				{
					STA_YGLY = "N";
					STA_PVLJZT = "N";
					STA_PVZT = "N";
				}
				if(Pre_pipe_en == 0)
				{
					STA_YZ = "N";
				}
				if(Env_Gas_en == 0)
				{
					STA_ND = "N";
				}
				else
				{
					STA_ND = QString::number(sta_fga[1]);
				}
				send_gun_sta = "";
				for(unsigned int i = 0;i < Amount_Dispener;i++)
				{
					printf("!!!!!!!!!%d",Amount_Gasgun[i]);
					for(unsigned int j = 0;j < Amount_Gasgun[i];j++)
					{
						if(Flag_Accumto[i][j] == 0)
						{
							sta_gun = "0";
						}
						if((Flag_Accumto[i][j] > 0)&&(Flag_Accumto[i][j] <= 5))
						{
							sta_gun = "1";
						}
						if(Flag_Accumto[i][j] > 5)
						{
							sta_gun = "2";
						}
						gun_num = QString("%1").arg(Mapping[(i*8+j)],4,10,QLatin1Char('0'));//k为int型或char型都可
						send_gun_sta.append("q").append(gun_num).append("-AlvAlm=").append(sta_gun).append(";");
					}
				}
				//isoosi添加
				send_gun_sta = send_gun_sta.left(send_gun_sta.length()-1);
				emit gun_warn_data(send_gun_sta,STA_YGLY,STA_YZ,STA_PVZT,STA_ND,STA_PVLJZT,"N","N");
				/**************end***网络报警数据*******************/
			}
		}
		if(Flag_Network_Send_Version == 2) //重庆协议
		{
			if(id.toInt() == 0)//单次报警数据
			{
				if(sta_yg != "N")//发送油罐相关的报警
				{
					if(sta_yg == "0")
					{
						emit gun_warn_data_cq("N","N","0","N","0","0","N","N","N","N","N");
					}
					if(sta_yg == "1")//油罐零压预警
					{
						//emit gun_warn_data_cq("N","N","0","N","0","0","N","N","N","N","N");
					}
					if(sta_yg == "2")//油罐零压报警
					{
						emit gun_warn_data_cq("N","N","N","N","N","1","N","N","N","N","N");
					}
					if(sta_yg == "3")//压力真空阀临界预警
					{
						//emit gun_warn_data_cq("N","N","0","N","0","0","N","N","N","N","N");
					}
					if(sta_yg == "4")//压力真空阀临界报警
					{
						emit gun_warn_data_cq("N","N","N","N","1","N","N","N","N","N","N");
					}
					if(sta_yg == "5")//压力真空阀预警
					{
						//emit gun_warn_data_cq("N","N","0","N","0","0","N","N","N","N","N");
					}
					if(sta_yg == "6")//压力真空阀报警
					{
						//emit gun_warn_data_cq("N","N","N","N","N","N","N","N","N","N","N");
					}
				}
				if(sta_yz != "N")//发送液阻报警
				{
					if(sta_yz == "0")
					{
						emit gun_warn_data_cq("N","N","N","0","N","N","N","N","N","N","N");
					}
					if(sta_yz == "1")
					{
						//emit gun_warn_data_cq("N","N","N","0","N","N","N","N","N","N","N");
					}
					if(sta_yz == "2")
					{
						emit gun_warn_data_cq("N","N","N","1","N","N","N","N","N","N","N");
					}
				}
				if(hclzt != "N") //后处理浓度
				{
					if(hclzt == "0")
					{
						emit gun_warn_data_cq("N","N","N","N","N","N","N","N","0","N","N");
					}
					if(hclzt == "1")
					{
						//emit gun_warn_data_cq("N","N","N","N","N","N","N","N","N","N","N");
					}
					if(hclzt == "2")
					{
						emit gun_warn_data_cq("N","N","N","N","N","N","N","N","1","N","N");
					}
				}
			}
			if(id.toInt() == 1)//当天报警数据
			{
				/**************begin***网络报警数据*******************/
				printf("ready send one day data !!\n");

				STA_YGLY = "0";
				STA_PVLJZT = "0";
				STA_PVZT = "0";
				STA_YZ = "0";
				if(Sta_Tank_Postsend == 0)  //正常
				{
					STA_YGLY = "0";
					STA_PVLJZT = "0";
					STA_PVZT = "0";
				}
				if(Sta_Tank_Postsend == 1)//零压预警
				{
					//STA_YGLY = "1";
				}
				if(Sta_Tank_Postsend == 2)//零压报警
				{
					STA_YGLY = "1";
				}
				if(Sta_Tank_Postsend == 3)//泄压阀极限压力状态预警
				{
					//STA_PVLJZT = "1";
				}
				if(Sta_Tank_Postsend == 4)//泄压阀极限压力状态报警 当做油罐压力报警
				{
					STA_PVLJZT = "1";
				}
				if(Sta_Tank_Postsend == 5)//压力真空阀状态预警
				{
					STA_PVZT = "1";
				}
				if(Sta_Tank_Postsend == 9)//压力真空阀状态报警
				{
					STA_PVZT = "2";
				}

				if(sta_pre[1] == 0)
				{
					STA_YZ = "0";
				}
				if(sta_pre[1] == 1)
				{
					//STA_YZ = "1";
				}
				if(sta_pre[1] == 2)
				{
					STA_YZ = "1";
				}
				for(unsigned int i = 0;i < Amount_Dispener;i++)
				{
					printf("!!!!!!!!!%d",Amount_Gasgun[i]);
					for(unsigned int j = 0;j < Amount_Gasgun[i];j++)
					{
						if(Flag_Accumto[i][j] == 0)
						{
							sta_gun = "0";
						}
						if(Flag_Accumto[i][j] > 5)
						{
							sta_gun = "1";
						}
						gun_num = QString("%1").arg(Mapping[(i*8+j)],3,10,QLatin1Char('0'));//k为int型或char型都可

						emit gun_warn_data_cq(sta_gun,gun_num,"N",STA_YZ,STA_PVLJZT,STA_YGLY,"N","N","N","N","N");
					}
				}
				/**************end***网络报警数据*******************/
			}
		}
		if(Flag_Network_Send_Version == 3) //唐山协议，与福建相同
		{
			if(id.toInt() == 0)//单次报警数据
			{
				if(sta_yg != "N")//发送油罐相关的报警
				{
					if(sta_yg == "0")
					{
						Send_Warndata(DATAID_POST,"N","0","N","0","0","0","0");
					}
					if(sta_yg == "1")//油罐零压预警
					{
						Send_Warndata(DATAID_POST,"N","0","N","1","0","0","0");
					}
					if(sta_yg == "2")//油罐零压报警
					{
						Send_Warndata(DATAID_POST,"N","0","N","2","0","0","0");
					}
					if(sta_yg == "3")//压力真空阀临界预警
					{
						Send_Warndata(DATAID_POST,"N","0","N","0","0","1","0");
					}
					if(sta_yg == "4")//压力真空阀临界报警
					{
						Send_Warndata(DATAID_POST,"N","0","N","0","0","2","0");
					}
					if(sta_yg == "5")//压力真空阀预警
					{
						Send_Warndata(DATAID_POST,"N","0","N","0","1","0","0");
					}
					if(sta_yg == "6")//压力真空阀报警
					{
						Send_Warndata(DATAID_POST,"N","0","N","0","2","0","0");
					}
				}
				if(sta_yz != "N")//发送液阻报警
				{
					if(sta_yz == "0")
					{
						Send_Warndata(DATAID_POST,"N","0","0","N","N","N","0");
					}
					if(sta_yz == "1")
					{
						Send_Warndata(DATAID_POST,"N","0","1","N","N","N","0");
					}
					if(sta_yz == "2")
					{
						Send_Warndata(DATAID_POST,"N","0","2","N","N","N","0");
					}
				}
				if(hclzt != "N")//浓度报警
				{
					if(hclzt == "0")
					{
						Send_Warndata(DATAID_POST,"N","N","N","N","N","N","0");
					}
					if(hclzt == "1")
					{
						Send_Warndata(DATAID_POST,"N","N","N","N","N","N","1");
					}
					if(hclzt == "2")
					{
						Send_Warndata(DATAID_POST,"N","N","N","N","N","N","2");
					}
				}
			}
			if(id.toInt() == 1)//当天报警数据
			{
				/**************begin***网络报警数据*******************/
				printf("ready send one day data !!\n");
				STA_YGLY = "0";
				STA_PVLJZT = "0";
				STA_PVZT = "0";
				STA_YZ = "0";
				if(Pre_tank_en == 1)
				{

					if(Sta_Tank_Postsend == 0)  //正常
					{
						STA_YGLY = "0";
						STA_PVLJZT = "0";
						STA_PVZT = "0";
					}
					if(Sta_Tank_Postsend == 1)//零压预警
					{
						STA_YGLY = "1";
					}
					if(Sta_Tank_Postsend == 2)//零压报警
					{
						STA_YGLY = "2";
					}
					if(Sta_Tank_Postsend == 3)//泄压阀极限压力状态预警
					{
						STA_PVLJZT = "1";
					}
					if(Sta_Tank_Postsend == 4)//泄压阀极限压力状态报警
					{
						STA_PVLJZT = "2";
					}
					if(Sta_Tank_Postsend == 5)//压力真空阀状态预警
					{
						STA_PVZT = "1";
					}
					if(Sta_Tank_Postsend == 9)//压力真空阀状态报警
					{
						STA_PVZT = "2";
					}
				}
				else
				{
					STA_YGLY = "N";
					STA_PVLJZT = "N";
					STA_PVZT = "N";
				}
				if(Pre_pipe_en == 1)
				{
					if(sta_pre[1] == 0)
					{
						STA_YZ = "0";
					}
					if(sta_pre[1] == 1)
					{
						STA_YZ = "1";
					}
					if(sta_pre[1] == 2)
					{
						STA_YZ = "2";
					}
				}
				else
				{
					STA_YZ = "N";
				}
				if(Env_Gas_en == 1)
				{
					if(Flag_StaFga_Temp[1] == 0)
					{
						STA_ND = "0";
					}
					if(Flag_StaFga_Temp[1] == 1)
					{
						STA_ND = "1";
					}
					if(Flag_StaFga_Temp[1] == 2)
					{
						STA_ND = "2";
					}
				}
				else
				{
					STA_ND = "N";
				}

				for(unsigned int i = 0;i < Amount_Dispener;i++)
				{
					printf("!!!!!!!!!%d",Amount_Gasgun[i]);
					for(unsigned int j = 0;j < Amount_Gasgun[i];j++)
					{
						num_gun++;
						if(Flag_Accumto[i][j] == 0)
						{
							sta_gun = "0";
						}
						if((Flag_Accumto[i][j] > 0)&&(Flag_Accumto[i][j] <= 5))
						{
							sta_gun = "1";
						}
						if(Flag_Accumto[i][j] > 5)
						{
							sta_gun = "2";
						}
						al_post.append(QString::number(num_gun).append(":").append(sta_gun).append(";"));
					}
				}
				num_gun = 0;
				Send_Warndata(DATAID_POST,al_post,"0",STA_YZ,STA_YGLY,STA_PVZT,STA_PVLJZT,"N");
				/**************end***网络报警数据*******************/
			}
		}
		if(Flag_Network_Send_Version == 4) //湖南协议，与福建类似
		{
			if(id.toInt() == 0)//单次报警数据
			{
				if(sta_yg != "N")//发送油罐相关的报警
				{
					if(sta_yg == "0")
					{                                     //时间、气液比、密闭性、液阻、油罐压力、后处理浓度、pv阀、后处理启动、后处理停止、回气管
						Send_Warndata_HuNan(DATAID_POST,"date_kong","N",  "0",  "N",  "0",     "N",    "0",   "N",   "N", "N");
					}
					if(sta_yg == "1")//油罐零压预警
					{							           //时间、气液比、密闭性、液阻、油罐压力、后处理浓度、pv阀、后处理启动、后处理停止、回气管
						Send_Warndata_HuNan(DATAID_POST,"date_kong","N",  "0",  "N",  "0",     "N",    "0",   "N",   "N", "N");
					}
					if(sta_yg == "2")//油罐零压报警
					{						               //时间、气液比、密闭性、液阻、油罐压力、后处理浓度、pv阀、后处理启动、后处理停止、回气管
						Send_Warndata_HuNan(DATAID_POST,"date_kong","N",  "0",  "N",  "0",     "N",    "0",   "N",   "N", "N");
					}
					if(sta_yg == "3")//压力真空阀临界预警
					{                                      //时间、气液比、密闭性、液阻、油罐压力、后处理浓度、pv阀、后处理启动、后处理停止、回气管
						Send_Warndata_HuNan(DATAID_POST,"date_kong","N",  "N",  "N",  "1",     "N",    "N",   "N",   "N", "N");
					}
					if(sta_yg == "4")//压力真空阀临界报警
					{                                      //时间、气液比、密闭性、液阻、油罐压力、后处理浓度、pv阀、后处理启动、后处理停止、回气管
						Send_Warndata_HuNan(DATAID_POST,"date_kong","N",  "N",  "N",  "2",     "N",    "N",   "N",   "N", "N");
					}
					if(sta_yg == "5")//压力真空阀预警
					{                                      //时间、气液比、密闭性、液阻、油罐压力、后处理浓度、pv阀、后处理启动、后处理停止、回气管
						Send_Warndata_HuNan(DATAID_POST,"date_kong","N",  "N",  "N",  "N",     "N",    "1",   "N",   "N", "N");
					}
					if(sta_yg == "6")//压力真空阀报警
					{                                      //时间、气液比、密闭性、液阻、油罐压力、后处理浓度、pv阀、后处理启动、后处理停止、回气管
						Send_Warndata_HuNan(DATAID_POST,"date_kong","N",  "N",  "N",  "N",     "N",    "2",   "N",   "N", "N");
					}
				}
				if(sta_yz != "N")//发送液阻报警
				{
					if(sta_yz == "0")
					{                                      //时间、气液比、密闭性、液阻、油罐压力、后处理浓度、pv阀、后处理启动、后处理停止、回气管
						Send_Warndata_HuNan(DATAID_POST,"date_kong","N",  "N",  "0",  "N",     "N",    "N",   "N",   "N", "N");
					}
					if(sta_yz == "1")
					{                                      //时间、气液比、密闭性、液阻、油罐压力、后处理浓度、pv阀、后处理启动、后处理停止、回气管
						Send_Warndata_HuNan(DATAID_POST,"date_kong","N",  "N",  "1",  "N",     "N",    "N",   "N",   "N", "N");
					}
					if(sta_yz == "2")
					{                                      //时间、气液比、密闭性、液阻、油罐压力、后处理浓度、pv阀、后处理启动、后处理停止、回气管
						Send_Warndata_HuNan(DATAID_POST,"date_kong","N",  "N",  "2",  "N",     "N",    "N",   "N",   "N", "N");
					}
				}
				if(hclzt != "N")//浓度报警
				{
					if(hclzt == "0")
					{                                       //时间、气液比、密闭性、液阻、油罐压力、后处理浓度、pv阀、后处理启动、后处理停止、回气管
						Send_Warndata_HuNan(DATAID_POST,"date_kong","N",  "N",  "N",  "N",     "0",    "N",   "N",   "N", "N");
					}
					if(hclzt == "1")
					{
						Send_Warndata_HuNan(DATAID_POST,"date_kong","N",  "N",  "N",  "N",     "1",    "N",   "N",   "N", "N");
					}
					if(hclzt == "2")
					{
						Send_Warndata_HuNan(DATAID_POST,"date_kong","N",  "N",  "N",  "N",     "2",    "N",   "N",   "N", "N");
					}
				}
			}
			if(id.toInt() == 1)//当天报警数据
			{
				/**************begin***网络报警数据*******************/
				printf("ready send one day data !!\n");
				STA_YGLY = "0";
				STA_PVLJZT = "0";
				STA_PVZT = "0";
				STA_YZ = "0";
				STA_ND = "0";
				if(Pre_tank_en == 1)
				{

					if(Sta_Tank_Postsend == 0)  //正常
					{
						STA_YGLY = "0";
						STA_PVLJZT = "0";
						STA_PVZT = "0";
					}
					if(Sta_Tank_Postsend == 1)//零压预警
					{
						STA_YGLY = "1";
					}
					if(Sta_Tank_Postsend == 2)//零压报警
					{
						STA_YGLY = "2";
					}
					if(Sta_Tank_Postsend == 3)//泄压阀极限压力状态预警
					{
						STA_PVLJZT = "1";
					}
					if(Sta_Tank_Postsend == 4)//泄压阀极限压力状态报警
					{
						STA_PVLJZT = "2";
					}
					if(Sta_Tank_Postsend == 5)//压力真空阀状态预警
					{
						STA_PVZT = "1";
					}
					if(Sta_Tank_Postsend == 9)//压力真空阀状态报警
					{
						STA_PVZT = "2";
					}
				}
				else
				{
					STA_YGLY = "N";
					STA_PVLJZT = "N";
					STA_PVZT = "N";
				}
				if(Pre_pipe_en == 1)
				{
					if(sta_pre[1] == 0)
					{
						STA_YZ = "0";
					}
					if(sta_pre[1] == 1)
					{
						STA_YZ = "1";
					}
					if(sta_pre[1] == 2)
					{
						STA_YZ = "2";
					}
				}
				else
				{
					STA_YZ = "N";
				}
				if(Env_Gas_en == 1)
				{
					if(Flag_StaFga_Temp[1] == 0)
					{
						STA_ND = "0";
					}
					if(Flag_StaFga_Temp[1] == 1)
					{
						STA_ND = "1";
					}
					if(Flag_StaFga_Temp[1] == 2)
					{
						STA_ND = "2";
					}
				}
				else
				{
					STA_ND = "N";
				}

				for(unsigned int i = 0;i < Amount_Dispener;i++)
				{
					printf("!!!!!!!!!%d",Amount_Gasgun[i]);
					for(unsigned int j = 0;j < Amount_Gasgun[i];j++)
					{
						num_gun++;
						if(Flag_Accumto[i][j] == 0)
						{
							sta_gun = "0";
						}
						if((Flag_Accumto[i][j] > 0)&&(Flag_Accumto[i][j] <= 5))
						{
							sta_gun = "1";
						}
						if(Flag_Accumto[i][j] > 5)
						{
							sta_gun = "2";
						}
						al_post.append(QString::number(num_gun).append(":").append(sta_gun).append(";"));
					}
				}
				num_gun = 0;
				                                    //时间、气液比、 密闭性、 液阻、油罐压力、 后处理浓度、 pv阀、  后处理启动、后处理停止、回气管
				Send_Warndata_HuNan(DATAID_POST,"date_kong",al_post,"N", STA_YZ,STA_PVLJZT,STA_ND,STA_PVZT,"N",   "N", "N");
				/**************end***网络报警数据*******************/
			}
		}
		if(Flag_Network_Send_Version == 5) //江门协议  与 唐山协议，与福建相同
		{
			if(id.toInt() == 0)//单次报警数据
			{
				if(sta_yg != "N")//发送油罐相关的报警
				{
					if(sta_yg == "0")
					{
						Send_Warndata(DATAID_POST,"N","0","N","0","0","0","0");
					}
					if(sta_yg == "1")//油罐零压预警
					{
						Send_Warndata(DATAID_POST,"N","0","N","1","0","0","0");
					}
					if(sta_yg == "2")//油罐零压报警
					{
						Send_Warndata(DATAID_POST,"N","0","N","2","0","0","0");
					}
					if(sta_yg == "3")//压力真空阀临界预警
					{
						Send_Warndata(DATAID_POST,"N","0","N","0","0","1","0");
					}
					if(sta_yg == "4")//压力真空阀临界报警
					{
						Send_Warndata(DATAID_POST,"N","0","N","0","0","2","0");
					}
					if(sta_yg == "5")//压力真空阀预警
					{
						Send_Warndata(DATAID_POST,"N","0","N","0","1","0","0");
					}
					if(sta_yg == "6")//压力真空阀报警
					{
						Send_Warndata(DATAID_POST,"N","0","N","0","2","0","0");
					}
				}
				if(sta_yz != "N")//发送液阻报警
				{
					if(sta_yz == "0")
					{
						Send_Warndata(DATAID_POST,"N","0","0","N","N","N","0");
					}
					if(sta_yz == "1")
					{
						Send_Warndata(DATAID_POST,"N","0","1","N","N","N","0");
					}
					if(sta_yz == "2")
					{
						Send_Warndata(DATAID_POST,"N","0","2","N","N","N","0");
					}
				}
				if(hclzt != "N")//浓度报警
				{
					if(hclzt == "0")
					{
						Send_Warndata(DATAID_POST,"N","N","N","N","N","N","0");
					}
					if(hclzt == "1")
					{
						Send_Warndata(DATAID_POST,"N","N","N","N","N","N","1");
					}
					if(hclzt == "2")
					{
						Send_Warndata(DATAID_POST,"N","N","N","N","N","N","2");
					}
				}
			}
			if(id.toInt() == 1)//当天报警数据
			{
				/**************begin***网络报警数据*******************/
				printf("ready send one day data !!\n");
				STA_YGLY = "0";
				STA_PVLJZT = "0";
				STA_PVZT = "0";
				STA_YZ = "0";
				if(Pre_tank_en == 1)
				{

					if(Sta_Tank_Postsend == 0)  //正常
					{
						STA_YGLY = "0";
						STA_PVLJZT = "0";
						STA_PVZT = "0";
					}
					if(Sta_Tank_Postsend == 1)//零压预警
					{
						STA_YGLY = "1";
					}
					if(Sta_Tank_Postsend == 2)//零压报警
					{
						STA_YGLY = "2";
					}
					if(Sta_Tank_Postsend == 3)//泄压阀极限压力状态预警
					{
						STA_PVLJZT = "1";
					}
					if(Sta_Tank_Postsend == 4)//泄压阀极限压力状态报警
					{
						STA_PVLJZT = "2";
					}
					if(Sta_Tank_Postsend == 5)//压力真空阀状态预警
					{
						STA_PVZT = "1";
					}
					if(Sta_Tank_Postsend == 9)//压力真空阀状态报警
					{
						STA_PVZT = "2";
					}
				}
				else
				{
					STA_YGLY = "N";
					STA_PVLJZT = "N";
					STA_PVZT = "N";
				}
				if(Pre_pipe_en == 1)
				{
					if(sta_pre[1] == 0)
					{
						STA_YZ = "0";
					}
					if(sta_pre[1] == 1)
					{
						STA_YZ = "1";
					}
					if(sta_pre[1] == 2)
					{
						STA_YZ = "2";
					}
				}
				else
				{
					STA_YZ = "N";
				}
				if(Env_Gas_en == 1)
				{
					if(Flag_StaFga_Temp[1] == 0)
					{
						STA_ND = "0";
					}
					if(Flag_StaFga_Temp[1] == 1)
					{
						STA_ND = "1";
					}
					if(Flag_StaFga_Temp[1] == 2)
					{
						STA_ND = "2";
					}
				}
				else
				{
					STA_ND = "N";
				}

				for(unsigned int i = 0;i < Amount_Dispener;i++)
				{
					printf("!!!!!!!!!%d",Amount_Gasgun[i]);
					for(unsigned int j = 0;j < Amount_Gasgun[i];j++)
					{
						num_gun++;
						if(Flag_Accumto[i][j] == 0)
						{
							sta_gun = "0";
						}
						if((Flag_Accumto[i][j] > 0)&&(Flag_Accumto[i][j] <= 5))
						{
							sta_gun = "1";
						}
						if(Flag_Accumto[i][j] > 5)
						{
							sta_gun = "2";
						}
						al_post.append(QString::number(num_gun).append(":").append(sta_gun).append(";"));
					}
				}
				num_gun = 0;
				Send_Warndata(DATAID_POST,al_post,"0",STA_YZ,STA_YGLY,STA_PVZT,STA_PVLJZT,"N");
				/**************end***网络报警数据*******************/
			}
		}
		if(Flag_Network_Send_Version == 6)
		{
			if(id.toInt() == 0)//单次报警数据
			{
				if(sta_yg != "N")//发送油罐相关的报警
				{
					if(sta_yg == "0")
					{
						emit send_warninfo_foshan(DATAID_POST,"","N","0","N","0","0","0","0","N","N","N");
					}
					if(sta_yg == "1")//油罐零压预警
					{
						emit send_warninfo_foshan(DATAID_POST,"","N","N","N","N","1","N","N","N","N","N");
					}
					if(sta_yg == "2")//油罐零压报警
					{
						emit send_warninfo_foshan(DATAID_POST,"","N","N","N","N","2","N","N","N","N","N");
					}
					if(sta_yg == "3")//压力真空阀临界预警
					{
						emit send_warninfo_foshan(DATAID_POST,"","N","N","N","N","N","N","1","N","N","N");
					}
					if(sta_yg == "4")//压力真空阀临界报警
					{
						emit send_warninfo_foshan(DATAID_POST,"","N","N","N","N","N","N","2","N","N","N");
					}
					if(sta_yg == "5")//压力真空阀预警
					{
						emit send_warninfo_foshan(DATAID_POST,"","N","N","N","N","N","1","N","N","N","N");
					}
					if(sta_yg == "6")//压力真空阀报警
					{
						emit send_warninfo_foshan(DATAID_POST,"","N","N","N","N","N","2","N","N","N","N");
					}
				}
				if(sta_yz != "N")//发送液阻报警
				{
					emit send_warninfo_foshan(DATAID_POST,"","N","N",sta_yz,"N","N","N","N","N","N","N");
				}
				if(hclzt != "N")//浓度报警
				{
					emit send_warninfo_foshan(DATAID_POST,"","N","N","N","N","N","N","N","N",hclzt,"N");
				}
			}
			if(id.toInt() == 1)//当天报警数据
			{
				/**************begin***网络报警数据*******************/
				printf("ready send one day data !!\n");
				STA_YGLY = "0";
				STA_PVLJZT = "0";
				STA_PVZT = "0";
				STA_YZ = "0";
				if(Pre_tank_en == 1)
				{

					if(Sta_Tank_Postsend == 0)  //正常
					{
						STA_YGLY = "0";
						STA_PVLJZT = "0";
						STA_PVZT = "0";
					}
					if(Sta_Tank_Postsend == 1)//零压预警
					{
						STA_YGLY = "1";
					}
					if(Sta_Tank_Postsend == 2)//零压报警
					{
						STA_YGLY = "2";
					}
					if(Sta_Tank_Postsend == 3)//泄压阀极限压力状态预警
					{
						STA_PVLJZT = "1";
					}
					if(Sta_Tank_Postsend == 4)//泄压阀极限压力状态报警
					{
						STA_PVLJZT = "2";
					}
					if(Sta_Tank_Postsend == 5)//压力真空阀状态预警
					{
						STA_PVZT = "1";
					}
					if(Sta_Tank_Postsend == 9)//压力真空阀状态报警
					{
						STA_PVZT = "2";
					}
				}
				else
				{
					STA_YGLY = "N";
					STA_PVLJZT = "N";
					STA_PVZT = "N";
				}
				if(Pre_pipe_en == 1)
				{
					if(sta_pre[1] == 0)
					{
						STA_YZ = "0";
					}
					if(sta_pre[1] == 1)
					{
						STA_YZ = "1";
					}
					if(sta_pre[1] == 2)
					{
						STA_YZ = "2";
					}
				}
				else
				{
					STA_YZ = "N";
				}
				if(Env_Gas_en == 1)
				{
					if(Flag_StaFga_Temp[1] == 0)
					{
						STA_ND = "0";
					}
					else if(Flag_StaFga_Temp[1] == 1)
					{
						STA_ND = "1";
					}
					else if(Flag_StaFga_Temp[1] == 2)
					{
						STA_ND = "2";
					}
					else {
						STA_ND = "0";
					}
				}
				else
				{
					STA_ND = "N";
				}

				for(unsigned int i = 0;i < Amount_Dispener;i++)
				{
					printf("!!!!!!!!!%d",Amount_Gasgun[i]);
					for(unsigned int j = 0;j < Amount_Gasgun[i];j++)
					{
						num_gun++;
						if(Flag_Accumto[i][j] == 0)
						{
							sta_gun = "0";
						}
						if((Flag_Accumto[i][j] > 0)&&(Flag_Accumto[i][j] <= 5))
						{
							sta_gun = "1";
						}
						if(Flag_Accumto[i][j] > 5)
						{
							sta_gun = "2";
						}
						al_post.append(QString::number(num_gun).append(":").append(sta_gun).append(";"));
					}
				}
				num_gun = 0;
				//密闭性和油罐零压公用一个，油罐压力和压力真空阀报警状态用一个,后处理装置状态没有，卸油回气管状态没有
				//qDebug()<<"*************************"<<STA_ND;
				emit send_warninfo_foshan(DATAID_POST,"",al_post,STA_YGLY,STA_YZ,STA_PVZT,STA_YGLY,STA_PVZT,STA_PVLJZT,"N",STA_ND,"N");
				/**************end***网络报警数据*******************/
			}
		}
        if(Flag_Network_Send_Version == 8)
        {
            if(id.toInt() == 0)//单次报警数据
            {
                if(sta_yg != "N")//发送油罐相关的报警
                {
                    if(sta_yg == "0")
                    {
                        Send_Warndata_dg(DATAID_POST,"N","0","N","N","0","0","0","N","N","N");
                    }
                    if(sta_yg == "1")//油罐零压预警
                    {
                        Send_Warndata_dg(DATAID_POST,"N","0","N","N","1","0","0","N","N","N");
                    }
                    if(sta_yg == "2")//油罐零压报警
                    {
                        Send_Warndata_dg(DATAID_POST,"N","0","N","N","2","0","0","N","N","N");
                    }
                    if(sta_yg == "3")//压力真空阀临界预警
                    {
                        Send_Warndata_dg(DATAID_POST,"N","0","N","N","0","0","1","N","N","N");
                    }
                    if(sta_yg == "4")//压力真空阀临界报警
                    {
                        Send_Warndata_dg(DATAID_POST,"N","0","N","N","0","0","2","N","N","N");
                    }
                    if(sta_yg == "5")//压力真空阀预警
                    {
                        Send_Warndata_dg(DATAID_POST,"N","0","N","N","0","1","0","N","N","N");
                    }
                    if(sta_yg == "6")//压力真空阀报警
                    {
                        Send_Warndata_dg(DATAID_POST,"N","0","N","N","0","2","0","N","N","N");
                    }
                }
                if(sta_yz != "N")//发送液阻报警
                {
                    if(sta_yz == "0")
                    {
                        Send_Warndata_dg(DATAID_POST,"N","0","0","N","N","N","N","N","N","N");
                    }
                    if(sta_yz == "1")
                    {
                        Send_Warndata_dg(DATAID_POST,"N","0","1","N","N","N","N","N","N","N");
                    }
                    if(sta_yz == "2")
                    {
                        Send_Warndata_dg(DATAID_POST,"N","0","2","N","N","N","N","N","N","N");
                    }
                }
                if(hclzt != "N")//浓度报警
                {
                    if(hclzt == "0")
                    {
                        Send_Warndata_dg(DATAID_POST,"N","N","N","N","N","N","N","0","N","N");
                    }
                    if(hclzt == "1")
                    {
                        Send_Warndata_dg(DATAID_POST,"N","N","N","N","N","N","N","1","N","N");
                    }
                    if(hclzt == "2")
                    {
                        Send_Warndata_dg(DATAID_POST,"N","N","N","N","N","N","N","2","N","N");
                    }
                }
            }
            if(id.toInt() == 1)//当天报警数据
            {
                /**************begin***网络报警数据*******************/
                printf("ready send one day data !!\n");
                STA_YGLY = "0";
                STA_PVLJZT = "0";
                STA_PVZT = "0";
                STA_YZ = "0";
                if(Pre_tank_en == 1)
                {

                    if(Sta_Tank_Postsend == 0)  //正常
                    {
                        STA_YGLY = "0";
                        STA_PVLJZT = "0";
                        STA_PVZT = "0";
                    }
                    if(Sta_Tank_Postsend == 1)//零压预警
                    {
                        STA_YGLY = "1";
                    }
                    if(Sta_Tank_Postsend == 2)//零压报警
                    {
                        STA_YGLY = "2";
                    }
                    if(Sta_Tank_Postsend == 3)//泄压阀极限压力状态预警
                    {
                        STA_PVLJZT = "1";
                    }
                    if(Sta_Tank_Postsend == 4)//泄压阀极限压力状态报警
                    {
                        STA_PVLJZT = "2";
                    }
                    if(Sta_Tank_Postsend == 5)//压力真空阀状态预警
                    {
                        STA_PVZT = "1";
                    }
                    if(Sta_Tank_Postsend == 9)//压力真空阀状态报警
                    {
                        STA_PVZT = "2";
                    }
                }
                else
                {
                    STA_YGLY = "N";
                    STA_PVLJZT = "N";
                    STA_PVZT = "N";
                }
                if(Pre_pipe_en == 1)
                {
                    if(sta_pre[1] == 0)
                    {
                        STA_YZ = "0";
                    }
                    if(sta_pre[1] == 1)
                    {
                        STA_YZ = "1";
                    }
                    if(sta_pre[1] == 2)
                    {
                        STA_YZ = "2";
                    }
                }
                else
                {
                    STA_YZ = "N";
                }
                if(Env_Gas_en == 1)
                {
                    if(Flag_StaFga_Temp[1] == 0)
                    {
                        STA_ND = "0";
                    }
                    if(Flag_StaFga_Temp[1] == 1)
                    {
                        STA_ND = "1";
                    }
                    if(Flag_StaFga_Temp[1] == 2)
                    {
                        STA_ND = "2";
                    }
                }
                else
                {
                    STA_ND = "N";
                }

                for(unsigned int i = 0;i < Amount_Dispener;i++)
                {
                    printf("!!!!!!!!!%d",Amount_Gasgun[i]);
                    for(unsigned int j = 0;j < Amount_Gasgun[i];j++)
                    {
                        num_gun++;
                        if(Flag_Accumto[i][j] == 0)
                        {
                            sta_gun = "0";
                        }
                        if((Flag_Accumto[i][j] > 0)&&(Flag_Accumto[i][j] <= 5))
                        {
                            sta_gun = "1";
                        }
                        if(Flag_Accumto[i][j] > 5)
                        {
                            sta_gun = "2";
                        }
                        al_post.append(QString::number(num_gun).append(":").append(sta_gun).append(";"));
                    }
                }
                num_gun = 0;
                Send_Warndata_dg(DATAID_POST,al_post,"0",STA_YZ,"N",STA_YGLY,STA_PVZT,STA_PVLJZT,"0","N","N");
                /**************end***网络报警数据*******************/
            }
        }

		if(Flag_MyServerEn == 1) //myserver协议
		{
			if(id.toInt() == 0)//单次报警数据
			{
				if(sta_yg != "N")//发送油罐相关的报警
				{
					if(sta_yg == "0")
					{
						emit gun_warn_data_myserver("q0000=N","0","N","0","N","0","N","N");
					}
					if(sta_yg == "1")//油罐零压预警
					{
						//Send_Warndata(DATAID_POST,"N","0","N","1","0","0","0");
					}
					if(sta_yg == "2")//油罐零压报警
					{
						//Send_Warndata(DATAID_POST,"N","0","N","2","0","0","0");
					}
					if(sta_yg == "3")//压力真空阀临界预警
					{
						emit gun_warn_data_myserver("q0000=N","0","N","0","N","1","N","N");
					}
					if(sta_yg == "4")//压力真空阀临界报警
					{
						emit gun_warn_data_myserver("q0000=N","0","N","0","N","2","N","N");
					}
					if(sta_yg == "5")//压力真空阀预警
					{
						emit gun_warn_data_myserver("q0000=N","0","N","1","N","0","N","N");
					}
					if(sta_yg == "6")//压力真空阀报警
					{
						emit gun_warn_data_myserver("q0000=N","0","N","2","N","0","N","N");
					}
				}
				if(sta_yz != "N")//发送液阻报警
				{
					if(sta_yz == "0")
					{
						emit gun_warn_data_myserver("q0000=N","0","0","N","N","N","N","N");
					}
					if(sta_yz == "1")
					{
						emit gun_warn_data_myserver("q0000=N","0","1","N","N","N","N","N");
					}
					if(sta_yz == "2")
					{
						emit gun_warn_data_myserver("q0000=N","0","2","N","N","N","N","N");
					}
				}
				if(hclzt != "N")
				{
					if(hclzt == "0")
					{
						emit gun_warn_data_myserver("q0000=N","0","N","N","0","N","N","N");
					}
					if(hclzt == "1")
					{
						emit gun_warn_data_myserver("q0000=N","0","N","N","1","N","N","N");
					}
					if(hclzt == "2")
					{
						emit gun_warn_data_myserver("q0000=N","0","N","N","2","N","N","N");
					}
				}
			}
			if(id.toInt() == 1)//当天报警数据
			{
				/**************begin***网络报警数据*******************/
				printf("ready send one day data !!\n");

				STA_YGLY = "0";
				STA_PVLJZT = "0";
				STA_PVZT = "0";
				STA_YZ = "0";
				STA_ND = "0";
				if(Sta_Tank_Postsend == 0)  //正常
				{
					STA_YGLY = "0";
					STA_PVLJZT = "0";
					STA_PVZT = "0";
				}
				if(Sta_Tank_Postsend == 1)//零压预警
				{
					STA_YGLY = "1";
				}
				if(Sta_Tank_Postsend == 2)//零压报警
				{
					STA_YGLY = "2";
				}
				if(Sta_Tank_Postsend == 3)//泄压阀极限压力状态预警
				{
					STA_PVLJZT = "1";
				}
				if(Sta_Tank_Postsend == 4)//泄压阀极限压力状态报警
				{
					STA_PVLJZT = "2";
				}
				if(Sta_Tank_Postsend == 5)//压力真空阀状态预警
				{
					STA_PVZT = "1";
				}
				if(Sta_Tank_Postsend == 9)//压力真空阀状态报警
				{
					STA_PVZT = "2";
				}

				if(sta_pre[1] == 0)
				{
					STA_YZ = "0";
				}
				if(sta_pre[1] == 1)
				{
					STA_YZ = "1";
				}
				if(sta_pre[1] == 2)
				{
					STA_YZ = "2";
				}

				if(Pre_tank_en == 0)
				{
					STA_YGLY = "N";
					STA_PVLJZT = "N";
					STA_PVZT = "N";
				}
				if(Pre_pipe_en == 0)
				{
					STA_YZ = "N";
				}
				if(Env_Gas_en == 0)
				{
					STA_ND = "N";
				}
				else
				{
					STA_ND = QString::number(sta_fga[1]);
				}
				unsigned int gun_num_send = 0;
				send_gun_sta = "";
				for(unsigned int i = 0;i < Amount_Dispener;i++)
				{
					printf("!!!!!!!!!%d",Amount_Gasgun[i]);
					for(unsigned int j = 0;j < Amount_Gasgun[i];j++)
					{
						gun_num_send++;
						if(Flag_Accumto[i][j] == 0)
						{
							sta_gun = "0";
						}
						if((Flag_Accumto[i][j] > 0)&&(Flag_Accumto[i][j] <= 5))
						{
							sta_gun = "1";
						}
						if(Flag_Accumto[i][j] > 5)
						{
							sta_gun = "2";
						}
						gun_num = QString("%1").arg(gun_num_send,4,10,QLatin1Char('0'));//k为int型或char型都可
						send_gun_sta.append("q").append(gun_num).append("-AlvAlm=").append(sta_gun).append(";");
					}
				}
				//myserver添加
				send_gun_sta = send_gun_sta.left(send_gun_sta.length()-1);
				emit gun_warn_data_myserver(send_gun_sta,STA_YGLY,STA_YZ,STA_PVZT,STA_ND,STA_PVLJZT,"N","N");
				/**************end***网络报警数据*******************/
			}
		}
	}
}
/**************网络发送环境数据**************************
 * id     没有用
 * ygyl   油罐压力
 * yzyl   液阻压力
 * yqkj   油气空间
 * xynd   卸油区浓度 NULL关闭
 * hclnd  后处理浓度 NULL关闭
 * yqwd   油气温度
 * ************************************/
void FGA1000_485:: network_Surroundingsdata(QString id,QString ygyl,QString yzyl,QString yqkj,QString xynd,QString hclnd,QString yqwd)    //发送环境数据报文
{
    if(Flag_Postsend_Enable)
	{
		id = id;
		yqkj = yqkj;
		xynd = xynd;
		hclnd = hclnd;
		yqwd = yqwd;
		qDebug()<<"network send Surroundingsdata!"<< Flag_Network_Send_Version;
		if(Flag_Network_Send_Version == 0) //福建协议
		{
			Send_Surroundingsdata(DATAID_POST,ygyl,yzyl,"NULL");
		}
		if(Flag_Network_Send_Version == 1) //广州协议
		{
			QString postdata_tank;
			QString postdata_pipe;
			QString postdata_xieyou;//卸油浓度
			QString postdata_hcnd;//后处理浓度
			if(Pre_tank_en == 0)
			{
				postdata_tank = "0";
			}
			else
			{
				postdata_tank = QString::number(Pre[0]*1000,'f',2);
			}
			if(Pre_pipe_en == 0)
			{
				postdata_pipe = "0";
			}
			else
			{
				postdata_pipe = QString::number(Pre[1]*1000,'f',2);
			}

			if(Num_Fga >= 3)
			{
				postdata_xieyou = QString::number(Gas_Concentration_Fga[3],'f',2);
			}
			else
			{
				postdata_xieyou = "0";
			}
			if(Env_Gas_en == 1)
			{
				postdata_hcnd = QString::number(Gas_Concentration_Fga[1],'f',2);
			}
			else
			{
				postdata_hcnd = "0";
			}
			emit environmental_data(postdata_pipe,postdata_tank,postdata_xieyou,postdata_hcnd,"N","N");
		}
		if(Flag_Network_Send_Version == 2) //重庆协议
		{
			QString postdata_tank;
			QString postdata_pipe;
			QString postdata_xieyou;//卸油浓度
			QString postdata_hcnd;//后处理浓度
			QString gas_tem;//油气温度
			if(Pre_tank_en == 0)
			{
				postdata_tank = "0";
			}
			else
			{
				postdata_tank = QString::number(Pre[0]*1000,'f',2);
			}
			if(Pre_pipe_en == 0)
			{
				postdata_pipe = "0";
			}
			else
			{
				postdata_pipe = QString::number(Pre[1]*1000,'f',2);
			}
			if(Tem_tank_en == 0)
			{
				gas_tem = "0";
			}
			else
			{
				gas_tem = QString::number(Tem[0],'f',1);
			}
			if(Num_Fga >= 3)
			{
				postdata_xieyou = QString::number(Gas_Concentration_Fga[3],'f',2);
			}
			else
			{
				postdata_xieyou = "0";
			}
			if(Env_Gas_en == 1)
			{
				postdata_hcnd = QString::number(Gas_Concentration_Fga[1],'f',2);
			}
			else
			{
				postdata_hcnd = "0";
			}
			emit environmental_data_cq(postdata_pipe,postdata_tank,postdata_xieyou,postdata_hcnd,gas_tem,"0");
		}
		if(Flag_Network_Send_Version == 3) //唐山协议，与福建相同
		{
			Send_Surroundingsdata(DATAID_POST,ygyl,yzyl,"NULL");
		}
		if(Flag_Network_Send_Version == 4) //湖南协议，与福建相同
		{
			Send_Surroundingsdata_HuNan(DATAID_POST,"data_kong",ygyl,yzyl,"NULL",hclnd,yqwd,"NULL");
		}
		if(Flag_Network_Send_Version == 5) //江门协议 与唐山协议，与福建相同
		{
			Send_Surroundingsdata(DATAID_POST,ygyl,yzyl,"NULL");
		}
		if(Flag_Network_Send_Version == 6) //佛山协议
		{
			send_environment_foshan(DATAID_POST,"date",ygyl,yzyl,yqkj,xynd,hclnd,yqwd);
		}
		if(Flag_Network_Send_Version == 7) //合肥协议，只有这一个
		{
			QString ygyl_hefei,yzyl_hefei,yqwd_hefei;

			if(ygyl == "NULL"){ygyl_hefei == "0";}
			else {ygyl_hefei = QString::number(ygyl.toFloat(),'f',1);}

			if(yzyl == "NULL"){yzyl_hefei == "0";}
			else{yzyl_hefei = QString::number(yzyl.toFloat(),'f',1);}

			if(yqwd == "NULL"){yqwd_hefei == "0";}
			else{yqwd_hefei = QString::number(yqwd.toFloat(),'f',1);}
			//ygyl = "100";
			//yzyl = "1.1";
			//yqwd = "2.2";
			Send_Surroundingsdata_HeFei(ygyl_hefei,yzyl_hefei,yqwd_hefei);
		}
        if(Flag_Network_Send_Version == 8) //东莞协议，只有这一个
        {
            Send_Surroundingsdata_dg(DATAID_POST,ygyl,yzyl,"NULL","NULL","NULL","NULL");
        }
		if(Flag_MyServerEn == 1) //myserver协议
		{
			QString postdata_tank;
			QString postdata_pipe;
			QString postdata_xieyou;//卸油浓度
			QString postdata_hcnd;//后处理浓度
			if(Pre_tank_en == 0)
			{
				postdata_tank = "0";
			}
			else
			{
				postdata_tank = QString::number(Pre[0]*1000,'f',2);
			}
			if(Pre_pipe_en == 0)
			{
				postdata_pipe = "0";
			}
			else
			{
				postdata_pipe = QString::number(Pre[1]*1000,'f',2);
			}

			if(Num_Fga >= 3)
			{
				postdata_xieyou = QString::number(Gas_Concentration_Fga[3],'f',2);
			}
			else
			{
				postdata_xieyou = "0";
			}
			if(Env_Gas_en == 1)
			{
				postdata_hcnd = QString::number(Gas_Concentration_Fga[1],'f',2);
			}
			else
			{
				postdata_hcnd = "0";
			}
			emit environmental_data_myserver(postdata_pipe,postdata_tank,postdata_xieyou,postdata_hcnd,"N","N");
		}
	}
}

/**************网络发送故障数据*********************
 * id     0单次 1整点
 * type   故障编码 031001 罐压通信故障 031002 液阻传感器故障  041000 浓度传感器故障（通用）
 * ***********************************/
void FGA1000_485:: network_Wrongsdata(QString id,QString type) //发送故障数据报文
{
	if(1)//如果网线连接
	{

		qDebug()<<"network send Wrongsdata!"<< Flag_Network_Send_Version;
		if(Flag_Network_Send_Version == 0)//福建协议
		{
			if(id == "1")
			{
				//发送通讯是否正常
				unsigned char flag_uartwrong = 0;
				if( ( (Env_Gas_en==1&&sta_fga[1]==0)||(Env_Gas_en == 0) )
				        &&( (Pre_tank_en==1&&sta_pre[0] == 0)||(Pre_tank_en==0))
				        &&( (Pre_pipe_en==1&&sta_pre[1] == 0)||(Pre_pipe_en==0)))
				{
					QString wrongdata_post = "0111";//post添加
					unsigned int sensor_id = 0;
					//qDebug() << "biao zhengchang!!!!!!";
					for(int i = 0;i < Amount_Dispener;i++)
					{
						for(int j = 0;j < (Amount_Gasgun[i]);j++)
						{
							//qDebug() << i*4+j<< "!!!" << Flag_CommunicateError_Maindisp[i*4+j]<< "???";
							if(ReoilgasPop_GunSta[i*8+j] >= 10)//如果有通信故障
							{
								wrongdata_post = "0111";
								if((i*8+j)%2==0)//采集器的第一把枪
								{
									sensor_id = (i*8+j)/2+1;//采集器编号
									Send_Wrongsdata(DATAID_POST,wrongdata_post.append(QString("%1").arg(sensor_id, 2, 10, QLatin1Char('0'))));
								}
								flag_uartwrong = 1;
							}
						}
					}
				}
				else
				{
					flag_uartwrong = 1;
					//Send_Wrongsdata(DATAID_POST,"031000");
				}
				if(flag_uartwrong == 0)
				{
					Send_Wrongsdata(DATAID_POST,"000000");
				}
				else
				{
					qDebug()<<"have uart wrong!";
					//Send_Wrongsdata(DATAID_POST,"031000");
				}
			}
			if(id == "0")
			{
				Send_Wrongsdata(DATAID_POST,type);
			}
		}
		if(Flag_Network_Send_Version == 1)//广州协议
		{
			//没有
		}
		if(Flag_Network_Send_Version == 2)//重庆协议
		{
			if(id == "1")
			{
				//发送通讯是否正常
				unsigned char flag_uartwrong = 0;
				if( ( (Env_Gas_en==1&&sta_fga[1]==0)||(Env_Gas_en == 0) )
				        &&( (Pre_tank_en==1&&sta_pre[0] == 0)||(Pre_tank_en==0))
				        &&( (Pre_pipe_en==1&&sta_pre[1] == 0)||(Pre_pipe_en==0)))
				{
					QString wrongdata_post = "0111";//post添加
					unsigned int sensor_id = 0;
					//qDebug() << "biao zhengchang!!!!!!";
					for(int i = 0;i < Amount_Dispener;i++)
					{
						for(int j = 0;j < (Amount_Gasgun[i]);j++)
						{
							//qDebug() << i*4+j<< "!!!" << Flag_CommunicateError_Maindisp[i*4+j]<< "???";
							if(ReoilgasPop_GunSta[i*8+j] >= 10)//如果有通信故障
							{
								wrongdata_post = "0111";
								if((i*8+j)%2==0)//采集器的第一把枪
								{
									sensor_id = (i*8+j)/2+1;//采集器编号
									refueling_wrongdata_cq(wrongdata_post.append(QString("%1").arg(sensor_id, 2, 10, QLatin1Char('0'))));
								}
								flag_uartwrong = 1;
							}
						}
					}
				}
				else
				{
					flag_uartwrong = 1;
					Send_Wrongsdata(DATAID_POST,"031000");
				}
				if(flag_uartwrong == 0)
				{
					refueling_wrongdata_cq("000000");
				}
				else
				{
					qDebug()<<"have uart wrong!";
				}
			}
			if(id == "0")
			{
				refueling_wrongdata_cq(type);
			}
		}
		if(Flag_Network_Send_Version == 3)//唐山协议，与福建相同
		{
			if(id == "1")
			{
				//发送通讯是否正常
				unsigned char flag_uartwrong = 0;
				if( ( (Env_Gas_en==1&&sta_fga[1]==0)||(Env_Gas_en == 0) )
				        &&( (Pre_tank_en==1&&sta_pre[0] == 0)||(Pre_tank_en==0))
				        &&( (Pre_pipe_en==1&&sta_pre[1] == 0)||(Pre_pipe_en==0)))
				{
					QString wrongdata_post = "0111";//post添加
					unsigned int sensor_id = 0;
					//qDebug() << "biao zhengchang!!!!!!";
					for(int i = 0;i < Amount_Dispener;i++)
					{
						for(int j = 0;j < (Amount_Gasgun[i]);j++)
						{
							//qDebug() << i*4+j<< "!!!" << Flag_CommunicateError_Maindisp[i*4+j]<< "???";
							if(ReoilgasPop_GunSta[i*8+j] >= 10)//如果有通信故障
							{
								wrongdata_post = "0111";
								if((i*8+j)%2==0)//采集器的第一把枪
								{
									sensor_id = (i*8+j)/2+1;//采集器编号
									Send_Wrongsdata(DATAID_POST,wrongdata_post.append(QString("%1").arg(sensor_id, 2, 10, QLatin1Char('0'))));
								}
								flag_uartwrong = 1;
							}
						}
					}
				}
				else
				{
					flag_uartwrong = 1;
					Send_Wrongsdata(DATAID_POST,"031000");
				}
				if(flag_uartwrong == 0)
				{
					Send_Wrongsdata(DATAID_POST,"000000");
				}
				else
				{
					qDebug()<<"have uart wrong!";
				}
			}
			if(id == "0")
			{
				Send_Wrongsdata(DATAID_POST,type);
			}
		}
		if(Flag_Network_Send_Version == 4)//湖南协议，与福建类似
		{
			if(id == "1")
			{
				//发送通讯是否正常
				unsigned char flag_uartwrong = 0;
				if( ( (Env_Gas_en==1&&sta_fga[1]==0)||(Env_Gas_en == 0) )
				        &&( (Pre_tank_en==1&&sta_pre[0] == 0)||(Pre_tank_en==0))
				        &&( (Pre_pipe_en==1&&sta_pre[1] == 0)||(Pre_pipe_en==0)))
				{
					QString wrongdata_post = "0111";//post添加
					unsigned int sensor_id = 0;
					//qDebug() << "biao zhengchang!!!!!!";
					for(int i = 0;i < Amount_Dispener;i++)
					{
						for(int j = 0;j < (Amount_Gasgun[i]);j++)
						{
							//qDebug() << i*4+j<< "!!!" << Flag_CommunicateError_Maindisp[i*4+j]<< "???";
							if(ReoilgasPop_GunSta[i*8+j] >= 10)//如果有通信故障
							{
								wrongdata_post = "0111";
								if((i*8+j)%2==0)//采集器的第一把枪
								{
									sensor_id = (i*8+j)/2+1;//采集器编号
									Send_Wrongsdata_HuNan(DATAID_POST,wrongdata_post.append(QString("%1").arg(sensor_id, 2, 10, QLatin1Char('0'))));
								}
								flag_uartwrong = 1;
							}
						}
					}
				}
				else
				{
					flag_uartwrong = 1;
				}
				if(flag_uartwrong == 0)
				{
					Send_Wrongsdata_HuNan(DATAID_POST,"000000");
				}
				else
				{
					qDebug()<<"have uart wrong!";
				}
			}
			if(id == "0")
			{
				Send_Wrongsdata_HuNan(DATAID_POST,type);
			}
		}
		if(Flag_Network_Send_Version == 5)//江门协议 与唐山协议，与福建相同
		{
			if(id == "1")
			{
				//发送通讯是否正常
				unsigned char flag_uartwrong = 0;
				if( ( (Env_Gas_en==1&&sta_fga[1]==0)||(Env_Gas_en == 0) )
				        &&( (Pre_tank_en==1&&sta_pre[0] == 0)||(Pre_tank_en==0))
				        &&( (Pre_pipe_en==1&&sta_pre[1] == 0)||(Pre_pipe_en==0)))
				{
					QString wrongdata_post = "0111";//post添加
					unsigned int sensor_id = 0;
					//qDebug() << "biao zhengchang!!!!!!";
					for(int i = 0;i < Amount_Dispener;i++)
					{
						for(int j = 0;j < (Amount_Gasgun[i]);j++)
						{
							//qDebug() << i*4+j<< "!!!" << Flag_CommunicateError_Maindisp[i*4+j]<< "???";
							if(ReoilgasPop_GunSta[i*8+j] >= 10)//如果有通信故障
							{
								wrongdata_post = "0111";
								if((i*8+j)%2==0)//采集器的第一把枪
								{
									sensor_id = (i*8+j)/2+1;//采集器编号
									Send_Wrongsdata(DATAID_POST,wrongdata_post.append(QString("%1").arg(sensor_id, 2, 10, QLatin1Char('0'))));
								}
								flag_uartwrong = 1;
							}
						}
					}
				}
				else
				{
					flag_uartwrong = 1;
					Send_Wrongsdata(DATAID_POST,"031000");
				}
				if(flag_uartwrong == 0)
				{
					Send_Wrongsdata(DATAID_POST,"000000");
				}
				else
				{
					qDebug()<<"have uart wrong!";
				}
			}
			if(id == "0")
			{
				Send_Wrongsdata(DATAID_POST,type);
			}
		}
		if(Flag_Network_Send_Version == 6)
		{
			if(id == "1")
			{
				//发送通讯是否正常
				unsigned char flag_uartwrong = 0;
				unsigned int send_gun_num = 0;
				if( ( (Env_Gas_en==1&&sta_fga[1]==0)||(Env_Gas_en == 0) )
						&&( (Pre_tank_en==1&&sta_pre[0] == 0)||(Pre_tank_en==0))
						&&( (Pre_pipe_en==1&&sta_pre[1] == 0)||(Pre_pipe_en==0)))
				{
					QString wrongdata_post = "0111";//post添加
					unsigned int sensor_id = 0;
					//qDebug() << "biao zhengchang!!!!!!";
					for(int i = 0;i < Amount_Dispener;i++)
					{
						for(int j = 0;j < (Amount_Gasgun[i]);j++)
						{
							//qDebug() << i*4+j<< "!!!" << Flag_CommunicateError_Maindisp[i*4+j]<< "???";
							if(ReoilgasPop_GunSta[i*8+j] >= 10)//如果有通信故障
							{
								wrongdata_post = "0111";
								if((i*8+j)%2==0)//采集器的第一把枪
								{
									sensor_id = (i*8+j)/2+1;//采集器编号
									send_wrong_foshan(DATAID_POST,"date",wrongdata_post.append(QString("%1").arg(sensor_id, 2, 10, QLatin1Char('0'))));
								}
								flag_uartwrong = 1;
							}
						}
					}
				}
				else
				{
					flag_uartwrong = 1;
				}
				if(flag_uartwrong == 0)
				{
					send_wrong_foshan(DATAID_POST,"","000000");
				}
				else  //不正常则不上传
				{
					qDebug()<<"have uart wrong!";
				}
				send_gun_num = 0;
			}
			if(id == "0")
			{
				send_wrong_foshan(DATAID_POST,"date",type);
			}
		}

        if(Flag_Network_Send_Version == 8)//东莞
        {
            if(id == "1")
            {
                //发送通讯是否正常
                unsigned char flag_uartwrong = 0;
                if( ( (Env_Gas_en==1&&sta_fga[1]==0)||(Env_Gas_en == 0) )
                        &&( (Pre_tank_en==1&&sta_pre[0] == 0)||(Pre_tank_en==0))
                        &&( (Pre_pipe_en==1&&sta_pre[1] == 0)||(Pre_pipe_en==0)))
                {
                    QString wrongdata_post = "0111";//post添加
                    unsigned int sensor_id = 0;
                    //qDebug() << "biao zhengchang!!!!!!";
                    for(int i = 0;i < Amount_Dispener;i++)
                    {
                        for(int j = 0;j < (Amount_Gasgun[i]);j++)
                        {
                            //qDebug() << i*4+j<< "!!!" << Flag_CommunicateError_Maindisp[i*4+j]<< "???";
                            if(ReoilgasPop_GunSta[i*8+j] >= 10)//如果有通信故障
                            {
                                wrongdata_post = "0111";
                                if((i*8+j)%2==0)//采集器的第一把枪
                                {
                                    sensor_id = (i*8+j)/2+1;//采集器编号
                                    Send_Wrongsdata_dg(DATAID_POST,wrongdata_post.append(QString("%1").arg(sensor_id, 2, 10, QLatin1Char('0'))));
                                }
                                flag_uartwrong = 1;
                            }
                        }
                    }
                }
                else
                {
                    flag_uartwrong = 1;
                    Send_Wrongsdata_dg(DATAID_POST,"031000");
                }
                if(flag_uartwrong == 0)
                {
                    Send_Wrongsdata_dg(DATAID_POST,"000000");
                }
                else
                {
                    qDebug()<<"have uart wrong!";
                }
            }
            if(id == "0")
            {
                Send_Wrongsdata_dg(DATAID_POST,type);
            }
        }

		if(Flag_MyServerEn == 1)//myserver协议
		{
			//没有
		}


	}
}

/************网络发送油枪状态*****************
 * id     没有用
 * status 油枪状态
 * ***************************/
void FGA1000_485:: network_Stagundata(QString id,QString status)//发送油枪状态报文
{
	if(1)//如果网线连接
	{

		id = id;
		status = status;
		qDebug()<<"network send Stagundata!"<< Flag_Network_Send_Version;
		if(Flag_Network_Send_Version == 0) //福建协议
		{
			//发送加油枪关停状态  全部是开启状态
			QString sta_postgundata = "";
			unsigned int num_gun = 0;
			for(unsigned int i = 0;i < Amount_Dispener;i++)
			{
				for(unsigned int j = 0;j < Amount_Gasgun[i];j++)
				{
					num_gun++;
					sta_postgundata.append(QString::number(num_gun).append(":").append("1").append(";"));
				}
			}
			Send_Stagundata(DATAID_POST,sta_postgundata);
		}
		if(Flag_Network_Send_Version == 1) //广州协议
		{
			QString send_gun_sta;
			QString gun_num;
			//isoosi油枪实际状态 全部设置为开启
			for(unsigned int i = 0;i < Amount_Dispener;i++)
			{
				for(unsigned int j = 0;j < Amount_Gasgun[i];j++)
				{
					//isoosi添加
					//                if(Flag_Accumto[i][j] == 0)
					//                {
					//                    sta_gun = "0";
					//                }
					//                if((Flag_Accumto[i][j] > 0)&&(Flag_Accumto[i][j] <= 5))
					//                {
					//                    sta_gun = "1";
					//                }
					//                if(Flag_Accumto[i][j] > 5)
					//                {
					//                    sta_gun = "2";
					//                }
					gun_num = QString("%1").arg(Mapping[(i*8+j)],4,10,QLatin1Char('0'));//k为int型或char型都可
					send_gun_sta.append("q").append(gun_num).append("-Status=").append("1").append(";");
				}
			}
			send_gun_sta = send_gun_sta.left(send_gun_sta.length()-1);
			emit refueling_gun_sta(send_gun_sta);
		}
		if(Flag_Network_Send_Version == 2)//重庆协议
		{
			//没有
		}
		if(Flag_Network_Send_Version == 3) //唐山协议 与福建协议相同
		{
			//发送加油枪关停状态  全部是开启状态
			QString sta_postgundata = "";
			unsigned int num_gun = 0;
			for(unsigned int i = 0;i < Amount_Dispener;i++)
			{
				for(unsigned int j = 0;j < Amount_Gasgun[i];j++)
				{
					num_gun++;
					sta_postgundata.append(QString::number(num_gun).append(":").append("1").append(";"));
				}
			}
			Send_Stagundata(DATAID_POST,sta_postgundata);
		}
		if(Flag_Network_Send_Version == 4) //湖南协议 与福建协议类似
		{
			//发送加油枪关停状态  全部是开启状态
			QString sta_postgundata = "";
			unsigned int num_gun = 0;
			for(unsigned int i = 0;i < Amount_Dispener;i++)
			{
				for(unsigned int j = 0;j < Amount_Gasgun[i];j++)
				{
					num_gun++;
					sta_postgundata.append(QString::number(num_gun).append(":").append("1").append(";"));
				}
			}
			Send_Stagundata_HuNan(DATAID_POST,sta_postgundata);
		}
		if(Flag_Network_Send_Version == 5) //江门协议 与唐山协议 与福建协议相同
		{
			//发送加油枪关停状态  全部是开启状态
			QString sta_postgundata = "";
			unsigned int num_gun = 0;
			for(unsigned int i = 0;i < Amount_Dispener;i++)
			{
				for(unsigned int j = 0;j < Amount_Gasgun[i];j++)
				{
					num_gun++;
					sta_postgundata.append(QString::number(num_gun).append(":").append("1").append(";"));
				}
			}
			Send_Stagundata(DATAID_POST,sta_postgundata);
		}
		if(Flag_Network_Send_Version == 6) //佛山协议
		{
			//发送加油枪关停状态  全部是开启状态
			QString sta_postgundata = "";
			unsigned int num_gun = 0;
			for(unsigned int i = 0;i < Amount_Dispener;i++)
			{
				for(unsigned int j = 0;j < Amount_Gasgun[i];j++)
				{
					num_gun++;
					sta_postgundata.append(QString::number(num_gun).append(":").append("1").append(";"));
				}
			}
			send_gunsta_foshan(DATAID_POST,"date",sta_postgundata);
		}
        if(Flag_Network_Send_Version == 8) //东莞协议 与福建协议类似
        {
            //发送加油枪关停状态  全部是开启状态
            QString sta_postgundata = "";
            unsigned int num_gun = 0;
            for(unsigned int i = 0;i < Amount_Dispener;i++)
            {
                for(unsigned int j = 0;j < Amount_Gasgun[i];j++)
                {
                    num_gun++;
                    sta_postgundata.append(QString::number(num_gun).append(":").append("1").append(";"));
                }
            }
            Send_Stagundata_dg(DATAID_POST,sta_postgundata);
        }

		if(Flag_MyServerEn == 1) //myserver协议
		{
			QString send_gun_sta;
			QString gun_num;
			unsigned gun_num_send = 0;
			//isoosi油枪实际状态 全部设置为开启
			for(unsigned int i = 0;i < Amount_Dispener;i++)
			{
				for(unsigned int j = 0;j < Amount_Gasgun[i];j++)
				{
					gun_num_send++;
					gun_num = QString("%1").arg(gun_num_send,4,10,QLatin1Char('0'));//k为int型或char型都可
					send_gun_sta.append("q").append(gun_num).append("-Status=").append("1").append(";");
				}
			}
			send_gun_sta = send_gun_sta.left(send_gun_sta.length()-1);
			emit refueling_gun_sta_myserver(send_gun_sta);
		}

	}
}
/************网络发送关枪状态*****************
 * id     0单次  1全天
 * status 油枪状态
 * ***************************/
void FGA1000_485:: network_Closegunsdata(QString id,QString jyjid,QString jyqid,QString operate,QString event)//关枪数据
{
	if(1)//如果网线连接
	{

		jyjid = jyjid;
		jyqid = jyqid;
		operate = operate;
		event = event;
		qDebug()<<"network send Closegunsdata!"<< Flag_Network_Send_Version;
		if(Flag_Network_Send_Version == 0)//福建协议
		{
			if(Flag_SendOnceGunCloseOperate == 0)//该协议并没有是要上传整点关枪
			{
				//发送加油枪关停状态  全部是开启状态
				unsigned int num_gun = 0;
				for(unsigned int i = 0;i < Amount_Dispener;i++)
				{
					for(unsigned int j = 0;j < Amount_Gasgun[i];j++)
					{
						num_gun++;
						//sta_postgundata.append(QString::number(num_gun).append(":").append("1").append(";"));
						Send_Closegunsdata(DATAID_POST,QString::number(i+1),QString::number(num_gun),"1","N");
					}
				}
			Flag_SendOnceGunCloseOperate = 1;
			}
		}
		if(Flag_Network_Send_Version == 1)//广州协议
		{
			emit refueling_gun_stop("N","N","N");//全部是开启状态
		}
		if(Flag_Network_Send_Version == 2)//重庆协议
		{
			QString gun_num;
			for(unsigned int i = 0;i < Amount_Dispener;i++)
			{
				for(unsigned int j = 0;j < Amount_Gasgun[i];j++)
				{
					gun_num = QString("%1").arg(Mapping[(i*8+j)],3,10,QLatin1Char('0'));//k为int型或char型都可
					emit refueling_gun_stop_cq(gun_num,"1","0");//全部是开启状态
				}
			}
		}
		if(Flag_Network_Send_Version == 3)//唐山协议，与福建相同
		{
			if( Flag_SendOnceGunCloseOperate == 0)//该协议并没有是要上传整点关枪
			{
				//发送加油枪关停状态  全部是开启状态
				unsigned int num_gun = 0;
				for(unsigned int i = 0;i < Amount_Dispener;i++)
				{
					for(unsigned int j = 0;j < Amount_Gasgun[i];j++)
					{
						num_gun++;
						Send_Closegunsdata(DATAID_POST,QString::number(i+1),QString::number(num_gun),"1","N");
					}
				}
				Flag_SendOnceGunCloseOperate = 1;
			}
		}
		if(Flag_Network_Send_Version == 4)//湖南协议  没有
		{
			//if(id == "4")//该协议并没有是要上传整点关枪
			//{
			    //发送加油枪关停状态  全部是开启状态
			    QString sta_postgundata = "";
				unsigned int num_gun = 0;
				for(unsigned int i = 0;i < Amount_Dispener;i++)
				{
					for(unsigned int j = 0;j < Amount_Gasgun[i];j++)
					{
						num_gun++;
						sta_postgundata.append(QString::number(num_gun).append(":").append("1").append(";"));
						Send_Closegunsdata_HuNan(DATAID_POST,QString::number(i+1),QString::number(j+1),"1","0");
						usleep(100);
					}
				}
				//Send_Stagundata_HuNan(DATAID_POST,sta_postgundata);
				//Send_Closegunsdata_HuNan(DATAID_POST,QString::number( j+1),QString::number(i+1),QString::number(1),QString::number(0));
				//}
		}
		if(Flag_Network_Send_Version == 5)//江门协议 与唐山协议，与福建相同
		{
			if(Flag_SendOnceGunCloseOperate == 0)//该协议并没有是要上传整点关枪
			{
				//发送加油枪关停状态  全部是开启状态
				unsigned int num_gun = 0;
				for(unsigned int i = 0;i < Amount_Dispener;i++)
				{
					for(unsigned int j = 0;j < Amount_Gasgun[i];j++)
					{
						num_gun++;
						Send_Closegunsdata(DATAID_POST,QString::number(i+1),QString::number(num_gun),"1","N");
					}
				}
				Flag_SendOnceGunCloseOperate = 1;
			}
		}
		if(Flag_Network_Send_Version == 6)
		{
			if(Flag_SendOnceGunCloseOperate == 0)//该协议并没有是要上传整点关枪
			{
				//发送加油枪关停状态  全部是开启状态
				unsigned int num_gun = 0;
				for(unsigned int i = 0;i < Amount_Dispener;i++)
				{
					for(unsigned int j = 0;j < Amount_Gasgun[i];j++)
					{
						num_gun++;
						//Send_Closegunsdata(DATAID_POST,QString::number(i+1),QString::number(num_gun),"1","N");
						send_gunoperate_foshan(DATAID_POST,"date",QString::number(i+1),QString::number(num_gun),"1","N");
					}
				}
				Flag_SendOnceGunCloseOperate = 1;
			}
		}
        if(Flag_Network_Send_Version == 8)//东莞协议  没有
        {
            //if(id == "4")//该协议并没有是要上传整点关枪
            //{
                //发送加油枪关停状态  全部是开启状态
                QString sta_postgundata = "";
                unsigned int num_gun = 0;
                for(unsigned int i = 0;i < Amount_Dispener;i++)
                {
                    for(unsigned int j = 0;j < Amount_Gasgun[i];j++)
                    {
                        num_gun++;
                        sta_postgundata.append(QString::number(num_gun).append(":").append("1").append(";"));
                        Send_Closegunsdata_dg(DATAID_POST,QString::number(i+1),QString::number(j+1),"1","0");
                        usleep(100);
                    }
                }
                //Send_Stagundata_HuNan(DATAID_POST,sta_postgundata);
                //Send_Closegunsdata_HuNan(DATAID_POST,QString::number( j+1),QString::number(i+1),QString::number(1),QString::number(0));
                //}
        }

		if(Flag_MyServerEn == 1)//myserver协议
		{
			emit refueling_gun_stop_myserver("N","N","N");//全部是开启状态
		}
	}
}
/************网络发送设置信息 一天一次*****************
 * id     未使用
 * ***************************/
void FGA1000_485::network_Configurationdata(QString id)//设置数据，每天发送一次用
{
	if(1)//如果网线连接
	{

		id = id;
		qDebug()<<"network send onfigurationdata!"<< Flag_Network_Send_Version;
		if(Flag_Network_Send_Version == 0) //福建协议
		{
			QString jyqs = QString::number(Amount_Gasgun[0]+Amount_Gasgun[1]+Amount_Gasgun[2]+Amount_Gasgun[3]+Amount_Gasgun[4]+Amount_Gasgun[5]+
			        Amount_Gasgun[6]+Amount_Gasgun[7]+Amount_Gasgun[8]+Amount_Gasgun[9]+Amount_Gasgun[10]+Amount_Gasgun[11]);
			Send_Configurationdata(DATAID_POST,jyqs,QString::number(Positive_Pres,'f',1),QString::number(Negative_Pres,'f',1),
			                       "600.0",QString::number(Far_Dispener));
		}
		if(Flag_Network_Send_Version == 1) //广州协议
		{
			emit setup_data(QString::number(Positive_Pres,'f',2),QString::number(-Negative_Pres,'f',2),"0.00","0.00");
		}
		if(Flag_Network_Send_Version == 2)//重庆协议
		{
			emit setup_data_cq(QString::number(Positive_Pres,'f',2),QString::number(Negative_Pres,'f',2),"0","0");
		}
		if(Flag_Network_Send_Version == 3) //唐山协议，与福建相同
		{
			QString jyqs = QString::number(Amount_Gasgun[0]+Amount_Gasgun[1]+Amount_Gasgun[2]+Amount_Gasgun[3]+Amount_Gasgun[4]+Amount_Gasgun[5]+
			        Amount_Gasgun[6]+Amount_Gasgun[7]+Amount_Gasgun[8]+Amount_Gasgun[9]+Amount_Gasgun[10]+Amount_Gasgun[11]);
			Send_Configurationdata(DATAID_POST,jyqs,QString::number(Positive_Pres,'f',1),QString::number(Negative_Pres,'f',1),
			                       "600.0",QString::number(Far_Dispener));
		}
		if(Flag_Network_Send_Version == 4) //湖南协议，与福建相同
		{
			QString jyqs = QString::number(Amount_Gasgun[0]+Amount_Gasgun[1]+Amount_Gasgun[2]+Amount_Gasgun[3]+Amount_Gasgun[4]+Amount_Gasgun[5]+
			        Amount_Gasgun[6]+Amount_Gasgun[7]+Amount_Gasgun[8]+Amount_Gasgun[9]+Amount_Gasgun[10]+Amount_Gasgun[11]);
			Send_Configurationdata_HuNan(DATAID_POST,jyqs,QString::number(Positive_Pres,'f',1),QString::number(Negative_Pres,'f',1),"0",QString::number(Far_Dispener));
		}
		if(Flag_Network_Send_Version == 5) //江门协议 与唐山协议，与福建相同
		{
			QString jyqs = QString::number(Amount_Gasgun[0]+Amount_Gasgun[1]+Amount_Gasgun[2]+Amount_Gasgun[3]+Amount_Gasgun[4]+Amount_Gasgun[5]+
			        Amount_Gasgun[6]+Amount_Gasgun[7]+Amount_Gasgun[8]+Amount_Gasgun[9]+Amount_Gasgun[10]+Amount_Gasgun[11]);
			Send_Configurationdata(DATAID_POST,jyqs,QString::number(Positive_Pres,'f',1),QString::number(Negative_Pres,'f',1),
			                       "600.0",QString::number(Far_Dispener));
		}
		if(Flag_Network_Send_Version == 6) //佛山协议
		{
			QString jyqs = QString::number(Amount_Gasgun[0]+Amount_Gasgun[1]+Amount_Gasgun[2]+Amount_Gasgun[3]+Amount_Gasgun[4]+Amount_Gasgun[5]+
					Amount_Gasgun[6]+Amount_Gasgun[7]+Amount_Gasgun[8]+Amount_Gasgun[9]+Amount_Gasgun[10]+Amount_Gasgun[11]);
			emit send_setinfo_foshan(DATAID_POST,"0",jyqs,QString::number(Positive_Pres,'f',1),QString::number(Negative_Pres,'f',1),"0","0",QString::number(Far_Dispener));
			//第二位date为0时，不上传加油机和加油枪对应编号信息，为1时上传。
		}
        if(Flag_Network_Send_Version == 8) //东莞协议，与福建相同
        {
            QString jyqs = QString::number(Amount_Gasgun[0]+Amount_Gasgun[1]+Amount_Gasgun[2]+Amount_Gasgun[3]+Amount_Gasgun[4]+Amount_Gasgun[5]+
                    Amount_Gasgun[6]+Amount_Gasgun[7]+Amount_Gasgun[8]+Amount_Gasgun[9]+Amount_Gasgun[10]+Amount_Gasgun[11]);
            emit Send_Configurationdata_dg(DATAID_POST,jyqs,QString::number(Positive_Pres,'f',1),QString::number(Negative_Pres,'f',1),
                                        "600.0",QString::number(Far_Dispener));
        }

		if(Flag_MyServerEn == 1) //mysetver协议
		{
			emit setup_data_myserver(QString::number(Positive_Pres,'f',2),QString::number(-Negative_Pres,'f',2),"0.00","0.00");
		}

	}
}

/************初次连接服务器的时候发送所有数据*****************
 *
 * ***************************/
void FGA1000_485::Myserver_First_Client()//服务器第一次连接，需要上传一次所有状态
{
	Flag_MyserverFirstSend = 1;

}
void FGA1000_485::First_Send()
{
	network_Configurationdata("N");   //日设置信息
	sleep(1);
	network_Warndata("1","N","N","N");//发送日报警信息
	sleep(1);
	network_Stagundata("0","N");      //日油枪状态
	sleep(1);
	network_Wrongsdata("1","N");      //日故障信息
	sleep(1);
	network_Closegunsdata("0","N","N","N","N");//关枪数据
	sleep(1);
	Flag_MyserverFirstSend = 0;
}
