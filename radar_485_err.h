#ifndef _RADAR_485_H
#define _RADAR_485_H
//#include <stm32f10x_type.h>

#define HOLDREGISTERAMOUNT 37
#define INPUTREGISTERAMOUNT 127
#define COILAMOUNT  25
/////////////////////////错误代码
#define ILLEGAL_FUNCTION  0X01	 //无效功能码
#define ILLEGAL_DATA_ADDRESS   0X02	  //无效地址数据
#define ILLEGAL_DATA_VALUE     0X03	 //无效数据值
#define BUSY_REJECTED_MESSAGE  0X06	  //下位机忙


#define RS485_CON1 PAout(0)      	//485模块发送接收控制 

void ModbusInit(void);

void Send_Command(unsigned char Slave_Address);
//void RECORD_ALARM(u8 LOOP_NUM,u8 CHANNEL_NUM,u8 Data );
//void RECORD_BAD(u8 LOOP_NUM,u8 CHANNEL_NUM,u8 Data );


	 	 
//bool GetFunctioncode(unsigned char *revframe, unsigned char *funcode); 	 
bool SlaveCheckCRC(unsigned char *revframe,int framelen); 
bool SlaveCheckCRC_LCD(unsigned char *revframe,int framelen);
void CommunicationProcess(unsigned char* rcvbuff,unsigned char rcvcount);
//void CommunicationBad(void);

extern unsigned char Rec_Flag[4];	 //通信收到答复的标志
 
extern unsigned char Send_MODE[4] ;    //通信传输的消息模式

		 
extern unsigned char OBJ_numb;
//extern float OBJ_Location[20][2];
extern float  Goal[4][20][2];  //存储目标位置
//extern unsigned char Back_Groud_Value[200][2];	 
//extern u8 Per_Address;
//extern u8 Flag_rec_bad;
//extern u8 alarm_flag[254][8];
 extern unsigned char Com_Update_Flag[4];
 extern unsigned char  Get_Back_Flag[4];
 extern unsigned char Back_Groud_Temp[4][200][2];//定义一个200×4的二维数组存背景值，现在只使用180个

extern unsigned int Last_Distance_A;
extern unsigned int Last_CFAR_Value_A[2];
extern unsigned int Cursor_Time;
extern int Master_Boundary_Point_Disp[4][6][6][2];
extern unsigned char Master_Back_Groud_Value[4][200][2];
 void Get_CFAR_Back_C_2(void);
 extern unsigned char Threshold_Setting_Step;
 extern unsigned int Page_Numb;
#endif					 
		



