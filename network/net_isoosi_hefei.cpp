/********************************
 * 合肥油气回收数据上传
 * Tcpclient方式上传 ASCII码上传
 * ip 220.178.73.140:5003
 * 2020-07-01  马浩
 * ******************************/
#include "net_isoosi_hefei.h"
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
#include <QMainWindow>
#include <QTcpServer>
#include <QHostAddress>


QMutex wait_send_over_hefei;
int nsockfd_tcp_hefei;        //tcp套接字
int sockfd_hefei;            //tcp套接字
int PORT_TCP_CLIENT_HEFEI = 0;
int on_tcp_hefei = 1;
unsigned char Flag_TcpClient_Success_HeFei = 0;//tcpclient连接成功
unsigned char Flag_FirstClient_HeFei = 2;//判断是不是第一次连接，做历史记录用
QString urlport_pre_hefei = "";
QString urlip_pre_hefei = "";
unsigned int Tcp_send_count_hefei = 0;
unsigned char flag_send_succes_hefei = 0;//发送成功置1，记录一次
char Sdbuf_hefei[10240] = {0};
char isoosi_revbuf_hefei[128] = {0};
int num_recv_hefei;
unsigned char Flag_TcpClient_SendOver_HeFei = 0;

net_isoosi_hefei::net_isoosi_hefei(QWidget *parent):
	QThread(parent)
{

}
void net_isoosi_hefei::run()
{
	urlport_pre_hefei = IsoOis_UrlPort;
	urlip_pre_hefei = IsoOis_UrlIp;
	while (is_runnable)
	{
		if(Flag_Network_Send_Version == 7)//只有需要该协议的时候才运行tcpclient
		{
			if(net_state == 0)//如果网线插进去了
			{
				tcp_client();
			}
		}
		//跳出之后判断是断线还是ip地址变了
		if((urlip_pre_hefei!=IsoOis_UrlIp)||(urlport_pre_hefei!=IsoOis_UrlPort))
		{
			sleep(1);
			urlport_pre_hefei = IsoOis_UrlPort;
			urlip_pre_hefei = IsoOis_UrlIp;
			qDebug()<<"HeFei IP changed! So sleep 1!";
		}
		else
		{
			sleep(25);
		}
	}
}
void net_isoosi_hefei::stop()
{
	is_runnable = false;
}
void net_isoosi_hefei::tcp_client()
{
	const QByteArray text = IsoOis_UrlIp.toLocal8Bit();
	const char *tcp_sever_ip = text.data();
	PORT_TCP_CLIENT_HEFEI = IsoOis_UrlPort.toInt();

	struct sockaddr_in sever_remote;
	// Get the Socket file descriptor
	if( (sockfd_hefei = socket(AF_INET, SOCK_STREAM, 0)) == -1 )
	{
		printf ("HeFei TCPClient Failed to obtain Socket Despcritor.\n");
		//add_value_netinfo("TCPClient Failed to obtain Socket Despcritor");
		//return 0;
	}
	else
	{
		printf ("HeFei TCPClient Obtain Socket Despcritor sucessfully.\n");
		//add_value_netinfo("TCPClient Obtain Socket Despcritor sucessfully");
		//history_net_write("Tcp_Obtain");
	}
	sever_remote.sin_family = AF_INET;           		// Protocol Family 协议族
	sever_remote.sin_port = htons (PORT_TCP_CLIENT_HEFEI);         		// Port number 端口号
	sever_remote.sin_addr.s_addr  = inet_addr(tcp_sever_ip);  	// 填写远程地址
	memset (sever_remote.sin_zero,0,8);          		// Flush the rest of struct 刷新struct的其余部分

	//端口重用
	if((setsockopt(sockfd_hefei,SOL_SOCKET,SO_REUSEADDR,&on_tcp_hefei,sizeof(on_tcp_hefei)))<0)
	{
		perror("TCPClient setsockopt failed");
		exit(1);
	}

	bool  bDontLinger = FALSE;
	setsockopt(sockfd_hefei,SOL_SOCKET,SO_LINGER,(const char*)&bDontLinger,sizeof(bool));//不要因为数据未发送就阻塞关闭操作
	setsockopt(sockfd_hefei, IPPROTO_TCP, TCP_NODELAY,&on_tcp_hefei,sizeof(on_tcp_hefei));//立即发送，不沾包
	//int nNetTimeout = 800;//超时时长
	struct timeval timeout = {0,500000}; //设置接收超时 1秒？ 第一个参数秒，第二个微秒
	setsockopt(sockfd_hefei,SOL_SOCKET,SO_RCVTIMEO,(char *)&timeout,sizeof(struct timeval));//设置为非阻塞模式
	//主动连接目标,连接不上貌似会卡在这，10秒再连一次
	if (( nsockfd_tcp_hefei = ::connect(sockfd_hefei,(struct sockaddr *)&sever_remote,sizeof(struct sockaddr))) < 0)
	{
		printf ("HeFei TCPClient Failed to Client Port %d.\n", PORT_TCP_CLIENT_HEFEI);
		//return (0);
		Flag_TcpClient_Success_HeFei = 0;//连接失败
		close(nsockfd_tcp_hefei);
		close(sockfd_hefei);
		if(Flag_FirstClient_HeFei != 0)
		{
			add_value_netinfo("HeFei TCPClient Client the Port 5003 Failed");
			Flag_FirstClient_HeFei = 0;
		}

	}
	else
	{
		printf ("HeFei TCPClient Client the Port %d sucessfully.\n", PORT_TCP_CLIENT_HEFEI);
		Flag_TcpClient_Success_HeFei = 1;//连接成功
		client_keep_ali(sockfd_hefei);//tcp保活

		if(Flag_FirstClient_HeFei != 1)
		{
			add_value_netinfo("HeFei TCPClient Client the Port 5003 sucessfully");
			Flag_FirstClient_HeFei = 1;
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
		if(Flag_TcpClient_Success_HeFei == 0)
		{
			printf ("wait TCPClient  Client \n");
			close(nsockfd_tcp_hefei);
			close(sockfd_hefei);
			sleep(5);
			break;
		}
		else
		{
			if((urlip_pre_hefei!=IsoOis_UrlIp)||(urlport_pre_hefei!=IsoOis_UrlPort))
			{
				qDebug()<<"IP changed! So break!";
				break;
			}
			//qDebug()<<"ready to send!";
		}
		//如果不要该协议则退出
		if(Flag_Network_Send_Version != 7)
		{
			close(nsockfd_tcp_hefei);
			close(sockfd_hefei);
			break;
		}
		sleep(1);
	}
	//qDebug()<<"chagged xieyi !!!@@@@######";
	close(nsockfd_tcp_hefei);
	close(sockfd_hefei);
	sleep(1);
}
//要发送的数据
void net_isoosi_hefei::send_tcpclient_data(QString data)
{
	qDebug()<<data;
	unsigned int if_send = 0;
	unsigned int sendfail_num = 0;
	if(Flag_TcpClient_Success_HeFei == 1)
	{
		wait_send_over_hefei.lock();//上锁
		//记得加锁
		QByteArray byte_send = data.toUtf8();
		Tcp_send_count_hefei = data.length();
		memset(Sdbuf_hefei,0,sizeof(char)*10240);//清零数组
		for(unsigned int i = 0;i < Tcp_send_count_hefei;i++)
		{
			Sdbuf_hefei[i] = byte_send[i];
		}
		if(Tcp_send_count_hefei != 0)
		{
			while(!if_send)
			{
				int num_send = 0;
				if((num_send = ::send(sockfd_hefei,Sdbuf_hefei, Tcp_send_count_hefei, 0)) == -1)
				{
					printf("ERROR: Failed to sent string.\n");
					if(flag_send_succes_hefei == 1)
					{
						add_value_netinfo("TCP 数据发送失败");
						Flag_Ifsend = 0;//在这里把全局变量置0，之后时间到了要再重新发送零点的信息
					}
					//sendfail_num++;//发送失败再发一次
					if_send = 1; //发送失败 端口有问题  直接退出
					sendfail_num = 0;
					Tcp_send_count_hefei = 0;
					Flag_TcpClient_Success_HeFei = 0;//连接失败 //重新连接
					flag_send_succes_hefei = 0;
				}
				else
				{
					msleep(50);

					qDebug()<<"HeFei Tcp send success"<<Tcp_send_count_hefei;
					if(flag_send_succes_hefei == 0)
					{
						add_value_netinfo("TCP 数据发送成功");
						flag_send_succes_hefei =1;
					}
					if_send = 1;
					Tcp_send_count_hefei = 0;
				}

			}
		}
		else
		{
			qDebug()<<"hefei isoosi semd data is kong!!";
		}
		wait_send_over_hefei.unlock();//解锁
	}
	else
	{
		qDebug()<< "tcpclient is not connected!!";
	}
}
/********发送环境信息*********
 * YGYL 油罐压力 pa 小数后一位
 * YZYL 液阻压力 pa 小数后一位
 * YQWD 油气温度 ℃ 小数后一位
 * *************************/
void net_isoosi_hefei::send_surround_message(QString YGYL,QString YZYL,QString YQWD)
{
	 QString dataSegment = "";
	 QString MN = "34012131UZAL01";
	 QString PW = "123456";
	 MN = IsoOis_MN;
	 PW = IsoOis_PW;

	 QStringList contaminantLists;
	 contaminantLists.append("Wq1");//油罐压力
	 contaminantLists.append("Wq2");//液阻压力
	 contaminantLists.append("Wq3");//油气温度
	 QStringList dataNums;
	 QString wq1 = YGYL;
	 QString wq2 = YZYL;
	 QString wq3 = YQWD;
	 dataNums.append(wq1);//油罐压力
	 dataNums.append(wq2);//液阻压力
	 dataNums.append(wq3);//油气温度
	 //构建数据区信息
	 QString dataArea = buildRealtimeData(contaminantLists,"Rtd",dataNums);
	 //构建数据段信息
	 dataSegment = buildUploadDataSegment("31","2011",MN,PW,dataArea);

	//构建通用完整信息
	QString data = buildNormalMsg(dataSegment);
	//发送tcp信息(通用完整信息在tcpClient里构建)
	send_tcpclient_data(data);

}
/************发送加油信息************
 * al  气液比
 * ********************************/
void net_isoosi_hefei::send_gundata(QString al)
{
	QString dataSegment = "";
	QString MN = "34012131UZAL01";
	QString PW = "123456";
	MN = IsoOis_MN;
	PW = IsoOis_PW;

	QStringList contaminantLists;
	contaminantLists.append("Wq4");//气液比
	QStringList dataNums;
	QString wq4 = al;
	dataNums.append(wq4);//气液比
	//构建数据区信息
	QString dataArea = buildRealtimeData(contaminantLists,"Rtd",dataNums);
	//构建数据段信息
	dataSegment = buildUploadDataSegment("31","2011",MN,PW,dataArea);

	//构建通用完整信息
	QString data = buildNormalMsg(dataSegment);
   send_tcpclient_data(data);
}
//构建##长度CRC等，最后和数据表拼接成完整信息，需要传入经过buildDataSegment()后的内容
QString net_isoosi_hefei::buildNormalMsg(QString preMsg){
	QDateTime dateTime = QDateTime::currentDateTime();
	// 字符串格式化
	QString timestamp = dateTime.toString("yyyyMMddhhmmsszzz");

	int dataLength = 0;
	dataLength = preMsg.length();
	QString result = "##";

	QByteArray byte = preMsg.toUtf8();
	unsigned char *p = (unsigned char *)byte.data();
	unsigned int crc = CRC16_Checkout(p,byte.length());
	//长度不足四位补0
	QString lengthStr = QString("%1").arg(dataLength, 4, 10, QLatin1Char('0'));
	QString hexStr = QString("%1").arg(crc, 4, 16, QLatin1Char('0'));
	return  result.append(lengthStr).append(preMsg).append(hexStr).append("\r\n");
}
/*-------------------------------------------------------------------------------------------------------------------------------------------------*/
/*下面开始为构建数据段的方法*/
//构建应答数据段,需要传入ST,CN,Flag和数据区的内容，其他的如QN自己获得，PW、MN等直接读取
QString net_isoosi_hefei::buildAnswerDataSegment(QString ST,QString CN,QString MN,QString PW,QString Flag,QString dataArea){
	QDateTime dateTime = QDateTime::currentDateTime();
	// 字符串格式化
	QString timestamp = dateTime.toString("yyyyMMddhhmmsszzz");
	QString result = "QN=";
	return  result.append(timestamp).append(";ST=").append(ST).append(";CN=").append(CN).append(";PW=").append(PW).append(";MN=").append(MN).append(";Flag=").append(Flag).append(";CP=&&")
			.append(dataArea).append("&&");
}
//构建主动上传数据段,需要传入ST,CN和数据区的内容，其他的如QN自己获得，PW、MN等直接读取
QString net_isoosi_hefei::buildUploadDataSegment(QString ST,QString CN,QString MN,QString PW,QString dataArea){
	QDateTime dateTime = QDateTime::currentDateTime();
	// 字符串格式化
	QString timestamp = dateTime.toString("yyyyMMddhhmmsszzz");
	QString result ="ST=";
	return  result.append(ST).append(";CN=").append(CN).append(";PW=").append(PW).append(";MN=").append(MN).append(";CP=&&")
			.append(dataArea).append("&&");
}
/*-------------------------------------------------------------------------------------------------------------------------------------------------*/
/*下面开始为构建数据区的方法*/
//构建超标报警数据区信息,contaminant为污染物编号如"101"，contaminantType为污染物编号后缀例如Alr、Max等，dataNum污染物报警瞬时数据,alarmType为报警时间类型
QString net_isoosi_hefei::buildOverLimitAlarm(QString contaminant,QString contaminantType,QString dataNum,QString alarmType){
	QDateTime dateTime = QDateTime::currentDateTime();
	// 字符串格式化
	QString timestamp = dateTime.toString("yyyyMMddhhmmsszzz");
	QString result = "AlarmTime=";
	return  result.append(timestamp).append(";").append(contaminant).append("-").append(contaminantType).append("=").append(dataNum).append(";AlarmType=").append(alarmType);
}
//构建实时数据区信息,contaminantLists为污染物编号列表如"101"，contaminantType为污染物编号后缀例如Alr、Max等，dataNums污染物报警瞬时数据列表
QString net_isoosi_hefei::buildRealtimeData(QStringList contaminantLists,QString contaminantType,QStringList dataNums){
	QDateTime dateTime = QDateTime::currentDateTime();
	// 字符串格式化
	QString timestamp = dateTime.toString("yyyyMMddhhmmsszzz");
	QString contaminant = "";
	for(int i = 0;i<contaminantLists.size();i++)
	{
		if(i ==contaminantLists.size()-1 )
		{
			contaminant.append(contaminantLists.at(i)).append("-").append(contaminantType).append("=").append(dataNums.at(i)).append(",").append(contaminantLists.at(i)).append("-Flag=N");//最后一个后面没有分号
		}
		else
		{
			contaminant.append(contaminantLists.at(i)).append("-").append(contaminantType).append("=").append(dataNums.at(i)).append(",").append(contaminantLists.at(i)).append("-Flag=N;");
		}
	}
	QString result = "DataTime=";
	return  result.append(timestamp).append(";").append(contaminant);
}
/*-------------------------------------------------------------------------------------------------------------------------------------------------*/
/*下面是一些工具方法*/
//crc校验
unsigned int net_isoosi_hefei::CRC16_Checkout(unsigned char *puchMsg,unsigned int usDataLen){
	unsigned int i,j,crc_reg,check;

	crc_reg = 0xFFFF;
	for(i = 0;i<usDataLen;i++){
		crc_reg = (crc_reg>>8)^puchMsg[i];
		for(j = 0;j<8;j++){
			check = crc_reg & 0x0001;
			crc_reg >>= 1;
			if(check == 0x0001){
				crc_reg ^= 0xA001;
			}
		}
	}
	return crc_reg;
}
//  tcp长连接保活
void net_isoosi_hefei::client_keep_ali(int sockfd_hefei)
{
	int keepAlive = 1;      // 开启keepalive属性
	int keepIdle = 10;      // 如该连接在60秒内没有任何数据往来,则进行探测
	int keepInterval = 10;   // 探测时发包的时间间隔为5 秒
	int keepCount = 3;      // 探测尝试的次数.如果第1次探测包就收到响应了,则后2次
	setsockopt(sockfd_hefei, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepAlive, sizeof(keepAlive));
	setsockopt(sockfd_hefei, SOL_TCP, TCP_KEEPIDLE, (void*)&keepIdle, sizeof(keepIdle)); //对应tcp_keepalive_time
	setsockopt(sockfd_hefei, SOL_TCP, TCP_KEEPINTVL, (void *)&keepInterval, sizeof(keepInterval)); //对应tcp_keepalive_intvl
	setsockopt(sockfd_hefei, SOL_TCP, TCP_KEEPCNT, (void *)&keepCount, sizeof(keepCount)); //对应tcp_keepalive_probes
}
