#ifndef REOILGASTHREAD_H
#define REOILGASTHREAD_H

#include<QThread>
#include<QTimer>
extern unsigned char ReoilgasPreSta[2];//压力传感器状态    与Fga1000_485共享
extern unsigned char ReoilGasFgaSta[8];//浓度传感器状态    与Fga1000_485共享
extern unsigned char ReoilgasTemSta[2];//温度传感器状态    与Fga1000_485共享
extern unsigned char Fga1000_Value[8]; //浓度值           与Fga1000_485共享
class reoilgasthread :public QThread
{
    Q_OBJECT
public:
    explicit reoilgasthread(QObject *parent = 0);
    void run();
private:
    void network_oilgundata(QString id,QString jyjid,QString jyqid,QString al,QString qls,QString qll,QString yls,QString yll,QString yz); //发送油枪数据报文
	void D433T3D_init();
	void Ask_Sensor();
	void ask_pressure();
	void ask_temperature();
	void ask_fga1000();
	void floating_point_conversion();
	void sync_data();//同步采集器设置
	void ask_690();
	void read_690();
	void send_data_690();
signals:
    void Version_To_Mainwindow(unsigned char high,unsigned char low);
    void Setinfo_To_Mainwindow(unsigned char factoroil11,unsigned char factoroil12,unsigned char factorgas11,unsigned char factorgas12,unsigned char delay1,unsigned char factoroil21,unsigned char factoroil22,unsigned char factorgas21,unsigned char factor22,unsigned char delay2);
//    void mainwindow_display(int i,int j,int a,int b,int c);
    void Warn_UartWrong_Mainwindowdisp(unsigned char whichone,unsigned char state);
    void Reoilgas_Factor_Setover();
	void reflash_setinfo();//刷新设置页脉冲当量
    //post添加
    void Send_Oilgundata(QString id,QString jyjid,QString jyqid,QString al,QString qls,QString qll,QString yls,QString yll,QString yz); //发送油枪数据报文
	void Send_Oilgundata_HuNan(QString id,QString data,QString jyjid,QString jyqid,QString al,QString qls,QString qll,QString yls,QString yll,QString yqnd,QString yqwd,QString yz); //发送油枪数据报文
	void send_gundata_foshan(QString DataId,QString Date,QString JYJID,QString JYQID,QString AL,QString QLS,
						 QString QLL,QString YLS,QString YLL,QString HYQND,QString HYQWD,QString YZ,QString STATE);

	void Send_Oilgundata_LF(QString id,QString jyjid,QString jyqid,QString al,QString qls,QString qll,QString yls,QString yll,
						 QString hyqnd,QString hyqwd,QString yz); //发送油枪数据报文
    	void Send_Oilgundata_CQYB(QString id,QString jyjid,QString jyqid,QString al,QString qls,QString qll,QString yls,QString yll,
                        	 QString hyqnd,QString hyqwd,QString yz); //发送油枪数据报文

	//isoosi添加
    void refueling_gun_data(QString gun_num,QString AlvR,QString GasCur,QString GasFlow,QString FuelCur,QString FuelFlow,QString DynbPrs);
	//isoosi重庆
	void refueling_gun_data_cq(QString gun_num,QString AlvR,QString GasCur,QString GasFlow,QString FuelCur,QString FuelFlow,QString gas_con,QString gas_tem,QString DynbPrs);

	//myserver添加
	void refueling_gun_data_myserver(QString gun_num,QString AlvR,QString GasCur,QString GasFlow,QString FuelCur,QString FuelFlow,QString DynbPrs);
	//isoosi合肥添加
	void refueling_gun_data_hefei(QString al);

	//同步采集器脉冲当量
	void signal_sync_data(unsigned int idi,unsigned int idj,float oil_factor1,float gas_factor1,float oil_factor2,float gas_factor2);
private slots:
    void SendDataReoilgas();
    void ReadDataReoilgas();
    void ReadDataReoilgas_Version();
    void ReadDataReoilgas_Setinfo();
    //第二版本气液比采集器
    void SendDataReoilgas_v2();
    void ReadDataReoilgas_v2();

    unsigned char Select_Sensor();
};

#endif // REOILGASTHREAD_H
