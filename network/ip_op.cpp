#include<stdio.h>
#include<unistd.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<net/if.h>
#include<string.h>
#include<sys/ioctl.h>
#include"config.h"

/*获取本地ip*/
int get_local_ip(char * ifname,char * ip)
{
    char *temp = NULL;
    int inet_sock;
    struct ifreq ifr;
    inet_sock = socket(AF_INET,SOCK_DGRAM,0);
    memset(ifr.ifr_name,0,sizeof(ifr.ifr_name));
    memcpy(ifr.ifr_name,ifname,strlen(ifname));

    if(0 != ioctl(inet_sock,SIOCGIFADDR,&ifr))
    {
        perror("ioctl error");
        return -1;
    }
    temp = inet_ntoa(((struct sockaddr_in*)&(ifr.ifr_addr))->sin_addr);
    memcpy(ip, temp, strlen(temp));

	if(0 != ioctl(inet_sock,SIOCGIFBRDADDR,&ifr))
	{
		perror("udp ioctl error");
		return -1;
	}
	temp = inet_ntoa(((struct sockaddr_in*)&(ifr.ifr_addr))->sin_addr);
	memcpy(IP_BRO,temp,strlen(temp));

    close(inet_sock);
    return 0;
}
/*远端IP转字符串*/
/*void IPToString(char* IP)
{
   unsigned char add1,add2,add3,add4;
   add1=IP[0];
   add2=IP[1];
   add3=IP[2];
   add4=IP[3];
   sprintf(IP_DES,"%d.%d.%d.%d",add1,add2,add3,add4);
}*/
/*本地IP提取*/
void ip_value(char *ip)
{
	unsigned long ip_l = inet_addr(ip);
	unsigned char *ip_a = (unsigned char*)&ip_l;
	ipa = ip_a[0];
	ipb = ip_a[1];
	ipc = ip_a[2];
	ipd = ip_a[3];
}
