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

    void signal_close_fangyiyoufa(uchar add);

private:
    void ConvertDexToIEE754(float fpointer,unsigned char *a);
    float ConvertIEE754ToDex(unsigned char *SpModRegister);

private slots:
    void Asking_Handle_YWY();
    void Data_Handle_YWY ();
    void Recving_Handle_YWY();

    void slot_close_fangyiyoufa(uchar add);

};

void* close_fangyiyoufa(void*);

struct Display_HeightData
{
    char OIL_Height[7]       = {0,0,0,0,0,0};
    char Water_Height[7]     = {0,0,0,0,0,0};
    char TEMP_Height[7]      = {0,0,0,0,0,0};
    char Height_Range[7]     = {0,0,0,0,0,0};
    char OIL_compensation[7] = {0,0,0,0,0,0};
    char Water_compensation[7] = {0,0,0,0,0,0};

    uint OIL_Height_int = 0;
    uint Water_Height_int = 0;
    int TEMP_Height_int_1 = 0;
    int TEMP_Height_int_2 = 0;
    int TEMP_Height_int_3 = 0;
    uchar OIL_compensation_ucahr[10] = {0};
    uchar Water_compensation_uchar[10] = {0};

    float OIL_Height_float = 0;
    float Water_Height_float = 0;
    float TEMP_Height_float_1 = 0;
    float TEMP_Height_float_2 = 0;
    float TEMP_Height_float_3 = 0;

    QString strOIL_Height;
    QString strWater_Height;
    QString strTEMP;
    QString strRange;
};
struct Tangan_Configuration
{
    uchar OIL_compensation[30] ;
    uchar Water_compensation[30] ;
};


#endif // YWYTHREAD_H
