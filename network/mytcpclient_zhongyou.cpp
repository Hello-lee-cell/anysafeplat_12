#include <QApplication>
#include <QTimer>
#include <qsemaphore.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <math.h>//abs
#include <network/udp.h>
#include <config.h>
#include <network/ip_op.h>
#include <serial.h>
#include <uart_main.h>
#include <file_op.h>
#include <qsocketnotifier.h>
#include <QSocketNotifier>
#include "mytcpclient_zhongyou.h"
#include "main_main_zhongyou.h"
#include "io_op.h"
#include "database_op.h"

unsigned char Flag_FirstClient_zhongyou = 2;//判断是不是第一次连接，做历史记录用

QString ip_renzheng = "11.0.31.88";
sem_t sem;//信号量做阻塞
QMutex wait_send_over_iffs;

char Remote_Ip_Zy[20] = {0};
unsigned int receiveudp = 0;
int nsockfd_tcp_ifis;        //tcp套接字
int sockfd_ifis;            //tcp套接字
int on_tcp_ifis = 1;
char revbuf_tcp[LENGTH];
unsigned char Flag_TcpClient_Success_Ifis = 0;//tcpclient连接成功
unsigned char Tcp_send_buf[256] = {0};
unsigned char Tcp_send_count_Ifis = 0;
unsigned char Flag_connect_tcpClient = 1;//连接tcpclient标志位   1开启 2关闭
unsigned char flag_reboot_mytcpclient = 0;//第一次启动需要重启一次socket，不知道为什么
unsigned char Flag_TcpClient_SendOver_Ifis = 0;//判断是否发送完成。
long num_signal_receive = 0;//测试时，接收到的信号
long num_send_client = 0;//测试时，发送计数

mytcpclient_zhongyou::mytcpclient_zhongyou(QWidget *parent):
    QThread(parent)
{
	sem_init(&sem,0,0);//信号量初始化
	timer_rtcp = new QTimer();
	timer_rtcp->setInterval(5000);
	timer_rtcp->start();
	connect(timer_rtcp,SIGNAL(timeout()),this,SLOT(reset_sem()));
}

void mytcpclient_zhongyou::run()         //线程创建函数要求 void* 类型
{
	sleep(1);
	while(1)
	{
		//if((Flag_TcpClose_FromUdp == 0)&&(receiveudp == 1))
		if(receiveudp == 1)
		{
			send_tcp();
			sleep(3);
			qDebug()<<"tcp client client client!";
		}
		else
		{
			sleep(1);
			//qDebug()<<"tcp not client client client!";
		}
	}
	this->exec();
}

void mytcpclient_zhongyou::send_tcp()
{
	//获取udp发送过来的ip地址
	const QByteArray text = ip_renzheng.toLocal8Bit();
	const char *tcp_sever_ip = text.data();

	//int sockfd;                        	// Socket file descriptor
	struct sockaddr_in sever_remote;
	// Get the Socket file descriptor
	if( (sockfd_ifis = socket(AF_INET, SOCK_STREAM, 0)) == -1 )
	{
		printf ("TCPClient Failed to obtain Socket Despcritor.\n");
//        return 0;
	}
	else
	{
		printf ("TCPClient Obtain Socket Despcritor sucessfully.\n");
		//history_net_write("Tcp_Obtain");
	}
	sever_remote.sin_family = AF_INET;           		// Protocol Family 协议族
	sever_remote.sin_port = htons (porttcpclietn);         		// Port number 端口号
	sever_remote.sin_addr.s_addr  = inet_addr(tcp_sever_ip);  	// 填写远程地址
	//sever_remote.sin_addr.s_addr  = inet_aton(tcp_sever_ip);  	// 填写远程地址
	memset (sever_remote.sin_zero,0,8);          		// Flush the rest of struct 刷新struct的其余部分

	//端口重用
	if((setsockopt(sockfd_ifis,SOL_SOCKET,SO_REUSEADDR,&on_tcp_ifis,sizeof(on_tcp_ifis)))<0)
	{
		perror("TCPClient setsockopt failed");
		exit(1);
	}

	bool  bDontLinger = FALSE;
	setsockopt(sockfd_ifis,SOL_SOCKET,SO_LINGER,(const char*)&bDontLinger,sizeof(bool));//不要因为数据未发送就阻塞关闭操作

	//setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &on_tcp_ifis, sizeof(on_tcp_ifis));
	setsockopt(sockfd_ifis, IPPROTO_TCP, TCP_NODELAY,&on_tcp_ifis,sizeof(on_tcp_ifis));//立即发送，不沾包

	//主动连接目标,连接不上貌似会卡在这，10秒再连一次
	if (( nsockfd_tcp_ifis = ::connect(sockfd_ifis,(struct sockaddr *)&sever_remote,sizeof(struct sockaddr))) < 0)
	{
		printf ("TCPClient Failed to Client Port %d.\n",porttcpclietn);
		//return (0);
		Flag_TcpClient_Success_Ifis = 0;//连接失败
		close(nsockfd_tcp_ifis);

		if(Flag_FirstClient_zhongyou != 0)
		{
			add_value_netinfo("ZhongYou TcpClient is failed");
			Flag_FirstClient_zhongyou = 0;
		}

	}
	else
	{
		printf ("TCPClient Client the Port %d sucessfully.\n", porttcpclietn);
		Flag_TcpClient_Success_Ifis = 1;//连接成功
		client_keep_ali(sockfd_ifis);//tcp保活

		if(Flag_FirstClient_zhongyou != 1)
		{
			add_value_netinfo("ZhongYou TcpClient is cennected!");
			Flag_FirstClient_zhongyou = 1;
		}

	}
	//新线程，tcp  主动发送关掉
	pthread_t id_tcptalk;
	long t_tcp = 0;
	int ret_tcp = pthread_create(&id_tcptalk, NULL,client_tcp_zy, (void*)t_tcp);
	if(ret_tcp != 0)
	{
		printf("Can not create thread!");
	}
	else
	{
		pthread_detach(id_tcptalk);//分离线程，结束后立即释放资源
	}
	sleep(2);
	while(1)
	{
		if(Flag_TcpClient_Success_Ifis == 0)
		{
			printf ("wait TCPClient  Client \n");
			sleep(5);
			break;
		}
		else
		{
			sem_wait(&sem);//可用资源-1，阻塞

			//wait_send_over.lock();//锁数组Sdbuf
			if(Tcp_send_count_Ifis != 0)
			{
				int num_send = 0;
				if((num_send = ::send(sockfd_ifis,Sdbuf_Ifis, Tcp_send_count_Ifis, 0)) == -1)
				{
					printf("ERROR: Failed to sent string.\n");
					Flag_TcpClient_Success_Ifis = 0;//连接失败
					break;
				}
				else
				{
					printf("TcpClient send success %d.\n",Tcp_send_count_Ifis);
					for(unsigned int i = 0;i<Tcp_send_count_Ifis;i++)
					{
						printf("%02x ",Sdbuf_Ifis[i]);
					}
					printf("\n");
					Tcp_send_count_Ifis = 0;
//                  num_send_client++;
//                  printf("send to sever num is %d\n",num_send_client);
				}
				Flag_TcpClient_SendOver_Ifis = 0;
			}
			else
			{

			}
			//wait_send_over.unlock();//解锁数组Sdbuf

		}
		if(Flag_connect_tcpClient == 0)//设置为断开
		{
			printf("TCP Client Active disconnect\n");
			break;
		}
	}
	shutdown(sockfd_ifis,2);
	close(sockfd_ifis);
	close(nsockfd_tcp_ifis);
	sleep(1);
}

unsigned char mytcpclient_zhongyou::net_reply(int send_buf_count)//回复tcp消息
{
	printf("receive signal of old tcp\n");
	Tcp_send_count_Ifis = send_buf_count;
	sem_post(&sem);//信号量可用资源+1
	Wait_Send_Flag = 0;//假装发送完成了
//    num_signal_receive++;
//    printf("receive from main_mian signal num is %d\n",num_signal_receive);

}
//  tcp长连接保活
void mytcpclient_zhongyou::client_keep_ali(int sockfd_ifis)
{
	int keepAlive = 1;      // 开启keepalive属性
	int keepIdle = 10;      // 如该连接在60秒内没有任何数据往来,则进行探测
	int keepInterval = 10;   // 探测时发包的时间间隔为5 秒
	int keepCount = 3;      // 探测尝试的次数.如果第1次探测包就收到响应了,则后2次
	setsockopt(sockfd_ifis, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepAlive, sizeof(keepAlive));
	setsockopt(sockfd_ifis, SOL_TCP, TCP_KEEPIDLE, (void*)&keepIdle, sizeof(keepIdle)); //对应tcp_keepalive_time
	setsockopt(sockfd_ifis, SOL_TCP, TCP_KEEPINTVL, (void *)&keepInterval, sizeof(keepInterval)); //对应tcp_keepalive_intvl
	setsockopt(sockfd_ifis, SOL_TCP, TCP_KEEPCNT, (void *)&keepCount, sizeof(keepCount)); //对应tcp_keepalive_probes
}
//   TCP通信：报警状态主动发送
void *client_tcp_zy(void*)
{
	int num;        //报警状态主动发送标志位

	char sdbuf_active[LENGTH];
	printf("WARNING From sony client!!\n");
	while(1)
	{
		if(Flag_connect_tcpClient == 0)
		{
			//shutdown(nsockfd_tcp_ifis,2);
			//close(nsockfd_tcp_ifis);
			Flag_TcpClient_Success_Ifis = 0;
			break;
		}

		sleep(5);
		if(Flag_TcpClose_FromUdp == 1)
		{
			printf("tcp_client close becouse udp close!!!!\n");
			//Flag_TcpClose_FromTcp = 0;
			//shutdown(nsockfd_tcp_ifis,2);
			//close(nsockfd_tcp_ifis);
			Flag_TcpClient_Success_Ifis = 0;
			break;
		}

		memset(sdbuf_active,0,sizeof(char)*512);
		sdbuf_active[0] = 0x02;
		sdbuf_active[1] = 0x01;
		sdbuf_active[2] = 0x07;
		sdbuf_active[3] = ID_M;
		sdbuf_active[4] = 0; //Mc
		sdbuf_active[5] = 0x80;//M_st
		sdbuf_active[6] = 0; //M_Lg 高位
		sdbuf_active[7] = 0x0d;
		sdbuf_active[8] = 2; //DB_Ad_Lg
		sdbuf_active[21] = 0x64;

		for(int i = 1;i < 9;i++)
		{
			sdbuf_active[9] = 0x02;

			if(Mythread_basin[i])
			{
				sdbuf_active[11] = 0x02;
				sdbuf_active[12] = 0x02;
				sdbuf_active[13] = OIL_BASIN[(i-1)*2] & 0x0f;;
				sdbuf_active[14] = OIL_BASIN[(i-1)*2+1]&0x7f;;
				sdbuf_active[15] = 0x03;
				sdbuf_active[16] = 0x01;
				sdbuf_active[17] = OIL_BASIN[(i-1)*2]>>6;;
				sdbuf_active[18] = 0x04;
				sdbuf_active[19] = 0x01;
				sdbuf_active[20] = i;
				//qDebug()<<"#######02###"<<i;

				sdbuf_active[10] = i |0x10;
				printf("WARNING From sony2\n");
				if((num = send(sockfd_ifis,sdbuf_active,21,0)) == -1)
				{
					printf("error:failed to send warning message!\n");
					Flag_TcpClient_Success_Ifis = 0;
					return 0;
				}
				for(unsigned char i = 0;i<22;i++)
				{
					printf(" %02x",sdbuf_active[i]);
				}
				printf("****************\n");
				usleep(300000);
				sleep(1);
			}
		}

		for(int i = 1;i < 9;i++)
		{

			sdbuf_active[9] = 0x03;
			if(Mythread_pipe[i])
			{
				sdbuf_active[11] = 0x02;
				sdbuf_active[12] = 0x02;
				sdbuf_active[13] = OIL_PIPE[(i-1)*2] & 0x0f;;
				sdbuf_active[14] = OIL_PIPE[(i-1)*2+1]&0x7f;;
				sdbuf_active[15] = 0x03;
				sdbuf_active[16] = 0x01;
				sdbuf_active[17] = OIL_PIPE[(i-1)*2]>>6;;
				sdbuf_active[18] = 0x04;
				sdbuf_active[19] = 0x01;
				sdbuf_active[20] = i;
				//qDebug()<<"#######03###"<<i;
				sdbuf_active[10] = i |0x10;
				printf("WARNING From sony2\n");

				if((num = send(sockfd_ifis,sdbuf_active,21,0)) == -1)
				{
					printf("error:failed to send warning message!\n");
					Flag_TcpClient_Success_Ifis = 0;

					return 0;
				}


				for(unsigned char i = 0;i<22;i++)
				{
					printf(" %02x",sdbuf_active[i]);
				}
				printf("****************\n");
				usleep(300000);
				sleep(1);
			}
		}

		for(int i = 1;i < 9;i++)
		{

			sdbuf_active[9] = 0x04;
			if(Mythread_tank[i])
			{
				sdbuf_active[11] = 0x02;
				sdbuf_active[12] = 0x02;
				sdbuf_active[13] = OIL_TANK[(i-1)*2] & 0x0f;;
				sdbuf_active[14] = OIL_TANK[(i-1)*2+1]&0x7f;;
				sdbuf_active[15] = 0x03;
				sdbuf_active[16] = 0x01;
				sdbuf_active[17] = OIL_TANK[(i-1)*2]>>6;;
				sdbuf_active[18] = 0x04;
				sdbuf_active[19] = 0x01;
				sdbuf_active[20] = i;
				//qDebug()<<"#######04###"<<i;

				sdbuf_active[10] = i |0x10;
				printf("WARNING From sony2\n");

				if((num = send(sockfd_ifis,sdbuf_active,21,0)) == -1)
				{
					printf("error:failed to send warning message %d!\n",num);
					Flag_TcpClient_Success_Ifis = 0;

					return 0;
				}


				for(unsigned char i = 0;i<22;i++)
				{
					printf(" %02x",sdbuf_active[i]);
				}
				printf("****************\n");
				usleep(300000);
				sleep(1);
			}
		}

		for(int i = 1;i < 9;i++)
		{
			sdbuf_active[9] = 0x05;
			if(Mythread_dispener[i])
			{
				sdbuf_active[11] = 0x02;
				sdbuf_active[12] = 0x02;
				sdbuf_active[13] = OIL_DISPENER[(i-1)*2] & 0x0f;;
				sdbuf_active[14] = OIL_DISPENER[(i-1)*2+1]&0x7f;;
				sdbuf_active[15] = 0x03;
				sdbuf_active[16] = 0x01;
				sdbuf_active[17] = OIL_DISPENER[(i-1)*2]>>6;;
				sdbuf_active[18] = 0x04;
				sdbuf_active[19] = 0x01;
				sdbuf_active[20] = i;
				//qDebug()<<"#######05###"<<i;

				sdbuf_active[10] = i |0x10;
				printf("WARNING From sony2\n");

				if((num = send(sockfd_ifis,sdbuf_active,21,0)) == -1)
				{
					printf("error:failed to send warning message!\n");
					Flag_TcpClient_Success_Ifis = 0;

					return 0;
				}


				for(unsigned char i = 0;i<22;i++)
				{
					printf(" %02x",sdbuf_active[i]);
				}
				printf("****************\n");
				usleep(300000);
				sleep(1);
			}
		}
		if(LEAK_DETECTOR != 2)
		{

			sdbuf_active[0] = 0x02;
			sdbuf_active[1] = 0x01;
			sdbuf_active[2] = 0x07;
			sdbuf_active[3] = ID_M;
			sdbuf_active[4] = 0;    //Mc
			sdbuf_active[5] = 0x81; //M_st
			sdbuf_active[6] = 0;
			sdbuf_active[7] = 0x04;
			sdbuf_active[8] = 0x01;//db_ad_lg
			sdbuf_active[9] = 0x06;
			sdbuf_active[10] = 0x64;
		}
	}
	return 0;
}
void mytcpclient_zhongyou::reset_sem()//重置信号量  10秒超时，防止tcp断线，不能继续往下进行
{
	sem_post(&sem);//信号量可用资源+1
	int num = 0;
	sem_getvalue(&sem,&num);
	//printf("sem is**************%d\n",num);
	if(num>10)
	{
		sem_wait(&sem);//可用资源-1，阻塞
	}
	if(num>150)
	{
		for(unsigned int i = 0;i<80;i++)
		{
			sem_wait(&sem);//可用资源-1，阻塞
		}
	}
}

