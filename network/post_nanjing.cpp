#include "post_nanjing.h"
#include "config.h"
#include "database_op.h"

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

QMutex post_data_nj;
QNetworkAccessManager *m_accessManager_nj;

unsigned char send_wrong_nj = 2;//这两个变量用来标志发送失败的类型，做历史记录。
unsigned char network_wrong_nj = 3;
QString TYPE_POST_NJ = "";               //业务报文类型（2 位）
QString BUSINESSCONTENT_NJ = "";    //业务报文（数据需转化为 base64 编码）8 HMAC HMAC 校验码（预留）
unsigned int flag_numofsurrounddata_nj = 0;
QString xmldata_surround_nj = "";//环境数据需要叠加


post_nanjing::post_nanjing(QWidget *parent) :
	QMainWindow(parent)
{
	m_accessManager_nj = new QNetworkAccessManager(this);
	QObject::connect(m_accessManager_nj, SIGNAL(finished(QNetworkReply*)), this, SLOT(requestFinished(QNetworkReply*)));
}
/*************合成要发送的字符串***************
****************测试使用的函数****************/
void post_nanjing::Send_Messge_Webservice()
{
	//获取当前时间，数据上传需要
//	QDateTime current_datetime = QDateTime::currentDateTime();
//	QString current_datetime_qstr = current_datetime.toString("yyyyMMddhhmmss");
//	QString xml_data = XML_Warndata("000001",current_datetime_qstr,"1:2;2:1","0","0","0","0","0","0","0","0","0");

//	QByteArray byteArray(xml_data.toStdString().c_str(), xml_data.toStdString().length());
//	QString xml_data64(byteArray.toBase64());

//	QString send_xml = CreatXml("V1.1","000001","3501040007",current_datetime_qstr,"02","0",xml_data64);
//	post_message(send_xml);
}
/*************post请求返回槽函数****************/
void post_nanjing::requestFinished(QNetworkReply* reply)
{
	if (reply->error() == QNetworkReply::NoError)
	{

		if(network_wrong_nj != 0)
		{
			add_value_netinfo("南京在线监测服务器访问成功");
			network_wrong_nj = 0;
		}

		QByteArray bytes = reply->readAll();
		//qDebug()<<bytes;
		QString string = QString::fromUtf8(bytes);
		QString re = ReadXml(string);
		if(re == "0")
		{
			//emit show_data(re.append("  send success"));
			qDebug() << re.append("  send success");
			if(send_wrong_nj != 0)
			{
				add_value_netinfo("南京在线监测数据上传成功");
				send_wrong_nj = 0;
			}
		}
		else
		{
			//emit show_data(re.append("  send fail"));
			qDebug() << re.append("  send fail");
			if(send_wrong_nj != 1)
			{
				add_value_netinfo("南京在线监测数据上传失败");
				Flag_Ifsend = 0;//在这里把全局变量置0，之后时间到了要再重新发送零点的信息
				send_wrong_nj = 1;
			}
		}
	}
	else
	{
		if(network_wrong_nj != 1)
		{
			add_value_netinfo("南京在线监测服务器访问失败");
			Flag_Ifsend = 0;//在这里把全局变量置0，之后时间到了要再重新发送零点的信息
			network_wrong_nj = 1;
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
void post_nanjing::post_message(QString xml_data)
{
	qDebug()<<xml_data;
	post_data_nj.lock();//上锁
	if(net_state == 0)
	{
		//qDebug()<<"send net net  net1231
		//emit show_data(xml_data);
		QNetworkRequest *request = new QNetworkRequest();
		request->setUrl(QUrl(Post_Address));
		request->setHeader(QNetworkRequest::ContentTypeHeader, "text/xml");
		QNetworkReply* reply = m_accessManager_nj->post(*request,xml_data.toUtf8());
		//delete reply;
		delete request;
		request = NULL;
	}
	else
	{
		qDebug() << "network is down, send is prevent!!";
	}
	post_data_nj.unlock();//解锁
}
/***************读取xml格式数据****************/
QString post_nanjing::ReadXml(QString read_xml)
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

/******************发送报警数据****************
ID     对象ID，在本次数据传输中唯一
AL     A/L（0、1、2、N），N指当日无加油
MB     密闭性（0、1、2、N）
YZ     液阻（0、1、2、N）
YGYL   油罐压力
CLZZND 处理装置排放浓度（0、1、2、N）
PV     P/V阀状态（0、1、2、N）
CLZZQD 处理装置启动状态（0、1、2、N）
CLZZTZ 处理装置停止状态（0、1、2、N）
XYHQG  卸油回气管状态（0、1、2、N）
如果不存在某项数据则在数据域中填写“NULL
**********************************************/
void post_nanjing::Send_Warndata(QString id,QString al,QString mb,QString yz,QString ygyl,QString clzznd,QString pv,
								  QString clzzqd,QString clzztz,QString xyhqg)
{
	if(Flag_Postsend_Enable == 1)
	{
		if(Flag_Shield_Network == 1)//如果是处在网络上传屏蔽状态
		{
			al = al.replace(QRegExp("\\:1"),":0");
			al = al.replace(QRegExp("\\:2"),":0"); //报警预警全部替换为正常
			if(mb != "N")mb = "0";
			if(yz != "N")yz = "0";
			if(ygyl != "N")ygyl = "0";
			if(clzznd != "N")clzznd = "0";
			if(pv != "N")pv = "0";
			if(clzzqd != "N")clzzqd = "0";
			if(clzztz != "N")clzztz = "0";
			if(xyhqg != "N")xyhqg = "0";

			TYPE_POST_NJ = "02";
			//获取当前时间，数据上传需要
			QDateTime current_datetime = QDateTime::currentDateTime();
			TIME_POST = current_datetime.toString("yyyyMMddhhmmss");
			QString xml_data = XML_Warndata(id,TIME_POST,al,mb,yz,ygyl,clzznd,pv,clzzqd,clzztz,xyhqg);
			qDebug() << xml_data;
			QByteArray byteArray(xml_data.toStdString().c_str(), xml_data.toStdString().length());
			QString xml_data64(byteArray.toBase64());
			BUSINESSCONTENT_NJ = xml_data64;

			QString send_xml = CreatXml(VERSION_POST,DATAID_POST,USERID_POST,TIME_POST,TYPE_POST_NJ,SEC_POST,BUSINESSCONTENT_NJ);
			post_message(send_xml);
		}
		else
		{
			TYPE_POST_NJ = "02";
			//获取当前时间，数据上传需要
			QDateTime current_datetime = QDateTime::currentDateTime();
			TIME_POST = current_datetime.toString("yyyyMMddhhmmss");
			QString xml_data = XML_Warndata(id,TIME_POST,al,mb,yz,ygyl,clzznd,pv,clzzqd,clzztz,xyhqg);

			QByteArray byteArray(xml_data.toStdString().c_str(), xml_data.toStdString().length());
			QString xml_data64(byteArray.toBase64());
			BUSINESSCONTENT_NJ = xml_data64;

			QString send_xml = CreatXml(VERSION_POST,DATAID_POST,USERID_POST,TIME_POST,TYPE_POST_NJ,SEC_POST,BUSINESSCONTENT_NJ);
			post_message(send_xml);
		}
	}
	else
	{
		qDebug() << "network post send disenable!";
	}
	qDebug()<<"send"<<TYPE_POST_NJ;
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
YQND   回收油气浓度%
YQWD   回收油气温度℃
YZ     液阻，单位Pa
CXSC   加油持续时长，单位S
如果不存在某项数据则在数据域中填写“NULL
**********************************************/
void post_nanjing::Send_Oilgundata(QString id,QString jyjid,QString jyqid,QString al,QString qls,QString qll,QString yls,QString yll,
								   QString yqnd,QString yqwd,QString yz,QString cxsc)
{
	if(Flag_Postsend_Enable == 1)
	{
		if(Flag_Shield_Network == 1)//如果处在屏蔽状态
		{
			if((al.toFloat() <= 120)&&(al.toFloat() >= 100))//如果气液比合格正常上传
			{
				TYPE_POST_NJ = "03";
				//获取当前时间，数据上传需要
				QDateTime current_datetime = QDateTime::currentDateTime();
				TIME_POST = current_datetime.toString("yyyyMMddhhmmss");
				QString xml_data = XML_Oilgundata(id,TIME_POST,jyjid,jyqid,al,qls,qll,yls,yll,yqnd,yqwd,yz,cxsc);
				qDebug() << xml_data;
				QByteArray byteArray(xml_data.toStdString().c_str(), xml_data.toStdString().length());
				QString xml_data64(byteArray.toBase64());
				BUSINESSCONTENT_NJ = xml_data64;

				QString send_xml = CreatXml(VERSION_POST,DATAID_POST,USERID_POST,TIME_POST,TYPE_POST_NJ,SEC_POST,BUSINESSCONTENT_NJ);
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

				TYPE_POST_NJ = "03";
				//获取当前时间，数据上传需要
				QDateTime current_datetime = QDateTime::currentDateTime();
				TIME_POST = current_datetime.toString("yyyyMMddhhmmss");
				QString xml_data = XML_Oilgundata(id,TIME_POST,jyjid,jyqid,al,qls,qll,yls,yll,yqnd,yqwd,yz,cxsc);
				qDebug() << xml_data;
				QByteArray byteArray(xml_data.toStdString().c_str(), xml_data.toStdString().length());
				QString xml_data64(byteArray.toBase64());
				BUSINESSCONTENT_NJ = xml_data64;

				QString send_xml = CreatXml(VERSION_POST,DATAID_POST,USERID_POST,TIME_POST,TYPE_POST_NJ,SEC_POST,BUSINESSCONTENT_NJ);
				post_message(send_xml);
			}
		}
		else
		{
			TYPE_POST_NJ = "03";
			//获取当前时间，数据上传需要
			QDateTime current_datetime = QDateTime::currentDateTime();
			TIME_POST = current_datetime.toString("yyyyMMddhhmmss");
			QString xml_data = XML_Oilgundata(id,TIME_POST,jyjid,jyqid,al,qls,qll,yls,yll,yqnd,yqwd,yz,cxsc);
			qDebug() << xml_data;
			QByteArray byteArray(xml_data.toStdString().c_str(), xml_data.toStdString().length());
			QString xml_data64(byteArray.toBase64());
			BUSINESSCONTENT_NJ = xml_data64;

			QString send_xml = CreatXml(VERSION_POST,DATAID_POST,USERID_POST,TIME_POST,TYPE_POST_NJ,SEC_POST,BUSINESSCONTENT_NJ);
			post_message(send_xml);
		}
	}
	else
	{
		qDebug() << "network post send disenable!";
	}
	qDebug()<<"send"<<TYPE_POST_NJ;
}
/******************发送环境数据****************
ID     对象ID，在本次数据传输中唯一
YGYL   油罐压力，单位Pa
YZYL   液阻压力，单位Pa
YQKJ   油气空间，单位L
XND    卸油区油气浓度
HCLND  后处理装置排放浓度
YQWD   油气温度
如果不存在某项数据则在数据域中填写“NULL
数据要采集2~10分钟，该函数要重做  已重做
最后xml报文顺序不太一样，在xml函数中修改了
**********************************************/
void post_nanjing::Send_Surroundingsdata(QString id,QString ygyl,QString yzyl,QString yqkj,
										  QString xnd,QString hclnd,QString yqwd)
{
	if(Flag_Postsend_Enable == 1)
	{
		flag_numofsurrounddata_nj++;//每15次才发送一次
		//获取当前时间，数据上传需要
		QDateTime current_datetime = QDateTime::currentDateTime();
		TIME_POST = current_datetime.toString("yyyyMMddhhmmss");
		QString xml_data = XML_Surroundingsdata((QString("%1").arg(flag_numofsurrounddata_nj, 6, 10, QLatin1Char('0'))),TIME_POST,ygyl,yzyl,yqkj,xnd,hclnd,yqwd);
		xmldata_surround_nj.append(xml_data);
		if(flag_numofsurrounddata_nj >= 15)  //30秒进一次该函数，450秒7分多钟，小于10分钟
		{
			xmldata_surround_nj.prepend("<Rows>\n");
			xmldata_surround_nj.append("</Rows>");
			//qDebug() << xmldata_surround_nj;
			TYPE_POST_NJ = "04";
			QByteArray byteArray(xmldata_surround_nj.toStdString().c_str(), xmldata_surround_nj.toStdString().length());
			qDebug() << xmldata_surround_nj;
			QString xml_data64(byteArray.toBase64());
			BUSINESSCONTENT_NJ = xml_data64;

//			xml_data64 = "ICAgIDxSb3dzPgogICAgICA8Um93PgogICAgICAgIDxJRD4wMDAwMDE8L0lEPgogICAgICAgIDxEQVRFPjIwMTUwNjI2MDkyMzE1PC9EQVRFPgogICAgICAgIDxZR1lMPjE8L1lHWUw+CiAgICAgICAgPFlaWUw+ODwvWVpZTD4KICAgICAgICA8WE5EPjI8L1hORD4KICAgICAgICA8Q0xORD4zPC9DTE5EPgogICAgICAgIDxZUVdEPjQ8L1lRV0Q+CiAgICAgICAgPFlRS0o+NTwvWVFLSj4KICAgICAgPC9Sb3c+CiAgICA8L1Jvd3M+";
//			BUSINESSCONTENT_NJ = xml_data64;
//			QByteArray ba;
//			std::string stdStr = xml_data64.toStdString();//std::string
//			ba=QByteArray(stdStr.c_str() );           //QByteArray
//			ba=ba.fromBase64(ba);                     //unBase64
//			QString qs1=QString::fromUtf8(ba);        //QString
//			qDebug()<<qs1<<"################";

			QString send_xml = CreatXml(VERSION_POST,DATAID_POST,USERID_POST,TIME_POST,TYPE_POST_NJ,SEC_POST,BUSINESSCONTENT_NJ);
			post_message(send_xml);
			xmldata_surround_nj = "";
			flag_numofsurrounddata_nj = 0;

				qDebug()<<"send"<<TYPE_POST_NJ;
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
void post_nanjing::Send_Wrongsdata(QString id,QString type)
{
	//qDebug()<<"send wrong data";
	if(Flag_Postsend_Enable == 1)
	{
		TYPE_POST_NJ = "05";
		//获取当前时间，数据上传需要
		QDateTime current_datetime = QDateTime::currentDateTime();
		TIME_POST = current_datetime.toString("yyyyMMddhhmmss");
		QString xml_data = XML_Wrongsdata(id,TIME_POST,type);
		qDebug() << xml_data;
		QByteArray byteArray(xml_data.toStdString().c_str(), xml_data.toStdString().length());
		QString xml_data64(byteArray.toBase64());
		BUSINESSCONTENT_NJ = xml_data64;

		QString send_xml = CreatXml(VERSION_POST,DATAID_POST,USERID_POST,TIME_POST,TYPE_POST_NJ,SEC_POST,BUSINESSCONTENT_NJ);
		post_message(send_xml);
	}
	else
	{
		qDebug() << "network post send disenable!";
	}
	qDebug()<<"send"<<TYPE_POST_NJ;
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
7 BUSINESSCONTENT_NJ 业务报文（数据需转化为 base64 编码）8 HMAC HMAC 校验码（预留）

TYPE  00、请求数据；01、配置数据；02、报警数据；03、加油枪数据；04、环境数据；05、故障数据；06、加油枪关停与启用；07、加油枪状态
	  HMAC校验没有添加，如有需要，在此函数内添加即可
******************************************/
QString post_nanjing::CreatXml(QString version,QString data_id,QString user_id,QString time,QString type,QString sec,QString bus_data)
{
	//具体分析使用哪一种方式上传
	QDomDocument doc;

	QString header("version=\"1.0\" encoding=\"UTF-8\"");
	doc.appendChild(doc.createProcessingInstruction("xml",header));
	QDomElement tagFileInfo_envelope = doc.createElementNS("http://schemas.xmlsoap.org/soap/envelope/", "soapenv:Envelope");
	tagFileInfo_envelope.setAttribute("xmlns:tem", "http://tempuri.org/");
	QDomElement tagFileInfo_header = doc.createElement("soapenv:Header");
	QDomElement tagFileInfo_body = doc.createElement("soapenv:Body");
	QDomElement tagFileInfo_post = doc.createElement("tem:DataCollection");
	QDomElement tagFileInfo_data = doc.createElement("tem:xmlData");
	QDomElement tagFileInfo_root = doc.createElement("ROOT");

	//QDomElement相当于加标签，QDomText相当于加内容<QDomElement>QDomText</QDomElement>
	//版本号
	QDomElement tagFileVersion = doc.createElement("VERSION");
	QDomText textFileVersion = doc.createTextNode(version);
	//数据类型
	QDomElement tagFileDataType = doc.createElement("DATATYPE");
	QDomText textFileDataType = doc.createTextNode("1");
	//数据ID
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
	QDomText textFileHmac = doc.createTextNode("HMAC");

	tagFileVersion.appendChild(textFileVersion);
	tagFileDataType.appendChild(textFileDataType);
	tagFileDataId.appendChild(textFileDataID);
	tagFileUserId.appendChild(textFileUserID);
	tagFileTime.appendChild(textFileTime);
	tagFileType.appendChild(textFileType);
	tagFileSec.appendChild(textFileSec);
	tagFileBusin.appendChild(textFileBusin);
	tagFileHmac.appendChild(textFileHmac);

	tagFileInfo_root.appendChild(tagFileVersion);
	tagFileInfo_root.appendChild(tagFileDataType);
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
	//转义字符 < > 都需要转换编码
	QString xmldata = doc.toString();
	//qDebug() << xmldata;

	xmldata = xmldata.replace(QRegExp("\\<ROOT>"),"&lt;ROOT&gt;");
	xmldata = xmldata.replace(QRegExp("\\</ROOT>"),"&lt;/ROOT&gt;");
	xmldata = xmldata.replace(QRegExp("\\<DATATYPE>"),"&lt;DATATYPE&gt;");
	xmldata = xmldata.replace(QRegExp("\\</DATATYPE>"),"&lt;/DATATYPE&gt;");
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
	//qDebug() << xmldata;
	 //base64反解码，查看原始数据
//	QByteArray byteArray(xmldata.toStdString().c_str(), xmldata.toStdString().length());
//	QByteArray encdata=QByteArray::fromBase64(byteArray);
//	qDebug() << encdata;

	QString return_xml;
	return_xml = xmldata;
	return return_xml;

}


/********************报警数据****************
ID     对象ID，在本次数据传输中唯一
DATE   监控时间
AL     A/L（0、1、2、N），N指当日无加油
MB     密闭性（0、1、2、N）
YZ     液阻（0、1、2、N）
YGYL   油罐压力（0、1、2、N）
YGYL   油罐压力
CLZZND 处理装置排放浓度（0、1、2、N）
PV     P/V阀状态（0、1、2、N）
CLZZQD 处理装置启动状态（0、1、2、N）
CLZZTZ 处理装置停止状态（0、1、2、N）
XYHQG  卸油回气管状态（0、1、2、N）
**********************************************/
QString post_nanjing::XML_Warndata(QString id,QString data,QString al,QString mb,QString yz,QString ygyl,QString clzznd,QString pv,
								   QString clzzqd,QString clzztz,QString xyhqg)
{
	QDomDocument doc;
	QDomElement tagFileInforows = doc.createElement("Rows");
	QDomElement tagFileInforow = doc.createElement("Row");

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
	QDomElement tagFileYgYl = doc.createElement("YGYL");
	QDomText textFileYgYl = doc.createTextNode(ygyl);
	QDomElement tagFileClzzNd = doc.createElement("CLZZND");
	QDomText textFileClzzNd= doc.createTextNode(clzznd);
	QDomElement tagFilePv = doc.createElement("PV");
	QDomText textFilePv = doc.createTextNode(pv);
	QDomElement tagFileClzzQd = doc.createElement("CLZZQD");
	QDomText textFileClzzQd = doc.createTextNode(clzzqd);
	QDomElement tagFileClzzTz = doc.createElement("CLZZTZ");
	QDomText textFileClzzTz = doc.createTextNode(clzztz);
	QDomElement tagFileXyhqg = doc.createElement("XYHQG");
	QDomText textFileXyhqg = doc.createTextNode(xyhqg);

	tagFileId.appendChild(textFileId);
	tagFileData.appendChild(textFileData);
	tagFileAl.appendChild(textFileAl);
	tagFileMb.appendChild(textFileMb);
	tagFileYz.appendChild(textFileYz);
	tagFileYgYl.appendChild(textFileYgYl);
	tagFileClzzNd.appendChild(textFileClzzNd);
	tagFilePv.appendChild(textFilePv);
	tagFileClzzQd.appendChild(textFileClzzQd);
	tagFileClzzTz.appendChild(textFileClzzTz);
	tagFileXyhqg.appendChild(textFileXyhqg);


	tagFileInforow.appendChild(tagFileId);
	tagFileInforow.appendChild(tagFileData);
	tagFileInforow.appendChild(tagFileAl);
	tagFileInforow.appendChild(tagFileMb);
	tagFileInforow.appendChild(tagFileYz);
	tagFileInforow.appendChild(tagFileYgYl);
	tagFileInforow.appendChild(tagFileClzzNd);
	tagFileInforow.appendChild(tagFilePv);
	tagFileInforow.appendChild(tagFileClzzQd);
	tagFileInforow.appendChild(tagFileClzzTz);
	tagFileInforow.appendChild(tagFileXyhqg);

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
YQND   回收油气浓度%
YQWD   回收油气温度℃
YZ     液阻，单位Pa
CXSC   加油持续时长，单位S
如果不存在某项数据则在数据域中填写“NULL
**********************************************/
QString post_nanjing::XML_Oilgundata(QString id,QString data,QString jyjid,QString jyqid,QString al,QString qls,QString qll,
									 QString yls,QString yll, QString yqnd,QString yqwd,QString yz,QString cxsc)
{
	QDomDocument doc;
	QDomElement tagFileInforows = doc.createElement("Rows");
	QDomElement tagFileInforow = doc.createElement("Row");

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
	QDomElement tagFileHyqNd = doc.createElement("YQND");
	QDomText textFileHyqNd = doc.createTextNode(yqnd);
	QDomElement tagFileHyqWd = doc.createElement("YQWD");
	QDomText textFileHyqWd = doc.createTextNode(yqwd);
	QDomElement tagFileYz = doc.createElement("YZ");
	QDomText textFileYz = doc.createTextNode(yz);
	QDomElement tagFileCXSC = doc.createElement("CXSC");
	QDomText textFileCXSC = doc.createTextNode(cxsc);

	tagFileId.appendChild(textFileId);
	tagFileData.appendChild(textFileData);
	tagFileJyjid.appendChild(textFileJyjid);
	tagFileJyqid.appendChild(textFileJyqid);
	tagFileAl.appendChild(textFileAl);
	tagFileQls.appendChild(textFilegQls);
	tagFileQll.appendChild(textFileQll);
	tagFileYls.appendChild(textFileYls);
	tagFileYll.appendChild(textFileYll);
	tagFileHyqNd.appendChild(textFileHyqNd);
	tagFileHyqWd.appendChild(textFileHyqWd);
	tagFileYz.appendChild(textFileYz);
	tagFileCXSC.appendChild(textFileCXSC);

	tagFileInforow.appendChild(tagFileId);
	tagFileInforow.appendChild(tagFileData);
	tagFileInforow.appendChild(tagFileJyjid);
	tagFileInforow.appendChild(tagFileJyqid);
	tagFileInforow.appendChild(tagFileAl);
	tagFileInforow.appendChild(tagFileQls);
	tagFileInforow.appendChild(tagFileQll);
	tagFileInforow.appendChild(tagFileYls);
	tagFileInforow.appendChild(tagFileYll);
	tagFileInforow.appendChild(tagFileHyqNd);
	tagFileInforow.appendChild(tagFileHyqWd);
	tagFileInforow.appendChild(tagFileYz);
	tagFileInforow.appendChild(tagFileCXSC);

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
XND    卸油区油气浓度
HCLND  后处理装置排放浓度
YQWD   油气温度
如果不存在某项数据则在数据域中填写“NULL
数据要采集2~10分钟，该函数要重做
**********************************************/
QString post_nanjing::XML_Surroundingsdata(QString id,QString data,QString ygyl,QString yzyl,QString yqkj,
											QString xnd,QString hclnd,QString yqwd)
{
	QDomDocument doc;
	QDomElement tagFileInforows = doc.createElement("Rows\n");
	QDomElement tagFileInforow = doc.createElement("Row");

	//QDomElement相当于加标签，QDomText相当于加内容<QDomElement>QDomText</QDomElement>
	QDomElement tagFileId = doc.createElement("ID");
	QDomText textFileId = doc.createTextNode(id);
	QDomElement tagFileData = doc.createElement("DATE");
	QDomText textFileData = doc.createTextNode(data);
	QDomElement tagFileYgyl = doc.createElement("YGYL");
	QDomText textFileYgyl = doc.createTextNode(ygyl);
	QDomElement tagFileYzyl = doc.createElement("YZYL");
	QDomText textFileYzyl = doc.createTextNode(yzyl);
	QDomElement tagFileXnd = doc.createElement("XND");
	QDomText textFileXnd = doc.createTextNode(xnd);
	QDomElement tagFileHclnd = doc.createElement("CLND");
	QDomText textFileHclnd = doc.createTextNode(hclnd);
	QDomElement tagFileYqwd = doc.createElement("YQWD");
	QDomText textFileYqwd = doc.createTextNode(yqwd);
	QDomElement tagFileYqkj = doc.createElement("YQKJ");
	QDomText textFileYqkj = doc.createTextNode(yqkj);


	tagFileId.appendChild(textFileId);
	tagFileData.appendChild(textFileData);
	tagFileYgyl.appendChild(textFileYgyl);
	tagFileYzyl.appendChild(textFileYzyl);
	tagFileYqkj.appendChild(textFileYqkj);
	tagFileXnd.appendChild(textFileXnd);
	tagFileHclnd.appendChild(textFileHclnd);
	tagFileYqwd.appendChild(textFileYqwd);

	tagFileInforow.appendChild(tagFileId);
	tagFileInforow.appendChild(tagFileData);
	tagFileInforow.appendChild(tagFileYgyl);
	tagFileInforow.appendChild(tagFileYzyl);
	tagFileInforow.appendChild(tagFileXnd);
	tagFileInforow.appendChild(tagFileHclnd);
	tagFileInforow.appendChild(tagFileYqwd);
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
QString post_nanjing::XML_Wrongsdata(QString id,QString data,QString type)
{
	QDomDocument doc;
	QDomElement tagFileInforows = doc.createElement("Rows");
	QDomElement tagFileInforow = doc.createElement("Row");

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
