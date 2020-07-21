#ifndef YWYTHREAD_H
#define YWYTHREAD_H

#include <QThread>

class ywythread :public QThread
{
    Q_OBJECT
public:
    explicit ywythread(QObject *parent = 0);
    void run();

signals:
    void Send_Height_Signal(unsigned char add, QString str1,QString str2,QString str3);
    void Send_alarm_info(unsigned char add,unsigned char flag);
    void Send_compensation_Signal(unsigned char command,unsigned char hang, float data);
    void Set_tangan_add_success(unsigned char add);
    void compensation_set_result(unsigned char command,unsigned char add,QString str);

private slots:
    void Asking_Handle_YWY();
    void Data_Handle_YWY ();
    void Recving_Handle_YWY();
};

//ASCII
#define SOH  0x01
#define STX  0x02
#define ETX  0x03
#define EOH  0x04

struct Display_HeightData
{
    char OIL_Height[7]       = {0,0,0,0,0,0};
    char Water_Height[7]     = {0,0,0,0,0,0};
    char TEMP_Height[7]      = {0,0,0,0,0,0};
    char Height_Range[7]     = {0,0,0,0,0,0};
    char OIL_compensation[7] = {0,0,0,0,0,0};
    char Water_compensation[7] = {0,0,0,0,0,0};

    QString strOIL_Height;
    QString strWater_Height;
    QString strTEMP;
    QString strRange;
};
struct Tangan_Configuration
{
    char OIL_compensation[30] ;
    char Water_compensation[30] ;
};


#endif // YWYTHREAD_H
