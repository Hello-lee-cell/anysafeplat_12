#ifndef MYSERVER_H
#define MYSERVER_H

#include <QMainWindow>
#include <QTcpServer>
#include <QHostAddress>
#include <QThread>

class myserver:public QThread
{
	Q_OBJECT
public:
	explicit myserver(QWidget *parent = 0);
	void stop(); //用于结束线程
protected:
	void run();
private:
	volatile bool is_runnable = true;

signals:
	void Myserver_First_Client();//服务器第一次连接，需要上传一次所有状态
public slots:
	void refueling_gun_data(QString gun_num,QString AlvR,QString GasCur,QString GasFlow,QString FuelCur,QString FuelFlow,QString DynbPrs);
	void environmental_data(QString dynbPress,QString tankPress,QString unloadgasCon,QString DevicegasCon,QString GasTemp,QString GasVolume);
	void setup_data(QString PVFrwPrs,QString PVRevPrs,QString TrOpenPrs,QString TrStopPrs);
	void gun_warn_data(QString gun_data,QString TightAlm,QString DynbPAlm,QString TankPAlm,QString DeviceAlm,QString PVTapAlm,QString DevOpenAlm,QString DevStopAlm);
	void refueling_gun_stop(QString gun_num,QString operate,QString Event);
	void refueling_gun_sta(QString gun_data);
	void xielousta(QString data_type,QString num,QString sensor_type,QString sta,QString data);
	void xielousetup(QString tank_num,QString tank_type,QString pipe_num,QString dispener_num,QString basin_num);

private slots:
	void tcp_client();
	void client_keep_ali(int sockfd);
	void send_tcpclient_data(QString data);
	unsigned int CRC16_Checkout ( unsigned char *puchMsg, unsigned int usDataLen );

private:
	void analysis_xielou_sta();//分析泄漏状态，用于发送
	int net_history(int num,int sta);
};

#endif // MYSERVER_H
