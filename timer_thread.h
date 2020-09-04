#ifndef TIMER_THREAD_H
#define TIMER_THREAD_H
#include <QTimer>
#include <QThread>

class timer_thread :public QThread
{
	Q_OBJECT
public:
	bool stop;
	explicit timer_thread(QObject *parent = 0);
	void run();
signals:
	void delay_500ms();
	void delay_1000ms();
private slots:
	void delay_time();
};

#endif // TIMER_THREAD_H
