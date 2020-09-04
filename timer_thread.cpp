#include "timer_thread.h"
#include "oilgas/fga1000_485.h"
#include <QDebug>

unsigned int jishu = 0;
timer_thread::timer_thread(QObject *parent):
    QThread(parent)
{

}
void timer_thread::run()
{

//	QTimer *delay_time = new QTimer(this);
//	delay_time->setInterval(499);
//	connect(delay_time,SIGNAL(timeout()),this,SLOT(delay_time()));
//	delay_time->start();

	while(1)
	{
		//QTimer::singleShot(500,this,SLOT(delay_time()));
		msleep(499);
		//qDebug()<<"timeouttimeouttimeouttimeouttimeout";
		delay_time();
	}

	this->exec();
	return;
}
void timer_thread::delay_time()
{
	emit delay_500ms();
	if(jishu%2)
	{
		emit delay_1000ms();
	}
	jishu++;
	if(jishu>=4){jishu = 0;}
}
