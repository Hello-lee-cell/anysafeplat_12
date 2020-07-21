#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "keyboard.h"
#include "login.h"
#include "systemset.h"
#include "connectus.h"
#include "uartthread.h"
#include "history.h"
#include "network/post_webservice.h"
#include "network/net_isoosi.h"
#include "reoilgas_pop.h"
#include "network/net_isoosi_cq.h"
#include "network/post_webservice_hunan.h"
#include "network/mytcpclient_zhongyou.h"
#include "network/main_main_zhongyou.h"
#include "myserver/myserver.h"
#include "oilgas/reoilgasthread.h"
#include "oilgas/fga1000_485.h"
#include "network/post_foshan.h"
#include "network/net_isoosi_hefei.h"

#include"ywythread.h"

class QLineEdit;
namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
private:
    keyboard *touchkey;
    login *Log;

//    QMovie *gif_nosensor;
//    QMovie *gif_oil;
//    QMovie *gif_sensor;
//    QMovie *gif_water;
//    QMovie *gif_uart;
//    QMovie *gif_right;
//    QMovie *gif_high;
//    QMovie *gif_low;
//    QMovie *gif_presurepre;
//    QMovie *gif_presurewarn;
    QMovie *gif_radar;

//    QMovie *gif_sensor_pre;
//    QMovie *gif_uart_pre;
//    QMovie *gif_presurepre_pre;
//    QMovie *gif_presurewarn_pre;
//    QMovie *gif_right_pre;

//    QMovie *gif_crash_warn;

    history *history_exec;
    systemset *systemset_exec;
    connectus *connectus_show;
    reoilgas_pop *reoilgas_pop_exec;
	reoilgasthread *uart_reoilgas;
	FGA1000_485 *uart_fga;
    ywythread *uart_ywy;

    //post添加
    post_webservice *post_message;
	post_webservice_hunan *post_message_hunan;
	post_foshan *post_message_foshan;
    //isoosi添加
    net_isoosi *thread_isoosi;
	net_isoosi_cq *thread_isoosi_cq;
	net_isoosi_hefei *thread_isoosi_hefei;
	myserver *myserver_thread;
private:
    Ui::MainWindow *ui;

uartthread *uart_run;

    QTimer *timer;//定时器
    QTimer *timer_lcd;//时间显示倒计时
    QTimer *timer_drw;//区域刷新计时

    QStandardItemModel *model;
private slots:
    bool eventFilter(QObject *, QEvent *);

    void Timerout_lcd();

    void on_pushButton_clicked();

//basin
    void warn_oil_set_basin(int t);     //油报警
    void warn_water_set_basin(int t);   //水报警
    void warn_sensor_set_basin(int t);  //传感器故障
    void warn_uart_set_basin(int t);    //通信故障
    void right_set_basin(int t);        //正常
//pipe
    void warn_oil_set_pipe(int t);     //油报警
    void warn_water_set_pipe(int t);   //水报警
    void warn_sensor_set_pipe(int t);  //传感器故障
    void warn_uart_set_pipe(int t);    //通信故障
    void right_set_pipe(int t);        //正常
//dispener
    void warn_oil_set_dispener(int t);     //油报警
    void warn_water_set_dispener(int t);   //水报警
    void warn_sensor_set_dispener(int t);  //传感器故障
    void warn_uart_set_dispener(int t);    //通信故障
    void right_set_dispener(int t);        //正常
//tank
    //传感器法
    void warn_oil_set_tank(int t);     //油报警
    void warn_water_set_tank(int t);   //水报警
    //液媒法
    void warn_high_set_tank(int t);     //高报警
    void warn_low_set_tank(int t);      //低报警
    //压力法
    void warn_pre_set_tank(int t);      //预报警
    void warn_warn_set_tank(int t);     //报警

    void warn_sensor_set_tank(int t);  //传感器故障
    void warn_uart_set_tank(int t);    //通信故障
    void right_set_tank(int t);        //正常

    void login_enter_set(int t);
    void Press_number();     //显示气压数值

    void amount_basin_setted();
    void amount_pipe_setted();
    void amount_dispener_setted();
    void amount_tank_setted();
    void method_tank_setted();

    void on_pushButton_7_clicked();
    void on_pushButton_3_clicked();

    void mainwindow_enabled();
    void pushButton_history_enabled();
    void pushButton_connect_enabled();

    void on_pushButton_2_clicked();

    void history_exec_creat();


    //***************added for radar****//
    //坐标绘制
    void area_painted();
    //描点
    void area_pointdrw();

    void on_toolButton_radar1_clicked();

    void on_toolButton_syssetshow_clicked();

    void key_syssetshow();

    void label_state_setted();

    //**********added for 人体静电*****//
    void set_label_jingdian(unsigned char whichbit, unsigned char value);
    void IIE_show(unsigned char IIE_uart_m, int IIE_R_m,int IIE_V_m,int IIE_oil_time_m,int IIE_people_time_m);
    void reset_safe();//重绘安全防护界面

    //油气回收
    void set_amount_oilgas_dispen(int t);
    void set_amount_oilgas_gun();

    void on_toolButton_close_oilgas_details_clicked();

    void on_pushButton_4_clicked();

    void on_toolButton_dispen_1_clicked();
    void on_toolButton_dispen_2_clicked();
    void on_toolButton_dispen_3_clicked();
    void on_toolButton_dispen_4_clicked();
    void on_toolButton_dispen_5_clicked();
    void on_toolButton_dispen_6_clicked();
    void on_toolButton_dispen_7_clicked();
    void on_toolButton_dispen_8_clicked();
    void on_toolButton_dispen_9_clicked();
    void on_toolButton_dispen_10_clicked();

    void gun_state_show();

    void Reoilgas_UartWrong_Maindisped(unsigned char whichone, unsigned char state);

    void Version_Recv_FromReoilgas(unsigned char high,unsigned char low);
    void Setinfo_Recv_FromReoilgas(unsigned char factoroil11,unsigned char factoroil12,unsigned char factorgas11,unsigned char factorgas12,unsigned char delay1,unsigned char factoroil21,unsigned char factoroil22,unsigned char factorgas21,unsigned char factorgas22,unsigned char delay2);

    void Disp_Reoilgas_Env();

    void Env_warn_normal_fga(int t);
    void Env_warn_hig_fga(int t);
    void Env_warn_low_fga(int t);
    void Env_warn_sensor_fga(int t);    //传感器故障
    void Env_warn_uart_fga(int t);
    void Env_warn_sensor_de_fga(int t); //探测器传感器故障

    void Env_warn_pre_uart(int t);
    void Env_warn_pre_normal(int t);
    void Env_warn_pre_pre(int t);
    void Env_warn_pre_warn(int t);

    void Env_warn_uart_tem();
    void Env_warn_normal_tem();

    void Env_Pre_Tank_Close();
    void Env_Pre_Pipe_Close();
    void Env_FGA_Gas1_Close();
    void Env_Tem_Tank_Close();

    void Reoilgas_Factor_SettedtoSys();

    void Draw_Tank_Tempr_Day();
    void Draw_Tank_Pre_Day();
    void Draw_Pipe_Pre_Day();
    void Draw_Tank_Nongdu_Day();    //取最大值
    void Draw_Oilgun_Al_Day(unsigned char t);//取最大值，最小值，双曲线

    void Draw_Tank_Tempr_Month();
    void Draw_Tank_Pre_Month();
    void Draw_Pipe_Pre_Month();
    void Draw_Tank_Nongdu_Month();  //取最大值
    void Draw_Oilgun_Al_Month(unsigned char t);
    //可燃气体
    void amount_burngas_setted(int t);

    void on_toolButton_dispen_11_clicked();

    void on_toolButton_dispen_12_clicked();

    void on_toolButton_detail_tankpre_clicked();

    void on_toolButton_close_draw_clicked();

    void on_toolButton_detail_tanktempr_clicked();

    void on_toolButton_daydata_clicked();

    void on_toolButton_monthdata_clicked();

    void on_toolButton_detail_gasvalue_clicked();

    void on_toolButton_gundetail_1_clicked();

    void on_toolButton_gundetail_2_clicked();

    void on_toolButton_gundetail_3_clicked();

    void on_toolButton_gundetail_4_clicked();

    void on_toolButton_gundetail_5_clicked();

    void on_toolButton_gundetail_6_clicked();

    void on_toolButton_detail_pipepre_clicked();

    void liquid_warn_s();//高液位报警
    void liquid_nomal_s();//液位正常
    void liquid_close_s();
    void pump_run_s();//油泵开启
    void pump_stop_s();//油泵关闭
    void pump_close_s();
    void crash_column_stashow(unsigned char whichone,unsigned char sta);
    void crash_column_reset();//防撞柱界面重绘

    void on_toolButton_gundetail_7_clicked();
    void on_toolButton_gundetail_8_clicked();

    void time_out_slotfunction(QDateTime date_time);//每秒执行的函数

    void on_pushButton_close_warn_rom_clicked();

    void hide_tablewidget(unsigned char which,unsigned char sta);

    //post添加
    void network_Wrongsdata(QString id ,QString whichone);//报警网络数据上传
    void on_pushButton_close_zaixianjiance_clicked();

    void on_pushButton_hiswarn_zaixianjiance_clicked();

    void show_pop_ups(int sta);//显示在线监测提示弹窗

    void on_toolButton_clicked();
    //加油机详情页
    void refresh_dispener_data_slot();
	//IIE电磁阀相关
	void on_pushButton_IIE_electromagnetic_clicked();
	void show_IIE_electromagnetic(unsigned char sta);

	void on_pushButton_IIEDetails_close_clicked();

    /**************** 液位仪**********************/
    //与MainWindow相关
    void Initialization_label();

    void draw_OilTank(int t);
    void draw_Tangan();
    void Display_Height_Data(unsigned char add, QString str1, QString str2, QString str3);
    void Display_alarm_Tangan_Data(unsigned char add, unsigned char flag);

    //与systemset相关
    void set_Tangan_add_success_slot(unsigned char add);
    void Send_compensation_slot(unsigned char command,unsigned char hang,float data);
    void compensation_set_result_slot(unsigned char command,unsigned char add,QString str);

    void on_btn_enter_add_Oil_clicked();

    void on_comboBox_addoil_currentIndexChanged(int index);

    void on_btn_addoil_start_clicked();

    void on_btn_addoil_end_clicked();

    void on_toolbtn_addoil_close_clicked();

signals:
    void config_sensoramount();
    void closeing_connect();
    void closeing_history();
    void closeing_login();
    void whoareyou(unsigned char t);
    void systemset_show();
    void set_press_number();  //气压设置信号
    void Version_To_SystemSet(unsigned char high,unsigned char low);
    void Setinfo_To_SystemSet(unsigned char factoroil11,unsigned char factoroil12,unsigned char factorgas11,unsigned char factorgas12,unsigned char delay1,unsigned char factoroil21,unsigned char factoroil22,unsigned char factorgas21,unsigned char factorgas22,unsigned char delay2);
    void Close_Oilgun(unsigned char i,unsigned char j,unsigned char whichone);
    void Time_Fga_1s();
    void Reoilgas_Factorset_UartWrong(unsigned char state,unsigned int whichone);
    void Reoilgas_Factor_Setted();
    void time_out_slotfunction_s(QDateTime date_time);//每秒执行的函数
    //post添加
    void Send_Wrongsdata(QString id,QString type);       //发送故障数据报文
	void Send_Wrongsdat_HuNan(QString id,QString type);       //发送故障数据报文
	void send_wrong_foshan(QString DataId,QString Date,QString TYPE);
	//isoosi重庆
	void refueling_wrongdata_cq(QString warn_data);
    //时间校准函数 每周
    void Time_calibration();
    //加油机详情页
    void refresh_dispener_data_signal();

    /**************** 液位仪**********************/
    //与systemset相关
    void set_Tangan_add_success_signal(unsigned char add);
    void Send_compensation_Signal(unsigned char command,unsigned char hang,float data);
    void compensation_set_result_Signal(unsigned char command,unsigned char add,QString str);
    void amount_Oil_Tank_draw(int t);
    void amount_Tangan_draw();
};

#endif // MAINWINDOW_H

