/*******************
 * 郑州开物通电子科技有限公司
 * MT510 液位监控仪通讯协议 V1.1
 * 与维德路特通讯协议  50  全部兼容
 * 云飞扬开放协议
 * ********************/
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
#include<QString>

#include "mainwindow.h"
#include "systemset.h"

#include "ywythread.h"
#include"file_op.h"
#include"database_op.h"
#include"ywy_yfy.h"
#include"config.h"


int fd_uart_yfy;
int len_uart_yfy, ret_uart_yfy;

union IEEEtoFloat Reply_Data_OT[15][8]; //上传的关于油罐的7个数据  IEEE浮点数格式  [0][0]空置

int len_Reply_str = 0;

char Data_Buf_yfy[20] = {0};
char Reply_DataBuf_yfy[300] = {0};
char Reply_DataBuf_alarm[15][8] = {0};

char flag_place_hoider = 0;  //0 ywy  1 anysafe

QMutex yfyRecv_Data_Lock;   //互斥锁

ywy_yfy_thread::ywy_yfy_thread(QObject *parent):
    QThread(parent)
{

}

void ywy_yfy_thread::run()
{
    fd_uart_yfy = open(FGA_SERI, O_RDWR | O_NOCTTY);
    if (fd_uart_yfy<0)
    {
        perror(FGA_SERI);
    }
    ret_uart_yfy = set_port_attr (fd_uart_yfy,B9600,8,"1",'N',0,0);   //9600波特率 数据位8位 停止位1位 无校验
    if (ret_uart_yfy<0)
    {
           printf("set uart TTYMXC2 FAILED \n");
    }

    sleep(1);
    for(;;)
    {
        msleep(100);


//        len_uart_yfy = write(fd_uart_yfy, Reply_DataBuf_yfy, 8);
//        if (len_uart_yfy<0)
//        {
//            printf("test data failed \n");
//        }

        memset(Data_Buf_yfy,0,sizeof(Data_Buf_yfy));
        yfyRecv_Data_Lock.lock();  //锁
        len_uart_yfy = read(fd_uart_yfy, Data_Buf_yfy, sizeof(Data_Buf_yfy));      //listen 数据
        yfyRecv_Data_Lock.unlock();    //解锁
        if (len_uart_yfy < 0)
        {
            qDebug("read data failed \n");
        }
        else
        {
            yfyRecv_Data_Lock.lock();  //锁
            Data_Handle_yfy();
            yfyRecv_Data_Lock.unlock();    //解锁
        }
        //    for(int i = 0;i<len_uart_yfy;i++)
        //    {
        //        printf("%02x ",Data_Buf_yfy[i]);
        //        fflush(stdout);
        //    }
        //    printf("\n");

    }
}


void ywy_yfy_thread::Data_Handle_yfy()
{
    char numtemp[6] = {0};
    QString strcmd,strnum;

    numtemp[0] = Data_Buf_yfy[5];
    numtemp[1] = Data_Buf_yfy[6];
    strcmd = Data_Buf_yfy;
    strnum = numtemp;

    if(strcmd.indexOf("i201") >= 0)  //读储油罐状态及数据
    {
        if(strnum.toInt() == 0)
        {
           len_uart_yfy = write(fd_uart_yfy, package_Inventory_all(sum_Tangan_Amount), (len_Reply_str+6));
        }
        else if(strnum.toInt()>0)
        {
           len_uart_yfy = write(fd_uart_yfy, package_Inventory_signal(strnum.toInt(),0), (len_Reply_str+6));
        }
    }
    /*
    else if(strcmd.indexOf("i205") >= 0)  //读储油罐报警信息
    {
        if(strnum.toInt() == 0)
        {
            len_uart_yfy = write(fd_uart_yfy, package_alarm_all(sum_Tangan_Amount), (len_Reply_str+6));
        }
        else if(strnum.toInt()>0)
        {
           len_uart_yfy = write(fd_uart_yfy, package_alarm_signal(strnum.toInt()), (len_Reply_str+6));
        }
    }
    else if(strcmd.indexOf("i20C") >= 0)  //读储油罐最近一次进油报告
    {

    }
    else if(strcmd.indexOf("i501") >= 0)  //读取系统时间
    {

    }
    else if(strcmd.indexOf("i602") >= 0)  //读取油品数据
    {

    }
    else if(strcmd.indexOf("i607") >= 0)  //读取油罐直径
    {

    }
    else if(strcmd.indexOf("s501") >= 0)  //设置系统时间
    {

    }
    else if(strcmd.indexOf("i10100") >= 0)  //系统状态
    {

    }
    */
}





char* ywy_yfy_thread::package_Inventory_signal(char add,char oilkind)
{
    char Reply_DataBuf[150];
    char tempOT[100];
    char tempsum[20];

    Reply_DataBuf[0] = SOH;
    sprintf(tempOT,"%s",GetIEEEHex_Inventory_signal(add,oilkind).toAscii().data());

    len_Reply_str = strlen(tempOT);
    for(int i=0;i<len_Reply_str;i++)
    {
       Reply_DataBuf[i+1] = tempOT[i];
    }

    QString str = QString("%1").arg(checksum16(Reply_DataBuf,len_Reply_str+1),2,16,QLatin1Char('0')).toUpper();  //qDebug()<<str<<endl;printf(" %02x",checksum16(Reply_DataBuf));fflush(stdout);
    sprintf(tempsum,"%s",str.toAscii().data());
    Reply_DataBuf[len_Reply_str+1] = tempsum[0];
    Reply_DataBuf[len_Reply_str+2] = tempsum[1];
    Reply_DataBuf[len_Reply_str+3] = tempsum[2];
    Reply_DataBuf[len_Reply_str+4] = tempsum[3];
    Reply_DataBuf[len_Reply_str+5] = ETX;
//    printf(" %s",Reply_DataBuf);fflush(stdout);

    return Reply_DataBuf;
}


QString ywy_yfy_thread::GetIEEEHex_Inventory_signal(uchar add,char oilkind)
{
    int temp[30];
    QString str;

    str.append(QString("i201%1").arg(add,2,16,QLatin1Char('0')));

    QDateTime current_date_time =QDateTime::currentDateTime();
    QString current_date =current_date_time.toString("yyMMddhhmm");
    str.append(current_date);
    str.append(QString("%1").arg(add,2,16,QLatin1Char('0')));
    str.append(QString("%1").arg(oilkind,1,16));
    str.append("0000");
    str.append("07");

    for(char i=0;i<4;i++)
    {
        temp[i] =    Reply_Data_OT[add][1].buf[3-i]&0xFF;
        temp[i+4] =  Reply_Data_OT[add][2].buf[3-i]&0xFF;
        temp[i+8] =  Reply_Data_OT[add][3].buf[3-i]&0xFF;
        temp[i+12] = Reply_Data_OT[add][4].buf[3-i]&0xFF;
        temp[i+16] = Reply_Data_OT[add][5].buf[3-i]&0xFF;
        temp[i+20] = Reply_Data_OT[add][6].buf[3-i]&0xFF;
        temp[i+24] = Reply_Data_OT[add][7].buf[3-i]&0xFF;
    }
    for(char i=0;i<28;i++)
    {
       str.append(QString("%1").arg(temp[i],2,16,QLatin1Char('0')).toUpper());
    }

    str.append(EME); //&&

    return str;
}


char* ywy_yfy_thread::package_Inventory_all(char sum)
{
    char Reply_DataBuf[1000];
    char tempOT[500];
    char tempsum[20];

    Reply_DataBuf[0] = SOH;
    sprintf(tempOT,"%s",GetIEEEHex_Inventory_all(sum).toAscii().data());

    len_Reply_str = strlen(tempOT);
    for(int i=0;i<len_Reply_str;i++)
    {
       Reply_DataBuf[i+1] = tempOT[i];
    }

    QString str = QString("%1").arg(checksum16(Reply_DataBuf,len_Reply_str+1),2,16,QLatin1Char('0')).toUpper();  //qDebug()<<str<<endl;printf(" %02x",checksum16(Reply_DataBuf));fflush(stdout);
    sprintf(tempsum,"%s",str.toAscii().data());
    Reply_DataBuf[len_Reply_str+1] = tempsum[0];
    Reply_DataBuf[len_Reply_str+2] = tempsum[1];
    Reply_DataBuf[len_Reply_str+3] = tempsum[2];
    Reply_DataBuf[len_Reply_str+4] = tempsum[3];
    Reply_DataBuf[len_Reply_str+5] = ETX;
//    printf(" %s",Reply_DataBuf);fflush(stdout);

    return Reply_DataBuf;
}

QString ywy_yfy_thread::GetIEEEHex_Inventory_all(char sum)
{
    int temp[30];
    QString str;

    str.append(QString("i20100"));

    QDateTime current_date_time =QDateTime::currentDateTime();
    QString current_date =current_date_time.toString("yyMMddhhmm");
    str.append(current_date);

    for(int j=0;j<sum;j++)
    {
        str.append(QString("%1").arg(1+j,2,16,QLatin1Char('0')));
        str.append(QString("0"));
        str.append("0000");
        str.append("07");

        for(char i=0;i<4;i++)
        {
            temp[i] =    Reply_Data_OT[j+1][1].buf[3-i]&0xFF;
            temp[i+4] =  Reply_Data_OT[j+1][2].buf[3-i]&0xFF;
            temp[i+8] =  Reply_Data_OT[j+1][3].buf[3-i]&0xFF;
            temp[i+12] = Reply_Data_OT[j+1][4].buf[3-i]&0xFF;
            temp[i+16] = Reply_Data_OT[j+1][5].buf[3-i]&0xFF;
            temp[i+20] = Reply_Data_OT[j+1][6].buf[3-i]&0xFF;
            temp[i+24] = Reply_Data_OT[j+1][7].buf[3-i]&0xFF;
        }
        for(char i=0;i<28;i++)
        {
            str.append(QString("%1").arg(temp[i],2,16,QLatin1Char('0')).toUpper());
        }
    }

    str.append(EME); //&&

    return str;
}


char* ywy_yfy_thread::package_alarm_signal(char add)
{
    char Reply_DataBuf[150];
    char tempOT[100];
    char tempsum[20];

    Reply_DataBuf[0] = SOH;
    sprintf(tempOT,"%s",GetIEEEHex_alarm_signal(add).toAscii().data());
    len_Reply_str = strlen(tempOT);
    for(int i=0;i<len_Reply_str;i++)
    {
       Reply_DataBuf[i+1] = tempOT[i];
    }

    QString str = QString("%1").arg(checksum16(Reply_DataBuf,len_Reply_str+1),2,16,QLatin1Char('0')).toUpper();
    sprintf(tempsum,"%s",str.toAscii().data());
    Reply_DataBuf[len_Reply_str+1] = tempsum[0];
    Reply_DataBuf[len_Reply_str+2] = tempsum[1];
    Reply_DataBuf[len_Reply_str+3] = tempsum[2];
    Reply_DataBuf[len_Reply_str+4] = tempsum[3];
    Reply_DataBuf[len_Reply_str+5] = ETX;
//    printf(" %s",Reply_DataBuf);fflush(stdout);

    return Reply_DataBuf;
}

QString ywy_yfy_thread::GetIEEEHex_alarm_signal(uchar add)
{
    QString str;

    str.append(QString("i205%1").arg(add,2,16,QLatin1Char('0')));
    QDateTime current_date_time =QDateTime::currentDateTime();
    QString current_date =current_date_time.toString("yyMMddhhmm");
    str.append(current_date);
    str.append(QString("%1").arg(add,2,16,QLatin1Char('0')));
    str.append(QString("%1").arg(Reply_DataBuf_alarm[add][0],2,16,QLatin1Char('0')));  //报警数量

    for(char i=0;i<Reply_DataBuf_alarm[add][0];i++)
    {
       str.append(QString("%1").arg(Reply_DataBuf_alarm[add][i+1],2,16,QLatin1Char('0')).toUpper());
    }

    str.append(EME); //&&

    return str;
}



char* ywy_yfy_thread::package_alarm_all(char sum)
{
    char Reply_DataBuf[500];
    char tempOT[500];
    char tempsum[20];

    Reply_DataBuf[0] = SOH;
    sprintf(tempOT,"%s",GetIEEEHex_alarm_all(sum).toAscii().data());

    len_Reply_str = strlen(tempOT);
    for(int i=0;i<len_Reply_str;i++)
    {
       Reply_DataBuf[i+1] = tempOT[i];
    }

    QString str = QString("%1").arg(checksum16(Reply_DataBuf,(len_Reply_str+1)),2,16,QLatin1Char('0')).toUpper();
    sprintf(tempsum,"%s",str.toAscii().data());
    Reply_DataBuf[len_Reply_str+1] = tempsum[0];
    Reply_DataBuf[len_Reply_str+2] = tempsum[1];
    Reply_DataBuf[len_Reply_str+3] = tempsum[2];
    Reply_DataBuf[len_Reply_str+4] = tempsum[3];
    Reply_DataBuf[len_Reply_str+5] = ETX;
//    printf(" %s",Reply_DataBuf);fflush(stdout);

    return Reply_DataBuf;
}

QString ywy_yfy_thread::GetIEEEHex_alarm_all(char sum)
{
    QString str;

    str.append(QString("i20500"));

    QDateTime current_date_time =QDateTime::currentDateTime();
    QString current_date =current_date_time.toString("yyMMddhhmm");
    str.append(current_date);

    for(int j=0;j<sum;j++)
    {
        str.append(QString("%1").arg(j+1,2,16,QLatin1Char('0')));
        str.append(QString("%1").arg(Reply_DataBuf_alarm[j+1][0],2,16,QLatin1Char('0')));  //报警数量

        for(char i=0;i<Reply_DataBuf_alarm[j+1][0];i++)
        {
            str.append(QString("%1").arg(Reply_DataBuf_alarm[j+1][i+1],2,16,QLatin1Char('0')).toUpper());
        }
    }

    str.append(EME); //&&

    return str;
}

char ywy_yfy_thread::getoilkingcode(char add)
{
    if(Oil_Kind[add-1][0] == 92)
    {
        return oilcode92;
    }
    else if(Oil_Kind[add-1][0] == 95)
    {
        return oilcode95;
    }
    else if(Oil_Kind[add-1][0] == 98)
    {
        return oilcode98;
    }
}

uint checksum16(char *p,char len)
{
    uint sum = 0;

    for(int i=0;i<len;i++)
    {
       sum = sum + p[i];
    }
    sum = ((~sum) + 1)&0xffff;
    return sum;
}
