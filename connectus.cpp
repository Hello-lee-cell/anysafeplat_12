#include "connectus.h"
#include "ui_connectus.h"
#include"config.h"
connectus::connectus(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::connectus)
{
    ui->setupUi(this);
    this->setAttribute(Qt::WA_DeleteOnClose,true);
    Flag_Timeto_CloseNeeded[0] = 1;
    setWindowFlags(Qt::Tool|Qt::WindowStaysOnTopHint|Qt::FramelessWindowHint);
    move(0,73);
}

connectus::~connectus()
{
    delete ui;
}

void connectus::on_pushButton_clicked()
{
    Flag_Timeto_CloseNeeded[0] = 0;
    emit pushButton_connect_enable();
    close();
}
