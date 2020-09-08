#ifndef POST_NANJING_H
#define POST_NANJING_H
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkAccessManager>
#include <QMainWindow>

class post_nanjing : public QMainWindow
{
	Q_OBJECT
public:
	explicit post_nanjing(QWidget *parent = 0);
private:
	void Send_Messge_Webservice();
	void post_message(QString xml_data);
	QString ReadXml(QString read_xml);
	QString CreatXml(QString version,QString data_id,QString user_id,QString time,QString type,QString sec,QString bus_data);

	QString XML_Warndata(QString id,QString data,QString al,QString mb,QString yz,QString ygyl,QString clzznd,QString pv,
						 QString clzzqd,QString clzztz,QString xyhqg);
	QString XML_Oilgundata(QString id,QString data,QString jyjid,QString jyqid,QString al,QString qls,QString qll,
						   QString yls,QString yll, QString yqnd,QString yqwd,QString yz,QString cxsc);
	QString XML_Surroundingsdata(QString id,QString data,QString ygyl,QString yzyl,QString yqkj,
								 QString xnd,QString hclnd,QString yqwd);
	QString XML_Wrongsdata(QString id,QString data,QString type);
private slots:
	void requestFinished(QNetworkReply* reply);
	void Send_Warndata(QString id,QString al,QString mb,QString yz,QString ygyl,QString clzznd,QString pv,
					   QString clzzqd,QString clzztz,QString xyhqg); //发送报警数据报文
	void Send_Oilgundata(QString id,QString jyjid,QString jyqid,QString al,QString qls,QString qll,QString yls,QString yll,
						 QString yqnd,QString yqwd,QString yz,QString cxsc); //发送油枪数据报文
	void Send_Surroundingsdata(QString id,QString ygyl,QString yzyl,QString yqkj,
							   QString xnd,QString hclnd,QString yqwd);    //发送环境数据报文
	void Send_Wrongsdata(QString id,QString type);       //发送故障数据报文
};

#endif // POST_NANJING_H
