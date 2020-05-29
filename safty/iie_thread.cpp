#include "iie_thread.h"
#include "config.h"
#include "mainwindow.h"
#include "serial.h"
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <QMutex>

QMutex SendLock;

unsigned char send_buf[8] = {0x00,0x04,0x00,0x02,0x00,0x02,0x00,0x00};//问传感器的数组
unsigned char receive_buf[16] = {0};//传感器但会的数据
int fd_uart_IIE;
int ret_uart_IIE;
int len_uart_IIE;
int SCRC = 0;
int RCRC = 0,RCRC_H = 0,RCRC_L = 0;

unsigned char sta_psa2[2] = {0};
unsigned char sta_IIE[8] = {0};
unsigned char sta_valve[5] = {0};
unsigned char uart_sta_psa2 = 0;
unsigned char uart_sta_IIE = 0;
unsigned char uart_sta_valve = 0;

IIE_thread::IIE_thread(QObject *parent):
    QThread(parent)
{

}
void IIE_thread::run()
{
	uart_init();//初始化串口
	while (is_runnable)
	{
		if(Flag_Psa2 == 1)
		{
			ask_psa2();
		}
		msleep(10);
		if(Flag_IIE == 1)
		{
			ask_IIE();
		}
		msleep(10);
		if(Flag_Valve == 1)
		{
			ask_valve();
		}
		msleep(10);
		send_data();
	}
	qDebug()<<"IIE Thread Will Quit!!";
}
void IIE_thread::stop()
{
	is_runnable = false;
}
/**************
 * 人体静电
 * ***********/
void IIE_thread::ask_psa2()
{
	send_buf[0] = 0xb1;
	send_buf[1] = 0x04;
	send_buf[2] = 0x00;
	send_buf[3] = 0x02;
	send_buf[4] = 0x00;
	send_buf[5] = 0x02;

	SCRC = CRC_Test(send_buf,8);
	send_buf[6] = ((SCRC & 0xff00) >> 8);
	send_buf[7] = (SCRC & 0x00ff);
	len_uart_IIE = write(fd_uart_IIE,send_buf,8);

	msleep(200);
	memset(receive_buf,0,sizeof(char)*16);//清空数据

	len_uart_IIE = read(fd_uart_IIE, receive_buf, sizeof(receive_buf));        //读取串口发来的数据
	if(len_uart_IIE > 0)
	{
		//数据crc校验
		RCRC = CRC_Test(receive_buf,7);
		RCRC_H = ((RCRC & 0xff00) >> 8);//高八位
		RCRC_L = (RCRC & 0x00ff); //低8位
		if((receive_buf[0] == 0xb1)&&(receive_buf[5] == RCRC_H)&&(receive_buf[6] == RCRC_L))
		{
			uart_sta_psa2 = 0;
			sta_psa2[0] = receive_buf[3];
			sta_psa2[1] = receive_buf[4];
		}
		else
		{
			uart_sta_psa2++;
		}
	}
	else
	{
		uart_sta_psa2++;
		//qDebug()<<"CRC wrong!!!!!!!";
	}
	if(uart_sta_psa2 >= 3)
	{
		uart_sta_psa2 = 3;
		sta_psa2[0] = 0xFF;
		sta_psa2[1] = 0xFF;
	}
}
/**************
 * 卸油流程
 * ***********/
void IIE_thread::ask_IIE()
{
	send_buf[0] = 0xb2;
	send_buf[1] = 0x04;
	send_buf[2] = 0x00;
	send_buf[3] = 0x02;
	send_buf[4] = IIE_SetModel_Time; //设置位1
	send_buf[5] = IIE_SetModel_Warn; //设置位2

	SCRC = CRC_Test(send_buf,8);
	send_buf[6] = ((SCRC & 0xff00) >> 8);
	send_buf[7] = (SCRC & 0x00ff);
	len_uart_IIE = write(fd_uart_IIE,send_buf,8);

	msleep(250);
	memset(receive_buf,0,sizeof(char)*16);//清空数据
	len_uart_IIE = read(fd_uart_IIE, receive_buf, sizeof(receive_buf));        //读取串口发来的数据
	if(len_uart_IIE > 0)
	{
		//数据crc校验
		RCRC = CRC_Test(receive_buf,12);
		RCRC_H = ((RCRC & 0xff00) >> 8);//高八位
		RCRC_L = (RCRC & 0x00ff); //低8位
		if((receive_buf[0] == 0xb2)&&(receive_buf[10] == RCRC_H)&&(receive_buf[11] == RCRC_L))
		{
			uart_sta_IIE = 0;
			sta_IIE[0] = 0; //通信状态,当该位为0xff时后面的都没用了
			sta_IIE[1] = receive_buf[3];//电压
			sta_IIE[2] = receive_buf[4];//电阻
			sta_IIE[3] = receive_buf[5];//稳油倒计时低位
			sta_IIE[4] = receive_buf[6];//稳油倒计时高位
			sta_IIE[5] = receive_buf[7];//人员值守倒计时
			sta_IIE[6] = receive_buf[8];//状态信息
			sta_IIE[7] = receive_buf[9];//屏蔽状态信息
		}
		else
		{
			uart_sta_IIE++;
		}
	}
	else
	{
		uart_sta_IIE++;
	}
	if(uart_sta_IIE >= 3)
	{
		uart_sta_IIE = 3;
		sta_IIE[0] = 0xff; //通信状态
	}
}
/**************
 * 电磁阀
 * ***********/
void IIE_thread::ask_valve()
{
	send_buf[0] = 0xb3;
	send_buf[1] = 0x04;
	send_buf[2] = 0x00;
	send_buf[3] = 0x02;
	send_buf[4] = 0x00; //设置位1
	send_buf[5] = 0x00; //设置位2

	SCRC = CRC_Test(send_buf,8);
	send_buf[6] = ((SCRC & 0xff00) >> 8);
	send_buf[7] = (SCRC & 0x00ff);
	len_uart_IIE = write(fd_uart_IIE,send_buf,8);

	msleep(200);
	memset(receive_buf,0,sizeof(char)*16);//清空数据
	len_uart_IIE = read(fd_uart_IIE, receive_buf, sizeof(receive_buf));        //读取串口发来的数据
	if(len_uart_IIE > 0)
	{
		//数据crc校验
		RCRC = CRC_Test(receive_buf,12);
		RCRC_H = ((RCRC & 0xff00) >> 8);               //高八位
		RCRC_L = (RCRC & 0x00ff);                      //低八位
		if((receive_buf[0] == 0xb3)&&(RCRC_H == receive_buf[10]) && (RCRC_L == receive_buf[11]))
		{
			uart_sta_valve = 0;
			//数据处理
			sta_valve[0] = receive_buf[3]; //阀1
			sta_valve[1] = receive_buf[4]; //阀2
			sta_valve[2] = receive_buf[5]; //阀3
			sta_valve[3] = receive_buf[6]; //阀4
			sta_valve[4] = receive_buf[7]; //阀5
		}
		else
		{
			uart_sta_valve++;
		}
	}
	else
	{
		uart_sta_valve++;
	}
	if(uart_sta_valve >= 3)
	{
		uart_sta_valve = 3;
		sta_valve[0] = 0xff; //阀1
		sta_valve[1] = 0xff; //阀2
		sta_valve[2] = 0xff; //阀3
		sta_valve[3] = 0xff; //阀4
		sta_valve[4] = 0xff; //阀5
	}
}

/***************
 * 将取得的数据赋值到传递数组
 * *************/
void IIE_thread::send_data()
{
	SendLock.lock();
	Ptr_Ask690[17] = sta_psa2[1];//人体静电状态

	Ptr_Ask690[36] = sta_IIE[0];//IIE通讯
	Ptr_Ask690[37] = sta_IIE[6];//IIE状态信息
	Ptr_Ask690[38] = sta_IIE[7];//IIE屏蔽信息
	Ptr_Ask690[39] = sta_IIE[1];//IIE电压
	Ptr_Ask690[40] = sta_IIE[2];//IIE电阻
	Ptr_Ask690[41] = sta_IIE[3];//IIE稳油计时低位
	Ptr_Ask690[42] = sta_IIE[4];//IIE稳油计时高位
	Ptr_Ask690[43] = sta_IIE[5];//IIE人员值守计时

	Ptr_Ask690[44] = sta_valve[0];
	Ptr_Ask690[45] = sta_valve[1];
	Ptr_Ask690[46] = sta_valve[2];
	Ptr_Ask690[47] = sta_valve[3];
	Ptr_Ask690[48] = sta_valve[4];
	SendLock.unlock();
	for(uchar i = 0;i < 49;i++) //增加IIE后由36改为44
	{
		//printf("%02x ",Ptr_Ask690[i]);  //打印接收数据
	}
}
/**************
 * 初始化串口
 * 非阻塞模式
 * ***********/
void IIE_thread::uart_init()
{
	//串口初始化
	fd_uart_IIE = open(SAFTY_IIE,O_RDWR|O_NOCTTY);
	ret_uart_IIE = set_port_attr(fd_uart_IIE,B9600,8,"1",'N',0,0);  //非阻塞模式
}
