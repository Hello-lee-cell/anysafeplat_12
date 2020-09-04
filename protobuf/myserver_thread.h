#ifndef MYSERVER_THREAD_H
#define MYSERVER_THREAD_H

#include <QMainWindow>
#include <QTcpServer>
#include <QHostAddress>
#include <QThread>
#include <QTimer>

class myserver_thread:public QThread
{
	Q_OBJECT
public:
	void stop();
	explicit myserver_thread(QWidget *parent = 0);
	void run();
signals:

private:
	volatile bool is_runnable = true;
	void tcp_client();
	void client_keep_ali(int sockfd_hb);
	void senddata(unsigned char *send_data,int data_length);
	int pack_user_data(uint8_t *buffer);
	int unpack_user_data(const uint8_t *buffer, size_t len);
public:

public slots:
	void send_xielou(QString data_type,QString senser_num,QString sensor_type,QString sensor_sta,QString sensor_data);
	void send_reoilgas();
	void send_settingdata(QString OilTankNum,QString OilTankType,QString PipeNum,QString DispenerNum,QString BasinNum,
	QString AmountDispenerNum,QString GunNum1,QString GunNum2,	QString GunNum3,QString GunNum4,QString GunNum5,QString GunNum6,
	QString GunNum7,QString GunNum8,QString GunNum9,QString GunNum10,QString GunNum11,QString GunNum12,
	QString PipPreEn,QString TankPreEN,	QString HclFgaEn,
   //安全防护部分
	QString FgaNum,	QString RadarEn,QString CrashNum,
   //预留设置部分
	QString Reserve1, QString Reserve2,	QString Reserve3 );
public slots:

};

#endif // MYSERVER_THREAD_H
