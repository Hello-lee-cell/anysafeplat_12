#ifndef UARTTHREAD_H
#define UARTTHREAD_H

#include<QThread>

//extern unsigned char count_radar_uart;
//extern unsigned char Flag_Sound_Radar[5];
class uartthread :public QThread
{
    Q_OBJECT
public:
    bool stop;
    explicit uartthread(QObject *parent = 0);
    void run();
 //   void* uart_read(void*);
private:

signals:
    void arm_pic_uart_wrong();
    void arm_pic_uart_right();
    void repaint_set_yuzhi();
    void warn_to_mainwindowstatelabel();

private slots:
  //  void Sencor_Handle();
    void SendDataRadar();
    void ReadDataRadar();
};

#endif // UARTTHREAD_H
