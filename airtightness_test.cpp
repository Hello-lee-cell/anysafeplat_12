#include "airtightness_test.h"
#include "ui_airtightness_test.h"
#include "config.h"
#include <QTimer>
QTimer *time_500ms;
float OilGasPlace = 0;//当前油气空间
unsigned int Time_Num = 0;//时间计数
QString pre_show = "";//显示用的qstring

Airtightness_Test::Airtightness_Test(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Airtightness_Test)
{

	this->setWindowFlags(Qt::WindowStaysOnTopHint);
	ui->setupUi(this);
	this->setAttribute(Qt::WA_DeleteOnClose,true);//立即释放缓存
	move(120,10);

	time_500ms = new QTimer();
	time_500ms->setInterval(500);
	//time_500ms->start();
	connect(time_500ms,SIGNAL(timeout()),this,SLOT(delay_500ms()));

	ui->lineEdit_oilgas_space->installEventFilter(this);
	ui->pushButton_start_text->setEnabled(0); //开始时测试按钮不可点
	ui->label_oilgas_space->setText("0");      //初始油气空间显示0
	ui->label_result_text->setText("待测试");   //检测结果显示为待检测
	ui->label_show_live->setHidden(1);         //测试中……隐藏
	ui->lcdNumber_pre_now->setDecMode();      //lcd显示为十进制
	ui->lcdNumber_pre_start->setDecMode();      //lcd显示为十进制
	ui->lcdNumber_pre_stop->setDecMode();      //lcd显示为十进制
	ui->lcdNumber_pre_now->display(0);
	ui->lcdNumber_pre_start->display(0);
	ui->lcdNumber_pre_stop->display(0);
	ui->textEdit_pre_his->document ()->setMaximumBlockCount (600);//最大行数
}

//输入相关
bool Airtightness_Test::eventFilter(QObject *watch, QEvent *event)
{
	if(watch == ui->lineEdit_oilgas_space)
	{
		if(event->type()==QEvent::MouseButtonPress)
		{
			emit closeing_touchkey();
			touchkey = new keyboard;
			connect(touchkey->signalMapper,SIGNAL(mapped(const QString&)),this,SLOT(setText_OilGasSpace(const QString&)));
			connect(touchkey,SIGNAL(display_backspace()),this,SLOT(setBackspace_OilGasSpace()));
			connect(this,SIGNAL(closeing_touchkey()),touchkey,SLOT(onEnter()));
			touchkey->show();
			return true;
		}
	}
	return QWidget::eventFilter(watch,event);
}
void Airtightness_Test::setText_OilGasSpace(const QString &text)
{
	ui->lineEdit_oilgas_space->insert(text);
	qDebug()<< "shurushurushrurhsur";
}

void Airtightness_Test::setBackspace_OilGasSpace()
{
	ui->lineEdit_oilgas_space->backspace();
}

Airtightness_Test::~Airtightness_Test()
{
	delete ui;
}

void Airtightness_Test::on_pushButton_oilgas_space_clicked()
{
	OilGasPlace = ui->lineEdit_oilgas_space->text().toFloat();
	ui->label_oilgas_space->setText(QString::number(OilGasPlace));
	if(OilGasPlace > 0)  //油气空间设置合格后才能开始
	{
		ui->pushButton_start_text->setEnabled(1);
	}
}

void Airtightness_Test::on_pushButton_start_text_clicked() //开始测试
{
	if(Time_Num==0)
	{
		time_500ms->start(); //开始计时
	}
	else
	{

	}
	pre_show = QString::number(Pre[0]*1000,'f',1);
	ui->lcdNumber_pre_start->display(pre_show);
}

void Airtightness_Test::delay_500ms()
{
	QString His_Show = "";
	Time_Num++;
	pre_show = QString::number(Pre[0]*1000,'f',1);
	ui->lcdNumber_pre_now->display(pre_show);
	if(Time_Num > 2400)  //1200S  10分钟  防止溢出
	{
		Time_Num = 0;
	}
	if(Time_Num%2) //测试中闪烁
	{
		ui->label_show_live->setHidden(1);
		ui->label_run_time->setText(QString::number(Time_Num/2));
	}
	else
	{
		ui->label_show_live->setHidden(0);
	}
	if(Time_Num%6 == 0) //两秒一次
	{
		QDateTime date_time = QDateTime::currentDateTime();
		His_Show = date_time.toString("yyyy-MM-dd hh:mm:ss    ");
		His_Show.append(QString::number((Pre[0]*1000),'f',2).append("Pa"));
		ui->textEdit_pre_his->append(His_Show);
	}
}

void Airtightness_Test::on_pushButton_clean_his_clicked() //清空历史记录
{
	ui->textEdit_pre_his->setText("");
}
