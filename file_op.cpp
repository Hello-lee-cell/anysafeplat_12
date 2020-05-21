#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>
#include<stdio.h>
#include<string.h>
#include<QDebug>
#include<QDateTime>
#include<QFile>
#include"file_op.h"
#include"mainwindow.h"
#include"config.h"
#include"serial.h"
#include"radar_485.h"
#include"systemset.h"
#include"security.h"


//将传感器数量写入文本
void config_SensorAmountChanged()
{
    char amount_sensor[10] = {0};
    sprintf(amount_sensor,"%d\n%d\n%d\n%d\n%d\n%d/n",count_tank,count_pipe,count_dispener,count_basin,Test_Method,Flag_pre_mode);
    QString q_amount_sensor;
    q_amount_sensor = QString(QLatin1String(amount_sensor));
    QFile file(CONFIG_SENSORAMOUNT);
    file.open(QIODevice::WriteOnly |QIODevice::Text |QIODevice::Truncate);
    QTextStream in(&file);
    in<<q_amount_sensor;
    file.close();

    int fp = open(CONFIG_SENSORAMOUNT,O_RDONLY);
    fsync(fp);
    close(fp);
}

//网络状态写入文本
void history_net_write(const char *t)      //right   warn   error
{
    QDateTime current_datetime = QDateTime::currentDateTime();
    QString current_datetime_qstr = current_datetime.toString("yyyy-MM-dd  hh:mm:ss");
    qDebug()<<current_datetime_qstr<<endl;
    if(strcmp(t,"online") == 0)
    {
        const char *net_state = "  网络状态正常";
        //写入
        QFile file(HISTORY_NET);
        file.open(QIODevice::WriteOnly |QIODevice::Text |QIODevice::Truncate);
        QTextStream in(&file);
        in<<current_datetime_qstr + " " + net_state+"\r\n";
        file.close();
        //追加
        int f_sor;
        int f_des;
        int len_;
        char buff[1024];
        f_sor = ::open(HISTORY_NET_T,O_RDONLY);
        f_des = ::open(HISTORY_NET,O_RDWR|O_CREAT);

        while((len_ = read(f_sor,buff,1024)))
        {
            lseek(f_des,0,SEEK_END);
            write(f_des,buff,len_);
        }
        fsync(f_des);
        ::close(f_sor);
        ::close(f_des);
        history_net_del();
        //复制
        f_sor = ::open(HISTORY_NET,O_RDONLY);
        f_des = ::open(HISTORY_NET_T,O_RDWR|O_CREAT);

        while((len_ = read(f_sor,buff,1024)))
        {
            write(f_des,buff,len_);
        }
        fsync(f_des);
        ::close(f_sor);
        ::close(f_des);
    }
    if(strcmp(t,"net_warn") == 0)
    {
        const char *net_state = "  KeepAlive包丢失";
        //写入
        QFile file(HISTORY_NET);
        file.open(QIODevice::WriteOnly |QIODevice::Text |QIODevice::Truncate);
        QTextStream in(&file);
        in<<current_datetime_qstr + " " + net_state+"\r\n";
        file.close();
        //追加
        int f_sor;
        int f_des;
        int len_;
        char buff[1024];
        f_sor = ::open(HISTORY_NET_T,O_RDONLY);
        f_des = ::open(HISTORY_NET,O_RDWR|O_CREAT);

        while((len_ = read(f_sor,buff,1024)))
        {
            lseek(f_des,0,SEEK_END);
            write(f_des,buff,len_);
        }
        fsync(f_des);
        ::close(f_sor);
        ::close(f_des);
        history_net_del();
        //复制
        f_sor = ::open(HISTORY_NET,O_RDONLY);
        f_des = ::open(HISTORY_NET_T,O_RDWR|O_CREAT);

        while((len_ = read(f_sor,buff,1024)))
        {
            write(f_des,buff,len_);
        }
        fsync(f_des);
        ::close(f_sor);
        ::close(f_des);
    }
    if(strcmp(t,"net_off") == 0)
    {
        const char *net_state = "  网络已断开";
        //写入
        QFile file(HISTORY_NET);
        file.open(QIODevice::WriteOnly |QIODevice::Text |QIODevice::Truncate);
        QTextStream in(&file);
        in<<current_datetime_qstr + " " + net_state+"\r\n";
        file.close();
        //追加
        int f_sor;
        int f_des;
        int len_;
        char buff[1024];
        f_sor = ::open(HISTORY_NET_T,O_RDONLY);
        f_des = ::open(HISTORY_NET,O_RDWR|O_CREAT);

        while((len_ = read(f_sor,buff,1024)))
        {
            lseek(f_des,0,SEEK_END);
            write(f_des,buff,len_);
        }
        fsync(f_des);
        ::close(f_sor);
        ::close(f_des);
        history_net_del();
        //复制
        f_sor = ::open(HISTORY_NET,O_RDONLY);
        f_des = ::open(HISTORY_NET_T,O_RDWR|O_CREAT);

        while((len_ = read(f_sor,buff,1024)))
        {
            write(f_des,buff,len_);
        }
        fsync(f_des);
        ::close(f_sor);
        ::close(f_des);
    }
    if(strcmp(t,"Tcp_Obtain") == 0)
    {
        const char *net_state = "  TCP Socket创建成功";
        //写入
        QFile file(HISTORY_NET);
        file.open(QIODevice::WriteOnly |QIODevice::Text |QIODevice::Truncate);
        QTextStream in(&file);
        in<<current_datetime_qstr + " " + net_state+"\r\n";
        file.close();
        //追加
        int f_sor;
        int f_des;
        int len_;
        char buff[1024];
        f_sor = ::open(HISTORY_NET_T,O_RDONLY);
        f_des = ::open(HISTORY_NET,O_RDWR|O_CREAT);

        while((len_ = read(f_sor,buff,1024)))
        {
            lseek(f_des,0,SEEK_END);
            write(f_des,buff,len_);
        }
        fsync(f_des);
        ::close(f_sor);
        ::close(f_des);
        history_net_del();
        //复制
        f_sor = ::open(HISTORY_NET,O_RDONLY);
        f_des = ::open(HISTORY_NET_T,O_RDWR|O_CREAT);

        while((len_ = read(f_sor,buff,1024)))
        {
            write(f_des,buff,len_);
        }
        fsync(f_des);
        ::close(f_sor);
        ::close(f_des);
    }
    if(strcmp(t,"Tcp_Bind") == 0)
    {
        const char *net_state = "  TCP服务器端口绑定成功";
        //写入
        QFile file(HISTORY_NET);
        file.open(QIODevice::WriteOnly |QIODevice::Text |QIODevice::Truncate);
        QTextStream in(&file);
        in<<current_datetime_qstr + " " + net_state+"\r\n";
        file.close();
        //追加
        int f_sor;
        int f_des;
        int len_;
        char buff[1024];
        f_sor = ::open(HISTORY_NET_T,O_RDONLY);
        f_des = ::open(HISTORY_NET,O_RDWR|O_CREAT);

        while((len_ = read(f_sor,buff,1024)))
        {
            lseek(f_des,0,SEEK_END);
            write(f_des,buff,len_);
        }
        fsync(f_des);
        ::close(f_sor);
        ::close(f_des);
        history_net_del();
        //复制
        f_sor = ::open(HISTORY_NET,O_RDONLY);
        f_des = ::open(HISTORY_NET_T,O_RDWR|O_CREAT);

        while((len_ = read(f_sor,buff,1024)))
        {
            write(f_des,buff,len_);
        }
        fsync(f_des);
        ::close(f_sor);
        ::close(f_des);
    }
    if(strcmp(t,"Tcp_Listen") == 0)
    {
        const char *net_state = "  TCP服务器端口监听中";
        //写入
        QFile file(HISTORY_NET);
        file.open(QIODevice::WriteOnly |QIODevice::Text |QIODevice::Truncate);
        QTextStream in(&file);
        in<<current_datetime_qstr + " " + net_state+"\r\n";
        file.close();
        //追加
        int f_sor;
        int f_des;
        int len_;
        char buff[1024];
        f_sor = ::open(HISTORY_NET_T,O_RDONLY);
        f_des = ::open(HISTORY_NET,O_RDWR|O_CREAT);

        while((len_ = read(f_sor,buff,1024)))
        {
            lseek(f_des,0,SEEK_END);
            write(f_des,buff,len_);
        }
        fsync(f_des);
        ::close(f_sor);
        ::close(f_des);
        history_net_del();
        //复制
        f_sor = ::open(HISTORY_NET,O_RDONLY);
        f_des = ::open(HISTORY_NET_T,O_RDWR|O_CREAT);

        while((len_ = read(f_sor,buff,1024)))
        {
            write(f_des,buff,len_);
        }
        fsync(f_des);
        ::close(f_sor);
        ::close(f_des);
    }
    if(strcmp(t,"Tcp_connect") == 0)
    {
        const char *net_state = "  TCP连接已建立";
        //写入
        QFile file(HISTORY_NET);
        file.open(QIODevice::WriteOnly |QIODevice::Text |QIODevice::Truncate);
        QTextStream in(&file);
        in<<current_datetime_qstr + " " + net_state+"\r\n";
        file.close();
        //追加
        int f_sor;
        int f_des;
        int len_;
        char buff[1024];
        f_sor = ::open(HISTORY_NET_T,O_RDONLY);
        f_des = ::open(HISTORY_NET,O_RDWR|O_CREAT);

        while((len_ = read(f_sor,buff,1024)))
        {
            lseek(f_des,0,SEEK_END);
            write(f_des,buff,len_);
        }
        fsync(f_des);
        ::close(f_sor);
        ::close(f_des);
        history_net_del();
        //复制
        f_sor = ::open(HISTORY_NET,O_RDONLY);
        f_des = ::open(HISTORY_NET_T,O_RDWR|O_CREAT);

        while((len_ = read(f_sor,buff,1024)))
        {
            write(f_des,buff,len_);
        }
        fsync(f_des);
        ::close(f_sor);
        ::close(f_des);
    }
    if(strcmp(t,"Tcp_shutdown") == 0)
    {
        const char *net_state = "  TCP连接已断开";
        //写入
        QFile file(HISTORY_NET);
        file.open(QIODevice::WriteOnly |QIODevice::Text |QIODevice::Truncate);
        QTextStream in(&file);
        in<<current_datetime_qstr + " " + net_state+"\r\n";
        file.close();
        //追加
        int f_sor;
        int f_des;
        int len_;
        char buff[1024];
        f_sor = ::open(HISTORY_NET_T,O_RDONLY);
        f_des = ::open(HISTORY_NET,O_RDWR|O_CREAT);

        while((len_ = read(f_sor,buff,1024)))
        {
            lseek(f_des,0,SEEK_END);
            write(f_des,buff,len_);
        }
        fsync(f_des);
        ::close(f_sor);
        ::close(f_des);
        history_net_del();
        //复制
        f_sor = ::open(HISTORY_NET,O_RDONLY);
        f_des = ::open(HISTORY_NET_T,O_RDWR|O_CREAT);

        while((len_ = read(f_sor,buff,1024)))
        {
            write(f_des,buff,len_);
        }
        fsync(f_des);
        ::close(f_sor);
        ::close(f_des);
    }
    if(strcmp(t,"Udp_listening") == 0)
    {
        const char *net_state = "  UDP服务器监听中";
        //写入
        QFile file(HISTORY_NET);
        file.open(QIODevice::WriteOnly |QIODevice::Text |QIODevice::Truncate);
        QTextStream in(&file);
        in<<current_datetime_qstr + " " + net_state+"\r\n";
        file.close();
        //追加
        int f_sor;
        int f_des;
        int len_;
        char buff[1024];
        f_sor = ::open(HISTORY_NET_T,O_RDONLY);
        f_des = ::open(HISTORY_NET,O_RDWR|O_CREAT);

        while((len_ = read(f_sor,buff,1024)))
        {
            lseek(f_des,0,SEEK_END);
            write(f_des,buff,len_);
        }
        fsync(f_des);
        ::close(f_sor);
        ::close(f_des);
        history_net_del();
        //复制
        f_sor = ::open(HISTORY_NET,O_RDONLY);
        f_des = ::open(HISTORY_NET_T,O_RDWR|O_CREAT);

        while((len_ = read(f_sor,buff,1024)))
        {
            write(f_des,buff,len_);
        }
        fsync(f_des);
        ::close(f_sor);
        ::close(f_des);
    }
    if(strcmp(t,"Udp_talk") == 0)
    {
        const char *net_state = "  UDP客户端广播中";
        //写入
        QFile file(HISTORY_NET);
        file.open(QIODevice::WriteOnly |QIODevice::Text |QIODevice::Truncate);
        QTextStream in(&file);
        in<<current_datetime_qstr + " " + net_state+"\r\n";
        file.close();
        //追加
        int f_sor;
        int f_des;
        int len_;
        char buff[1024];
        f_sor = ::open(HISTORY_NET_T,O_RDONLY);
        f_des = ::open(HISTORY_NET,O_RDWR|O_CREAT);

        while((len_ = read(f_sor,buff,1024)))
        {
            lseek(f_des,0,SEEK_END);
            write(f_des,buff,len_);
        }
        fsync(f_des);
        ::close(f_sor);
        ::close(f_des);
        history_net_del();
        //复制
        f_sor = ::open(HISTORY_NET,O_RDONLY);
        f_des = ::open(HISTORY_NET_T,O_RDWR|O_CREAT);

        while((len_ = read(f_sor,buff,1024)))
        {
            write(f_des,buff,len_);
        }
        fsync(f_des);
        ::close(f_sor);
        ::close(f_des);
    }
}

void history_net_del()              //nNum = 2000   此处为33行  删33行则nNum = 32
{
    QString strall;
    QFile readfile(HISTORY_NET);
    if(readfile.open(QIODevice::ReadOnly))
    {
        QTextStream stream(&readfile);
        strall = stream.readAll();
    }
    readfile.close();

    //delete one line
    int nLine = 0;
    int Index = 0;
    //算出行数nLine
    while(Index != -1)
    {
        Index = strall.indexOf('\n',Index + 1);
        nLine++;
    }

    int nTemp = DELETE_WHICHLINE;
    int nIndex = 0;
    int nIndex2 = 0;
    while(nTemp--)
    {
        nIndex = strall.indexOf('\n',nIndex + 1);
        if(nIndex != -1)  //说明是有效的
        {
            nIndex2 = strall.indexOf('\n',nIndex + 1);
        }
    }
    //删除的行不是最后一行（从nindex+1这个位置起nIndex2-nIndex个字符全部抹去
    if(DELETE_WHICHLINE < nLine - 1)
    {
        strall.remove(nIndex + 1,nIndex2 - nIndex);//不用减一
    }
    else if(DELETE_WHICHLINE == nLine - 1)
    {
        int len = strall.length();
        strall.remove(nIndex,len - nIndex);
    }

    QFile writefile(HISTORY_NET);
    if(writefile.open(QIODevice::WriteOnly))
    {
        QTextStream wrtstream(&writefile);
        wrtstream<<strall;
    }
    writefile.close();

    int fd = open(HISTORY_NET,O_RDONLY);

    fsync(fd);
    close(fd);
}

//**压力法记录用
void history_sensor_write_pr(int t)
{

    QString pr = QString::number(count_Pressure[t],'f',2);
   QString n = QString::number(t+1);

    QDateTime current_datetime = QDateTime::currentDateTime();
    QString current_datetime_qstr = current_datetime.toString("yyyy-MM-dd  hh:mm:ss");

    //写入
    QFile file(HISTORY_SENSOR);
    file.open(QIODevice::WriteOnly |QIODevice::Text |QIODevice::Truncate);
    QTextStream in(&file);
    in<<current_datetime_qstr + "  " +n+"# 油罐"+"   "+ pr +"KPa" +"\r\n";
    file.close();
    //追加
    int f_sor;
    int f_des;
    int len_;
    char buff[1024];
    f_sor = ::open(HISTORY_SENSOR_T,O_RDONLY);
    f_des = ::open(HISTORY_SENSOR,O_RDWR|O_CREAT);

    while((len_ = read(f_sor,buff,1024)))
    {
        lseek(f_des,0,SEEK_END);
        write(f_des,buff,len_);
    }
    fsync(f_des);
    ::close(f_sor);
    ::close(f_des);
    history_sensor_del();
    //复制
    f_sor = ::open(HISTORY_SENSOR,O_RDONLY);
    f_des = ::open(HISTORY_SENSOR_T,O_RDWR|O_CREAT);

    while((len_ = read(f_sor,buff,1024)))
    {
        write(f_des,buff,len_);
    }
    fsync(f_des);
    ::close(f_sor);
    ::close(f_des);
}

void history_sensor_write(const char *numb,const char *t)
{
    QDateTime current_datetime = QDateTime::currentDateTime();
    QString current_datetime_qstr = current_datetime.toString("yyyy-MM-dd  hh:mm:ss");

    //写入
    QFile file(HISTORY_SENSOR);
    file.open(QIODevice::WriteOnly |QIODevice::Text |QIODevice::Truncate);
    QTextStream in(&file);
    in<<current_datetime_qstr + "  " + numb + t +"\r\n";
    file.close();
    //追加
    int f_sor;
    int f_des;
    int len_;
    char buff[1024];
    f_sor = ::open(HISTORY_SENSOR_T,O_RDONLY);
    f_des = ::open(HISTORY_SENSOR,O_RDWR|O_CREAT);

    while((len_ = read(f_sor,buff,1024)))
    {
        lseek(f_des,0,SEEK_END);
        write(f_des,buff,len_);
    }
    fsync(f_des);
    ::close(f_sor);
    ::close(f_des);
    history_sensor_del();
    //复制
    f_sor = ::open(HISTORY_SENSOR,O_RDONLY);
    f_des = ::open(HISTORY_SENSOR_T,O_RDWR|O_CREAT);

    while((len_ = read(f_sor,buff,1024)))
    {
        write(f_des,buff,len_);
    }
    fsync(f_des);
    ::close(f_sor);
    ::close(f_des);
}

void history_sensor_del()
{
    QString strall;
    QFile readfile(HISTORY_SENSOR);
    if(readfile.open(QIODevice::ReadOnly))
    {
        QTextStream stream(&readfile);
        strall = stream.readAll();
    }
    readfile.close();

    //delete one line
    int nLine = 0;
    int Index = 0;
    //算出行数nLine
    while(Index != -1)
    {
        Index = strall.indexOf('\n',Index + 1);
        nLine++;
    }

    int nTemp = DELETE_WHICHLINE;
    int nIndex = 0;
    int nIndex2 = 0;
    while(nTemp--)
    {
        nIndex = strall.indexOf('\n',nIndex + 1);
        if(nIndex != -1)  //说明是有效的
        {
            nIndex2 = strall.indexOf('\n',nIndex + 1);
        }
    }
    //删除的行不是最后一行（从nindex+1这个位置起nIndex2-nIndex个字符全部抹去
    if(DELETE_WHICHLINE < nLine - 1)
    {
        strall.remove(nIndex + 1,nIndex2 - nIndex);//不用减一
    }
    else if(DELETE_WHICHLINE == nLine - 1)
    {
        int len = strall.length();
        strall.remove(nIndex,len - nIndex);
    }

    QFile writefile(HISTORY_SENSOR);
    if(writefile.open(QIODevice::WriteOnly))
    {
        QTextStream wrtstream(&writefile);
        wrtstream<<strall;
    }
    writefile.close();

  //  char buff[11] = {0};
    int fd = open(HISTORY_SENSOR,O_RDONLY);
  //  read(fd,buff,10);
    fsync(fd);
    close(fd);
}

void history_operate_write(char *t)
{
    //开机记录
    if(strcmp(t,"m_open") == 0)
    {
        //写入
        QString a = "";     //应该是强制转换qtextstream in中输入的const char
        const char *mechine_start = "正常开机";
        QFile file(HISTORY_OPERATE);
        file.open(QIODevice::WriteOnly |QIODevice::Text |QIODevice::Truncate);
        QTextStream in(&file);
        in<<a + "    " + mechine_start + "\r\n";
        file.close();
        //追加
        int f_sor;
        int f_des;
        int len_;
        char buff[1024];
        f_sor = ::open(HISTORY_OPERATE_T,O_RDONLY);
        f_des = ::open(HISTORY_OPERATE,O_RDWR|O_CREAT);

        while((len_ = read(f_sor,buff,1024)))
        {
            lseek(f_des,0,SEEK_END);
            write(f_des,buff,len_);
        }
        fsync(f_des);
        ::close(f_sor);
        ::close(f_des);
        history_operate_del();
        //复制
        f_sor = ::open(HISTORY_OPERATE,O_RDONLY);
        f_des = ::open(HISTORY_OPERATE_T,O_RDWR,O_CREAT);

        while((len_ = read(f_sor,buff,1024)))
        {
            write(f_des,buff,len_);
        }
        fsync(f_des);
        ::close(f_sor);
        ::close(f_des);

        //第二步：时间记录
        QDateTime current_datetime = QDateTime::currentDateTime();
        QString current_datetime_qstr = current_datetime.toString("yyyy-MM-dd  hh:mm:ss");
            //时间写入
        QFile file_time(HISTORY_OPERATE);
        file_time.open(QIODevice::WriteOnly |QIODevice::Text |QIODevice::Truncate); //覆盖
        QTextStream in_time(&file_time);
        in_time<<current_datetime_qstr+"\r\n";
        file_time.close();
            //时间追加

        f_sor = ::open(HISTORY_OPERATE_T,O_RDONLY);
        f_des = ::open(HISTORY_OPERATE,O_RDWR|O_CREAT);

        while((len_ = read(f_sor,buff,1024)))
        {
            lseek(f_des,0,SEEK_END);
            write(f_des,buff,len_);
        }
        fsync(f_des);
        ::close(f_sor);
        ::close(f_des);
        history_operate_del();
            //时间复制
        f_sor = ::open(HISTORY_OPERATE,O_RDONLY);
        f_des = ::open(HISTORY_OPERATE_T,O_RDWR|O_CREAT);

        while((len_ = read(f_sor,buff,1024)))
        {
            write(f_des,buff,len_);
        }
        fsync(f_des);
        ::close(f_sor);
        ::close(f_des);
    }
}

void history_operate_write_detail(const QString &text, const char *t)
{
    //第1步：详细内容记录（操作人，操作内容）

    // if(strcmp(t,"m_close") == 0)

    //写入
    const char *mechine_close = t;
    QFile file(HISTORY_OPERATE);
    file.open(QIODevice::WriteOnly |QIODevice::Text |QIODevice::Truncate);
    QTextStream in(&file);
    in<<"    用户" + text+ "  " + mechine_close +"\r\n";
    file.close();
    //追加
    int f_sor;
    int f_des;
    int len_;
    char buff[1024];
    f_sor = ::open(HISTORY_OPERATE_T,O_RDONLY);
    f_des = ::open(HISTORY_OPERATE,O_RDWR|O_CREAT);

    while((len_ = read(f_sor,buff,1024)))
    {
        lseek(f_des,0,SEEK_END);
        write(f_des,buff,len_);
    }
    fsync(f_des);
    ::close(f_sor);
    ::close(f_des);
    history_operate_del();
    //复制
    f_sor = ::open(HISTORY_OPERATE,O_RDONLY);
    f_des = ::open(HISTORY_OPERATE_T,O_RDWR|O_CREAT);

    while((len_ = read(f_sor,buff,1024)))
    {
        write(f_des,buff,len_);
    }
    fsync(f_des);
    ::close(f_sor);
    ::close(f_des);

    //第2步：时间记录
    QDateTime current_datetime = QDateTime::currentDateTime();
    QString current_datetime_qstr = current_datetime.toString("yyyy-MM-dd  hh:mm:ss");
        //时间写入
    QFile file_time(HISTORY_OPERATE);
    file_time.open(QIODevice::WriteOnly |QIODevice::Text |QIODevice::Truncate); //覆盖
    QTextStream in_time(&file_time);
    in_time<<current_datetime_qstr+"\r\n";
    file_time.close();
        //时间追加
    f_sor = ::open(HISTORY_OPERATE_T,O_RDONLY);
    f_des = ::open(HISTORY_OPERATE,O_RDWR|O_CREAT);

    while((len_ = read(f_sor,buff,1024)))
    {
        lseek(f_des,0,SEEK_END);
        write(f_des,buff,len_);
    }
    fsync(f_des);
    ::close(f_sor);
    ::close(f_des);
    history_operate_del();
        //时间复制
    f_sor = ::open(HISTORY_OPERATE,O_RDONLY);
    f_des = ::open(HISTORY_OPERATE_T,O_RDWR|O_CREAT);

    while((len_ = read(f_sor,buff,1024)))
    {
        write(f_des,buff,len_);
    }
    fsync(f_des);
    ::close(f_sor);
    ::close(f_des);
}
void history_operate_del()
{
    QString strall;
    QFile readfile(HISTORY_OPERATE);
    if(readfile.open(QIODevice::ReadOnly))
    {
        QTextStream stream(&readfile);
        strall = stream.readAll();
    }
    readfile.close();

    //delete one line
    int nLine = 0;
    int Index = 0;
    //算出行数nLine
    while(Index != -1)
    {
        Index = strall.indexOf('\n',Index + 1);
        nLine++;
    }

    int nTemp = DELETE_WHICHLINE;
    int nIndex = 0;
    int nIndex2 = 0;
    while(nTemp--)
    {
        nIndex = strall.indexOf('\n',nIndex + 1);
        if(nIndex != -1)  //说明是有效的
        {
            nIndex2 = strall.indexOf('\n',nIndex + 1);
        }
    }
    //删除的行不是最后一行（从nindex+1这个位置起nIndex2-nIndex个字符全部抹去
    if(DELETE_WHICHLINE < nLine - 1)
    {
        strall.remove(nIndex + 1,nIndex2 - nIndex);//不用减一
    }
    else if(DELETE_WHICHLINE == nLine - 1)
    {
        int len = strall.length();
        strall.remove(nIndex,len - nIndex);
    }

    QFile writefile(HISTORY_OPERATE);
    if(writefile.open(QIODevice::WriteOnly))
    {
        QTextStream wrtstream(&writefile);
        wrtstream<<strall;
    }
    writefile.close();

  //  char buff[11] = {0};
    int fd = open(HISTORY_OPERATE,O_RDONLY);
  //  read(fd,buff,10);
    fsync(fd);
    close(fd);
}
void history_clearall()
{
    system("rm -f alptecdata.db");
    system("reboot");
}
void all_sta_clean()
{
    system("echo 0 > /opt/reoilgas/info_accum/1-1");
    system("echo 0 > /opt/reoilgas/info_accum/1-2");
    system("echo 0 > /opt/reoilgas/info_accum/1-3");
    system("echo 0 > /opt/reoilgas/info_accum/1-4");
    system("echo 0 > /opt/reoilgas/info_accum/1-5");
    system("echo 0 > /opt/reoilgas/info_accum/1-6");
    system("echo 0 > /opt/reoilgas/info_accum/1-7");
    system("echo 0 > /opt/reoilgas/info_accum/1-8");

    system("echo 0 > /opt/reoilgas/info_accum/2-1");
    system("echo 0 > /opt/reoilgas/info_accum/2-2");
    system("echo 0 > /opt/reoilgas/info_accum/2-3");
    system("echo 0 > /opt/reoilgas/info_accum/2-4");
    system("echo 0 > /opt/reoilgas/info_accum/2-5");
    system("echo 0 > /opt/reoilgas/info_accum/2-6");
    system("echo 0 > /opt/reoilgas/info_accum/2-7");
    system("echo 0 > /opt/reoilgas/info_accum/2-8");

    system("echo 0 > /opt/reoilgas/info_accum/3-1");
    system("echo 0 > /opt/reoilgas/info_accum/3-2");
    system("echo 0 > /opt/reoilgas/info_accum/3-3");
    system("echo 0 > /opt/reoilgas/info_accum/3-4");
    system("echo 0 > /opt/reoilgas/info_accum/3-5");
    system("echo 0 > /opt/reoilgas/info_accum/3-6");
    system("echo 0 > /opt/reoilgas/info_accum/3-7");
    system("echo 0 > /opt/reoilgas/info_accum/3-8");

    system("echo 0 > /opt/reoilgas/info_accum/4-1");
    system("echo 0 > /opt/reoilgas/info_accum/4-2");
    system("echo 0 > /opt/reoilgas/info_accum/4-3");
    system("echo 0 > /opt/reoilgas/info_accum/4-4");
    system("echo 0 > /opt/reoilgas/info_accum/4-5");
    system("echo 0 > /opt/reoilgas/info_accum/4-6");
    system("echo 0 > /opt/reoilgas/info_accum/4-7");
    system("echo 0 > /opt/reoilgas/info_accum/4-8");

    system("echo 0 > /opt/reoilgas/info_accum/5-1");
    system("echo 0 > /opt/reoilgas/info_accum/5-2");
    system("echo 0 > /opt/reoilgas/info_accum/5-3");
    system("echo 0 > /opt/reoilgas/info_accum/5-4");
    system("echo 0 > /opt/reoilgas/info_accum/5-5");
    system("echo 0 > /opt/reoilgas/info_accum/5-6");
    system("echo 0 > /opt/reoilgas/info_accum/5-7");
    system("echo 0 > /opt/reoilgas/info_accum/5-8");

    system("echo 0 > /opt/reoilgas/info_accum/6-1");
    system("echo 0 > /opt/reoilgas/info_accum/6-2");
    system("echo 0 > /opt/reoilgas/info_accum/6-3");
    system("echo 0 > /opt/reoilgas/info_accum/6-4");
    system("echo 0 > /opt/reoilgas/info_accum/6-5");
    system("echo 0 > /opt/reoilgas/info_accum/6-6");
    system("echo 0 > /opt/reoilgas/info_accum/6-7");
    system("echo 0 > /opt/reoilgas/info_accum/6-8");

    system("echo 0 > /opt/reoilgas/info_accum/7-1");
    system("echo 0 > /opt/reoilgas/info_accum/7-2");
    system("echo 0 > /opt/reoilgas/info_accum/7-3");
    system("echo 0 > /opt/reoilgas/info_accum/7-4");
    system("echo 0 > /opt/reoilgas/info_accum/7-5");
    system("echo 0 > /opt/reoilgas/info_accum/7-6");
    system("echo 0 > /opt/reoilgas/info_accum/7-7");
    system("echo 0 > /opt/reoilgas/info_accum/7-8");

    system("echo 0 > /opt/reoilgas/info_accum/8-1");
    system("echo 0 > /opt/reoilgas/info_accum/8-2");
    system("echo 0 > /opt/reoilgas/info_accum/8-3");
    system("echo 0 > /opt/reoilgas/info_accum/8-4");
    system("echo 0 > /opt/reoilgas/info_accum/8-5");
    system("echo 0 > /opt/reoilgas/info_accum/8-6");
    system("echo 0 > /opt/reoilgas/info_accum/8-7");
    system("echo 0 > /opt/reoilgas/info_accum/8-8");

    system("echo 0 > /opt/reoilgas/info_accum/9-1");
    system("echo 0 > /opt/reoilgas/info_accum/9-2");
    system("echo 0 > /opt/reoilgas/info_accum/9-3");
    system("echo 0 > /opt/reoilgas/info_accum/9-4");
    system("echo 0 > /opt/reoilgas/info_accum/9-5");
    system("echo 0 > /opt/reoilgas/info_accum/9-6");
    system("echo 0 > /opt/reoilgas/info_accum/9-7");
    system("echo 0 > /opt/reoilgas/info_accum/9-8");

    system("echo 0 > /opt/reoilgas/info_accum/10-1");
    system("echo 0 > /opt/reoilgas/info_accum/10-2");
    system("echo 0 > /opt/reoilgas/info_accum/10-3");
    system("echo 0 > /opt/reoilgas/info_accum/10-4");
    system("echo 0 > /opt/reoilgas/info_accum/10-5");
    system("echo 0 > /opt/reoilgas/info_accum/10-6");
    system("echo 0 > /opt/reoilgas/info_accum/10-7");
    system("echo 0 > /opt/reoilgas/info_accum/10-8");

    system("echo 0 > /opt/reoilgas/info_accum/11-1");
    system("echo 0 > /opt/reoilgas/info_accum/11-2");
    system("echo 0 > /opt/reoilgas/info_accum/11-3");
    system("echo 0 > /opt/reoilgas/info_accum/11-4");
    system("echo 0 > /opt/reoilgas/info_accum/11-5");
    system("echo 0 > /opt/reoilgas/info_accum/11-6");
    system("echo 0 > /opt/reoilgas/info_accum/11-7");
    system("echo 0 > /opt/reoilgas/info_accum/11-8");

    system("echo 0 > /opt/reoilgas/info_accum/12-1");
    system("echo 0 > /opt/reoilgas/info_accum/12-2");
    system("echo 0 > /opt/reoilgas/info_accum/12-3");
    system("echo 0 > /opt/reoilgas/info_accum/12-4");
    system("echo 0 > /opt/reoilgas/info_accum/12-5");
    system("echo 0 > /opt/reoilgas/info_accum/12-6");
    system("echo 0 > /opt/reoilgas/info_accum/12-7");
    system("echo 0 > /opt/reoilgas/info_accum/12-8");

    system("echo 0 > /opt/reoilgas/info_accum/50-50");
    system("echo 0 > /opt/reoilgas/info_accum/600-1000");

    system("sync");
    sleep(1);

    //system("rm -f alptecdata.db");
    system("reboot");

}

//*****added for radar*****/

//雷达历史记录
void history_radar_warn_write(const char *numb, char *t)
{
    QDateTime current_datetime = QDateTime::currentDateTime();
    QString current_datetime_qstr = current_datetime.toString("yyyy-MM-dd hh:mm:ss");

    //写入
    QFile file(HISTORY_RADAR);
    file.open(QIODevice::WriteOnly |QIODevice::Text |QIODevice::Truncate);
    QTextStream in(&file);
    in<<current_datetime_qstr + " " + numb + t +"\r\n";
    file.close();
    //追加
    int f_sor;
    int f_des;
    int len_;
    char buff[1024];
    f_sor = ::open(HISTORY_RADAR_T,O_RDONLY);
    f_des = ::open(HISTORY_RADAR,O_RDWR|O_CREAT);

    while((len_ = read(f_sor,buff,1024)))
    {
        lseek(f_des,0,SEEK_END);
        write(f_des,buff,len_);
    }
    fsync(f_des);
    ::close(f_sor);
    ::close(f_des);
    history_radar_warn_del();
    //复制
    f_sor = ::open(HISTORY_RADAR,O_RDONLY);
    f_des = ::open(HISTORY_RADAR_T,O_RDWR|O_CREAT);

    while((len_ = read(f_sor,buff,1024)))
    {
        write(f_des,buff,len_);
    }
    fsync(f_des);
    ::close(f_sor);
    ::close(f_des);
}

void history_radar_warn_del()
{
    QString strall;
    QFile readfile(HISTORY_RADAR);
    if(readfile.open(QIODevice::ReadOnly))
    {
        QTextStream stream(&readfile);
        strall = stream.readAll();
    }
    readfile.close();

    //delete one line
    int nLine = 0;
    int Index = 0;
    //算出行数nLine
    while(Index != -1)
    {
        Index = strall.indexOf('\n',Index + 1);
        nLine++;
    }

    int nTemp = DELETE_WHICHLINE;
    int nIndex = 0;
    int nIndex2 = 0;
    while(nTemp--)
    {
        nIndex = strall.indexOf('\n',nIndex + 1);
        if(nIndex != -1)  //说明是有效的
        {
            nIndex2 = strall.indexOf('\n',nIndex + 1);
        }
    }
    //删除的行不是最后一行（从nindex+1这个位置起nIndex2-nIndex个字符全部抹去
    if(DELETE_WHICHLINE < nLine - 1)
    {
        strall.remove(nIndex + 1,nIndex2 - nIndex);//不用减一
    }
    else if(DELETE_WHICHLINE == nLine - 1)
    {
        int len = strall.length();
        strall.remove(nIndex,len - nIndex);
    }

    QFile writefile(HISTORY_RADAR);
    if(writefile.open(QIODevice::WriteOnly))
    {
        QTextStream wrtstream(&writefile);
        wrtstream<<strall;
    }
    writefile.close();

  //  char buff[11] = {0};
    int fd = open(HISTORY_RADAR,O_RDONLY);
  //  read(fd,buff,10);
    fsync(fd);
    close(fd);
}

void history_jingdian_warn_write(const char *t)
{
    QDateTime current_datetime = QDateTime::currentDateTime();
    QString current_datetime_qstr = current_datetime.toString("yyyy-MM-dd hh:mm:ss");

    //写入
    QFile file(HISTORY_JINGDIAN);
    file.open(QIODevice::WriteOnly |QIODevice::Text |QIODevice::Truncate);
    QTextStream in(&file);
    in<<current_datetime_qstr + "   " + t +"\r\n";
    file.close();
    //追加
    int f_sor;
    int f_des;
    int len_;
    char buff[1024];
    f_sor = ::open(HISTORY_JINGDIAN_T,O_RDONLY);
    f_des = ::open(HISTORY_JINGDIAN,O_RDWR|O_CREAT);

    while((len_ = read(f_sor,buff,1024)))
    {
        lseek(f_des,0,SEEK_END);
        write(f_des,buff,len_);
    }
    fsync(f_des);
    ::close(f_sor);
    ::close(f_des);
    history_jingdian_warn_del();
    //复制
    f_sor = ::open(HISTORY_JINGDIAN,O_RDONLY);
    f_des = ::open(HISTORY_JINGDIAN_T,O_RDWR|O_CREAT);

    while((len_ = read(f_sor,buff,1024)))
    {
        write(f_des,buff,len_);
    }
    fsync(f_des);
    ::close(f_sor);
    ::close(f_des);
}
void history_jingdian_warn_del()
{
    QString strall;
    QFile readfile(HISTORY_JINGDIAN);
    if(readfile.open(QIODevice::ReadOnly))
    {
        QTextStream stream(&readfile);
        strall = stream.readAll();
    }
    readfile.close();

    //delete one line
    int nLine = 0;
    int Index = 0;
    //算出行数nLine
    while(Index != -1)
    {
        Index = strall.indexOf('\n',Index + 1);
        nLine++;
    }

    int nTemp = DELETE_WHICHLINE;
    int nIndex = 0;
    int nIndex2 = 0;
    while(nTemp--)
    {
        nIndex = strall.indexOf('\n',nIndex + 1);
        if(nIndex != -1)  //说明是有效的
        {
            nIndex2 = strall.indexOf('\n',nIndex + 1);
        }
    }
    //删除的行不是最后一行（从nindex+1这个位置起nIndex2-nIndex个字符全部抹去
    if(DELETE_WHICHLINE < nLine - 1)
    {
        strall.remove(nIndex + 1,nIndex2 - nIndex);//不用减一
    }
    else if(DELETE_WHICHLINE == nLine - 1)
    {
        int len = strall.length();
        strall.remove(nIndex,len - nIndex);
    }

    QFile writefile(HISTORY_JINGDIAN);
    if(writefile.open(QIODevice::WriteOnly))
    {
        QTextStream wrtstream(&writefile);
        wrtstream<<strall;
    }
    writefile.close();

  //  char buff[11] = {0};
    int fd = open(HISTORY_JINGDIAN,O_RDONLY);
  //  read(fd,buff,10);
    fsync(fd);
    close(fd);
}

//写雷达智能设置配置文件 config_zn.txt
void config_radar_znwrite()
{
    char radar_znconfig[50] = {0};
    sprintf(radar_znconfig,"%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n",Start_time_h,Start_time_m,Stop_time_h,Stop_time_m,
            Silent_time_h,Silent_time_m,Warn_delay_m,Warn_delay_s,Flag_outdoor_warn,Flag_area_ctrl[0],
            Flag_area_ctrl[1],Flag_area_ctrl[2],Flag_area_ctrl[3],Flag_sensitivity);
    QString q_radar_znconfig;
    q_radar_znconfig = QString(QLatin1String(radar_znconfig));
    QFile file("/opt/radar/config_zn.txt");
    file.open(QIODevice::WriteOnly |QIODevice::Text |QIODevice::Truncate);
    QTextStream in(&file);
    in<<q_radar_znconfig;
    file.close();

  //  char buff[11] = {0};
    int fp = open("/opt/radar/config_zn.txt",O_RDONLY);
  //  read(fp,buff,10);
  //  printf("%s\n",buff);
    fsync(fp);
    close(fp);
}

void config_backgroundvalue_machine1()
{
    char radar_config[4000] = {0};
    for(unsigned int i = 0;i<200;i++)
    {
        for(unsigned char j = 0;j<2;j++)
        {
            sprintf(radar_config,"%s\n%d",radar_config,Master_Back_Groud_Value[0][i][j]);
        }
    }
    QString q_radar_backgroud_config;
    q_radar_backgroud_config = QString(QLatin1String(radar_config));
    QFile file("/opt/radar/backgroundvalue_machine1.txt");
    file.open(QIODevice::WriteOnly |QIODevice::Text |QIODevice::Truncate);
    QTextStream in(&file);
    in<<q_radar_backgroud_config;
    file.close();

    int fp = open("/opt/radar/backgroundvalue_machine1.txt",O_RDONLY);
    fsync(fp);
    close(fp);
}

void config_boundary_machine1_area1()
{
    char radar_znconfig[100] = {0};
    sprintf(radar_znconfig,"%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n",
            Master_Boundary_Point_Disp[0][0][0][1],Master_Boundary_Point_Disp[0][0][0][0],
            Master_Boundary_Point_Disp[0][0][1][1],Master_Boundary_Point_Disp[0][0][1][0],
            Master_Boundary_Point_Disp[0][0][2][1],Master_Boundary_Point_Disp[0][0][2][0],
            Master_Boundary_Point_Disp[0][0][3][1],Master_Boundary_Point_Disp[0][0][3][0],
            Master_Boundary_Point_Disp[0][0][4][1],Master_Boundary_Point_Disp[0][0][4][0],
            Master_Boundary_Point_Disp[0][0][5][1],Master_Boundary_Point_Disp[0][0][5][0]
            );
    QString q_radar_znconfig;
    q_radar_znconfig = QString(QLatin1String(radar_znconfig));
    QFile file("/opt/radar/boundary_machine1_area1.txt");
    file.open(QIODevice::WriteOnly |QIODevice::Text |QIODevice::Truncate);
    QTextStream in(&file);
    in<<q_radar_znconfig;
    file.close();

    int fp = open("/opt/radar/boundary_machine1_area1.txt",O_RDONLY);
    fsync(fp);
    close(fp);
}

void config_boundary_machine1_area2()
{
    char radar_znconfig[100] = {0};
    sprintf(radar_znconfig,"%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n",
            Master_Boundary_Point_Disp[0][1][0][1],Master_Boundary_Point_Disp[0][1][0][0],
            Master_Boundary_Point_Disp[0][1][1][1],Master_Boundary_Point_Disp[0][1][1][0],
            Master_Boundary_Point_Disp[0][1][2][1],Master_Boundary_Point_Disp[0][1][2][0],
            Master_Boundary_Point_Disp[0][1][3][1],Master_Boundary_Point_Disp[0][1][3][0],
            Master_Boundary_Point_Disp[0][1][4][1],Master_Boundary_Point_Disp[0][1][4][0],
            Master_Boundary_Point_Disp[0][1][5][1],Master_Boundary_Point_Disp[0][1][5][0]
            );
    QString q_radar_znconfig;
    q_radar_znconfig = QString(QLatin1String(radar_znconfig));
    QFile file("/opt/radar/boundary_machine1_area2.txt");
    file.open(QIODevice::WriteOnly |QIODevice::Text |QIODevice::Truncate);
    QTextStream in(&file);
    in<<q_radar_znconfig;
    file.close();

    int fp = open("/opt/radar/boundary_machine1_area2.txt",O_RDONLY);
    fsync(fp);
    close(fp);
}

void config_boundary_machine1_area3()
{
    char radar_znconfig[100] = {0};
    sprintf(radar_znconfig,"%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n",
            Master_Boundary_Point_Disp[0][2][0][1],Master_Boundary_Point_Disp[0][2][0][0],
            Master_Boundary_Point_Disp[0][2][1][1],Master_Boundary_Point_Disp[0][2][1][0],
            Master_Boundary_Point_Disp[0][2][2][1],Master_Boundary_Point_Disp[0][2][2][0],
            Master_Boundary_Point_Disp[0][2][3][1],Master_Boundary_Point_Disp[0][2][3][0],
            Master_Boundary_Point_Disp[0][2][4][1],Master_Boundary_Point_Disp[0][2][4][0],
            Master_Boundary_Point_Disp[0][2][5][1],Master_Boundary_Point_Disp[0][2][5][0]
            );
    QString q_radar_znconfig;
    q_radar_znconfig = QString(QLatin1String(radar_znconfig));
    QFile file("/opt/radar/boundary_machine1_area3.txt");
    file.open(QIODevice::WriteOnly |QIODevice::Text |QIODevice::Truncate);
    QTextStream in(&file);
    in<<q_radar_znconfig;
    file.close();

    int fp = open("/opt/radar/boundary_machine1_area3.txt",O_RDONLY);
    fsync(fp);
    close(fp);
}

void config_boundary_machine1_area4()
{
    char radar_znconfig[100] = {0};
    sprintf(radar_znconfig,"%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n",
            Master_Boundary_Point_Disp[0][3][0][1],Master_Boundary_Point_Disp[0][3][0][0],
            Master_Boundary_Point_Disp[0][3][1][1],Master_Boundary_Point_Disp[0][3][1][0],
            Master_Boundary_Point_Disp[0][3][2][1],Master_Boundary_Point_Disp[0][3][2][0],
            Master_Boundary_Point_Disp[0][3][3][1],Master_Boundary_Point_Disp[0][3][3][0],
            Master_Boundary_Point_Disp[0][3][4][1],Master_Boundary_Point_Disp[0][3][4][0],
            Master_Boundary_Point_Disp[0][3][5][1],Master_Boundary_Point_Disp[0][3][5][0]
            );
    QString q_radar_znconfig;
    q_radar_znconfig = QString(QLatin1String(radar_znconfig));
    QFile file("/opt/radar/boundary_machine1_area4.txt");
    file.open(QIODevice::WriteOnly |QIODevice::Text |QIODevice::Truncate);
    QTextStream in(&file);
    in<<q_radar_znconfig;
    file.close();

    int fp = open("/opt/radar/boundary_machine1_area4.txt",O_RDONLY);
    fsync(fp);
    close(fp);
}

void config_boundary_machine1_area5()
{
    char radar_znconfig[100] = {0};
    sprintf(radar_znconfig,"%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n",
            Master_Boundary_Point_Disp[0][4][0][1],Master_Boundary_Point_Disp[0][4][0][0],
            Master_Boundary_Point_Disp[0][4][1][1],Master_Boundary_Point_Disp[0][4][1][0],
            Master_Boundary_Point_Disp[0][4][2][1],Master_Boundary_Point_Disp[0][4][2][0],
            Master_Boundary_Point_Disp[0][4][3][1],Master_Boundary_Point_Disp[0][4][3][0],
            Master_Boundary_Point_Disp[0][4][4][1],Master_Boundary_Point_Disp[0][4][4][0],
            Master_Boundary_Point_Disp[0][4][5][1],Master_Boundary_Point_Disp[0][4][5][0]
            );
    QString q_radar_znconfig;
    q_radar_znconfig = QString(QLatin1String(radar_znconfig));
    QFile file("/opt/radar/boundary_machine1_area5.txt");
    file.open(QIODevice::WriteOnly |QIODevice::Text |QIODevice::Truncate);
    QTextStream in(&file);
    in<<q_radar_znconfig;
    file.close();

    int fp = open("/opt/radar/boundary_machine1_area5.txt",O_RDONLY);
    fsync(fp);
    close(fp);
}

void config_boundary_machine1_area6()
{
    char radar_znconfig[100] = {0};
    sprintf(radar_znconfig,"%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n",
            Master_Boundary_Point_Disp[0][5][0][1],Master_Boundary_Point_Disp[0][5][0][0],
            Master_Boundary_Point_Disp[0][5][1][1],Master_Boundary_Point_Disp[0][5][1][0],
            Master_Boundary_Point_Disp[0][5][2][1],Master_Boundary_Point_Disp[0][5][2][0],
            Master_Boundary_Point_Disp[0][5][3][1],Master_Boundary_Point_Disp[0][5][3][0],
            Master_Boundary_Point_Disp[0][5][4][1],Master_Boundary_Point_Disp[0][5][4][0],
            Master_Boundary_Point_Disp[0][5][5][1],Master_Boundary_Point_Disp[0][5][5][0]
            );
    QString q_radar_znconfig;
    q_radar_znconfig = QString(QLatin1String(radar_znconfig));
    QFile file("/opt/radar/boundary_machine1_area6.txt");
    file.open(QIODevice::WriteOnly |QIODevice::Text |QIODevice::Truncate);
    QTextStream in(&file);
    in<<q_radar_znconfig;
    file.close();

    int fp = open("/opt/radar/boundary_machine1_area6.txt",O_RDONLY);
    fsync(fp);
    close(fp);
}

//*****added for radar*****/<-
//可燃气体
void config_num_fga()
{
    QFile config_fga("/opt/fga/config_fga.txt");
    if(!config_fga.open(QIODevice::WriteOnly |QIODevice::Text |QIODevice::Truncate))
    {
        qDebug() <<"Can't open config_fga file!"<<endl;
    }
    QTextStream in(&config_fga);
    QString q_num_fga(QString("%1").arg(Num_Fga));
    in<<q_num_fga;
    config_fga.close();

    int fp = open("/opt/fga/config_fga.txt",O_RDONLY);
    fsync(fp);
    close(fp);
}
//油气回收
void config_reoilgas_write()    //加油机及油枪数量
{
    char reoilgas_config[50] = {0};
    sprintf(reoilgas_config,"%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n",Amount_Dispener,Amount_Gasgun[0],Amount_Gasgun[1],Amount_Gasgun[2],
            Amount_Gasgun[3],Amount_Gasgun[4],Amount_Gasgun[5],Amount_Gasgun[6],Amount_Gasgun[7],Amount_Gasgun[8],Amount_Gasgun[9],Amount_Gasgun[10],Amount_Gasgun[11]);
    QString q_reoilgas_config;
    q_reoilgas_config = QString(QLatin1String(reoilgas_config));
    QFile file(CONFIG_REOILGAS);
    file.open(QIODevice::WriteOnly |QIODevice::Text |QIODevice::Truncate);
    QTextStream in(&file);
    in<<q_reoilgas_config;
    file.close();

    int fp = open(CONFIG_REOILGAS,O_RDONLY);
    fsync(fp);
    close(fp);
}
void config_reoilgas_gasdetail_write()
{
    QString q_config;
    for(unsigned char i = 0;i < 12;i++)
    {
        for(unsigned char j = 0;j < 8;j++)
        {
            q_config.append(QString("\n%1").arg(Gas_Factor[i][j]));
        }
    }

    QFile file(CONFIG_GAS_FACTOR);
    file.open(QIODevice::WriteOnly |QIODevice::Text |QIODevice::Truncate);
    QTextStream in(&file);
    in<<q_config;
    file.close();

    int fp = open(CONFIG_GAS_FACTOR,O_RDONLY);
    fsync(fp);
    close(fp);
}
void config_reoilgas_oildetail_write()
{
    QString q_config;
    for(unsigned char i = 0;i < 12;i++)
    {
        for(unsigned char j = 0;j < 8;j++)
        {
            q_config.append(QString("\n%1").arg(Fueling_Factor[i][j]));
        }
    }

    QFile file(CONFIG_OIL_FACTOR);
    file.open(QIODevice::WriteOnly |QIODevice::Text |QIODevice::Truncate);
    QTextStream in(&file);
    in<<q_config;
    file.close();

    int fp = open(CONFIG_OIL_FACTOR,O_RDONLY);
    fsync(fp);
    close(fp);
}
void config_info_accum_write()
{
    qDebug()<<"weite warn days!";
    for(unsigned char i = 1;i < 13;i++)
    {
        for(unsigned char j = 1;j < 9;j++)
        {
            QString path = QString("/opt/reoilgas/info_accum/%1-%2").arg(i).arg(j);
            //qDebug()<<path<<endl;
            QFile file(path);
            file.open(QIODevice::WriteOnly|QIODevice::Text|QIODevice::Truncate);
            QTextStream in(&file);
            in<<QString("%1").arg(Flag_Accumto[i-1][j-1]);
            file.close();

            int fp = open(path.toStdString().c_str(),O_RDONLY);
            fsync(fp);
            close(fp);
            //printf("write over %d-%d  %d\n",i,j,Flag_Accumto[i-1][j-1]);
        }
    }
}
//油枪映射表文本写入
void config_mapping_write()
{
    QString q_config;
	QString q_config_show;
	QString q_config_oilno;
    for(unsigned char i = 0;i < 12;i++)
    {
        for(unsigned char j = 0;j < 8;j++)
        {
            q_config.append(QString("%1\n").arg(Mapping[i*8+j]));
			q_config_show.append(Mapping_Show[i*8+j]).append("\n");
			q_config_oilno.append(Mapping_OilNo[i*8+j]).append("\n");
        }
    }

    QFile file(CONFIG_MAPPING);
    file.open(QIODevice::WriteOnly |QIODevice::Text |QIODevice::Truncate);
    QTextStream in(&file);
    in<<q_config;
    file.close();
    int fp = open(CONFIG_MAPPING,O_RDONLY);
    fsync(fp);
    close(fp);

	QFile file_show(CONFIG_MAPPING_SHOW);
	file_show.open(QIODevice::WriteOnly |QIODevice::Text |QIODevice::Truncate);
	QTextStream in_show(&file_show);
	in_show<<q_config_show;
	file_show.close();
	int fp_show = open(CONFIG_MAPPING_SHOW,O_RDONLY);
	fsync(fp_show);
	close(fp_show);

	QFile file_oilno(CONFIG_MAPPING_OILNO);
	file_oilno.open(QIODevice::WriteOnly |QIODevice::Text |QIODevice::Truncate);
	QTextStream in_oilno(&file_oilno);
	in_oilno<<q_config_oilno;
	file_oilno.close();
	int fp_oilno = open(CONFIG_MAPPING_OILNO,O_RDONLY);
	fsync(fp_oilno);
	close(fp_oilno);
}

void config_fga_accum_write(unsigned char type1,unsigned char type3)
{
    QString path("/opt/reoilgas/info_accum/50-50");
    qDebug()<<path<<endl;
    QFile file(path);
    file.open(QIODevice::WriteOnly|QIODevice::Text|QIODevice::Truncate);
    QTextStream in(&file);
    in<<QString("%1").arg(type1);
    file.close();

    int fp = open(path.toStdString().c_str(),O_RDONLY);
    fsync(fp);
    close(fp);

    QString path_3("/opt/reoilgas/info_accum/600-1000");
    qDebug()<<path_3<<endl;
    QFile file_3(path_3);
    file_3.open(QIODevice::WriteOnly|QIODevice::Text|QIODevice::Truncate);
    QTextStream in_3(&file_3);
    in_3<<QString("%1").arg(type3);
    file.close();

    int fp_3 = open(path_3.toStdString().c_str(),O_RDONLY);
    fsync(fp_3);
    close(fp_3);
}

void config_reoilgas_pre_en_write()
{
    QString q_en_sensor = QString("%1\n%2\n%3\n%4\n").arg(Pre_tank_en).arg(Pre_pipe_en).arg(Env_Gas_en).arg(Tem_tank_en);
    QFile file("/opt/reoilgas/config_pre.txt");
    file.open(QIODevice::WriteOnly |QIODevice::Text |QIODevice::Truncate);
    QTextStream in(&file);
    in<<q_en_sensor;
    file.close();

    int fp = open("/opt/reoilgas/config_pre.txt",O_RDONLY);
    fsync(fp);
    close(fp);
}
//在线监测传感器类型初始化
void PreTemGasSensor_Type_init()
{
	QFile config_ver("/opt/reoilgas/config_pre_type.txt");
	if(!config_ver.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qDebug() <<"can not open /opt/reoilgas/config_pre_type.txt"<<endl;
	}
	QTextStream in(&config_ver);
	QString line;
	line = in.readLine();
	Flag_TankPre_Type = line.toInt();
	line = in.readLine();
	Flag_PipePre_Type = line.toInt();
	line = in.readLine();
	Flag_TankTem_Type = line.toInt();
	line = in.readLine();
	Flag_Gas_Type = line.toInt();

	config_ver.close();
	QFileInfo fileInfo("/opt/reoilgas/config_pre_type.txt");
	if(fileInfo.isFile())
	{
	}
	else
	{
		Flag_TankPre_Type = 1;//0有线版本 1无线版本  默认无线版本
		Flag_PipePre_Type = 1;
		Flag_TankTem_Type = 1;
		Flag_Gas_Type = 1;
	}
}
//在线监测传感器类型写入
void PreTemGasSensor_Type_write()
{
	QFile file("/opt/reoilgas/config_pre_type.txt");
	file.open(QIODevice::WriteOnly |QIODevice::Text |QIODevice::Truncate);
	QTextStream in(&file);
	in<<QString::number(Flag_TankPre_Type)+"\r\n";
	in<<QString::number(Flag_PipePre_Type)+"\r\n";
	in<<QString::number(Flag_TankTem_Type)+"\r\n";
	in<<QString::number(Flag_Gas_Type)+"\r\n";
	file.close();

	int fp = open("/opt/reoilgas/config_pre_type.txt",O_RDONLY);
	fsync(fp);
	close(fp);
}

void config_pv_positive_write()
{
    QString q_pv_positive;
    q_pv_positive = QString("%1").arg(Positive_Pres);
    QFile file("/opt/reoilgas/config_pv_positive.txt");
    file.open(QIODevice::WriteOnly |QIODevice::Text |QIODevice::Truncate);
    QTextStream in(&file);
    in<<q_pv_positive;
    file.close();
    int fp = open("/opt/reoilgas/config_pv_positive.txt",O_RDONLY);
    fsync(fp);
    close(fp);
}
void config_pv_negative_write()
{

    QString q_pv_negative;
    q_pv_negative = QString("%1").arg(Negative_Pres);
    QFile file("/opt/reoilgas/config_pv_negative.txt");
    file.open(QIODevice::WriteOnly |QIODevice::Text |QIODevice::Truncate);
    QTextStream in(&file);
    in<<q_pv_negative;
    file.close();
    int fp = open("/opt/reoilgas/config_pv_negative.txt",O_RDONLY);
    fsync(fp);
    close(fp);
}
void config_jingdian_write()
{
    QString q_jingdian;
    q_jingdian = QString("%1").arg(Flag_xieyou);
    QFile file("/opt/jingdian/config_jingdian.txt");
    file.open(QIODevice::WriteOnly |QIODevice::Text |QIODevice::Truncate);
    QTextStream in(&file);
    in<<q_jingdian;
    file.close();
    int fp = open("/opt/jingdian/config_jingdian.txt",O_RDONLY);
    fsync(fp);
    close(fp);
}
void config_IIE_write()
{
    QString q_IIE;
    q_IIE = QString("%1").arg(Flag_IIE);
    QFile file("/opt/jingdian/config_IIE.txt");
    file.open(QIODevice::WriteOnly |QIODevice::Text |QIODevice::Truncate);
    QTextStream in(&file);
    in<<q_IIE;
    file.close();
    int fp = open("/opt/jingdian/config_IIE.txt",O_RDONLY);
    fsync(fp);
    close(fp);
}

//写入液阻报警相关信息
void config_Liquid_resistance()
{
    char amount_LR[10] = {0};
    sprintf(amount_LR,"%d\n%d\n",Far_Dispener,Speed_fargas);
    QString q_amount_LR;
    q_amount_LR = QString(QLatin1String(amount_LR));
    QFile file("/opt/reoilgas/config_Liquid_resistance.txt");
    file.open(QIODevice::WriteOnly |QIODevice::Text |QIODevice::Truncate);
    QTextStream in(&file);
    in<<q_amount_LR;
    file.close();

    int fp = open("/opt/reoilgas/config_Liquid_resistance.txt",O_RDONLY);
    fsync(fp);
    close(fp);
}
//初始化液阻相关
void init_Liquid_resistance()
{
    QFile config_LR("/opt/reoilgas/config_Liquid_resistance.txt");
    if(!config_LR.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() <<"Can't open config_Liquid_resistance.txt!"<<endl;
    }
    QTextStream in(&config_LR);
    QString line;
    for(uchar i = 0; i < 2; i ++)
    {
        line = in.readLine();
        QByteArray read_config_LR = line.toLatin1();
        char *read_data_LR = read_config_LR.data();
        if(i == 0)
        {
            Far_Dispener = atoi(read_data_LR);
            printf("read far dispener num is %d\n",Far_Dispener);
        }
        if(i == 1)
        {
            Speed_fargas = atoi(read_data_LR);
            printf("read speed is %d\n",Speed_fargas);
        }
    }
    config_LR.close();
}
//写入气液比相关设置
void config_alset()
{
    QFile file("/opt/reoilgas/config_alset.txt");
    file.open(QIODevice::WriteOnly |QIODevice::Text |QIODevice::Truncate);
    QTextStream in(&file);
    in<<(QString::number(NormalAL_Low))+"\r\n";
    in<<(QString::number(NormalAL_High))+"\r\n";
    in<<(QString::number(WarnAL_Days))+"\r\n";
    in<<(QString::number(Flag_Gun_off))+"\r\n";//自动关枪使能
    file.close();

    int fp = open("/opt/reoilgas/config_alset.txt",O_RDONLY);
    fsync(fp);
    close(fp);
}
//初始化气液比相关
void init_alset()
{
    QFile config_LR("/opt/reoilgas/config_alset.txt");
    if(!config_LR.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() <<"Can't open config_alset.txt!"<<endl;
    }
    QTextStream in(&config_LR);
    QString line;
	for(int i = 0; i < 4; i ++)
	{
		line = in.readLine();
		QByteArray read_config = line.toLatin1();
		qDebug()<<read_config;
		char *read_data = read_config.data();
		if(i == 0)
		{
			NormalAL_Low = line.toFloat();
			printf("NormalAL_Low is %f\n",NormalAL_Low);
		}
		if(i == 1)
		{
			NormalAL_High = line.toFloat();
			printf("NormalAL_High is %f\n",NormalAL_High);
		}
		if(i == 2)
		{
			WarnAL_Days = atoi(read_data);
			printf("WarnAL_Days is %d\n",WarnAL_Days);
		}
		if(i == 3)
		{
			Flag_Gun_off = atoi(read_data);
			printf("Flag_Gun_off is %d\n",Flag_Gun_off);
		}
	}
    config_LR.close();
    if(NormalAL_Low == 0){NormalAL_Low = 1.0;printf("NormalAL_Low is %f\n",NormalAL_Low);}
    if(NormalAL_High == 0){NormalAL_High = 1.2;printf("NormalAL_High is %f\n",NormalAL_High);}
    if(WarnAL_Days == 0){WarnAL_Days = 5;printf("WarnAL_Days is %d\n",WarnAL_Days);}
}
//将安全防护配置写入文本
void config_security()
{
    QString ch1 = QString::number(Flag_Enable_liqiud);
    QString ch2 = QString::number(Flag_Enable_pump);
    QString ch3 = QString::number(Num_Crash_Column);
    QFile file(CONFIG_SECURITY);
    file.open(QIODevice::WriteOnly |QIODevice::Text |QIODevice::Truncate);
    QTextStream in(&file);
    in<<ch1+"\r\n";
    in<<ch2+"\r\n";
    in<<ch3+"\r\n";

    file.close();

    int fp = open(CONFIG_SECURITY,O_RDONLY);
    fsync(fp);
    close(fp);
}
//将安全post net写入文本  增加ifis配置
void config_post_network()
{
    QFile file(CONFIG_POSTNETWORK);
    file.open(QIODevice::WriteOnly |QIODevice::Text |QIODevice::Truncate);
    QTextStream in(&file);
	in<<Post_Address+"\r\n";
	in<<USERID_POST+"\r\n";
	in<<DATAID_POST+"\r\n";
	in<<VERSION_POST+"\r\n";
	in<<POSTUSERNAME_HUNAN+"\r\n";
	in<<POSTPASSWORD_HUNAN+"\r\n";
    file.close();

    int fp = open(CONFIG_POSTNETWORK,O_RDONLY);
    fsync(fp);
    close(fp);
}
//写入界面显示相关设置
void config_tabwidget()
{
    QFile file("/opt/config_tabwidget.txt");
    file.open(QIODevice::WriteOnly |QIODevice::Text |QIODevice::Truncate);
    QTextStream in(&file);
    in<<QString::number(Flag_screen_xielou)+"\r\n";
    in<<QString::number(Flag_screen_radar)+"\r\n";
    in<<QString::number(Flag_screen_safe)+"\r\n";
    in<<QString::number(Flag_screen_burngas)+"\r\n";
    in<<QString::number(Flag_screen_zaixian)+"\r\n";
    in<<QString::number(Flag_screen_cc)+"\r\n";
    file.close();

    int fp = open("/opt/config_tabwidget.txt",O_RDONLY);
    fsync(fp);
    close(fp);
}
//初始化界面显示相关
void init_tabwidget()
{
    QFile config_LR("/opt/config_tabwidget.txt");
    if(!config_LR.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() <<"Can't open config_tabwidget.txt!"<<endl;
    }
    QTextStream in(&config_LR);
    QString line;
    for(uchar i = 0; i < 6; i ++)
    {
        line = in.readLine();
        QByteArray read_config = line.toLatin1();
        char *read_data = read_config.data();
        if(i == 0)
        {
            Flag_screen_xielou = atoi(read_data);
        }
        if(i == 1)
        {
            Flag_screen_radar = atoi(read_data);
        }
        if(i == 2)
        {
            Flag_screen_safe = atoi(read_data);
        }
        if(i == 3)
        {
            Flag_screen_burngas = atoi(read_data);
        }
        if(i == 4)
        {
            Flag_screen_zaixian = atoi(read_data);
        }
        if(i == 5)
        {
            Flag_screen_cc = atoi(read_data);
        }
    }
    config_LR.close();
    QFileInfo fileInfo("/opt/config_tabwidget.txt");
    if(fileInfo.isFile())
    {
    }
    else
    {
        Flag_screen_xielou = 1;
        Flag_screen_radar = 1;
        Flag_screen_safe = 1;
        Flag_screen_burngas = 1;
        Flag_screen_zaixian = 1;
        Flag_screen_cc= 1;
    }
}
void config_Pressure_Transmitters_Mode_write()//压力变送器模式设置写入
{
    QFile file("/opt/reoilgas/config_Pressure_Transmitters_Mode.txt");
    file.open(QIODevice::WriteOnly |QIODevice::Text |QIODevice::Truncate);
    QTextStream in(&file);
    in<<QString::number(Flag_Pressure_Transmitters_Mode)+"\r\n";
    in<<QString::number(Flag_Reoilgas_Version)+"\r\n";
    file.close();

    int fp = open("/opt/reoilgas/config_Pressure_Transmitters_Mode.txt",O_RDONLY);
    fsync(fp);
    close(fp);
}
void init_Pressure_Transmitters_Mode()//压力变送器模式设置读取
{
    QFile config_LR("/opt/reoilgas/config_Pressure_Transmitters_Mode.txt");
    if(!config_LR.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() <<"can not open /opt/reoilgas/config_Pressure_Transmitters_Mode.txt"<<endl;
    }
    QTextStream in(&config_LR);
    QString line;

    line = in.readLine();
    QByteArray read_config = line.toLatin1();
    char *read_data = read_config.data();
    Flag_Pressure_Transmitters_Mode = atoi(read_data);

    line = in.readLine();
    QByteArray read_version = line.toLatin1();
    char *read_ver = read_version.data();
    Flag_Reoilgas_Version = atoi(read_ver);

    config_LR.close();
    QFileInfo fileInfo("/opt/reoilgas/config_Pressure_Transmitters_Mode.txt");
    if(fileInfo.isFile())
    {
    }
    else
    {
        Flag_Pressure_Transmitters_Mode = 1;
        Flag_Reoilgas_Version = 1;
    }
}
void config_network_Version_write()//油气回收网络上传版本写入
{
    QFile file("/opt/reoilgas/config_network_version.txt");
    file.open(QIODevice::WriteOnly |QIODevice::Text |QIODevice::Truncate);
    QTextStream in(&file);
    in<<QString::number(Flag_Network_Send_Version)+"\r\n";
    in<<QString::number(Flag_Shield_Network)+"\r\n";
    file.close();

    int fp = open("/opt/reoilgas/config_network_version.txt",O_RDONLY);
    fsync(fp);
    close(fp);
}
void init_network_Version()//油气回收网络上传版本读取
{
    QFile config_LR("/opt/reoilgas/config_network_version.txt");
    if(!config_LR.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() <<"can not open /opt/reoilgas/config_network_version.txt"<<endl;
    }
    QTextStream in(&config_LR);
    QString line;
    line = in.readLine();
    QByteArray read_config = line.toLatin1();
    char *read_data = read_config.data();
    Flag_Network_Send_Version = atoi(read_data);

    line = in.readLine();
    QByteArray read_config2 = line.toLatin1();
    char *read_data2 = read_config2.data();
    Flag_Shield_Network = atoi(read_data2);
    config_LR.close();
    QFileInfo fileInfo("/opt/reoilgas/config_network_version.txt");
    if(fileInfo.isFile())
    {
    }
    else
    {
        Flag_Network_Send_Version = 100;
        Flag_Shield_Network = 0;//未屏蔽
    }
}
void config_isoosi_write()//油气回收网络上传相关参数
{
    QFile file("/opt/reoilgas/config_isoosi.txt");
    file.open(QIODevice::WriteOnly |QIODevice::Text |QIODevice::Truncate);
    QTextStream in(&file);
	in<<IsoOis_UrlIp+"\r\n";
	in<<IsoOis_UrlPort+"\r\n";
    in<<IsoOis_MN+"\r\n";
    in<<IsoOis_PW+"\r\n";
	in<<IsoOis_StationId_Cq+"\r\n";
    file.close();

    int fp = open("/opt/reoilgas/config_isoosi.txt",O_RDONLY);
    fsync(fp);
    close(fp);
}
void init_isoosi_set()//油气回收网络上传相关参数
{
    QFile config_LR("/opt/reoilgas/config_isoosi.txt");
    if(!config_LR.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() <<"can not open /opt/reoilgas/config_isoosi.txt"<<endl;
    }
    QTextStream in(&config_LR);
    QString line;
	line = in.readLine();
	IsoOis_UrlIp = line;
	line = in.readLine();
	IsoOis_UrlPort = line;
    line = in.readLine();
    IsoOis_MN = line;
    line = in.readLine();
    IsoOis_PW = line;
	line = in.readLine();
	IsoOis_StationId_Cq = line;
    config_LR.close();
    QFileInfo fileInfo("/opt/reoilgas/config_isoosi.txt");
    if(fileInfo.isFile())
    {
    }
    else
    {
		IsoOis_UrlIp = "";
		IsoOis_UrlPort = "";
        IsoOis_MN = "";
        IsoOis_PW = "";
		IsoOis_StationId_Cq = "";
    }
}
void config_xielou_network()//泄漏网络上传相关参数写入
{
    QFile file("/opt/config_xielou_net.txt");
    file.open(QIODevice::WriteOnly |QIODevice::Text |QIODevice::Truncate);
    QTextStream in(&file);
    in<<QString::number(PORT_UDP)+"\r\n";//ISFS
    in<<QString::number(PORT_TCP)+"\r\n";//ISFS
    in<<QString::number(Flag_XieLou_Version)+"\r\n";
    in<<QString::number(Flag_HuBeitcp_Enable)+"\r\n";
    in<<QString::number(Station_ID_HB[0]*256+Station_ID_HB[1])+"\r\n";
    file.close();

    int fp = open("/opt/config_xielou_net.txt",O_RDONLY);
    fsync(fp);
    close(fp);
}
void init_xielou_network()//泄漏网络上传相关参数读取
{
    QFile config_LR("/opt/config_xielou_net.txt");
    if(!config_LR.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() <<"can not open /opt/config_xielou_net.txt"<<endl;
    }
    QTextStream in(&config_LR);
    QString line;
    line = in.readLine();
    PORT_UDP = line.toInt();
    line = in.readLine();
    PORT_TCP = line.toInt();
    line = in.readLine();
    Flag_XieLou_Version = line.toInt();
    line = in.readLine();
    Flag_HuBeitcp_Enable = line.toInt();
    line = in.readLine();
    Station_ID_HB[0] = line.toInt()/256;
    Station_ID_HB[1] = line.toInt()%256;
    config_LR.close();
    QFileInfo fileInfo("/opt/config_xielou_net.txt");
    if(fileInfo.isFile())
    {
    }
    else
    {
        PORT_UDP = 3486;
        PORT_TCP = 3487;
        Flag_HuBeitcp_Enable = 0;
        Station_ID_HB[0] = 0;
        Station_ID_HB[1] = 0;
    }
}
void config_MyServer_network()//服务器网络上传相关参数写入
{
	QFile file("/opt/config_myserver_net.txt");
	file.open(QIODevice::WriteOnly |QIODevice::Text |QIODevice::Truncate);
	QTextStream in(&file);
	in<<MyStationId+"\r\n";
	in<<MyStationPW+"\r\n";
	in<<MyServerIp+"\r\n";
	in<<QString::number(MyServerPort)+"\r\n";
	in<<QString::number(Flag_MyServerEn)+"\r\n";
	file.close();

	int fp = open("/opt/config_myserver_net.txt",O_RDONLY);
	fsync(fp);
	close(fp);
}
void init_myserver_network()//服务器网络上传相关参数读取
{
	QFile config_LR("/opt/config_myserver_net.txt");
	if(!config_LR.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qDebug() <<"can not open /opt/config_xielou_net.txt"<<endl;
	}
	QTextStream in(&config_LR);
	QString line;
	line = in.readLine();
	MyStationId = line;
	line = in.readLine();
	MyStationPW = line;
	line = in.readLine();
	MyServerIp = line;
	line = in.readLine();
	MyServerPort = line.toInt();
	line = in.readLine();
	Flag_MyServerEn = line.toInt();

	QFileInfo fileInfo("/opt/config_myserver_net.txt");
	if(fileInfo.isFile())
	{
	}
	else
	{
		MyStationId = "123456789";
		MyStationPW = "1234";
		MyServerIp = "118.178.180.140";
		MyServerPort = 8080;
		Flag_MyServerEn = 0;
	}
}
void config_reoilgas_warnpop()//弹窗设置相关设置写入
{
    QFile file("/opt/reoilgas/config_pop.txt");
    file.open(QIODevice::WriteOnly |QIODevice::Text |QIODevice::Truncate);
    QTextStream in(&file);
    //in<<QString::number(Flag_Show_ReoilgasPop)+"\r\n";//
    //in<<QString::number(Flag_Reoilgas_DayShow)+"\r\n";//
    in<<QString::number(Flag_Reoilgas_NeverShow)+"\r\n";
    file.close();

    int fp = open("/opt/reoilgas/config_pop.txt",O_RDONLY);
    fsync(fp);
    close(fp);
}

void init_reoilgas_warnpop()//弹窗设置相关读取
{
    QFile config_LR("/opt/reoilgas/config_pop.txt");
    if(!config_LR.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() <<"can not open /opt/reoilgas/config_pop.txt"<<endl;
    }
    QTextStream in(&config_LR);
    QString line;
    line = in.readLine();
    Flag_Reoilgas_NeverShow = line.toInt();

    config_LR.close();
    QFileInfo fileInfo("/opt/reoilgas/config_pop.txt");
    if(fileInfo.isFile())
    {
    }
    else
    {
        Flag_Reoilgas_NeverShow = 1;//默认弹窗功能关闭
    }
}
//控制器硬件版本初始化
void Controller_Version_init()
{
	QFile config_ver("/opt/controller_version.txt");
	if(!config_ver.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qDebug() <<"can not open /opt/controller_version.txt"<<endl;
	}
	QTextStream in(&config_ver);
	QString line;
	line = in.readLine();
	Flag_Controller_Version = line.toInt();

	config_ver.close();
	QFileInfo fileInfo("/opt/controller_version.txt");
	if(fileInfo.isFile())
	{
	}
	else
	{
		Flag_Controller_Version = 0;//默认是最旧版的控制器
	}
}
//控制器硬件版本写入
void Controller_Version_write()
{
	QFile file("/opt/controller_version.txt");
	file.open(QIODevice::WriteOnly |QIODevice::Text |QIODevice::Truncate);
	QTextStream in(&file);
	in<<QString::number(Flag_Controller_Version)+"\r\n";
	file.close();

	int fp = open("/opt/controller_version.txt",O_RDONLY);
	fsync(fp);
	close(fp);
}
