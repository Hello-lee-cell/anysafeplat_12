#ifndef NET_ISOOSI_CQ_H
#define NET_ISOOSI_CQ_H
#include <QMainWindow>
#include <QTcpServer>
#include <QHostAddress>
#include <QThread>
class net_isoosi_cq:public QThread
{
    Q_OBJECT
public:
    explicit net_isoosi_cq(QWidget *parent = 0);
    void stop(); //用于结束线程
protected:
    void run();
private:
    volatile bool is_runnable = true;

signals:

public slots:
	void refueling_gun_data(QString gun_num,QString AlvR,QString GasCur,QString GasFlow,QString FuelCur,QString FuelFlow,QString gas_con,QString gas_tem,QString DynbPrs);
    void environmental_data(QString dynbPress,QString tankPress,QString unloadgasCon,QString DevicegasCon,QString GasTemp,QString GasVolume);
    void setup_data(QString PVFrwPrs,QString PVRevPrs,QString TrOpenPrs,QString TrStopPrs);
	void gun_warn_data(QString gun_data,QString gun_num,QString TightAlm,QString DynbPAlm,QString TankPAlm,QString TankZerom,QString Prevalve,QString prevavlelimit,QString DevOpenAlm,QString DeviceAlm,QString xieyousta );
    void refueling_gun_stop(QString gun_num,QString operate,QString Event);
	void refueling_wrongdata(QString warn_data);

private slots:
    unsigned int CRC16_Checkout ( unsigned char *puchMsg, unsigned int usDataLen );
    void tcp_client();
    void client_keep_ali(int sockfd);
    void send_tcpclient_data(QString data);
};
#endif // NET_ISOOSI_CQ_H
