/********************************
 * 自助服务器上传测试
 * TCPclien上传
 * 2020-02-28  马浩
 * ******************************/
#include "myserver_thread.h"
#include <QApplication>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <QSocketNotifier>
#include <semaphore.h>
#include <unistd.h>
#include <qdatetime.h>
#include <QMutex>
#include "pb.h"
#include "pb_common.h"
#include "pb_decode.h"
#include "pb_encode.h"
#include "xielou.pb.h"
#include "database_op.h"
#include <QMutex>
#include "config.h"

#define NoData "NULL";
#define DATA_BUFFER_SIZE 256


int nsockfd_myserver_thread;        //tcp套接字
int sockfd_myserver_thread;            //tcp套接字

QString MyServerIp_pre;
int MyServerPort_pre;
unsigned char Flag_MyServerClient = 0;//1连接成功  0连接失败
char Remote_Ip_myserver[20] = {0};
const char* tcp_sever_ip_myserver = "118.178.180.140";//123.57.63.66
int on_tcp_myserver = 1;
unsigned char Sdbuf_myserver[256] = {0};
unsigned int Tcp_send_count_myserver = 0; //发送字符串长度

QMutex MyServer_Lock;

myserver_thread::myserver_thread(QWidget *parent) :
    QThread(parent)
{

}
void myserver_thread::run()
{
	while(is_runnable)
	{
		if(Flag_MyServerEn == 1)
		{
			tcp_client();
			sleep(2);
		}
		else
		{
			sleep(2);
		}
	}
}
void myserver_thread::stop()
{
	is_runnable = false;
}
void myserver_thread::tcp_client()
{
	MyServerIp_pre = MyServerIp;
	MyServerPort_pre = MyServerPort;

	const QByteArray text = MyServerIp.toLocal8Bit();
	const char* myserver_ip = text.data();

	struct sockaddr_in sever_remote;
	// Get the Socket file descriptor
	if( (sockfd_myserver_thread = socket(AF_INET, SOCK_STREAM, 0)) == -1 )
	{
		printf ("myserver Failed to obtain Socket Despcritor.\n");
		//add_value_netinfo("HuBei TCPClient Failed to obtain Socket Despcritor");
	}
	else
	{
		printf ("myserver TCPClient Obtain Socket Despcritor sucessfully.\n");

	}
	sever_remote.sin_family = AF_INET;           		// Protocol Family 协议族
	sever_remote.sin_port = htons (MyServerPort);         		// Port number 端口号
	sever_remote.sin_addr.s_addr  = inet_addr(myserver_ip);  	// 填写远程地址
	memset (sever_remote.sin_zero,0,8);          		// Flush the rest of struct 刷新struct的其余部分

	//端口重用
	if((setsockopt(sockfd_myserver_thread,SOL_SOCKET,SO_REUSEADDR,&on_tcp_myserver,sizeof(on_tcp_myserver)))<0)
	{
		perror("TCPClient setsockopt failed");
		exit(1);
	}

	bool  bDontLinger = FALSE;
	setsockopt(sockfd_myserver_thread,SOL_SOCKET,SO_LINGER,(const char*)&bDontLinger,sizeof(bool));//不要因为数据未发送就阻塞关闭操作
	setsockopt(sockfd_myserver_thread, IPPROTO_TCP, TCP_NODELAY,&on_tcp_myserver,sizeof(on_tcp_myserver));//立即发送，不沾包
	//int nNetTimeout = 800;//超时时长
	struct timeval timeout = {1,0}; //设置接收超时 1秒？第一个参数秒，第二个微秒
	setsockopt(sockfd_myserver_thread,SOL_SOCKET,SO_RCVTIMEO,(char *)&timeout,sizeof(struct timeval));//设置为非阻塞模式
	//主动连接目标,连接不上貌似会卡在这，10秒再连一次  非阻塞模式就不会卡在这了
	if (( nsockfd_myserver_thread = ::connect(sockfd_myserver_thread,(struct sockaddr *)&sever_remote,sizeof(struct sockaddr))) < 0)
	{
		printf ("myserver TCPClient Failed to Client Port %d.\n", MyServerPort);
		//return (0);

		Flag_MyServerClient = 0;
		close(nsockfd_myserver_thread);
		close(sockfd_myserver_thread);
		add_value_netinfo("MyServer Client Failed");
	}
	else
	{
		printf ("myserver TCPClient Client the Port %d sucessfully.\n", MyServerPort);
		add_value_netinfo("MyServer TCPClient Client sucessfully");
		Flag_MyServerClient = 1;//连接成功
		client_keep_ali(sockfd_myserver_thread);//tcp保活
	}
	sleep(2);
	while(1)
	{
		if(Flag_MyServerClient == 0)
		{
			sleep(10);
			break;
		}
		else
		{
			if(Flag_MyServerEn == 0)
			{
				close(nsockfd_myserver_thread);
				close(sockfd_myserver_thread);
				qDebug()<<"MyServer shut down!!";
			}
			if((MyServerIp_pre != MyServerIp)||(MyServerPort_pre != MyServerPort))
			{
				MyServerIp_pre = MyServerIp;
				MyServerPort_pre = MyServerPort;
				close(nsockfd_myserver_thread);
				close(sockfd_myserver_thread);
				qDebug()<<"MyServer config changed!!";
				break;
			}
			else
			{   int stationid = rand()%5;
				if(stationid == 0){MyStationId = "123456789";}
				if(stationid == 1){MyStationId = "987654321";}
				if(stationid == 2){MyStationId = "234567891";}
				if(stationid == 3){MyStationId = "345678912";}
				if(stationid == 4){MyStationId = "456789123";}
				QString data_type = QString::number(rand()%4);
				QString senser_type = QString::number(rand()%4);
				QString senser_num = QString::number(rand()%13);
				QString senser_sta = QString::number(rand()%9);
				QString senser_data = QString::number(rand()%101);

				 send_xielou(data_type,senser_num,senser_type,senser_sta,senser_data);
				 msleep(1000);
				 send_reoilgas();
				 msleep(1000);
				 send_settingdata(senser_num,senser_type,senser_num,senser_num,senser_num,senser_num,"1;2","4;3;4;5;6","0","0","3;7;8;9","1;10","8;11;12;13;14;15;16;17;18",
				                  "0","0","0","0","0","1","0","1","1","1","0","0","0","0");
				 msleep(1000);
			}
			msleep(5);
		}
	}
	close(nsockfd_myserver_thread);
	close(sockfd_myserver_thread);
	sleep(10);
}
void myserver_thread::senddata(unsigned char* send_data,int data_length)
{
	MyServer_Lock.lock();//上锁
	int num_send = 0;
	Tcp_send_count_myserver = data_length;
	memset(Sdbuf_myserver, 0, DATA_BUFFER_SIZE);
	for(int i = 0;i<data_length;i++)
	{
		Sdbuf_myserver[i] = send_data[i];
		printf("%02x ",Sdbuf_myserver[i]);
	}
	printf("\n");

	if((num_send = ::send(sockfd_myserver_thread,Sdbuf_myserver, Tcp_send_count_myserver, 0)) == -1)
	{
		printf("ERROR: Failed to sent string.\n");
		Flag_MyServerClient = 0;//发送失败需要重连
	}
	else
	{
		printf("myserver TcpClient send success %d.\n",Tcp_send_count_myserver);
	}
	MyServer_Lock.unlock();//解锁
}

//  tcp长连接保活
void myserver_thread::client_keep_ali(int sockfd)
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

/*
 * 给自己服务器发送泄漏数据
 * 全部数据类型为Qstring
 * StationID		（油站ID，编码由省份代码+市区代码+油站代码组成）
 * DateTime		（数据上传日期）
 * DataType		（上传数据类型， 0油罐、1管线、2加油机、3防渗池）
 * SensorNum     （传感器编号1~12）
 * SensorType		（传感器类型， 0传感器法、1液媒法、2压力法、3其他方法）
 * SensorSta		（传感器状态：  0 设备正常		存在类型0、1、2、3
*                                 1 检油报警		存在类型0
*                                 2 检水报警		存在类型0
*                                 3 通信故障		存在类型0、1、2、3
*                                 4 传感器故障	存在类型0、1、2、3
*                                 5 高液位报警	存在类型1
*                                 6 低液位报警	存在类型1
*                                 7 压力预警		存在类型2
*                                 8 压力报警		存在类型2			）
* SensorDate		（传感器数据，仅当传感器类型为压力法时，有压力数据，如88.8Kpa，其他情况为NULL）
 * */
void myserver_thread::send_xielou(QString data_type,QString senser_num,QString sensor_type,QString sensor_sta,QString sensor_data)
{
	unsigned char buffer[DATA_BUFFER_SIZE];
	int lenght = 0;
	memset(buffer, 0, DATA_BUFFER_SIZE);
	QDateTime date_time = QDateTime::currentDateTime();
	QString send_date = date_time.toString("yyyyMMddhhmmss");
	const QByteArray text = MyStationId.toLocal8Bit();
	const char *send_staid = text.data();
	const QByteArray text2 = send_date.toLocal8Bit();
	const char *send_datetime = text2.data();
	//Initialize the UserInformation structure;
	MessageInfo send_mess = MessageInfo_init_zero;
	send_mess.message_tpye = MessageInfo_MessageType_OilTankMonitorType;
	send_mess.which_messageBody = MessageInfo_oilTankMonitor_tag;
	// Initialize buffer
	strcpy(send_mess.messageBody.oilTankMonitor.StationID, send_staid);
	strcpy(send_mess.messageBody.oilTankMonitor.DateTime, send_datetime);
	send_mess.messageBody.oilTankMonitor.DataType = data_type.toInt();
	send_mess.messageBody.oilTankMonitor.SensorNum = senser_num.toInt();
	send_mess.messageBody.oilTankMonitor.SensorType = sensor_type.toInt();
	send_mess.messageBody.oilTankMonitor.SensorSta = sensor_sta.toInt();
	send_mess.messageBody.oilTankMonitor.SensorDate = sensor_data.toInt();
	// encode userInfo data
	pb_ostream_t enc_stream;
	enc_stream = pb_ostream_from_buffer(buffer, DATA_BUFFER_SIZE);
	if (!pb_encode(&enc_stream, MessageInfo_fields, &send_mess))
	{
		//encode error happened
		printf("pb encode XieLou error in %s [%s]\n", __func__,PB_GET_ERROR(&enc_stream));
	}
	lenght = enc_stream.bytes_written;
	printf("ProtoBuf Encode XieLou %d Byte!!\n",lenght);
	senddata(buffer,lenght);
}
void myserver_thread::send_reoilgas()
{
	unsigned char buffer[DATA_BUFFER_SIZE];
	int lenght = 0;
	memset(buffer, 0, DATA_BUFFER_SIZE);
	QDateTime date_time = QDateTime::currentDateTime();
	QString send_date = date_time.toString("yyyyMMddhhmmss");
	const QByteArray text = MyStationId.toLocal8Bit();
	const char *send_staid = text.data();
	const QByteArray text2 = send_date.toLocal8Bit();
	const char *send_datetime = text2.data();
	//Initialize the UserInformation structure;
	MessageInfo send_mess = MessageInfo_init_zero;
	send_mess.message_tpye = MessageInfo_MessageType_ReOilGasType;
	send_mess.which_messageBody = MessageInfo_reOilGas_tag;
	// Initialize buffer
	strcpy(send_mess.messageBody.reOilGas.StationID, send_staid);
	strcpy(send_mess.messageBody.reOilGas.DateTime, send_datetime);
	// encode userInfo data
	pb_ostream_t enc_stream;
	enc_stream = pb_ostream_from_buffer(buffer, DATA_BUFFER_SIZE);
	if (!pb_encode(&enc_stream, MessageInfo_fields, &send_mess))
	{
		//encode error happened
		printf("pb encode XieLou error in %s [%s]\n", __func__,PB_GET_ERROR(&enc_stream));
	}
	lenght = enc_stream.bytes_written;
	printf("ProtoBuf Encode XieLou %d Byte!!\n",lenght);
	senddata(buffer,lenght);
}
/*
 * 给自己的服务器发送设置数据
 * StationID			（油站ID，编码由省份代码+市区代码+油站代码组成）
 *DateTime			（数据上传日期）
//泄漏部分
 *OilTankNum			（油罐传感器数量1~12，十进制）
 *OilTankType			（油罐传感器类型：0传感器法、1液媒法、2压力法、3其他方法）
 *PipeNum			（管线传感器数量1~12，十进制）
 *DispenerNum		（加油机传感器数量1~12，十进制）
 *Basin				（防渗池传感器数量1~12，十进制）
//在线监测部分
 *AmountDispenerNum	（加油机数量1~12，十进制）
 *GunNum1			（1号加油机加油枪数0~8，以及映射枪号）
 *GunNum2			（2号加油机加油枪数0~8，以及映射枪号）
 *GunNum3			（3号加油机加油枪数0~8，以及映射枪号）
 *GunNum4			（4号加油机加油枪数0~8，以及映射枪号）
 *GunNum5			（5号加油机加油枪数0~8，以及映射枪号）
 *GunNum6			（6号加油机加油枪数0~8，以及映射枪号）
 *GunNum7			（7号加油机加油枪数0~8，以及映射枪号）
 *GunNum8			（8号加油机加油枪数0~8，以及映射枪号）
 *GunNum9			（9号加油机加油枪数0~8，以及映射枪号）
 *GunNum10			（10号加油机加油枪数0~8，以及映射枪号）
 *GunNum11			（11号加油机加油枪数0~8，以及映射枪号）
 *GunNum12			（12号加油机加油枪数0~8，以及映射枪号）
 *PipPreEn			（管线压力传感器使能，1：开启 0；未开启或未配置）
 *TankPreEN			（油罐压力传感器是能，1：开启 0：未开启或未配置）
 *HclFgaEn			（后处理装置油气浓度传感器，1：开启 0：未开启或未配置）
//安全防护部分
 *FgaNum				（可燃气体浓度传感器数目：0~8，显示编号从1开始顺序排列即可）
 *RadarEn				（雷达开启状态，1：开启 0：未开启或未配置）
 *CrashNum			（防撞柱传感器数目，0~32，显示编号从1开始顺序排列即可）
 *预留设置部分
 *Reserve1			（预留设置信息1）
 *Reserve2			（预留设置信息2）
 *Reserve3			（预留设置信息3）
 *
 * */
void myserver_thread::send_settingdata(QString OilTankNum,QString OilTankType,QString PipeNum,QString DispenerNum,QString BasinNum,
                      QString AmountDispenerNum,QString GunNum1,QString GunNum2,QString GunNum3,QString GunNum4,QString GunNum5,QString GunNum6,
                      QString GunNum7,QString GunNum8,QString GunNum9,QString GunNum10,QString GunNum11,QString GunNum12,
                      QString PipPreEn,QString TankPreEN,QString HclFgaEn,
                      QString FgaNum,	QString RadarEn,QString CrashNum,QString Reserve1, QString Reserve2,QString Reserve3 )
{
	unsigned char buffer[DATA_BUFFER_SIZE];
	int lenght = 0;
	memset(buffer, 0, DATA_BUFFER_SIZE);
	QDateTime date_time = QDateTime::currentDateTime();
	QString send_date = date_time.toString("yyyyMMddhhmmss");
	const QByteArray text = MyStationId.toLocal8Bit();
	const char *send_staid = text.data();
	const QByteArray text2 = send_date.toLocal8Bit();
	const char *send_datetime = text2.data();

	const QByteArray gun1 = GunNum1.toLocal8Bit();
	const char *send_gun1 = gun1.data();
	const QByteArray gun2 = GunNum2.toLocal8Bit();
	const char *send_gun2 = gun2.data();
	const QByteArray gun3 = GunNum3.toLocal8Bit();
	const char *send_gun3 = gun3.data();
	const QByteArray gun4 = GunNum4.toLocal8Bit();
	const char *send_gun4 = gun4.data();
	const QByteArray gun5 = GunNum5.toLocal8Bit();
	const char *send_gun5 = gun5.data();
	const QByteArray gun6 = GunNum6.toLocal8Bit();
	const char *send_gun6 = gun6.data();
	const QByteArray gun7 = GunNum7.toLocal8Bit();
	const char *send_gun7 = gun7.data();
	const QByteArray gun8 = GunNum8.toLocal8Bit();
	const char *send_gun8 = gun8.data();
	const QByteArray gun9 = GunNum9.toLocal8Bit();
	const char *send_gun9 = gun9.data();
	const QByteArray gun10 = GunNum10.toLocal8Bit();
	const char *send_gun10 = gun10.data();
	const QByteArray gun11 = GunNum11.toLocal8Bit();
	const char *send_gun11 = gun11.data();
	const QByteArray gun12 = GunNum12.toLocal8Bit();
	const char *send_gun12 = gun12.data();
	//Initialize the UserInformation structure;
	MessageInfo send_mess = MessageInfo_init_zero;
	send_mess.message_tpye = MessageInfo_MessageType_SettingDataType;
	send_mess.which_messageBody = MessageInfo_settingData_tag;
	// Initialize buffer
	strcpy(send_mess.messageBody.settingData.StationID, send_staid);
	strcpy(send_mess.messageBody.settingData.DateTime, send_datetime);
	//泄漏部分
	send_mess.messageBody.settingData.OilTankNum = OilTankNum.toInt();
	send_mess.messageBody.settingData.OilTankType = OilTankType.toInt();
	send_mess.messageBody.settingData.PipeNum = PipeNum.toInt();
	send_mess.messageBody.settingData.DispenerNum = DispenerNum.toInt();
	send_mess.messageBody.settingData.BasinNum = BasinNum.toInt();
	//油气回收部分
	send_mess.messageBody.settingData.AmountDispenerNum = AmountDispenerNum.toInt();
	strcpy(send_mess.messageBody.settingData.GunNum1, send_gun1);
	strcpy(send_mess.messageBody.settingData.GunNum2, send_gun2);
	strcpy(send_mess.messageBody.settingData.GunNum3, send_gun3);
	strcpy(send_mess.messageBody.settingData.GunNum4, send_gun4);
	strcpy(send_mess.messageBody.settingData.GunNum5, send_gun5);
	strcpy(send_mess.messageBody.settingData.GunNum6, send_gun6);
	strcpy(send_mess.messageBody.settingData.GunNum7, send_gun7);
	strcpy(send_mess.messageBody.settingData.GunNum8, send_gun8);
	strcpy(send_mess.messageBody.settingData.GunNum9, send_gun9);
	strcpy(send_mess.messageBody.settingData.GunNum10, send_gun10);
	strcpy(send_mess.messageBody.settingData.GunNum11, send_gun11);
	strcpy(send_mess.messageBody.settingData.GunNum12, send_gun12);
	send_mess.messageBody.settingData.PipPreEn = PipPreEn.toInt();
	send_mess.messageBody.settingData.TankPreEN = TankPreEN.toInt();
	send_mess.messageBody.settingData.HclFgaEn = HclFgaEn.toInt();
	//安全防护部分
	send_mess.messageBody.settingData.FgaNum = FgaNum.toInt();
	send_mess.messageBody.settingData.RadarEn = RadarEn.toInt();
	send_mess.messageBody.settingData.CrashNum = CrashNum.toInt();
	//预留部分
	send_mess.messageBody.settingData.Reserve1 = Reserve1.toInt();
	send_mess.messageBody.settingData.Reserve2 = Reserve2.toInt();
	send_mess.messageBody.settingData.Reserve3 = Reserve3.toInt();

	// encode userInfo data
	pb_ostream_t enc_stream;
	enc_stream = pb_ostream_from_buffer(buffer, DATA_BUFFER_SIZE);
	if (!pb_encode(&enc_stream, MessageInfo_fields, &send_mess))
	{
		//encode error happened
		printf("pb encode XieLou error in %s [%s]\n", __func__,PB_GET_ERROR(&enc_stream));
	}
	lenght = enc_stream.bytes_written;
	printf("ProtoBuf Encode XieLou %d Byte!!\n",lenght);
	senddata(buffer,lenght);
}



//示例打包程序，弃用可作参考
int myserver_thread::pack_user_data(uint8_t *buffer)
{
	//Initialize the UserInformation structure;
	MessageInfo meminfo = MessageInfo_init_zero;
	meminfo.message_tpye = MessageInfo_MessageType_OilTankMonitorType;
	meminfo.which_messageBody = MessageInfo_oilTankMonitor_tag;

	// Initialize buffer
	memset(buffer, 0, DATA_BUFFER_SIZE);

	strcpy(meminfo.messageBody.oilTankMonitor.StationID, "1234654");
	strcpy(meminfo.messageBody.oilTankMonitor.DateTime, "20200228101715");

	meminfo.messageBody.oilTankMonitor.DataType = 2;
	meminfo.messageBody.oilTankMonitor.SensorNum = 1;
	meminfo.messageBody.oilTankMonitor.SensorType = 2;
	meminfo.messageBody.oilTankMonitor.SensorSta = 3;
	meminfo.messageBody.oilTankMonitor.SensorDate = 4.7;

	// encode userInfo data
	pb_ostream_t enc_stream;
	enc_stream = pb_ostream_from_buffer(buffer, DATA_BUFFER_SIZE);
	if (!pb_encode(&enc_stream, MessageInfo_fields, &meminfo))
	{
		//encode error happened
		printf("pb encode error in %s [%s]\n", __func__,PB_GET_ERROR(&enc_stream));
		return -1;
	}

	return enc_stream.bytes_written;
}
//示例解包程序，弃用可作参考
int myserver_thread::unpack_user_data(const uint8_t *buffer, size_t len)
{
	//OilTankMonitor userInfo;
	MessageInfo mess;

	// decode userInfo data
	pb_istream_t dec_stream;
	dec_stream = pb_istream_from_buffer(buffer, len);
	if (!pb_decode(&dec_stream, MessageInfo_fields, &mess))
	{
		printf("pb decode error in %s\n", __func__);
		return -1;
	}

	printf("Unpack: %s %s %d  %d %d %d %f\n", mess.messageBody.oilTankMonitor.StationID,
	       mess.messageBody.oilTankMonitor.DateTime,
	       (int)mess.messageBody.oilTankMonitor.DataType,
	       (int)mess.messageBody.oilTankMonitor.SensorNum,
	       (int)mess.messageBody.oilTankMonitor.SensorType,
	       (int)mess.messageBody.oilTankMonitor.SensorSta,
	       (float)mess.messageBody.oilTankMonitor.SensorDate);

	return 0;
}
