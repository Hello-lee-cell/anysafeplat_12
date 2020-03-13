#ifndef TIMER_POP_H
#define TIMER_POP_H
#include <QThread>

class timer_pop :public QThread
{
    Q_OBJECT
public:
    bool stop;
    explicit timer_pop(QObject *parent = 0);
    void run();
private:

signals:
    void show_reoilgas_pop(int sta);//显示弹窗用的信号
    void refresh_dispener_data();
private slots:
    void show_pop_ups();//显示弹窗

};
 #endif //TIMER_POP_H
