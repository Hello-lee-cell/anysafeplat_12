#include "warn.h"
#include "ui_warn.h"
#include <stdint.h>
#include <stdio.h>
#include <QDialog>
#include"mainwindow.h"
#include <time.h>
//#include"systemset.h"
#include"config.h"
warn::warn(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::warn)
{
    ui->setupUi(this);
    setWindowFlags(Qt::FramelessWindowHint|Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_DeleteOnClose); //关闭变量之后立即释放
}

warn::~warn()
{
    delete ui;
}
void warn::on_pushButton_clicked()
{
    system("udhcpc -i eth1 &");
    system("echo udhcpc -i eth1 '&' > /etc/init.d/S92udhcpc");
    system("sync");
    close();
/*
     emit history_ipautoset();
     //获取ip   写入默认配置文件
     const char *udhcpc_="udhcpc -i eth1 &";
     FILE *fp;
     fp = fopen("/etc/init.d/S92udhcpc","w+");
  //   system("chmod +x S92udhcpc");
     fwrite(udhcpc_,strlen(udhcpc_),1,fp);
     fclose(fp);
     ui->pushButton->setEnabled(0);
     ui->pushButton_2->setEnabled(0);
     system("reboot");
*/
}
void warn::on_pushButton_2_clicked()
{
    close();
}

void warn::on_pushButton_3_clicked()
{

    emit history_ipset();
    FILE *fp;
    fp = fopen("/etc/init.d/S92udhcpc","w+");
 //   system("chmod +x S92udhcpc");
    fwrite(ipstr,strlen(ipstr),1,fp);
    fclose(fp);
    ui->pushButton_3->setEnabled(0);
    ui->pushButton_2->setEnabled(0);
    system("reboot");
}
void warn::hide_warn_button_set(int t)
{
    switch(t)
    {
        case 1 : ui->pushButton_3->setEnabled(0);
                 ui->pushButton_3->setHidden(1);// button1 udhcpc  隐藏按钮3
                 ui->label_3->setHidden(1);
                 break;
        case 2 : ui->pushButton->setEnabled(0);
                 ui->pushButton->setHidden(1);
    }
}




