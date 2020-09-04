#ifndef FGA1000_485_H
#define FGA1000_485_H

#include <QThread>
#include <QTimer>
extern unsigned char Flag_MyserverFirstSend;
class FGA1000_485 :public QThread
{
    Q_OBJECT
public:
    bool stop;
    explicit FGA1000_485(QObject *parent = 0);
    void run();

private:
    void gpio_26_high();
    void gpio_26_low();
    void gpio_27_high();
    void gpio_27_low();

    void init_fga_num();//读取传感器数目
    void data_processing();//发送信号
    void empty_array(); //把没有设置的传感器浓度数组清零

    void send_Pressure_Transmitters();//问压力变送器
    void floating_point_conversion();//浮点数转换函数
    void send_Pressure_Transmitters_485();//问压力变送器
    void floating_point_conversion_485();//浮点数转换函数
    void sta_pressure();//压力变送器状态判断函数

    void network_Warndata(QString id,QString sta_yg,QString sta_yz,QString hclzt); //发送报警数据报文
    void network_Surroundingsdata(QString id,QString ygyl,QString yzyl,QString yqkj,QString xynd,QString hclnd,QString yqwd);    //发送环境数据报文
    void network_Wrongsdata(QString id,QString type);       //发送故障数据报文
    void network_Stagundata(QString id,QString status);      //发送油枪状态报文
    void network_Closegunsdata(QString id,QString jyjid,QString jyqid,QString operate,QString event);//关枪数据
    void network_Configurationdata(QString id);//设置数据，每天发送一次用


signals:
    void data_show();//显示数值
    void normal_fga(int n);//正常
    void alarm_hig_fga(int n);//低限位报警
    void alarm_low_fga(int n);//高限位报警
    void alarm_sensor_fga(int n);//传感器故障
    void alarm_uart_fga(int n);//通讯故障
    void alarm_sensor_de_fga(int n);//探测器传感器故障

    void init_burngas_setted(int);//桌面开机初始化可燃气体数量

    void Zeroing_success(int n); //校零成功信号
    void Zeroing_fail(int n); //校零失败信号

    void alarm_uart_pressure(int n);//压力变送器通讯故障，显示数值
    void normal_pressure(int n);//压力变送器通讯正常，显示数值
    void alarm_early_pre(int n);//压力预警
    void alarm_warn_pre(int n);//压力报警

    void alarm_tem_warn();//温度传感器故障
    void alarm_tem_normal();//温度传感器正常

    //post添加
    void Send_Warndata(QString id,QString al,QString mb,QString yz,QString ygly,QString pvzt,QString pvljzt,QString hclzt); //发送报警数据报文
    void Send_Surroundingsdata(QString id,QString ygyl,QString yzyl,QString yqkj);    //发送环境数据报文
    void Send_Wrongsdata(QString id,QString type);       //发送故障数据报文
    void Send_Stagundata(QString id,QString status);      //发送油枪状态报文
    void Send_Closegunsdata(QString id,QString jyjid,QString jyqid,QString operate,QString event);//关枪数据
    void Send_Configurationdata(QString id,QString jyqs,QString pvz,QString pvf,QString hclk,QString yzqh);   //发送配置数据报文,每天一次的那种
	//湖南
	void Send_Requestdata_HuNan(QString type,QString data);     //发送请求数据报文
	void Send_Configurationdata_HuNan(QString id,QString jyqs,QString pvz,QString pvf,QString hclk,QString yzqh);   //发送配置数据报文
	void Send_Warndata_HuNan(QString id,QString data,QString al,QString mb,QString yz,QString ygyl,QString clzznd,QString pv,QString clzzqd,QString clzztz,QString xyhqg); //发送报警数据报文
	void Send_Surroundingsdata_HuNan(QString id,QString data,QString ygyl,QString yzyl,QString xnd,QString clnd,QString yqwd,QString yqkj);    //发送环境数据报文
	void Send_Wrongsdata_HuNan(QString id,QString type);       //发送故障数据报文
	void Send_Closegunsdata_HuNan(QString id,QString jyjid,QString jyqid,QString operate,QString event);       //发送关枪数据报文
	void Send_Stagundata_HuNan(QString id,QString status);      //发送油枪状态报文

	//廊坊
	void Send_Configurationdata_LF(QString id,QString jyqs,QString pvz,QString pvf,QString hclk,QString hclt,QString yzqh);   //发送配置数据报文
	void Send_Warndata_LF(QString id,QString al,QString mb,QString yz,QString ygyl,QString ygly,QString pvzt,
					   QString pvljzt,QString hclzt,QString hclnd,QString xyhqg); //发送报警数据报文
	void Send_Surroundingsdata_LF(QString id,QString ygyl,QString yzyl,QString yqkj,
							   QString xnd,QString hclnd,QString yqwd);    //发送环境数据报文
	void Send_Wrongsdata_LF(QString id,QString type);       //发送故障数据报文
	void Send_Closegunsdata_LF(QString id,QString jyjid,QString jyqid,QString operate,QString event);       //发送关枪数据报文
	void Send_Stagundata_LF(QString id,QString status);      //发送油枪状态报文
        //重庆渝北
        void Send_Configurationdata_CQYB(QString id,QString jyqs,QString pvz,QString pvf,QString hclk,QString hclt,QString yzqh);   //发送配置数据报文
        void Send_Warndata_CQYB(QString id,QString al,QString mb,QString yz,QString ygyl,QString clzznd,
                                      QString pv,QString clzzqd,QString clzztz,QString xyhqg);//发送报警数据报文
        void Send_Surroundingsdata_CQYB(QString id,QString ygyl,QString yzyl, QString xnd,QString hclnd,QString yqwd,QString yqkj);    //发送环境数据报文
        void Send_Wrongsdata_CQYB(QString id,QString type);       //发送故障数据报文
	//佛山
	void send_setinfo_foshan(QString DataId,QString Date,QString JYQS,QString PVZ,QString PVF,
						 QString HCLK,QString HCLT,QString YZQH);
	void send_warninfo_foshan(QString DataId,QString Date,QString AL,QString MB,QString YZ,QString YGYL
						  ,QString YGLY,QString PVZT,QString PVLJZT,QString HCLZT,QString HCLND,QString XYHQG);
	void send_environment_foshan(QString DataId,QString Date,QString YGYL,QString YZYL,QString YQKJ,QString XND,QString HCLND,QString YQWD);
	void send_wrong_foshan(QString DataId,QString Date,QString TYPE);
	void send_gunoperate_foshan(QString DataId,QString Date,QString JYJID,QString JYQID,QString OPERATE,QString EVENT);
	void send_gunsta_foshan(QString DataId,QString Date,QString STATUS);


    //isoosi添加
    void environmental_data(QString dynbPress,QString tankPress,QString unloadgasCon,QString DevicegasCon,QString GasTemp,QString GasVolume);
    void gun_warn_data(QString gun_data,QString TightAlm,QString DynbPAlm,QString TankPAlm,QString DeviceAlm,QString PVTapAlm,QString DevOpenAlm,QString DevStopAlm);
    void refueling_gun_sta(QString gun_data);
    void refueling_gun_stop(QString gun_num,QString operate,QString Event);
    void setup_data(QString PVFrwPrs,QString PVRevPrs,QString TrOpenPrs,QString TrStopPrs);//用来每天上传一次设置数据
	//isoosi添加 重庆
	void environmental_data_cq(QString dynbPress,QString tankPress,QString unloadgasCon,QString DevicegasCon,QString GasTemp,QString GasVolume);
	void setup_data_cq(QString PVFrwPrs,QString PVRevPrs,QString TrOpenPrs,QString TrStopPrs);
	void gun_warn_data_cq(QString gun_data,QString gun_num,QString TightAlm,QString DynbPAlm,QString TankPAlm,QString TankZerom,QString Prevalve,QString prevavlelimit,QString DevOpenAlm,QString DeviceAlm,QString xieyousta );
	void refueling_gun_stop_cq(QString gun_num,QString operate,QString Event);
	void refueling_wrongdata_cq(QString warn_data);

	//myserver添加
	void environmental_data_myserver(QString dynbPress,QString tankPress,QString unloadgasCon,QString DevicegasCon,QString GasTemp,QString GasVolume);
	void gun_warn_data_myserver(QString gun_data,QString TightAlm,QString DynbPAlm,QString TankPAlm,QString DeviceAlm,QString PVTapAlm,QString DevOpenAlm,QString DevStopAlm);
	void refueling_gun_sta_myserver(QString gun_data);
	void refueling_gun_stop_myserver(QString gun_num,QString operate,QString Event);
	void setup_data_myserver(QString PVFrwPrs,QString PVRevPrs,QString TrOpenPrs,QString TrStopPrs);//用来每天上传一次设置数据

	//isoosi合肥测试
	void Send_Surroundingsdata_HeFei(QString YGYL,QString YZYL,QString YQWD);
private slots:
    void SendDataFGA();  //发送数据
    void ReadDataFGA();  //接收数据
    void read_pressure_transmitters();//读取压力变送器4-20ma
    void read_pressure_transmitters_485();//读取压力变送器485

    void Zeroing(int n); //校零函数  槽函数，与设置窗口的某个信号关联

    void time_time();

    void testday();
    //post添加
    void Fga_WarnPostSend(QString which,QString sta);
    void Fga_StaPostSend();
	//myserver添加
	void Myserver_First_Client();//服务器第一次连接，需要上传一次所有状态
	void First_Send(); //服务器第一次连接，需要上传一次所有状态
};

#endif // FGA1000_485_H
