/********************************
 *
 * 重庆油气回收数据上传
 * Tcpclient方式上传 ASCII码上传
 * 2019-11-28  马浩
 * ******************************/

#define	I_START_TIME		        "i41001-Info="		//	启用时间
#define	I_GUN_NUM		            "i41002-Info="		//	加油枪数量
#define	I_PV_FORWARD		        "i41003-Info="		//	PV阀正向压力值
#define	I_PV_NEGATIVE		        "i41004-Info="		//	PV阀负向压力值
#define	I_OPEN_PRESSURE		        "i41005-Info="		//	后处理装置开启压力值（无后处理装置统一填0）
#define	I_CLOSE_PRESSURE		    "i41006-Info="		//	后处理装置停止压力值（无后处理装置统一填0）
#define	I_FLUIDRESISTANCE_NUM		"i41007-Info="		//	安装液阻传感器加油机编号
#define	I_AL_ALARM		            "i41008-Info="		//	A/L报警	0表示正常 1表示预警，2表示报警 ，N表示未加油
#define	I_AIRTIGHT_ALARM		    "i41009-Info="		//	密闭性报警	0表示正常，1表示报警
#define	I_LIQUIDRESISTANCE_ALARM	"i41010-Info="		//	液阻报警	0表示正常，1表示报警
#define	I_TANK_PRESSURE_ALARM		"i41011-Info="		//	油罐压力报警	0表示正常，1表示报警
#define	I_TANK_ZERO_ALARM		    "i41012-Info="		//	油罐零压报警	0表示正常，1表示报警
#define	I_PRESSURE_STATUS		    "i41013-Info="		//	压力/真空阀状态
#define	I_CRITICAL_PRESSURE_ALARM	"i41014-Info="		//	压力/真空阀临界压力状态
#define	I_POSTPROCESSING		    "i41015-Info="		//	后处理装置状态
#define	I_AFTERTREATMENT_DEVICE		"i41016-Info="		//	后处理装置排放浓度
#define	I_AIRPIPE_STATE		        "i41017-Info="		//	卸油回气管状态
#define	I_GUN_ID_DATA		        "i41019-Info="		//	加油枪标识
#define	I_AL		                "i41020-Info="		//	气液比
#define	I_GAS_SPEED		            "i41021-Info="		//	油气流速
#define	I_GASFLOW		            "i41022-Info="		//	油气流量
#define	I_OIL_SPEED		            "i41023-Info="		//	燃油流速
#define	I_OIL_FLOW		            "i41024-Info="		//	燃油流量
#define	I_GAS_CONCENTRATION		    "i41025-Info="		//	回收油气浓度，没有填0
#define	I_GAS_TEMPERATURE		    "i41026-Info="		//	回收油气温度，没有填0
#define	I_FLUID_RESISTANCE		    "i41027-Info="		//	液阻
#define	I_TANK_PRESSURE		        "i41028-Info="		//	油罐压力,单位Pa	要求一个上传周期，上传所有数据，没有的参数填0
#define	I_RESISTANCE_PRESSURE		"i41029-Info="		//	液阻压力,单位Pa
#define	I_OILGAS_CONCENTRATION		"i41030-Info="		//	卸油区油气浓度,单位%/ppm,,没有填0
#define	I_AFTER_DEVICE		        "i41031-Info="		//	后处理装置排放浓度,单位g/m3,没有填0
#define	I_TANKORPIPE_TEM		    "i41032-Info="		//	油气温度,单位℃,没有填0
#define	I_WRONG_DATE		        "i41033-Info="		//	故障数据产生时间	一次只能上传一个故障码
#define	I_WRONG_NUM		            "i41034-Info="		//	故障码
#define	I_GUNSTART_TIME		        "i41035-Info="		//	启用/关停时间
#define	I_GUN_ID_START		        "i41019-Info="		//	加油枪标识
#define	I_OPERATION_TYPE		    "i41038-Info="		//	操作类型(启用/关停)	0-关停 1-启用
#define	I_START_TYPE		        "i41039-Info="		//	关停或启用事件类型(自动/手动)	关停事件类型：0 自动关停 1 手动关停 启用事件类型：0（预留） 1 手动启用 未知事件类型用 N 表示

#include "net_isoosi_cq.h"
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

unsigned char Flag_FirstClient_cq = 3;//判断是不是第一次连接，做历史记录用
QMutex wait_send_over_cq;
int nsockfd_tcp_cq;        //tcp套接字
int sockfd_cq;            //tcp套接字
QString urlport_pre_cq = "";
QString urlip_pre_cq = "";
int PORT_TCP_CLIENT_CQ = 0;     // The port which is communicate with server     可设置
//const char* tcp_sever_ip_cq = "139.9.158.84";
int on_tcp_cq = 1;
unsigned char Flag_TcpClient_Success_Cq = 0;//tcpclient连接成功
char Sdbuf_Cq[10240] = {0};
unsigned int Tcp_send_count_cq = 0;
unsigned char Flag_TcpClient_SendOver_Cq = 0;
QString send_head_cq = "##";
//QString send_type = "0";//明文传输
QString IsoOis_QN_CQ = "20160801085857223";
QString IsoOis_MN_CQ =IsoOis_MN; ////"440111301A52TWUF73000001";    //全局与广州协议公用
QString IsoOis_ST_Cq = "ST=31";
QString IsoOis_PW_CQ = IsoOis_PW;//"758534";                         //全局与广州协议公用
//QString IsoOis_StationId_Cq = "5001130001";//加油站ID                               全局变量
QString IsoOsi_CN_CQ = "CN=3020";
int num_recv_cq;
char isoosi_revbuf_cq[128] = {0};
unsigned char flag_send_succes_cq = 0;//发送成功置1，记录一次

net_isoosi_cq::net_isoosi_cq(QWidget *parent) :
    QThread(parent)
{

}
void net_isoosi_cq::run()
{
	sleep(5);
	urlport_pre_cq = IsoOis_UrlPort;
	urlip_pre_cq = IsoOis_UrlIp;
    while (is_runnable)
    {
        if(Flag_Network_Send_Version == 2)//只有需要该协议的时候才运行tcpclient
        {
			if(net_state == 0)//如果网线插进去了
			{
				tcp_client();
			}
        }
		//跳出之后判断是断线还是ip地址变了
		if((urlip_pre_cq!=IsoOis_UrlIp)||(urlport_pre_cq!=IsoOis_UrlPort))
		{
			sleep(1);
			urlport_pre_cq = IsoOis_UrlPort;
			urlip_pre_cq = IsoOis_UrlIp;
			qDebug()<<"IP changed! So sleep 1!";
		}
		else
		{
			sleep(25);
		}
    }
}
void net_isoosi_cq::stop()
{
    is_runnable = false;
}
void net_isoosi_cq::tcp_client()
{
	const QByteArray text = IsoOis_UrlIp.toLocal8Bit();
	const char *tcp_sever_ip_cq = text.data();
	PORT_TCP_CLIENT_CQ = IsoOis_UrlPort.toInt();

    struct sockaddr_in sever_remote;
    // Get the Socket file descriptor
    if( (sockfd_cq = socket(AF_INET, SOCK_STREAM, 0)) == -1 )
    {
		printf ("ChongQing TCPClient Failed to obtain Socket Despcritor.\n");
    }
    else
    {
		printf ("ChongQing TCPClient Obtain Socket Despcritor sucessfully.\n");
    }
    sever_remote.sin_family = AF_INET;           		// Protocol Family 协议族
    sever_remote.sin_port = htons (PORT_TCP_CLIENT_CQ);         		// Port number 端口号
    sever_remote.sin_addr.s_addr  = inet_addr(tcp_sever_ip_cq);  	// 填写远程地址
    memset (sever_remote.sin_zero,0,8);          		// Flush the rest of struct 刷新struct的其余部分

    //端口重用
    if((setsockopt(sockfd_cq,SOL_SOCKET,SO_REUSEADDR,&on_tcp_cq,sizeof(on_tcp_cq)))<0)
    {
        perror("TCPClient setsockopt failed");
        exit(1);
    }

    bool  bDontLinger = FALSE;
    setsockopt(sockfd_cq,SOL_SOCKET,SO_LINGER,(const char*)&bDontLinger,sizeof(bool));//不要因为数据未发送就阻塞关闭操作
    setsockopt(sockfd_cq, IPPROTO_TCP, TCP_NODELAY,&on_tcp_cq,sizeof(on_tcp_cq));//立即发送，不沾包
	struct timeval timeout = {0,500000}; //设置接收超时 1秒？第一个参数秒，第二个微秒
    setsockopt(sockfd_cq,SOL_SOCKET,SO_RCVTIMEO,(char *)&timeout,sizeof(struct timeval));//设置为非阻塞模式
    //主动连接目标,连接不上貌似会卡在这，10秒再连一次
    if (( nsockfd_tcp_cq = ::connect(sockfd_cq,(struct sockaddr *)&sever_remote,sizeof(struct sockaddr))) < 0)
    {
		qDebug()<<"ChongQing TCPClient Failed to Client Port"<<IsoOis_UrlIp<<PORT_TCP_CLIENT_CQ;
        //return (0);
        Flag_TcpClient_Success_Cq = 0;//连接失败
        close(nsockfd_tcp_cq);
        close(sockfd_cq);
		if(Flag_FirstClient_cq != 0)
		{
			add_value_netinfo("ChongQing TCPClient Client the Port 8201 Failed");
			Flag_FirstClient_cq = 0;
		}


    }
    else
    {
		printf ("ChongQing TCPClient Client the Port %d sucessfully.\n", PORT_TCP_CLIENT_CQ);
		add_value_netinfo("ChongQing TCPClient Client the Port 8201 sucessfully");
        Flag_TcpClient_Success_Cq = 1;//连接成功
        client_keep_ali(sockfd_cq);//tcp保活

		if(Flag_FirstClient_cq != 1)
		{
			add_value_netinfo("ChongQing TCPClient Client the Port 8201 Failed");
			Flag_FirstClient_cq = 1;
		}
    }
    sleep(2);
    while(1)
    {
        if(Flag_TcpClient_Success_Cq == 0)
        {
            printf ("wait TCPClient  Client \n");
			close(nsockfd_tcp_cq);
			close(sockfd_cq);
            sleep(5);
            break;
        }
        else
        {
			if((urlip_pre_cq!=IsoOis_UrlIp)||(urlport_pre_cq!=IsoOis_UrlPort))
			{
				qDebug()<<"IP changed! So break!";
				break;
			}
			qDebug()<<"ready to send!";
        }
        //如果不要该协议则退出
        if(Flag_Network_Send_Version != 2)
        {
			close(nsockfd_tcp_cq);
			close(sockfd_cq);
            break;
        }
        sleep(1);
		IsoOis_PW_CQ = IsoOis_PW;//"758534";
		IsoOis_MN_CQ = IsoOis_MN;
    }
    close(nsockfd_tcp_cq);
    close(sockfd_cq);
    sleep(1);
}
//  tcp长连接保活
void net_isoosi_cq::client_keep_ali(int sockfd_cq)
{
    int keepAlive = 1;      // 开启keepalive属性
    int keepIdle = 10;      // 如该连接在60秒内没有任何数据往来,则进行探测
    int keepInterval = 10;   // 探测时发包的时间间隔为5 秒
    int keepCount = 3;      // 探测尝试的次数.如果第1次探测包就收到响应了,则后2次
    setsockopt(sockfd_cq, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepAlive, sizeof(keepAlive));
    setsockopt(sockfd_cq, SOL_TCP, TCP_KEEPIDLE, (void*)&keepIdle, sizeof(keepIdle)); //对应tcp_keepalive_time
    setsockopt(sockfd_cq, SOL_TCP, TCP_KEEPINTVL, (void *)&keepInterval, sizeof(keepInterval)); //对应tcp_keepalive_intvl
    setsockopt(sockfd_cq, SOL_TCP, TCP_KEEPCNT, (void *)&keepCount, sizeof(keepCount)); //对应tcp_keepalive_probes
}
//要发送的数据
void net_isoosi_cq::send_tcpclient_data(QString data)
{
    unsigned int if_send = 0;
    unsigned int sendfail_num = 0;
    if(Flag_TcpClient_Success_Cq == 1)
    {
        wait_send_over_cq.lock();//上锁
        //记得加锁
		qDebug()<<data;
        QByteArray byte_send = data.toUtf8();
        Tcp_send_count_cq = data.length();
		memset(Sdbuf_Cq,0,sizeof(char)*10240);//清零数组
        for(unsigned int i = 0;i < Tcp_send_count_cq;i++)
        {
            Sdbuf_Cq[i] = byte_send[i];
        }
        if(Tcp_send_count_cq != 0)
        {
            while(!if_send)
            {
                int num_send = 0;
                if((num_send = ::send(sockfd_cq,Sdbuf_Cq, Tcp_send_count_cq, 0)) == -1)
                {
                    printf("ERROR: Failed to sent string.\n");
                    if(flag_send_succes_cq == 1)
                    {
                        add_value_netinfo("TCP 数据发送失败");
                    }
					//sendfail_num++;//发送失败再发一次
					if_send = 1;//发送失败 端口有问题  直接退出
					sendfail_num = 0;
					Tcp_send_count_cq = 0;
					Flag_TcpClient_Success_Cq = 0;//连接失败 //重新连接
                    flag_send_succes_cq = 0;
                }
                else
                {
					msleep(50);
                    memset(isoosi_revbuf_cq,0,sizeof(char)*128);//清零数组
					if((num_recv_cq = recv(sockfd_cq,isoosi_revbuf_cq,101,MSG_WAITALL)) <= 0)//MSG_DONTWAIT 1秒延时或者收到89个字节
                    {
                        //发送失败
						qDebug()<<"chongqing send faile !!";
                        sendfail_num++;//发送失败再发一次
                    }
                    else
                    {
						if(num_recv_cq < 50) //返回的是错误信息 需要在发送
						{
							sendfail_num++;//发送失败再发一次
						}
						else
						{
							qDebug()<<"ChongQing Tcp send "<<Tcp_send_count_cq<<" Receive "<<num_recv_cq;
							qDebug()<<isoosi_revbuf_cq;
							if(flag_send_succes_cq == 0)
							{
								add_value_netinfo("TCP 数据发送成功");
							}
							flag_send_succes_cq =1;
							if_send = 1;
							Tcp_send_count_cq = 0;
						}
                    }
                }
				if(sendfail_num>=3)
                {
                    if_send = 1;
                    sendfail_num = 0;
                    Tcp_send_count_cq = 0;
					//Flag_TcpClient_Success_Cq = 0;//连接失败 //没有收到返回。不重新连接
                }
            }
        }
        else
        {
            qDebug()<<"isoosi semd data is kong!!";
        }
        wait_send_over_cq.unlock();//解锁
    }
    else
    {
		qDebug()<< "chongqing tcpclient is not connected!!";
    }
}
/*****************准备发送油枪数据CQ****************
gun_num    油枪编号 3位
AlvR       油枪气液比 1.2
GasCur     油气流速
GasFlow    油气流量
FuelCur    燃油流速
FuelFlow   燃油流量
Gas_con    回收油气浓度
Gas_tem    回收油气温度
DynbPrs    液阻   单位 pa
信号来自reoilgasthread
**********************************************/
void net_isoosi_cq::refueling_gun_data(QString gun_num,QString AlvR,QString GasCur,QString GasFlow,QString FuelCur,QString FuelFlow,
                                       QString gas_con,QString gas_tem, QString DynbPrs)
{
    QDateTime date_time = QDateTime::currentDateTime();
    QString date = date_time.toString("yyyyMMddhhmmss");
    QString send_data;
	IsoOis_QN_CQ = date_time.toString("yyyyMMddhhmmsszzz");

	send_data = "QN="+IsoOis_QN_CQ+";"+IsoOis_ST_Cq+";"+IsoOsi_CN_CQ+";"+"PW="+IsoOis_PW_CQ+";"+"MN="+IsoOis_MN+";"+"Flag=5;"+"CP=&&DataTime="+date;
	send_data.append(";").append(I_GUN_ID_DATA).append(IsoOis_StationId_Cq).append(gun_num)
	         .append(";").append(I_AL).append(AlvR)
	         .append(";").append(I_GAS_SPEED).append(GasCur)
	         .append(";").append(I_GASFLOW).append(GasFlow)
	         .append(";").append(I_OIL_SPEED).append(FuelCur)
	         .append(";").append(I_OIL_FLOW).append(FuelFlow)
	         .append(";").append(I_GAS_CONCENTRATION).append(gas_con)
	         .append(";").append(I_GAS_TEMPERATURE).append(gas_tem)
	         .append(";").append(I_FLUID_RESISTANCE).append(DynbPrs)
	         .append("&&");

    unsigned int long_data = send_data.length();
    QString send_dada_length = QString("%1").arg(long_data,4,10,QLatin1Char('0'));

    QByteArray byte = send_data.toUtf8();
    unsigned char *p = (unsigned char *)byte.data();
    unsigned int crc = CRC16_Checkout(p,byte.length());
    //qDebug()<< byte;
	QString key = QString("%1").arg(crc,4,16,QLatin1Char('0'));//k为int型或char型都可
	key = key.toUpper();
	send_data.append(key).append("\\r\\n");
    //qDebug() << key;
	send_data.prepend(send_dada_length).prepend(send_head_cq);
	//qDebug()<<send_data;
    send_tcpclient_data(send_data);
}

/*****************准备发送环境数据CQ****************
dynbPress     液阻压力 pa
tankPress     油罐压力 pa
unloadgasCon  卸油区油气浓度 卸油区油气浓度的监测数据，单位%/ppm
DevicegasCon  处理装置油气浓度 处理装置排放浓度的监测数据，单位g/m³
QString GasTemp 油气温度
QString GasVolume 油气空间
信号来自fga——1000
**********************************************/
void net_isoosi_cq::environmental_data(QString dynbPress,QString tankPress,QString unloadgasCon,QString DevicegasCon,QString GasTemp,QString GasVolume)
{
    QDateTime date_time = QDateTime::currentDateTime();
    QString date = date_time.toString("yyyyMMddhhmmss");
    QString send_data;
	IsoOis_QN_CQ = date_time.toString("yyyyMMddhhmmsszzz");
	GasVolume = "";

	send_data = "QN="+IsoOis_QN_CQ+";"+IsoOis_ST_Cq+";"+IsoOsi_CN_CQ+";"+"PW="+IsoOis_PW_CQ+";"+"MN="+IsoOis_MN+";"+"Flag=5;"+"CP=&&DataTime="+date;
	send_data.append(";").append(I_TANK_PRESSURE).append(tankPress)
	         .append(";").append(I_RESISTANCE_PRESSURE).append(dynbPress)
	         .append(";").append(I_OILGAS_CONCENTRATION).append(unloadgasCon)
	         .append(";").append(I_AFTER_DEVICE).append(DevicegasCon)
	         .append(";").append(I_TANKORPIPE_TEM).append(GasTemp)
	         .append("&&");

    unsigned int long_data = send_data.length();
    QString send_dada_length = QString("%1").arg(long_data,4,10,QLatin1Char('0'));

    QByteArray byte = send_data.toUtf8();
    unsigned char *p = (unsigned char *)byte.data();
    unsigned int crc = CRC16_Checkout(p,byte.length());
    //qDebug()<< byte;
    QString key = QString("%1").arg(crc,4,16,QLatin1Char('0'));//k为int型或char型都可
	key = key.toUpper();
	send_data.append(key).append("\\r\\n");
    //qDebug() << key;
	send_data.prepend(send_dada_length).prepend(send_head_cq);
	//qDebug()<<send_data;
    send_tcpclient_data(send_data);

}

/*****************准备发送设置数据CQ****************
PVFrwPrs    PV阀正向开启压力
PVRevPrs    PV阀负向开启压力
TrOpenPrs   三次油气回收开启压力
TrStopPrs   三次停止压力
信号来自systemset 和 fga1000
**********************************************/
void net_isoosi_cq::setup_data(QString PVFrwPrs,QString PVRevPrs,QString TrOpenPrs,QString TrStopPrs)
{
	QString send_data;
    QDateTime date_time = QDateTime::currentDateTime();
    QString date = date_time.toString("yyyyMMddhhmmss");
	QString Gun_Num = QString::number(Amount_Gasgun[0]+Amount_Gasgun[1]+Amount_Gasgun[2]+Amount_Gasgun[3]+Amount_Gasgun[4]+Amount_Gasgun[5]+
	                                  Amount_Gasgun[6]+Amount_Gasgun[7]+Amount_Gasgun[8]+Amount_Gasgun[9]+Amount_Gasgun[10]+Amount_Gasgun[11]);
	QString far_dispener_num = QString("%1").arg(Far_Dispener,3,10,QLatin1Char('0'));//k为int型或char型都可;
	IsoOis_QN_CQ = date_time.toString("yyyyMMddhhmmsszzz");
	QString i_date = date_time.toString("yyyyMMdd");

	QString i_time = I_START_TIME + i_date;
	QString i_gunnum = I_GUN_NUM + Gun_Num;
	QString i_PVFrwPrs = I_PV_FORWARD + PVFrwPrs;//PV阀正向开启压力
	QString i_PVRevPrs = I_PV_NEGATIVE + PVRevPrs;//PV阀负向开启压力 pa
	QString i_TrOpenPrs = I_OPEN_PRESSURE + TrOpenPrs; //三次油气回收开启压力
	QString i_TrStopPrs = I_CLOSE_PRESSURE + TrStopPrs;//三次停止压力
	QString i_far_dis = I_FLUIDRESISTANCE_NUM + far_dispener_num;//最远端加油机，安装液阻加油机

	send_data = "QN="+IsoOis_QN_CQ+";"+IsoOis_ST_Cq+";"+IsoOsi_CN_CQ+";"+"PW="+IsoOis_PW_CQ+";"+"MN="+IsoOis_MN+";"+"Flag=5;"+"CP=&&DataTime="+date+";"
	             +i_time+";"+i_gunnum+";"+i_PVFrwPrs+";"+i_PVRevPrs+";"+i_TrOpenPrs+";"+i_TrStopPrs+";"+i_far_dis+"&&";

    unsigned int long_data = send_data.length();
    QString send_dada_length = QString("%1").arg(long_data,4,10,QLatin1Char('0'));

    QByteArray byte = send_data.toUtf8();
    unsigned char *p = (unsigned char *)byte.data();
    unsigned int crc = CRC16_Checkout(p,byte.length());
    //qDebug()<< byte;
    QString key = QString("%1").arg(crc,4,16,QLatin1Char('0'));//k为int型或char型都可
	key = key.toUpper();
	send_data.append(key).append("\\r\\n");
    //qDebug() << key;
	send_data.prepend(send_dada_length).prepend(send_head_cq);
	//qDebug()<<send_data;
    send_tcpclient_data(send_data);
}

/*****************准备发送报警数据CQ****************
gun_data     油枪状态 一次发送1个，在之前处理  后面可以添加密闭性之类的东西
gun_num      油枪编号
TightAlm     密闭性状态
DynbPAlm     液阻预警状态
TankPAlm     油罐压力预警状态
TankZerom    油罐零压预警状态
Prevalve     压力真空阀状态
prevavlelimit 压力真空阀极限状态
DevOpenAlm   后处理装置状态
DeviceAlm    处理装置浓度预警状态
xieyousta    卸油回气管状态
信号来自fga——1000 包括单次和整点的报警
**********************************************/
void net_isoosi_cq::gun_warn_data(QString gun_data,QString gun_num,QString TightAlm,QString DynbPAlm,QString TankPAlm,QString TankZerom,
                                  QString Prevalve,QString prevavlelimit,QString DevOpenAlm,QString DeviceAlm,QString xieyousta )
{
    QDateTime date_time = QDateTime::currentDateTime();
    QString date = date_time.toString("yyyyMMddhhmmss");
    QString send_data;
	IsoOis_QN_CQ = date_time.toString("yyyyMMddhhmmsszzz");


	send_data = "QN="+IsoOis_QN_CQ+";"+IsoOis_ST_Cq+";"+IsoOsi_CN_CQ+";"+"PW="+IsoOis_PW_CQ+";"+"MN="+IsoOis_MN+";"+"Flag=5;"+"CP=&&DataTime="+date;
	if(gun_data!="N")
	{
		send_data.append(";").append(I_GUN_ID_DATA).append(IsoOis_StationId_Cq).append(gun_num).append(";")
		        .append(I_AL_ALARM).append(gun_data);
	}
	if(TightAlm!="N")
	{
		send_data.append(";").append(I_AIRTIGHT_ALARM).append(TightAlm);
	}
	if(DynbPAlm!="N")
	{
		send_data.append(";").append(I_LIQUIDRESISTANCE_ALARM).append(DynbPAlm);
	}
	if(TankPAlm!="N")
	{
		send_data.append(";").append(I_TANK_PRESSURE_ALARM).append(TankPAlm);
	}
	if(TankZerom!="N")
	{
		send_data.append(";").append(I_TANK_ZERO_ALARM).append(TankZerom);
	}
	if(Prevalve!="N")
	{
		send_data.append(";").append(I_PRESSURE_STATUS).append(Prevalve);
	}
	if(prevavlelimit!="N")
	{
		send_data.append(";").append(I_CRITICAL_PRESSURE_ALARM).append(prevavlelimit);
	}
	if(DevOpenAlm!="N")
	{
		send_data.append(";").append(I_POSTPROCESSING).append(DevOpenAlm);
	}
	if(DeviceAlm!="N")
	{
		send_data.append(";").append(I_AFTERTREATMENT_DEVICE).append(DeviceAlm);
	}
	if(xieyousta!="N")
	{
		send_data.append(";").append(I_AIRPIPE_STATE).append(xieyousta);
	}
	send_data.append("&&");

    unsigned int long_data = send_data.length();
    QString send_dada_length = QString("%1").arg(long_data,4,10,QLatin1Char('0'));

    QByteArray byte = send_data.toUtf8();
    unsigned char *p = (unsigned char *)byte.data();
    unsigned int crc = CRC16_Checkout(p,byte.length());
    //qDebug()<< byte;
    QString key = QString("%1").arg(crc,4,16,QLatin1Char('0'));//k为int型或char型都可
	key = key.toUpper();
	send_data.append(key).append("\\r\\n");
    //qDebug() << key;
	send_data.prepend(send_dada_length).prepend(send_head_cq);
	//qDebug()<<send_data;
    send_tcpclient_data(send_data);


}

/*****************准备发送油枪关停启用数据CQ****************
gun_num  油枪编号   一次只能传一把枪
operate  操作类型 0 关 1 启用
Event    事件类型 0 自动 2手动 N未知
//未设置
**********************************************/
void net_isoosi_cq::refueling_gun_stop(QString gun_num,QString operate,QString Event)
{
    QDateTime date_time = QDateTime::currentDateTime();
    QString date = date_time.toString("yyyyMMddhhmmss");
    QString send_data;
	IsoOis_QN_CQ = date_time.toString("yyyyMMddhhmmsszzz");

	send_data = "QN="+IsoOis_QN_CQ+";"+IsoOis_ST_Cq+";"+IsoOsi_CN_CQ+";"+"PW="+IsoOis_PW_CQ+";"+"MN="+IsoOis_MN+";"+"Flag=5;"+"CP=&&DataTime="+date;
	send_data.append(";").append(I_GUN_ID_START).append(IsoOis_StationId_Cq).append(gun_num);
	send_data.append(";").append(I_GUNSTART_TIME).append(date);
	send_data.append(";").append(I_OPERATION_TYPE).append(operate);
	send_data.append(";").append(I_START_TYPE).append(Event);
	send_data.append("&&");

    unsigned int long_data = send_data.length();
    QString send_dada_length = QString("%1").arg(long_data,4,10,QLatin1Char('0'));

    QByteArray byte = send_data.toUtf8();
    unsigned char *p = (unsigned char *)byte.data();
    unsigned int crc = CRC16_Checkout(p,byte.length());
    //qDebug()<< byte;
    QString key = QString("%1").arg(crc,4,16,QLatin1Char('0'));//k为int型或char型都可
	key = key.toUpper();
	send_data.append(key).append("\\r\\n");
    //qDebug() << key;
	send_data.prepend(send_dada_length).prepend(send_head_cq);
	//qDebug()<<send_data;
    send_tcpclient_data(send_data);

}

/*****************准备发送故障数据CQ****************
warn_data  故障编码
信号来自 fga1000和mainwindow

**********************************************/
void net_isoosi_cq::refueling_wrongdata(QString warn_data)
{
    QDateTime date_time = QDateTime::currentDateTime();
    QString date = date_time.toString("yyyyMMddhhmmss");
    QString send_data;
	IsoOis_QN_CQ = date_time.toString("yyyyMMddhhmmsszzz");

	send_data = "QN="+IsoOis_QN_CQ+";"+IsoOis_ST_Cq+";"+IsoOsi_CN_CQ+";"+"PW="+IsoOis_PW_CQ+";"+"MN="+IsoOis_MN+";"+"Flag=5;"+"CP=&&DataTime="+date;

	send_data.append(";").append(I_WRONG_DATE).append(date);
	send_data.append(";").append(I_WRONG_NUM).append(warn_data);
	send_data.append("&&");

	//send_data = "QN=20170627152933831;MN=74829970030002;ST=32;CN=2051;PW=123456;Flag=3;CP=&&BeginTime=20170626153030;EndTime=20170627193030&&";
    unsigned int long_data = send_data.length();
    QString send_dada_length = QString("%1").arg(long_data,4,10,QLatin1Char('0'));

    QByteArray byte = send_data.toUtf8();
    unsigned char *p = (unsigned char *)byte.data();
    unsigned int crc = CRC16_Checkout(p,byte.length());
    //qDebug()<< byte;
	QString key = QString("%1").arg(crc,4,16,QLatin1Char('0'));//k为int型或char型都可
	key = key.toUpper();
	send_data.append(key).append("\\r\\n");
    //qDebug() << key;
	send_data.prepend(send_dada_length).prepend(send_head_cq);
	//qDebug()<<send_data;
    send_tcpclient_data(send_data);
}

/******************************************************************************
函数：CRC16_Checkout
描述：CRC16循环冗余校验算法。
参数一：*puchMsg，需要校验的字符串指针
参数二：usDataLen，要校验的字符串长度
返回值：返回CRC16校验码
******************************************************************************/
unsigned int net_isoosi_cq::CRC16_Checkout ( unsigned char *puchMsg, unsigned int usDataLen )
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


