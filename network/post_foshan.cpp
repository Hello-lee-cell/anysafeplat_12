/********************
 * 2020-06-16
 * mahao
 * 佛山上传协议
 * 本来打算单独做一个线程不断读取上传状态，但后来发现需要用exec(),时间循环才能使post发送成功槽函数被成功调用，
 * 那么就在没测发送之前判断一下是是因为没有登录完成的发送，如果是的话需要先登陆下
 * *******************/
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
#include "post_foshan.h"
#include "json/parser.h"
#include "json/serializer.h"
#include "config.h"
#include "database_op.h"


QString Account_Foshan = "4406060260";                                                            //全局 会变
QString Pwdcode_Foshan = "4406060260";                                                            //全局 会变

QString	Version_FoShan = "1.0";//	通信协议版本
QString	DataId_FoShan = "000001";//	数据序号（6位），自动记录当前最新序号（不同类别的数据分别排序）。
QString	UserId_FoShan = "4406060185";//	区域代码标识（6位）+ 加油站标识（4位）                          //全局 会变
QString	Time_FoShan = "";//	在线监控系统当前时间（年月日时分14位）
QString	Type_FoShan = "";//	业务报文类型（2位）
QString	Sec_FoShan = "1";//	加密标识（1表示业务数据为密文传输，0表示明文）
QString	Businesscontent_FoShan = "";//	业务报文（数据需转化为base64编码）
QString	Hmac_FoShan = "hmac";//	HMAC校验码（预留） 不能为空

QString PostAdd_FoShan = "http://183.237.191.186:15001";   //地址，会不会变不一定，作成可变的           //全局 会变
QString Login_Qs = "/login";
QString Logout_Qs = "/logout";
QString SubmitData_Qs = "/yqhs/SubmitData";

QString Token = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJhY2NvdW50IjoiNDQwNjA2MDE4NSIsInB3ZGNvZGUiOiI0NDA2MDYwMTg1IiwiaWF0IjoxNTkyMzU5MDY1LCJleHAiOjE1OTI0NDU0NjV9.MkUolZ0PvOSvVl4MRPplX_TYnb0Pb9_TeM3s0ddW2wE";

QMutex Post_Lock;

unsigned char FlagLogin_His = 10;//登陆成功一次历史记录
unsigned char FlagSend_His = 10;//发送成功一次历史记录
unsigned char NumSendEnvironment = 0;//环境数据发送频率较低，30S一次，3min发一次，则6次发送一次
QString JsonEnvironmentTem = "";

post_foshan::post_foshan(QObject *parent) :
	QThread(parent)
{


}

void post_foshan::run()
{

	m_accessManager = new QNetworkAccessManager(this);
	QObject::connect(m_accessManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(requestFinished(QNetworkReply*)));


	QTimer *AutoLogin = new QTimer;
	AutoLogin->setInterval(300000);  //5分钟
	connect(AutoLogin,SIGNAL(timeout()),this,SLOT(TestLogin()));
	AutoLogin->start();
	//qDebug()<<"once";
	TestLogin();
	exec();
	while (is_runnable)
	{
		sleep(1);
	}
}

//
void post_foshan::Post_Send_FoShan(unsigned char Flag_DataType, QString data)
{
	Post_Lock.lock();//上锁
	QString url_post = PostAdd_FoShan;
	if(Flag_DataType == 1)
	{
		url_post.append(Login_Qs);
		QNetworkRequest *request = new QNetworkRequest();
		request->setUrl(QUrl( url_post ));
		request->setHeader(QNetworkRequest::ContentTypeHeader, "application/json;charset=UTF-8");
		request->setRawHeader("Accept-Charset","UTF-8");
		QNetworkReply* reply = m_accessManager->post(*request,data.toUtf8());
		//delete reply;
		delete request;
		request = NULL;
	}
	else if(Flag_DataType == 2)
	{
		url_post.append(Logout_Qs);
		QNetworkRequest *request = new QNetworkRequest();
		request->setUrl(QUrl( url_post ));
		request->setHeader(QNetworkRequest::ContentTypeHeader, "application/json;charset=UTF-8");
		request->setRawHeader("AuthenticationToken", Token.toUtf8());
		request->setRawHeader("Accept-Charset","UTF-8");
		QNetworkReply* reply = m_accessManager->post(*request,data.toUtf8());
		//delete reply;
		delete request;
		request = NULL;
	}
	else if(Flag_DataType == 3)
	{
		url_post.append(SubmitData_Qs);
		QNetworkRequest *request = new QNetworkRequest();
		request->setUrl(QUrl( url_post ));
		request->setHeader(QNetworkRequest::ContentTypeHeader, "application/json;charset=UTF-8");
		request->setRawHeader("AuthenticationToken", Token.toUtf8());
		request->setRawHeader("Accept-Charset","UTF-8");
		QNetworkReply* reply = m_accessManager->post(*request,data.toUtf8());
		//delete reply;
		delete request;
		request = NULL;
	}
	else{	}
	Post_Lock.unlock();//解锁
	//qDebug()<<"post send";
}

/*************post请求返回槽函数****************/
void post_foshan::requestFinished(QNetworkReply* reply)
{
	if (reply->error() == QNetworkReply::NoError)
	{
		QByteArray bytes = reply->readAll();
		//qDebug()<<bytes;
		QString string = QString::fromUtf8(bytes);

		QJson::Parser parser;
		bool ok;
		QVariantMap json_receive = parser.parse (string.toAscii(), &ok).toMap();
		if (!ok)
		{
			qFatal("An error occurred during parsing");
		}

		qDebug() << "code:" << json_receive["code"].toString();
		qDebug() << "data:" << json_receive["data"].toString();
		qDebug() << "message:" << json_receive["message"].toString();
		//history_comsta_write((json_receive["message"].toString()));
		if((json_receive["message"].toString() == "登录成功")||
				(json_receive["message"].toString() == "此用户已登录"))
		{
			if(FlagLogin_His != 1)
			{
				//历史记录 登陆成功
				//history_state_write("登陆成功");
				add_value_netinfo("佛山在线监测登陆成功");
				FlagLogin_His = 1;
			}
			Token = json_receive["data"].toString();//获取令牌
		}
		if((json_receive["message"].toString() == "账户密码错误"))
		{
			if(FlagLogin_His != 2)
			{
				//历史记录 账户密码错误
				//history_state_write("账户密码错误");
				add_value_netinfo("佛山在线监测账户或密码错误");
				FlagLogin_His = 2;
			}
			Token = "";//清空令牌
		}
		if((json_receive["message"].toString() == "01"))
		{
			if(FlagSend_His != 1)
			{
				//历史记录 发送成功
				//history_state_write("发送成功");
				add_value_netinfo("佛山在线监测数据发送成功");
				FlagSend_His = 1;
			}
		}
		else if((json_receive["message"].toString() != "登陆成功")&&(json_receive["message"].toString() != "账户密码错误")&&
				(json_receive["message"].toString() != "此用户已登录"))
		{
			if(FlagSend_His != 2)
			{
				//历史记录 发送失败，记录返回值
				//history_state_write((json_receive["message"].toString()).append("发送失败"));
				add_value_netinfo("佛山在线监测数据发送失败");
				FlagSend_His = 2;
				qDebug() << "message:" << json_receive["message"].toString();//返回值
			}
		}
	}
	else
	{
		if(FlagSend_His != 3)
		{
			//历史记录 服务器访问失败
			//history_state_write("断网了，服务器访问失败！");
			add_value_netinfo("佛山在线监测服务器访问失败");
			FlagSend_His = 3;
		}
		Token = "";//清空令牌
		qDebug()<<"handle errors here";
		QVariant statusCodeV = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
		//statusCodeV是HTTPserver的对应码，reply->error()是Qt定义的错误码，能够參考QT的文档
		qDebug( "found error ....code: %d %d\n", statusCodeV.toInt(), (int)reply->error());
		qDebug(qPrintable(reply->errorString()));
	}
	reply->deleteLater();
}
void post_foshan::post_test(unsigned int i)
{
	if(i == 0)
	{
		send_login("4406060260", "4406060260");
		//send_login("4406060260", "4406060260");
		//send_environment("000001","1","1","1","1","1","1","1");
	}
	if(i == 1)
	{
		send_environment("000001","0.0","0.0","0.0","0.0","0.0","0.0","0.0");
		//send_setinfo("000001","2020-06-17 09:59:01","2","0.00","0.00","0","0","1");
		//send_wrong("000001","","031001");
		//send_warninfo("000001","date","1:0;2:0","0","0","0","0","0","0","0","0","0");
		//send_gundata("000001","date","1","1","0.0","0.0","0.0","0.0","0.0","0.0","0.0","0.0","0");
		//send_gunoperate("000001","date","1","1","1","1");
		//send_gunsta("000001","date","1:1;2:1");
		//send_stationinfo("000001","000001","000001","000001","4425","测试","有限公司","116.2563","39.3563","中国","经理","10011012012",
		//				 "4","8","1","1","1","有限公司","1","经理");
		//send_guninfo("000001","000001","1","1");
	}
	if(i == 2)
	{
		send_login("4406060260", "4406060260");
		//send_environment("000001","0.0","0.0","0.0","0.0","0.0","0.0","0.0");
		//send_stationinfo("000001","","","","","","","","","","","","","","","","","","","");
		//send_environment("000001","0","0","0","0","0","0","0");
		//send_wrong("000001","","031001");
		//send_logout("4406060185", "4406060185");
		//send_logout("4406060184", "4406060184");
	}
}

/********testlogin*************
   每隔五分钟登录一次，虽然每次发送之前都登陆过一次，但是还是每隔一段时间登陆一次
 * ****************************/
void post_foshan::TestLogin()
{
	if(Flag_Network_Send_Version == 6)
	{
		send_login(Account_Foshan,Pwdcode_Foshan);
	}
}
/********0、send发送数据打包***************
1	VERSION	通信协议版本
2	DATAID	数据序号（6位），自动记录当前最新序号（不同类别的数据分别排序）。
3	USERID	区域代码标识（6位）+ 加油站标识（4位）
4	TIME	在线监控系统当前时间（年月日时分14位）
5	TYPE	业务报文类型（2位）
6	SEC	加密标识（1表示业务数据为密文传输，0表示明文）
7	BUSINESSCONTENT	业务报文（数据需转化为base64编码）
8	HMAC	HMAC校验码（预留）
 * ****************************/
void post_foshan::Data_Pack(unsigned int date_type,QString BUSINESSCONTENT)
{
	unsigned char post_type = 0;
	Businesscontent_FoShan = BUSINESSCONTENT;
	QDateTime current_datetime = QDateTime::currentDateTime();
	Time_FoShan = current_datetime.toString("yyyy-MM-dd#hh:mm:ss");
	if(date_type == 1){Type_FoShan = "";post_type = 1;}//登录
	else if(date_type == 2){Type_FoShan = "";post_type = 2;}//退出登录
	else if(date_type == 3){Type_FoShan = "01";post_type = 3;}//设置信息
	else if(date_type == 4){Type_FoShan = "02";post_type = 3;}//报警信息
	else if(date_type == 5){Type_FoShan = "03";post_type = 3;}//加油枪信息
	else if(date_type == 6){Type_FoShan = "04";post_type = 3;}//环境信息
	else if(date_type == 7){Type_FoShan = "05";post_type = 3;}//故障信息
	else if(date_type == 8){Type_FoShan = "06";post_type = 3;}//加油枪关停信息
	else if(date_type == 9){Type_FoShan = "07";post_type = 3;}//加油枪状态信息
	else if(date_type == 10){Type_FoShan = "08";post_type = 3;}//加油站信息
	else if(date_type == 11){Type_FoShan = "09";post_type = 3;}//加油机信息
	else {Type_FoShan = "";}
	qDebug()<<BUSINESSCONTENT;
	if((post_type==1)||(post_type==2))
	{
		Post_Send_FoShan(post_type,BUSINESSCONTENT);
	}
	else
	{
		QString json_data = json_data_pack(Version_FoShan,DataId_FoShan,UserId_FoShan,Time_FoShan,Type_FoShan,Sec_FoShan,Businesscontent_FoShan,Hmac_FoShan);
		//json_data = json_data.replace(QRegExp("\\\""),"&quot;");
		//qDebug()<<json_data;
		Post_Send_FoShan(post_type,json_data);
	}
}
/************以下数据发送函数*************/
/********1、send登录***************
 * caaount：登录账户
 * pwd：登录密码
 * ****************************/
void post_foshan::send_login(QString account,QString pwd)
{
//	qDebug()<<"loginiiiiiiiiiiiii";
//	qDebug()<<"account"<<Account_Foshan;
//	qDebug()<<"pwd"<<Pwdcode_Foshan;
//	qDebug()<<"add"<<PostAdd_FoShan;
	QString json_data;
	json_data = json_login(account,pwd);
	Data_Pack(1,json_data);
}

/*******2、send退出登录**************

 * ****************************/
void post_foshan::send_logout(QString account,QString pwd)
{
	QString json_data;
	json_data = json_logout(account,pwd);
	Data_Pack(2,json_data);
}

/*******3、send设置信息**************
 *
元素名称	数据格式	  是否可控	数据描述
DATAID	Varchar2(6)	否	对象ID，在本次数据传输中唯一
DATE	Date	    否	启用时间
JYQS	Varchar2(2)	否	加油枪数量
PVZ	    Number(6,1)	否	PV阀正向压力值
PVF	    Number(6,1)	否	PV阀负向压力值
HCLK	Number(6,1)	否	后处理装置开启压力值（无后处理装置统一填0）
HCLT	Number(6,1)	否	后处理装置停止压力值（无后处理装置统一填0）
YZQH	Varchar1(1)	否	安装液阻传感器加油机编号（无后处理装置统一填0）
 * ****************************/
void post_foshan::send_setinfo(QString DataId,QString Date,QString JYQS,QString PVZ,QString PVF,
					 QString HCLK,QString HCLT,QString YZQH)
{
	send_login(Account_Foshan,Pwdcode_Foshan );//每次发送前登陆一次
	QString json_data;
	QDateTime current_datetime = QDateTime::currentDateTime();
	Date = current_datetime.toString("yyyy-MM-dd#hh:mm:ss");//#替换成空格
	json_data = json_setinfo(DataId,Date,JYQS,PVZ,PVF,HCLK,HCLT,YZQH);
	Data_Pack(3,json_data);
}

/********4、send报警信息**************
 * json报警信息
元素名称	数据格式	是否可控	数据描述
DATAID	Varchar2(6)	否	对象ID，在本次数据传输中唯一
DATE	Date	    否	监控时间
AL	    Varchar2(500)	否	A/L（0、1、2、N），N指当日无加油
MB	    Varchar2(1)	否	密闭性（0、1、2、N）
YZ	    Varchar2(1)	否	液阻（0、1、2、N）
YGYL	Varchar2(1)	否	系统压力（0、1、2、N）
YGLY	Varchar2(1)	否	油罐零压（0、1、2、N）
PVZT	Varchar2(1)	否	压力/真空阀状态（0、1、2、N）
PVLJZT	Varchar2(1)	否	压力/真空阀临界压力状态（0、1、2、N）
HCLZT	Varchar2(1)	否	后处理装置状态（0、1、2、N）
HCLND	Varchar2(1)	否	后处理装置排放浓度（0、1、2、N）
XYHQG	Varchar2(1)	否	卸油回气管状态（0、1、2、N）
 * ****************************/
void post_foshan::send_warninfo(QString DataId,QString Date,QString AL,QString MB,QString YZ,QString YGYL
					  ,QString YGLY,QString PVZT,QString PVLJZT,QString HCLZT,QString HCLND,QString XYHQG)
{
	send_login(Account_Foshan,Pwdcode_Foshan );//每次发送前登陆一次
	if(Flag_Shield_Network == 1)//屏蔽状态
	{
		AL = AL.replace(QRegExp("\\:1"),":0");//报警预警全部替换为正常
		AL = AL.replace(QRegExp("\\:2"),":0");
		MB = MB.replace(QRegExp("\\:1"),":0");
		MB = MB.replace(QRegExp("\\:2"),":0");
		YZ = YZ.replace(QRegExp("\\:1"),":0");
		YZ = YZ.replace(QRegExp("\\:2"),":0");
		YGYL = YGYL.replace(QRegExp("\\:1"),":0");
		YGYL = YGYL.replace(QRegExp("\\:2"),":0");
		YGLY = YGLY.replace(QRegExp("\\:1"),":0");
		YGLY = YGLY.replace(QRegExp("\\:2"),":0");
		PVZT = PVZT.replace(QRegExp("\\:1"),":0");
		PVZT = PVZT.replace(QRegExp("\\:2"),":0");
		PVLJZT = PVLJZT.replace(QRegExp("\\:1"),":0");
		PVLJZT = PVLJZT.replace(QRegExp("\\:2"),":0");
		HCLZT = HCLZT.replace(QRegExp("\\:1"),":0");
		HCLZT = HCLZT.replace(QRegExp("\\:2"),":0");
		HCLND = HCLND.replace(QRegExp("\\:1"),":0");
		HCLND = HCLND.replace(QRegExp("\\:2"),":0");
		XYHQG = XYHQG.replace(QRegExp("\\:1"),":0");
		XYHQG = XYHQG.replace(QRegExp("\\:2"),":0");
	}

	QString json_data;
	QDateTime current_datetime = QDateTime::currentDateTime();
	Date = current_datetime.toString("yyyy-MM-dd#hh:mm:ss");//#替换成空格
	json_data = json_warninfo(DataId,Date,AL,MB,YZ,YGYL,YGLY,PVZT,PVLJZT,HCLZT,HCLND,XYHQG);

	Data_Pack(4,json_data);
}

/********5、send加油枪信息*************
元素名称	数据格式	是否可控	数据描述
DATAID	Varchar2(6)	否	对象ID，在本次数据传输中唯一
DATE	Date	否	监控时间
JYJID	Varchar2(4)	否	加油机标识
JYQID	Varchar2(4)	否	加油枪标识
AL	    NUMBER(3,2)	否	气液比（例如气液比为80% 时，上报是为0.8）
QLS	    NUMBER(6,1)	否	油气流速
QLL	    NUMBER(6,1)	否	油气流量
YLS	    NUMBER(6,1)	否	燃油流速
YLL	    NUMBER(6,1)	否	燃油流量
HYQND	NUMBER(6,1)	否	回收油气浓度
HYQWD	NUMBER(6,1)	否	回收油气温度
YZ	NUMBER(6,1)	否	液阻，单位Pa
STATE	Varchar2(4)	否	当前气液比状态（0正常1报警）
 * ****************************/
void post_foshan::send_gundata(QString DataId,QString Date,QString JYJID,QString JYQID,QString AL,QString QLS,
					 QString QLL,QString YLS,QString YLL,QString HYQND,QString HYQWD,QString YZ,QString STATE)
{
	send_login(Account_Foshan,Pwdcode_Foshan );//每次发送前登陆一次
	QString json_data;
	QDateTime current_datetime = QDateTime::currentDateTime();
	Date = current_datetime.toString("yyyy-MM-dd#hh:mm:ss");//#替换成空格

	if(Flag_Shield_Network == 1)//如果处在屏蔽状态
	{
		if((AL.toFloat() <= NormalAL_High)&&(AL.toFloat() >= NormalAL_Low))//如果气液比合格正常上传
		{
			STATE = "0";//al状态正常
			json_data = json_gundata( DataId, Date, JYJID, JYQID, AL, QLS, QLL, YLS, YLL, HYQND, HYQWD, YZ, STATE);
			Data_Pack(5,json_data);
		}
		else//如果气液比不合格修正后上传
		{
			STATE = "0";//al状态正常
			int al_num = qrand()%(120-100);//用1.0~1.2之间的随机数代替
			float al_xiuzheng = (float)((float)al_num+100)/100;
			//qDebug()<<num+min;
			AL = QString::number(al_xiuzheng,'f',2);
			QLS = QString::number( YLS.toFloat()*al_xiuzheng,'f',2);
			QLL = QString::number( YLL.toFloat()*al_xiuzheng,'f',2); //对气体流速和流量进行修正

			json_data = json_gundata( DataId, Date, JYJID, JYQID, AL, QLS, QLL, YLS, YLL, HYQND, HYQWD, YZ, STATE);
			/*******************************特殊情况，服务器端暂时不支持NULL输入，所已用0代替*******************/
			json_data = json_data.replace(QRegExp("\"NULL\""),"\"0\"");//把NULL替换成0
			Data_Pack(5,json_data);
		}
	}
	else
	{
		if((AL.toFloat() <= NormalAL_High)&&(AL.toFloat() >= NormalAL_Low)){STATE = "0";}
		else {STATE = "1";}
		json_data = json_gundata( DataId, Date, JYJID, JYQID, AL, QLS, QLL, YLS, YLL, HYQND, HYQWD, YZ, STATE);
		/*******************************特殊情况，服务器端暂时不支持NULL输入，所已用0代替*******************/
		json_data = json_data.replace(QRegExp("\"NULL\""),"\"0\"");//把NULL替换成0
		Data_Pack(5,json_data);
	}
}

/********6、send环境信息***********
元素名称	数据格式	是否可控	数据描述
DATAID	Varchar2(6)	否	对象ID，在本次数据传输中唯一
DATE	Date	否	监控时间
YGYL	Number(6,1)	否	系统压力，单位Pa
YZYL	Number(6,1)	否	液阻压力，单位Pa
YQKJ	Number(6,1)	否	油气空间，单位L
XND	    Number(6,1)	否	卸油区油气浓度，单位%/ppm
HCLND	Number(6,1)	否	后处理装置排放浓度，单位g/m³
YQWD	Number(6,1)	否	油气温度，单位℃
 * ****************************/
void post_foshan::send_environment(QString DataId,QString Date,QString YGYL,QString YZYL,QString YQKJ,QString XND,QString HCLND,QString YQWD)
{
	NumSendEnvironment++;

	QString json_data;
	QDateTime current_datetime = QDateTime::currentDateTime();
	Date = current_datetime.toString("yyyy-MM-dd#hh:mm:ss");//#替换成空格
	json_data = json_environment(DataId,Date,YGYL,YZYL,YQKJ,XND,HCLND,YQWD);

	json_data.remove(QRegExp("\\["));//去除[
	json_data.remove(QRegExp("\\]"));//去除[

	JsonEnvironmentTem = JsonEnvironmentTem.append(json_data).append(",");
	if(NumSendEnvironment >= 12)//6分钟
	{
		send_login(Account_Foshan,Pwdcode_Foshan );//每次发送前登陆一次
		JsonEnvironmentTem = JsonEnvironmentTem.left(JsonEnvironmentTem.length() -1);//去掉右端，
		JsonEnvironmentTem.append("]");
		JsonEnvironmentTem.prepend("[");
		/*******************************特殊情况，服务器端暂时不支持NULL输入，所已用0代替*******************/
		JsonEnvironmentTem = JsonEnvironmentTem.replace(QRegExp("\"NULL\""),"\"0\"");//把NULL替换成0
		Data_Pack(6,JsonEnvironmentTem);
		JsonEnvironmentTem = "";
		NumSendEnvironment = 0;
	}
}

/******7、send故障信息************
元素名称	数据格式	是否可空	数据描述
DATAID	Varchar2(6)	否	对象ID，在本次数据传输中唯一
DATE	Date	否	故障数据产生时间
TYPE	Varchar2(6)	否	故障码
 * ****************************/
void post_foshan::send_wrong(QString DataId,QString Date,QString TYPE)
{
	send_login(Account_Foshan,Pwdcode_Foshan );//每次发送前登陆一次
	QString json_data;
	QDateTime current_datetime = QDateTime::currentDateTime();
	Date = current_datetime.toString("yyyy-MM-dd#hh:mm:ss");//#替换成空格
	json_data = json_wrong(DataId,Date,TYPE);
	Data_Pack(7,json_data);
}

/********8、send加油枪关停启用信息************
元素名称	数据格式	是否可控	数据描述
DATAID	Varchar2(6)	否	对象ID，在本次数据传输中唯一
DATE	Date	否	启用/关停时间
JYJID	Varchar2(4)	否	加油机标识
JYQID	Varchar2(4)	否	加油枪标识
OPERATE	Varchar2(1)	否	操作类型 0-关停 1-启用
EVENT	Varchar2(1)	否	关停或启用事件类型
关停事件类型：0 自动关停
1 手动关停
启用事件类型：0（预留）
1 手动启用
未知事件类型用 N 表示
 * ****************************/
void post_foshan::send_gunoperate(QString DataId,QString Date,QString JYJID,QString JYQID,QString OPERATE,QString EVENT)
{
	send_login(Account_Foshan,Pwdcode_Foshan );//每次发送前登陆一次
	QString json_data;
	QDateTime current_datetime = QDateTime::currentDateTime();
	Date = current_datetime.toString("yyyy-MM-dd#hh:mm:ss");//#替换成空格
	json_data = json_gunoperate(DataId,Date,JYJID,JYQID,OPERATE,EVENT);
	Data_Pack(8,json_data);
}

/******9、send加油枪状态信息*********
DATAID	Varchar2(6)	否	对象ID，在本次数据传输中唯一
DATE	Date	否	状态采集时间
STATUS	Varchar2(256)	否	加油枪开关状态：0-关停，1-正常
 * ****************************/
void post_foshan::send_gunsta(QString DataId,QString Date,QString STATUS)
{
	send_login(Account_Foshan,Pwdcode_Foshan );//每次发送前登陆一次
	QString json_data;
	QDateTime current_datetime = QDateTime::currentDateTime();
	Date = current_datetime.toString("yyyy-MM-dd#hh:mm:ss");//#替换成空格
	json_data = json_gunsta(DataId,Date,STATUS);
	Data_Pack(9,json_data);
}

/*******10、send加油站信息**********
字段	数据格式	是否可空	数据描述
DATAID	    Varchar2(6)	否	对象ID，在本次数据传输中唯一
DATE	    Date	否	状态采集时间
CityCode	varchar	否	城市编码
AreaCode	varchar	否	行政区划编码
TownCode	varchar	是	镇街编码
StationName	varchar	否	加油站名称
Company	    varchar	否	加油站所属的石油公司
Lon	        varchar	否	经度
Lat	        varchar	否	纬度
Address	    varchar	否	加油站地址
Contact	    varchar	否	加油站联系人
Phone	    varchar	否	手机号码，预警或报警短信提示
JYJNum	    int	否	加油机数量
JYQNum	    int	否	加油枪数量
Scale	    varchar	否	年经营规模 : 8千吨以上、5千-8千吨、3千-5千吨、3千吨以下详见表A4.1.1
OwnerType	varchar	否	所有制情况 : 中石油、中石化、中海油、中化、民营、外资企业、其他国有.详见下表4.1.2
HasSystem	int	否	是否安装油气回收系统:0未安装，1已安装
Manufacturer	varchar	是	在线系统供应商
IsAcceptance	int	是	建设情况：0未验收，1已验收
OperateStaff	varchar	是	在线系统运维负责人
 * ****************************/
void post_foshan::send_stationinfo(QString DataId,QString Date,QString CityCode,QString AreaCode,QString TownCode,QString StationName
						 ,QString Company,QString Lon,QString Lat,QString Address,QString Contact,QString Phone,QString JYJNum
						 ,QString JYQNum,QString Scale,QString OwnerType,QString HasSystem,QString Manufacturer,QString IsAcceptance
						 ,QString OperateStaff)
{
	send_login(Account_Foshan,Pwdcode_Foshan );//每次发送前登陆一次
	QString json_data;
	QDateTime current_datetime = QDateTime::currentDateTime();
	Date = current_datetime.toString("yyyy-MM-dd#hh:mm:ss");//#替换成空格
	json_data = json_stationinfo(DataId,Date,CityCode,AreaCode,TownCode,StationName,Company,Lon,Lat,Address,Contact,Phone,JYJNum
								 ,JYQNum,Scale,OwnerType,HasSystem,Manufacturer,IsAcceptance,OperateStaff);
	Data_Pack(10,json_data);
}

/******11、send加油机信息**************
DATAID	Varchar2(6)	否	对象ID，在本次数据传输中唯一
DATE	Date	否	状态采集时间
JYJID	varchar	否	加油机标识
JYQID	varchar	否	加油枪标识
 * ****************************/
void post_foshan::send_guninfo(QString DataId,QString Date,QString JYJID,QString JYQID)
{
	send_login(Account_Foshan,Pwdcode_Foshan );//每次发送前登陆一次
	QString json_data;
	QDateTime current_datetime = QDateTime::currentDateTime();
	Date = current_datetime.toString("yyyy-MM-dd#hh:mm:ss");//#替换成空格
	json_data = json_guninfo(DataId,Date,JYJID,JYQID);
	Data_Pack(11,json_data);
}

/******12、send加油站所有信息**************
同时发送10和11两条信息
 * ****************************/
void post_foshan::send_station_message()
{
	QString CityCode,AreaCode,TownCode,StationName,Company,Lon,Lat,Address,Contact,Phone,JYJNum,JYQNum,
			Scale,OwnerType,HasSystem,Manufacturer,IsAcceptance,OperateStaff, JYJID,JYQID;

	QSettings *configIniRead = new QSettings("/opt/StationMessage.ini", QSettings::IniFormat);
	//将读取到的ini文件保存在QString中，先取值，然后通过toString()函数转换成QString类型
	CityCode = configIniRead->value("StationMessage/CityCode").toString();
	AreaCode = configIniRead->value("StationMessage/AreaCode").toString();
	TownCode = configIniRead->value("StationMessage/TownCode").toString();
	StationName = configIniRead->value("StationMessage/StationName").toString();
	Company = configIniRead->value("StationMessage/Company").toString();
	Lon = configIniRead->value("StationMessage/Lon").toString();
	Lat = configIniRead->value("StationMessage/Lat").toString();
	Address = configIniRead->value("StationMessage/Address").toString();
	Contact = configIniRead->value("StationMessage/Contact").toString();
	Phone = configIniRead->value("StationMessage/Phone").toString();
	JYJNum = configIniRead->value("StationMessage/JYJNum").toString();
	JYQNum = configIniRead->value("StationMessage/JYQNum").toString();
	Scale = configIniRead->value("StationMessage/Scale").toString();
	OwnerType = configIniRead->value("StationMessage/OwnerType").toString();
	HasSystem = configIniRead->value("StationMessage/HasSystem").toString();
	Manufacturer = configIniRead->value("StationMessage/Manufacturer").toString();
	IsAcceptance = configIniRead->value("StationMessage/IsAcceptance").toString();
	OperateStaff = configIniRead->value("StationMessage/OperateStaff").toString();

	JYJID = configIniRead->value("JYJMessage/JYJID").toString();
	JYQID = configIniRead->value("JYJMessage/JYQID").toString();
	//读入入完成后删除指针
	delete configIniRead;

	send_stationinfo(DATAID_POST,"date",CityCode,AreaCode,TownCode,StationName,Company,Lon,Lat,Address,Contact,Phone,JYJNum,JYQNum,
					 Scale,OwnerType,HasSystem,Manufacturer,IsAcceptance,OperateStaff);
	send_guninfo(DATAID_POST,"date",JYJID,JYQID);
}

/*********以下json数据转换函数****************/
/********0、json发送包***************
1	VERSION	通信协议版本
2	DATAID	数据序号（6位），自动记录当前最新序号（不同类别的数据分别排序）。
3	USERID	区域代码标识（6位）+ 加油站标识（4位）
4	TIME	在线监控系统当前时间（年月日时分14位）
5	TYPE	业务报文类型（2位）
6	SEC	加密标识（1表示业务数据为密文传输，0表示明文）
7	BUSINESSCONTENT	业务报文（数据需转化为base64编码）
8	HMAC	HMAC校验码（预留）
 * ****************************/
QString post_foshan::json_data_pack(QString VERSION,QString DATAID,QString USERID,QString TIME,QString TYPE,
									QString SEC, QString BUSINESSCONTENT,QString HMAC)
{
	QByteArray json_zh;
	bool ok;
	QVariantList Json_data;
	QVariantMap JsonRead;

	QByteArray byteArray(BUSINESSCONTENT.toStdString().c_str(), BUSINESSCONTENT.toStdString().length());
	QString json_data64(byteArray.toBase64());

	JsonRead.insert("VERSION",VERSION);
	JsonRead.insert("DATAID",DATAID);
	JsonRead.insert("USERID",USERID);
	JsonRead.insert("TIME",TIME);
	JsonRead.insert("TYPE",TYPE);
	JsonRead.insert("SEC",SEC);
	JsonRead.insert("BUSINESSCONTENT",json_data64);
	JsonRead.insert("HMAC",HMAC);


	Json_data << JsonRead;
	//qDebug()<<Json_data;

	QJson::Serializer serializer;
	QByteArray json = serializer.serialize(Json_data, &ok);
	json_zh = json;
	QString test = json;
	test = test.left(test.length() -1);//去掉右端】
	test = test.right(test.length() -1);//去掉左端【
	test.remove(QRegExp("\\s"));//去除空格
	test = test.replace(QRegExp("\\#")," ");//添加空格
	if (ok)
	{
		return test;
	}
	else
	{
		qCritical() << "log json data went wrong:" << serializer.errorMessage();
		return "-1";
	}
}
/********1、json登录***************
 *
 * caaount：登录账户
 * pwd：登录密码
 * ****************************/
QString post_foshan::json_login(QString account,QString pwd)
{
	QByteArray json_zh;
	bool ok;
	QVariantList Json_data;
	QVariantMap JsonRead;
	JsonRead.insert("account",account);
	JsonRead.insert("pwdcode",pwd);


	Json_data << JsonRead;

	QJson::Serializer serializer;
	QByteArray json = serializer.serialize(Json_data, &ok);
	json_zh = json;
	QString test = json;
	test = test.left(test.length() -1);//去掉右端】
	test = test.right(test.length() -1);//去掉左端【
	test.remove(QRegExp("\\s"));//去除空格
	test = test.replace(QRegExp("\\#")," ");//添加空格
	if (ok)
	{
		return test;
	}
	else
	{
		qCritical() << "log json data went wrong:" << serializer.errorMessage();
		return "-1";
	}
}

/*******2、json退出登录**************

 * ****************************/
QString post_foshan::json_logout(QString account,QString pwd)
{
	QByteArray json_zh;
	bool ok;
	QVariantList Json_data;
	QVariantMap JsonRead;
	JsonRead.insert("account",account);
	JsonRead.insert("pwdcode",pwd);


	Json_data << JsonRead;

	QJson::Serializer serializer;
	QByteArray json = serializer.serialize(Json_data, &ok);
	json_zh = json;
	QString test = json;
	test = test.left(test.length() -1);//去掉右端】
	test = test.right(test.length() -1);//去掉左端【
	test.remove(QRegExp("\\s"));//去除空格
	test = test.replace(QRegExp("\\#")," ");//添加空格
	if (ok)
	{
		return test;
	}
	else
	{
		qCritical() << "log json data went wrong:" << serializer.errorMessage();
		return "-1";
	}
}

/*******3、json设置信息**************
 *
元素名称	数据格式	  是否可控	数据描述
DATAID	Varchar2(6)	否	对象ID，在本次数据传输中唯一
DATE	Date	    否	启用时间
JYQS	Varchar2(2)	否	加油枪数量
PVZ	    Number(6,1)	否	PV阀正向压力值
PVF	    Number(6,1)	否	PV阀负向压力值
HCLK	Number(6,1)	否	后处理装置开启压力值（无后处理装置统一填0）
HCLT	Number(6,1)	否	后处理装置停止压力值（无后处理装置统一填0）
YZQH	Varchar1(1)	否	安装液阻传感器加油机编号（无后处理装置统一填0）
 * ****************************/
QString post_foshan::json_setinfo(QString DataId,QString Date,QString JYQS,QString PVZ,QString PVF,
					 QString HCLK,QString HCLT,QString YZQH)
{
	QByteArray json_zh;
	bool ok;
	QVariantList Json_data;
	QVariantMap JsonRead;
	JsonRead.insert("DATAID",DataId);
	JsonRead.insert("DATE",Date);
	JsonRead.insert("JYQS",JYQS);
	JsonRead.insert("PVZ",PVZ);
	JsonRead.insert("PVF",PVF);
	JsonRead.insert("HCLK",HCLK);
	JsonRead.insert("HCLT",HCLT);
	JsonRead.insert("YZQH",YZQH);


	Json_data << JsonRead;

	QJson::Serializer serializer;
	QByteArray json = serializer.serialize(Json_data, &ok);
	json_zh = json;
	QString test = json;
	//test = test.left(test.length() -1);//去掉右端】
	//test = test.right(test.length() -1);//去掉左端【
	test.remove(QRegExp("\\s"));//去除空格
	test = test.replace(QRegExp("\\#")," ");//添加空格
	if (ok)
	{
		return test;
	}
	else
	{
		qCritical() << "setinfo json data went wrong:" << serializer.errorMessage();
		return "-1";
	}
}

/********4、json报警信息**************
 * json报警信息
元素名称	数据格式	是否可控	数据描述
DATAID	Varchar2(6)	否	对象ID，在本次数据传输中唯一
DATE	Date	    否	监控时间
AL	    Varchar2(500)	否	A/L（0、1、2、N），N指当日无加油
MB	    Varchar2(1)	否	密闭性（0、1、2、N）
YZ	    Varchar2(1)	否	液阻（0、1、2、N）
YGYL	Varchar2(1)	否	系统压力（0、1、2、N）
YGLY	Varchar2(1)	否	油罐零压（0、1、2、N）
PVZT	Varchar2(1)	否	压力/真空阀状态（0、1、2、N）
PVLJZT	Varchar2(1)	否	压力/真空阀临界压力状态（0、1、2、N）
HCLZT	Varchar2(1)	否	后处理装置状态（0、1、2、N）
HCLND	Varchar2(1)	否	后处理装置排放浓度（0、1、2、N）
XYHQG	Varchar2(1)	否	卸油回气管状态（0、1、2、N）
 * ****************************/
QString post_foshan::json_warninfo(QString DataId,QString Date,QString AL,QString MB,QString YZ,QString YGYL
					  ,QString YGLY,QString PVZT,QString PVLJZT,QString HCLZT,QString HCLND,QString XYHQG)
{
	QByteArray json_zh;
	bool ok;
	QVariantList Json_data;
	QVariantMap JsonRead;
	JsonRead.insert("DATAID",DataId);
	JsonRead.insert("DATE",Date);
	JsonRead.insert("AL",AL);
	JsonRead.insert("MB",MB);
	JsonRead.insert("YZ",YZ);
	JsonRead.insert("YGYL",YGYL);
	JsonRead.insert("YGLY",YGLY);
	JsonRead.insert("PVZT",PVZT);
	JsonRead.insert("PVLJZT",PVLJZT);
	JsonRead.insert("HCLZT",HCLZT);
	JsonRead.insert("HCLND",HCLND);
	JsonRead.insert("XYHQG",XYHQG);

	Json_data << JsonRead;

	QJson::Serializer serializer;
	QByteArray json = serializer.serialize(Json_data, &ok);
	json_zh = json;
	QString test = json;
	//test = test.left(test.length() -1);//去掉右端】
	//test = test.right(test.length() -1);//去掉左端【
	test.remove(QRegExp("\\s"));//去除空格
	test = test.replace(QRegExp("\\#")," ");//添加空格
	if (ok)
	{
		return test;
	}
	else
	{
		qCritical() << "setinfo json data went wrong:" << serializer.errorMessage();
		return "-1";
	}
}

/********5、json加油枪信息*************
元素名称	数据格式	是否可控	数据描述
DATAID	Varchar2(6)	否	对象ID，在本次数据传输中唯一
DATE	Date	否	监控时间
JYJID	Varchar2(4)	否	加油机标识
JYQID	Varchar2(4)	否	加油枪标识
AL	    NUMBER(3,2)	否	气液比（例如气液比为80% 时，上报是为0.8）
QLS	    NUMBER(6,1)	否	油气流速
QLL	    NUMBER(6,1)	否	油气流量
YLS	    NUMBER(6,1)	否	燃油流速
YLL	    NUMBER(6,1)	否	燃油流量
HYQND	NUMBER(6,1)	否	回收油气浓度
HYQWD	NUMBER(6,1)	否	回收油气温度
YZ	NUMBER(6,1)	否	液阻，单位Pa
STATE	Varchar2(4)	否	当前气液比状态（0正常1报警）
 * ****************************/
QString post_foshan::json_gundata(QString DataId,QString Date,QString JYJID,QString JYQID,QString AL,QString QLS,
					 QString QLL,QString YLS,QString YLL,QString HYQND,QString HYQWD,QString YZ,QString STATE)
{
	QByteArray json_zh;
	bool ok;
	QVariantList Json_data;
	QVariantMap JsonRead;
	JsonRead.insert("DATAID",DataId);
	JsonRead.insert("DATE",Date);
	JsonRead.insert("JYJID",JYJID);
	JsonRead.insert("JYQID",JYQID);
	JsonRead.insert("AL",AL);
	JsonRead.insert("QLS",QLS);
	JsonRead.insert("QLL",QLL);
	JsonRead.insert("YLS",YLS);
	JsonRead.insert("YLL",YLL);
	JsonRead.insert("HYQND",HYQND);
	JsonRead.insert("HYQWD",HYQWD);
	JsonRead.insert("YZ",YZ);
	JsonRead.insert("STATE",STATE);

	Json_data << JsonRead;

	QJson::Serializer serializer;
	QByteArray json = serializer.serialize(Json_data, &ok);
	json_zh = json;
	QString test = json;
	//test = test.left(test.length() -1);//去掉右端】
	//test = test.right(test.length() -1);//去掉左端【
	test.remove(QRegExp("\\s"));//去除空格
	test = test.replace(QRegExp("\\#")," ");//添加空格
	if (ok)
	{
		return test;
	}
	else
	{
		qCritical() << "setinfo json data went wrong:" << serializer.errorMessage();
		return "-1";
	}
}

/********6、json环境信息***********
元素名称	数据格式	是否可控	数据描述
DATAID	Varchar2(6)	否	对象ID，在本次数据传输中唯一
DATE	Date	否	监控时间
YGYL	Number(6,1)	否	系统压力，单位Pa
YZYL	Number(6,1)	否	液阻压力，单位Pa
YQKJ	Number(6,1)	否	油气空间，单位L
XND	    Number(6,1)	否	卸油区油气浓度，单位%/ppm
HCLND	Number(6,1)	否	后处理装置排放浓度，单位g/m³
YQWD	Number(6,1)	否	油气温度，单位℃
 * ****************************/
QString post_foshan::json_environment(QString DataId,QString Date,QString YGYL,QString YZYL,QString YQKJ,QString XND,QString HCLND,QString YQWD)
{
	QByteArray json_zh;
	bool ok;
	QVariantList Json_data;
	QVariantMap JsonRead;
	JsonRead.insert("DATAID",DataId);
	JsonRead.insert("DATE",Date);
	JsonRead.insert("YGYL",YGYL);
	JsonRead.insert("YZYL",YZYL);
	JsonRead.insert("YQKJ",YQKJ);
	JsonRead.insert("XND",XND);
	JsonRead.insert("HCLND",HCLND);
	JsonRead.insert("YQWD",YQWD);

	Json_data << JsonRead;

	QJson::Serializer serializer;
	QByteArray json = serializer.serialize(Json_data, &ok);
	json_zh = json;
	QString test = json;
	//test = test.left(test.length() -1);//去掉右端】
	//test = test.right(test.length() -1);//去掉左端【
	test.remove(QRegExp("\\s"));//去除空格
	test = test.replace(QRegExp("\\#")," ");//添加空格
	if (ok)
	{
		return test;
	}
	else
	{
		qCritical() << "setinfo json data went wrong:" << serializer.errorMessage();
		return "-1";
	}
}

/******7、json故障信息************
元素名称	数据格式	是否可空	数据描述
DATAID	Varchar2(6)	否	对象ID，在本次数据传输中唯一
DATE	Date	否	故障数据产生时间
TYPE	Varchar2(6)	否	故障码
 * ****************************/
QString post_foshan::json_wrong(QString DataId,QString Date,QString TYPE)
{
	QByteArray json_zh;
	bool ok;
	QVariantList Json_data;
	QVariantMap JsonRead;
	JsonRead.insert("DATAID",DataId);
	JsonRead.insert("DATE",Date);
	JsonRead.insert("TYPE",TYPE);

	Json_data << JsonRead;

	QJson::Serializer serializer;
	QByteArray json = serializer.serialize(Json_data, &ok);
	json_zh = json;
	QString test = json;
	//test = test.left(test.length() -1);//去掉右端】
	//test = test.right(test.length() -1);//去掉左端【
	test.remove(QRegExp("\\s"));//去除空格
	test = test.replace(QRegExp("\\#")," ");//添加空格
	if (ok)
	{
		return test;
	}
	else
	{
		qCritical() << "setinfo json data went wrong:" << serializer.errorMessage();
		return "-1";
	}
}

/********8、json加油枪关停启用信息************
元素名称	数据格式	是否可控	数据描述
DATAID	Varchar2(6)	否	对象ID，在本次数据传输中唯一
DATE	Date	否	启用/关停时间
JYJID	Varchar2(4)	否	加油机标识
JYQID	Varchar2(4)	否	加油枪标识
OPERATE	Varchar2(1)	否	操作类型 0-关停 1-启用
EVENT	Varchar2(1)	否	关停或启用事件类型
关停事件类型：0 自动关停
1 手动关停
启用事件类型：0（预留）
1 手动启用
未知事件类型用 N 表示
 * ****************************/
QString post_foshan::json_gunoperate(QString DataId,QString Date,QString JYJID,QString JYQID,QString OPERATE,QString EVENT)
{
	QByteArray json_zh;
	bool ok;
	QVariantList Json_data;
	QVariantMap JsonRead;
	JsonRead.insert("DATAID",DataId);
	JsonRead.insert("DATE",Date);
	JsonRead.insert("JYJID",JYJID);
	JsonRead.insert("JYQID",JYQID);
	JsonRead.insert("OPERATE",OPERATE);
	JsonRead.insert("EVENT",EVENT);

	Json_data << JsonRead;

	QJson::Serializer serializer;
	QByteArray json = serializer.serialize(Json_data, &ok);
	json_zh = json;
	QString test = json;
	//test = test.left(test.length() -1);//去掉右端】
	//test = test.right(test.length() -1);//去掉左端【
	test.remove(QRegExp("\\s"));//去除空格
	test = test.replace(QRegExp("\\#")," ");//添加空格
	if (ok)
	{
		return test;
	}
	else
	{
		qCritical() << "setinfo json data went wrong:" << serializer.errorMessage();
		return "-1";
	}
}

/******9、json加油枪状态信息*********
DATAID	Varchar2(6)	否	对象ID，在本次数据传输中唯一
DATE	Date	否	状态采集时间
STATUS	Varchar2(256)	否	加油枪开关状态：0-关停，1-正常
 * ****************************/
QString post_foshan::json_gunsta(QString DataId,QString Date,QString STATUS)
{
	QByteArray json_zh;
	bool ok;
	QVariantList Json_data;
	QVariantMap JsonRead;
	JsonRead.insert("DATAID",DataId);
	JsonRead.insert("DATE",Date);
	JsonRead.insert("STATUS",STATUS);

	Json_data << JsonRead;

	QJson::Serializer serializer;
	QByteArray json = serializer.serialize(Json_data, &ok);
	json_zh = json;
	QString test = json;
	//test = test.left(test.length() -1);//去掉右端】
	//test = test.right(test.length() -1);//去掉左端【
	test.remove(QRegExp("\\s"));//去除空格
	test = test.replace(QRegExp("\\#")," ");//添加空格
	if (ok)
	{
		return test;
	}
	else
	{
		qCritical() << "setinfo json data went wrong:" << serializer.errorMessage();
		return "-1";
	}
}

/*******10、json加油站信息**********
字段	数据格式	是否可空	数据描述
DATAID	    Varchar2(6)	否	对象ID，在本次数据传输中唯一
DATE	    Date	否	状态采集时间
CityCode	varchar	否	城市编码
AreaCode	varchar	否	行政区划编码
TownCode	varchar	是	镇街编码
StationName	varchar	否	加油站名称
Company	    varchar	否	加油站所属的石油公司
Lon	        varchar	否	经度
Lat	        varchar	否	纬度
Address	    varchar	否	加油站地址
Contact	    varchar	否	加油站联系人
Phone	    varchar	否	手机号码，预警或报警短信提示
JYJNum	    int	否	加油机数量
JYQNum	    int	否	加油枪数量
Scale	    varchar	否	年经营规模 : 8千吨以上、5千-8千吨、3千-5千吨、3千吨以下详见表A4.1.1
OwnerType	varchar	否	所有制情况 : 中石油、中石化、中海油、中化、民营、外资企业、其他国有.详见下表4.1.2
HasSystem	int	否	是否安装油气回收系统:0未安装，1已安装
Manufacturer	varchar	是	在线系统供应商
IsAcceptance	int	是	建设情况：0未验收，1已验收
OperateStaff	varchar	是	在线系统运维负责人
 * ****************************/
QString post_foshan::json_stationinfo(QString DataId,QString Date,QString CityCode,QString AreaCode,QString TownCode,QString StationName
						 ,QString Company,QString Lon,QString Lat,QString Address,QString Contact,QString Phone,QString JYJNum
						 ,QString JYQNum,QString Scale,QString OwnerType,QString HasSystem,QString Manufacturer,QString IsAcceptance
						 ,QString OperateStaff)
{
	QByteArray json_zh;
	bool ok;
	QVariantList Json_data;
	QVariantMap JsonRead;
	JsonRead.insert("DATAID",DataId);
	JsonRead.insert("DATE",Date);
	JsonRead.insert("CityCode",CityCode);
	JsonRead.insert("AreaCode",AreaCode);
	JsonRead.insert("TownCode",TownCode);
	JsonRead.insert("StationName",StationName);
	JsonRead.insert("Company",Company);
	JsonRead.insert("Lon",Lon);
	JsonRead.insert("Lat",Lat);
	JsonRead.insert("Address",Address);
	JsonRead.insert("Contact",Contact);
	JsonRead.insert("Phone",Phone);
	JsonRead.insert("JYJNum",JYJNum);
	JsonRead.insert("JYQNum",JYQNum);
	JsonRead.insert("Scale",Scale);
	JsonRead.insert("OwnerType",OwnerType);
	JsonRead.insert("HasSystem",HasSystem);
	JsonRead.insert("Manufacturer",Manufacturer);
	JsonRead.insert("IsAcceptance",IsAcceptance);
	JsonRead.insert("OperateStaff",OperateStaff);

	Json_data << JsonRead;

	QJson::Serializer serializer;
	QByteArray json = serializer.serialize(Json_data, &ok);
	json_zh = json;
	QString test = json;
	//test = test.left(test.length() -1);//去掉右端】
	//test = test.right(test.length() -1);//去掉左端【
	test.remove(QRegExp("\\s"));//去除空格
	test = test.replace(QRegExp("\\#")," ");//添加空格
	if (ok)
	{
		return test;
	}
	else
	{
		qCritical() << "setinfo json data went wrong:" << serializer.errorMessage();
		return "-1";
	}
}

/******11、json加油机信息**************
DATAID	Varchar2(6)	否	对象ID，在本次数据传输中唯一
DATE	Date	否	状态采集时间
JYJID	varchar	否	加油机标识
JYQID	varchar	否	加油枪标识
 * ****************************/
QString post_foshan::json_guninfo(QString DataId,QString Date,QString JYJID,QString JYQID)
{
	QByteArray json_zh;
	bool ok;
	QVariantList Json_data;
	QVariantMap JsonRead;
	JsonRead.insert("DATAID",DataId);
	JsonRead.insert("DATE",Date);
	JsonRead.insert("JYJID",JYJID);
	JsonRead.insert("JYQID",JYQID);


	Json_data << JsonRead;

	QJson::Serializer serializer;
	QByteArray json = serializer.serialize(Json_data, &ok);
	json_zh = json;
	QString test = json;
	//test = test.left(test.length() -1);//去掉右端】
	//test = test.right(test.length() -1);//去掉左端【
	test.remove(QRegExp("\\s"));//去除空格
	test = test.replace(QRegExp("\\#")," ");//添加空格
	if (ok)
	{
		return test;
	}
	else
	{
		qCritical() << "setinfo json data went wrong:" << serializer.errorMessage();
		return "-1";
	}
}
