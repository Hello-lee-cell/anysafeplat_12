#ifndef MYTHREAD_H
#define MYTHREAD_H

#include<QThread>
#include<QSharedMemory>
extern unsigned char FLAG_STACHANGE[50][11];//第1位，标志有没有数据，后面是设备地址，状态，时间
class mythread :public QThread
{
    Q_OBJECT
public:
    bool stop;
    explicit mythread(QObject *parent=0);
    void run();

signals:

//basin
    void warning_oil_basin(int t);      //油报警
    void warning_water_basin(int t);    //水报警
    void warning_sensor_basin(int t);   //传感器故障
    void warning_uart_basin(int t);     //通信故障
    void right_basin(int t);            //正常
//pipe
    void warning_oil_pipe(int t);      //油报警
    void warning_water_pipe(int t);    //水报警
    void warning_sensor_pipe(int t);   //传感器故障
    void warning_uart_pipe(int t);     //通信故障
    void right_pipe(int t);            //正常
//dispener
    void warning_oil_dispener(int t);      //油报警
    void warning_water_dispener(int t);    //水报警
    void warning_sensor_dispener(int t);   //传感器故障
    void warning_uart_dispener(int t);     //通信故障
    void right_dispener(int t);            //正常
//tank
    //传感器法
    void warning_oil_tank(int t);      //油报警
    void warning_water_tank(int t);    //水报警
    //液媒法
    void warning_high_tank(int t);      //高报警
    void warning_low_tank(int t);       //低报警
    //压力法
    void warning_pre_tank(int t);       //预报警
    void warning_warn_tank(int t);      //报警

    void warning_sensor_tank(int t);   //传感器故障
    void warning_uart_tank(int t);     //通信故障
    void right_tank(int t);            //正常
    void set_pressure_number(); //设置屏幕参数
	//服务器上传
	void myserver_send_single(QString DataType,QString SensorNum,QString SensorType,QString SensorSta,QString SensorData);
//    void warning_s_pipe_oil();          //0
//    void warning_s_pipe_water();        //1
//    void warning_s_pipe_sensor();       //2
//    void warning_s_pipe_uart();         //3

//    void warning_s_tank_oil();          //4
//    void warning_s_tank_water();        //5
//    void warning_s_tank_sensor();       //6
//    void warning_s_tank_uart();         //7
//    void warning_s_tank_pre();          //8
//    void warning_s_tank_warn();         //9
//    void warning_s_tank_high();         //10
//    void warning_s_tank_low();          //11

//    void warning_s_basin_oil();         //12
//    void warning_s_basin_water();       //13
//    void warning_s_basin_sensor();      //14
//    void warning_s_basin_uart();        //15

//    void warning_s_dispener_oil();      //16
//    void warning_s_dispener_water();    //17
//    void warning_s_dispener_sensor();   //18
//    void warning_s_dispener_uart();     //19

//    void warning_radar_uart_1#    //20    Flag_Sound_Radar[0
//    void warning_radar_warn_1#    //21    Flag_Sound_Radar[1
//    void warnint_radar_monibaojing //22   Flag_Sound_Radar[2
//    void warning_radar_uart_2#    //23
//    void warning_radar_warn_2#    //24
//    void warning_radar_uart_3#    //25
//    void warning_radar_warn_3#    //26
//    void warning_radar_uart_4#    //27
//    void warning_radar_warn_4#    //28    //现在程序中  sound数组少一位

    //***************给人体静电的信号*******//
    void set_renti(unsigned char whichbit,unsigned char value);
    void IIE_display(unsigned char uart, int R,int V,int oil_time,int people_time);
	void IIE_Electromagnetic_Show(unsigned char sta);//显示IIE电磁阀状态

public slots:
 //   void readFromSharedMem();
    void Data_Display(); //压力值显示
private:
	void myserver_send(QString DataType,QString SensorNum,QString SensorType,QString SensorSta,QString SensorData);//服务器上传传感器状态
	void IIE_analysis();
	void IIE_Electromagnetic_Analysis();//IIE电磁阀数据分析
	int net_history(int num,int sta);
};

#endif // MYTHREAD_H














