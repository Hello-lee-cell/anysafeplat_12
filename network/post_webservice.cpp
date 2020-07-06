/********************************
 * 福州油气回收数据上传
 * post网络数据上传
 * 2019-10-30  马浩
 * ******************************/
#include "post_webservice.h"
#include <QtGui/QMainWindow>
#include <QtCore/QByteArray>
#include <QtDebug>
#include <QMetaObject>
#include <QtNetwork/qnetworkrequest.h>
#include <QtNetwork/qnetworkreply.h>
#include <QtNetwork/QtNetwork>
#include <QApplication>
#include <bits/stdc++.h>
#include <QObject>
#include <QtXml/QtXml>
#include <QTextDocument>
#include <QtXml/QDomComment>
#include <qmutex.h>
#include "config.h"
#include "database_op.h"

QMutex post_data;

unsigned char send_wrong = 2;//这两个变量用来标志发送失败的类型，做历史记录。
unsigned char network_wrong = 0;
//QString TIME_POST = "";                //在线监控设备当前时间（年月日时分 14 位）
QString TYPE_POST = "";               //业务报文类型（2 位）
//QString SEC_POST = "0";                //加密标识（1 表示业务数据为密文传输，0 表示明文）
QString BUSINESSCONTENT = "";    //业务报文（数据需转化为 base64 编码）8 HMAC HMAC 校验码（预留）
unsigned int flag_numofsurrounddata = 0;
QString xmldata_surround = "";//环境数据需要叠加

QNetworkAccessManager *m_accessManager;

post_webservice::post_webservice(QWidget *parent) :
    QMainWindow(parent)
{
    m_accessManager = new QNetworkAccessManager(this);
    QObject::connect(m_accessManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(requestFinished(QNetworkReply*)));
}
/*************合成要发送的字符串***************
****************测试使用的函数****************/
void post_webservice::Send_Messge_Webservice()
{
    //获取当前时间，数据上传需要
    QDateTime current_datetime = QDateTime::currentDateTime();
    QString current_datetime_qstr = current_datetime.toString("yyyyMMddhhmmss");
    QString xml_data = XML_Warndata("000001",current_datetime_qstr,"1:2;2:1","0","0","0","0","0","0");

    QByteArray byteArray(xml_data.toStdString().c_str(), xml_data.toStdString().length());
    QString xml_data64(byteArray.toBase64());

    QString send_xml = CreatXml("V1.1","000001","3501040007",current_datetime_qstr,"02","0",xml_data64);
    post_message(send_xml);
}
/*************post请求返回槽函数****************/
void post_webservice::requestFinished(QNetworkReply* reply)
{
    if (reply->error() == QNetworkReply::NoError)
    {
		if(Flag_Network_Send_Version == 5)
		{
			if(network_wrong == 1)
			{
				add_value_netinfo("在线监测服务器访问成功");
				network_wrong = 0;
			}

			QByteArray bytes = reply->readAll();
			//qDebug()<<bytes;
			QString string = QString::fromUtf8(bytes);
			QString re = ReadXml(string);
			if(re == "ok")
			{
				//emit show_data(re.append("  send success"));
				qDebug() << re.append("  send success");
				if(send_wrong != 0)
				{
					add_value_netinfo("在线监测数据上传成功");
					send_wrong = 0;
				}
			}
			else
			{
				//emit show_data(re.append("  send fail"));
				qDebug() << re.append("  send fail");
				if(send_wrong != 1)
				{
					add_value_netinfo("在线监测数据上传失败");
					send_wrong = 1;
					Flag_Ifsend = 0;//在这里把全局变量置0，之后时间到了要再重新发送零点的信息
				}
			}
		}
		else
		{
			if(network_wrong == 1)
			{
				add_value_netinfo("在线监测服务器访问成功");
				network_wrong = 0;
			}

			QByteArray bytes = reply->readAll();
			//qDebug()<<bytes;
			QString string = QString::fromUtf8(bytes);
			QString re = ReadXml(string);
			if(re == "01")
			{
				//emit show_data(re.append("  send success"));
				qDebug() << re.append("  send success");
				if(send_wrong != 0)
				{
					add_value_netinfo("在线监测数据上传成功");
					send_wrong = 0;
				}
			}
			else
			{
				//emit show_data(re.append("  send fail"));
				qDebug() << re.append("  send fail");
				if(send_wrong != 1)
				{
					add_value_netinfo("在线监测数据上传失败");
					Flag_Ifsend = 0;//在这里把全局变量置0，之后时间到了要再重新发送零点的信息
					send_wrong = 1;
				}
			}
		}
    }
    else
    {
        if(network_wrong == 0)
        {
			if(Flag_Network_Send_Version == 0){add_value_netinfo("福州在线监测服务器访问失败");}
			if(Flag_Network_Send_Version == 3){add_value_netinfo("唐山在线监测服务器访问失败");}
			if(Flag_Network_Send_Version == 5){add_value_netinfo("江门在线监测服务器访问失败");}
			Flag_Ifsend = 0;//在这里把全局变量置0，之后时间到了要再重新发送零点的信息
            network_wrong = 1;
            qDebug() << "webservice fail~!!";
        }

        qDebug()<<"handle errors here";
        QVariant statusCodeV = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
        //statusCodeV是HTTPserver的对应码，reply->error()是Qt定义的错误码，能够參考QT的文档
        qDebug( "found error ....code: %d %d\n", statusCodeV.toInt(), (int)reply->error());
        qDebug(qPrintable(reply->errorString()));
    }
    reply->deleteLater();
}
/*************post请求槽函数********************/
void post_webservice::post_message(QString xml_data)
{
	//qDebug()<<xml_data;
    post_data.lock();//上锁
    if(net_state == 0)
    {
		//qDebug()<<"send net net  net";
        //POST  //森茂加油站对应协议
        emit show_data(xml_data);
        QNetworkRequest *request = new QNetworkRequest();
		request->setUrl(QUrl(Post_Address));
        request->setHeader(QNetworkRequest::ContentTypeHeader, "text/xml");
        QNetworkReply* reply = m_accessManager->post(*request,xml_data.toUtf8());
        //delete reply;
        delete request;
		request = NULL;
    }
    else
    {
        qDebug() << "network is down, send is prevent!!";
    }
    post_data.unlock();//解锁
}

/*************下面都是发报文的相关函数*******************
 * 增加了判断网络上传功能是否开启的开关
    void Send_Requestdata           //发送请求数据报文             没有添加??
    void Send_Configurationdata     //发送配置数据报文  与北京不同   设置修改后发送     信号来自systemset
    void Send_Warndata              //发送报警数据报文  与北京不同   有报警或零点发送   信号来自fga1000
    void Send_Oilgundata            //发送油枪数据报文  与北京不同   加油后发送        信号来自reoilgasthread
    void Send_Surroundingsdata      //发送环境数据报文  与北京不同   间断发送          信号来自fga1000
    void Send_Wrongsdata            //发送故障数据报文  与北京不同   故障发送          信号来自fga1000和mainwindow
    void Send_Closegunsdata         //发送关枪数据报文  与北京不同   关枪？？？        信号来自fga1000 全部正常
    void Send_Stagundata            //发送油枪状态报文  与北京不同   油枪开关状态？？？ 信号来自fga1000  根据实际

 * ******************************************/
/*****************发送请求数据****************
TYPE 请求类型：''01''表示口令修改请求
DATA 当TYPE 为"01"时，DATA表示修改后的口令值（字符串）
**********************************************/
void post_webservice::Send_Requestdata(QString type,QString data)
{

    if(Flag_Postsend_Enable == 1)
    {
        TYPE_POST = "00";
        //获取当前时间，数据上传需要
        QDateTime current_datetime = QDateTime::currentDateTime();
        TIME_POST = current_datetime.toString("yyyyMMddhhmmss");
        QString xml_data = XML_Requestdata(type,data);

        QByteArray byteArray(xml_data.toStdString().c_str(), xml_data.toStdString().length());
        QString xml_data64(byteArray.toBase64());
        BUSINESSCONTENT = xml_data64;

        QString send_xml = CreatXml(VERSION_POST,DATAID_POST,USERID_POST,TIME_POST,TYPE_POST,SEC_POST,BUSINESSCONTENT);
        post_message(send_xml);
    }
    else
    {
        qDebug() << "network post send disenable!";
    }
	qDebug()<<"send"<<TYPE_POST;
}
/******************发送配置数据****************
ID   对象ID，在本次数据传输中唯一
JYQS 加油枪数量
PVZ  PV阀正向压力值
PVF  PV阀负向压力值
HCLK 后处理装置开启压力值
YZQH 安装液阻传感器加油机
如果不存在某项数据则在数据域中填写“NULL
**********************************************/
void post_webservice::Send_Configurationdata(QString id,QString jyqs,QString pvz,QString pvf,QString hclk,QString yzqh)
{
    if(Flag_Postsend_Enable == 1)
    {
        TYPE_POST = "01";
        //获取当前时间，数据上传需要
        QDateTime current_datetime = QDateTime::currentDateTime();
        TIME_POST = current_datetime.toString("yyyyMMddhhmmss");
        QString xml_data = XML_Configurationdata(id,TIME_POST,jyqs,pvz,pvf,hclk,yzqh);

        QByteArray byteArray(xml_data.toStdString().c_str(), xml_data.toStdString().length());
        QString xml_data64(byteArray.toBase64());
        BUSINESSCONTENT = xml_data64;

        QString send_xml = CreatXml(VERSION_POST,DATAID_POST,USERID_POST,TIME_POST,TYPE_POST,SEC_POST,BUSINESSCONTENT);
        post_message(send_xml);
    }
    else
    {
        qDebug() << "network post send disenable!";
    }
	qDebug()<<"send"<<TYPE_POST;
}
/******************发送报警数据****************
ID     对象ID，在本次数据传输中唯一
AL     A/L（0、1、2、N），N指当日无加油
MB     密闭性（0、1、2、N）
YZ     液阻（0、1、2、N）
YGLY   油罐零压（0、1、2、N）
PVZT   压力/真空阀状态（0、1、2、N）
PVLJZT 压力/真空阀临界压力状态（0、1、2、N)
HCLZT  后处理装置状态（0、1、2、N)
如果不存在某项数据则在数据域中填写“NULL
**********************************************/
void post_webservice::Send_Warndata(QString id,QString al,QString mb,QString yz,QString ygly,QString pvzt,QString pvljzt,QString hclzt)
{
    if(Flag_Postsend_Enable == 1)
    {
        if(Flag_Shield_Network == 1)//如果是处在网络上传屏蔽状态
        {
            al = al.replace(QRegExp("\\:1"),":0");
            al = al.replace(QRegExp("\\:2"),":0"); //报警预警全部替换为正常
            mb = "0";
            yz = "0";
            ygly = "0";
            pvzt = "0";
            pvljzt = "0";
            hclzt = "0";

            TYPE_POST = "02";
            //获取当前时间，数据上传需要
            QDateTime current_datetime = QDateTime::currentDateTime();
            TIME_POST = current_datetime.toString("yyyyMMddhhmmss");
            QString xml_data = XML_Warndata(id,TIME_POST,al,mb,yz,ygly,pvzt,pvljzt,hclzt);

            QByteArray byteArray(xml_data.toStdString().c_str(), xml_data.toStdString().length());
            QString xml_data64(byteArray.toBase64());
            BUSINESSCONTENT = xml_data64;

            QString send_xml = CreatXml(VERSION_POST,DATAID_POST,USERID_POST,TIME_POST,TYPE_POST,SEC_POST,BUSINESSCONTENT);
            post_message(send_xml);
        }
        else
        {
            TYPE_POST = "02";
            //获取当前时间，数据上传需要
            QDateTime current_datetime = QDateTime::currentDateTime();
            TIME_POST = current_datetime.toString("yyyyMMddhhmmss");
            QString xml_data = XML_Warndata(id,TIME_POST,al,mb,yz,ygly,pvzt,pvljzt,hclzt);

            QByteArray byteArray(xml_data.toStdString().c_str(), xml_data.toStdString().length());
            QString xml_data64(byteArray.toBase64());
            BUSINESSCONTENT = xml_data64;

            QString send_xml = CreatXml(VERSION_POST,DATAID_POST,USERID_POST,TIME_POST,TYPE_POST,SEC_POST,BUSINESSCONTENT);
            post_message(send_xml);
        }
    }
    else
    {
        qDebug() << "network post send disenable!";
    }
	qDebug()<<"send"<<TYPE_POST;
}
/******************发送油枪数据****************
ID     对象ID，在本次数据传输中唯一
JYJID  加油机标识
JYQID  加油枪标识
AL     气液比
QLS    油气流速
QLL    油气流量
YLS    燃油流速
YLL    燃油流量
YZ     液阻，单位Pa
如果不存在某项数据则在数据域中填写“NULL
**********************************************/
void post_webservice::Send_Oilgundata(QString id,QString jyjid,QString jyqid,QString al,QString qls,QString qll,QString yls,QString yll,QString yz)
{
    if(Flag_Postsend_Enable == 1)
    {
        if(Flag_Shield_Network == 1)//如果处在屏蔽状态
        {
            if((al.toFloat() <= 120)&&(al.toFloat() >= 100))//如果气液比合格正常上传
            {
                TYPE_POST = "03";
                //获取当前时间，数据上传需要
                QDateTime current_datetime = QDateTime::currentDateTime();
                TIME_POST = current_datetime.toString("yyyyMMddhhmmss");
                QString xml_data = XML_Oilgundata(id,TIME_POST,jyjid,jyqid,al,qls,qll,yls,yll,yz);

                QByteArray byteArray(xml_data.toStdString().c_str(), xml_data.toStdString().length());
                QString xml_data64(byteArray.toBase64());
                BUSINESSCONTENT = xml_data64;

                QString send_xml = CreatXml(VERSION_POST,DATAID_POST,USERID_POST,TIME_POST,TYPE_POST,SEC_POST,BUSINESSCONTENT);
                post_message(send_xml);
            }
            else//如果气液比不合格修正后上传
            {
                int al_num = qrand()%(120-100);//用1.0~1.2之间的随机数代替
                float al_xiuzheng = al_num+100;
                //qDebug()<<num+min;
                al = QString::number(al_xiuzheng,'f',1);
                qls = QString::number( yls.toFloat()*al_xiuzheng/100,'f',1);
                qll = QString::number( yll.toFloat()*al_xiuzheng/100,'f',1); //对气体流速和流量进行修正

                TYPE_POST = "03";
                //获取当前时间，数据上传需要
                QDateTime current_datetime = QDateTime::currentDateTime();
                TIME_POST = current_datetime.toString("yyyyMMddhhmmss");
                QString xml_data = XML_Oilgundata(id,TIME_POST,jyjid,jyqid,al,qls,qll,yls,yll,yz);

                QByteArray byteArray(xml_data.toStdString().c_str(), xml_data.toStdString().length());
                QString xml_data64(byteArray.toBase64());
                BUSINESSCONTENT = xml_data64;

                QString send_xml = CreatXml(VERSION_POST,DATAID_POST,USERID_POST,TIME_POST,TYPE_POST,SEC_POST,BUSINESSCONTENT);
                post_message(send_xml);
            }
        }
        else
        {
            TYPE_POST = "03";
            //获取当前时间，数据上传需要
            QDateTime current_datetime = QDateTime::currentDateTime();
            TIME_POST = current_datetime.toString("yyyyMMddhhmmss");
            QString xml_data = XML_Oilgundata(id,TIME_POST,jyjid,jyqid,al,qls,qll,yls,yll,yz);

            QByteArray byteArray(xml_data.toStdString().c_str(), xml_data.toStdString().length());
            QString xml_data64(byteArray.toBase64());
            BUSINESSCONTENT = xml_data64;

            QString send_xml = CreatXml(VERSION_POST,DATAID_POST,USERID_POST,TIME_POST,TYPE_POST,SEC_POST,BUSINESSCONTENT);
            post_message(send_xml);
        }
    }
    else
    {
        qDebug() << "network post send disenable!";
    }
	qDebug()<<"send"<<TYPE_POST;
}
/******************发送环境数据****************
ID     对象ID，在本次数据传输中唯一
YGYL   油罐压力，单位Pa
YZYL   液阻压力，单位Pa
YQKJ   油气空间，单位L
如果不存在某项数据则在数据域中填写“NULL
数据要采集2~10分钟，该函数要重做  已重做
**********************************************/
void post_webservice::Send_Surroundingsdata(QString id,QString ygyl,QString yzyl,QString yqkj)
{
    if(Flag_Postsend_Enable == 1)
    {
        flag_numofsurrounddata++;//每15次才发送一次
        //获取当前时间，数据上传需要
        QDateTime current_datetime = QDateTime::currentDateTime();
        TIME_POST = current_datetime.toString("yyyyMMddhhmmss");
        QString xml_data = XML_Surroundingsdata(id,TIME_POST,ygyl,yzyl,yqkj);
        xmldata_surround.append(xml_data);
        if(flag_numofsurrounddata >= 15)  //30秒进一次该函数，450秒7分多钟，小于10分钟
        {
            xmldata_surround.prepend("<rows>");
            xmldata_surround.append("</rows>");
			//qDebug() << xmldata_surround;
            TYPE_POST = "04";
            QByteArray byteArray(xmldata_surround.toStdString().c_str(), xmldata_surround.toStdString().length());
            QString xml_data64(byteArray.toBase64());
            BUSINESSCONTENT = xml_data64;

            QString send_xml = CreatXml(VERSION_POST,DATAID_POST,USERID_POST,TIME_POST,TYPE_POST,SEC_POST,BUSINESSCONTENT);
            post_message(send_xml);
            xmldata_surround = "";
            flag_numofsurrounddata = 0;

			    qDebug()<<"send"<<TYPE_POST;
        }
    }
    else
    {
        qDebug() << "network post send disenable!";
    }

}
/******************发送故障数据****************
ID     对象ID，在本次数据传输中唯一
TYPE   故障码
**********************************************/
void post_webservice::Send_Wrongsdata(QString id,QString type)
{
	//qDebug()<<"send wrong data";
    if(Flag_Postsend_Enable == 1)
    {
        TYPE_POST = "05";
        //获取当前时间，数据上传需要
        QDateTime current_datetime = QDateTime::currentDateTime();
        TIME_POST = current_datetime.toString("yyyyMMddhhmmss");
        QString xml_data = XML_Wrongsdata(id,TIME_POST,type);

        QByteArray byteArray(xml_data.toStdString().c_str(), xml_data.toStdString().length());
        QString xml_data64(byteArray.toBase64());
        BUSINESSCONTENT = xml_data64;

        QString send_xml = CreatXml(VERSION_POST,DATAID_POST,USERID_POST,TIME_POST,TYPE_POST,SEC_POST,BUSINESSCONTENT);
        post_message(send_xml);
    }
    else
    {
        qDebug() << "network post send disenable!";
    }
	qDebug()<<"send"<<TYPE_POST;
}
/******************发送油枪关停数据****************
ID       对象ID，在本次数据传输中唯一
JYJID    加油机标识
JYQID    加油枪标识
OPERATE  操作类型 0-关停 1-启用
EVENT    关停或启用事件类型关停事件类型：0 自动关停 1 手动关停 启用事件类型：0（预留） 1 手动启用未知事件类型用 N
**********************************************/
void post_webservice::Send_Closegunsdata(QString id,QString jyjid,QString jyqid,QString operate,QString event)
{
    if(Flag_Postsend_Enable == 1)
    {
        TYPE_POST = "06";
        //获取当前时间，数据上传需要
        QDateTime current_datetime = QDateTime::currentDateTime();
        TIME_POST = current_datetime.toString("yyyyMMddhhmmss");
        QString xml_data = XML_Closegunsdata(id,TIME_POST,jyjid,jyqid,operate,event);

        QByteArray byteArray(xml_data.toStdString().c_str(), xml_data.toStdString().length());
        QString xml_data64(byteArray.toBase64());
        BUSINESSCONTENT = xml_data64;

        QString send_xml = CreatXml(VERSION_POST,DATAID_POST,USERID_POST,TIME_POST,TYPE_POST,SEC_POST,BUSINESSCONTENT);
        post_message(send_xml);
    }
    else
    {
        qDebug() << "network post send disenable!";
    }
	qDebug()<<"send"<<TYPE_POST;
}
/******************发送加油枪状态****************
ID       对象ID，在本次数据传输中唯一
STATUS   加油枪开关
**********************************************/
void post_webservice::Send_Stagundata(QString id,QString status)
{
    if(Flag_Postsend_Enable == 1)
    {
        TYPE_POST = "07";
        //获取当前时间，数据上传需要
        QDateTime current_datetime = QDateTime::currentDateTime();
        TIME_POST = current_datetime.toString("yyyyMMddhhmmss");
        QString xml_data = XML_Stagundata(id,TIME_POST,status);

        QByteArray byteArray(xml_data.toStdString().c_str(), xml_data.toStdString().length());
        QString xml_data64(byteArray.toBase64());
        BUSINESSCONTENT = xml_data64;

        QString send_xml = CreatXml(VERSION_POST,DATAID_POST,USERID_POST,TIME_POST,TYPE_POST,SEC_POST,BUSINESSCONTENT);
        post_message(send_xml);
    }
    else
    {
        qDebug() << "network post send disenable!";
    }
	qDebug()<<"send"<<TYPE_POST;
}


/*************下面都是XML相关函数*******************
 * CreatXml               创建xml字符串
 * ReadXml                读取zml字符串
 * XML_Requestdata        请求数据报文
 * XML_Configurationdata  配置数据报文
 * XML_Warndata           报警数据报文
 * XML_Oilgundata         油枪数据报文
 * XML_Surroundingsdata   环境数据报文
 * XML_Wrongsdata         故障数据报文
 * XML_Closegunsdata      关枪数据报文
 * XML_Stagundata         油枪状态报文
 * ******************************************/
/***************创建xml格式数据*************
1 VERSION 通信协议版本
2 DATAID 数据序号（6 位）
3 USERID 区域代码标识（6 位）+ 加油站标识（4 位）
4 TIME 在线监控设备当前时间（年月日时分 14 位）
5 TYPE 业务报文类型（2 位）
6 SEC 加密标识（1 表示业务数据为密文传输，0 表示明文）
7 BUSINESSCONTENT 业务报文（数据需转化为 base64 编码）8 HMAC HMAC 校验码（预留）

TYPE  00、请求数据；01、配置数据；02、报警数据；03、加油枪数据；04、环境数据；05、故障数据；06、加油枪关停与启用；07、加油枪状态
      HMAC校验没有添加，如有需要，在此函数内添加即可
******************************************/
QString CreatXml(QString version,QString data_id,QString user_id,QString time,QString type,QString sec,QString bus_data)
{
	QDomDocument doc;
	if(Flag_Network_Send_Version == 0) //福州协议
	{
		QString header("version=\"1.0\" encoding=\"UTF-8\"");
		doc.appendChild(doc.createProcessingInstruction("xml",header));
		QDomElement tagFileInfo_envelope = doc.createElementNS("http://schemas.xmlsoap.org/soap/envelope/", "soapenv:Envelope");
		tagFileInfo_envelope.setAttribute("xmlns:web", "http://webservice.ffcs.cn/");
		QDomElement tagFileInfo_header = doc.createElement("soapenv:Header");
		QDomElement tagFileInfo_body = doc.createElement("soapenv:Body");
		QDomElement tagFileInfo_post = doc.createElement("web:post");
		QDomElement tagFileInfo_data = doc.createElement("data");
		QDomElement tagFileInfo_root = doc.createElement("ROOT");

		//QDomElement相当于加标签，QDomText相当于加内容<QDomElement>QDomText</QDomElement>
		//版本号
		QDomElement tagFileVersion = doc.createElement("VERSION");
		QDomText textFileVersion = doc.createTextNode(version);
		//数据类型
		QDomElement tagFileDataId = doc.createElement("DATAID");
		QDomText textFileDataID = doc.createTextNode(data_id);
		//用户ID
		QDomElement tagFileUserId = doc.createElement("USERID");
		QDomText textFileUserID = doc.createTextNode(user_id);
		//时间
		QDomElement tagFileTime = doc.createElement("TIME");
		QDomText textFileTime = doc.createTextNode(time);
		//数据类型
		QDomElement tagFileType = doc.createElement("TYPE");
		QDomText textFileType = doc.createTextNode(type);
		//传输方式 明文
		QDomElement tagFileSec = doc.createElement("SEC");
		QDomText textFileSec = doc.createTextNode(sec);
		//数据报文
		QDomElement tagFileBusin = doc.createElement("BUSINESSCONTENT");
		QDomText textFileBusin = doc.createTextNode(bus_data);
		//HMAC校验  预留
		QDomElement tagFileHmac = doc.createElement("HMAC");
		QDomText textFileHmac = doc.createTextNode("");

		tagFileVersion.appendChild(textFileVersion);
		tagFileDataId.appendChild(textFileDataID);
		tagFileUserId.appendChild(textFileUserID);
		tagFileTime.appendChild(textFileTime);
		tagFileType.appendChild(textFileType);
		tagFileSec.appendChild(textFileSec);
		tagFileBusin.appendChild(textFileBusin);
		tagFileHmac.appendChild(textFileHmac);

		tagFileInfo_root.appendChild(tagFileVersion);
		tagFileInfo_root.appendChild(tagFileDataId);
		tagFileInfo_root.appendChild(tagFileUserId);
		tagFileInfo_root.appendChild(tagFileTime);
		tagFileInfo_root.appendChild(tagFileType);
		tagFileInfo_root.appendChild(tagFileSec);
		tagFileInfo_root.appendChild(tagFileBusin);
		tagFileInfo_root.appendChild(tagFileHmac);

		tagFileInfo_data.appendChild(tagFileInfo_root);
		tagFileInfo_post.appendChild(tagFileInfo_data);
		tagFileInfo_body.appendChild(tagFileInfo_post);
		//tagFileInfo_header.appendChild(tagFileInfo_body);
		tagFileInfo_envelope.appendChild(tagFileInfo_header);
		tagFileInfo_envelope.appendChild(tagFileInfo_body);

		doc.appendChild(tagFileInfo_envelope);
	}
	else if(Flag_Network_Send_Version == 3) //唐山协议，与福建相同
	{
		QString header("version=\"1.0\" encoding=\"UTF-8\"");
		doc.appendChild(doc.createProcessingInstruction("xml",header));
		QDomElement tagFileInfo_envelope = doc.createElementNS("http://schemas.xmlsoap.org/soap/envelope/", "soapenv:Envelope");
		tagFileInfo_envelope.setAttribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
		tagFileInfo_envelope.setAttribute("xmlns:xsd", "http://www.w3.org/2001/XMLSchema");

		QDomElement tagFileInfo_body = doc.createElement("soapenv:Body");
		QDomElement tagFileInfo_post = doc.createElementNS("http://service.core.zlww.com/","post");
		QDomElement tagFileInfo_data = doc.createElementNS("","arg0");
		QDomElement tagFileInfo_root = doc.createElement("ROOT");

		//QDomElement相当于加标签，QDomText相当于加内容<QDomElement>QDomText</QDomElement>
		//版本号
		QDomElement tagFileVersion = doc.createElement("VERSION");
		QDomText textFileVersion = doc.createTextNode(version);
		//数据类型
		QDomElement tagFileDataId = doc.createElement("DATAID");
		QDomText textFileDataID = doc.createTextNode(data_id);
		//用户ID
		QDomElement tagFileUserId = doc.createElement("USERID");
		QDomText textFileUserID = doc.createTextNode(user_id);
		//时间
		QDomElement tagFileTime = doc.createElement("TIME");
		QDomText textFileTime = doc.createTextNode(time);
		//数据类型
		QDomElement tagFileType = doc.createElement("TYPE");
		QDomText textFileType = doc.createTextNode(type);
		//传输方式 明文
		QDomElement tagFileSec = doc.createElement("SEC");
		QDomText textFileSec = doc.createTextNode(sec);
		//数据报文
		QDomElement tagFileBusin = doc.createElement("BUSINESSCONTENT");
		QDomText textFileBusin = doc.createTextNode(bus_data);
		//HMAC校验  预留
		QDomElement tagFileHmac = doc.createElement("HMAC");
		QDomText textFileHmac = doc.createTextNode("");

		tagFileVersion.appendChild(textFileVersion);
		tagFileDataId.appendChild(textFileDataID);
		tagFileUserId.appendChild(textFileUserID);
		tagFileTime.appendChild(textFileTime);
		tagFileType.appendChild(textFileType);
		tagFileSec.appendChild(textFileSec);
		tagFileBusin.appendChild(textFileBusin);
		tagFileHmac.appendChild(textFileHmac);

		tagFileInfo_root.appendChild(tagFileVersion);
		tagFileInfo_root.appendChild(tagFileDataId);
		tagFileInfo_root.appendChild(tagFileUserId);
		tagFileInfo_root.appendChild(tagFileTime);
		tagFileInfo_root.appendChild(tagFileType);
		tagFileInfo_root.appendChild(tagFileSec);
		tagFileInfo_root.appendChild(tagFileBusin);
		tagFileInfo_root.appendChild(tagFileHmac);

		tagFileInfo_data.appendChild(tagFileInfo_root);
		tagFileInfo_post.appendChild(tagFileInfo_data);
		tagFileInfo_body.appendChild(tagFileInfo_post);
		//tagFileInfo_header.appendChild(tagFileInfo_body);
		//tagFileInfo_envelope.appendChild(tagFileInfo_header);
		tagFileInfo_envelope.appendChild(tagFileInfo_body);

		doc.appendChild(tagFileInfo_envelope);
	}
	else if(Flag_Network_Send_Version == 5) //江门协议，与唐山 福州相同
	{
		/*
<?xml version="1.0" encoding="utf-8"?>
<soap:Envelope xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/">
  <soap:Body>
	<post xmlns="http://tempuri.org/">
	   <root>
				<VERSION>V1.1</VERSION>
				<DATAID>000001</DATAID>
				<USERID>1111111111</USERID>
				<TIME>20200410160132</TIME>
				<TYPE>01</TYPE>
				<SEC>01</SEC>
				<BUSINESSCONTENT>PHJvd3M+PHJvdz48SUQ+MDAwMDAxPC9JRD48REFURT4yMDE4MDExODEwMjkzNjwvREFURT48QUw+MTowOzI6MDszOjA7NDoxOzU6MDs2OjA7NzoxOzg6MTs5OjE7MTA6MDsxMTowOzEyOjE7MTM6MTsxNDoxOzE1OjE7MTY6MTsxNzoxOzE4OjE7PC9BTD48TUI+MDwvTUI+PFlaPjA8L1laPjxZR0xZPjA8L1lHTFk+PFBWWlQ+MjwvUFZaVD48UFZMSlpUPjA8L1BWTEpaVD48SENMWlQ+MDwvSENMWlQ+PC9yb3c+PC9yb3dzPg==</BUSINESSCONTENT>
				<HMAC></HMAC>
			</root>
	</post>
  </soap:Body>
</soap:Envelope>
		 * */
		QString header("version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"");
		doc.appendChild(doc.createProcessingInstruction("xml",header));
		QDomElement tagFileInfo_envelope = doc.createElementNS("http://schemas.xmlsoap.org/soap/envelope/", "soapenv:Envelope");
		tagFileInfo_envelope.setAttribute("xmlns:tem", "http://tempuri.org/");
		QDomElement tagFileInfo_header = doc.createElement("soapenv:Header");
		QDomElement tagFileInfo_body = doc.createElement("soapenv:Body");
		QDomElement tagFileInfo_post = doc.createElement("tem:post");
		QDomElement tagFileInfo_data = doc.createElement("tem:data");
		QDomElement tagFileInfo_root = doc.createElement("ROOT");

		//QDomElement相当于加标签，QDomText相当于加内容<QDomElement>QDomText</QDomElement>
		//版本号
		QDomElement tagFileVersion = doc.createElement("VERSION");
		QDomText textFileVersion = doc.createTextNode(version);
		//数据类型
		QDomElement tagFileDataId = doc.createElement("DATAID");
		QDomText textFileDataID = doc.createTextNode(data_id);
		//用户ID
		QDomElement tagFileUserId = doc.createElement("USERID");
		QDomText textFileUserID = doc.createTextNode(user_id);
		//时间
		QDomElement tagFileTime = doc.createElement("TIME");
		QDomText textFileTime = doc.createTextNode(time);
		//数据类型
		QDomElement tagFileType = doc.createElement("TYPE");
		QDomText textFileType = doc.createTextNode(type);
		//传输方式 明文
		QDomElement tagFileSec = doc.createElement("SEC");
		QDomText textFileSec = doc.createTextNode(sec);
		//数据报文
		QDomElement tagFileBusin = doc.createElement("BUSINESSCONTENT");
		QDomText textFileBusin = doc.createTextNode(bus_data);
		//HMAC校验  预留
		QDomElement tagFileHmac = doc.createElement("HMAC");
		QDomText textFileHmac = doc.createTextNode("");

		tagFileVersion.appendChild(textFileVersion);
		tagFileDataId.appendChild(textFileDataID);
		tagFileUserId.appendChild(textFileUserID);
		tagFileTime.appendChild(textFileTime);
		tagFileType.appendChild(textFileType);
		tagFileSec.appendChild(textFileSec);
		tagFileBusin.appendChild(textFileBusin);
		tagFileHmac.appendChild(textFileHmac);

		tagFileInfo_root.appendChild(tagFileVersion);
		tagFileInfo_root.appendChild(tagFileDataId);
		tagFileInfo_root.appendChild(tagFileUserId);
		tagFileInfo_root.appendChild(tagFileTime);
		tagFileInfo_root.appendChild(tagFileType);
		tagFileInfo_root.appendChild(tagFileSec);
		tagFileInfo_root.appendChild(tagFileBusin);
		tagFileInfo_root.appendChild(tagFileHmac);

		tagFileInfo_data.appendChild(tagFileInfo_root);
		tagFileInfo_post.appendChild(tagFileInfo_data);
		tagFileInfo_body.appendChild(tagFileInfo_post);
		//tagFileInfo_header.appendChild(tagFileInfo_body);
		//tagFileInfo_envelope.appendChild(tagFileInfo_header);
		tagFileInfo_envelope.appendChild(tagFileInfo_header);
		tagFileInfo_envelope.appendChild(tagFileInfo_body);

		doc.appendChild(tagFileInfo_envelope);
	}

	//转义字符 < > 都需要转换编码
	QString xmldata = doc.toString();
	//qDebug() << xmldata;

	xmldata = xmldata.replace(QRegExp("\\<ROOT>"),"&lt;ROOT&gt;");
	xmldata = xmldata.replace(QRegExp("\\</ROOT>"),"&lt;/ROOT&gt;");
	xmldata = xmldata.replace(QRegExp("\\<VERSION>"),"&lt;VERSION&gt;");
	xmldata = xmldata.replace(QRegExp("\\</VERSION>"),"&lt;/VERSION&gt;");
	xmldata = xmldata.replace(QRegExp("\\<DATAID>"),"&lt;DATAID&gt;");
	xmldata = xmldata.replace(QRegExp("\\</DATAID>"),"&lt;/DATAID&gt;");
	xmldata = xmldata.replace(QRegExp("\\<USERID>"),"&lt;USERID&gt;");
	xmldata = xmldata.replace(QRegExp("\\</USERID>"),"&lt;/USERID&gt;");
	xmldata = xmldata.replace(QRegExp("\\<TIME>"),"&lt;TIME&gt;");
	xmldata = xmldata.replace(QRegExp("\\</TIME>"),"&lt;/TIME&gt;");
	xmldata = xmldata.replace(QRegExp("\\<TYPE>"),"&lt;TYPE&gt;");
	xmldata = xmldata.replace(QRegExp("\\</TYPE>"),"&lt;/TYPE&gt;");
	xmldata = xmldata.replace(QRegExp("\\<SEC>"),"&lt;SEC&gt;");
	xmldata = xmldata.replace(QRegExp("\\</SEC>"),"&lt;/SEC&gt;");
	xmldata = xmldata.replace(QRegExp("\\<HMAC>"),"&lt;HMAC&gt;");
	xmldata = xmldata.replace(QRegExp("\\</HMAC>"),"&lt;/HMAC&gt;");
	xmldata = xmldata.replace(QRegExp("\\<BUSINESSCONTENT>"),"&lt;BUSINESSCONTENT&gt;");
	xmldata = xmldata.replace(QRegExp("\\</BUSINESSCONTENT>"),"&lt;/BUSINESSCONTENT&gt;");
	//        qDebug() << xmldata;
	//        //base64反解码，查看原始数据
	//        QByteArray byteArray(xmldata.toStdString().c_str(), xmldata.toStdString().length());
	//        QByteArray encdata=QByteArray::fromBase64(byteArray);
	//        qDebug() << encdata;
	//qDebug() << xmldata;
	QString return_xml;
	return_xml = xmldata;
	return return_xml;

}
/***************读取xml格式数据****************/
QString ReadXml(QString read_xml)
{
    QString return_data;
    QDomDocument doc;
    doc .setContent(read_xml);
    QDomElement root = doc.documentElement();
    //读取根元素下的内容,从第一个标签下读取，读到两级，协议只用一级
    QDomNode childNode = root.firstChild();
    while(!childNode.isNull())
    {
        if(childNode.isElement())
        {
            QDomElement element = childNode.toElement();
            if(!element.isNull())
            {
                //qDebug() << element.tagName() << element.text() ;
            }
            return_data = element.text();
        }
        childNode = childNode.nextSibling();
    }
    return return_data;
}

/********************请求数据报文****************
TYPE 请求类型：''01''表示口令修改请求
DATA 当TYPE 为"01"时，DATA表示修改后的口令值（字符串）
**********************************************/
QString XML_Requestdata(QString type,QString data)
{
    QDomDocument doc;
    QDomElement tagFileInforows = doc.createElement("rows");
    QDomElement tagFileInforow = doc.createElement("row");

    //QDomElement相当于加标签，QDomText相当于加内容<QDomElement>QDomText</QDomElement>
    QDomElement tagFileType = doc.createElement("TYPE");
    QDomText textFileType = doc.createTextNode(type);
	QDomElement tagFileData = doc.createElement("DATA");
    QDomText textFileData = doc.createTextNode(data);

    tagFileType.appendChild(textFileType);
    tagFileData.appendChild(textFileData);

    tagFileInforow.appendChild(tagFileType);
    tagFileInforow.appendChild(tagFileData);
    tagFileInforows.appendChild(tagFileInforow);
    doc.appendChild(tagFileInforows);

    QString xmldata = doc.toString();
    qDebug() << xmldata;
    return xmldata;
}
/********************配置数据****************
ID   对象ID，在本次数据传输中唯一
DATE 启用时间
JYQS 加油枪数量
PVZ  PV阀正向压力值
PVF  PV阀负向压力值
SCK  后处理装置开启压力值
SCT  后处理装置关闭压力值  ！！！！与SCK公用一个传入参数
YZQH 安装液阻传感器加油机
如果不存在某项数据则在数据域中填写“NULL
**********************************************/
QString XML_Configurationdata(QString id,QString data,QString jyqs,QString pvz,
                              QString pvf,QString hclk,QString yzqh)
{
    QDomDocument doc;
    QDomElement tagFileInforows = doc.createElement("rows");
    QDomElement tagFileInforow = doc.createElement("row");

    //QDomElement相当于加标签，QDomText相当于加内容<QDomElement>QDomText</QDomElement>
    QDomElement tagFileId = doc.createElement("ID");
    QDomText textFileId = doc.createTextNode(id);
    QDomElement tagFileData = doc.createElement("DATE");
    QDomText textFileData = doc.createTextNode(data);
    QDomElement tagFileJyqs = doc.createElement("JYQS");
    QDomText textFileJyqs = doc.createTextNode(jyqs);
    QDomElement tagFilePvz = doc.createElement("PVZ");
    QDomText textFilePvz = doc.createTextNode(pvz);
    QDomElement tagFilePvf = doc.createElement("PVF");
    QDomText textFilePvf  = doc.createTextNode(pvf);
	QDomElement tagFileHclk = doc.createElement("SCK");
    QDomText textFileHclk = doc.createTextNode(hclk);
	QDomElement tagFileHclt = doc.createElement("SCT");
	QDomText textFileHclt = doc.createTextNode(hclk);
    QDomElement tagFileYzqh = doc.createElement("YZQH");
    QDomText textFileYzqh = doc.createTextNode(yzqh);

    tagFileId.appendChild(textFileId);
    tagFileData.appendChild(textFileData);
    tagFileJyqs.appendChild(textFileJyqs);
    tagFilePvz.appendChild(textFilePvz);
    tagFilePvf.appendChild(textFilePvf);
    tagFileHclk.appendChild(textFileHclk);
	tagFileHclt.appendChild(textFileHclt);
    tagFileYzqh.appendChild(textFileYzqh);

    tagFileInforow.appendChild(tagFileId);
    tagFileInforow.appendChild(tagFileData);
    tagFileInforow.appendChild(tagFileJyqs);
    tagFileInforow.appendChild(tagFilePvz);
    tagFileInforow.appendChild(tagFilePvf);
    tagFileInforow.appendChild(tagFileHclk);
	tagFileInforow.appendChild(tagFileHclt);
    tagFileInforow.appendChild(tagFileYzqh);

    tagFileInforows.appendChild(tagFileInforow);
    doc.appendChild(tagFileInforows);

    QString xmldata = doc.toString();
	//qDebug() << xmldata;
    return xmldata;
}

/********************报警数据****************
ID     对象ID，在本次数据传输中唯一
DATE   监控时间
AL     A/L（0、1、2、N），N指当日无加油
MB     密闭性（0、1、2、N）
YZ     液阻（0、1、2、N）
YGYL   油罐压力（0、1、2、N）
CLZZND 处理装置浓度
PV     PV阀状态
CLZZQD 处理装置启动状态
CLZZTZ 后处理装置关闭状态
XYHQG  卸油回气管状态

PVZT   压力/真空阀状态（0、1、2、N）
PVLJZT 压力/真空阀临界压力状态（0、1、2、N)
HCLZT  后处理装置状态（0、1、2、N)
如果不存在某项数据则在数据域中填写“NULL
**********************************************/
QString XML_Warndata(QString id,QString data,QString al,QString mb,
                              QString yz,QString ygly,QString pvzt,QString pvljzt,QString hclzt)
{
    QDomDocument doc;
    QDomElement tagFileInforows = doc.createElement("rows");
    QDomElement tagFileInforow = doc.createElement("row");

    //QDomElement相当于加标签，QDomText相当于加内容<QDomElement>QDomText</QDomElement>
    QDomElement tagFileId = doc.createElement("ID");
    QDomText textFileId = doc.createTextNode(id);
    QDomElement tagFileData = doc.createElement("DATE");
    QDomText textFileData = doc.createTextNode(data);
    QDomElement tagFileAl = doc.createElement("AL");
    QDomText textFileAl = doc.createTextNode(al);
    QDomElement tagFileMb = doc.createElement("MB");
    QDomText textFileMb = doc.createTextNode(mb);
    QDomElement tagFileYz = doc.createElement("YZ");
    QDomText textFileYz  = doc.createTextNode(yz);
    QDomElement tagFileYglY = doc.createElement("YGLY");
    QDomText textFileYgLy = doc.createTextNode(ygly);
    QDomElement tagFilePvzt = doc.createElement("PVZT");
    QDomText textFilePvzt = doc.createTextNode(pvzt);
    QDomElement tagFilePvljzt = doc.createElement("PVLJZT");
    QDomText textFilePvljzt = doc.createTextNode(pvljzt);
    QDomElement tagFileHclzt = doc.createElement("HCLZT");
    QDomText textFileHclzt = doc.createTextNode(hclzt);

    tagFileId.appendChild(textFileId);
    tagFileData.appendChild(textFileData);
    tagFileAl.appendChild(textFileAl);
    tagFileMb.appendChild(textFileMb);
    tagFileYz.appendChild(textFileYz);
    tagFileYglY.appendChild(textFileYgLy);
    tagFilePvzt.appendChild(textFilePvzt);
    tagFilePvljzt.appendChild(textFilePvljzt);
    tagFileHclzt.appendChild(textFileHclzt);

    tagFileInforow.appendChild(tagFileId);
    tagFileInforow.appendChild(tagFileData);
    tagFileInforow.appendChild(tagFileAl);
    tagFileInforow.appendChild(tagFileMb);
    tagFileInforow.appendChild(tagFileYz);
    tagFileInforow.appendChild(tagFileYglY);
    tagFileInforow.appendChild(tagFilePvzt);
    tagFileInforow.appendChild(tagFilePvljzt);
    tagFileInforow.appendChild(tagFileHclzt);

    tagFileInforows.appendChild(tagFileInforow);
    doc.appendChild(tagFileInforows);

    QString xmldata = doc.toString();
	//qDebug() << xmldata;
    return xmldata;
}
/********************油枪数据****************
ID     对象ID，在本次数据传输中唯一
DATE   监控时间
JYJID  加油机标识
JYQID  加油枪标识
AL     气液比
QLS    油气流速
QLL    油气流量
YLS    燃油流速
YLL    燃油流量
YZ     液阻，单位Pa
如果不存在某项数据则在数据域中填写“NULL
**********************************************/
QString XML_Oilgundata(QString id,QString data,QString jyjid,QString jyqid,QString al,
                              QString qls,QString qll,QString yls,QString yll,QString yz )
{
    QDomDocument doc;
    QDomElement tagFileInforows = doc.createElement("rows");
    QDomElement tagFileInforow = doc.createElement("row");

    //QDomElement相当于加标签，QDomText相当于加内容<QDomElement>QDomText</QDomElement>
    QDomElement tagFileId = doc.createElement("ID");
    QDomText textFileId = doc.createTextNode(id);
    QDomElement tagFileData = doc.createElement("DATE");
    QDomText textFileData = doc.createTextNode(data);
    QDomElement tagFileJyjid = doc.createElement("JYJID");
    QDomText textFileJyjid = doc.createTextNode(jyjid);
    QDomElement tagFileJyqid = doc.createElement("JYQID");
    QDomText textFileJyqid = doc.createTextNode(jyqid);
    QDomElement tagFileAl = doc.createElement("AL");
    QDomText textFileAl = doc.createTextNode(al);
    QDomElement tagFileQls = doc.createElement("QLS");
    QDomText textFilegQls = doc.createTextNode(qls);
    QDomElement tagFileQll = doc.createElement("QLL");
    QDomText textFileQll = doc.createTextNode(qll);
    QDomElement tagFileYls = doc.createElement("YLS");
    QDomText textFileYls = doc.createTextNode(yls);
    QDomElement tagFileYll = doc.createElement("YLL");
    QDomText textFileYll = doc.createTextNode(yll);
    QDomElement tagFileYz = doc.createElement("YZ");
    QDomText textFileYz = doc.createTextNode(yz);

    tagFileId.appendChild(textFileId);
    tagFileData.appendChild(textFileData);
    tagFileJyjid.appendChild(textFileJyjid);
    tagFileJyqid.appendChild(textFileJyqid);
    tagFileAl.appendChild(textFileAl);
    tagFileQls.appendChild(textFilegQls);
    tagFileQll.appendChild(textFileQll);
    tagFileYls.appendChild(textFileYls);
    tagFileYll.appendChild(textFileYll);
    tagFileYz.appendChild(textFileYz);

    tagFileInforow.appendChild(tagFileId);
    tagFileInforow.appendChild(tagFileData);
    tagFileInforow.appendChild(tagFileJyjid);
    tagFileInforow.appendChild(tagFileJyqid);
    tagFileInforow.appendChild(tagFileAl);
    tagFileInforow.appendChild(tagFileQls);
    tagFileInforow.appendChild(tagFileQll);
    tagFileInforow.appendChild(tagFileYls);
    tagFileInforow.appendChild(tagFileYll);
    tagFileInforow.appendChild(tagFileYz);

    tagFileInforows.appendChild(tagFileInforow);
    doc.appendChild(tagFileInforows);

    QString xmldata = doc.toString();
	//qDebug() << xmldata;
    return xmldata;
}
/********************环境数据****************
ID     对象ID，在本次数据传输中唯一
DATE   监控时间
YGYL   油罐压力，单位Pa
YZYL   液阻压力，单位Pa
YQKJ   油气空间，单位L
如果不存在某项数据则在数据域中填写“NULL
数据要采集2~10分钟，该函数要重做
**********************************************/
QString XML_Surroundingsdata(QString id,QString data,QString ygyl,QString yzyl,QString yqkj)
{
    QDomDocument doc;
    QDomElement tagFileInforows = doc.createElement("rows\n");
    QDomElement tagFileInforow = doc.createElement("row");

    //QDomElement相当于加标签，QDomText相当于加内容<QDomElement>QDomText</QDomElement>
    QDomElement tagFileId = doc.createElement("ID");
    QDomText textFileId = doc.createTextNode(id);
    QDomElement tagFileData = doc.createElement("DATE");
    QDomText textFileData = doc.createTextNode(data);
    QDomElement tagFileYgyl = doc.createElement("YGYL");
    QDomText textFileYgyl = doc.createTextNode(ygyl);
    QDomElement tagFileYzyl = doc.createElement("YZYL");
    QDomText textFileYzyl = doc.createTextNode(yzyl);
    QDomElement tagFileYqkj = doc.createElement("YQKJ");
    QDomText textFileYqkj = doc.createTextNode(yqkj);

    tagFileId.appendChild(textFileId);
    tagFileData.appendChild(textFileData);
    tagFileYgyl.appendChild(textFileYgyl);
    tagFileYzyl.appendChild(textFileYzyl);
    tagFileYqkj.appendChild(textFileYqkj);

    tagFileInforow.appendChild(tagFileId);
    tagFileInforow.appendChild(tagFileData);
    tagFileInforow.appendChild(tagFileYgyl);
    tagFileInforow.appendChild(tagFileYzyl);
    tagFileInforow.appendChild(tagFileYqkj);

    tagFileInforows.appendChild(tagFileInforow);
    //doc.appendChild(tagFileInforows);
    doc.appendChild(tagFileInforow);

    QString xmldata = doc.toString();
   // qDebug() << xmldata;
    return xmldata;
}
/********************故障数据****************
ID     对象ID，在本次数据传输中唯一
DATE   故障数据产生时间
TYPE   故障码
**********************************************/
QString XML_Wrongsdata(QString id,QString data,QString type)
{
    QDomDocument doc;
    QDomElement tagFileInforows = doc.createElement("rows");
    QDomElement tagFileInforow = doc.createElement("row");

    //QDomElement相当于加标签，QDomText相当于加内容<QDomElement>QDomText</QDomElement>
    QDomElement tagFileId = doc.createElement("ID");
    QDomText textFileId = doc.createTextNode(id);
    QDomElement tagFileData = doc.createElement("DATE");
    QDomText textFileData = doc.createTextNode(data);
    QDomElement tagFileType = doc.createElement("TYPE");
    QDomText textFileType = doc.createTextNode(type);

    tagFileId.appendChild(textFileId);
    tagFileData.appendChild(textFileData);
    tagFileType.appendChild(textFileType);

    tagFileInforow.appendChild(tagFileId);
    tagFileInforow.appendChild(tagFileData);
    tagFileInforow.appendChild(tagFileType);

    tagFileInforows.appendChild(tagFileInforow);
    doc.appendChild(tagFileInforows);

    QString xmldata = doc.toString();
   // qDebug() << xmldata;
    return xmldata;
}
/********************油枪关停数据****************
ID       对象ID，在本次数据传输中唯一
DATE     启用/关停时间
JYJID    加油机标识
JYQID    加油枪标识
OPERATE  操作类型 0-关停 1-启用
EVENT    关停或启用事件类型关停事件类型：0 自动关停 1 手动关停 启用事件类型：0（预留） 1 手动启用未知事件类型用 N
**********************************************/
QString XML_Closegunsdata(QString id,QString data,QString jyjid,QString jyqid,QString operate,QString event)
{
    QDomDocument doc;
    QDomElement tagFileInforows = doc.createElement("rows");
    QDomElement tagFileInforow = doc.createElement("row");

    //QDomElement相当于加标签，QDomText相当于加内容<QDomElement>QDomText</QDomElement>
    QDomElement tagFileId = doc.createElement("ID");
    QDomText textFileId = doc.createTextNode(id);
    QDomElement tagFileData = doc.createElement("DATE");
    QDomText textFileData = doc.createTextNode(data);
    QDomElement tagFileJyjid = doc.createElement("JYJID");
    QDomText textFileJyjid = doc.createTextNode(jyjid);
    QDomElement tagFileJyqid = doc.createElement("JYQID");
    QDomText textFileJyqid = doc.createTextNode(jyqid);
    QDomElement tagFileOperate = doc.createElement("OPERATE");
    QDomText textFileOperate = doc.createTextNode(operate);
    QDomElement tagFileEvent = doc.createElement("EVENT");
    QDomText textFileEvent = doc.createTextNode(event);

    tagFileId.appendChild(textFileId);
    tagFileData.appendChild(textFileData);
    tagFileJyjid.appendChild(textFileJyjid);
    tagFileJyqid.appendChild(textFileJyqid);
    tagFileOperate.appendChild(textFileOperate);
    tagFileEvent.appendChild(textFileEvent);

    tagFileInforow.appendChild(tagFileId);
    tagFileInforow.appendChild(tagFileData);
    tagFileInforow.appendChild(tagFileJyjid);
    tagFileInforow.appendChild(tagFileJyqid);
    tagFileInforow.appendChild(tagFileEvent);

    tagFileInforows.appendChild(tagFileInforow);
    doc.appendChild(tagFileInforow);

    QString xmldata = doc.toString();
	//qDebug() << xmldata;
    return xmldata;
}
/********************加油枪状态****************
ID       对象ID，在本次数据传输中唯一
DATE     状态采集时间
STATUS   加油枪开关
**********************************************/
QString XML_Stagundata(QString id,QString data,QString status)
{
    QDomDocument doc;
    QDomElement tagFileInforows = doc.createElement("rows");
    QDomElement tagFileInforow = doc.createElement("row");

    //QDomElement相当于加标签，QDomText相当于加内容<QDomElement>QDomText</QDomElement>
    QDomElement tagFileId = doc.createElement("ID");
    QDomText textFileId = doc.createTextNode(id);
    QDomElement tagFileData = doc.createElement("DATE");
    QDomText textFileData = doc.createTextNode(data);
    QDomElement tagFileStatus = doc.createElement("STATUS");
    QDomText textFileStatus = doc.createTextNode(status);

    tagFileId.appendChild(textFileId);
    tagFileData.appendChild(textFileData);
    tagFileStatus.appendChild(textFileStatus);

    tagFileInforow.appendChild(tagFileId);
    tagFileInforow.appendChild(tagFileData);
    tagFileInforow.appendChild(tagFileStatus);

    tagFileInforows.appendChild(tagFileInforow);
    doc.appendChild(tagFileInforows);

    QString xmldata = doc.toString();
   // qDebug() << xmldata;
    return xmldata;
}
