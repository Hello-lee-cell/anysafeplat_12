#ifndef IIE_THREAD_H
#define IIE_THREAD_H
#include <QThread>

class IIE_thread :public QThread
{
	Q_OBJECT
public:
	explicit IIE_thread(QObject *parent = 0);
	void run();
	void stop();

private:
	volatile bool is_runnable = true;
	void uart_init();
	void ask_psa2();
	void ask_IIE();
	void ask_valve();
	void send_data();
signals:

};

#endif // IIE_THREAD_H
