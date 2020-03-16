#ifndef CONFIG
#define CONFIG

#include<qmutex.h>
//全局变量区域
//宏定义区域
//油气回收
#define CONFIG_REOILGAS "/opt/reoilgas/amountset_reoilgas.ini"
#define CONFIG_GAS_FACTOR   "/opt/reoilgas/config_gas_factor.txt"
#define CONFIG_OIL_FACTOR   "/opt/reoilgas/config_oil_factor.txt"
#define CONFIG_MAPPING   "/opt/reoilgas/config_mapping.txt"
#define CONFIG_MAPPING_SHOW   "/opt/reoilgas/config_mapping_show.txt"

//数据库列表名称
#define CONTROLINFO     "controlinfo"
#define OPERATEINFO     "operateinfo"
#define NETINFO         "netinfo"
#define HIS_RADAR       "history_radarinfo"
#define HIS_JINGDIAN    "history_jingdianinfo"

//进程通信
#define EXCHANGE_690_PLAT   "/opt/exchange690.txt"
#define EXCHANGE_690_PLAT_TEMP  "/opt/exchange690_temp.txt"

#define EXCHANGE_SOUND  "/opt/exchangesound.txt"
#define EXCHANGE_SOUND_TEMP "/opt/exchangesound_temp.txt"

//与690通信
#define READ_DATA 0x04
#define START_BYTE1 0xA5        //起始字节1
#define START_BYTE2 0x5A        //起始字节2


#define BACKLOG 	10
#define LENGTH 512   // Buffer length
#define UDP_local_id_high    0x16    //UDP逻辑节点高位
#define UDP_local_id_low     0x01    //UDP逻辑diwei

#define if_name     "eth1"

#define DEV_NAME    "/dev/ttymxc6"   //ttySP0 泄漏
#define RADAR_SERI  "/dev/ttymxc7"  //uart8  雷达
#define REOILGAS_SERI   "/dev/ttymxc4"  //uart5 加油机
#define FGA_SERI    "/dev/ttymxc3"  //uart4  气体监测
#define CRASH_COLUMN "/dev/ttymxc1"  //uart2  防撞柱

#define CONFIG_SENSORAMOUNT "/opt/config.txt"
#define HISTORY_NET "/opt/net_info.txt"
#define HISTORY_SENSOR "/opt/control_info.txt"
#define HISTORY_OPERATE "/opt/operate_info.txt"

#define HISTORY_RADAR "/opt/radar/history_radar.txt"
#define HISTORY_JINGDIAN "/opt/jingdian/history_jingdian.txt"

#define CONFIG_SECURITY "/opt/jingdian/config_security.txt"
#define CONFIG_POSTNETWORK "/opt/reoilgas/config_postnetwork.txt"
//用户管理
#define USER_MANAGER "/opt/manager/SU/"
#define USER_USER   "/opt/manager/"

//加油枪详细报警信息
#define GUN_UARTWRONG "通信故障：控制器与气液比采集器通信故障"
#define GUN_NORMAL "设备正常"
#define GUN_LOW_OILSPEED "燃油流速慢：加油机发油速度慢，请检查加油机是否正常"
#define GUN_LOW_GASSPEED "油气流速慢：油气流速低于正常值，真空泵效率低或油气回收管路堵塞"
#define GUN_GASPUMP_WRONG "真空泵故障：油气回收管路回气量几乎为0，请检查真空泵是否正常工作"
#define GUN_EARLY_WARNING "油枪预警：连续报警超过规定天数，加油枪报警"
#define GUN_WARNING "油枪报警：一天加油次数中气液比不合格占比超过25%"

//main_main.h
extern char IP_BRO[32];
extern unsigned char Flag_TcpClose_FromUdp;
extern unsigned char Flag_TcpClose_FromTcp;

//mainwindow.h
extern unsigned char Flag_Sound_Radar_temp;
extern unsigned int Flag_warn_delay;
extern unsigned char count_basin;
extern unsigned char count_pipe;
extern unsigned char count_tank;
extern unsigned char count_dispener;
extern unsigned char flag_silent;
extern unsigned char Flag_xieyou;  //人体静电使能
extern unsigned char Flag_IIE; //IIE使能
extern float count_Pressure[8]; //存储压力值

extern unsigned char IIE_sta[8]; //IIE状态
extern unsigned char IIE_set[8]; //IIE设置

extern unsigned char Flag_Timeto_CloseNeeded[6];
extern QMutex Lock_Flag_Timeto;
extern QMutex Lock_Flag_xielou;
extern QMutex Lock_Flag_youqi;
extern unsigned char Flag_Network_Send_Version;//油气回收网络上传版本
extern unsigned char Flag_Show_ReoilgasPop;//油气回收检测有问题的弹窗弹出标志位 0 不弹出 1 通信故障  2 报警、预警相关 3 加油机诊断
extern unsigned char Flag_Reoilgas_DayShow;//弹窗当日不再显示 0显示 1不显示
extern unsigned char Flag_Reoilgas_NeverShow;//弹窗不在显示 0显示 1不显示

extern long debug_send;
extern long debug_read;

//mythread.h
extern unsigned char flag_waitsetup;
extern unsigned char flag_mythread;
extern unsigned char Flag_Sound_Xielou[20];

//radar_485.h
extern unsigned char Flag_Set_SendMode;
extern unsigned char Flag_autoget_area;
extern unsigned char Amp_R_Date[30];
extern unsigned char Get_Are_Flag[4];
extern int Master_Boundary_Point_Disp[4][6][6][2];
extern unsigned char Send_MODE[4];
extern unsigned char Master_Back_Groud_Value[4][200][2];
extern unsigned char Back_Groud_Temp[4][200][2];
extern unsigned char Get_Back_Flag[4];
extern float Goal[4][20][2];
extern unsigned char Flag_moni_warn;
extern unsigned char Alarm_Re_Flag[4];
extern unsigned char Communication_Machine;

//serial.h
extern unsigned char Data_Buf_Sencor[45];
extern unsigned char Test_Method;

//systemset.h
extern unsigned char Flag_Set_yuzhi;
extern unsigned char Flag_pre_mode;
extern unsigned char Flag_screen_xielou;
extern unsigned char Flag_screen_radar;
extern unsigned char Flag_screen_safe;
extern unsigned char Flag_screen_burngas;
extern unsigned char Flag_screen_zaixian;
extern unsigned char Flag_screen_cc;
extern unsigned char Mapping[96];
extern QString Mapping_Show[96];
//uartthread.h
extern unsigned char count_radar_uart;
extern unsigned char Flag_Sound_Radar[5];

//udp.h
extern unsigned char net_state;

//added for radar
extern unsigned int Count_auto_silent;
extern unsigned char Flag_auto_silent;
extern int area[6][6][2];   //六个手动设置区域

extern unsigned char Which_Area;

extern unsigned int Start_time_h;    //监控时间设置  h   开始  mainwindow
extern unsigned int Start_time_m;    //监控时间设置  m   开始
extern unsigned int Stop_time_h;     //监控时间设置  h   停止
extern unsigned int Stop_time_m;     //监控时间设置  m   停止

extern unsigned int Silent_time_h;    //自动取消静音 h
extern unsigned int Silent_time_m;    //自动取消静音 m
extern unsigned int Warn_delay_m;      //报警延长设置 m
extern unsigned int Warn_delay_s;      //报警延长设置 s
extern unsigned char Flag_outdoor_warn;    //室外报警器使能
extern unsigned char Flag_area_ctrl[4];    //防区开启设置
extern unsigned char Flag_sensitivity;     //灵敏度设置  1-6
extern int fd_uart_radar;
extern int ret_uart_radar;
extern int len_uart_radar;
//added for radar <-
//油气回收
extern unsigned char Flag_Accumto[12][8];
extern unsigned char Flag_Delay_State[12][8];
extern double PerDay_Percent[8];
extern float PerDay_AL[8];
extern float PerDay_Al_Big[8];
extern float PerDay_Al_Smal[8];
extern float Gas_Factor[12][8];      //气量校正因子
extern float Fueling_Factor[12][8];  //油量校正因子
extern unsigned char Amount_Dispener;
extern int Far_Dispener ;            //最远端加油机编号
extern unsigned char Speed_fargas ;  //最远端油气流速
extern unsigned char Amount_Gasgun[12];
extern unsigned char Flag_SendMode_Oilgas;
extern unsigned char Reoilgas_Version_Set_Whichone;
extern QMutex Lock_Mode_Reoilgas;
extern float Temperature_Day[290][2];//图标日数据
extern float Temperature_Month[750][2];//月数据
extern float Temperature_Month_Min[750][2];
extern float AL_Day[1440][2];
extern unsigned char flag_morethan5[96];//记录连续几天没有到达5次加油
extern float NormalAL_Low ;      //气液比下线
extern float NormalAL_High ;     //气液比上限
extern unsigned char WarnAL_Days;//连续几天预警后报警
extern unsigned char Flag_Gun_off;
extern unsigned char Flag_Reoilgas_Version;//气液比采集器版本选择
extern QString Time_Reoilgas;    //第二版本气液比采集器发上来的时间

//post添加  net_isoosi
extern QString Post_Address ;    //通信地址
extern QString VERSION_POST ;   // 通信协议版本
extern QString DATAID_POST ;    //数据序号（6 位）
extern QString USERID_POST ;    // 区域代码标识（6 位）+ 加油站标识（4 位）
extern QString TIME_POST ;      //在线监控设备当前时间（年月日时分 14 位）
extern QString TYPE_POST ;      //业务报文类型（2 位）
extern QString SEC_POST ;
extern QString POSTUSERNAME_HUNAN;
extern QString POSTPASSWORD_HUNAN;
extern unsigned char Flag_Shield_Network;                //屏蔽网络上传的报警信息，只上传正常数据
extern unsigned char Flag_CommunicateError_Maindisp[48]; //加油机通信故障报警，用来一天结束判断是否需要报警
extern unsigned char Flag_Postsend_Enable;               //网络上传使能位，1 上传  0不上传
//isoosi
extern QString IsoOis_MN ;      //"440111301A52TWUF73000001";
extern QString IsoOis_PW ;      //"758534";
extern QString IsoOis_UrlIp;  //网络上传ip
extern QString IsoOis_UrlPort; //网络上传端口
//isoosi重庆
extern QString IsoOis_StationId_Cq;      //加油站ID 重庆

//reoilgas_pop
extern unsigned char ReoilgasPop_GunSta[96];//96把枪的状态
//fga
extern unsigned char Env_Gas_en;//气体浓度传感器使能
extern unsigned char Pre_tank_en;//油罐压力传感器使能
extern unsigned char Pre_pipe_en;//管线液阻压使能
extern unsigned char Tem_tank_en;//油罐、气体温度使能
extern float Tem[2];             //温度  tem[0]油罐 1管线
extern float Pre[2];             //压力
extern float Positive_Pres;      //正压开启压力
extern float Negative_Pres;      //负压开启压力
extern unsigned char Num_Fga;
extern unsigned char Gas_Concentration_Fga[7];       //fga浓度 [0]没用
extern unsigned char Flag_Pressure_Transmitters_Mode;//压力变送器模式 0 485  1 4-20ma  2 无线模式，在reoilgas线程中问询
//extern QMutex Lock_Exchange_Reoilgas;

//泄漏网络
extern int PORT_UDP;
extern int PORT_TCP;
extern unsigned char Flag_XieLou_Version;//泄漏上传版本 0中石化泄漏 1湖北高速 2中油泄漏
extern unsigned char OIL_BASIN[16];
extern unsigned char OIL_PIPE[16];
extern unsigned char OIL_TANK[16];
extern unsigned char OIL_DISPENER[16];
extern unsigned char Mythread_basin[9];  //给tcp主动信号
extern unsigned char Mythread_pipe[9];
extern unsigned char Mythread_dispener[9];
extern unsigned char Mythread_tank[9];   //main_main

extern unsigned char LEAK_DETECTOR;      //设置时为维护状态，此时回复tcp状态，其他不回复，不检测
extern unsigned char version[7];         //初始化失败，网络绑定失败需转为不可操作状态
extern unsigned int ipa,ipb,ipc,ipd;
extern unsigned int ID_M;
extern int on;
extern unsigned int heartbeat_time;
extern char revbuf[LENGTH];
extern int time_count_talk;  //udp

extern unsigned int Timer_Counter;
extern int fd_uart;
extern int len_uart,ret_uart;

//防撞柱
extern int fd_uart_crash_column;
extern int ret_uart_crash_column;
extern int len_uart_crash_column;
extern unsigned int Num_Crash_Column;//自恢复防撞柱的数量

extern char *ipstr;

//net_tcpclient_hb 湖北高速泄漏协议
extern unsigned char Warn_Data_Tcp_Send_Hb[32][2];//32个点 油罐 油管 加油机 防渗池 各8个  0标志位 1状态位
extern unsigned char Station_ID_HB[2];//站端id 两位
extern unsigned char Flag_HuBeitcp_Enable;//功能使能

//reoilgas_pop
extern unsigned char Flag_Reoilgas_Pop_Sta;//在线油气回收详情页面打开状态 0 未打开 1打开

//my server thread
extern QString MyStationId; //全局变量youzhanID
extern QString MyStationPW; //全局变量
extern QString MyServerIp;
extern int MyServerPort;
extern unsigned char Flag_MyServerEn;
//void Sencor_Handle(int signo);
#endif // CONFIG

//传感器法  sensor   液媒法  liquid  压力法   pressure


