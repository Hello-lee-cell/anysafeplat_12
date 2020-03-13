#ifndef FILE_OP_H
#define FILE_OP_H

#include<QDebug>


#define HISTORY_SENSOR_T "/opt/control_test.txt"
#define HISTORY_OPERATE_T   "/opt/operate_test.txt"
#define HISTORY_NET_T   "/opt/net_test.txt"
#define HISTORY_RADAR_T     "/opt/radar/history_radar_t.txt"
#define HISTORY_JINGDIAN_T  "/opt/jingdian/history_jingdian_t.txt"
#define DELETE_WHICHLINE    2000


void config_SensorAmountChanged();
void history_net_write(const char *t);
void history_net_del();

void history_sensor_write_pr(int t);   //压力报警记录写入
void history_sensor_write(const char *numb,const char *t);
void history_sensor_del();
void history_operate_write(char *t);    //仅用于开机
void history_operate_write_detail(const QString &text,const char *t);  //用户名  操作内容
void history_operate_del();
void history_clearall();    //历史记录清除
void all_sta_clean();    //清除历史记录和所有报警记录

//主页面显示设置
void config_tabwidget();
void init_tabwidget();

//雷达历史记录部分
void history_radar_warn_write(const char *numb,char *t);
void history_radar_warn_del();

void config_radar_znwrite();
void config_backgroundvalue_machine1();
void config_boundary_machine1_area1();
void config_boundary_machine1_area2();
void config_boundary_machine1_area3();
void config_boundary_machine1_area4();
void config_boundary_machine1_area5();
void config_boundary_machine1_area6();


//人体静电
void history_jingdian_warn_write(const char *t);
void history_jingdian_warn_del();
//可燃气体
void config_num_fga();
//油气回收
void config_reoilgas_write();
void config_reoilgas_gasdetail_write();
void config_reoilgas_oildetail_write();
void config_info_accum_write(); //连续报警天数记录  气液比
void config_fga_accum_write(unsigned char type1, unsigned char type3);  //连续报警天数记录  气体/压力
void config_reoilgas_pre_en_write();
void config_pv_positive_write();
void config_pv_negative_write();
void config_mapping_write();//映射表部分写入
void config_Pressure_Transmitters_Mode_write();//压力变送器模式设置写入
void init_Pressure_Transmitters_Mode();//压力变送器模式设置读取
void config_reoilgas_warnpop();//弹窗设置相关
void init_reoilgas_warnpop();//弹窗设置相关读取
//气液比相关
void config_alset();
void init_alset();

void config_jingdian_write();
void config_IIE_write();
//液阻相关
void config_Liquid_resistance();
void init_Liquid_resistance();

//安全防护相关
void config_security();
//post添加
void config_post_network();
void config_network_Version_write();//油气回收网络上传版本写入
void init_network_Version();//油气回收网络上传版本读取
void config_isoosi_write();//油气回收网络上传相关参数
void init_isoosi_set();//油气回收网络上传相关参数
//泄漏网络上传
void config_xielou_network();//泄漏网络上传相关参数写入
void init_xielou_network();//泄漏网络上传相关参数读取
//服务器网络上传
void config_MyServer_network();//服务器网络上传相关参数写入
void init_myserver_network();//服务器网络上传相关参数读取
#endif // FILE_OP_H

