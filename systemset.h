#ifndef SYSTEMSET_H
#define SYSTEMSET_H

#include <QDialog>
#include<QStandardItemModel>

#include"warn.h"
#include"keyboard.h"
//extern unsigned char Flag_Set_yuzhi;
//extern unsigned char Flag_pre_mode;

namespace Ui {
class systemset;
}

class systemset : public QDialog
{
    Q_OBJECT

public:
    explicit systemset(QWidget *parent = 0);
    ~systemset();

private:
    Ui::systemset *ui;
    warn *warn_Scr;
    keyboard *touchkey;
    QTimer *delay10s;
    QSignalMapper *Manager_Mapper;
    QStandardItemModel *model;
    QStandardItemModel *model_gundetail;
    QStandardItemModel *model_yingshe;
    bool IPLive(QString ip, int port);
private slots:
    bool eventFilter(QObject *, QEvent *);

    void on_pushButton_3_clicked();

    void on_pushButton_autoip_clicked();
    void on_pushButton_ipset_clicked();

    void setText_ip(const QString& text);
    void setText_mask(const QString& text);
    void setText_bcast(const QString& text);
	void setText_ifisport_udp(const QString& text);
    void setText_ifisport_tcp(const QString& text);
    void setText_hubei_station_id(const QString& text);
    void setBackspace_ip();
    void setBackspace_mask();
    void setBackspace_bcast();
    void setBackspace_ifisport_udp();
    void setBackspace_ifisport_tcp();
    void setBackspace_hubei_station_id();

//用户管理
    void setText_manid(const QString& text);
    void setText_manpw(const QString& text);
    void setText_username(const QString& text);
    void setText_userpw(const QString& text);
    void setText_adduser(const QString& text);
    void setText_adduser_2(const QString &text);
    void setText_deluser(const QString& text);

    void setBackspace_mainid();
    void setBackspace_manpw();
    void setBackspace_username();
    void setBackspace_userpw();
    void setBackspace_adduser();
    void setBackspace_adduser_2();
    void setBackspace_deluser();
    //post添加
	void setText_postaddress(const QString& text);
    void setText_postdataid(const QString& text);
    void setText_postuserid(const QString &text);
    void setText_postversion(const QString& text);
	void setText_postusername(const QString &text);
	void setText_postpassword(const QString& text);

	void setBackspace_postaddress();
    void setBackspace_postdataid();
    void setBackspace_postuserid();
    void setBackspace_postversion();
	void setBackspace_postusername();
	void setBackspace_postpassword();
    //isoosi添加
    void setText_isoosi_mn(const QString &text);
    void setText_isoosi_pw(const QString &text);
	void setText_isoosi_urlip(const QString &text);
	void setText_isoosi_urlport(const QString &text);
    void setBackspace_isoosi_mn();
    void setBackspace_isoosi_pw();
	void setBackspace_isoosi_urlip();
	void setBackspace_isoosi_urlport();

	//isoosi重庆
	void setText_isoosi_cqid(const QString &text);
	void setBackspace_isoosi_cqid();
    //**************added for radar*************/

    void setText_area_x1(const QString& text);
    void setText_area_x2(const QString& text);
    void setText_area_x3(const QString& text);
    void setText_area_x4(const QString& text);
    void setText_area_x5(const QString& text);
    void setText_area_x6(const QString& text);
    void setText_area_y1(const QString& text);
    void setText_area_y2(const QString& text);
    void setText_area_y3(const QString& text);
    void setText_area_y4(const QString& text);
    void setText_area_y5(const QString& text);
    void setText_area_y6(const QString& text);
    void setText_yuzhi_handinput(const QString& text);

    void setBackspace_area_x1();
    void setBackspace_area_x2();
    void setBackspace_area_x3();
    void setBackspace_area_x4();
    void setBackspace_area_x5();
    void setBackspace_area_x6();
    void setBackspace_area_y1();
    void setBackspace_area_y2();
    void setBackspace_area_y3();
    void setBackspace_area_y4();
    void setBackspace_area_y5();
    void setBackspace_area_y6();
    void setBackspace_yuzhi_handinput();
	//服务器
	void setText_myserverid(const QString& text);
	void setText_myserverpw(const QString& text);
	void setText_myserverip(const QString& text);
	void setText_myserverport(const QString& text);
	void setBackspace_myserverid();
	void setBackspace_myserverpw();
	void setBackspace_myserverip();
	void setBackspace_myserverport();



    //**************added for radar*************/
	void area_point_disp(unsigned char t);

    void on_comboBox_5_currentIndexChanged(int index);


    void on_comboBox_4_currentIndexChanged(int index);

    void on_comboBox_2_currentIndexChanged(int index);

    void on_comboBox_3_currentIndexChanged(int index);

    void on_comboBox_currentIndexChanged(int index);

    void setok_delay10sclose();

    void on_toolButton_clicked();

    void on_toolButton_manid_clicked();

    void on_toolButton_manpw_clicked();

    void on_toolButton_userid_clicked();

    void on_toolButton_useradd_clicked();

    void on_toolButton_userdel_clicked();

    void on_toolButton_userpw_clicked();

    void on_toolButton_useradd_2_clicked();

    void warn_manid_showed();
    void warn_manpw_showed();
    void warn_userpw_showed();
    void warn_add_showed();
    void warn_del_showed();

    void on_toolButton_warn_manid_enter_clicked();

    void on_toolButton_warn_manpw_enter_clicked();

    void on_toolButton_warn_userpw_enter_clicked();

    void on_toolButton_warn_add_enter_clicked();

    void on_toolButton_warn_del_enter_clicked();

    void on_toolButton_warn_manid_cancel_clicked();

    void on_toolButton_warn_manpw_cancel_clicked();

    void on_toolButton_warn_userpw_cancel_clicked();

    void on_toolButton_warn_add_cancel_clicked();

    void on_toolButton_warn_del_cancel_clicked();

    void on_toolButton_warn_inputerror_enter_clicked();

    void on_toolButton_warn_usernoexist_enter_clicked();

    void whoareyou_userset(unsigned char t);

    void dispset_for_managerid(const QString& text);
    void on_toolButton_poweroff_enter_clicked();
    void on_toolButton_poweroff_cancl_clicked();
    void on_toolButton_poweroff_clicked();

    void on_toolButton_warn_update_enter_clicked();

    void on_toolButton_warn_update_cancl_clicked();

    void on_toolButton_histoclr_clicked();

    void on_toolButton_warn_histclr_enter_clicked();

    void on_toolButton_warn_histclr_cancl_clicked();

    void history_autoip();      //记录ip设置
    void history_ipset();       //手动

    void on_toolButton_gujian_clicked();

    void on_toolButton_warn_gujian_enter_clicked();

    void on_toolButton_warn_gujian_cancl_clicked();

    /*****************************以下为雷达部分槽***************************/

    void toolButton_areaset_enter_clicked();

    void on_comboBox_starttime_h_currentIndexChanged(int index);

    void on_comboBox_starttime_m_currentIndexChanged(int index);

    void on_comboBox_stoptime_h_currentIndexChanged(int index);

    void on_comboBox_stoptime_m_currentIndexChanged(int index);

    void on_comboBox_silent_h_currentIndexChanged(int index);

    void on_comboBox_silent_m_currentIndexChanged(int index);

    void on_comboBox_warndelay_m_currentIndexChanged(int index);

    void on_comboBox_warndelay_s_currentIndexChanged(int index);

    void on_toolButton_outdor_warnopen_clicked();

    void on_toolButton_outdor_warnclo_clicked();

    void on_toolButton_areaopen_1_clicked();

    void on_toolButton_areaclo_1_clicked();

    void on_toolButton_areaopen_2_clicked();

    void on_toolButton_areaopen_3_clicked();

    void on_toolButton_areaopen_4_clicked();

    void on_toolButton_areaclo_2_clicked();

    void on_toolButton_areaclo_3_clicked();

    void on_toolButton_areaclo_4_clicked();

    void on_comboBox_sensitivity_currentIndexChanged(int index);

    void on_toolButton_yuzhi_collect_clicked();

    void on_toolButton_yuzhicollect_clicked();

    void paint_yuzhi_collect();

    int Check_Temp();//更新阈值设置的200三维数组

    void on_toolButton_mov_left_clicked();

    void on_toolButton_yuzhi_doublauto_clicked();

    void on_toolButton_yuzhi_doublhand_clicked();

    void on_toolButton_mov_righ_clicked();

    void doublhand_pointdrw();

    void doublanto_linedrw();

    void on_toolButton_areaautoset_clicked();

    //*******************雷达部分槽*****//

    void systemset_showed();

    void on_toolButton_setyuzhi_refresh_clicked();

    void on_toolButton_yuzhigengxin_clicked();

    void on_toolButton_yuzhihecheng_clicked();



    void on_toolButton_kaiqi_clicked();

    void on_toolButton_guanbi_clicked();

    void repaint_seted_yuzhi();


    void on_lineEdit_yuzhi_handinput_textChanged(const QString &arg1);

    void on_pushButton_time_clicked();

    void on_comboBox_whicharea_currentIndexChanged(int index);

    void on_toolButton_setanquan_clicked();

    void on_tabWidget_all_currentChanged(int index);

    void on_tabWidget_radar_currentChanged(int index);

    void on_pushButton_pre_mode_clicked();
//油气回收
    void on_comboBox_dispen_sumset_currentIndexChanged(const QString &arg1);
    void tableview_1_replay(int t);
    void tableview_2_replay();
    void set_amount_gun(const QString &text);
    void set_detailof_gun(const QString &text);
    void set_detailof_gun_backspace();
    void on_tableView_dispenerset_clicked(const QModelIndex &index);

    void tableView_yingshe_replay();//映射表绘制
    void set_yingshe_gun(const QString &text);//映射表设置
    void set_yingshe_gun_backspace();//映射表设置

    void on_toolButton_gunamount_enter_clicked();

    void on_tableView_gundetailset_clicked(const QModelIndex &index);

    void on_toolButton_gundetail_enter_clicked();

    void on_toolButton_reoilgas_versionask_clicked();
    void Version_Recv_FromMainwindow(unsigned char high,unsigned char low);

    void on_toolButton_reoilgas_setask_clicked();
    void Setinfo_Recv_FromMainwindow(unsigned char a,unsigned char b,unsigned char c,unsigned char d,unsigned char e,unsigned char f,unsigned char g,unsigned char h,unsigned char i,unsigned char j);
    void setText_pv(const QString &text);
    void setBackspace_pv();
    void setText_pv_nega(const QString &text);
    void setBackspace_pv_nega();
    void on_toolButton_pv_clicked();

    void on_toolButton_pv_nega_clicked();

    void on_comboBox_burngas_currentIndexChanged(const QString &arg1);

    void Reoilgas_FactorUartError_Setted(unsigned char state,unsigned int whichone);
    void Reoilgas_Factor_Setover();

    void on_toolButton_reoilgas_tankpre_open_clicked();

    void on_toolButton_reoilgas_tankpre_close_clicked();

    void on_toolButton_reoilgas_pipepre_open_clicked();

    void on_toolButton_reoilgas_pipepre_close_clicked();

    void on_toolButton_gas1_open_clicked();

    void on_toolButton_gas1_close_clicked();

    void on_toolButton_kaiqi_IIE_clicked();

    void on_toolButton_guanbi_IIE_clicked();

    void on_comboBox_far_dispener_currentIndexChanged(int index);

    void on_pushButton_allclean_clicked();

    void on_toolButton_warn_all_clean_queren_clicked();

    void on_toolButton_warn_allclean_quxiao_clicked();

    void on_toolButton_kaiqi_yewei_clicked();

    void on_toolButton_guanbi_yewei_clicked();

    void on_toolButton_kaiqi_beng_clicked();

    void on_toolButton_guanbi_beng_clicked();

    void on_toolButton_warn_ip_wrong_enter_clicked();

    void on_toolButton_warn_ip_wrong_cancl_clicked();

    void on_tableView_yingshe_clicked(const QModelIndex &index);

    void on_pushButtonsave_yingshe_clicked();

    void on_comboBox_num_cc_currentIndexChanged(int index);

    void on_pushButton_enter_cc_clicked();


    void on_comboBox_burngas_currentIndexChanged(int index);

    void on_toolButton_postdataid_clicked();

    void on_pushButton_shield_network_clicked();

    void on_toolButton_post_kaiqi_clicked();

    void on_toolButton_post_guanbi_clicked();

    void on_toolButton_ifisport_tcp_clicked();

    void on_comboBox_al_low_activated(const QString &arg1);

    void on_comboBox_al_high_activated(const QString &arg1);

    void on_comboBox_alwarndays_activated(const QString &arg1);

    void on_toolButton_screen_cc_clicked();

    void on_toolButton_screen_zaixian_clicked();

    void on_toolButton_screen_burngas_clicked();

    void on_toolButton_screen_safe_clicked();

    void on_toolButton_screen_radar_clicked();

    void on_toolButton_screen_xielou_clicked();

    void on_toolButton_gun_off_kaiqi_clicked();

    void on_toolButton_gun_off_guanbi_clicked();

    void on_comboBox_pressure_transmitters_mode_mode_currentIndexChanged(int index);

    void on_toolButton_isoosi_up_clicked();

    void on_toolButton_isoosi_down_clicked();

    void on_toolButton_isoosi_pw_clicked();

    void on_comboBox_network_version_currentIndexChanged(int index);

    void on_toolButton_isoosi_pb_clicked();

    void on_pushButton_testnet_clicked();

    void on_comboBox_reoilgas_ver_currentIndexChanged(int index);

    void on_toolButton_station_id_queren_clicked();

    void on_toolButton_hubei_kaiguan_clicked();

    void on_comboBox_xielou_net_version_currentIndexChanged(int index);

    void on_toolButton_gun_off_kaiqi_2_clicked();

    void on_toolButton_pop_show_clicked();

    void on_toolButton_reoilgas_tem_open_clicked();

    void on_toolButton_reoilgas_tem_close_clicked();

	void on_pushButtonsave_yingshe_copy_clicked();

	void on_toolButton_MyServerSwitch_clicked();

	void on_pushButton_U_clear_clicked();

signals:

    void amount_basin_reset(int t);
    void amount_pipe_reset(int t);
    void amount_dispener_reset(int t);
    void amount_tank_reset(int t);
    void method_tank_reset(int t);

    void hide_warn_button(int t);       //ip设置两个按钮中一个需要隐藏
    void mainwindow_enable();
    void closeing_touchkey();


    void mainwindow_repainting();
    void amount_safe_reset();//安全防护设备重绘

    void hide_tablewidget(unsigned char which,unsigned char sta);//隐藏不需要的页面
    //警告窗口弹出
    //void warn_manid_show();
    //void warn_manpw_show();
    //void warn_userpw_show();
    //void warn_add_show();
    //void warn_del_show();

    /*****************************以下为雷达部分信号***************************/
    void amount_oilgas_dispen_set(int t);
    void amount_oilgas_gun_set();
    /****************可燃气体信号*****/
    void amount_burngas_set(int t);
    /***************油气回收信号******/
    void Pre_Tank_Close();
    void Pre_Pipe_Close();
    void Fga_Gas_Close();
    void Tem_Tank_Close();
    /***************防撞柱信号******/
    void crash_num_reset();
private:
    QTimer *timer_Check_temp;   //temp与master交换论寻
    QTimer *timer_doublauto_linedrw;    //双路自动设置曲线刷新
    QTimer *timer_doublhand_pointdrw;   //双路手动设置点刷新
    void network_onfigurationdata(QString id,QString jyqs,QString pvz,QString pvf,QString hclk,QString yzqh);//网络发送配置信息函数
signals:
    void button_sysshow();  //主界面取消自动设置按钮显示
 //   void mainwindow_radar_click();  //点击自动设置后，主界面跳转
    //post添加
    void Send_Configurationdata(QString id,QString jyqs,QString pvz,QString pvf,QString hclk,QString yzqh);   //发送配置数据报文
	void Send_Configurationdata_HuNan(QString id,QString jyqs,QString pvz,QString pvf,QString hclk,QString yzqh);   //发送配置数据报文
	//isoosi添加
    void setup_data(QString PVFrwPrs,QString PVRevPrs,QString TrOpenPrs,QString TrStopPrs);
	//isoosi添加重庆
	void setup_data_cq(QString PVFrwPrs,QString PVRevPrs,QString TrOpenPrs,QString TrStopPrs);

};

#endif // SYSTEMSET_H















