#include <QSqlRecord>
#include <QSqlQuery>
#include <qsqlerror.h>
#include <QDebug>
#include <QDateTime>
#include "file_op.h"
#include "database_op.h"
#include "config.h"
#include "systemset.h"
#include "network/net_tcpclient_hb.h"
#include <QSqlQueryModel>

/*************表************/
//泄漏检测：
//          报警记录    controlinfo
//          操作日志    operateinfo
//          通信日志    netinfo
//安防雷达：
//          报警记录    history_radarinfo
//静电接地：
//          报警记录    history_jingdianinfo
//可燃气体：
//          报警记录    fgainfo
//油气回收：
//          环境监测数据  envinfo
//          油枪检测数据  reoilgasinfo
//          油气回收报警  reoilgaswarn
//          可燃气体     fgainfo
//          液位仪       history_liquid
//          潜油泵       history_pump
//          防撞柱       history_crash
//          加油机详情报警 gunwarn_details
/***************************/

//三个弹窗提醒计数器
unsigned char count_lowspeed_oil = 0;
unsigned char count_lowspeed_gas = 0;
unsigned char count_wrong_gas = 0;
unsigned char Dispener_Troubleshooting[96][2] = {0};
//创建表
void creat_table()
{
    QSqlQuery qry;

    //创建泄漏检测    表
    qry.prepare("CREATE TABLE IF NOT EXISTS controlinfo (id INTEGER PRIMARY KEY AUTOINCREMENT,time VARCHAR(30),whichone VARCHAR(30),state VARCHAR(30)) ");
    qry.exec();
    qry.prepare("CREATE TABLE IF NOT EXISTS operateinfo (id INTEGER PRIMARY KEY AUTOINCREMENT,time VARCHAR(30),whichone VARCHAR(30),operate VARCHAR(30)) ");
    qry.exec();
    qry.prepare("CREATE TABLE IF NOT EXISTS netinfo (id INTEGER PRIMARY KEY AUTOINCREMENT,time VARCHAR(30),state VARCHAR(30)) ");
    qry.exec();
    //创建安防雷达    表
    qry.prepare("CREATE TABLE IF NOT EXISTS history_radarinfo (id INTEGER PRIMARY KEY AUTOINCREMENT,time VARCHAR(30),whichandstate VARCHAR(30),location VARCHAR(30)) ");
    qry.exec();
    //创建静电接地    表
    qry.prepare("CREATE TABLE IF NOT EXISTS history_jingdianinfo (id INTEGER PRIMARY KEY AUTOINCREMENT,time VARCHAR(30),state VARCHAR(30)) ");
    qry.exec();
    //创建IIE    表
    qry.prepare("CREATE TABLE IF NOT EXISTS history_IIE (id INTEGER PRIMARY KEY AUTOINCREMENT,time VARCHAR(30),state VARCHAR(30)) ");
    qry.exec();
    //创建油气回收    气液比表
    qry.prepare("CREATE TABLE IF NOT EXISTS reoilgasinfo (id INTEGER PRIMARY KEY AUTOINCREMENT,whichone VARCHAR(30),whichnum VARCHAR(30),time VARCHAR(30),howlong VARCHAR(30),al VARCHAR(30),countofgas VARCHAR(30),countofoil VARCHAR(30),speedofgas VARCHAR(30),speedofoil VARCHAR(30),flag15l VARCHAR(2),flagstate VARCHAR(2)) ");
    qry.exec();
    //创建环境检测    表
    qry.prepare("CREATE TABLE IF NOT EXISTS envinfo (id INTEGER PRIMARY KEY AUTOINCREMENT,time VARCHAR(30),tankpre VARCHAR(30),pipepre VARCHAR(30),temperature VARCHAR(30),nongdu VARCHAR(30))");
    qry.exec();
    //创建可燃气体    表
    qry.prepare("CREATE TABLE IF NOT EXISTS fgainfo (id INTEGER PRIMARY KEY AUTOINCREMENT,time VARCHAR(30),whichone VARCHAR(30),state VARCHAR(30))");
    qry.exec();
    //油气回收报警记录 时间、编号、状态
    qry.prepare("CREATE TABLE IF NOT EXISTS reoilgaswarn (id INTEGER PRIMARY KEY AUTOINCREMENT,time VARCHAR(30),whichone VARCHAR(30),state VARCHAR(30))");
    qry.exec();
    //创建液位仪    表
    qry.prepare("CREATE TABLE IF NOT EXISTS history_liquid (id INTEGER PRIMARY KEY AUTOINCREMENT,time VARCHAR(30),state VARCHAR(30)) ");
    qry.exec();
    //创建潜油泵    表
    qry.prepare("CREATE TABLE IF NOT EXISTS history_pump (id INTEGER PRIMARY KEY AUTOINCREMENT,time VARCHAR(30),state VARCHAR(30)) ");
    qry.exec();
    //创建防撞柱    表
    qry.prepare("CREATE TABLE IF NOT EXISTS history_crash (id INTEGER PRIMARY KEY AUTOINCREMENT,time VARCHAR(30),state VARCHAR(30)) ");
    qry.exec();
    //加油枪报警详情    表
    qry.prepare("CREATE TABLE IF NOT EXISTS gunwarn_details (id INTEGER PRIMARY KEY AUTOINCREMENT,time VARCHAR(30),gunid VARCHAR(30),gunnum VARCHAR(30),state VARCHAR(30)) ");
    qry.exec();
}

//数据库写入
//表：    reoilgasinfo
void add_value_reoilgas(int whichone_i,int whichgun_j, int howlong, int countofgas, int countofoil)
{
    QSqlQuery qry;
    QDateTime current_datetime = QDateTime::currentDateTime();
    QString current_datetime_qstr = current_datetime.toString("yyyy-MM-dd  hh:mm:ss");
    if(Flag_Reoilgas_Version == 2)//第二版气液比采集器 时间是气液比采集器上传来的
    {
        QString year,mon,day,hour,min,sec;
        year = Time_Reoilgas.mid(0,4);
        mon = Time_Reoilgas.mid(4,2);
        day = Time_Reoilgas.mid(6,2);
        hour = Time_Reoilgas.mid(8,2);
        min = Time_Reoilgas.mid(10,2);
        sec = Time_Reoilgas.mid(12,2);
        current_datetime_qstr = year+"-"+mon+"-"+day+"  "+hour+":"+min+":"+sec;
    }
    float al = 0;
    float countofgas_e = 0;
    float countofoil_e = 0;
    float speedofgas = 0;
    float speedofoil = 0;
    float time_m = howlong/60.0;
    unsigned char flag_15L = 0;     //记录是否超过15L
    unsigned char flag_state = 0;   //记录是否超出气液比要求的范围  0 符合  1不符合气液比  2燃油流速过慢  3回气量过慢,真空泵故障 4回气量过慢 5不足15L
    countofgas_e = Gas_Factor[whichone_i][whichgun_j]*countofgas/1000.0;
    QString countofgas_str = QString::number(countofgas_e,'f',2);
    countofoil_e = Fueling_Factor[whichone_i][whichgun_j]*countofoil/1000.0;
    QString countofoil_str = QString::number(countofoil_e,'f',2);
    if(countofoil_e >= 15) //测试的时候改成1L
    {
        flag_15L = 1;
        al = (countofgas_e)/(countofoil_e);
        speedofgas = countofgas_e/time_m;
        speedofoil = countofoil_e/time_m;
        QString al_str = QString::number(al, 'f', 2);
        QString speedofgas_str = QString::number(speedofgas, 'f', 2);
        QString speedofoil_str = QString::number(speedofoil, 'f', 2);

        if(al > NormalAL_High || al < NormalAL_Low)//气液比超标
        {
            flag_state = 1;//初始置1

            if(speedofoil < 10)//燃油流速过慢不记录到有效数据中
            {
                flag_state = 2;//加油机流速慢
				//下面的有关加油机详情的故障判断仅仅是在 0  3 4 5 之间跳变，通信故障优先级最高，然后是报警，预警，最后才是他们
                if(Dispener_Troubleshooting[whichone_i*8+whichgun_j][1] == 2)
                {
                    Dispener_Troubleshooting[whichone_i*8+whichgun_j][0]++;
                }
                else
                {
                    Dispener_Troubleshooting[whichone_i*8+whichgun_j][0] = 0;
                    Dispener_Troubleshooting[whichone_i*8+whichgun_j][1] = 2;
                }
				if(Dispener_Troubleshooting[whichone_i*8+whichgun_j][0] >= 5)//三次报警，连续报警只报一次
                {
                    Flag_Show_ReoilgasPop = 3;//弹窗 加油机诊断类
                    if(Dispener_Troubleshooting[whichone_i*8+whichgun_j][0]==3)
                    {
						//只有正常情况或者其他两种故障诊断状态下才能进行跳转
						if((ReoilgasPop_GunSta[whichone_i*8+whichgun_j] == 0)||(ReoilgasPop_GunSta[whichone_i*8+whichgun_j] == 3)||(ReoilgasPop_GunSta[whichone_i*8+whichgun_j] == 4))
						{
							ReoilgasPop_GunSta[whichone_i*8+whichgun_j] = 5;
							add_value_gunwarn_details(QString::number(whichone_i*8+whichgun_j),GUN_LOW_OILSPEED);//加油枪详情
						} //只有正常的时候才做判断

                    }
					Dispener_Troubleshooting[whichone_i*8+whichgun_j][0] = 6;
                }
            }
            else if((speedofgas < 1))//油气过慢只有1升
            {
                flag_state = 3;//真空泵故障
                if(Dispener_Troubleshooting[whichone_i*8+whichgun_j][1] == 3)
                {
                    Dispener_Troubleshooting[whichone_i*8+whichgun_j][0]++;
                }
                else
                {
                    Dispener_Troubleshooting[whichone_i*8+whichgun_j][0] = 0;
                    Dispener_Troubleshooting[whichone_i*8+whichgun_j][1] = 3;
                }
				if(Dispener_Troubleshooting[whichone_i*8+whichgun_j][0] >= 5)//三次报警，连续报警只报一次
                {
                    Flag_Show_ReoilgasPop = 3;//弹窗 加油机诊断类
                    if(Dispener_Troubleshooting[whichone_i*8+whichgun_j][0]==3)
                    {
						//只有正常情况或者其他两种故障诊断状态下才能进行跳转
						if((ReoilgasPop_GunSta[whichone_i*8+whichgun_j] == 0)||(ReoilgasPop_GunSta[whichone_i*8+whichgun_j] == 5)||(ReoilgasPop_GunSta[whichone_i*8+whichgun_j] == 4))
						{
							ReoilgasPop_GunSta[whichone_i*8+whichgun_j] = 3;
							add_value_gunwarn_details(QString::number(whichone_i*8+whichgun_j),GUN_GASPUMP_WRONG);//加油枪详情
						}

                    }
					Dispener_Troubleshooting[whichone_i*8+whichgun_j][0] = 6;
                }
            }
            else if((speedofgas >= 1)&&(speedofgas < 10))//油气过慢只有1升
            {
                flag_state = 4;//回气量过慢，气路诊断
                if(Dispener_Troubleshooting[whichone_i*8+whichgun_j][1] == 4)
                {
                    Dispener_Troubleshooting[whichone_i*8+whichgun_j][0]++;
                }
                else
                {
                    Dispener_Troubleshooting[whichone_i*8+whichgun_j][0] = 0;
                    Dispener_Troubleshooting[whichone_i*8+whichgun_j][1] = 4;
                }
				if(Dispener_Troubleshooting[whichone_i*8+whichgun_j][0] >= 5)//三次报警，连续报警只报一次
                {
                    Flag_Show_ReoilgasPop = 3;//弹窗 加油机诊断类
                    if(Dispener_Troubleshooting[whichone_i*8+whichgun_j][0]==3)
                    {
						//只有正常情况或者其他两种故障诊断状态下才能进行跳转
						if((ReoilgasPop_GunSta[whichone_i*8+whichgun_j] == 0)||(ReoilgasPop_GunSta[whichone_i*8+whichgun_j] == 3)||(ReoilgasPop_GunSta[whichone_i*8+whichgun_j] == 5))
						{
							ReoilgasPop_GunSta[whichone_i*8+whichgun_j] = 4;
							add_value_gunwarn_details(QString::number(whichone_i*8+whichgun_j),GUN_LOW_GASSPEED);//加油枪详情
						}

                    }
					Dispener_Troubleshooting[whichone_i*8+whichgun_j][0] = 6;
                }
            }
			else
			{

			}
            qDebug()<<"gungungun"<<Dispener_Troubleshooting[whichone_i*8+whichgun_j][0]<<Dispener_Troubleshooting[whichone_i*8+whichgun_j][1];

        }
        else //气液比合格
        {
			if(Dispener_Troubleshooting[whichone_i*8+whichgun_j][1] != 0)
			{
				Dispener_Troubleshooting[whichone_i*8+whichgun_j][0] = 0;
				Dispener_Troubleshooting[whichone_i*8+whichgun_j][1] = 0;
			}
			else
			{
				Dispener_Troubleshooting[whichone_i*8+whichgun_j][0]++;
			}
			if(Dispener_Troubleshooting[whichone_i*8+whichgun_j][0] >= 3)
			{
				if(Dispener_Troubleshooting[whichone_i*8+whichgun_j][0] == 3)
				{
					if((ReoilgasPop_GunSta[whichone_i*8+whichgun_j] == 3)||(ReoilgasPop_GunSta[whichone_i*8+whichgun_j] == 4)||(ReoilgasPop_GunSta[whichone_i*8+whichgun_j] == 5))
					{
						ReoilgasPop_GunSta[whichone_i*8+whichgun_j] = 0;
						add_value_gunwarn_details(QString::number(whichone_i*8+whichgun_j),GUN_NORMAL);//加油枪详情
					}
				}
				Dispener_Troubleshooting[whichone_i*8+whichgun_j][0] = 4;
			}


            //获取油气流速，用来判断液阻压力是否超标
            if(whichone_i == (Far_Dispener-1))
            {
                if(speedofgas>Speed_fargas)//选取最大值
                {
                    Speed_fargas = speedofgas;
                    config_Liquid_resistance();
                }
            }
            if(Far_Dispener == 0)
            {
                if(Speed_fargas != 0)
                {
                    Speed_fargas = 0;
                    config_Liquid_resistance();
                }
            }
        }

		qry.prepare((QString("INSERT INTO reoilgasinfo (id,whichone,whichnum,time,howlong,al,countofgas,countofoil,speedofgas,speedofoil,flag15l,flagstate) values (NULL,'%1','%2','%3','%4','%5','%6','%7','%8','%9','%10','%11')").arg(QString("%1-%2").arg(whichone_i+1).arg(whichgun_j+1)).arg(Mapping_Show[whichone_i*8+whichgun_j]).arg(current_datetime_qstr).arg(QString("%1").arg(howlong)).arg(al_str).arg(countofgas_str).arg(countofoil_str).arg(speedofgas_str).arg(speedofoil_str).arg(QString("%1").arg(flag_15L)).arg(QString("%1").arg(flag_state))));
        if(!qry.exec())
        {
            qDebug()<<qry.lastError();
        }
        else
        {
            qDebug("Reoilgas Data Inserted!%d",qry.lastInsertId().toInt());
        }
        if((count_lowspeed_gas>=3)&&(count_lowspeed_oil>=3)&&(count_wrong_gas>=3))
        {
            count_lowspeed_gas = 0;
            count_lowspeed_oil = 0;
            count_wrong_gas = 0;
            Flag_Show_ReoilgasPop = 3;//弹窗 加油机诊断类
        }
    }
    else if((countofoil_e < 15)&&(countofoil_e >= 5))//不足15L仍然记录但不作为有效数据作为统计
    {
        flag_15L = 0;

        al = (countofgas_e)/(countofoil_e);

        QString al_str = QString::number(al, 'f', 2);
        flag_state = 5;
        speedofgas = countofgas_e/time_m;
        QString speedofgas_str = QString::number(speedofgas, 'f', 2);
        speedofoil = countofoil_e/time_m;
        QString speedofoil_str = QString::number(speedofoil, 'f', 2);
		qry.prepare((QString("INSERT INTO reoilgasinfo (id,whichone,whichnum,time,howlong,al,countofgas,countofoil,speedofgas,speedofoil,flag15l,flagstate) values (NULL,'%1','%2','%3','%4','%5','%6','%7','%8','%9','%10','%11')").arg(QString("%1-%2").arg(whichone_i+1).arg(whichgun_j+1)).arg(Mapping_Show[whichone_i*8+whichgun_j]).arg(current_datetime_qstr).arg(QString("%1").arg(howlong)).arg(al_str).arg(countofgas_str).arg(countofoil_str).arg(speedofgas_str).arg(speedofoil_str).arg(QString("%1").arg(flag_15L)).arg(QString("%1").arg(flag_state))));
        if(!qry.exec())
        {
            qDebug()<<qry.lastError();
        }
        else
        {
            qDebug("Reoilgas Data Inserted!%d",qry.lastInsertId().toInt());
        }
    }
}
//表：    controlin泄漏报警
void add_value_controlinfo(QString whichone, QString state)
{
    QString num_warn;
    QString type;
    QSqlQuery qry;
    QDateTime current_datetime = QDateTime::currentDateTime();
    QString current_datetime_qstr = current_datetime.toString("yyyy-MM-dd  hh:mm:ss");
//    QString str_input = "INSERT INTO controlinfo (id,time,whichone,state) values (";
//    str_input.append(QString("%1").arg(Count_ControlInfo)).append(",'").append(current_datetime_qstr).append("','").append(whichone).append("','").append(state).append("')");
    qry.prepare(QString("INSERT INTO controlinfo (id,time,whichone,state) values (NULL,'%1','%2','%3')").arg(current_datetime_qstr).arg(whichone).arg(state));
//    qry.prepare(str_input);
    if(!qry.exec())
    {
        qDebug()<<qry.lastError();
    }
    else
    {
        qDebug("Data Inserted!\n");
    }  
}
//表：   operateinfo操作日志
void add_value_operateinfo(QString whichone, QString operate)
{
    QSqlQuery qry;
    QDateTime current_datetime = QDateTime::currentDateTime();
    QString current_datetime_qstr = current_datetime.toString("yyyy-MM-dd  hh:mm:ss");
    qry.prepare(QString("INSERT INTO operateinfo (id,time,whichone,operate) values (NULL,'%1','%2','%3')").arg(current_datetime_qstr).arg(whichone).arg(operate));
    if(!qry.exec())
    {
        qDebug()<<qry.lastError();
    }
    else
    {
        qDebug("Data Inserted!");
    }
}
//表：    netinfo通信日志
void add_value_netinfo(QString state)
{
    QSqlQuery qry;
    QDateTime current_datetime = QDateTime::currentDateTime();
    QString current_datetime_qstr = current_datetime.toString("yyyy-MM-dd  hh:mm:ss");
    qry.prepare(QString("INSERT INTO netinfo (id,time,state) values (NULL,'%1','%2')").arg(current_datetime_qstr).arg(state));
    if(!qry.exec())
    {
        qDebug()<<qry.lastError();
    }
    else
    {
        qDebug("Data Inserted!");
    }
}
//表：   history_radarinfo雷达记录
void add_value_radarinfo(QString whichandstate, QString location)
{
    QSqlQuery qry;
    QDateTime current_datetime = QDateTime::currentDateTime();
    QString current_datetime_qstr = current_datetime.toString("yyyy-MM-dd  hh:mm:ss");
    qry.prepare(QString("INSERT INTO history_radarinfo (id,time,whichandstate,location) values (NULL,'%1','%2','%3')").arg(current_datetime_qstr).arg(whichandstate).arg(location));
    if(!qry.exec())
    {
        qDebug()<<qry.lastError();
    }
    else
    {
        qDebug("Data Inserted!");
    }
}
//表：    history_jingdianinfo静电报警
void add_value_jingdianinfo(QString state)
{
    QSqlQuery qry;
    QDateTime current_datetime = QDateTime::currentDateTime();
    QString current_datetime_qstr = current_datetime.toString("yyyy-MM-dd  hh:mm:ss");
    qry.prepare(QString("INSERT INTO history_jingdianinfo (id,time,state) values (NULL,'%1','%2')").arg(current_datetime_qstr).arg(state));
    if(!qry.exec())
    {
        qDebug()<<qry.lastError();
    }
    else
    {
        qDebug("Data Inserted!");
    }
}
//表：    history_IIE IIE报警
void add_value_IIE(QString state)
{
    QSqlQuery qry;
    QDateTime current_datetime = QDateTime::currentDateTime();
    QString current_datetime_qstr = current_datetime.toString("yyyy-MM-dd  hh:mm:ss");
    qry.prepare(QString("INSERT INTO history_IIE (id,time,state) values (NULL,'%1','%2')").arg(current_datetime_qstr).arg(state));
    if(!qry.exec())
    {
        qDebug()<<qry.lastError();
    }
    else
    {
        qDebug("Data Inserted!");
    }
}
//表：    evninfo环境记录
void add_value_envinfo(float tankpre, float pipepre, float temperature, unsigned char nongdu)
{
    QSqlQuery qry;
    QDateTime current_datetime = QDateTime::currentDateTime();
    QString current_datetime_qstr = current_datetime.toString("yyyy-MM-dd  hh:mm:ss");
    QString tankpre_qstr = QString::number(tankpre,'f',2);
    QString pipepre_qstr = QString::number(pipepre,'f',2);
    QString tempera_qstr = QString::number(temperature,'f',2);
    qry.prepare(QString("INSERT INTO envinfo (id,time,tankpre,pipepre,temperature,nongdu) values (NULL,'%1','%2','%3','%4','%5')").arg(current_datetime_qstr).arg(tankpre_qstr).arg(pipepre_qstr).arg(tempera_qstr).arg(nongdu/2));
    if(!qry.exec())
    {
        qDebug()<<qry.lastError();
    }
    else
    {
        qDebug("Data Inserted!%d",qry.lastInsertId().toInt());
    }
}
//表：    fgainfo浓度记录
void add_value_fgainfo(QString whichone, QString state)
{
    QSqlQuery qry;
    QDateTime current_datetime = QDateTime::currentDateTime();
    QString current_datetime_qstr = current_datetime.toString("yyyy-MM-dd  hh:mm:ss");
    qry.prepare(QString("INSERT INTO fgainfo (id,time,whichone,state) values (NULL,'%1','%2','%3')").arg(current_datetime_qstr).arg(whichone).arg(state));
    if(!qry.exec())
    {
        qDebug()<<qry.lastError();
    }
    else
    {
        qDebug("FGA Data Inserted!\n");
    }
}
//表：    reoilgaswarn 在线监测报警记录
void add_value_reoilgaswarn(QString whichone, QString state)
{
    QSqlQuery qry;
    QDateTime current_datetime = QDateTime::currentDateTime();
    QString current_datetime_qstr = current_datetime.toString("yyyy-MM-dd  hh:mm:ss");
    qry.prepare(QString("INSERT INTO reoilgaswarn (id,time,whichone,state) values (NULL,'%1','%2','%3')").arg(current_datetime_qstr).arg(whichone).arg(state));
    if(!qry.exec())
    {
        qDebug()<<qry.lastError();
    }
    else
    {
        qDebug("reoilgaswarn Data Inserted!\n");
    }
}
//表：   history_liquid 液位仪报警记录
void add_value_liquid(QString state)
{
    QSqlQuery qry;
    QDateTime current_datetime = QDateTime::currentDateTime();
    QString current_datetime_qstr = current_datetime.toString("yyyy-MM-dd  hh:mm:ss");
    qry.prepare(QString("INSERT INTO history_liquid (id,time,state) values (NULL,'%1','%2')").arg(current_datetime_qstr).arg(state));
    if(!qry.exec())
    {
        qDebug()<<qry.lastError();
    }
    else
    {
        qDebug("Data Inserted!");
    }
}
//表：     history_pump 潜油泵报警记录
void add_value_pump(QString state)
{
    QSqlQuery qry;
    QDateTime current_datetime = QDateTime::currentDateTime();
    QString current_datetime_qstr = current_datetime.toString("yyyy-MM-dd  hh:mm:ss");
    qry.prepare(QString("INSERT INTO history_pump (id,time,state) values (NULL,'%1','%2')").arg(current_datetime_qstr).arg(state));
    if(!qry.exec())
    {
        qDebug()<<qry.lastError();
    }
    else
    {
        qDebug("Data Inserted!");
    }
}
//表：  history_crash  防撞柱报警记录
void add_value_crash(QString state)
{
    QSqlQuery qry;
    QDateTime current_datetime = QDateTime::currentDateTime();
    QString current_datetime_qstr = current_datetime.toString("yyyy-MM-dd  hh:mm:ss");
    qry.prepare(QString("INSERT INTO history_crash (id,time,state) values (NULL,'%1','%2')").arg(current_datetime_qstr).arg(state));
    if(!qry.exec())
    {
        qDebug()<<qry.lastError();
    }
    else
    {
        //qDebug("Data Inserted!");
    }
}
//表：  gunwarn_details 加油枪报警详情
void add_value_gunwarn_details(QString whichone,QString state)
{
    QSqlQuery qry;
    QDateTime current_datetime = QDateTime::currentDateTime();
    QString current_datetime_qstr = current_datetime.toString("yyyy-MM-dd  hh:mm:ss");
	QString gunid = "";
    int which_dis = (whichone.toInt())/8;
    int which_gun = (whichone.toInt())%8;
	if((state == GUN_UARTWRONG)||(state == GUN_NORMAL))
	{
		gunid = QString::number(which_dis+1).append("-").append(QString::number(which_gun+1)).append(".").append(QString::number(which_gun+2));
	}
	else
	{
		gunid = QString::number(which_dis+1).append("-").append(QString::number(which_gun+1));
	}
	qry.prepare(QString("INSERT INTO gunwarn_details (id,time,gunid,gunnum,state) values (NULL,'%1','%2','%3','%4')").arg(current_datetime_qstr).arg(gunid).arg(Mapping_OilNo[which_dis*8+which_gun]).arg(state));
    if(!qry.exec())
    {
        qDebug()<<qry.lastError();
    }
    else
    {
        //qDebug("Data Inserted!");
    }
}


//可以来初始化主键id  停用！ 使用主键自增
void select_table()
{
    QSqlQuery qry;
    //获取行数 并初始化controlinfo表主键
    qry.prepare("SELECT id FROM controlinfo");
    qry.exec();
    //QSqlRecord rec = qry.record();
    //int cols = rec.count(); //列数
    qry.last();     //定位到最后一行
    //Count_ControlInfo = qry.at() + 1;   //当前行数
    if(qry.at() < 0)
    {
 //       Count_ControlInfo = 1;
    }
    else
    {
 //       Count_ControlInfo = qry.at() + 2;   //需要写入的行数
    }
}

//油气回收单日数据分析
void oneday_analy(unsigned char whichdisp, unsigned char flag_time)
{
    QDateTime current_datetime = QDateTime::currentDateTime();
    //current_datetime.addDays()
    QString current_datetime_qstr = current_datetime.toString("yyyy-MM-dd");

    QSqlQuery qry;
    unsigned int count_state = 0;
    unsigned int count_all = 0;
    unsigned int count_al_big = 0;
    unsigned int count_al_small = 0;
    QSqlRecord rec;
    int cols;
    int data_sqlvalue = 0;

	//获取最大id
	qry.exec("SELECT * FROM reoilgasinfo");
	QSqlQueryModel *queryModel = new QSqlQueryModel();
	queryModel->setQuery(qry);
	while(queryModel->canFetchMore())
	{
		queryModel->fetchMore();
	}
	unsigned int max_id = queryModel->rowCount();
	qDebug()<<max_id<<"hahahahahahahahahhahahahahhhahahahahahahahahahhahaa";

	switch (flag_time)
	{
        case 0: //按键查询  当前加油机详情显示
                PerDay_AL[0] = 0;
                PerDay_AL[1] = 0;
                PerDay_AL[2] = 0;
                PerDay_AL[3] = 0;
                PerDay_AL[4] = 0;
                PerDay_AL[5] = 0;
                PerDay_AL[6] = 0;
                PerDay_AL[7] = 0;
                //1#油枪
                if(Amount_Gasgun[whichdisp-1]>=1)
                {
                    count_state = 0;
                    count_all = 0;
                    count_al_big = 0;
                    count_al_small = 0;
					qry.prepare(QString("SELECT flagstate,al FROM reoilgasinfo LIMIT 1000 OFFSET 900 WHERE whichone LIKE '%1-1%' AND time like '%2%'").arg(whichdisp).arg(current_datetime_qstr));
					//sql = "select * from TableName where "+条件+" order by "+排序+" limit "+要显示多少条记录+" offset "+跳过多少条记录;
					qry.exec();
                    rec = qry.record();
                    cols = rec.count();
					int num_jishu = 0;
                    for(int r = 0;qry.next();r++)
                    {
						num_jishu++;
                        data_sqlvalue = qry.value(0).toInt();
                        if((data_sqlvalue == 0)||(data_sqlvalue == 1) ) //满足15L且是有效数据
                        {
                            for(int c = 0;c < cols;c++)
                            {
                                qDebug()<<QString("Row %1 : %2").arg(c).arg(qry.value(c).toString())<<endl;
                            }
                            if(!data_sqlvalue)
                            {
                                count_state++;
                            }
                            if((qry.value(1).toFloat())>1.2)
                            {
                                count_al_big++;
                            }
                            if((qry.value(1).toFloat())<1.0)
                            {
                                count_al_small++;
                            }
                            count_all = r + 1;
                            PerDay_AL[0] = PerDay_AL[0] + qry.value(1).toFloat();
                        }
                    }
					qDebug()<<num_jishu<<"**********************************";
                    PerDay_AL[0] = PerDay_AL[0]/count_all;
                    PerDay_Percent[0] = (float)count_state/count_all;
                    PerDay_Al_Big[0] = (float)count_al_big/count_all;
                    PerDay_Al_Smal[0] = (float)count_al_small/count_all;
                }
                //2#油枪
                if(Amount_Gasgun[whichdisp-1]>=2)
                {
                    count_state = 0;
                    count_all = 0;
                    count_al_big = 0;
                    count_al_small = 0;
                    qry.prepare(QString("SELECT flagstate,al FROM reoilgasinfo WHERE whichone LIKE '%1-2%' AND time like '%2%'").arg(whichdisp).arg(current_datetime_qstr));
                    qry.exec();
                    rec = qry.record();
                    cols = rec.count();
                    for(int r = 0;qry.next();r++)
                    {
                        data_sqlvalue = qry.value(0).toInt();
                        if((data_sqlvalue == 0)||(data_sqlvalue == 1) ) //满足15L且是有效数据
                        {
                            for(int c = 0;c < cols;c++)
                            {
                                qDebug()<<QString("Row %1 : %2").arg(c).arg(qry.value(c).toString())<<endl;
                            }
                            if(!data_sqlvalue)
                            {
                                count_state++;
                            }
                            if((qry.value(1).toFloat())>1.2)
                            {
                                count_al_big++;
                            }
                            if((qry.value(1).toFloat())<1.0)
                            {
                                count_al_small++;
                            }
                            count_all = r + 1;
                            PerDay_AL[1] = PerDay_AL[1] + qry.value(1).toFloat();
                        }
                    }
                    PerDay_AL[1] = PerDay_AL[1]/count_all;
                    PerDay_Percent[1] = (float)count_state/count_all;
                    PerDay_Al_Big[1] = (float)count_al_big/count_all;
                    PerDay_Al_Smal[1] = (float)count_al_small/count_all;
                }

                //3#油枪
                if(Amount_Gasgun[whichdisp-1]>=3)
                {
                    count_state = 0;
                    count_all = 0;
                    count_al_big = 0;
                    count_al_small = 0;
                    qry.prepare(QString("SELECT flagstate,al FROM reoilgasinfo WHERE whichone LIKE '%1-3%' AND time like '%2%'").arg(whichdisp).arg(current_datetime_qstr));
                    qry.exec();
                    rec = qry.record();
                    cols = rec.count();
                    for(int r = 0;qry.next();r++)
                    {
                        data_sqlvalue = qry.value(0).toInt();
                        if((data_sqlvalue == 0)||(data_sqlvalue == 1) ) //满足15L且是有效数据
                        {
                            for(int c = 0;c < cols;c++)
                            {
                                qDebug()<<QString("Row %1 : %2").arg(c).arg(qry.value(c).toString())<<endl;
                            }
                            if(!data_sqlvalue)
                            {
                                count_state++;
                            }
                            if((qry.value(1).toFloat())>1.2)
                            {
                                count_al_big++;
                            }
                            if((qry.value(1).toFloat())<1.0)
                            {
                                count_al_small++;
                            }
                            count_all = r + 1;
                            PerDay_AL[2] = PerDay_AL[2] + qry.value(1).toFloat();
                        }
                    }
                    PerDay_AL[2] = PerDay_AL[2]/count_all;
                    PerDay_Percent[2] = (float)count_state/count_all;
                    PerDay_Al_Big[2] = (float)count_al_big/count_all;
                    PerDay_Al_Smal[2] = (float)count_al_small/count_all;
                }

                //4#油枪
                if(Amount_Gasgun[whichdisp-1]>=4)
                {
                    count_state = 0;
                    count_all = 0;
                    count_al_big = 0;
                    count_al_small = 0;
                    qry.prepare(QString("SELECT flagstate,al FROM reoilgasinfo WHERE whichone LIKE '%1-4%' AND time like '%2%'").arg(whichdisp).arg(current_datetime_qstr));
                    qry.exec();
                    rec = qry.record();
                    cols = rec.count();
                    for(int r = 0;qry.next();r++)
                    {
                        data_sqlvalue = qry.value(0).toInt();
                        if((data_sqlvalue == 0)||(data_sqlvalue == 1) ) //满足15L且是有效数据
                        {
                            for(int c = 0;c < cols;c++)
                            {
                                qDebug()<<QString("Row %1 : %2").arg(c).arg(qry.value(c).toString())<<endl;
                            }
                            if(!data_sqlvalue)
                            {
                                count_state++;
                            }
                            if((qry.value(1).toFloat())>1.2)
                            {
                                count_al_big++;
                            }
                            if((qry.value(1).toFloat())<1.0)
                            {
                                count_al_small++;
                            }
                            count_all = r + 1;
                            PerDay_AL[3] = PerDay_AL[3] + qry.value(1).toFloat();
                        }
                    }
                    PerDay_AL[3] = PerDay_AL[3]/count_all;
                    PerDay_Percent[3] = (float)count_state/count_all;
                    PerDay_Al_Big[3] = (float)count_al_big/count_all;
                    PerDay_Al_Smal[3] = (float)count_al_small/count_all;
                }

                //5#油枪
                if(Amount_Gasgun[whichdisp-1]>=5)
                {
                    count_state = 0;
                    count_all = 0;
                    count_al_big = 0;
                    count_al_small = 0;
                    qry.prepare(QString("SELECT flagstate,al FROM reoilgasinfo WHERE whichone LIKE '%1-5%' AND time like '%2%'").arg(whichdisp).arg(current_datetime_qstr));
                    qry.exec();
                    rec = qry.record();
                    cols = rec.count();
                    for(int r = 0;qry.next();r++)
                    {
                        data_sqlvalue = qry.value(0).toInt();
                        if((data_sqlvalue == 0)||(data_sqlvalue == 1) ) //满足15L且是有效数据
                        {
                            for(int c = 0;c < cols;c++)
                            {
                                qDebug()<<QString("Row %1 : %2").arg(c).arg(qry.value(c).toString())<<endl;
                            }
                            if(!data_sqlvalue)
                            {
                                count_state++;
                            }
                            if((qry.value(1).toFloat())>1.2)
                            {
                                count_al_big++;
                            }
                            if((qry.value(1).toFloat())<1.0)
                            {
                                count_al_small++;
                            }
                            count_all = r + 1;
                            PerDay_AL[4] = PerDay_AL[4] + qry.value(1).toFloat();
                        }
                    }
                    PerDay_AL[4] = PerDay_AL[4]/count_all;
                    PerDay_Percent[4] = (float)count_state/count_all;
                    PerDay_Al_Big[4] = (float)count_al_big/count_all;
                    PerDay_Al_Smal[4] = (float)count_al_small/count_all;
                }

                //6#油枪
                if(Amount_Gasgun[whichdisp-1]>=6)
                {
                    count_state = 0;
                    count_all = 0;
                    count_al_big = 0;
                    count_al_small = 0;
                    qry.prepare(QString("SELECT flagstate,al FROM reoilgasinfo WHERE whichone LIKE '%1-6%' AND time like '%2%'").arg(whichdisp).arg(current_datetime_qstr));
                    qry.exec();
                    rec = qry.record();
                    cols = rec.count();
                    for(int r = 0;qry.next();r++)
                    {
                        data_sqlvalue = qry.value(0).toInt();
                        if((data_sqlvalue == 0)||(data_sqlvalue == 1) ) //满足15L且是有效数据
                        {
                            for(int c = 0;c < cols;c++)
                            {
                                qDebug()<<QString("Row %1 : %2").arg(c).arg(qry.value(c).toString())<<endl;
                            }
                            if(!data_sqlvalue)
                            {
                                count_state++;
                            }
                            if((qry.value(1).toFloat())>1.2)
                            {
                                count_al_big++;
                            }
                            if((qry.value(1).toFloat())<1.0)
                            {
                                count_al_small++;
                            }
                            count_all = r + 1;
                            PerDay_AL[5] = PerDay_AL[5] + qry.value(1).toFloat();
                        }
                    }
                    PerDay_AL[5] = PerDay_AL[5]/count_all;
                    PerDay_Percent[5] = (float)count_state/count_all;
                    PerDay_Al_Big[5] = (float)count_al_big/count_all;
                    PerDay_Al_Smal[5] = (float)count_al_small/count_all;
                }

                //7#油枪
                if(Amount_Gasgun[whichdisp-1]>=7)
                {
                    count_state = 0;
                    count_all = 0;
                    count_al_big = 0;
                    count_al_small = 0;
                    qry.prepare(QString("SELECT flagstate,al FROM reoilgasinfo WHERE whichone LIKE '%1-7%' AND time like '%2%'").arg(whichdisp).arg(current_datetime_qstr));
                    qry.exec();
                    rec = qry.record();
                    cols = rec.count();
                    for(int r = 0;qry.next();r++)
                    {
                        data_sqlvalue = qry.value(0).toInt();
                        if((data_sqlvalue == 0)||(data_sqlvalue == 1) ) //满足15L且是有效数据
                        {
                            for(int c = 0;c < cols;c++)
                            {
                                qDebug()<<QString("Row %1 : %2").arg(c).arg(qry.value(c).toString())<<endl;
                            }
                            if(!data_sqlvalue)
                            {
                                count_state++;
                            }
                            if((qry.value(1).toFloat())>1.2)
                            {
                                count_al_big++;
                            }
                            if((qry.value(1).toFloat())<1.0)
                            {
                                count_al_small++;
                            }
                            count_all = r + 1;
                            PerDay_AL[6] = PerDay_AL[6] + qry.value(1).toFloat();
                        }
                    }
                    PerDay_AL[6] = PerDay_AL[6]/count_all;
                    PerDay_Percent[6] = (float)count_state/count_all;
                    PerDay_Al_Big[6] = (float)count_al_big/count_all;
                    PerDay_Al_Smal[6] = (float)count_al_small/count_all;
                }
                //8#油枪
                if(Amount_Gasgun[whichdisp-1]>=8)
                {
                    count_state = 0;
                    count_all = 0;
                    count_al_big = 0;
                    count_al_small = 0;
                    qry.prepare(QString("SELECT flagstate,al FROM reoilgasinfo WHERE whichone LIKE '%1-8%' AND time like '%2%'").arg(whichdisp).arg(current_datetime_qstr));
                    qry.exec();
                    rec = qry.record();
                    cols = rec.count();
                    for(int r = 0;qry.next();r++)
                    {
                        data_sqlvalue = qry.value(0).toInt();
                        if((data_sqlvalue == 0)||(data_sqlvalue == 1) ) //满足15L且是有效数据
                        {
                            for(int c = 0;c < cols;c++)
                            {
                                qDebug()<<QString("Row %1 : %2").arg(c).arg(qry.value(c).toString())<<endl;
                            }
                            if(!data_sqlvalue)
                            {
                                count_state++;
                            }
                            if((qry.value(1).toFloat())>1.2)
                            {
                                count_al_big++;
                            }
                            if((qry.value(1).toFloat())<1.0)
                            {
                                count_al_small++;
                            }
                            count_all = r + 1;
                            PerDay_AL[7] = PerDay_AL[7] + qry.value(1).toFloat();
                        }
                    }
                    PerDay_AL[7] = PerDay_AL[7]/count_all;
                    PerDay_Percent[7] = (float)count_state/count_all;
                    PerDay_Al_Big[7] = (float)count_al_big/count_all;
                    PerDay_Al_Smal[7] = (float)count_al_small/count_all;
                }
                break;
        case 1: //零点信息查询，更新连续报警天数
        //qDebug() << "once oneday";
                for(unsigned char i = 1;i < Amount_Dispener+1;i++)
                {

                    //1#油枪
                    if(Amount_Gasgun[i-1]>=1)
                    {
                        count_state = 0;
                        count_all = 0;
                        qry.prepare(QString("SELECT flagstate FROM reoilgasinfo WHERE whichone LIKE '%1-1%' AND time like '%2%'").arg(i).arg(current_datetime_qstr));
                        qry.exec();
                        rec = qry.record();
                        cols = rec.count();
                        for(int r = 0;qry.next();r++)
                        {
                            if((qry.value(0).toInt() == 0)||(qry.value(0).toInt() == 1)) //有效数据  大于15L且不是流速过低数据
                            {
                                for(int c = 0;c < cols;c++)
                                {
                                    qDebug()<<QString("Row %1 : %2").arg(c).arg(qry.value(c).toString())<<endl;
                                }
                                if(!(qry.value(0).toInt()))
                                {
                                    count_state++;
                                }
                                count_all++;
                            }
                        }
                        if((count_all < 5)||(flag_morethan5[(i-1)*8+0] > 0))
                        {
                            if(flag_morethan5[(i-1)*8+0] > 0)
                            {
                                QDateTime current_datetime_pre = current_datetime.addDays(-(flag_morethan5[(i-1)*8+0]));
                                QDateTime current_datetime_later = current_datetime;
                                QString current_datetime_qstr_pre = current_datetime_pre.toString("yyyy-MM-dd");
                                QString current_datetime_qstr_later = current_datetime_later.toString("yyyy-MM-dd  23:59:59");
                                qDebug() << i<<"-1"<<current_datetime_qstr_pre <<"  to  " << current_datetime_qstr_later;
                                qry.prepare(QString("SELECT flagstate FROM reoilgasinfo WHERE whichone LIKE '%1-1%' AND time between '%2' and '%3'").arg(i).arg(current_datetime_qstr_pre).arg(current_datetime_qstr_later));
                                qry.exec();
                                rec = qry.record();
                                cols = rec.count();
                                count_all = 0;
                                count_state = 0;
                                for(int r = 0;qry.next();r++)
                                {
                                    if((qry.value(0).toInt() == 0)||(qry.value(0).toInt() == 1))//有效数据  大于15L且不是流速过低数据
                                    {
                                        for(int c = 0;c < cols;c++)
                                        {
                                            qDebug()<<QString("Row %1 : %2").arg(c).arg(qry.value(c).toString())<<endl;
                                        }
                                        if(!(qry.value(0).toInt()))
                                        {
                                            count_state++;
                                        }
                                        count_all++;
                                    }
                                }
                                if(count_all > 5) //如果大于5比数据了，正常处理
                                {
                                    PerDay_Percent[0] = (float)count_state/count_all;
                                    if(PerDay_Percent[0] < 0.75)
                                    {
                                        Flag_Accumto[i-1][0]++;
                                    }
                                    else
                                    {
                                        Flag_Accumto[i-1][0] = 0;
                                    }
                                    flag_morethan5[(i-1)*8+0] = 0;
                                }
                                else //如果少于5比数据
                                {
                                    flag_morethan5[(i-1)*8+0]++;
                                }
                            }
                            else
                            {
                                flag_morethan5[(i-1)*8+0]++;
                            }
                            if(flag_morethan5[(i-1)*8+0]>100)//连续100天没有加油则清零
                            {
                                flag_morethan5[(i-1)*8+0] = 0;
                            }
                        }
                        else
                        {
                            flag_morethan5[(i-1)*8+0] = 0;
                            PerDay_Percent[0] = (float)count_state/count_all;
                            if(PerDay_Percent[0] < 0.75)
                            {
                                Flag_Accumto[i-1][0]++;
                            }
                            else
                            {
                                Flag_Accumto[i-1][0] = 0;
                            }
                        }
                    }

                    //2#油枪
                    if(Amount_Gasgun[i-1]>=2)
                    {
                        count_state = 0;
                        count_all = 0;
                        qry.prepare(QString("SELECT flagstate FROM reoilgasinfo WHERE whichone LIKE '%1-2%' AND time like '%2%'").arg(i).arg(current_datetime_qstr));
                        qry.exec();
                        rec = qry.record();
                        cols = rec.count();
                        for(int r = 0;qry.next();r++)
                        {
                            if((qry.value(0).toInt() == 0)||(qry.value(0).toInt() == 1))//有效数据  大于15L且不是流速过低数据
                            {
                                for(int c = 0;c < cols;c++)
                                {
                                    qDebug()<<QString("Row %1 : %2").arg(c).arg(qry.value(c).toString())<<endl;
                                }
                                if(!(qry.value(0).toInt()))
                                {
                                    count_state++;
                                }
                                count_all++;
                            }
                        }
                        if((count_all < 5)||(flag_morethan5[(i-1)*8+1] > 0))//如果不足五次或者之前有数据
                        {
                            if(flag_morethan5[(i-1)*8+1] > 0)
                            {
                                QDateTime current_datetime_pre = current_datetime.addDays(-(flag_morethan5[(i-1)*8+1]));
                                QDateTime current_datetime_later = current_datetime;
                                QString current_datetime_qstr_pre = current_datetime_pre.toString("yyyy-MM-dd");
                                QString current_datetime_qstr_later = current_datetime_later.toString("yyyy-MM-dd  23:59:59");
                                qDebug() << i<<"-1"<<current_datetime_qstr_pre <<"  to  " << current_datetime_qstr_later;
                                qry.prepare(QString("SELECT flagstate FROM reoilgasinfo WHERE whichone LIKE '%1-1%' AND time between '%2' and '%3'").arg(i).arg(current_datetime_qstr_pre).arg(current_datetime_qstr_later));
                                qry.exec();
                                rec = qry.record();
                                cols = rec.count();
                                count_all = 0;
                                count_state = 0;
                                for(int r = 0;qry.next();r++)
                                {
                                    if((qry.value(0).toInt() == 0)||(qry.value(0).toInt() == 1))//有效数据  大于15L且不是流速过低数据
                                    {
                                        for(int c = 0;c < cols;c++)
                                        {
                                            qDebug()<<QString("Row %1 : %2").arg(c).arg(qry.value(c).toString())<<endl;
                                        }
                                        if(!(qry.value(0).toInt()))
                                        {
                                            count_state++;
                                        }
                                        count_all++;
                                    }
                                }
                                if(count_all > 5) //如果大于5比数据了，正常处理
                                {
                                    PerDay_Percent[1] = (float)count_state/count_all;
                                    if(PerDay_Percent[1] < 0.75)
                                    {
                                        Flag_Accumto[i-1][1]++;
                                    }
                                    else
                                    {
                                        Flag_Accumto[i-1][1] = 0;
                                    }
                                    flag_morethan5[(i-1)*8+1] = 0;
                                }
                                else //如果少于5比数据
                                {
                                    flag_morethan5[(i-1)*8+1]++;
                                }
                            }
                            else
                            {
                                flag_morethan5[(i-1)*8+1]++;
                            }
                            if(flag_morethan5[(i-1)*8+1]>100)//连续100天没有加油则清零
                            {
                                flag_morethan5[(i-1)*8+1] = 0;
                            }
                        }
                        else
                        {
                            flag_morethan5[(i-1)*8+1] = 0;
                            PerDay_Percent[1] = (float)count_state/count_all;
                            if(PerDay_Percent[1] < 0.75)
                            {
                                Flag_Accumto[i-1][1]++;
                            }
                            else
                            {
                                Flag_Accumto[i-1][1] = 0;
                            }
                        }
                    }

                    //3#油枪
                    if(Amount_Gasgun[i-1]>=3)
                    {
                        count_state = 0;
                        count_all = 0;
                        qry.prepare(QString("SELECT flagstate FROM reoilgasinfo WHERE whichone LIKE '%1-3%' AND time like '%2%'").arg(i).arg(current_datetime_qstr));
                        qry.exec();
                        rec = qry.record();
                        cols = rec.count();
                        for(int r = 0;qry.next();r++)
                        {
                            if((qry.value(0).toInt() == 0)||(qry.value(0).toInt() == 1))//有效数据  大于15L且不是流速过低数据
                            {
                                for(int c = 0;c < cols;c++)
                                {
                                    qDebug()<<QString("Row %1 : %2").arg(c).arg(qry.value(c).toString())<<endl;
                                }
                                if(!(qry.value(0).toInt()))
                                {
                                    count_state++;
                                }
                                count_all++;
                            }
                        }
                        if((count_all < 5)||(flag_morethan5[(i-1)*8+2] > 0))
                        {
                            if(flag_morethan5[(i-1)*8+2] > 0)
                            {
                                QDateTime current_datetime_pre = current_datetime.addDays(-(flag_morethan5[(i-1)*8+2]));
                                QDateTime current_datetime_later = current_datetime;
                                QString current_datetime_qstr_pre = current_datetime_pre.toString("yyyy-MM-dd");
                                QString current_datetime_qstr_later = current_datetime_later.toString("yyyy-MM-dd  23:59:59");
                                qDebug() << i<<"-1"<<current_datetime_qstr_pre <<"  to  " << current_datetime_qstr_later;
                                qry.prepare(QString("SELECT flagstate FROM reoilgasinfo WHERE whichone LIKE '%1-1%' AND time between '%2' and '%3'").arg(i).arg(current_datetime_qstr_pre).arg(current_datetime_qstr_later));
                                qry.exec();
                                rec = qry.record();
                                cols = rec.count();
                                count_all = 0;
                                count_state = 0;
                                for(int r = 0;qry.next();r++)
                                {
                                    if((qry.value(0).toInt() == 0)||(qry.value(0).toInt() == 1))//有效数据  大于15L且不是流速过低数据
                                    {
                                        for(int c = 0;c < cols;c++)
                                        {
                                            qDebug()<<QString("Row %1 : %2").arg(c).arg(qry.value(c).toString())<<endl;
                                        }
                                        if(!(qry.value(0).toInt()))
                                        {
                                            count_state++;
                                        }
                                        count_all++;
                                    }
                                }
                                if(count_all > 5) //如果大于5比数据了，正常处理
                                {
                                    PerDay_Percent[2] = (float)count_state/count_all;
                                    if(PerDay_Percent[2] < 0.75)
                                    {
                                        Flag_Accumto[i-1][2]++;
                                    }
                                    else
                                    {
                                        Flag_Accumto[i-1][2] = 0;
                                    }
                                    flag_morethan5[(i-1)*8+2] = 0;
                                }
                                else //如果少于5比数据
                                {
                                    flag_morethan5[(i-1)*8+2]++;
                                }
                            }
                            else
                            {
                                flag_morethan5[(i-1)*8+2]++;
                            }
                            if(flag_morethan5[(i-1)*8+2]>100)//连续100天没有加油则清零
                            {
                                flag_morethan5[(i-1)*8+2] = 0;
                            }
                        }
                        else
                        {
                            flag_morethan5[(i-1)*8+2] = 0;
                            PerDay_Percent[2] = (float)count_state/count_all;
                            if(PerDay_Percent[2] < 0.75)
                            {
                                Flag_Accumto[i-1][2]++;
                            }
                            else
                            {
                                Flag_Accumto[i-1][2] = 0;
                            }
                        }
                    }

                    //4#油枪
                    if(Amount_Gasgun[i-1]>=4)
                    {
                        count_state = 0;
                        count_all = 0;
                        qry.prepare(QString("SELECT flagstate FROM reoilgasinfo WHERE whichone LIKE '%1-4%' AND time like '%2%'").arg(i).arg(current_datetime_qstr));
                        qry.exec();
                        rec = qry.record();
                        cols = rec.count();
                        for(int r = 0;qry.next();r++)
                        {
                            if((qry.value(0).toInt() == 0)||(qry.value(0).toInt() == 1))//有效数据  大于15L且不是流速过低数据
                            {
                                for(int c = 0;c < cols;c++)
                                {
                                    qDebug()<<QString("Row %1 : %2").arg(c).arg(qry.value(c).toString())<<endl;
                                }
                                if(!(qry.value(0).toInt()))
                                {
                                    count_state++;
                                }
                                count_all++;
                            }
                        }
                        if((count_all < 5)||(flag_morethan5[(i-1)*8+3] > 0))
                        {
                            if(flag_morethan5[(i-1)*8+3] > 0)
                            {
                                QDateTime current_datetime_pre = current_datetime.addDays(-(flag_morethan5[(i-1)*8+3]));
                                QDateTime current_datetime_later = current_datetime;
                                QString current_datetime_qstr_pre = current_datetime_pre.toString("yyyy-MM-dd");
                                QString current_datetime_qstr_later = current_datetime_later.toString("yyyy-MM-dd  23:59:59");
                                qDebug() << i<<"-1"<<current_datetime_qstr_pre <<"  to  " << current_datetime_qstr_later;
                                qry.prepare(QString("SELECT flagstate FROM reoilgasinfo WHERE whichone LIKE '%1-1%' AND time between '%2' and '%3'").arg(i).arg(current_datetime_qstr_pre).arg(current_datetime_qstr_later));
                                qry.exec();
                                rec = qry.record();
                                cols = rec.count();
                                count_all = 0;
                                count_state = 0;
                                for(int r = 0;qry.next();r++)
                                {
                                    if((qry.value(0).toInt() == 0)||(qry.value(0).toInt() == 1))//有效数据  大于15L且不是流速过低数据
                                    {
                                        for(int c = 0;c < cols;c++)
                                        {
                                            qDebug()<<QString("Row %1 : %2").arg(c).arg(qry.value(c).toString())<<endl;
                                        }
                                        if(!(qry.value(0).toInt()))
                                        {
                                            count_state++;
                                        }
                                        count_all++;
                                    }
                                }
                                if(count_all > 5) //如果大于5比数据了，正常处理
                                {
                                    PerDay_Percent[3] = (float)count_state/count_all;
                                    if(PerDay_Percent[3] < 0.75)
                                    {
                                        Flag_Accumto[i-1][3]++;
                                    }
                                    else
                                    {
                                        Flag_Accumto[i-1][3] = 0;
                                    }
                                    flag_morethan5[(i-1)*8+3] = 0;
                                }
                                else //如果少于5比数据
                                {
                                    flag_morethan5[(i-1)*8+3]++;
                                }
                            }
                            else
                            {
                                flag_morethan5[(i-1)*8+3]++;
                            }
                            if(flag_morethan5[(i-1)*8+3]>100)//连续100天没有加油则清零
                            {
                                flag_morethan5[(i-1)*8+3] = 0;
                            }
                        }
                        else
                        {
                            flag_morethan5[(i-1)*8+3] = 0;
                            PerDay_Percent[3] = (float)count_state/count_all;
                            if(PerDay_Percent[3] < 0.75)
                            {
                                Flag_Accumto[i-1][3]++;
                            }
                            else
                            {
                                Flag_Accumto[i-1][3] = 0;
                            }
                        }
                    }

                    //5#油枪
                    if(Amount_Gasgun[i-1]>=5)
                    {
                        count_state = 0;
                        count_all = 0;
                        qry.prepare(QString("SELECT flagstate FROM reoilgasinfo WHERE whichone LIKE '%1-5%' AND time like '%2%'").arg(i).arg(current_datetime_qstr));
                        qry.exec();
                        rec = qry.record();
                        cols = rec.count();
                        for(int r = 0;qry.next();r++)
                        {
                            if((qry.value(0).toInt() == 0)||(qry.value(0).toInt() == 1))//有效数据  大于15L且不是流速过低数据
                            {
                                for(int c = 0;c < cols;c++)
                                {
                                    qDebug()<<QString("Row %1 : %2").arg(c).arg(qry.value(c).toString())<<endl;
                                }
                                if(!(qry.value(0).toInt()))
                                {
                                    count_state++;
                                }
                                count_all++;
                            }
                        }
                        if((count_all < 5)||(flag_morethan5[(i-1)*8+4] > 0))
                        {
                            if(flag_morethan5[(i-1)*8+4] > 0)
                            {
                                QDateTime current_datetime_pre = current_datetime.addDays(-(flag_morethan5[(i-1)*8+4]));
                                QDateTime current_datetime_later = current_datetime;
                                QString current_datetime_qstr_pre = current_datetime_pre.toString("yyyy-MM-dd");
                                QString current_datetime_qstr_later = current_datetime_later.toString("yyyy-MM-dd  23:59:59");
                                qDebug() << i<<"-1"<<current_datetime_qstr_pre <<"  to  " << current_datetime_qstr_later;
                                qry.prepare(QString("SELECT flagstate FROM reoilgasinfo WHERE whichone LIKE '%1-1%' AND time between '%2' and '%3'").arg(i).arg(current_datetime_qstr_pre).arg(current_datetime_qstr_later));
                                qry.exec();
                                rec = qry.record();
                                cols = rec.count();
                                count_all = 0;
                                count_state = 0;
                                for(int r = 0;qry.next();r++)
                                {
                                    if((qry.value(0).toInt() == 0)||(qry.value(0).toInt() == 1))//有效数据  大于15L且不是流速过低数据
                                    {
                                        for(int c = 0;c < cols;c++)
                                        {
                                            qDebug()<<QString("Row %1 : %2").arg(c).arg(qry.value(c).toString())<<endl;
                                        }
                                        if(!(qry.value(0).toInt()))
                                        {
                                            count_state++;
                                        }
                                        count_all++;
                                    }
                                }
                                if(count_all > 5) //如果大于5比数据了，正常处理
                                {
                                    PerDay_Percent[4] = (float)count_state/count_all;
                                    if(PerDay_Percent[4] < 0.75)
                                    {
                                        Flag_Accumto[i-1][4]++;
                                    }
                                    else
                                    {
                                        Flag_Accumto[i-1][4] = 0;
                                    }
                                    flag_morethan5[(i-1)*8+4] = 0;
                                }
                                else //如果少于5比数据
                                {
                                    flag_morethan5[(i-1)*8+4]++;
                                }
                            }
                            else
                            {
                                flag_morethan5[(i-1)*8+4]++;
                            }
                            if(flag_morethan5[(i-1)*8+4]>100)//连续100天没有加油则清零
                            {
                                flag_morethan5[(i-1)*8+4] = 0;
                            }
                        }
                        else
                        {
                            flag_morethan5[(i-1)*8+4] = 0;
                            PerDay_Percent[4] = (float)count_state/count_all;
                            if(PerDay_Percent[4] < 0.75)
                            {
                                Flag_Accumto[i-1][4]++;
                            }
                            else
                            {
                                Flag_Accumto[i-1][4] = 0;
                            }
                        }
                    }

                    //6#油枪
                    if(Amount_Gasgun[i-1]>=6)
                    {
                        count_state = 0;
                        count_all = 0;
                        qry.prepare(QString("SELECT flagstate FROM reoilgasinfo WHERE whichone LIKE '%1-6%' AND time like '%2%'").arg(i).arg(current_datetime_qstr));
                        qry.exec();
                        rec = qry.record();
                        cols = rec.count();
                        for(int r = 0;qry.next();r++)
                        {
                            if((qry.value(0).toInt() == 0)||(qry.value(0).toInt() == 1))//有效数据  大于15L且不是流速过低数据
                            {
                                for(int c = 0;c < cols;c++)
                                {
                                    qDebug()<<QString("Row %1 : %2").arg(c).arg(qry.value(c).toString())<<endl;
                                }
                                if(!(qry.value(0).toInt()))
                                {
                                    count_state++;
                                }
                                count_all++;
                            }
                        }
                        if((count_all < 5)||(flag_morethan5[(i-1)*8+5] > 0))
                        {
                            if(flag_morethan5[(i-1)*8+5] > 0)
                            {
                                QDateTime current_datetime_pre = current_datetime.addDays(-(flag_morethan5[(i-1)*8+5]));
                                QDateTime current_datetime_later = current_datetime;
                                QString current_datetime_qstr_pre = current_datetime_pre.toString("yyyy-MM-dd");
                                QString current_datetime_qstr_later = current_datetime_later.toString("yyyy-MM-dd  23:59:59");
                                qDebug() << i<<"-1"<<current_datetime_qstr_pre <<"  to  " << current_datetime_qstr_later;
                                qry.prepare(QString("SELECT flagstate FROM reoilgasinfo WHERE whichone LIKE '%1-1%' AND time between '%2' and '%3'").arg(i).arg(current_datetime_qstr_pre).arg(current_datetime_qstr_later));
                                qry.exec();
                                rec = qry.record();
                                cols = rec.count();
                                count_all = 0;
                                count_state = 0;
                                for(int r = 0;qry.next();r++)
                                {
                                    if((qry.value(0).toInt() == 0)||(qry.value(0).toInt() == 1))//有效数据  大于15L且不是流速过低数据
                                    {
                                        for(int c = 0;c < cols;c++)
                                        {
                                            qDebug()<<QString("Row %1 : %2").arg(c).arg(qry.value(c).toString())<<endl;
                                        }
                                        if(!(qry.value(0).toInt()))
                                        {
                                            count_state++;
                                        }
                                        count_all++;
                                    }
                                }
                                if(count_all > 5) //如果大于5比数据了，正常处理
                                {
                                    PerDay_Percent[5] = (float)count_state/count_all;
                                    if(PerDay_Percent[5] < 0.75)
                                    {
                                        Flag_Accumto[i-1][5]++;
                                    }
                                    else
                                    {
                                        Flag_Accumto[i-1][5] = 0;
                                    }
                                    flag_morethan5[(i-1)*8+5] = 0;
                                }
                                else //如果少于5比数据
                                {
                                    flag_morethan5[(i-1)*8+5]++;
                                }
                            }
                            else
                            {
                                flag_morethan5[(i-1)*8+5]++;
                            }
                            if(flag_morethan5[(i-1)*8+5]>100)//连续100天没有加油则清零
                            {
                                flag_morethan5[(i-1)*8+5] = 0;
                            }
                        }
                        else
                        {
                            flag_morethan5[(i-1)*8+5] = 0;
                            PerDay_Percent[5] = (float)count_state/count_all;
                            if(PerDay_Percent[5] < 0.75)
                            {
                                Flag_Accumto[i-1][5]++;
                            }
                            else
                            {
                                Flag_Accumto[i-1][5] = 0;
                            }
                        }
                    }

                    //7#油枪
                    if(Amount_Gasgun[i-1]>=7)
                    {
                        count_state = 0;
                        count_all = 0;
                        qry.prepare(QString("SELECT flagstate FROM reoilgasinfo WHERE whichone LIKE '%1-7%' AND time like '%2%'").arg(i).arg(current_datetime_qstr));
                        qry.exec();
                        rec = qry.record();
                        cols = rec.count();
                        for(int r = 0;qry.next();r++)
                        {
                            if((qry.value(0).toInt() == 0)||(qry.value(0).toInt() == 1))//有效数据  大于15L且不是流速过低数据
                            {
                                for(int c = 0;c < cols;c++)
                                {
                                    qDebug()<<QString("Row %1 : %2").arg(c).arg(qry.value(c).toString())<<endl;
                                }
                                if(!(qry.value(0).toInt()))
                                {
                                    count_state++;
                                }
                                count_all++;
                            }
                        }
                        if((count_all < 5)||(flag_morethan5[(i-1)*8+6] > 0))
                        {
                            if(flag_morethan5[(i-1)*8+6] > 0)
                            {
                                QDateTime current_datetime_pre = current_datetime.addDays(-(flag_morethan5[(i-1)*8+6]));
                                QDateTime current_datetime_later = current_datetime;
                                QString current_datetime_qstr_pre = current_datetime_pre.toString("yyyy-MM-dd");
                                QString current_datetime_qstr_later = current_datetime_later.toString("yyyy-MM-dd  23:59:59");
                                qDebug() << i<<"-1"<<current_datetime_qstr_pre <<"  to  " << current_datetime_qstr_later;
                                qry.prepare(QString("SELECT flagstate FROM reoilgasinfo WHERE whichone LIKE '%1-1%' AND time between '%2' and '%3'").arg(i).arg(current_datetime_qstr_pre).arg(current_datetime_qstr_later));
                                qry.exec();
                                rec = qry.record();
                                cols = rec.count();
                                count_all = 0;
                                count_state = 0;
                                for(int r = 0;qry.next();r++)
                                {
                                    if((qry.value(0).toInt() == 0)||(qry.value(0).toInt() == 1))//有效数据  大于15L且不是流速过低数据
                                    {
                                        for(int c = 0;c < cols;c++)
                                        {
                                            qDebug()<<QString("Row %1 : %2").arg(c).arg(qry.value(c).toString())<<endl;
                                        }
                                        if(!(qry.value(0).toInt()))
                                        {
                                            count_state++;
                                        }
                                        count_all++;
                                    }
                                }
                                if(count_all > 5) //如果大于5比数据了，正常处理
                                {
                                    PerDay_Percent[6] = (float)count_state/count_all;
                                    if(PerDay_Percent[6] < 0.75)
                                    {
                                        Flag_Accumto[i-1][6]++;
                                    }
                                    else
                                    {
                                        Flag_Accumto[i-1][6] = 0;
                                    }
                                    flag_morethan5[(i-1)*8+6] = 0;
                                }
                                else //如果少于5比数据
                                {
                                    flag_morethan5[(i-1)*8+6]++;
                                }
                            }
                            else
                            {
                                flag_morethan5[(i-1)*8+6]++;
                            }
                            if(flag_morethan5[(i-1)*8+6]>100)//连续100天没有加油则清零
                            {
                                flag_morethan5[(i-1)*8+6] = 0;
                            }
                        }
                        else
                        {
                            flag_morethan5[(i-1)*8+6] = 0;
                            PerDay_Percent[6] = (float)count_state/count_all;
                            if(PerDay_Percent[6] < 0.75)
                            {
                                Flag_Accumto[i-1][6]++;
                            }
                            else
                            {
                                Flag_Accumto[i-1][6] = 0;
                            }
                        }
                    }

                    //8#油枪
                    if(Amount_Gasgun[i-1]>=8)
                    {
                        count_state = 0;
                        count_all = 0;
                        qry.prepare(QString("SELECT flagstate FROM reoilgasinfo WHERE whichone LIKE '%1-8%' AND time like '%2%'").arg(i).arg(current_datetime_qstr));
                        qry.exec();
                        rec = qry.record();
                        cols = rec.count();
                        for(int r = 0;qry.next();r++)
                        {
                            if((qry.value(0).toInt() == 0)||(qry.value(0).toInt() == 1))//有效数据  大于15L且不是流速过低数据
                            {
                                for(int c = 0;c < cols;c++)
                                {
                                    qDebug()<<QString("Row %1 : %2").arg(c).arg(qry.value(c).toString())<<endl;
                                }
                                if(!(qry.value(0).toInt()))
                                {
                                    count_state++;
                                }
                                count_all++;
                            }
                        }
                        if((count_all < 5)||(flag_morethan5[(i-1)*8+7] > 0))
                        {
                            if(flag_morethan5[(i-1)*8+7] > 0)
                            {
                                QDateTime current_datetime_pre = current_datetime.addDays(-(flag_morethan5[(i-1)*8+7]));
                                QDateTime current_datetime_later = current_datetime;
                                QString current_datetime_qstr_pre = current_datetime_pre.toString("yyyy-MM-dd");
                                QString current_datetime_qstr_later = current_datetime_later.toString("yyyy-MM-dd  23:59:59");
                                qDebug() << i<<"-1"<<current_datetime_qstr_pre <<"  to  " << current_datetime_qstr_later;
                                qry.prepare(QString("SELECT flagstate FROM reoilgasinfo WHERE whichone LIKE '%1-1%' AND time between '%2' and '%3'").arg(i).arg(current_datetime_qstr_pre).arg(current_datetime_qstr_later));
                                qry.exec();
                                rec = qry.record();
                                cols = rec.count();
                                count_all = 0;
                                count_state = 0;
                                for(int r = 0;qry.next();r++)
                                {
                                    if((qry.value(0).toInt() == 0)||(qry.value(0).toInt() == 1))//有效数据  大于15L且不是流速过低数据
                                    {
                                        for(int c = 0;c < cols;c++)
                                        {
                                            qDebug()<<QString("Row %1 : %2").arg(c).arg(qry.value(c).toString())<<endl;
                                        }
                                        if(!(qry.value(0).toInt()))
                                        {
                                            count_state++;
                                        }
                                        count_all++;
                                    }
                                }
                                if(count_all > 5) //如果大于5比数据了，正常处理
                                {
                                    PerDay_Percent[7] = (float)count_state/count_all;
                                    if(PerDay_Percent[7] < 0.75)
                                    {
                                        Flag_Accumto[i-1][7]++;
                                    }
                                    else
                                    {
                                        Flag_Accumto[i-1][7] = 0;
                                    }
                                    flag_morethan5[(i-1)*8+7] = 0;
                                }
                                else //如果少于5比数据
                                {
                                    flag_morethan5[(i-1)*8+7]++;
                                }
                            }
                            else
                            {
                                flag_morethan5[(i-1)*8+7]++;
                            }
                            if(flag_morethan5[(i-1)*8+7]>100)//连续100天没有加油则清零
                            {
                                flag_morethan5[(i-1)*8+7] = 0;
                            }
                        }
                        else
                        {
                            flag_morethan5[(i-1)*8+7] = 0;
                            PerDay_Percent[7] = (float)count_state/count_all;
                            if(PerDay_Percent[7] < 0.75)
                            {
                                Flag_Accumto[i-1][7]++;
                            }
                            else
                            {
                                Flag_Accumto[i-1][7] = 0;
                            }
                        }
                    }

                }
                config_info_accum_write();
                break;
    }
}


//图表显示  日数据选择
void select_data_oneday_tanktempr()
{
    memset(Temperature_Day,0,sizeof(Temperature_Day));
    QSqlQuery qry;
    qry.prepare(QString("SELECT time,temperature FROM envinfo WHERE time LIKE '%1%'").arg(QDate::currentDate().toString("yyyy-MM-dd")));
    qry.exec();
    int i_time = 0;
    for(int r = 0;qry.next();r++)
    {
        QDateTime time_temp = QDateTime::fromString(qry.value(0).toString(),"yyyy-MM-dd  hh:mm:ss");
        int time_h = time_temp.toString("hh").toInt();
        int time_m = time_temp.toString("mm").toInt();
 //       qDebug()<<time_temp.toString("hh:mm") + QString("time %1:%2").arg(time_h).arg(time_m)<<endl;//+"++"+qry.value(1).toString()<<endl;
        int time_temp_int = time_h*60 + time_m;
        if(time_temp_int > (Temperature_Day[i_time][0] + 4))
        {
            i_time++;
            Temperature_Day[i_time][0] = (float)time_temp_int;
            Temperature_Day[i_time][1] = qry.value(1).toFloat();
        }
    }
}
void select_data_oneday_tankpre()
{
    memset(Temperature_Day,0,sizeof(Temperature_Day));
    QSqlQuery qry;
    qry.prepare(QString("SELECT time,tankpre FROM envinfo WHERE time LIKE '%1%'").arg(QDate::currentDate().toString("yyyy-MM-dd")));
    qry.exec();
    int i_time = 0;
    for(int r = 0;qry.next();r++)
    {
        QDateTime time_temp = QDateTime::fromString(qry.value(0).toString(),"yyyy-MM-dd  hh:mm:ss");
        int time_h = time_temp.toString("hh").toInt();
        int time_m = time_temp.toString("mm").toInt();
        int time_temp_int = time_h*60 + time_m;
        if(time_temp_int > (Temperature_Day[i_time][0] + 4))
        {
            i_time++;
            Temperature_Day[i_time][0] = (float)time_temp_int;
            Temperature_Day[i_time][1] = qry.value(1).toFloat();
        }
    }
}
void select_data_oneday_pipepre()
{
    memset(Temperature_Day,0,sizeof(Temperature_Day));
    QSqlQuery qry;
    qry.prepare(QString("SELECT time,pipepre FROM envinfo WHERE time LIKE '%1%'").arg(QDate::currentDate().toString("yyyy-MM-dd")));
    qry.exec();
    int i_time = 0;
    for(int r = 0;qry.next();r++)
    {
        QDateTime time_temp = QDateTime::fromString(qry.value(0).toString(),"yyyy-MM-dd  hh:mm:ss");
        int time_h = time_temp.toString("hh").toInt();
        int time_m = time_temp.toString("mm").toInt();
        int time_temp_int = time_h*60 + time_m;
        if(time_temp_int > (Temperature_Day[i_time][0] + 4))
        {
            i_time++;
            Temperature_Day[i_time][0] = (float)time_temp_int;
            Temperature_Day[i_time][1] = qry.value(1).toFloat();
        }
    }
}
void select_data_oneday_tanknongdu()
{
    memset(Temperature_Day,0,sizeof(Temperature_Day));
    QSqlQuery qry;
    qry.prepare(QString("SELECT time,nongdu FROM envinfo WHERE time LIKE '%1%'").arg(QDate::currentDate().toString("yyyy-MM-dd")));
    qry.exec();
    int i_time = 0;
    float nongdu_max = 0;
    int time_max = 0;
    int time_until = 0;
    for(int r = 0;qry.next();r++)
    {
        QDateTime time_temp = QDateTime::fromString(qry.value(0).toString(),"yyyy-MM-dd  hh:mm:ss");
        int time_h = time_temp.toString("hh").toInt();
        int time_m = time_temp.toString("mm").toInt();
        int time_temp_int = time_h*60 + time_m;
        if((qry.value(1).toFloat() >= nongdu_max) && (time_temp_int>time_until))
        {
            nongdu_max = qry.value(1).toFloat();
            time_max = time_temp_int;
        }
        if(time_temp_int > (time_until + 4))
        {
            i_time++;
            Temperature_Day[i_time][0] = (float)time_max;
            time_until = time_temp_int;
            Temperature_Day[i_time][1] = nongdu_max;
            nongdu_max = 0;
            printf("fga vale %f--%f\n",Temperature_Day[i_time][0],Temperature_Day[i_time][1]);
        }
    }
}
void select_data_oneday_oilgun_al(unsigned char t)
{
    unsigned char which_disp = t/8 + 1;
    unsigned char which_gun = t%8;
    if(!which_gun)
    {
        which_gun = 8;
        which_disp--;
    }
    memset(AL_Day,0,sizeof(Temperature_Day));
    QSqlQuery qry;
    qry.prepare(QString("SELECT time,al FROM reoilgasinfo WHERE time LIKE '%1%' and whichone = '%2-%3'").arg(QDate::currentDate().toString("yyyy-MM-dd")).arg(which_disp).arg(which_gun));
    qry.exec();
    int i_time = 0;
    float al_temp[1440];
    for(int i = 0;i<1440;i++)
    {
        al_temp[i] = -1;
    }
    for(int r = 0;qry.next();r++)
    {
        QDateTime time_temp = QDateTime::fromString(qry.value(0).toString(),"yyyy-MM-dd  hh:mm:ss");
        int time_h = time_temp.toString("hh").toInt();
        int time_m = time_temp.toString("mm").toInt();
        int time_temp_int = time_h*60 + time_m;
        if(!time_temp_int)
        {
            al_temp[time_temp_int] = qry.value(1).toFloat()+2;  //防止为0
        }
        al_temp[time_temp_int] = qry.value(1).toFloat();

    }
    for(int i = 0;i<1440;i++)
    {
        if(al_temp[i]>=0)
        {
            AL_Day[i_time][0] = i;
            AL_Day[i_time][1] = al_temp[i];
            i_time++;
        }
    }
}
//月数据选择
void select_data_onemonth_tanktempr()
{
    memset(Temperature_Month,0,sizeof(Temperature_Month));
    QSqlQuery qry;
    qry.prepare(QString("SELECT time,temperature FROM envinfo WHERE time LIKE '%1%'").arg(QDate::currentDate().toString("yyyy-MM-")));
    qry.exec();
    unsigned int i_time = 0;
    for(int r = 0;qry.next();r++)
    {
        QDateTime time_temp = QDateTime::fromString(qry.value(0).toString(),"yyyy-MM-dd  hh:mm:ss");
        int time_d = time_temp.toString("dd").toInt();
        int time_h = time_temp.toString("hh").toInt();

        int time_temp_int = time_d*24 + time_h;

        if(time_temp_int > Temperature_Month[i_time][0])
        {
            i_time++;
            Temperature_Month[i_time][0] = time_temp_int;
            Temperature_Month[i_time][1] = qry.value(1).toFloat();
            printf("monthdata %f  %f\n",Temperature_Month[i_time][0],Temperature_Month[i_time][1]);
        }
    }
}
void select_data_onemonth_tankpre()
{
    memset(Temperature_Month,0,sizeof(Temperature_Month));
    QSqlQuery qry;
    qry.prepare(QString("SELECT time,tankpre FROM envinfo WHERE time LIKE '%1%'").arg(QDate::currentDate().toString("yyyy-MM-")));
    qry.exec();
    unsigned int i_time = 0;
    for(int r = 0;qry.next();r++)
    {
        QDateTime time_temp = QDateTime::fromString(qry.value(0).toString(),"yyyy-MM-dd  hh:mm:ss");
        int time_d = time_temp.toString("dd").toInt();
        int time_h = time_temp.toString("hh").toInt();
        int time_temp_int = time_d*24 + time_h;
        if(time_temp_int > Temperature_Month[i_time][0])
        {
            i_time++;
            Temperature_Month[i_time][0] = time_temp_int;
            Temperature_Month[i_time][1] = qry.value(1).toFloat();
        }
    }
}
void select_data_onemonth_pipepre()
{
    memset(Temperature_Month,0,sizeof(Temperature_Month));
    QSqlQuery qry;
    qry.prepare(QString("SELECT time,pipepre FROM envinfo WHERE time LIKE '%1%'").arg(QDate::currentDate().toString("yyyy-MM-")));
    qry.exec();
    unsigned int i_time = 0;
    for(int r = 0;qry.next();r++)
    {
        QDateTime time_temp = QDateTime::fromString(qry.value(0).toString(),"yyyy-MM-dd  hh:mm:ss");
        int time_d = time_temp.toString("dd").toInt();
        int time_h = time_temp.toString("hh").toInt();
        int time_temp_int = time_d*24 + time_h;
        if(time_temp_int > Temperature_Month[i_time][0])
        {
            i_time++;
            Temperature_Month[i_time][0] = time_temp_int;
            Temperature_Month[i_time][1] = qry.value(1).toFloat();
        }
    }
}
void select_data_onemonth_tanknongdu()
{
    memset(Temperature_Month,0,sizeof(Temperature_Month));
    QSqlQuery qry;
    qry.prepare(QString("SELECT time,nongdu FROM envinfo WHERE time LIKE '%1%'").arg(QDate::currentDate().toString("yyyy-MM-")));
    qry.exec();
    unsigned int i_time = 0;
    float nongdu_max = 0;
    int time_max = 0;
    int time_until = 0;
    for(int r = 0;qry.next();r++)
    {
        QDateTime time_temp = QDateTime::fromString(qry.value(0).toString(),"yyyy-MM-dd  hh:mm:ss");
        int time_h = time_temp.toString("hh").toInt();
        int time_d = time_temp.toString("dd").toInt();
        int time_temp_int = time_d*24 + time_h;
        if((qry.value(1).toInt() >= nongdu_max) && (time_temp_int>time_max))
        {
            nongdu_max = qry.value(1).toFloat();
            time_max = time_temp_int;
        }
        if(time_temp_int > time_until)
        {
            i_time++;
            Temperature_Month[i_time][0] = (float)time_max;
            time_until = time_temp_int;
            Temperature_Month[i_time][1] = nongdu_max;
            nongdu_max = 0;
        }

    }
}
void select_data_onemonth_oilgun_al(unsigned char t)
{
    unsigned char which_disp = t/8 + 1;
    unsigned char which_gun = t%8;
    if(!which_gun)
    {
        which_gun = 8;
        which_disp--;
    }
    memset(Temperature_Month,0,sizeof(Temperature_Month));
    memset(Temperature_Month_Min,0,sizeof(Temperature_Month_Min));
    QSqlQuery qry;
    qry.prepare(QString("SELECT time,al FROM reoilgasinfo WHERE time LIKE '%1%' and whichone = '%2-%3'").arg(QDate::currentDate().toString("yyyy-MM-")).arg(which_disp).arg(which_gun));
    qry.exec();
    qDebug()<<QString("SELECT time,al FROM reoilgasinfo WHERE time LIKE '%1%' and whichone = '%2-%3'").arg(QDate::currentDate().toString("yyyy-MM-")).arg(which_disp).arg(which_gun)<<endl;

    qry.next();
    unsigned int i_time = 0;
    float al_max = qry.value(1).toFloat();
    float al_min = al_max;
    QDateTime temp = QDateTime::fromString(qry.value(0).toString(),"yyyy-MM-dd  hh:mm:ss");
    //int time_max = (temp.toString("dd").toInt())*24 + temp.toString("hh").toInt();
    int time_max = (temp.toString("dd").toInt())*4 + (temp.toString("hh").toInt())/6; //每小时分析改为每6小时分析
    int time_min = time_max;
    int time_last = time_max;
    while(1)
    {
        if(qry.next())
        {
            QDateTime time_temp = QDateTime::fromString(qry.value(0).toString(),"yyyy-MM-dd  hh:mm:ss");
            int time_h = time_temp.toString("hh").toInt();
            int time_d = time_temp.toString("dd").toInt();
            //int time_temp_int = time_d*24 + time_h;
            int time_temp_int = time_d*4 + time_h/6;
            float a = qry.value(1).toFloat();
            if(time_temp_int == time_last)
            {

                if(a >= al_max)
                {
                    al_max = a;
                    time_max = time_temp_int;
                }
                if(a <= al_min)
                {
                    al_min = a;
                    time_min = time_temp_int;
                }
            }
            else
            {
                Temperature_Month[i_time][0] = time_max*6;
                Temperature_Month[i_time][1] = al_max;
                Temperature_Month_Min[i_time][0] = time_min*6;
                Temperature_Month_Min[i_time][1] = al_min;
                al_max = a;
                time_max = time_temp_int;
                al_min = a;
                time_min = time_temp_int;
                time_last = time_temp_int;
                i_time++;
            }
        }
        else
        {
            Temperature_Month[i_time][0] = time_max*6;
            Temperature_Month[i_time][1] = al_max;
            Temperature_Month_Min[i_time][0] = time_min*6;
            Temperature_Month_Min[i_time][1] = al_min;
            break;
        }
    }
}
//预留  雷达有部分可用
void update_table()
{
}
//预留  看删除记录怎么组织
void delete_table()
{
}
//预留  输出表格
void output_table()
{}





