#ifndef POST_WEBSERVICE_DONGGUAN_H
#define POST_WEBSERVICE_DONGGUAN_H

#include <QMainWindow>
#include<QNetworkReply>

class post_webservice_dongguan : public QMainWindow
{
    Q_OBJECT
    public:
        post_webservice_dongguan(QWidget *parent = 0);

private slots:
    void requestFinished(QNetworkReply* reply);

    void post_message(QString xml_data);

    void Send_Configurationdata(QString id,QString jyqs,QString pvz,QString pvf,QString hclk,QString yzqh);
    void Send_Warndata(QString id,QString al,QString mb,QString yz,QString ygyl,QString ygly,QString pvzt,QString pvljzt,QString hclzt,QString hclnd,QString xyhqg);
    void Send_Oilgundata(QString id,QString jyjid,QString jyqid,QString al,QString qls,QString qll,QString yls,QString yll,QString hyqnd,QString hyqwd,QString yz);
    void Send_Surroundingsdata(QString id,QString ygyl,QString yzyl,QString yqkj,QString xnd,QString hclnd,QString yqwd);
    void Send_Wrongsdata(QString id,QString type);
    void Send_Closegunsdata(QString id,QString jyjid,QString jyqid,QString operate,QString event);
    void Send_Stagundata(QString id,QString status);

private:
    QNetworkAccessManager *m_accessManager;
};

QString CreatXml_dg(QString version,QString data_id,QString user_id,QString time,QString type,QString sec,QString bus_data,QString HMAC_data);
QString ReadXml_dg(QString read_xml);

QString XML_Configurationdata_dg(QString id,QString data,QString jyqs,QString pvz,
                              QString pvf,QString hclk,QString yzqh);
QString XML_Warndata(QString id,QString data,QString al,QString mb,
                              QString yz,QString ygyl,QString ygly,QString pvzt,QString pvljzt,QString hclzt,QString hclnd,QString xyhqg);
QString XML_Oilgundata(QString id,QString data,QString jyjid,QString jyqid,QString al,
                              QString qls,QString qll,QString yls,QString yll,QString hyqnd,QString hyqwd,QString yz );
QString XML_Surroundingsdata(QString id,QString data,QString ygyl,QString yzyl,QString yqkj,QString xnd,QString hclnd,QString yqwd);
QString XML_Wrongsdata_dg(QString id,QString data,QString type);
QString XML_Closegunsdata_dg(QString id,QString data,QString jyjid,QString jyqid,QString operate,QString event);
QString XML_Stagundata_dg(QString id,QString data,QString status);

#endif // POST_WEBSERVICE_DONGGUAN_H
