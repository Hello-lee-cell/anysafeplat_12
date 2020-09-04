#ifndef POST_LANGFANG_H
#define POST_LANGFANG_H
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkAccessManager>
#include <QMainWindow>

class post_langfang : public QMainWindow
{
	Q_OBJECT
public:
	explicit post_langfang(QWidget *parent = 0);

signals:
	void show_data(QString data);

public slots:

private slots:
	void requestFinished(QNetworkReply* reply);
	void post_message(QString xml_data);
	void Send_Messge_Webservice();

	void Send_Requestdata(QString type,QString data);     //发送请求数据报文
	void Send_Configurationdata(QString id,QString jyqs,QString pvz,QString pvf,QString hclk,QString hclt,QString yzqh);   //发送配置数据报文
	void Send_Warndata(QString id,QString al,QString mb,QString yz,QString ygyl,QString ygly,QString pvzt,
					   QString pvljzt,QString hclzt,QString hclnd,QString xyhqg); //发送报警数据报文
	void Send_Oilgundata(QString id,QString jyjid,QString jyqid,QString al,QString qls,QString qll,QString yls,QString yll,
						 QString hyqnd,QString hyqwd,QString yz); //发送油枪数据报文
	void Send_Surroundingsdata(QString id,QString ygyl,QString yzyl,QString yqkj,
							   QString xnd,QString hclnd,QString yqwd);    //发送环境数据报文
	void Send_Wrongsdata(QString id,QString type);       //发送故障数据报文
	void Send_Closegunsdata(QString id,QString jyjid,QString jyqid,QString operate,QString event);       //发送关枪数据报文
	void Send_Stagundata(QString id,QString status);      //发送油枪状态报文
private:
	QString CreatXml(QString version,QString data_id,QString user_id,QString time,QString type,QString sec,QString bus_data);
	QString ReadXml(QString read_xml);
	QString XML_Requestdata(QString type,QString data);
	QString XML_Configurationdata(QString id,QString data,QString jyqs,QString pvz,
								  QString pvf,QString hclk,QString hclt,QString yzqh);
	QString XML_Warndata(QString id,QString data,QString al,QString mb,QString yz,QString ygyl,QString ygly,
						 QString pvzt,QString pvljzt,QString hclzt,QString hclnd,QString xyhqg);
	QString XML_Oilgundata(QString id,QString data,QString jyjid,QString jyqid,QString al,
								  QString qls,QString qll,QString yls,QString yll,QString hyqnd,QString hyqwd,QString yz );
	QString XML_Surroundingsdata(QString id,QString data,QString ygyl,QString yzyl,QString yqkj,
								 QString xnd,QString hclnd,QString yqwd);
	QString XML_Wrongsdata(QString id,QString data,QString type);
	QString XML_Closegunsdata(QString id,QString data,QString jyjid,QString jyqid,QString operate,QString event);
	QString XML_Stagundata(QString id,QString data,QString status);
};

#endif // POST_LANGFANG_H
