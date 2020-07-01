#ifndef NET_ISOOSI_HEFEI_H
#define NET_ISOOSI_HEFEI_H
#include <QThread>


class net_isoosi_hefei:public QThread
{
	Q_OBJECT
public:
	explicit net_isoosi_hefei(QWidget *parent = 0);
	void stop();//用于结束线程
protected:
	void run();
private:
	volatile bool is_runnable = true;
	void tcp_client();
	void send_tcpclient_data(QString data);

	QString buildNormalMsg(QString preMsg);
	QString buildAnswerDataSegment(QString ST,QString CN,QString MN,QString PW,QString Flag,QString dataArea);
	QString buildUploadDataSegment(QString ST,QString CN,QString MN,QString PW,QString dataArea);
	QString buildOverLimitAlarm(QString contaminant,QString contaminantType,QString dataNum,QString alarmType);
	QString buildRealtimeData(QStringList contaminantLists,QString contaminantType,QStringList dataNums);
	unsigned int CRC16_Checkout(unsigned char *puchMsg,unsigned int usDataLen);
	void client_keep_ali(int sockfd);
private slots:
	void send_surround_message(QString YGYL,QString YZYL,QString YQWD);

};

#endif // NET_ISOOSI_HEFEI_H
