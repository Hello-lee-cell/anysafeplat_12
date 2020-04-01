#ifndef MAIN_MAIN_ZHONGYOU_H
#define MAIN_MAIN_ZHONGYOU_H
#include <qthread.h>

void *shutdown_tcp(void*);//静态函数

//udp
void *talk_udp_zy(void*);
void *listen_udp_zy(void*);
extern char *IP_local_ZY;
extern char Sdbuf_Ifis[512];
extern int porttcpclietn;
extern unsigned char Wait_Send_Flag;

class main_main_zhongyou :public QThread
{
	Q_OBJECT
public:
	bool stop;
	explicit main_main_zhongyou(QObject *parent=0);
	void run();

	void net_data();
signals:
	unsigned char net_reply(int send_buf_count);
private slots:

};

#endif // MAIN_MAIN_ZHONGYOU_H
