#include "mainwindow.h"
#include <QApplication>
#include<unistd.h>
#include<stdlib.h>
#include<stdio.h>
#include<errno.h>
#include<string.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<sys/wait.h>
#include<sys/socket.h>
#include<netinet/tcp.h>
#include<arpa/inet.h>
#include<sys/ioctl.h>
#include<net/if.h>
#include<math.h>//abs
#include"mythread.h"
#include"udp.h"
#include"config.h"
#include"ip_op.h"
#include"serial.h"
#include"uart_main.h"
#include"io_op.h"
#include"database_op.h"
#include"security.h"
unsigned char version[7]={0x05,0x99,0x99,0x99,0x99,0x99,0x99};   //协议版本号
unsigned char LEAK_DETECTOR=0x02; //测漏报警控制器状态数据
int PORT_UDP = 3486;       		// The port which is communicate with server     可设置
int PORT_TCP = 3487;
int nsockfd;        //tcp套接字
int on = 1;
int time_count_talk = 1;
char revbuf[LENGTH];
unsigned char Logic_Add_S = 0x01;
unsigned char Logic_Add_N = 0x01;

char *IP_local = "192.168.1.1";
int net_detect(char* net_name);
//  tcp长连接保活
void keep_ali(int sockfd)
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

//   TCP通信：报警状态主动发送
void *client_tcp(void*)
{
    int num;        //报警状态主动发送标志位

    char sdbuf_active[LENGTH];

    while(1)
    {
        usleep(500000);
        if(Flag_TcpClose_FromUdp == 1||Flag_TcpClose_FromTcp == 1)
        {
            printf("tcp_talk close!!!!\n");
            Flag_TcpClose_FromTcp = 0;
            shutdown(nsockfd,2);
            break;
        }
		//02 01 07 01 00 20 00 XX 02 04 11 01 02 00 00
        memset(sdbuf_active,0,sizeof(char)*512);
        sdbuf_active[0] = 0x02;  //Logic_Add_S;
        sdbuf_active[1] = 0x01;  //Logic_Add_N;
        sdbuf_active[2] = 0x07;   //LNAO_S;
        sdbuf_active[3] = 0x01;  //LNAO_N;
        sdbuf_active[4] = 0x00;  //0; //Mc
		sdbuf_active[5] = 0x20;  //M_st
        sdbuf_active[6] = 0x00;  //M_Lg 高位
		sdbuf_active[7] = 0x07;  //M_Lg 低位
		sdbuf_active[8] = 0x02;  //DB_Ad_Lg，数据库地址长度
		sdbuf_active[11] = 0x02; //各个传感器状态数据，暂时只传状态数据
		sdbuf_active[12] = 0x02; //状态数据长度

        for(unsigned int i = 0;i<50;i++)
        {
            if(FLAG_STACHANGE[i][0] != 0)//如果数据不是空，则有报警数据，需要发送
            {
                if(Flag_TcpClose_FromUdp == 1||Flag_TcpClose_FromTcp == 1 || net_state == 1)
                {
                    usleep(10000);
                }//发送数据之前判断网络是否良好
                else
                {
					sdbuf_active[9] = FLAG_STACHANGE[i][0];
					sdbuf_active[10] = FLAG_STACHANGE[i][1];
					sdbuf_active[13] = FLAG_STACHANGE[i][2];
					sdbuf_active[14] = FLAG_STACHANGE[i][3];
//主动上传加上时间  符合广东中石化的要求  传输字节由15变成22  数据长度字节改为0X0E //后面的时间又不传了
                    sdbuf_active[15] = FLAG_STACHANGE[i][4];
                    sdbuf_active[16] = FLAG_STACHANGE[i][5];
                    sdbuf_active[17] = FLAG_STACHANGE[i][6];
                    sdbuf_active[18] = FLAG_STACHANGE[i][7];
                    sdbuf_active[19] = FLAG_STACHANGE[i][8];
                    sdbuf_active[20] = FLAG_STACHANGE[i][9];
                    sdbuf_active[21] = FLAG_STACHANGE[i][10];

					if((num = send(nsockfd,sdbuf_active,15,0)) == -1)
                    {
                        printf("error:failed to send warning message!\n");
                        return 0;
                    }
                    else
                    {
                        usleep(100);
                        if(Flag_TcpClose_FromUdp == 1||Flag_TcpClose_FromTcp == 1 || net_state == 1)
                        {
                            usleep(10000);
                        }
                        else
                        {
                            FLAG_STACHANGE[i][0] = 0;FLAG_STACHANGE[i][1] = 0;FLAG_STACHANGE[i][2] = 0;FLAG_STACHANGE[i][3] = 0;FLAG_STACHANGE[i][4] = 0;
                            FLAG_STACHANGE[i][5] = 0;FLAG_STACHANGE[i][6] = 0;FLAG_STACHANGE[i][7] = 0;FLAG_STACHANGE[i][8] = 0;FLAG_STACHANGE[i][9] = 0;//清空记录的数据
                            FLAG_STACHANGE[i][10] = 0;
                            usleep(400000);
                        }
                    }
                }
            }
            else
            {

            }
        }

    }
    return 0;
}

void* main_main (void*)         //线程创建函数要求 void* 类型
{
    io_init();//设备初始化，待补充

    int sockfd;                        	// Socket file descriptor
    int sin_size;                      	// to store struct size

    struct sockaddr_in addr_local;
    struct sockaddr_in addr_remote;
    char IP[32] = {0};
	//char IP_bro[20] = {0};
    get_local_ip(if_name,IP);
	//IP_local = IP;
    ip_value(IP);
    printf("%d~%d~%d~%d\n",ipa,ipb,ipc,ipd);



    //创建新线程,UDP
    pthread_t id_udplisten;    //新线程ID
    long t = 0;
    int ret = pthread_create(&id_udplisten, NULL, listen_udp, (void*)t);
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
        return (0);
    }
    else
    {
        printf ("OK: Obtain Socket Despcritor sucessfully.\n");
        add_value_netinfo("TCP Socket创建成功");
    }

    // Fill the local socket address struct
    addr_local.sin_family = AF_INET;           		// Protocol Family
    addr_local.sin_port = htons (PORT_TCP);         		// Port number
    addr_local.sin_addr.s_addr  = htonl (INADDR_ANY);  	// AutoFill local address
    memset (addr_local.sin_zero,0,8);          		// Flush the rest of struct
    //端口重用
    if((setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on)))<0)
    {
        perror("setsockopt failed");
        exit(1);
    }

    //  Blind a special Port
    if( bind(sockfd, (struct sockaddr*)&addr_local, sizeof(struct sockaddr)) == -1 )
    {
        printf ("ERROR_TCP: Failed to bind Port_tcp %d.\n",PORT_TCP);
        return (0);
    }
    else
    {
        printf("OK: TCP Bind the Port_tcp %d sucessfully.\n",PORT_TCP);
        add_value_netinfo("TCP服务器端口绑定成功");
    }

    //  Listen remote connect/calling
    if(listen(sockfd,BACKLOG) == -1)
    {
        printf ("ERROR: Failed to listen Port_tcp %d.\n", PORT_TCP);
        return (0);
    }
    else
    {
        printf ("OK:TCP Listening the Port_tcp %d sucessfully.\n", PORT_TCP);
        add_value_netinfo("TCP服务器端口监听中");
    }

    sin_size = sizeof(struct sockaddr_in);

        while(1)
        {
            sin_size = sizeof(struct sockaddr_in);

            //  Wait a connection, and obtain a new socket file despriptor for single connection
            if ((nsockfd = accept(sockfd, (struct sockaddr *)&addr_remote, (socklen_t*)&sin_size)) == -1)
            {
                printf ("ERROR: Obtain new Socket Despcritor error.\n");
                continue;
            }
            else
            {
                printf ("OK: Server has got connect from %s.\n", inet_ntoa(addr_remote.sin_addr));
                keep_ali(nsockfd);
                add_value_netinfo("TCP连接已建立");
            }
            //新线程，tcp
            pthread_t id_tcptalk;
            pthread_attr_t attr;
            long t_tcp = 0;
            int ret_tcp = pthread_create(&id_tcptalk, &attr,client_tcp, (void*)t_tcp);
            if(ret_tcp != 0)
            {
                printf("Can not create thread!");
                continue;
            }


            printf("recevie string.\n");
            int num_send,num_recv;      //发送，接收状态标志位   2recv
            char sdbuf[LENGTH];
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
                memset(revbuf,0,sizeof(char)*512);
                if((num_recv = recv(nsockfd,revbuf,sizeof(revbuf),0)) <= 0)
                {
                    printf("TCP recv shutdown!** \n");
                    shutdown(nsockfd,2);
                    add_value_netinfo("TCP连接已断开");
//                    Wait_Send_Flag = 0;
                    Flag_TcpClose_FromTcp = 1;
                    sleep(1);
                    break;
                }
                if(num_recv > 0)
                {
                    printf("receive over!\n");
                }

                //数据解析
//                while(Wait_Send_Flag);
 //               Wait_Send_Flag = 1;
                usleep(1000);
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
 //               Wait_Send_Flag = 0;
                if((num_recv < 256) && (num_recv > 0))
                {
                    memset(Data_El,0,sizeof(char)*20);
                    memset(Data_ID,0,sizeof(char)*20);
                    Mc = revbuf[4];
                    M_St = revbuf[5];
                    M_Lg = revbuf[7];
                    DB_Ad_Lg = revbuf[8];
                    for(int i = DB_Ad_Lg;i > 0;i--)
                    {
                        DB_Ad[i - 1] = revbuf[8 + i];
                    }

                    if(DB_Ad_Lg == 1 && DB_Ad[0] == 0x00)//通信服务数据
                    {
                        sdbuf[0] = 0x02;//把数据节点写死
                        sdbuf[1] = 0x01;
                        Logic_Add_S = revbuf[2];
                        Logic_Add_N = revbuf[3];
                        sdbuf[2] = 0x07;
                        sdbuf[3] = ID_M;
                        sdbuf[4] = 0; //Mc
                        sdbuf[6] = 0; //M_Lg 高位
                        sdbuf[8] = 1; //DB_Ad_Lg
                        sdbuf[9] = 0; //DB_Ad
                        if(Mc == 2)//Mc判断的位置好像不对 。。。。过滤要加在前面
                        {
                            if((M_St&0xe0) == 0)          //对系统 读消息 进行回复
                            {
                                for(int i = 0;i < (M_Lg-2);i++)
                                {
                                    Data_ID[i] = revbuf[10 + i];

                                    if(Data_ID[i] == 0x01)        //协议版本号
                                    {
                                        sdbuf[sdbuf_count] = 0x01;
                                        sdbuf_count++;
                                        sdbuf[sdbuf_count] = 0x07;
                                        sdbuf_count++;
                                        sdbuf[sdbuf_count] = version[0];
                                        sdbuf_count++;
                                        sdbuf[sdbuf_count] = version[1];
                                        sdbuf_count++;
                                        sdbuf[sdbuf_count] = version[2];
                                        sdbuf_count++;
                                        sdbuf[sdbuf_count] = version[3];
                                        sdbuf_count++;
                                        sdbuf[sdbuf_count] = version[4];
                                        sdbuf_count++;
                                        sdbuf[sdbuf_count] = version[5];
                                        sdbuf_count++;
                                        sdbuf[sdbuf_count] = version[6];
                                        sdbuf_count++;
                                    }
                                    if(Data_ID[i] == 0x04)        //心跳间隔
                                    {
                                        sdbuf[sdbuf_count] = 0x04;
                                        sdbuf_count++;
                                        sdbuf[sdbuf_count] = 1;
                                        sdbuf_count++;
                                        sdbuf[sdbuf_count] = heartbeat_time;   //心跳间隔
                                        sdbuf_count++;
                                    }

                                }
                                sdbuf[5] = 0x20|(0x1f&M_St);
                                sdbuf[7] = sdbuf_count - 8;
                                if((num_send = send(nsockfd, sdbuf, sdbuf_count, 0)) == -1)
                                {
                                    printf("ERROR: Failed to sent string.\n");
                                    Flag_TcpClose_FromTcp = 1; //close(nsockfd);
                                    break;
                                }
                                printf("OK: Sent %d bytes sucessful, please enter again.\n", num_send);
                                sdbuf_count = 10;
 //                               Wait_Send_Flag = 0;
                            }
                            if((M_St&0xe0) == 0x40)           //对写入信息进行设置，回复
                            {
                                int i_4 = 0;
                                int lon = 0;
                                while(lon < (M_Lg - 2))         //提取Data_ID
                                {
                                    Data_ID[i_4] = revbuf[10 + lon];
                                    lon++;
                                    Data_Lg = revbuf[10 + lon];
                                    lon++;
                                    for(int j = 0;j < Data_Lg;j++)
                                    {
                                        Data_El[j] = revbuf[10 + lon + j];
                                    }
                                    lon = lon + Data_Lg;
                                    if(Data_ID[i_4] == 0x02)//设置节点地址
                                    {
                                        ID_M = Data_El[1];
                                        sdbuf[3] = ID_M;
                                        sdbuf[sdbuf_count] = 0;
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
                                sdbuf[5] = 0xe0|(0x1f&M_St);
                                sdbuf[7] = sdbuf_count - 8;
                                if((num_send = send(nsockfd,sdbuf,sdbuf_count,0)) == -1)
                                {
                                    printf("ERROR: Failed to sent string.\n");
                                    Flag_TcpClose_FromTcp = 1; //close(nsockfd);
                                    break;
                                }
                                printf("OK: Sent %d bytes sucessful, please enter again.\n", num_send);
                                sdbuf_count = 10;
 //                               Wait_Send_Flag = 0;

                            }
                        }
                    }

                    if((DB_Ad_Lg == 1) && (DB_Ad[0] == 0x06))   //测漏报警控制器状态数据
                    {
                        sdbuf[0] = 0x02;//把数据节点写死
                        sdbuf[1] = 0x01;
                        sdbuf[2] = 0x07;
                        sdbuf[3] = ID_M;
                        sdbuf[4] = 0;
                        sdbuf[5] = 0x20|(0x1f&M_St);
                        sdbuf[6] = 0;
                        sdbuf[7] = 5;
                        sdbuf[8] = 1;
                        sdbuf[9] = 0x06;
                        sdbuf[10] = 0x01;
                        sdbuf[11] = 0x01;
                        sdbuf[12] = LEAK_DETECTOR;
                        if((num_send = send(nsockfd,sdbuf,13,0)) == -1)
                        {
                            printf("ERROR: Failed to sent string.\n");
                            Flag_TcpClose_FromTcp = 1; //close(nsockfd);
                            break;
                        }
                        printf("OK: Sent %d bytes sucessful, please enter again.\n", num_send);
//                        Wait_Send_Flag = 0;
                    }
                    if((DB_Ad_Lg == 2) && (DB_Ad[0] == 0x02))//人井测漏状态数据
                    {
                        sdbuf[0] = 0x02;//把数据节点写死
                        sdbuf[1] = 0x01;
                        sdbuf[2] = 0x07;
                        sdbuf[3] = ID_M;
                        sdbuf[4] = 0; //Mc
                        sdbuf[6] = 0; //M_Lg 高位
                        sdbuf[8] = 2; //DB_Ad_Lg
                        sdbuf[9] = 0x02; //DB_Ad
                        sdbuf[sdbuf_count] = revbuf[10];//DB_Ad
                        sdbuf_count++;
                        int LB_ID = DB_Ad[1]&0x0f;
                        unsigned char type_basin = 0;
                        int OIL_BASIN_count = 0;
                        if(!LB_ID)
                        {
                            LB_ID = count_basin;    //需设置为实际传感器数量

                            for(int i = 0;i < LB_ID;i++)
                            {
                                sdbuf[sdbuf_count] = 0x02;
                                sdbuf_count++;
                                sdbuf[sdbuf_count] = 0x02;
                                sdbuf_count++;
                                sdbuf[sdbuf_count] = OIL_BASIN_TEMP[OIL_BASIN_count]&0x0f;
                                type_basin = OIL_BASIN_TEMP[OIL_BASIN_count] >> 6;
                                sdbuf_count++;
                                OIL_BASIN_count++;
                                sdbuf[sdbuf_count] = OIL_BASIN_TEMP[OIL_BASIN_count];
                                sdbuf_count++;
                                OIL_BASIN_count++;
                                sdbuf[sdbuf_count] = 0x03;      //测漏类型
                                sdbuf_count++;
                                sdbuf[sdbuf_count] = 0x01;
                                sdbuf_count++;
                                sdbuf[sdbuf_count] = type_basin;
                                sdbuf_count++;
                            }
                        }
                        else
                        {
                            if(LB_ID>count_basin)//如果问的传感器没有设置
                            {
                                Data_ID[0] = revbuf[11];

                                if((Data_ID[0] == 0x02)||(!Data_ID[0]))
                                {
                                    sdbuf[sdbuf_count] = 0x02;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x02;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] =  0x10;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] == 0x03)||(!Data_ID[0]))
                                {
                                    sdbuf[sdbuf_count] = 0x03;//测漏类型
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x01;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x04; //类型为0x04，其他
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] == 0x04)||(!Data_ID[0]))
                                {
                                    sdbuf[sdbuf_count] = 0x04;//编号
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x01;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = LB_ID&0x0f;
                                    sdbuf_count++;
                                }
                                //问压力值
                                if((Data_ID[0] == 0x05)||(!Data_ID[0]))
                                {
                                    sdbuf[sdbuf_count] = 0x05;//压力值
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x04;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x05;  //压力值正
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //压力值空位
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //压力值前两位
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;  //压力值后两位
                                    sdbuf_count++;

                                }
                                if((Data_ID[0] == 0x06)||(!Data_ID[0]))//气体浓度
                                {
                                    sdbuf[sdbuf_count] = 0x06;//气体浓度
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x02;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;  //浓度高位
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //浓度低位
                                    sdbuf_count++;
                                }


                                if((Data_ID[0] == 0x10)||(!Data_ID[0]))//高液位报警值
                                {
                                    sdbuf[sdbuf_count] = 0x10;//高液位
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x04;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;  //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x03; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] == 0x11)||(!Data_ID[0]))//高高液位报警值
                                {
                                    sdbuf[sdbuf_count] = 0x11;//高高液位
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x04;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;  //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x06; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] == 0x12)||(!Data_ID[0]))//低液位报警值
                                {
                                    sdbuf[sdbuf_count] = 0x12;//低液位
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x04;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;  //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x03; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] == 0x13)||(!Data_ID[0]))//低低液位报警值
                                {
                                    sdbuf[sdbuf_count] = 0x13;//低低液位
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x04;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;  //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x06; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] == 0x14)||(!Data_ID[0]))//浓度预警值
                                {
                                    sdbuf[sdbuf_count] = 0x14;//气体浓度
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x02;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x25;
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] == 0x15)||(!Data_ID[0]))//浓度报警值
                                {
                                    sdbuf[sdbuf_count] = 0x15;//气体浓度
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x02;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x40;
                                    sdbuf_count++;
                                }

                                if((Data_ID[0] != 0x02) && (Data_ID[0] != 0x03) && (Data_ID[0] != 0x04) && (Data_ID[0] != 0x05) && (Data_ID[0] != 0x06) &&
                                        (Data_ID[0] != 0x10) && (Data_ID[0] != 0x11) && (Data_ID[0] != 0x12) && (Data_ID[0] != 0x13) && (Data_ID[0] != 0x14) &&
                                        (Data_ID[0] != 0x15))
                                {
                                    sdbuf[5] = 0xe0|(0x1f&M_St);  //M_St
                                    sdbuf[sdbuf_count] = 2;//确认消息
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = Data_ID[0];
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 5;
                                    sdbuf_count++;
                                }
                            }
                            else
                            {
                                Data_ID[0] = revbuf[11];
                                OIL_BASIN_count = (LB_ID - 1) * 2;
                                if((Data_ID[0] == 0x02)||(!Data_ID[0]))
                                {
                                    sdbuf[sdbuf_count] = 0x02;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x02;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] =  OIL_BASIN_TEMP[OIL_BASIN_count] & 0x0f;
                                    type_basin = OIL_BASIN_TEMP[OIL_BASIN_count]>>6;
                                    sdbuf_count++;
                                    OIL_BASIN_count++;
                                    sdbuf[sdbuf_count] = OIL_BASIN_TEMP[OIL_BASIN_count]&0x7f;  //&0111 1111
                                    sdbuf_count++;
                                    OIL_BASIN_count++;
                                }
                                if((Data_ID[0] == 0x03)||(!Data_ID[0]))
                                {
                                    type_basin = OIL_BASIN_TEMP[OIL_BASIN_count]>>6;
                                    sdbuf[sdbuf_count] = 0x03;//测漏类型
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x01;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = type_basin; //类型为0x03，传感器方法
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] == 0x04)||(!Data_ID[0]))
                                {
                                    sdbuf[sdbuf_count] = 0x04;//编号
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x01;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = LB_ID&0x0f;
                                    sdbuf_count++;
                                }
                                //问压力值
                                if((Data_ID[0] == 0x05)||(!Data_ID[0]))
                                {
                                    sdbuf[sdbuf_count] = 0x05;//压力值
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x04;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x05;  //压力值正
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //压力值空位
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //压力值前两位
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;  //压力值后两位
                                    sdbuf_count++;

                                }
                                if((Data_ID[0] == 0x06)||(!Data_ID[0]))//气体浓度
                                {
                                    sdbuf[sdbuf_count] = 0x06;//气体浓度
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x02;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;  //浓度高位
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //浓度低位
                                    sdbuf_count++;
                                }

                                if((Data_ID[0] == 0x10)||(!Data_ID[0]))//高液位报警值
                                {
                                    sdbuf[sdbuf_count] = 0x10;//高液位
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x04;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;  //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x03; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] == 0x11)||(!Data_ID[0]))//高高液位报警值
                                {
                                    sdbuf[sdbuf_count] = 0x11;//高高液位
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x04;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;  //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x06; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] == 0x12)||(!Data_ID[0]))//低液位报警值
                                {
                                    sdbuf[sdbuf_count] = 0x12;//低液位
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x04;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;  //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x03; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] == 0x13)||(!Data_ID[0]))//低低液位报警值
                                {
                                    sdbuf[sdbuf_count] = 0x13;//低低液位
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x04;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;  //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x06; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] == 0x14)||(!Data_ID[0]))//浓度预警值
                                {
                                    sdbuf[sdbuf_count] = 0x14;//气体浓度
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x02;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x25;
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] == 0x15)||(!Data_ID[0]))//浓度报警值
                                {
                                    sdbuf[sdbuf_count] = 0x15;//气体浓度
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x02;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x40;
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] != 0x02) && (Data_ID[0] != 0x03) && (Data_ID[0] != 0x04) && (Data_ID[0] != 0x05) && (Data_ID[0] != 0x06) &&
                                        (Data_ID[0] != 0x10) && (Data_ID[0] != 0x11) && (Data_ID[0] != 0x12) && (Data_ID[0] != 0x13) && (Data_ID[0] != 0x14) &&
                                        (Data_ID[0] != 0x15))
                                {
                                    sdbuf[5] = 0xe0|(0x1f&M_St);  //M_St
                                    sdbuf[sdbuf_count] = 2;//确认消息
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = Data_ID[0];
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 5;
                                    sdbuf_count++;
                                }
                            }
                        }
                        sdbuf[7] = sdbuf_count - 8;
                        if((num_send = send(nsockfd,sdbuf,sdbuf_count,0)) == -1)
                        {
                            printf("ERROR: Failed to sent string.\n");
                            Flag_TcpClose_FromTcp = 1; //close(nsockfd);
                            break;
                        }
                        printf("OK: Sent %d bytes sucessful, please enter again.\n", num_send);
                        sdbuf_count = 10;
//                        Wait_Send_Flag = 0;
                    }
                    if(DB_Ad_Lg == 2 && DB_Ad[0] == 0x03)   //管线测漏状态数据
                    {
                        sdbuf[0] = 0x02;//把数据节点写死
                        sdbuf[1] = 0x01;
                        sdbuf[2] = 0x07;
                        sdbuf[3] = ID_M;
                        sdbuf[4] = 0;                   //Mc
                        sdbuf[6] = 0;                   //M_Lg 高位
                        sdbuf[8] = 2;                   //DB_Ad_Lg
                        sdbuf[9] = 0x03;                //DB_Ad
                        sdbuf[sdbuf_count] = revbuf[10];  //DB_Ad
                        sdbuf_count++;
                        int LP_ID = DB_Ad[1] & 0x0f;
                        int OIL_PIPE_count = 0;
                        unsigned char type_pipe = 0;
                        sdbuf[5] = 0x20|(0x1f & M_St);  //M_St
                        if(!LP_ID)
                        {
                            LP_ID = count_pipe;

                            for(int i = 0;i < LP_ID;i++)
                            {
                                sdbuf[sdbuf_count] = 0x02;    //ID
                                sdbuf_count++;
                                sdbuf[sdbuf_count] = 0x02;    //LONG
                                sdbuf_count++;
                                sdbuf[sdbuf_count] = OIL_PIPE_TEMP[OIL_PIPE_count]&0x0f;
                                type_pipe = OIL_PIPE_TEMP[OIL_PIPE_count]>>6;
                                sdbuf_count++;
                                OIL_PIPE_count++;
                                sdbuf[sdbuf_count] = OIL_PIPE_TEMP[OIL_PIPE_count];
                                sdbuf_count++;
                                OIL_PIPE_count++;
                                sdbuf[sdbuf_count] = 0x03;//测漏类型
                                sdbuf_count++;
                                sdbuf[sdbuf_count] = 0x01;
                                sdbuf_count++;
                                sdbuf[sdbuf_count] = type_pipe;
                                sdbuf_count++;

                            }
                        }
                        else
                        {
                            if(LP_ID>count_pipe)
                            {
                                Data_ID[0] = revbuf[11];
                                if((Data_ID[0] == 0x02)||(!Data_ID[0]))
                                {
                                    sdbuf[sdbuf_count] = 0x02;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x02;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x10;
                                    type_pipe = OIL_PIPE_TEMP[OIL_PIPE_count]>>6;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] == 0x03)||(!Data_ID[0]))
                                {
                                    type_pipe = OIL_TANK_TEMP[OIL_PIPE_count]>>6;
                                    sdbuf[sdbuf_count] = 0x03;//其他类型
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x01;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x04;//未设置传感器类型为其他
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] == 0x04)||(!Data_ID[0]))
                                {
                                    sdbuf[sdbuf_count] = 0x04;//编号
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x01;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = LP_ID&0x0f;
                                    sdbuf_count++;
                                }
                                //问压力值
                                if((Data_ID[0] == 0x05)||(!Data_ID[0]))
                                {
                                    sdbuf[sdbuf_count] = 0x05;//压力值
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x04;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x05;  //压力值正
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //压力值空位
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //压力值前两位
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;  //压力值后两位
                                    sdbuf_count++;

                                }
                                if((Data_ID[0] == 0x06)||(!Data_ID[0]))//气体浓度
                                {
                                    sdbuf[sdbuf_count] = 0x06;//气体浓度
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x02;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;  //浓度高位
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //浓度低位
                                    sdbuf_count++;
                                }

                                if((Data_ID[0] == 0x10)||(!Data_ID[0]))//高液位报警值
                                {
                                    sdbuf[sdbuf_count] = 0x10;//高液位
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x04;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;  //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x03; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] == 0x11)||(!Data_ID[0]))//高高液位报警值
                                {
                                    sdbuf[sdbuf_count] = 0x11;//高高液位
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x04;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;  //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x06; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] == 0x12)||(!Data_ID[0]))//低液位报警值
                                {
                                    sdbuf[sdbuf_count] = 0x12;//低液位
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x04;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;  //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x03; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] == 0x13)||(!Data_ID[0]))//低低液位报警值
                                {
                                    sdbuf[sdbuf_count] = 0x13;//低低液位
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x04;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;  //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x06; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] == 0x14)||(!Data_ID[0]))//浓度预警值
                                {
                                    sdbuf[sdbuf_count] = 0x14;//气体浓度
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x02;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x25;
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] == 0x15)||(!Data_ID[0]))//浓度报警值
                                {
                                    sdbuf[sdbuf_count] = 0x15;//气体浓度
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x02;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x40;
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] != 0x02) && (Data_ID[0] != 0x03) && (Data_ID[0] != 0x04) && (Data_ID[0] != 0x05) && (Data_ID[0] != 0x06) &&
                                        (Data_ID[0] != 0x10) && (Data_ID[0] != 0x11) && (Data_ID[0] != 0x12) && (Data_ID[0] != 0x13) && (Data_ID[0] != 0x14) &&
                                        (Data_ID[0] != 0x15))
                                {
                                    sdbuf[5] = 0xe0|(0x1f&M_St);  //M_St
                                    sdbuf[sdbuf_count] = 2;//确认消息
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = Data_ID[0];
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 5;
                                    sdbuf_count++;
                                }
                            }
                            else
                            {
                                Data_ID[0] = revbuf[11];
                                OIL_PIPE_count = (LP_ID - 1)*2;
                                if((Data_ID[0] == 0x02)||(!Data_ID[0]))
                                {
                                    sdbuf[sdbuf_count] = 0x02;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x02;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = OIL_PIPE_TEMP[OIL_PIPE_count] & 0x0f;
                                    type_pipe = OIL_PIPE_TEMP[OIL_PIPE_count]>>6;
                                    sdbuf_count++;
                                    OIL_PIPE_count++;
                                    sdbuf[sdbuf_count] = OIL_PIPE_TEMP[OIL_PIPE_count]&0x7f;  //&0111 1111;
                                    sdbuf_count++;
                                    OIL_PIPE_count++;
                                }
                                if((Data_ID[0] == 0x03)||(!Data_ID[0]))
                                {
                                    type_pipe = OIL_PIPE_TEMP[OIL_PIPE_count]>>6;
                                    sdbuf[sdbuf_count] = 0x03;//测漏类型
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x01;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = type_pipe;
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] == 0x04)||(!Data_ID[0]))
                                {
                                    sdbuf[sdbuf_count] = 0x04;//编号
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x01;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = LP_ID&0x0f;
                                    sdbuf_count++;
                                }
                                //问压力值
                                if((Data_ID[0] == 0x05)||(!Data_ID[0]))
                                {
                                    sdbuf[sdbuf_count] = 0x05;//压力值
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x04;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x05;  //压力值正
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //压力值空位
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //压力值前两位
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;  //压力值后两位
                                    sdbuf_count++;

                                }
                                if((Data_ID[0] == 0x06)||(!Data_ID[0]))//气体浓度
                                {
                                    sdbuf[sdbuf_count] = 0x06;//气体浓度
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x02;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;  //浓度高位
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //浓度低位
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] == 0x10)||(!Data_ID[0]))//高液位报警值
                                {
                                    sdbuf[sdbuf_count] = 0x10;//高液位
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x04;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;  //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x03; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] == 0x11)||(!Data_ID[0]))//高高液位报警值
                                {
                                    sdbuf[sdbuf_count] = 0x11;//高高液位
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x04;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;  //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x06; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] == 0x12)||(!Data_ID[0]))//低液位报警值
                                {
                                    sdbuf[sdbuf_count] = 0x12;//低液位
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x04;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;  //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x03; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] == 0x13)||(!Data_ID[0]))//低低液位报警值
                                {
                                    sdbuf[sdbuf_count] = 0x13;//低低液位
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x04;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;  //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x06; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] == 0x14)||(!Data_ID[0]))//浓度预警值
                                {
                                    sdbuf[sdbuf_count] = 0x14;//气体浓度
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x02;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x25;
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] == 0x15)||(!Data_ID[0]))//浓度报警值
                                {
                                    sdbuf[sdbuf_count] = 0x15;//气体浓度
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x02;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x40;
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] != 0x02) && (Data_ID[0] != 0x03) && (Data_ID[0] != 0x04) && (Data_ID[0] != 0x05) && (Data_ID[0] != 0x06) &&
                                        (Data_ID[0] != 0x10) && (Data_ID[0] != 0x11) && (Data_ID[0] != 0x12) && (Data_ID[0] != 0x13) && (Data_ID[0] != 0x14) &&
                                        (Data_ID[0] != 0x15))
                                {
                                    sdbuf[5] = 0xe0|(0x1f&M_St);  //M_St
                                    sdbuf[sdbuf_count] = 2;//确认消息
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = Data_ID[0];
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 5;
                                    sdbuf_count++;
                                }
                            }
                        }
                        sdbuf[7] = sdbuf_count - 8;     //M_Lg
                        if((num_send = send(nsockfd,sdbuf,sdbuf_count,0)) == -1)
                        {
                            printf("ERROR: Failed to sent string.\n");
                            Flag_TcpClose_FromTcp = 1; //close(nsockfd);
                            break;
                        }
                        printf("OK: Sent %d bytes sucessful, please enter again.\n", num_send);
                        sdbuf_count = 10;
//                        Wait_Send_Flag = 0;
                    }
                    if(DB_Ad_Lg == 2 && DB_Ad[0] == 0x04)//油罐测漏状态数据
                    {
                        sdbuf[0] = 0x02;//把数据节点写死
                        sdbuf[1] = 0x01;
                        sdbuf[2] = 0x07;
                        sdbuf[3] = ID_M;
                        sdbuf[4] = 0; //Mc
                        sdbuf[6] = 0; //M_Lg 高位
                        sdbuf[8] = 2; //DB_Ad_Lg
                        sdbuf[9] = 0x04; //DB_Ad
                        sdbuf[sdbuf_count] = revbuf[10];//DB_Ad
                        sdbuf_count++;
                        int LT_ID = DB_Ad[1]&0x0f;
                        int OIL_TANK_count = 0;
                        unsigned char type_tank = 0;
                        sdbuf[5] = 0x20|(0x1f&M_St);  //M_St
                        if(!LT_ID)
                        {
                            LT_ID = count_tank;

                            for(int i = 0;i < LT_ID;i++)
                            {
                                sdbuf[sdbuf_count] = 0x02;    //ID
                                sdbuf_count++;
                                sdbuf[sdbuf_count] = 0x02;    //LONG
                                sdbuf_count++;
                                sdbuf[sdbuf_count] = OIL_TANK_TEMP[OIL_TANK_count] & 0x0f;
                                type_tank = OIL_TANK_TEMP[OIL_TANK_count]>>6;
                                sdbuf_count++;
                                OIL_TANK_count++;
                                sdbuf[sdbuf_count] = OIL_TANK_TEMP[OIL_TANK_count];
                                sdbuf_count++;
                                OIL_TANK_count++;
                                sdbuf[sdbuf_count] = 0x03;//测漏类型
                                sdbuf_count++;
                                sdbuf[sdbuf_count] = 0x01;
                                sdbuf_count++;
                                sdbuf[sdbuf_count] = type_tank;
                                sdbuf_count++;
                            }
                        }
                        else
                        {
                            if(LT_ID>count_tank)
                            {
                                Data_ID[0] = revbuf[11];//1125
                                if((Data_ID[0] == 0x02)||(!Data_ID[0]))
                                {
                                    sdbuf[sdbuf_count] = 0x02;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x02;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x10;
                                    type_tank = OIL_TANK_TEMP[OIL_TANK_count]>>6;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] == 0x03)||(!Data_ID[0]))
                                {
                                    type_tank = OIL_TANK_TEMP[OIL_TANK_count]>>6;
                                    sdbuf[sdbuf_count] = 0x03;//测漏类型
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x01;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x04;//未设置传感器类型为其他
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] == 0x04)||(!Data_ID[0]))
                                {
                                    sdbuf[sdbuf_count] = 0x04;//编号
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x01;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = LT_ID&0x0f;
                                    sdbuf_count++;
                                }
                                //问压力值
                                if((Data_ID[0] == 0x05)||(!Data_ID[0]))
                                {
                                    sdbuf[sdbuf_count] = 0x05;//压力值
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x04;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x05;  //压力值正
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //压力值空位
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //压力值前两位
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;  //压力值后两位
                                    sdbuf_count++;

                                }
                                if((Data_ID[0] == 0x06)||(!Data_ID[0]))//气体浓度
                                {
                                    sdbuf[sdbuf_count] = 0x06;//气体浓度
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x02;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;  //浓度高位
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //浓度低位
                                    sdbuf_count++;
                                }

                                if((Data_ID[0] == 0x10)||(!Data_ID[0]))//高液位报警值
                                {
                                    sdbuf[sdbuf_count] = 0x10;//高液位
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x04;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;  //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x03; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] == 0x11)||(!Data_ID[0]))//高高液位报警值
                                {
                                    sdbuf[sdbuf_count] = 0x11;//高高液位
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x04;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;  //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x06; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] == 0x12)||(!Data_ID[0]))//低液位报警值
                                {
                                    sdbuf[sdbuf_count] = 0x12;//低液位
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x04;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;  //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x03; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] == 0x13)||(!Data_ID[0]))//低低液位报警值
                                {
                                    sdbuf[sdbuf_count] = 0x13;//低低液位
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x04;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;  //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x06; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] == 0x14)||(!Data_ID[0]))//浓度预警值
                                {
                                    sdbuf[sdbuf_count] = 0x14;//气体浓度
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x02;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x25;
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] == 0x15)||(!Data_ID[0]))//浓度报警值
                                {
                                    sdbuf[sdbuf_count] = 0x15;//气体浓度
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x02;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x40;
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] != 0x02) && (Data_ID[0] != 0x03) && (Data_ID[0] != 0x04) && (Data_ID[0] != 0x05) && (Data_ID[0] != 0x06) &&
                                        (Data_ID[0] != 0x10) && (Data_ID[0] != 0x11) && (Data_ID[0] != 0x12) && (Data_ID[0] != 0x13) && (Data_ID[0] != 0x14) &&
                                        (Data_ID[0] != 0x15))
                                {
                                    sdbuf[5] = 0xe0|(0x1f&M_St);  //M_St
                                    sdbuf[sdbuf_count] = 2;//确认消息
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = Data_ID[0];
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 5;
                                    sdbuf_count++;
                                }
                            }
                            else
                            {
                                Data_ID[0] = revbuf[11];//1125
                                OIL_TANK_count = (LT_ID - 1)*2;
                                if((Data_ID[0] == 0x02)||(!Data_ID[0]))
                                {
                                    sdbuf[sdbuf_count] = 0x02;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x02;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = OIL_TANK_TEMP[OIL_TANK_count] & 0x0f;
                                    type_tank = OIL_TANK_TEMP[OIL_TANK_count]>>6;
                                    sdbuf_count++;
                                    OIL_TANK_count++;
                                    sdbuf[sdbuf_count] = OIL_TANK_TEMP[OIL_TANK_count]&0x7f;  //&0111 1111;
                                    sdbuf_count++;
                                    OIL_TANK_count++;
                                }
                                if((Data_ID[0] == 0x03)||(!Data_ID[0]))
                                {
                                    type_tank = OIL_TANK_TEMP[OIL_TANK_count]>>6;
                                    sdbuf[sdbuf_count] = 0x03;//测漏类型
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x01;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = type_tank;
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] == 0x04)||(!Data_ID[0]))
                                {
                                    sdbuf[sdbuf_count] = 0x04;//编号
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x01;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = LT_ID&0x0f;
                                    sdbuf_count++;
                                }
                                //问压力值
                                if((Data_ID[0] == 0x05)||(!Data_ID[0]))
                                {
                                    sdbuf[sdbuf_count] = 0x05;//压力值
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x04;
                                    sdbuf_count++;

                                    //LT_ID
                                    if(count_Pressure[LT_ID-1]<0)
                                    {
                                        sdbuf[sdbuf_count] = 0x85;  //压力值负
                                        sdbuf_count++;
                                    }
                                    else
                                    {
                                        sdbuf[sdbuf_count] = 0x05;  //压力值正
                                        sdbuf_count++;
                                    }
                                    sdbuf[sdbuf_count] = 0x00; //压力值空位
                                    sdbuf_count++;
                                    int sdpress_h = int(count_Pressure[LT_ID-1])/10; //高两位
                                    sdpress_h = abs(sdpress_h);
                                    int sdpress_l = int(count_Pressure[LT_ID-1]*10)%100; //低两位
                                    sdpress_l = abs(sdpress_l);
                                    printf("*******%d***********%d**************",sdpress_h,sdpress_l);
                                    int a_h = sdpress_h/10;
                                    int hex_h = sdpress_h + a_h*6;
                                    int a_l = sdpress_l/10;
                                    int hex_l = sdpress_l + a_l*6;

                                    if(sdpress_h>0)
                                        sdbuf[sdbuf_count] = hex_h; //压力值前两位
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = hex_l;  //压力值后两位
                                    sdbuf_count++;

                                }
                                if((Data_ID[0] == 0x06)||(!Data_ID[0]))//气体浓度
                                {
                                    sdbuf[sdbuf_count] = 0x06;//气体浓度
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x02;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;  //浓度高位
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //浓度低位
                                    sdbuf_count++;
                                }

                                if((Data_ID[0] == 0x10)||(!Data_ID[0]))//高液位报警值
                                {
                                    sdbuf[sdbuf_count] = 0x10;//高液位
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x04;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;  //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x03; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] == 0x11)||(!Data_ID[0]))//高高液位报警值
                                {
                                    sdbuf[sdbuf_count] = 0x11;//高高液位
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x04;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;  //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x06; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] == 0x12)||(!Data_ID[0]))//低液位报警值
                                {
                                    sdbuf[sdbuf_count] = 0x12;//低液位
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x04;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;  //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x03; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] == 0x13)||(!Data_ID[0]))//低低液位报警值
                                {
                                    sdbuf[sdbuf_count] = 0x13;//低低液位
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x04;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;  //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x06; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] == 0x14)||(!Data_ID[0]))//浓度预警值
                                {
                                    sdbuf[sdbuf_count] = 0x14;//气体浓度
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x02;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x25;
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] == 0x15)||(!Data_ID[0]))//浓度报警值
                                {
                                    sdbuf[sdbuf_count] = 0x15;//气体浓度
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x02;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x40;
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] != 0x02) && (Data_ID[0] != 0x03) && (Data_ID[0] != 0x04) && (Data_ID[0] != 0x05) && (Data_ID[0] != 0x06) &&
                                        (Data_ID[0] != 0x10) && (Data_ID[0] != 0x11) && (Data_ID[0] != 0x12) && (Data_ID[0] != 0x13) && (Data_ID[0] != 0x14) &&
                                        (Data_ID[0] != 0x15))
                                {
                                    sdbuf[5] = 0xe0|(0x1f&M_St);  //M_St
                                    sdbuf[sdbuf_count] = 2;//确认消息
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = Data_ID[0];
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 5;
                                    sdbuf_count++;
                                }
                            }
                        }
                        sdbuf[7] = sdbuf_count-8;     //M_Lg
                        if((num_send = send(nsockfd,sdbuf,sdbuf_count,0)) == -1)
                        {
                            printf("ERROR: Failed to sent string.\n");
                            Flag_TcpClose_FromTcp = 1; //close(nsockfd);
                            break;
                        }
                        printf("OK: Sent %d bytes sucessful, please enter again.\n", num_send);
                        sdbuf_count = 10;
//                        Wait_Send_Flag = 0;
                    }
                    if(DB_Ad_Lg == 2 && DB_Ad[0] == 0x05) //加油机底槽测漏状态数据
                    {
                        sdbuf[0] = 0x02;//把数据节点写死
                        sdbuf[1] = 0x01;
                        sdbuf[2] = 0x07;
                        sdbuf[3] = ID_M;
                        sdbuf[4] = 0; //Mc
                        sdbuf[6] = 0; //M_Lg 高位
                        sdbuf[8] = 2; //DB_Ad_Lg
                        sdbuf[9] = 0x05; //DB_Ad
                        sdbuf[sdbuf_count] = revbuf[10];//DB_Ad
                        sdbuf_count++;
                        int LD_ID = DB_Ad[1]&0x0f;
                        int OIL_DISPENER_count = 0;
                        unsigned char type_dispener;
                        if(!LD_ID)
                        {
                            LD_ID = count_dispener;

                            for(int i = 0;i < LD_ID;i++)
                            {
                                sdbuf[sdbuf_count] = 0x02;
                                sdbuf_count++;
                                sdbuf[sdbuf_count] = 0x02;
                                sdbuf_count++;
                                sdbuf[sdbuf_count] = OIL_DISPENER_TEMP[OIL_DISPENER_count]&0x0f;
                                type_dispener = OIL_DISPENER_TEMP[OIL_DISPENER_count]>>6;
                                sdbuf_count++;
                                OIL_DISPENER_count++;
                                sdbuf[sdbuf_count] = OIL_DISPENER_TEMP[OIL_DISPENER_count];
                                sdbuf_count++;
                                OIL_DISPENER_count++;
                                sdbuf[sdbuf_count] = 0x03;//测漏类型
                                sdbuf_count++;
                                sdbuf[sdbuf_count] = 0x01;
                                sdbuf_count++;
                                sdbuf[sdbuf_count] = type_dispener;
                                sdbuf_count++;
                            }
                        }
                        else
                        {
                            if(LD_ID>count_dispener)
                            {
                                Data_ID[0] = revbuf[11];//1125
                                if((Data_ID[0] == 0x02)||(!Data_ID[0]))
                                {
                                    sdbuf[sdbuf_count] = 0x02;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x02;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x10;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] == 0x03)||(!Data_ID[0]))
                                {
                                    sdbuf[sdbuf_count] = 0x03;//测漏类型
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x01;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x04; //其他类型
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] == 0x04)||(!Data_ID[0]))
                                {
                                    sdbuf[sdbuf_count] = 0x04;//编号
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x01;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = LD_ID&0x0f;
                                    sdbuf_count++;
                                }
                                //问压力值
                                if((Data_ID[0] == 0x05)||(!Data_ID[0]))
                                {
                                    sdbuf[sdbuf_count] = 0x05;//压力值
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x04;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x05;  //压力值正
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //压力值空位
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //压力值前两位
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;  //压力值后两位
                                    sdbuf_count++;

                                }
                                if((Data_ID[0] == 0x06)||(!Data_ID[0]))//气体浓度
                                {
                                    sdbuf[sdbuf_count] = 0x06;//气体浓度
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x02;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;  //浓度高位
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //浓度低位
                                    sdbuf_count++;
                                }

                                if((Data_ID[0] == 0x10)||(!Data_ID[0]))//高液位报警值
                                {
                                    sdbuf[sdbuf_count] = 0x10;//高液位
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x04;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;  //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x03; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] == 0x11)||(!Data_ID[0]))//高高液位报警值
                                {
                                    sdbuf[sdbuf_count] = 0x11;//高高液位
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x04;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;  //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x06; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] == 0x12)||(!Data_ID[0]))//低液位报警值
                                {
                                    sdbuf[sdbuf_count] = 0x12;//低液位
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x04;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;  //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x03; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] == 0x13)||(!Data_ID[0]))//低低液位报警值
                                {
                                    sdbuf[sdbuf_count] = 0x13;//低低液位
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x04;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;  //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x06; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] == 0x14)||(!Data_ID[0]))//浓度预警值
                                {
                                    sdbuf[sdbuf_count] = 0x14;//气体浓度
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x02;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x25;
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] == 0x15)||(!Data_ID[0]))//浓度报警值
                                {
                                    sdbuf[sdbuf_count] = 0x15;//气体浓度
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x02;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x40;
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] != 0x02) && (Data_ID[0] != 0x03) && (Data_ID[0] != 0x04) && (Data_ID[0] != 0x05) && (Data_ID[0] != 0x06) &&
                                        (Data_ID[0] != 0x10) && (Data_ID[0] != 0x11) && (Data_ID[0] != 0x12) && (Data_ID[0] != 0x13) && (Data_ID[0] != 0x14) &&
                                        (Data_ID[0] != 0x15))
                                {
                                    sdbuf[5] = 0xe0|(0x1f&M_St);  //M_St
                                    sdbuf[sdbuf_count] = 2;//确认消息
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = Data_ID[0];
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 5;
                                    sdbuf_count++;
                                }
                            }
                            else
                            {
                                Data_ID[0] = revbuf[11];//1125
                                OIL_DISPENER_count = (LD_ID-1)*2;
                                if((Data_ID[0] == 0x02)||(!Data_ID[0]))
                                {
                                    sdbuf[sdbuf_count] = 0x02;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x02;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = OIL_DISPENER_TEMP[OIL_DISPENER_count]&0x0f;
                                    type_dispener = OIL_DISPENER_TEMP[OIL_DISPENER_count]>>6;
                                    sdbuf_count++;
                                    OIL_DISPENER_count++;
                                    sdbuf[sdbuf_count] = OIL_DISPENER_TEMP[OIL_DISPENER_count]&0x7f;  //&0111 1111;
                                    sdbuf_count++;
                                    OIL_DISPENER_count++;
                                }
                                if((Data_ID[0] == 0x03)||(!Data_ID[0]))
                                {
                                    type_dispener = OIL_DISPENER_TEMP[OIL_DISPENER_count]>>6;
                                    sdbuf[sdbuf_count] = 0x03;//测漏类型
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x01;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = type_dispener; //固定类型
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] == 0x04)||(!Data_ID[0]))
                                {
                                    sdbuf[sdbuf_count] = 0x04;//编号
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x01;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = LD_ID&0x0f;
                                    sdbuf_count++;
                                }
                                //问压力值
                                if((Data_ID[0] == 0x05)||(!Data_ID[0]))
                                {
                                    sdbuf[sdbuf_count] = 0x05;//压力值
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x04;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x05;  //压力值正
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //压力值空位
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //压力值前两位
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;  //压力值后两位
                                    sdbuf_count++;

                                }
                                if((Data_ID[0] == 0x06)||(!Data_ID[0]))//气体浓度
                                {
                                    sdbuf[sdbuf_count] = 0x06;//气体浓度
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x02;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;  //浓度高位
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //浓度低位
                                    sdbuf_count++;
                                }

                                if((Data_ID[0] == 0x10)||(!Data_ID[0]))//高液位报警值
                                {
                                    sdbuf[sdbuf_count] = 0x10;//高液位
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x04;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;  //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x03; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] == 0x11)||(!Data_ID[0]))//高高液位报警值
                                {
                                    sdbuf[sdbuf_count] = 0x11;//高高液位
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x04;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;  //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x06; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] == 0x12)||(!Data_ID[0]))//低液位报警值
                                {
                                    sdbuf[sdbuf_count] = 0x12;//低液位
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x04;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;  //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x03; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] == 0x13)||(!Data_ID[0]))//低低液位报警值
                                {
                                    sdbuf[sdbuf_count] = 0x13;//低低液位
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x04;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;  //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x06; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00; //
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] == 0x14)||(!Data_ID[0]))//浓度预警值
                                {
                                    sdbuf[sdbuf_count] = 0x14;//气体浓度
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x02;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x25;
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] == 0x15)||(!Data_ID[0]))//浓度报警值
                                {
                                    sdbuf[sdbuf_count] = 0x15;//气体浓度
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x02;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x00;
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 0x40;
                                    sdbuf_count++;
                                }
                                if((Data_ID[0] != 0x02) && (Data_ID[0] != 0x03) && (Data_ID[0] != 0x04) && (Data_ID[0] != 0x05) && (Data_ID[0] != 0x06) &&
                                        (Data_ID[0] != 0x10) && (Data_ID[0] != 0x11) && (Data_ID[0] != 0x12) && (Data_ID[0] != 0x13) && (Data_ID[0] != 0x14) &&
                                        (Data_ID[0] != 0x15))
                                {
                                    sdbuf[5] = 0xe0|(0x1f&M_St);  //M_St
                                    sdbuf[sdbuf_count] = 2;//确认消息
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = Data_ID[0];
                                    sdbuf_count++;
                                    sdbuf[sdbuf_count] = 5;
                                    sdbuf_count++;
                                }
                            }
                        }
                        sdbuf[7] = sdbuf_count - 8;
                        if((num_send = send(nsockfd,sdbuf,sdbuf_count,0)) == -1)
                        {
                            printf("ERROR: Failed to sent string.\n");
                            Flag_TcpClose_FromTcp = 1; //close(nsockfd);
                            break;
                        }
                        printf("OK: Sent %d bytes sucessful, please enter again.\n", num_send);
                        sdbuf_count = 10;
                    }

                   //液位仪相关数据上传
                    //printf("DB_LG   %d###################\n",DB_Ad_Lg);
                    if(DB_Ad_Lg == 2 && DB_Ad[0] == 0x60) //液位仪相关数据
                    {
                        sdbuf[0] = 0x02;//把数据节点写死
                        sdbuf[1] = 0x01;
                        sdbuf[2] = 0x07;
                        sdbuf[3] = ID_M;
                        sdbuf[4] = 0;  //Mc
                        sdbuf[5] = 0x20;
                        sdbuf[6] = 0;  //M_Lg 高位
                        sdbuf[8] = 2;  //DB_Ad_Lg
                        sdbuf[9] = 0x60; //DB_Ad
                        sdbuf[sdbuf_count] = revbuf[10];//DB_Ad
                        sdbuf_count++;
                        sdbuf[sdbuf_count] = 0x01;
                        sdbuf_count++;
                        if(Flag_Enable_liqiud == 1)
                        {
                            sdbuf[sdbuf_count] = !Flag_sta_liquid;//液位仪状态
                            sdbuf_count++;
                        }
                        else
                        {
                            sdbuf[sdbuf_count] = 0xff;//液位仪状态,关闭监测
                            sdbuf_count++;
                        }
                        sdbuf[7] = sdbuf_count - 8;
                        if((num_send = send(nsockfd,sdbuf,sdbuf_count,0)) == -1)
                        {
                            printf("ERROR: Failed to sent string.\n");
                            Flag_TcpClose_FromTcp = 1; //close(nsockfd);
                            break;
                        }
                        printf("OK: Sent %d bytes sucessful, please enter again.\n", num_send);
                        sdbuf_count = 10;
                    }
                    //油泵相关数据上传
                     if(DB_Ad_Lg == 2 && DB_Ad[0] == 0x61) //液位仪相关数据
                     {
                         sdbuf[0] = 0x02;//把数据节点写死
                         sdbuf[1] = 0x01;
                         sdbuf[2] = 0x07;
                         sdbuf[3] = ID_M;
                         sdbuf[4] = 0;  //Mc
                         sdbuf[5] = 0x20;
                         sdbuf[6] = 0;  //M_Lg 高位
                         sdbuf[8] = 2;  //DB_Ad_Lg
                         sdbuf[9] = 0x61; //DB_Ad
                         sdbuf[sdbuf_count] = revbuf[10];//DB_Ad
                         sdbuf_count++;
                         sdbuf[sdbuf_count] = 0x01;
                         sdbuf_count++;
                         if(Flag_Enable_pump == 1)
                         {
                             sdbuf[sdbuf_count] = !Flag_sta_pump;//油泵状态
                             sdbuf_count++;
                         }
                         else
                         {
                             sdbuf[sdbuf_count] = 0xff;//油泵状态,关闭监测
                             sdbuf_count++;
                         }
                         sdbuf[7] = sdbuf_count - 8;
                         if((num_send = send(nsockfd,sdbuf,sdbuf_count,0)) == -1)
                         {
                             printf("ERROR: Failed to sent string.\n");
                             Flag_TcpClose_FromTcp = 1; //close(nsockfd);
                             break;
                         }
                         printf("OK: Sent %d bytes sucessful, please enter again.\n", num_send);
                         sdbuf_count = 10;
                     }
                }

            }
//            Wait_Send_Flag = 0;
       }
       return 0;
}

int net_detect(char* net_name)
{
    int skfd = 0;
    struct ifreq ifr;
    //struct sockaddr_in *pAddr = NULL;

    skfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(skfd < 0)
    {
        //printf("%s:%d Open socket error!\n", __FILE__, __LINE__);
        close(skfd);
        return -1;
    }

    strcpy(ifr.ifr_name, net_name);
    if(ioctl(skfd, SIOCGIFFLAGS, &ifr) <0 )
    {
        //printf("%s:%d IOCTL error!\n", __FILE__, __LINE__);
        //printf("Maybe ethernet inferface %s is not valid!", ifr.ifr_name);
        close(skfd);
        return -1;
    }

    if(ifr.ifr_flags & IFF_RUNNING)
    {
        //printf("%s is running :)\n", ifr.ifr_name);
    }
    else
    {
        //printf("%s is not running :(\n", ifr.ifr_name);
        close(skfd);
        return -1; //自己按需求添加
    }

//    if(ioctl(skfd,SIOCGIFADDR,&ifr)<0)
//    {
//        //printf("SIOCGIFADDR IOCTL error!\n");
//        close(skfd);
//        return -1;
//    }

//    pAddr = (struct sockaddr_in *)&(ifr.ifr_addr);
//    printf("ip addr :[%s]\n", inet_ntoa(pAddr->sin_addr));

//    if(ioctl(skfd,SIOCGIFHWADDR,&ifr)<0)
//    {
//        printf("SIOCGIFHWADDR IOCTL error!\n");
//        close(skfd);
//        return -1;
//    }
//    printf("mac addr: %02x:%02x:%02x:%02x:%02x:%02x\n",
//        (unsigned char)ifr.ifr_hwaddr.sa_data[0],
//        (unsigned char)ifr.ifr_hwaddr.sa_data[1],
//        (unsigned char)ifr.ifr_hwaddr.sa_data[2],
//        (unsigned char)ifr.ifr_hwaddr.sa_data[3],
//        (unsigned char)ifr.ifr_hwaddr.sa_data[4],
//        (unsigned char)ifr.ifr_hwaddr.sa_data[5]);
    close(skfd);
    return 0;
}




