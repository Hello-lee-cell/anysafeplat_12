#include "mainwindow.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <asm/termios.h>
#include <signal.h>
#include <sys/time.h>
#include <QTimer>
#include<serial.h>
#include<config.h>
#include"systemset.h"
#include<sys/shm.h>
#include<sys/ipc.h>
#include<errno.h>
#define MY_SHM_ID 60
#define MY_SHM_TANK 70
#define MY_SHM_PIPE 80
#define MY_SHM_DISPENER 90
#define MY_SHM_BASIN 100

//加入设置下位机点数
unsigned char Ask_Pic[10] = {0xA0,0xF5,0x01,0xff,0xff,0xff,0xff,0x0A,0x3b,0x6a};//询问pic当前传感器状态
unsigned char Set_Pic[10] = {0xA0,0xF5,0x02,0x00,0x00,0x00,0x00,0x0A,0x3b,0x6a};//设置传感器数量
unsigned char Set_Pic_Rec_Flag = 0;						//PIC返回10字节数据时置1，代表设置成功
unsigned char onlyonce = 1;
void Sencor_Handle(int signo)
{
    //判断设置状态，如果ARM更新了设置，则发送设置PIC数据
    unsigned int CRC = 0x0000;
    int shmid;                          //内存共享所需变量
    void *mem;                          //内存共享
    shmid = shmget(MY_SHM_ID,1,IPC_CREAT);
    mem = shmat(shmid,(const void*)0,0);
    if(onlyonce)
    {
        Set_Pic[3] = count_tank;
        Set_Pic[4] = count_pipe;
        Set_Pic[5] = count_dispener;
        Set_Pic[6] = count_basin;
        CRC = CRC_Test(Set_Pic,10);
        Set_Pic[8] = ( (CRC & 0xFF00) >> 8);
        Set_Pic[9] = (CRC & 0x00FF);
        len_uart = write(fd_uart, Set_Pic, 10);
        onlyonce = 0;
    }

    if(strcmp((char *)mem,"1") == 0)
    {
        int shm_basin;
        void *mem_basin;
        shm_basin = shmget(MY_SHM_BASIN,1,IPC_CREAT);
        mem_basin = shmat(shm_basin,(const void*)0,0);
        sscanf((char *)mem_basin,"%d",&count_basin);
        shmdt(mem_basin);


        int shm_tank;
        void *mem_tank;
        shm_tank = shmget(MY_SHM_TANK,1,IPC_CREAT);
        mem_tank = shmat(shm_tank,(const void*)0,0);
        sscanf((char *)mem_tank,"%d",&count_tank);
        shmdt(mem_tank);

        int shm_pipe;
        void *mem_pipe;
        shm_pipe = shmget(MY_SHM_PIPE,1,IPC_CREAT);
        mem_pipe = shmat(shm_pipe,(const void*)0,0);
        sscanf((char *)mem_pipe,"%d",&count_pipe);
        shmdt(mem_pipe);

        int shm_dispener;
        void *mem_dispener;
        shm_dispener = shmget(MY_SHM_DISPENER,1,IPC_CREAT);
        mem_dispener = shmat(shm_dispener,(const void*)0,0);
        sscanf((char *)mem_dispener,"%d",&count_dispener);
        shmdt(mem_dispener);

        strcpy((char *)mem,"0");        //清零标志位
        shmdt(mem);                     //清零标志位
        Set_Pic[3] = count_tank;
        Set_Pic[4] = count_pipe;
        Set_Pic[5] = count_dispener;
        Set_Pic[6] = count_basin;
        CRC = CRC_Test(Set_Pic,10);
        Set_Pic[8] = ( (CRC & 0xFF00) >> 8);
        Set_Pic[9] = (CRC & 0x00FF);
        len_uart = write(fd_uart, Set_Pic, 10);		//5s设置一次传感器数目，如果PIC未应答，则继续发，连续5次无返回报通信故障（未加）
        //DEBUG 打印出设置PIC单片机的数据
        for(uchar i = 0; i < 10; i++)               //DEBUG
        {                                           //DEBUG
            printf("%02x ",Set_Pic[i]);             //DEBUG
        }                                           //DEBUG
        printf("\n");                               //DEBUG
    }
    else
    {
        len_uart = write(fd_uart, Ask_Pic, 10);		//5s询问一次传感器状态
        strcpy((char *)mem,"0");        //清零标志位
        shmdt(mem);
        //debug
        for(uchar i = 0; i < 10; i++)
        {
            printf("%02x ",Ask_Pic[i]);
        }
    }


    if (len_uart < 0)
    {
        printf("write data error \n");
    }
    printf("Ask pic & Wait data return.\n");
    alarm(1);
}



//全功能DEBUG
void* uart_main(void*)
{
    Init_Uart_SP0();    //初始化串口SP0,波特率9600，阻塞
    if(!fork())
    {
        signal(SIGALRM,(__sighandler_t)Sencor_Handle);  //定时询问下位机
        alarm(1);                                       //定时5s
    }
    while(1)
    {
        len_uart = read(fd_uart, Data_Buf_Sencor, sizeof(Data_Buf_Sencor));        //读取串口发来的数据

        if (len_uart <= 0)
        {
            printf("read error \n");
            //return -1;
        }

        if(len_uart == 19)                          //如果接收到的数据长度＝２０，则进行数据处理，ＣＲＣ校验调试阶段不开
        {
            printf("copy that 19.\n");              //DEBUG

            for(uchar i = 0; i < 19; i++)           //DEBUG
            {                                       //DEBUG
                printf("%02x ",Data_Buf_Sencor[i]); //DEBUG
            }                                       //DEBUG
            printf("\n");                           //DEBUG
            len_uart = 0;                           //DEBUG
            Data_Handle();                          //数据处理与打包更新ｂｕｆ
        }
        else if(len_uart == 10)//判断接收的数据是否是接收到设置数据后的应答数据，如果是，则代表下位机接收到设置数据(接收数据未校验)
        {
            Set_Pic_Rec_Flag = 1;

        }
/*
        printf("%02x ",Data_Buf_Sencor_Convert[0]);
        printf("%02x \n",Data_Buf_Sencor_Convert[1]);

        printf("%02x ",Data_Buf_Sencor_Convert[16]);
        printf("%02x \n",Data_Buf_Sencor_Convert[17]);

        printf("%02x ",Data_Buf_Sencor_Convert[32]);
        printf("%02x \n",Data_Buf_Sencor_Convert[33]);

        printf("%02x ",Data_Buf_Sencor_Convert[48]);
        printf("%02x \n",Data_Buf_Sencor_Convert[49]);
*/
    }

}


