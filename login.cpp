#include "login.h"
#include "ui_login.h"
#include<QDialog>
#include<stdio.h>       //sprintf
#include<stdint.h>
#include<unistd.h>
#include<fcntl.h>       //open
#include"config.h"
#include"database_op.h"


login::login(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::login)
{
    ui->setupUi(this);
    Flag_Timeto_CloseNeeded[0] = 1;
    this->setAttribute(Qt::WA_DeleteOnClose,true);
  //  setAttribute(Qt::WA_DeleteOnClose); //关闭变量之后立即释放
    setWindowFlags(Qt::Tool|Qt::WindowStaysOnTopHint|Qt::FramelessWindowHint);
  //  setWindowFlags(windowFlags()&~Qt::WindowCloseButtonHint&~Qt::WindowContextHelpButtonHint);   //去掉对话框帮助按钮
    ui->label->setHidden(1);
    ui->lineEdit->installEventFilter(this);
    ui->lineEdit_2->installEventFilter(this);//安装eventfilter
    touchkey = new keyboard;
    connect(this,SIGNAL(closeing_touchkey()),touchkey,SLOT(onEnter()));

    QTextCodec *codec = QTextCodec::codecForName("UTF-8");
    QString text_temp;
    QByteArray usr = "点击输入用户名";
    QByteArray passwd_ = "请输入六位密码";
    text_temp = codec->toUnicode(usr);
    ui->lineEdit->setPlaceholderText(text_temp);
    ui->lineEdit->setMaxLength(9);

    text_temp = codec->toUnicode(passwd_);
    ui->lineEdit_2->setPlaceholderText(text_temp);
    ui->lineEdit_2->setMaxLength(6);
    ui->lineEdit_2->setEchoMode(QLineEdit::Password);
    ui->lineEdit->setText("admin");
    ui->label_log_in->setHidden(1);
}

login::~login()
{
    delete ui;
}

void login::on_pushButton_clicked()             //enter
{
    //密码验证
    char *username;     //提取用户输入
    char *passwd;
	char username_temp[9] = {0};   //username 与路径合成，打开文本文件
	char passwd_temp[7] = {0};
    unsigned char flag_right = 0;

    //提取输入的用户名
    QByteArray Q_user = ui->lineEdit->text().toLatin1();
    username = Q_user.data();

    //后门
    //flag_right = 3;
//    sprintf(username_temp,"%s%s",USER_USER,username);
//    if(strcmp(username_temp,"/opt/manager/") == 0)        //s1<s2   <0
//    {
//            Flag_Timeto_CloseNeeded[0] = 0;
//        emit closeing_touchkey();
//        close();
//        emit login_enter(3);
//        return;
//    }

    //密码验证
    //验证是否为用户帐号
    sprintf(username_temp,"%s%s",USER_USER,username);
    if(strcmp(username_temp,"/opt/manager/") != 0)
    {

         int fd;
         fd = ::open(username_temp,O_RDONLY);
         if(fd == -1)
         {
             perror("manager file open");
         }
         else
         {
             printf("S1 ok~\n");
             memset(passwd_temp,0,sizeof(char)*6);
             read(fd,passwd_temp,6);
             printf("%s\n",passwd_temp);
             QByteArray Q_passwd = ui->lineEdit_2->text().toLatin1();
             passwd = Q_passwd.data();
             if(!strcmp(passwd,passwd_temp))
             {
                 printf("login success!\n");
                 //history_operate_write_detail(ui->lineEdit->text(),"登录系统");
                 add_value_operateinfo(ui->lineEdit->text(),"登录系统");
                 flag_right = 2;
             }
         }
         ::close(fd);
    }
    //验证是否为管理员帐号
    sprintf(username_temp,"%s%s",USER_MANAGER,username);
    if(strcmp(username_temp,USER_MANAGER) != 0)
    {
        int fd;
        fd = ::open(username_temp,O_RDONLY);
        if(fd == -1)
        {
            perror("manager file open");
        }
        else
        {
            printf("SU  S1 ok!\n");
            memset(passwd_temp,0,6);
            read(fd,passwd_temp,6);
            printf("%s\n",passwd_temp);
            QByteArray Q_passwd = ui->lineEdit_2->text().toLatin1();
            passwd = Q_passwd.data();
            if(!strcmp(passwd,passwd_temp))
            {
                printf("SU login success!\n");
                //history_operate_write_detail(ui->lineEdit->text(),"登录系统");
                add_value_operateinfo(ui->lineEdit->text(),"登录系统");
                flag_right = 1;
            }
        }
        ::close(fd);
    }
    //验证是否为后门帐号
    sprintf(username_temp,"%s%s","/opt/manager/SU/ALP/",username);
    if(strcmp(username_temp,"/opt/manager/SU/ALP/") != 0)
    {
		qDebug()<<username;
		qDebug()<<username_temp;
        int fd;
        fd = ::open(username_temp,O_RDONLY);
        if(fd == -1)
        {
            perror("manager file open");
        }
        else
        {
            printf("alp  S1 ok!\n");
            memset(passwd_temp,0,6);
            read(fd,passwd_temp,6);
            printf("%s\n",passwd_temp);
            QByteArray Q_passwd = ui->lineEdit_2->text().toLatin1();
            passwd = Q_passwd.data();
			qDebug()<<passwd_temp;
			qDebug()<<Q_passwd;
            if(!strcmp(passwd,passwd_temp))
            {
                printf("alp login success!\n");
                flag_right = 3;
            }
        }
        ::close(fd);
    }
    if(flag_right)
    {
        ui->label_log_in->setHidden(0);
        ui->label->setHidden(1);
        qApp->processEvents();
        Flag_Timeto_CloseNeeded[0] = 0;
        emit closeing_touchkey();
        emit login_enter(flag_right);         //flag_right = 1为管理员登录
                                              //flag_right = 2为用户登录
                                              //flag_right = 3为服务帐号
		usleep(10000);
        emit disp_for_managerid(ui->lineEdit->text());
        close();
    }
    else
    {
        ui->label_log_in->setHidden(1);
        ui->label->setHidden(0); //显示密码错误
        qApp->processEvents();
    }
}

void login::on_pushButton_2_clicked()
{
    Flag_Timeto_CloseNeeded[0] = 0;
    emit closeing_touchkey();
    emit mainwindow_enable();
    ui->lineEdit->clear();
    ui->lineEdit_2->clear();
    close();
}

bool login::eventFilter(QObject *watched, QEvent *event)
{
    if(watched == ui->lineEdit)
    {
        if(event->type()==QEvent::MouseButtonPress)
        {
            ui->lineEdit_2->clear();
            emit closeing_touchkey();
            touchkey = new keyboard;
            connect(touchkey->signalMapper,SIGNAL(mapped(const QString&)),this,SLOT(setDispText(const QString&)));
            connect(touchkey,SIGNAL(display_backspace()),this,SLOT(setDispBackspace()));
            connect(this,SIGNAL(closeing_touchkey()),touchkey,SLOT(onEnter()));
            touchkey->show();
            return true;  //去掉会导致光标位置不在最后，实际位置为触发事件点击处
        }
    }
    if(watched == ui->lineEdit_2)
    {
        if(event->type()==QEvent::MouseButtonPress)
        {
            emit closeing_touchkey();
            touchkey = new keyboard;
            connect(touchkey->signalMapper,SIGNAL(mapped(const QString&)),this,SLOT(setDispText_Line_2(const QString&)));
            connect(touchkey,SIGNAL(display_backspace()),this,SLOT(setDispBackspace_2()));
            connect(this,SIGNAL(closeing_touchkey()),touchkey,SLOT(onEnter()));
            touchkey->show();
            return true;  //去掉会导致光标位置不在最后，实际位置为触发事件点击处

        }
    }

    return QObject::eventFilter(watched,event);
}

void login::setDispText(const QString &text)
{
    ui->lineEdit->insert(text);
}
void login::setDispText_Line_2(const QString &text)
{
    ui->lineEdit_2->insert(text);
}
void login::setDispBackspace()
{
    ui->lineEdit->backspace();
}
void login::setDispBackspace_2()
{
    ui->lineEdit_2->backspace();
}













