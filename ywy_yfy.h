#ifndef YWY_YFY_H
#define YWY_YFY_H

#include <QThread>
#include<iostream>
#include<math.h>

class ywy_yfy_thread :public QThread
{
    Q_OBJECT
public:
    explicit ywy_yfy_thread(QObject *parent = 0);
    void run();

private:
    void Data_Handle_yfy();
    char getoilkingcode(char add);
    QString GetIEEEHex_Inventory_signal(uchar add,char oilkind);
    char* package_Inventory_signal(char add,char oilkind);

    QString GetIEEEHex_Inventory_all(char sum);
    char* package_Inventory_all(char sum);

    QString GetIEEEHex_alarm_signal(uchar add);
    char* package_alarm_signal(char add);

    QString GetIEEEHex_alarm_all(char sum);
    char* package_alarm_all(char sum);
};

uint checksum16(char *p,char len);

union IEEEtoFloat{
    float f;
    char buf[4];
};

extern union IEEEtoFloat Reply_Data_OT[15][8];


#endif // YWY_YFY_H
