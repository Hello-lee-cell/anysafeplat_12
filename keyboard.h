#ifndef  KeyBoard_H
#define  KeyBoard_H

#include <QtGui>
#include <QSignalMapper>
#include "ui_keyboard.h"

enum {
    iMode_Normal = 0,
    iMode_Passwd = 1
};

namespace Ui {
    class keyboard;
}

class keyboard : public QDialog, Ui::keyboard
{
    Q_OBJECT

public:
     keyboard();
     ~keyboard();

  //   void mouseMoveEvent(QMouseEvent *);  //窗体移动
     void mousePressEvent(QMouseEvent *);
//	 void paintEvent(QPaintEvent * event);
public:
   //  QString text;
     QSignalMapper *signalMapper;
private:
    // QSignalMapper *signalMapper;
     QPoint dragPosition;
     bool caps_Lock;
	 bool num_lock;
     int inputMode;
     bool waitingForOperand;

private:
     void setMapper();
     void connectMapper();

signals:
     void setvalue(const QString &);
     void display_backspace();

private slots:
     void onCursorUp();
     void onCursorDown();


     void onCapslock();
     void onEnter();
     void onBackspace();
	 void on_toolButton_capslock_2_clicked();
	 void on_toolButton_num_clicked();
	 void on_toolButton_close_clicked();
};

#endif //  KeyBoard_H

