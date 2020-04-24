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
	sleep(5);
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
		Flag_MyServerClientSuccess = 1;//连接成功
		if(Flag_FirstClient_MyServer != 1)
		{
			add_value_netinfo("MyServer TCPClient Client the Port 8201 sucessfully");
			Flag_FirstClient_MyServer = 1;
		}
		sleep(1);
		emit Myserver_First_Client();//第一次连接发送数据

		client_keep_ali(sockfd_myserve);//tcp保活


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
					msleep(400);
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
	if(Flag_MyServerEn == 1)
	{
		unsigned char SensorType = 0;;
		if(Test_Method == 0){SensorType = 3;}//其他方法
		else if(Test_Method == 1){SensorType = 2;}//压力法
		else if(Test_Method == 2){SensorType = 1;}//液媒法
		else if(Test_Method == 3){SensorType = 0;}//传感器法
		else {
			SensorType = 0;
		}
		xielousetup(QString::number(count_tank),QString::number(SensorType),QString::number(count_pipe),QString::number(count_dispener),QString::number(count_basin));
	}

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
		unsigned int gun_num_send = 0;
		for(unsigned int i = 0;i < Amount_Dispener;i++)
		{
			for(unsigned int j = 0;j < Amount_Gasgun[i];j++)
			{
				gun_num_send++;
				gun_head_num = QString("%1").arg(gun_num_send,4,10,QLatin1Char('0'));//k为int型或char型都可
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
	if(Flag_MyServerEn == 1)//泄漏状态
	{
		analysis_xielou_sta();
	}
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

void myserver::analysis_xielou_sta()//分析泄漏状态，用于发送
{
	//basin   防渗池
	if(count_basin>=1)          //1#
	{
		if(OIL_BASIN[0]==0xc0)
		{
			if(OIL_BASIN[1]==0x00 )           //flag_output_basin[0] == 0 || //flag_output_basin[0] != 1))
			{
				//emit right_basin(1);
				net_history(17,0);
				//add_value_controlinfo("1# 防渗池 ","设备正常");
				//flag_output_basin[0] = 1;
			}
			else
			{
				if(OIL_BASIN[1]==0x88 )           //flag_output_basin[0] == 0 || //flag_output_basin[0] != 2))
				{
					//emit warning_oil_basin(1);//油报警
					net_history(17,1);
					//add_value_controlinfo("1# 防渗池 ","检油报警");
					//flag_output_basin[0] = 2;
				}
				if(OIL_BASIN[1]==0x90 )           //flag_output_basin[0] == 0 || //flag_output_basin[0] != 3))
				{
					//emit warning_water_basin(1);//水报警
					net_history(17,2);
					//add_value_controlinfo("1# 防渗池 ","检水报警");
					//flag_output_basin[0] = 3;
				}
				if(OIL_BASIN[1]==0x01 )           //flag_output_basin[0] == 0 || //flag_output_basin[0] != 4))
				{
					//emit warning_sensor_basin(1);//传感器故障
					net_history(17,3);
					//add_value_controlinfo("1# 防渗池 ","传感器故障");
					//flag_output_basin[0] = 4;
				}
				if(OIL_BASIN[1]==0x04 )           //flag_output_basin[0] == 0 || //flag_output_basin[0] != 5))
				{
					//emit warning_uart_basin(1);//通信故障
					net_history(17,4);
					//add_value_controlinfo("1# 防渗池 ","通信故障");
					//flag_output_basin[0] = 5;
				}
			}
		}
	}
	if(count_basin>=2)              //2#
	{
		if(OIL_BASIN[2]==0xc0)
		{
			if(OIL_BASIN[3]==0x00 )           //flag_output_basin[1] == 0 || //flag_output_basin[1] != 1))
			{
				//emit right_basin(2);
				net_history(18,0);
				//add_value_controlinfo("2# 防渗池 ","设备正常");
				//flag_output_basin[1] = 1;
			}
			else
			{
				if(OIL_BASIN[3]==0x88 )           //flag_output_basin[1] == 0 || //flag_output_basin[1] != 2))
				{
					//emit warning_oil_basin(2);//油报警
					net_history(18,1);
					//add_value_controlinfo("2# 防渗池 ","检油报警");
					//flag_output_basin[1] = 2;
				}
				if(OIL_BASIN[3]==0x90 )           //flag_output_basin[1] == 0 || //flag_output_basin[1] != 3))
				{
					//emit warning_water_basin(2);//水报警
					net_history(18,2);
					//add_value_controlinfo("2# 防渗池 ","检水报警");
					//flag_output_basin[1] = 3;
				}
				if(OIL_BASIN[3]==0x01 )           //flag_output_basin[1] == 0 || //flag_output_basin[1] != 4))
				{
					//emit warning_sensor_basin(2);//传感器故障
					net_history(18,3);
					//add_value_controlinfo("2# 防渗池 ","传感器故障");
					//flag_output_basin[1] = 4;
				}
				if(OIL_BASIN[3]==0x04 )           //flag_output_basin[1] == 0 || //flag_output_basin[1] != 5))
				{
					//emit warning_uart_basin(2);//通信故障
					net_history(18,4);
					//add_value_controlinfo("2# 防渗池 ","通信故障");
					//flag_output_basin[1] = 5;
				}
			}
		}
	}
	if(count_basin>=3)              //3#
	{
		if(OIL_BASIN[4]==0xc0)
		{
			if(OIL_BASIN[5]==0x00 )           //flag_output_basin[2] == 0 || //flag_output_basin[2] != 1))
			{
				//emit right_basin(3);
				net_history(19,0);
				//add_value_controlinfo("3# 防渗池 ","设备正常");
				//flag_output_basin[2] = 1;
			}
			else
			{
				if(OIL_BASIN[5]==0x88 )           //flag_output_basin[2] == 0 || //flag_output_basin[2] != 2))
				{
					//emit warning_oil_basin(3);//油报警
					net_history(19,1);
					//add_value_controlinfo("3# 防渗池 ","检油报警");
					//flag_output_basin[2] = 2;
				}
				if(OIL_BASIN[5]==0x90 )           //flag_output_basin[2] == 0 || //flag_output_basin[2] != 3))
				{
					//emit warning_water_basin(3);//水报警
					net_history(19,2);
					//add_value_controlinfo("3# 防渗池 ","检水报警");
					//flag_output_basin[2] = 3;
				}
				if(OIL_BASIN[5]==0x01 )           //flag_output_basin[2] == 0 || //flag_output_basin[2] != 4))
				{
					//emit warning_sensor_basin(3);//传感器故障
					net_history(19,3);
					//add_value_controlinfo("3# 防渗池 ","传感器故障");
					//flag_output_basin[2] = 4;
				}
				if(OIL_BASIN[5]==0x04 )           //flag_output_basin[2] == 0 || //flag_output_basin[2] != 5))
				{
					//emit warning_uart_basin(3);//通信故障
					net_history(19,4);
					//add_value_controlinfo("3# 防渗池 ","通信故障");
					//flag_output_basin[2] = 5;
				}
			}
		}
	}
	if(count_basin>=4)              //4#
	{
		if(OIL_BASIN[6]==0xc0)
		{
			if(OIL_BASIN[7]==0x00 )           //flag_output_basin[3] == 0 || //flag_output_basin[3] != 1))
			{
				//emit right_basin(4);
				net_history(20,0);
				//add_value_controlinfo("4# 防渗池 ","设备正常");
				//flag_output_basin[3] = 1;
			}
			else
			{
				if(OIL_BASIN[7]==0x88 )           //flag_output_basin[3] == 0 || //flag_output_basin[3] != 2))
				{
					//emit warning_oil_basin(4);//油报警
					net_history(20,1);
					//add_value_controlinfo("4# 防渗池 ","检油报警");
					//flag_output_basin[3] = 2;
				}
				if(OIL_BASIN[7]==0x90 )           //flag_output_basin[3] == 0 || //flag_output_basin[3] != 3))
				{
					//emit warning_water_basin(4);//水报警
					net_history(20,2);
					//add_value_controlinfo("4# 防渗池 ","检水报警");
					//flag_output_basin[3] = 3;
				}
				if(OIL_BASIN[7]==0x01 )           //flag_output_basin[3] == 0 || //flag_output_basin[3] != 4))
				{
					//emit warning_sensor_basin(4);//传感器故障
					net_history(20,3);
					//add_value_controlinfo("4# 防渗池 ","传感器故障");
					//flag_output_basin[3] = 4;
				}
				if(OIL_BASIN[7]==0x04 )           //flag_output_basin[3] == 0 || //flag_output_basin[3] != 5))
				{
					//emit warning_uart_basin(4);//通信故障
					net_history(20,4);
					//add_value_controlinfo("4# 防渗池 ","通信故障");
					//flag_output_basin[3] = 5;
				}
			}
		}
	}
	if(count_basin>=5)             //5#
	{
		if(OIL_BASIN[8]==0xc0)
		{
			if(OIL_BASIN[9]==0x00 )           //flag_output_basin[4] == 0 || //flag_output_basin[4] != 1))
			{
				//emit right_basin(5);
				net_history(21,0);
				//add_value_controlinfo("5# 防渗池 ","设备正常");
				//flag_output_basin[4] = 1;
			}
			else
			{
				if(OIL_BASIN[9]==0x88 )           //flag_output_basin[4] == 0 || //flag_output_basin[4] != 2))
				{
					//emit warning_oil_basin(5);//油报警
					net_history(21,1);
					//add_value_controlinfo("5# 防渗池 ","检油报警");
					//flag_output_basin[4] = 2;
				}
				if(OIL_BASIN[9]==0x90 )           //flag_output_basin[4] == 0 || //flag_output_basin[4] != 3))
				{
					//emit warning_water_basin(5);//水报警
					net_history(21,2);
					//add_value_controlinfo("5# 防渗池 ","检水报警");
					//flag_output_basin[4] = 3;
				}
				if(OIL_BASIN[9]==0x01 )           //flag_output_basin[4] == 0 || //flag_output_basin[4] != 4))
				{
					//emit warning_sensor_basin(5);//传感器故障
					net_history(21,3);
					//add_value_controlinfo("5# 防渗池 ","传感器故障");
					//flag_output_basin[4] = 4;
				}
				if(OIL_BASIN[9]==0x04 )           //flag_output_basin[4] == 0 || //flag_output_basin[4] != 5))
				{
					//emit warning_uart_basin(5);//通信故障
					net_history(21,4);
					//add_value_controlinfo("5# 防渗池 ","通信故障");
					//flag_output_basin[4] = 5;
				}
			}
		}
	}
	if(count_basin>=6)             //6#
	{
		if(OIL_BASIN[10]==0xc0)
		{
			if(OIL_BASIN[11]==0x00 )           //flag_output_basin[5] == 0 || //flag_output_basin[5] != 1))
			{
				//emit right_basin(6);
				net_history(22,0);
				//add_value_controlinfo("6# 防渗池 ","设备正常");
				//flag_output_basin[5] = 1;
			}
			else
			{
				if(OIL_BASIN[11]==0x88 )           //flag_output_basin[5] == 0 || //flag_output_basin[5] != 2))
				{
					//emit warning_oil_basin(6);//油报警
					net_history(22,1);
					//add_value_controlinfo("6# 防渗池 ","检油报警");
					//flag_output_basin[5] = 2;
				}
				if(OIL_BASIN[11]==0x90 )           //flag_output_basin[5] == 0 || //flag_output_basin[5] != 3))
				{
					//emit warning_water_basin(6);//水报警
					net_history(22,2);
					//add_value_controlinfo("6# 防渗池 ","检水报警");
					//flag_output_basin[5] = 3;
				}
				if(OIL_BASIN[11]==0x01 )           //flag_output_basin[5] == 0 || //flag_output_basin[5] != 4))
				{
					//emit warning_sensor_basin(6);//传感器故障
					net_history(22,3);
					//add_value_controlinfo("6# 防渗池 ","传感器故障");
					//flag_output_basin[5] = 4;
				}
				if(OIL_BASIN[11]==0x04 )           //flag_output_basin[5] == 0 || //flag_output_basin[5] != 5))
				{
					//emit warning_uart_basin(6);//通信故障
					net_history(22,4);
					//add_value_controlinfo("6# 防渗池 ","通信故障");
					//flag_output_basin[5] = 5;
				}
			}
		}
	}
	if(count_basin>=7)              //7#
	{
		if(OIL_BASIN[12]==0xc0)
		{
			if(OIL_BASIN[13]==0x00 )           //flag_output_basin[6] == 0 || //flag_output_basin[6] != 1))
			{
				//emit right_basin(7);
				net_history(23,0);
				//add_value_controlinfo("7# 防渗池 ","设备正常");
				//flag_output_basin[6] = 1;
			}
			else
			{
				if(OIL_BASIN[13]==0x88 )           //flag_output_basin[6] == 0 || //flag_output_basin[6] != 2))
				{
					//emit warning_oil_basin(7);//油报警
					net_history(23,1);
					//add_value_controlinfo("7# 防渗池 ","检油报警");
					//flag_output_basin[6] = 2;
				}
				if(OIL_BASIN[13]==0x90 )           //flag_output_basin[6] == 0 || //flag_output_basin[6] != 3))
				{
					//emit warning_water_basin(7);//水报警
					net_history(23,2);
					//add_value_controlinfo("7# 防渗池 ","检水报警");
					//flag_output_basin[6] = 3;
				}
				if(OIL_BASIN[13]==0x01 )           //flag_output_basin[6] == 0 || //flag_output_basin[6] != 4))
				{
					//emit warning_sensor_basin(7);//传感器故障
					net_history(23,3);
					//add_value_controlinfo("7# 防渗池 ","传感器故障");
					//flag_output_basin[6] = 4;
				}
				if(OIL_BASIN[13]==0x04 )           //flag_output_basin[6] == 0 || //flag_output_basin[6] != 5))
				{
					//emit warning_uart_basin(7);//通信故障
					net_history(23,4);
					//add_value_controlinfo("7# 防渗池 ","通信故障");
					//flag_output_basin[6] = 5;
				}
			}
		}
	}
	if(count_basin>=8)           //8#
	{
		if(OIL_BASIN[14]==0xc0)
		{

			if(OIL_BASIN[15]==0x00 )           //flag_output_basin[7] == 0 || //flag_output_basin[7] != 1))
			{
				//emit right_basin(8);
				net_history(24,0);
				//add_value_controlinfo("8# 防渗池 ","设备正常");
				//flag_output_basin[7] = 1;
			}
			else
			{
				if(OIL_BASIN[15]==0x88 )           //flag_output_basin[7] == 0 || //flag_output_basin[7] != 2))
				{
					//emit warning_oil_basin(8);//油报警
					net_history(24,1);
					//add_value_controlinfo("8# 防渗池 ","检油报警");
					//flag_output_basin[7] = 2;
				}
				if(OIL_BASIN[15]==0x90 )           //flag_output_basin[7] == 0 || //flag_output_basin[7] != 3))
				{
					//emit warning_water_basin(8);//水报警
					net_history(24,2);
					//add_value_controlinfo("8# 防渗池 ","检水报警");
					//flag_output_basin[7] = 3;
				}
				if(OIL_BASIN[15]==0x01 )           //flag_output_basin[7] == 0 || //flag_output_basin[7] != 4))
				{
					//emit warning_sensor_basin(8);//传感器故障
					net_history(24,3);
					//add_value_controlinfo("8# 防渗池 ","传感器故障");
					//flag_output_basin[7] = 4;
				}
				if(OIL_BASIN[15]==0x04 )           //flag_output_basin[7] == 0 || //flag_output_basin[7] != 5))
				{
					//emit warning_uart_basin(8);//通信故障
					net_history(24,4);
					//add_value_controlinfo("8# 防渗池 ","通信故障");
					//flag_output_basin[7] = 5;
				}
			}
		}
	}
	//pipe 管线
	if(count_pipe>=1)        //1#
	{
		if(OIL_PIPE[0]==0xc0)
		{
			if(OIL_PIPE[1]==0x00 )           //flag_output_pipe[0] == 0 || //flag_output_pipe[0] != 1))
			{
				//emit right_pipe(91);
				net_history(9,0);
				//add_value_controlinfo("1# 管线   ","设备正常");
				//flag_output_pipe[0] = 1;
			}
			else
			{
				if(OIL_PIPE[1]==0x88 )           //flag_output_pipe[0] == 0 || //flag_output_pipe[0] != 2))
				{
					//emit warning_oil_pipe(91);//油报警
					net_history(9,1);
					//add_value_controlinfo("1# 管线   ","检油报警");
					//flag_output_pipe[0] = 2;
				}
				if(OIL_PIPE[1]==0x90 )           //flag_output_pipe[0] == 0 || //flag_output_pipe[0] != 3))
				{
					//emit warning_water_pipe(91);//水报警
					net_history(9,2);
					//add_value_controlinfo("1# 管线   ","检水报警");
					//flag_output_pipe[0] = 3;
				}
				if(OIL_PIPE[1]==0x01 )           //flag_output_pipe[0] == 0 || //flag_output_pipe[0] != 4))
				{
					//emit warning_sensor_pipe(91);//传感器故障
					net_history(9,3);
					//add_value_controlinfo("1# 管线   ","传感器故障");
					//flag_output_pipe[0] = 4;
				}
				if(OIL_PIPE[1]==0x04 )           //flag_output_pipe[0] == 0 || //flag_output_pipe[0] != 5))
				{
					//emit warning_uart_pipe(91);//通信故障
					net_history(9,4);
					//add_value_controlinfo("1# 管线   ","通信故障");
					//flag_output_pipe[0] = 5;
				}
			}

		}
	}
	if(count_pipe>=2)       //2#
	{
		if(OIL_PIPE[2]==0xc0)
		{
			if(OIL_PIPE[3]==0x00 )           //flag_output_pipe[1] == 0 || //flag_output_pipe[1] != 1))
			{
				//emit right_pipe(92);
				net_history(10,0);
				//add_value_controlinfo("2# 管线   ","设备正常");
				//flag_output_pipe[1] = 1;
			}
			else
			{
				if(OIL_PIPE[3]==0x88 )           //flag_output_pipe[1] == 0 || //flag_output_pipe[1] != 2))
				{
					//emit warning_oil_pipe(92);//油报警
					net_history(10,1);
					//add_value_controlinfo("2# 管线   ","检油报警");
					//flag_output_pipe[1] = 2;
				}
				if(OIL_PIPE[3]==0x90 )           //flag_output_pipe[1] == 0 || //flag_output_pipe[1] != 3))
				{
					//emit warning_water_pipe(92);//水报警
					net_history(10,2);
					//add_value_controlinfo("2# 管线   ","检水报警");
					//flag_output_pipe[1] = 3;
				}
				if(OIL_PIPE[3]==0x01 )           //flag_output_pipe[1] == 0 || //flag_output_pipe[1] != 4))
				{
					//emit warning_sensor_pipe(92);//传感器故障
					net_history(10,3);
					//add_value_controlinfo("2# 管线   ","传感器故障");
					//flag_output_pipe[1] = 4;
				}
				if(OIL_PIPE[3]==0x04 )           //flag_output_pipe[1] == 0 || //flag_output_pipe[1] != 5))
				{
					//emit warning_uart_pipe(92);//通信故障
					net_history(10,4);
					//add_value_controlinfo("2# 管线   ","通信故障");
					//flag_output_pipe[1] = 5;
				}
			}
		}
	}
	if(count_pipe>=3)           //3#
	{
		if(OIL_PIPE[4]==0xc0)
		{
			if(OIL_PIPE[5]==0x00 )           //flag_output_pipe[2] == 0 || //flag_output_pipe[2] != 1))
			{
				//emit right_pipe(93);
				net_history(11,0);
				//add_value_controlinfo("3# 管线   ","设备正常");
				//flag_output_pipe[2] = 1;
			}
			else
			{
				if(OIL_PIPE[5]==0x88 )           //flag_output_pipe[2] == 0 || //flag_output_pipe[2] != 2))
				{
					//emit warning_oil_pipe(93);//油报警
					net_history(11,1);
					//add_value_controlinfo("3# 管线   ","检油报警");
					//flag_output_pipe[2] = 2;
				}
				if(OIL_PIPE[5]==0x90 )           //flag_output_pipe[2] == 0 || //flag_output_pipe[2] != 3))
				{
					//emit warning_water_pipe(93);//水报警
					net_history(11,2);
					//add_value_controlinfo("3# 管线   ","检水报警");
					//flag_output_pipe[2] = 3;
				}
				if(OIL_PIPE[5]==0x01 )           //flag_output_pipe[2] == 0 || //flag_output_pipe[2] != 4))
				{
					//emit warning_sensor_pipe(93);//传感器故障
					net_history(11,3);
					//add_value_controlinfo("3# 管线   ","传感器故障");
					//flag_output_pipe[2] = 4;
				}
				if(OIL_PIPE[5]==0x04 )           //flag_output_pipe[2] == 0 || //flag_output_pipe[2] != 5))
				{
					//emit warning_uart_pipe(93);//通信故障
					net_history(11,4);
					//add_value_controlinfo("3# 管线   ","通信故障");
					//flag_output_pipe[2] = 5;
				}
			}
		}
	}
	if(count_pipe>=4)       //4#
	{
		if(OIL_PIPE[6]==0xc0)
		{

			if(OIL_PIPE[7]==0x00 )           //flag_output_pipe[3] == 0 || //flag_output_pipe[3] != 1))
			{
				//emit right_pipe(94);
				net_history(12,0);
				//add_value_controlinfo("4# 管线   ","设备正常");
				//flag_output_pipe[3] = 1;
			}
			else
			{
				if(OIL_PIPE[7]==0x88 )           //flag_output_pipe[3] == 0 || //flag_output_pipe[3] != 2))
				{
					//emit warning_oil_pipe(94);//油报警
					net_history(12,1);
					//add_value_controlinfo("4# 管线   ","检油报警");
					//flag_output_pipe[3] = 2;
				}
				if(OIL_PIPE[7]==0x90 )           //flag_output_pipe[3] == 0 || //flag_output_pipe[3] != 3))
				{
					//emit warning_water_pipe(94);//水报警
					net_history(12,2);
					//add_value_controlinfo("4# 管线   ","检水报警");
					//flag_output_pipe[3] = 3;
				}
				if(OIL_PIPE[7]==0x01 )           //flag_output_pipe[3] == 0 || //flag_output_pipe[3] != 4))
				{
					//emit warning_sensor_pipe(94);//传感器故障
					net_history(12,3);
					//add_value_controlinfo("4# 管线   ","传感器故障");
					//flag_output_pipe[3] = 4;
				}
				if(OIL_PIPE[7]==0x04 )           //flag_output_pipe[3] == 0 || //flag_output_pipe[3] != 5))
				{
					//emit warning_uart_pipe(94);//通信故障
					net_history(12,4);
					//add_value_controlinfo("4# 管线   ","通信故障");
					//flag_output_pipe[3] = 5;
				}
			}
		}
	}
	if(count_pipe>=5)       //5#
	{
		if(OIL_PIPE[8]==0xc0)
		{

			if(OIL_PIPE[9]==0x00 )           //flag_output_pipe[4] == 0 || //flag_output_pipe[4] != 1))
			{
				//emit right_pipe(95);
				net_history(13,0);
				//add_value_controlinfo("5# 管线   ","设备正常");
				//flag_output_pipe[4] = 1;
			}
			else
			{
				if(OIL_PIPE[9]==0x88 )           //flag_output_pipe[4] == 0 || //flag_output_pipe[4] != 2))
				{
					//emit warning_oil_pipe(95);//油报警
					net_history(13,1);
					//add_value_controlinfo("5# 管线   ","检油报警");
					//flag_output_pipe[4] = 2;
				}
				if(OIL_PIPE[9]==0x90 )           //flag_output_pipe[4] == 0 || //flag_output_pipe[4] != 3))
				{
					//emit warning_water_pipe(95);//水报警
					net_history(13,2);
					//add_value_controlinfo("5# 管线   ","检水报警");
					//flag_output_pipe[4] = 3;
				}
				if(OIL_PIPE[9]==0x01 )           //flag_output_pipe[4] == 0 || //flag_output_pipe[4] != 4))
				{
					//emit warning_sensor_pipe(95);//传感器故障
					net_history(13,3);
					//add_value_controlinfo("5# 管线   ","传感器故障");
					//flag_output_pipe[4] = 4;
				}
				if(OIL_PIPE[9]==0x04 )           //flag_output_pipe[4] == 0 || //flag_output_pipe[4] != 5))
				{
					//emit warning_uart_pipe(95);//通信故障
					net_history(13,4);
					//add_value_controlinfo("5# 管线   ","通信故障");
					//flag_output_pipe[4] = 5;
				}
			}
		}
	}
	if(count_pipe>=6)           //6#
	{
		if(OIL_PIPE[10]==0xc0)
		{

			if(OIL_PIPE[11]==0x00 )           //flag_output_pipe[5] == 0 || //flag_output_pipe[5] != 1))
			{
				//emit right_pipe(96);
				net_history(14,0);
				//add_value_controlinfo("6# 管线   ","设备正常");
				//flag_output_pipe[5] = 1;
			}
			else
			{
				if(OIL_PIPE[11]==0x88 )           //flag_output_pipe[5] == 0 || //flag_output_pipe[5] != 2))
				{
					//emit warning_oil_pipe(96);//油报警
					net_history(14,1);
					//add_value_controlinfo("6# 管线   ","检油报警");
					//flag_output_pipe[5] = 2;
				}
				if(OIL_PIPE[11]==0x90 )           //flag_output_pipe[5] == 0 || //flag_output_pipe[5] != 3))
				{
					//emit warning_water_pipe(96);//水报警
					net_history(14,2);
					//add_value_controlinfo("6# 管线   ","检水报警");
					//flag_output_pipe[5] = 3;
				}
				if(OIL_PIPE[11]==0x01 )           //flag_output_pipe[5] == 0 || //flag_output_pipe[5] != 4))
				{
					//emit warning_sensor_pipe(96);//传感器故障
					net_history(14,3);
					//add_value_controlinfo("6# 管线   ","传感器故障");
					//flag_output_pipe[5] = 4;
				}
				if(OIL_PIPE[11]==0x04 )           //flag_output_pipe[5] == 0 || //flag_output_pipe[5] != 5))
				{
					//emit warning_uart_pipe(96);//通信故障
					net_history(14,4);
					//add_value_controlinfo("6# 管线   ","通信故障");
					//flag_output_pipe[5] = 5;
				}
			}
		}
	}
	if(count_pipe>=7)            //7#
	{
		if(OIL_PIPE[12]==0xc0)
		{

			if(OIL_PIPE[13]==0x00 )           //flag_output_pipe[6] == 0 || //flag_output_pipe[6] != 1))
			{
				//emit right_pipe(97);
				net_history(15,0);
				//add_value_controlinfo("7# 管线   ","设备正常");
				//flag_output_pipe[6] = 1;
			}
			else
			{
				if(OIL_PIPE[13]==0x88 )           //flag_output_pipe[6] == 0 || //flag_output_pipe[6] != 2))
				{
					//emit warning_oil_pipe(97);//油报警
					net_history(15,1);
					//add_value_controlinfo("7# 管线   ","检油报警");
					//flag_output_pipe[6] = 2;
				}
				if(OIL_PIPE[13]==0x90 )           //flag_output_pipe[6] == 0 || //flag_output_pipe[6] != 3))
				{
					//emit warning_water_pipe(97);//水报警
					net_history(15,2);
					//add_value_controlinfo("7# 管线   ","检水报警");
					//flag_output_pipe[6] = 3;
				}
				if(OIL_PIPE[13]==0x01 )           //flag_output_pipe[6] == 0 || //flag_output_pipe[6] != 4))
				{
					//emit warning_sensor_pipe(97);//传感器故障
					net_history(15,3);
					//add_value_controlinfo("7# 管线   ","传感器故障");
					//flag_output_pipe[6] = 4;
				}
				if(OIL_PIPE[13]==0x04 )           //flag_output_pipe[6] == 0 || //flag_output_pipe[6] != 5))
				{
					//emit warning_uart_pipe(97);//通信故障
					net_history(15,4);
					//add_value_controlinfo("7# 管线   ","通信故障");
					//flag_output_pipe[6] = 5;
				}
			}
		}
	}
	if(count_pipe>=8)           //8#
	{
		if(OIL_PIPE[14]==0xc0)
		{

			if(OIL_PIPE[15]==0x00 )           //flag_output_pipe[7] == 0 || //flag_output_pipe[7] != 1))
			{
				//emit right_pipe(98);
				net_history(16,0);
				//add_value_controlinfo("8# 管线   ","设备正常");
				//flag_output_pipe[7] = 1;
			}
			else
			{
				if(OIL_PIPE[15]==0x88 )           //flag_output_pipe[7] == 0 || //flag_output_pipe[7] != 2))
				{
					//emit warning_oil_pipe(98);//油报警
					net_history(16,1);
					//add_value_controlinfo("8# 管线   ","检油报警");
					//flag_output_pipe[7] = 2;
				}
				if(OIL_PIPE[15]==0x90 )           //flag_output_pipe[7] == 0 || //flag_output_pipe[7] != 3))
				{
					//emit warning_water_pipe(98);//水报警
					net_history(16,2);
					//add_value_controlinfo("8# 管线   ","检水报警");
					//flag_output_pipe[7] = 3;
				}
				if(OIL_PIPE[15]==0x01 )           //flag_output_pipe[7] == 0 || //flag_output_pipe[7] != 4))
				{
					//emit warning_sensor_pipe(98);//传感器故障
					net_history(16,3);
					//add_value_controlinfo("8# 管线   ","传感器故障");
					//flag_output_pipe[7] = 4;
				}
				if(OIL_PIPE[15]==0x04 )           //flag_output_pipe[7] == 0 || //flag_output_pipe[7] != 5))
				{
					//emit warning_uart_pipe(98);//通信故障
					net_history(16,4);
					//add_value_controlinfo("8# 管线   ","通信故障");
					//flag_output_pipe[7] = 5;
				}
			}
		}
	}

	//tank 罐
	if((OIL_TANK[0]&0xf0)==0xc0)      //传感器法
	{
		if(count_tank>=1)           //1#
		{
			if((OIL_TANK[0]&0x0f)==0)
			{
				if(OIL_TANK[1]==0 )           //flag_output_tank[0] == 0 || //flag_output_tank[0] != 1))
				{
					//emit right_tank(71);
					net_history(1,0);
					//add_value_controlinfo("1# 油罐   ","设备正常");
					//flag_output_tank[0] = 1;
				}
				else
				{
					if(OIL_TANK[1]==0x88 )           //flag_output_tank[0] == 0 || //flag_output_tank[0] != 2))
					{
						//emit warning_oil_tank(71);//油报警
						net_history(1,1);
						//add_value_controlinfo("1# 油罐   ","检油报警");
						//flag_output_tank[0] = 2;
					}
					if(OIL_TANK[1]==0x90 )           //flag_output_tank[0] == 0 || //flag_output_tank[0] != 3))
					{
						//emit warning_water_tank(71);//水报警
						net_history(1,2);
						//add_value_controlinfo("1# 油罐   ","检水报警");
						//flag_output_tank[0] = 3;
					}
					if(OIL_TANK[1]==0x01 )           //flag_output_tank[0] == 0 || //flag_output_tank[0] != 4))
					{
						//emit warning_sensor_tank(71);//传感器故障
						net_history(1,3);
						//add_value_controlinfo("1# 油罐   ","传感器故障");
						//flag_output_tank[0] = 4;
					}
					if(OIL_TANK[1]==0x04 )           //flag_output_tank[0] == 0 || //flag_output_tank[0] != 5))
					{
						//emit warning_uart_tank(71);//通信故障
						net_history(1,4);
						//add_value_controlinfo("1# 油罐   ","通信故障");
						//flag_output_tank[0] = 5;
					}
				}
			}
		}
		if(count_tank>=2)            //2#
		{
			if((OIL_TANK[2]&0x0f)==0)
			{
				if(OIL_TANK[3]==0 )           //flag_output_tank[1] == 0 || //flag_output_tank[1] != 1))
				{
					//emit right_tank(72);
					net_history(2,0);
					//add_value_controlinfo("2# 油罐   ","设备正常");
					//flag_output_tank[1] = 1;
				}
				else
				{
					if(OIL_TANK[3]==0x88 )           //flag_output_tank[1] == 0 || //flag_output_tank[1] != 2))
					{
						//emit warning_oil_tank(72);//油报警
						net_history(2,1);
						//add_value_controlinfo("2# 油罐   ","检油报警");
						//flag_output_tank[1] = 2;
					}
					if(OIL_TANK[3]==0x90 )           //flag_output_tank[1] == 0 || //flag_output_tank[1] != 3))
					{
						//emit warning_water_tank(72);//水报警
						net_history(2,2);
						//add_value_controlinfo("2# 油罐   ","检水报警");
						//flag_output_tank[1] = 3;
					}
					if(OIL_TANK[3]==0x01 )           //flag_output_tank[1] == 0 || //flag_output_tank[1] != 4))
					{
						//emit warning_sensor_tank(72);//传感器故障
						net_history(2,3);
						//add_value_controlinfo("2# 油罐   ","传感器故障");
						//flag_output_tank[1] = 4;
					}
					if(OIL_TANK[3]==0x04 )           //flag_output_tank[1] == 0 || //flag_output_tank[1] != 5))
					{
						//emit warning_uart_tank(72);//通信故障
						net_history(2,4);
						//add_value_controlinfo("2# 油罐   ","通信故障");
						//flag_output_tank[1] = 5;
					}
				}
			}
		}
		if(count_tank>=3)            //3#
		{
			if((OIL_TANK[4]&0x0f)==0)
			{
				if(OIL_TANK[5]==0 )           //flag_output_tank[2] == 0 || //flag_output_tank[2] != 1))
				{
					//emit right_tank(73);
					net_history(3,0);
					//add_value_controlinfo("3# 油罐   ","设备正常");
					//flag_output_tank[2] = 1;
				}
				else
				{
					if(OIL_TANK[5]==0x88 )           //flag_output_tank[2] == 0 || //flag_output_tank[2] != 2))
					{
						//emit warning_oil_tank(73);//油报警
						net_history(3,1);
						//add_value_controlinfo("3# 油罐   ","检油报警");
						//flag_output_tank[2] = 2;
					}
					if(OIL_TANK[5]==0x90 )           //flag_output_tank[2] == 0 || //flag_output_tank[2] != 3))
					{
						//emit warning_water_tank(73);//水报警
						net_history(3,2);
						//add_value_controlinfo("3# 油罐   ","检水报警");
						//flag_output_tank[2] = 3;
					}
					if(OIL_TANK[5]==0x01 )           //flag_output_tank[2] == 0 || //flag_output_tank[2] != 4))
					{
						//emit warning_sensor_tank(73);//传感器故障
						net_history(3,3);
						//add_value_controlinfo("3# 油罐   ","传感器故障");
						//flag_output_tank[2] = 4;
					}
					if(OIL_TANK[5]==0x04 )           //flag_output_tank[2] == 0 || //flag_output_tank[2] != 5))
					{
						//emit warning_uart_tank(73);//通信故障
						net_history(3,4);
						//add_value_controlinfo("3# 油罐   ","通信故障");
						//flag_output_tank[2] = 5;
					}
				}
			}
		}
		if(count_tank>=4)           //4#
		{
			if((OIL_TANK[6]&0x0f)==0)
			{
				if(OIL_TANK[7]==0 )           //flag_output_tank[3] == 0 || //flag_output_tank[3] != 1))
				{
					//emit right_tank(74);
					net_history(4,0);
					//add_value_controlinfo("4# 油罐   ","设备正常");
					//flag_output_tank[3] = 1;
				}
				else
				{
					if(OIL_TANK[7]==0x88 )           //flag_output_tank[3] == 0 || //flag_output_tank[3] != 2))
					{
						//emit warning_oil_tank(74);//油报警
						net_history(4,1);
						//add_value_controlinfo("4# 油罐   ","检油报警");
						//flag_output_tank[3] = 2;
					}
					if(OIL_TANK[7]==0x90 )           //flag_output_tank[3] == 0 || //flag_output_tank[3] != 3))
					{
						//emit warning_water_tank(74);//水报警
						net_history(4,2);
						//add_value_controlinfo("4# 油罐   ","检水报警");
						//flag_output_tank[3] = 3;
					}
					if(OIL_TANK[7]==0x01 )           //flag_output_tank[3] == 0 || //flag_output_tank[3] != 4))
					{
						//emit warning_sensor_tank(74);//传感器故障
						net_history(4,3);
						//add_value_controlinfo("4# 油罐   ","传感器故障");
						//flag_output_tank[3] = 4;
					}
					if(OIL_TANK[7]==0x04 )           //flag_output_tank[3] == 0 || //flag_output_tank[3] != 5))
					{
						//emit warning_uart_tank(74);//通信故障
						net_history(4,4);
						//add_value_controlinfo("4# 油罐   ","通信故障");
						//flag_output_tank[3] = 5;
					}
				}
			}
		}
		if(count_tank>=5)       //5#
		{
			if((OIL_TANK[8]&0x0f)==0)
			{
				if(OIL_TANK[9]==0 )           //flag_output_tank[4] == 0 || //flag_output_tank[4] != 1))
				{
					//emit right_tank(75);
					net_history(5,0);
					//add_value_controlinfo("5# 油罐   ","设备正常");
					//flag_output_tank[4] = 1;
				}
				else
				{
					if(OIL_TANK[9]==0x88 )           //flag_output_tank[4] == 0 || //flag_output_tank[4] != 2))
					{
						//emit warning_oil_tank(75);//油报警
						net_history(5,1);
						//add_value_controlinfo("5# 油罐   ","检油报警");
						//flag_output_tank[4] = 2;
					}
					if(OIL_TANK[9]==0x90 )           //flag_output_tank[4] == 0 || //flag_output_tank[4] != 3))
					{
						//emit warning_water_tank(75);//水报警
						net_history(5,2);
						//add_value_controlinfo("5# 油罐   ","检水报警");
						//flag_output_tank[4] = 3;
					}
					if(OIL_TANK[9]==0x01 )           //flag_output_tank[4] == 0 || //flag_output_tank[4] != 4))
					{
						//emit warning_sensor_tank(75);//传感器故障
						net_history(5,3);
						//add_value_controlinfo("5# 油罐   ","传感器故障");
						//flag_output_tank[4] = 4;
					}
					if(OIL_TANK[9]==0x04 )           //flag_output_tank[4] == 0 || //flag_output_tank[4] != 5))
					{
						//emit warning_uart_tank(75);//通信故障
						net_history(5,4);
						//add_value_controlinfo("5# 油罐   ","通信故障");
						//flag_output_tank[4] = 5;
					}
				}
			}
		}
		if(count_tank>=6)         //6#
		{
			if((OIL_TANK[10]&0x0f)==0)
			{
				if(OIL_TANK[11]==0 )           //flag_output_tank[5] == 0 || //flag_output_tank[5] != 1))
				{
					//emit right_tank(76);
					net_history(6,0);
					//add_value_controlinfo("6# 油罐   ","设备正常");
					//flag_output_tank[5] = 1;
				}
				else
				{
					if(OIL_TANK[11]==0x88 )           //flag_output_tank[5] == 0 || //flag_output_tank[5] != 2))
					{
						//emit warning_oil_tank(76);//油报警
						net_history(6,1);
						//add_value_controlinfo("6# 油罐   ","检油报警");
						//flag_output_tank[5] = 2;
					}
					if(OIL_TANK[11]==0x90 )           //flag_output_tank[5] == 0 || //flag_output_tank[5] != 3))
					{
						//emit warning_water_tank(76);//水报警
						net_history(6,2);
						//add_value_controlinfo("6# 油罐   ","检水报警");
						//flag_output_tank[5] = 3;
					}
					if(OIL_TANK[11]==0x01 )           //flag_output_tank[5] == 0 || //flag_output_tank[5] != 4))
					{
						//emit warning_sensor_tank(76);//传感器故障
						net_history(6,3);
						//add_value_controlinfo("6# 油罐   ","传感器故障");
						//flag_output_tank[5] = 4;
					}
					if(OIL_TANK[11]==0x04 )           //flag_output_tank[5] == 0 || //flag_output_tank[5] != 5))
					{
						//emit warning_uart_tank(76);//通信故障
						net_history(6,4);
						//add_value_controlinfo("6# 油罐   ","通信故障");
						//flag_output_tank[5] = 5;
					}
				}
			}
		}
		if(count_tank>=7)         //7#
		{
			if((OIL_TANK[12]&0x0f)==0)
			{
				if(OIL_TANK[13]==0 )           //flag_output_tank[6] == 0 || //flag_output_tank[6] != 1))
				{
					//emit right_tank(77);
					net_history(7,0);
					//add_value_controlinfo("7# 油罐   ","设备正常");
					//flag_output_tank[6] = 1;
				}
				else
				{
					if(OIL_TANK[13]==0x88 )           //flag_output_tank[6] == 0 || //flag_output_tank[6] != 2))
					{
						//emit warning_oil_tank(77);//油报警
						net_history(7,1);
						//add_value_controlinfo("7# 油罐   ","检油报警");
						//flag_output_tank[6] = 2;
					}
					if(OIL_TANK[13]==0x90 )           //flag_output_tank[6] == 0 || //flag_output_tank[6] != 3))
					{
						//emit warning_water_tank(77);//水报警
						net_history(7,2);
						//add_value_controlinfo("7# 油罐   ","检水报警");
						//flag_output_tank[6] = 3;
					}
					if(OIL_TANK[13]==0x01 )           //flag_output_tank[6] == 0 || //flag_output_tank[6] != 4))
					{
						//emit warning_sensor_tank(77);//传感器故障
						net_history(7,3);
						//add_value_controlinfo("7# 油罐   ","传感器故障");
						//flag_output_tank[6] = 4;
					}
					if(OIL_TANK[13]==0x04 )           //flag_output_tank[6] == 0 || //flag_output_tank[6] != 5))
					{
						//emit warning_uart_tank(77);//通信故障
						net_history(7,4);
						//add_value_controlinfo("7# 油罐   ","通信故障");
						//flag_output_tank[6] = 5;
					}
				}
			}
		}
		if(count_tank>=8)     //8#
		{
			if((OIL_TANK[14]&0x0f)==0)
			{
				if(OIL_TANK[15]==0 )           //flag_output_tank[7] == 0 || //flag_output_tank[7] != 1))
				{
					//emit right_tank(78);
					net_history(8,0);
					//add_value_controlinfo("8# 油罐   ","设备正常");
					//flag_output_tank[7] = 1;
				}
				else
				{
					if(OIL_TANK[15]==0x88 )           //flag_output_tank[7] == 0 || //flag_output_tank[7] != 2))
					{
						//emit warning_oil_tank(78);//油报警
						net_history(8,1);
						//add_value_controlinfo("8# 油罐   ","检油报警");
						//flag_output_tank[7] = 2;
					}
					if(OIL_TANK[15]==0x90 )           //flag_output_tank[7] == 0 || //flag_output_tank[7] != 3))
					{
						//emit warning_water_tank(78);//水报警
						net_history(8,2);
						//add_value_controlinfo("8# 油罐   ","检水报警");
						//flag_output_tank[7] = 3;
					}
					if(OIL_TANK[15]==0x01 )           //flag_output_tank[7] == 0 || //flag_output_tank[7] != 4))
					{
						//emit warning_sensor_tank(78);//传感器故障
						net_history(8,3);
						//add_value_controlinfo("8# 油罐   ","传感器故障");
						//flag_output_tank[7] = 4;
					}
					if(OIL_TANK[15]==0x04 )           //flag_output_tank[7] == 0 || //flag_output_tank[7] != 5))
					{
						//emit warning_uart_tank(78);//通信故障
						net_history(8,4);
						//add_value_controlinfo("8# 油罐   ","通信故障");
						//flag_output_tank[7] = 5;
					}
				}
			}
		}

	}
	if((OIL_TANK[0]&0xf0)==0x80)      //液媒法
	{

		if(count_tank >= 1)     //1#
		{
			if((OIL_TANK[0]&0x0f)==0)
			{
				if(OIL_TANK[1]==0 )           //flag_output_tank[0] == 0 || //flag_output_tank[0] != 1))
				{
					//emit right_tank(71);
					net_history(1,0);
					//add_value_controlinfo("1# 油罐   ","设备正常");
					//flag_output_tank[0] = 1;
				}
				else
				{
					if(OIL_TANK[1]==0xa0 )           //flag_output_tank[0] == 0 || //flag_output_tank[0] != 2))
					{
						//emit warning_high_tank(71);//高报警
						net_history(1,5);
						//add_value_controlinfo("1# 油罐   ","高液位报警");
						//flag_output_tank[0] = 2;
					}
					if(OIL_TANK[1]==0xc0 )           //flag_output_tank[0] == 0 || //flag_output_tank[0] != 3))
					{
						//emit warning_low_tank(71);//低报警
						net_history(1,6);
						//add_value_controlinfo("1# 油罐   ","低液位报警");
						//flag_output_tank[0] = 3;
					}
					if(OIL_TANK[1]==0x01 )           //flag_output_tank[0] == 0 || //flag_output_tank[0] != 4))
					{
						//emit warning_sensor_tank(71);//传感器故障
						net_history(1,3);
						//add_value_controlinfo("1# 油罐   ","传感器故障");
						//flag_output_tank[0] = 4;
					}
					if(OIL_TANK[1]==0x04 )           //flag_output_tank[1] == 0 || //flag_output_tank[1] != 5))
					{
						//emit warning_uart_tank(71);//通信故障
						net_history(1,4);
						//add_value_controlinfo("1# 油罐   ","通信故障");
						//flag_output_tank[0] = 5;
					}
				}
			}
		}
		if(count_tank >= 2)       //2#
		{
			if((OIL_TANK[2]&0x0f)==0)
			{
				if(OIL_TANK[3]==0 )           //flag_output_tank[1] == 0 || //flag_output_tank[1] != 1))
				{
					//emit right_tank(72);
					net_history(2,0);
					//add_value_controlinfo("2# 油罐   ","设备正常");
					//flag_output_tank[1] = 1;
				}
				else
				{
					if(OIL_TANK[3]==0xa0 )           //flag_output_tank[1] == 0 || //flag_output_tank[1] != 2))
					{
						//emit warning_high_tank(72);//高报警
						net_history(2,5);
						//add_value_controlinfo("2# 油罐   ","高液位报警");
						//flag_output_tank[1] = 2;
					}
					if(OIL_TANK[3]==0xc0 )           //flag_output_tank[1] == 0 || //flag_output_tank[1] != 3))
					{
						//emit warning_low_tank(72);//低报警
						net_history(2,6);
						//add_value_controlinfo("2# 油罐   ","低液位报警");
						//flag_output_tank[1] = 3;
					}
					if(OIL_TANK[3]==0x01 )           //flag_output_tank[1] == 0 || //flag_output_tank[1] != 4))
					{
						//emit warning_sensor_tank(72);//传感器故障
						net_history(2,3);
						//add_value_controlinfo("2# 油罐   ","传感器故障");
						//flag_output_tank[1] = 4;
					}
					if(OIL_TANK[3]==0x04 )           //flag_output_tank[1] == 0 || //flag_output_tank[1] != 5))
					{
						//emit warning_uart_tank(72);//通信故障
						net_history(2,4);
						//add_value_controlinfo("2# 油罐   ","通信故障");
						//flag_output_tank[1] = 5;
					}
				}
			}
		}
		if(count_tank >= 3)      //3#
		{
			if((OIL_TANK[4]&0x0f)==0)
			{
				if(OIL_TANK[5]==0 )           //flag_output_tank[2] == 0 || //flag_output_tank[2] != 1))
				{
					//emit right_tank(73);
					net_history(3,0);
					//add_value_controlinfo("3# 油罐   ","设备正常");
					//flag_output_tank[2] = 1;
				}
				else
				{
					if(OIL_TANK[5]==0xa0 )           //flag_output_tank[2] == 0 || //flag_output_tank[2] != 2))
					{
						//emit warning_high_tank(73);//高报警
						net_history(3,5);
						//add_value_controlinfo("3# 油罐   ","高液位报警");
						//flag_output_tank[2] = 2;
					}
					if(OIL_TANK[5]==0xc0 )           //flag_output_tank[2] == 0 || //flag_output_tank[2] != 3))
					{
						//emit warning_low_tank(73);//低报警
						net_history(3,6);
						//add_value_controlinfo("3# 油罐   ","低液位报警");
						//flag_output_tank[2] = 3;
					}
					if(OIL_TANK[5]==0x01 )           //flag_output_tank[2] == 0 || //flag_output_tank[2] != 4))
					{
						//emit warning_sensor_tank(73);//传感器故障
						net_history(3,3);
						//add_value_controlinfo("3# 油罐   ","传感器故障");
						//flag_output_tank[2] = 4;
					}
					if(OIL_TANK[5]==0x04 )           //flag_output_tank[2] == 0 || //flag_output_tank[2] != 5))
					{
						//emit warning_uart_tank(73);//通信故障
						net_history(3,4);
						//add_value_controlinfo("3# 油罐   ","通信故障");
						//flag_output_tank[2] = 5;
					}
				}
			}
		}
		if(count_tank >= 4)      //4#
		{
			if((OIL_TANK[6]&0x0f)==0)
			{
				if(OIL_TANK[7]==0 )           //flag_output_tank[3] == 0 || //flag_output_tank[3] != 1))
				{
					//emit right_tank(74);
					net_history(4,0);
					//add_value_controlinfo("4# 油罐   ","设备正常");
					//flag_output_tank[3] = 1;
				}
				else
				{
					if(OIL_TANK[7]==0xa0 )           //flag_output_tank[3] == 0 || //flag_output_tank[3] != 2))
					{
						//emit warning_high_tank(74);//高报警
						net_history(4,5);
						//add_value_controlinfo("4# 油罐   ","高液位报警");
						//flag_output_tank[3] = 2;
					}
					if(OIL_TANK[7]==0xc0 )           //flag_output_tank[3] == 0 || //flag_output_tank[3] != 3))
					{
						//emit warning_low_tank(74);//低报警
						net_history(4,6);
						//add_value_controlinfo("4# 油罐   ","低液位报警");
						//flag_output_tank[3] = 3;
					}
					if(OIL_TANK[7]==0x01 )           //flag_output_tank[3] == 0 || //flag_output_tank[3] != 4))
					{
						//emit warning_sensor_tank(74);//传感器故障
						net_history(4,3);
						//add_value_controlinfo("4# 油罐   ","传感器故障");
						//flag_output_tank[3] = 4;
					}
					if(OIL_TANK[7]==0x04 )           //flag_output_tank[3] == 0 || //flag_output_tank[3] != 5))
					{
						//emit warning_uart_tank(74);//通信故障
						net_history(4,4);
						//add_value_controlinfo("4# 油罐   ","通信故障");
						//flag_output_tank[3] = 5;
					}
				}
			}
		}
		if(count_tank >= 5)     //5#
		{
			if((OIL_TANK[8]&0x0f)==0)
			{
				if(OIL_TANK[9]==0 )           //flag_output_tank[4] == 0 || //flag_output_tank[4] != 1))
				{
					//emit right_tank(75);
					net_history(5,0);
					//add_value_controlinfo("5# 油罐   ","设备正常");
					//flag_output_tank[4] = 1;
				}
				else
				{
					if(OIL_TANK[9]==0xa0 )           //flag_output_tank[4] == 0 || //flag_output_tank[4] != 2))
					{
						//emit warning_high_tank(75);//高报警
						net_history(5,5);
						//add_value_controlinfo("5# 油罐   ","高液位报警");
						//flag_output_tank[4] = 2;
					}
					if(OIL_TANK[9]==0xc0 )           //flag_output_tank[4] == 0 || //flag_output_tank[4] != 3))
					{
						//emit warning_low_tank(75);//低报警
						net_history(5,6);
						//add_value_controlinfo("5# 油罐   ","低液位报警");
						//flag_output_tank[4] = 3;
					}
					if(OIL_TANK[9]==0x01 )           //flag_output_tank[4] == 0 || //flag_output_tank[4] != 4))
					{
						//emit warning_sensor_tank(75);//传感器故障
						net_history(5,3);
						//add_value_controlinfo("5# 油罐   ","传感器故障");
						//flag_output_tank[4] = 4;
					}
					if(OIL_TANK[9]==0x04 )           //flag_output_tank[4] == 0 || //flag_output_tank[4] != 5))
					{
						//emit warning_uart_tank(75);//通信故障
						net_history(5,4);
						//add_value_controlinfo("5# 油罐   ","通信故障");
						//flag_output_tank[4] = 5;
					}
				}
			}
		}
		if(count_tank >= 6)     //6#
		{
			if((OIL_TANK[10]&0x0f)==0)
			{
				if(OIL_TANK[11]==0 )           //flag_output_tank[5] == 0 || //flag_output_tank[5] != 1))
				{
					//emit right_tank(76);
					net_history(6,0);
					//add_value_controlinfo("6# 油罐   ","设备正常");
					//flag_output_tank[5] = 1;
				}
				else
				{
					if(OIL_TANK[11]==0xa0 )           //flag_output_tank[5] == 0 || //flag_output_tank[5] != 2))
					{
						//emit warning_high_tank(76);//高报警
						net_history(6,5);
						//add_value_controlinfo("6# 油罐   ","高液位报警");
						//flag_output_tank[5] = 2;
					}
					if(OIL_TANK[11]==0xc0 )           //flag_output_tank[5] == 0 || //flag_output_tank[5] != 3))
					{
						//emit warning_low_tank(76);//低报警
						net_history(6,6);
						//add_value_controlinfo("6# 油罐   ","低液位报警");
						//flag_output_tank[5] = 3;
					}
					if(OIL_TANK[11]==0x01 )           //flag_output_tank[5] == 0 || //flag_output_tank[5] != 4))
					{
						//emit warning_sensor_tank(76);//传感器故障
						net_history(6,3);
						//add_value_controlinfo("6# 油罐   ","传感器故障");
						//flag_output_tank[5] = 4;
					}
					if(OIL_TANK[11]==0x04 )           //flag_output_tank[5] == 0 || //flag_output_tank[5] != 5))
					{
						//emit warning_uart_tank(76);//通信故障
						net_history(6,4);
						//add_value_controlinfo("6# 油罐   ","通信故障");
						//flag_output_tank[5] = 5;
					}
				}
			}
		}
		if(count_tank >= 7)     //7#
		{
			if((OIL_TANK[12]&0x0f)==0)
			{
				if(OIL_TANK[13]==0 )           //flag_output_tank[6] == 0 || //flag_output_tank[6] != 1))
				{
					//emit right_tank(77);
					net_history(7,0);
					//add_value_controlinfo("7# 油罐   ","设备正常");
					//flag_output_tank[6] = 1;
				}
				else
				{
					if(OIL_TANK[13]==0xa0 )           //flag_output_tank[6] == 0 || //flag_output_tank[6] != 2))
					{
						//emit warning_high_tank(77);//高报警
						net_history(7,5);
						//add_value_controlinfo("7# 油罐   ","高液位报警");
						//flag_output_tank[6] = 2;
					}
					if(OIL_TANK[13]==0xc0 )           //flag_output_tank[6] == 0 || //flag_output_tank[6] != 3))
					{
						//emit warning_low_tank(77);//低报警
						net_history(7,6);
						//add_value_controlinfo("7# 油罐   ","低液位报警");
						//flag_output_tank[6] = 3;
					}
					if(OIL_TANK[13]==0x01 )           //flag_output_tank[6] == 0 || //flag_output_tank[6] != 4))
					{
						//emit warning_sensor_tank(77);//传感器故障
						net_history(7,3);
						//add_value_controlinfo("7# 油罐   ","传感器故障");
						//flag_output_tank[6] = 4;
					}
					if(OIL_TANK[13]==0x04 )           //flag_output_tank[6] == 0 || //flag_output_tank[6] != 5))
					{
						//emit warning_uart_tank(77);//通信故障
						net_history(7,4);
						//add_value_controlinfo("7# 油罐   ","通信故障");
						//flag_output_tank[6] = 5;
					}
				}
			}
		}
		if(count_tank >= 8)     //8#
		{
			if((OIL_TANK[14]&0x0f)==0)
			{
				if(OIL_TANK[15]==0 )           //flag_output_tank[7] == 0 || //flag_output_tank[7] != 1))
				{
					//emit right_tank(78);
					net_history(8,0);
					//add_value_controlinfo("8# 油罐   ","设备正常");
					//flag_output_tank[7] = 1;
				}
				else
				{
					if(OIL_TANK[15]==0xa0 )           //flag_output_tank[7] == 0 || //flag_output_tank[7] != 2))
					{
						//emit warning_high_tank(78);//高报警
						net_history(8,5);
						//add_value_controlinfo("8# 油罐   ","高液位报警");
						//flag_output_tank[7] = 2;
					}
					if(OIL_TANK[15]==0xc0 )           //flag_output_tank[7] == 0 || //flag_output_tank[7] != 3))
					{
						//emit warning_low_tank(78);//低报警
						net_history(8,6);
						//add_value_controlinfo("8# 油罐   ","低液位报警");
						//flag_output_tank[7] = 3;
					}
					if(OIL_TANK[15]==0x01 )           //flag_output_tank[7] == 0 || //flag_output_tank[7] != 4))
					{
						//emit warning_sensor_tank(78);//传感器故障
						net_history(8,3);
						//add_value_controlinfo("8# 油罐   ","传感器故障");
						//flag_output_tank[7] = 4;
					}
					if(OIL_TANK[15]==0x04 )           //flag_output_tank[7] == 0 || //flag_output_tank[7] != 5))
					{
						//emit warning_uart_tank(78);//通信故障
						net_history(8,4);
						//add_value_controlinfo("8# 油罐   ","通信故障");
						//flag_output_tank[7] = 5;
					}
				}
			}
		}
	}
	if((OIL_TANK[0]&0xf0)==0x40)      //压力法
	{
		if(count_tank >= 1)     //1#
		{
			if(((OIL_TANK[0] == 0x40)&&(OIL_TANK[1] == 0)) )           //flag_output_tank[0] == 0 || //flag_output_tank[0] != 1)) //1#
			{
				//emit right_tank(71);
				net_history(1,0);
				//add_value_controlinfo("1# 油罐   ","设备正常");
				//flag_output_tank[0] = 1;
			}
			if(((OIL_TANK[0] == 0x41)&&(OIL_TANK[1] == 0x80)) )           //flag_output_tank[0] == 0 || //flag_output_tank[0] != 2))
			{
				//emit warning_pre_tank(71);//预报警
				net_history(1,7);
				//add_value_controlinfo("1# 油罐   ","压力预报警");
				//flag_output_tank[0] = 2;
			}
			if(((OIL_TANK[0] == 0x42)&&(OIL_TANK[1] == 0x80)) )           //flag_output_tank[0] == 0 || //flag_output_tank[0] != 3))
			{
				//emit warning_warn_tank(71);//报警
				net_history(1,6);
				//add_value_controlinfo("1# 油罐   ","压力报警");
				//flag_output_tank[0] = 3;
			}
			if(((OIL_TANK[0] == 0x40)&&(OIL_TANK[1] == 0x01)) )           //flag_output_tank[0] == 0 || //flag_output_tank[0] != 4))
			{
				//emit warning_sensor_tank(71);//传感器故障
				net_history(1,3);
				//add_value_controlinfo("1# 油罐   ","传感器故障");
				//flag_output_tank[0] = 4;
			}
			if(((OIL_TANK[0] == 0x40)&&(OIL_TANK[1] == 0x04)) )           //flag_output_tank[0] == 0 || //flag_output_tank[0] != 5))
			{
				//emit warning_uart_tank(71);//通信故障
				net_history(1,4);
				//add_value_controlinfo("1# 油罐   ","通信故障");
				//flag_output_tank[0] = 5;
			}
		}

		if(count_tank >= 2)     //2#
		{
			if(((OIL_TANK[2] == 0x40)&&(OIL_TANK[3] == 0)) )           //flag_output_tank[1] == 0 || //flag_output_tank[1] != 1)) //2#
			{
				//emit right_tank(72);
				net_history(2,0);
				//add_value_controlinfo("2# 油罐   ","设备正常");
				//flag_output_tank[1] = 1;
			}
			if(((OIL_TANK[2] == 0x41)&&(OIL_TANK[3] == 0x80)) )           //flag_output_tank[1] == 0 || //flag_output_tank[1] != 2))
			{
				//emit warning_pre_tank(72);//预报警
				net_history(2,7);
				//add_value_controlinfo("2# 油罐   ","压力预报警");
				//flag_output_tank[1] = 2;
			}
			if(((OIL_TANK[2] == 0x42)&&(OIL_TANK[3] == 0x80)) )           //flag_output_tank[1] == 0 || //flag_output_tank[1] != 3))
			{
				//emit warning_warn_tank(72);//报警
				net_history(2,8);
				//add_value_controlinfo("2# 油罐   ","压力报警");
				//flag_output_tank[1] = 3;
			}
			if(((OIL_TANK[2] == 0x40)&&(OIL_TANK[3] == 0x01)) )           //flag_output_tank[1] == 0 || //flag_output_tank[1] != 4))
			{
				//emit warning_sensor_tank(72);//传感器故障
				net_history(2,3);
				//add_value_controlinfo("2# 油罐   ","传感器故障");
				//flag_output_tank[1] = 4;
			}
			if(((OIL_TANK[2] == 0x40)&&(OIL_TANK[3] == 0x04)) )           //flag_output_tank[1] == 0 || //flag_output_tank[1] != 5))
			{
				//emit warning_uart_tank(72);//通信故障
				net_history(2,4);
				//add_value_controlinfo("2# 油罐   ","通信故障");
				//flag_output_tank[1] = 5;
			}
		}
		if(count_tank >= 3)    //3#
		{
			if(((OIL_TANK[4] == 0x40)&&(OIL_TANK[5] == 0)) )           //flag_output_tank[2] == 0 || //flag_output_tank[2] != 1)) //3#
			{
				//emit right_tank(73);
				net_history(3,0);
				//add_value_controlinfo("3# 油罐   ","设备正常");
				//flag_output_tank[2] = 1;
			}
			if(((OIL_TANK[4] == 0x41)&&(OIL_TANK[5] == 0x80)) )           //flag_output_tank[2] == 0 || //flag_output_tank[2] != 2))
			{
				//emit warning_pre_tank(73);//预报警
				net_history(3,7);
				//add_value_controlinfo("3# 油罐   ","压力预报警");
				//flag_output_tank[2] = 2;
			}
			if(((OIL_TANK[4] == 0x42)&&(OIL_TANK[5] == 0x80)) )           //flag_output_tank[2] == 0 || //flag_output_tank[2] != 3))
			{
				//emit warning_warn_tank(73);//报警
				net_history(3,8);
				//add_value_controlinfo("3# 油罐   ","压力报警");
				//flag_output_tank[2] = 3;
			}
			if(((OIL_TANK[4] == 0x40)&&(OIL_TANK[5] == 0x01)) )           //flag_output_tank[2] == 0 || //flag_output_tank[2] != 4))
			{
				//emit warning_sensor_tank(73);//传感器故障
				net_history(3,3);
				//add_value_controlinfo("3# 油罐   ","传感器故障");
				//flag_output_tank[2] = 4;
			}
			if(((OIL_TANK[4] == 0x40)&&(OIL_TANK[5] == 0x04)) )           //flag_output_tank[2] == 0 || //flag_output_tank[2] != 5))
			{
				//emit warning_uart_tank(73);//通信故障
				net_history(3,4);
				//add_value_controlinfo("3# 油罐   ","通信故障");
				//flag_output_tank[2] = 5;
			}
		}
		if(count_tank >= 4)    //4#
		{
			if(((OIL_TANK[6] == 0x40)&&(OIL_TANK[7] == 0)) )           //flag_output_tank[3] == 0 || //flag_output_tank[3] != 1)) //4#
			{
				//emit right_tank(74);
				net_history(4,0);
				//add_value_controlinfo("4# 油罐   ","设备正常");
				//flag_output_tank[3] = 1;
			}
			if(((OIL_TANK[6] == 0x41)&&(OIL_TANK[7] == 0x80)) )           //flag_output_tank[3] == 0 || //flag_output_tank[3] != 2))
			{
				//emit warning_pre_tank(74);//预报警
				net_history(4,7);
				//add_value_controlinfo("4# 油罐   ","压力预报警");
				//flag_output_tank[3] = 2;
			}
			if(((OIL_TANK[6] == 0x42)&&(OIL_TANK[7] == 0x80)) )           //flag_output_tank[3] == 0 || //flag_output_tank[3] != 3))
			{
				//emit warning_warn_tank(74);//报警
				net_history(4,8);
				//add_value_controlinfo("4# 油罐   ","压力报警");
				//flag_output_tank[3] = 3;
			}
			if(((OIL_TANK[6] == 0x40)&&(OIL_TANK[7] == 0x01)) )           //flag_output_tank[3] == 0 || //flag_output_tank[3] != 4))
			{
				//emit warning_sensor_tank(74);//传感器故障
				net_history(4,3);
				//add_value_controlinfo("4# 油罐   ","传感器故障");
				//flag_output_tank[3] = 4;
			}
			if(((OIL_TANK[6] == 0x40)&&(OIL_TANK[7] == 0x04)) )           //flag_output_tank[3] == 0 || //flag_output_tank[3] != 5))
			{
				//emit warning_uart_tank(74);//通信故障
				net_history(4,4);
				//add_value_controlinfo("4# 油罐   ","通信故障");
				//flag_output_tank[3] = 5;
			}
		}
		if(count_tank >= 5)    //5#
		{
			if(((OIL_TANK[8] == 0x40)&&(OIL_TANK[9] == 0)) )           //flag_output_tank[4] == 0 || //flag_output_tank[4] != 1)) //5#
			{
				//emit right_tank(75);
				net_history(5,0);
				//add_value_controlinfo("5# 油罐   ","设备正常");
				//flag_output_tank[4] = 1;
			}
			if(((OIL_TANK[8] == 0x41)&&(OIL_TANK[9] == 0x80)) )           //flag_output_tank[4] == 0 || //flag_output_tank[4] != 2))
			{
				//emit warning_pre_tank(75);//预报警
				net_history(5,7);
				//add_value_controlinfo("5# 油罐   ","压力预报警");
				//flag_output_tank[4] = 2;
			}
			if(((OIL_TANK[8] == 0x42)&&(OIL_TANK[9] == 0x80)) )           //flag_output_tank[4] == 0 || //flag_output_tank[4] != 3))
			{
				//emit warning_warn_tank(75);//报警
				net_history(5,8);
				//add_value_controlinfo("5# 油罐   ","压力报警");
				//flag_output_tank[4] = 3;
			}
			if(((OIL_TANK[8] == 0x40)&&(OIL_TANK[9] == 0x01)) )           //flag_output_tank[4] == 0 || //flag_output_tank[4] != 4))
			{
				//emit warning_sensor_tank(75);//传感器故障
				net_history(5,3);
				//add_value_controlinfo("5# 油罐   ","传感器故障");
				//flag_output_tank[4] = 4;
			}
			if(((OIL_TANK[8] == 0x40)&&(OIL_TANK[9] == 0x04)) )           //flag_output_tank[4] == 0 || //flag_output_tank[4] != 5))
			{
				//emit warning_uart_tank(75);//通信故障
				net_history(5,4);
				//add_value_controlinfo("5# 油罐   ","通信故障");
				//flag_output_tank[4] = 5;
			}
		}
		if(count_tank >= 6)    //6#
		{
			if(((OIL_TANK[10] == 0x40)&&(OIL_TANK[11] == 0)) )           //flag_output_tank[5] == 0 || //flag_output_tank[5] != 1)) //6#
			{
				//emit right_tank(76);
				net_history(6,0);
				//add_value_controlinfo("6# 油罐   ","设备正常");
				//flag_output_tank[5] = 1;
			}
			if(((OIL_TANK[10] == 0x41)&&(OIL_TANK[11] == 0x80)) )           //flag_output_tank[5] == 0 || //flag_output_tank[5] != 2))
			{
				//emit warning_pre_tank(76);//预报警
				net_history(6,7);
				//add_value_controlinfo("6# 油罐   ","压力预报警");
				//flag_output_tank[5] = 2;
			}
			if(((OIL_TANK[10] == 0x42)&&(OIL_TANK[11] == 0x80)) )           //flag_output_tank[5] == 0 || //flag_output_tank[5] != 3))
			{
				//emit warning_warn_tank(76);//报警
				net_history(6,8);
				//add_value_controlinfo("6# 油罐   ","压力报警");
				//flag_output_tank[5] = 3;
			}
			if(((OIL_TANK[10] == 0x40)&&(OIL_TANK[11] == 0x01)) )           //flag_output_tank[5] == 0 || //flag_output_tank[5] != 4))
			{
				//emit warning_sensor_tank(76);//传感器故障
				net_history(6,3);
				//add_value_controlinfo("6# 油罐   ","传感器故障");
				//flag_output_tank[5] = 4;
			}
			if(((OIL_TANK[10] == 0x40)&&(OIL_TANK[11] == 0x04)) )           //flag_output_tank[5] == 0 || //flag_output_tank[5] != 5))
			{
				//emit warning_uart_tank(76);//通信故障
				net_history(6,4);
				//add_value_controlinfo("6# 油罐   ","通信故障");
				//flag_output_tank[5] = 5;
			}
		}

		if(count_tank >= 7)    //7#
		{
			if(((OIL_TANK[12] == 0x40)&&(OIL_TANK[13] == 0)) )           //flag_output_tank[6] == 0 || //flag_output_tank[6] != 1)) //7#
			{
				//emit right_tank(77);
				net_history(7,0);
				//add_value_controlinfo("7# 油罐   ","设备正常");
				//flag_output_tank[6] = 1;
			}
			if(((OIL_TANK[12] == 0x41)&&(OIL_TANK[13] == 0x80)) )           //flag_output_tank[6] == 0 || //flag_output_tank[6] != 2))
			{
				//emit warning_pre_tank(77);//预报警
				net_history(7,7);
				//add_value_controlinfo("7# 油罐   ","压力预报警");
				//flag_output_tank[6] = 2;
			}
			if(((OIL_TANK[12] == 0x42)&&(OIL_TANK[13] == 0x80)) )           //flag_output_tank[6] == 0 || //flag_output_tank[6] != 3))
			{
				//emit warning_warn_tank(77);//报警
				net_history(7,8);
				//add_value_controlinfo("7# 油罐   ","压力报警");
				//flag_output_tank[6] = 3;
			}
			if(((OIL_TANK[12] == 0x40)&&(OIL_TANK[13] == 0x01)) )           //flag_output_tank[6] == 0 || //flag_output_tank[6] != 4))
			{
				//emit warning_sensor_tank(77);//传感器故障
				net_history(7,3);
				//add_value_controlinfo("7# 油罐   ","传感器故障");
				//flag_output_tank[6] = 4;
			}
			if(((OIL_TANK[12] == 0x40)&&(OIL_TANK[13] == 0x04)) )           //flag_output_tank[6] == 0 || //flag_output_tank[6] != 5))
			{
				//emit warning_uart_tank(77);//通信故障
				net_history(7,4);
				//add_value_controlinfo("7# 油罐   ","通信故障");
				//flag_output_tank[6] = 5;
			}
		}

		if(count_tank >= 8)    //8#
		{
			if(((OIL_TANK[14] == 0x40)&&(OIL_TANK[15] == 0)) )           //flag_output_tank[7] == 0 || //flag_output_tank[7] != 1)) //7#
			{
				//emit right_tank(78);
				net_history(8,0);
				//add_value_controlinfo("8# 油罐   ","设备正常");
				//flag_output_tank[7] = 1;
			}
			if(((OIL_TANK[14] == 0x41)&&(OIL_TANK[15] == 0x80)) )           //flag_output_tank[7] == 0 || //flag_output_tank[7] != 2))
			{
				//emit warning_pre_tank(78);//预报警
				net_history(8,7);
				//add_value_controlinfo("8# 油罐   ","压力预报警");
				//flag_output_tank[7] = 2;
			}
			if(((OIL_TANK[14] == 0x42)&&(OIL_TANK[15] == 0x80)) )           //flag_output_tank[7] == 0 || //flag_output_tank[7] != 3))
			{
				//emit warning_warn_tank(78);//报警
				net_history(8,8);
				//add_value_controlinfo("8# 油罐   ","压力报警");
				//flag_output_tank[7] = 3;
			}
			if(((OIL_TANK[14] == 0x40)&&(OIL_TANK[15] == 0x01)) )           //flag_output_tank[7] == 0 || //flag_output_tank[7] != 4))
			{
				//emit warning_sensor_tank(78);//传感器故障
				net_history(8,3);
				//add_value_controlinfo("8# 油罐   ","传感器故障");
				//flag_output_tank[7] = 4;
			}
			if(((OIL_TANK[14] == 0x40)&&(OIL_TANK[15] == 0x04)) )           //flag_output_tank[7] == 0 || //flag_output_tank[7] != 5))
			{
				//emit warning_uart_tank(78);//通信故障
				net_history(8,4);
				//add_value_controlinfo("8# 油罐   ","通信故障");
				//flag_output_tank[7] = 5;
			}
		}
	}
	//加油机 dis
	if(count_dispener>=1)       //1#
	{
		if(OIL_DISPENER[0]==0xc0)
		{
			if(OIL_DISPENER[1]==0x00 )           //flag_output_dispener[0] == 0 || //flag_output_dispener[0] != 1))
			{
				//emit right_dispener(81);
				net_history(25,0);
				//add_value_controlinfo("1# 加油机 ","设备正常");
				//flag_output_dispener[0] = 1;
			}
			else
			{
				if(OIL_DISPENER[1]==0x88 )           //flag_output_dispener[0] == 0 || //flag_output_dispener[0] != 2))
				{
					//emit warning_oil_dispener(81);//油报警
					net_history(25,1);
					//add_value_controlinfo("1# 加油机 ","检油报警");
					//flag_output_dispener[0] = 2;
				}
				if(OIL_DISPENER[1]==0x90 )           //flag_output_dispener[0] == 0 || //flag_output_dispener[0] != 3))
				{
					//emit warning_water_dispener(81);//水报警
					net_history(25,2);
					//add_value_controlinfo("1# 加油机 ","检水报警");
					//flag_output_dispener[0] = 3;
				}
				if(OIL_DISPENER[1]==0x01 )           //flag_output_dispener[0] == 0 || //flag_output_dispener[0] != 4))
				{
					//emit warning_sensor_dispener(81);//传感器故障
					net_history(25,3);
					//add_value_controlinfo("1# 加油机 ","传感器故障");
					//flag_output_dispener[0] = 4;
				}
				if(OIL_DISPENER[1]==0x04 )           //flag_output_dispener[0] == 0 || //flag_output_dispener[0] != 5))
				{
					//emit warning_uart_dispener(81);//通信故障
					net_history(25,4);
					//add_value_controlinfo("1# 加油机 ","通信故障");
					//flag_output_dispener[0] = 5;
				}
			}
		}
	}
	if(count_dispener>=2)       //2#
	{
		if(OIL_DISPENER[2]==0xc0)
		{
			if(OIL_DISPENER[3]==0x00 )           //flag_output_dispener[1] == 0 || //flag_output_dispener[1] != 1))
			{
				//emit right_dispener(82);
				net_history(26,0);
				//add_value_controlinfo("2# 加油机 ","设备正常");
				//flag_output_dispener[1] = 1;
			}
			else
			{
				if(OIL_DISPENER[3]==0x88 )           //flag_output_dispener[1] == 0 || //flag_output_dispener[1] != 2))
				{
					//emit warning_oil_dispener(82);//油报警
					net_history(26,1);
					//add_value_controlinfo("2# 加油机 ","检油报警");
					//flag_output_dispener[1] = 2;
				}
				if(OIL_DISPENER[3]==0x90 )           //flag_output_dispener[1] == 0 || //flag_output_dispener[1] != 3))
				{
					//emit warning_water_dispener(82);//水报警
					net_history(26,2);
					//add_value_controlinfo("2# 加油机 ","检水报警");
					//flag_output_dispener[1] = 3;
				}
				if(OIL_DISPENER[3]==0x01 )           //flag_output_dispener[1] == 0 || //flag_output_dispener[1] != 4))
				{
					//emit warning_sensor_dispener(82);//传感器故障
					net_history(26,3);
					//add_value_controlinfo("2# 加油机 ","传感器故障");
					//flag_output_dispener[1] = 4;
				}
				if(OIL_DISPENER[3]==0x04 )           //flag_output_dispener[1] == 0 || //flag_output_dispener[1] != 5))
				{
					//emit warning_uart_dispener(82);//通信故障
					net_history(26,4);
					//add_value_controlinfo("2# 加油机 ","通信故障");
					//flag_output_dispener[1] = 5;
				}
			}
		}
	}
	if(count_dispener>=3)      //3#
	{
		if(OIL_DISPENER[4]==0xc0)
		{

			if(OIL_DISPENER[5]==0x00 )           //flag_output_dispener[2] == 0 || //flag_output_dispener[2] != 1))
			{
				//emit right_dispener(83);
				net_history(27,0);
				//add_value_controlinfo("3# 加油机 ","设备正常");
				//flag_output_dispener[2] = 1;
			}
			else
			{
				if(OIL_DISPENER[5]==0x88 )           //flag_output_dispener[2] == 0 || //flag_output_dispener[2] != 2))
				{
					//emit warning_oil_dispener(83);//油报警
					net_history(27,1);
					//add_value_controlinfo("3# 加油机 ","检油报警");
					//flag_output_dispener[2] = 2;
				}
				if(OIL_DISPENER[5]==0x90 )           //flag_output_dispener[2] == 0 || //flag_output_dispener[2] != 3))
				{
					//emit warning_water_dispener(83);//水报警
					net_history(27,2);
					//add_value_controlinfo("3# 加油机 ","检水报警");
					//flag_output_dispener[2] = 3;
				}
				if(OIL_DISPENER[5]==0x01 )           //flag_output_dispener[2] == 0 || //flag_output_dispener[2] != 4))
				{
					//emit warning_sensor_dispener(83);//传感器故障
					net_history(27,3);
					//add_value_controlinfo("3# 加油机 ","传感器故障");
					//flag_output_dispener[2] = 4;
				}
				if(OIL_DISPENER[5]==0x04 )           //flag_output_dispener[2] == 0 || //flag_output_dispener[2] != 5))
				{
					//emit warning_uart_dispener(83);//通信故障
					net_history(27,4);
					//add_value_controlinfo("3# 加油机 ","通信故障");
					//flag_output_dispener[2] = 5;
				}
			}
		}
	}
	if(count_dispener>=4)      //4#
	{
		if(OIL_DISPENER[6]==0xc0)
		{

			if(OIL_DISPENER[7]==0x00 )           //flag_output_dispener[3] == 0 || //flag_output_dispener[3] != 1))
			{
				//emit right_dispener(84);
				net_history(28,0);
				//add_value_controlinfo("4# 加油机 ","设备正常");
				//flag_output_dispener[3] = 1;
			}
			else
			{
				if(OIL_DISPENER[7]==0x88 )           //flag_output_dispener[3] == 0 || //flag_output_dispener[3] != 2))
				{
					//emit warning_oil_dispener(84);//油报警
					net_history(28,1);
					//add_value_controlinfo("4# 加油机 ","检油报警");
					//flag_output_dispener[3] = 2;
				}
				if(OIL_DISPENER[7]==0x90 )           //flag_output_dispener[3] == 0 || //flag_output_dispener[3] != 3))
				{
					//emit warning_water_dispener(84);//水报警
					net_history(28,2);
					//add_value_controlinfo("4# 加油机 ","检水报警");
					//flag_output_dispener[3] = 3;
				}
				if(OIL_DISPENER[7]==0x01 )           //flag_output_dispener[3] == 0 || //flag_output_dispener[3] != 4))
				{
					//emit warning_sensor_dispener(84);//传感器故障
					net_history(28,3);
					//add_value_controlinfo("4# 加油机 ","传感器故障");
					//flag_output_dispener[3] = 4;
				}
				if(OIL_DISPENER[7]==0x04 )           //flag_output_dispener[3] == 0 || //flag_output_dispener[3] != 5))
				{
					//emit warning_uart_dispener(84);//通信故障
					net_history(28,4);
					//add_value_controlinfo("4# 加油机 ","通信故障");
					//flag_output_dispener[3] = 5;
				}
			}
		}
	}
	if(count_dispener>=5)      //5#
	{
		if(OIL_DISPENER[8]==0xc0)      //5#
		{

			if(OIL_DISPENER[9]==0x00 )           //flag_output_dispener[4] == 0 || //flag_output_dispener[4] != 1))
			{
				//emit right_dispener(85);
				net_history(29,0);
				//add_value_controlinfo("5# 加油机 ","设备正常");
				//flag_output_dispener[4] = 1;
			}
			else
			{
				if(OIL_DISPENER[9]==0x88 )           //flag_output_dispener[4] == 0 || //flag_output_dispener[4] != 2))
				{
					//emit warning_oil_dispener(85);//油报警
					net_history(29,1);
					//add_value_controlinfo("5# 加油机 ","检油报警");
					//flag_output_dispener[4] = 2;
				}
				if(OIL_DISPENER[9]==0x90 )           //flag_output_dispener[4] == 0 || //flag_output_dispener[4] != 3))
				{
					//emit warning_water_dispener(85);//水报警
					net_history(29,2);
					//add_value_controlinfo("5# 加油机 ","检水报警");
					//flag_output_dispener[4] = 3;
				}
				if(OIL_DISPENER[9]==0x01 )           //flag_output_dispener[4] == 0 || //flag_output_dispener[4] != 4))
				{
					//emit warning_sensor_dispener(85);//传感器故障
					net_history(29,3);
					//add_value_controlinfo("5# 加油机 ","传感器故障");
					//flag_output_dispener[4] = 4;
				}
				if(OIL_DISPENER[9]==0x04 )           //flag_output_dispener[4] == 0 || //flag_output_dispener[4] != 5))
				{
					//emit warning_uart_dispener(85);//通信故障
					net_history(29,4);
					//add_value_controlinfo("5# 加油机 ","通信故障");
					//flag_output_dispener[4] = 5;
				}
			}
		}
	}
	if(count_dispener>=6)      //6#
	{
		if(OIL_DISPENER[10]==0xc0)
		{

			if(OIL_DISPENER[11]==0x00 )           //flag_output_dispener[5] == 0 || //flag_output_dispener[5] != 1))
			{
				//emit right_dispener(86);
				net_history(30,0);
				//add_value_controlinfo("6# 加油机 ","设备正常");
				//flag_output_dispener[5] = 1;
			}
			else
			{
				if(OIL_DISPENER[11]==0x88 )           //flag_output_dispener[5] == 0 || //flag_output_dispener[5] != 2))
				{
					//emit warning_oil_dispener(86);//油报警
					net_history(30,1);
					//add_value_controlinfo("6# 加油机 ","检油报警");
					//flag_output_dispener[5] = 2;
				}
				if(OIL_DISPENER[11]==0x90 )           //flag_output_dispener[5] == 0 || //flag_output_dispener[5] != 3))
				{
					//emit warning_water_dispener(86);//水报警
					net_history(30,2);
					//add_value_controlinfo("6# 加油机 ","检水报警");
					//flag_output_dispener[5] = 3;
				}
				if(OIL_DISPENER[11]==0x01 )           //flag_output_dispener[5] == 0 || //flag_output_dispener[5] != 4))
				{
					//emit warning_sensor_dispener(86);//传感器故障
					net_history(30,3);
					//add_value_controlinfo("6# 加油机 ","传感器故障");
					//flag_output_dispener[5] = 4;
				}
				if(OIL_DISPENER[11]==0x04 )           //flag_output_dispener[5] == 0 || //flag_output_dispener[5] != 5))
				{
					//emit warning_uart_dispener(86);//通信故障
					net_history(30,4);
					//add_value_controlinfo("6# 加油机 ","通信故障");
					//flag_output_dispener[5] = 5;
				}
			}
		}
	}
	if(count_dispener>=7)      //7#
	{
		if(OIL_DISPENER[12]==0xc0)
		{

			if(OIL_DISPENER[13]==0x00 )           //flag_output_dispener[6] == 0 || //flag_output_dispener[6] != 1))
			{
				//emit right_dispener(87);
				net_history(31,0);
				//add_value_controlinfo("7# 加油机 ","设备正常");
				//flag_output_dispener[6] = 1;
			}
			else
			{
				if(OIL_DISPENER[13]==0x88 )           //flag_output_dispener[6] == 0 || //flag_output_dispener[6] != 2))
				{
					//emit warning_oil_dispener(87);//油报警
					net_history(31,1);
					//add_value_controlinfo("7# 加油机 ","检油报警");
					//flag_output_dispener[6] = 2;
				}
				if(OIL_DISPENER[13]==0x90 )           //flag_output_dispener[6] == 0 || //flag_output_dispener[6] != 3))
				{
					//emit warning_water_dispener(87);//水报警
					net_history(31,2);
					//add_value_controlinfo("7# 加油机 ","检水报警");
					//flag_output_dispener[6] = 3;
				}
				if(OIL_DISPENER[13]==0x01 )           //flag_output_dispener[6] == 0 || //flag_output_dispener[6] != 4))
				{
					//emit warning_sensor_dispener(87);//传感器故障
					net_history(31,3);
					//add_value_controlinfo("7# 加油机 ","传感器故障");
					//flag_output_dispener[6] = 4;
				}
				if(OIL_DISPENER[13]==0x04 )           //flag_output_dispener[6] == 0 || //flag_output_dispener[6] != 5))
				{
					//emit warning_uart_dispener(87);//通信故障
					net_history(31,4);
					//add_value_controlinfo("7# 加油机 ","通信故障");
					//flag_output_dispener[6] = 5;
				}
			}
		}
	}
	if(count_dispener>=8)      //8#
	{
		if(OIL_DISPENER[14]==0xc0)
		{

			if(OIL_DISPENER[15]==0x00 )           //flag_output_dispener[7] == 0 || //flag_output_dispener[7] != 1))
			{
				//emit right_dispener(88);
				net_history(32,0);
				//add_value_controlinfo("8# 加油机 ","设备正常");
				//flag_output_dispener[7] = 1;
			}
			else
			{
				if(OIL_DISPENER[15]==0x88 )           //flag_output_dispener[7] == 0 || //flag_output_dispener[7] != 2))
				{
					//emit warning_oil_dispener(88);//油报警
					net_history(32,1);
					//add_value_controlinfo("8# 加油机 ","检油报警");
					//flag_output_dispener[7] = 2;
				}
				if(OIL_DISPENER[15]==0x90 )           //flag_output_dispener[7] == 0 || //flag_output_dispener[7] != 3))
				{
					//emit warning_water_dispener(88);//水报警
					net_history(32,2);
					//add_value_controlinfo("8# 加油机 ","检水报警");
					//flag_output_dispener[7] = 3;
				}
				if(OIL_DISPENER[15]==0x01 )           //flag_output_dispener[7] == 0 || //flag_output_dispener[7] != 4))
				{
					//emit warning_sensor_dispener(88);//传感器故障
					net_history(32,3);
					//add_value_controlinfo("8# 加油机 ","传感器故障");
					//flag_output_dispener[7] = 4;
				}
				if(OIL_DISPENER[15]==0x04 )           //flag_output_dispener[7] == 0 || //flag_output_dispener[7] != 5))
				{
					//emit warning_uart_dispener(88);//通信故障
					net_history(32,4);
					//add_value_controlinfo("8# 加油机 ","通信故障");
					//flag_output_dispener[7] = 5;
				}
			}
		}
	}
}


int myserver::net_history(int num,int sta)
{
	QString DataType;
	QString SensorNum;
	QString SensorType;
	QString SensorSta;
	QString SensorData;
	if((num>=1)&&(num<=8))//油罐
	{
		DataType = "0";
		SensorNum = QString::number(num);
	}
	if((num>=9)&&(num<=16))//管线
	{
		DataType = "1";
		SensorNum = QString::number(num-8);
	}
	if((num>=17)&&(num<=24))//人井
	{
		DataType = "3";
		SensorNum = QString::number(num-16);
	}
	if((num>=25)&&(num<=32))//加油机
	{
		DataType = "2";
		SensorNum = QString::number(num-24);
	}

	if(sta == 0) //正常
	{
		SensorSta = "0";
	}
	if(sta == 1) //检油
	{
		SensorSta = "1";
	}
	if(sta == 2) //检水
	{
		SensorSta = "2";
	}
	if(sta == 3) //传感器
	{
		SensorSta = "4";
	}
	if(sta == 4) //通信
	{
		SensorSta = "3";
	}
	if(sta == 5) //高液位
	{
		SensorSta = "5";
	}
	if(sta == 6) //低液位
	{
		SensorSta = "6";
	}
	if(sta == 7) //压力预警
	{
		SensorSta = "7";
	}
	if(sta == 8) //压力报警
	{
		SensorSta = "8";
	}

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

	xielousta(DataType,SensorNum,SensorType,SensorSta,SensorData);
	return 0;
}
