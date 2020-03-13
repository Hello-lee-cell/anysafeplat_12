#include<stdio.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<string.h>
#include<unistd.h>
#include"mainwindow.h"
#include"config.h"
#include"io_op.h"

#define TRIGGER     "trigger"
#define BEEP_PATH    "/sys/class/leds/beeper-pwm/trigger"
#define DI_PATH      "/sys/class/leds/beeper-pwm/brightness"
#define LED_STATUS  "brightness"

#define ERR_PATH    "/sys/class/leds/led-err/trigger"

#define TRIGGER_HEARTBEAT    "heartbeat"
#define TRIGGER_NONE    "none"
#define TRIGGER_TIMER   "timer"

#define EXPORT_PATH "/sys/class/gpio/export"
#define GPIO3_4     "68"
#define DIRECT_PATH_3_4 "/sys/class/gpio/gpio68/direction"
#define VALUE_PATH_3_4  "/sys/class/gpio/gpio68/value"
#define DIRECT_OUT  "out"
#define DIRECT_IN   "in"
#define OFF      "0"
#define ON       "1"


#define GPIO5_5     "133"
#define DIRECT_PATH_5_5 "/sys/class/gpio/gpio133/direction"
#define VALUE_PATH_5_5  "/sys/class/gpio/gpio133/value"

#define GPIO5_6     "134"
#define DIRECT_PATH_5_6 "/sys/class/gpio/gpio134/direction"
#define VALUE_PATH_5_6  "/sys/class/gpio/gpio134/value"

unsigned char flag_beeping = 0;
void beep_heartbeat()
{
    int fd;
    int ret;
    if((!flag_silent) && (!flag_beeping) )
    {
        fd = open(BEEP_PATH,O_RDWR);
        if(fd < 0)
        {
            perror("open beep error!");
        }
        ret = write(fd,TRIGGER_HEARTBEAT,strlen(TRIGGER_HEARTBEAT));
        if(ret < 0)
        {
            perror("write beep error!");
        }
        close(fd);
        flag_beeping = 1;
    }
}
void beep_timer()
{
    int fd;
    int ret;
    if((!flag_silent) && (!flag_beeping) )
    {
        fd = open(BEEP_PATH,O_RDWR);
        if(fd < 0)
        {
            perror("open beep error!");
        }
        ret = write(fd,TRIGGER_TIMER,strlen(TRIGGER_TIMER));
        if(ret < 0)
        {
            perror("write beep error!");
        }
        close(fd);
        flag_beeping = 1;
    }
}
void err_heartbeat()
{
    int fd;
    int ret;
    fd = open(ERR_PATH,O_RDWR);
    if(fd < 0)
    {
        perror("open err error!\n");
    }
    ret = write(fd,TRIGGER_HEARTBEAT,strlen(TRIGGER_HEARTBEAT));
    if(ret < 0)
    {
        perror("write err error!\n");
    }
    close(fd);
}
void err_none()
{
    int fd;
    int ret;
    fd = open(ERR_PATH,O_RDWR);
    if(fd < 0)
    {
        perror("open err error!\n");
    }
    ret = write(fd,TRIGGER_NONE,strlen(TRIGGER_NONE));
    if(ret < 0)
    {
        perror("write err error!\n");
    }
    close(fd);
}
void beep_di()
{
    int fd;
    fd = open(DI_PATH,O_RDWR);
    if(fd < 0)
    {
        perror("open beep error");
    }
    write(fd,"60",strlen("60"));
    usleep(50000);
    write(fd,"0",strlen("0"));
    close(fd);
}
void beep_none()
{
    int fd;
    int ret;

    fd = open(BEEP_PATH,O_RDWR);
    if(fd < 0)
    {
        perror("open beep error!");
    }
    ret = write(fd,TRIGGER_NONE,strlen(TRIGGER_NONE));
    if(ret < 0)
    {
        perror("write beep error!");
    }
    close(fd);
    flag_beeping = 0;
}
void gpio_on()
{
    int fd_export = open(EXPORT_PATH,O_WRONLY);   //打开gpio设备导出设备
    if(fd_export < 0)
    {
        perror("open export error");
    }
    write(fd_export,GPIO3_4,strlen(GPIO3_4));   //向export文件写入gpio排列序号字符串
    int fd_dir;
    int ret;
    fd_dir = open(DIRECT_PATH_3_4,O_RDWR);
    if(fd_dir < 0)
    {
        perror("open io error!");
    }
    ret = write(fd_dir,DIRECT_OUT,sizeof(DIRECT_OUT));
    if(ret < 0)
    {
        perror("write direct error!");
    }
    int fd_val;
    fd_val = open(VALUE_PATH_3_4,O_RDWR);
    if(fd_val < 0)
    {
        perror("value set error!");
    }
    ret = write(fd_val,ON,strlen(ON));
    if(ret < 0)
    {
        perror("write value error!");
    }
    close(fd_export);
    close(fd_dir);
    close(fd_val);
}
void gpio_off()
{
    int fd_export = open(EXPORT_PATH,O_WRONLY);   //打开gpio设备导出设备
    if(fd_export < 0)
    {
        perror("open export error");
    }
    write(fd_export,GPIO3_4,strlen(GPIO3_4));   //向export文件写入gpio排列序号字符串
    int fd_dir;
    int ret;
    fd_dir = open(DIRECT_PATH_3_4,O_RDWR);
    if(fd_dir < 0)
    {
        perror("open io error!");
    }
    ret = write(fd_dir,DIRECT_OUT,sizeof(DIRECT_OUT));
    if(ret < 0)
    {
        perror("write direct error!");
    }
    int fd_val;
    fd_val = open(VALUE_PATH_3_4,O_RDWR);
    if(fd_val < 0)
    {
        perror("value set error!");
    }
    ret = write(fd_val,OFF,strlen(OFF));
    if(ret < 0)
    {
        perror("write value error!");
    }
    close(fd_export);
    close(fd_dir);
    close(fd_val);
}
void io_init()
{
}

void gpio_low(int num_io1,int num_io2)
{
    QString DIRECT_PATH1 = "/sys/class/gpio/gpio";
    QString DIRECT_PATH2 = "/direction";
    QString VALUE_PATH1 = "/sys/class/gpio/gpio";
    QString VALUE_PATH2 = "/value";

    int IO_NUM = (num_io1-1)*32+num_io2;

    QString GPIO_NUM = QString::number(IO_NUM,10);
	QString DIRECT_PATH = DIRECT_PATH1.append(GPIO_NUM).append(DIRECT_PATH2);
	QString VALUE_PATH = VALUE_PATH1.append(GPIO_NUM).append(VALUE_PATH2);

    const char * gpio_R = GPIO_NUM.toStdString().data();
    const char * direct_path_R = DIRECT_PATH.toStdString().data();
    const char * value_path_R = VALUE_PATH.toStdString().data();

    int fd_export = open(EXPORT_PATH,O_WRONLY);   //打开gpio设备导出设备
    if(fd_export < 0)
    {
        perror("open export error");
    }
    write(fd_export,gpio_R,strlen(gpio_R));   //向export文件写入gpio排列序号字符串
    int fd_dir;
    int ret;
    fd_dir = open(direct_path_R,O_RDWR);
    if(fd_dir < 0)
    {
        perror("open io error!");
    }
    ret = write(fd_dir,DIRECT_OUT,sizeof(DIRECT_OUT));
    if(ret < 0)
    {
        perror("write direct error!");
    }
    int fd_val;
    fd_val = open(value_path_R,O_RDWR);
    if(fd_val < 0)
    {
        perror("value set error!");
    }
    ret = write(fd_val,OFF,strlen(OFF));
    if(ret < 0)
    {
        perror("write value error!");
    }
    close(fd_export);
    close(fd_dir);
    close(fd_val);
}
void gpio_high(int num_io1,int num_io2)
{
    QString DIRECT_PATH1 = "/sys/class/gpio/gpio";
    QString DIRECT_PATH2 = "/direction";
    QString VALUE_PATH1 = "/sys/class/gpio/gpio";
    QString VALUE_PATH2 = "/value";

    int IO_NUM = (num_io1-1)*32+num_io2;

    QString GPIO_NUM = QString::number(IO_NUM,10);
	QString DIRECT_PATH = DIRECT_PATH1.append(GPIO_NUM).append(DIRECT_PATH2);
	QString VALUE_PATH = VALUE_PATH1.append(GPIO_NUM).append(VALUE_PATH2);

    const char * gpio_R = GPIO_NUM.toStdString().data();
    const char * direct_path_R = DIRECT_PATH.toStdString().data();
    const char * value_path_R = VALUE_PATH.toStdString().data();

    int fd_export = open(EXPORT_PATH,O_WRONLY);   //打开gpio设备导出设备
    if(fd_export < 0)
    {
        perror("open export error");
    }
    write(fd_export,gpio_R,strlen(gpio_R));   //向export文件写入gpio排列序号字符串
    int fd_dir;
    int ret;
    fd_dir = open(direct_path_R,O_RDWR);
    if(fd_dir < 0)
    {
        perror("open io error!");
    }
    ret = write(fd_dir,DIRECT_OUT,sizeof(DIRECT_OUT));
    if(ret < 0)
    {
        perror("write direct error!");
    }
    int fd_val;
    fd_val = open(value_path_R,O_RDWR);
    if(fd_val < 0)
    {
        perror("value set error!");
    }
    ret = write(fd_val,ON,strlen(ON));
    if(ret < 0)
    {
        perror("write value error!");
    }
    close(fd_export);
    close(fd_dir);
    close(fd_val);
}



int gpio5_5_read()
{
    char buf[10];
//    int fd_dev;
	int fd_export,fd_dir,ret;
    fd_export = open(EXPORT_PATH,O_WRONLY);   //打开gpio设备导出设备
    if(fd_export < 0)
    {
        perror("open export error");
    }
    write(fd_export,GPIO5_5,strlen(GPIO5_5));   //向export文件写入gpio排列序号字符串

//    fd_dev = open(VALUE_PATH_5_5,O_RDWR);
//    if(fd_dev < 0)
//    {
//            perror("open gpio:");
//            return -1;
//    }
    fd_dir = open(DIRECT_PATH_5_5,O_RDWR);
    if(fd_dir < 0)
    {
        perror("open io error!");
    }
    ret = write(fd_dir,DIRECT_IN,sizeof(DIRECT_IN));
    if(ret < 0)
    {
        perror("write direct error!");
    }
    int fd_val;
    fd_val = open(VALUE_PATH_5_5,O_RDWR);
    if(fd_val < 0)
    {
        perror("value set error!");
    }
    ret = read(fd_val,buf,sizeof(buf));
    if(ret < 0)
    {
        perror("write value error!");
    }
    close(fd_export);
    close(fd_dir);
    close(fd_val);
    int x = int(buf[0]);
    //printf("buf0000000000%d",int(buf[0]));
    return x-48;
}
int gpio5_6_read()
{
    char buf[10];
    int fd_export = open(EXPORT_PATH,O_WRONLY);   //打开gpio设备导出设备
    if(fd_export < 0)
    {
        perror("open export error");
    }
    write(fd_export,GPIO5_6,strlen(GPIO5_6));   //向export文件写入gpio排列序号字符串
    int fd_dir;
    int ret;
    fd_dir = open(DIRECT_PATH_5_6,O_RDWR);
    if(fd_dir < 0)
    {
        perror("open io error!");
    }
    ret = write(fd_dir,DIRECT_IN,sizeof(DIRECT_IN));
    if(ret < 0)
    {
        perror("write direct error!");
    }
    int fd_val;
    fd_val = open(VALUE_PATH_5_6,O_RDWR);
    if(fd_val < 0)
    {
        perror("value set error!");
    }
    ret = read(fd_val,buf,sizeof(buf));
    if(ret < 0)
    {
        perror("write value error!");
    }
    close(fd_export);
    close(fd_dir);
    close(fd_val);
    int x = int(buf[0]);
    return x-48;
}











