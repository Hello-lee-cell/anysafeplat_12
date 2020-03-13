#include "uartthread.h"
#include "mainwindow.h"
#include <QtDebug>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <asm/termios.h>
#include <signal.h>
#include <sys/time.h>
#include <QTimer>
#include<serial.h>
#include<sys/shm.h>
#include<sys/ipc.h>
#include<errno.h>
#include<sys/shm.h>

#include"systemset.h"
#include"mythread.h"
#include"config.h"      //OIL_BASIN[]   PIPE[]  TANK[]  DISPENER[]
//#include"file_op.h"
#include"io_op.h"
#include"radar_485.h"
#include"database_op.h"
//unsigned char Flag_Sound_Radar[5] = {0};
//unsigned char count_radar_uart = 0; //雷达通信故障计数

uartthread::uartthread(QObject *parent):
    QThread(parent)
{
   // stop = false;
}

void uartthread::run()
{
    ModbusInit();

    sleep(1);
    while(1)
    {
        msleep(450);
        SendDataRadar();
        msleep(250);
        ReadDataRadar();
    }
}

void uartthread::SendDataRadar()
{
    if(!Flag_area_ctrl[0])
    {
        Alarm_Re_Flag[Communication_Machine] = 0;
        Flag_Sound_Radar[0] = 0;
        Flag_Sound_Radar[1] = 0;
        Flag_Sound_Radar[2] = 0;
        return;
    }

    switch(Flag_Set_SendMode)
    {
        case 1: Send_MODE[Communication_Machine] = 0;   //阈值采集
                Flag_Set_SendMode = 0;
                break;
        case 2: Send_MODE[Communication_Machine] = 31;    //双路/一路设置
                Flag_Set_SendMode = 0;
                break;
        case 3:
                Send_MODE[Communication_Machine] = 36;  //取消自动区域设置
                Flag_Set_SendMode = 0;
                break;
        case 4:
                Send_MODE[Communication_Machine] = 13; //阈值更新
                Flag_Set_SendMode = 0;
                break;
        case 5:
                Send_MODE[Communication_Machine] = 37;
                Flag_Set_SendMode = 0;
                break;
        case 6:
                Send_MODE[Communication_Machine] = 11;      //？？？
                Flag_Set_SendMode = 0;
                break;
        case 7:
                Send_MODE[Communication_Machine] = 43;
                Flag_Set_SendMode = 0;
                break;
    }

    if((Send_MODE[Communication_Machine] == 11)&& (Flag_Set_yuzhi == 1))
    {
        Flag_Set_yuzhi = 0;
        emit repaint_set_yuzhi();
    }

    Send_Command(Send_MODE[Communication_Machine]);



    msleep(10);      //？该值不确定，需要确认
    gpio_11_high();
}
void uartthread::ReadDataRadar()
{
    if(!Flag_area_ctrl[0])
    {
        Alarm_Re_Flag[Communication_Machine] = 0;
        Flag_Sound_Radar[0] = 0;
        Flag_Sound_Radar[1] = 0;
        Flag_Sound_Radar[2] = 0;
        return;
    }
    unsigned char recvbuf[200];
    len_uart = read(fd_uart_radar, recvbuf, 200);        //读取串口发来的数据
    if (len_uart <= 0)
    {
        Alarm_Re_Flag[Communication_Machine] = 0;
        //雷达通信故障

        if(count_radar_uart > 60)
        {
            Flag_Sound_Radar[0] = 1;
            if(count_radar_uart == 61)
            {
                add_value_radarinfo("1# 通信故障","");
                emit warn_to_mainwindowstatelabel();
            }
            count_radar_uart = 62;
        }
        Flag_Sound_Radar[1] = 0;
        Flag_Sound_Radar[2] = 0;
        count_radar_uart++;
        return;
    }
    if(count_radar_uart > 60)
    {
        emit warn_to_mainwindowstatelabel();
    }

    Flag_Sound_Radar[0] = 0;
//    for(uchar i = 0; i < len_uart; i++)
//    {
//        printf("%02x ",recvbuf[i]);

//    }
//    printf("\n");
    CommunicationProcess(recvbuf,len_uart);
}










