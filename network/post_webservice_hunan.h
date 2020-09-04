#ifndef POST_WEBSERVICE_HUNAN_H
#define POST_WEBSERVICE_HUNAN_H
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkAccessManager>

#include <QMainWindow>


class post_webservice_hunan : public QMainWindow
{
	Q_OBJECT
public:
	explicit post_webservice_hunan(QWidget *parent = 0);

signals:
	void show_data_HuNan(QString data);

public slots:

private slots:
	void requestFinished_HuNan(QNetworkReply* reply);
	void post_message_HuNan(QString xml_data);
	void Send_Messge_Webservice_HuNan();

	void Send_Requestdata_HuNan(QString type,QString data);     //发送请求数据报文
	void Send_Configurationdata_HuNan(QString id,QString jyqs,QString pvz,QString pvf,QString hclk,QString yzqh);   //发送配置数据报文
	void Send_Warndata_HuNan(QString id,QString data,QString al,QString mb,
	                         QString yz,QString ygyl,QString clzznd,QString pv,QString clzzqd,QString clzztz,QString xyhqg); //发送报警数据报文
	void Send_Oilgundata_HuNan(QString id,QString data,QString jyjid,QString jyqid,QString al,
	                           QString qls,QString qll,QString yls,QString yll,QString yqnd,QString yqwd,QString yz); //发送油枪数据报文
	void Send_Surroundingsdata_HuNan(QString id,QString data,QString ygyl,QString yzyl,QString xnd,QString clnd,QString yqwd,QString yqkj);    //发送环境数据报文
	void Send_Wrongsdata_HuNan(QString id,QString type);       //发送故障数据报文
	void Send_Closegunsdata_HuNan(QString id,QString jyjid,QString jyqid,QString operate,QString event);       //发送关枪数据报文
	void Send_Stagundata_HuNan(QString id,QString status);      //发送油枪状态报文
private:
	QString CreatXml_HuNan(QString version,QString data_id,QString user_id,QString time,QString type,QString sec,QString bus_data);
	QString ReadXml_HuNan(QString read_xml);
	QString XML_Requestdata_HuNan(QString type,QString data);
	QString XML_Configurationdata_HuNan(QString id,QString data,QString jyqs,QString pvz,
								  QString pvf,QString hclk,QString yzqh);
	QString XML_Warndata_HuNan(QString id,QString data,QString al,QString mb,
								  QString yz,QString ygyl,QString clzznd,QString pv,QString clzzqd,QString clzztz,QString xyhqg);
	QString XML_Oilgundata_HuNan(QString id,QString data,QString jyjid,QString jyqid,QString al,
								  QString qls,QString qll,QString yls,QString yll,QString yqnd,QString yqwd,QString yz );
	QString XML_Surroundingsdata_HuNan(QString id,QString data,QString ygyl,QString yzyl,QString xnd,QString clnd,QString yqwd,QString yqkj);
	QString XML_Wrongsdata_HuNan(QString id,QString data,QString type);
	QString XML_Closegunsdata_HuNan(QString id,QString data,QString jyjid,QString jyqid,QString operate,QString event);
	QString XML_Stagundata_HuNan(QString id,QString data,QString status);
};

#endif // POST_WEBSERVICE_HUNAN_H
