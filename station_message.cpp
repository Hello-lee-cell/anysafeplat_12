#include "station_message.h"
#include "ui_station_message.h"
#include "config.h"
#include <QSettings>

station_message::station_message(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::station_message)
{
	ui->setupUi(this);
	setWindowModality(Qt::WindowModal);
	this->setAttribute(Qt::WA_DeleteOnClose,true);
	setWindowFlags(Qt::FramelessWindowHint);
	setAttribute(Qt::WA_TranslucentBackground,true);    //窗体透明
	ui->widget_message->setHidden(1);
	ui->widget_WarnEmpty->setHidden(1);
	init_show();
	move(0,85);
}

station_message::~station_message()
{
	delete ui;
}

void station_message::on_pushButton_close_clicked()
{
	close();
}


void station_message::on_pushButton_queren_clicked()
{
	unsigned char flag_ifEmpty = 0;
	QString CityCode,AreaCode,TownCode,StationName,Company,Lon,Lat,Address,Contact,Phone,JYJNum,JYQNum,
			Scale,OwnerType,HasSystem,Manufacturer,IsAcceptance,OperateStaff, JYJID,JYQID;
	CityCode = ui->lineEdit_CityCode->text();
	AreaCode = ui->lineEdit_AreaCode->text();
	TownCode = ui->lineEdit_TownCode->text();
	StationName = ui->lineEdit_StationName->text();
	Company = ui->lineEdit_Company->text();
	Lon = ui->lineEdit_Lon->text();
	Lat = ui->lineEdit_Lat->text();
	Address = ui->lineEdit_Address->text();
	Contact = ui->lineEdit_Contact->text();
	Phone = ui->lineEdit_Phone->text();
	JYJNum = ui->lineEdit_JYJNum->text();
	JYQNum = ui->lineEdit_JYQNum->text();
	Scale = ui->lineEdit_Scale->text();
	OwnerType = ui->lineEdit_OwnerType->text();
	HasSystem = ui->lineEdit_HasSystem->text();
	Manufacturer = ui->lineEdit_Manufacturer->text();
	IsAcceptance = ui->lineEdit_IsAcceptance->text();
	OperateStaff = ui->lineEdit_OperateStaff->text();

	JYJID = ui->lineEdit_JYJID->text();
	JYQID = ui->lineEdit_JYQID->text();

	if(CityCode.isEmpty()){ui->lineEdit_CityCode->setPlaceholderText("该项不能为空！！！");flag_ifEmpty = 1;}
	if(AreaCode.isEmpty()){ui->lineEdit_AreaCode->setPlaceholderText("该项不能为空！！！");flag_ifEmpty = 1;}
	//if(TownCode.isEmpty()){ui->lineEdit_TownCode->setPlaceholderText("该项不能为空！！！");flag_ifEmpty = 1;}
	if(StationName.isEmpty()){ui->lineEdit_StationName->setPlaceholderText("该项不能为空！！！");flag_ifEmpty = 1;}
	if(Company.isEmpty()){ui->lineEdit_Company->setPlaceholderText("该项不能为空！！！");flag_ifEmpty = 1;}
	if(Lon.isEmpty()){ui->lineEdit_Lon->setPlaceholderText("该项不能为空！！！");flag_ifEmpty = 1;}
	if(Lat.isEmpty()){ui->lineEdit_Lat->setPlaceholderText("该项不能为空！！！");flag_ifEmpty = 1;}
	if(Address.isEmpty()){ui->lineEdit_Address->setPlaceholderText("该项不能为空！！！");flag_ifEmpty = 1;}
	if(Contact.isEmpty()){ui->lineEdit_Contact->setPlaceholderText("该项不能为空！！！");flag_ifEmpty = 1;}
	if(Phone.isEmpty()){ui->lineEdit_Phone->setPlaceholderText("该项不能为空！！！");flag_ifEmpty = 1;}
	if(JYJNum.isEmpty()){ui->lineEdit_JYJNum->setPlaceholderText("该项不能为空！！！");flag_ifEmpty = 1;}
	if(JYQNum.isEmpty()){ui->lineEdit_JYQNum->setPlaceholderText("该项不能为空！！！");flag_ifEmpty = 1;}
	if(Scale.isEmpty()){ui->lineEdit_Scale->setPlaceholderText("该项不能为空！！！");flag_ifEmpty = 1;}
	if(OwnerType.isEmpty()){ui->lineEdit_OwnerType->setPlaceholderText("该项不能为空！！！");flag_ifEmpty = 1;}
	if(HasSystem.isEmpty()){ui->lineEdit_HasSystem->setPlaceholderText("该项不能为空！！！");flag_ifEmpty = 1;}
	//if(Manufacturer.isEmpty()){ui->lineEdit_Manufacturer->setPlaceholderText("该项不能为空！！！");flag_ifEmpty = 1;}
	//if(IsAcceptance.isEmpty()){ui->lineEdit_IsAcceptance->setPlaceholderText("该项不能为空！！！");flag_ifEmpty = 1;}
	//if(OperateStaff.isEmpty()){ui->lineEdit_OperateStaff->setPlaceholderText("该项不能为空！！！");flag_ifEmpty = 1;}

	if(JYJID.isEmpty()){ui->lineEdit_JYJID->setPlaceholderText("该项不能为空！！！");flag_ifEmpty = 1;}
	if(JYQID.isEmpty()){ui->lineEdit_JYQID->setPlaceholderText("该项不能为空！！！");flag_ifEmpty = 1;}

	if(flag_ifEmpty == 1)
	{
		ui->widget_WarnEmpty->setHidden(0);
	}
	else
	{
		//将信息写入文件ini
		QSettings *configIniWrite = new QSettings("/opt/StationMessage.ini", QSettings::IniFormat);
		//向ini文件中写入内容,setValue函数的两个参数是键值对
		configIniWrite->setValue("StationMessage/CityCode", CityCode);
		configIniWrite->setValue("StationMessage/AreaCode", AreaCode);
		configIniWrite->setValue("StationMessage/TownCode", TownCode);
		configIniWrite->setValue("StationMessage/StationName", StationName);
		configIniWrite->setValue("StationMessage/Company", Company);
		configIniWrite->setValue("StationMessage/Lon", Lon);
		configIniWrite->setValue("StationMessage/Lat", Lat);
		configIniWrite->setValue("StationMessage/Address", Address);
		configIniWrite->setValue("StationMessage/Contact", Contact);
		configIniWrite->setValue("StationMessage/Phone", Phone);
		configIniWrite->setValue("StationMessage/JYJNum", JYJNum);
		configIniWrite->setValue("StationMessage/JYQNum", JYQNum);
		configIniWrite->setValue("StationMessage/Scale", Scale);
		configIniWrite->setValue("StationMessage/OwnerType", OwnerType);
		configIniWrite->setValue("StationMessage/HasSystem", HasSystem);
		configIniWrite->setValue("StationMessage/Manufacturer", Manufacturer);
		configIniWrite->setValue("StationMessage/IsAcceptance", IsAcceptance);
		configIniWrite->setValue("StationMessage/OperateStaff", OperateStaff);

		configIniWrite->setValue("JYJMessage/JYJID", JYJID);
		configIniWrite->setValue("JYJMessage/JYQID", JYQID);
		//写入完成后删除指针
		delete configIniWrite;
		if(Flag_Network_Send_Version == 6)
		{
			emit SendStationMessage();
		}
		close();
	}

}

void station_message::on_pushButton_message_clicked()
{
	ui->widget_message->setHidden(0);
}

void station_message::on_pushButton_hide_message_clicked()
{
	ui->widget_message->setHidden(1);
}
void station_message::init_show()
{
	QString CityCode,AreaCode,TownCode,StationName,Company,Lon,Lat,Address,Contact,Phone,JYJNum,JYQNum,
			Scale,OwnerType,HasSystem,Manufacturer,IsAcceptance,OperateStaff, JYJID,JYQID;

	QSettings *configIniRead = new QSettings("/opt/StationMessage.ini", QSettings::IniFormat);
	//将读取到的ini文件保存在QString中，先取值，然后通过toString()函数转换成QString类型
	CityCode = configIniRead->value("StationMessage/CityCode").toString();
	AreaCode = configIniRead->value("StationMessage/AreaCode").toString();
	TownCode = configIniRead->value("StationMessage/TownCode").toString();
	StationName = configIniRead->value("StationMessage/StationName").toString();
	Company = configIniRead->value("StationMessage/Company").toString();
	Lon = configIniRead->value("StationMessage/Lon").toString();
	Lat = configIniRead->value("StationMessage/Lat").toString();
	Address = configIniRead->value("StationMessage/Address").toString();
	Contact = configIniRead->value("StationMessage/Contact").toString();
	Phone = configIniRead->value("StationMessage/Phone").toString();
	JYJNum = configIniRead->value("StationMessage/JYJNum").toString();
	JYQNum = configIniRead->value("StationMessage/JYQNum").toString();
	Scale = configIniRead->value("StationMessage/Scale").toString();
	OwnerType = configIniRead->value("StationMessage/OwnerType").toString();
	HasSystem = configIniRead->value("StationMessage/HasSystem").toString();
	Manufacturer = configIniRead->value("StationMessage/Manufacturer").toString();
	IsAcceptance = configIniRead->value("StationMessage/IsAcceptance").toString();
	OperateStaff = configIniRead->value("StationMessage/OperateStaff").toString();

	JYJID = configIniRead->value("JYJMessage/JYJID").toString();
	JYQID = configIniRead->value("JYJMessage/JYQID").toString();
	//读入入完成后删除指针
	delete configIniRead;
	ui->lineEdit_CityCode->setText(CityCode);
	ui->lineEdit_AreaCode->setText(AreaCode);
	ui->lineEdit_TownCode->setText(TownCode);
	ui->lineEdit_StationName->setText(StationName);
	ui->lineEdit_Company->setText(Company);
	ui->lineEdit_Lon->setText(Lon);
	ui->lineEdit_Lat->setText(Lat);
	ui->lineEdit_Address->setText(Address);
	ui->lineEdit_Contact->setText(Contact);
	ui->lineEdit_Phone->setText(Phone);
	ui->lineEdit_JYJNum->setText(JYJNum);
	ui->lineEdit_JYQNum->setText(JYQNum);
	ui->lineEdit_Scale->setText(Scale);
	ui->lineEdit_OwnerType->setText(OwnerType);
	ui->lineEdit_HasSystem->setText(HasSystem);
	ui->lineEdit_Manufacturer->setText(Manufacturer);
	ui->lineEdit_IsAcceptance->setText(IsAcceptance);
	ui->lineEdit_OperateStaff->setText(OperateStaff);

	ui->lineEdit_JYJID->setText(JYJID);
	ui->lineEdit_JYQID->setText(JYQID);
}

void station_message::on_pushButton_WarnEmptyQuit_clicked()
{
	ui->widget_WarnEmpty->setHidden(1);
}
