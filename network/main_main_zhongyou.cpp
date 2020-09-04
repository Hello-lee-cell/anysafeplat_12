/*中油泄漏协议 与mytcpclient_zhongyou.cpp一起使用
 *
 * */
#include <QApplication>
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
#include "main_main_zhongyou.h"
#include "mytcpclient_zhongyou.h"
#include "database_op.h"
#include "io_op.h"


unsigned char version_zhongyou[7]={0x05,0x99,0x99,0x99,0x99,0x99,0x99};   //协议版本号
unsigned char LEAK_DETECTOR_ZHONGYOU=0x02; //测漏报警控制器状态数据  非认证没有其他状态
unsigned char Wait_Send_Flag = 0;                                   //文件读写状态位，防冲突
long num_receive = 0;//测试时对接收计数
int nsockfd_zhongyou;        //tcp套接字
int on_zhongyou = 1;
int time_count_talk_zhongyou = 1;
char revbuf_zhongyou[LENGTH];
unsigned char Logic_Add_S_ZY = 0x01;
unsigned char Logic_Add_N_ZY = 0x01;
char *IP_local_ZY = "192.168.1.1";
char Sdbuf_Ifis[LENGTH];
unsigned char Flag_Count_TCP = 0;
//udp
unsigned char UDP_STATE_ZY = 0;
int porttcpclietn = 6666;
main_main_zhongyou::main_main_zhongyou(QObject *parent):
    QThread(parent)
{

}

void main_main_zhongyou::run()
{
	net_data();
	this->exec();
}
//  tcp长连接保活
void keep_ali_zy(int sockfd)
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

//   TCP通信：报警状态主动发送 //转移到mytcpclient中去
void *shutdown_tcp(void*)
{

}

void main_main_zhongyou::net_data ()
{
   // system("ntpdate 192.168.1.106");
	io_init();//设备初始化，待补充

	int sockfd;                        	// Socket file descriptor
	int sin_size;                      	// to store struct size

	struct sockaddr_in addr_local;
	struct sockaddr_in addr_remote;
	char IP[32] = {0};
	get_local_ip(if_name,IP);
	IP_local_ZY = IP;
	ip_value(IP);
	printf("%d~%d~%d~%d\n",ipa,ipb,ipc,ipd);

	//创建新线程,UDP
	pthread_t id_udplisten;    //新线程ID
	long t = 0;
	int ret = pthread_create(&id_udplisten, NULL, listen_udp_zy, (void*)t);
	if(ret != 0)
	{
		printf("Can not create thread!");
		exit(1);
	}
//以上UDP
sleep(1);
//TCP
    // Get the Socket file descriptor
    if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1 )
	{
		printf ("ERROR: Failed to obtain Socket Despcritor.\n");
		//return (0);
	}
	else
	{
		printf ("OK: Obtain Socket Despcritor sucessfully.\n");
		history_net_write("Tcp_Obtain");
	}

	// Fill the local socket address struct
	addr_local.sin_family = AF_INET;           		// Protocol Family
	addr_local.sin_port = htons (PORT_TCP);         		// Port number
	addr_local.sin_addr.s_addr  = htonl (INADDR_ANY);  	// AutoFill local address
	memset (addr_local.sin_zero,0,8);          		// Flush the rest of struct
	//端口重用
	if((setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&on_zhongyou,sizeof(on_zhongyou)))<0)
	{
		perror("setsockopt failed");
		exit(1);
	}

	//  Blind a special Port
	if( bind(sockfd, (struct sockaddr*)&addr_local, sizeof(struct sockaddr)) == -1 )
	{
		printf ("ERROR_TCP: Failed to bind Port %d.\n",PORT_TCP);
		//return (0);
	}
	else
	{
		printf("OK: TCP Bind the Port %d sucessfully.\n",PORT_TCP);
		history_net_write("Tcp_Bind");
	}

	//  Listen remote connect/calling
	if(listen(sockfd,BACKLOG) == -1)
	{
		printf ("ERROR: Failed to listen Port %d.\n", PORT_TCP);
		//return (0);
	}
	else
	{
		printf ("OK:TCP Listening the Port %d sucessfully.\n", PORT_TCP);
		history_net_write("Tcp_Listen");
	}

	sin_size = sizeof(struct sockaddr_in);

	    while(1)
		{
			sin_size = sizeof(struct sockaddr_in);

			//  Wait a connection, and obtain a new socket file despriptor for single connection
			//等待连接，并为单连接获取新的套接字文件释放器
			if ((nsockfd_zhongyou = accept(sockfd, (struct sockaddr *)&addr_remote, (socklen_t*)&sin_size)) == -1)
			{
				printf ("ERROR: Obtain new Socket Despcritor error.\n");
				continue;
				receiveudp = 0;
			}
			else
			{
				printf ("OK: Server has got connect from %s.\n", inet_ntoa(addr_remote.sin_addr));
				keep_ali_zy(nsockfd_zhongyou);
				history_net_write("Tcp_connect");
				//receiveudp = 1; //tcp server已连接
			}
			//新线程，tcp
//			pthread_t id_tcpdown;
//			long t_tcpdown = 0;
//			int ret_tcpdown = pthread_create(&id_tcpdown, NULL,shutdown_tcp, (void*)t_tcpdown);
//			if(ret_tcpdown != 0)
//			{
//				printf("Can not create thread!");
//				continue;
//			}


			printf("recevie string.\n");
			int num_recv;      //发送，接收状态标志位   2recv

			int sdbuf_count = 10;

			int Mc,M_St,M_Lg,DB_Ad_Lg,Data_Lg;
			unsigned char DB_Ad[2];
			unsigned char Data_El[20];
			unsigned char Data_ID[20];

			unsigned char OIL_BASIN_TEMP[16];
			unsigned char OIL_PIPE_TEMP[16];            //缓存
			unsigned char OIL_TANK_TEMP[16];
			unsigned char OIL_DISPENER_TEMP[16];
			while(1)
			{
				memset(revbuf_zhongyou,0,sizeof(char)*512);
				if((num_recv = recv(nsockfd_zhongyou,revbuf_zhongyou,sizeof(revbuf_zhongyou),0)) < 0)
				{
					qDebug()<< num_recv;
					printf("TCP recv shutdown!** \n");
					shutdown(nsockfd_zhongyou,2);
					Wait_Send_Flag = 0;
					//Flag_TcpClose_FromTcp = 1;
					receiveudp = 0;
					sleep(1);
					break;
				}
				if(num_recv > 0)
				{
					printf("Received over: ");
					for(int i= 0;i<num_recv;i++)
					{
						printf("%02x  ",revbuf_zhongyou[i]);
					}

				}

				if(Flag_TcpClient_SendOver_Ifis == 0)
				{

				}
				else
				{
					int js = 0;
					while(Flag_TcpClient_SendOver_Ifis)
					{
						js++;
						usleep(1);
						if(js>500)
						{
							Flag_TcpClient_SendOver_Ifis = 0;
						}
					}
				}
				Flag_TcpClient_SendOver_Ifis = 1;//接收一条数据+1，发送完成后置0

//                num_receive++;
//                printf("receive from client num is %d\n",num_receive);
				//数据解析
				while(Wait_Send_Flag);

				Wait_Send_Flag = 1;
//                usleep(1000);
				for(int i = 0;i < 16;i++)
				{
					OIL_BASIN_TEMP[i] = OIL_BASIN[i];
				}
				for(int i = 0;i < 16;i++)
				{
					OIL_PIPE_TEMP[i] = OIL_PIPE[i];
				}
				for(int i = 0;i < 16;i++)
				{
					OIL_TANK_TEMP[i] = OIL_TANK[i];
				}
				for(int i = 0;i < 16;i++)
				{
					OIL_DISPENER_TEMP[i] = OIL_DISPENER[i];
				}
				Wait_Send_Flag = 0;
				if((num_recv < 256) && (num_recv > 0))
				{
					memset(Data_El,0,sizeof(char)*20);
					memset(Data_ID,0,sizeof(char)*20);
					Mc = revbuf_zhongyou[4];
					M_St = revbuf_zhongyou[5];
					M_Lg = revbuf_zhongyou[7];
					DB_Ad_Lg = revbuf_zhongyou[8];
					for(int i = DB_Ad_Lg;i > 0;i--)
					{
						DB_Ad[i - 1] = revbuf_zhongyou[8 + i];
					}

					if(DB_Ad_Lg == 1 && DB_Ad[0] == 0x00)//通信服务数据
					{
						//Sdbuf_Ifis[0] = revbuf_zhongyou[2];
						//Sdbuf_Ifis[1] = revbuf_zhongyou[3];
						Sdbuf_Ifis[0] = 0x02;//把数据节点写死
						Sdbuf_Ifis[1] = 0x01;
						Logic_Add_S_ZY = revbuf_zhongyou[2];
						Logic_Add_N_ZY = revbuf_zhongyou[3];
						Sdbuf_Ifis[2] = 0x07;
						Sdbuf_Ifis[3] = ID_M;
						Sdbuf_Ifis[4] = 0; //Mc
						Sdbuf_Ifis[6] = 0; //M_Lg 高位
						Sdbuf_Ifis[8] = 1; //DB_Ad_Lg
						Sdbuf_Ifis[9] = 0; //DB_Ad
						if(Mc == 2)//Mc判断的位置好像不对 。。。。过滤要加在前面
						{
							if((M_St&0xe0) == 0)          //对系统 读消息 进行回复
							{
								for(int i = 0;i < (M_Lg-2);i++)
								{
									Data_ID[i] = revbuf_zhongyou[10 + i];

									if(Data_ID[i] == 0x01)        //协议版本号
									{
										Sdbuf_Ifis[sdbuf_count] = 0x01;
										sdbuf_count++;
										Sdbuf_Ifis[sdbuf_count] = 0x07;
										sdbuf_count++;
										Sdbuf_Ifis[sdbuf_count] = version_zhongyou[0];
										sdbuf_count++;
										Sdbuf_Ifis[sdbuf_count] = version_zhongyou[1];
										sdbuf_count++;
										Sdbuf_Ifis[sdbuf_count] = version_zhongyou[2];
										sdbuf_count++;
										Sdbuf_Ifis[sdbuf_count] = version_zhongyou[3];
										sdbuf_count++;
										Sdbuf_Ifis[sdbuf_count] = version_zhongyou[4];
										sdbuf_count++;
										Sdbuf_Ifis[sdbuf_count] = version_zhongyou[5];
										sdbuf_count++;
										Sdbuf_Ifis[sdbuf_count] = version_zhongyou[6];
										sdbuf_count++;
									}
									if(Data_ID[i] == 0x04)        //心跳间隔
									{
										Sdbuf_Ifis[sdbuf_count] = 0x04;
										sdbuf_count++;
										Sdbuf_Ifis[sdbuf_count] = 1;
										sdbuf_count++;
										Sdbuf_Ifis[sdbuf_count] = heartbeat_time;   //心跳间隔
										sdbuf_count++;
									}

								}
								Sdbuf_Ifis[5] = 0x20|(0x1f&M_St);
								Sdbuf_Ifis[7] = sdbuf_count - 8;

								emit net_reply(sdbuf_count);

//                                if((num_send = send(nsockfd_zhongyou, Sdbuf, sdbuf_count, 0)) == -1)
//                                {
//                                    printf("ERROR: Failed to sent string.\n");
//                                    Flag_TcpClose_FromTcp = 1; //close(nsockfd_zhongyou);
//                                    break;
//                                }
//                                else
//                                {
//                                    printf("OK: Sent %d bytes sucessful.\n", num_send);
//                                }
								sdbuf_count = 10;
								Wait_Send_Flag = 0;
							}
							if((M_St&0xe0) == 0x40)           //对写入信息进行设置，回复
							{
								int i_4 = 0;
								int lon = 0;
								while(lon < (M_Lg - 2))         //提取Data_ID
								{
									Data_ID[i_4] = revbuf_zhongyou[10 + lon];
									lon++;
									Data_Lg = revbuf_zhongyou[10 + lon];
									lon++;
									for(int j = 0;j < Data_Lg;j++)
									{
										Data_El[j] = revbuf_zhongyou[10 + lon + j];
									}
									lon = lon + Data_Lg;
									if(Data_ID[i_4] == 0x02)//设置节点地址
									{
										ID_M = Data_El[1];
										Sdbuf_Ifis[3] = ID_M;
										Sdbuf_Ifis[sdbuf_count] = 0;
										sdbuf_count++;
										printf("ID set ok~\n");
										fflush(stdout);
									}
									if(Data_ID[i_4] == 0x04)    //设置心跳间隔
									{
										heartbeat_time = Data_El[0];
										printf("heartbeattime set over\n");
									}
									i_4++;
								}
								Sdbuf_Ifis[5] = 0xe0|(0x1f&M_St);
								Sdbuf_Ifis[7] = sdbuf_count - 8;

								emit net_reply(sdbuf_count);

//                                if((num_send = send(nsockfd_zhongyou,Sdbuf,sdbuf_count,0)) == -1)
//                                {
//                                    printf("ERROR: Failed to sent string.\n");
//                                    Flag_TcpClose_FromTcp = 1; //close(nsockfd_zhongyou);
//                                    break;
//                                }
//                                else
//                                {
//                                    printf("OK: Sent %d bytes sucessful.\n", num_send);

//                                }

								sdbuf_count = 10;
								Wait_Send_Flag = 0;

							}
						}
					}

					if((DB_Ad_Lg == 1) && (DB_Ad[0] == 0x06))   //测漏报警控制器状态数据
					{
						//Sdbuf_Ifis[0] = revbuf_zhongyou[2];
						//Sdbuf_Ifis[1] = revbuf_zhongyou[3];
						Sdbuf_Ifis[0] = 0x02;//把数据节点写死
						Sdbuf_Ifis[1] = 0x01;
						Sdbuf_Ifis[2] = 0x07;
						Sdbuf_Ifis[3] = ID_M;
						Sdbuf_Ifis[4] = 0;
						Sdbuf_Ifis[5] = 0x20|(0x1f&M_St);
						Sdbuf_Ifis[6] = 0;
						Sdbuf_Ifis[7] = 5;
						Sdbuf_Ifis[8] = 1;
						Sdbuf_Ifis[9] = 0x06;
						Sdbuf_Ifis[10] = 0x01;
						Sdbuf_Ifis[11] = 0x01;
						Sdbuf_Ifis[12] = LEAK_DETECTOR_ZHONGYOU;
						// sdbuf_count = 13;
						emit net_reply(13);

//                        if((num_send = send(nsockfd_zhongyou,Sdbuf,13,0)) == -1)
//                        {
//                            printf("ERROR: Failed to sent string.\n");
//                            Flag_TcpClose_FromTcp = 1; //close(nsockfd_zhongyou);
//                            break;
//                        }
//                        else
//                        {
//                            printf("OK: Sent %d bytes sucessful.\n", num_send);

//                        }

						Wait_Send_Flag = 0;
					}
					if((DB_Ad_Lg == 2) && (DB_Ad[0] == 0x02))//人井测漏状态数据
					{
						unsigned char data_x2 = 0;
						unsigned char data_x3 = 0;
						unsigned char data_x4 = 0;
						unsigned char data_x5 = 0;
						unsigned char data_x6 = 0;
						unsigned char data_x10 = 0;
						unsigned char data_x11 = 0;
						unsigned char data_x12 = 0;
						unsigned char data_x13 = 0;
						unsigned char data_x14 = 0;
						unsigned char data_x15 = 0;

						for(unsigned char i = 0;i<11;i++)
						{
							Data_ID[i] = revbuf_zhongyou[11+i];
							if(Data_ID[i]==0x02){data_x2 = 1;}
							if(Data_ID[i]==0x03){data_x3 = 1;}
							if(Data_ID[i]==0x04){data_x4 = 1;}
							if(Data_ID[i]==0x05){data_x5 = 1;}
							if(Data_ID[i]==0x06){data_x6 = 1;}
							if(Data_ID[i]==0x10){data_x10 = 1;}
							if(Data_ID[i]==0x11){data_x11 = 1;}
							if(Data_ID[i]==0x12){data_x12 = 1;}
							if(Data_ID[i]==0x13){data_x13 = 1;}
							if(Data_ID[i]==0x14){data_x14 = 1;}
							if(Data_ID[i]==0x15){data_x15 = 1;}
						}
						Sdbuf_Ifis[0] = 0x02;//把数据节点写死
						Sdbuf_Ifis[1] = 0x01;
						Sdbuf_Ifis[2] = 0x07;
						Sdbuf_Ifis[3] = ID_M;
						Sdbuf_Ifis[4] = 0; //Mc
						Sdbuf_Ifis[6] = 0; //M_Lg 高位
						Sdbuf_Ifis[8] = 2; //DB_Ad_Lg
						Sdbuf_Ifis[9] = 0x02; //DB_Ad
						Sdbuf_Ifis[sdbuf_count] = revbuf_zhongyou[10];//DB_Ad
						sdbuf_count++;
						int LB_ID = DB_Ad[1]&0x0f;
						unsigned char type_basin = 0;
						int OIL_BASIN_count = 0;
						if(!LB_ID)
						{
							LB_ID = count_basin;    //需设置为实际传感器数量

							for(int i = 0;i < LB_ID;i++)
							{
								Sdbuf_Ifis[sdbuf_count] = 0x02;
								sdbuf_count++;
								Sdbuf_Ifis[sdbuf_count] = 0x02;
								sdbuf_count++;
								Sdbuf_Ifis[sdbuf_count] = OIL_BASIN_TEMP[OIL_BASIN_count]&0x0f;
								type_basin = OIL_BASIN_TEMP[OIL_BASIN_count] >> 6;
								sdbuf_count++;
								OIL_BASIN_count++;
								Sdbuf_Ifis[sdbuf_count] = OIL_BASIN_TEMP[OIL_BASIN_count]&0x7f;  //&0111 1111
								sdbuf_count++;
								OIL_BASIN_count++;
								Sdbuf_Ifis[sdbuf_count] = 0x03;      //测漏类型
								sdbuf_count++;
								Sdbuf_Ifis[sdbuf_count] = 0x01;
								sdbuf_count++;
								Sdbuf_Ifis[sdbuf_count] = type_basin;
								sdbuf_count++;
								Sdbuf_Ifis[sdbuf_count] = 0x04;//编号
								sdbuf_count++;
								Sdbuf_Ifis[sdbuf_count] = 0x01;
								sdbuf_count++;
								Sdbuf_Ifis[sdbuf_count] = i+1;
								sdbuf_count++;
							}
						}
						else
						{
							if(LB_ID>count_basin)//如果问的传感器没有设置
							{
								Data_ID[0] = revbuf_zhongyou[11];

								if((Data_ID[0] == 0x02)||(!Data_ID[0]))
								{
									Sdbuf_Ifis[sdbuf_count] = 0x02;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x02;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] =  0x10;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;
									sdbuf_count++;
								}
								if((Data_ID[0] == 0x03)||(!Data_ID[0]))
								{
									Sdbuf_Ifis[sdbuf_count] = 0x03;//测漏类型
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x01;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x04; //类型为0x04，其他
									sdbuf_count++;
								}
								if((Data_ID[0] == 0x04)||(!Data_ID[0]))
								{
									Sdbuf_Ifis[sdbuf_count] = 0x04;//编号
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x01;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = LB_ID&0x0f;
									sdbuf_count++;
								}
								//问压力值
								if((Data_ID[0] == 0x05)||(!Data_ID[0]))
								{
									Sdbuf_Ifis[sdbuf_count] = 0x05;//压力值
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x04;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x05;  //压力值正
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //压力值空位
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //压力值前两位
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;  //压力值后两位
									sdbuf_count++;

								}
								if((Data_ID[0] == 0x06)||(!Data_ID[0]))//气体浓度
								{
									Sdbuf_Ifis[sdbuf_count] = 0x06;//气体浓度
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x02;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;  //浓度高位
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //浓度低位
									sdbuf_count++;
								}


								if((Data_ID[0] == 0x10)||(!Data_ID[0]))//高液位报警值
								{
									Sdbuf_Ifis[sdbuf_count] = 0x10;//高液位
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x04;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;  //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x03; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
								}
								if((Data_ID[0] == 0x11)||(!Data_ID[0]))//高高液位报警值
								{
									Sdbuf_Ifis[sdbuf_count] = 0x11;//高高液位
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x04;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;  //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x06; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
								}
								if((Data_ID[0] == 0x12)||(!Data_ID[0]))//低液位报警值
								{
									Sdbuf_Ifis[sdbuf_count] = 0x12;//低液位
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x04;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;  //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x03; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
								}
								if((Data_ID[0] == 0x13)||(!Data_ID[0]))//低低液位报警值
								{
									Sdbuf_Ifis[sdbuf_count] = 0x13;//低低液位
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x04;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;  //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x06; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
								}
								if((Data_ID[0] == 0x14)||(!Data_ID[0]))//浓度预警值
								{
									Sdbuf_Ifis[sdbuf_count] = 0x14;//气体浓度
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x02;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x25;
									sdbuf_count++;
								}
								if((Data_ID[0] == 0x15)||(!Data_ID[0]))//浓度报警值
								{
									Sdbuf_Ifis[sdbuf_count] = 0x15;//气体浓度
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x02;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x40;
									sdbuf_count++;
								}

								if((Data_ID[0] != 0x02) && (Data_ID[0] != 0x03) && (Data_ID[0] != 0x04) && (Data_ID[0] != 0x05) && (Data_ID[0] != 0x06) &&
								        (Data_ID[0] != 0x10) && (Data_ID[0] != 0x11) && (Data_ID[0] != 0x12) && (Data_ID[0] != 0x13) && (Data_ID[0] != 0x14) &&
								        (Data_ID[0] != 0x15))
								{
									Sdbuf_Ifis[5] = 0xe0|(0x1f&M_St);  //M_St
									Sdbuf_Ifis[sdbuf_count] = 2;//确认消息
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = Data_ID[0];
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 5;
									sdbuf_count++;
								}
							}
							else
							{
								//Data_ID[0] = revbuf_zhongyou[11];
								OIL_BASIN_count = (LB_ID - 1) * 2;
								qDebug()<<Data_ID[0]<<Data_ID[1]<<Data_ID[2];
								if((Data_ID[0] == 0x02)||(!Data_ID[0])||(data_x2 == 1))
								{
									Sdbuf_Ifis[sdbuf_count] = 0x02;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x02;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] =  OIL_BASIN_TEMP[OIL_BASIN_count] & 0x0f;
									type_basin = OIL_BASIN_TEMP[OIL_BASIN_count]>>6;
									sdbuf_count++;
									OIL_BASIN_count++;
									Sdbuf_Ifis[sdbuf_count] = OIL_BASIN_TEMP[OIL_BASIN_count]&0x7f;  //&0111 1111
									sdbuf_count++;
									OIL_BASIN_count++;
								}
								if((Data_ID[0] == 0x03)||(!Data_ID[0])||(data_x3 == 1))
								{
									type_basin = OIL_BASIN_TEMP[OIL_BASIN_count]>>6;
									Sdbuf_Ifis[sdbuf_count] = 0x03;//测漏类型
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x01;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = type_basin; //类型为0x03，传感器方法
									sdbuf_count++;
								}
								if((Data_ID[0] == 0x04)||(!Data_ID[0])||(data_x4 == 1))
								{
									Sdbuf_Ifis[sdbuf_count] = 0x04;//编号
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x01;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = LB_ID&0x0f;
									sdbuf_count++;
								}
								//问压力值
								if((Data_ID[0] == 0x05)||(!Data_ID[0])||(data_x5 == 1))
								{
									Sdbuf_Ifis[sdbuf_count] = 0x05;//压力值
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x04;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x05;  //压力值正
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //压力值空位
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //压力值前两位
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;  //压力值后两位
									sdbuf_count++;

								}
								if((Data_ID[0] == 0x06)||(!Data_ID[0])||(data_x6 == 1))//气体浓度
								{
									Sdbuf_Ifis[sdbuf_count] = 0x06;//气体浓度
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x02;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;  //浓度高位
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //浓度低位
									sdbuf_count++;
								}

								if((Data_ID[0] == 0x10)||(!Data_ID[0])||(data_x10 == 1))//高液位报警值
								{
									Sdbuf_Ifis[sdbuf_count] = 0x10;//高液位
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x04;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;  //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x03; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
								}
								if((Data_ID[0] == 0x11)||(!Data_ID[0])||(data_x11 == 1))//高高液位报警值
								{
									Sdbuf_Ifis[sdbuf_count] = 0x11;//高高液位
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x04;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;  //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x06; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
								}
								if((Data_ID[0] == 0x12)||(!Data_ID[0])||(data_x12 == 1))//低液位报警值
								{
									Sdbuf_Ifis[sdbuf_count] = 0x12;//低液位
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x04;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;  //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x03; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
								}
								if((Data_ID[0] == 0x13)||(!Data_ID[0])||(data_x13 == 1))//低低液位报警值
								{
									Sdbuf_Ifis[sdbuf_count] = 0x13;//低低液位
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x04;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;  //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x06; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
								}
								if((Data_ID[0] == 0x14)||(!Data_ID[0])||(data_x14 == 1))//浓度预警值
								{
									Sdbuf_Ifis[sdbuf_count] = 0x14;//气体浓度
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x02;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x25;
									sdbuf_count++;
								}
								if((Data_ID[0] == 0x15)||(!Data_ID[0])||(data_x15 == 1))//浓度报警值
								{
									Sdbuf_Ifis[sdbuf_count] = 0x15;//气体浓度
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x02;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x40;
									sdbuf_count++;
								}
								if((Data_ID[0] != 0x02) && (Data_ID[0] != 0x03) && (Data_ID[0] != 0x04) && (Data_ID[0] != 0x05) && (Data_ID[0] != 0x06) &&
								        (Data_ID[0] != 0x10) && (Data_ID[0] != 0x11) && (Data_ID[0] != 0x12) && (Data_ID[0] != 0x13) && (Data_ID[0] != 0x14) &&
								        (Data_ID[0] != 0x15))
								{
									Sdbuf_Ifis[5] = 0xe0|(0x1f&M_St);  //M_St
									Sdbuf_Ifis[sdbuf_count] = 2;//确认消息
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = Data_ID[0];
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 5;
									sdbuf_count++;
								}
							}
						}
						Sdbuf_Ifis[7] = sdbuf_count - 8;

						emit net_reply(sdbuf_count);

//                        if((num_send = send(nsockfd_zhongyou,Sdbuf,sdbuf_count,0)) == -1)
//                        {
//                            printf("ERROR: Failed to sent string.\n");
//                            Flag_TcpClose_FromTcp = 1; //close(nsockfd_zhongyou);
//                            break;
//                        }
//                        else
//                        {
//                            printf("OK: Sent %d bytes sucessful.\n", num_send);

//                        }

						sdbuf_count = 10;
						Wait_Send_Flag = 0;
					}
					if(DB_Ad_Lg == 2 && DB_Ad[0] == 0x03)   //管线测漏状态数据
					{
						//Sdbuf_Ifis[0] = revbuf_zhongyou[2];
						//Sdbuf_Ifis[1] = revbuf_zhongyou[3];
						unsigned char data_x2 = 0;
						unsigned char data_x3 = 0;
						unsigned char data_x4 = 0;
						unsigned char data_x5 = 0;
						unsigned char data_x6 = 0;
						unsigned char data_x10 = 0;
						unsigned char data_x11 = 0;
						unsigned char data_x12 = 0;
						unsigned char data_x13 = 0;
						unsigned char data_x14 = 0;
						unsigned char data_x15 = 0;

						for(unsigned char i = 0;i<11;i++)
						{
							Data_ID[i] = revbuf_zhongyou[11+i];
							if(Data_ID[i]==0x02){data_x2 = 1;}
							if(Data_ID[i]==0x03){data_x3 = 1;}
							if(Data_ID[i]==0x04){data_x4 = 1;}
							if(Data_ID[i]==0x05){data_x5 = 1;}
							if(Data_ID[i]==0x06){data_x6 = 1;}
							if(Data_ID[i]==0x10){data_x10 = 1;}
							if(Data_ID[i]==0x11){data_x11 = 1;}
							if(Data_ID[i]==0x12){data_x12 = 1;}
							if(Data_ID[i]==0x13){data_x13 = 1;}
							if(Data_ID[i]==0x14){data_x14 = 1;}
							if(Data_ID[i]==0x15){data_x15 = 1;}
						}
						Sdbuf_Ifis[0] = 0x02;//把数据节点写死
						Sdbuf_Ifis[1] = 0x01;
						Sdbuf_Ifis[2] = 0x07;
						Sdbuf_Ifis[3] = ID_M;
						Sdbuf_Ifis[4] = 0;                   //Mc
						Sdbuf_Ifis[6] = 0;                   //M_Lg 高位
						Sdbuf_Ifis[8] = 2;                   //DB_Ad_Lg
						Sdbuf_Ifis[9] = 0x03;                //DB_Ad
						Sdbuf_Ifis[sdbuf_count] = revbuf_zhongyou[10];  //DB_Ad
						sdbuf_count++;
						int LP_ID = DB_Ad[1] & 0x0f;
						int OIL_PIPE_count = 0;
						unsigned char type_pipe = 0;
						Sdbuf_Ifis[5] = 0x20|(0x1f & M_St);  //M_St
						if(!LP_ID)
						{
							LP_ID = count_pipe;

							for(int i = 0;i < LP_ID;i++)
							{
								Sdbuf_Ifis[sdbuf_count] = 0x02;    //ID
								sdbuf_count++;
								Sdbuf_Ifis[sdbuf_count] = 0x02;    //LONG
								sdbuf_count++;
								Sdbuf_Ifis[sdbuf_count] = OIL_PIPE_TEMP[OIL_PIPE_count]&0x0f;
								type_pipe = OIL_PIPE_TEMP[OIL_PIPE_count]>>6;
								sdbuf_count++;
								OIL_PIPE_count++;
								Sdbuf_Ifis[sdbuf_count] = OIL_PIPE_TEMP[OIL_PIPE_count]&0x7f;  //&0111 1111;
								sdbuf_count++;
								OIL_PIPE_count++;
								Sdbuf_Ifis[sdbuf_count] = 0x03;//测漏类型
								sdbuf_count++;
								Sdbuf_Ifis[sdbuf_count] = 0x01;
								sdbuf_count++;
								Sdbuf_Ifis[sdbuf_count] = type_pipe;
								sdbuf_count++;
								Sdbuf_Ifis[sdbuf_count] = 0x04;//编号
								sdbuf_count++;
								Sdbuf_Ifis[sdbuf_count] = 0x01;
								sdbuf_count++;
								Sdbuf_Ifis[sdbuf_count] = i+1;
								sdbuf_count++;
							}
						}
						else
						{
							if(LP_ID>count_pipe)
							{
								Data_ID[0] = revbuf_zhongyou[11];
								if((Data_ID[0] == 0x02)||(!Data_ID[0]))
								{
									Sdbuf_Ifis[sdbuf_count] = 0x02;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x02;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x10;
									type_pipe = OIL_PIPE_TEMP[OIL_PIPE_count]>>6;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;
									sdbuf_count++;
								}
								if((Data_ID[0] == 0x03)||(!Data_ID[0]))
								{
									type_pipe = OIL_TANK_TEMP[OIL_PIPE_count]>>6;
									Sdbuf_Ifis[sdbuf_count] = 0x03;//其他类型
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x01;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x04;//未设置传感器类型为其他
									sdbuf_count++;
								}
								if((Data_ID[0] == 0x04)||(!Data_ID[0]))
								{
									Sdbuf_Ifis[sdbuf_count] = 0x04;//编号
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x01;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = LP_ID&0x0f;
									sdbuf_count++;
								}
								//问压力值
								if((Data_ID[0] == 0x05)||(!Data_ID[0]))
								{
									Sdbuf_Ifis[sdbuf_count] = 0x05;//压力值
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x04;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x05;  //压力值正
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //压力值空位
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //压力值前两位
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;  //压力值后两位
									sdbuf_count++;

								}
								if((Data_ID[0] == 0x06)||(!Data_ID[0]))//气体浓度
								{
									Sdbuf_Ifis[sdbuf_count] = 0x06;//气体浓度
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x02;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;  //浓度高位
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //浓度低位
									sdbuf_count++;
								}

								if((Data_ID[0] == 0x10)||(!Data_ID[0]))//高液位报警值
								{
									Sdbuf_Ifis[sdbuf_count] = 0x10;//高液位
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x04;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;  //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x03; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
								}
								if((Data_ID[0] == 0x11)||(!Data_ID[0]))//高高液位报警值
								{
									Sdbuf_Ifis[sdbuf_count] = 0x11;//高高液位
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x04;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;  //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x06; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
								}
								if((Data_ID[0] == 0x12)||(!Data_ID[0]))//低液位报警值
								{
									Sdbuf_Ifis[sdbuf_count] = 0x12;//低液位
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x04;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;  //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x03; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
								}
								if((Data_ID[0] == 0x13)||(!Data_ID[0]))//低低液位报警值
								{
									Sdbuf_Ifis[sdbuf_count] = 0x13;//低低液位
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x04;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;  //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x06; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
								}
								if((Data_ID[0] == 0x14)||(!Data_ID[0]))//浓度预警值
								{
									Sdbuf_Ifis[sdbuf_count] = 0x14;//气体浓度
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x02;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x25;
									sdbuf_count++;
								}
								if((Data_ID[0] == 0x15)||(!Data_ID[0]))//浓度报警值
								{
									Sdbuf_Ifis[sdbuf_count] = 0x15;//气体浓度
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x02;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x40;
									sdbuf_count++;
								}
								if((Data_ID[0] != 0x02) && (Data_ID[0] != 0x03) && (Data_ID[0] != 0x04) && (Data_ID[0] != 0x05) && (Data_ID[0] != 0x06) &&
								        (Data_ID[0] != 0x10) && (Data_ID[0] != 0x11) && (Data_ID[0] != 0x12) && (Data_ID[0] != 0x13) && (Data_ID[0] != 0x14) &&
								        (Data_ID[0] != 0x15))
								{
									Sdbuf_Ifis[5] = 0xe0|(0x1f&M_St);  //M_St
									Sdbuf_Ifis[sdbuf_count] = 2;//确认消息
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = Data_ID[0];
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 5;
									sdbuf_count++;
								}
							}
							else
							{
								Data_ID[0] = revbuf_zhongyou[11];
								OIL_PIPE_count = (LP_ID - 1)*2;
								if((Data_ID[0] == 0x02)||(!Data_ID[0])||(data_x2 == 1))
								{
									Sdbuf_Ifis[sdbuf_count] = 0x02;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x02;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = OIL_PIPE_TEMP[OIL_PIPE_count] & 0x0f;
									type_pipe = OIL_PIPE_TEMP[OIL_PIPE_count]>>6;
									sdbuf_count++;
									OIL_PIPE_count++;
									Sdbuf_Ifis[sdbuf_count] = OIL_PIPE_TEMP[OIL_PIPE_count]&0x7f;  //&0111 1111;
									sdbuf_count++;
									OIL_PIPE_count++;
								}
								if((Data_ID[0] == 0x03)||(!Data_ID[0])||(data_x3 == 1))
								{
									type_pipe = OIL_PIPE_TEMP[OIL_PIPE_count]>>6;
									Sdbuf_Ifis[sdbuf_count] = 0x03;//测漏类型
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x01;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = type_pipe;
									sdbuf_count++;
								}
								if((Data_ID[0] == 0x04)||(!Data_ID[0])||(data_x4 == 1))
								{
									Sdbuf_Ifis[sdbuf_count] = 0x04;//编号
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x01;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = LP_ID&0x0f;
									sdbuf_count++;
								}
								//问压力值
								if((Data_ID[0] == 0x05)||(!Data_ID[0])||(data_x5 == 1))
								{
									Sdbuf_Ifis[sdbuf_count] = 0x05;//压力值
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x04;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x05;  //压力值正
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //压力值空位
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //压力值前两位
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;  //压力值后两位
									sdbuf_count++;

								}
								if((Data_ID[0] == 0x06)||(!Data_ID[0])||(data_x6 == 1))//气体浓度
								{
									Sdbuf_Ifis[sdbuf_count] = 0x06;//气体浓度
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x02;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;  //浓度高位
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //浓度低位
									sdbuf_count++;
								}
								if((Data_ID[0] == 0x10)||(!Data_ID[0])||(data_x10 == 1))//高液位报警值
								{
									Sdbuf_Ifis[sdbuf_count] = 0x10;//高液位
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x04;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;  //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x03; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
								}
								if((Data_ID[0] == 0x11)||(!Data_ID[0])||(data_x11 == 1))//高高液位报警值
								{
									Sdbuf_Ifis[sdbuf_count] = 0x11;//高高液位
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x04;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;  //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x06; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
								}
								if((Data_ID[0] == 0x12)||(!Data_ID[0])||(data_x12 == 1))//低液位报警值
								{
									Sdbuf_Ifis[sdbuf_count] = 0x12;//低液位
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x04;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;  //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x03; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
								}
								if((Data_ID[0] == 0x13)||(!Data_ID[0])||(data_x13 == 1))//低低液位报警值
								{
									Sdbuf_Ifis[sdbuf_count] = 0x13;//低低液位
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x04;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;  //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x06; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
								}
								if((Data_ID[0] == 0x14)||(!Data_ID[0])||(data_x14 == 1))//浓度预警值
								{
									Sdbuf_Ifis[sdbuf_count] = 0x14;//气体浓度
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x02;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x25;
									sdbuf_count++;
								}
								if((Data_ID[0] == 0x15)||(!Data_ID[0])||(data_x15 == 1))//浓度报警值
								{
									Sdbuf_Ifis[sdbuf_count] = 0x15;//气体浓度
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x02;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x40;
									sdbuf_count++;
								}
								if((Data_ID[0] != 0x02) && (Data_ID[0] != 0x03) && (Data_ID[0] != 0x04) && (Data_ID[0] != 0x05) && (Data_ID[0] != 0x06) &&
								        (Data_ID[0] != 0x10) && (Data_ID[0] != 0x11) && (Data_ID[0] != 0x12) && (Data_ID[0] != 0x13) && (Data_ID[0] != 0x14) &&
								        (Data_ID[0] != 0x15))
								{
									Sdbuf_Ifis[5] = 0xe0|(0x1f&M_St);  //M_St
									Sdbuf_Ifis[sdbuf_count] = 2;//确认消息
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = Data_ID[0];
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 5;
									sdbuf_count++;
								}
							}
						}

						Sdbuf_Ifis[7] = sdbuf_count - 8;     //M_Lg

						emit net_reply(sdbuf_count);

//                        if((num_send = send(nsockfd_zhongyou,Sdbuf,sdbuf_count,0)) == -1)
//                        {
//                            printf("ERROR: Failed to sent string.\n");
//                            Flag_TcpClose_FromTcp = 1; //close(nsockfd_zhongyou);
//                            break;
//                        }

//                        else
//                        {
//                            printf("OK: Sent %d bytes sucessful.\n", num_send);

//                        }
						sdbuf_count = 10;
						Wait_Send_Flag = 0;
					}
					if(DB_Ad_Lg == 2 && DB_Ad[0] == 0x04)//油罐测漏状态数据
					{
						unsigned char data_x2 = 0;
						unsigned char data_x3 = 0;
						unsigned char data_x4 = 0;
						unsigned char data_x5 = 0;
						unsigned char data_x6 = 0;
						unsigned char data_x10 = 0;
						unsigned char data_x11 = 0;
						unsigned char data_x12 = 0;
						unsigned char data_x13 = 0;
						unsigned char data_x14 = 0;
						unsigned char data_x15 = 0;

						for(unsigned char i = 0;i<11;i++)
						{
							Data_ID[i] = revbuf_zhongyou[11+i];
							if(Data_ID[i]==0x02){data_x2 = 1;}
							if(Data_ID[i]==0x03){data_x3 = 1;}
							if(Data_ID[i]==0x04){data_x4 = 1;}
							if(Data_ID[i]==0x05){data_x5 = 1;}
							if(Data_ID[i]==0x06){data_x6 = 1;}
							if(Data_ID[i]==0x10){data_x10 = 1;}
							if(Data_ID[i]==0x11){data_x11 = 1;}
							if(Data_ID[i]==0x12){data_x12 = 1;}
							if(Data_ID[i]==0x13){data_x13 = 1;}
							if(Data_ID[i]==0x14){data_x14 = 1;}
							if(Data_ID[i]==0x15){data_x15 = 1;}
						}
						//Sdbuf_Ifis[0] = revbuf_zhongyou[2];
						//Sdbuf_Ifis[1] = revbuf_zhongyou[3];
						Sdbuf_Ifis[0] = 0x02;//把数据节点写死
						Sdbuf_Ifis[1] = 0x01;
						Sdbuf_Ifis[2] = 0x07;
						Sdbuf_Ifis[3] = ID_M;
						Sdbuf_Ifis[4] = 0; //Mc
						Sdbuf_Ifis[6] = 0; //M_Lg 高位
						Sdbuf_Ifis[8] = 2; //DB_Ad_Lg
						Sdbuf_Ifis[9] = 0x04; //DB_Ad
						Sdbuf_Ifis[sdbuf_count] = revbuf_zhongyou[10];//DB_Ad
						sdbuf_count++;
						int LT_ID = DB_Ad[1]&0x0f;
						int OIL_TANK_count = 0;
						unsigned char type_tank = 0;
						Sdbuf_Ifis[5] = 0x20|(0x1f&M_St);  //M_St
						if(!LT_ID)
						{
							LT_ID = count_tank;

							for(int i = 0;i < LT_ID;i++)
							{
								Sdbuf_Ifis[sdbuf_count] = 0x02;    //ID
								sdbuf_count++;
								Sdbuf_Ifis[sdbuf_count] = 0x02;    //LONG
								sdbuf_count++;
								Sdbuf_Ifis[sdbuf_count] = OIL_TANK_TEMP[OIL_TANK_count] & 0x0f;
								type_tank = OIL_TANK_TEMP[OIL_TANK_count]>>6;
								sdbuf_count++;
								OIL_TANK_count++;
								Sdbuf_Ifis[sdbuf_count] = OIL_TANK_TEMP[OIL_TANK_count]&0x7f;  //&0111 1111;
								sdbuf_count++;
								OIL_TANK_count++;
								Sdbuf_Ifis[sdbuf_count] = 0x03;//测漏类型
								sdbuf_count++;
								Sdbuf_Ifis[sdbuf_count] = 0x01;
								sdbuf_count++;
								Sdbuf_Ifis[sdbuf_count] = type_tank;
								sdbuf_count++;
								Sdbuf_Ifis[sdbuf_count] = 0x04;//编号
								sdbuf_count++;
								Sdbuf_Ifis[sdbuf_count] = 0x01;
								sdbuf_count++;
								Sdbuf_Ifis[sdbuf_count] = i+1;
								sdbuf_count++;
							}
						}
						else
						{
							if(LT_ID>count_tank)
							{
								Data_ID[0] = revbuf_zhongyou[11];//1125
								if((Data_ID[0] == 0x02)||(!Data_ID[0]))
								{
									Sdbuf_Ifis[sdbuf_count] = 0x02;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x02;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x10;
									type_tank = OIL_TANK_TEMP[OIL_TANK_count]>>6;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;
									sdbuf_count++;
								}
								if((Data_ID[0] == 0x03)||(!Data_ID[0]))
								{
									type_tank = OIL_TANK_TEMP[OIL_TANK_count]>>6;
									Sdbuf_Ifis[sdbuf_count] = 0x03;//测漏类型
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x01;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x04;//未设置传感器类型为其他
									sdbuf_count++;
								}
								if((Data_ID[0] == 0x04)||(!Data_ID[0]))
								{
									Sdbuf_Ifis[sdbuf_count] = 0x04;//编号
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x01;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = LT_ID&0x0f;
									sdbuf_count++;
								}
								//问压力值
								if((Data_ID[0] == 0x05)||(!Data_ID[0]))
								{
									Sdbuf_Ifis[sdbuf_count] = 0x05;//压力值
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x04;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x05;  //压力值正
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //压力值空位
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //压力值前两位
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;  //压力值后两位
									sdbuf_count++;

								}
								if((Data_ID[0] == 0x06)||(!Data_ID[0]))//气体浓度
								{
									Sdbuf_Ifis[sdbuf_count] = 0x06;//气体浓度
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x02;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;  //浓度高位
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //浓度低位
									sdbuf_count++;
								}

								if((Data_ID[0] == 0x10)||(!Data_ID[0]))//高液位报警值
								{
									Sdbuf_Ifis[sdbuf_count] = 0x10;//高液位
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x04;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;  //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x03; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
								}
								if((Data_ID[0] == 0x11)||(!Data_ID[0]))//高高液位报警值
								{
									Sdbuf_Ifis[sdbuf_count] = 0x11;//高高液位
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x04;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;  //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x06; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
								}
								if((Data_ID[0] == 0x12)||(!Data_ID[0]))//低液位报警值
								{
									Sdbuf_Ifis[sdbuf_count] = 0x12;//低液位
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x04;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;  //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x03; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
								}
								if((Data_ID[0] == 0x13)||(!Data_ID[0]))//低低液位报警值
								{
									Sdbuf_Ifis[sdbuf_count] = 0x13;//低低液位
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x04;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;  //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x06; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
								}
								if((Data_ID[0] == 0x14)||(!Data_ID[0]))//浓度预警值
								{
									Sdbuf_Ifis[sdbuf_count] = 0x14;//气体浓度
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x02;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x25;
									sdbuf_count++;
								}
								if((Data_ID[0] == 0x15)||(!Data_ID[0]))//浓度报警值
								{
									Sdbuf_Ifis[sdbuf_count] = 0x15;//气体浓度
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x02;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x40;
									sdbuf_count++;
								}
								if((Data_ID[0] != 0x02) && (Data_ID[0] != 0x03) && (Data_ID[0] != 0x04) && (Data_ID[0] != 0x05) && (Data_ID[0] != 0x06) &&
								        (Data_ID[0] != 0x10) && (Data_ID[0] != 0x11) && (Data_ID[0] != 0x12) && (Data_ID[0] != 0x13) && (Data_ID[0] != 0x14) &&
								        (Data_ID[0] != 0x15))
								{
									Sdbuf_Ifis[5] = 0xe0|(0x1f&M_St);  //M_St
									Sdbuf_Ifis[sdbuf_count] = 2;//确认消息
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = Data_ID[0];
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 5;
									sdbuf_count++;
								}
							}
							else
							{
								Data_ID[0] = revbuf_zhongyou[11];//1125
								OIL_TANK_count = (LT_ID - 1)*2;
								if((Data_ID[0] == 0x02)||(!Data_ID[0])||(data_x2 == 1))
								{
									Sdbuf_Ifis[sdbuf_count] = 0x02;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x02;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = OIL_TANK_TEMP[OIL_TANK_count] & 0x0f;
									type_tank = OIL_TANK_TEMP[OIL_TANK_count]>>6;
									sdbuf_count++;
									OIL_TANK_count++;
									Sdbuf_Ifis[sdbuf_count] = OIL_TANK_TEMP[OIL_TANK_count]&0x7f;  //&0111 1111;
									sdbuf_count++;
									OIL_TANK_count++;
								}
								if((Data_ID[0] == 0x03)||(!Data_ID[0])||(data_x3 == 1))
								{
									type_tank = OIL_TANK_TEMP[OIL_TANK_count]>>6;
									Sdbuf_Ifis[sdbuf_count] = 0x03;//测漏类型
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x01;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = type_tank;
									sdbuf_count++;
								}
								if((Data_ID[0] == 0x04)||(!Data_ID[0])||(data_x4 == 1))
								{
									Sdbuf_Ifis[sdbuf_count] = 0x04;//编号
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x01;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = LT_ID&0x0f;
									sdbuf_count++;
								}
								//问压力值
								if((Data_ID[0] == 0x05)||(!Data_ID[0])||(data_x5 == 1))
								{
									Sdbuf_Ifis[sdbuf_count] = 0x05;//压力值
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x04;
									sdbuf_count++;

									//LT_ID
									//if(count_Pressure[LT_ID-1]<0)
									//{
									    Sdbuf_Ifis[sdbuf_count] = 0x85;  //压力值负
										sdbuf_count++;
										//}
										//else
										//{
										//Sdbuf_Ifis[sdbuf_count] = 0x05;  //压力值正
										//sdbuf_count++;
										//}
										Sdbuf_Ifis[sdbuf_count] = 0x00;
									sdbuf_count++;
									//int sdpress_h = int(count_Pressure[LT_ID-1])/10; //高两位
									int sdpress_h = 0; //高两位
									sdpress_h = abs(sdpress_h);
									//int sdpress_l = int(count_Pressure[LT_ID-1]*10)%100; //低两位
									int sdpress_l = 0; //低两位
									sdpress_l = abs(sdpress_l);
									printf("*******%d***********%d**************",sdpress_h,sdpress_l);
									int a_h = sdpress_h/10;
									int hex_h = sdpress_h + a_h*6;
									int a_l = sdpress_l/10;
									int hex_l = sdpress_l + a_l*6;

									if(sdpress_h>0)
										Sdbuf_Ifis[sdbuf_count] = hex_h; //压力值前两位
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = hex_l;  //压力值后两位
									sdbuf_count++;

								}
								if((Data_ID[0] == 0x06)||(!Data_ID[0])||(data_x6 == 1))//气体浓度
								{
									Sdbuf_Ifis[sdbuf_count] = 0x06;//气体浓度
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x02;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;  //浓度高位
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //浓度低位
									sdbuf_count++;
								}

								if((Data_ID[0] == 0x10)||(!Data_ID[0])||(data_x10 == 1))//高液位报警值
								{
									Sdbuf_Ifis[sdbuf_count] = 0x10;//高液位
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x04;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;  //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x03; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
								}
								if((Data_ID[0] == 0x11)||(!Data_ID[0])||(data_x11 == 1))//高高液位报警值
								{
									Sdbuf_Ifis[sdbuf_count] = 0x11;//高高液位
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x04;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;  //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x06; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
								}
								if((Data_ID[0] == 0x12)||(!Data_ID[0])||(data_x12 == 1))//低液位报警值
								{
									Sdbuf_Ifis[sdbuf_count] = 0x12;//低液位
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x04;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;  //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x03; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
								}
								if((Data_ID[0] == 0x13)||(!Data_ID[0])||(data_x13 == 1))//低低液位报警值
								{
									Sdbuf_Ifis[sdbuf_count] = 0x13;//低低液位
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x04;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;  //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x06; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
								}
								if((Data_ID[0] == 0x14)||(!Data_ID[0])||(data_x14 == 1))//浓度预警值
								{
									Sdbuf_Ifis[sdbuf_count] = 0x14;//气体浓度
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x02;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x25;
									sdbuf_count++;
								}
								if((Data_ID[0] == 0x15)||(!Data_ID[0])||(data_x15 == 1))//浓度报警值
								{
									Sdbuf_Ifis[sdbuf_count] = 0x15;//气体浓度
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x02;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x40;
									sdbuf_count++;
								}
								if((Data_ID[0] != 0x02) && (Data_ID[0] != 0x03) && (Data_ID[0] != 0x04) && (Data_ID[0] != 0x05) && (Data_ID[0] != 0x06) &&
								        (Data_ID[0] != 0x10) && (Data_ID[0] != 0x11) && (Data_ID[0] != 0x12) && (Data_ID[0] != 0x13) && (Data_ID[0] != 0x14) &&
								        (Data_ID[0] != 0x15))
								{
									Sdbuf_Ifis[5] = 0xe0|(0x1f&M_St);  //M_St
									Sdbuf_Ifis[sdbuf_count] = 2;//确认消息
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = Data_ID[0];
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 5;
									sdbuf_count++;
								}
							}
						}

						Sdbuf_Ifis[7] = sdbuf_count-8;     //M_Lg

						emit net_reply(sdbuf_count);

//                        if((num_send = send(nsockfd_zhongyou,Sdbuf,sdbuf_count,0)) == -1)
//                        {
//                            printf("ERROR: Failed to sent string.\n");
//                            Flag_TcpClose_FromTcp = 1; //close(nsockfd_zhongyou);
//                            break;
//                        }
//                        else
//                        {
//                            printf("OK: Sent %d bytes sucessful.\n", num_send);

//                        }

						sdbuf_count = 10;
						Wait_Send_Flag = 0;
					}
					if(DB_Ad_Lg == 2 && DB_Ad[0] == 0x05) //加油机底槽测漏状态数据
					{
						unsigned char data_x2 = 0;
						unsigned char data_x3 = 0;
						unsigned char data_x4 = 0;
						unsigned char data_x5 = 0;
						unsigned char data_x6 = 0;
						unsigned char data_x10 = 0;
						unsigned char data_x11 = 0;
						unsigned char data_x12 = 0;
						unsigned char data_x13 = 0;
						unsigned char data_x14 = 0;
						unsigned char data_x15 = 0;

						for(unsigned char i = 0;i<11;i++)
						{
							Data_ID[i] = revbuf_zhongyou[11+i];
							if(Data_ID[i]==0x02){data_x2 = 1;}
							if(Data_ID[i]==0x03){data_x3 = 1;}
							if(Data_ID[i]==0x04){data_x4 = 1;}
							if(Data_ID[i]==0x05){data_x5 = 1;}
							if(Data_ID[i]==0x06){data_x6 = 1;}
							if(Data_ID[i]==0x10){data_x10 = 1;}
							if(Data_ID[i]==0x11){data_x11 = 1;}
							if(Data_ID[i]==0x12){data_x12 = 1;}
							if(Data_ID[i]==0x13){data_x13 = 1;}
							if(Data_ID[i]==0x14){data_x14 = 1;}
							if(Data_ID[i]==0x15){data_x15 = 1;}
						}
						//Sdbuf_Ifis[0] = revbuf_zhongyou[2];
						//Sdbuf_Ifis[1] = revbuf_zhongyou[3];
						Sdbuf_Ifis[0] = 0x02;//把数据节点写死
						Sdbuf_Ifis[1] = 0x01;
						Sdbuf_Ifis[2] = 0x07;
						Sdbuf_Ifis[3] = ID_M;
						Sdbuf_Ifis[4] = 0; //Mc
						Sdbuf_Ifis[6] = 0; //M_Lg 高位
						Sdbuf_Ifis[8] = 2; //DB_Ad_Lg
						Sdbuf_Ifis[9] = 0x05; //DB_Ad
						Sdbuf_Ifis[sdbuf_count] = revbuf_zhongyou[10];//DB_Ad
						sdbuf_count++;
						int LD_ID = DB_Ad[1]&0x0f;
						int OIL_DISPENER_count = 0;
						unsigned char type_dispener;
						if(!LD_ID)
						{
							LD_ID = count_dispener;

							for(int i = 0;i < LD_ID;i++)
							{
								Sdbuf_Ifis[sdbuf_count] = 0x02;
								sdbuf_count++;
								Sdbuf_Ifis[sdbuf_count] = 0x02;
								sdbuf_count++;
								Sdbuf_Ifis[sdbuf_count] = OIL_DISPENER_TEMP[OIL_DISPENER_count]&0x0f;
								type_dispener = OIL_DISPENER_TEMP[OIL_DISPENER_count]>>6;
								sdbuf_count++;
								OIL_DISPENER_count++;
								Sdbuf_Ifis[sdbuf_count] = OIL_DISPENER_TEMP[OIL_DISPENER_count]&0x7f;  //&0111 1111;
								sdbuf_count++;
								OIL_DISPENER_count++;
								Sdbuf_Ifis[sdbuf_count] = 0x03;//测漏类型
								sdbuf_count++;
								Sdbuf_Ifis[sdbuf_count] = 0x01;
								sdbuf_count++;
								Sdbuf_Ifis[sdbuf_count] = type_dispener;
								sdbuf_count++;
								Sdbuf_Ifis[sdbuf_count] = 0x04;//编号
								sdbuf_count++;
								Sdbuf_Ifis[sdbuf_count] = 0x01;
								sdbuf_count++;
								Sdbuf_Ifis[sdbuf_count] = i+1;
								sdbuf_count++;

							}
						}
						else
						{
							if(LD_ID>count_dispener)
							{
								Data_ID[0] = revbuf_zhongyou[11];//1125
								if((Data_ID[0] == 0x02)||(!Data_ID[0]))
								{
									Sdbuf_Ifis[sdbuf_count] = 0x02;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x02;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x10;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;
									sdbuf_count++;
								}
								if((Data_ID[0] == 0x03)||(!Data_ID[0]))
								{
									Sdbuf_Ifis[sdbuf_count] = 0x03;//测漏类型
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x01;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x04; //其他类型
									sdbuf_count++;
								}
								if((Data_ID[0] == 0x04)||(!Data_ID[0]))
								{
									Sdbuf_Ifis[sdbuf_count] = 0x04;//编号
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x01;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = LD_ID&0x0f;
									sdbuf_count++;
								}
								//问压力值
								if((Data_ID[0] == 0x05)||(!Data_ID[0]))
								{
									Sdbuf_Ifis[sdbuf_count] = 0x05;//压力值
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x04;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x05;  //压力值正
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //压力值空位
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //压力值前两位
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;  //压力值后两位
									sdbuf_count++;

								}
								if((Data_ID[0] == 0x06)||(!Data_ID[0]))//气体浓度
								{
									Sdbuf_Ifis[sdbuf_count] = 0x06;//气体浓度
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x02;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;  //浓度高位
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //浓度低位
									sdbuf_count++;
								}

								if((Data_ID[0] == 0x10)||(!Data_ID[0]))//高液位报警值
								{
									Sdbuf_Ifis[sdbuf_count] = 0x10;//高液位
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x04;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;  //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x03; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
								}
								if((Data_ID[0] == 0x11)||(!Data_ID[0]))//高高液位报警值
								{
									Sdbuf_Ifis[sdbuf_count] = 0x11;//高高液位
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x04;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;  //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x06; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
								}
								if((Data_ID[0] == 0x12)||(!Data_ID[0]))//低液位报警值
								{
									Sdbuf_Ifis[sdbuf_count] = 0x12;//低液位
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x04;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;  //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x03; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
								}
								if((Data_ID[0] == 0x13)||(!Data_ID[0]))//低低液位报警值
								{
									Sdbuf_Ifis[sdbuf_count] = 0x13;//低低液位
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x04;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;  //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x06; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
								}
								if((Data_ID[0] == 0x14)||(!Data_ID[0]))//浓度预警值
								{
									Sdbuf_Ifis[sdbuf_count] = 0x14;//气体浓度
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x02;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x25;
									sdbuf_count++;
								}
								if((Data_ID[0] == 0x15)||(!Data_ID[0]))//浓度报警值
								{
									Sdbuf_Ifis[sdbuf_count] = 0x15;//气体浓度
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x02;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x40;
									sdbuf_count++;
								}
								if((Data_ID[0] != 0x02) && (Data_ID[0] != 0x03) && (Data_ID[0] != 0x04) && (Data_ID[0] != 0x05) && (Data_ID[0] != 0x06) &&
								        (Data_ID[0] != 0x10) && (Data_ID[0] != 0x11) && (Data_ID[0] != 0x12) && (Data_ID[0] != 0x13) && (Data_ID[0] != 0x14) &&
								        (Data_ID[0] != 0x15))
								{
									Sdbuf_Ifis[5] = 0xe0|(0x1f&M_St);  //M_St
									Sdbuf_Ifis[sdbuf_count] = 2;//确认消息
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = Data_ID[0];
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 5;
									sdbuf_count++;
								}
							}
							else
							{
								Data_ID[0] = revbuf_zhongyou[11];//1125
								OIL_DISPENER_count = (LD_ID-1)*2;
								if((Data_ID[0] == 0x02)||(!Data_ID[0])||(data_x2 == 1))
								{
									Sdbuf_Ifis[sdbuf_count] = 0x02;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x02;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = OIL_DISPENER_TEMP[OIL_DISPENER_count]&0x0f;
									type_dispener = OIL_DISPENER_TEMP[OIL_DISPENER_count]>>6;
									sdbuf_count++;
									OIL_DISPENER_count++;
									Sdbuf_Ifis[sdbuf_count] = OIL_DISPENER_TEMP[OIL_DISPENER_count]&0x7f;  //&0111 1111;
									sdbuf_count++;
									OIL_DISPENER_count++;
								}
								if((Data_ID[0] == 0x03)||(!Data_ID[0])||(data_x3 == 1))
								{
									type_dispener = OIL_DISPENER_TEMP[OIL_DISPENER_count]>>6;
									Sdbuf_Ifis[sdbuf_count] = 0x03;//测漏类型
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x01;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = type_dispener; //固定类型
									sdbuf_count++;
								}
								if((Data_ID[0] == 0x04)||(!Data_ID[0])||(data_x4 == 1))
								{
									Sdbuf_Ifis[sdbuf_count] = 0x04;//编号
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x01;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = LD_ID&0x0f;
									sdbuf_count++;
								}
								//问压力值
								if((Data_ID[0] == 0x05)||(!Data_ID[0])||(data_x5 == 1))
								{
									Sdbuf_Ifis[sdbuf_count] = 0x05;//压力值
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x04;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x05;  //压力值正
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //压力值空位
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //压力值前两位
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;  //压力值后两位
									sdbuf_count++;

								}
								if((Data_ID[0] == 0x06)||(!Data_ID[0])||(data_x6 == 1))//气体浓度
								{
									Sdbuf_Ifis[sdbuf_count] = 0x06;//气体浓度
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x02;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;  //浓度高位
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //浓度低位
									sdbuf_count++;
								}

								if((Data_ID[0] == 0x10)||(!Data_ID[0])||(data_x10 == 1))//高液位报警值
								{
									Sdbuf_Ifis[sdbuf_count] = 0x10;//高液位
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x04;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;  //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x03; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
								}
								if((Data_ID[0] == 0x11)||(!Data_ID[0])||(data_x11 == 1))//高高液位报警值
								{
									Sdbuf_Ifis[sdbuf_count] = 0x11;//高高液位
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x04;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;  //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x06; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
								}
								if((Data_ID[0] == 0x12)||(!Data_ID[0])||(data_x12 == 1))//低液位报警值
								{
									Sdbuf_Ifis[sdbuf_count] = 0x12;//低液位
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x04;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;  //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x03; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
								}
								if((Data_ID[0] == 0x13)||(!Data_ID[0])||(data_x13 == 1))//低低液位报警值
								{
									Sdbuf_Ifis[sdbuf_count] = 0x13;//低低液位
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x04;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;  //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x06; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00; //
									sdbuf_count++;
								}
								if((Data_ID[0] == 0x14)||(!Data_ID[0])||(data_x14 == 1))//浓度预警值
								{
									Sdbuf_Ifis[sdbuf_count] = 0x14;//气体浓度
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x02;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x25;
									sdbuf_count++;
								}
								if((Data_ID[0] == 0x15)||(!Data_ID[0])||(data_x15 == 1))//浓度报警值
								{
									Sdbuf_Ifis[sdbuf_count] = 0x15;//气体浓度
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x02;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x00;
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 0x40;
									sdbuf_count++;
								}
								if((Data_ID[0] != 0x02) && (Data_ID[0] != 0x03) && (Data_ID[0] != 0x04) && (Data_ID[0] != 0x05) && (Data_ID[0] != 0x06) &&
								        (Data_ID[0] != 0x10) && (Data_ID[0] != 0x11) && (Data_ID[0] != 0x12) && (Data_ID[0] != 0x13) && (Data_ID[0] != 0x14) &&
								        (Data_ID[0] != 0x15))
								{
									Sdbuf_Ifis[5] = 0xe0|(0x1f&M_St);  //M_St
									Sdbuf_Ifis[sdbuf_count] = 2;//确认消息
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = Data_ID[0];
									sdbuf_count++;
									Sdbuf_Ifis[sdbuf_count] = 5;
									sdbuf_count++;
								}
							}
						}
						Sdbuf_Ifis[7] = sdbuf_count - 8;

						emit net_reply(sdbuf_count);

//                        if((num_send = send(nsockfd_zhongyou,Sdbuf,sdbuf_count,0)) == -1)
//                        {
//                            printf("ERROR: Failed to sent string.\n");
//                            Flag_TcpClose_FromTcp = 1; //close(nsockfd_zhongyou);
//                            break;
//                        }
//                        else
//                        {
//                            printf("OK: Sent %d bytes sucessful.\n", num_send);

//                        }

						sdbuf_count = 10;
						Wait_Send_Flag = 0;
					}
					Wait_Send_Flag = 0;
				}
			}
			Wait_Send_Flag = 0;
		}
		//return 0;

}


//udp相关


//unsigned char net_state = 0;
void sig_alarm_zy()
{
	qDebug()<<"off line!!!!!!!!!!!!!!";
	receiveudp = 0;
	add_value_netinfo("udp心跳已断开");
}

/*心跳发送*/
void* talk_udp_zy(void*)
{
	int count = 0;
	int count_history  = 0;

	int sockfd_udp;                     // udp通信设备号
	int num_udprecv;                    // Counter of received bytes
	struct sockaddr_in addr_remote;    	// Host address information本机地址信息


	/* Get the Socket file descriptor */
	if ((sockfd_udp = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		printf("ERROR: Failed to obtain Socket Descriptor!\n");
		return (0);
	}
	const int opt = 1;
	int nb=0;
	if((nb=setsockopt(sockfd_udp,SOL_SOCKET,SO_BROADCAST,(char*)&opt,sizeof(opt)))==-1)
	{
		printf("set udptalk socket error!\n");
	}

	/* Fill the socket address struct */
	addr_remote.sin_family = AF_INET;          		// Protocol Family
	addr_remote.sin_port = htons(PORT_UDP);//0x9e0d;//htons(PORT_UDP);   // Port number
  //  addr_remote.sin_addr.s_addr=htonl(INADDR_BROADCAST);
	inet_pton(AF_INET, IP_BRO, &addr_remote.sin_addr); 	// Net Address
	memset (addr_remote.sin_zero,0,8);

	add_value_netinfo("UDP客户端广播中");
	char sdbuf[10]={(char)ipa,(char)ipb,(char)ipc,(char)ipd,0x0d,0x9f,0x07,(char)ID_M,0x01,UDP_STATE_ZY};
	while(1)
	{
		sleep(heartbeat_time);      //3486端口设置
		sdbuf[0] = (char)ipa;
		sdbuf[1] = (char)ipb;
		sdbuf[2] = (char)ipc;
		sdbuf[3] = (char)ipd;
		sdbuf[4] = PORT_TCP/256;
		sdbuf[5] = PORT_TCP%256;
		sdbuf[6] = 0x07;
		sdbuf[7] = (char)ID_M;
		sdbuf[8] = 0x01;
		sdbuf[9] = UDP_STATE_ZY;
		num_udprecv = sendto(sockfd_udp, sdbuf,sizeof(sdbuf),0,(struct sockaddr *)&addr_remote, sizeof(struct sockaddr_in));
		for(unsigned int i = 0;i<10;i++)
		{
			printf(" %02x",sdbuf[i]);
		}
		if( num_udprecv < 0 )
		{
			printf ("ERROR: Failed to send your data!\n");
		}
		else
		{
			printf ("OK: Send to all total %d bytes !\n",num_udprecv);
			if(time_count_talk_zhongyou==1)
			{
				switch(count)
				{
				    case 0: printf("***Off_Line_No.1!\n");
					        gpio_on();
							count = 1;
					        break;
				    case 1: printf("***Off_Line_No.2!\n");
					        count_history=0;    //1129
							count = 2;
					        break;
				    default:    printf("***Off_Line_No.3!\n");
					            signal(SIGALRM,(__sighandler_t)sig_alarm_zy);
								alarm(22);//定时22S后输出信号
				}
			}
			else
			{
				printf("***OnLine!\n");
				Flag_TcpClose_FromUdp = 0;
				Flag_TcpClose_FromTcp = 0;
				//net_state = 0;
				if(!count_history)
				{
					gpio_off();
				}
				count_history=1;
				alarm(0);
				count = 0;
			}
			time_count_talk_zhongyou = 1;
		}
	}
}
void* listen_udp_zy(void*)
{
	struct sockaddr_in si_me, si_other;

	int s, slen = sizeof(si_other) , recv_len;
	unsigned char buf[LENGTH];

	//create a UDP socket
	if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		perror("socket");
		exit(1);
	}

	// zero out the structure
	memset((char *) &si_me, 0, sizeof(si_me));

	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(PORT_UDP);
	si_me.sin_addr.s_addr = htonl(INADDR_ANY);

	//端口重用
	if((setsockopt(s,SOL_SOCKET,SO_REUSEADDR,&on_zhongyou,sizeof(on_zhongyou)))<0)
	{
		perror("setsockopt failed");
		exit(1);
	}
	//bind socket to port
	if( bind(s , (struct sockaddr*)&si_me, sizeof(si_me) ) == -1)
	{
		perror("socket");
		exit(1);
	}
	else{
		printf("udp bind success!!!!!!!!\n");
		add_value_netinfo("UDP服务器监听中");
	}

	//新线程udptalk
	pthread_t id_udptalk;    //新线程ID
	long t=0;
	int ret = pthread_create(&id_udptalk, NULL, talk_udp_zy, (void*)t);
	if(ret != 0)
	{
		printf("Can not create thread!");
		exit(1);
	}

	//keep listening for data
	while(1)
	{
		memset(buf, 0, sizeof(buf));

		//try to receive some data, this is a blocking call
		if ((recv_len = recvfrom(s, buf, LENGTH, 0, (struct sockaddr *) &si_other, (socklen_t*)&slen)) == -1)
		{
			perror("recvfrom");
			exit(1);
		}
		//printf("test  Received packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
		if((buf[6] == 0x02)&& (buf[7] == 0x01) &&(recv_len == 10))//6  7   合起来是数据长度
		{
			printf("Received packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
			time_count_talk_zhongyou=0;
			alarm(0);
			for(unsigned int i = 0;i<10;i++)
			{
				printf(" %02x",buf[i]);
			}
			printf("\n");
			//qDebug()<<"clietn tcpserver!!";
			porttcpclietn=buf[4]*256+buf[5];

			ip_renzheng = QString::number(int(buf[0])).append(".").append(QString::number(int(buf[1]))).append(".")
			        .append(QString::number(int(buf[2]))).append(".").append(QString::number(int(buf[3])));
			//qDebug()<<ip_renzheng;
			receiveudp = 1;
		}
	}
	close(s);
}



