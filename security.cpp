#include "security.h"
#include <fcntl.h>
#include <asm/termios.h>
#include <unistd.h>
#include "mainwindow.h"
#include "io_op.h"
#include "database_op.h"
#include "config.h"
#include "serial.h"

int fd_uart_crash_column;
int ret_uart_crash_column;
int len_uart_crash_column;

unsigned int Num_Crash_Column = 0;//自恢复防撞柱的数量

unsigned char ask_cc[8] = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
unsigned char read_cc[8] = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
unsigned int add_read_cc = 0;

unsigned int Flag_Enable_liqiud = 0;//液位仪使能
unsigned int Flag_Enable_pump = 0;//潜油泵使能
unsigned char Flag_sta_liquid = 0;//液位仪状态，全局，网络用
unsigned char Flag_sta_pump = 0;//油泵状态，全局，网络用
unsigned int sta_liquid = 0;
unsigned int sta_pump = 0;
unsigned char Sta_Crash_Column[32] = {0};//32个防撞柱状态
//unsigned char Sta_Crash_Column_Pre[32] = {3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3};//32个防撞柱上一个状态
unsigned char Sta_Crash_Column_Pre[32] = {0};//32个防撞柱上一个状态
//int Time_cc_uartwrong[32] = {3900,3900,3900,3900,3900,3900,3900,3900,3900,3900
//                            ,3900,3900,3900,3900,3900,3900,3900,3900,3900,3900
//                            ,3900,3900,3900,3900,3900,3900,3900,3900,3900,3900
//                            ,3900,3900};//3900秒没数据判断通信故障 1小时5分钟
int Time_cc_uartwrong[32] = {975,975,975,975,975,975,975,975,975,975
                            ,975,975,975,975,975,975,975,975,975,975
                            ,975,975,975,975,975,975,975,975,975,975
                            ,975,975};//3900秒没数据判断通信故障 1小时5分钟

//油泵和液位仪函数，问开关量函数，ask_switch
unsigned int sta_liquid_pre = 3;
unsigned int sta_pump_pre = 3;
unsigned int pre_liquidclose = 0;
unsigned int pre_pumpclose = 0;
 QTimer *timer_safe;
security::security(QObject *parent):
    QThread(parent)
{
    timer_safe = new QTimer(this);
    timer_safe->setInterval(4000);
    timer_safe->start();
    connect(timer_safe,SIGNAL(timeout()),this,SLOT(time_uartwrong()));
}
void security::run()
{
    sleep(1);
    security_init();//设备初始化

    while (1)
    {
        //ask_crash_column();
        //msleep(500);
        read_crach_column();//阻塞模式读取
        msleep(100);
    }

    this->exec();
}

void security::ask_crash_column()
{
    unsigned char CRC = 0;
    unsigned char *sucArray_cc = ask_cc;
    CRC = CRC_Test(sucArray_cc,8);
    *(sucArray_cc + 6) = ((CRC & 0xff00) >> 8);
    *(sucArray_cc + 7) = (CRC & 0x00ff);
    len_uart_crash_column = write(fd_uart_crash_column,ask_cc,8);
}

void security::read_crach_column()
{
    printf("I am security ,i am reading\n");
    unsigned char *sucArray_cc = read_cc;
    int SCRC = 0;
    unsigned char CRCHi;
    unsigned char CRCLi;
    len_uart_crash_column = read(fd_uart_crash_column, read_cc, sizeof(read_cc));
    if(len_uart_crash_column > 0)
    {
        SCRC = CRC_Test(sucArray_cc,8);
        CRCHi = ((SCRC & 0xff00) >> 8);
        CRCLi = (SCRC & 0x00ff);
        if((CRCHi == read_cc[6])&&(CRCLi == read_cc[7])&&(read_cc[0] == 0xAD))//数据校验通过
        {
            printf("yes! CC CRC is ok\n");
            add_read_cc = read_cc[3];
            Sta_Crash_Column[add_read_cc-1] = read_cc[4];
            Time_cc_uartwrong[add_read_cc-1] = 975;//重置通信故障标志位
        }
    }
}
void security::time_uartwrong() //4秒进一次，通信故障3900/4=975
{
    int warn_sound = 0;//声音报警标志位
    for(unsigned int i = 0;i<Num_Crash_Column;i++)
    {
        Time_cc_uartwrong[i]--;
        if(Time_cc_uartwrong[i] <= 0)
        {
            Time_cc_uartwrong[i] = 0;
            Sta_Crash_Column[i] = 0xff;//通信故障
        }
        if(Sta_Crash_Column[i] != Sta_Crash_Column_Pre[i])
        {
            emit crash_column_stashow(i+1,Sta_Crash_Column[i]);
            Sta_Crash_Column_Pre[i] = Sta_Crash_Column[i];
            printf("send message crash!!!!!!\n");
            msleep(200);
            //添加历史记录
            QString history_crash;
            history_crash.append(QString::number(i+1)).append("号").append("防撞柱");
            if(Sta_Crash_Column[i] == 0x00)
            {
                history_crash.append("设备正常");
            }
            if(Sta_Crash_Column[i] == 0x01)
            {
                history_crash.append("碰撞报警");
            }
            if(Sta_Crash_Column[i] == 0xff)
            {
                history_crash.append("通信故障");
            }
            add_value_crash(history_crash);
        }
        if(Sta_Crash_Column[i] != 0)
        {
            warn_sound++;
        }
//        printf(" %02x ",Sta_Crash_Column[i]);
//        printf("\n");
//        printf(" %02x ",Sta_Crash_Column_Pre[i]);
//        printf("\n");
    }
    if(warn_sound >= 1)
    {
        beep_timer();
    }
    else
    {
        beep_none();
    }
    ask_switch();
}
//问潜油泵液位仪等开关量，每一秒问一次
void security::ask_switch()
{
    //sta_liquid = gpio5_5_read();
    sta_liquid = 1;
    Flag_sta_liquid = sta_liquid;
    //sta_pump = gpio5_6_read();
    sta_liquid = 1;
    Flag_sta_pump = sta_pump;
    if(Flag_Enable_liqiud == 1)
    {
        if(sta_liquid_pre != sta_liquid)
        {
            if(sta_liquid == 0)
            {
                emit liquid_warn();
                add_value_liquid("高液位报警");
            }
            else
            {
                emit liquid_nomal();
                add_value_liquid("液位正常");
            }
            sta_liquid_pre = sta_liquid;
        }
        pre_liquidclose = 0;
    }
    else
    {
        if(pre_liquidclose == 0 )
        {
            emit liquid_close();
            add_value_liquid("设备监测关闭");
            pre_liquidclose =1;
        }
    }
    if(Flag_Enable_pump == 1)
    {
        if(sta_pump_pre != sta_pump)
        {
            if(sta_pump == 0)
            {
                emit pump_run();
                add_value_pump("设备开启");
            }
            else
            {
                emit pump_stop();
                add_value_pump("设备停止");
            }
            sta_pump_pre = sta_pump;
        }
        pre_pumpclose = 0;
    }
    else
    {
        if(pre_pumpclose == 0 )
        {
            emit pump_close();
            add_value_pump("设备监测关闭");
            pre_pumpclose =1;
        }
    }
}

void security::security_init()
{
    //串口初始化
    fd_uart_crash_column = open(CRASH_COLUMN,O_RDWR|O_NOCTTY);
    ret_uart_crash_column = set_port_attr(fd_uart_crash_column,B9600,8,"1",'N',5,8);  //20  7 阻塞接收 超时时长2s 最小7字符     尝试1S   延时单位是0.1S

    if(Flag_Enable_liqiud == 0)
    {
        emit liquid_close();
    }
    else
    {
        emit liquid_nomal();
    }
    if(Flag_Enable_pump == 0)
    {
        emit liquid_close();
    }
    else
    {
        emit pump_stop();
    }
}

