#ifndef SECURITY_H
#define SECURITY_H
#include <QThread>

extern unsigned int Flag_Enable_liqiud ;//液位仪使能
extern unsigned int Flag_Enable_pump ;//潜油泵使能
extern unsigned char Flag_sta_liquid ;//液位仪状态，全局，网络用
extern unsigned char Flag_sta_pump ;//油泵状态，全局，网络用
class security :public QThread
{
    Q_OBJECT
public:
    bool stop;
    explicit security(QObject *parent = 0);
    void run();
private:
    void ask_crash_column();//问防撞柱
    void read_crach_column();//读防撞柱信息
    void security_init();//初始化
signals:
    void liquid_uartwrong();//用不到
    void pump_uartwrong();//用不到
    void liquid_warn();//高液位报警
    void liquid_nomal();//液位正常
    void liquid_close();
    void pump_run();//油泵开启
    void pump_stop();//油泵关闭
    void pump_close();
    void crash_column_stashow(unsigned char which, unsigned char sta);//主界面显示防撞柱状态
private slots:
    void ask_switch();//问开关量
    void time_uartwrong();//通信故障倒计时


};

#endif // SECURITY_H
