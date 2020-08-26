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
#include"file_op.h"
#include"database_op.h"
#include"ywy_yfy.h"

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
uchar flag_ask_whichfunction = 0;
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
    uchar i_com = 0;

    if(Data_Buf_Sencor[0]==Ask_Tanggan[0])  //比对地址
    {      
        unsigned int SCRC = CRC_Test(Data_Buf_Sencor,len_uart_ywy);
        if(Data_Buf_Sencor[len_uart_ywy - 2] == ((SCRC & 0xff00) >> 8) && Data_Buf_Sencor[len_uart_ywy - 1] == (SCRC & 0x00ff))
        {
            if(Data_Buf_Sencor[1] == YWY_Read )   //读
            {
                switch (flag_ask_whichfunction)
                {
                    case Tangan_Height_register:
                        Flag_Communicate_YWY_Error[i_Tanggan_ADD] = 0;
                        if(i_alarm_record[i_Tanggan_ADD][0])
                        {
                            emit Send_alarm_info(i_Tanggan_ADD,6);
                            i_alarm_record[i_Tanggan_ADD][0] = 0;
                            add_yeweiyi_alarminfo(QString("%1 #罐").arg(i_Tanggan_ADD),"通讯正常");
                        }

                        Dis_HeightData.OIL_Height_int = Data_Buf_Sencor[3];                                       //油高
                        Dis_HeightData.OIL_Height_int = (Dis_HeightData.OIL_Height_int << 8) | Data_Buf_Sencor[4];
                        Dis_HeightData.OIL_Height_int = (Dis_HeightData.OIL_Height_int << 8) | Data_Buf_Sencor[5];
                        Dis_HeightData.OIL_Height_int = (Dis_HeightData.OIL_Height_int << 8) | Data_Buf_Sencor[6];
                        Dis_HeightData.OIL_Height_float = (float)Dis_HeightData.OIL_Height_int / 65536;

                        Dis_HeightData.Water_Height_int = Data_Buf_Sencor[7];                                       //水高
                        Dis_HeightData.Water_Height_int = (Dis_HeightData.Water_Height_int << 8) | Data_Buf_Sencor[8];
                        Dis_HeightData.Water_Height_int = (Dis_HeightData.Water_Height_int << 8) | Data_Buf_Sencor[9];
                        Dis_HeightData.Water_Height_int = (Dis_HeightData.Water_Height_int << 8) | Data_Buf_Sencor[10];
                        Dis_HeightData.Water_Height_float = (float)Dis_HeightData.Water_Height_int / 65536;

                        Dis_HeightData.TEMP_Height_int_1 =  Data_Buf_Sencor[11];                                    //温度
                        Dis_HeightData.TEMP_Height_int_1 = (Dis_HeightData.TEMP_Height_int_1 << 8) | Data_Buf_Sencor[12];
                        if(Data_Buf_Sencor[11] > 0x0F)
                        {
                            Dis_HeightData.TEMP_Height_float_1 =  -((float)(((~Dis_HeightData.TEMP_Height_int_1)&0x0000FFFF)+1)/16) ;
                        }
                        else
                        {
                            Dis_HeightData.TEMP_Height_float_1 = (float)Dis_HeightData.TEMP_Height_int_1 / 16;
                        }

                        Dis_HeightData.strOIL_Height     = QString::number(Dis_HeightData.OIL_Height_float, 'f', 1);
                        Dis_HeightData.strWater_Height   = QString::number(Dis_HeightData.Water_Height_float, 'f', 1);
                        Dis_HeightData.strTEMP           = QString::number(Dis_HeightData.TEMP_Height_float_1, 'f', 1);

                        //云飞扬上传相关
                        Reply_Data_OT[i_Tanggan_ADD][4].f = Dis_HeightData.OIL_Height_float;
                        Reply_Data_OT[i_Tanggan_ADD][5].f = Dis_HeightData.Water_Height_float;
                        Reply_Data_OT[i_Tanggan_ADD][6].f = Dis_HeightData.TEMP_Height_float_1;

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
                                    emit Send_alarm_info(i_Tanggan_ADD,6);
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
                                    emit Send_alarm_info(i_Tanggan_ADD,6);
                                }
                            }

                            if(Dis_HeightData.strWater_Height.toFloat() > OilTank_Set[i_Tanggan_ADD-1][1])
                            {
                                if(i_alarm_record[i_Tanggan_ADD][5] == 0)
                                {
                                    emit Send_alarm_info(Ask_Tanggan[0],5);
                                    i_alarm_record[i_Tanggan_ADD][5] = 1;
                                    add_yeweiyi_alarminfo(QString("%1 #罐").arg(i_Tanggan_ADD),"水位过高");
                                    printf("%d  %f  %f \n",i_Tanggan_ADD,Dis_HeightData.strWater_Height.toFloat(),OilTank_Set[i_Tanggan_ADD-1][1]);fflush(stdout);
                                }
                            }
                            else
                            {
                                if(i_alarm_record[i_Tanggan_ADD][5])
                                {
                                    i_alarm_record[i_Tanggan_ADD][5] = 0;
                                    add_yeweiyi_alarminfo(QString("%1 #罐").arg(i_Tanggan_ADD),"水位正常");
                                    emit Send_alarm_info(i_Tanggan_ADD,6);
                                    printf("%d  %f  %f \n",i_Tanggan_ADD,Dis_HeightData.strWater_Height.toFloat(),OilTank_Set[i_Tanggan_ADD-1][1]);fflush(stdout);
                                }
                            }

//                            if(Dis_HeightData.strOIL_Height == "E505" || Dis_HeightData.strWater_Height == "E505")
//                            {
//                                if(i_alarm_record[i_Tanggan_ADD][3] == 0)
//                                {
//                                    emit Send_alarm_info(Ask_Tanggan[0],3);
//                                    i_alarm_record[i_Tanggan_ADD][3] = 1;
//                                    add_yeweiyi_alarminfo(QString("%1 #罐").arg(i_Tanggan_ADD),"浮子故障");
//                                }
//                            }
//                            else
//                            {
//                                if(i_alarm_record[i_Tanggan_ADD][3])
//                                {
//                                    i_alarm_record[i_Tanggan_ADD][3] = 0;
//                                    add_yeweiyi_alarminfo(QString("%1 #罐").arg(i_Tanggan_ADD),"浮子正常");
//                                    emit Send_alarm_info(Tanggan_ADD[i_Tanggan_ADD],6);
//                                }
//                            }
                        }


//                       printf("%02d:%02d:%02d",QTime::currentTime().hour(),QTime::currentTime().minute(),QTime::currentTime().second());fflush(stdout);printf("\n");
//                       printf("OIL Height  : %s",Dis_HeightData.strOIL_Height.toStdString().data());      fflush(stdout);printf("\n");
//                       printf("Water Height: %s",Dis_HeightData.strWater_Height.toStdString().data());    fflush(stdout);printf("\n");
//                       printf("Temperature : %s",Dis_HeightData.strTEMP.toStdString().data());            fflush(stdout);printf("\n");

                        emit Send_Height_Signal(Ask_Tanggan[0],Dis_HeightData.strOIL_Height,Dis_HeightData.strWater_Height,Dis_HeightData.strTEMP);
                        break;
                    case OIL_compensation_register:
                        Dis_HeightData.OIL_compensation_ucahr[0] = Data_Buf_Sencor[3];
                        Dis_HeightData.OIL_compensation_ucahr[1] = Data_Buf_Sencor[4];
                        Dis_HeightData.OIL_compensation_ucahr[2] = Data_Buf_Sencor[5];
                        Dis_HeightData.OIL_compensation_ucahr[3] = Data_Buf_Sencor[6];
                        Compension[i_Tanggan_ADD][0] = ConvertIEE754ToDex(Dis_HeightData.OIL_compensation_ucahr);

                        emit Send_compensation_Signal(OIL_compensation_register,i_Tanggan_ADD, Compension[i_Tanggan_ADD][0]);

                        printf("OIL_compensation : %f",Compension[i_Tanggan_ADD][0]);            fflush(stdout);printf("\n");

                        emit compensation_set_result(OIL_compensation_register,i_Tanggan_ADD,"成功");
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
                    case Water_compensation_register:
                        Dis_HeightData.Water_compensation_uchar[0] = Data_Buf_Sencor[3];
                        Dis_HeightData.Water_compensation_uchar[1] = Data_Buf_Sencor[4];
                        Dis_HeightData.Water_compensation_uchar[2] = Data_Buf_Sencor[5];
                        Dis_HeightData.Water_compensation_uchar[3] = Data_Buf_Sencor[6];
                        Compension[i_Tanggan_ADD][1] = ConvertIEE754ToDex(Dis_HeightData.Water_compensation_uchar);

                        emit Send_compensation_Signal(Water_compensation_register,i_Tanggan_ADD, Compension[i_Tanggan_ADD][1]);

                        printf("Water_compensation : %f",Compension[i_Tanggan_ADD][1]);            fflush(stdout);printf("\n");


                        emit compensation_set_result(Water_compensation_register,i_Tanggan_ADD,"成功");
                        if((i_Tanggan_ADD < sum_Tangan_Amount))
                        {
                            i_Tanggan_ADD++;
                        }
                        else
                        {
                            emit compensation_set_result(Water_compensation_register ,0,"完成问询");
                            Uart_Channel = 1;
                            i_Tanggan_ADD = 0;
                        }
                        i_Ask_Tanggan = 0;

                        break;
                    default:
                        printf(("default read \n"));fflush(stdout);
                        break;
                }
            }
            //=============================================================================
            //=============================================================================
            else if(Data_Buf_Sencor[1] == YWY_Write)  //写返回
            {
                switch (Data_Buf_Sencor[3])
                {
                    case OIL_compensation_register:
                        for( i_com=0;i_com<10;i_com++)
                        {
                            if(Data_Buf_Sencor[i_com] != Ask_Tanggan[i_com])
                            {
                                i_com = 20;
                            }
                        }
                        if(i_com==10)
                        {
                            emit compensation_set_result(OIL_compensation_register + 3,i_Tanggan_ADD,"成功");
                            if((i_Tanggan_ADD < sum_Tangan_Amount))
                            {
                                i_Tanggan_ADD++;
                            }
                            else
                            {
                                Uart_Channel = 12;
                                i_Tanggan_ADD = 1;
                            }
                            i_Ask_Tanggan = 0;
                        }
                        else
                        {
                            emit compensation_set_result(OIL_compensation_register +3,i_Tanggan_ADD,"失败");
                        }
                        break;
                    case Water_compensation_register:
                        for( i_com=0;i_com<10;i_com++)
                        {
                            if(Data_Buf_Sencor[i_com] != Ask_Tanggan[i_com])
                            {
                                i_com = 20;
                            }
                        }
                        if(i_com==10)
                        {
                            emit compensation_set_result(Water_compensation_register +4,i_Tanggan_ADD,"成功");
                            if((i_Tanggan_ADD < sum_Tangan_Amount))
                            {
                                i_Tanggan_ADD++;
                            }
                            else
                            {
                                Uart_Channel = 1;
                                i_Tanggan_ADD = 0;
                                emit compensation_set_result(Water_compensation_register ,0,"完成问询");
                            }
                            i_Ask_Tanggan = 0;
                        }
                        else
                        {
                           emit compensation_set_result(Water_compensation_register+4,i_Tanggan_ADD,"失败");
                        }
                        break;
                    case Tangan_Address_register:
                        for( i_com=0;i_com<8;i_com++)
                        {
                            if(Data_Buf_Sencor[i_com] != Ask_Tanggan[i_com])
                            {
                                i_com = 20;
                            }
                        }
                        if(i_com==8)
                        {
                           Uart_Channel = 1;
                           emit Set_tangan_add_success(Ask_Tanggan[5]);
                        }
                        break;
                    default:
                        printf(("default read \n"));fflush(stdout);
                        break;
                }
            }
        }

        memset(Ask_Tanggan,0,20);
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

//========================================================================
//====================              ======================================
//========================================================================
void ywythread:: Asking_Handle_YWY()
{
    unsigned int SCRC = 0;
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
                           emit Send_alarm_info(i,0);
                           i_alarm_record[i][0] = 1;
                           add_yeweiyi_alarminfo(QString("%1 #罐").arg(i),"通讯故障");
                       }
                   }
               }
           }
        }
        Flag_Communicate_YWY_Error[i_Tanggan_ADD]++;

        Ask_Tanggan[0] =  i_Tanggan_ADD;
        Ask_Tanggan[1] = YWY_Read;
        Ask_Tanggan[2] = 0;
        Ask_Tanggan[3] = Tangan_Height_register;
        Ask_Tanggan[4] = 0;
        Ask_Tanggan[5] = 7;
        SCRC = CRC_Test(Ask_Tanggan,8);
        Ask_Tanggan[6] = (SCRC & 0xff00) >> 8;
        Ask_Tanggan[7] = (SCRC & 0x00ff);
        len_uart_ywy = write(fd_uart_ywy, Ask_Tanggan, 8);        
        if (len_uart_ywy<0)
        {
            printf("write ask yeweiyi data failed \n");
        }
        else
        {
//            printf("%02x write ask yeweiyi data success \n",Ask_Tanggan[0]);fflush(stdout);
            flag_ask_whichfunction = Tangan_Height_register;
        }
    }
    else if(Uart_Channel == 3)      //  问 油面补偿值
    {
        if(i_Ask_Tanggan>2)
        {
            emit compensation_set_result(OIL_compensation_register,i_Tanggan_ADD,"失败");
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
            Ask_Tanggan[0] =  i_Tanggan_ADD;
            Ask_Tanggan[1] = YWY_Read;
            Ask_Tanggan[2] = 0;
            Ask_Tanggan[3] = OIL_compensation_register;
            Ask_Tanggan[4] = 0;
            Ask_Tanggan[5] = 2;
            SCRC = CRC_Test(Ask_Tanggan,8);
            Ask_Tanggan[6] = (SCRC & 0xff00) >> 8;
            Ask_Tanggan[7] = (SCRC & 0x00ff);
            len_uart_ywy = write(fd_uart_ywy, Ask_Tanggan, 8);            
            if (len_uart_ywy<0)
            {
                printf("write ask OIL_compensation data failed \n");
            }
            else
            {
//                printf("%02x write ask OIL_compensation data success \n",Ask_Tanggan[0]);fflush(stdout);
                flag_ask_whichfunction = OIL_compensation_register;
            }
        }
    }
    else if(Uart_Channel == 4)    //问水面补偿值
    {
        if(i_Ask_Tanggan>2)
        {
            emit compensation_set_result(Water_compensation_register,i_Tanggan_ADD,"失败");
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
                emit compensation_set_result(Water_compensation_register ,0,"完成问询");
            }
        }
        else
        {
           i_Ask_Tanggan++;
        }

        if(Flag_carry_the_produce)
        {
            Ask_Tanggan[0] =  i_Tanggan_ADD;
            Ask_Tanggan[1] = YWY_Read;
            Ask_Tanggan[2] = 0;
            Ask_Tanggan[3] = Water_compensation_register;
            Ask_Tanggan[4] = 0;
            Ask_Tanggan[5] = 2;
            SCRC = CRC_Test(Ask_Tanggan,8);
            Ask_Tanggan[6] = (SCRC & 0xff00) >> 8;
            Ask_Tanggan[7] = (SCRC & 0x00ff);
            len_uart_ywy = write(fd_uart_ywy, Ask_Tanggan, 8);            
            if (len_uart_ywy<0)
            {
                printf("write ask Water_compensation data failed \n");
            }
            else
            {
//                printf("%02x write ask Water_compensation data success \n",Ask_Tanggan[0]);fflush(stdout);
                flag_ask_whichfunction = Water_compensation_register;
            }
        }
    }

    //**************************************** 设置 *************************************************//
    else if(Uart_Channel == 10)    // 设置地址
    {
        Ask_Tanggan[0] =  0;
        Ask_Tanggan[1] = YWY_Write;
        Ask_Tanggan[2] = 0;
        Ask_Tanggan[3] = Tangan_Address_register;
        Ask_Tanggan[4] = 0;
        Ask_Tanggan[5] = Tanggan_SET_ADD;
        SCRC = CRC_Test(Ask_Tanggan,8);
        Ask_Tanggan[6] = (SCRC & 0xff00) >> 8;
        Ask_Tanggan[7] = (SCRC & 0x00ff);
        len_uart_ywy = write(fd_uart_ywy, Ask_Tanggan, 8);
        if (len_uart_ywy<0)
        {
            printf("write yeweiyi ADD data failed \n");
        }
//        else
//        {
//            printf("write yeweiyi ADD data success \n");fflush(stdout);
//        }
    }
    else if(Uart_Channel == 11)  //设置 油面补偿
    {
        if(i_Ask_Tanggan>2)
        {
            emit compensation_set_result(OIL_compensation_register,i_Tanggan_ADD,"失败");
            i_Ask_Tanggan = 1;
            if((i_Tanggan_ADD < sum_Tangan_Amount))
            {
                i_Tanggan_ADD++;
            }
            else
            {
                i_Ask_Tanggan = 0;
                i_Tanggan_ADD = 1;
                Uart_Channel = 12; //最后一根探杆问了三遍，无果，返回
                Flag_carry_the_produce = 0;
            }
        }
        else
        {
            i_Ask_Tanggan++;
        }

        if(Flag_carry_the_produce)
        {
            ConvertDexToIEE754(Compension[i_Tanggan_ADD][0],Tangan_Config.OIL_compensation);
            Ask_Tanggan[0] =  i_Tanggan_ADD;
            Ask_Tanggan[1] = YWY_Write;
            Ask_Tanggan[2] = 0;
            Ask_Tanggan[3] = OIL_compensation_register;
            Ask_Tanggan[4] = Tangan_Config.OIL_compensation[0];
            Ask_Tanggan[5] = Tangan_Config.OIL_compensation[1];
            Ask_Tanggan[6] = Tangan_Config.OIL_compensation[2];
            Ask_Tanggan[7] = Tangan_Config.OIL_compensation[3];
            SCRC = CRC_Test(Ask_Tanggan,10);
            Ask_Tanggan[8] = (SCRC & 0xff00) >> 8;
            Ask_Tanggan[9] = (SCRC & 0x00ff);
            len_uart_ywy = write(fd_uart_ywy, Ask_Tanggan, 10);
            if (len_uart_ywy<0)
            {
                printf("write OIL_compensation ADD data failed \n");
            }
//            else
//            {
//                printf("%02x write OIL_compensation ADD data success \n",Ask_Tanggan[0]);fflush(stdout);
//            }
        }
    }

    else if(Uart_Channel == 12)  //设置  水面补偿
    {
        if(i_Ask_Tanggan>2)
        {
            emit compensation_set_result(Water_compensation_register,i_Tanggan_ADD,"失败");
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
                emit compensation_set_result(Water_compensation_register ,0,"完成问询");
            }
        }
        else
        {
            i_Ask_Tanggan++;
        }

        if(Flag_carry_the_produce)
        {
            ConvertDexToIEE754(Compension[i_Tanggan_ADD][1],Tangan_Config.Water_compensation);
            Ask_Tanggan[0] =  i_Tanggan_ADD;
            Ask_Tanggan[1] = YWY_Write;
            Ask_Tanggan[2] = 0;
            Ask_Tanggan[3] = Water_compensation_register;
            Ask_Tanggan[4] = Tangan_Config.Water_compensation[0];
            Ask_Tanggan[5] = Tangan_Config.Water_compensation[1];
            Ask_Tanggan[6] = Tangan_Config.Water_compensation[2];
            Ask_Tanggan[7] = Tangan_Config.Water_compensation[3];
            SCRC = CRC_Test(Ask_Tanggan,10);
            Ask_Tanggan[8] = (SCRC & 0xff00) >> 8;
            Ask_Tanggan[9] = (SCRC & 0x00ff);
            len_uart_ywy = write(fd_uart_ywy, Ask_Tanggan, 10);
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
}
void ywythread::ConvertDexToIEE754(float fpointer,unsigned char *a)  //十进制转化为 IEEE745 小数  <1 >-1 转换还有点问题
{
    int Flag=0;
    double   integer,decimal;    //整数，小数
    unsigned  long   bininteger,bindecimal;   //二进制整数，二进制小数
    int   _power,i;

    if(fpointer<0)
    {
      fpointer=fpointer*(-1);
      Flag=1;
    }

    decimal = modf(fpointer,&integer);   //将整数位到存到 integer ，小数位返回到decimal
    if(decimal || integer)    //判断 fpointer是否为0
    {
     bindecimal = (unsigned  long )(decimal * 0x800000);   //0x800000=2^23 。得到小数位二进制表现形式
     while((bindecimal & 0xff800000) > 0) //计算有没有超过23位二进制数
     bindecimal >>= 1;
     if(integer > 0)
     {
      bininteger = (unsigned  long )integer;
       for(i=0;i<32;i++)               //计算整数部分的2的幂指数，整数位转化为二进制后的位数，计算小数点移动的位数
       {
          if(bininteger&0x1)
         _power = i;
         bininteger >>= 0x1;
       }
    bininteger = (unsigned  long )integer;
    bininteger &= ~(0x1 << _power); //去掉最高位的1
    if(_power >= 23) //如果幂指数>23 则舍弃小数位部分
    {
          bininteger >>= (_power-23);
          bindecimal = 127+_power;
          bininteger |= bindecimal << 23;
    }
    else
    {
          bininteger <<= (23 - _power);//将去掉最高位的整数二进制，移至第22为
          bindecimal >>= _power;       //将小数部分左移1，
          bininteger |= bindecimal;    //二者向或得到1.x的x，
          bindecimal = 127+_power;     //指数，右移的小数点数+127
          bininteger |= bindecimal << 23; // 指数为右移23为变为或上x。
    }
  }
    else if(integer == 0)
    {
            bindecimal <<= 9; //将小数部分的二进制移至最高位31位
            _power = 0;
            bininteger = bindecimal;
            while(bininteger == ((bindecimal<<1)>>1)) //判断小数位最高位是否为1.  最高位为0 ：真
            {
              _power++;
              bindecimal <<= 0x1;
              bininteger = bindecimal;  //直到最高位为1,退出循环
            }
            _power++;
            bindecimal <<= 0x1;    //将1.x的1去掉 求x的值，
            bindecimal >>= 9;      //将小数位回到0-22位
            bininteger = bindecimal; //暂存到二进制整数中，
            bindecimal = 127-_power;
            bininteger |= bindecimal << 23;  //将指数为右移值23为向或得到其值，printf("%02x",bininteger);fflush(stdout);
    }
     if(Flag==1)
      bininteger |= 0x80000000;


    i = 0;
    a[i++] =(unsigned char) ((bininteger >> 24) & 0xff);
    a[i++] = (unsigned char)((bininteger >> 16) & 0xff);
    a[i++] = (unsigned char)((bininteger >> 8 ) & 0xff);
    a[i++] =(unsigned char)( bininteger & 0xff);
    }
}
float ywythread::ConvertIEE754ToDex(unsigned char *SpModRegister)
{
    float x,fpointer;
    unsigned long bininteger,bintmp;
    int _power,i=0,s;

    bintmp = SpModRegister[i++] << 8;
    bintmp |= SpModRegister[i++];
    bininteger = SpModRegister[i++] << 8;
    bininteger |= SpModRegister[i++];
    bininteger |= bintmp << 16;

    if(bininteger == 0)
    return 0.0;

    bintmp = bininteger & 0x7FFFFF;
    x = (double)bintmp / 0x800000;
    bintmp = bininteger >> 23;
    _power = bintmp & 0xff;

    bintmp = bininteger & 0x80000000;
    s = (bintmp) ? 1 : 0 ;

    bintmp =(unsigned  long ) pow(2,fabs(_power-127)) ;
    if(_power >= 127)
    fpointer = (1+x) * bintmp ;
    else
    fpointer = (1+x) / bintmp ;
    //bintmp = (fpointer * 10000 + 5)/10;
    //fpointer = (double)bintmp / 1000;

     if(s)
       fpointer *= -1;

     return (fpointer);
}

void ywythread::run()
{
    fd_uart_ywy = open(REOILGAS_SERI, O_RDWR | O_NOCTTY);
    if (fd_uart_ywy<0)
    {
           perror(REOILGAS_SERI);
    }
    ret_uart_ywy = set_port_attr (fd_uart_ywy,B9600,8,"1",'E',0,0);   //9600波特率 数据位8位 停止位1位 偶校验
    if (ret_uart_ywy<0)
    {
           printf("set uart TTYMXC4 FAILED \n");
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
