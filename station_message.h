#ifndef STATION_MESSAGE_H
#define STATION_MESSAGE_H

#include <QDialog>

namespace Ui {
class station_message;
}

class station_message : public QDialog
{
	Q_OBJECT

public:
	explicit station_message(QWidget *parent = 0);
	~station_message();

private slots:
	void on_pushButton_close_clicked();

	void on_pushButton_queren_clicked();

	void on_pushButton_message_clicked();

	void on_pushButton_hide_message_clicked();

	void on_pushButton_WarnEmptyQuit_clicked();

private:
	Ui::station_message *ui;
	void init_show();
signals:
	void SendStationMessage();
};

#endif // STATION_MESSAGE_H
