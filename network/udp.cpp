#include<unistd.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<arpa/inet.h>
#include<math.h>
#include<pthread.h>
#include<signal.h>
#include<fcntl.h>
#include<QDebug>
#include<QDateTime>
#include<QFile>
#include"ip_op.h"
#include"config.h"
#include"io_op.h"
#include"main_main.h"
#include"database_op.h"
unsigned char UDP_STATE = 0;
//unsigned char net_state = 0;
void sig_alarm()
{
//  printf("offline over 30s!\n");
    Flag_TcpClose_FromUdp = 1;
    //add_value_netinfo("网络已断开");
}

/*心跳发送*/
void* talk_udp(void*)
{
    int count = 0;
    int count_history  = 0;

    int sockfd_udp;                     // udp通信设备号
    int num_udprecv;                    // Counter of received bytes
    struct sockaddr_in addr_remote;    	// Host address information本机地址信息


    /* Get the Socket file descriptor */
	if ((sockfd_udp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
//        printf("ERROR: Failed to obtain Socket Descriptor!\n");
        return (0);
    }

    /* Fill the socket address struct */
	memset (addr_remote.sin_zero,0,8);
    addr_remote.sin_family = AF_INET;          		// Protocol Family
    addr_remote.sin_port = htons(PORT_UDP);//0x9e0d;//htons(PORT_UDP);   // Port number
	//addr_remote.sin_addr.s_addr=htonl(INADDR_ANY);
	inet_pton(AF_INET, IP_BRO, &addr_remote.sin_addr); 	// Net Address
	memset (addr_remote.sin_zero,0,8);

	const int opt = 1;
	int nb=0;
	if((nb=setsockopt(sockfd_udp,SOL_SOCKET,SO_BROADCAST,(char*)&opt,sizeof(opt)))==-1)
	{
		printf("set udptalk socket error!\n");
	}
//	if( bind(sockfd_udp, (struct sockaddr*)&addr_remote, sizeof(addr_remote) ) == -1)
//	{
//		perror("socket");
//		exit(1);
//	}

    //history_net_write("Udp_talk");
    add_value_netinfo("UDP客户端广播中");
    char sdbuf[10]={(char)ipa,(char)ipb,(char)ipc,(char)ipd,0x0d,0x9f,0x07,(char)ID_M,0x01,UDP_STATE};
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
        sdbuf[9] = UDP_STATE;
        num_udprecv = sendto(sockfd_udp, sdbuf,sizeof(sdbuf),0,(struct sockaddr *)&addr_remote, sizeof(struct sockaddr_in));
        if( num_udprecv < 0 )
        {
 //           printf ("ERROR: Failed to send your data!\n");
        }
        else
        {
			printf ("OK: Send to all total %d bytes %d !\n",num_udprecv,PORT_UDP);
			for(unsigned int i = 0;i<10;i++)
			{
				printf("%02x ",sdbuf[i]);
			}
			printf("\n");
            if(time_count_talk==1)
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
					            signal(SIGALRM,(__sighandler_t)sig_alarm);
								alarm(22);
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
            time_count_talk = 1;
        }
    }
}
void* listen_udp(void*)
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
    if((setsockopt(s,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on)))<0)
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
	int ret = pthread_create(&id_udptalk, NULL, talk_udp, (void*)t);
	if(ret != 0)
	{
		printf("Can not create thread!");

		exit(1);
	}

    //keep listening for data
	int num_udprecv;

    while(1)
    {
        memset(buf, 0, sizeof(buf));
		if( num_udprecv < 0 )
		{
			printf ("ERROR: Failed to send your data!\n");
		}
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
            time_count_talk=0;
            alarm(0);
        }

    }

    close(s);
}
