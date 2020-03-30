#include "mythread.h"
#include <QtDebug>
#include <serial.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include "net_tcpclient_hb.h"
#include "keyboard.h"
#include "config.h"      //OIL_BASIN[]   PIPE[]  TANK[]  DISPENER[]
#include "mainwindow.h"
#include "io_op.h"
#include "systemset.h"
#include "database_op.h"

unsigned char FLAG_STACHANGE[50][11] = {0};//判断32个点状态是否变化
QMutex net_data;
unsigned char FLAG_num_net = 0;//对断网记录的状态记录次数进行标注


unsigned char flag_output_basin[8] = {0,0,0,0,0,0,0,0}; //0 初始化，1 正常写入 ，2油报，3水报，4传感器，5通信故障
unsigned char flag_output_pipe[8] = {0,0,0,0,0,0,0,0};
unsigned char flag_output_dispener[8] = {0,0,0,0,0,0,0,0};
unsigned char flag_output_tank[8] = {0,0,0,0,0,0,0,0};

unsigned char IIE_uart = 0;//通信状态
unsigned char IIE_sta[8] = {0};//IIE状态位
unsigned char IIE_set[8] = {0};//IIE设置位
unsigned char IIE_sta_pre = 0;//IIE前一次状态位
unsigned char IIE_set_pre = 0;//IIE前一次设置位
unsigned int IIE_R = 0;//电压
unsigned int IIE_V = 0;//电阻
unsigned int IIE_people_time = 0;//人员值守倒计时
int IIE_oil_time = 0;//稳油倒计时
int Flag_IIE_c = 6;//上一次是否开启

unsigned char flag_syswro = 0;  //ask690进程检测位

#define SHASIZE 100
#define SHMNAME "shareMem"
#define SEMSIG  "signalMem"

int shmid;
char *ptr;
sem_t *semid;
mythread::mythread(QObject *parent):
    QThread(parent)
{
    qDebug() << "creat sharemem!";
   //共享内存初始化：ask690
   shmid = shm_open(SHMNAME,O_RDWR|O_CREAT,0644);
   ftruncate(shmid,SHASIZE);
   semid = sem_open(SEMSIG,O_CREAT,0644,0);
   ptr = (char *)mmap(NULL,SHASIZE,PROT_READ|PROT_WRITE,MAP_SHARED,shmid,0);
   strcpy(ptr,"\0");

}

//压力法数值显示函数
void mythread::Data_Display()
{
    emit set_pressure_number();
}
void mythread::run()
{
    unsigned char count_renti = 0;
//    unsigned char i_sound = 0;                //8tankpre    9tankwarn   10tankhigh  11tanklow
//    unsigned char flag_sound[20] = {0};     //2223   //4tankoil  5tankwater  6tanksensor 7tankuart
    while(1)                                //0pipeoil  1pipewater  2pipesensor 3tankuart
    {
        while(flag_waitsetup)
        {
            usleep(20000);
        }
        //读共享内存
        sem_wait(semid);
        for(uchar i = 0;i < 44;i++) //增加IIE后由36改为44
        {
            Data_Buf_Sencor[i] = ptr[i];
        }
        strcpy(ptr,"\0");

//        for(uchar i = 0;i < 44;i++) //增加IIE后由36改为44
//        {
//            printf("%02x ",Data_Buf_Sencor[i]);  //打印接收数据
//        }
        //printf("receive from ask 690 44\n");

        //数据打包,计算压力值
        Data_Handle();
        //显示压力值
        Data_Display();

        //状态检测入口

        //人体静电检测
        if(Flag_xieyou)
        {
            emit set_renti(8,1);
            unsigned char flag_renti;
            if(Data_Buf_Sencor[17] == 0xff)
            {
                if(count_renti > 12)
                {
                    emit set_renti(7,1);
                    count_renti = 12;
                }
                count_renti++;
            }
            else
            {
                emit set_renti(7,0);
                if(count_renti > 4)
                {
                    count_renti = 3;
                }
                if(count_renti == 0)
                {
                    for(unsigned char i = 0;i<7;i++)
                    {

                        if(i == 0)
                        {
                            flag_renti = ((Data_Buf_Sencor[17]>>i) &0x03);
                            i = 1;
                        }
                        else
                        {
                            if(i < 6)
                            {
                                flag_renti = ((Data_Buf_Sencor[17]>>i) & 0x01);
                            }
                            else
                            {
                                flag_renti = ((Data_Buf_Sencor[17]>>i) &0x03);
                            }
                        }
                        emit set_renti(i,flag_renti);
                    }
                }
                else
                {
                    count_renti--;
                }
            }
        }
        else
        {
            emit set_renti(8,0);
        }
        //IIE监控部分
        IIE_analysis();

        if(flag_mythread == 1)
        {
            for(int i = 0;i < 8;i++)
            {
                flag_output_basin[i] = 0;
                flag_output_pipe[i] = 0;
                flag_output_dispener[i] = 0;
                flag_output_tank[i] = 0;
            }
            flag_mythread = 0;
        }
        for(int i = 0;i < 9;i++)
        {
            Mythread_basin[i] = 0;
            Mythread_dispener[i] = 0;
            Mythread_pipe[i] = 0;
            Mythread_tank[i] = 0;
        }

//basin   防渗池
        if(count_basin>=1)          //1#
        {
            if(OIL_BASIN[0]==0xc0)
            {
                if(OIL_BASIN[1]==0x00 && (flag_output_basin[0] == 0 || flag_output_basin[0] != 1))
                {
                    emit right_basin(1);
                    net_history(17,0);
                    add_value_controlinfo("1# 防渗池 ","设备正常");
                    flag_output_basin[0] = 1;
                }
                else
                {
                    if(OIL_BASIN[1]==0x88 && (flag_output_basin[0] == 0 || flag_output_basin[0] != 2))
                    {
                        emit warning_oil_basin(1);//油报警
                        net_history(17,1);
                        add_value_controlinfo("1# 防渗池 ","检油报警");
                        flag_output_basin[0] = 2;
                    }
                    if(OIL_BASIN[1]==0x90 && (flag_output_basin[0] == 0 || flag_output_basin[0] != 3))
                    {
                        emit warning_water_basin(1);//水报警
                        net_history(17,2);
                        add_value_controlinfo("1# 防渗池 ","检水报警");
                        flag_output_basin[0] = 3;
                    }
                    if(OIL_BASIN[1]==0x01 && (flag_output_basin[0] == 0 || flag_output_basin[0] != 4))
                    {
                        emit warning_sensor_basin(1);//传感器故障
                        net_history(17,3);
                        add_value_controlinfo("1# 防渗池 ","传感器故障");
                        flag_output_basin[0] = 4;
                    }
                    if(OIL_BASIN[1]==0x04 && (flag_output_basin[0] == 0 || flag_output_basin[0] != 5))
                    {
                        emit warning_uart_basin(1);//通信故障
                        net_history(17,4);
                        add_value_controlinfo("1# 防渗池 ","通信故障");
                        flag_output_basin[0] = 5;
                    }
                }
                if(OIL_BASIN[1] != 0x00)
                {
                    Mythread_basin[1] = 1;
                }
            }
        }
        if(count_basin>=2)              //2#
        {
            if(OIL_BASIN[2]==0xc0)
            {
                if(OIL_BASIN[3]==0x00 && (flag_output_basin[1] == 0 || flag_output_basin[1] != 1))
                {
                    emit right_basin(2);
                    net_history(18,0);
                    add_value_controlinfo("2# 防渗池 ","设备正常");
                    flag_output_basin[1] = 1;
                }
                else
                {
                    if(OIL_BASIN[3]==0x88 && (flag_output_basin[1] == 0 || flag_output_basin[1] != 2))
                    {
                        emit warning_oil_basin(2);//油报警
                        net_history(18,1);
                        add_value_controlinfo("2# 防渗池 ","检油报警");
                        flag_output_basin[1] = 2;
                    }
                    if(OIL_BASIN[3]==0x90 && (flag_output_basin[1] == 0 || flag_output_basin[1] != 3))
                    {
                        emit warning_water_basin(2);//水报警
                        net_history(18,2);
                        add_value_controlinfo("2# 防渗池 ","检水报警");
                        flag_output_basin[1] = 3;
                    }
                    if(OIL_BASIN[3]==0x01 && (flag_output_basin[1] == 0 || flag_output_basin[1] != 4))
                    {
                        emit warning_sensor_basin(2);//传感器故障
                        net_history(18,3);
                        add_value_controlinfo("2# 防渗池 ","传感器故障");
                        flag_output_basin[1] = 4;
                    }
                    if(OIL_BASIN[3]==0x04 && (flag_output_basin[1] == 0 || flag_output_basin[1] != 5))
                    {
                        emit warning_uart_basin(2);//通信故障
                        net_history(18,4);
                        add_value_controlinfo("2# 防渗池 ","通信故障");
                        flag_output_basin[1] = 5;
                    }
                }
                if(OIL_BASIN[3] != 0x00)
                {
                    Mythread_basin[2] = 1;
                }
            }
        }
        if(count_basin>=3)              //3#
        {
            if(OIL_BASIN[4]==0xc0)
            {
                if(OIL_BASIN[5]==0x00 && (flag_output_basin[2] == 0 || flag_output_basin[2] != 1))
                {
                    emit right_basin(3);
                    net_history(19,0);
                    add_value_controlinfo("3# 防渗池 ","设备正常");
                    flag_output_basin[2] = 1;
                }
                else
                {
                    if(OIL_BASIN[5]==0x88 && (flag_output_basin[2] == 0 || flag_output_basin[2] != 2))
                    {
                        emit warning_oil_basin(3);//油报警
                        net_history(19,1);
                        add_value_controlinfo("3# 防渗池 ","检油报警");
                        flag_output_basin[2] = 2;
                    }
                    if(OIL_BASIN[5]==0x90 && (flag_output_basin[2] == 0 || flag_output_basin[2] != 3))
                    {
                        emit warning_water_basin(3);//水报警
                        net_history(19,2);
                        add_value_controlinfo("3# 防渗池 ","检水报警");
                        flag_output_basin[2] = 3;
                    }
                    if(OIL_BASIN[5]==0x01 && (flag_output_basin[2] == 0 || flag_output_basin[2] != 4))
                    {
                        emit warning_sensor_basin(3);//传感器故障
                        net_history(19,3);
                        add_value_controlinfo("3# 防渗池 ","传感器故障");
                        flag_output_basin[2] = 4;
                    }
                    if(OIL_BASIN[5]==0x04 && (flag_output_basin[2] == 0 || flag_output_basin[2] != 5))
                    {
                        emit warning_uart_basin(3);//通信故障
                        net_history(19,4);
                        add_value_controlinfo("3# 防渗池 ","通信故障");
                        flag_output_basin[2] = 5;
                    }
                }
                if(OIL_BASIN[5] != 0x00)
                {
                    Mythread_basin[3] = 1;
                }
            }
        }
        if(count_basin>=4)              //4#
        {
            if(OIL_BASIN[6]==0xc0)
            {
                if(OIL_BASIN[7]==0x00 && (flag_output_basin[3] == 0 || flag_output_basin[3] != 1))
                {
                    emit right_basin(4);
                    net_history(20,0);
                    add_value_controlinfo("4# 防渗池 ","设备正常");
                    flag_output_basin[3] = 1;
                }
                else
                {
                    if(OIL_BASIN[7]==0x88 && (flag_output_basin[3] == 0 || flag_output_basin[3] != 2))
                    {
                        emit warning_oil_basin(4);//油报警
                        net_history(20,1);
                         add_value_controlinfo("4# 防渗池 ","检油报警");
                         flag_output_basin[3] = 2;
                    }
                    if(OIL_BASIN[7]==0x90 && (flag_output_basin[3] == 0 || flag_output_basin[3] != 3))
                    {
                        emit warning_water_basin(4);//水报警
                        net_history(20,2);
                        add_value_controlinfo("4# 防渗池 ","检水报警");
                        flag_output_basin[3] = 3;
                    }
                    if(OIL_BASIN[7]==0x01 && (flag_output_basin[3] == 0 || flag_output_basin[3] != 4))
                    {
                        emit warning_sensor_basin(4);//传感器故障
                        net_history(20,3);
                        add_value_controlinfo("4# 防渗池 ","传感器故障");
                        flag_output_basin[3] = 4;
                    }
                    if(OIL_BASIN[7]==0x04 && (flag_output_basin[3] == 0 || flag_output_basin[3] != 5))
                    {
                        emit warning_uart_basin(4);//通信故障
                        net_history(20,4);
                        add_value_controlinfo("4# 防渗池 ","通信故障");
                        flag_output_basin[3] = 5;
                    }
                }
                if(OIL_BASIN[7] != 0x00)
                {
                    Mythread_basin[4] = 1;
                }
            }
        }
        if(count_basin>=5)             //5#
        {
            if(OIL_BASIN[8]==0xc0)
            {
                if(OIL_BASIN[9]==0x00 && (flag_output_basin[4] == 0 || flag_output_basin[4] != 1))
                {
                    emit right_basin(5);
                    net_history(21,0);
                    add_value_controlinfo("5# 防渗池 ","设备正常");
                    flag_output_basin[4] = 1;
                }
                else
                {
                    if(OIL_BASIN[9]==0x88 && (flag_output_basin[4] == 0 || flag_output_basin[4] != 2))
                    {
                        emit warning_oil_basin(5);//油报警
                        net_history(21,1);
                        add_value_controlinfo("5# 防渗池 ","检油报警");
                        flag_output_basin[4] = 2;
                    }
                    if(OIL_BASIN[9]==0x90 && (flag_output_basin[4] == 0 || flag_output_basin[4] != 3))
                    {
                        emit warning_water_basin(5);//水报警
                        net_history(21,2);
                        add_value_controlinfo("5# 防渗池 ","检水报警");
                        flag_output_basin[4] = 3;
                    }
                    if(OIL_BASIN[9]==0x01 && (flag_output_basin[4] == 0 || flag_output_basin[4] != 4))
                    {
                        emit warning_sensor_basin(5);//传感器故障
                        net_history(21,3);
                        add_value_controlinfo("5# 防渗池 ","传感器故障");
                        flag_output_basin[4] = 4;
                    }
                    if(OIL_BASIN[9]==0x04 && (flag_output_basin[4] == 0 || flag_output_basin[4] != 5))
                    {
                        emit warning_uart_basin(5);//通信故障
                        net_history(21,4);
                        add_value_controlinfo("5# 防渗池 ","通信故障");
                        flag_output_basin[4] = 5;
                    }
                }
                if(OIL_BASIN[9] != 0x00)
                {
                    Mythread_basin[5] = 1;
                }
            }
        }
        if(count_basin>=6)             //6#
        {
            if(OIL_BASIN[10]==0xc0)
            {
                if(OIL_BASIN[11]==0x00 && (flag_output_basin[5] == 0 || flag_output_basin[5] != 1))
                {
                    emit right_basin(6);
                    net_history(22,0);
                    add_value_controlinfo("6# 防渗池 ","设备正常");
                    flag_output_basin[5] = 1;
                }
                else
                {
                    if(OIL_BASIN[11]==0x88 && (flag_output_basin[5] == 0 || flag_output_basin[5] != 2))
                    {
                        emit warning_oil_basin(6);//油报警
                        net_history(22,1);
                        add_value_controlinfo("6# 防渗池 ","检油报警");
                        flag_output_basin[5] = 2;
                    }
                    if(OIL_BASIN[11]==0x90 && (flag_output_basin[5] == 0 || flag_output_basin[5] != 3))
                    {
                        emit warning_water_basin(6);//水报警
                        net_history(22,2);
                        add_value_controlinfo("6# 防渗池 ","检水报警");
                        flag_output_basin[5] = 3;
                    }
                    if(OIL_BASIN[11]==0x01 && (flag_output_basin[5] == 0 || flag_output_basin[5] != 4))
                    {
                        emit warning_sensor_basin(6);//传感器故障
                        net_history(22,3);
                        add_value_controlinfo("6# 防渗池 ","传感器故障");
                        flag_output_basin[5] = 4;
                    }
                    if(OIL_BASIN[11]==0x04 && (flag_output_basin[5] == 0 || flag_output_basin[5] != 5))
                    {
                        emit warning_uart_basin(6);//通信故障
                        net_history(22,4);
                        add_value_controlinfo("6# 防渗池 ","通信故障");
                        flag_output_basin[5] = 5;
                    }
                }
                if(OIL_BASIN[11] != 0x00)
                {
                    Mythread_basin[6] = 1;
                }
            }
        }
        if(count_basin>=7)              //7#
        {
            if(OIL_BASIN[12]==0xc0)
            {
                if(OIL_BASIN[13]==0x00 && (flag_output_basin[6] == 0 || flag_output_basin[6] != 1))
                {
                    emit right_basin(7);
                    net_history(23,0);
                    add_value_controlinfo("7# 防渗池 ","设备正常");
                    flag_output_basin[6] = 1;
                }
                else
                {
                    if(OIL_BASIN[13]==0x88 && (flag_output_basin[6] == 0 || flag_output_basin[6] != 2))
                    {
                        emit warning_oil_basin(7);//油报警
                        net_history(23,1);
                        add_value_controlinfo("7# 防渗池 ","检油报警");
                        flag_output_basin[6] = 2;
                    }
                    if(OIL_BASIN[13]==0x90 && (flag_output_basin[6] == 0 || flag_output_basin[6] != 3))
                    {
                        emit warning_water_basin(7);//水报警
                        net_history(23,2);
                        add_value_controlinfo("7# 防渗池 ","检水报警");
                        flag_output_basin[6] = 3;
                    }
                    if(OIL_BASIN[13]==0x01 && (flag_output_basin[6] == 0 || flag_output_basin[6] != 4))
                    {
                        emit warning_sensor_basin(7);//传感器故障
                        net_history(23,3);
                        add_value_controlinfo("7# 防渗池 ","传感器故障");
                        flag_output_basin[6] = 4;
                    }
                    if(OIL_BASIN[13]==0x04 && (flag_output_basin[6] == 0 || flag_output_basin[6] != 5))
                    {
                        emit warning_uart_basin(7);//通信故障
                        net_history(23,4);
                        add_value_controlinfo("7# 防渗池 ","通信故障");
                        flag_output_basin[6] = 5;
                    }
                }
                if(OIL_BASIN[13] != 0x00)
                {
                    Mythread_basin[7] = 1;
                }
            }
        }
        if(count_basin>=8)           //8#
        {
            if(OIL_BASIN[14]==0xc0)
            {

                if(OIL_BASIN[15]==0x00 && (flag_output_basin[7] == 0 || flag_output_basin[7] != 1))
                {
                    emit right_basin(8);
                    net_history(24,0);
                    add_value_controlinfo("8# 防渗池 ","设备正常");
                    flag_output_basin[7] = 1;
                }
                else
                {
                    if(OIL_BASIN[15]==0x88 && (flag_output_basin[7] == 0 || flag_output_basin[7] != 2))
                    {
                        emit warning_oil_basin(8);//油报警
                        net_history(24,1);
                        add_value_controlinfo("8# 防渗池 ","检油报警");
                        flag_output_basin[7] = 2;
                    }
                    if(OIL_BASIN[15]==0x90 && (flag_output_basin[7] == 0 || flag_output_basin[7] != 3))
                    {
                        emit warning_water_basin(8);//水报警
                        net_history(24,2);
                        add_value_controlinfo("8# 防渗池 ","检水报警");
                        flag_output_basin[7] = 3;
                    }
                    if(OIL_BASIN[15]==0x01 && (flag_output_basin[7] == 0 || flag_output_basin[7] != 4))
                    {
                        emit warning_sensor_basin(8);//传感器故障
                        net_history(24,3);
                        add_value_controlinfo("8# 防渗池 ","传感器故障");
                        flag_output_basin[7] = 4;
                    }
                    if(OIL_BASIN[15]==0x04 && (flag_output_basin[7] == 0 || flag_output_basin[7] != 5))
                    {
                        emit warning_uart_basin(8);//通信故障
                        net_history(24,4);
                        add_value_controlinfo("8# 防渗池 ","通信故障");
                        flag_output_basin[7] = 5;
                    }
                }
                if(OIL_BASIN[15] != 0x00)
                {
                    Mythread_basin[8] = 1;
                }
            }
        }
        if((OIL_BASIN[1]==0x00||OIL_BASIN[1]==0xFF)&&(OIL_BASIN[3]==0x00||OIL_BASIN[3]==0xFF)&&
                (OIL_BASIN[5]==0x00||OIL_BASIN[5]==0xFF)&&(OIL_BASIN[7]==0x00||OIL_BASIN[7]==0xFF)&&
                (OIL_BASIN[9]==0x00||OIL_BASIN[9]==0xFF)&&(OIL_BASIN[11]==0x00||OIL_BASIN[11]==0xFF)&&
                (OIL_BASIN[13]==0x00||OIL_BASIN[13]==0xFF)&&(OIL_BASIN[15]==0x00||OIL_BASIN[15]==0xFF))
        {
            Flag_Sound_Xielou[12] = 0;
            Flag_Sound_Xielou[13] = 0;
            Flag_Sound_Xielou[14] = 0;
            Flag_Sound_Xielou[15] = 0;
        }
        if((OIL_BASIN[1]==0x88)||(OIL_BASIN[3]==0x88)||
                (OIL_BASIN[5]==0x88)||(OIL_BASIN[7]==0x88)||
                (OIL_BASIN[9]==0x88)||(OIL_BASIN[11]==0x88)||
                (OIL_BASIN[13]==0x88)||(OIL_BASIN[15]==0x88))
        {
            Flag_Sound_Xielou[12] = 1;
        }
        else
        {
            Flag_Sound_Xielou[12] = 0;
        }
        if((OIL_BASIN[1]==0x90)||(OIL_BASIN[3]==0x90)||
                (OIL_BASIN[5]==0x90)||(OIL_BASIN[7]==0x90)||
                (OIL_BASIN[9]==0x90)||(OIL_BASIN[11]==0x90)||
                (OIL_BASIN[13]==0x90)||(OIL_BASIN[15]==0x90))
        {
            Flag_Sound_Xielou[13] = 1;
        }
        else
        {
            Flag_Sound_Xielou[13] = 0;
        }
        if((OIL_BASIN[1]==0x01)||(OIL_BASIN[3]==0x01)||
                (OIL_BASIN[5]==0x01)||(OIL_BASIN[7]==0x01)||
                (OIL_BASIN[9]==0x01)||(OIL_BASIN[11]==0x01)||
                (OIL_BASIN[13]==0x01)||(OIL_BASIN[15]==0x01))
        {
            Flag_Sound_Xielou[14] = 1;
        }
        else
        {
            Flag_Sound_Xielou[14] = 0;
        }
        if((OIL_BASIN[1]==0x04)||(OIL_BASIN[3]==0x04)||
                (OIL_BASIN[5]==0x04)||(OIL_BASIN[7]==0x04)||
                (OIL_BASIN[9]==0x04)||(OIL_BASIN[11]==0x04)||
                (OIL_BASIN[13]==0x04)||(OIL_BASIN[15]==0x04))
        {
            Flag_Sound_Xielou[15] = 1;
        }
        else
        {
            Flag_Sound_Xielou[15] = 0;
        }
        if(count_basin <= 0)
        {
            Flag_Sound_Xielou[12] = 0;
            Flag_Sound_Xielou[13] = 0;
            Flag_Sound_Xielou[14] = 0;
            Flag_Sound_Xielou[15] = 0;
        }
//pipe 管线
        if(count_pipe>=1)        //1#
        {
            if(OIL_PIPE[0]==0xc0)
            {
                if(OIL_PIPE[1]==0x00 && (flag_output_pipe[0] == 0 || flag_output_pipe[0] != 1))
                {
                    emit right_pipe(91);
                    net_history(9,0);
                    add_value_controlinfo("1# 管线   ","设备正常");
                    flag_output_pipe[0] = 1;
                }
                else
                {
                    if(OIL_PIPE[1]==0x88 && (flag_output_pipe[0] == 0 || flag_output_pipe[0] != 2))
                    {
                        emit warning_oil_pipe(91);//油报警
                        net_history(9,1);
                        add_value_controlinfo("1# 管线   ","检油报警");
                        flag_output_pipe[0] = 2;
                    }
                    if(OIL_PIPE[1]==0x90 && (flag_output_pipe[0] == 0 || flag_output_pipe[0] != 3))
                    {
                        emit warning_water_pipe(91);//水报警
                        net_history(9,2);
                        add_value_controlinfo("1# 管线   ","检水报警");
                        flag_output_pipe[0] = 3;
                    }
                    if(OIL_PIPE[1]==0x01 && (flag_output_pipe[0] == 0 || flag_output_pipe[0] != 4))
                    {
                        emit warning_sensor_pipe(91);//传感器故障
                        net_history(9,3);
                        add_value_controlinfo("1# 管线   ","传感器故障");
                        flag_output_pipe[0] = 4;
                    }
                    if(OIL_PIPE[1]==0x04 && (flag_output_pipe[0] == 0 || flag_output_pipe[0] != 5))
                    {
                        emit warning_uart_pipe(91);//通信故障
                        net_history(9,4);
                        add_value_controlinfo("1# 管线   ","通信故障");
                        flag_output_pipe[0] = 5;
                    }
                }
                if(OIL_PIPE[1] != 0x00)
                {
                    Mythread_pipe[1] = 1;
                }
            }
        }
        if(count_pipe>=2)       //2#
        {
            if(OIL_PIPE[2]==0xc0)
            {
                if(OIL_PIPE[3]==0x00 && (flag_output_pipe[1] == 0 || flag_output_pipe[1] != 1))
                {
                    emit right_pipe(92);
                    net_history(10,0);
                    add_value_controlinfo("2# 管线   ","设备正常");
                    flag_output_pipe[1] = 1;
                }
                else
                {
                    if(OIL_PIPE[3]==0x88 && (flag_output_pipe[1] == 0 || flag_output_pipe[1] != 2))
                    {
                        emit warning_oil_pipe(92);//油报警
                        net_history(10,1);
                        add_value_controlinfo("2# 管线   ","检油报警");
                        flag_output_pipe[1] = 2;
                    }
                    if(OIL_PIPE[3]==0x90 && (flag_output_pipe[1] == 0 || flag_output_pipe[1] != 3))
                    {
                        emit warning_water_pipe(92);//水报警
                        net_history(10,2);
                        add_value_controlinfo("2# 管线   ","检水报警");
                        flag_output_pipe[1] = 3;
                    }
                    if(OIL_PIPE[3]==0x01 && (flag_output_pipe[1] == 0 || flag_output_pipe[1] != 4))
                    {
                        emit warning_sensor_pipe(92);//传感器故障
                        net_history(10,3);
                        add_value_controlinfo("2# 管线   ","传感器故障");
                        flag_output_pipe[1] = 4;
                    }
                    if(OIL_PIPE[3]==0x04 && (flag_output_pipe[1] == 0 || flag_output_pipe[1] != 5))
                    {
                        emit warning_uart_pipe(92);//通信故障
                        net_history(10,4);
                        add_value_controlinfo("2# 管线   ","通信故障");
                        flag_output_pipe[1] = 5;
                    }
                }
                if(OIL_PIPE[3] != 0x00)
                {
                    Mythread_pipe[2] = 1;
                }
            }
        }
        if(count_pipe>=3)           //3#
        {
            if(OIL_PIPE[4]==0xc0)
            {
                if(OIL_PIPE[5]==0x00 && (flag_output_pipe[2] == 0 || flag_output_pipe[2] != 1))
                {
                    emit right_pipe(93);
                    net_history(11,0);
                    add_value_controlinfo("3# 管线   ","设备正常");
                    flag_output_pipe[2] = 1;
                }
                else
                {
                    if(OIL_PIPE[5]==0x88 && (flag_output_pipe[2] == 0 || flag_output_pipe[2] != 2))
                    {
                        emit warning_oil_pipe(93);//油报警
                        net_history(11,1);
                        add_value_controlinfo("3# 管线   ","检油报警");
                        flag_output_pipe[2] = 2;
                    }
                    if(OIL_PIPE[5]==0x90 && (flag_output_pipe[2] == 0 || flag_output_pipe[2] != 3))
                    {
                        emit warning_water_pipe(93);//水报警
                        net_history(11,2);
                        add_value_controlinfo("3# 管线   ","检水报警");
                        flag_output_pipe[2] = 3;
                    }
                    if(OIL_PIPE[5]==0x01 && (flag_output_pipe[2] == 0 || flag_output_pipe[2] != 4))
                    {
                        emit warning_sensor_pipe(93);//传感器故障
                        net_history(11,3);
                        add_value_controlinfo("3# 管线   ","传感器故障");
                        flag_output_pipe[2] = 4;
                    }
                    if(OIL_PIPE[5]==0x04 && (flag_output_pipe[2] == 0 || flag_output_pipe[2] != 5))
                    {
                        emit warning_uart_pipe(93);//通信故障
                        net_history(11,4);
                        add_value_controlinfo("3# 管线   ","通信故障");
                        flag_output_pipe[2] = 5;
                    }
                }
                if(OIL_PIPE[5] != 0x00)
                {
                    Mythread_pipe[3] = 1;
                }
            }
        }
        if(count_pipe>=4)       //4#
        {
            if(OIL_PIPE[6]==0xc0)
            {

                if(OIL_PIPE[7]==0x00 && (flag_output_pipe[3] == 0 || flag_output_pipe[3] != 1))
                {
                    emit right_pipe(94);
                    net_history(12,0);
                    add_value_controlinfo("4# 管线   ","设备正常");
                    flag_output_pipe[3] = 1;
                }
                else
                {
                    if(OIL_PIPE[7]==0x88 && (flag_output_pipe[3] == 0 || flag_output_pipe[3] != 2))
                    {
                        emit warning_oil_pipe(94);//油报警
                        net_history(12,1);
                        add_value_controlinfo("4# 管线   ","检油报警");
                        flag_output_pipe[3] = 2;
                    }
                    if(OIL_PIPE[7]==0x90 && (flag_output_pipe[3] == 0 || flag_output_pipe[3] != 3))
                    {
                        emit warning_water_pipe(94);//水报警
                        net_history(12,2);
                        add_value_controlinfo("4# 管线   ","检水报警");
                        flag_output_pipe[3] = 3;
                    }
                    if(OIL_PIPE[7]==0x01 && (flag_output_pipe[3] == 0 || flag_output_pipe[3] != 4))
                    {
                        emit warning_sensor_pipe(94);//传感器故障
                        net_history(12,3);
                        add_value_controlinfo("4# 管线   ","传感器故障");
                        flag_output_pipe[3] = 4;
                    }
                    if(OIL_PIPE[7]==0x04 && (flag_output_pipe[3] == 0 || flag_output_pipe[3] != 5))
                    {
                        emit warning_uart_pipe(94);//通信故障
                        net_history(12,4);
                        add_value_controlinfo("4# 管线   ","通信故障");
                        flag_output_pipe[3] = 5;
                    }
                }
                if(OIL_PIPE[7] != 0x00)
                {
                    Mythread_pipe[4] = 1;
                }
            }
        }
        if(count_pipe>=5)       //5#
        {
            if(OIL_PIPE[8]==0xc0)
            {

                if(OIL_PIPE[9]==0x00 && (flag_output_pipe[4] == 0 || flag_output_pipe[4] != 1))
                {
                    emit right_pipe(95);
                    net_history(13,0);
                    add_value_controlinfo("5# 管线   ","设备正常");
                    flag_output_pipe[4] = 1;
                }
                else
                {
                    if(OIL_PIPE[9]==0x88 && (flag_output_pipe[4] == 0 || flag_output_pipe[4] != 2))
                    {
                        emit warning_oil_pipe(95);//油报警
                        net_history(13,1);
                        add_value_controlinfo("5# 管线   ","检油报警");
                        flag_output_pipe[4] = 2;
                    }
                    if(OIL_PIPE[9]==0x90 && (flag_output_pipe[4] == 0 || flag_output_pipe[4] != 3))
                    {
                        emit warning_water_pipe(95);//水报警
                        net_history(13,2);
                        add_value_controlinfo("5# 管线   ","检水报警");
                        flag_output_pipe[4] = 3;
                    }
                    if(OIL_PIPE[9]==0x01 && (flag_output_pipe[4] == 0 || flag_output_pipe[4] != 4))
                    {
                        emit warning_sensor_pipe(95);//传感器故障
                        net_history(13,3);
                        add_value_controlinfo("5# 管线   ","传感器故障");
                        flag_output_pipe[4] = 4;
                    }
                    if(OIL_PIPE[9]==0x04 && (flag_output_pipe[4] == 0 || flag_output_pipe[4] != 5))
                    {
                        emit warning_uart_pipe(95);//通信故障
                        net_history(13,4);
                        add_value_controlinfo("5# 管线   ","通信故障");
                        flag_output_pipe[4] = 5;
                    }
                }
                if(OIL_PIPE[9] != 0x00)
                {
                    Mythread_pipe[5] = 1;
                }
            }
        }
        if(count_pipe>=6)           //6#
        {
            if(OIL_PIPE[10]==0xc0)
            {

                if(OIL_PIPE[11]==0x00 && (flag_output_pipe[5] == 0 || flag_output_pipe[5] != 1))
                {
                    emit right_pipe(96);
                    net_history(14,0);
                    add_value_controlinfo("6# 管线   ","设备正常");
                    flag_output_pipe[5] = 1;
                }
                else
                {
                    if(OIL_PIPE[11]==0x88 && (flag_output_pipe[5] == 0 || flag_output_pipe[5] != 2))
                    {
                        emit warning_oil_pipe(96);//油报警
                        net_history(14,1);
                        add_value_controlinfo("6# 管线   ","检油报警");
                        flag_output_pipe[5] = 2;
                    }
                    if(OIL_PIPE[11]==0x90 && (flag_output_pipe[5] == 0 || flag_output_pipe[5] != 3))
                    {
                        emit warning_water_pipe(96);//水报警
                        net_history(14,2);
                        add_value_controlinfo("6# 管线   ","检水报警");
                        flag_output_pipe[5] = 3;
                    }
                    if(OIL_PIPE[11]==0x01 && (flag_output_pipe[5] == 0 || flag_output_pipe[5] != 4))
                    {
                        emit warning_sensor_pipe(96);//传感器故障
                        net_history(14,3);
                        add_value_controlinfo("6# 管线   ","传感器故障");
                        flag_output_pipe[5] = 4;
                    }
                    if(OIL_PIPE[11]==0x04 && (flag_output_pipe[5] == 0 || flag_output_pipe[5] != 5))
                    {
                        emit warning_uart_pipe(96);//通信故障
                        net_history(14,4);
                        add_value_controlinfo("6# 管线   ","通信故障");
                        flag_output_pipe[5] = 5;
                    }
                }
                if(OIL_PIPE[11] != 0x00)
                {
                    Mythread_pipe[6] = 1;
                }
            }
        }
        if(count_pipe>=7)            //7#
        {
            if(OIL_PIPE[12]==0xc0)
            {

                if(OIL_PIPE[13]==0x00 && (flag_output_pipe[6] == 0 || flag_output_pipe[6] != 1))
                {
                    emit right_pipe(97);
                    net_history(15,0);
                    add_value_controlinfo("7# 管线   ","设备正常");
                    flag_output_pipe[6] = 1;
                }
                else
                {
                    if(OIL_PIPE[13]==0x88 && (flag_output_pipe[6] == 0 || flag_output_pipe[6] != 2))
                    {
                        emit warning_oil_pipe(97);//油报警
                        net_history(15,1);
                        add_value_controlinfo("7# 管线   ","检油报警");
                        flag_output_pipe[6] = 2;
                    }
                    if(OIL_PIPE[13]==0x90 && (flag_output_pipe[6] == 0 || flag_output_pipe[6] != 3))
                    {
                        emit warning_water_pipe(97);//水报警
                        net_history(15,2);
                        add_value_controlinfo("7# 管线   ","检水报警");
                        flag_output_pipe[6] = 3;
                    }
                    if(OIL_PIPE[13]==0x01 && (flag_output_pipe[6] == 0 || flag_output_pipe[6] != 4))
                    {
                        emit warning_sensor_pipe(97);//传感器故障
                        net_history(15,3);
                        add_value_controlinfo("7# 管线   ","传感器故障");
                        flag_output_pipe[6] = 4;
                    }
                    if(OIL_PIPE[13]==0x04 && (flag_output_pipe[6] == 0 || flag_output_pipe[6] != 5))
                    {
                        emit warning_uart_pipe(97);//通信故障
                        net_history(15,4);
                        add_value_controlinfo("7# 管线   ","通信故障");
                        flag_output_pipe[6] = 5;
                    }
                }
                if(OIL_PIPE[13] != 0x00)
                {
                    Mythread_pipe[7] = 1;
                }
            }
        }
        if(count_pipe>=8)           //8#
        {
            if(OIL_PIPE[14]==0xc0)
            {

                if(OIL_PIPE[15]==0x00 && (flag_output_pipe[7] == 0 || flag_output_pipe[7] != 1))
                {
                    emit right_pipe(98);
                    net_history(16,0);
                    add_value_controlinfo("8# 管线   ","设备正常");
                    flag_output_pipe[7] = 1;
                }
                else
                {
                    if(OIL_PIPE[15]==0x88 && (flag_output_pipe[7] == 0 || flag_output_pipe[7] != 2))
                    {
                        emit warning_oil_pipe(98);//油报警
                        net_history(16,1);
                        add_value_controlinfo("8# 管线   ","检油报警");
                        flag_output_pipe[7] = 2;
                    }
                    if(OIL_PIPE[15]==0x90 && (flag_output_pipe[7] == 0 || flag_output_pipe[7] != 3))
                    {
                        emit warning_water_pipe(98);//水报警
                        net_history(16,2);
                        add_value_controlinfo("8# 管线   ","检水报警");
                        flag_output_pipe[7] = 3;
                    }
                    if(OIL_PIPE[15]==0x01 && (flag_output_pipe[7] == 0 || flag_output_pipe[7] != 4))
                    {
                        emit warning_sensor_pipe(98);//传感器故障
                        net_history(16,3);
                        add_value_controlinfo("8# 管线   ","传感器故障");
                        flag_output_pipe[7] = 4;
                    }
                    if(OIL_PIPE[15]==0x04 && (flag_output_pipe[7] == 0 || flag_output_pipe[7] != 5))
                    {
                        emit warning_uart_pipe(98);//通信故障
                        net_history(16,4);
                        add_value_controlinfo("8# 管线   ","通信故障");
                        flag_output_pipe[7] = 5;
                    }
                }
                if(OIL_PIPE[15] != 0x00)
                {
                    Mythread_pipe[8] = 1;
                }
            }
        }
        if((OIL_PIPE[1]==0x00||OIL_PIPE[1]==0xFF)&&(OIL_PIPE[3]==0x00||OIL_PIPE[3]==0xFF)&&
            (OIL_PIPE[5]==0x00||OIL_PIPE[5]==0xFF)&&(OIL_PIPE[7]==0x00||OIL_PIPE[7]==0xFF)&&
            (OIL_PIPE[9]==0x00||OIL_PIPE[9]==0xFF)&&(OIL_PIPE[11]==0x00||OIL_PIPE[11]==0xFF)&&
            (OIL_PIPE[13]==0x00||OIL_PIPE[13]==0xFF)&&(OIL_PIPE[15]==0x00||OIL_PIPE[15]==0xFF))
        {
            Flag_Sound_Xielou[0] = 0;
            Flag_Sound_Xielou[1] = 0;
            Flag_Sound_Xielou[2] = 0;
            Flag_Sound_Xielou[3] = 0;
        }
        if((OIL_PIPE[1]==0x88)||(OIL_PIPE[3]==0x88)||
                (OIL_PIPE[5]==0x88)||(OIL_PIPE[7]==0x88)||
                (OIL_PIPE[9]==0x88)||(OIL_PIPE[11]==0x88)||
                (OIL_PIPE[13]==0x88)||(OIL_PIPE[15]==0x88))
        {
            Flag_Sound_Xielou[0] = 1;
        }
        else
        {
            Flag_Sound_Xielou[0] = 0;
        }
        if((OIL_PIPE[1]==0x90)||(OIL_PIPE[3]==0x90)||
                (OIL_PIPE[5]==0x90)||(OIL_PIPE[7]==0x90)||
                (OIL_PIPE[9]==0x90)||(OIL_PIPE[11]==0x90)||
                (OIL_PIPE[13]==0x90)||(OIL_PIPE[15]==0x90))
        {
            Flag_Sound_Xielou[1] = 1;
        }
        else
        {
            Flag_Sound_Xielou[1] = 0;
        }
        if((OIL_PIPE[1]==0x01)||(OIL_PIPE[3]==0x01)||
                (OIL_PIPE[5]==0x01)||(OIL_PIPE[7]==0x01)||
                (OIL_PIPE[9]==0x01)||(OIL_PIPE[11]==0x01)||
                (OIL_PIPE[13]==0x01)||(OIL_PIPE[15]==0x01))
        {
            Flag_Sound_Xielou[2] = 1;
        }
        else
        {
            Flag_Sound_Xielou[2] = 0;
        }
        if((OIL_PIPE[1]==0x04)||(OIL_PIPE[3]==0x04)||
                (OIL_PIPE[5]==0x04)||(OIL_PIPE[7]==0x04)||
                (OIL_PIPE[9]==0x04)||(OIL_PIPE[11]==0x04)||
                (OIL_PIPE[13]==0x04)||(OIL_PIPE[15]==0x04))
        {
            Flag_Sound_Xielou[3] = 1;
        }
        else
        {
            Flag_Sound_Xielou[3] = 0;
        }
        if(count_pipe<=0)
        {
            Flag_Sound_Xielou[0] = 0;
            Flag_Sound_Xielou[1] = 0;
            Flag_Sound_Xielou[2] = 0;
            Flag_Sound_Xielou[3] = 0;
        }

//tank 罐
        if((OIL_TANK[0]&0xf0)==0xc0)      //传感器法
        {

            //对非传感器法的语音标志位进行清零
            Flag_Sound_Xielou[8] = 0;
            Flag_Sound_Xielou[9] = 0;
            Flag_Sound_Xielou[10] = 0;
            Flag_Sound_Xielou[11] = 0;


            if(count_tank>=1)           //1#
            {
                if((OIL_TANK[0]&0x0f)==0)
                {
                    if(OIL_TANK[1]==0 && (flag_output_tank[0] == 0 || flag_output_tank[0] != 1))
                    {
                        emit right_tank(71);
                        net_history(1,0);
                        add_value_controlinfo("1# 油罐   ","设备正常");
                        flag_output_tank[0] = 1;
                    }
                    else
                    {
                        if(OIL_TANK[1]==0x88 && (flag_output_tank[0] == 0 || flag_output_tank[0] != 2))
                        {
                            emit warning_oil_tank(71);//油报警
                            net_history(1,1);
                            add_value_controlinfo("1# 油罐   ","检油报警");
                            flag_output_tank[0] = 2;
                        }
                        if(OIL_TANK[1]==0x90 && (flag_output_tank[0] == 0 || flag_output_tank[0] != 3))
                        {
                            emit warning_water_tank(71);//水报警
                            net_history(1,2);
                            add_value_controlinfo("1# 油罐   ","检水报警");
                            flag_output_tank[0] = 3;
                        }
                        if(OIL_TANK[1]==0x01 && (flag_output_tank[0] == 0 || flag_output_tank[0] != 4))
                        {
                            emit warning_sensor_tank(71);//传感器故障
                            net_history(1,3);
                            add_value_controlinfo("1# 油罐   ","传感器故障");
                            flag_output_tank[0] = 4;
                        }
                        if(OIL_TANK[1]==0x04 && (flag_output_tank[0] == 0 || flag_output_tank[0] != 5))
                        {
                            emit warning_uart_tank(71);//通信故障
                            net_history(1,4);
                            add_value_controlinfo("1# 油罐   ","通信故障");
                            flag_output_tank[0] = 5;
                        }
                    }
                    if(OIL_TANK[1] != 0x00)
                    {
                        Mythread_tank[1] = 1;
                    }
                }
            }
            if(count_tank>=2)            //2#
            {
                if((OIL_TANK[2]&0x0f)==0)
                {
                    if(OIL_TANK[3]==0 && (flag_output_tank[1] == 0 || flag_output_tank[1] != 1))
                    {
                        emit right_tank(72);
                        net_history(2,0);
                        add_value_controlinfo("2# 油罐   ","设备正常");
                        flag_output_tank[1] = 1;
                    }
                    else
                    {
                        if(OIL_TANK[3]==0x88 && (flag_output_tank[1] == 0 || flag_output_tank[1] != 2))
                        {
                            emit warning_oil_tank(72);//油报警
                            net_history(2,1);
                            add_value_controlinfo("2# 油罐   ","检油报警");
                            flag_output_tank[1] = 2;
                        }
                        if(OIL_TANK[3]==0x90 && (flag_output_tank[1] == 0 || flag_output_tank[1] != 3))
                        {
                            emit warning_water_tank(72);//水报警
                            net_history(2,2);
                            add_value_controlinfo("2# 油罐   ","检水报警");
                            flag_output_tank[1] = 3;
                        }
                        if(OIL_TANK[3]==0x01 && (flag_output_tank[1] == 0 || flag_output_tank[1] != 4))
                        {
                            emit warning_sensor_tank(72);//传感器故障
                            net_history(2,3);
                            add_value_controlinfo("2# 油罐   ","传感器故障");
                            flag_output_tank[1] = 4;
                        }
                        if(OIL_TANK[3]==0x04 && (flag_output_tank[1] == 0 || flag_output_tank[1] != 5))
                        {
                            emit warning_uart_tank(72);//通信故障
                            net_history(2,4);
                            add_value_controlinfo("2# 油罐   ","通信故障");
                            flag_output_tank[1] = 5;
                        }
                    }
                    if(OIL_TANK[3] != 0x00)
                    {
                        Mythread_tank[2] = 1;
                    }
                }
            }
            if(count_tank>=3)            //3#
            {
                if((OIL_TANK[4]&0x0f)==0)
                {
                    if(OIL_TANK[5]==0 && (flag_output_tank[2] == 0 || flag_output_tank[2] != 1))
                    {
                        emit right_tank(73);
                        net_history(3,0);
                        add_value_controlinfo("3# 油罐   ","设备正常");
                        flag_output_tank[2] = 1;
                    }
                    else
                    {
                        if(OIL_TANK[5]==0x88 && (flag_output_tank[2] == 0 || flag_output_tank[2] != 2))
                        {
                            emit warning_oil_tank(73);//油报警
                            net_history(3,1);
                            add_value_controlinfo("3# 油罐   ","检油报警");
                            flag_output_tank[2] = 2;
                        }
                        if(OIL_TANK[5]==0x90 && (flag_output_tank[2] == 0 || flag_output_tank[2] != 3))
                        {
                            emit warning_water_tank(73);//水报警
                            net_history(3,2);
                            add_value_controlinfo("3# 油罐   ","检水报警");
                            flag_output_tank[2] = 3;
                        }
                        if(OIL_TANK[5]==0x01 && (flag_output_tank[2] == 0 || flag_output_tank[2] != 4))
                        {
                            emit warning_sensor_tank(73);//传感器故障
                            net_history(3,3);
                            add_value_controlinfo("3# 油罐   ","传感器故障");
                            flag_output_tank[2] = 4;
                        }
                        if(OIL_TANK[5]==0x04 && (flag_output_tank[2] == 0 || flag_output_tank[2] != 5))
                        {
                            emit warning_uart_tank(73);//通信故障
                            net_history(3,4);
                            add_value_controlinfo("3# 油罐   ","通信故障");
                            flag_output_tank[2] = 5;
                        }
                    }
                    if(OIL_TANK[5] != 0x00)
                    {
                        Mythread_tank[3] = 1;
                    }
                }
            }
            if(count_tank>=4)           //4#
            {
                if((OIL_TANK[6]&0x0f)==0)
                {
                    if(OIL_TANK[7]==0 && (flag_output_tank[3] == 0 || flag_output_tank[3] != 1))
                    {
                        emit right_tank(74);
                        net_history(4,0);
                        add_value_controlinfo("4# 油罐   ","设备正常");
                        flag_output_tank[3] = 1;
                    }
                    else
                    {
                        if(OIL_TANK[7]==0x88 && (flag_output_tank[3] == 0 || flag_output_tank[3] != 2))
                        {
                            emit warning_oil_tank(74);//油报警
                            net_history(4,1);
                            add_value_controlinfo("4# 油罐   ","检油报警");
                            flag_output_tank[3] = 2;
                        }
                        if(OIL_TANK[7]==0x90 && (flag_output_tank[3] == 0 || flag_output_tank[3] != 3))
                        {
                            emit warning_water_tank(74);//水报警
                            net_history(4,2);
                            add_value_controlinfo("4# 油罐   ","检水报警");
                            flag_output_tank[3] = 3;
                        }
                        if(OIL_TANK[7]==0x01 && (flag_output_tank[3] == 0 || flag_output_tank[3] != 4))
                        {
                            emit warning_sensor_tank(74);//传感器故障
                            net_history(4,3);
                            add_value_controlinfo("4# 油罐   ","传感器故障");
                            flag_output_tank[3] = 4;
                        }
                        if(OIL_TANK[7]==0x04 && (flag_output_tank[3] == 0 || flag_output_tank[3] != 5))
                        {
                            emit warning_uart_tank(74);//通信故障
                            net_history(4,4);
                            add_value_controlinfo("4# 油罐   ","通信故障");
                            flag_output_tank[3] = 5;
                        }
                    }
                    if(OIL_TANK[7] != 0x00)
                    {
                        Mythread_tank[4] = 1;
                    }
                }
            }
            if(count_tank>=5)       //5#
            {
                if((OIL_TANK[8]&0x0f)==0)
                {
                    if(OIL_TANK[9]==0 && (flag_output_tank[4] == 0 || flag_output_tank[4] != 1))
                    {
                        emit right_tank(75);
                        net_history(5,0);
                        add_value_controlinfo("5# 油罐   ","设备正常");
                        flag_output_tank[4] = 1;
                    }
                    else
                    {
                        if(OIL_TANK[9]==0x88 && (flag_output_tank[4] == 0 || flag_output_tank[4] != 2))
                        {
                            emit warning_oil_tank(75);//油报警
                            net_history(5,1);
                            add_value_controlinfo("5# 油罐   ","检油报警");
                            flag_output_tank[4] = 2;
                        }
                        if(OIL_TANK[9]==0x90 && (flag_output_tank[4] == 0 || flag_output_tank[4] != 3))
                        {
                            emit warning_water_tank(75);//水报警
                            net_history(5,2);
                            add_value_controlinfo("5# 油罐   ","检水报警");
                            flag_output_tank[4] = 3;
                        }
                        if(OIL_TANK[9]==0x01 && (flag_output_tank[4] == 0 || flag_output_tank[4] != 4))
                        {
                            emit warning_sensor_tank(75);//传感器故障
                            net_history(5,3);
                            add_value_controlinfo("5# 油罐   ","传感器故障");
                            flag_output_tank[4] = 4;
                        }
                        if(OIL_TANK[9]==0x04 && (flag_output_tank[4] == 0 || flag_output_tank[4] != 5))
                        {
                            emit warning_uart_tank(75);//通信故障
                            net_history(5,4);
                            add_value_controlinfo("5# 油罐   ","通信故障");
                            flag_output_tank[4] = 5;
                        }
                    }
                    if(OIL_TANK[9] != 0x00)
                    {
                        Mythread_tank[5] = 1;
                    }
                }
            }
            if(count_tank>=6)         //6#
            {
                if((OIL_TANK[10]&0x0f)==0)
                {
                    if(OIL_TANK[11]==0 && (flag_output_tank[5] == 0 || flag_output_tank[5] != 1))
                    {
                        emit right_tank(76);
                        net_history(6,0);
                        add_value_controlinfo("6# 油罐   ","设备正常");
                        flag_output_tank[5] = 1;
                    }
                    else
                    {
                        if(OIL_TANK[11]==0x88 && (flag_output_tank[5] == 0 || flag_output_tank[5] != 2))
                        {
                            emit warning_oil_tank(76);//油报警
                            net_history(6,1);
                            add_value_controlinfo("6# 油罐   ","检油报警");
                            flag_output_tank[5] = 2;
                        }
                        if(OIL_TANK[11]==0x90 && (flag_output_tank[5] == 0 || flag_output_tank[5] != 3))
                        {
                            emit warning_water_tank(76);//水报警
                            net_history(6,2);
                            add_value_controlinfo("6# 油罐   ","检水报警");
                            flag_output_tank[5] = 3;
                        }
                        if(OIL_TANK[11]==0x01 && (flag_output_tank[5] == 0 || flag_output_tank[5] != 4))
                        {
                            emit warning_sensor_tank(76);//传感器故障
                            net_history(6,3);
                            add_value_controlinfo("6# 油罐   ","传感器故障");
                            flag_output_tank[5] = 4;
                        }
                        if(OIL_TANK[11]==0x04 && (flag_output_tank[5] == 0 || flag_output_tank[5] != 5))
                        {
                            emit warning_uart_tank(76);//通信故障
                            net_history(6,4);
                            add_value_controlinfo("6# 油罐   ","通信故障");
                            flag_output_tank[5] = 5;
                        }
                    }
                    if(OIL_TANK[11] != 0x00)
                    {
                        Mythread_tank[6] = 1;
                    }
                }
            }
            if(count_tank>=7)         //7#
            {
                if((OIL_TANK[12]&0x0f)==0)
                {
                    if(OIL_TANK[13]==0 && (flag_output_tank[6] == 0 || flag_output_tank[6] != 1))
                    {
                        emit right_tank(77);
                        net_history(7,0);
                        add_value_controlinfo("7# 油罐   ","设备正常");
                        flag_output_tank[6] = 1;
                    }
                    else
                    {
                        if(OIL_TANK[13]==0x88 && (flag_output_tank[6] == 0 || flag_output_tank[6] != 2))
                        {
                            emit warning_oil_tank(77);//油报警
                            net_history(7,1);
                            add_value_controlinfo("7# 油罐   ","检油报警");
                            flag_output_tank[6] = 2;
                        }
                        if(OIL_TANK[13]==0x90 && (flag_output_tank[6] == 0 || flag_output_tank[6] != 3))
                        {
                            emit warning_water_tank(77);//水报警
                            net_history(7,2);
                            add_value_controlinfo("7# 油罐   ","检水报警");
                            flag_output_tank[6] = 3;
                        }
                        if(OIL_TANK[13]==0x01 && (flag_output_tank[6] == 0 || flag_output_tank[6] != 4))
                        {
                            emit warning_sensor_tank(77);//传感器故障
                            net_history(7,3);
                            add_value_controlinfo("7# 油罐   ","传感器故障");
                            flag_output_tank[6] = 4;
                        }
                        if(OIL_TANK[13]==0x04 && (flag_output_tank[6] == 0 || flag_output_tank[6] != 5))
                        {
                            emit warning_uart_tank(77);//通信故障
                            net_history(7,4);
                            add_value_controlinfo("7# 油罐   ","通信故障");
                            flag_output_tank[6] = 5;
                        }
                    }
                    if(OIL_TANK[13] != 0x00)
                    {
                        Mythread_tank[7] = 1;
                    }
                }
            }
            if(count_tank>=8)     //8#
            {
                if((OIL_TANK[14]&0x0f)==0)
                {
                    if(OIL_TANK[15]==0 && (flag_output_tank[7] == 0 || flag_output_tank[7] != 1))
                    {
                        emit right_tank(78);
                        net_history(8,0);
                        add_value_controlinfo("8# 油罐   ","设备正常");
                        flag_output_tank[7] = 1;
                    }
                    else
                    {
                        if(OIL_TANK[15]==0x88 && (flag_output_tank[7] == 0 || flag_output_tank[7] != 2))
                        {
                            emit warning_oil_tank(78);//油报警
                            net_history(8,1);
                            add_value_controlinfo("8# 油罐   ","检油报警");
                            flag_output_tank[7] = 2;
                        }
                        if(OIL_TANK[15]==0x90 && (flag_output_tank[7] == 0 || flag_output_tank[7] != 3))
                        {
                            emit warning_water_tank(78);//水报警
                            net_history(8,2);
                            add_value_controlinfo("8# 油罐   ","检水报警");
                            flag_output_tank[7] = 3;
                        }
                        if(OIL_TANK[15]==0x01 && (flag_output_tank[7] == 0 || flag_output_tank[7] != 4))
                        {
                            emit warning_sensor_tank(78);//传感器故障
                            net_history(8,3);
                            add_value_controlinfo("8# 油罐   ","传感器故障");
                            flag_output_tank[7] = 4;
                        }
                        if(OIL_TANK[15]==0x04 && (flag_output_tank[7] == 0 || flag_output_tank[7] != 5))
                        {
                            emit warning_uart_tank(78);//通信故障
                            net_history(8,4);
                            add_value_controlinfo("8# 油罐   ","通信故障");
                            flag_output_tank[7] = 5;
                        }
                    }
                    if(OIL_TANK[15] != 0x00)
                    {
                        Mythread_tank[8] = 1;
                    }
                }
            }
            //如果8个油罐均为正常或者未设置，则对报警标志位进行清零
            if((OIL_TANK[1]==0x00||OIL_TANK[1]==0xFF)&&(OIL_TANK[3]==0x00||OIL_TANK[3]==0xFF)&&
                (OIL_TANK[5]==0x00||OIL_TANK[5]==0xFF)&&(OIL_TANK[7]==0x00||OIL_TANK[7]==0xFF)&&
                (OIL_TANK[9]==0x00||OIL_TANK[9]==0xFF)&&(OIL_TANK[11]==0x00||OIL_TANK[11]==0xFF)&&
                (OIL_TANK[13]==0x00||OIL_TANK[13]==0xFF)&&(OIL_TANK[15]==0x00||OIL_TANK[15]==0xFF))
            {
                Flag_Sound_Xielou[4] = 0;  //oil
                Flag_Sound_Xielou[5] = 0;  //water
                Flag_Sound_Xielou[6] = 0;  //sensor
                Flag_Sound_Xielou[7] = 0;  //uart
                Flag_Sound_Xielou[8] = 0;  //pre
                Flag_Sound_Xielou[9] = 0;  //warn
                Flag_Sound_Xielou[10] = 0; //high
                Flag_Sound_Xielou[11] = 0; //low
            }
            //如果有一个为油报警，则油报警标志位置1;全部都不是油报警，则进行检油标志位清零
            if((OIL_TANK[1]==0x88)||(OIL_TANK[3]==0x88)||
                    (OIL_TANK[5]==0x88)||(OIL_TANK[7]==0x88)||
                    (OIL_TANK[9]==0x88)||(OIL_TANK[11]==0x88)||
                    (OIL_TANK[13]==0x88)||(OIL_TANK[15]==0x88))
            {
                Flag_Sound_Xielou[4] = 1;
            }
            else
            {
                Flag_Sound_Xielou[4] = 0;
            }
            //如果有一个为水报警，则水报警标志位置1;全部都不是水报警，则进行检水标志位清零
            if((OIL_TANK[1]==0x90)||(OIL_TANK[3]==0x90)||
                    (OIL_TANK[5]==0x90)||(OIL_TANK[7]==0x90)||
                    (OIL_TANK[9]==0x90)||(OIL_TANK[11]==0x90)||
                    (OIL_TANK[13]==0x90)||(OIL_TANK[15]==0x90))
            {
                Flag_Sound_Xielou[5] = 1;
            }
            else
            {
                Flag_Sound_Xielou[5] = 0;
            }
            //如果有一个为传感器故障，则传感器故障标志位置1;全部都不是传感器故障，则进行传感器故障标志位清零
            if((OIL_TANK[1]==0x01)||(OIL_TANK[3]==0x01)||
                    (OIL_TANK[5]==0x01)||(OIL_TANK[7]==0x01)||
                    (OIL_TANK[9]==0x01)||(OIL_TANK[11]==0x01)||
                    (OIL_TANK[13]==0x01)||(OIL_TANK[15]==0x01))
            {
                Flag_Sound_Xielou[6] = 1;
            }
            else
            {
                Flag_Sound_Xielou[6] = 0;
            }
            //如果有一个为通信故障，则通信故障标志位置1;全部都不是通信故障，则进行通信故障标志位清零
            if((OIL_TANK[1]==0x04)||(OIL_TANK[3]==0x04)||
                    (OIL_TANK[5]==0x04)||(OIL_TANK[7]==0x04)||
                    (OIL_TANK[9]==0x04)||(OIL_TANK[11]==0x04)||
                    (OIL_TANK[13]==0x04)||(OIL_TANK[15]==0x04))
            {
                Flag_Sound_Xielou[7] = 1;
            }
            else
            {
                Flag_Sound_Xielou[7] = 0;
            }
            //如果油罐数量为零，则进行标志位清零
            if(count_tank<=0)
            {
                Flag_Sound_Xielou[4] = 0;  //oil
                Flag_Sound_Xielou[5] = 0;  //water
                Flag_Sound_Xielou[6] = 0;  //sensor
                Flag_Sound_Xielou[7] = 0;  //uart
                Flag_Sound_Xielou[8] = 0;  //pre
                Flag_Sound_Xielou[9] = 0;  //warn
                Flag_Sound_Xielou[10] = 0; //high
                Flag_Sound_Xielou[11] = 0; //low
            }
        }

        if((OIL_TANK[0]&0xf0)==0x80)      //液媒法
        {
            //对非液媒法法的语音标志位进行清零
            Flag_Sound_Xielou[8] = 0;
            Flag_Sound_Xielou[9] = 0;
            Flag_Sound_Xielou[4] = 0;
            Flag_Sound_Xielou[5] = 0;

            if(count_tank >= 1)     //1#
            {
                if((OIL_TANK[0]&0x0f)==0)
                {
                    if(OIL_TANK[1]==0 && (flag_output_tank[0] == 0 || flag_output_tank[0] != 1))
                    {
                        emit right_tank(71);
                        net_history(1,0);
                        add_value_controlinfo("1# 油罐   ","设备正常");
                        flag_output_tank[0] = 1;
                    }
                    else
                    {
                        if(OIL_TANK[1]==0xa0 && (flag_output_tank[0] == 0 || flag_output_tank[0] != 2))
                        {
                            emit warning_high_tank(71);//高报警
                            net_history(1,5);
                            add_value_controlinfo("1# 油罐   ","高液位报警");
                            flag_output_tank[0] = 2;
                        }
                        if(OIL_TANK[1]==0xc0 && (flag_output_tank[0] == 0 || flag_output_tank[0] != 3))
                        {
                            emit warning_low_tank(71);//低报警
                            net_history(1,6);
                            add_value_controlinfo("1# 油罐   ","低液位报警");
                            flag_output_tank[0] = 3;
                        }
                        if(OIL_TANK[1]==0x01 && (flag_output_tank[0] == 0 || flag_output_tank[0] != 4))
                        {
                            emit warning_sensor_tank(71);//传感器故障
                            net_history(1,3);
                            add_value_controlinfo("1# 油罐   ","传感器故障");
                            flag_output_tank[0] = 4;
                        }
                        if(OIL_TANK[1]==0x04 && (flag_output_tank[1] == 0 || flag_output_tank[1] != 5))
                        {
                            emit warning_uart_tank(71);//通信故障
                            net_history(1,4);
                            add_value_controlinfo("1# 油罐   ","通信故障");
                            flag_output_tank[0] = 5;
                        }
                    }
                    if(OIL_TANK[1] != 0x00)
                    {
                        Mythread_tank[1] = 1;
                    }
                }
            }
            if(count_tank >= 2)       //2#
            {
                if((OIL_TANK[2]&0x0f)==0)
                {
                    if(OIL_TANK[3]==0 && (flag_output_tank[1] == 0 || flag_output_tank[1] != 1))
                    {
                        emit right_tank(72);
                        net_history(2,0);
                        add_value_controlinfo("2# 油罐   ","设备正常");
                        flag_output_tank[1] = 1;
                    }
                    else
                    {
                        if(OIL_TANK[3]==0xa0 && (flag_output_tank[1] == 0 || flag_output_tank[1] != 2))
                        {
                            emit warning_high_tank(72);//高报警
                            net_history(2,5);
                            add_value_controlinfo("2# 油罐   ","高液位报警");
                            flag_output_tank[1] = 2;
                        }
                        if(OIL_TANK[3]==0xc0 && (flag_output_tank[1] == 0 || flag_output_tank[1] != 3))
                        {
                            emit warning_low_tank(72);//低报警
                            net_history(2,6);
                            add_value_controlinfo("2# 油罐   ","低液位报警");
                            flag_output_tank[1] = 3;
                        }
                        if(OIL_TANK[3]==0x01 && (flag_output_tank[1] == 0 || flag_output_tank[1] != 4))
                        {
                            emit warning_sensor_tank(72);//传感器故障
                            net_history(2,3);
                            add_value_controlinfo("2# 油罐   ","传感器故障");
                            flag_output_tank[1] = 4;
                        }
                        if(OIL_TANK[3]==0x04 && (flag_output_tank[1] == 0 || flag_output_tank[1] != 5))
                        {
                            emit warning_uart_tank(72);//通信故障
                            net_history(2,4);
                            add_value_controlinfo("2# 油罐   ","通信故障");
                            flag_output_tank[1] = 5;
                        }
                    }
                    if(OIL_TANK[3] != 0x00)
                    {
                        Mythread_tank[2] = 1;
                    }
                }
            }
            if(count_tank >= 3)      //3#
            {
                if((OIL_TANK[4]&0x0f)==0)
                {
                    if(OIL_TANK[5]==0 && (flag_output_tank[2] == 0 || flag_output_tank[2] != 1))
                    {
                        emit right_tank(73);
                            net_history(3,0);
                        add_value_controlinfo("3# 油罐   ","设备正常");
                        flag_output_tank[2] = 1;
                    }
                    else
                    {
                        if(OIL_TANK[5]==0xa0 && (flag_output_tank[2] == 0 || flag_output_tank[2] != 2))
                        {
                            emit warning_high_tank(73);//高报警
                            net_history(3,5);
                            add_value_controlinfo("3# 油罐   ","高液位报警");
                            flag_output_tank[2] = 2;
                        }
                        if(OIL_TANK[5]==0xc0 && (flag_output_tank[2] == 0 || flag_output_tank[2] != 3))
                        {
                            emit warning_low_tank(73);//低报警
                            net_history(3,6);
                            add_value_controlinfo("3# 油罐   ","低液位报警");
                            flag_output_tank[2] = 3;
                        }
                        if(OIL_TANK[5]==0x01 && (flag_output_tank[2] == 0 || flag_output_tank[2] != 4))
                        {
                            emit warning_sensor_tank(73);//传感器故障
                            net_history(3,3);
                            add_value_controlinfo("3# 油罐   ","传感器故障");
                            flag_output_tank[2] = 4;
                        }
                        if(OIL_TANK[5]==0x04 && (flag_output_tank[2] == 0 || flag_output_tank[2] != 5))
                        {
                            emit warning_uart_tank(73);//通信故障
                            net_history(3,4);
                            add_value_controlinfo("3# 油罐   ","通信故障");
                            flag_output_tank[2] = 5;
                        }
                    }
                    if(OIL_TANK[5] != 0x00)
                    {
                        Mythread_tank[3] = 1;
                    }
                }
            }
            if(count_tank >= 4)      //4#
            {
                if((OIL_TANK[6]&0x0f)==0)
                {
                    if(OIL_TANK[7]==0 && (flag_output_tank[3] == 0 || flag_output_tank[3] != 1))
                    {
                        emit right_tank(74);
                        net_history(4,0);
                        add_value_controlinfo("4# 油罐   ","设备正常");
                        flag_output_tank[3] = 1;
                    }
                    else
                    {
                        if(OIL_TANK[7]==0xa0 && (flag_output_tank[3] == 0 || flag_output_tank[3] != 2))
                        {
                            emit warning_high_tank(74);//高报警
                            net_history(4,5);
                            add_value_controlinfo("4# 油罐   ","高液位报警");
                            flag_output_tank[3] = 2;
                        }
                        if(OIL_TANK[7]==0xc0 && (flag_output_tank[3] == 0 || flag_output_tank[3] != 3))
                        {
                            emit warning_low_tank(74);//低报警
                            net_history(4,6);
                            add_value_controlinfo("4# 油罐   ","低液位报警");
                            flag_output_tank[3] = 3;
                        }
                        if(OIL_TANK[7]==0x01 && (flag_output_tank[3] == 0 || flag_output_tank[3] != 4))
                        {
                            emit warning_sensor_tank(74);//传感器故障
                            net_history(4,3);
                            add_value_controlinfo("4# 油罐   ","传感器故障");
                            flag_output_tank[3] = 4;
                        }
                        if(OIL_TANK[7]==0x04 && (flag_output_tank[3] == 0 || flag_output_tank[3] != 5))
                        {
                            emit warning_uart_tank(74);//通信故障
                            net_history(4,4);
                            add_value_controlinfo("4# 油罐   ","通信故障");
                            flag_output_tank[3] = 5;
                        }
                    }
                    if(OIL_TANK[7] != 0x00)
                    {
                        Mythread_tank[4] = 1;
                    }
                }
            }
            if(count_tank >= 5)     //5#
            {
                if((OIL_TANK[8]&0x0f)==0)
                {
                    if(OIL_TANK[9]==0 && (flag_output_tank[4] == 0 || flag_output_tank[4] != 1))
                    {
                        emit right_tank(75);
                        net_history(5,0);
                        add_value_controlinfo("5# 油罐   ","设备正常");
                        flag_output_tank[4] = 1;
                    }
                    else
                    {
                        if(OIL_TANK[9]==0xa0 && (flag_output_tank[4] == 0 || flag_output_tank[4] != 2))
                        {
                            emit warning_high_tank(75);//高报警
                            net_history(5,5);
                            add_value_controlinfo("5# 油罐   ","高液位报警");
                            flag_output_tank[4] = 2;
                        }
                        if(OIL_TANK[9]==0xc0 && (flag_output_tank[4] == 0 || flag_output_tank[4] != 3))
                        {
                            emit warning_low_tank(75);//低报警
                            net_history(5,6);
                            add_value_controlinfo("5# 油罐   ","低液位报警");
                            flag_output_tank[4] = 3;
                        }
                        if(OIL_TANK[9]==0x01 && (flag_output_tank[4] == 0 || flag_output_tank[4] != 4))
                        {
                            emit warning_sensor_tank(75);//传感器故障
                            net_history(5,3);
                            add_value_controlinfo("5# 油罐   ","传感器故障");
                            flag_output_tank[4] = 4;
                        }
                        if(OIL_TANK[9]==0x04 && (flag_output_tank[4] == 0 || flag_output_tank[4] != 5))
                        {
                            emit warning_uart_tank(75);//通信故障
                            net_history(5,4);
                            add_value_controlinfo("5# 油罐   ","通信故障");
                            flag_output_tank[4] = 5;
                        }
                    }
                    if(OIL_TANK[9] != 0x00)
                    {
                        Mythread_tank[5] = 1;
                    }
                }
            }
            if(count_tank >= 6)     //6#
            {
                if((OIL_TANK[10]&0x0f)==0)
                {
                    if(OIL_TANK[11]==0 && (flag_output_tank[5] == 0 || flag_output_tank[5] != 1))
                    {
                        emit right_tank(76);
                        net_history(6,0);
                        add_value_controlinfo("6# 油罐   ","设备正常");
                        flag_output_tank[5] = 1;
                    }
                    else
                    {
                        if(OIL_TANK[11]==0xa0 && (flag_output_tank[5] == 0 || flag_output_tank[5] != 2))
                        {
                            emit warning_high_tank(76);//高报警
                            net_history(6,5);
                            add_value_controlinfo("6# 油罐   ","高液位报警");
                            flag_output_tank[5] = 2;
                        }
                        if(OIL_TANK[11]==0xc0 && (flag_output_tank[5] == 0 || flag_output_tank[5] != 3))
                        {
                            emit warning_low_tank(76);//低报警
                            net_history(6,6);
                            add_value_controlinfo("6# 油罐   ","低液位报警");
                            flag_output_tank[5] = 3;
                        }
                        if(OIL_TANK[11]==0x01 && (flag_output_tank[5] == 0 || flag_output_tank[5] != 4))
                        {
                            emit warning_sensor_tank(76);//传感器故障
                            net_history(6,3);
                            add_value_controlinfo("6# 油罐   ","传感器故障");
                            flag_output_tank[5] = 4;
                        }
                        if(OIL_TANK[11]==0x04 && (flag_output_tank[5] == 0 || flag_output_tank[5] != 5))
                        {
                            emit warning_uart_tank(76);//通信故障
                            net_history(6,4);
                            add_value_controlinfo("6# 油罐   ","通信故障");
                            flag_output_tank[5] = 5;
                        }
                    }
                    if(OIL_TANK[11] != 0x00)
                    {
                        Mythread_tank[6] = 1;
                    }
                }
            }
            if(count_tank >= 7)     //7#
            {
                if((OIL_TANK[12]&0x0f)==0)
                {
                    if(OIL_TANK[13]==0 && (flag_output_tank[6] == 0 || flag_output_tank[6] != 1))
                    {
                        emit right_tank(77);
                        net_history(7,0);
                        add_value_controlinfo("7# 油罐   ","设备正常");
                        flag_output_tank[6] = 1;
                    }
                    else
                    {
                        if(OIL_TANK[13]==0xa0 && (flag_output_tank[6] == 0 || flag_output_tank[6] != 2))
                        {
                            emit warning_high_tank(77);//高报警
                            net_history(7,5);
                            add_value_controlinfo("7# 油罐   ","高液位报警");
                            flag_output_tank[6] = 2;
                        }
                        if(OIL_TANK[13]==0xc0 && (flag_output_tank[6] == 0 || flag_output_tank[6] != 3))
                        {
                            emit warning_low_tank(77);//低报警
                            net_history(7,6);
                            add_value_controlinfo("7# 油罐   ","低液位报警");
                            flag_output_tank[6] = 3;
                        }
                        if(OIL_TANK[13]==0x01 && (flag_output_tank[6] == 0 || flag_output_tank[6] != 4))
                        {
                            emit warning_sensor_tank(77);//传感器故障
                            net_history(7,3);
                            add_value_controlinfo("7# 油罐   ","传感器故障");
                            flag_output_tank[6] = 4;
                        }
                        if(OIL_TANK[13]==0x04 && (flag_output_tank[6] == 0 || flag_output_tank[6] != 5))
                        {
                            emit warning_uart_tank(77);//通信故障
                            net_history(7,4);
                            add_value_controlinfo("7# 油罐   ","通信故障");
                            flag_output_tank[6] = 5;
                        }
                    }
                    if(OIL_TANK[13] != 0x00)
                    {
                        Mythread_tank[7] = 1;
                    }
                }
            }
            if(count_tank >= 8)     //8#
            {
                if((OIL_TANK[14]&0x0f)==0)
                {
                    if(OIL_TANK[15]==0 && (flag_output_tank[7] == 0 || flag_output_tank[7] != 1))
                    {
                        emit right_tank(78);
                        net_history(8,0);
                        add_value_controlinfo("8# 油罐   ","设备正常");
                        flag_output_tank[7] = 1;
                    }
                    else
                    {
                        if(OIL_TANK[15]==0xa0 && (flag_output_tank[7] == 0 || flag_output_tank[7] != 2))
                        {
                            emit warning_high_tank(78);//高报警
                            net_history(8,5);
                            add_value_controlinfo("8# 油罐   ","高液位报警");
                            flag_output_tank[7] = 2;
                        }
                        if(OIL_TANK[15]==0xc0 && (flag_output_tank[7] == 0 || flag_output_tank[7] != 3))
                        {
                            emit warning_low_tank(78);//低报警
                            net_history(8,6);
                            add_value_controlinfo("8# 油罐   ","低液位报警");
                            flag_output_tank[7] = 3;
                        }
                        if(OIL_TANK[15]==0x01 && (flag_output_tank[7] == 0 || flag_output_tank[7] != 4))
                        {
                            emit warning_sensor_tank(78);//传感器故障
                            net_history(8,3);
                            add_value_controlinfo("8# 油罐   ","传感器故障");
                            flag_output_tank[7] = 4;
                        }
                        if(OIL_TANK[15]==0x04 && (flag_output_tank[7] == 0 || flag_output_tank[7] != 5))
                        {
                            emit warning_uart_tank(78);//通信故障
                            net_history(8,4);
                            add_value_controlinfo("8# 油罐   ","通信故障");
                            flag_output_tank[7] = 5;
                        }
                    }
                    if(OIL_TANK[15] != 0x00)
                    {
                        Mythread_tank[8] = 1;
                    }
                }
            }
            //液媒法语音部分
            //如果8个油罐均为正常或者未设置，则对报警标志位进行清零
            if((OIL_TANK[1]==0x00||OIL_TANK[1]==0xFF)&&(OIL_TANK[3]==0x00||OIL_TANK[3]==0xFF)&&
                (OIL_TANK[5]==0x00||OIL_TANK[5]==0xFF)&&(OIL_TANK[7]==0x00||OIL_TANK[7]==0xFF)&&
                (OIL_TANK[9]==0x00||OIL_TANK[9]==0xFF)&&(OIL_TANK[11]==0x00||OIL_TANK[11]==0xFF)&&
                (OIL_TANK[13]==0x00||OIL_TANK[13]==0xFF)&&(OIL_TANK[15]==0x00||OIL_TANK[15]==0xFF))
            {
                Flag_Sound_Xielou[4] = 0;  //oil
                Flag_Sound_Xielou[5] = 0;  //water
                Flag_Sound_Xielou[6] = 0;  //sensor
                Flag_Sound_Xielou[7] = 0;  //uart
                Flag_Sound_Xielou[8] = 0;  //pre
                Flag_Sound_Xielou[9] = 0;  //warn
                Flag_Sound_Xielou[10] = 0; //high
                Flag_Sound_Xielou[11] = 0; //low
            }
            //如果有一个为高液位报警，则高液位报警标志位置1;全部都不是高液位报警，则进行高液位标志位清零
            if((OIL_TANK[1]==0xa0)||(OIL_TANK[3]==0xa0)||
                    (OIL_TANK[5]==0xa0)||(OIL_TANK[7]==0xa0)||
                    (OIL_TANK[9]==0xa0)||(OIL_TANK[11]==0xa0)||
                    (OIL_TANK[13]==0xa0)||(OIL_TANK[15]==0xa0))
            {
                Flag_Sound_Xielou[10] = 1;
            }
            else
            {
                Flag_Sound_Xielou[10] = 0;
            }
            //如果有一个为低液位报警，则低液位报警标志位置1;全部都不是低液位报警，则进行低液位标志位清零
            if((OIL_TANK[1]==0xc0)||(OIL_TANK[3]==0xc0)||
                    (OIL_TANK[5]==0xc0)||(OIL_TANK[7]==0xc0)||
                    (OIL_TANK[9]==0xc0)||(OIL_TANK[11]==0xc0)||
                    (OIL_TANK[13]==0xc0)||(OIL_TANK[15]==0xc0))
            {
                Flag_Sound_Xielou[11] = 1;
            }
            else
            {
                Flag_Sound_Xielou[11] = 0;
            }
            //如果有一个为传感器故障，则传感器故障标志位置1;全部都不是传感器故障，则进行传感器故障标志位清零
            if((OIL_TANK[1]==0x01)||(OIL_TANK[3]==0x01)||
                    (OIL_TANK[5]==0x01)||(OIL_TANK[7]==0x01)||
                    (OIL_TANK[9]==0x01)||(OIL_TANK[11]==0x01)||
                    (OIL_TANK[13]==0x01)||(OIL_TANK[15]==0x01))
            {
                Flag_Sound_Xielou[6] = 1;
            }
            else
            {
                Flag_Sound_Xielou[6] = 0;
            }
            //如果有一个为通信故障，则通信故障标志位置1;全部都不是通信故障，则进行通信故障标志位清零
            if((OIL_TANK[1]==0x04)||(OIL_TANK[3]==0x04)||
                    (OIL_TANK[5]==0x04)||(OIL_TANK[7]==0x04)||
                    (OIL_TANK[9]==0x04)||(OIL_TANK[11]==0x04)||
                    (OIL_TANK[13]==0x04)||(OIL_TANK[15]==0x04))
            {
                Flag_Sound_Xielou[7] = 1;
            }
            else
            {
                Flag_Sound_Xielou[7] = 0;
            }
            //如果油罐数量为零，则进行标志位清零
            if(count_tank<=0)
            {
                Flag_Sound_Xielou[4] = 0;  //oil
                Flag_Sound_Xielou[5] = 0;  //water
                Flag_Sound_Xielou[6] = 0;  //sensor
                Flag_Sound_Xielou[7] = 0;  //uart
                Flag_Sound_Xielou[8] = 0;  //pre
                Flag_Sound_Xielou[9] = 0;  //warn
                Flag_Sound_Xielou[10] = 0; //high
                Flag_Sound_Xielou[11] = 0; //low
            }
        }
        if((OIL_TANK[0]&0xf0)==0x40)      //压力法
        {
            //对非压力法的语音标志位进行清零
            Flag_Sound_Xielou[4] = 0;
            Flag_Sound_Xielou[5] = 0;
            Flag_Sound_Xielou[10] = 0;
            Flag_Sound_Xielou[11] = 0;

            if(count_tank >= 1)     //1#
            {
                if(((OIL_TANK[0] == 0x40) && (OIL_TANK[1] == 0)) && (flag_output_tank[0] == 0 || flag_output_tank[0] != 1)) //1#
                {
                    emit right_tank(71);
                    net_history(1,0);
                    add_value_controlinfo("1# 油罐   ","设备正常");
                    flag_output_tank[0] = 1;
                }
                if(((OIL_TANK[0] == 0x41) && (OIL_TANK[1] == 0x80)) && (flag_output_tank[0] == 0 || flag_output_tank[0] != 2))
                {
                    emit warning_pre_tank(71);//预报警
                    net_history(1,7);
                    add_value_controlinfo("1# 油罐   ","压力预报警");
                    flag_output_tank[0] = 2;
                }
                if(((OIL_TANK[0] == 0x42) && (OIL_TANK[1] == 0x80)) && (flag_output_tank[0] == 0 || flag_output_tank[0] != 3))
                {
                    emit warning_warn_tank(71);//报警
                    net_history(1,6);
                    add_value_controlinfo("1# 油罐   ","压力报警");
                    flag_output_tank[0] = 3;
                }
                if(((OIL_TANK[0] == 0x40) && (OIL_TANK[1] == 0x01)) && (flag_output_tank[0] == 0 || flag_output_tank[0] != 4))
                {
                    emit warning_sensor_tank(71);//传感器故障
                    net_history(1,3);
                    add_value_controlinfo("1# 油罐   ","传感器故障");
                    flag_output_tank[0] = 4;
                }
                if(((OIL_TANK[0] == 0x40) && (OIL_TANK[1] == 0x04)) && (flag_output_tank[0] == 0 || flag_output_tank[0] != 5))
                {
                    emit warning_uart_tank(71);//通信故障
                    net_history(1,4);
                    add_value_controlinfo("1# 油罐   ","通信故障");
                    flag_output_tank[0] = 5;
                }
                if(OIL_TANK[1] != 0x00)
                {
                    Mythread_tank[1] = 1;
                }
            }

            if(count_tank >= 2)     //2#
            {
                if(((OIL_TANK[2] == 0x40) && (OIL_TANK[3] == 0)) && (flag_output_tank[1] == 0 || flag_output_tank[1] != 1)) //2#
                {
                    emit right_tank(72);
                    net_history(2,0);
                    add_value_controlinfo("2# 油罐   ","设备正常");
                    flag_output_tank[1] = 1;
                }
                if(((OIL_TANK[2] == 0x41) && (OIL_TANK[3] == 0x80)) && (flag_output_tank[1] == 0 || flag_output_tank[1] != 2))
                {
                    emit warning_pre_tank(72);//预报警
                    net_history(2,7);
                    add_value_controlinfo("2# 油罐   ","压力预报警");
                    flag_output_tank[1] = 2;
                }
                if(((OIL_TANK[2] == 0x42) && (OIL_TANK[3] == 0x80)) && (flag_output_tank[1] == 0 || flag_output_tank[1] != 3))
                {
                    emit warning_warn_tank(72);//报警
                    net_history(2,8);
                    add_value_controlinfo("2# 油罐   ","压力报警");
                    flag_output_tank[1] = 3;
                }
                if(((OIL_TANK[2] == 0x40) && (OIL_TANK[3] == 0x01)) && (flag_output_tank[1] == 0 || flag_output_tank[1] != 4))
                {
                    emit warning_sensor_tank(72);//传感器故障
                    net_history(2,3);
                    add_value_controlinfo("2# 油罐   ","传感器故障");
                    flag_output_tank[1] = 4;
                }
                if(((OIL_TANK[2] == 0x40) && (OIL_TANK[3] == 0x04)) && (flag_output_tank[1] == 0 || flag_output_tank[1] != 5))
                {
                    emit warning_uart_tank(72);//通信故障
                    net_history(2,4);
                    add_value_controlinfo("2# 油罐   ","通信故障");
                    flag_output_tank[1] = 5;
                }
                if(OIL_TANK[3] != 0x00)
                {
                    Mythread_tank[2] = 1;
                }
            }
            if(count_tank >= 3)    //3#
            {
                if(((OIL_TANK[4] == 0x40) && (OIL_TANK[5] == 0)) && (flag_output_tank[2] == 0 || flag_output_tank[2] != 1)) //3#
                {
                    emit right_tank(73);
                    net_history(3,0);
                    add_value_controlinfo("3# 油罐   ","设备正常");
                    flag_output_tank[2] = 1;
                }
                if(((OIL_TANK[4] == 0x41) && (OIL_TANK[5] == 0x80)) && (flag_output_tank[2] == 0 || flag_output_tank[2] != 2))
                {
                    emit warning_pre_tank(73);//预报警
                    net_history(3,7);
                    add_value_controlinfo("3# 油罐   ","压力预报警");
                    flag_output_tank[2] = 2;
                }
                if(((OIL_TANK[4] == 0x42) && (OIL_TANK[5] == 0x80)) && (flag_output_tank[2] == 0 || flag_output_tank[2] != 3))
                {
                    emit warning_warn_tank(73);//报警
                    net_history(3,8);
                    add_value_controlinfo("3# 油罐   ","压力报警");
                    flag_output_tank[2] = 3;
                }
                if(((OIL_TANK[4] == 0x40) && (OIL_TANK[5] == 0x01)) && (flag_output_tank[2] == 0 || flag_output_tank[2] != 4))
                {
                    emit warning_sensor_tank(73);//传感器故障
                    net_history(3,3);
                    add_value_controlinfo("3# 油罐   ","传感器故障");
                    flag_output_tank[2] = 4;
                }
                if(((OIL_TANK[4] == 0x40) && (OIL_TANK[5] == 0x04)) && (flag_output_tank[2] == 0 || flag_output_tank[2] != 5))
                {
                    emit warning_uart_tank(73);//通信故障
                    net_history(3,4);
                    add_value_controlinfo("3# 油罐   ","通信故障");
                    flag_output_tank[2] = 5;
                }
                if(OIL_TANK[5] != 0x00)
                {
                    Mythread_tank[3] = 1;
                }
            }
            if(count_tank >= 4)    //4#
            {
                if(((OIL_TANK[6] == 0x40) && (OIL_TANK[7] == 0)) && (flag_output_tank[3] == 0 || flag_output_tank[3] != 1)) //4#
                {
                    emit right_tank(74);
                    net_history(4,0);
                    add_value_controlinfo("4# 油罐   ","设备正常");
                    flag_output_tank[3] = 1;
                }
                if(((OIL_TANK[6] == 0x41) && (OIL_TANK[7] == 0x80)) && (flag_output_tank[3] == 0 || flag_output_tank[3] != 2))
                {
                    emit warning_pre_tank(74);//预报警
                    net_history(4,7);
                    add_value_controlinfo("4# 油罐   ","压力预报警");
                    flag_output_tank[3] = 2;
                }
                if(((OIL_TANK[6] == 0x42) && (OIL_TANK[7] == 0x80)) && (flag_output_tank[3] == 0 || flag_output_tank[3] != 3))
                {
                    emit warning_warn_tank(74);//报警
                    net_history(4,8);
                    add_value_controlinfo("4# 油罐   ","压力报警");
                    flag_output_tank[3] = 3;
                }
                if(((OIL_TANK[6] == 0x40) && (OIL_TANK[7] == 0x01)) && (flag_output_tank[3] == 0 || flag_output_tank[3] != 4))
                {
                    emit warning_sensor_tank(74);//传感器故障
                    net_history(4,3);
                    add_value_controlinfo("4# 油罐   ","传感器故障");
                    flag_output_tank[3] = 4;
                }
                if(((OIL_TANK[6] == 0x40) && (OIL_TANK[7] == 0x04)) && (flag_output_tank[3] == 0 || flag_output_tank[3] != 5))
                {
                    emit warning_uart_tank(74);//通信故障
                    net_history(4,4);
                    add_value_controlinfo("4# 油罐   ","通信故障");
                    flag_output_tank[3] = 5;
                }
                if(OIL_TANK[7] != 0x00)
                {
                    Mythread_tank[4] = 1;
                }
            }
            if(count_tank >= 5)    //5#
            {
                if(((OIL_TANK[8] == 0x40) && (OIL_TANK[9] == 0)) && (flag_output_tank[4] == 0 || flag_output_tank[4] != 1)) //5#
                {
                    emit right_tank(75);
                    net_history(5,0);
                    add_value_controlinfo("5# 油罐   ","设备正常");
                    flag_output_tank[4] = 1;
                }
                if(((OIL_TANK[8] == 0x41) && (OIL_TANK[9] == 0x80)) && (flag_output_tank[4] == 0 || flag_output_tank[4] != 2))
                {
                    emit warning_pre_tank(75);//预报警
                    net_history(5,7);
                    add_value_controlinfo("5# 油罐   ","压力预报警");
                    flag_output_tank[4] = 2;
                }
                if(((OIL_TANK[8] == 0x42) && (OIL_TANK[9] == 0x80)) && (flag_output_tank[4] == 0 || flag_output_tank[4] != 3))
                {
                    emit warning_warn_tank(75);//报警
                    net_history(5,8);
                    add_value_controlinfo("5# 油罐   ","压力报警");
                    flag_output_tank[4] = 3;
                }
                if(((OIL_TANK[8] == 0x40) && (OIL_TANK[9] == 0x01)) && (flag_output_tank[4] == 0 || flag_output_tank[4] != 4))
                {
                    emit warning_sensor_tank(75);//传感器故障
                    net_history(5,3);
                    add_value_controlinfo("5# 油罐   ","传感器故障");
                    flag_output_tank[4] = 4;
                }
                if(((OIL_TANK[8] == 0x40) && (OIL_TANK[9] == 0x04)) && (flag_output_tank[4] == 0 || flag_output_tank[4] != 5))
                {
                    emit warning_uart_tank(75);//通信故障
                    net_history(5,4);
                    add_value_controlinfo("5# 油罐   ","通信故障");
                    flag_output_tank[4] = 5;
                }
                if(OIL_TANK[9] != 0x00)
                {
                    Mythread_tank[5] = 1;
                }
            }
            if(count_tank >= 6)    //6#
            {
                if(((OIL_TANK[10] == 0x40) && (OIL_TANK[11] == 0)) && (flag_output_tank[5] == 0 || flag_output_tank[5] != 1)) //6#
                {
                    emit right_tank(76);
                    net_history(6,0);
                    add_value_controlinfo("6# 油罐   ","设备正常");
                    flag_output_tank[5] = 1;
                }
                if(((OIL_TANK[10] == 0x41) && (OIL_TANK[11] == 0x80)) && (flag_output_tank[5] == 0 || flag_output_tank[5] != 2))
                {
                    emit warning_pre_tank(76);//预报警
                    net_history(6,7);
                    add_value_controlinfo("6# 油罐   ","压力预报警");
                    flag_output_tank[5] = 2;
                }
                if(((OIL_TANK[10] == 0x42) && (OIL_TANK[11] == 0x80)) && (flag_output_tank[5] == 0 || flag_output_tank[5] != 3))
                {
                    emit warning_warn_tank(76);//报警
                    net_history(6,8);
                    add_value_controlinfo("6# 油罐   ","压力报警");
                    flag_output_tank[5] = 3;
                }
                if(((OIL_TANK[10] == 0x40) && (OIL_TANK[11] == 0x01)) && (flag_output_tank[5] == 0 || flag_output_tank[5] != 4))
                {
                    emit warning_sensor_tank(76);//传感器故障
                    net_history(6,3);
                    add_value_controlinfo("6# 油罐   ","传感器故障");
                    flag_output_tank[5] = 4;
                }
                if(((OIL_TANK[10] == 0x40) && (OIL_TANK[11] == 0x04)) && (flag_output_tank[5] == 0 || flag_output_tank[5] != 5))
                {
                    emit warning_uart_tank(76);//通信故障
                    net_history(6,4);
                    add_value_controlinfo("6# 油罐   ","通信故障");
                    flag_output_tank[5] = 5;
                }
                if(OIL_TANK[11] != 0x00)
                {
                    Mythread_tank[6] = 1;
                }
            }

            if(count_tank >= 7)    //7#
            {
                if(((OIL_TANK[12] == 0x40) && (OIL_TANK[13] == 0)) && (flag_output_tank[6] == 0 || flag_output_tank[6] != 1)) //7#
                {
                    emit right_tank(77);
                    net_history(7,0);
                    add_value_controlinfo("7# 油罐   ","设备正常");
                    flag_output_tank[6] = 1;
                }
                if(((OIL_TANK[12] == 0x41) && (OIL_TANK[13] == 0x80)) && (flag_output_tank[6] == 0 || flag_output_tank[6] != 2))
                {
                    emit warning_pre_tank(77);//预报警
                    net_history(7,7);
                    add_value_controlinfo("7# 油罐   ","压力预报警");
                    flag_output_tank[6] = 2;
                }
                if(((OIL_TANK[12] == 0x42) && (OIL_TANK[13] == 0x80)) && (flag_output_tank[6] == 0 || flag_output_tank[6] != 3))
                {
                    emit warning_warn_tank(77);//报警
                    net_history(7,8);
                    add_value_controlinfo("7# 油罐   ","压力报警");
                    flag_output_tank[6] = 3;
                }
                if(((OIL_TANK[12] == 0x40) && (OIL_TANK[13] == 0x01)) && (flag_output_tank[6] == 0 || flag_output_tank[6] != 4))
                {
                    emit warning_sensor_tank(77);//传感器故障
                    net_history(7,3);
                    add_value_controlinfo("7# 油罐   ","传感器故障");
                    flag_output_tank[6] = 4;
                }
                if(((OIL_TANK[12] == 0x40) && (OIL_TANK[13] == 0x04)) && (flag_output_tank[6] == 0 || flag_output_tank[6] != 5))
                {
                    emit warning_uart_tank(77);//通信故障
                    net_history(7,4);
                    add_value_controlinfo("7# 油罐   ","通信故障");
                    flag_output_tank[6] = 5;
                }
                if(OIL_TANK[13] != 0x00)
                {
                    Mythread_tank[7] = 1;
                }
            }

            if(count_tank >= 8)    //8#
            {
                if(((OIL_TANK[14] == 0x40) && (OIL_TANK[15] == 0)) && (flag_output_tank[7] == 0 || flag_output_tank[7] != 1)) //7#
                {
                    emit right_tank(78);
                    net_history(8,0);
                    add_value_controlinfo("8# 油罐   ","设备正常");
                    flag_output_tank[7] = 1;
                }
                if(((OIL_TANK[14] == 0x41) && (OIL_TANK[15] == 0x80)) && (flag_output_tank[7] == 0 || flag_output_tank[7] != 2))
                {
                    emit warning_pre_tank(78);//预报警
                    net_history(8,7);
                    add_value_controlinfo("8# 油罐   ","压力预报警");
                    flag_output_tank[7] = 2;
                }
                if(((OIL_TANK[14] == 0x42) && (OIL_TANK[15] == 0x80)) && (flag_output_tank[7] == 0 || flag_output_tank[7] != 3))
                {
                    emit warning_warn_tank(78);//报警
                    net_history(8,8);
                    add_value_controlinfo("8# 油罐   ","压力报警");
                    flag_output_tank[7] = 3;
                }
                if(((OIL_TANK[14] == 0x40) && (OIL_TANK[15] == 0x01)) && (flag_output_tank[7] == 0 || flag_output_tank[7] != 4))
                {
                    emit warning_sensor_tank(78);//传感器故障
                    net_history(8,3);
                    add_value_controlinfo("8# 油罐   ","传感器故障");
                    flag_output_tank[7] = 4;
                }
                if(((OIL_TANK[14] == 0x40) && (OIL_TANK[15] == 0x04)) && (flag_output_tank[7] == 0 || flag_output_tank[7] != 5))
                {
                    emit warning_uart_tank(78);//通信故障
                    net_history(8,4);
                    add_value_controlinfo("8# 油罐   ","通信故障");
                    flag_output_tank[7] = 5;
                }
                if(OIL_TANK[15] != 0x00)
                {
                    Mythread_tank[8] = 1;
                }
            }
                //压力法语音部分
                //如果8个油罐均为正常或者未设置，则对报警标志位进行清零
                if((OIL_TANK[1]==0x00||OIL_TANK[1]==0xFF)&&(OIL_TANK[3]==0x00||OIL_TANK[3]==0xFF)&&
                    (OIL_TANK[5]==0x00||OIL_TANK[5]==0xFF)&&(OIL_TANK[7]==0x00||OIL_TANK[7]==0xFF)&&
                    (OIL_TANK[9]==0x00||OIL_TANK[9]==0xFF)&&(OIL_TANK[11]==0x00||OIL_TANK[11]==0xFF)&&
                    (OIL_TANK[13]==0x00||OIL_TANK[13]==0xFF)&&(OIL_TANK[15]==0x00||OIL_TANK[15]==0xFF))
                {
                    Flag_Sound_Xielou[4] = 0;  //oil
                    Flag_Sound_Xielou[5] = 0;  //water
                    Flag_Sound_Xielou[6] = 0;  //sensor
                    Flag_Sound_Xielou[7] = 0;  //uart
                    Flag_Sound_Xielou[8] = 0;  //pre
                    Flag_Sound_Xielou[9] = 0;  //warn
                    Flag_Sound_Xielou[10] = 0; //high
                    Flag_Sound_Xielou[11] = 0; //low
                }
                //如果有一个为预报警，则预报警标志位置1;全部都不是预报警，则进行预报警标志位清零
                if((OIL_TANK[0] == 0x41 && OIL_TANK[1] == 0x80)||(OIL_TANK[2] == 0x41 && OIL_TANK[3] == 0x80)||
                   (OIL_TANK[4] == 0x41 && OIL_TANK[5] == 0x80)||(OIL_TANK[6] == 0x41 && OIL_TANK[7] == 0x80)||
                   (OIL_TANK[8] == 0x41 && OIL_TANK[9] == 0x80)||(OIL_TANK[10] == 0x41 && OIL_TANK[11] == 0x80)||
                   (OIL_TANK[12] == 0x41 && OIL_TANK[13] == 0x80)||(OIL_TANK[14] == 0x41 && OIL_TANK[15] == 0x80))
                {
                    Flag_Sound_Xielou[8] = 1;
                }
                else
                {
                    Flag_Sound_Xielou[8] = 0;
                }
                //如果有一个为欠压报警，则欠压报警标志位置1;全部都不是欠压报警，则进行欠压标志位清零
                if((OIL_TANK[0] == 0x42 && OIL_TANK[1] == 0x80)||(OIL_TANK[2] == 0x42 && OIL_TANK[3] == 0x80)||
                   (OIL_TANK[4] == 0x42 && OIL_TANK[5] == 0x80)||(OIL_TANK[6] == 0x42 && OIL_TANK[7] == 0x80)||
                   (OIL_TANK[8] == 0x42 && OIL_TANK[9] == 0x80)||(OIL_TANK[10] == 0x42 && OIL_TANK[11] == 0x80)||
                   (OIL_TANK[12] == 0x42 && OIL_TANK[13] == 0x80)||(OIL_TANK[14] == 0x42 && OIL_TANK[15] == 0x80))
                {
                    Flag_Sound_Xielou[9] = 1;
                }
                else
                {
                    Flag_Sound_Xielou[9] = 0;
                }
                //如果有一个为传感器故障，则传感器故障标志位置1;全部都不是传感器故障，则进行传感器故障标志位清零
                if((OIL_TANK[1]==0x01)||(OIL_TANK[3]==0x01)||
                        (OIL_TANK[5]==0x01)||(OIL_TANK[7]==0x01)||
                        (OIL_TANK[9]==0x01)||(OIL_TANK[11]==0x01)||
                        (OIL_TANK[13]==0x01)||(OIL_TANK[15]==0x01))
                {
                    Flag_Sound_Xielou[6] = 1;
                }
                else
                {
                    Flag_Sound_Xielou[6] = 0;
                }
                //如果有一个为通信故障，则通信故障标志位置1;全部都不是通信故障，则进行通信故障标志位清零
                if((OIL_TANK[1]==0x04)||(OIL_TANK[3]==0x04)||
                        (OIL_TANK[5]==0x04)||(OIL_TANK[7]==0x04)||
                        (OIL_TANK[9]==0x04)||(OIL_TANK[11]==0x04)||
                        (OIL_TANK[13]==0x04)||(OIL_TANK[15]==0x04))
                {
                    Flag_Sound_Xielou[7] = 1;
                }
                else
                {
                    Flag_Sound_Xielou[7] = 0;
                }
                //如果油罐数量为零，则进行标志位清零
                if(count_tank<=0)
                {
                    Flag_Sound_Xielou[4] = 0;  //oil
                    Flag_Sound_Xielou[5] = 0;  //water
                    Flag_Sound_Xielou[6] = 0;  //sensor
                    Flag_Sound_Xielou[7] = 0;  //uart
                    Flag_Sound_Xielou[8] = 0;  //pre
                    Flag_Sound_Xielou[9] = 0;  //warn
                    Flag_Sound_Xielou[10] = 0; //high
                    Flag_Sound_Xielou[11] = 0; //low
                }
        }
//加油机 dis
        if(count_dispener>=1)       //1#
        {
            if(OIL_DISPENER[0]==0xc0)
            {
                if(OIL_DISPENER[1]==0x00 && (flag_output_dispener[0] == 0 || flag_output_dispener[0] != 1))
                {
                    emit right_dispener(81);
                    net_history(25,0);
                    add_value_controlinfo("1# 加油机 ","设备正常");
                    flag_output_dispener[0] = 1;
                }
                else
                {
                    if(OIL_DISPENER[1]==0x88 && (flag_output_dispener[0] == 0 || flag_output_dispener[0] != 2))
                    {
                        emit warning_oil_dispener(81);//油报警
                        net_history(25,1);
                        add_value_controlinfo("1# 加油机 ","检油报警");
                        flag_output_dispener[0] = 2;
                    }
                    if(OIL_DISPENER[1]==0x90 && (flag_output_dispener[0] == 0 || flag_output_dispener[0] != 3))
                    {
                        emit warning_water_dispener(81);//水报警
                        net_history(25,2);
                        add_value_controlinfo("1# 加油机 ","检水报警");
                        flag_output_dispener[0] = 3;
                    }
                    if(OIL_DISPENER[1]==0x01 && (flag_output_dispener[0] == 0 || flag_output_dispener[0] != 4))
                    {
                        emit warning_sensor_dispener(81);//传感器故障
                        net_history(25,3);
                        add_value_controlinfo("1# 加油机 ","传感器故障");
                        flag_output_dispener[0] = 4;
                    }
                    if(OIL_DISPENER[1]==0x04 && (flag_output_dispener[0] == 0 || flag_output_dispener[0] != 5))
                    {
                        emit warning_uart_dispener(81);//通信故障
                        net_history(25,4);
                        add_value_controlinfo("1# 加油机 ","通信故障");
                        flag_output_dispener[0] = 5;
                    }
                }
                if(OIL_DISPENER[1] != 0x00)
                {
                    Mythread_dispener[1] = 1;
                }
            }
        }
        if(count_dispener>=2)       //2#
        {
            if(OIL_DISPENER[2]==0xc0)
            {
                if(OIL_DISPENER[3]==0x00 && (flag_output_dispener[1] == 0 || flag_output_dispener[1] != 1))
                {
                    emit right_dispener(82);
                    net_history(26,0);
                    add_value_controlinfo("2# 加油机 ","设备正常");
                    flag_output_dispener[1] = 1;
                }
                else
                {
                    if(OIL_DISPENER[3]==0x88 && (flag_output_dispener[1] == 0 || flag_output_dispener[1] != 2))
                    {
                        emit warning_oil_dispener(82);//油报警
                        net_history(26,1);
                        add_value_controlinfo("2# 加油机 ","检油报警");
                        flag_output_dispener[1] = 2;
                    }
                    if(OIL_DISPENER[3]==0x90 && (flag_output_dispener[1] == 0 || flag_output_dispener[1] != 3))
                    {
                        emit warning_water_dispener(82);//水报警
                        net_history(26,2);
                        add_value_controlinfo("2# 加油机 ","检水报警");
                        flag_output_dispener[1] = 3;
                    }
                    if(OIL_DISPENER[3]==0x01 && (flag_output_dispener[1] == 0 || flag_output_dispener[1] != 4))
                    {
                        emit warning_sensor_dispener(82);//传感器故障
                        net_history(26,3);
                        add_value_controlinfo("2# 加油机 ","传感器故障");
                        flag_output_dispener[1] = 4;
                    }
                    if(OIL_DISPENER[3]==0x04 && (flag_output_dispener[1] == 0 || flag_output_dispener[1] != 5))
                    {
                        emit warning_uart_dispener(82);//通信故障
                        net_history(26,4);
                        add_value_controlinfo("2# 加油机 ","通信故障");
                        flag_output_dispener[1] = 5;
                    }
                }
                if(OIL_DISPENER[3] != 0x00)
                {
                    Mythread_dispener[2] = 1;
                }
            }
        }
        if(count_dispener>=3)      //3#
        {
            if(OIL_DISPENER[4]==0xc0)
            {

                if(OIL_DISPENER[5]==0x00 && (flag_output_dispener[2] == 0 || flag_output_dispener[2] != 1))
                {
                   emit right_dispener(83);
                    net_history(27,0);
                   add_value_controlinfo("3# 加油机 ","设备正常");
                   flag_output_dispener[2] = 1;
                }
                else
                {
                    if(OIL_DISPENER[5]==0x88 && (flag_output_dispener[2] == 0 || flag_output_dispener[2] != 2))
                    {
                        emit warning_oil_dispener(83);//油报警
                        net_history(27,1);
                        add_value_controlinfo("3# 加油机 ","检油报警");
                        flag_output_dispener[2] = 2;
                    }
                    if(OIL_DISPENER[5]==0x90 && (flag_output_dispener[2] == 0 || flag_output_dispener[2] != 3))
                    {
                        emit warning_water_dispener(83);//水报警
                        net_history(27,2);
                        add_value_controlinfo("3# 加油机 ","检水报警");
                        flag_output_dispener[2] = 3;
                    }
                    if(OIL_DISPENER[5]==0x01 && (flag_output_dispener[2] == 0 || flag_output_dispener[2] != 4))
                    {
                        emit warning_sensor_dispener(83);//传感器故障
                        net_history(27,3);
                        add_value_controlinfo("3# 加油机 ","传感器故障");
                        flag_output_dispener[2] = 4;
                    }
                    if(OIL_DISPENER[5]==0x04 && (flag_output_dispener[2] == 0 || flag_output_dispener[2] != 5))
                    {
                        emit warning_uart_dispener(83);//通信故障
                        net_history(27,4);
                        add_value_controlinfo("3# 加油机 ","通信故障");
                        flag_output_dispener[2] = 5;
                    }
                }
                if(OIL_DISPENER[5] != 0x00)
                {
                    Mythread_dispener[3] = 1;
                }
            }
        }
        if(count_dispener>=4)      //4#
        {
            if(OIL_DISPENER[6]==0xc0)
            {

                if(OIL_DISPENER[7]==0x00 && (flag_output_dispener[3] == 0 || flag_output_dispener[3] != 1))
                {
                    emit right_dispener(84);
                    net_history(28,0);
                    add_value_controlinfo("4# 加油机 ","设备正常");
                    flag_output_dispener[3] = 1;
                }
                else
                {
                    if(OIL_DISPENER[7]==0x88 && (flag_output_dispener[3] == 0 || flag_output_dispener[3] != 2))
                    {
                        emit warning_oil_dispener(84);//油报警
                        net_history(28,1);
                        add_value_controlinfo("4# 加油机 ","检油报警");
                        flag_output_dispener[3] = 2;
                    }
                    if(OIL_DISPENER[7]==0x90 && (flag_output_dispener[3] == 0 || flag_output_dispener[3] != 3))
                    {
                        emit warning_water_dispener(84);//水报警
                        net_history(28,2);
                        add_value_controlinfo("4# 加油机 ","检水报警");
                        flag_output_dispener[3] = 3;
                    }
                    if(OIL_DISPENER[7]==0x01 && (flag_output_dispener[3] == 0 || flag_output_dispener[3] != 4))
                    {
                        emit warning_sensor_dispener(84);//传感器故障
                        net_history(28,3);
                        add_value_controlinfo("4# 加油机 ","传感器故障");
                        flag_output_dispener[3] = 4;
                    }
                    if(OIL_DISPENER[7]==0x04 && (flag_output_dispener[3] == 0 || flag_output_dispener[3] != 5))
                    {
                        emit warning_uart_dispener(84);//通信故障
                        net_history(28,4);
                        add_value_controlinfo("4# 加油机 ","通信故障");
                        flag_output_dispener[3] = 5;
                    }
                }
                if(OIL_DISPENER[7] != 0x00)
                {
                    Mythread_dispener[4] = 1;
                }
            }
        }
        if(count_dispener>=5)      //5#
        {
            if(OIL_DISPENER[8]==0xc0)      //5#
            {

                if(OIL_DISPENER[9]==0x00 && (flag_output_dispener[4] == 0 || flag_output_dispener[4] != 1))
                {
                    emit right_dispener(85);
                    net_history(29,0);
                    add_value_controlinfo("5# 加油机 ","设备正常");
                    flag_output_dispener[4] = 1;
                }
                else
                {
                    if(OIL_DISPENER[9]==0x88 && (flag_output_dispener[4] == 0 || flag_output_dispener[4] != 2))
                    {
                        emit warning_oil_dispener(85);//油报警
                        net_history(29,1);
                        add_value_controlinfo("5# 加油机 ","检油报警");
                        flag_output_dispener[4] = 2;
                    }
                    if(OIL_DISPENER[9]==0x90 && (flag_output_dispener[4] == 0 || flag_output_dispener[4] != 3))
                    {
                        emit warning_water_dispener(85);//水报警
                        net_history(29,2);
                        add_value_controlinfo("5# 加油机 ","检水报警");
                        flag_output_dispener[4] = 3;
                    }
                    if(OIL_DISPENER[9]==0x01 && (flag_output_dispener[4] == 0 || flag_output_dispener[4] != 4))
                    {
                        emit warning_sensor_dispener(85);//传感器故障
                        net_history(29,3);
                        add_value_controlinfo("5# 加油机 ","传感器故障");
                        flag_output_dispener[4] = 4;
                    }
                    if(OIL_DISPENER[9]==0x04 && (flag_output_dispener[4] == 0 || flag_output_dispener[4] != 5))
                    {
                        emit warning_uart_dispener(85);//通信故障
                        net_history(29,4);
                        add_value_controlinfo("5# 加油机 ","通信故障");
                        flag_output_dispener[4] = 5;
                    }
                }
                if(OIL_DISPENER[9] != 0x00)
                {
                    Mythread_dispener[5] = 1;
                }
            }
        }
        if(count_dispener>=6)      //6#
        {
            if(OIL_DISPENER[10]==0xc0)
            {

                if(OIL_DISPENER[11]==0x00 && (flag_output_dispener[5] == 0 || flag_output_dispener[5] != 1))
                {
                    emit right_dispener(86);
                    net_history(30,0);
                    add_value_controlinfo("6# 加油机 ","设备正常");
                    flag_output_dispener[5] = 1;
                }
                else
                {
                    if(OIL_DISPENER[11]==0x88 && (flag_output_dispener[5] == 0 || flag_output_dispener[5] != 2))
                    {
                        emit warning_oil_dispener(86);//油报警
                        net_history(30,1);
                        add_value_controlinfo("6# 加油机 ","检油报警");
                        flag_output_dispener[5] = 2;
                    }
                    if(OIL_DISPENER[11]==0x90 && (flag_output_dispener[5] == 0 || flag_output_dispener[5] != 3))
                    {
                        emit warning_water_dispener(86);//水报警
                        net_history(30,2);
                        add_value_controlinfo("6# 加油机 ","检水报警");
                        flag_output_dispener[5] = 3;
                    }
                    if(OIL_DISPENER[11]==0x01 && (flag_output_dispener[5] == 0 || flag_output_dispener[5] != 4))
                    {
                        emit warning_sensor_dispener(86);//传感器故障
                        net_history(30,3);
                        add_value_controlinfo("6# 加油机 ","传感器故障");
                        flag_output_dispener[5] = 4;
                    }
                    if(OIL_DISPENER[11]==0x04 && (flag_output_dispener[5] == 0 || flag_output_dispener[5] != 5))
                    {
                        emit warning_uart_dispener(86);//通信故障
                        net_history(30,4);
                        add_value_controlinfo("6# 加油机 ","通信故障");
                        flag_output_dispener[5] = 5;
                    }
                }
                if(OIL_DISPENER[11] != 0x00)
                {
                    Mythread_dispener[6] = 1;
                }
            }
        }
        if(count_dispener>=7)      //7#
        {
            if(OIL_DISPENER[12]==0xc0)
            {

                if(OIL_DISPENER[13]==0x00 && (flag_output_dispener[6] == 0 || flag_output_dispener[6] != 1))
                {
                    emit right_dispener(87);
                    net_history(31,0);
                    add_value_controlinfo("7# 加油机 ","设备正常");
                    flag_output_dispener[6] = 1;
                }
                else
                {
                    if(OIL_DISPENER[13]==0x88 && (flag_output_dispener[6] == 0 || flag_output_dispener[6] != 2))
                    {
                        emit warning_oil_dispener(87);//油报警
                        net_history(31,1);
                        add_value_controlinfo("7# 加油机 ","检油报警");
                        flag_output_dispener[6] = 2;
                    }
                    if(OIL_DISPENER[13]==0x90 && (flag_output_dispener[6] == 0 || flag_output_dispener[6] != 3))
                    {
                        emit warning_water_dispener(87);//水报警
                        net_history(31,2);
                        add_value_controlinfo("7# 加油机 ","检水报警");
                        flag_output_dispener[6] = 3;
                    }
                    if(OIL_DISPENER[13]==0x01 && (flag_output_dispener[6] == 0 || flag_output_dispener[6] != 4))
                    {
                        emit warning_sensor_dispener(87);//传感器故障
                        net_history(31,3);
                        add_value_controlinfo("7# 加油机 ","传感器故障");
                        flag_output_dispener[6] = 4;
                    }
                    if(OIL_DISPENER[13]==0x04 && (flag_output_dispener[6] == 0 || flag_output_dispener[6] != 5))
                    {
                        emit warning_uart_dispener(87);//通信故障
                        net_history(31,4);
                        add_value_controlinfo("7# 加油机 ","通信故障");
                        flag_output_dispener[6] = 5;
                    }
                }
                if(OIL_DISPENER[13] != 0x00)
                {
                    Mythread_dispener[7] = 1;
                }
            }
        }
        if(count_dispener>=8)      //8#
        {
            if(OIL_DISPENER[14]==0xc0)
            {

                if(OIL_DISPENER[15]==0x00 && (flag_output_dispener[7] == 0 || flag_output_dispener[7] != 1))
                {
                    emit right_dispener(88);
                    net_history(32,0);
                    add_value_controlinfo("8# 加油机 ","设备正常");
                    flag_output_dispener[7] = 1;
                }
                else
                {
                    if(OIL_DISPENER[15]==0x88 && (flag_output_dispener[7] == 0 || flag_output_dispener[7] != 2))
                    {
                        emit warning_oil_dispener(88);//油报警
                        net_history(32,1);
                        add_value_controlinfo("8# 加油机 ","检油报警");
                        flag_output_dispener[7] = 2;
                    }
                    if(OIL_DISPENER[15]==0x90 && (flag_output_dispener[7] == 0 || flag_output_dispener[7] != 3))
                    {
                        emit warning_water_dispener(88);//水报警
                        net_history(32,2);
                        add_value_controlinfo("8# 加油机 ","检水报警");
                        flag_output_dispener[7] = 3;
                    }
                    if(OIL_DISPENER[15]==0x01 && (flag_output_dispener[7] == 0 || flag_output_dispener[7] != 4))
                    {
                        emit warning_sensor_dispener(88);//传感器故障
                        net_history(32,3);
                        add_value_controlinfo("8# 加油机 ","传感器故障");
                        flag_output_dispener[7] = 4;
                    }
                    if(OIL_DISPENER[15]==0x04 && (flag_output_dispener[7] == 0 || flag_output_dispener[7] != 5))
                    {
                        emit warning_uart_dispener(88);//通信故障
                        net_history(32,4);
                        add_value_controlinfo("8# 加油机 ","通信故障");
                        flag_output_dispener[7] = 5;
                    }
                }
                if(OIL_DISPENER[15] != 0x00)
                {
                    Mythread_dispener[8] = 1;
                }
            }
        }
        //加油机语音部分
        if((OIL_DISPENER[1]==0x00||OIL_DISPENER[1]==0xFF)&&(OIL_DISPENER[3]==0x00||OIL_DISPENER[3]==0xFF)&&
            (OIL_DISPENER[5]==0x00||OIL_DISPENER[5]==0xFF)&&(OIL_DISPENER[7]==0x00||OIL_DISPENER[7]==0xFF)&&
            (OIL_DISPENER[9]==0x00||OIL_DISPENER[9]==0xFF)&&(OIL_DISPENER[11]==0x00||OIL_DISPENER[11]==0xFF)&&
            (OIL_DISPENER[13]==0x00||OIL_DISPENER[13]==0xFF)&&(OIL_DISPENER[15]==0x00||OIL_DISPENER[15]==0xFF))
        {
            Flag_Sound_Xielou[16] = 0;
            Flag_Sound_Xielou[17] = 0;
            Flag_Sound_Xielou[18] = 0;
            Flag_Sound_Xielou[19] = 0;
        }
        if((OIL_DISPENER[1]==0x88)||(OIL_DISPENER[3]==0x88)||
                (OIL_DISPENER[5]==0x88)||(OIL_DISPENER[7]==0x88)||
                (OIL_DISPENER[9]==0x88)||(OIL_DISPENER[11]==0x88)||
                (OIL_DISPENER[13]==0x88)||(OIL_DISPENER[15]==0x88))
        {
            Flag_Sound_Xielou[16] = 1;
        }
        else
        {
            Flag_Sound_Xielou[16] = 0;
        }
        if((OIL_DISPENER[1]==0x90)||(OIL_DISPENER[3]==0x90)||
                (OIL_DISPENER[5]==0x90)||(OIL_DISPENER[7]==0x90)||
                (OIL_DISPENER[9]==0x90)||(OIL_DISPENER[11]==0x90)||
                (OIL_DISPENER[13]==0x90)||(OIL_DISPENER[15]==0x90))
        {
            Flag_Sound_Xielou[17] = 1;
        }
        else
        {
            Flag_Sound_Xielou[17] = 0;
        }
        if((OIL_DISPENER[1]==0x01)||(OIL_DISPENER[3]==0x01)||
                (OIL_DISPENER[5]==0x01)||(OIL_DISPENER[7]==0x01)||
                (OIL_DISPENER[9]==0x01)||(OIL_DISPENER[11]==0x01)||
                (OIL_DISPENER[13]==0x01)||(OIL_DISPENER[15]==0x01))
        {
            Flag_Sound_Xielou[18] = 1;
        }
        else
        {
            Flag_Sound_Xielou[18] = 0;
        }
        if((OIL_DISPENER[1]==0x04)||(OIL_DISPENER[3]==0x04)||
                (OIL_DISPENER[5]==0x04)||(OIL_DISPENER[7]==0x04)||
                (OIL_DISPENER[9]==0x04)||(OIL_DISPENER[11]==0x04)||
                (OIL_DISPENER[13]==0x04)||(OIL_DISPENER[15]==0x04))
        {
            Flag_Sound_Xielou[19] = 1;
        }
        else
        {
            Flag_Sound_Xielou[19] = 0;
        }
        if(count_dispener <= 0)
        {
            Flag_Sound_Xielou[16] = 0;
            Flag_Sound_Xielou[17] = 0;
            Flag_Sound_Xielou[18] = 0;
            Flag_Sound_Xielou[19] = 0;
        }
    }
}
void mythread::IIE_analysis()
{
    if(Flag_IIE == 0)
    {
        Flag_IIE_c = 6;
    }
    if(Flag_IIE == 1)//使能状态
    {
        Flag_IIE_c--;
        if((IIE_sta_pre == Data_Buf_Sencor[37])&&(IIE_set_pre == Data_Buf_Sencor[38])&&(IIE_uart == Data_Buf_Sencor[36])&&(IIE_V == Data_Buf_Sencor[39])
               &&(IIE_R == Data_Buf_Sencor[40])&&(IIE_people_time == Data_Buf_Sencor[43])&&(IIE_oil_time == (Data_Buf_Sencor[41] + Data_Buf_Sencor[42]*256))
                &&(Flag_IIE_c <= 0))
        {
            //如果数据与上次完全一致，则不在进行刷新显示
        }
        else
        {
            Flag_IIE_c = 0;

            IIE_sta_pre = Data_Buf_Sencor[37];
            IIE_set_pre = Data_Buf_Sencor[38];
            unsigned char sta = Data_Buf_Sencor[37];
            unsigned char set = Data_Buf_Sencor[38];
            IIE_uart = Data_Buf_Sencor[36];
            IIE_V = Data_Buf_Sencor[39];
            IIE_R = Data_Buf_Sencor[40];
            IIE_people_time = Data_Buf_Sencor[43];
            IIE_oil_time = Data_Buf_Sencor[41] + Data_Buf_Sencor[42]*256;
            //状态
            if((sta&0x80) == 0x80)
            {
                IIE_sta[7] = 1;
            }
            else{IIE_sta[7] = 0;}

            if((sta&0x40) == 0x40)
            {
                IIE_sta[6] = 1;
            }
            else{IIE_sta[6] = 0;}

            if((sta&0x20) == 0x20)
            {
                IIE_sta[5] = 1;
            }
            else{IIE_sta[5] = 0;}

            if((sta&0x10) == 0x10)
            {
                IIE_sta[4] = 1;
            }
            else{IIE_sta[4] = 0;}

            if((sta&0x08) == 0x08)
            {
                IIE_sta[3] = 1;
            }
            else{IIE_sta[3] = 0;}

            if((sta&0x04) == 0x04)
            {
                IIE_sta[2] = 1;
            }
            else{IIE_sta[2] = 0;}

            if((sta&0x02) == 0x02)
            {
                IIE_sta[1] = 1;
            }
            else{IIE_sta[1] = 0;}

            if((sta&0x01) == 0x01)
            {
                IIE_sta[0] = 1;
            }
            else{IIE_sta[0] = 0;}

            //设置
            if((set&0x80) == 0x80)
            {
                IIE_set[7] = 1;
            }
            else{IIE_set[7] = 0;}

            if((set&0x40) == 0x40)
            {
                IIE_set[6] = 1;
            }
            else{IIE_set[6] = 0;}

            if((set&0x20) == 0x20)
            {
                IIE_set[5] = 1;
            }
            else{IIE_set[5] = 0;}

            if((set&0x10) == 0x10)
            {
                IIE_set[4] = 1;
            }
            else{IIE_set[4] = 0;}

            if((set&0x08) == 0x08)
            {
                IIE_set[3] = 1;
            }
            else{IIE_set[3] = 0;}

            if((set&0x04) == 0x04)
            {
                IIE_set[2] = 1;
            }
            else{IIE_set[2] = 0;}

            if((set&0x02) == 0x02)
            {
                IIE_set[1] = 1;
            }
            else{IIE_set[1] = 0;}

            if((set&0x01) == 0x01)
            {
                IIE_set[0] = 1;
            }
            else{IIE_set[0] = 0;}

            emit IIE_display(IIE_uart,IIE_R,IIE_V,IIE_oil_time,IIE_people_time);//显示数据到屏幕
            usleep(10000);
        }
    }

}



//0高液位报警状态：1：高液位报警；0：高液位正常
//1静电报警状态   1：静电危险 0：静电安全
//2触摸状态      1：触摸     0：未触摸
//3工作状态板     1：归位    0：使用
//4夹子状态       1：夹子正常 0：夹子报警
//5接地状态       1：接地故障 0：接地正常
//6稳油倒计时     1：倒计时中 0：结束
//7人员值守倒计时  1值守中     0：结束

//0高报持续时间      1;屏蔽  0：正常
//1静电功能         1;屏蔽   0：正常
//2高液位报警        1;屏蔽  0：正常
//3来人检测          1;屏蔽   0：正常
//4人体静电信号      1;屏蔽   0：正常
//5钥匙管理器        1;屏蔽   0：正常
//6人员值守         1;屏蔽   0：正常
//7倒计时工作阈值    1;屏蔽   0：正常

//***
//*ptr[36] = Sta_IIE[0];//IIE通讯
//*ptr[37] = Sta_IIE[6];//IIE状态信息
//*ptr[38] = Sta_IIE[7];//IIE屏蔽信息
//*ptr[39] = Sta_IIE[1];//IIE电压
//*ptr[40] = Sta_IIE[2];//IIE电阻
//*ptr[41] = Sta_IIE[3];//IIE稳油计时低位
//*ptr[42] = Sta_IIE[4];//IIE稳油计时高位
//*ptr[43] = Sta_IIE[5];//IIE人员值守计时
// *** /
//当断网时对数据进行临时记录
int mythread::net_history(int num,int sta)
{
	QString DataType;
	QString SensorNum;
	QString SensorType;
	QString SensorSta;
	QString SensorData;
	net_data.lock();//上锁

	unsigned char hubei_sta = 0;

	QDate date = QDate::currentDate();
	QTime time = QTime::currentTime();
	int year = date.year();
	int month = date.month();
	int day = date.day();
	int hour = time.hour();
	int minute = time.minute();
	int second = time.second();
	int year1 = year/100;
	int year2 = year%100;
	int hex_year1 = year1+(year1/10)*6;
	int hex_year2 = year2+(year2/10)*6;
	int hex_month = month+(month/10)*6;
	int hex_day = day+(day/10)*6;
	int hex_hour = hour+ (hour/10)*6;
	int hex_minute = minute+(minute/10)*6;
	//printf("*****************%02x\n",hex_minute);
	int hex_second = second+(second/10)*6;

	int num1 = 0;
	int num2 = 0;
	int sta1 = 0;
	int sta2 = 0;

	if(FLAG_num_net > 49)//如果数组记录满了，
	{
//        for(unsigned char i = 0;i<49;i++)
//        {
//            for(unsigned char j = 0;j<11;j++)
//            {
//                FLAG_STACHANGE[i][j] = FLAG_STACHANGE[i+1][j];
//            }
//        }
		FLAG_num_net = 0;
	}

	if((num>=1)&&(num<=8))//油罐
	{
		num1 = 0x04;
		num2 = num;
		DataType = "0";
		SensorNum = QString::number(num);
	}
	if((num>=9)&&(num<=16))//管线
	{
		num1 = 0x03;
		num2 = num-8;
		DataType = "1";
		SensorNum = QString::number(num-8);
	}
	if((num>=17)&&(num<=24))//人井
	{
		num1 = 0x02;
		num2 = num-16;
		DataType = "3";
		SensorNum = QString::number(num-16);
	}
	if((num>=25)&&(num<=32))//加油机
	{
		num1 = 0x05;
		num2 = num-24;
		DataType = "2";
		SensorNum = QString::number(num-24);
	}

	if(sta == 0) //正常
	{
		sta1 = 0x00;
		sta2 = 0x00;
		hubei_sta = 0x00;
		SensorSta = "0";
	}
	if(sta == 1) //检油
	{
		sta1 = 0x00;
		sta2 = 0x08;
		hubei_sta = 0x02;
		SensorSta = "1";
	}
	if(sta == 2) //检水
	{
		sta1 = 0x00;
		sta2 = 0x10;
		hubei_sta = 0x01;
		SensorSta = "2";
	}
	if(sta == 3) //传感器
	{
		sta1 = 0x00;
		sta2 = 0x01;
		hubei_sta = 0x10;
		SensorSta = "4";
	}
	if(sta == 4) //通信
	{
		sta1 = 0x00;
		sta2 = 0x04;
		hubei_sta = 0xff;
		SensorSta = "3";
	}
	if(sta == 5) //高液位
	{
		sta1 = 0x00;
		sta2 = 0x20;
		hubei_sta = 0x03;
		SensorSta = "5";
	}
	if(sta == 6) //低液位
	{
		sta1 = 0x00;
		sta2 = 0x40;
		hubei_sta = 0x04;
		SensorSta = "6";
	}
	if(sta == 7) //压力预警
	{
		sta1 = 0x01;
		sta2 = 0x00;
		hubei_sta = 0x06;
		SensorSta = "7";
	}
	if(sta == 8) //压力报警
	{
		sta1 = 0x02;
		sta2 = 0x00;
		hubei_sta = 0x07;
		SensorSta = "8";
	}

	FLAG_STACHANGE[FLAG_num_net][0] = num1;
	FLAG_STACHANGE[FLAG_num_net][1] = num2+16;
	FLAG_STACHANGE[FLAG_num_net][2] = sta1;
	FLAG_STACHANGE[FLAG_num_net][3] = sta2;
	FLAG_STACHANGE[FLAG_num_net][4] = hex_year1;
	FLAG_STACHANGE[FLAG_num_net][5] = hex_year2;
	FLAG_STACHANGE[FLAG_num_net][6] = hex_month;
	FLAG_STACHANGE[FLAG_num_net][7] = hex_day;
	FLAG_STACHANGE[FLAG_num_net][8] = hex_hour;
	FLAG_STACHANGE[FLAG_num_net][9] = hex_minute;
	FLAG_STACHANGE[FLAG_num_net][10] = hex_second;
	FLAG_num_net++;//每记录一次加1

	Warn_Data_Tcp_Send_Hb[num-1][0] = 1;
	Warn_Data_Tcp_Send_Hb[num-1][1] = hubei_sta;
	if(DataType == "0")
	{
		if(Test_Method == 0){SensorType = "3";SensorData = "N";}//其他方法
		else if(Test_Method == 1){SensorType = "2";SensorData = QString::number(count_Pressure[num-1],'f',1);}//压力法
		else if(Test_Method == 2){SensorType = "1";SensorData = "N";}//液媒法
		else if(Test_Method == 3){SensorType = "0";SensorData = "N";}//传感器法
		else {
			SensorType = "0";
			SensorData = "N";
		}
	}
	else {
		SensorType = "0";
		SensorData = "N";
	}

	myserver_send(DataType,SensorNum,SensorType,SensorSta,SensorData);
	net_data.unlock();//解锁
	return 0;
}
/************泄漏信息服务器上传*****************
 DataType		（上传数据类型， 0油罐、1管线、2加油机、3防渗池）
 SensorNum      （传感器编号1~12）
 SensorType		（传感器类型， 0传感器法、1液媒法、2压力法、3其他方法）
 SensorSta		（传感器状态：  0 设备正常		存在类型0、1、2、3
							  1 检油报警		存在类型0
							  2 检水报警		存在类型0
							  3 通信故障		存在类型0、1、2、3
							  4 传感器故障	存在类型0、1、2、3
							  5 高液位报警	存在类型1
							  6 低液位报警	存在类型1
							  7 压力预警		存在类型2
							  8 压力报警		存在类型2			）
SensorDate		（传感器数据，仅当传感器类型为压力法时，有压力数据，如88.8Kpa，其他情况为N）
 * ***********************××××××*********/
void mythread::myserver_send(QString DataType,QString SensorNum,QString SensorType,QString SensorSta,QString SensorData)
{
	emit myserver_send_single(DataType,SensorNum,SensorType,SensorSta,SensorData);
}
