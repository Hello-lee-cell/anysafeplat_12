/********************************
 * 湖北高速泄漏监测报警信息主动上传
 * TCPclien上传，有心跳数据
 * 2019-10-30  马浩
 * ******************************/
#include "net_tcpclient_hb.h"
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
#include "serial.h"
#include "mainwindow.h"
#include "database_op.h"

unsigned char Flag_FirstClient_hb = 3;//用来标识是不是第一次连接，历史记录用
unsigned char Warn_Data_Tcp_Send_Hb[32][2] = {0};//32个点 油罐 油管 加油机 防渗池 各8个  0标志位 1状态位
unsigned char Station_ID_HB[2] = {0};//站端id 两位
unsigned char Flag_HuBeitcp_Enable = 0;//功能使能
int flag_send_heart = 0;
QMutex wait_send_over_hb;
int nsockfd_tcp_hb;        //tcp套接字
int sockfd_hb;            //tcp套接字
int PORT_TCP_CLIENT_HB = 5588;     //1111 The port which is communicate with server     可设置
char Remote_Ip_Hb[20] = {0};
const char* tcp_sever_ip_hb = "112.126.120.213";//123.57.63.66

int on_tcp_hb = 1;
unsigned char Flag_TcpClient_Success_Hb = 0;//tcpclient连接成功
unsigned char Sdbuf_Hb[64] = {0};
unsigned int Tcp_send_count_Hb = 0;
unsigned char Flag_TcpClient_SendOver_Hb = 0;
unsigned char Flag_connect_tcpClient_hb = 1;

int num_recv_hb;
char revbuf_hb[64] = {0};
unsigned char flag_send_succes_hb = 0;//发送成功置1，记录一次

net_tcpclient_hb::net_tcpclient_hb(QWidget *parent) :
    QThread(parent)
{

}
void net_tcpclient_hb::run()
{
	sleep(5);
    while(1)
    {
		if(net_state == 0)//如果网线插进去了
		{
			tcp_client();
		}
        sleep(10);
    }
    this->exec();
}
void net_tcpclient_hb::tcp_client()
{
    if((Flag_HuBeitcp_Enable == 1)&&(Flag_TcpClient_Success_Hb == 0))
    {
        struct sockaddr_in sever_remote;
        // Get the Socket file descriptor
        if( (sockfd_hb = socket(AF_INET, SOCK_STREAM, 0)) == -1 )
        {
            printf ("TCPClient Failed to obtain Socket Despcritor.\n");
			//add_value_netinfo("HuBei TCPClient Failed to obtain Socket Despcritor");
            //return 0;
        }
        else
        {
            printf ("HuBei TCPClient Obtain Socket Despcritor sucessfully.\n");
			//add_value_netinfo("HuBei TCPClient Obtain Socket Despcritor sucessfully");
            //history_net_write("Tcp_Obtain");
        }
        sever_remote.sin_family = AF_INET;           		// Protocol Family 协议族
        sever_remote.sin_port = htons (PORT_TCP_CLIENT_HB);         		// Port number 端口号
        sever_remote.sin_addr.s_addr  = inet_addr(tcp_sever_ip_hb);  	// 填写远程地址
        memset (sever_remote.sin_zero,0,8);          		// Flush the rest of struct 刷新struct的其余部分

        //端口重用
        if((setsockopt(sockfd_hb,SOL_SOCKET,SO_REUSEADDR,&on_tcp_hb,sizeof(on_tcp_hb)))<0)
        {
            perror("TCPClient setsockopt failed");
            exit(1);
        }

        bool  bDontLinger = FALSE;
        setsockopt(sockfd_hb,SOL_SOCKET,SO_LINGER,(const char*)&bDontLinger,sizeof(bool));//不要因为数据未发送就阻塞关闭操作
        setsockopt(sockfd_hb, IPPROTO_TCP, TCP_NODELAY,&on_tcp_hb,sizeof(on_tcp_hb));//立即发送，不沾包
        //int nNetTimeout = 800;//超时时长
		struct timeval timeout = {1,0}; //设置接收超时 1秒？第一个参数秒，第二个微秒
        setsockopt(sockfd_hb,SOL_SOCKET,SO_RCVTIMEO,(char *)&timeout,sizeof(struct timeval));//设置为非阻塞模式
		//主动连接目标,连接不上貌似会卡在这，10秒再连一次  非阻塞模式就不会卡在这了
        if (( nsockfd_tcp_hb = ::connect(sockfd_hb,(struct sockaddr *)&sever_remote,sizeof(struct sockaddr))) < 0)
        {
            printf ("HuBei TCPClient Failed to Client Port %d.\n", PORT_TCP_CLIENT_HB);
            //return (0);
            Flag_TcpClient_Success_Hb = 0;//连接失败
            close(nsockfd_tcp_hb);
            close(sockfd_hb);

			if(Flag_FirstClient_hb != 0)
			{
				add_value_netinfo("HuBei TCPClient Client the Port 1111 Failed");
				Flag_FirstClient_hb = 0;
			}
		}
        else
        {
            printf ("HuBei TCPClient Client the Port %d sucessfully.\n", PORT_TCP_CLIENT_HB);

            Flag_TcpClient_Success_Hb = 1;//连接成功
            client_keep_ali(sockfd_hb);//tcp保活

			if(Flag_FirstClient_hb != 1)
			{
				add_value_netinfo("HuBei TCPClient Client the Port 1111 sucessfully");
				Flag_FirstClient_hb = 1;
			}
        }
        sleep(2);
        while(1)
        {
            if(Flag_TcpClient_Success_Hb == 0)
            {
                printf ("wait TCPClient  Client \n");
                flag_send_heart = 0;
                sleep(5);
                close(nsockfd_tcp_hb);
                close(sockfd_hb);
                break;
            }
            else
            {
                flag_send_heart++;
                if(flag_send_heart >= 182) //大概十分钟一次心跳
                {
                    send_heart();
                    flag_send_heart = 0;
                }
                //qDebug()<<"ready to send!";
                warn_data();//发送报警信息
                sleep(2);//加后面的1秒共3秒检测一次
            }
            //如果不要该协议则退出
            if(Flag_HuBeitcp_Enable != 1)
            {
				//close(nsockfd_tcp_hb);
				//close(sockfd_hb);
                break;
            }
            sleep(1);
        }
        close(nsockfd_tcp_hb);
        close(sockfd_hb);
        sleep(1);
    }
    else
    {
        qDebug()<<"HuBei Tcpclient is not connected!";
    }
}
//  tcp长连接保活
void net_tcpclient_hb::client_keep_ali(int sockfd_hb)
{
    int keepAlive = 1;      // 开启keepalive属性
    int keepIdle = 10;      // 如该连接在60秒内没有任何数据往来,则进行探测
    int keepInterval = 10;   // 探测时发包的时间间隔为5 秒
    int keepCount = 3;      // 探测尝试的次数.如果第1次探测包就收到响应了,则后2次
    setsockopt(sockfd_hb, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepAlive, sizeof(keepAlive));
    setsockopt(sockfd_hb, SOL_TCP, TCP_KEEPIDLE, (void*)&keepIdle, sizeof(keepIdle)); //对应tcp_keepalive_time
    setsockopt(sockfd_hb, SOL_TCP, TCP_KEEPINTVL, (void *)&keepInterval, sizeof(keepInterval)); //对应tcp_keepalive_intvl
    setsockopt(sockfd_hb, SOL_TCP, TCP_KEEPCNT, (void *)&keepCount, sizeof(keepCount)); //对应tcp_keepalive_probes
}
//要发送的数据
void net_tcpclient_hb::send_tcpclient_data(unsigned int data_length)
{
    if(Flag_TcpClient_Success_Hb == 1)
    {
        wait_send_over_hb.lock();//上锁
        //记得加锁
        Tcp_send_count_Hb = data_length;
        for(unsigned int i = 0;i<Tcp_send_count_Hb;i++)
        {
            printf(" %02x",Sdbuf_Hb[i]);
        }
        printf("\n");
        if(Tcp_send_count_Hb != 0)
        {
            int num_send = 0;
            if((num_send = ::send(sockfd_hb,Sdbuf_Hb, Tcp_send_count_Hb, 0)) == -1)
            {
                printf("ERROR: Failed to sent string.\n");
                Flag_TcpClient_Success_Hb = 0;//连接失败
                if(flag_send_succes_hb == 1)
                {
                    add_value_netinfo("HuBei TCP 数据发送失败");
                }
                flag_send_succes_hb = 0;
                //msleep(10);
                //num_send = send(sockfd_hb,Sdbuf_Hb, Tcp_send_count_Hb, 0);//发送失败隔10ms在发送一次
            }
            else
            {
                printf("HuBei TcpClient send success %d.\n",Tcp_send_count_Hb);
                if(flag_send_succes_hb == 0)
                {
                    add_value_netinfo("HuBei TCP 数据发送成功");
                }
                flag_send_succes_hb =1;
                msleep(50);
                Tcp_send_count_Hb = 0;
            }
            Flag_TcpClient_SendOver_Hb = 0;
        }
        else
        {
            qDebug()<<"xielou semd data is kong!!";
        }
        wait_send_over_hb.unlock();//解锁
    }
    else
    {
        qDebug()<< "tcpclient is fail!!";
    }
}

void net_tcpclient_hb::warn_data()
{
    unsigned int sdbuf_num = 0;
    unsigned int CRC = 0x0000;
    for(unsigned int i=0;i<32;i++)
    {
        if(Warn_Data_Tcp_Send_Hb[i][0] >= 1)
        {
            sdbuf_num = 0;
            memset(Sdbuf_Hb,0,sizeof(char)*64);//清零数组
            Sdbuf_Hb[0] = Station_ID_HB[0];
            sdbuf_num++;
            Sdbuf_Hb[1] = Station_ID_HB[1];
            sdbuf_num++;
            Sdbuf_Hb[2] = 0x60;//报警数据
            sdbuf_num++;
            Sdbuf_Hb[3] = 0x05;//数据长度
            sdbuf_num++;
            Sdbuf_Hb[4] = select_data_type(i);//传感器类型
            sdbuf_num++;
            Sdbuf_Hb[5] = select_data_num(i);//传感器类型
            sdbuf_num++;
            Sdbuf_Hb[6] = Warn_Data_Tcp_Send_Hb[i][1];//传感器状态
            sdbuf_num++;
            CRC = CRC_Test(Sdbuf_Hb,9);
            Sdbuf_Hb[7] = ( (CRC & 0xFF00) >> 8);
            sdbuf_num++;
            Sdbuf_Hb[8] = (CRC & 0x00FF);
            sdbuf_num++;
            send_tcpclient_data(sdbuf_num);//发送数据
            msleep(5);
            if((num_recv_hb = recv(sockfd_hb,revbuf_hb,9,MSG_WAITALL)) <= 0)//MSG_DONTWAIT 1秒延时或者收到9个字节
            {
                printf("HuBei TCP recv shutdown!** \n");
            }
            else
            {
                qDebug()<<"HuBei Tcp Receive Over";
                if(revbuf[6] == 0)
                {
                    qDebug()<<"HuBei Tcp Receive right";
                    Warn_Data_Tcp_Send_Hb[i][0] = 0;
                }
                else
                {
                    qDebug()<<"HuBei Tcp Receive wrong";
                    Warn_Data_Tcp_Send_Hb[i][0]++;
                }
                if(Warn_Data_Tcp_Send_Hb[i][0] >=3)//三次没收到返回则不发送了
                {
                    Warn_Data_Tcp_Send_Hb[i][0] = 0;
                }
//                    for(unsigned char y = 0; y< num_recv_hb;y++)  //debug调试用
//                    {
//                        printf("%02x ",revbuf_hb[y]);
//                    }
//                    printf("\n");
            }
            msleep(100);
            Warn_Data_Tcp_Send_Hb[i][0] = 0;
        }
        else
        {
            msleep(10);
        }
    }
}
//发送心跳
void net_tcpclient_hb::send_heart()
{
    unsigned int sdbuf_num = 0;
    int CRC = 0;
    memset(Sdbuf_Hb,0,sizeof(char)*64);//清零数组
    Sdbuf_Hb[sdbuf_num] = Station_ID_HB[0];
    sdbuf_num++;
    Sdbuf_Hb[sdbuf_num] = Station_ID_HB[1];
    sdbuf_num++;
    Sdbuf_Hb[sdbuf_num] = 0x64;//心跳数据
    sdbuf_num++;
    Sdbuf_Hb[sdbuf_num] = 0x00;//数据长度,最后修改
    sdbuf_num++;
    Sdbuf_Hb[sdbuf_num] = count_tank;//油罐数量
    sdbuf_num++;
    for(unsigned int i = 0;i < count_tank;i++)
    {
        Sdbuf_Hb[sdbuf_num] = Warn_Data_Tcp_Send_Hb[i][1];
        sdbuf_num++;
    }
    Sdbuf_Hb[sdbuf_num] = count_pipe;//管线数量
    sdbuf_num++;
    for(unsigned int x = 0;x < count_pipe;x++)
    {
        Sdbuf_Hb[sdbuf_num] = Warn_Data_Tcp_Send_Hb[x+8][1];
        sdbuf_num++;
    }
    Sdbuf_Hb[sdbuf_num] = count_basin;//防渗池数量
    sdbuf_num++;
    for(unsigned int y = 0;y < count_basin;y++)
    {
        Sdbuf_Hb[sdbuf_num] = Warn_Data_Tcp_Send_Hb[y+16][1];
        sdbuf_num++;
    }
    Sdbuf_Hb[sdbuf_num] = count_dispener;//加油机数量
    sdbuf_num++;
    for(unsigned int z = 0;z < count_dispener;z++)
    {
        Sdbuf_Hb[sdbuf_num] = Warn_Data_Tcp_Send_Hb[z+24][1];
        sdbuf_num++;
    }
    Sdbuf_Hb[3] = sdbuf_num+2-4;//数据长度,最后修改

    CRC = CRC_Test(Sdbuf_Hb,sdbuf_num+2);
    Sdbuf_Hb[sdbuf_num] = ( (CRC & 0xFF00) >> 8);
    sdbuf_num++;
    Sdbuf_Hb[sdbuf_num] = (CRC & 0x00FF);
    sdbuf_num++;
    send_tcpclient_data(sdbuf_num);//发送数据
}
//选择传感器类型
//count_basin;防渗池
//count_pipe;管线
//count_tank;罐
//count_dispener;加油机
int net_tcpclient_hb::select_data_type(unsigned int data)
{
    int type_num = data/8;
    return type_num+1;//返回传感器类型
}
//选则传感器编号
int net_tcpclient_hb::select_data_num(unsigned int data)
{
    int type_num = data%8;
    return type_num+1;//返回传感器编号
}
