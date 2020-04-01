#ifndef _RADAR_485_H
#define _RADAR_485_H
//#include <stm32f10x_type.h>

#define HOLDREGISTERAMOUNT 37
#define INPUTREGISTERAMOUNT 127
#define COILAMOUNT  25
/////////////////////////�������
#define ILLEGAL_FUNCTION  0X01	 //��Ч������
#define ILLEGAL_DATA_ADDRESS   0X02	  //��Ч��ַ����
#define ILLEGAL_DATA_VALUE     0X03	 //��Ч����ֵ
#define BUSY_REJECTED_MESSAGE  0X06	  //��λ��æ


#define RS485_CON1 PAout(0)      	//485ģ�鷢�ͽ��տ��� 

void ModbusInit(void);

void Send_Command(unsigned char Slave_Address);
//void RECORD_ALARM(u8 LOOP_NUM,u8 CHANNEL_NUM,u8 Data );
//void RECORD_BAD(u8 LOOP_NUM,u8 CHANNEL_NUM,u8 Data );


	 	 
//bool GetFunctioncode(unsigned char *revframe, unsigned char *funcode); 	 
bool SlaveCheckCRC(unsigned char *revframe,int framelen); 
bool SlaveCheckCRC_LCD(unsigned char *revframe,int framelen);
void CommunicationProcess(unsigned char* rcvbuff,unsigned char rcvcount);
//void CommunicationBad(void);

extern unsigned char Rec_Flag[4];	 //ͨ���յ��𸴵ı�־
 
extern unsigned char Send_MODE[4] ;    //ͨ�Ŵ������Ϣģʽ

		 
extern unsigned char OBJ_numb;
//extern float OBJ_Location[20][2];
extern float  Goal[4][20][2];  //�洢Ŀ��λ��
//extern unsigned char Back_Groud_Value[200][2];	 
//extern u8 Per_Address;
//extern u8 Flag_rec_bad;
//extern u8 alarm_flag[254][8];
 extern unsigned char Com_Update_Flag[4];
 extern unsigned char  Get_Back_Flag[4];
 extern unsigned char Back_Groud_Temp[4][200][2];//����һ��200��4�Ķ�ά����汳��ֵ������ֻʹ��180��

extern unsigned int Last_Distance_A;
extern unsigned int Last_CFAR_Value_A[2];
extern unsigned int Cursor_Time;
extern int Master_Boundary_Point_Disp[4][6][6][2];
extern unsigned char Master_Back_Groud_Value[4][200][2];
 void Get_CFAR_Back_C_2(void);
 extern unsigned char Threshold_Setting_Step;
 extern unsigned int Page_Numb;
#endif					 
		



