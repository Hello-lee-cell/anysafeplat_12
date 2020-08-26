#include "post_webservice_dongguan.h"
#include<QtDebug>
#include<QByteArray>
#include<QDateTime>
#include<QMutex>

#include<QNetworkAccessManager>
#include<QNetworkRequest>
#include<QNetworkReply>
#include<QDomDocument>

#include"network/gongguan_POST/sm4_uilt.h"
#include"config.h"
#include"file_op.h"
#include"database_op.h"

QMutex post_data_dongguan;

//QString Post_Address = "http://222.128.204.152:6008/dataRsv-DG/service/fileUpload";    //通信地址
//QString USERID_POST= "44010010006" ;    // 加油站编号11位


//QString VERSION_POST = "V1.1";   // 通信协议版本
//QString DATAID_POST  ;    //数据序号（6 位）
//QString OBJID_POST = "1";

//QString TIME_POST = "";      //在线监控设备当前时间（年月日时分 14 位）
//QString TYPE_POST = "";      //业务报文类型（2 位）
//QString SEC_POST = "1";
QString BUSINESSCONTENT_dg = "";    //业务报文（数据先SM4加密，然后转化为 base64 编码）
QString HMAC_POST = "";  //哈希校验 把业务报文 SM3加密

post_webservice_dongguan::post_webservice_dongguan(QWidget *parent) :
    QMainWindow(parent)
{
    m_accessManager = new QNetworkAccessManager(this);
    connect(m_accessManager,SIGNAL(finished(QNetworkReply*)),SLOT(requestFinished(QNetworkReply*)));
}

void post_webservice_dongguan::requestFinished(QNetworkReply* reply)
{
    if (reply->error() == QNetworkReply::NoError)
    {
        if(Flag_Network_Send_Version == 8)
        {
            QByteArray bytes = reply->readAll();
//                     qDebug() << bytes;
            QString string = QString::fromUtf8(bytes);
            QString re = ReadXml_dg(string);
//            qDebug() << re;
            if(re.indexOf('0') == 0)
            {
               add_value_netinfo("东莞在线监测数据上传成功");
            }
            else
            {
                add_value_netinfo("东莞在线监测数据上传失败");
            }
            qDebug()<<"dongguan send return "<<re;
        }
    }
    else
    {
        if(Flag_Network_Send_Version == 8){add_value_netinfo("东莞在线监测服务器访问失败");}

        qDebug()<<"dongguan errors here";
        QVariant statusCodeV = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
        //statusCodeV是HTTPserver的对应码，reply->error()是Qt定义的错误码，能够參考QT的文档
        qDebug( "found error ....code: %d %d\n", statusCodeV.toInt(), (int)reply->error());
        qDebug(qPrintable(reply->errorString()));
    }
    reply->deleteLater();
}

/*************post请求槽函数********************/
void post_webservice_dongguan::post_message(QString xml_data)
{
    post_data_dongguan.lock();//上锁

    if(net_state == 0)
    {
        QNetworkRequest *request = new QNetworkRequest();
        request->setUrl(QUrl(Post_Address));
        request->setHeader(QNetworkRequest::ContentTypeHeader,"application/xml");
        QNetworkReply* reply = m_accessManager->post(*request,xml_data.toUtf8());

        delete request;
        request = NULL;
    }
    else
    {
        qDebug() << "network is down, send is prevent!!";
    }
    post_data_dongguan.unlock();//解锁
}

/***************创建xml格式数据*************
1 VERSION 通信协议版本
2 DATAID 数据序号（6 位）
3 USERID 区域代码标识（6 位）+ 加油站标识（4 位）
4 TIME 在线监控设备当前时间（年月日时分 14 位）
5 TYPE 业务报文类型（2 位）
6 SEC 加密标识（1 表示业务数据为密文传输，0 表示明文）
7 BUSINESSCONTENT_dg 业务报文（数据需转化为 base64 编码  明文）
8 HMAC HMAC 校验码（预留）

TYPE  00、请求数据；01、配置数据；02、报警数据；03、加油枪数据；04、环境数据；05、故障数据；06、加油枪关停与启用；07、加油枪状态

******************************************/
QString CreatXml_dg(QString version,QString data_id,QString user_id,QString time,QString type,QString sec,QString bus_data,QString HMAC_data)
{
    QDomDocument doc;

    QString header("version=\"1.0\" encoding=\"UTF-8\"");
    doc.appendChild(doc.createProcessingInstruction("xml",header));
    QDomElement tagFileInfo_envelope = doc.createElementNS("http://www.w3.org/2003/05/soap-envelope", "soap:Envelope");
    tagFileInfo_envelope.setAttribute("xmlns:tem", "http://tempuri.org/");
    QDomElement tagFileInfo_header = doc.createElement("soap:Header");
    QDomElement tagFileInfo_body = doc.createElement("soap:Body");
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
    //传输方式
    QDomElement tagFileSec = doc.createElement("SEC");
    QDomText textFileSec = doc.createTextNode(sec);
    //数据报文
    QDomElement tagFileBusin = doc.createElement("BUSINESSCONTENT");
    QDomText textFileBusin = doc.createTextNode(bus_data);
    //HMAC校验  预留
    QDomElement tagFileHmac = doc.createElement("HMAC");
    QDomText textFileHmac = doc.createTextNode(HMAC_data);

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



   tagFileInfo_envelope.appendChild(tagFileInfo_header);
   tagFileInfo_envelope.appendChild(tagFileInfo_body);

    doc.appendChild(tagFileInfo_envelope);

    //转义字符 < > 都需要转换编码
    QString xmldata = doc.toString();
//     qDebug() << xmldata;

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
//             QByteArray byteArray(xmldata.toStdString().c_str(), xmldata.toStdString().length());
//             QByteArray encdata=QByteArray::fromBase64(byteArray);
//             qDebug() << encdata;
//     qDebug() << xmldata;
    QString return_xml;
    return_xml = xmldata;
    return return_xml;
}


/***************读取xml格式数据****************/
QString ReadXml_dg(QString read_xml)
{
    QString return_data;
    QDomDocument doc;
    doc.setContent(read_xml);
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



/******************发送配置数据****************
ID   对象ID，在本次数据传输中唯一
JYQS 加油枪数量
PVZ  PV阀正向压力值
PVF  PV阀负向压力值
HCLK 后处理装置开启压力值
YZQH 安装液阻传感器加油机
如果不存在某项数据则在数据域中填写“NULL
**********************************************/
void post_webservice_dongguan::Send_Configurationdata(QString id,QString jyqs,QString pvz,QString pvf,QString hclk,QString yzqh)
{
    if(Flag_Postsend_Enable == 1)
    {
        TYPE_POST = "01";
        //获取当前时间，数据上传需要
        QDateTime current_datetime = QDateTime::currentDateTime();
        TIME_POST = current_datetime.toString("yyyyMMddhhmmss");
        QString xml_data = XML_Configurationdata_dg(id,TIME_POST,jyqs,pvz,pvf,hclk,yzqh);

             qDebug() << xml_data;

        BUSINESSCONTENT_dg = encryptData_CBC(xml_data);
        HMAC_POST = encryptHMac(BUSINESSCONTENT_dg);

        QString send_xml = CreatXml_dg(VERSION_POST,DATAID_POST,USERID_POST,TIME_POST,TYPE_POST,SEC_POST,BUSINESSCONTENT_dg,HMAC_POST);

//             qDebug() << send_xml;

        post_message(send_xml);
    }
    else
    {
        qDebug() << "network post send disenable!";
    }
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
QString XML_Configurationdata_dg(QString id,QString data,QString jyqs,QString pvz,
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

/******************发送报警数据****************
ID     对象ID，在本次数据传输中唯一
AL     A/L（0、1、2、N），N指当日无加油
MB     密闭性（0、1、2、N）
YZ     液阻（0、1、2、N）
YGYL   油罐压力（0、1、2、N）
YGLY   油罐零压（0、1、2、N）
PVZT   压力/真空阀状态（0、1、2、N）
PVLJZT 压力/真空阀临界压力状态（0、1、2、N)
HCLZT  后处理装置状态（0、1、2、N)
HCLND   处理装置浓度
XYHQG  卸油回气管状态（0、1、2、N）
如果不存在某项数据则在数据域中填写“NULL
**********************************************/
void post_webservice_dongguan::Send_Warndata(QString id,QString al,QString mb,QString yz,QString ygyl,QString ygly,QString pvzt,QString pvljzt,QString hclzt,QString hclnd,QString xyhqg)
{
    if(Flag_Postsend_Enable == 1)
    {
        TYPE_POST = "02";
        //获取当前时间，数据上传需要
        QDateTime current_datetime = QDateTime::currentDateTime();
        TIME_POST = current_datetime.toString("yyyyMMddhhmmss");
        QString xml_data = XML_Warndata(id,TIME_POST,al,mb,yz,ygyl,ygly,pvzt,pvljzt,hclzt,hclnd,xyhqg);

        qDebug() << xml_data;

        BUSINESSCONTENT_dg = encryptData_CBC(xml_data);
        HMAC_POST = encryptHMac(BUSINESSCONTENT_dg);

        QString send_xml = CreatXml_dg(VERSION_POST,DATAID_POST,USERID_POST,TIME_POST,TYPE_POST,SEC_POST,BUSINESSCONTENT_dg,HMAC_POST);
        post_message(send_xml);
    }
    else
    {
        qDebug() << "network post send disenable!";
    }
}

/********************报警数据****************
ID     对象ID，在本次数据传输中唯一
DATE   监控时间
AL     A/L（0、1、2、N），N指当日无加油
MB     密闭性（0、1、2、N）
YZ     液阻（0、1、2、N）
YGYL   油罐压力（0、1、2、N）
YGLY   油罐零压（0、1、2、N）
PVZT   压力/真空阀状态（0、1、2、N）
PVLJZT 压力/真空阀临界压力状态（0、1、2、N)
HCLZT  后处理装置状态（0、1、2、N)
HCLND   处理装置浓度
XYHQG  卸油回气管状态（0、1、2、N）


// CLZZND 处理装置浓度
// PV     PV阀状态
// CLZZQD 处理装置启动状态
// CLZZTZ 后处理装置关闭状态
// XYHQG  卸油回气管状态

// PVZT   压力/真空阀状态（0、1、2、N）
// PVLJZT 压力/真空阀临界压力状态（0、1、2、N)
// HCLZT  后处理装置状态（0、1、2、N)
// 如果不存在某项数据则在数据域中填写“NULL
**********************************************/
QString XML_Warndata(QString id,QString data,QString al,QString mb,
                              QString yz,QString ygyl,QString ygly,QString pvzt,QString pvljzt,QString hclzt,QString hclnd,QString xyhqg)
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
    QDomElement tagFileYGYL = doc.createElement("YGYL");
    QDomText textFileYGYL = doc.createTextNode(ygyl);
    QDomElement tagFileYglY = doc.createElement("YGLY");
    QDomText textFileYgLy = doc.createTextNode(ygly);
    QDomElement tagFilePvzt = doc.createElement("PVZT");
    QDomText textFilePvzt = doc.createTextNode(pvzt);
    QDomElement tagFilePvljzt = doc.createElement("PVLJZT");
    QDomText textFilePvljzt = doc.createTextNode(pvljzt);
    QDomElement tagFileHclzt = doc.createElement("HCLZT");
    QDomText textFileHclzt = doc.createTextNode(hclzt);
    QDomElement tagFileHCLND = doc.createElement("HCLND");
    QDomText textFileHCLND = doc.createTextNode(hclnd);
    QDomElement tagFileXYHQG = doc.createElement("XYHQG");
    QDomText textFileXYHQG = doc.createTextNode(xyhqg);

    tagFileId.appendChild(textFileId);
    tagFileData.appendChild(textFileData);
    tagFileAl.appendChild(textFileAl);
    tagFileMb.appendChild(textFileMb);
    tagFileYz.appendChild(textFileYz);
    tagFileYGYL.appendChild(textFileYGYL);
    tagFileYglY.appendChild(textFileYgLy);
    tagFilePvzt.appendChild(textFilePvzt);
    tagFilePvljzt.appendChild(textFilePvljzt);
    tagFileHclzt.appendChild(textFileHclzt);
    tagFileHCLND.appendChild(textFileHCLND);
    tagFileXYHQG.appendChild(textFileXYHQG);

    tagFileInforow.appendChild(tagFileId);
    tagFileInforow.appendChild(tagFileData);
    tagFileInforow.appendChild(tagFileAl);
    tagFileInforow.appendChild(tagFileMb);
    tagFileInforow.appendChild(tagFileYz);
    tagFileInforow.appendChild(textFileYGYL);
    tagFileInforow.appendChild(tagFileYglY);
    tagFileInforow.appendChild(tagFilePvzt);
    tagFileInforow.appendChild(tagFilePvljzt);
    tagFileInforow.appendChild(tagFileHclzt);
    tagFileInforow.appendChild(tagFileHCLND);
    tagFileInforow.appendChild(tagFileXYHQG);

    tagFileInforows.appendChild(tagFileInforow);
    doc.appendChild(tagFileInforows);

    QString xmldata = doc.toString();
    //qDebug() << xmldata;
    return xmldata;
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
void post_webservice_dongguan::Send_Oilgundata(QString id,QString jyjid,QString jyqid,QString al,QString qls,QString qll,QString yls,QString yll,QString hyqnd,QString hyqwd,QString yz)
{
    if(Flag_Postsend_Enable == 1)
    {
        TYPE_POST = "03";
        //获取当前时间，数据上传需要
        QDateTime current_datetime = QDateTime::currentDateTime();
        TIME_POST = current_datetime.toString("yyyyMMddhhmmss");
        QString xml_data = XML_Oilgundata(id,TIME_POST,jyjid,jyqid,al,qls,qll,yls,yll,hyqnd,hyqwd,yz);

        qDebug() << xml_data;

        BUSINESSCONTENT_dg = encryptData_CBC(xml_data);
        HMAC_POST = encryptHMac(BUSINESSCONTENT_dg);

        QString send_xml = CreatXml_dg(VERSION_POST,DATAID_POST,USERID_POST,TIME_POST,TYPE_POST,SEC_POST,BUSINESSCONTENT_dg,HMAC_POST);
        post_message(send_xml);
    }
    else
    {
        qDebug() << "network post send disenable!";
    }
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
HYQND  回收油气浓度
HYQWD  回收油气温度
YZ     液阻，单位Pa
如果不存在某项数据则在数据域中填写“NULL
**********************************************/
QString XML_Oilgundata(QString id,QString data,QString jyjid,QString jyqid,QString al,
                              QString qls,QString qll,QString yls,QString yll,QString hyqnd,QString hyqwd,QString yz )
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
    QDomElement tagFileHYQND = doc.createElement("HYQND");
    QDomText textFileHYQND = doc.createTextNode(hyqnd);
    QDomElement tagFileHYQWD = doc.createElement("HYQWD");
    QDomText textFileHYQWD = doc.createTextNode(hyqwd);
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
    tagFileHYQND.appendChild(textFileHYQND);
    tagFileHYQWD.appendChild(textFileHYQWD);
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
    tagFileInforow.appendChild(tagFileHYQND);
    tagFileInforow.appendChild(tagFileHYQWD);
    tagFileInforow.appendChild(tagFileYz);

    tagFileInforows.appendChild(tagFileInforow);
    doc.appendChild(tagFileInforows);

    QString xmldata = doc.toString();
    //qDebug() << xmldata;
    return xmldata;
}


/******************发送环境数据****************
ID     对象ID，在本次数据传输中唯一
YGYL   油罐压力，单位Pa
YZYL   液阻压力，单位Pa
YQKJ   油气空间，单位L

XND    卸油区油气浓度，单位%/ppm
HCLND  后处理装置排放浓度，单位 g/m³
YQWD   油气温度，单位℃
如果不存在某项数据则在数据域中填写 "NULL"
不大于 30s 的间隔采集环境数据,2 到 10min 左右的时间间隔打包上传环境数据
**********************************************/
void post_webservice_dongguan::Send_Surroundingsdata(QString id,QString ygyl,QString yzyl,QString yqkj,QString xnd,QString hclnd,QString yqwd)
{
    if(Flag_Postsend_Enable == 1)
    {
        TYPE_POST = "04";
        QDateTime current_datetime = QDateTime::currentDateTime();
        TIME_POST = current_datetime.toString("yyyyMMddhhmmss");
        QString xml_data = XML_Surroundingsdata(id,TIME_POST,ygyl,yzyl,yqkj,xnd,hclnd,yqwd);

        qDebug() << xml_data;

        BUSINESSCONTENT_dg = encryptData_CBC(xml_data);
        HMAC_POST = encryptHMac(BUSINESSCONTENT_dg);

        QString send_xml = CreatXml_dg(VERSION_POST,DATAID_POST,USERID_POST,TIME_POST,TYPE_POST,SEC_POST,BUSINESSCONTENT_dg,HMAC_POST);
        post_message(send_xml);
    }
    else
    {
        qDebug() << "network post send disenable!";
    }
}

/********************环境数据****************
ID     对象ID，在本次数据传输中唯一
DATE   监控时间
YGYL   油罐压力，单位Pa
YZYL   液阻压力，单位Pa
YQKJ   油气空间，单位L

XND    卸油区油气浓度，单位%/ppm
HCLND  后处理装置排放浓度，单位 g/m³
YQWD   油气温度，单位℃
如果不存在某项数据则在数据域中填写 "NULL"
不大于 30s 的间隔采集环境数据,2 到 10min 左右的时间间隔打包上传环境数据
**********************************************/
QString XML_Surroundingsdata(QString id,QString data,QString ygyl,QString yzyl,QString yqkj,QString xnd,QString hclnd,QString yqwd)
{
    QDomDocument doc;
    QDomElement tagFileInforows = doc.createElement("rows");
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
    QDomElement tagFileXND = doc.createElement("XND");
    QDomText textFileXND = doc.createTextNode(xnd);
    QDomElement tagFileHCLND = doc.createElement("HCLND");
    QDomText textFileHCLND = doc.createTextNode(hclnd);
    QDomElement tagFileYQWD = doc.createElement("YQWD");
    QDomText textFileYQWD = doc.createTextNode(yqwd);

    tagFileId.appendChild(textFileId);
    tagFileData.appendChild(textFileData);
    tagFileYgyl.appendChild(textFileYgyl);
    tagFileYzyl.appendChild(textFileYzyl);
    tagFileYqkj.appendChild(textFileYqkj);
    tagFileXND.appendChild(textFileXND);
    tagFileHCLND.appendChild(textFileHCLND);
    tagFileYQWD.appendChild(textFileYQWD);

    tagFileInforow.appendChild(tagFileId);
    tagFileInforow.appendChild(tagFileData);
    tagFileInforow.appendChild(tagFileYgyl);
    tagFileInforow.appendChild(tagFileYzyl);
    tagFileInforow.appendChild(tagFileYqkj);
    tagFileInforow.appendChild(tagFileXND);
    tagFileInforow.appendChild(tagFileHCLND);
    tagFileInforow.appendChild(tagFileYQWD);

    tagFileInforows.appendChild(tagFileInforow);
    //doc.appendChild(tagFileInforows);
    doc.appendChild(tagFileInforow);

    QString xmldata = doc.toString();
   // qDebug() << xmldata;
    return xmldata;
}


/******************发送故障数据****************
ID     对象ID，在本次数据传输中唯一
TYPE   故障码
**********************************************/
void post_webservice_dongguan::Send_Wrongsdata(QString id,QString type)
{
    if(Flag_Postsend_Enable == 1)
    {
        TYPE_POST = "05";
        //获取当前时间，数据上传需要
        QDateTime current_datetime = QDateTime::currentDateTime();
        TIME_POST = current_datetime.toString("yyyyMMddhhmmss");
        QString xml_data = XML_Wrongsdata_dg(id,TIME_POST,type);

        qDebug() << xml_data;

        BUSINESSCONTENT_dg = encryptData_CBC(xml_data);
        HMAC_POST = encryptHMac(BUSINESSCONTENT_dg);

        QString send_xml = CreatXml_dg(VERSION_POST,DATAID_POST,USERID_POST,TIME_POST,TYPE_POST,SEC_POST,BUSINESSCONTENT_dg,HMAC_POST);
        post_message(send_xml);
    }
    else
    {
        qDebug() << "network post send disenable!";
    }
}

/********************故障数据****************
ID     对象ID，在本次数据传输中唯一
DATE   故障数据产生时间
TYPE   故障码
**********************************************/
QString XML_Wrongsdata_dg(QString id,QString data,QString type)
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

/******************发送油枪关停数据****************
ID       对象ID，在本次数据传输中唯一
JYJID    加油机标识
JYQID    加油枪标识
OPERATE  操作类型 0-关停 1-启用
EVENT    关停或启用事件类型关停事件类型：0 自动关停 1 手动关停 启用事件类型：0（预留） 1 手动启用未知事件类型用 N
**********************************************/
void post_webservice_dongguan::Send_Closegunsdata(QString id,QString jyjid,QString jyqid,QString operate,QString event)
{
    if(Flag_Postsend_Enable == 1)
    {
        TYPE_POST = "06";
        //获取当前时间，数据上传需要
        QDateTime current_datetime = QDateTime::currentDateTime();
        TIME_POST = current_datetime.toString("yyyyMMddhhmmss");
        QString xml_data = XML_Closegunsdata_dg(id,TIME_POST,jyjid,jyqid,operate,event);

        qDebug() << xml_data;

        BUSINESSCONTENT_dg = encryptData_CBC(xml_data);
        HMAC_POST = encryptHMac(BUSINESSCONTENT_dg);

        QString send_xml = CreatXml_dg(VERSION_POST,DATAID_POST,USERID_POST,TIME_POST,TYPE_POST,SEC_POST,BUSINESSCONTENT_dg,HMAC_POST);
        post_message(send_xml);
    }
    else
    {
        qDebug() << "network post send disenable!";
    }
}

/********************油枪关停数据****************
ID       对象ID，在本次数据传输中唯一
DATE     启用/关停时间
JYJID    加油机标识
JYQID    加油枪标识
OPERATE  操作类型 0-关停 1-启用
EVENT    关停或启用事件类型关停事件类型：0 自动关停 1 手动关停 2 环保关停  启用事件类型：0（预留） 1 手动启用未知事件类型用 N
**********************************************/
QString XML_Closegunsdata_dg(QString id,QString data,QString jyjid,QString jyqid,QString operate,QString event)
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
    tagFileInforow.appendChild(tagFileOperate);
    tagFileInforow.appendChild(tagFileEvent);

    tagFileInforows.appendChild(tagFileInforow);
    doc.appendChild(tagFileInforow);

    QString xmldata = doc.toString();
//     qDebug() << xmldata;
    return xmldata;
}


/******************发送加油枪状态****************
ID       对象ID，在本次数据传输中唯一
STATUS   加油枪开关
**********************************************/
void post_webservice_dongguan::Send_Stagundata(QString id,QString status)
{
    if(Flag_Postsend_Enable == 1)
    {
        TYPE_POST = "07";
        //获取当前时间，数据上传需要
        QDateTime current_datetime = QDateTime::currentDateTime();
        TIME_POST = current_datetime.toString("yyyyMMddhhmmss");
        QString xml_data = XML_Stagundata_dg(id,TIME_POST,status);

        qDebug() << xml_data;

        BUSINESSCONTENT_dg = encryptData_CBC(xml_data);
        HMAC_POST = encryptHMac(BUSINESSCONTENT_dg);

        QString send_xml = CreatXml_dg(VERSION_POST,DATAID_POST,USERID_POST,TIME_POST,TYPE_POST,SEC_POST,BUSINESSCONTENT_dg,HMAC_POST);
        post_message(send_xml);
    }
    else
    {
        qDebug() << "network post send disenable!";
    }
}

/********************加油枪状态****************
ID       对象ID，在本次数据传输中唯一
DATE     状态采集时间
STATUS   加油枪开关
**********************************************/
QString XML_Stagundata_dg(QString id,QString data,QString status)
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


