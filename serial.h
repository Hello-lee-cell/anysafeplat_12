#ifndef SERIAL_H
#define SERIAL_H

/*********************变量声明********************/
//extern unsigned char Sencor_Num[4];
extern unsigned char Test_Method;
/*********************函数声明********************/
void Data_Handle(void);
//void Init_Uart_SP0(void);
void Data_Handle_SF(void);
void Data_Handle_FF(void);
void Data_Handle_Pressure(void);
void Data_Transfer(void);
void Data_Analysis(void);
unsigned int CRC_Test(unsigned char *ReceData,unsigned char RecLen);

unsigned int CRC_Radar(unsigned char* puchMSG,unsigned int usDataLen);  //雷达用校验，区别为数据长度少两位

int  set_port_attr (
                          int fd,
                          int  baudrate,          // B1200 B2400 B4800 B9600 .. B115200
                          int  databit,           // 5, 6, 7, 8
                          const char *stopbit,    //  "1", "1.5", "2"
                          char parity,            // N(o), O(dd), E(ven)
                          int vtime,
                          int vmin
                    );




#endif // SERIAL_H



