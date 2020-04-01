#ifndef NET_TCPCLIENT_HB_H
#define NET_TCPCLIENT_HB_H
#include <QMainWindow>
#include <QTcpServer>
#include <QHostAddress>
#include <QThread>
#include <QTimer>

class net_tcpclient_hb:public QThread
{
    Q_OBJECT
public:
    bool stop;
    explicit net_tcpclient_hb(QWidget *parent = 0);
    void run();
signals:

public slots:

private:
    QTimer *Client_timer;

private slots:
    void tcp_client();
    void client_keep_ali(int sockfd);
    void send_tcpclient_data(unsigned int data_length);
    void warn_data();
    int select_data_type(unsigned int data);
    //选则传感器编号
    int select_data_num(unsigned int data);
    void send_heart();
};
#endif // NET_TCPCLIENT_HB_H
