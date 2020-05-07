#include"reoilgasthread.h"
#include"serial.h"
#include"config.h"
#include"database_op.h"
#include<fcntl.h>
#include<termio.h>
#include<unistd.h>
#include<stdio.h>
#include<QDebug>
#include<systemset.h>
#include"io_op.h"

int fd_uart_reoilgas;
int ret_uart_reoilgas;
int len_uart_reoilgas;

int Reoilgas_Data_Temp[12][8] = {0};
unsigned char Which_Dispener_i = 0;
unsigned char Which_GasCtrler_j = 0;
unsigned char Flag_Whichone = 0;
unsigned char Address_Reoilgas[48] = {0xA1,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF,0xB0,
                                      0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF,0xC0,
                                      0xC1,0xC2,0xC3,0xC4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,0xd0}; //增加到每个加油机8把枪，增加12个地址
unsigned char Flag_Communicate_Error[48] = {0};     //通讯错误之后再次读取数据
unsigned char Flag_CommunicateError_Maindisp[48] = {0};     //通信故障报警
unsigned char RecvBuff_Oilgas[50];
unsigned char Flag_Askagain = 0;
unsigned char Flag_Whichone_Temp = 0;   //是否设置完一圈

unsigned char Flag_ask_PreAndTem = 0;//每隔一段时间问一次压力传感器等设备
unsigned char Pressure_AskNum = 1;   //问压力传感器的地址
unsigned char Temperature_AskNum = 1;   //问温度传感器的地址
unsigned char Fga1000_AskNum = 1;   //问fga传感器的地址
unsigned char Flag_PreUartWrong[2] = {0};//通信故障计数
unsigned char Flag_TemUartWrong[2] = {0};//通信故障计数
unsigned char Flag_FgaUartWrong[8] = {0};//通信故障计数
unsigned char ReoilgasPreSta[2] = {0};//压力传感器状态    与Fga1000_485共享
unsigned char ReoilGasFgaSta[8] = {0};//浓度传感器状态    与Fga1000_485共享
unsigned char ReoilgasTemSta[2] = {0};//温度传感器状态    与Fga1000_485共享
float pressure_value[8] = {0};        //压力数值 源数据，转换后赋值给Pre
float temperature_value[8] = {0};     //温度值 源数据，转换后赋值给Tem
unsigned char Fga1000_Value[8] = {0}; //浓度值           与Fga1000_485共享


//ask690 05.22
unsigned char Address[16] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0xF,0x00};          //修改了16号地址为0x00
unsigned char Address_Extend[16] = {0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F};  //加油机和防渗池的地址
unsigned char Flag_Ask_jingdian = 0;  //1表示问静电监测设备
unsigned char Count_Ask_jingdian = 0;   //如果不回连续问8次
unsigned char Flag_ASk_IIE = 0; //1表示问IIE
unsigned char Count_Ask_IIE = 0; //问IIE的次数
unsigned char Sta_IIE[15] = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};//IIE的状态 第一位用来存储通讯故障，0正常，f通讯故障
unsigned char Flag_Jingdian_Enable = 0;//使能
unsigned char Flag_IIE_Enable = 0;//使能
unsigned char Flag_Jingdian_uartwrong = 0;//静电通讯故障
unsigned char Flag_IIE_uartwrong = 0;//IIE通信故障
unsigned char Ask_690_Buf[8] = {0x00,0x04,0x00,0x02,0x00,0x02,0,0}; //发送缓存区初始化，后两位由程序加上校验码
unsigned char Count_Ask690 = 0;     //0管线 1油罐 2防渗池 3加油机
unsigned char Count_Ask690_pipetank = 0;
unsigned char Count_Ask690_basindispener = 0;
unsigned char Data_Buf_Sencor_Pre[21] = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};//19字节预处理数组  第一位和最后两位做校验
unsigned char aucAlarmState = 0;    //每位状态传递
unsigned char Flag_690 = 0; //通信故障检测位
unsigned char Receive_Pressure_Data[16] = {0}; //用来存储压力数值的数组
unsigned char Ptr_Ask690[44] = {0};

reoilgasthread::reoilgasthread(QObject *parent):
    QThread(parent)
{

}

void reoilgasthread::run()
{
	sleep(2);
	//串口初始化
	fd_uart_reoilgas = open(REOILGAS_SERI,O_RDWR|O_NOCTTY);
	ret_uart_reoilgas = set_port_attr(fd_uart_reoilgas,B9600,8,"1",'N',0,0);
	sleep(1);
	if((Flag_Reoilgas_Version == 3)||(Flag_Reoilgas_Version == 4)||(Flag_Reoilgas_Version == 5))
	{
		D433T3D_init();//无线模块初始化
	}
	gpio_low(1,22); //m1
	//gpio_high(1,23); //m0
	gpio_low(1,23); //m0
	sleep(1);
	//数据分析

	while(1)   //1：旧型号采集器 有线； 2：带调节阀的气液比采集器一代，有线  3：带调节阀的采集器无线 4：带屏幕的采集器，有线  5：带屏幕的采集器无线
	{
		if(Flag_Reoilgas_Version == 1)
		{
			msleep(100);
			SendDataReoilgas();
		}
		else if((Flag_Reoilgas_Version == 2)||(Flag_Reoilgas_Version == 3)||(Flag_Reoilgas_Version == 4)||(Flag_Reoilgas_Version == 5))//有线无线气液比采集器
		{
			msleep(100);
			SendDataReoilgas_v2();
		}
		else
		{
			msleep(200);
		}
		//新版本采集器同步气量脉冲当量和油量脉冲当量
		if(Flag_WaitSync == 1) //如果需要同步数据
		{
			sync_data();
		}
		if(Flag_SendMode_Oilgas == 1)//正常模式的时候才进入下面的问询
		{
			Flag_ask_PreAndTem++;
			if(Flag_ask_PreAndTem >= 10)
			{
				//network_oilgundata(DATAID_POST,"1","1","1.1","40","25","40","25","200");
				if(Flag_Controller_Version == 1)//压力表无线模式  且是新版控制器
				{
					Flag_ask_PreAndTem = 0;
					msleep(200);
					Ask_Sensor();
				}
				Flag_ask_PreAndTem = 0;
			}
			//ask690部分
			if(Flag_ask_PreAndTem%5 == 0)
			{
				if(Flag_Controller_Version == 1)
				{
					if((count_basin!=0)&&(count_dispener!=0)&&(count_pipe!=0)&&(count_tank!=0))//传感器数目不等于0
					{
						for(unsigned int i = 0;i<8;i++)
						{
							ask_690();
							msleep(100);
							read_690();
							msleep(10);
							if((Flag_ASk_IIE == 0)&&(Flag_Ask_jingdian == 0))//正常数据分析
							{
								send_data_690(); //处理传感器返回数据并发送
							}
						}
					}
				}
			}
		}

	}
}

void reoilgasthread::SendDataReoilgas()
{
	if(!Flag_Askagain)
	{
		Flag_Whichone = Select_Sensor();
		//printf("Select Next Reoilgas Sensor!\n");
	}
	Lock_Mode_Reoilgas.lock();
	unsigned char flag_sendmode_temp = Flag_SendMode_Oilgas;
	Lock_Mode_Reoilgas.unlock();
	switch(flag_sendmode_temp)
	{
	case 1: if(Flag_Whichone == 0xee)
		{
			Flag_Timeto_CloseNeeded[2] = 0;
			break;
		}
		else
		{
			unsigned char SendBuff_Oilgas[8];
			SendBuff_Oilgas[0] = Address_Reoilgas[Flag_Whichone];
			SendBuff_Oilgas[1] = 0x03;
			SendBuff_Oilgas[2] = 0x00;
			SendBuff_Oilgas[3] = 0x00;
			SendBuff_Oilgas[4] = 0x00;
			SendBuff_Oilgas[5] = 0x0d;
			unsigned int SCRC = 0;
			SCRC = CRC_Test(SendBuff_Oilgas,8);
			SendBuff_Oilgas[6] = (SCRC & 0xff00) >> 8;
			SendBuff_Oilgas[7] = (SCRC & 0x00ff);
			write(fd_uart_reoilgas,SendBuff_Oilgas,sizeof(SendBuff_Oilgas));
			//                    for(uchar i = 0; i < 8; i++)           //DEBUG
			//                    {                                       //DEBUG
			//                        printf("%02x ",SendBuff_Oilgas[i]); //DEBUG
			//                    }
			//                    printf("ask oilgas!\n");
			msleep(250);
			ReadDataReoilgas();
			break;
		}
		break;
	case 2:     //版本读取
	{
		unsigned char SendBuff_Oilgas[8];
		SendBuff_Oilgas[0] = Address_Reoilgas[Reoilgas_Version_Set_Whichone];
		SendBuff_Oilgas[1] = 0x03;
		SendBuff_Oilgas[2] = 0x00;
		SendBuff_Oilgas[3] = 0x64;
		SendBuff_Oilgas[4] = 0x00;
		SendBuff_Oilgas[5] = 0x01;
		unsigned int SCRC = 0;
		SCRC = CRC_Test(SendBuff_Oilgas,8);
		SendBuff_Oilgas[6] = (SCRC & 0xff00) >> 8;
		SendBuff_Oilgas[7] = (SCRC & 0x00ff);
		write(fd_uart_reoilgas,SendBuff_Oilgas,sizeof(SendBuff_Oilgas));
		msleep(200);
		ReadDataReoilgas_Version();
		Flag_Askagain = 0;
	}
		break;
	case 3:     //写设置
		if(Flag_Whichone == 0xee)
		{
			Flag_Timeto_CloseNeeded[2] = 0;
			Lock_Mode_Reoilgas.lock();
			Flag_SendMode_Oilgas = 1;
			Flag_Askagain = 0;
			Flag_Whichone_Temp = 0;
			Lock_Mode_Reoilgas.unlock();
			emit Reoilgas_Factor_Setover();
			break;
		}
		else
		{
			if(Flag_Whichone >= Flag_Whichone_Temp)
			{
				unsigned char SendBuff_Oilgas[27];
				SendBuff_Oilgas[0] = Address_Reoilgas[Flag_Whichone];
				SendBuff_Oilgas[1] = 0x10;
				SendBuff_Oilgas[2] = 0x00;
				SendBuff_Oilgas[3] = 0xC8;
				SendBuff_Oilgas[4] = 0x00;
				SendBuff_Oilgas[5] = 0x09;
				SendBuff_Oilgas[6] = 0x12;
				SendBuff_Oilgas[7] = 0x00;
				SendBuff_Oilgas[8] = 0x01;
				SendBuff_Oilgas[9] = 0x00;      //加油枪标识
				SendBuff_Oilgas[10] = 0x01;     //加油枪标识
				SendBuff_Oilgas[11] = Fueling_Factor[Which_Dispener_i][Which_GasCtrler_j*2];     //油量校正因子
				SendBuff_Oilgas[12] = (int)(Fueling_Factor[Which_Dispener_i][Which_GasCtrler_j*2]*100)%100;//油量校正因子
				SendBuff_Oilgas[13] = Gas_Factor[Which_Dispener_i][Which_GasCtrler_j*2];     //气量校正因子
				SendBuff_Oilgas[14] = (int)(Gas_Factor[Which_Dispener_i][Which_GasCtrler_j*2]*100)%100;    //气量校正因子
				SendBuff_Oilgas[15] = 0x00;     //关枪继电器
				SendBuff_Oilgas[16] = Flag_Delay_State[Which_Dispener_i][Which_GasCtrler_j*2];     //关枪继电器         //目前未使用
				if(Flag_Gun_off == 0)//未开启关枪功能
				{
					SendBuff_Oilgas[16] = 0x00;
				}
				SendBuff_Oilgas[17] = 0x00;     //加油枪标识
				SendBuff_Oilgas[18] = 0x02;     //加油枪标识
				SendBuff_Oilgas[19] = Fueling_Factor[Which_Dispener_i][Which_GasCtrler_j*2+1];     //油量校正因子
				SendBuff_Oilgas[20] = (int)(Fueling_Factor[Which_Dispener_i][Which_GasCtrler_j*2+1]*100)%100;//油量校正因子
				SendBuff_Oilgas[21] = Gas_Factor[Which_Dispener_i][Which_GasCtrler_j*2+1];     //气量校正因子
				SendBuff_Oilgas[22] = (int)(Gas_Factor[Which_Dispener_i][Which_GasCtrler_j*2+1]*100)%100;     //气量校正因子
				SendBuff_Oilgas[23] = 0x00;     //关枪继电器
				SendBuff_Oilgas[24] = Flag_Delay_State[Which_Dispener_i][Which_GasCtrler_j*2+1];     //关枪继电器         //目前未使用
				if(Flag_Gun_off == 0)//未开启关枪功能
				{
					SendBuff_Oilgas[24] = 0x00;
				}
				unsigned int SCRC = 0;
				SCRC = CRC_Test(SendBuff_Oilgas,27);
				SendBuff_Oilgas[25] = (SCRC & 0xff00) >> 8;
				SendBuff_Oilgas[26] = (SCRC & 0x00ff);
				write(fd_uart_reoilgas,SendBuff_Oilgas,sizeof(SendBuff_Oilgas));
				Flag_Whichone_Temp = Flag_Whichone;

				msleep(500);  //200改为500，写设置的时候能够看到设置信息，不然刷新太快
				len_uart_reoilgas = read(fd_uart_reoilgas,RecvBuff_Oilgas,sizeof(RecvBuff_Oilgas));
				SCRC = CRC_Test(RecvBuff_Oilgas,len_uart_reoilgas);
				if((RecvBuff_Oilgas[0] == SendBuff_Oilgas[0]) && (RecvBuff_Oilgas[len_uart_reoilgas-2] == ((SCRC & 0xff00)>>8)) && (RecvBuff_Oilgas[len_uart_reoilgas-1] == (SCRC & 0x00ff)))
				{
					Flag_Askagain = 0;
					emit Warn_UartWrong_Mainwindowdisp(Flag_Whichone,0);
					Flag_CommunicateError_Maindisp[Flag_Whichone] = 0;
					printf("set over!\n");
					Which_GasCtrler_j++;
					Flag_Whichone_Temp++;
				}
				else
				{
					Flag_Askagain = 1;
					Flag_CommunicateError_Maindisp[Flag_Whichone]++;
					if(Flag_CommunicateError_Maindisp[Flag_Whichone]>3)//5次改为3次
					{
						//sleep(1);
						emit Warn_UartWrong_Mainwindowdisp(Flag_Whichone,1);
						Flag_CommunicateError_Maindisp[Flag_Whichone] = 5;
						Flag_Askagain = 0; //M新增，防止设置不成功卡住地址
						Which_GasCtrler_j++;
						Flag_Whichone_Temp++;
					}
				}
			}
			else
			{
				Lock_Mode_Reoilgas.lock();
				Flag_SendMode_Oilgas = 1;
				Flag_Askagain = 0;
				Flag_Whichone_Temp = 0;
				Lock_Mode_Reoilgas.unlock();
				emit Reoilgas_Factor_Setover();
			}
		}
		break;
	case 4:     //写设置准备
	{
		Which_Dispener_i = 0;
		Which_GasCtrler_j = 0;
		Flag_Askagain = 0;
		Flag_Whichone_Temp = 0;
		Lock_Mode_Reoilgas.lock();
		Flag_SendMode_Oilgas = 3;
		Lock_Mode_Reoilgas.unlock();
	}
		break;
	case 5:     //设置读取
	{
		unsigned char SendBuff_Oilgas[8];
		SendBuff_Oilgas[0] = Address_Reoilgas[Reoilgas_Version_Set_Whichone];
		SendBuff_Oilgas[1] = 0x03;
		SendBuff_Oilgas[2] = 0x00;
		SendBuff_Oilgas[3] = 0xC8;
		SendBuff_Oilgas[4] = 0x00;
		SendBuff_Oilgas[5] = 0x09;
		unsigned int SCRC = 0;
		SCRC = CRC_Test(SendBuff_Oilgas,8);
		SendBuff_Oilgas[6] = (SCRC & 0xff00) >> 8;
		SendBuff_Oilgas[7] = (SCRC & 0x00ff);
		write(fd_uart_reoilgas,SendBuff_Oilgas,sizeof(SendBuff_Oilgas));
		msleep(300);
		ReadDataReoilgas_Setinfo();
	}
	}
}
void reoilgasthread::ReadDataReoilgas()
{
	len_uart_reoilgas = read(fd_uart_reoilgas,RecvBuff_Oilgas,sizeof(RecvBuff_Oilgas));
	//数据分析
	unsigned int SCRC = 0;
	SCRC = CRC_Test(RecvBuff_Oilgas,len_uart_reoilgas);
	if((RecvBuff_Oilgas[len_uart_reoilgas-2] == ((SCRC & 0xff00)>>8)) && (RecvBuff_Oilgas[len_uart_reoilgas-1] == (SCRC & 0x00ff)))
	{
		if(RecvBuff_Oilgas[0] == Address_Reoilgas[Flag_Whichone])
		{
			//            if(Amount_Gasgun[Which_Dispener_i]%2)   //只有一个枪
			//            {
			//                printf("Data Recved! Mode: one gun\n");
			//                unsigned char flag = RecvBuff_Oilgas[8];
			//                if((!flag) || Flag_Communicate_Error[Flag_Whichone])
			//                {
			//                    int oil_count;
			//                    int gas_count;
			//                    int oilgas_time;
			//                    oil_count = (RecvBuff_Oilgas[9]<<8) + RecvBuff_Oilgas[10];
			//                    gas_count = (RecvBuff_Oilgas[11]<<8) + RecvBuff_Oilgas[12];
			//                    oilgas_time = (RecvBuff_Oilgas[13]<<8) + RecvBuff_Oilgas[14];
			//                    printf("reoilgas data output\n");
			//                    if(Reoilgas_Data_Temp[Which_Dispener_i][Which_GasCtrler_j*2] == oil_count)
			//                    {}
			//                    else
			//                    {
			//                        Reoilgas_Data_Temp[Which_Dispener_i][Which_GasCtrler_j*2] = oil_count;
			//                        add_value_reoilgas(Which_Dispener_i,Which_GasCtrler_j*2,oilgas_time,gas_count,oil_count);
			//                    }
			//                }
			//            }
			//            else
			//            {
			//全部按双枪判断
			unsigned char flag = RecvBuff_Oilgas[8];
			int oil_count;
			int gas_count;
			int oilgas_time;
			//post添加
			float countofgas_e = 0;
			float countofoil_e = 0;

			if((!flag) || Flag_Communicate_Error[Flag_Whichone])
			{
				oil_count = (RecvBuff_Oilgas[9]<<8) + RecvBuff_Oilgas[10];
				gas_count = (RecvBuff_Oilgas[11]<<8) + RecvBuff_Oilgas[12];
				oilgas_time = (RecvBuff_Oilgas[13]<<8) + RecvBuff_Oilgas[14];
				printf("reoilgas data output\n");
				if(Reoilgas_Data_Temp[Which_Dispener_i][Which_GasCtrler_j*2] == oil_count)
				{}
				else
				{
					Reoilgas_Data_Temp[Which_Dispener_i][Which_GasCtrler_j*2] = oil_count;
					add_value_reoilgas(Which_Dispener_i,Which_GasCtrler_j*2,oilgas_time,gas_count,oil_count);
					//post添加
					countofgas_e = Gas_Factor[Which_Dispener_i][Which_GasCtrler_j*2]*gas_count/1000.0;
					countofoil_e = Fueling_Factor[Which_Dispener_i][Which_GasCtrler_j*2]*oil_count/1000.0;
					if(countofoil_e >= 15)
					{
						network_oilgundata(DATAID_POST,QString::number(Which_Dispener_i+1),QString::number(Which_GasCtrler_j*2+1),
						                   QString::number((countofgas_e/countofoil_e)*100,'f',1),
						                   QString::number(countofgas_e/oilgas_time*60,'f',1),QString::number(countofgas_e,'f',1),
						                   QString::number(countofoil_e/oilgas_time*60,'f',1),QString::number(countofoil_e,'f',1),
						                   QString::number(Pre[1]*1000,'f',1));
					}
				}
			}
			flag = RecvBuff_Oilgas[20];
			if((!flag) || Flag_Communicate_Error[Flag_Whichone])
			{
				oil_count = (RecvBuff_Oilgas[21]<<8) + RecvBuff_Oilgas[22];
				gas_count = (RecvBuff_Oilgas[23]<<8) + RecvBuff_Oilgas[24];
				oilgas_time = (RecvBuff_Oilgas[25]<<8) + RecvBuff_Oilgas[26];
				printf("reoilgas data_2 output\n");
				if(Reoilgas_Data_Temp[Which_Dispener_i][Which_GasCtrler_j*2+1] == oil_count)
				{}
				else
				{
					Reoilgas_Data_Temp[Which_Dispener_i][Which_GasCtrler_j*2+1] = oil_count;
					add_value_reoilgas(Which_Dispener_i,Which_GasCtrler_j*2+1,oilgas_time,gas_count,oil_count);
					//post添加
					countofgas_e = Gas_Factor[Which_Dispener_i][Which_GasCtrler_j*2+1]*gas_count/1000.0;
					countofoil_e = Fueling_Factor[Which_Dispener_i][Which_GasCtrler_j*2+1]*oil_count/1000.0;
					if(countofoil_e >= 15)
					{
						network_oilgundata(DATAID_POST,QString::number(Which_Dispener_i+1),QString::number(Which_GasCtrler_j*2+2),
						                   QString::number((countofgas_e/countofoil_e)*100,'f',1),
						                   QString::number(countofgas_e/oilgas_time*60,'f',1),QString::number(countofgas_e,'f',1),
						                   QString::number(countofoil_e/oilgas_time*60,'f',1),QString::number(countofoil_e,'f',1),
						                   QString::number(Pre[1]*1000,'f',1));
					}
				}
			}

			Flag_Communicate_Error[Flag_Whichone] = 0;
			emit Warn_UartWrong_Mainwindowdisp(Flag_Whichone,0);
			Flag_CommunicateError_Maindisp[Flag_Whichone] = 0;
		}
	}
	else
	{
		Flag_Communicate_Error[Flag_Whichone] = 1;
		Flag_CommunicateError_Maindisp[Flag_Whichone]++;
		if(Flag_CommunicateError_Maindisp[Flag_Whichone] > 3) //5次改为3次
		{
			emit Warn_UartWrong_Mainwindowdisp(Flag_Whichone,1);
			Flag_CommunicateError_Maindisp[Flag_Whichone] = 5;
		}
	}
	Which_GasCtrler_j++;
}
void reoilgasthread::ReadDataReoilgas_Version()
{
	len_uart_reoilgas = read(fd_uart_reoilgas,RecvBuff_Oilgas,sizeof(RecvBuff_Oilgas));

	unsigned int SCRC = 0;
	SCRC = CRC_Test(RecvBuff_Oilgas,len_uart_reoilgas);
	if((RecvBuff_Oilgas[len_uart_reoilgas-2] == ((SCRC & 0xff00)>>8)) && (RecvBuff_Oilgas[len_uart_reoilgas-1] == (SCRC & 0x00ff)))
	{
		//debug_read++;
		if(RecvBuff_Oilgas[0] == Address_Reoilgas[Reoilgas_Version_Set_Whichone])
		{
			emit Version_To_Mainwindow(RecvBuff_Oilgas[3],RecvBuff_Oilgas[4]);
			Lock_Mode_Reoilgas.lock();
			Flag_SendMode_Oilgas = 1;
			Lock_Mode_Reoilgas.unlock();
		}
		else
		{
			emit Version_To_Mainwindow(0,0);
			Lock_Mode_Reoilgas.lock();
			Flag_SendMode_Oilgas = 1;
			Lock_Mode_Reoilgas.unlock();
		}
	}
	else
	{
		emit Version_To_Mainwindow(0,0);
		Lock_Mode_Reoilgas.lock();
		Flag_SendMode_Oilgas = 1;
		Lock_Mode_Reoilgas.unlock();
	}
}
void reoilgasthread::ReadDataReoilgas_Setinfo()
{
	len_uart_reoilgas = read(fd_uart_reoilgas,RecvBuff_Oilgas,sizeof(RecvBuff_Oilgas));
	unsigned int SCRC = 0;
	SCRC = CRC_Test(RecvBuff_Oilgas,len_uart_reoilgas);
	if((RecvBuff_Oilgas[len_uart_reoilgas-2] == ((SCRC & 0xff00)>>8)) && (RecvBuff_Oilgas[len_uart_reoilgas-1] == (SCRC & 0x00ff)))
	{
		//debug_read++;
		if(RecvBuff_Oilgas[0] == Address_Reoilgas[Reoilgas_Version_Set_Whichone])
		{
			emit Setinfo_To_Mainwindow(RecvBuff_Oilgas[7],RecvBuff_Oilgas[8],RecvBuff_Oilgas[9],RecvBuff_Oilgas[10],RecvBuff_Oilgas[12],RecvBuff_Oilgas[15],RecvBuff_Oilgas[16],RecvBuff_Oilgas[17],RecvBuff_Oilgas[18],RecvBuff_Oilgas[20]);
			//qDebug()<<"receive"<<RecvBuff_Oilgas[9]<<RecvBuff_Oilgas[10];
			Lock_Mode_Reoilgas.lock();
			Flag_SendMode_Oilgas = 1;
			Lock_Mode_Reoilgas.unlock();
			//带屏幕无线气液比采集器读取设置
			if((Flag_Reoilgas_Version == 4)||(Flag_Reoilgas_Version == 5))
			{
				unsigned int i = (RecvBuff_Oilgas[0]-161)/4;
				unsigned int j = (RecvBuff_Oilgas[0]-161)%4;
				Fueling_Factor[i][j*2] = float(RecvBuff_Oilgas[7])+float(RecvBuff_Oilgas[8])/100;
				Gas_Factor[i][j*2] = float(RecvBuff_Oilgas[9])+float(RecvBuff_Oilgas[10])/100;
				Fueling_Factor[i][j*2+1] = float(RecvBuff_Oilgas[15])+float(RecvBuff_Oilgas[16])/100;
				Gas_Factor[i][j*2+1] = float(RecvBuff_Oilgas[17])+float(RecvBuff_Oilgas[18])/100;
				qDebug()<<Fueling_Factor[i][j*2] <<Gas_Factor[i][j*2] <<Fueling_Factor[i][j*2+1] <<Gas_Factor[i][j*2+1];
				emit reflash_setinfo();

			}
		}
		else
		{
			emit Setinfo_To_Mainwindow(0,0,0,0,0,0,0,0,0,0);
			Lock_Mode_Reoilgas.lock();
			Flag_SendMode_Oilgas = 1;
			Lock_Mode_Reoilgas.unlock();
		}
	}
	else
	{
		emit Setinfo_To_Mainwindow(0,0,0,0,0,0,0,0,0,0);
		Lock_Mode_Reoilgas.lock();
		Flag_SendMode_Oilgas = 1;
		Lock_Mode_Reoilgas.unlock();
	}
}
unsigned char reoilgasthread::Select_Sensor()
{
	unsigned char count = 0;
	if((Amount_Dispener == 0) ||((Amount_Dispener>0) && (Amount_Gasgun[Amount_Dispener-1] == 0)))
	{
		return 0xee;
	}
	if(Which_Dispener_i < Amount_Dispener)
	{
		if(Amount_Gasgun[Which_Dispener_i]%2)
		{
			if(Which_GasCtrler_j < (Amount_Gasgun[Which_Dispener_i]/2 + 1))
			{
				count = 4*Which_Dispener_i + Which_GasCtrler_j;   //默认一个加油机只能挂两个油气设备，扩展前端地址后，改为4×,  后来又改成4X
			}
			else
			{
				Which_Dispener_i++;
				Which_GasCtrler_j = 0;
				count = Select_Sensor();
			}
		}
		else
		{
			if(Which_GasCtrler_j < (Amount_Gasgun[Which_Dispener_i]/2))
			{
				count = 4*Which_Dispener_i + Which_GasCtrler_j;
			}
			else
			{
				Which_Dispener_i++;
				Which_GasCtrler_j = 0;
				count = Select_Sensor();
			}
		}
	}
	else
	{
		Which_Dispener_i = 0;
		count = Select_Sensor();
	}
	return count;
}


void reoilgasthread::SendDataReoilgas_v2()
{
	//debug_send++;

	if(!Flag_Askagain)
	{
		Flag_Whichone = Select_Sensor();
		//printf("Select Next Reoilgas Sensor!\n");
	}
	Lock_Mode_Reoilgas.lock();
	unsigned char flag_sendmode_temp = Flag_SendMode_Oilgas;
	Lock_Mode_Reoilgas.unlock();
	switch(flag_sendmode_temp)
	{
	case 1: if(Flag_Whichone == 0xee)
		{
			Flag_Timeto_CloseNeeded[2] = 0;
			break;
		}
		else
		{
			unsigned char SendBuff_Oilgas[8];
			SendBuff_Oilgas[0] = Address_Reoilgas[Flag_Whichone];
			SendBuff_Oilgas[1] = 0x03;
			SendBuff_Oilgas[2] = 0x00;
			SendBuff_Oilgas[3] = 0x00;
			SendBuff_Oilgas[4] = 0x00;
			SendBuff_Oilgas[5] = 0x0d;
			unsigned int SCRC = 0;
			SCRC = CRC_Test(SendBuff_Oilgas,8);
			SendBuff_Oilgas[6] = (SCRC & 0xff00) >> 8;
			SendBuff_Oilgas[7] = (SCRC & 0x00ff);
			write(fd_uart_reoilgas,SendBuff_Oilgas,sizeof(SendBuff_Oilgas));
			//                    for(uchar i = 0; i < 8; i++)           //DEBUG
			//                    {                                       //DEBUG
			//                        printf("%02x ",SendBuff_Oilgas[i]); //DEBUG
			//                    }
			//                    printf("ask oilgas!\n");
			if((Flag_Reoilgas_Version == 3)||(Flag_Reoilgas_Version == 5)) //无线
			{
				msleep(1300);
			}
			else
			{
				msleep(500);
			}
			ReadDataReoilgas_v2();
			break;
		}
		break;
	case 2:     //版本读取
	{
		unsigned char SendBuff_Oilgas[8];
		SendBuff_Oilgas[0] = Address_Reoilgas[Reoilgas_Version_Set_Whichone];
		SendBuff_Oilgas[1] = 0x03;
		SendBuff_Oilgas[2] = 0x00;
		SendBuff_Oilgas[3] = 0x64;
		SendBuff_Oilgas[4] = 0x00;
		SendBuff_Oilgas[5] = 0x01;
		unsigned int SCRC = 0;
		SCRC = CRC_Test(SendBuff_Oilgas,8);
		SendBuff_Oilgas[6] = (SCRC & 0xff00) >> 8;
		SendBuff_Oilgas[7] = (SCRC & 0x00ff);
		write(fd_uart_reoilgas,SendBuff_Oilgas,sizeof(SendBuff_Oilgas));
		if((Flag_Reoilgas_Version == 3)||(Flag_Reoilgas_Version == 5)) //无线
		{
			msleep(1400);
		}
		else
		{
			msleep(500);
		}
		ReadDataReoilgas_Version();
		Flag_Askagain = 0;
	}
		break;
	case 3:     //写设置
		if(Flag_Whichone == 0xee)
		{
			Flag_Timeto_CloseNeeded[2] = 0;
			Lock_Mode_Reoilgas.lock();
			Flag_SendMode_Oilgas = 1;
			Flag_Askagain = 0;
			Flag_Whichone_Temp = 0;
			Lock_Mode_Reoilgas.unlock();
			emit Reoilgas_Factor_Setover();
			break;
		}
		else
		{
			if(Flag_Whichone >= Flag_Whichone_Temp)
			{
				int year_h = 0;
				int year_l = 0;
				int month = 0;
				int day = 0;
				int hour = 0;
				int min = 0;
				int sec = 0;
				QDateTime current_datetime = QDateTime::currentDateTime();
				//TIME_POST = current_datetime.toString("yyyyMMddhhmmss");
				year_h = current_datetime.toString("yyyy").toInt()/256;
				year_l = current_datetime.toString("yyyy").toInt()%256;
				month = current_datetime.toString("MM").toInt();
				day = current_datetime.toString("dd").toInt();
				hour = current_datetime.toString("hh").toInt();
				min = current_datetime.toString("mm").toInt();
				sec = current_datetime.toString("ss").toInt();

				unsigned char SendBuff_Oilgas[35];
				SendBuff_Oilgas[0] = Address_Reoilgas[Flag_Whichone];
				SendBuff_Oilgas[1] = 0x10;
				SendBuff_Oilgas[2] = 0x00;
				SendBuff_Oilgas[3] = 0xC8;
				SendBuff_Oilgas[4] = 0x00;
				SendBuff_Oilgas[5] = 0x09;
				SendBuff_Oilgas[6] = 0x12;
				SendBuff_Oilgas[7] = 0x00;
				SendBuff_Oilgas[8] = 0x01;
				SendBuff_Oilgas[9] = 0x00;      //加油枪标识
				SendBuff_Oilgas[10] = 0x01;     //加油枪标识
				SendBuff_Oilgas[11] = Fueling_Factor[Which_Dispener_i][Which_GasCtrler_j*2];     //油量校正因子
				SendBuff_Oilgas[12] = (int)(Fueling_Factor[Which_Dispener_i][Which_GasCtrler_j*2]*100)%100;//油量校正因子
				SendBuff_Oilgas[13] = Gas_Factor[Which_Dispener_i][Which_GasCtrler_j*2];     //气量校正因子
				SendBuff_Oilgas[14] = (int)(Gas_Factor[Which_Dispener_i][Which_GasCtrler_j*2]*100)%100;    //气量校正因子
				//qDebug()<<"send "<<SendBuff_Oilgas[13]<<SendBuff_Oilgas[14];
				SendBuff_Oilgas[15] = 0x00;     //关枪继电器
				SendBuff_Oilgas[16] = Flag_Delay_State[Which_Dispener_i][Which_GasCtrler_j*2];     //关枪继电器         //目前未使用
				if(Flag_Gun_off == 0)//未开启关枪功能
				{
					SendBuff_Oilgas[16] = 0x00;
				}
				SendBuff_Oilgas[17] = 0x00;     //加油枪标识
				SendBuff_Oilgas[18] = 0x02;     //加油枪标识
				SendBuff_Oilgas[19] = Fueling_Factor[Which_Dispener_i][Which_GasCtrler_j*2+1];     //油量校正因子
				SendBuff_Oilgas[20] = (int)(Fueling_Factor[Which_Dispener_i][Which_GasCtrler_j*2+1]*100)%100;//油量校正因子
				SendBuff_Oilgas[21] = Gas_Factor[Which_Dispener_i][Which_GasCtrler_j*2+1];     //气量校正因子
				SendBuff_Oilgas[22] = (int)(Gas_Factor[Which_Dispener_i][Which_GasCtrler_j*2+1]*100)%100;     //气量校正因子
				SendBuff_Oilgas[23] = 0x00;     //关枪继电器
				SendBuff_Oilgas[24] = Flag_Delay_State[Which_Dispener_i][Which_GasCtrler_j*2+1];     //关枪继电器
				if(Flag_Gun_off == 0)//未开启关枪功能
				{
					SendBuff_Oilgas[24] = 0x00;
				}
				SendBuff_Oilgas[25] = year_h;
				SendBuff_Oilgas[26] = year_l;
				SendBuff_Oilgas[27] = month;
				SendBuff_Oilgas[28] = day;
				SendBuff_Oilgas[29] = hour;
				SendBuff_Oilgas[30] = min;
				SendBuff_Oilgas[31] = sec;
				SendBuff_Oilgas[32] = 0;
				//qDebug()<<"write date"<<year_h;
				//目前未使用
				unsigned int SCRC = 0;
				SCRC = CRC_Test(SendBuff_Oilgas,35);
				SendBuff_Oilgas[33] = (SCRC & 0xff00) >> 8;
				SendBuff_Oilgas[34] = (SCRC & 0x00ff);
				for(unsigned int i = 0;i<35;i++)
				{
					printf(" %02x",SendBuff_Oilgas[i]);
				}printf(" send over\n");
				write(fd_uart_reoilgas,SendBuff_Oilgas,sizeof(SendBuff_Oilgas));
				Flag_Whichone_Temp = Flag_Whichone;

				if((Flag_Reoilgas_Version == 3)||(Flag_Reoilgas_Version == 5)) //无线
				{
					msleep(1700);
				}
				else
				{
					msleep(500);
				}
				//msleep(500);  //200改为500，写设置的时候能够看到设置信息，不然刷新太快
				len_uart_reoilgas = read(fd_uart_reoilgas,RecvBuff_Oilgas,sizeof(RecvBuff_Oilgas));
				for(unsigned int i = 0;i<len_uart_reoilgas;i++)
				{
					printf(" %02x",RecvBuff_Oilgas[i]);
				}printf(" receive over\n");
				SCRC = CRC_Test(RecvBuff_Oilgas,len_uart_reoilgas);
				if((RecvBuff_Oilgas[0] == SendBuff_Oilgas[0]) && (RecvBuff_Oilgas[len_uart_reoilgas-2] == ((SCRC & 0xff00)>>8)) && (RecvBuff_Oilgas[len_uart_reoilgas-1] == (SCRC & 0x00ff)))
				{
					//debug_read++;
					Flag_Askagain = 0;
					emit Warn_UartWrong_Mainwindowdisp(Flag_Whichone,0);
					Flag_CommunicateError_Maindisp[Flag_Whichone] = 0;
					printf("set over! %d\n",Flag_Whichone);
					Which_GasCtrler_j++;
					Flag_Whichone_Temp++;
				}
				else
				{
					printf("set fail! %d\n",Flag_Whichone);
					Flag_Askagain = 1;
					Flag_CommunicateError_Maindisp[Flag_Whichone]++;
					if(Flag_CommunicateError_Maindisp[Flag_Whichone]>3)//5次改为3次
					{
						//sleep(1);
						emit Warn_UartWrong_Mainwindowdisp(Flag_Whichone,1);
						Flag_CommunicateError_Maindisp[Flag_Whichone] = 5;
						Flag_Askagain = 0; //M新增，防止设置不成功卡住地址
						Which_GasCtrler_j++;
						Flag_Whichone_Temp++;
					}
				}
			}
			else
			{
				Lock_Mode_Reoilgas.lock();
				Flag_SendMode_Oilgas = 1;
				Flag_Askagain = 0;
				Flag_Whichone_Temp = 0;
				Lock_Mode_Reoilgas.unlock();
				emit Reoilgas_Factor_Setover();
			}
		}
		break;
	case 4:     //写设置准备
	{
		Which_Dispener_i = 0;
		Which_GasCtrler_j = 0;
		Flag_Askagain = 0;
		Flag_Whichone_Temp = 0;
		Lock_Mode_Reoilgas.lock();
		Flag_SendMode_Oilgas = 3;
		Lock_Mode_Reoilgas.unlock();
	}
		break;
	case 5:     //设置读取
	{
		unsigned char SendBuff_Oilgas[8];
		SendBuff_Oilgas[0] = Address_Reoilgas[Reoilgas_Version_Set_Whichone];
		SendBuff_Oilgas[1] = 0x03;
		SendBuff_Oilgas[2] = 0x00;
		SendBuff_Oilgas[3] = 0xC8;
		SendBuff_Oilgas[4] = 0x00;
		SendBuff_Oilgas[5] = 0x09;
		unsigned int SCRC = 0;
		SCRC = CRC_Test(SendBuff_Oilgas,8);
		SendBuff_Oilgas[6] = (SCRC & 0xff00) >> 8;
		SendBuff_Oilgas[7] = (SCRC & 0x00ff);
		write(fd_uart_reoilgas,SendBuff_Oilgas,sizeof(SendBuff_Oilgas));
		if((Flag_Reoilgas_Version == 3)||(Flag_Reoilgas_Version == 5)) //无线
		{
			msleep(1300);
		}
		else
		{
			msleep(500);
		}
		ReadDataReoilgas_Setinfo();
	}
	}
}
void reoilgasthread::ReadDataReoilgas_v2()
{
	memset(RecvBuff_Oilgas,0,sizeof(char)*50);
	usleep(1);//不知道有什么用，就是想加上
	len_uart_reoilgas = read(fd_uart_reoilgas,RecvBuff_Oilgas,sizeof(RecvBuff_Oilgas));
	//数据分析
	unsigned int SCRC = 0;
	SCRC = CRC_Test(RecvBuff_Oilgas,len_uart_reoilgas);
	//    for(unsigned int i = 0;i<len_uart_reoilgas;i++)
	//    {
	//        printf("%02x ",RecvBuff_Oilgas[i]);
	//    }
	//    printf("%d--%02x %02x\n",len_uart_reoilgas,(SCRC & 0xff00)>>8,(SCRC & 0x00ff));
	if((RecvBuff_Oilgas[len_uart_reoilgas-2] == ((SCRC & 0xff00)>>8)) && (RecvBuff_Oilgas[len_uart_reoilgas-1] == (SCRC & 0x00ff)))
	{
		//debug_read++;
		//qDebug()<<"CRC is right";
		if(RecvBuff_Oilgas[0] == Address_Reoilgas[Flag_Whichone])
		{
			//全部按双枪判断
			unsigned char flag = RecvBuff_Oilgas[8];
			int oil_count;
			int gas_count;
			int oilgas_time;

			//post添加
			float countofgas_e = 0;
			float countofoil_e = 0;

			if((!flag) || Flag_Communicate_Error[Flag_Whichone])
			{
				oil_count = (RecvBuff_Oilgas[9]<<8) + RecvBuff_Oilgas[10];
				gas_count = (RecvBuff_Oilgas[11]<<8) + RecvBuff_Oilgas[12];
				oilgas_time = (RecvBuff_Oilgas[20]<<8) + RecvBuff_Oilgas[21];
				printf("reoilgas data output\n");
				if(Reoilgas_Data_Temp[Which_Dispener_i][Which_GasCtrler_j*2] == oil_count)
				{}
				else
				{
					//年
					Time_Reoilgas = QString("%1").arg((RecvBuff_Oilgas[13]*256+RecvBuff_Oilgas[14]),4,10,QLatin1Char('0'));//k为int型或char型都可
					//月
					Time_Reoilgas.append(QString("%1").arg(RecvBuff_Oilgas[15],2,10,QLatin1Char('0')));
					//日
					Time_Reoilgas.append(QString("%1").arg(RecvBuff_Oilgas[16],2,10,QLatin1Char('0')));
					//时
					Time_Reoilgas.append(QString("%1").arg(RecvBuff_Oilgas[17],2,10,QLatin1Char('0')));
					//分
					Time_Reoilgas.append(QString("%1").arg(RecvBuff_Oilgas[18],2,10,QLatin1Char('0')));
					//秒
					Time_Reoilgas.append(QString("%1").arg(RecvBuff_Oilgas[19],2,10,QLatin1Char('0')));
					qDebug()<<Time_Reoilgas<<"##############";
					Reoilgas_Data_Temp[Which_Dispener_i][Which_GasCtrler_j*2] = oil_count;
					add_value_reoilgas(Which_Dispener_i,Which_GasCtrler_j*2,oilgas_time,gas_count,oil_count);
					//post添加
					countofgas_e = Gas_Factor[Which_Dispener_i][Which_GasCtrler_j*2]*gas_count/1000.0;
					countofoil_e = Fueling_Factor[Which_Dispener_i][Which_GasCtrler_j*2]*oil_count/1000.0;
					if(countofoil_e >= 15)
					{
						network_oilgundata(DATAID_POST,QString::number(Which_Dispener_i+1),QString::number(Which_GasCtrler_j*2+1),
						                   QString::number((countofgas_e/countofoil_e)*100,'f',1),
						                   QString::number(countofgas_e/oilgas_time*60,'f',1),QString::number(countofgas_e,'f',1),
						                   QString::number(countofoil_e/oilgas_time*60,'f',1),QString::number(countofoil_e,'f',1),
						                   QString::number(Pre[1]*1000,'f',1));
					}
				}
			}
			flag = RecvBuff_Oilgas[27];
			if((!flag) || Flag_Communicate_Error[Flag_Whichone])
			{
				oil_count = (RecvBuff_Oilgas[28]<<8) + RecvBuff_Oilgas[29];
				gas_count = (RecvBuff_Oilgas[30]<<8) + RecvBuff_Oilgas[31];
				oilgas_time = (RecvBuff_Oilgas[39]<<8) + RecvBuff_Oilgas[40];
				printf("reoilgas data_2 output\n");
				if(Reoilgas_Data_Temp[Which_Dispener_i][Which_GasCtrler_j*2+1] == oil_count)
				{}
				else
				{
					//年
					Time_Reoilgas = QString("%1").arg((RecvBuff_Oilgas[32]*256+RecvBuff_Oilgas[33]),4,10,QLatin1Char('0'));//k为int型或char型都可
					//月
					Time_Reoilgas.append(QString("%1").arg(RecvBuff_Oilgas[34],2,10,QLatin1Char('0')));
					//日
					Time_Reoilgas.append(QString("%1").arg(RecvBuff_Oilgas[35],2,10,QLatin1Char('0')));
					//时
					Time_Reoilgas.append(QString("%1").arg(RecvBuff_Oilgas[36],2,10,QLatin1Char('0')));
					//分
					Time_Reoilgas.append(QString("%1").arg(RecvBuff_Oilgas[37],2,10,QLatin1Char('0')));
					//秒
					Time_Reoilgas.append(QString("%1").arg(RecvBuff_Oilgas[38],2,10,QLatin1Char('0')));
					qDebug()<<Time_Reoilgas;
					Reoilgas_Data_Temp[Which_Dispener_i][Which_GasCtrler_j*2+1] = oil_count;
					add_value_reoilgas(Which_Dispener_i,Which_GasCtrler_j*2+1,oilgas_time,gas_count,oil_count);
					//post添加
					countofgas_e = Gas_Factor[Which_Dispener_i][Which_GasCtrler_j*2+1]*gas_count/1000.0;
					countofoil_e = Fueling_Factor[Which_Dispener_i][Which_GasCtrler_j*2+1]*oil_count/1000.0;
					if(countofoil_e >= 15)
					{
						network_oilgundata(DATAID_POST,QString::number(Which_Dispener_i+1),QString::number(Which_GasCtrler_j*2+2),
						                   QString::number((countofgas_e/countofoil_e)*100,'f',1),
						                   QString::number(countofgas_e/oilgas_time*60,'f',1),QString::number(countofgas_e,'f',1),
						                   QString::number(countofoil_e/oilgas_time*60,'f',1),QString::number(countofoil_e,'f',1),
						                   QString::number(Pre[1]*1000,'f',1));
					}
				}
			}

			Flag_Communicate_Error[Flag_Whichone] = 0;
			emit Warn_UartWrong_Mainwindowdisp(Flag_Whichone,0);
			Flag_CommunicateError_Maindisp[Flag_Whichone] = 0;
		}
		else
		{
			//qDebug()<<"reoilgasthread read address wrong";
		}
	}
	else
	{
		//qDebug()<<"crc is wrong or no data return";
		Flag_Communicate_Error[Flag_Whichone] = 1;
		Flag_CommunicateError_Maindisp[Flag_Whichone]++;
		if(Flag_CommunicateError_Maindisp[Flag_Whichone] > 5) //5次改为5次
		{
			emit Warn_UartWrong_Mainwindowdisp(Flag_Whichone,1);
			Flag_CommunicateError_Maindisp[Flag_Whichone] = 5;
		}
	}
	Which_GasCtrler_j++;
}

/***
 * 问485变送器\fga100
 * */
void reoilgasthread::Ask_Sensor()
{
	msleep(100);
	//先读一次串口，把缓存读完
	unsigned char RecvBuff_init[100] = {0};
	len_uart_reoilgas = read(fd_uart_reoilgas,RecvBuff_init,sizeof(RecvBuff_init));

	if(Pre_tank_en == 1)
	{
		Pressure_AskNum = 1;
	}
	else if(Pre_pipe_en == 1)
	{
		Pressure_AskNum = 2;
	}
	else
	{
		Pressure_AskNum = 0;
	}
	if(Flag_Pressure_Transmitters_Mode == 2)
	{
		ask_pressure();
	}
	else
	{
		//不是新版的485传感器，其他协议没有添加到这里所以什么都不问了
	}

	msleep(50);
	if(Env_Gas_en == 1)
	{
		Fga1000_AskNum = 1;
	}
	else if(Num_Fga > 2)
	{
		Fga1000_AskNum = 3;
	}
	else
	{
		Fga1000_AskNum = 0;
	}
	ask_fga1000();
	msleep(50);
	ask_temperature();
	msleep(50);
}
void reoilgasthread::ask_pressure()
{
	unsigned char Flag_AskPre_Over = 1;
	//问压力变送器
	while(Flag_AskPre_Over)
	{
		if(Pressure_AskNum == 0)
		{
			Flag_AskPre_Over = 0;
			break;
		}
		unsigned char SendBuff_init[8] = {0x01,0x04,0x00,0x01,0x00,0x02,0x00,0x00};
		unsigned char RecvBuff_init[9] = {0};
		//写
		SendBuff_init[0] = Pressure_AskNum+0x29; //0x2A  0x2B
		unsigned int SCRC = 0;
		SCRC = CRC_Test(SendBuff_init,8);
		SendBuff_init[6] = (SCRC & 0xff00) >> 8;
		SendBuff_init[7] = (SCRC & 0x00ff);
		write(fd_uart_reoilgas,SendBuff_init,sizeof(SendBuff_init));
		if((Pressure_AskNum == 1)&&(Flag_TankPre_Type == 1))//油罐压力且为无线
		{
			msleep(1300);
		}
		else if((Pressure_AskNum == 2)&&(Flag_PipePre_Type == 1))//管线且为无线
		{
			msleep(1300);
		}
		else //有线
		{
			msleep(200);
		}
		//读
		len_uart_reoilgas = read(fd_uart_reoilgas,RecvBuff_init,sizeof(RecvBuff_init));
		SCRC = CRC_Test(RecvBuff_init,len_uart_reoilgas);
		if((RecvBuff_init[len_uart_reoilgas-2] == ((SCRC & 0xff00)>>8)) && (RecvBuff_init[len_uart_reoilgas-1] == (SCRC & 0x00ff)))//校验是否成功
		{
			printf("pressure duqu chenggong \n");
			if(Pressure_AskNum == 1)
			{
				pressure_value[0] = RecvBuff_init[3];
				pressure_value[1] = RecvBuff_init[4];
				pressure_value[2] = RecvBuff_init[5];
				pressure_value[3] = RecvBuff_init[6];
			}
			else if(Pressure_AskNum == 2)
			{
				pressure_value[4] = RecvBuff_init[3];
				pressure_value[5] = RecvBuff_init[4];
				pressure_value[6] = RecvBuff_init[5];
				pressure_value[7] = RecvBuff_init[6];
			}

			Flag_PreUartWrong[Pressure_AskNum-1] = 0;
			ReoilgasPreSta[Pressure_AskNum-1] = 0; //通信正常 后期分析之后在改变状态
			Pressure_AskNum++;     //问下一个地址
			if(Pressure_AskNum == 2)
			{
				if(Pre_pipe_en != 1)//管线传感器没有激活
				{
					Flag_AskPre_Over = 0;
				}
			}
			if(Pressure_AskNum >= 3)//退出循环
			{
				Flag_AskPre_Over = 0;
			}
		}
		else //ask again
		{
			//			qDebug()<<len_uart_reoilgas<<"^^^^^^^^^^^^^^^^^";
			//			for(unsigned int i = 0;i<9;i++)
			//			{
			//				printf("%02x ",RecvBuff_init[i]);
			//			}
			//printf("pressure jiaoyanshibai!!\n");
			Flag_PreUartWrong[Pressure_AskNum-1]++;
			if(Flag_PreUartWrong[Pressure_AskNum-1] >= 3)
			{
				Flag_PreUartWrong[Pressure_AskNum-1] = 0;
				ReoilgasPreSta[Pressure_AskNum-1] = 0x04; //通信故障
				Pressure_AskNum++;     //问下一个地址
				if(Pressure_AskNum == 2)
				{
					if(Pre_pipe_en != 1)//管线传感器没有激活
					{
						Flag_AskPre_Over = 0;
					}
				}
				if(Pressure_AskNum >= 3)//退出循环
				{
					Flag_AskPre_Over = 0;
				}
			}
		}

	}
	floating_point_conversion();
}

void reoilgasthread::ask_fga1000()
{
	unsigned char Flag_AskFga_Over = 1;
	while(Flag_AskFga_Over)
	{
		if(Fga1000_AskNum == 0)
		{
			break;
		}
		unsigned char SendBuff_init[8] = {0x01,0x03,0x00,0x00,0x00,0x02,0x00,0x00};
		unsigned char RecvBuff_init[9] = {0};
		//写
		SendBuff_init[0] = Fga1000_AskNum+0x20;//从0x21开始到0x28，共8个传感器
		unsigned int SCRC = 0;
		SCRC = CRC_Test(SendBuff_init,8);
		SendBuff_init[6] = (SCRC & 0xff00) >> 8;
		SendBuff_init[7] = (SCRC & 0x00ff);
		write(fd_uart_reoilgas,SendBuff_init,sizeof(SendBuff_init));
		if(Flag_Gas_Type == 1) //无线模式
		{
			msleep(1300);
		}
		else //有线模式
		{
			msleep(200);
		}

		len_uart_reoilgas = read(fd_uart_reoilgas,RecvBuff_init,sizeof(RecvBuff_init));
		SCRC = CRC_Test(RecvBuff_init,len_uart_reoilgas);
		if((RecvBuff_init[len_uart_reoilgas-2] == ((SCRC & 0xff00)>>8)) && (RecvBuff_init[len_uart_reoilgas-1] == (SCRC & 0x00ff)))//校验是否成功
		{
			printf("fga1000 duqu chenggong \n");
			Flag_FgaUartWrong[Fga1000_AskNum-1] = 0;
			ReoilGasFgaSta[Fga1000_AskNum-1] = RecvBuff_init[4];
			if(Fga1000_AskNum <= 2)
			{
				if((RecvBuff_init[4]==0x03)||(RecvBuff_init[4]==0x04)||(RecvBuff_init[4]==0x05) )//如果是通讯故障/传感器故障/探测器故障
				{
					ReoilGasFgaSta[Fga1000_AskNum-1] = RecvBuff_init[4];
					Fga1000_Value[Fga1000_AskNum-1] = 0;//数值设置为0
				}
				else
				{
					ReoilGasFgaSta[Fga1000_AskNum-1] = 0;//暂时判定为正常
					Fga1000_Value[Fga1000_AskNum-1] = RecvBuff_init[6]*2;
				}
			}
			else
			{
				ReoilGasFgaSta[Fga1000_AskNum-1] = RecvBuff_init[4];
				Fga1000_Value[Fga1000_AskNum-1] = RecvBuff_init[6];
			}

			Fga1000_AskNum++;
			if(Fga1000_AskNum == 2)
			{
				Fga1000_AskNum++;
			}
			if(Fga1000_AskNum>Num_Fga)
			{
				Flag_AskFga_Over = 0;//结束问询
			}
		}
		else //校验失败
		{
			printf("fga1000 jiaoiyan shibai \n");
			Flag_FgaUartWrong[Fga1000_AskNum-1]++;
			if(Flag_FgaUartWrong[Fga1000_AskNum-1] >= 3)
			{
				Flag_FgaUartWrong[Fga1000_AskNum-1] = 0;
				ReoilGasFgaSta[Fga1000_AskNum-1] = 0x04;//判断为通信故障
				Fga1000_Value[Fga1000_AskNum-1] = 0;//数值设置为0
				Fga1000_AskNum++;
				if(Fga1000_AskNum == 2)
				{
					Fga1000_AskNum++;
				}
				if(Fga1000_AskNum>Num_Fga)
				{
					Flag_AskFga_Over = 0;//结束问询
				}
			}

		}
	}

}
void reoilgasthread::ask_temperature()
{
	unsigned char Flag_AskTem_Over = 1;
	unsigned char SendBuff_init[8] = {0x01,0x03,0x00,0x00,0x00,0x02,0x00,0x00};
	unsigned char RecvBuff_init[9] = {0};
	//写  //只问一个地址0x13
	Temperature_AskNum = 1;//只有1个温度
	while (Flag_AskTem_Over)
	{
		if(Tem_tank_en == 1) //如果开启了
		{
			SendBuff_init[0] = Temperature_AskNum+0x2B; //温度地址从0x13开始
			unsigned int SCRC = 0;
			SCRC = CRC_Test(SendBuff_init,8);
			SendBuff_init[6] = (SCRC & 0xff00) >> 8;
			SendBuff_init[7] = (SCRC & 0x00ff);
			write(fd_uart_reoilgas,SendBuff_init,sizeof(SendBuff_init));
			if(Flag_TankTem_Type == 1) //无线模式
			{
				msleep(1300);
			}
			else //有线模式
			{
				msleep(200);
			}
			len_uart_reoilgas = read(fd_uart_reoilgas,RecvBuff_init,sizeof(RecvBuff_init));
			SCRC = CRC_Test(RecvBuff_init,len_uart_reoilgas);
			if((RecvBuff_init[len_uart_reoilgas-2] == ((SCRC & 0xff00)>>8)) && (RecvBuff_init[len_uart_reoilgas-1] == (SCRC & 0x00ff)))//校验是否成功
			{
				//printf("Temperature duqu chenggong \n");
				ReoilgasTemSta[Temperature_AskNum-1] = 0;
				Flag_TemUartWrong[Temperature_AskNum-1] = 0;
				//温度读取
				Tem[0] = 0;
				Tem[1] = 0;
				//温度读取
				Temperature_AskNum++;
				Flag_AskTem_Over = 0;//结束。退出循环
			}
			else //校验失败
			{
				//printf("Temperature jiaoiyan shibai \n");
				Flag_TemUartWrong[Temperature_AskNum-1]++;
				if(Flag_TemUartWrong[Temperature_AskNum-1] >= 3)
				{
					//温度读取
					Tem[0] = 0;
					Tem[1] = 0;
					//温度读取
					Flag_TemUartWrong[Temperature_AskNum-1] = 0;
					ReoilgasTemSta[Temperature_AskNum-1] = 0x04;//判断为通信故障
					Temperature_AskNum++;
					Flag_AskTem_Over = 0;//结束。退出循环
				}
			}
		}
		else
		{
			Flag_AskTem_Over = 0;
		}
	}
}

//浮点数转换 485
void reoilgasthread::floating_point_conversion()
{
	union{
		float f;
		char buf[4];
	}data;
	if(Pre_tank_en == 1)
	{
		data.buf[0] = pressure_value[0];
		data.buf[1] = pressure_value[1];
		data.buf[2] = pressure_value[3];
		data.buf[3] = pressure_value[2];
		if((data.f<200)&&(data.f>-200))//解决偶尔出现乱码
		{
			Pre[0] = data.f;                 //DEBUG
		}
		//qDebug()<<"_______________"<<data.f;  //DEBUG
		//printf("****1  %.4f****\n",Pre[0]);  //DEBUG

	}
	else
	{
		//没有设置则数据清零      //DEBUG
		Pre[0] = 0.0000;                         //DEBUG
		pressure_value[0] = 0;
		pressure_value[1] = 0;
		pressure_value[2] = 0;
		pressure_value[3] = 0;

	}
	if(Pre_pipe_en == 1)
	{
		data.buf[0] = pressure_value[5];
		data.buf[1] = pressure_value[4];
		data.buf[2] = pressure_value[7];
		data.buf[3] = pressure_value[6];
		if((data.f<200)&&(data.f>-200))//解决偶尔出现乱码
		{
			Pre[1] = data.f;  // 2号压力       //DEBUG
		}
		//qDebug()<<"================="<<data.f;  //DEBUG
		//printf("****2  %.4f****\n",Pre[1]);   //DEBUG
	}
	else
	{
		pressure_value[4] = 0;
		pressure_value[5] = 0;
		pressure_value[6] = 0;
		pressure_value[7] = 0;
		//没有设置则数据清零    //DEBUG
		Pre[1] = 0.0000;                     //DEBUG
	}

}
/*
 * 无线模块初始化
 * */
void reoilgasthread::D433T3D_init()
{
	gpio_high(1,22); //m1
	gpio_high(1,23); //m0
	sleep(1);
	unsigned char SendBuff_init[6] = {0xc0,0x00,0x00,0x1a,0x00,0x44};
	unsigned char RecvBuff_init[6] = {0};
	write(fd_uart_reoilgas,SendBuff_init,sizeof(SendBuff_init));
	sleep(1);
	len_uart_reoilgas = read(fd_uart_reoilgas,RecvBuff_init,sizeof(RecvBuff_init));
	if((SendBuff_init[0]==RecvBuff_init[0])&&(SendBuff_init[1]==RecvBuff_init[1])&&
	        (SendBuff_init[2]==RecvBuff_init[2])&&(SendBuff_init[3]==RecvBuff_init[3])&&
	        (SendBuff_init[4]==RecvBuff_init[4])&&(SendBuff_init[5]==RecvBuff_init[5]))
	{
		printf("wuxian peizhi chenggong!!\n");
	}
	else
	{
		printf("wuxian peizhi shibai!!\n");
	}
	gpio_low(1,22); //m1
	gpio_high(1,23); //m0
}

/**************同步带屏气液比采集器脉冲当量**************

 * *********************************/
void reoilgasthread::sync_data()
{
	unsigned int loop = 0;
	unsigned int loop_num = 0;
	unsigned int which_add = 0;
	for(unsigned int i = 0;i<Amount_Dispener;i++)
	{
		if(Amount_Gasgun[i]%2==0)
		{
			which_add = Amount_Gasgun[i]/2;
		}
		else
		{
			which_add = Amount_Gasgun[i]/2+1;
		}
		for(unsigned int j=0;j<which_add;j++)
		{
			qDebug()<<"ask tongbutongbutongbu";
			unsigned char SendBuff_Oilgas[8];
			SendBuff_Oilgas[0] = Address_Reoilgas[i*4+j];
			SendBuff_Oilgas[1] = 0x03;
			SendBuff_Oilgas[2] = 0x00;
			SendBuff_Oilgas[3] = 0xC8;
			SendBuff_Oilgas[4] = 0x00;
			SendBuff_Oilgas[5] = 0x09;
			unsigned int SCRC = 0;
			SCRC = CRC_Test(SendBuff_Oilgas,8);
			SendBuff_Oilgas[6] = (SCRC & 0xff00) >> 8;
			SendBuff_Oilgas[7] = (SCRC & 0x00ff);
			loop = 1;
			while(loop)
			{
				//问同步信息
				write(fd_uart_reoilgas,SendBuff_Oilgas,sizeof(SendBuff_Oilgas));
				if(Flag_Reoilgas_Version == 5) //无线
				{
					msleep(1800);
				}
				else
				{
					msleep(800);
				}
				//qDebug()<<"ask!!!!!!!!!!!!"<<Address_Reoilgas[i*4+j];
				//读取同步信息
				len_uart_reoilgas = read(fd_uart_reoilgas,RecvBuff_Oilgas,sizeof(RecvBuff_Oilgas));
				SCRC = CRC_Test(RecvBuff_Oilgas,len_uart_reoilgas);
				if((RecvBuff_Oilgas[len_uart_reoilgas-2] == ((SCRC & 0xff00)>>8)) && (RecvBuff_Oilgas[len_uart_reoilgas-1] == (SCRC & 0x00ff)))
				{
					//debug_read++;
					if(RecvBuff_Oilgas[0] == Address_Reoilgas[i*4+j])
					{
						unsigned int i = (RecvBuff_Oilgas[0]-161)/4;
						unsigned int j = (RecvBuff_Oilgas[0]-161)%4;
						Fueling_Factor[i][j*2] = float(RecvBuff_Oilgas[7])+float(RecvBuff_Oilgas[8])/100;
						Gas_Factor[i][j*2] = float(RecvBuff_Oilgas[9])+float(RecvBuff_Oilgas[10])/100;
						Fueling_Factor[i][j*2+1] = float(RecvBuff_Oilgas[15])+float(RecvBuff_Oilgas[16])/100;
						Gas_Factor[i][j*2+1] = float(RecvBuff_Oilgas[17])+float(RecvBuff_Oilgas[18])/100;
						qDebug()<<Fueling_Factor[i][j*2] <<Gas_Factor[i][j*2] <<Fueling_Factor[i][j*2+1] <<Gas_Factor[i][j*2+1];
						//同步界面是在systemset类创建的，所以要先把信号发送到systemset，然后发送到one_click_sync
						emit signal_sync_data(i,j,float(RecvBuff_Oilgas[7])+float(RecvBuff_Oilgas[8])/100,float(RecvBuff_Oilgas[9])+float(RecvBuff_Oilgas[10])/100,
						        float(RecvBuff_Oilgas[15])+float(RecvBuff_Oilgas[16])/100,float(RecvBuff_Oilgas[17])+float(RecvBuff_Oilgas[18])/100);
						emit reflash_setinfo();//记录到文件
						loop = 0;
					}
					else
					{
						loop_num++;
					}
				}
				else
				{
					loop_num++;
				}
				if(loop_num >= 4)
				{
					loop_num = 0;
					loop = 0;
					emit signal_sync_data(i,j,256,256,256,256);
					//qDebug()<<"no receive!!!!!!!!!!!!!!!!!!!!!!";
				}
			}
		}
	}
	Flag_WaitSync = 0; //结束同步
}

//ask690，泄漏部分
void reoilgasthread::ask_690()
{
	//设备数目初始化
	Flag_Jingdian_Enable = Flag_xieyou;
	Flag_IIE_Enable = Flag_IIE;
	unsigned int  SCRC = 0;
	//询问690状态  Count_Ask690 == 0 询问管线| ==1 油罐| == 2 防渗池| == 3 加油机
	if((Count_Ask_jingdian >= 4) && Flag_Ask_jingdian)
	{
		if(Flag_Jingdian_Enable)
		{
			Flag_Ask_jingdian = 0;
			Count_Ask_jingdian = 0;
			Flag_ASk_IIE = 1;//通讯故障，接下来问IIE,在读取数据那里也有一个置1
			Flag_Jingdian_uartwrong++;//二轮通信故障才通信故障
			if(Flag_Jingdian_uartwrong>=2)
			{
				Flag_Jingdian_uartwrong = 0;
				Data_Buf_Sencor_Pre[17] = 0xff;
			}
		}
		else
		{
			Flag_ASk_IIE = 1;//通讯故障，接下来问IIE,在读取数据那里也有一个置1
			Flag_Ask_jingdian = 0;
		}

	}
	if((Count_Ask_jingdian < 4) && Flag_Ask_jingdian)
	{
		if(Flag_Jingdian_Enable)
		{

			Ask_690_Buf[0] = 0xb1;
			SCRC = CRC_Test(Ask_690_Buf,8);
			Ask_690_Buf[6] = ((SCRC & 0xff00) >> 8);
			Ask_690_Buf[7] = (SCRC & 0x00ff);
			len_uart_reoilgas = write(fd_uart_reoilgas,Ask_690_Buf,8);

			Count_Ask_jingdian++;
			usleep(1000);
			return;
		}
		else
		{
			Flag_Ask_jingdian = 0;
			Count_Ask_jingdian = 0;
			Flag_Jingdian_uartwrong = 0;
			Flag_ASk_IIE = 1;//未设置，接下来问IIE,在读取数据那里也有一个置1
		}

	}
	//问IIE部分
	if((Flag_ASk_IIE == 1)&&(Count_Ask_IIE >= 4))//通讯故障
	{
		if(Flag_IIE_Enable)
		{
			Flag_IIE_uartwrong++;//二轮通信故障才通信故障  修改为4轮
			Flag_ASk_IIE = 0;
			Count_Ask_IIE = 0;
			if(Flag_IIE_uartwrong>=4)
			{
				Flag_IIE_uartwrong = 0;
				Sta_IIE[0] = 0xff;
				Sta_IIE[1]=0;Sta_IIE[2]=0;Sta_IIE[3]=0;Sta_IIE[4]=0;Sta_IIE[5]=0;Sta_IIE[6]=0;Sta_IIE[7]=0;Sta_IIE[8]=0;Sta_IIE[9]=0;Sta_IIE[10]=0;Sta_IIE[11]=0;
			}
		}
		else
		{
			Flag_ASk_IIE = 0;
		}
	}
	if((Flag_ASk_IIE == 1)&&(Count_Ask_IIE < 4))
	{
		if(Flag_IIE_Enable)
		{

			Ask_690_Buf[0] = 0xb2;
			SCRC = CRC_Test(Ask_690_Buf,8);
			Ask_690_Buf[6] = ((SCRC & 0xff00) >> 8);
			Ask_690_Buf[7] = (SCRC & 0x00ff);
			len_uart_reoilgas = write(fd_uart_reoilgas,Ask_690_Buf,8);

			Count_Ask_IIE++;
			usleep(1000);
			return;
		}
		else
		{
			Count_Ask_IIE = 0;
			Flag_IIE_uartwrong = 0;
			Flag_ASk_IIE = 0;

		}
	}

	switch(Count_Ask690)
	{
	case 0: if((count_pipe > 0) && (Count_Ask690_pipetank < count_pipe))
		{
			Ask_690_Buf[0] = Address[Count_Ask690_pipetank + 8];
			SCRC = CRC_Test(Ask_690_Buf,8);
			Ask_690_Buf[6] = ((SCRC & 0xff00) >> 8);
			Ask_690_Buf[7] = (SCRC & 0x00ff);
			len_uart_reoilgas = write(fd_uart_reoilgas,Ask_690_Buf,8);
			Flag_690 = 1;
		}
		break;
	case 1: if((count_tank > 0) && (Count_Ask690_pipetank < (count_tank + 8)))
		{
			Ask_690_Buf[0] = Address[Count_Ask690_pipetank - 8];
			SCRC = CRC_Test(Ask_690_Buf,8);
			Ask_690_Buf[6] = ((SCRC & 0xff00) >> 8);
			Ask_690_Buf[7] = (SCRC & 0x00ff);

			len_uart_reoilgas = write(fd_uart_reoilgas,Ask_690_Buf,8);
			Flag_690 = 1;
		}
		break;
	case 2:if((count_basin > 0) && (Count_Ask690_basindispener < count_basin))
		{
			Ask_690_Buf[0] = Address_Extend[Count_Ask690_basindispener + 8];
			SCRC = CRC_Test(Ask_690_Buf,8);
			Ask_690_Buf[6] = ((SCRC & 0xff00) >> 8);
			Ask_690_Buf[7] = (SCRC & 0x00ff);


			len_uart_reoilgas = write(fd_uart_reoilgas,Ask_690_Buf,8);
			Flag_690 = 1;
		}
		break;
	case 3:if((count_dispener > 0) && (Count_Ask690_basindispener < (count_dispener + 8)))
		{
			Ask_690_Buf[0] = Address_Extend[Count_Ask690_basindispener-8];
			SCRC = CRC_Test(Ask_690_Buf,8);
			Ask_690_Buf[6] = ((SCRC & 0xff00) >> 8);
			Ask_690_Buf[7] = (SCRC & 0x00ff);
			len_uart_reoilgas = write(fd_uart_reoilgas,Ask_690_Buf,8);
			Flag_690 = 1;
		}
	}
	//   Exchange = 1;

}
void reoilgasthread::read_690()
{
	unsigned int RCRC = 0;
	unsigned char CRC_L = 0,CRC_H = 0;
	unsigned char Data_Recv_690[20] = {0};

	float Pressure_data = 0;
	float Pressure_data_pre = 0;
	len_uart_reoilgas = read(fd_uart_reoilgas, Data_Recv_690, sizeof(Data_Recv_690));        //读取串口发来的数据
	//for(unsigned char i = 0 ; i<10 ; i++)
	//{
	//    printf("%02x ",Data_Recv_690[i]);
	//}
	//printf("******\n");
	//数据接收
	//qDebug()<<"ask690 read "<< len_uart;
	if (len_uart_reoilgas <= 0)
	{
		printf("read error \n");
	}

	if(len_uart_reoilgas == 7)                          //如果接收到的数据长度7，则进行数据处理，ＣＲＣ校验调试阶段不开
	{
		len_uart_reoilgas = 0;

		//数据crc校验
		RCRC = CRC_Test(Data_Recv_690,7);
		CRC_H = ((RCRC & 0xff00) >> 8);               //高八位
		CRC_L = (RCRC & 0x00ff);                      //低八位

		if((CRC_H == Data_Recv_690[5]) && (CRC_L == Data_Recv_690[6]))
		{
			if(Data_Recv_690[1] == READ_DATA)
			{
				//压力法罐
				if((Test_Method == 0x01) && (Data_Recv_690[0] < 9)) //如果是压力法而且是罐
				{
					Receive_Pressure_Data[(Data_Recv_690[0]-1)*2] = Data_Recv_690[2];
					Receive_Pressure_Data[(Data_Recv_690[0]-1)*2+1] = Data_Recv_690[3];

					Pressure_data_pre = Receive_Pressure_Data[(Data_Recv_690[0]-1)*2]*256 + Receive_Pressure_Data[(Data_Recv_690[0]-1)*2+1];
					//printf("***** %02f *****\n",Pressure_data_pre);
					Pressure_data =( Pressure_data_pre-410)*30.53/1000;   //计算出压力值
					//printf("***** %02f *****\n",Pressure_data);

					if(Pressure_data >= 70 && Pressure_data <= 100 )   //正常
					{
						aucAlarmState = 0x00;
						Flag_690 = 0;
					}

					if(Pressure_data < 0 || Pressure_data > 100 )   //传感器故障
					{
						aucAlarmState = 0x04 ;
						Flag_690 = 0;
					}
					if(Pressure_data >= 0 && Pressure_data < 35 )   //压力报警
					{
						aucAlarmState = 0x01;
						Flag_690 = 0;
					}
					if(Pressure_data >= 35 && Pressure_data < 70 )   //压力预警
					{
						aucAlarmState = 0x02;
						Flag_690 = 0;
					}
				}


				//非压力法的罐
				if((Data_Recv_690[2] == 0x02) && (Test_Method != 0x01) && (Data_Recv_690[0] < 9))//非压力法的罐
				{
					if(Flag_Ask_jingdian && (Data_Recv_690[0] == 0xb1))//实际这段判断没有用了，地址不可能是B1
					{
						Data_Buf_Sencor_Pre[17] = Data_Recv_690[4];
						Flag_Ask_jingdian = 0;
						Count_Ask_jingdian = 0x10;
					}
					else
						//Real_Time_Kpa_Temp = aucReceiveBuf_Sencor[3];    //用于存储实时压力值
					{
						switch (Data_Recv_690[4])
						{
						case 0x00: //O_NORMAL
							aucAlarmState = 0x00;
							Flag_690 = 0;
							break;

						case 0x02: //	O_ALARM_water
							aucAlarmState = 0x02;
							Flag_690 = 0;
							break;

						case 0x01:  //	O_ALARM_oil
							aucAlarmState = 0x01;
							Flag_690 = 0;
							break;

						case 0x04:	//O_ERROR
							aucAlarmState = 0x04;
							Flag_690 = 0;
							break;
						}
					}
				}

				//除罐之外的其他设备进行正常判断流程
				if((Data_Recv_690[2] == 0x02)&& (Data_Recv_690[0] > 8))//对于除罐之外的其他设备进行正常判断流程
				{
					if(Flag_Ask_jingdian && (Data_Recv_690[0] == 0xb1))
					{
						Data_Buf_Sencor_Pre[17] = Data_Recv_690[4];
						Flag_Ask_jingdian = 0;
						Count_Ask_jingdian = 0;
						Flag_Jingdian_uartwrong = 0;
						Flag_ASk_IIE = 1;//静电设备有返回，接下来问IIE
					}
					else
						//Real_Time_Kpa_Temp = aucReceiveBuf_Sencor[3];    //用于存储实时压力值
					{
						switch (Data_Recv_690[4])
						{
						case 0x00: //O_NORMAL
							aucAlarmState = 0x00;
							Flag_690 = 0;
							break;

						case 0x02: //	O_ALARM_water
							aucAlarmState = 0x02;
							Flag_690 = 0;
							break;

						case 0x01:  //	O_ALARM_oil
							aucAlarmState = 0x01;
							Flag_690 = 0;
							break;

						case 0x04:	//O_ERROR
							aucAlarmState = 0x04;
							Flag_690 = 0;
							break;
						}
					}
				}
			}
		}
	}
	if(len_uart == 12)//IIE收12位数据
	{
		len_uart = 0;

		//数据crc校验
		RCRC = CRC_Test(Data_Recv_690,12);
		CRC_H = ((RCRC & 0xff00) >> 8);               //高八位
		CRC_L = (RCRC & 0x00ff);                      //低八位

		if((CRC_H == Data_Recv_690[10]) && (CRC_L == Data_Recv_690[11]))
		{
			if((Data_Recv_690[0] == 0xb2)&&(Data_Recv_690[2] == 0x02))//IIE的地址
			{
				Flag_ASk_IIE = 0;
				Count_Ask_IIE = 0;
				Flag_IIE_uartwrong = 0;
				//数据处理
				Sta_IIE[0] = 0;
				Sta_IIE[1] = Data_Recv_690[3];//电压
				Sta_IIE[2] = Data_Recv_690[4];//电阻
				Sta_IIE[3] = Data_Recv_690[5];//稳油倒计时低位
				Sta_IIE[4] = Data_Recv_690[6];//稳油倒计时高位
				Sta_IIE[5] = Data_Recv_690[7];//人员值守倒计时
				Sta_IIE[6] = Data_Recv_690[8];//状态信息
				Sta_IIE[7] = Data_Recv_690[9];//屏蔽状态信息
			}
		}
	}

}

void reoilgasthread::send_data_690()
{
	unsigned tank_ads;//油罐罐号
	if(Flag_690)
	{
		for(uchar i = 0 ; i < 2; i ++)  //三次通讯故障报通讯故障
		{
			ask_690();
			msleep(90);
			if(Flag_690 == 0 )
			{
				break;
			}  //如果回数据了，则退出当前循环，不必再重新问了。
		}
		if(Flag_690)
		{
			aucAlarmState = 0x0f;//通讯故障
			Flag_690 = 0;
			if ((Test_Method == 1) &&( Count_Ask690_pipetank > 7 && Count_Ask690_pipetank < 17)) //如果是压力法而且是罐的通信故障
			{
				tank_ads = Count_Ask690_pipetank -8;
				Receive_Pressure_Data[tank_ads*2] = 0xf0;
			}
		}
	}
	//数据预处理，与初始版本的19字节数组保持一致
	switch(Count_Ask690)
	{
	    case 0: if((count_pipe > 0) && (Count_Ask690_pipetank < count_pipe))
		        {
			        if(Count_Ask690_pipetank%2)
					{
						if(aucAlarmState == 0x00){Data_Buf_Sencor_Pre[Count_Ask690_pipetank/2+1]&=0xF0;}
						if(aucAlarmState == 0x01){Data_Buf_Sencor_Pre[Count_Ask690_pipetank/2+1]&=0xF0;Data_Buf_Sencor_Pre[Count_Ask690_pipetank/2+1]|=0x01;}//油报警
						if(aucAlarmState == 0x02){Data_Buf_Sencor_Pre[Count_Ask690_pipetank/2+1]&=0xF0;Data_Buf_Sencor_Pre[Count_Ask690_pipetank/2+1]|=0x02;}//水报警
						if(aucAlarmState == 0x04){Data_Buf_Sencor_Pre[Count_Ask690_pipetank/2+1]&=0xF0;Data_Buf_Sencor_Pre[Count_Ask690_pipetank/2+1]|=0x04;}//传感器故障
						if(aucAlarmState == 0x0F){Data_Buf_Sencor_Pre[Count_Ask690_pipetank/2+1]&=0xF0;Data_Buf_Sencor_Pre[Count_Ask690_pipetank/2+1]|=0x0F;}//通信故障
					}
					else
					{
						if(aucAlarmState == 0x00){Data_Buf_Sencor_Pre[Count_Ask690_pipetank/2+1]&=0x0f;}
						if(aucAlarmState == 0x01){Data_Buf_Sencor_Pre[Count_Ask690_pipetank/2+1]&=0x0f;Data_Buf_Sencor_Pre[Count_Ask690_pipetank/2+1]|=0x10;}//油报警
						if(aucAlarmState == 0x02){Data_Buf_Sencor_Pre[Count_Ask690_pipetank/2+1]&=0x0f;Data_Buf_Sencor_Pre[Count_Ask690_pipetank/2+1]|=0x20;}//水报警
						if(aucAlarmState == 0x04){Data_Buf_Sencor_Pre[Count_Ask690_pipetank/2+1]&=0x0f;Data_Buf_Sencor_Pre[Count_Ask690_pipetank/2+1]|=0x40;}//传感器故障
						if(aucAlarmState == 0x0F){Data_Buf_Sencor_Pre[Count_Ask690_pipetank/2+1]&=0x0f;Data_Buf_Sencor_Pre[Count_Ask690_pipetank/2+1]|=0xF0;}//通信故障
					}

					Count_Ask690_pipetank++;
					if(Count_Ask690_pipetank >= count_pipe)
					{
						Count_Ask690_pipetank = 8;
						Count_Ask690 = 1;
						for(uchar i = count_pipe; i<8 ; i++)
						{
							if(i%2)
							{ Data_Buf_Sencor_Pre[i/2+1]&=0xF0;}
							else
							{ Data_Buf_Sencor_Pre[i/2+1]&=0x0F;}
						}
					}
		        }
		        else
		        {
			        Count_Ask690_pipetank = 8;
					Count_Ask690 = 1;
					for(uchar i = count_pipe; i<8 ; i++)
					{
						if(i%2)
						{ Data_Buf_Sencor_Pre[i/2+1]&=0xF0;}
						else
						{ Data_Buf_Sencor_Pre[i/2+1]&=0x0F;}
					}
		        }
		        break;
	    case 1: if((count_tank > 0) && (Count_Ask690_pipetank < (count_tank + 8)))
		        {
			        if(Count_Ask690_pipetank%2)                                                                              //取余数，判断奇偶性？更新送显示BUF，原来三方通信的时候需要send出去，这里好像没必要了。
					{
						if(aucAlarmState == 0x00){Data_Buf_Sencor_Pre[Count_Ask690_pipetank/2+1]&=0xF0;}                                               //正常
						if(aucAlarmState == 0x01){Data_Buf_Sencor_Pre[Count_Ask690_pipetank/2+1]&=0xF0;Data_Buf_Sencor_Pre[Count_Ask690_pipetank/2+1]|=0x01;}//油报警
						if(aucAlarmState == 0x02){Data_Buf_Sencor_Pre[Count_Ask690_pipetank/2+1]&=0xF0;Data_Buf_Sencor_Pre[Count_Ask690_pipetank/2+1]|=0x02;}//水报警
						if(aucAlarmState == 0x04){Data_Buf_Sencor_Pre[Count_Ask690_pipetank/2+1]&=0xF0;Data_Buf_Sencor_Pre[Count_Ask690_pipetank/2+1]|=0x04;}//传感器故障
						if(aucAlarmState == 0x0F){Data_Buf_Sencor_Pre[Count_Ask690_pipetank/2+1]&=0xF0;Data_Buf_Sencor_Pre[Count_Ask690_pipetank/2+1]|=0x0F;}//通信故障
					}
					else
					{
						if(aucAlarmState == 0x00){Data_Buf_Sencor_Pre[Count_Ask690_pipetank/2+1]&=0x0f;}                                               //正常
					   if(aucAlarmState == 0x01){Data_Buf_Sencor_Pre[Count_Ask690_pipetank/2+1]&=0x0f;Data_Buf_Sencor_Pre[Count_Ask690_pipetank/2+1]|=0x10;}//油报警
					   if(aucAlarmState == 0x02){Data_Buf_Sencor_Pre[Count_Ask690_pipetank/2+1]&=0x0f;Data_Buf_Sencor_Pre[Count_Ask690_pipetank/2+1]|=0x20;}//水报警
					   if(aucAlarmState == 0x04){Data_Buf_Sencor_Pre[Count_Ask690_pipetank/2+1]&=0x0f;Data_Buf_Sencor_Pre[Count_Ask690_pipetank/2+1]|=0x40;}//传感器故障
					   if(aucAlarmState == 0x0F){Data_Buf_Sencor_Pre[Count_Ask690_pipetank/2+1]&=0x0f;Data_Buf_Sencor_Pre[Count_Ask690_pipetank/2+1]|=0xF0;}//通信故障
					}

					Count_Ask690_pipetank++;
					if(Count_Ask690_pipetank >= (count_tank + 8))
					{
						Count_Ask690_pipetank = 0;
						Count_Ask690 = 2;
						for(uchar i=(count_tank+8); i<16 ; i++)
						{
							if(i%2)
							{ Data_Buf_Sencor_Pre[i/2+1]&=0xF0;}
							else
							{ Data_Buf_Sencor_Pre[i/2+1]&=0x0F; }
						}
					}
		        }
		        else
		        {
			        Count_Ask690_pipetank = 0;
					Count_Ask690 = 2;
					for(uchar i=(count_tank+8); i<16 ; i++)
					{
						if(i%2)
						{ Data_Buf_Sencor_Pre[i/2+1]&=0xF0; }
						else
						{ Data_Buf_Sencor_Pre[i/2+1]&=0x0F;}
					}
		        }
		        break;
	    case 2: if((count_basin > 0) && (Count_Ask690_basindispener < count_basin))
		        {
			        if((Count_Ask690_basindispener + 16)%2)
					{
						if(aucAlarmState == 0x00){Data_Buf_Sencor_Pre[(Count_Ask690_basindispener + 16)/2+1]&=0xF0;}                                               //正常
						if(aucAlarmState == 0x01){Data_Buf_Sencor_Pre[(Count_Ask690_basindispener + 16)/2+1]&=0xF0;Data_Buf_Sencor_Pre[(Count_Ask690_basindispener + 16)/2+1]|=0x01;}//油报警
						if(aucAlarmState == 0x02){Data_Buf_Sencor_Pre[(Count_Ask690_basindispener + 16)/2+1]&=0xF0;Data_Buf_Sencor_Pre[(Count_Ask690_basindispener + 16)/2+1]|=0x02;}//水报警
						if(aucAlarmState == 0x04){Data_Buf_Sencor_Pre[(Count_Ask690_basindispener + 16)/2+1]&=0xF0;Data_Buf_Sencor_Pre[(Count_Ask690_basindispener + 16)/2+1]|=0x04;}//传感器故障
						if(aucAlarmState == 0x0F){Data_Buf_Sencor_Pre[(Count_Ask690_basindispener + 16)/2+1]&=0xF0;Data_Buf_Sencor_Pre[(Count_Ask690_basindispener + 16)/2+1]|=0x0F;}//通信故障
					}
					else
					{
						if(aucAlarmState == 0x00){Data_Buf_Sencor_Pre[(Count_Ask690_basindispener + 16)/2+1]&=0x0f;}                                               //正常
					   if(aucAlarmState == 0x01){Data_Buf_Sencor_Pre[(Count_Ask690_basindispener + 16)/2+1]&=0x0f;Data_Buf_Sencor_Pre[(Count_Ask690_basindispener + 16)/2+1]|=0x10;}//油报警
					   if(aucAlarmState == 0x02){Data_Buf_Sencor_Pre[(Count_Ask690_basindispener + 16)/2+1]&=0x0f;Data_Buf_Sencor_Pre[(Count_Ask690_basindispener + 16)/2+1]|=0x20;}//水报警
					   if(aucAlarmState == 0x04){Data_Buf_Sencor_Pre[(Count_Ask690_basindispener + 16)/2+1]&=0x0f;Data_Buf_Sencor_Pre[(Count_Ask690_basindispener + 16)/2+1]|=0x40;}//传感器故障
					   if(aucAlarmState == 0x0F){Data_Buf_Sencor_Pre[(Count_Ask690_basindispener + 16)/2+1]&=0x0f;Data_Buf_Sencor_Pre[(Count_Ask690_basindispener + 16)/2+1]|=0xF0;}//通信故障
					}
					Count_Ask690_basindispener++;
					if(Count_Ask690_basindispener >= count_basin)
					{
						Count_Ask690_basindispener = 8;
						Count_Ask690 = 3;
						for(uchar i = (count_basin + 16); i<24 ; i++)
						{
							if(i%2)
							{ Data_Buf_Sencor_Pre[i/2+1]&=0xF0;}
							else
							{ Data_Buf_Sencor_Pre[i/2+1]&=0x0F; }
						}
					}
		        }
		        else
		        {
			        Count_Ask690_basindispener = 8;
					Count_Ask690 =3;
					for(uchar i = (count_basin + 16); i<24 ; i++)
					{
						if(i%2)
						{ Data_Buf_Sencor_Pre[i/2+1]&=0xF0;}
						else
						{ Data_Buf_Sencor_Pre[i/2+1]&=0x0F;}
					}
		        }
		        break;
	    case 3: if((count_dispener > 0) && (Count_Ask690_basindispener < (count_dispener + 8)))
		        {
			        if((Count_Ask690_basindispener + 16)%2)
					{
						if(aucAlarmState == 0x00){Data_Buf_Sencor_Pre[(Count_Ask690_basindispener + 16)/2+1]&=0xF0;}                                               //正常
						if(aucAlarmState == 0x01){Data_Buf_Sencor_Pre[(Count_Ask690_basindispener + 16)/2+1]&=0xF0;Data_Buf_Sencor_Pre[(Count_Ask690_basindispener + 16)/2+1]|=0x01;}//油报警
						if(aucAlarmState == 0x02){Data_Buf_Sencor_Pre[(Count_Ask690_basindispener + 16)/2+1]&=0xF0;Data_Buf_Sencor_Pre[(Count_Ask690_basindispener + 16)/2+1]|=0x02;}//水报警
						if(aucAlarmState == 0x04){Data_Buf_Sencor_Pre[(Count_Ask690_basindispener + 16)/2+1]&=0xF0;Data_Buf_Sencor_Pre[(Count_Ask690_basindispener + 16)/2+1]|=0x04;}//传感器故障
						if(aucAlarmState == 0x0F){Data_Buf_Sencor_Pre[(Count_Ask690_basindispener + 16)/2+1]&=0xF0;Data_Buf_Sencor_Pre[(Count_Ask690_basindispener + 16)/2+1]|=0x0F;}//通信故障
					}
					else
					{
						if(aucAlarmState == 0x00){Data_Buf_Sencor_Pre[(Count_Ask690_basindispener + 16)/2+1]&=0x0f;}                                               //正常
					   if(aucAlarmState == 0x01){Data_Buf_Sencor_Pre[(Count_Ask690_basindispener + 16)/2+1]&=0x0f;Data_Buf_Sencor_Pre[(Count_Ask690_basindispener + 16)/2+1]|=0x10;}//油报警
					   if(aucAlarmState == 0x02){Data_Buf_Sencor_Pre[(Count_Ask690_basindispener + 16)/2+1]&=0x0f;Data_Buf_Sencor_Pre[(Count_Ask690_basindispener + 16)/2+1]|=0x20;}//水报警
					   if(aucAlarmState == 0x04){Data_Buf_Sencor_Pre[(Count_Ask690_basindispener + 16)/2+1]&=0x0f;Data_Buf_Sencor_Pre[(Count_Ask690_basindispener + 16)/2+1]|=0x40;}//传感器故障
					   if(aucAlarmState == 0x0F){Data_Buf_Sencor_Pre[(Count_Ask690_basindispener + 16)/2+1]&=0x0f;Data_Buf_Sencor_Pre[(Count_Ask690_basindispener + 16)/2+1]|=0xF0;}//通信故障
					}
					Count_Ask690_basindispener++;
					if(Count_Ask690_basindispener >= (count_dispener + 8))
					{
						Count_Ask690_basindispener = 0;
						Count_Ask690 = 0;

						Flag_Ask_jingdian = 1;
						Count_Ask_jingdian = 0;
						for(uchar i=(count_dispener+24); i<32 ; i++)
						{
							if(i%2)
							{ Data_Buf_Sencor_Pre[i/2+1]&=0xF0;}
							else
							{ Data_Buf_Sencor_Pre[i/2+1]&=0x0F;}
						}
					}
		        }
		        else
		        {
			        Count_Ask690_basindispener = 0;
					Count_Ask690 = 0;
					Flag_Ask_jingdian = 1;
					Count_Ask_jingdian = 0;

					for(uchar i=(count_dispener+24); i<32 ; i++)
					{
						if(i%2)
						{ Data_Buf_Sencor_Pre[i/2+1]&=0xF0;}
						else
						{ Data_Buf_Sencor_Pre[i/2+1]&=0x0F;}
					}
		        }
	}

	//准备发送数据
	Data_Buf_Sencor_Pre[0] = 0xa0; //添加的手动校验
	Data_Buf_Sencor_Pre[18] = 0xaa;
	Data_Buf_Sencor_Pre[19] = 0xaa;
	for(uchar i = 0;i < 20;i++)
	{
		Ptr_Ask690[i] = Data_Buf_Sencor_Pre[i];
	}
	for(uchar i = 0;i < 16;i++)
	{
		Ptr_Ask690[i+20] = Receive_Pressure_Data[i];
	}
	Ptr_Ask690[36] = Sta_IIE[0];//IIE通讯
	Ptr_Ask690[37] = Sta_IIE[6];//IIE状态信息
	Ptr_Ask690[38] = Sta_IIE[7];//IIE屏蔽信息
	Ptr_Ask690[39] = Sta_IIE[1];//IIE电压
	Ptr_Ask690[40] = Sta_IIE[2];//IIE电阻
	Ptr_Ask690[41] = Sta_IIE[3];//IIE稳油计时低位
	Ptr_Ask690[42] = Sta_IIE[4];//IIE稳油计时高位
	Ptr_Ask690[43] = Sta_IIE[5];//IIE人员值守计时

//	for(unsigned char i = 0 ; i<44 ; i++)//加IIE变为44个数据
//	{
//		printf("%02x ",Ptr_Ask690[i]);
//	}
}
/**************网络相关******************
 * id      未使用
 * jyjid   加油机编号   填充到数组中记得减1
 * jyqid   加油枪编号   填充到数组中记得减1
 * al      气液比
 * qls     气体流速
 * qll     气体流量
 * yls     油流速
 * yll     油流量
 * yz      液阻
 * *********************************/
void reoilgasthread::network_oilgundata(QString id, QString jyjid, QString jyqid, QString al, QString qls, QString qll, QString yls, QString yll, QString yz)
{
	if(net_state == 0) //如果有网线连接
	{

		qDebug()<<"network send oilgundata!"<< Flag_Network_Send_Version;
		if(Flag_Network_Send_Version == 0) //福建协议
		{
			id = id;//没有用，为了 一个警告
			emit Send_Oilgundata(DATAID_POST,jyjid,jyqid,al,qls,qll,yls,yll,yz);
		}
		if(Flag_Network_Send_Version == 1)  //广州协议
		{
			float send_al = al.toFloat();
			QString gun_num = QString("%1").arg((Mapping[(jyjid.toInt()-1)*8+jyqid.toInt()-1]),4,10,QLatin1Char('0'));//k为int型或char型都可
			gun_num.prepend("q");
			if((send_al<=120)&&(send_al>=100))
			{
				refueling_gun_data(gun_num,al.append("0"),qls.append("0"),qll.append("0"),yls.append("0"),yll.append("0"),yz.append("0"));//.append保证小数点后两位
			}
			else
			{
				if(Flag_Shield_Network == 1)//屏蔽状态
				{
					float time = (yll.toFloat())/(yls.toFloat());
					int al_num = qrand()%(12000-10000);//用1.0~1.2之间的随机数代替
					float al_xiuzheng = al_num+10000;
					float send_qll = (yll.toFloat())*(al_xiuzheng/10000);
					float send_qls = send_qll/time;
					qDebug()<<al_xiuzheng<<send_qll<<send_qls<<time;
					refueling_gun_data(gun_num,QString::number((al_xiuzheng/100),'f',2),
					                   QString::number(send_qls,'f',2),
					                   QString::number(send_qll,'f',2),
					                   yls.append("0"),yll.append("0"),yz.append("0"));

				}
				else
				{
					refueling_gun_data(gun_num,al.append("0"),qls.append("0"),qll.append("0"),yls.append("0"),yll.append("0"),yz.append("0"));//.append保证小数点后两位
				}
			}

		}
		if(Flag_Network_Send_Version == 2)  //重庆协议
		{
			float send_al = (al.toFloat())/100;
			QString gun_num = QString("%1").arg((Mapping[(jyjid.toInt()-1)*8+jyqid.toInt()-1]),3,10,QLatin1Char('0'));//k为int型或char型都可
			QString gas_tem;
			if(Tem_tank_en == 1)
			{
				gas_tem = QString::number(Tem[0],'f',1);
			}
			else {
				gas_tem = "0";
			}
			if((send_al<=NormalAL_High)&&(send_al>=NormalAL_Low))
			{
				refueling_gun_data_cq(gun_num,QString::number(send_al,'f',2),qls.append("0"),qll.append("0"),yls.append("0"),yll.append("0"),"0", gas_tem,yz.append("0"));//.append保证小数点后两位
			}
			else
			{
				if(Flag_Shield_Network == 1)//屏蔽状态
				{
					float time = (yll.toFloat())/(yls.toFloat());
					int al_num = qrand()%(12000-10000);//用1.0~1.2之间的随机数代替
					float al_xiuzheng = al_num+10000;
					float send_qll = (yll.toFloat())*(al_xiuzheng/10000);
					float send_qls = send_qll/time;
					qDebug()<<al_xiuzheng<<send_qll<<send_qls<<time;
					refueling_gun_data_cq(gun_num,QString::number((al_xiuzheng/10000),'f',2),
					                      QString::number(send_qls,'f',2),
					                      QString::number(send_qll,'f',2),
					                      yls.append("0"),
					                      yll.append("0"),
					                      "0",
					                      gas_tem,
					                      yz.append("0"));//.append保证小数点后两位
				}
				else
				{
					refueling_gun_data_cq(gun_num,QString::number(send_al,'f',2),qls.append("0"),qll.append("0"),yls.append("0"),yll.append("0"),"0", gas_tem,yz.append("0"));//.append保证小数点后两位
				}
			}
		}
		if(Flag_Network_Send_Version == 3) //唐山协议，与福建相同
		{
			emit Send_Oilgundata(DATAID_POST,jyjid,jyqid,al,qls,qll,yls,yll,yz);
		}
		if(Flag_Network_Send_Version == 4) //湖南协议，与福建相同
		{
			emit Send_Oilgundata_HuNan(DATAID_POST,"date_kong",jyjid,jyqid,al,qls,qll,yls,yll,"NULL","NULL",yz);
		}
		if(Flag_Network_Send_Version == 5) //江门协议 与唐山协议，与福建相同
		{
			emit Send_Oilgundata(DATAID_POST,jyjid,jyqid,al,qls,qll,yls,yll,yz);
		}


		if(Flag_MyServerEn == 1)  //myserver协议
		{
			float send_al = al.toFloat();
			//用来获取上传的序号
			unsigned int gun_num_true = 0;
			unsigned int gun_num_send = 0;
			for(int i = 0;i<Amount_Dispener;i++)
			{
				for(int j = 0;j<Amount_Gasgun[i];j++)
				{
					gun_num_true++;
					if((jyjid.toInt()-1 == i)&&((jyqid.toInt()-1) == j))
					{
						//qDebug()<<i<<j<<"break"<<gun_num_true;
						gun_num_send = gun_num_true;
						break;
					}
				}
			}
			QString gun_num = QString("%1").arg((gun_num_send),4,10,QLatin1Char('0'));//k为int型或char型都可
			gun_num.prepend("q");
			if((send_al<=120)&&(send_al>=100))
			{
				refueling_gun_data(gun_num,al.append("0"),qls.append("0"),qll.append("0"),yls.append("0"),yll.append("0"),yz.append("0"));//.append保证小数点后两位
			}
			else
			{
				if(Flag_Shield_Network == 1)//屏蔽状态
				{
					float time = (yll.toFloat())/(yls.toFloat());
					int al_num = qrand()%(12000-10000);//用1.0~1.2之间的随机数代替
					float al_xiuzheng = al_num+10000;
					float send_qll = (yll.toFloat())*(al_xiuzheng/10000);
					float send_qls = send_qll/time;
					qDebug()<<al_xiuzheng<<send_qll<<send_qls<<time;
					refueling_gun_data(gun_num,QString::number((al_xiuzheng/100),'f',2),
					                   QString::number(send_qls,'f',2),
					                   QString::number(send_qll,'f',2),
					                   yls.append("0"),yll.append("0"),yz.append("0"));

				}
				else
				{
					refueling_gun_data_myserver(gun_num,al.append("0"),qls.append("0"),qll.append("0"),yls.append("0"),yll.append("0"),yz.append("0"));//.append保证小数点后两位
				}
			}

		}
	}
}






