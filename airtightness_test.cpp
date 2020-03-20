#include "airtightness_test.h"
#include "ui_airtightness_test.h"
#include "config.h"
#include <QTimer>
#include <stdlib.h>

QTimer *time_500ms;
float OilGasPlace = 0;//当前油气空间
unsigned int Time_Num = 0;//时间计数
unsigned int Time_Countdown = 300;//5分钟
QString pre_show = "";//显示用的qstring
float PressureStart = 0; //开始时的压力
float PressureStop = 0;  //结束时的压力
unsigned int gun_num = 0;//当前油枪总数
unsigned char Sensor_Num = 0;//Pre[sensor_num]的压力值，方便修改
unsigned char Select_Gun = 0;//选择油枪数量范围
unsigned char Flag_TextStart = 0;//开始测试标志位
float PreDeviation = 0;//初始压力相对于500Pa的偏差
//剩余压力数组1~6	 7~12	13~18	19~24	>24
float ResidualPressure[5][33] = {{182,199,217,232,244,257,267,277,286,294,301,329,349,364,376,389,396,404,411,416,421,431,438,446,451,458,463,468,471,473,481,486,488},
                                 {172,189,204,219,234,244,257,267,277,284,294,319,341,356,371,381,391,399,406,411,418,428,436,443,448,456,461,466,471,473,481,486,488},
                                 {162,179,194,209,224,234,247,257,267,277,284,311,334,351,364,376,386,394,401,409,414,423,433,441,446,453,461,463,468,471,481,483,488},
                                 {152,169,184,199,214,227,237,249,257,267,274,304,326,344,359,371,381,389,396,404,409,421,428,436,443,451,458,463,466,468,478,483,486},
                                 {142,159,177,192,204,217,229,239,249,259,267,296,319,336,351,364,376,384,391,399,404,416,426,433,441,448,456,461,466,468,478,483,486}};
//油气空间数组
float OilGasArray[33] = {1893,2082,2271,2460,2650,2839,3028,3217,3407,3596,3785,4542,5299,6056,6813,7570,8327,9084,9841,10598,11355,13248,15140,17033,18925,22710,26495,30280,34065,37850,56775,75700,94625};
float PressureLimit = 0;//计算分析得来的压力限值

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
	connect(time_500ms,SIGNAL(timeout()),this,SLOT(delay_500ms()));
	time_500ms->start();

	ui->lineEdit_oilgas_space->installEventFilter(this);
	ui->pushButton_start_text->setEnabled(0);  //开始时测试按钮不可点
	ui->label_oilgas_space->setText("0");      //初始油气空间显示0
	ui->label_result_text->setText("待测试");   //检测结果显示为待检测
	ui->label_show_live->setHidden(1);         //测试中……隐藏
	ui->lcdNumber_pre_now->setDecMode();       //lcd显示为十进制
	ui->lcdNumber_pre_start->setDecMode();     //lcd显示为十进制
	ui->lcdNumber_pre_stop->setDecMode();      //lcd显示为十进制
	ui->lcdNumber_pre_now->display(0);
	ui->lcdNumber_pre_start->display(0);
	ui->lcdNumber_pre_stop->display(0);
	ui->textEdit_pre_his->document ()->setMaximumBlockCount (600);//最大行数

	//数据初始化
	gun_num = Amount_Gasgun[0]+Amount_Gasgun[1]+Amount_Gasgun[2]+Amount_Gasgun[3]+Amount_Gasgun[4]+Amount_Gasgun[5]
	                      +Amount_Gasgun[6]+Amount_Gasgun[7]+Amount_Gasgun[8]+Amount_Gasgun[9]+Amount_Gasgun[10]+Amount_Gasgun[11];
	if((gun_num>=1)&&(gun_num<=6)){Select_Gun = 0;}
	if((gun_num>=7)&&(gun_num<=12)){Select_Gun = 1;}
	if((gun_num>=13)&&(gun_num<=18)){Select_Gun = 2;}
	if((gun_num>=19)&&(gun_num<=24)){Select_Gun = 3;}
	if((gun_num>24)){Select_Gun = 4;}
	OilGasPlace = 0;//当前油气空间
	Time_Num = 0;//时间计数
	pre_show = "";//显示用的qstring
	PressureStart = 0; //开始时的压力
	PressureStop = 0;  //结束时的压力
	Select_Gun = 0;//选择油枪数量范围
	Flag_TextStart = 0;//开始测试标志位
	PressureStart = 0;//初始压力差值
	ui->label_oilgas_space-> setText(QString::number(OilGasPlace));
	ui->label_oilgas_space->setStyleSheet("color:black");
	ui->label_pre_limit->setText("");//压力限值label空
	ui->label_PreOver->setHidden(1);
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
	if(OilGasPlace > 0)  //油气空间设置合格后才能开始
	{
		if((OilGasPlace < OilGasArray[0])||(OilGasPlace > OilGasArray[32]))//如果输入的值比表中的最小值还要小或者比最大值还要大
		{
			ui->pushButton_start_text->setEnabled(0);
			ui->label_oilgas_space-> setText("应大于1893");
			ui->label_oilgas_space->setStyleSheet("color:red");
		}
		else
		{
			ui->label_oilgas_space->setText(QString::number(OilGasPlace));
			ui->label_oilgas_space->setStyleSheet("color:black");
			ui->pushButton_start_text->setEnabled(1);
		}
	}
	calc_pre_limit();//计算剩余压力限值
	emit closeing_touchkey();
}

void Airtightness_Test::on_pushButton_start_text_clicked() //开始测试
{
	if(Flag_TextStart == 0)
	{
		PressureStart = Pre[Sensor_Num]*1000;
		//获取压力偏差；相对于500pa
		PreDeviation = PressureStart-500;//计算开始压力的偏差，用于修正最小限值
		if(abs(PreDeviation)>100) //压力偏差太大
		{
			ui->label_PreOver->setHidden(0);
		}
		else
		{
			ui->label_PreOver->setHidden(1);
			Flag_TextStart = 1;
			pre_show = QString::number(PressureStart,'f',1);
			ui->lcdNumber_pre_start->display(pre_show);
			ui->pushButton_start_text->setText("停止测试");
			ui->label_show_live->setText("测试中……");
			ui->label_result_text->setText("待测试");
			ui->label_show_live->setStyleSheet("color:red");
			ui->label_result_text->setStyleSheet("color:black");
			ui->lcdNumber_pre_stop->display(0);
		}
	}
	else
	{
		PreDeviation = 0;//初始压力值与500pa的差值
		Flag_TextStart = 0;
		ui->pushButton_start_text->setText("开始测试");
		Time_Num = 0;
		Time_Countdown = 300;
		PressureStop = Pre[Sensor_Num]*1000;
		ui->lcdNumber_pre_stop->display(QString::number(PressureStop,'f',1));
		ui->label_show_live->setText("测试结束");
		ui->label_show_live->setHidden(0);
		ui->label_result_text->setText("测试未完成");
		ui->label_result_text->setStyleSheet("color:red");
	}
}

void Airtightness_Test::delay_500ms()
{
	if(Flag_TextStart == 1)//测试开始了
	{
		QString His_Show = "";
		Time_Num++;
		pre_show = QString::number(Pre[Sensor_Num]*1000,'f',1);
		ui->lcdNumber_pre_now->display(pre_show);
		if(Time_Num > 2400)  //1200S  10分钟  防止溢出
		{
			Time_Num = 0;
		}
		if(Time_Num%2) //测试中闪烁
		{
			if(Time_Countdown == 0){Time_Countdown = 1;}
			Time_Countdown--;
			ui->label_show_live->setHidden(1);
			ui->label_run_time->setText(QString::number(Time_Countdown).append(" 秒后结束"));
		}
		else
		{
			ui->label_show_live->setHidden(0);
		}
		if(Time_Num%6 == 0) //3秒一次,历史记录
		{
			QDateTime date_time = QDateTime::currentDateTime();
			His_Show = date_time.toString("yyyy-MM-dd hh:mm:ss    ");
			His_Show.append(QString::number((Pre[Sensor_Num]*1000),'f',2).append("Pa"));
			ui->textEdit_pre_his->append(His_Show);
		}
		if(Time_Num > 600) //5分钟结束检测
		{
			Time_Countdown = 300;
			//time_500ms->stop();//停止计时
			Flag_TextStart = 0;//停止测试
			ui->pushButton_start_text->setText("开始测试");
			Time_Num = 0;
			PressureStop = Pre[Sensor_Num]*1000;
			ui->lcdNumber_pre_stop->display(QString::number(PressureStop,'f',1));
			ui->label_show_live->setText("测试结束");
			ui->label_show_live->setHidden(0);
			data_analysis();
		}
	}
	else//只显示当前压力值
	{
		pre_show = QString::number(Pre[Sensor_Num]*1000,'f',1);
		ui->lcdNumber_pre_now->display(pre_show);
	}
}

void Airtightness_Test::on_pushButton_clean_his_clicked() //清空历史记录
{
	ui->textEdit_pre_his->setText("");
}

//气密性数据分析计算
void Airtightness_Test::data_analysis()
{
	float PreLimitNow = 0;
	//限值修正，当初始值比500pa大时，按0.2倍修正，小于500pa时，按0.4倍修正
	if(PreDeviation<0)//负数
	{
		PreLimitNow = PressureLimit + PreDeviation*0.4;
	}
	else//正数
	{
		PreLimitNow = PressureLimit + PreDeviation*0.2;
	}
	qDebug()<<PreLimitNow;
	if(PressureStop >= PreLimitNow) //合格  对参数根据初始值按照0.6倍进行了修正
	{
		ui->label_result_text->setText("测试合格");
		ui->label_result_text->setStyleSheet("color:black");
	}
	else
	{
		ui->label_result_text->setText("测试不合格");
		ui->label_result_text->setStyleSheet("color:red");
	}
}
//计算压力限值
void Airtightness_Test::calc_pre_limit()
{
	unsigned char flag_mode = 0;
	//第一种情况，如果能在表中查到信息
	for(unsigned int i = 0;i < 33;i++)
	{
		if(abs(OilGasPlace - OilGasArray[i])<0.001) //可以查表判断是否合格；
		{
			qDebug()<<"chabiao"<<OilGasArray[i];
			PressureLimit = ResidualPressure[Select_Gun][i];
			ui->label_pre_limit->setText(QString::number(PressureLimit,'f',1).prepend("参考压力限值 ").append("Pa"));
			flag_mode = 1;
			break;
		}
	}
	//第二种情况，在表中查不到相关信息，需要使用内插公式计算
	if(flag_mode == 0)
	{
		for(unsigned int i = 0;i<33;i++)
		{
			if(OilGasPlace < OilGasArray[i]) //找到n+1号油气空间
			{
				qDebug()<<OilGasArray[i-1]<<OilGasArray[i];
				if(i == 0)//如果油气空间比最小值还小，则不进行测试
				{
				}
				else
				{
					PressureLimit = (OilGasPlace-OilGasArray[i-1])*(ResidualPressure[Select_Gun][i]-ResidualPressure[Select_Gun][i-1])/(OilGasArray[i]-OilGasArray[i-1])+ResidualPressure[Select_Gun][i-1];
					ui->label_pre_limit->setText(QString::number(PressureLimit,'f',1).prepend("参考压力限值 ").append("Pa"));
				}
				break;
			}
		}
	}
}



