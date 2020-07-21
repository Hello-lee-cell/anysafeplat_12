#include <QApplication>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
//#include <asm/termios.h>
#include <termios.h>  //电脑端运行
#include <signal.h>
#include <sys/time.h>
#include <QTimer>
#include<QTime>
#include <serial.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <errno.h>
#include <QMutex>
#include <QtDebug>
#include <sys/shm.h>
#include "config.h"
#include "mainwindow.h"

#include "ywythread.h"
#include"config.h"
#include"file_op.h"
#include"database_op.h"

int fd_uart_ywy;
int len_uart_ywy, ret_uart_ywy;

QMutex Recv_Data_Lock;   //互斥锁

unsigned char Ask_Tanggan[30] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0X00,0x00,0x00,0x00};//询问探杆液位
unsigned char Tanggan_ADD[30] = {0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0XD9,0xDA,0x00};
unsigned char i_Tanggan_ADD = 0;//问询探杆地址数组指示
unsigned char i_Ask_Tanggan = 0;//问询次数指示
unsigned char Flag_Communicate_YWY_Error[20]= {0};
unsigned char Flag_carry_the_produce = 1;
unsigned char Flag_alarm_off_on = 0;
unsigned char i_alarm_record[12][6] = {0};  //报警次数记录  i 探杆 j 报警代码
unsigned char Flag_Connect_Wrong = 0;   //如果探杆一直不回
unsigned char Uart_Channel = 1;           //轮寻液位仪参数  2:上电先问量程
struct Display_HeightData Dis_HeightData;
struct Tangan_Configuration Tangan_Config;
unsigned char len_Tangan_Config_CompOIL;
unsigned char len_Tangan_Config_CompWater;
unsigned char Tanggan_SET_ADD;

float add_Oil_array[10][3] = {0};
//存文件
unsigned char Amount_OilTank;
unsigned char Tangan_Amount[12] = {1,1,1,1,1,1,1,1,1,1,1};    //每个油罐探杆数目
unsigned char sum_Tangan_Amount = 0;
float OilTank_Set[12][6];

unsigned char Oil_Kind[12][9] = {0};  //油品
float Compension[12][2] = {0};

float Oil_Tank_Table[12][300] = {0};  //罐表
unsigned char index_Oil_Tank_Table = 0;

ywythread::ywythread(QObject *parent):
    QThread(parent)
{

}

void ywythread:: Data_Handle_YWY()
{
    int i,j,k,postA;
    if(Data_Buf_Sencor[0]==Ask_Tanggan[0])  //读取
    {

        switch (Data_Buf_Sencor[1])
        {
            case ASK_average:
                                Flag_Communicate_YWY_Error[i_Tanggan_ADD] = 0;
                                if(i_alarm_record[i_Tanggan_ADD][0])
                                {
                                   emit Send_alarm_info(Tanggan_ADD[i_Tanggan_ADD],6);
                                    i_alarm_record[i_Tanggan_ADD][0] = 0;
                                    add_yeweiyi_alarminfo(QString("%1 #罐").arg(i_Tanggan_ADD),"通讯正常");
                                }

                                memset(Dis_HeightData.OIL_Height,0,6);
                                memset(Dis_HeightData.Water_Height,0,6);
                                memset(Dis_HeightData.TEMP_Height,0,6);
                                for(i=0;i<6;i++)
                                {
                                    if(Data_Buf_Sencor[i+3] !=0x3A)
                                    {
                                       Dis_HeightData.OIL_Height[i] = Data_Buf_Sencor[i+3];
                                       postA = 10;
                                    }
                                    else
                                    {
                                        postA = i+4;
                                        i=7;
                                    }
                                }

                                for(j=0;j<6;j++)
                                {
                                    if(Data_Buf_Sencor[postA] != 0x3A)
                                    {
                                        Dis_HeightData.Water_Height[j] = Data_Buf_Sencor[postA++];
                                    }
                                    else
                                    {
                                       j=7;
                                    }
                                }
                                postA++;            //需要 +1
                                for(k=0;k<6;k++)
                                {
                                    if(Data_Buf_Sencor[postA] != ETX)
                                    {
                                        Dis_HeightData.TEMP_Height[k] = Data_Buf_Sencor[postA++];
                                    }
                                    else
                                    {
                                        k=7;
                                    }
                                }

                                    Dis_HeightData.strOIL_Height     = QString((Dis_HeightData.OIL_Height));
                                    Dis_HeightData.strWater_Height   = QString((Dis_HeightData.Water_Height));
                                    Dis_HeightData.strTEMP    = QString((Dis_HeightData.TEMP_Height));

                                    //故障判断  报警判断   优先级 浮子故障 > 液位报警 >温度传感器故障
                                    if(Flag_alarm_off_on)
                                    {
                                        if(Dis_HeightData.strTEMP == "85.0")
                                        {
                                            emit Send_alarm_info(Ask_Tanggan[0],4);
                                            i_alarm_record[i_Tanggan_ADD][4] = 1;
                                            add_yeweiyi_alarminfo(QString("%1 #罐").arg(i_Tanggan_ADD),"温度传感器故障");
                                        }
                                        else
                                        {
                                            if(i_alarm_record[i_Tanggan_ADD][4])
                                            {
                                                i_alarm_record[i_Tanggan_ADD][4] = 0;
                                                emit Send_alarm_info(Tanggan_ADD[i_Tanggan_ADD],6);
                                                add_yeweiyi_alarminfo(QString("%1 #罐").arg(i_Tanggan_ADD),"温度传感器正常");
                                            }
                                        }

                                        if(Dis_HeightData.strOIL_Height.toFloat()   > OilTank_Set[i_Tanggan_ADD-1][3])
                                        {
                                            if(i_alarm_record[i_Tanggan_ADD][1] == 0)
                                            {
                                                emit Send_alarm_info(Ask_Tanggan[0],1);
                                                i_alarm_record[i_Tanggan_ADD][1] = 1;
                                                add_yeweiyi_alarminfo(QString("%1 #罐").arg(i_Tanggan_ADD),"油位过高");
                                            }
                                        }
                                        else if(Dis_HeightData.strOIL_Height.toFloat()   < OilTank_Set[i_Tanggan_ADD-1][2])
                                        {
                                            if(i_alarm_record[i_Tanggan_ADD][2] == 0)
                                            {
                                               emit  Send_alarm_info(Ask_Tanggan[0],2);
                                                i_alarm_record[i_Tanggan_ADD][2] = 1;
                                                add_yeweiyi_alarminfo(QString("%1 #罐").arg(i_Tanggan_ADD),"油位过低");
                                            }
                                        }
                                        else
                                        {
                                            if(i_alarm_record[i_Tanggan_ADD][1] || i_alarm_record[i_Tanggan_ADD][2])
                                            {
                                               i_alarm_record[i_Tanggan_ADD][1] = 0;
                                               i_alarm_record[i_Tanggan_ADD][2] = 0;
                                               add_yeweiyi_alarminfo(QString("%1 #罐").arg(i_Tanggan_ADD),"油位正常");
                                               emit Send_alarm_info(Tanggan_ADD[i_Tanggan_ADD],6);
                                            }
                                        }

                                        if(Dis_HeightData.strWater_Height.toFloat() > OilTank_Set[i_Tanggan_ADD-1][1])
                                        {
                                            if(i_alarm_record[i_Tanggan_ADD][5] == 0)
                                            {
                                              emit Send_alarm_info(Ask_Tanggan[0],5);
                                              i_alarm_record[i_Tanggan_ADD][5] = 1;
                                              add_yeweiyi_alarminfo(QString("%1 #罐").arg(i_Tanggan_ADD),"水位过高");
                                            }
                                        }
                                        else
                                        {
                                            if(i_alarm_record[i_Tanggan_ADD][5])
                                            {
                                                i_alarm_record[i_Tanggan_ADD][5] = 0;
                                                add_yeweiyi_alarminfo(QString("%1 #罐").arg(i_Tanggan_ADD),"水位正常");
                                                emit Send_alarm_info(Tanggan_ADD[i_Tanggan_ADD],6);
                                            }
                                        }

                                        if(Dis_HeightData.strOIL_Height == "E505" || Dis_HeightData.strWater_Height == "E505")
                                        {
                                            if(i_alarm_record[i_Tanggan_ADD][3] == 0)
                                            {
                                                emit Send_alarm_info(Ask_Tanggan[0],3);
                                                i_alarm_record[i_Tanggan_ADD][3] = 1;
                                                add_yeweiyi_alarminfo(QString("%1 #罐").arg(i_Tanggan_ADD),"浮子故障");
                                            }
                                        }
                                        else
                                        {
                                            if(i_alarm_record[i_Tanggan_ADD][3])
                                            {
                                               i_alarm_record[i_Tanggan_ADD][3] = 0;
                                               add_yeweiyi_alarminfo(QString("%1 #罐").arg(i_Tanggan_ADD),"浮子正常");
                                                emit Send_alarm_info(Tanggan_ADD[i_Tanggan_ADD],6);
                                            }
                                        }
                                     }


//                                    printf("%02d:%02d:%02d",QTime::currentTime().hour(),QTime::currentTime().minute(),QTime::currentTime().second());fflush(stdout);printf("\n");
//                                    printf("OIL Height  : %s",Dis_HeightData.strOIL_Height.toStdString().data());      fflush(stdout);printf("\n");
//                                    printf("Water Height: %s",Dis_HeightData.strWater_Height.toStdString().data());    fflush(stdout);printf("\n");
//                                    printf("Temperature : %s",Dis_HeightData.strTEMP.toStdString().data());            fflush(stdout);printf("\n");

                                    emit Send_Height_Signal(Ask_Tanggan[0],Dis_HeightData.strOIL_Height,Dis_HeightData.strWater_Height,Dis_HeightData.strTEMP);
                                    break;
            case ASK_Range_KB:
                                    memset(Dis_HeightData.Height_Range,0,6);
                                    for(i=0;i<20;i++)
                                    {
                                        if(Data_Buf_Sencor[i] ==0x3A)
                                        {
                                           postA = i+1;
                                           i=21;
                                        }
                                    }
                                    for(j=0;j<6;j++)
                                    {
                                        if(Data_Buf_Sencor[postA] != ETX)
                                        {
                                            Dis_HeightData.Height_Range[j] = Data_Buf_Sencor[postA++];
                                        }
                                        else
                                        {
                                            j=8;
                                        }
                                    }
                                    Dis_HeightData.strRange = QString((Dis_HeightData.Height_Range));

                                    printf("Range : %s",Dis_HeightData.strRange.toStdString().data());            fflush(stdout);printf("\n");

                                    Uart_Channel = 3;
                                    break;
            case ASK_OIL_compensation:
                                    memset(Dis_HeightData.OIL_compensation,0,6);
                                    for(i=0;i<6;i++)
                                    {
                                        if(Data_Buf_Sencor[i+3] != ETX)
                                        {
                                           Dis_HeightData.OIL_compensation[i] = Data_Buf_Sencor[i+3];
                                        }
                                        else
                                        {
                                            i=7;
                                        }
                                    }

                                    Compension[i_Tanggan_ADD][0]  = QString((Dis_HeightData.OIL_compensation)).toFloat();
                                    emit Send_compensation_Signal(ASK_OIL_compensation,i_Tanggan_ADD, Compension[i_Tanggan_ADD][0]);

                                    printf("OIL_compensation : %f",QString((Dis_HeightData.OIL_compensation)).toFloat());            fflush(stdout);printf("\n");

                                    emit compensation_set_result(ASK_OIL_compensation,i_Tanggan_ADD,"成功");
                                    if((i_Tanggan_ADD < sum_Tangan_Amount))
                                    {
                                        i_Tanggan_ADD++;
                                    }
                                    else
                                    {
                                        Uart_Channel = 4;
                                        i_Tanggan_ADD = 1;
                                    }
                                    i_Ask_Tanggan = 0;

                                    break;
            case ASK_Water_compensation:
                                    memset(Dis_HeightData.Water_compensation,0,6);
                                    for(i=0;i<6;i++)
                                    {
                                        if(Data_Buf_Sencor[i+3] != ETX)
                                        {
                                           Dis_HeightData.Water_compensation[i] = Data_Buf_Sencor[i+3];
                                        }
                                        else
                                        {
                                            i=7;
                                        }
                                    }
                                    Compension[i_Tanggan_ADD][1] = QString((Dis_HeightData.Water_compensation)).toFloat();
                                    emit Send_compensation_Signal(ASK_Water_compensation,i_Tanggan_ADD, Compension[i_Tanggan_ADD][1]);

                                    printf("Water_compensation : %f",Compension[i_Tanggan_ADD][1]);            fflush(stdout);printf("\n");


                                    emit compensation_set_result(ASK_Water_compensation,i_Tanggan_ADD,"成功");
                                    if((i_Tanggan_ADD < sum_Tangan_Amount))
                                    {
                                        i_Tanggan_ADD++;
                                    }
                                    else
                                    {
                                        emit compensation_set_result(ASK_Water_compensation ,0,"完成问询");
                                        Uart_Channel = 1;
                                        i_Tanggan_ADD = 0;
                                    }
                                    i_Ask_Tanggan = 0;

                                    break;
        /************************************** 设置 ********************************************/
            case Write_OIL_compensation:
                                    Uart_Channel = 12;
                                    i_Ask_Tanggan = 0;
                                    emit compensation_set_result(Write_OIL_compensation,i_Tanggan_ADD,"成功");
                                    printf("Receive write OIL compensation ADD data %02x success \n",Ask_Tanggan[0]);fflush(stdout);
                                    break;
            case Write_Water_compensation:
                                    Uart_Channel = 14;
                                    i_Ask_Tanggan = 0;
                                    emit compensation_set_result(Write_Water_compensation,i_Tanggan_ADD,"成功");
                                    printf("Receive write Water compensation ADD data %02x success \n",Ask_Tanggan[0]);fflush(stdout);
                                    break;
            default:
                printf(("default read \n"));fflush(stdout);
                break;
        }

        memset(Ask_Tanggan,0,6);

    }
    else if(Data_Buf_Sencor[0]== STX) //设置补偿值返回
    {
        if(Uart_Channel == 12)   //油补偿设置返回 对比
        {
            for(i=0;i<len_Tangan_Config_CompOIL;i++)
            {
                if(Data_Buf_Sencor[i+1] != Ask_Tanggan[1+i])
                {
                    i = len_Tangan_Config_CompOIL + 5;   //如果值不对即刻退出
                }
            }
            if(i==len_Tangan_Config_CompOIL)
            {
                emit compensation_set_result(Write_OIL_compensation + STX,i_Tanggan_ADD,"成功");
                if((i_Tanggan_ADD < sum_Tangan_Amount))
                {
                    i_Tanggan_ADD++;
                    Uart_Channel = 11;
                }
                else
                {
                    Uart_Channel = 13;
                    i_Tanggan_ADD = 1;
                }
                i_Ask_Tanggan = 0;
            }
            else
            {
                emit compensation_set_result(Write_OIL_compensation + STX,i_Tanggan_ADD,"失败");
            }
        }
        if(Uart_Channel == 14)   //水补偿设置返回 对比
        {
            for(i=0;i<len_Tangan_Config_CompWater;i++)
            {
                if(Data_Buf_Sencor[i+1] != Ask_Tanggan[1+i])
                {
                    i = len_Tangan_Config_CompWater + 5;  //如果值不对即刻退出
                }
            }
            if(i==len_Tangan_Config_CompWater)
            {
                emit compensation_set_result(Write_Water_compensation + STX,i_Tanggan_ADD,"成功");
                if((i_Tanggan_ADD < sum_Tangan_Amount))
                {
                    i_Tanggan_ADD++;
                    Uart_Channel = 13;
                }
                else
                {
                    Uart_Channel = 1;
                    i_Tanggan_ADD = 0;
                    emit compensation_set_result(Write_Water_compensation ,0,"完成问询");
                }
                i_Ask_Tanggan = 0;
            }
            else
            {
               emit compensation_set_result(Write_Water_compensation + STX,i_Tanggan_ADD,"失败");
            }
        }
        memset(Ask_Tanggan,0,6);
    }
    else if(Data_Buf_Sencor[0]==Ask_Tanggan[3] && Data_Buf_Sencor[1] == Ask_Tanggan[3])   //设置地址
    {
            Uart_Channel = 1;
            printf("write yeweiyi ADD data %02x success \n",Ask_Tanggan[3]);fflush(stdout);
            emit Set_tangan_add_success(Ask_Tanggan[3]);
            memset(Ask_Tanggan,0,6);
    }

    memset(Data_Buf_Sencor,0,sizeof(Data_Buf_Sencor));
}

void ywythread:: Recving_Handle_YWY()
{
    memset(Data_Buf_Sencor,0xFF,sizeof(Data_Buf_Sencor));      //add by G
    Recv_Data_Lock.lock();  //锁
    len_uart_ywy = read(fd_uart_ywy, Data_Buf_Sencor, sizeof(Data_Buf_Sencor));      //读取下位机返回来的数据
    Recv_Data_Lock.unlock();    //解锁
    if (len_uart_ywy < 0)
    {
        qDebug("read data failed \n");
    }
//    for(int i = 0;i<len_uart_ywy;i++)
//    {
//        printf("%02x ",Data_Buf_Sencor[i]);
//        fflush(stdout);
//    }
//    printf("\n");
    Recv_Data_Lock.lock();     //锁
    Data_Handle_YWY();             //数据处理与打包更新buff
    Recv_Data_Lock.unlock();   //解锁
}

void ywythread:: Asking_Handle_YWY()
{
    char *setvalue;
    QString str;
    QByteArray Q_setvalue;

    if(Uart_Channel == 1)      //实时问液位
    {
        if((i_Tanggan_ADD < sum_Tangan_Amount))
        {
            i_Tanggan_ADD++;
        }
        else
        {
           i_Tanggan_ADD = 1;
           i_Ask_Tanggan++;   //问询一轮结束
           if(i_Ask_Tanggan>2)
           {
               i_Ask_Tanggan = 0;
               Uart_Channel = 1;
               for(unsigned char i=0;i<=Amount_OilTank;i++)
               {
                   if(Flag_Communicate_YWY_Error[i] >= 3)
                   {
                       printf("%d Tangan Flag_Communicate_YWY_Error! %d\n",i,Flag_Communicate_YWY_Error[i]);fflush(stdout);
                       Flag_Communicate_YWY_Error[i] = 0;
                       if(i_alarm_record[i][0] == 0)
                       {
                           emit Send_alarm_info(Tanggan_ADD[i],0);
                           i_alarm_record[i][0] = 1;
                           add_yeweiyi_alarminfo(QString("%1 #罐").arg(i),"通讯故障");
                       }
                   }
               }
           }
        }
        Flag_Communicate_YWY_Error[i_Tanggan_ADD]++;

        Ask_Tanggan[0]  = Tanggan_ADD[i_Tanggan_ADD];    //0  地址
        Ask_Tanggan[1]  = ASK_average;        //1  命令
        len_uart_ywy = write(fd_uart_ywy, Ask_Tanggan, 2);
        if (len_uart_ywy<0)
        {
            printf("write ask yeweiyi data failed \n");
        }
//        else
//        {
//            printf("%02x write ask yeweiyi data success \n",Ask_Tanggan[0]);fflush(stdout);
//        }
    }
    else if(Uart_Channel == 2)        //问 K B 值
    {
        Ask_Tanggan[0]  = Tanggan_ADD[i_Tanggan_ADD];    //0  地址
        Ask_Tanggan[1]  = ASK_Range_KB ;     //1  命令
        len_uart_ywy = write(fd_uart_ywy, Ask_Tanggan, 2);
        if (len_uart_ywy<0)
        {
            printf("write ask Range data failed \n");
        }
//        else
//        {
//            printf("write ask Range data success \n");fflush(stdout);
//        }
    }
    else if(Uart_Channel == 3)      //  问 油面补偿值
    {
        if(i_Ask_Tanggan>2)
        {
            emit compensation_set_result(ASK_OIL_compensation,i_Tanggan_ADD,"失败");
            i_Ask_Tanggan = 1;
            if((i_Tanggan_ADD < sum_Tangan_Amount))
            {
                i_Tanggan_ADD++;
            }
            else
            {
                i_Ask_Tanggan = 0;
                i_Tanggan_ADD = 1;
                Uart_Channel = 4; //最后一根探杆问了三遍，无果，返回
                Flag_carry_the_produce = 0;
            }
        }
        else
        {
           i_Ask_Tanggan++;
        }

        if(Flag_carry_the_produce)
        {
            Ask_Tanggan[0]  = Tanggan_ADD[i_Tanggan_ADD];    //0  地址
            Ask_Tanggan[1]  = ASK_OIL_compensation;        //1  命令
            len_uart_ywy = write(fd_uart_ywy, Ask_Tanggan, 2);
            if (len_uart_ywy<0)
            {
                printf("write ask OIL_compensation data failed \n");
            }
//            else
//            {
//                printf("%02x write ask OIL_compensation data success \n",Ask_Tanggan[0]);fflush(stdout);
//            }
        }
//        Flag_carry_the_produce = 1;
    }
    else if(Uart_Channel == 4)    //问水面补偿值
    {
        if(i_Ask_Tanggan>2)
        {
            emit compensation_set_result(ASK_Water_compensation,i_Tanggan_ADD,"失败");
            i_Ask_Tanggan = 1;
            if((i_Tanggan_ADD < sum_Tangan_Amount))
            {
                i_Tanggan_ADD++;
            }
            else
            {
                i_Ask_Tanggan = 0;
                i_Tanggan_ADD = 0;
                Uart_Channel = 1; //最后一根探杆问了三遍，无果，返回
                Flag_carry_the_produce = 0;
                emit compensation_set_result(ASK_Water_compensation ,0,"完成问询");
            }
        }
        else
        {
           i_Ask_Tanggan++;
        }

        if(Flag_carry_the_produce)
        {
            Ask_Tanggan[0]  = Tanggan_ADD[i_Tanggan_ADD];    //0  地址
            Ask_Tanggan[1]  = ASK_Water_compensation;        //1  命令
            len_uart_ywy = write(fd_uart_ywy, Ask_Tanggan, 2);
            if (len_uart_ywy<0)
            {
                printf("write ask Water_compensation data failed \n");
            }
//            else
//            {
//                printf("%02x write ask Water_compensation data success \n",Ask_Tanggan[0]);fflush(stdout);
//            }
        }
//        Flag_carry_the_produce = 1;
    }

    //**************************************** 设置 *************************************************//
    else if(Uart_Channel == 10)    // 设置地址
    {
        Ask_Tanggan[0]  = 0xDF;
        Ask_Tanggan[1]  = 0x73;
        Ask_Tanggan[2]  = 0x01;
        Ask_Tanggan[3]  = Tanggan_SET_ADD;
        Ask_Tanggan[4]  = EOH;
        len_uart_ywy = write(fd_uart_ywy, Ask_Tanggan, 5);
        if (len_uart_ywy<0)
        {
            printf("write yeweiyi ADD data failed \n");
        }
//        else
//        {
//            printf("write yeweiyi ADD data success \n");fflush(stdout);
//        }
    }
    else if(Uart_Channel == 11)  //设置时先发送地址 油面补偿命令
    {
        if(i_Ask_Tanggan>2)
        {
            emit compensation_set_result(Write_OIL_compensation,i_Tanggan_ADD,"失败");
            i_Ask_Tanggan = 1;
            if((i_Tanggan_ADD < sum_Tangan_Amount))
            {
                i_Tanggan_ADD++;
            }
            else
            {
                i_Ask_Tanggan = 0;
                i_Tanggan_ADD = 1;
                Uart_Channel = 13; //最后一根探杆问了三遍，无果，返回
                Flag_carry_the_produce = 0;
            }
        }
        else
        {
            i_Ask_Tanggan++;
        }

        if(Flag_carry_the_produce)
        {
            Ask_Tanggan[0]  = Tanggan_ADD[i_Tanggan_ADD];
            Ask_Tanggan[1]  = Write_OIL_compensation;        //1  命令
            len_uart_ywy = write(fd_uart_ywy, Ask_Tanggan, 2);
            if (len_uart_ywy<0)
            {
                printf("write OIL_compensation ADD data failed \n");
            }
//            else
//            {
//                printf("%02x write OIL_compensation ADD data success \n",Ask_Tanggan[0]);fflush(stdout);
//            }
        }
//        Flag_carry_the_produce = 1;
    }
    else if(Uart_Channel == 12)  //油面补偿
    {
        if(i_Ask_Tanggan>2)
        {
            emit compensation_set_result(Write_OIL_compensation + SOH + 4,i_Tanggan_ADD,"失败");
            i_Ask_Tanggan = 0;
            if((i_Tanggan_ADD < sum_Tangan_Amount))
            {
                i_Tanggan_ADD++;
                Uart_Channel = 11;
            }
            else
            {
               Uart_Channel = 13;
               i_Tanggan_ADD = 1;
            }
            Flag_carry_the_produce = 0;
        }
        else
        {
            i_Ask_Tanggan++;
        }

        if(Flag_carry_the_produce)
        {
            str = QString::number(Compension[i_Tanggan_ADD][0], 'f', 1);  //直接打印乱码
            Q_setvalue = str.toLatin1();
            setvalue = Q_setvalue.data();
            len_Tangan_Config_CompOIL = strlen(setvalue);
            for(int i=0;i<len_Tangan_Config_CompOIL;i++)
            {
               Tangan_Config.OIL_compensation[i] = setvalue[i];
            }

            Ask_Tanggan[0]  = SOH;        //2  SOH
            for(int i=0;i<len_Tangan_Config_CompOIL;i++)
            {
                Ask_Tanggan[1+i] = Tangan_Config.OIL_compensation[i];
    //                printf("%02x ",Ask_Tanggan[1+i]);fflush(stdout);
            }
    //            printf("\n");
            Ask_Tanggan[1+len_Tangan_Config_CompOIL] = EOH;
            len_uart_ywy = write(fd_uart_ywy, Ask_Tanggan, 2+len_Tangan_Config_CompOIL);
            if (len_uart_ywy<0)
            {
                printf("write OIL_compensation data failed \n");
            }
//            else
//            {
//                printf("%d write OIL_compensation data success \n",i_Tanggan_ADD);fflush(stdout);
//            }
        }

    }
    else if(Uart_Channel == 13)  //设置时先发送地址  水面补偿命令
    {
        if(i_Ask_Tanggan>2)
        {
            emit compensation_set_result(Write_Water_compensation,i_Tanggan_ADD,"失败");
            i_Ask_Tanggan = 1;
            if((i_Tanggan_ADD < sum_Tangan_Amount))
            {
                i_Tanggan_ADD++;
            }
            else
            {
                i_Ask_Tanggan = 0;
                i_Tanggan_ADD = 0;
                Uart_Channel = 1; //最后一根探杆问了三遍，无果，返回
                Flag_carry_the_produce = 0;
                emit compensation_set_result(Write_Water_compensation ,0,"完成问询");
            }
        }
        else
        {
            i_Ask_Tanggan++;
        }

        if(Flag_carry_the_produce)
        {
            Ask_Tanggan[0]  = Tanggan_ADD[i_Tanggan_ADD];
            Ask_Tanggan[1]  = Write_Water_compensation;        //1  命令
            len_uart_ywy = write(fd_uart_ywy, Ask_Tanggan, 2);
            if (len_uart_ywy<0)
            {
                printf("write Water_compensation ADD data failed \n");
            }
//            else
//            {
//                printf("write Water_compensation ADD data success \n");fflush(stdout);
//            }
        }
    }
    else if(Uart_Channel == 14)  //水面补偿
    {
        if(i_Ask_Tanggan>2)
        {
            emit compensation_set_result(Write_Water_compensation + SOH + 6,i_Tanggan_ADD,"失败");
            i_Ask_Tanggan = 0;
            if((i_Tanggan_ADD < sum_Tangan_Amount))
            {
                i_Tanggan_ADD++;
                Uart_Channel = 13;
            }
            else
            {
               Uart_Channel = 1;
               i_Tanggan_ADD = 0;
               emit compensation_set_result(Write_Water_compensation ,0,"完成问询");
            }
            Flag_carry_the_produce = 0;
        }
        else
        {
            i_Ask_Tanggan++;
        }

        if(Flag_carry_the_produce)
        {
            str = QString::number(Compension[i_Tanggan_ADD][1], 'f', 1);  //直接打印乱码
            Q_setvalue = str.toLatin1();
            setvalue = Q_setvalue.data();
            len_Tangan_Config_CompWater = strlen(setvalue);
            for(int i=0;i<len_Tangan_Config_CompWater;i++)
            {
               Tangan_Config.Water_compensation[i] = setvalue[i];
            }

            Ask_Tanggan[0]  = SOH;        //2  SOH
            for(int i=0;i<len_Tangan_Config_CompWater;i++)
            {
                Ask_Tanggan[1+i] = Tangan_Config.Water_compensation[i];
            }
            Ask_Tanggan[1+len_Tangan_Config_CompWater] = EOH;
            len_uart_ywy = write(fd_uart_ywy, Ask_Tanggan, 2+len_Tangan_Config_CompWater);
            if (len_uart_ywy<0)
            {
                printf("write Water_compensation data failed \n");
            }
//            else
//            {
//                printf("write Water_compensation data success \n");fflush(stdout);
//            }
        }
    }
}

void ywythread::run()
{
    fd_uart_ywy = open(REOILGAS_SERI, O_RDWR | O_NOCTTY);
    if (fd_uart_ywy<0)
    {
           perror(REOILGAS_SERI);
    }
    ret_uart_ywy = set_port_attr (fd_uart_ywy,B4800,8,"1",'N',0,0);   //4800波特率 数据位8位 停止位1位 无校验
    if (ret_uart_ywy<0)
    {
           printf("set uart TTYMXC1 FAILED \n");
    }

    sleep(1);
    for(;;)
    {
        msleep(100);
        Asking_Handle_YWY();
        if(Flag_carry_the_produce)
        {
            msleep(1000);
            Recving_Handle_YWY();
        }

        Flag_carry_the_produce =1;
    }
}
