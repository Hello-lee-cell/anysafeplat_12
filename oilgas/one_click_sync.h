#ifndef ONE_CLICK_SYNC_H
#define ONE_CLICK_SYNC_H

#include <QMainWindow>

namespace Ui {
class One_click_sync;
}

class One_click_sync : public QMainWindow
{
	Q_OBJECT

public:
	explicit One_click_sync(QWidget *parent = 0);
	~One_click_sync();


private slots:
	void on_pushButton_clicked();
	//同步带屏气液比采集器的脉冲当量
	void receive_factor_data(unsigned int idi,unsigned int idj,float oil_factor1,float gas_factor1,float oil_factor2,float gas_factor2);
	void flicker();
	void on_pushButton_2_clicked();

signals:

private:
	Ui::One_click_sync *ui;
	void show_data(unsigned int idi,unsigned int idj,float oil_factor1,float gas_factor1,float oil_factor2,float gas_factor2);
	void ask_data();
	void gun_num_calc();
};

#endif // ONE_CLICK_SYNC_H
