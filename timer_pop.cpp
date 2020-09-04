/*************新增时间线程**********
 * 用来对弹窗进行判断、延时
 * 也可用来对之后的需要计时的函数使用
 * ******************************/

#include "timer_pop.h"
#include "config.h"
#include <QTimer>
#include <QTime>

unsigned char flag_show_reoilgasPop_pre = 0;
QString year;
QString month;
QString day;
QString hour;
QString min;
QString sec;
unsigned char flag_refresh_data = 0;//计数刷新加油机详情页状态

timer_pop::timer_pop(QObject *parent):
    QThread(parent)
{

}
void timer_pop::run()
{
    while (1)
    {
        sleep(1);
        QDateTime date_time = QDateTime::currentDateTime();
        year = date_time.toString("yyyy");
        month = date_time.toString("MM");
        day = date_time.toString("dd");
        hour = date_time.toString("hh");
        min = date_time.toString("mm");
        sec = date_time.toString("ss");
        show_pop_ups();
        if(Flag_Reoilgas_Pop_Sta == 1)//如果详情页打开了
        {
            flag_refresh_data++;
            if(flag_refresh_data >= 3)
            {
                emit refresh_dispener_data();//刷新
                flag_refresh_data = 0;
            }
        }
    }
    this->exec();
}

void timer_pop::show_pop_ups()
{
    if((hour == "23")&&(min == "30")&&(sec == "00"))
    {
        Flag_Reoilgas_DayShow = 0;//每日提示功能开启
    }
    if(Flag_Reoilgas_NeverShow == 0)//如果弹窗功能开启
    {
        if((Flag_Reoilgas_DayShow == 0)&&(Flag_Reoilgas_Pop_Sta == 0))//如果当天弹窗功能开启而且加油机详情页未打开
        {
            if(Flag_Show_ReoilgasPop != 0)
            {
                if(Flag_Show_ReoilgasPop != flag_show_reoilgasPop_pre)
                {
                    flag_show_reoilgasPop_pre = Flag_Show_ReoilgasPop;
                    emit show_reoilgas_pop(1);//发送显示信号
                }
            }
            else
            {
                flag_show_reoilgasPop_pre = 0;
                emit show_reoilgas_pop(0);//发送隐藏信号
            }
        }
    }
}
