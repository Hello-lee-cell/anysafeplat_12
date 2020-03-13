#ifndef LOGIN_H
#define LOGIN_H

#include <QDialog>
#include"keyboard.h"
namespace Ui {
class login;
}

class login : public QDialog
{
    Q_OBJECT

public:
    explicit login(QWidget *parent = 0);
    ~login();

private:
    Ui::login *ui;
    keyboard *touchkey;
signals:
    void login_enter(int t);
    void mainwindow_enable();
    void closeing_touchkey();
    void disp_for_managerid(const QString &);
    void logging_in();

private slots:
    void on_pushButton_clicked();
    void on_pushButton_2_clicked();

    bool eventFilter(QObject *, QEvent *);

    void setDispText(const QString& text);
    void setDispText_Line_2(const QString& text);

    void setDispBackspace();
    void setDispBackspace_2();
};

#endif // LOGIN_H
