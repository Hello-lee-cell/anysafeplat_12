#include<stdlib.h>
#include<string.h>
#include<termio.h>
#include<fcntl.h>
#include<stdio.h>   //perror
#include<unistd.h>  //write

#include"radar_485.h"
#include"file_op.h"
#include"mythread.h"
#include"config.h"
#include"mainwindow.h"
//***************edit by G
#include"serial.h"  //CRC_16->CRC_Test
#include"config.h"
#include"uartthread.h"
#include"database_op.h"

//unsigned char Flag_Set_SendMode = 0;
//unsigned char Flag_autoget_area = 0;//是否完成区域一数组标志位
//timer2.h
unsigned char Debugging_Flag;   //调试标志 timer2.h
unsigned char Cmd_Flag = 0;//通信标志 timer2.h
unsigned char Cmd_Date[20];//通信数据 timer2.h
unsigned int Cmd_Numb;	  //通信次数  timer2.h
unsigned char Rec_Debugging_Flag; //

//uart3.h
unsigned char Sensitivity_Date = 1;
unsigned char Send_Air_Flag[4];
unsigned char Send_Sensitivity_Flag;
unsigned char Alarm_Time_Flag[5];
//unsigned char Alarm_Re_Flag[4];
//unsigned char Amp_R_Date[30];   //从前端读回的幅值

//lcd_display.h
//unsigned char Get_Are_Flag[4];
unsigned char Write_Control[228];

//test.c
float Back_C[90];
unsigned char Rec_Flag[4];  //通信收到答复的标志

//unsigned char Get_Back_Flag[4]; //取得阈值标志位
//float Goal[4][20][2];   //存储目标位置
unsigned char OBJ_numb;
unsigned char Com_Update_Flag[4];   //数据更新标志位
unsigned int Page_Numb;
unsigned char Threshold_Setting_Step;
//unsigned char Communication_Machine = 0;    //通信时的从机
//***********<-
unsigned char Ask_Are_Flag[4] = {0,0,0,0};

static unsigned char sendbuff[255];   //edit by G
unsigned char Per_Address = 0x01;//u8 Per_Address = 0x01;
unsigned char Flag_rec_bad = 0;
unsigned char alarm_flag[254][8];
unsigned char Debug_Enable[4];
unsigned char alarm_re_flag_pre[4][2] = {0};//雷达报警延时，连续几次报警之后才报警

//unsigned char Flag_moni_warn = 0;

void MYDMA1CH2_Enable(unsigned char *a,unsigned char b)
{
    gpio_11_low();
    write(fd_uart_radar,a,b);
}

 //雷达串口初始化
/********************************************
 函数功能： 485通讯初始化

函 数 名： ModbusInit
参    数： 
           
		   
返 回 值： FALSE  失败，未使用
           TRUE   成功 ，使用

*********************************************/
void ModbusInit(void)
{
    //added by G
    fd_uart_radar = open(RADAR_SERI,O_RDWR|O_NOCTTY);
    ret_uart_radar = set_port_attr(fd_uart_radar,B115200,8,"1",'N',0,0);       //vmin 0.1s
}

/********************************************
 函数功能： 发送查询命令

函 数 名： Send_Command
参    数： Slave_Address
           
Send_MODE		   
0	发送命令让前端取背景
1	询问是否取完背景
2	读取背景信息20
3	读取背景信息 21~40
4	读取背景信息41~60
5	读取背景信息61~80
6	读取背景信息81~100
7	读取背景信息101~120
8	读取背景信息121~140
	
11	读取目标数量
12	读取各个目标的位置
13	写背景20
14	写背景21~40
15	写背景41~60
16	写背景61~80
17	写背景81~100
18	写背景101~120
19	写背景121~140

*********************************************/

void Send_Command(unsigned char Slave_Address)
{
	 unsigned short crc;
     unsigned char i=0;
	 int m;
	 int temp_x;
	 int temp_y;
     unsigned char temp_able;
     unsigned char temp_must = 0;
//	 unsigned char temp[20];
//	 unsigned char temp_flag;
//	 unsigned char Zero_Flag;
//	 unsigned int Zero_Numb;

    //* RS485_CON1 = 0;
     gpio_11_low();

     temp_able = (!Flag_auto_silent)&&(Flag_outdoor_warn);

     if((Flag_warn_delay &&(!Flag_auto_silent) && (Flag_outdoor_warn) )|| Flag_moni_warn)
     {
        temp_must = 1;
     }
     else
     {
         temp_must = 0;
     }
     sendbuff[0] = (temp_must<<7)|(temp_able<<6)|(Communication_Machine+1);//只有一个从机 //0x01;
	 //****************************
	 //功能码：
	 //	 01/02	 读取状态
	 //	 03/04   读取寄存器
	 //	 05  写状态
	 //	 06  写单寄存器
	 // 15 写多寄存器
	 //*******************************
	 if(Debugging_Flag==0)
	 {
		 switch(Slave_Address)	//发送的命令
		 {
             case 0: //取背景信号 `//阈值采集
			 {
			 	sendbuff[1] = 5;
				sendbuff[2] = 2;
				sendbuff[3] = 1;
                crc = CRC_Radar(sendbuff,4);
				sendbuff[4] =crc&0xff;
				sendbuff[5] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,6);
			 	break;
			 }
			 case 1://读状态信息
             {
			 	sendbuff[1] = 1;
				sendbuff[2] = 129;
				sendbuff[3] = 1;
                crc = CRC_Radar(sendbuff,4);
				sendbuff[4] =crc&0xff;
				sendbuff[5] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,6);
			 	break;
			 }
			case 2://读背景信息0~6
			{
				sendbuff[1] = 3;
				sendbuff[2] = 21;
				sendbuff[3] = 7;
                crc = CRC_Radar(sendbuff,4);
				sendbuff[4] =crc&0xff;
                sendbuff[5] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,6);
				break;
			}
			case 101://读背景信息7~13
			{
				sendbuff[1] = 3;
				sendbuff[2] = 28;
				sendbuff[3] = 7;
                crc = CRC_Radar(sendbuff,4);
				sendbuff[4] =crc&0xff;
                sendbuff[5] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,6);
				break;
			}
            case 102://读背景信息14~20
			{
				sendbuff[1] = 3;
				sendbuff[2] = 35;
				sendbuff[3] = 7;
                crc = CRC_Radar(sendbuff,4);
				sendbuff[4] =crc&0xff;
                sendbuff[5] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,6);
				break;
			}
			case 103://读背景信息21~27
			{
				sendbuff[1] = 3;
				sendbuff[2] = 42;
				sendbuff[3] = 7;
                crc = CRC_Radar(sendbuff,4);
				sendbuff[4] =crc&0xff;
                sendbuff[5] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,6);
				break;
			}
			case 104://读背景信息28~34
			{
				sendbuff[1] = 3;
				sendbuff[2] = 49;
				sendbuff[3] = 7;
                crc = CRC_Radar(sendbuff,4);
				sendbuff[4] =crc&0xff;
                sendbuff[5] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,6);
				break;
			}
			case 105://读背景信息35~41
			{
				sendbuff[1] = 3;
				sendbuff[2] = 56;
				sendbuff[3] = 7;
                crc = CRC_Radar(sendbuff,4);
				sendbuff[4] =crc&0xff;
                sendbuff[5] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,6);
				break;
			}
			case 106://读背景信息42~48
			{
				sendbuff[1] = 3;
				sendbuff[2] = 63;
				sendbuff[3] = 7;
                crc = CRC_Radar(sendbuff,4);
				sendbuff[4] =crc&0xff;
                sendbuff[5] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,6);
				break;
			}
			case 107://读背景信息49~55
			{
				sendbuff[1] = 3;
				sendbuff[2] = 70;
				sendbuff[3] = 7;
                crc = CRC_Radar(sendbuff,4);
				sendbuff[4] =crc&0xff;
                sendbuff[5] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,6);
				break;
			}
			case 108://读背景信息56~62
			{
				sendbuff[1] = 3;
				sendbuff[2] = 77;
				sendbuff[3] = 7;
                crc = CRC_Radar(sendbuff,4);
				sendbuff[4] =crc&0xff;
                sendbuff[5] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,6);
				break;
			}
			case 109://读背景信息63~69
			{
				sendbuff[1] = 3;
				sendbuff[2] = 84;
				sendbuff[3] = 7;
                crc = CRC_Radar(sendbuff,4);
				sendbuff[4] =crc&0xff;
                sendbuff[5] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,6);
				break;
			}
			case 110://读背景信息70~76
			{
				sendbuff[1] = 3;
				sendbuff[2] = 91;
				sendbuff[3] = 7;
                crc = CRC_Radar(sendbuff,4);
				sendbuff[4] =crc&0xff;
                sendbuff[5] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,6);
				break ;
			}
			case 111://读背景信息77~83
			{
				sendbuff[1] = 3;
				sendbuff[2] = 98;
				sendbuff[3] = 7;
                crc = CRC_Radar(sendbuff,4);
				sendbuff[4] =crc&0xff;
                sendbuff[5] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,6);
				break;
			}
            case 112://读背景信息84~90
			{
				sendbuff[1] = 3;
				sendbuff[2] = 105;
				sendbuff[3] = 7;
                crc = CRC_Radar(sendbuff,4);
				sendbuff[4] =crc&0xff;
                sendbuff[5] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,6);
				break;
			}
			case 113://读背景信息91~97
			{
				sendbuff[1] = 3;
				sendbuff[2] = 112;
				sendbuff[3] = 7;
                crc = CRC_Radar(sendbuff,4);
				sendbuff[4] =crc&0xff;
                sendbuff[5] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,6);
				break;
			}
			case 114://读背景信息98~104
			{
				sendbuff[1] = 3;
				sendbuff[2] = 119;
				sendbuff[3] = 7;
                crc = CRC_Radar(sendbuff,4);
				sendbuff[4] =crc&0xff;
                sendbuff[5] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,6);
				break;
			}
			case 115://读背景信息105~111
			{
				sendbuff[1] = 3;
				sendbuff[2] = 126;
				sendbuff[3] = 7;
                crc = CRC_Radar(sendbuff,4);
				sendbuff[4] =crc&0xff;
                sendbuff[5] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,6);
				break;
			}
			case 116://读背景信息112~118
			{
				sendbuff[1] = 3;
				sendbuff[2] = 133;
				sendbuff[3] = 7;
                crc = CRC_Radar(sendbuff,4);
				sendbuff[4] =crc&0xff;
                sendbuff[5] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,6);
				break;
			}
			case 117://读背景信息119~125
			{
				sendbuff[1] = 3;
				sendbuff[2] = 140;
				sendbuff[3] = 7;
                crc = CRC_Radar(sendbuff,4);
				sendbuff[4] =crc&0xff;
                sendbuff[5] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,6);
				break;
			}
			case 118://读背景信息126~132
			{
				sendbuff[1] = 3;
				sendbuff[2] = 147;
				sendbuff[3] = 7;
                crc = CRC_Radar(sendbuff,4);
				sendbuff[4] =crc&0xff;
                sendbuff[5] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,6);
				break;
			}
			case 119://读背景信息133~139
			{
				sendbuff[1] = 3;
				sendbuff[2] = 154;
				sendbuff[3] = 7;
                crc = CRC_Radar(sendbuff,4);
				sendbuff[4] =crc&0xff;
                sendbuff[5] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,6);
				break;
			}
			case 120://读背景信息140~146
			{
				sendbuff[1] = 3;
				sendbuff[2] = 161;
				sendbuff[3] = 7;
                crc = CRC_Radar(sendbuff,4);
				sendbuff[4] =crc&0xff;
                sendbuff[5] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,6);
				break;
			}
            case 121://读背景信息147~153
			{
			    sendbuff[1] = 3;
				sendbuff[2] = 168;
				sendbuff[3] = 7;
                crc = CRC_Radar(sendbuff,4);
				sendbuff[4] =crc&0xff;
                sendbuff[5] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,6);
				break;
			}
			case 122://读背景信息154~160
			{
				sendbuff[1] = 3;
				sendbuff[2] = 175;
				sendbuff[3] = 7;
                crc = CRC_Radar(sendbuff,4);
				sendbuff[4] =crc&0xff;
                sendbuff[5] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,6);
				break;
			}
			case 123://读背景信息161~167
			{
				sendbuff[1] = 3;
				sendbuff[2] = 182;
				sendbuff[3] = 7;
                crc = CRC_Radar(sendbuff,4);
				sendbuff[4] =crc&0xff;
                sendbuff[5] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,6);
				break;
			}
			case 124://读背景信息168~174
			{
			    sendbuff[1] = 3;
				sendbuff[2] = 189;
				sendbuff[3] = 7;
                crc = CRC_Radar(sendbuff,4);
				sendbuff[4] =crc&0xff;
                sendbuff[5] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,6);
				break;
			}
			case 125://读背景信息175~179
			{
				sendbuff[1] = 3;
				sendbuff[2] = 196;
				sendbuff[3] = 5;
                crc = CRC_Radar(sendbuff,4);
				sendbuff[4] =crc&0xff;
                sendbuff[5] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,6);
				break;
			}	
             case 11:	//读取目标信息            一直发
			 {
			 	sendbuff[1] = 3;
				sendbuff[2] = 0;
				sendbuff[3] = 1;
                crc = CRC_Radar(sendbuff,4);
				sendbuff[4] =crc&0xff;
                sendbuff[5] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,6);
			 	break;
             }
             case 13: //写背景信息 0-7       阈值更新
			 {
			 	sendbuff[1] = 16;
				sendbuff[2] = 21;
				sendbuff[3] = 7;
				m = 0;
			 	for(i=0;i<7;i++)
				{
                    sendbuff[4+i*2] = Master_Back_Groud_Value[Communication_Machine][m+i][1];
                    sendbuff[4+i*2+1] = Master_Back_Groud_Value[Communication_Machine][m+i][0];
				}
                crc = CRC_Radar(sendbuff,18);
				sendbuff[18] =crc&0xff;
                sendbuff[19] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,20);
			 	break;
			 }
	
			case 126://写背景信息7~13	28	7
			{
				sendbuff[1] = 16;
				sendbuff[2] = 28;
				sendbuff[3] = 7;
				m = 7;
			 	for(i=0;i<7;i++)
				{
					sendbuff[4+i*2] = Master_Back_Groud_Value[Communication_Machine][m+i][1];	
					sendbuff[4+i*2+1] = Master_Back_Groud_Value[Communication_Machine][m+i][0];	
				}
                crc = CRC_Radar(sendbuff,18);
				sendbuff[18] =crc&0xff;
                sendbuff[19] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,20);
				break;
			}
			case 127://写背景信息14~20	35	14
			{
				sendbuff[1] = 16;
				sendbuff[2] = 35;
				sendbuff[3] = 7;
				m = 14;
			 	for(i=0;i<7;i++)
				{
					sendbuff[4+i*2] = Master_Back_Groud_Value[Communication_Machine][m+i][1];	
					sendbuff[4+i*2+1] = Master_Back_Groud_Value[Communication_Machine][m+i][0];	
				}
                crc = CRC_Radar(sendbuff,18);
				sendbuff[18] =crc&0xff;
                sendbuff[19] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,20);
				break;
			}
			case 128://写背景信息21~27	42	21
			{
				sendbuff[1] = 16;
				sendbuff[2] = 42;
				sendbuff[3] = 7;
				m = 21;
			 	for(i=0;i<7;i++)
				{
					sendbuff[4+i*2] = Master_Back_Groud_Value[Communication_Machine][m+i][1];	
					sendbuff[4+i*2+1] = Master_Back_Groud_Value[Communication_Machine][m+i][0];	
				}
                crc = CRC_Radar(sendbuff,18);
				sendbuff[18] =crc&0xff;
                sendbuff[19] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,20);
				break;
			}
			case 129://写背景信息28~34	49	28
			{
				sendbuff[1] = 16;
				sendbuff[2] = 49;
				sendbuff[3] = 7;
				m = 28;
			 	for(i=0;i<7;i++)
				{
					sendbuff[4+i*2] = Master_Back_Groud_Value[Communication_Machine][m+i][1];	
					sendbuff[4+i*2+1] = Master_Back_Groud_Value[Communication_Machine][m+i][0];	
				}
                crc = CRC_Radar(sendbuff,18);
				sendbuff[18] =crc&0xff;
                sendbuff[19] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,20);
				break;
			}
			case 130://写背景信息35~41	56	35
			{
				sendbuff[1] = 16;
				sendbuff[2] = 56;
				sendbuff[3] = 7;
				m = 35;
			 	for(i=0;i<7;i++)
				{
					sendbuff[4+i*2] = Master_Back_Groud_Value[Communication_Machine][m+i][1];	
					sendbuff[4+i*2+1] = Master_Back_Groud_Value[Communication_Machine][m+i][0];	
				}
                crc = CRC_Radar(sendbuff,18);
				sendbuff[18] =crc&0xff;
                sendbuff[19] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,20);
				break;
			}
			case 131://写背景信息42~48	63	42
			{
				sendbuff[1] = 16;
				sendbuff[2] = 63;
				sendbuff[3] = 7;
				m = 42;
			 	for(i=0;i<7;i++)
				{
					sendbuff[4+i*2] = Master_Back_Groud_Value[Communication_Machine][m+i][1];	
					sendbuff[4+i*2+1] = Master_Back_Groud_Value[Communication_Machine][m+i][0];	
				}
                crc = CRC_Radar(sendbuff,18);
				sendbuff[18] =crc&0xff;
                sendbuff[19] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,20);
				break;
			}
			case 132://写背景信息49~55	70	49
			{
				sendbuff[1] = 16;
				sendbuff[2] = 70;
				sendbuff[3] = 7;
				m = 49;
			 	for(i=0;i<7;i++)
				{
					sendbuff[4+i*2] = Master_Back_Groud_Value[Communication_Machine][m+i][1];	
					sendbuff[4+i*2+1] = Master_Back_Groud_Value[Communication_Machine][m+i][0];	
				}
                crc = CRC_Radar(sendbuff,18);
				sendbuff[18] =crc&0xff;
                sendbuff[19] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,20);
				break;
			}
			case 133://写背景信息56~62	77	56
			{
				sendbuff[1] = 16;
				sendbuff[2] = 77;
				sendbuff[3] = 7;
				m = 56;
			 	for(i=0;i<7;i++)
				{
					sendbuff[4+i*2] = Master_Back_Groud_Value[Communication_Machine][m+i][1];	
					sendbuff[4+i*2+1] = Master_Back_Groud_Value[Communication_Machine][m+i][0];	
				}
                crc = CRC_Radar(sendbuff,18);
				sendbuff[18] =crc&0xff;
                sendbuff[19] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,20);
				break;
			}
			case 134://写背景信息63~69	84	63
			{
				sendbuff[1] = 16;
				sendbuff[2] = 84;
				sendbuff[3] = 7;
				m = 63;
			 	for(i=0;i<7;i++)
				{
					sendbuff[4+i*2] = Master_Back_Groud_Value[Communication_Machine][m+i][1];	
					sendbuff[4+i*2+1] = Master_Back_Groud_Value[Communication_Machine][m+i][0];	
				}
                crc = CRC_Radar(sendbuff,18);
				sendbuff[18] =crc&0xff;
                sendbuff[19] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,20);
				break;
			}
            case 135://写背景信息70~76	91	70
			{
				sendbuff[1] = 16;
				sendbuff[2] = 91;
				sendbuff[3] = 7;
				m = 70;
			 	for(i=0;i<7;i++)
				{
					sendbuff[4+i*2] = Master_Back_Groud_Value[Communication_Machine][m+i][1];	
					sendbuff[4+i*2+1] = Master_Back_Groud_Value[Communication_Machine][m+i][0];	
				}
                crc = CRC_Radar(sendbuff,18);
				sendbuff[18] =crc&0xff;
                sendbuff[19] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,20);
				break;
			}
			case 136://写背景信息77~83	98	77
			{
				sendbuff[1] = 16;
				sendbuff[2] = 98;
				sendbuff[3] = 7;
				m = 77;
			 	for(i=0;i<7;i++)
				{
					sendbuff[4+i*2] = Master_Back_Groud_Value[Communication_Machine][m+i][1];	
					sendbuff[4+i*2+1] = Master_Back_Groud_Value[Communication_Machine][m+i][0];	
				}
                crc = CRC_Radar(sendbuff,18);
				sendbuff[18] =crc&0xff;
                sendbuff[19] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,20);
				break;
			}
			case 137://写背景信息84~90	105	84
			{
				sendbuff[1] = 16;
				sendbuff[2] = 105;
				sendbuff[3] = 7;
				m = 84;
			 	for(i=0;i<7;i++)
				{
					sendbuff[4+i*2] = Master_Back_Groud_Value[Communication_Machine][m+i][1];	
					sendbuff[4+i*2+1] = Master_Back_Groud_Value[Communication_Machine][m+i][0];	
				}
                crc = CRC_Radar(sendbuff,18);
				sendbuff[18] =crc&0xff;
                sendbuff[19] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,20);
				break;
			}
			case 138://写背景信息91~97	112	91
			{  
				sendbuff[1] = 16;
				sendbuff[2] = 112;
				sendbuff[3] = 7;
				m = 91;
			 	for(i=0;i<7;i++)
				{
					sendbuff[4+i*2] = Master_Back_Groud_Value[Communication_Machine][m+i][1];	
					sendbuff[4+i*2+1] = Master_Back_Groud_Value[Communication_Machine][m+i][0];	
				}
                crc = CRC_Radar(sendbuff,18);
				sendbuff[18] =crc&0xff;
                sendbuff[19] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,20);
				break;
			}
			case 139://写背景信息98~104	119	98
			{
				sendbuff[1] = 16;
				sendbuff[2] = 119;
				sendbuff[3] = 7;
				m = 98;
			 	for(i=0;i<7;i++)
				{
					sendbuff[4+i*2] = Master_Back_Groud_Value[Communication_Machine][m+i][1];	
					sendbuff[4+i*2+1] = Master_Back_Groud_Value[Communication_Machine][m+i][0];	
				}
                crc = CRC_Radar(sendbuff,18);
				sendbuff[18] =crc&0xff;
                sendbuff[19] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,20);
				break;
			}
			case 140://写背景信息105~111	126	105
			{
				sendbuff[1] = 16;
				sendbuff[2] = 126;
				sendbuff[3] = 7;
				m = 105;
			 	for(i=0;i<7;i++)
				{
					sendbuff[4+i*2] = Master_Back_Groud_Value[Communication_Machine][m+i][1];	
					sendbuff[4+i*2+1] = Master_Back_Groud_Value[Communication_Machine][m+i][0];	
				}
                crc = CRC_Radar(sendbuff,18);
				sendbuff[18] =crc&0xff;
                sendbuff[19] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,20);
				break;
			}
			case 141://写背景信息112~118	133	112
			{
				sendbuff[1] = 16;
				sendbuff[2] = 133;
				sendbuff[3] = 7;
				m = 112;
			 	for(i=0;i<7;i++)
				{
					sendbuff[4+i*2] = Master_Back_Groud_Value[Communication_Machine][m+i][1];	
					sendbuff[4+i*2+1] = Master_Back_Groud_Value[Communication_Machine][m+i][0];	
				}
                crc = CRC_Radar(sendbuff,18);
				sendbuff[18] =crc&0xff;
                sendbuff[19] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,20);
				break;
			}
			case 142://写背景信息119~125	140	119
			{
				sendbuff[1] = 16;
				sendbuff[2] = 140;
				sendbuff[3] = 7;
				m = 119;
			 	for(i=0;i<7;i++)
				{
					sendbuff[4+i*2] = Master_Back_Groud_Value[Communication_Machine][m+i][1];	
					sendbuff[4+i*2+1] = Master_Back_Groud_Value[Communication_Machine][m+i][0];	
				}
                crc = CRC_Radar(sendbuff,18);
				sendbuff[18] =crc&0xff;
                sendbuff[19] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,20);
				break;
			}
			case 143://写背景信息126~132	147	126
			{
				sendbuff[1] = 16;
				sendbuff[2] = 147;
				sendbuff[3] = 7;
				m = 126;
			 	for(i=0;i<7;i++)
				{
					sendbuff[4+i*2] = Master_Back_Groud_Value[Communication_Machine][m+i][1];	
					sendbuff[4+i*2+1] = Master_Back_Groud_Value[Communication_Machine][m+i][0];	
				}
                crc = CRC_Radar(sendbuff,18);
				sendbuff[18] =crc&0xff;
                sendbuff[19] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,20);
				break;
			}
			case 144://写背景信息133~139	154	133
			{
				sendbuff[1] = 16;
				sendbuff[2] = 154;
				sendbuff[3] = 7;
				m = 133;
			 	for(i=0;i<7;i++)
				{
					sendbuff[4+i*2] = Master_Back_Groud_Value[Communication_Machine][m+i][1];	
					sendbuff[4+i*2+1] = Master_Back_Groud_Value[Communication_Machine][m+i][0];	
				}
                crc = CRC_Radar(sendbuff,18);
				sendbuff[18] =crc&0xff;
                sendbuff[19] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,20);
				break;
			}
			case 145://写背景信息140~146	161	140
			{
				sendbuff[1] = 16;
				sendbuff[2] = 161;
				sendbuff[3] = 7;
				m = 140;
			 	for(i=0;i<7;i++)
				{
					sendbuff[4+i*2] = Master_Back_Groud_Value[Communication_Machine][m+i][1];	
					sendbuff[4+i*2+1] = Master_Back_Groud_Value[Communication_Machine][m+i][0];	
				}
                crc = CRC_Radar(sendbuff,18);
				sendbuff[18] =crc&0xff;
                sendbuff[19] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,20);
				break;
			}
			case 146://写背景信息147~153	168	147
			{
				sendbuff[1] = 16;
				sendbuff[2] = 168;
				sendbuff[3] = 7;
				m = 147;
			 	for(i=0;i<7;i++)
				{
					sendbuff[4+i*2] = Master_Back_Groud_Value[Communication_Machine][m+i][1];	
					sendbuff[4+i*2+1] = Master_Back_Groud_Value[Communication_Machine][m+i][0];	
				}
                crc = CRC_Radar(sendbuff,18);
				sendbuff[18] =crc&0xff;
                sendbuff[19] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,20);
				break;
			}
			case 147://写背景信息154~160	175	154
			{
				sendbuff[1] = 16;
				sendbuff[2] = 175;
				sendbuff[3] = 7;
				m = 154;
			 	for(i=0;i<7;i++)
				{
					sendbuff[4+i*2] = Master_Back_Groud_Value[Communication_Machine][m+i][1];	
					sendbuff[4+i*2+1] = Master_Back_Groud_Value[Communication_Machine][m+i][0];	
				}
                crc = CRC_Radar(sendbuff,18);
				sendbuff[18] =crc&0xff;
                sendbuff[19] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,20);
				break;
			}
			case 148://写背景信息161~167	182	161
			{
				sendbuff[1] = 16;
				sendbuff[2] = 182;
				sendbuff[3] = 7;
				m = 161;
			 	for(i=0;i<7;i++)
				{
					sendbuff[4+i*2] = Master_Back_Groud_Value[Communication_Machine][m+i][1];	
					sendbuff[4+i*2+1] = Master_Back_Groud_Value[Communication_Machine][m+i][0];	
				}
                crc = CRC_Radar(sendbuff,18);
				sendbuff[18] =crc&0xff;
                sendbuff[19] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,20);
				break;
			}
			case 149://写背景信息168~174	189	168
			{
				sendbuff[1] = 16;
				sendbuff[2] = 189;
				sendbuff[3] = 7;
				m = 168;
			 	for(i=0;i<7;i++)
				{
					sendbuff[4+i*2] = Master_Back_Groud_Value[Communication_Machine][m+i][1];	
					sendbuff[4+i*2+1] = Master_Back_Groud_Value[Communication_Machine][m+i][0];	
				}
                crc = CRC_Radar(sendbuff,18);
				sendbuff[18] =crc&0xff;
                sendbuff[19] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,20);
				break;
			}
			case 150://写背景信息175~179	196	175
			{
				sendbuff[1] = 16;
				sendbuff[2] = 196;
				sendbuff[3] = 5;
				m = 175;
			 	for(i=0;i<5;i++)
				{
					sendbuff[4+i*2] = Master_Back_Groud_Value[Communication_Machine][m+i][1];	
					sendbuff[4+i*2+1] = Master_Back_Groud_Value[Communication_Machine][m+i][0];	
				}
                crc = CRC_Radar(sendbuff,14);
				sendbuff[14] =crc&0xff;
                sendbuff[15] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,16);
				break;
			}
	
	
             case 31:	  //询问上升沿AMP_R      //双路或者1路自动设置
			 {
			 	sendbuff[1] = 3;
				sendbuff[2] = 201;
				sendbuff[3] = 10;
                crc = CRC_Radar(sendbuff,4);
				sendbuff[4] =crc&0xff;
                sendbuff[5] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,6);
			 	break;
			 }
	
            case 32:  //询问下降沿AMP_R     //2路自动设置
			{
				sendbuff[1] = 3;
				sendbuff[2] = 211;
				sendbuff[3] = 10;
                crc = CRC_Radar(sendbuff,4);
				sendbuff[4] =crc&0xff;
                sendbuff[5] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,6);
				break;
			}
	
             case 33:		//写入标志位，要求自动辨别区域
			 {
			 	sendbuff[1] = 5;
				sendbuff[2] = 4;
				sendbuff[3] = 1;
                crc = CRC_Radar(sendbuff,4);
				sendbuff[4] =crc&0xff;
                sendbuff[5] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,6);
			 	break;		  
			 }
			 case 34://读取是否已经取完区域
			 {
			 	sendbuff[1] = 1;
				sendbuff[2] = 131;
				sendbuff[3] = 1;
                crc = CRC_Radar(sendbuff,4);
				sendbuff[4] =crc&0xff;
                sendbuff[5] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,6);
			 	break;
			 }
	
			  case 35:	 //读取区域一四个区域
			 {
			 	sendbuff[1] = 3;
			//	sendbuff[2] = 281;
				sendbuff[2] = 241;
				sendbuff[3] = 1;
                crc = CRC_Radar(sendbuff,4);
				sendbuff[4] =crc&0xff;
                sendbuff[5] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,6);
			 	break;
			 }
			 case 36:	//写入标志位，要求取消自动辨别区域
			 {
			 	sendbuff[1] = 5;
				sendbuff[2] = 4;
				sendbuff[3] = 0;
                crc = CRC_Radar(sendbuff,4);
				sendbuff[4] =crc&0xff;
                sendbuff[5] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,6);
			 	break;		  
			 }
			 case 37:  //写入区域一
			 {
			 	sendbuff[1] = 16;
				sendbuff[2] = 241;
				sendbuff[3] = 1;
			    
			 	for(i=0;i<6;i++)
				{
	 			    temp_x = -Master_Boundary_Point_Disp[Communication_Machine][0][i][0];
	   			    temp_y =  Master_Boundary_Point_Disp[Communication_Machine][0][i][1];
					temp_x = 250 - temp_x;
	
				    sendbuff[4+i*3] = (temp_x>>4)&0xFF;
				    sendbuff[4+i*3+1] = ((temp_x<<4)&0xF0)|((temp_y>>8)&0x0f);
					sendbuff[4+i*3+2] = temp_y&0xFF;
				}
                crc = CRC_Radar(sendbuff,22);
				sendbuff[22] =crc&0xff;
                sendbuff[23] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,24);
			 	break;
			 }
			 case 38://写入区域二
			 {
			 	sendbuff[1] = 16;
				sendbuff[2] = 242;
				sendbuff[3] = 1;
			    
			 	for(i=0;i<6;i++)
				{

					temp_x = -Master_Boundary_Point_Disp[Communication_Machine][1][i][0];
					temp_y =  Master_Boundary_Point_Disp[Communication_Machine][1][i][1];
					temp_x = 250 - temp_x;
	
				    sendbuff[4+i*3] = (temp_x>>4)&0xFF;
				    sendbuff[4+i*3+1] = ((temp_x<<4)&0xF0)|((temp_y>>8)&0x0f);
					sendbuff[4+i*3+2] = temp_y&0xFF;
				}
                crc = CRC_Radar(sendbuff,22);
				sendbuff[22] =crc&0xff;
                sendbuff[23] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,24);
			 	break;
			 }
			 case 39://写入区域三
			 {
			 	sendbuff[1] = 16;
				sendbuff[2] = 243;
				sendbuff[3] = 1;
			    
			 	for(i=0;i<6;i++)
				{

					temp_x = -Master_Boundary_Point_Disp[Communication_Machine][2][i][0];
					temp_y =  Master_Boundary_Point_Disp[Communication_Machine][2][i][1];
					temp_x = 250 - temp_x;
	
				    sendbuff[4+i*3] = (temp_x>>4)&0xFF;
				    sendbuff[4+i*3+1] = ((temp_x<<4)&0xF0)|((temp_y>>8)&0x0f);
					sendbuff[4+i*3+2] = temp_y&0xFF;
				}
                crc = CRC_Radar(sendbuff,22);
				sendbuff[22] =crc&0xff;
                sendbuff[23] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,24);
			 	break;
			 }
			 case 40://写入区域四
			 {
			 	sendbuff[1] = 16;
				sendbuff[2] = 244;
				sendbuff[3] = 1;
			    
			 	for(i=0;i<6;i++)
				{

	
					temp_x = -Master_Boundary_Point_Disp[Communication_Machine][3][i][0];
					temp_y =  Master_Boundary_Point_Disp[Communication_Machine][3][i][1];
					temp_x = 250 - temp_x;
	
				    sendbuff[4+i*3] = (temp_x>>4)&0xFF;
				    sendbuff[4+i*3+1] = ((temp_x<<4)&0xF0)|((temp_y>>8)&0x0f);
					sendbuff[4+i*3+2] = temp_y&0xFF;
				}
                crc = CRC_Radar(sendbuff,22);
				sendbuff[22] =crc&0xff;
                sendbuff[23] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,24);
			 	break;
			 }
			 case 41://写入区域五
			 {
			 	sendbuff[1] = 16;
				sendbuff[2] = 245;
				sendbuff[3] = 1;
			    
			 	for(i=0;i<6;i++)
				{

					temp_x = -Master_Boundary_Point_Disp[Communication_Machine][4][i][0];
					temp_y =  Master_Boundary_Point_Disp[Communication_Machine][4][i][1];
					temp_x = 250 - temp_x;
	
				    sendbuff[4+i*3] = (temp_x>>4)&0xFF;
				    sendbuff[4+i*3+1] = ((temp_x<<4)&0xF0)|((temp_y>>8)&0x0f);
					sendbuff[4+i*3+2] = temp_y&0xFF;
				}
                crc = CRC_Radar(sendbuff,22);
				sendbuff[22] =crc&0xff;
                sendbuff[23] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,24);
			 	break;
			 }		  
			 case 42://写入区域六
			 {
			 	sendbuff[1] = 16;
				sendbuff[2] = 246;
				sendbuff[3] = 1;
			    
			 	for(i=0;i<6;i++)
				{

	
					temp_x = -Master_Boundary_Point_Disp[Communication_Machine][5][i][0];
					temp_y =  Master_Boundary_Point_Disp[Communication_Machine][5][i][1];
					temp_x = 250 - temp_x;
	
				    sendbuff[4+i*3] = (temp_x>>4)&0xFF;
				    sendbuff[4+i*3+1] = ((temp_x<<4)&0xF0)|((temp_y>>8)&0x0f);
					sendbuff[4+i*3+2] = temp_y&0xFF;
				}
                crc = CRC_Radar(sendbuff,22);
				sendbuff[22] =crc&0xff;
                sendbuff[23] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,24);
			 	break;
			 }
	
			 case 43://写入灵敏度
			 {
			 	sendbuff[1] = 16;
				sendbuff[2] = 247;
				sendbuff[3] = 1;
	
				sendbuff[4] = Sensitivity_Date;
	
                crc = CRC_Radar(sendbuff,5);
	
				sendbuff[5] =crc&0xff;
                sendbuff[6] =(crc>>8)&0xff;
                MYDMA1CH2_Enable(sendbuff,7);
			 	break;
			 }
			
		 }	 //end"switch( switch(Slave_Address))"
	 }	//end" if(Debugging_Flag==0)"
	 else//如果是调试模式
	 {

	 }

     usleep(500);
     usleep(4000);

  //*   RS485_CON1 = 1;
     gpio_11_high();
}

/******************************************************
函数功能： 下位机检验CRC

函 数 名： SlaveCheckCRC
参    数： char *revframe   接收帧
           int &funcode     功能码[out]
          		   
返 回 值： 
           TRUE   成功
		   FALSE  失败,地址或CRC 错误，返回参数无效
******************************************************/

bool SlaveCheckCRC(unsigned char *revframe,int framelen)
{
	  unsigned char Hi,Lo;
	  unsigned short crc;

	  if(framelen<=3)
	  {
	  	return FALSE;
	  }
      crc = CRC_Radar(revframe,framelen-2);  //计算的CRC 高低
      Lo = crc&0x00ff;
	  Hi = (crc>>8)&0x00ff;
	  if((Hi != revframe[framelen-1])||(Lo != revframe[framelen-2]))//实际传送的CRC 低高
	  {    
		 
		// uart3_send("crcbad!\r\n",strlen("crcbad!\r\n"));//调试用
		 return FALSE;
      }	 
	  return TRUE;  
}

bool SlaveCheckCRC_LCD(unsigned char *revframe,int framelen)
{
	  unsigned char Hi,Lo;
	  unsigned short crc;

	  if(framelen<=3)
	  {
	  	return FALSE;
	  }
      crc = CRC_Radar(revframe,framelen-2);   //计算的CRC 高低
      Lo = crc&0x00ff;
	  Hi = (crc>>8)&0x00ff;
	  if((Hi != revframe[framelen-1])||(Lo != revframe[framelen-2]))//实际传送的CRC 低高
	  {    
		 
		// uart3_send("crcbad!\r\n",strlen("crcbad!\r\n"));//调试用
		 return FALSE;
      }	 
	  return TRUE;  
}

/**********************************************************************	
函数功能： 485通讯接收完成后的数据处理

函 数 名： CommunicationProcess
参    数： char *rcvbuff   接收数据
           int rcvcount     数据长度
          		   
返 回 值： 
           无


Send_MODE		   
0	发送命令让前端取背景
1	询问是否取完背景
2	读取背景信息20
3	读取背景信息 21~40
4	读取背景信息41~60
5	读取背景信息61~80
6	读取背景信息81~100
7	读取背景信息101~120
8	读取背景信息121~140
	
11	读取目标数量
12	读取各个目标的位置
13	写背景20
14	写背景21~40
15	写背景41~60
16	写背景61~80
17	写背景81~100
18	写背景101~120
19	写背景121~140
*************************************************************************/

void CommunicationProcess(unsigned char* rcvbuff,unsigned char rcvcount)
{
//	u8  functioncode;
//	unsigned short crc;
	int  i,j;
	int m,n;
	int temp_1;
	int temp_2;
	float temp_3;
	float temp_4;
	int temp;
	int temp_x;
	int temp_y;
//	unsigned char temp_s[20];
//	unsigned char temp_flag;
//	unsigned int crc;
//	unsigned char Zero_Flag;
//	unsigned int Zero_Numb;
	if(Debugging_Flag==0)
	{
		if((rcvbuff[0]&0x0f)==(Communication_Machine+1))//判断首地址
		{
            if(SlaveCheckCRC(rcvbuff,rcvcount) != FALSE)//判断CRC校验
			{
                if(count_radar_uart == 63)
                {
                    //history_radar_warn_write("1# 通信正常","");
                    add_value_radarinfo("1# 通信正常","");
                }
                count_radar_uart = 0;
                switch(Send_MODE[Communication_Machine])//根据发送的来判断接受的小心     Slave_Address
				{
					case 0:	//如果是取背景命令
					{
						if(
							(rcvbuff[1]==5)&&
							(rcvbuff[2]==2)&&
							(rcvbuff[3]==1)
						   )//如果回应
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine] = 1;	//表示接受到正确的消息，开始查询状态
						
						}
						break;
					}
					case 1:
					{
						if(
								(rcvbuff[1]==1)&&
								(rcvbuff[2]==1) 
						   )//如果回应正确
						   {
						   	   Rec_Flag[Communication_Machine] = 1;
							   if(rcvbuff[3]==0)	//如果背景取好了
							   {
							   	 	Send_MODE[Communication_Machine] = 2;	   //开始读取背景
							   }
						   }
						break;
					}
					case 2://读背景信息0~6
					{	
						if(
							(rcvbuff[1]==3)&&
							(rcvbuff[2]==14) 
				    	)//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine] = 101;
	
							m = 0;
							for(i=0;i<7;i++)
							{
                                  Back_Groud_Temp[Communication_Machine][m+i][1] = rcvbuff[3+i*2];
                                  Back_Groud_Temp[Communication_Machine][m+i][0] = rcvbuff[4+i*2];
							}
						}				 
						break;
					}
                    case 101://读背景信息7~13
					{
					   if(
							(rcvbuff[1]==3)&&
							(rcvbuff[2]==14) 
				    	)//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine]++;
	
							m = 7;
							for(i=0;i<7;i++)
							{
                                  Back_Groud_Temp[Communication_Machine][m+i][1] = rcvbuff[3+i*2];
                                  Back_Groud_Temp[Communication_Machine][m+i][0] = rcvbuff[4+i*2];
							}
						}				 
						break;
					}
					case 102://读背景信息14~20
					{
						if(
							(rcvbuff[1]==3)&&
							(rcvbuff[2]==14) 
				    	)//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine]++;
	
							m = 14;
							for(i=0;i<7;i++)
							{
                                  Back_Groud_Temp[Communication_Machine][m+i][1] = rcvbuff[3+i*2];
                                  Back_Groud_Temp[Communication_Machine][m+i][0] = rcvbuff[4+i*2];
							}
						}				 
						break;
					}
					case 103://读背景信息21~27
					{
						if(
							(rcvbuff[1]==3)&&
							(rcvbuff[2]==14) 
				    	)//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine]++;
	
							m = 21;
							for(i=0;i<7;i++)
							{
                                  Back_Groud_Temp[Communication_Machine][m+i][1] = rcvbuff[3+i*2];
                                  Back_Groud_Temp[Communication_Machine][m+i][0] = rcvbuff[4+i*2];
							}
						}				 
						break;
					}
					case 104://读背景信息28~34
					{
						if(
							(rcvbuff[1]==3)&&
							(rcvbuff[2]==14) 
				    	)//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine]++;
	
							m = 28;
							for(i=0;i<7;i++)
							{
                                  Back_Groud_Temp[Communication_Machine][m+i][1] = rcvbuff[3+i*2];
                                  Back_Groud_Temp[Communication_Machine][m+i][0] = rcvbuff[4+i*2];
							}
						}				 
						break;
					}
					case 105://读背景信息35~41
					{
						if(
							(rcvbuff[1]==3)&&
							(rcvbuff[2]==14) 
				    	)//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine]++;
	
							m = 35;
							for(i=0;i<7;i++)
							{
                                  Back_Groud_Temp[Communication_Machine][m+i][1] = rcvbuff[3+i*2];
                                  Back_Groud_Temp[Communication_Machine][m+i][0] = rcvbuff[4+i*2];
							}
						}				 
						break;
					}
					case 106://读背景信息42~48
					{
						if(
							(rcvbuff[1]==3)&&
							(rcvbuff[2]==14) 
				    	)//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine]++;
	
							m = 42;
							for(i=0;i<7;i++)
							{
                                  Back_Groud_Temp[Communication_Machine][m+i][1] = rcvbuff[3+i*2];
                                  Back_Groud_Temp[Communication_Machine][m+i][0] = rcvbuff[4+i*2];
							}
						}				 
						break;
					}
					case 107://读背景信息49~55
					{
						if(
							(rcvbuff[1]==3)&&
							(rcvbuff[2]==14) 
				    	)//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine]++;
	
							m = 49;
							for(i=0;i<7;i++)
							{
                                  Back_Groud_Temp[Communication_Machine][m+i][1] = rcvbuff[3+i*2];
                                  Back_Groud_Temp[Communication_Machine][m+i][0] = rcvbuff[4+i*2];
							}
						}				 
						break;
					}
					case 108://读背景信息56~62
					{
						if(
							(rcvbuff[1]==3)&&
							(rcvbuff[2]==14) 
				    	)//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine]++;
	
							m = 56;
							for(i=0;i<7;i++)
							{
                                  Back_Groud_Temp[Communication_Machine][m+i][1] = rcvbuff[3+i*2];
                                  Back_Groud_Temp[Communication_Machine][m+i][0] = rcvbuff[4+i*2];
							}
						}				 
						break;
					}
					case 109://读背景信息63~69
					{
						if(
							(rcvbuff[1]==3)&&
							(rcvbuff[2]==14) 
				    	)//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine]++;
	
							m = 63;
							for(i=0;i<7;i++)
							{
                                  Back_Groud_Temp[Communication_Machine][m+i][1] = rcvbuff[3+i*2];
                                  Back_Groud_Temp[Communication_Machine][m+i][0] = rcvbuff[4+i*2];
							}
						}				 
						break;
					}
					case 110://读背景信息70~76
					{
						if(
							(rcvbuff[1]==3)&&
							(rcvbuff[2]==14) 
				    	)//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine]++;
	
							m = 70;
							for(i=0;i<7;i++)
							{
                                  Back_Groud_Temp[Communication_Machine][m+i][1] = rcvbuff[3+i*2];
                                  Back_Groud_Temp[Communication_Machine][m+i][0] = rcvbuff[4+i*2];
							}
						}				 
						break;
					}
					case 111://读背景信息77~83
					{
						if(
							(rcvbuff[1]==3)&&
							(rcvbuff[2]==14) 
				    	)//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine]++;
	
							m = 77;
							for(i=0;i<7;i++)
							{
                                  Back_Groud_Temp[Communication_Machine][m+i][1] = rcvbuff[3+i*2];
                                  Back_Groud_Temp[Communication_Machine][m+i][0] = rcvbuff[4+i*2];
							}
						}				 
						break;
					}
					case 112://读背景信息84~90
					{
						if(
							(rcvbuff[1]==3)&&
							(rcvbuff[2]==14) 
				    	)//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine]++;
	
							m = 84;
							for(i=0;i<7;i++)
							{
                                  Back_Groud_Temp[Communication_Machine][m+i][1] = rcvbuff[3+i*2];
                                  Back_Groud_Temp[Communication_Machine][m+i][0] = rcvbuff[4+i*2];
							}
						}				 
						break;
					}
					case 113://读背景信息91~97
					{
						if(
							(rcvbuff[1]==3)&&
							(rcvbuff[2]==14) 
				    	)//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine]++;
	
							m = 91;
							for(i=0;i<7;i++)
							{
                                  Back_Groud_Temp[Communication_Machine][m+i][1] = rcvbuff[3+i*2];
                                  Back_Groud_Temp[Communication_Machine][m+i][0] = rcvbuff[4+i*2];
							}
						}				 
						break;
					}
					case 114://读背景信息98~104
					{
						if(
							(rcvbuff[1]==3)&&
							(rcvbuff[2]==14) 
				    	)//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine]++;
	
							m = 98;
							for(i=0;i<7;i++)
							{
                                  Back_Groud_Temp[Communication_Machine][m+i][1] = rcvbuff[3+i*2];
                                  Back_Groud_Temp[Communication_Machine][m+i][0] = rcvbuff[4+i*2];
							}
						}				 
						break;
					}
					case 115://读背景信息105~111
					{
						if(
							(rcvbuff[1]==3)&&
							(rcvbuff[2]==14) 
				    	)//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine]++;
	
							m = 105;
							for(i=0;i<7;i++)
							{
                                  Back_Groud_Temp[Communication_Machine][m+i][1] = rcvbuff[3+i*2];
                                  Back_Groud_Temp[Communication_Machine][m+i][0] = rcvbuff[4+i*2];
							}
						}				 
						break;
					}
					case 116://读背景信息112~118
					{
						if(
							(rcvbuff[1]==3)&&
							(rcvbuff[2]==14) 
				    	)//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine]++;
	
							m = 112;
							for(i=0;i<7;i++)
							{
                                  Back_Groud_Temp[Communication_Machine][m+i][1] = rcvbuff[3+i*2];
                                  Back_Groud_Temp[Communication_Machine][m+i][0] = rcvbuff[4+i*2];
							}
						}				 
						break;
					}
					case 117://读背景信息119~125
					{
						if(
							(rcvbuff[1]==3)&&
							(rcvbuff[2]==14) 
				    	)//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine]++;
	
							m = 119;
							for(i=0;i<7;i++)
							{
                                  Back_Groud_Temp[Communication_Machine][m+i][1] = rcvbuff[3+i*2];
                                  Back_Groud_Temp[Communication_Machine][m+i][0] = rcvbuff[4+i*2];
							}
						}				 
						break;
					}
					case 118://读背景信息126~132
					{
						if(
							(rcvbuff[1]==3)&&
							(rcvbuff[2]==14) 
				    	)//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine]++;
	
							m = 126;
							for(i=0;i<7;i++)
							{
                                  Back_Groud_Temp[Communication_Machine][m+i][1] = rcvbuff[3+i*2];
                                  Back_Groud_Temp[Communication_Machine][m+i][0] = rcvbuff[4+i*2];
							}
						}				 
						break;
					}
					case 119://读背景信息133~139
					{
						if(
							(rcvbuff[1]==3)&&
							(rcvbuff[2]==14) 
				    	)//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine]++;
	
							m = 133;
							for(i=0;i<7;i++)
							{
                                  Back_Groud_Temp[Communication_Machine][m+i][1] = rcvbuff[3+i*2];
                                  Back_Groud_Temp[Communication_Machine][m+i][0] = rcvbuff[4+i*2];
							}
						}				 
						break;
					}
					case 120://读背景信息140~146
					{
						if(
							(rcvbuff[1]==3)&&
							(rcvbuff[2]==14) 
				    	)//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine]++;
	
							m = 140;
							for(i=0;i<7;i++)
							{
                                  Back_Groud_Temp[Communication_Machine][m+i][1] = rcvbuff[3+i*2];
                                  Back_Groud_Temp[Communication_Machine][m+i][0] = rcvbuff[4+i*2];
							}
						}				 
						break;
					}
					case 121://读背景信息147~153
					{
						if(
							(rcvbuff[1]==3)&&
							(rcvbuff[2]==14) 
				    	)//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine]++;
	
							m = 147;
							for(i=0;i<7;i++)
							{
                                  Back_Groud_Temp[Communication_Machine][m+i][1] = rcvbuff[3+i*2];
                                  Back_Groud_Temp[Communication_Machine][m+i][0] = rcvbuff[4+i*2];
							}
						}				 
						break;
					}
					case 122://读背景信息154~160
					{
						if(
							(rcvbuff[1]==3)&&
							(rcvbuff[2]==14) 
				    	)//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine]++;
	
							m = 154;
							for(i=0;i<7;i++)
							{
                                  Back_Groud_Temp[Communication_Machine][m+i][1] = rcvbuff[3+i*2];
                                  Back_Groud_Temp[Communication_Machine][m+i][0] = rcvbuff[4+i*2];
							}
						}				 
						break;
					}
					case 123://读背景信息161~167
					{
						if(
							(rcvbuff[1]==3)&&
							(rcvbuff[2]==14) 
				    	)//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine]++;
	
							m = 161;
							for(i=0;i<7;i++)
							{
                                  Back_Groud_Temp[Communication_Machine][m+i][1] = rcvbuff[3+i*2];
                                  Back_Groud_Temp[Communication_Machine][m+i][0] = rcvbuff[4+i*2];
							}
						}				 
						break;
					}
					case 124://读背景信息168~174
					{
						if(
							(rcvbuff[1]==3)&&
							(rcvbuff[2]==14) 
				    	)//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine]++;
	
							m = 168;
							for(i=0;i<7;i++)
							{
                                  Back_Groud_Temp[Communication_Machine][m+i][1] = rcvbuff[3+i*2];
                                  Back_Groud_Temp[Communication_Machine][m+i][0] = rcvbuff[4+i*2];
							}
						}				 
						break;
					}
					case 125://读背景信息175~179
					{
						if(
							(rcvbuff[1]==3)&&
							(rcvbuff[2]==10) 
				    	)//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
	
							if( Debug_Enable[Communication_Machine]==0 )
							{
                                Send_MODE[Communication_Machine]=11;  //读完背景开始问目标数量
			             	}
							else
							{
                                Send_MODE[Communication_Machine]=13;  //读完背景开始写背景
							}
	
                            Get_Back_Flag[Communication_Machine] = 0x38;

	
							m = 175;
							for(i=0;i<5;i++)
							{
                                  Back_Groud_Temp[Communication_Machine][m+i][1] = rcvbuff[3+i*2];
                                  Back_Groud_Temp[Communication_Machine][m+i][0] = rcvbuff[4+i*2];
							}
						}				 
						break;
					}
				
					case 11://读取目标信息
					{
						 if(
							(rcvbuff[1]==3)&&
							(rcvbuff[2]==18) 
						   )//如果回应正确
//                             for(unsigned char i = 0; i < len_uart; i++)           //DEBUG
//                             {                                       //DEBUG
//                                 printf("%02x ",rcvbuff[i]); //DEBUG       //12.27

//                             }
						   printf("^^^radar\n");
						   {
                              Rec_Flag[Communication_Machine] = 1;
	
								if((rcvbuff[3]>>7)&0x01)//如果需要写入背景
								{
                                    Send_MODE[Communication_Machine]=13;
								}
                                if((Get_Are_Flag[Communication_Machine]==1)&&(Send_MODE[Communication_Machine]==11))
								{
									Get_Are_Flag[Communication_Machine] = 0;
                                    Send_MODE[Communication_Machine] = 33;
									Ask_Are_Flag[Communication_Machine] = 1;
								} //end"if(Get_Are_Flag==1)"
								else if((Get_Are_Flag[Communication_Machine]==2)&&(Send_MODE[Communication_Machine]==11))
								{
									Get_Are_Flag[Communication_Machine] = 0;
									Send_MODE[Communication_Machine] = 36; 
									Ask_Are_Flag[Communication_Machine] = 0;
								}  //end "else if(Get_Are_Flag==2)"								
								else if((Send_MODE[Communication_Machine]==11))
								{
									if( Debug_Enable[Communication_Machine]==0 )
									{
										if(Ask_Are_Flag[Communication_Machine])
										{
												Send_MODE[Communication_Machine]=34; 
										}
										else
										{
					                		Send_MODE[Communication_Machine]=11; //读完背景开始问目标数量
										}
					        }
									else
									{
										Send_MODE[Communication_Machine]=13; //读完背景开始写背景
									}	   	
								}//end else if((Send_MODE[Communication_Machine]==11))
	
								if((Send_Air_Flag[Communication_Machine])&&(Send_MODE[Communication_Machine]==11))
								{
									Send_Air_Flag[Communication_Machine] = 0;
									Send_MODE[Communication_Machine] = 37;
								}
								if((Send_Sensitivity_Flag)&&(Send_MODE[Communication_Machine]==11))
								{
									  Send_Sensitivity_Flag = 0;
									  Send_MODE[Communication_Machine] = 43;
								}
								for(i=0;i<20;i++)
								{
									for(j=0;j<2;j++)
									{
                                        Goal[Communication_Machine][i][j] = 0;//首先清空目标位置
									}
								}
								//	rcvbuff[3]：BIT7:需要写入背景的标志位
								//	rcvbuff[3]：BIT0~BIT5，x坐标的高位
								//	rcvbuff[4]：BIT0~BIT5，y坐标的高位
								//	rcvbuff[5]~rcvbuff[16]：x坐标和y坐标
								//  rcvbuff[17]：BIT7：总报警标志位，BIT0~BIT5：6个目标的报警状态
								
								m = 0;
								for(i=0;i<6;i++)
								{
									temp = 	(rcvbuff[3]>>i)&0x01;
									temp_x = (rcvbuff[5+i*2])|(temp<<8);
                                    Goal[Communication_Machine][i][0] = -(250 - temp_x);
                                    Goal[Communication_Machine][i][0] /= 10;//X坐标
	
									temp =  (rcvbuff[4]>>i)&0x01;
									temp_y = (rcvbuff[6+i*2])|(temp<<8);							 
									Goal[Communication_Machine][i][1] = temp_y ;
                                    Goal[Communication_Machine][i][1] /= 10;//Y坐标
									if(temp_y)
									{
										m++	;
									}
									 
								}

                                //2019-11-27 增加，降低灵敏度  灵敏度可调
                                if(alarm_re_flag_pre[Communication_Machine][0] == rcvbuff[17])
                                {
                                    alarm_re_flag_pre[Communication_Machine][1]++;
                                    if(alarm_re_flag_pre[Communication_Machine][1]>=Flag_sensitivity)
                                    {
                                        Alarm_Re_Flag[Communication_Machine] = rcvbuff[17];//报警状态
                                        alarm_re_flag_pre[Communication_Machine][1] = 10;
                                    }
                                }
                                else
                                {
                                    alarm_re_flag_pre[Communication_Machine][0] = rcvbuff[17];
                                    alarm_re_flag_pre[Communication_Machine][1] = 0;
                                }

                                OBJ_numb = m;
                                Com_Update_Flag[Communication_Machine] = 1;
						   }
                           if(Flag_Sound_Radar_temp)
                           {
                               Flag_Sound_Radar[1] = 1;
                           }
                           else
                           {
                               Flag_Sound_Radar[1] = 0;
                           }
						   break;
					}
				
                    case 13://写背景信息0~6 	21	0  阈值更新
					{
						if(
							(rcvbuff[1]==16)&&
							(rcvbuff[2]==21)&&
							(rcvbuff[3]==7)
						  )//\C8\E7\B9\FB\BB\D8Ӧ\D5\FDȷ
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine] = 126;
						}									 
						break;
					}      
					case 126://写背景信息7~13	28	7
					{
						if(
							(rcvbuff[1]==16)&&
							(rcvbuff[2]==28)&&
							(rcvbuff[3]==7)
						  )//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine]++;
						}									 
						break;
					}
					case 127://写背景信息14~20	35	14
					{
						if(
							(rcvbuff[1]==16)&&
							(rcvbuff[2]==35)&&
							(rcvbuff[3]==7)
						  )//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine]++;
						}									 
						break;
					}
					case 128://写背景信息21~27	42	21
					{
						if(
							(rcvbuff[1]==16)&&
							(rcvbuff[2]==42)&&
							(rcvbuff[3]==7)
						  )//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine]++;
						}									 
						break;
					}
					case 129://写背景信息28~34	49	28
					{
						 if(
							(rcvbuff[1]==16)&&
							(rcvbuff[2]==49)&&
							(rcvbuff[3]==7)
						  )//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine]++;
						}									 
						break;
					}
					case 130://写背景信息35~41	56	35
					{
						 if(
							(rcvbuff[1]==16)&&
							(rcvbuff[2]==56)&&
							(rcvbuff[3]==7)
						  )//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine]++;
						}									 
						break;
					}
					case 131://写背景信息42~48	63	42
					{
						 if(
							(rcvbuff[1]==16)&&
							(rcvbuff[2]==63)&&
							(rcvbuff[3]==7)
						  )//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine]++;
						}									 
						break;
					}
					case 132://写背景信息49~55	70	49
					{
						 if(
							(rcvbuff[1]==16)&&
							(rcvbuff[2]==70)&&
							(rcvbuff[3]==7)
						  )//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine]++;
						}									 
						break;
					}
					case 133://写背景信息56~62	77	56
					{
						 if(
							(rcvbuff[1]==16)&&
							(rcvbuff[2]==77)&&
							(rcvbuff[3]==7)
						  )//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine]++;
						}									 
						break;
					}
					case 134://写背景信息63~69	84	63
					{
						 if(
							(rcvbuff[1]==16)&&
							(rcvbuff[2]==84)&&
							(rcvbuff[3]==7)
						  )//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine]++;
						}									 
						break;
					}
					case 135://写背景信息70~76	91	70
					{
						  if(
							(rcvbuff[1]==16)&&
							(rcvbuff[2]==91)&&
							(rcvbuff[3]==7)
						  )//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine]++;
						}									 
						break;
					}
					case 136://写背景信息77~83	98	77
					{
						if(
							(rcvbuff[1]==16)&&
							(rcvbuff[2]==98)&&
							(rcvbuff[3]==7)
						  )//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine]++;
						}									 
						break;
					}
					case 137://写背景信息84~90	105	84
					{
						if(
							(rcvbuff[1]==16)&&
							(rcvbuff[2]==105)&&
							(rcvbuff[3]==7)
						  )//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine]++;
						}									 
						break;
					}
					case 138://写背景信息91~97	112	91
					{
						if(
							(rcvbuff[1]==16)&&
							(rcvbuff[2]==112)&&
							(rcvbuff[3]==7)
						  )//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine]++;
						}									 
						break;
					}
					case 139://写背景信息98~104	119	98
					{
						 if(
							(rcvbuff[1]==16)&&
							(rcvbuff[2]==119)&&
							(rcvbuff[3]==7)
						  )//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine]++;
						}									 
						break;
					}
					case 140://写背景信息105~111	126	105
					{
						 if(
							(rcvbuff[1]==16)&&
							(rcvbuff[2]==126)&&
							(rcvbuff[3]==7)
						  )//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine]++;
						}									 
						break;
					}
					case 141://写背景信息112~118	133	112
					{
						if(
							(rcvbuff[1]==16)&&
							(rcvbuff[2]==133)&&
							(rcvbuff[3]==7)
						  )//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine]++;
						}									 
						break;
					}
					case 142://写背景信息119~125	140	119
					{
						if(
							(rcvbuff[1]==16)&&
							(rcvbuff[2]==140)&&
							(rcvbuff[3]==7)
						  )//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine]++;
						}									 
						break;
					}
					case 143://写背景信息126~132	147	126
					{
						if(
							(rcvbuff[1]==16)&&
							(rcvbuff[2]==147)&&
							(rcvbuff[3]==7)
						  )//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine]++;
						}									 
						break;
					}
					case 144://写背景信息133~139	154	133
					{
						 if(
							(rcvbuff[1]==16)&&
							(rcvbuff[2]==154)&&
							(rcvbuff[3]==7)
						  )//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine]++;
						}									 
						break;
					}
					case 145://写背景信息140~146	161	140
					{
						if(
							(rcvbuff[1]==16)&&
							(rcvbuff[2]==161)&&
							(rcvbuff[3]==7)
						  )//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine]++;
						}									 
						break;
					}
					case 146://写背景信息147~153	168	147
					{
						if(
							(rcvbuff[1]==16)&&
							(rcvbuff[2]==168)&&
							(rcvbuff[3]==7)
						  )//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine]++;
						}									 
						break;
					}
					case 147://写背景信息154~160	175	154
					{
						if(
							(rcvbuff[1]==16)&&
							(rcvbuff[2]==175)&&
							(rcvbuff[3]==7)
						  )//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine]++;
						}									 
						break;
					}
					case 148://写背景信息161~167	182	161
					{
					    if(
							(rcvbuff[1]==16)&&
							(rcvbuff[2]==182)&&
							(rcvbuff[3]==7)
						  )//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine]++;
						}									 
						break;
					}
					case 149://写背景信息168~174	189	168
					{
						 if(
							(rcvbuff[1]==16)&&
							(rcvbuff[2]==189)&&
							(rcvbuff[3]==7)
						  )//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine]++;
						}									 
						break;
					}
					case 150://写背景信息175~179	196	175
					{
						  if(
							(rcvbuff[1]==16)&&
							(rcvbuff[2]==196)&&
							(rcvbuff[3]==5)
						  )//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
							if( Debug_Enable[Communication_Machine]==0 )
							{
                                Send_MODE[Communication_Machine]=37;  //写完背景开始写入区域
			             	}
							else
							{
                                Send_MODE[Communication_Machine]=0;  //读完背景开始写背景
							}					
						}									 
						break;
					}

					case 31:	//读取上升沿AMP_R
					{
						if(
							(rcvbuff[1]==3)&&
							(rcvbuff[2]==30) 
						   )//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
							for(i=0;i<30;i++)
							{
								Amp_R_Date[i]= rcvbuff[3+i];
                            }
                            if(((Page_Numb==17)||(Page_Numb==20)||(Page_Numb==22))&&(Threshold_Setting_Step==0))
							{
                                Get_CFAR_Back_C_2();
							}
						}
						break;
					}
	
					case 32:	//读取下降沿AMP_R
					{
						if(
							(rcvbuff[1]==3)&&
							(rcvbuff[2]==30) 
						   )//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
							for(i=0;i<30;i++)
							{
								Amp_R_Date[i]= rcvbuff[3+i];
							} 
                            if(((Page_Numb==17)||(Page_Numb==20)||(Page_Numb==22))&&(Threshold_Setting_Step==0))
							{
                                Get_CFAR_Back_C_2();
							}
						}
						break;
					}
	
					case 33:	//如果是取检测区域命令
					{
						if(
							(rcvbuff[1]==5)&&
							(rcvbuff[2]==4)&&
							(rcvbuff[3]==1)
						   )//如果回应
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine] = 11;//表示接受到正确的消息，开始查询目标数量						
						}
						break;
					}
	
					case 34://如果是询问区域检测
					{
						if(
                                (rcvbuff[1]==1)&&
                                (rcvbuff[2]==1)
						   )//如果回应正确
						   {
                               Rec_Flag[Communication_Machine] = 1;
							   if(rcvbuff[3]==0)	//如果区域取好了
							   {
                                    Send_MODE[Communication_Machine] = 35;	   //开始读取背景
							   }
							   else
							   {
                                    Send_MODE[Communication_Machine] = 11;	   //如果没有取好，继续查询目标数量
							   }
						   }
                           break;
					}
	
	
                    case 35:    //读取
					{
						if(
					  		(rcvbuff[1]==3)&&
							(rcvbuff[2]==16) 	//(rcvbuff[2]==80)
						   )//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
						//	Send_MODE[Communication_Machine]=11; //继续询问目标数量 
							if( Debug_Enable[Communication_Machine]==0 )
							{
                                Send_MODE[Communication_Machine]=11; //读完背景开始问目标数量
                            }
							else
							{
                                Send_MODE[Communication_Machine]=13; //读完背景开始写背景
							}
							Ask_Are_Flag[Communication_Machine] = 0;
							Write_Control[214+Communication_Machine] = 1;
							for(i=0;i<6;i++)
							{							 
							//	Boundary_Point_Disp[0][i][0] = 0;
							// 	Boundary_Point_Disp[0][i][1] = 0;
                                Master_Boundary_Point_Disp[Communication_Machine][0][i][0] = 0;
                                Master_Boundary_Point_Disp[Communication_Machine][0][i][1] = 0;
							}  
							for(i=0,m=0,n=0;i<8;i++)
							{
								temp_1 = rcvbuff[3+i*2];
								temp_2 = rcvbuff[3+i*2+1];
								if(n)
								{
									temp_3= ((temp_1&0xFF)|((temp_2<<8)&0xFF00)) ; 
									//Boundary_Point_Disp[0][m][1] =  temp_3;
                                    Master_Boundary_Point_Disp[Communication_Machine][0][m][1] = temp_3;        //区域自动设置X
								}
								else
								{
									temp_4 = (250-((temp_1&0xFF)|((temp_2<<8)&0xFF00))) ;	 
									//Boundary_Point_Disp[0][m][0] = temp_4;
                                    Master_Boundary_Point_Disp[Communication_Machine][0][m][0] = temp_4;  //区域自动设置Y
								}
								n++;
								if(n>1)
								{
									n=0;
									m++;
								}
							} 
                            Flag_autoget_area = 1;
						}
						break;
					}
	
					case 36:	//如果是取消检测区域命令
					{
						if(
							(rcvbuff[1]==5)&&
							(rcvbuff[2]==4)&&
							(rcvbuff[3]==0)
						   )//如果回应
                            {
                                Rec_Flag[Communication_Machine] = 1;
                                Send_MODE[Communication_Machine] = 11;	//表示接受到正确的消息，开始查询目标数量

                            }
                            break;
					}
					case 37:  //写入区域一
					{
					   if(
							(rcvbuff[1]==16)&&
							(rcvbuff[2]==241)&&
							(rcvbuff[3]==1)
						  )//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine]++;
						}									 
						break;
					}
					case 38://写入区域二
					{
						 if(
							(rcvbuff[1]==16)&&
							(rcvbuff[2]==242)&&
							(rcvbuff[3]==1)
						  )//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine]++;
						}									 
						break;
					}
					case 39://写入区域三
					{
					    if(
							(rcvbuff[1]==16)&&
							(rcvbuff[2]==243)&&
							(rcvbuff[3]==1)
						  )//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine]++;
						}									 
						break;
					}
					case 40://写入区域四
					{
						 if(
							(rcvbuff[1]==16)&&
							(rcvbuff[2]==244)&&
							(rcvbuff[3]==1)
						  )//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine]++;
						}									 
						break;
					}
					case 41://写入区域五
					{
						   if(
							(rcvbuff[1]==16)&&
							(rcvbuff[2]==245)&&
							(rcvbuff[3]==1)
						  )//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine]++;
						}									 
						break;
					}
					case 42://写入区域六
					{
						   if(
							(rcvbuff[1]==16)&&
							(rcvbuff[2]==246)&&
							(rcvbuff[3]==1)
						  )//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine]++;
						}									 
						break;
					}
					case 43://写入灵敏度
					{
						 if(
							(rcvbuff[1]==16)&&
							(rcvbuff[2]==247)&&
							(rcvbuff[3]==1)
						  )//如果回应正确
						{
                            Rec_Flag[Communication_Machine] = 1;
                            Send_MODE[Communication_Machine] = 11;
						}									 
						break;
                    }
				 	default:break;
				}
			}
            else
            {
                printf("read error crc\n");
                //雷达通信故障
                if(count_radar_uart > 60)
                {
                    Flag_Sound_Radar[0] = 1;
                    if(count_radar_uart == 61)
                    {
                        //history_radar_warn_write("1# 通信故障","");
                        add_value_radarinfo("1# 通信故障","");
                    }
                    count_radar_uart = 62;
                }
                Flag_Sound_Radar[1] = 0;
                Flag_Sound_Radar[2] = 0;
                count_radar_uart++;
            }
		}
        else
        {
            printf("read error local\n");
            //雷达通信故障
            if(count_radar_uart > 60)
            {
                Flag_Sound_Radar[0] = 1;
                if(count_radar_uart == 61)
                {
                    //history_radar_warn_write("1# 通信故障","");
                    add_value_radarinfo("1# 通信故障","");
                }
                count_radar_uart = 62;
            }
            Flag_Sound_Radar[1] = 0;
            Flag_Sound_Radar[2] = 0;
            count_radar_uart++;
        }
	}	 //end"if(Debugging_Flag==0)"
	else//如果是调试模式
	{

	}
	return;
}


void Get_CFAR_Back_C_2(void)
{
	int i;
	int j;
	float temp_date[10][2];	
 		for(i=0;i<10;i++)
		{
            temp_date[i][0] = Amp_R_Date[i*3];
		   
		    temp_date[i][1] =((Amp_R_Date[i*3+1]<<8)|Amp_R_Date[i*3+2]);		 
		}
		for(i=0;i<10;i++)
		{
			j = temp_date[i][0];
			if(j)
			{
				if(Back_C[j]<temp_date[i][1])
				{
					Back_C[j] = temp_date[i][1];
				}
			}
		}	
}

void gpio_11_high()
{
    int fd_export = open(EXPORT_PATH_RADAR,O_WRONLY);   //打开gpio设备导出设备
    if(fd_export < 0)
    {
        perror("open export error");
    }
    write(fd_export,RADAR_CON_11,strlen(RADAR_CON_11));   //向export文件写入gpio排列序号字符串
    int fd_dir;
    int ret;
    fd_dir = open(DIRECT_PATH_11,O_RDWR);
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
    fd_val = open(VALUE_PATH_11,O_RDWR);
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
void gpio_11_low()
{
    int fd_export = open(EXPORT_PATH_RADAR,O_WRONLY);   //打开gpio设备导出设备
    if(fd_export < 0)
    {
        perror("open export error");
    }
    write(fd_export,RADAR_CON_11,strlen(RADAR_CON_11));   //向export文件写入gpio排列序号字符串
    int fd_dir;
    int ret;
    fd_dir = open(DIRECT_PATH_11,O_RDWR);
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
    fd_val = open(VALUE_PATH_11,O_RDWR);
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

