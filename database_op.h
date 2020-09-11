#ifndef DATABASE_OP_H
#define DATABASE_OP_H
#include<qstring.h>

void creat_table();

void add_value_controlinfo(QString whichone, QString state);
void add_value_operateinfo(QString whichone, QString operate);
void add_value_netinfo(QString state);
void add_value_radarinfo(QString whichandstate,QString location);
void add_value_jingdianinfo(QString state);
void add_value_IIE(QString state);
void add_value_reoilgas(int whichone_i, int whichgun_j, int howlong, int countofgas, int countofoil);
void add_value_envinfo(float tankpre, float pipepre, float temperature, unsigned char nongdu);
void add_value_fgainfo(QString whichone, QString state);
void add_value_reoilgaswarn(QString whichone, QString state);
void add_value_gunwarn_details(QString whichone,QString state);

void select_table();
void oneday_analy(unsigned char whichdisp,unsigned char flag_time);

//图表显示 日数据选择
void select_data_oneday_tanktempr();
void select_data_oneday_tankpre();
void select_data_oneday_pipepre();
void select_data_oneday_tanknongdu();
void select_data_oneday_oilgun_al(unsigned char t);
//图表显示 月数据选择
void select_data_onemonth_tanktempr();
void select_data_onemonth_tankpre();
void select_data_onemonth_pipepre();
void select_data_onemonth_tanknongdu();
void select_data_onemonth_oilgun_al(unsigned char t);
//预留
void update_table();
void delete_table();
void output_table();
//security
void add_value_liquid(QString state);
void add_value_pump(QString state);
void add_value_crash(QString state);

//液位仪
void add_yeweiyi_alarminfo(QString whichone, QString state);
void add_yeweiyi_addOil_Record_Write(QString start_time,QString whichone, QString sumall);
void add_yeweiyi_addOil_Auto_Record_Write(QString start_time,QString end_time,QString whichone, QString sumall);

#endif // DATABASE_OP_H
