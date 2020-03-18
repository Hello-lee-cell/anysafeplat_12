#ifndef AIRTIGHTNESS_TEST_H
#define AIRTIGHTNESS_TEST_H

#include <QMainWindow>
#include "keyboard.h"

namespace Ui {
class Airtightness_Test;
}

class Airtightness_Test : public QMainWindow
{
	Q_OBJECT

public:
	explicit Airtightness_Test(QWidget *parent = 0);
	~Airtightness_Test();

private slots:
	void on_pushButton_oilgas_space_clicked();
	void on_pushButton_start_text_clicked();
	void delay_500ms();
	void setText_OilGasSpace(const QString &text);
	void setBackspace_OilGasSpace();

	void on_pushButton_clean_his_clicked();

private:
	Ui::Airtightness_Test *ui;
	keyboard *touchkey;
	bool eventFilter(QObject *, QEvent *);

signals:
	void closeing_touchkey();
};

#endif // AIRTIGHTNESS_TEST_H
