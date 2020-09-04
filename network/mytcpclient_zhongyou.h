#ifndef MYTCPCLIENT_ZHONGYOU_H
#define MYTCPCLIENT_ZHONGYOU_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QHostAddress>
#include <QMessageBox>
#include <QThread>
#include <semaphore.h>
void *client_tcp_zy(void*);
extern int PORT_TCP;
extern char Remote_Ip[20];
extern unsigned char Flag_connect_tcpClient;
extern unsigned char Flag_TcpClient_SendOver_Ifis;//判断是否发送完成。
extern unsigned int receiveudp;
extern QString ip_renzheng;
class mytcpclient_zhongyou :public QThread
{
	Q_OBJECT
public:
	bool stop;
	explicit mytcpclient_zhongyou(QWidget *parent=0);
	void run();
	void send_tcp();
	void client_keep_ali(int sockfd);
private:
	QTimer *timer_rtcp;//时间显示倒计时
signals:

private slots:
	unsigned char net_reply(int send_buf_count);
	void reset_sem();//重置信号量

};

#endif // MYTCPCLIENT_ZHONGYOU_H
