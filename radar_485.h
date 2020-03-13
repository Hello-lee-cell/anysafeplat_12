#ifndef _485DATA_PROCESS_H
#define _485DATA_PROCESS_H

#define HOLDREGISTERAMOUNT 37
#define INPUTREGISTERAMOUNT 127
#define COILAMOUNT  25
/////////////////////////错误代码
#define ILLEGAL_FUNCTION  0X01	  //  无效功能码
#define ILLEGAL_DATA_ADDRESS   0X02	 //无效地址数据
#define ILLEGAL_DATA_VALUE     0X03	 //无效数据值
#define BUSY_REJECTED_MESSAGE  0X06	 //下位机忙
/*****@@@@@@@@@@@@@@added by G @@@@@@@******/
#define EXPORT_PATH_RADAR "/sys/class/gpio/export"
#define RADAR_CON_11 "11"
#define DIRECT_PATH_11 "/sys/class/gpio/gpio11/direction"
#define VALUE_PATH_11  "/sys/class/gpio/gpio11/value"
#define IO_DIRECTOUT    "out"
#define VALUE_HIGH   "1"
#define VALUE_LOW   "0"

void gpio_11_high();
void gpio_11_low();

//extern unsigned char Flag_moni_warn;
        //extern unsigned char Slave_Address_Temp; //与显示界面交互的变量

void ModbusInit(void);

void Send_Command(unsigned char Slave_Address);


bool SlaveCheckCRC(unsigned char *revframe,int framelen); 
bool SlaveCheckCRC_LCD(unsigned char *revframe,int framelen);
void CommunicationProcess(unsigned char* rcvbuff,unsigned char rcvcount);

//extern unsigned char Get_Are_Flag[4];
//extern unsigned char Send_MODE[4] ;     //通信传输的消息模式
//extern float  Goal[4][20][2]; //存储目标位置
//extern unsigned char  Get_Back_Flag[4];
//extern unsigned char Back_Groud_Temp[4][200][2];//定义一个200*4的二维数组存背景值，现在只使用180个

        //extern unsigned int Last_Distance_A;
        //extern unsigned int Last_CFAR_Value_A[2];
        //extern unsigned int Cursor_Time;
//extern int Master_Boundary_Point_Disp[4][6][6][2];
//extern unsigned char Master_Back_Groud_Value[4][200][2];
 void Get_CFAR_Back_C_2(void);

//extern unsigned char Amp_R_Date[30];   //从前端读回的幅值
//extern unsigned char Flag_autoget_area;
 //*************************added by G***********/
void MYDMA1CH2_Enable(unsigned int *a, unsigned char b);

//extern unsigned char Flag_Set_SendMode;

#endif					 
		




