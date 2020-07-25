/*
 * Program:     serial.c
 * Author:      Paul Dean
 * Version:     0.0.3
 * Date:        2002-02-19
 * Description: To provide underlying serial port function,
 *              for high level applications.
 *
*/

#include <termios.h>            /* tcgetattr, tcsetattr */
#include <stdio.h>              /* perror, printf, puts, fprintf, fputs */
#include <unistd.h>             /* read, write, close */
#include <fcntl.h>              /* open */
#include <sys/signal.h>
#include <termios.h>
#include <sys/types.h>
#include <string.h>             /* bzero, memcpy */
#include <limits.h>             /* CHAR_MAX */

#include "mainwindow.h"
#include "file_op.h"
#include "serial.h"
#include"config.h"
/**************************变量定义**************************/
const unsigned char auchCRCHi[] = {
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
0x80, 0x41, 0x00, 0xC1, 0x81, 0x40
} ;

const unsigned char auchCRCLo[] = {
0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06,
0x07, 0xC7, 0x05, 0xC5, 0xC4, 0x04, 0xCC, 0x0C, 0x0D, 0xCD,
0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09,
0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A,
0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC, 0x14, 0xD4,
0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3,
0xF2, 0x32, 0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4,
0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A,
0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29,
0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF, 0x2D, 0xED,
0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60,
0x61, 0xA1, 0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67,
0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F,
0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68,
0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA, 0xBE, 0x7E,
0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71,
0x70, 0xB0, 0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92,
0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C,
0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B,
0x99, 0x59, 0x58, 0x98, 0x88, 0x48, 0x49, 0x89, 0x4B, 0x8B,
0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42,
0x43, 0x83, 0x41, 0x81, 0x80, 0x40
} ;

unsigned char Data_Buf_Sencor_Convert[64] = {0xBB,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                             0xBB,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                             0xBB,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                             0xBB,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
unsigned char Sencor_Num[4] = {0x00,0x00,0x00,0x00};//配置的传感器数目，低位到高位依次是：油管、管线、加油机、防渗池
//unsigned char Test_Method = 0x03;                   //检测方法标志位，00其他方法，01压力法，02液媒法，03传感器法

float pre_press[8] = {0,0,0,0,0,0,0,0 };//记录上一次的压力值
unsigned char flag_pre1[8] = {0,0,0,0,0,0,0,0}; //连续11个值，用来做标志位每个分段只记录一次
unsigned char flag_pre2[8] = {0,0,0,0,0,0,0,0};
unsigned char flag_pre3[8] = {0,0,0,0,0,0,0,0};
unsigned char flag_pre4[8] = {0,0,0,0,0,0,0,0};
unsigned char flag_pre5[8] = {0,0,0,0,0,0,0,0};
unsigned char flag_pre6[8] = {0,0,0,0,0,0,0,0};
unsigned char flag_pre7[8] = {0,0,0,0,0,0,0,0};
unsigned char flag_pre8[8] = {0,0,0,0,0,0,0,0};
unsigned char flag_pre9[8] = {0,0,0,0,0,0,0,0};
unsigned char flag_pre10[8] = {0,0,0,0,0,0,0,0};
unsigned char flag_pre11[8] = {0,0,0,0,0,0,0,0};
unsigned char flag_pre12[8] = {0,0,0,0,0,0,0,0};
unsigned char flag_pre13[8] = {0,0,0,0,0,0,0,0};
unsigned char flag_pre14[8] = {0,0,0,0,0,0,0,0};
unsigned char flag_pre15[8] = {0,0,0,0,0,0,0,0};
unsigned char flag_pre16[8] = {0,0,0,0,0,0,0,0};
unsigned char flag_pre17[8] = {0,0,0,0,0,0,0,0};
unsigned char flag_pre18[8] = {0,0,0,0,0,0,0,0};
unsigned char flag_pre19[8] = {0,0,0,0,0,0,0,0};
unsigned char flag_pre20[8] = {0,0,0,0,0,0,0,0};

/***********************************************************/
/* set serial port baudrate by use of file descriptor fd */
static void set_baudrate (struct termios *opt, unsigned int baudrate)
{
    cfsetispeed(opt, baudrate);
    cfsetospeed(opt, baudrate);
}

static void set_data_bit (struct termios *opt, unsigned int databit)
{
    opt->c_cflag &= ~CSIZE;
    switch (databit) {
    case 8:
        opt->c_cflag |= CS8;
        break;
    case 7:
        opt->c_cflag |= CS7;
        break;
    case 6:
        opt->c_cflag |= CS6;
        break;
    case 5:
        opt->c_cflag |= CS5;
        break;
    default:
        opt->c_cflag |= CS8;
        break;
    }
}

static void set_stopbit (struct termios *opt, const char *stopbit)
{
    if (0 == strcmp (stopbit, "1")) {
        opt->c_cflag &= ~CSTOPB; /* 1 stop bit */
    }	else if (0 == strcmp (stopbit, "1")) {
        opt->c_cflag &= ~CSTOPB; /* 1.5 stop bit */
    }   else if (0 == strcmp (stopbit, "2")) {
        opt->c_cflag |= CSTOPB;  /* 2 stop bits */
    } else {
        opt->c_cflag &= ~CSTOPB; /* 1 stop bit */
    }
}

static void set_parity (struct termios *opt, char parity)
{
    switch (parity)
    {
        case 'N':                  /* no parity check */
            opt->c_cflag &= ~PARENB;
            break;
        case 'E':                  /* even */
            opt->c_cflag |= PARENB;
            opt->c_cflag &= ~PARODD;
            break;
        case 'O':                  /* odd */
            opt->c_cflag |= PARENB;
            opt->c_cflag |= ~PARODD;
            break;
        default:                   /* no parity check */
            opt->c_cflag &= ~PARENB;
            break;
    }
}


int  set_port_attr (
                        int fd,
                        int  baudrate,          // B1200 B2400 B4800 B9600 .. B115200
                        int  databit,           // 5, 6, 7, 8
                        const char *stopbit,    //  "1", "1.5", "2"
                        char parity,            // N(o), O(dd), E(ven)
                        int vtime,
                        int vmin
                   )
{
        struct termios opt;
//    	bzero(&opt, sizeof (opt));
//      cfmakeraw (&termios_new);

        tcgetattr(fd, &opt);                    //获得当前设备模式，与终端相关的参数。ｆｄ＝０标准输入

        set_baudrate(&opt, baudrate);

        opt.c_cflag 		 |= CLOCAL | CREAD;      /* | CRTSCTS */

        set_data_bit(&opt, databit); //数据位
        set_parity(&opt, parity); //校验位
        set_stopbit(&opt, stopbit);//停止位设置

        opt.c_lflag   &=   ~(ECHO   |   ICANON   |   IEXTEN   |   ISIG);
        opt.c_iflag   &=   ~(BRKINT   |   ICRNL   |   INPCK   |   ISTRIP   |   IXON);
        opt.c_oflag   &=   ~(OPOST);
//        opt.c_cflag   &=   ~(CSIZE   |   PARENB);
        opt.c_cflag   |=   CS8;

        opt.c_oflag = 0;
        opt.c_lflag |= 0;
        opt.c_oflag &= ~OPOST;

        opt.c_cc[VTIME]     	 = vtime;//设置非规范模式下的超时时长和最小字符数：
        opt.c_cc[VMIN]         	 = vmin;//VTIME与VMIN配合使用，是指限定的传输或等待的最长时间
        tcflush (fd, TCIFLUSH);                 //刷新输入队列

//        若 VMIN = 0 ，VTIME = 0  ，函数read未读到任何参数也立即返回，相当于非阻塞模式；
//        若 VMIN = 0,   VTIME > 0  ，函数read读取到数据立即返回，若无数据则等待VTIME时间返回；
//        若 VMIN > 0,   VTIME = 0  ，函数read()只有在读取到VMIN个字节的数据或者收到一个信号的时候才返回；
//        若 VMIN > 0,   VTIME > 0  ，从read读取第一个字节的数据时开始计时，并会在读取到VMIN个字节或者VTIME时间后返回。

        return (tcsetattr (fd, TCSANOW, &opt));        //修改立即生效
}


unsigned int CRC_Test(unsigned char *ReceData,unsigned char RecLen)
{
    unsigned char uchCRCHi = 0xFF ; 		//
    unsigned char uchCRCLo = 0xFF ; 		//
    unsigned long uIndex ; 				//
    RecLen -= 2;             			//
    while (RecLen--) 				//
    {
        uIndex = uchCRCHi ^ *ReceData++ ; 	//
        uchCRCHi = uchCRCLo ^ auchCRCHi[uIndex] ;
        uchCRCLo = auchCRCLo[uIndex] ;
    }
    return (uchCRCHi << 8 | uchCRCLo) ;      	//
    //return (uchCRCLo << 8 | uchCRCHi) ;    	//
}

unsigned int CRC_Radar(unsigned char *puchMSG, unsigned int usDataLen)
{
    unsigned char uchCRCHi=0xFF;
    unsigned char uchCRCLo=0xFF;
    unsigned long uIndex;
    while(usDataLen--)
    {
    uIndex=uchCRCLo^*puchMSG++;
    uchCRCLo=uchCRCHi^auchCRCHi[uIndex];
    uchCRCHi=auchCRCLo[uIndex];
    }
    return (uchCRCHi<<8|uchCRCLo);
}
/*
void Init_Uart_SP0(void)
{
    //fd = open(DEV_NAME, O_RDWR | O_NOCTTY | O_NDELAY);
    fd_uart = open(DEV_NAME, O_RDWR | O_NOCTTY);
    ret_uart = set_port_attr (fd_uart,B9600,8,"1",'N',20,7);
}
*/

void Data_Handle(void)
{
    //区分检测类型下位机加一字节（暂定为下位机发来的第一个字节）；０１压力法；０２液媒法；０３传感器法；００其他检测方式
    //数据的解析整合
    switch(Test_Method)     //跟据ARM设置的检测方式，选择不同的数据处理程序，默认为传感器法03
    {
        case 0x00:          //其他方法
        {
            break;
        }
        case 0x01:          //压力法
        {
            Data_Handle_Pressure();
            break;
        }
        case 0x02:          //液媒法
        {
            Data_Handle_FF();
            break;
        }
        case 0x03:          //传感器法
        {
            Data_Handle_SF();
            break;
        }
        default:
            break;
    }
}



void Data_Handle_SF(void)
{
    //取出有效数据（含有“８”的未设置传感器为无效数据）（后续优化时更改）
    unsigned char i = 0;
    for(i = 0;i<16;i++)
    {
        //奇数
        switch( (Data_Buf_Sencor[i+1]>>4) & 0x0f)
        {
            case 0x00:
            {
                Data_Buf_Sencor_Convert[4*i] = 0xC0;
                Data_Buf_Sencor_Convert[4*i+1] = 0x00;
                break;
            }
            case 0x01:
            {
                Data_Buf_Sencor_Convert[4*i] = 0xC0;
                Data_Buf_Sencor_Convert[4*i+1] = 0x88;
                break;
            }
            case 0x02:
            {
                Data_Buf_Sencor_Convert[4*i] = 0xC0;
                Data_Buf_Sencor_Convert[4*i+1] = 0x90;
                break;
            }
            case 0x04:
            {
                Data_Buf_Sencor_Convert[4*i] = 0xC0;
                Data_Buf_Sencor_Convert[4*i+1] = 0x01;
                break;
            }
            case 0x08://如果未设置，默认改为0XFF,可能会修改
            {
                Data_Buf_Sencor_Convert[4*i] = 0xFF;
                Data_Buf_Sencor_Convert[4*i+1] = 0xFF;
                break;
            }
            case 0x0F:
            {
                Data_Buf_Sencor_Convert[4*i] = 0xC0;
                Data_Buf_Sencor_Convert[4*i+1] = 0x04;
                break;
            }
            default:
                break;
        }
        //偶数点
        switch( Data_Buf_Sencor[i+1] & 0x0f)
        {
            case 0x00:
            {
                Data_Buf_Sencor_Convert[4*i+2] = 0xC0;
                Data_Buf_Sencor_Convert[4*i+3] = 0x00;
                break;
            }
            case 0x01:
            {
                Data_Buf_Sencor_Convert[4*i+2] = 0xC0;
                Data_Buf_Sencor_Convert[4*i+3] = 0x88;
                break;
            }
            case 0x02:
            {
                Data_Buf_Sencor_Convert[4*i+2] = 0xC0;
                Data_Buf_Sencor_Convert[4*i+3] = 0x90;
                break;
            }
            case 0x04:
            {
                Data_Buf_Sencor_Convert[4*i+2] = 0xC0;
                Data_Buf_Sencor_Convert[4*i+3] = 0x01;
                break;
            }
            case 0x08://如果未设置，默认改为0XFF,可能会修改
            {
                Data_Buf_Sencor_Convert[4*i+2] = 0xFF;
                Data_Buf_Sencor_Convert[4*i+3] = 0xFF;
                break;
            }
            case 0x0F:
            {
                Data_Buf_Sencor_Convert[4*i+2] = 0xC0;
                Data_Buf_Sencor_Convert[4*i+3] = 0x04;
                break;
            }
            default:
                break;

        }
    }

    Data_Transfer();//数据传递到约定数组
/*
    for(uchar i = 0; i < 19; i++)           //DEBUG
    {                                       //DEBUG
        printf("%02x ",Data_Buf_Sencor[i]); //DEBUG       //12.27
    }                                       //DEBUG
    printf("\n");
*/
}



//液媒法检测也只是在油罐区域使用此数据，其余区域应与ＳＦ传感器法数据一致
void Data_Handle_FF(void)
{
    //取出有效数据（含有“８”的未设置传感器为无效数据）（后续优化时更改）
    unsigned char i = 0;
    for(i = 0;i<4;i++)
    {
        //奇数
        switch( (Data_Buf_Sencor[i+1]>>4) & 0x0f)
        {
            case 0x00:
            {
                Data_Buf_Sencor_Convert[4*i] = 0xC0;
                Data_Buf_Sencor_Convert[4*i+1] = 0x00;
                break;
            }
            case 0x01:
            {
                Data_Buf_Sencor_Convert[4*i] = 0xC0;
                Data_Buf_Sencor_Convert[4*i+1] = 0x88;
                break;
            }
            case 0x02:
            {
                Data_Buf_Sencor_Convert[4*i] = 0xC0;
                Data_Buf_Sencor_Convert[4*i+1] = 0x90;
                break;
            }
            case 0x04:
            {
                Data_Buf_Sencor_Convert[4*i] = 0xC0;
                Data_Buf_Sencor_Convert[4*i+1] = 0x01;
                break;
            }
            case 0x08://如果未设置，默认改为0XFF,可能会修改
            {
                Data_Buf_Sencor_Convert[4*i] = 0xFF;
                Data_Buf_Sencor_Convert[4*i+1] = 0xFF;
                break;
            }
            case 0x0F:
            {
                Data_Buf_Sencor_Convert[4*i] = 0xC0;
                Data_Buf_Sencor_Convert[4*i+1] = 0x04;
                break;
            }
            default:
                break;
        }
        //偶数点
        switch( Data_Buf_Sencor[i+1] & 0x0f)
        {
            case 0x00:
            {
                Data_Buf_Sencor_Convert[4*i+2] = 0xC0;
                Data_Buf_Sencor_Convert[4*i+3] = 0x00;
                break;
            }
            case 0x01:
            {
                Data_Buf_Sencor_Convert[4*i+2] = 0xC0;
                Data_Buf_Sencor_Convert[4*i+3] = 0x88;
                break;
            }
            case 0x02:
            {
                Data_Buf_Sencor_Convert[4*i+2] = 0xC0;
                Data_Buf_Sencor_Convert[4*i+3] = 0x90;
                break;
            }
            case 0x04:
            {
                Data_Buf_Sencor_Convert[4*i+2] = 0xC0;
                Data_Buf_Sencor_Convert[4*i+3] = 0x01;
                break;
            }
            case 0x08://如果未设置，默认改为0XFF,可能会修改
            {
                Data_Buf_Sencor_Convert[4*i+2] = 0xFF;
                Data_Buf_Sencor_Convert[4*i+3] = 0xFF;
                break;
            }
            case 0x0F:
            {
                Data_Buf_Sencor_Convert[4*i+2] = 0xC0;
                Data_Buf_Sencor_Convert[4*i+3] = 0x04;
                break;
            }
            default:
                break;

        }
    }
    //FF油罐传感器状态转换
    for(i = 4;i<8;i++)
    {
        //奇数点
        switch( (Data_Buf_Sencor[i+1]>>4) & 0x0f)
        {
            case 0x00:
            {
                Data_Buf_Sencor_Convert[4*i] = 0x80;
                Data_Buf_Sencor_Convert[4*i+1] = 0x00;
                break;
            }
            case 0x01:
            {
                Data_Buf_Sencor_Convert[4*i] = 0x80;
                Data_Buf_Sencor_Convert[4*i+1] = 0xA0;
                break;
            }
            case 0x02:
            {
                Data_Buf_Sencor_Convert[4*i] = 0x80;
                Data_Buf_Sencor_Convert[4*i+1] = 0xC0;
                break;
            }
            case 0x04:
            {
                Data_Buf_Sencor_Convert[4*i] = 0x80;
                Data_Buf_Sencor_Convert[4*i+1] = 0x01;
                break;
            }
            case 0x08://如果未设置，默认改为0XFF,可能会修改
            {
                Data_Buf_Sencor_Convert[4*i] = 0xFF;
                Data_Buf_Sencor_Convert[4*i+1] = 0xFF;
                break;
            }
            case 0x0F:
            {
                Data_Buf_Sencor_Convert[4*i] = 0x80;
                Data_Buf_Sencor_Convert[4*i+1] = 0x04;
                break;
            }
            default:
                break;
        }
        //偶数点
        switch( Data_Buf_Sencor[i+1] & 0x0f)
        {
            case 0x00:
            {
                Data_Buf_Sencor_Convert[4*i+2] = 0x80;
                Data_Buf_Sencor_Convert[4*i+3] = 0x00;
                break;
            }
            case 0x01:
            {
                Data_Buf_Sencor_Convert[4*i+2] = 0x80;
                Data_Buf_Sencor_Convert[4*i+3] = 0xA0;
                break;
            }
            case 0x02:
            {
                Data_Buf_Sencor_Convert[4*i+2] = 0x80;
                Data_Buf_Sencor_Convert[4*i+3] = 0xC0;
                break;
            }
            case 0x04:
            {
                Data_Buf_Sencor_Convert[4*i+2] = 0x80;
                Data_Buf_Sencor_Convert[4*i+3] = 0x01;
                break;
            }
            case 0x08://如果未设置，默认改为0XFF,可能会修改
            {
                Data_Buf_Sencor_Convert[4*i+2] = 0xFF;
                Data_Buf_Sencor_Convert[4*i+3] = 0xFF;
                break;
            }
            case 0x0F:
            {
                Data_Buf_Sencor_Convert[4*i+2] = 0x80;
                Data_Buf_Sencor_Convert[4*i+3] = 0x04;
                break;
            }
            default:
                break;

        }
    }

    for(i = 8;i<16;i++)
    {
        //奇数
        switch( (Data_Buf_Sencor[i+1]>>4) & 0x0f)
        {
            case 0x00:
            {
                Data_Buf_Sencor_Convert[4*i] = 0xC0;
                Data_Buf_Sencor_Convert[4*i+1] = 0x00;
                break;
            }
            case 0x01:
            {
                Data_Buf_Sencor_Convert[4*i] = 0xC0;
                Data_Buf_Sencor_Convert[4*i+1] = 0x88;
                break;
            }
            case 0x02:
            {
                Data_Buf_Sencor_Convert[4*i] = 0xC0;
                Data_Buf_Sencor_Convert[4*i+1] = 0x90;
                break;
            }
            case 0x04:
            {
                Data_Buf_Sencor_Convert[4*i] = 0xC0;
                Data_Buf_Sencor_Convert[4*i+1] = 0x01;
                break;
            }
            case 0x08://如果未设置，默认改为0XFF,可能会修改
            {
                Data_Buf_Sencor_Convert[4*i] = 0xFF;
                Data_Buf_Sencor_Convert[4*i+1] = 0xFF;
                break;
            }
            case 0x0F:
            {
                Data_Buf_Sencor_Convert[4*i] = 0xC0;
                Data_Buf_Sencor_Convert[4*i+1] = 0x04;
                break;
            }
            default:
                break;
        }
        //偶数点
        switch( Data_Buf_Sencor[i+1] & 0x0f)
        {
            case 0x00:
            {
                Data_Buf_Sencor_Convert[4*i+2] = 0xC0;
                Data_Buf_Sencor_Convert[4*i+3] = 0x00;
                break;
            }
            case 0x01:
            {
                Data_Buf_Sencor_Convert[4*i+2] = 0xC0;
                Data_Buf_Sencor_Convert[4*i+3] = 0x88;
                break;
            }
            case 0x02:
            {
                Data_Buf_Sencor_Convert[4*i+2] = 0xC0;
                Data_Buf_Sencor_Convert[4*i+3] = 0x90;
                break;
            }
            case 0x04:
            {
                Data_Buf_Sencor_Convert[4*i+2] = 0xC0;
                Data_Buf_Sencor_Convert[4*i+3] = 0x01;
                break;
            }
            case 0x08://如果未设置，默认改为0XFF,可能会修改
            {
                Data_Buf_Sencor_Convert[4*i+2] = 0xFF;
                Data_Buf_Sencor_Convert[4*i+3] = 0xFF;
                break;
            }
            case 0x0F:
            {
                Data_Buf_Sencor_Convert[4*i+2] = 0xC0;
                Data_Buf_Sencor_Convert[4*i+3] = 0x04;
                break;
            }
            default:
                break;

        }
    }

    Data_Transfer();//数据传递到约定数组
}


void Data_Handle_Pressure(void)
{
    //取出有效数据（含有“８”的未设置传感器为无效数据）（后续优化时更改）
    unsigned char i = 0;
    //０～４为油水区分型传感器
    for(i = 0;i<4;i++)
    {
        //奇数
        switch( (Data_Buf_Sencor[i+1]>>4) & 0x0f)
        {
            case 0x00:
            {
                Data_Buf_Sencor_Convert[4*i] = 0xC0;
                Data_Buf_Sencor_Convert[4*i+1] = 0x00;
                break;
            }
            case 0x01:
            {
                Data_Buf_Sencor_Convert[4*i] = 0xC0;
                Data_Buf_Sencor_Convert[4*i+1] = 0x88;
                break;
            }
            case 0x02:
            {
                Data_Buf_Sencor_Convert[4*i] = 0xC0;
                Data_Buf_Sencor_Convert[4*i+1] = 0x90;
                break;
            }
            case 0x04:
            {
                Data_Buf_Sencor_Convert[4*i] = 0xC0;
                Data_Buf_Sencor_Convert[4*i+1] = 0x01;
                break;
            }
            case 0x08://如果未设置，默认改为0XFF,可能会修改
            {
                Data_Buf_Sencor_Convert[4*i] = 0xFF;
                Data_Buf_Sencor_Convert[4*i+1] = 0xFF;
                break;
            }
            case 0x0F:
            {
                Data_Buf_Sencor_Convert[4*i] = 0xC0;
                Data_Buf_Sencor_Convert[4*i+1] = 0x04;
                break;
            }
            default:
                break;
        }
        //偶数点
        switch( Data_Buf_Sencor[i+1] & 0x0f)
        {
            case 0x00:
            {
                Data_Buf_Sencor_Convert[4*i+2] = 0xC0;
                Data_Buf_Sencor_Convert[4*i+3] = 0x00;
                break;
            }
            case 0x01:
            {
                Data_Buf_Sencor_Convert[4*i+2] = 0xC0;
                Data_Buf_Sencor_Convert[4*i+3] = 0x88;
                break;
            }
            case 0x02:
            {
                Data_Buf_Sencor_Convert[4*i+2] = 0xC0;
                Data_Buf_Sencor_Convert[4*i+3] = 0x90;
                break;
            }
            case 0x04:
            {
                Data_Buf_Sencor_Convert[4*i+2] = 0xC0;
                Data_Buf_Sencor_Convert[4*i+3] = 0x01;
                break;
            }
            case 0x08://如果未设置，默认改为0XFF,可能会修改
            {
                Data_Buf_Sencor_Convert[4*i+2] = 0xFF;
                Data_Buf_Sencor_Convert[4*i+3] = 0xFF;
                break;
            }
            case 0x0F:
            {
                Data_Buf_Sencor_Convert[4*i+2] = 0xC0;
                Data_Buf_Sencor_Convert[4*i+3] = 0x04;
                break;
            }
            default:
                break;

        }
    }
    //压力法油罐传感器状态转换
    for(i = 4;i<8;i++)
    {
        //奇数点
        switch( (Data_Buf_Sencor[i+1]>>4) & 0x0f)
        {
            case 0x00:
            {
                Data_Buf_Sencor_Convert[4*i] = 0x40;
                Data_Buf_Sencor_Convert[4*i+1] = 0x00;
                break;
            }
            case 0x02:
            {
                Data_Buf_Sencor_Convert[4*i] = 0x41;
                Data_Buf_Sencor_Convert[4*i+1] = 0x80;
                break;
            }
            case 0x01:
            {
                Data_Buf_Sencor_Convert[4*i] = 0x42;
                Data_Buf_Sencor_Convert[4*i+1] = 0x80;
                break;
            }
            case 0x04:
            {
                Data_Buf_Sencor_Convert[4*i] = 0x40;
                Data_Buf_Sencor_Convert[4*i+1] = 0x01;
                break;
            }
            case 0x08://如果未设置，默认改为0XFF,可能会修改
            {
                Data_Buf_Sencor_Convert[4*i] = 0xFF;
                Data_Buf_Sencor_Convert[4*i+1] = 0xFF;
                break;
            }
            case 0x0F:
            {
                Data_Buf_Sencor_Convert[4*i] = 0x40;
                Data_Buf_Sencor_Convert[4*i+1] = 0x04;
                break;
            }
            default:
                break;
        }
        //偶数点
        switch( Data_Buf_Sencor[i+1] & 0x0f)
        {
            case 0x00:
            {
                Data_Buf_Sencor_Convert[4*i+2] = 0x40;
                Data_Buf_Sencor_Convert[4*i+3] = 0x00;
                break;
            }
            case 0x02:
            {
                Data_Buf_Sencor_Convert[4*i+2] = 0x41;
                Data_Buf_Sencor_Convert[4*i+3] = 0x80;
                break;
            }
            case 0x01:
            {
                Data_Buf_Sencor_Convert[4*i+2] = 0x42;
                Data_Buf_Sencor_Convert[4*i+3] = 0x80;
                break;
            }
            case 0x04:
            {
                Data_Buf_Sencor_Convert[4*i+2] = 0x40;
                Data_Buf_Sencor_Convert[4*i+3] = 0x01;
                break;
            }
            case 0x08://如果未设置，默认改为0XFF,可能会修改
            {
                Data_Buf_Sencor_Convert[4*i+2] = 0xFF;
                Data_Buf_Sencor_Convert[4*i+3] = 0xFF;
                break;
            }
            case 0x0F:
            {
                Data_Buf_Sencor_Convert[4*i+2] = 0x40;
                Data_Buf_Sencor_Convert[4*i+3] = 0x04;
                break;
            }
            default:
                break;

        }
    }


    //８～１６为油水区分型传感器
    for(i = 8;i<16;i++)
    {
        //奇数
        switch( (Data_Buf_Sencor[i+1]>>4) & 0x0f)
        {
            case 0x00:
            {
                Data_Buf_Sencor_Convert[4*i] = 0xC0;
                Data_Buf_Sencor_Convert[4*i+1] = 0x00;
                break;
            }
            case 0x01:
            {
                Data_Buf_Sencor_Convert[4*i] = 0xC0;
                Data_Buf_Sencor_Convert[4*i+1] = 0x88;
                break;
            }
            case 0x02:
            {
                Data_Buf_Sencor_Convert[4*i] = 0xC0;
                Data_Buf_Sencor_Convert[4*i+1] = 0x90;
                break;
            }
            case 0x04:
            {
                Data_Buf_Sencor_Convert[4*i] = 0xC0;
                Data_Buf_Sencor_Convert[4*i+1] = 0x01;
                break;
            }
            case 0x08://如果未设置，默认改为0XFF,可能会修改
            {
                Data_Buf_Sencor_Convert[4*i] = 0xFF;
                Data_Buf_Sencor_Convert[4*i+1] = 0xFF;
                break;
            }
            case 0x0F:
            {
                Data_Buf_Sencor_Convert[4*i] = 0xC0;
                Data_Buf_Sencor_Convert[4*i+1] = 0x04;
                break;
            }
            default:
                break;
        }
        //偶数点
        switch( Data_Buf_Sencor[i+1] & 0x0f)
        {
            case 0x00:
            {
                Data_Buf_Sencor_Convert[4*i+2] = 0xC0;
                Data_Buf_Sencor_Convert[4*i+3] = 0x00;
                break;
            }
            case 0x01:
            {
                Data_Buf_Sencor_Convert[4*i+2] = 0xC0;
                Data_Buf_Sencor_Convert[4*i+3] = 0x88;
                break;
            }
            case 0x02:
            {
                Data_Buf_Sencor_Convert[4*i+2] = 0xC0;
                Data_Buf_Sencor_Convert[4*i+3] = 0x90;
                break;
            }
            case 0x04:
            {
                Data_Buf_Sencor_Convert[4*i+2] = 0xC0;
                Data_Buf_Sencor_Convert[4*i+3] = 0x01;
                break;
            }
            case 0x08://如果未设置，默认改为0XFF,可能会修改
            {
                Data_Buf_Sencor_Convert[4*i+2] = 0xFF;
                Data_Buf_Sencor_Convert[4*i+3] = 0xFF;
                break;
            }
            case 0x0F:
            {
                Data_Buf_Sencor_Convert[4*i+2] = 0xC0;
                Data_Buf_Sencor_Convert[4*i+3] = 0x04;
                break;
            }
            default:
                break;

        }
    }

    Data_Transfer();//数据传递到约定数组
    Data_Analysis();//计算压力法压力值

}


//数据传递函数
void Data_Transfer(void)
{
    unsigned char i , j , k , m;
    //传递转换后的管线到数组Data_Buf_Refuel，Data_Buf_Line，Data_Buf_Tank，Data_Buf_Poor

    for(i = 0;i<16;i++)
    {
        OIL_PIPE[i] = Data_Buf_Sencor_Convert[i];
    }
    for(j = 0;j<16;j++)
    {
        OIL_TANK[j] = Data_Buf_Sencor_Convert[j+16];
    }
    for(k = 0;k<16;k++)
    {
        OIL_BASIN[k] = Data_Buf_Sencor_Convert[k+32];
    }
    for(m = 0;m<16;m++)
    {
        OIL_DISPENER[m] = Data_Buf_Sencor_Convert[m+48];
    }
}
//压力数据计算+显示  //记录
void Data_Analysis(void)
{
    float pressure_pre = 0;

    for( unsigned char i = 0 ; i < 8 ; i++)
    {
        pressure_pre = Data_Buf_Sencor[i*2+20] *256 + Data_Buf_Sencor[i*2+1+20];
        count_Pressure[i] = 0 - (pressure_pre-410)*30.53/1000;   //计算出压力值



        //接下来的部分用来每5分段记录一次压力值,并防止波动造成重复记录

        //95-100
        if((count_Pressure[i] >= -100 && count_Pressure[i] < -95) && (pre_press[i] < -100 || pre_press[i] >=-95))
        {
            if(flag_pre20[i] == 0)
            {
                flag_pre20[i] = 1;
            }
        }

        //90-95
        if((count_Pressure[i] >= -95 && count_Pressure[i] < -90) && (pre_press[i] < -95 || pre_press[i] >=-90))
        {
            if(flag_pre19[i] == 0)
            {
                flag_pre19[i] = 1;
            }
        }




        //85-90
        if((count_Pressure[i] >= -90 && count_Pressure[i] < -85) && (pre_press[i] < -90 || pre_press[i] >=-85))
        {
            if(flag_pre1[i] == 0)
            {
                flag_pre1[i] = 1;
            }
        }
        //80-85
        if((count_Pressure[i] >= -85 && count_Pressure[i] < -80) && (pre_press[i] < -85 || pre_press[i] >=-80))
        {
            if(flag_pre2[i] == 0)
            {
                flag_pre2[i] = 1;
            }
        }
        //75-80
        if((count_Pressure[i] >= -80 && count_Pressure[i] < -75) && (pre_press[i] < -80 || pre_press[i] >=-75))
        {
            if(flag_pre3[i] == 0)
            {
                flag_pre3[i] = 1;
            }
        }
        //70-75
        if((count_Pressure[i] >= -75 && count_Pressure[i] < -70) && (pre_press[i] < -75 || pre_press[i] >=-70))
        {
            if(flag_pre4[i] == 0)
            {
                flag_pre4[i] = 1;
            }
        }
        //65-70
        if((count_Pressure[i] >= -70 && count_Pressure[i] < -65) && (pre_press[i] < -70 || pre_press[i] >=-65))
        {
            if(flag_pre5[i] == 0)
            {
                flag_pre5[i] = 1;
            }
        }
        //60-65
        if((count_Pressure[i] >= -65 && count_Pressure[i] < -60) && (pre_press[i] < -65 || pre_press[i] >=-60))
        {
            if(flag_pre6[i] == 0)
            {
                flag_pre6[i] = 1;
            }
        }
        //55-60
        if((count_Pressure[i] >= -60 && count_Pressure[i] < -55) && (pre_press[i] < -60 || pre_press[i] >=-55))
        {
            if(flag_pre7[i] == 0)
            {
                flag_pre7[i] = 1;
            }
        }
        //50-55
        if((count_Pressure[i] >= -55 && count_Pressure[i] < -50) && (pre_press[i] < -55 || pre_press[i] >=-50))
        {
            if(flag_pre8[i] == 0)
            {
                flag_pre8[i] = 1;
            }
        }
        //45-50
        if((count_Pressure[i] >= -50 && count_Pressure[i] < -45) && (pre_press[i] < -50 || pre_press[i] >=-45))
        {
            if(flag_pre9[i] == 0)
            {
                flag_pre9[i] = 1;
            }
        }
        //40-45
        if((count_Pressure[i] >= -45 && count_Pressure[i] < -40) && (pre_press[i] < -45 || pre_press[i] >=-40))
        {
            if(flag_pre10[i] == 0)
            {
                flag_pre10[i] = 1;
            }
        }
        //35-40
        if((count_Pressure[i] >= -40 && count_Pressure[i] < -35) && (pre_press[i] < -40 || pre_press[i] >=-35))
        {
            if(flag_pre11[i] == 0)
            {
                flag_pre11[i] = 1;
            }
        }

        //30-35
        if((count_Pressure[i] >= -35 && count_Pressure[i] < -30) && (pre_press[i] < -35 || pre_press[i] >=-30))
        {
            if(flag_pre12[i] == 0)
            {
                flag_pre12[i] = 1;
            }

        }

        //25-30
        if((count_Pressure[i] >= -30 && count_Pressure[i] < -25) && (pre_press[i] < -30 || pre_press[i] >=-25))
        {
            if(flag_pre13[i] == 0)
            {
                flag_pre13[i] = 1;
            }

        }
        //20-25
        if((count_Pressure[i] >= -25 && count_Pressure[i] < -20) && (pre_press[i] < -25 || pre_press[i] >=-20))
        {
            if(flag_pre14[i] == 0)
            {
                flag_pre14[i] = 1;
            }

        }
        //15-20
        if((count_Pressure[i] >= -20 && count_Pressure[i] < -15) && (pre_press[i] < -20 || pre_press[i] >=-15))
        {
            if(flag_pre15[i] == 0)
            {
                flag_pre15[i] = 1;
            }

        }
        //10-15
        if((count_Pressure[i] >= -15 && count_Pressure[i] < -10) && (pre_press[i] < -15 || pre_press[i] >=-10))
        {
            if(flag_pre16[i] == 0)
            {
                flag_pre16[i] = 1;
            }

        }
        //5-10
        if((count_Pressure[i] >= -10 && count_Pressure[i] < -5) && (pre_press[i] < -10 || pre_press[i] >=-5))
        {
            if(flag_pre17[i] == 0)
            {
                flag_pre17[i] = 1;
            }

        }
        //0 - 5
        if((count_Pressure[i] >= -5 && count_Pressure[i] < 0) && (pre_press[i] < -5 || pre_press[i] >=0))
        {
            if(flag_pre18[i] == 0)
            {
                flag_pre18[i] = 1;
            }
        }
//避免重复计数
        if     (  (count_Pressure[i] > -99.2 && count_Pressure[i] < -95.8) || (count_Pressure[i] > -94.2 && count_Pressure[i] < -90.8)
                || (count_Pressure[i] > -89.2 && count_Pressure[i] < -85.8) || (count_Pressure[i] > -84.2 && count_Pressure[i] < -80.8)
                || (count_Pressure[i] > -79.2 && count_Pressure[i] < -75.8) || (count_Pressure[i] > -74.2 && count_Pressure[i] < -70.8)
                || (count_Pressure[i] > -69.2 && count_Pressure[i] < -65.8) || (count_Pressure[i] > -64.2 && count_Pressure[i] < -60.8)
                || (count_Pressure[i] > -59.2 && count_Pressure[i] < -55.8) || (count_Pressure[i] > -54.2 && count_Pressure[i] < -50.8)
                || (count_Pressure[i] > -49.2 && count_Pressure[i] < -45.8) || (count_Pressure[i] > -44.2 && count_Pressure[i] < -40.8)
                || (count_Pressure[i] > -39.2 && count_Pressure[i] < -35.8) || (count_Pressure[i] > -34.2 && count_Pressure[i] < -30.8)
                || (count_Pressure[i] > -29.2 && count_Pressure[i] < -25.8) || (count_Pressure[i] > -24.2 && count_Pressure[i] < -20.8)
                || (count_Pressure[i] > -19.2 && count_Pressure[i] < -15.8) || (count_Pressure[i] > -14.2 && count_Pressure[i] < -10.8)
                || (count_Pressure[i] > -9.2 && count_Pressure[i] < -5.8)     || (count_Pressure[i] > -4.2   && count_Pressure[i] < -0.8)
               )
        {
            flag_pre1[i] = 0;flag_pre2[i] = 0;flag_pre3[i] = 0;flag_pre4[i] = 0; flag_pre5[i] = 0;
            flag_pre6[i] = 0;flag_pre7[i] = 0;flag_pre8[i] = 0;flag_pre9[i] = 0;flag_pre10[i] = 0;
            flag_pre11[i] = 0;flag_pre12[i] = 0;flag_pre13[i] = 0;flag_pre14[i] = 0;flag_pre15[i] = 0;
            flag_pre16[i] = 0;flag_pre17[i] = 0;flag_pre18[i] = 0;flag_pre19[i] = 0;flag_pre20[i] = 0;
        }

        pre_press[i] = count_Pressure[i];
    }

//    for(uchar i = 0 ; i < 8 ; i ++)
//    {
//     printf("  %02f   ",count_Pressure[i])   ;
//    }
}







