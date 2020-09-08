#ifndef POST_FOSHAN_H
#define POST_FOSHAN_H

#include <QThread>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkAccessManager>

class post_foshan:public QThread
{
	Q_OBJECT
public:
	explicit post_foshan(QObject *parent = 0);
	void stop();
	void run();

private:
	volatile bool is_runnable = true;
	QNetworkAccessManager *m_accessManager;
protected:

private:
	//发送数据
	void Post_Send_FoShan(unsigned char Flag_DataType,QString data);
	//数据打包
	void Data_Pack(unsigned int date_type,QString BUSINESSCONTENT);

	//以下json数据转换函数
	QString json_data_pack(QString VERSION,QString DATAID,QString USERID,QString TIME,QString TYPE,
						   QString SEC, QString BUSINESSCONTENT,QString HMAC);
	QString json_login(QString account,QString pwd);
	QString json_logout(QString account,QString pwd);
	QString json_setinfo(QString DataId,QString Date,QString JYQS,QString PVZ,QString PVF,
						 QString HCLK,QString HCLT,QString YZQH);
	QString json_warninfo(QString DataId,QString Date,QString AL,QString MB,QString YZ,QString YGYL
						  ,QString YGLY,QString PVZT,QString PVLJZT,QString HCLZT,QString HCLND,QString XYHQG);
	QString json_gundata(QString DataId,QString Date,QString JYJID,QString JYQID,QString AL,QString QLS,
						 QString QLL,QString YLS,QString YLL,QString HYQND,QString HYQWD,QString YZ,QString STATE);
	QString json_environment(QString DataId,QString Date,QString YGYL,QString YZYL,QString YQKJ,QString XND,QString HCLND,QString YQWD);
	QString json_wrong(QString DataId,QString Date,QString TYPE);
	QString json_gunoperate(QString DataId,QString Date,QString JYJID,QString JYQID,QString OPERATE,QString EVENT);
	QString json_gunsta(QString DataId,QString Date,QString STATUS);
	QString json_stationinfo(QString DataId,QString Date,QString CityCode,QString AreaCode,QString TownCode,QString StationName,
							 QString Company,QString Lon,QString Lat,QString Address,QString Contact,QString Phone,QString JYJNum,
							 QString JYQNum,QString Scale,QString OwnerType,QString HasSystem,QString Manufacturer,QString IsAcceptance,
							 QString OperateStaff);
	QString json_guninfo(QString DataId,QString Date,QString JYJID,QString JYQID);

signals:

private slots:
	void requestFinished(QNetworkReply* reply);
	void TestLogin();

	//以下数据发送准备函数
	void send_login(QString account,QString pwd);
	void send_logout(QString account,QString pwd);
	void send_setinfo(QString DataId,QString Date,QString JYQS,QString PVZ,QString PVF,
						 QString HCLK,QString HCLT,QString YZQH);
	void send_warninfo(QString DataId,QString Date,QString AL,QString MB,QString YZ,QString YGYL
						  ,QString YGLY,QString PVZT,QString PVLJZT,QString HCLZT,QString HCLND,QString XYHQG);
	void send_gundata(QString DataId,QString Date,QString JYJID,QString JYQID,QString AL,QString QLS,
						 QString QLL,QString YLS,QString YLL,QString HYQND,QString HYQWD,QString YZ,QString STATE);
	void send_environment(QString DataId,QString Date,QString YGYL,QString YZYL,QString YQKJ,QString XND,QString HCLND,QString YQWD);
	void send_wrong(QString DataId,QString Date,QString TYPE);
	void send_gunoperate(QString DataId,QString Date,QString JYJID,QString JYQID,QString OPERATE,QString EVENT);
	void send_gunsta(QString DataId,QString Date,QString STATUS);
	void send_stationinfo(QString DataId,QString Date,QString CityCode,QString AreaCode,QString TownCode,QString StationName,
							 QString Company,QString Lon,QString Lat,QString Address,QString Contact,QString Phone,QString JYJNum,
							 QString JYQNum,QString Scale,QString OwnerType,QString HasSystem,QString Manufacturer,QString IsAcceptance,
							 QString OperateStaff);
	void send_guninfo(QString DataId,QString Date,QString JYJID,QString JYQID);
	void send_station_message();//发送油站的基本信息，仅仅一次，这个函数用来预处理
public slots:
	void post_test(unsigned int i);
};

#endif // POST_FOSHAN_H
