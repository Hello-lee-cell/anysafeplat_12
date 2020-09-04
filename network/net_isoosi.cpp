/********************************
 * 广州油气回收数据上传
 * Tcpclient方式上传 ASCII码上传
 * ip 210.72.1.33:8021
 * 2019-10-30  马浩
 * ******************************/
#include "net_isoosi.h"
#include <QApplication>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <qsocketnotifier.h>
#include <QSocketNotifier>
#include <semaphore.h>
#include <unistd.h>
#include <qdatetime.h>
#include <QMutex>
#include "config.h"
#include "systemset.h"
#include "database_op.h"

void *tcpclient_read(void*);

unsigned char Flag_FirstClient = 2;//判断是不是第一次连接，做历史记录用

QMutex wait_send_over;
QString IsoOis_QN = "20160801085857223";
//QString IsoOis_MN = "440111301A52TWUF73000001";//"440111301A52TWUF73000001";
QString IsoOis_ST = "ST=61";
//QString IsoOis_PW = "758534";//"758534";
//QString IsoOis_UrlIp = "";  //网络上传ip
//QString IsoOis_UrlPort = ""; //网络上传端口

int nsockfd_tcp;        //tcp套接字
int sockfd;            //tcp套接字
int PORT_TCP_CLIENT = 0;
char Remote_Ip[20] = {0};
QString urlport_pre = "";
QString urlip_pre = "";
//const QByteArray text = IsoOis_UrlIp.toLocal8Bit();
//const char *tcp_sever_ip = text.data();
//const char* tcp_sever_ip = "210.72.1.33";
int on_tcp = 1;
unsigned char Flag_TcpClient_Success = 0;//tcpclient连接成功
char Sdbuf[10240] = {0};
unsigned int Tcp_send_count = 0;
unsigned char Flag_TcpClient_SendOver = 0;
QString send_head = "##";
//QString data_length = 0;
//QString send_type = "0";//明文传输
int num_recv;
char isoosi_revbuf[128] = {0};
unsigned char flag_send_succes = 0;//发送成功置1，记录一次

net_isoosi::net_isoosi(QWidget *parent) :
    QThread(parent)
{

}
void net_isoosi::run()
{
	sleep(5);
	urlport_pre = IsoOis_UrlPort;
	urlip_pre = IsoOis_UrlIp;
    while (is_runnable)
    {
        if(Flag_Network_Send_Version == 1)//只有需要该协议的时候才运行tcpclient
        {
			if(net_state == 0)//如果网线插进去了
			{
				tcp_client();
			}
        }
		//跳出之后判断是断线还是ip地址变了
		if((urlip_pre!=IsoOis_UrlIp)||(urlport_pre!=IsoOis_UrlPort))
		{
			sleep(1);
			urlport_pre = IsoOis_UrlPort;
			urlip_pre = IsoOis_UrlIp;
			qDebug()<<"IP changed! So sleep 1!";
		}
		else
		{
			sleep(25);
		}
    }
}
void net_isoosi::stop()
{
    is_runnable = false;
}
void net_isoosi::tcp_client()
{	
	const QByteArray text = IsoOis_UrlIp.toLocal8Bit();
	const char *tcp_sever_ip = text.data();
	PORT_TCP_CLIENT = IsoOis_UrlPort.toInt();

    struct sockaddr_in sever_remote;
    // Get the Socket file descriptor
    if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1 )
    {
        printf ("GuangZhou TCPClient Failed to obtain Socket Despcritor.\n");
        //add_value_netinfo("TCPClient Failed to obtain Socket Despcritor");
        //return 0;
    }
    else
    {
        printf ("GuangZhou TCPClient Obtain Socket Despcritor sucessfully.\n");
        //add_value_netinfo("TCPClient Obtain Socket Despcritor sucessfully");
        //history_net_write("Tcp_Obtain");
    }
    sever_remote.sin_family = AF_INET;           		// Protocol Family 协议族
    sever_remote.sin_port = htons (PORT_TCP_CLIENT);         		// Port number 端口号
    sever_remote.sin_addr.s_addr  = inet_addr(tcp_sever_ip);  	// 填写远程地址
    memset (sever_remote.sin_zero,0,8);          		// Flush the rest of struct 刷新struct的其余部分

    //端口重用
    if((setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&on_tcp,sizeof(on_tcp)))<0)
    {
        perror("TCPClient setsockopt failed");
        exit(1);
    }

    bool  bDontLinger = FALSE;
    setsockopt(sockfd,SOL_SOCKET,SO_LINGER,(const char*)&bDontLinger,sizeof(bool));//不要因为数据未发送就阻塞关闭操作
    setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY,&on_tcp,sizeof(on_tcp));//立即发送，不沾包
    //int nNetTimeout = 800;//超时时长
	struct timeval timeout = {0,500000}; //设置接收超时 1秒？ 第一个参数秒，第二个微秒
    setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,(char *)&timeout,sizeof(struct timeval));//设置为非阻塞模式
    //主动连接目标,连接不上貌似会卡在这，10秒再连一次
    if (( nsockfd_tcp = ::connect(sockfd,(struct sockaddr *)&sever_remote,sizeof(struct sockaddr))) < 0)
    {
        printf ("GuangZhou TCPClient Failed to Client Port %d.\n", PORT_TCP_CLIENT);
        //return (0);
        Flag_TcpClient_Success = 0;//连接失败
        close(nsockfd_tcp);
        close(sockfd);
		if(Flag_FirstClient != 0)
		{
			add_value_netinfo("GuangZhou TCPClient Client the Port 8201 Failed");
			Flag_FirstClient = 0;
		}

    }
    else
    {
        printf ("GuangZhou TCPClient Client the Port %d sucessfully.\n", PORT_TCP_CLIENT);  
        Flag_TcpClient_Success = 1;//连接成功
        client_keep_ali(sockfd);//tcp保活

		if(Flag_FirstClient != 1)
		{
			add_value_netinfo("GuangZhou TCPClient Client the Port 8201 sucessfully");
			Flag_FirstClient = 1;
		}
    }
    //新线程，tcp_read
//    pthread_t id_tcpread;
//    pthread_attr_t attr;
//    pthread_attr_setdetachstate(&attr,1);//设置线程为分离属性，结束后立即释放资源
//    long t_tcp = 0;
//    int ret_tcp = pthread_create(&id_tcpread, &attr,tcpclient_read, (void*)t_tcp);
//    if(ret_tcp != 0)
//    {
//        printf("Can not create tcp read thread!");
//        //continue;
//    }
    sleep(2);
    while(1)
    {
        if(Flag_TcpClient_Success == 0)
        {
            printf ("wait TCPClient  Client \n");
			close(nsockfd_tcp);
			close(sockfd);
            sleep(5);
            break;
        }
        else
        {
			if((urlip_pre!=IsoOis_UrlIp)||(urlport_pre!=IsoOis_UrlPort))
			{
				qDebug()<<"IP changed! So break!";
				break;
			}
			//qDebug()<<"ready to send!";
        }
        //如果不要该协议则退出
        if(Flag_Network_Send_Version != 1)
        {
			close(nsockfd_tcp);
			close(sockfd);
            break;
        }
        sleep(1);
    }
	//qDebug()<<"chagged xieyi !!!@@@@######";
    close(nsockfd_tcp);
    close(sockfd);
    sleep(1);
}
//  tcp长连接保活
void net_isoosi::client_keep_ali(int sockfd)
{
    int keepAlive = 1;      // 开启keepalive属性
    int keepIdle = 10;      // 如该连接在60秒内没有任何数据往来,则进行探测
    int keepInterval = 10;   // 探测时发包的时间间隔为5 秒
    int keepCount = 3;      // 探测尝试的次数.如果第1次探测包就收到响应了,则后2次
    setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepAlive, sizeof(keepAlive));
    setsockopt(sockfd, SOL_TCP, TCP_KEEPIDLE, (void*)&keepIdle, sizeof(keepIdle)); //对应tcp_keepalive_time
    setsockopt(sockfd, SOL_TCP, TCP_KEEPINTVL, (void *)&keepInterval, sizeof(keepInterval)); //对应tcp_keepalive_intvl
    setsockopt(sockfd, SOL_TCP, TCP_KEEPCNT, (void *)&keepCount, sizeof(keepCount)); //对应tcp_keepalive_probes
}
//要发送的数据
void net_isoosi::send_tcpclient_data(QString data)
{
    unsigned int if_send = 0;
    unsigned int sendfail_num = 0;
    if(Flag_TcpClient_Success == 1)
    {
		wait_send_over.lock();//上锁
        //记得加锁
        QByteArray byte_send = data.toUtf8();
        Tcp_send_count = data.length();
		memset(Sdbuf,0,sizeof(char)*10240);//清零数组
        for(unsigned int i = 0;i < Tcp_send_count;i++)
        {
            Sdbuf[i] = byte_send[i];
        }
        if(Tcp_send_count != 0)
        {
            while(!if_send)
            {
                int num_send = 0;
                if((num_send = ::send(sockfd,Sdbuf, Tcp_send_count, 0)) == -1)
                {
                    printf("ERROR: Failed to sent string.\n");
                    if(flag_send_succes == 1)
                    {
                        add_value_netinfo("TCP 数据发送失败");
						Flag_Ifsend = 0;//在这里把全局变量置0，之后时间到了要再重新发送零点的信息
                    }
					//sendfail_num++;//发送失败再发一次
					if_send = 1; //发送失败 端口有问题  直接退出
					sendfail_num = 0;
					Tcp_send_count = 0;
					Flag_TcpClient_Success = 0;//连接失败 //重新连接
                    flag_send_succes = 0;
                }
                else
                {
                    msleep(50);
                    memset(isoosi_revbuf,0,sizeof(char)*128);//清零数组
					if((num_recv = recv(sockfd,isoosi_revbuf,40,MSG_WAITALL)) <= 0)//MSG_DONTWAIT 1秒延时或者收到40个字节
                    {
                        //发送失败
                        qDebug()<<"guangzhou send faile !!";
                        sendfail_num++;//发送失败再发一次
                    }
                    else
					{
						if(QString(isoosi_revbuf).contains("QnRtn=1",Qt::CaseSensitive))//成功返回true 第二个参数表示是否大小写敏感
						{
							//真正的发送成功
							//printf("TcpClient send success %d.\n",Tcp_send_count);
							qDebug()<<"GuangZhou Tcp send success"<<Tcp_send_count<<" Receive "<<num_recv;
							qDebug()<<isoosi_revbuf;
							if(flag_send_succes == 0)
							{
								add_value_netinfo("TCP 数据发送成功");
							}
							flag_send_succes =1;
							if_send = 1;
							Tcp_send_count = 0;
						}
						else
						{
							qDebug()<<"guangzhou send faile !!";
							sendfail_num++;//发送失败再发一次
						}
                    }
                }
                Flag_TcpClient_SendOver = 0;
				if(sendfail_num>=3)
                {
                    if_send = 1;
                    sendfail_num = 0;
                    Tcp_send_count = 0;
					//Flag_TcpClient_Success = 0;//连接失败 //没有收到返回。不重新连接
                }
            }
        }
        else
        {
            qDebug()<<"isoosi semd data is kong!!";
        }
		wait_send_over.unlock();//解锁
    }
    else
    {
		qDebug()<< "tcpclient is not connected!!";
    }
}
/*****************准备发送油枪数据****************
gun_num    油枪编号
AlvR       油枪气液比
GasCur     油气流速
GasFlow    油气流量
FuelCur    燃油流速
FuelFlow   燃油流量
DynbPrs  液阻
信号来自reoilgasthread
**********************************************/
void net_isoosi::refueling_gun_data(QString gun_num,QString AlvR,QString GasCur,QString GasFlow,QString FuelCur,QString FuelFlow,QString DynbPrs)
{
    QDateTime date_time = QDateTime::currentDateTime();
    QString date = date_time.toString("yyyyMMddhhmmss");
    QString send_data;
    IsoOis_QN = date;
    IsoOis_QN.append("000");

    QString send_AlvR = "-AlvR=";       //油枪气液比
    QString send_GasCur = "-GasCur=";     //油气流速
    QString send_GasFlow = "-GasFlow=";    //油气流量
    QString send_FuelCur = "-FuelCur=";    //燃油流速
    QString send_FuelFlow = "-FuelFlow=";   //燃油流量
    QString send_DynbPrs = "-DynbPrs=";   //燃油流量
    (send_AlvR.append(AlvR)).prepend(gun_num);
    (send_GasCur.append(GasCur)).prepend(gun_num);
    (send_GasFlow.append(GasFlow)).prepend(gun_num);
    (send_FuelCur.append(FuelCur)).prepend(gun_num);
    (send_FuelFlow.append(FuelFlow)).prepend(gun_num);
    (send_DynbPrs.append(DynbPrs)).prepend(gun_num);

    send_data = "QN="+IsoOis_QN+";"+"MN="+IsoOis_MN+";"+IsoOis_ST+";"+"CN=2011;"+"PW="+IsoOis_PW+";"+"CP=&&DataTime="+date+";"
                 +send_AlvR+";"+send_GasCur+";"+send_GasFlow+";"+send_FuelCur+";"+send_FuelFlow+";"+send_DynbPrs+"&&";

    unsigned int long_data = send_data.length();
    QString send_dada_length = QString("%1").arg(long_data,4,10,QLatin1Char('0'));

    QByteArray byte = send_data.toUtf8();
    unsigned char *p = (unsigned char *)byte.data();
    unsigned int crc = CRC16_Checkout(p,byte.length());
    //qDebug()<< byte;
    QString key = QString("%1").arg(crc,4,16,QLatin1Char('0'));//k为int型或char型都可
	key = key.toUpper();//变为大写
    send_data.append(key).append("\r\n");
    //qDebug() << key;
    send_data.prepend("0").prepend(send_dada_length).prepend(send_head);
    qDebug()<<send_data;
    send_tcpclient_data(send_data);


}

/*****************准备发送环境数据****************
dynbPress     液阻压力 pa
tankPress     油罐压力 pa
unloadgasCon  卸油区油气浓度 卸油区油气浓度的监测数据，单位%/ppm
DevicegasCon  处理装置油气浓度 处理装置排放浓度的监测数据，单位g/m³
QString GasTemp 油气温度
QString GasVolume 油气空间
信号来自fga——1000
**********************************************/
void net_isoosi::environmental_data(QString dynbPress,QString tankPress,QString unloadgasCon,QString DevicegasCon,QString GasTemp,QString GasVolume)
{
    QDateTime date_time = QDateTime::currentDateTime();
    QString date = date_time.toString("yyyyMMddhhmmss");
    QString send_data;
    IsoOis_QN = date;
    IsoOis_QN.append("000");

    QString send_dynbPress = "DynbPress=";       //液阻压力 pa
    QString send_tankPress = "TankPress=";     //油罐压力 pa
    QString send_unloadgasCon = "UnloadgasCon=";    //卸油区油气浓度 卸油区油气浓度的监测数据，单位%/ppm
    QString send_DevicegasCon = "DevicegasCon=";    //处理装置油气浓度 处理装置排放浓度的监测数据，单位g/m³
    QString send_GasTemp = "GasTemp=";    //处理装置油气浓度 处理装置排放浓度的监测数据，单位g/m³
    QString send_GasVolume = "GasVolume=";    //处理装置油气浓度 处理装置排放浓度的监测数据，单位g/m³

    send_dynbPress.append(dynbPress);
    send_tankPress.append(tankPress);
    send_unloadgasCon.append(unloadgasCon);
    send_DevicegasCon.append(DevicegasCon);
    send_GasTemp.append(GasTemp);
    send_GasVolume.append(GasVolume);

//    send_data = "QN="+IsoOis_QN+";"+"MN="+IsoOis_MN+";"+IsoOis_ST+";"+"CN=2021;"+"PW="+IsoOis_PW+";"+"CP=&&DataTime="+date+";"
//                 +send_dynbPress+";"+send_tankPress+";"+send_unloadgasCon+";"+send_DevicegasCon+";"+send_GasTemp+";"+send_GasVolume+"&&";
    //没有温度什么的
    send_data = "QN="+IsoOis_QN+";"+"MN="+IsoOis_MN+";"+IsoOis_ST+";"+"CN=2021;"+"PW="+IsoOis_PW+";"+"CP=&&DataTime="+date+";"
                 +send_dynbPress+";"+send_tankPress+";"+send_unloadgasCon+";"+send_DevicegasCon+"&&";

    unsigned int long_data = send_data.length();
    QString send_dada_length = QString("%1").arg(long_data,4,10,QLatin1Char('0'));

    QByteArray byte = send_data.toUtf8();
    unsigned char *p = (unsigned char *)byte.data();
    unsigned int crc = CRC16_Checkout(p,byte.length());
    //qDebug()<< byte;
    QString key = QString("%1").arg(crc,4,16,QLatin1Char('0'));//k为int型或char型都可
	key = key.toUpper();//变为大写
    send_data.append(key).append("\r\n");
    //qDebug() << key;
    send_data.prepend("0").prepend(send_dada_length).prepend(send_head);
    qDebug()<<send_data;
    send_tcpclient_data(send_data);

}

/*****************准备发送设置数据****************
PVFrwPrs    PV阀正向开启压力
PVRevPrs    PV阀负向开启压力
TrOpenPrs   三次油气回收开启压力
TrStopPrs   三次停止压力
信号来自systemset
**********************************************/
void net_isoosi::setup_data(QString PVFrwPrs,QString PVRevPrs,QString TrOpenPrs,QString TrStopPrs)
{
    QDateTime date_time = QDateTime::currentDateTime();
    QString date = date_time.toString("yyyyMMddhhmmss");
    QString send_data;
//    QString Gun_Num = QString::number(Amount_Gasgun[0]+Amount_Gasgun[1]+Amount_Gasgun[2]+Amount_Gasgun[3]+Amount_Gasgun[4]+Amount_Gasgun[5]+
//            Amount_Gasgun[6]+Amount_Gasgun[7]+Amount_Gasgun[8]+Amount_Gasgun[9]+Amount_Gasgun[10]+Amount_Gasgun[11]);
    IsoOis_QN = date;
    IsoOis_QN.append("000");

    QString send_PVFrwPrs = "PVFrwPrs=";       //PV阀正向开启压力
    QString send_PVRevPrs = "PVRevPrs=";     //PV阀负向开启压力 pa
    QString send_TrOpenPrs = "TrOpenPrs=";    //三次油气回收开启压力
    QString send_TrStopPrs = "TrStopPrs=";    //三次停止压力
    send_PVFrwPrs.append(PVFrwPrs);
    send_PVRevPrs.append(PVRevPrs);
    send_TrOpenPrs.append(TrOpenPrs);
    send_TrStopPrs.append(TrStopPrs);

  send_data = "QN="+IsoOis_QN+";"+"MN="+IsoOis_MN+";"+IsoOis_ST+";"+"CN=2031;"+"PW="+IsoOis_PW+";"+"CP=&&DataTime="+date+";"
                 +send_PVFrwPrs+";"+send_PVRevPrs+";"+send_TrOpenPrs+";"+send_TrStopPrs+"&&";

//    send_data = "QN="+IsoOis_QN+";"+"MN="+IsoOis_MN+";"+IsoOis_ST+";"+"CN=2031;"+"PW="+IsoOis_PW+";"+"CP=&&DataTime="+date+";"
//                 +"m01-GunNum="+Gun_Num+";"+"m01-"+send_PVFrwPrs+";"+"m01-"+send_PVRevPrs+";"+"m01-"+send_TrOpenPrs+";"+"m01-"+send_TrStopPrs+"&&";

    unsigned int long_data = send_data.length();
    QString send_dada_length = QString("%1").arg(long_data,4,10,QLatin1Char('0'));

    QByteArray byte = send_data.toUtf8();
    unsigned char *p = (unsigned char *)byte.data();
    unsigned int crc = CRC16_Checkout(p,byte.length());
    //qDebug()<< byte;
    QString key = QString("%1").arg(crc,4,16,QLatin1Char('0'));//k为int型或char型都可
	key = key.toUpper();//变为大写
    send_data.append(key).append("\r\n");
    //qDebug() << key;
    send_data.prepend("0").prepend(send_dada_length).prepend(send_head);
    qDebug()<<send_data;
    send_tcpclient_data(send_data);
}

/*****************准备发送报警数据****************
gun_data     油枪状态 1把或所有枪 ，在数据传入前需要预处理
TightAlm     密闭性状态
DynbPAlm     液阻预警状态
TankPAlm     油罐压力预警状态
DeviceAlm    处理装置浓度预警状态
PVTapAlm     PV阀状态
DevOpenAlm   处理装置启动状态
DevStopAlm   处理装置关闭状态
信号来自fga——1000 包括单次和整点的报警
**********************************************/
void net_isoosi::gun_warn_data(QString gun_data,QString TightAlm,QString DynbPAlm,QString TankPAlm,QString DeviceAlm,QString PVTapAlm,QString DevOpenAlm,QString DevStopAlm)
{
    QDateTime date_time = QDateTime::currentDateTime();
    QString date = date_time.toString("yyyyMMddhhmmss");
    QString send_data;
    IsoOis_QN = date;
    IsoOis_QN.append("000");

    QString send_gun_data = gun_data;       //油枪状态 1把或所有枪 ，在数据传入前需要预处理
    QString send_TightAlm = "TightAlm=";     //密闭性状态
    QString send_DynbPAlm  = "DynbPAlm=";    //液阻预警状态
    QString send_TankPAlm = "TankPAlm=";    //油罐压力预警状态
    QString send_DeviceAlm = "DeviceAlm=";   //处理装置浓度预警状态
    QString send_PVTapAlm  = "PVTapAlm=";    //PV阀状态
    QString send_DevOpenAlm = "DevOpenAlm=";    //处理装置启动状态
    QString send_DevStopAlm = "DevStopAlm=";   //处理装置关闭状态
    send_TightAlm.append(TightAlm) ;     //密闭性状态
    send_DynbPAlm .append(DynbPAlm);    //液阻预警状态
    send_TankPAlm.append(TankPAlm) ;    //油罐压力预警状态
    send_DeviceAlm.append(DeviceAlm) ;   //处理装置浓度预警状态
    send_PVTapAlm .append(PVTapAlm) ;    //PV阀状态
    send_DevOpenAlm.append(DevOpenAlm) ;    //处理装置启动状态
    send_DevStopAlm .append(DevStopAlm);   //处理装置关闭状态

    send_data = "QN="+IsoOis_QN+";"+"MN="+IsoOis_MN+";"+IsoOis_ST+";"+"CN=2041;"+"PW="+IsoOis_PW+";"+"CP=&&DataTime="+date+";"
                 +send_gun_data+";"+send_TightAlm+";"+send_DynbPAlm+";"+send_TankPAlm+";"+send_DeviceAlm+";"+send_PVTapAlm+";"
                 +send_DevOpenAlm+";"+send_DevStopAlm+"&&";

	if(Flag_Shield_Network == 1)//屏蔽状态
	{
		send_data = send_data.replace(QRegExp("\\=1;"),"=0;");
		send_data = send_data.replace(QRegExp("\\=2;"),"=0;"); //报警预警全部替换为正常
		send_data = send_data.replace(QRegExp("\\=1&&"),"=0&&");
		send_data = send_data.replace(QRegExp("\\=2&&"),"=0&&"); //报警预警全部替换为正常
	}

    unsigned int long_data = send_data.length();
    QString send_dada_length = QString("%1").arg(long_data,4,10,QLatin1Char('0'));

    QByteArray byte = send_data.toUtf8();
    unsigned char *p = (unsigned char *)byte.data();
    unsigned int crc = CRC16_Checkout(p,byte.length());
    //qDebug()<< byte;
    QString key = QString("%1").arg(crc,4,16,QLatin1Char('0'));//k为int型或char型都可
	key = key.toUpper();//变为大写
    send_data.append(key).append("\r\n");
    //qDebug() << key;
    send_data.prepend("0").prepend(send_dada_length).prepend(send_head);
    qDebug()<<send_data;
    send_tcpclient_data(send_data);


}

/*****************准备发送油枪关停启用数据****************
gun_num  油枪编号
operate  操作类型 0 关 1 启用
Event    事件类型 0 自动 2手动 N未知
//未设置
**********************************************/
void net_isoosi::refueling_gun_stop(QString gun_num,QString operate,QString Event)
{
    QDateTime date_time = QDateTime::currentDateTime();
    QString date = date_time.toString("yyyyMMddhhmmss");
    QString send_data;
    IsoOis_QN = date;
    IsoOis_QN.append("000");
    QString gun_head;
    QString gun_head_num;

    if(gun_num == "N")
    {
        for(unsigned int i = 0;i < Amount_Dispener;i++)
        {
            for(unsigned int j = 0;j < Amount_Gasgun[i];j++)
            {
                gun_head_num = QString("%1").arg(Mapping[(i*8+j)],4,10,QLatin1Char('0'));//k为int型或char型都可
                gun_head.append("q").append(gun_head_num).append("-OpDate=").append(date).append(";");
                gun_head.append("q").append(gun_head_num).append("-Operate=").append("1").append(";");
                gun_head.append("q").append(gun_head_num).append("-Event=").append("0").append(";");
            }
        }
        gun_head = gun_head.left(gun_head.length()-1);
        send_data = "QN="+IsoOis_QN+";"+"MN="+IsoOis_MN+";"+IsoOis_ST+";"+"CN=2051;"+"PW="+IsoOis_PW+";"+"CP=&&"
                     +gun_head+"&&";
    }
    else
    {
        QString send_operate = "-Operate=";     //操作类型 0 关 1 启用
        QString send_Event = "-Event=";    //事件类型 0 自动 2手动 N未知
        (send_operate.append(operate)).prepend(gun_num);
        (send_Event.append(Event)).prepend(gun_num);

        send_data = "QN="+IsoOis_QN+";"+"MN="+IsoOis_MN+";"+IsoOis_ST+";"+"CN=2051;"+"PW="+IsoOis_PW+";"+"CP=&&DataTime="+date+";"
                     +send_Event+";"+send_operate+"&&";
    }

    unsigned int long_data = send_data.length();
    QString send_dada_length = QString("%1").arg(long_data,4,10,QLatin1Char('0'));

    QByteArray byte = send_data.toUtf8();
    unsigned char *p = (unsigned char *)byte.data();
    unsigned int crc = CRC16_Checkout(p,byte.length());
    //qDebug()<< byte;
    QString key = QString("%1").arg(crc,4,16,QLatin1Char('0'));//k为int型或char型都可
	key = key.toUpper();//变为大写
    send_data.append(key).append("\r\n");
    //qDebug() << key;
    send_data.prepend("0").prepend(send_dada_length).prepend(send_head);
    qDebug()<<send_data;
    send_tcpclient_data(send_data);

}

/*****************准备发送油枪状态数据****************
gun_data  油枪状态，所有数据，需要预处理

**********************************************/
void net_isoosi::refueling_gun_sta(QString gun_data)
{
    QDateTime date_time = QDateTime::currentDateTime();
    QString date = date_time.toString("yyyyMMddhhmmss");
    QString send_data;
    IsoOis_QN = date;
    IsoOis_QN.append("000");

    send_data = "QN="+IsoOis_QN+";"+"MN="+IsoOis_MN+";"+IsoOis_ST+";"+"CN=2061;"+"PW="+IsoOis_PW+";"+"CP=&&DataTime="+date+";"
                 +gun_data+"&&";

    unsigned int long_data = send_data.length();
    QString send_dada_length = QString("%1").arg(long_data,4,10,QLatin1Char('0'));

    QByteArray byte = send_data.toUtf8();
    unsigned char *p = (unsigned char *)byte.data();
    unsigned int crc = CRC16_Checkout(p,byte.length());
    //qDebug()<< byte;
    QString key = QString("%1").arg(crc,4,16,QLatin1Char('0'));//k为int型或char型都可
	key = key.toUpper();//变为大写
    send_data.append(key).append("\r\n");
    //qDebug() << key;
    send_data.prepend("0").prepend(send_dada_length).prepend(send_head);
    qDebug()<<send_data;
    send_tcpclient_data(send_data);

}

/******************************************************************************
函数：CRC16_Checkout
描述：CRC16循环冗余校验算法。
参数一：*puchMsg，需要校验的字符串指针
参数二：usDataLen，要校验的字符串长度
返回值：返回CRC16校验码
******************************************************************************/
unsigned int CRC16_Checkout ( unsigned char *puchMsg, unsigned int usDataLen )
{
    unsigned int i,j,crc_reg,check;
    crc_reg = 0xFFFF;
    for(i=0;i<usDataLen;i++)
    {
        //qDebug()<< puchMsg[i];
        crc_reg = (crc_reg>>8) ^ puchMsg[i];
        for(j=0;j<8;j++)
        {
            check = crc_reg & 0x0001;
            crc_reg >>= 1;
            if(check==0x0001)
            {
                crc_reg ^= 0xA001;
            }
        }
    }
    return crc_reg;
}


void *tcpclient_read(void*)
{
    while(1)
    {
        usleep(100000);
        if((num_recv = recv(sockfd,isoosi_revbuf,sizeof(isoosi_revbuf),0)) <= 0)
        {
            qDebug()<<"receive none!!";
        }
        else
        {
            qDebug() << "receive is "<<isoosi_revbuf;
        }
    }

}
