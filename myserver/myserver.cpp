#include "myserver.h"
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
#include <hash_map>
QMutex myserver_send_over;
void *tcpclient_myserver_read(void*);

unsigned char Flag_FirstClient_MyServer = 3;//标识是否第一次传输，做历史记录
//QString MyStationId = "1234565789"; //全局变量youzhanID
//QString MyServerIp = "192.168.0.121";
//int MyServerPort = 8080;     //1111 The port which is communicate with server     可设置
QString MyStationId = "1234565789"; //全局变量youzhanID
QString MyStationPW = "1234";//全局变量youzhanPW
QString MyServerIp = "118.178.180.140";
int MyServerPort = 8080;     //1111 The port which is communicate with server     可设置
unsigned char Flag_MyServerEn = 0;//使能

QString MyServer_QN = "20160801085857223";
QString MyServer_ST = "ST=61";

int nsockfd_myserver;        //tcp套接字
int sockfd_myserve;            //tcp套接字
int UrlMyServerPort_Pre = 0;
QString UrlMyServerIp_Pre = "";
int on_myserverip = 1;
unsigned char Flag_MyServerClientSuccess = 0;//tcpclient连接成功
char SdbufMyServer[3072] = {0};
unsigned int MyServer_send_count = 0;
unsigned char Flag_MyServer_SendOver = 0;
QString MyServerSendHead = "##";
int MyServerRecvNum;
char myserver_revbuf[128] = {0};
unsigned char Flag_MyserverSendSuccess = 0;//发送成功置1，记录一次


myserver::myserver(QWidget *parent) :
    QThread(parent)
{
	//this->moveToThread(this);
}
void myserver::run()
{
	while(1)
	{
		if(Flag_MyServerEn == 1)
		{
			tcp_client();
			sleep(5);
		}
		else
		{
			sleep(5);
		}
	}
}

void myserver::stop()
{
	is_runnable = false;
}
void myserver::tcp_client()
{
	const QByteArray text = MyServerIp.toLocal8Bit();
	const char *tcp_sever_ip = text.data();
	UrlMyServerIp_Pre = MyServerIp;
	UrlMyServerPort_Pre = MyServerPort;

	struct sockaddr_in sever_remote;
	// Get the Socket file descriptor
	if( (sockfd_myserve = socket(AF_INET, SOCK_STREAM, 0)) == -1 )
	{
		printf ("MyServer TCPClient Failed to obtain Socket Despcritor.\n");
		//add_value_netinfo("MyServer TCPClient Failed to obtain Socket Despcritor");
		//return 0;
	}
	else
	{
		printf ("MyServer TCPClient Obtain Socket Despcritor sucessfully.\n");
		//add_value_netinfo("TCPClient Obtain Socket Despcritor sucessfully");
		//history_net_write("Tcp_Obtain");
	}
	sever_remote.sin_family = AF_INET;           		// Protocol Family 协议族
	sever_remote.sin_port = htons (MyServerPort);         		// Port number 端口号
	sever_remote.sin_addr.s_addr  = inet_addr(tcp_sever_ip);  	// 填写远程地址
	memset (sever_remote.sin_zero,0,8);          		// Flush the rest of struct 刷新struct的其余部分

	//端口重用
	if((setsockopt(sockfd_myserve,SOL_SOCKET,SO_REUSEADDR,&on_myserverip,sizeof(on_myserverip)))<0)
	{
		perror("TCPClient setsockopt failed");
		exit(1);
	}

	bool  bDontLinger = FALSE;
	setsockopt(sockfd_myserve,SOL_SOCKET,SO_LINGER,(const char*)&bDontLinger,sizeof(bool));//不要因为数据未发送就阻塞关闭操作
	setsockopt(sockfd_myserve, IPPROTO_TCP, TCP_NODELAY,&on_myserverip,sizeof(on_myserverip));//立即发送，不沾包
	//int nNetTimeout = 800;//超时时长
	struct timeval timeout = {0,1}; //设置接收超时 1秒？ 第一个参数秒，第二个微秒
	setsockopt(sockfd_myserve,SOL_SOCKET,SO_RCVTIMEO,(char *)&timeout,sizeof(struct timeval));//设置为非阻塞模式
	//主动连接目标,连接不上貌似会卡在这，10秒再连一次
	if (( nsockfd_myserver = ::connect(sockfd_myserve,(struct sockaddr *)&sever_remote,sizeof(struct sockaddr))) < 0)
	{
		printf ("MyServer TCPClient Failed to Client Port %d.\n", MyServerPort);
		//return (0);
		Flag_MyServerClientSuccess = 0;//连接失败
		close(nsockfd_myserver);
		close(sockfd_myserve);
		if(Flag_FirstClient_MyServer != 0)
		{
			add_value_netinfo("MyServer TCPClient Client the Port 8080 Failed");
			Flag_FirstClient_MyServer = 0;
		}

	}
	else
	{
		printf ("MyServer TCPClient Client the Port %d sucessfully.\n", MyServerPort);
		add_value_netinfo("MyServer TCPClient Client the Port 8201 sucessfully");
		Flag_MyServerClientSuccess = 1;//连接成功
		client_keep_ali(sockfd_myserve);//tcp保活

		if(Flag_FirstClient_MyServer != 1)
		{
			add_value_netinfo("MyServer TCPClient Client the Port 8080 Failed");
			Flag_FirstClient_MyServer = 1;
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
	xielousetup(QString::number(count_tank),QString::number(Test_Method),QString::number(count_pipe),QString::number(count_dispener),QString::number(count_basin));
	setup_data(QString::number(Positive_Pres,'f',1),QString::number(-Negative_Pres,'f',1),"0.00","0.00");
	//初次连接发送泄漏和油气回收的设置信息


	while(1)
	{
		if(Flag_MyServerClientSuccess == 0)
		{
			printf ("wait MyServer TCPClient  Client \n");
			sleep(5);
			break;
		}
		else
		{
			if((UrlMyServerIp_Pre!=MyServerIp)||(UrlMyServerPort_Pre!=MyServerPort))
			{
				qDebug()<<"MyServer IP changed! So break!";
				break;
			}
			//qDebug()<<"MyServer ready to send!";

			//xielousta("1","1","2","0","88.8");
			//sleep(2);
			//xielousetup("4","1","4","4","4");
			//refueling_gun_data("q0001","1.1","10.2","12.2","8.3","5.3","null");
			//sleep(2);
			 //environmental_data("10","12","0","1","36.5","0");
			 //sleep(2);
			//setup_data("0","1","2","3");
			//sleep(2);
			//gun_warn_data("q0001-AlvAlm=0","0","0","0","0","0","0","0");
			//sleep(2);
			//refueling_gun_stop("N","N","N");
			//sleep(2);
			//refueling_gun_sta("q0001-Status=0;q0003-Status=0;q0004-Status=0;q0005-Status=0");
			//sleep(2);
		}
		//如果不要该协议则退出
		if(Flag_MyServerEn != 1)
		{
			close(nsockfd_myserver);
			close(sockfd_myserve);
			break;
		}
		sleep(1);
	}
	//qDebug()<<"chagged xieyi !!!@@@@######";
	close(nsockfd_myserver);
	close(sockfd_myserve);
	sleep(1);
}
//  tcp长连接保活
void myserver::client_keep_ali(int sockfd_myserve)
{
	int keepAlive = 1;      // 开启keepalive属性
	int keepIdle = 10;      // 如该连接在60秒内没有任何数据往来,则进行探测
	int keepInterval = 10;   // 探测时发包的时间间隔为5 秒
	int keepCount = 3;      // 探测尝试的次数.如果第1次探测包就收到响应了,则后2次
	setsockopt(sockfd_myserve, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepAlive, sizeof(keepAlive));
	setsockopt(sockfd_myserve, SOL_TCP, TCP_KEEPIDLE, (void*)&keepIdle, sizeof(keepIdle)); //对应tcp_keepalive_time
	setsockopt(sockfd_myserve, SOL_TCP, TCP_KEEPINTVL, (void *)&keepInterval, sizeof(keepInterval)); //对应tcp_keepalive_intvl
	setsockopt(sockfd_myserve, SOL_TCP, TCP_KEEPCNT, (void *)&keepCount, sizeof(keepCount)); //对应tcp_keepalive_probes
}
//要发送的数据
void myserver::send_tcpclient_data(QString data)
{

	unsigned int if_send = 0;
	unsigned int sendfail_num = 0;
	if(Flag_MyServerClientSuccess == 1)
	{
		myserver_send_over.lock();//上锁
		//记得加锁
		qDebug()<<data;
		QByteArray byte_send = data.toUtf8();
		MyServer_send_count = data.length();
		memset(SdbufMyServer,0,sizeof(char)*3072);//清零数组
		for(unsigned int i = 0;i < MyServer_send_count;i++)
		{
			SdbufMyServer[i] = byte_send[i];
		}
		if(MyServer_send_count != 0)
		{
			while(!if_send)
			{
				int num_send = 0;
				if((num_send = ::send(sockfd_myserve,SdbufMyServer, MyServer_send_count, 0)) == -1)
				{
					printf("ERROR: MyServer Failed to sent string.\n");
					if(Flag_MyserverSendSuccess == 1)
					{
						add_value_netinfo("MyServer TCP 数据发送失败");
					}
					//sendfail_num++;//发送失败再发一次
					if_send = 1; //发送失败 端口有问题  直接退出
					sendfail_num = 0;
					MyServer_send_count = 0;
					Flag_MyServerClientSuccess = 0;//连接失败 //重新连接
					Flag_MyserverSendSuccess = 0;
				}
				else
				{
					msleep(300);
					memset(myserver_revbuf,0,sizeof(char)*128);//清零数组
					if((MyServerRecvNum = recv(sockfd_myserve,myserver_revbuf,200,MSG_WAITALL)) <= 0)//MSG_DONTWAIT 1秒延时或者收到40个字节
					{
						//发送失败
						qDebug()<<"MyServer send faile !!";
						sendfail_num++;//发送失败再发一次
					}
					else
					{
						if(QString(myserver_revbuf).contains("QnRtn=1",Qt::CaseSensitive))//成功返回true 第二个参数表示是否大小写敏感
						{
							//真正的发送成功
							//printf("MyServer TcpClient send success %d.\n",MyServer_send_count);
							qDebug()<<"MyServer Tcp send success"<<MyServer_send_count<<" Receive "<<MyServerRecvNum;
							qDebug()<<myserver_revbuf;
							if(Flag_MyserverSendSuccess == 0)
							{
								add_value_netinfo("MyServer TCP 数据发送成功");
							}
							Flag_MyserverSendSuccess =1;
							if_send = 1;
							MyServer_send_count = 0;
						}
						else
						{
							qDebug()<<"MyServer send faile !!";
							sendfail_num++;//发送失败再发一次
						}
					}
				}
				Flag_MyServer_SendOver = 0;
				if(sendfail_num>=3)
				{
					if_send = 1;
					sendfail_num = 0;
					MyServer_send_count = 0;
					//Flag_MyServerClientSuccess = 0;//连接失败 //没有收到返回。不重新连接
				}
			}
		}
		else
		{
			qDebug()<<"myserver semd data is kong!!";
		}
		myserver_send_over.unlock();//解锁
	}
	else
	{
		if(Flag_MyServerEn == 1)
		{
			qDebug()<< "MyServer tcpclient is not connected!!";
		}
	}
}
/*****************准备发送泄漏状态****************
data_type
sensor_type  0传感器法、1液媒法、2压力法、3其他方法
num        传感器编号
sta        传感器状态
data       传感器数据
信号来自reoilgasthread
**********************************************/
void myserver::xielousta(QString data_type,QString num,QString sensor_type,QString sta,QString data)
{
	QDateTime date_time = QDateTime::currentDateTime();
	QString date = date_time.toString("yyyyMMddhhmmss");
	QString send_data;
	MyServer_QN = date;
	MyServer_QN.append("000");

	QString send_SensorType = "SenserType=";       //检测方法
	QString send_DataType = "DataType=";     //数据类型，油罐、管线、加油机、防渗池
	QString send_SensorNum = "SensorNum=";    //传感器编号
	QString send_SensorSta = "SensorSta=";    //传感器状态
	QString send_SensorData = "SensorData=";   //传感器数据
	send_SensorType.append(sensor_type);
	send_DataType.append(data_type);
	send_SensorNum.append(num);
	send_SensorSta.append(sta);
	send_SensorData.append(data);

	send_data = "QN="+MyServer_QN+";"+"MN="+MyStationId+";"+MyServer_ST+";"+"CN=1011;"+"PW="+MyStationPW+";"+"CP=&&DataTime="+date+";"
	             +send_DataType+";"+send_SensorNum+";"+send_SensorType+";"+send_SensorSta+";"+send_SensorData+"&&";

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
	send_data.prepend("0").prepend(send_dada_length).prepend(MyServerSendHead);
	//qDebug()<<send_data;
	send_tcpclient_data(send_data);
}
/*****************准备发送泄漏设置数据****************
tank_num       油罐传感器数量
tank_type      油罐传感器类型
pipe_num       管线传感器数量
dispener_num   加油机传感器数量
basin_num      防渗池传感器数量
信号来自reoilgasthread
**********************************************/
void myserver::xielousetup(QString tank_num,QString tank_type,QString pipe_num,QString dispener_num,QString basin_num)
{
	QDateTime date_time = QDateTime::currentDateTime();
	QString date = date_time.toString("yyyyMMddhhmmss");
	QString send_data;
	MyServer_QN = date;
	MyServer_QN.append("000");

	QString send_OilTankNum = "OilTankNum=";       //油罐传感器数量
	QString send_OilTankType = "OilTankType=";     //油罐传感器类型
	QString send_PipeNum = "PipeNum=";             //管线传感器数量
	QString send_DispenerNum = "DispenerNum=";    //加油机传感器数量
	QString send_BasinNum = "BasinNum=";          //防渗池传感器数量

	send_OilTankNum.append(tank_num);
	send_OilTankType.append(tank_type);
	send_PipeNum.append(pipe_num);
	send_DispenerNum.append(dispener_num);
	send_BasinNum.append(basin_num);

	send_data = "QN="+MyServer_QN+";"+"MN="+MyStationId+";"+MyServer_ST+";"+"CN=1021;"+"PW="+MyStationPW+";"+"CP=&&DataTime="+date+";"
	             +send_OilTankNum+";"+send_OilTankType+";"+send_PipeNum+";"+send_DispenerNum+";"+send_BasinNum+"&&";

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
	send_data.prepend("0").prepend(send_dada_length).prepend(MyServerSendHead);
	//qDebug()<<send_data;
	send_tcpclient_data(send_data);
}
/*****************准备发送油枪数据****************
gun_num    油枪编号 q0001
AlvR       油枪气液比
GasCur     油气流速
GasFlow    油气流量
FuelCur    燃油流速
FuelFlow   燃油流量
DynbPrs    液阻  不上传
信号来自reoilgasthread
**********************************************/
void myserver::refueling_gun_data(QString gun_num,QString AlvR,QString GasCur,QString GasFlow,QString FuelCur,QString FuelFlow,QString DynbPrs)
{
	QDateTime date_time = QDateTime::currentDateTime();
	QString date = date_time.toString("yyyyMMddhhmmss");
	QString send_data;
	MyServer_QN = date;
	MyServer_QN.append("000");

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

	send_data = "QN="+MyServer_QN+";"+"MN="+MyStationId+";"+MyServer_ST+";"+"CN=2011;"+"PW="+MyStationPW+";"+"CP=&&DataTime="+date+";"
	             +send_AlvR+";"+send_GasCur+";"+send_GasFlow+";"+send_FuelCur+";"+send_FuelFlow+"&&";

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
	send_data.prepend("0").prepend(send_dada_length).prepend(MyServerSendHead);
	//qDebug()<<send_data;
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
void myserver::environmental_data(QString dynbPress,QString tankPress,QString unloadgasCon,QString DevicegasCon,QString GasTemp,QString GasVolume)
{
	QDateTime date_time = QDateTime::currentDateTime();
	QString date = date_time.toString("yyyyMMddhhmmss");
	QString send_data;
	MyServer_QN = date;
	MyServer_QN.append("000");

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

//    send_data = "QN="+MyServer_QN+";"+"MN="+MyStationId+";"+MyServer_ST+";"+"CN=2021;"+"PW="+MyStationPW+";"+"CP=&&DataTime="+date+";"
//                 +send_dynbPress+";"+send_tankPress+";"+send_unloadgasCon+";"+send_DevicegasCon+";"+send_GasTemp+";"+send_GasVolume+"&&";
	//没有温度什么的  myserver有温度
	send_data = "QN="+MyServer_QN+";"+"MN="+MyStationId+";"+MyServer_ST+";"+"CN=2021;"+"PW="+MyStationPW+";"+"CP=&&DataTime="+date+";"
	             +send_dynbPress+";"+send_tankPress+";"+send_unloadgasCon+";"+send_DevicegasCon+";"+send_GasTemp+"&&";

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
	send_data.prepend("0").prepend(send_dada_length).prepend(MyServerSendHead);
	//qDebug()<<send_data;
	send_tcpclient_data(send_data);

}

/*****************准备发送设置数据****************
PVFrwPrs    PV阀正向开启压力
PVRevPrs    PV阀负向开启压力
TrOpenPrs   三次油气回收开启压力
TrStopPrs   三次停止压力
信号来自systemset
**********************************************/
void myserver::setup_data(QString PVFrwPrs,QString PVRevPrs,QString TrOpenPrs,QString TrStopPrs)
{
	QDateTime date_time = QDateTime::currentDateTime();
	QString date = date_time.toString("yyyyMMddhhmmss");
	QString send_data;
	QString Gun_Num = QString::number(Amount_Gasgun[0]+Amount_Gasgun[1]+Amount_Gasgun[2]+Amount_Gasgun[3]+Amount_Gasgun[4]+Amount_Gasgun[5]+
	       Amount_Gasgun[6]+Amount_Gasgun[7]+Amount_Gasgun[8]+Amount_Gasgun[9]+Amount_Gasgun[10]+Amount_Gasgun[11]);
	MyServer_QN = date;
	MyServer_QN.append("000");

	QString send_PVFrwPrs = "PVFrwPrs=";       //PV阀正向开启压力
	QString send_PVRevPrs = "PVRevPrs=";     //PV阀负向开启压力 pa
	QString send_TrOpenPrs = "TrOpenPrs=";    //三次油气回收开启压力
	QString send_TrStopPrs = "TrStopPrs=";    //三次停止压力
	send_PVFrwPrs.append(PVFrwPrs);
	send_PVRevPrs.append(PVRevPrs);
	send_TrOpenPrs.append(TrOpenPrs);
	send_TrStopPrs.append(TrStopPrs);

	//下面的用来传输油枪号映射
	unsigned int num_map = 0;
	QString gun_map = "";
	QString Send_Map = "";
	for(unsigned int i = 0;i<Amount_Dispener;i++)
	{
		gun_map = "";
		for(unsigned int j = 0;j<Amount_Gasgun[i];j++)
		{
			num_map++;
			gun_map = QString("%1").arg(num_map,4,10,QLatin1Char('0'));
			gun_map.append("=").append(Mapping_Show[i*8+j]);
			gun_map.prepend("q");

			Send_Map.append(";").append(gun_map);
		}
	}


	send_data = "QN="+MyServer_QN+";"+"MN="+MyStationId+";"+MyServer_ST+";"+"CN=2031;"+"PW="+MyStationPW+";"+"CP=&&DataTime="+date+";"
	             +"GunNum="+Gun_Num+";"+send_PVFrwPrs+";"+send_PVRevPrs+";"+send_TrOpenPrs+";"+send_TrStopPrs+Send_Map+"&&";  //sendmap字符串前面已经有一个；了

//    send_data = "QN="+MyServer_QN+";"+"MN="+MyStationId+";"+MyServer_ST+";"+"CN=2031;"+"PW="+MyStationPW+";"+"CP=&&DataTime="+date+";"
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
	send_data.prepend("0").prepend(send_dada_length).prepend(MyServerSendHead);
	//qDebug()<<send_data;
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
void myserver::gun_warn_data(QString gun_data,QString TightAlm,QString DynbPAlm,QString TankPAlm,QString DeviceAlm,QString PVTapAlm,QString DevOpenAlm,QString DevStopAlm)
{
	QDateTime date_time = QDateTime::currentDateTime();
	QString date = date_time.toString("yyyyMMddhhmmss");
	QString send_data;
	MyServer_QN = date;
	MyServer_QN.append("000");

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

	send_data = "QN="+MyServer_QN+";"+"MN="+MyStationId+";"+MyServer_ST+";"+"CN=2041;"+"PW="+MyStationPW+";"+"CP=&&DataTime="+date+";"
	             +send_gun_data+";"+send_TightAlm+";"+send_DynbPAlm+";"+send_TankPAlm+";"+send_DeviceAlm+";"+send_PVTapAlm+";"
	             +send_DevOpenAlm+";"+send_DevStopAlm+"&&";

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
	send_data.prepend("0").prepend(send_dada_length).prepend(MyServerSendHead);
	//qDebug()<<send_data;
	send_tcpclient_data(send_data);


}

/*****************准备发送油枪关停启用数据****************
gun_num  油枪编号
operate  操作类型 0 关 1 启用
Event    事件类型 0 自动 2手动 N未知
//未设置
**********************************************/
void myserver::refueling_gun_stop(QString gun_num,QString operate,QString Event)
{
	QDateTime date_time = QDateTime::currentDateTime();
	QString date = date_time.toString("yyyyMMddhhmmss");
	QString send_data;
	MyServer_QN = date;
	MyServer_QN.append("000");
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
		send_data = "QN="+MyServer_QN+";"+"MN="+MyStationId+";"+MyServer_ST+";"+"CN=2051;"+"PW="+MyStationPW+";"+"CP=&&"
		             +gun_head+"&&";
	}
	else
	{
		QString send_operate = "-Operate=";     //操作类型 0 关 1 启用
		QString send_Event = "-Event=";    //事件类型 0 自动 2手动 N未知
		(send_operate.append(operate)).prepend(gun_num);
		(send_Event.append(Event)).prepend(gun_num);

		send_data = "QN="+MyServer_QN+";"+"MN="+MyStationId+";"+MyServer_ST+";"+"CN=2051;"+"PW="+MyStationPW+";"+"CP=&&DataTime="+date+";"
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
	send_data.prepend("0").prepend(send_dada_length).prepend(MyServerSendHead);
	//qDebug()<<send_data;
	send_tcpclient_data(send_data);

}

/*****************准备发送油枪状态数据****************
gun_data  油枪状态，所有数据，需要预处理

**********************************************/
void myserver::refueling_gun_sta(QString gun_data)
{
	QDateTime date_time = QDateTime::currentDateTime();
	QString date = date_time.toString("yyyyMMddhhmmss");
	QString send_data;
	MyServer_QN = date;
	MyServer_QN.append("000");

	send_data = "QN="+MyServer_QN+";"+"MN="+MyStationId+";"+MyServer_ST+";"+"CN=2061;"+"PW="+MyStationPW+";"+"CP=&&DataTime="+date+";"
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
	send_data.prepend("0").prepend(send_dada_length).prepend(MyServerSendHead);
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
unsigned int myserver::CRC16_Checkout ( unsigned char *puchMsg, unsigned int usDataLen )
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


void *tcpclient_myserver_read(void*)
{
	while(1)
	{
		usleep(100000);
		if((MyServerRecvNum = recv(sockfd_myserve,myserver_revbuf,sizeof(myserver_revbuf),0)) <= 0)
		{
			qDebug()<<"receive none!!";
		}
		else
		{
			qDebug() << "receive is "<<myserver_revbuf;
		}
	}

}
//qstring转base64
QString qs2B64(QString qs1){//ok

	QByteArray ba;
	ba=qs1.toUtf8();           //QByteArray
	ba=ba.toBase64();          //Base64
	char * cx=ba.data();       //char *
	QString b64qs1=QString(cx);//QString
	return b64qs1;
}
//base64转qstring
QString b64ToQs(QString b64qs1){//ok

	QByteArray ba;
	std::string stdStr = b64qs1.toStdString();//std::string
	ba=QByteArray(stdStr.c_str() );           //QByteArray
	ba=ba.fromBase64(ba);                     //unBase64
	QString qs1=QString::fromUtf8(ba);        //QString
	return  qs1;
}
