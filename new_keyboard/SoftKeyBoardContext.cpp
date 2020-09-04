#include <QtCore>
#include "SoftKeyBoardContext.h"
#include <unistd.h>

unsigned char flag_ifclose = 0;

SoftKeyBoardContext::SoftKeyBoardContext()
{
    keyboard_ = new SoftKeyBoard();
    connect(keyboard_, SIGNAL(characterGenerated(int)), SLOT(sendCharacter(int)));
}

SoftKeyBoardContext::~SoftKeyBoardContext()
{
}

bool SoftKeyBoardContext::filterEvent(const QEvent* event)
{
	//qDebug()<<"$$$$$$$$$$$$$$$"<<event->type();
	if (event->type() == QEvent::MouseButtonPress)  //MouseButtonPress
	//if (event->type() == QEvent::MouseButtonPress)
	{
		//updatePosition();  //移动到焦点位置，不需要
		keyboard_->show();
		//return true;
	}
	if (event->type() == QEvent::RequestSoftwareInputPanel)  //MouseButtonPress
	//if (event->type() == QEvent::MouseButtonPress)
    {
		//updatePosition();  //移动到焦点位置，不需要
        keyboard_->show();
		return true;
    }
	else if (event->type() == QEvent::CloseSoftwareInputPanel)
	//else if (event->type() == QEvent::NonClientAreaMouseButtonPress)
	{
		keyboard_->hide() ;
		return true;
	}
    return false;
}

QString SoftKeyBoardContext::identifierName()
{
    return "SoftKeyBoardContext";
}

void SoftKeyBoardContext::reset()
{

}

bool SoftKeyBoardContext::isComposing() const
{
    return false;
}

QString SoftKeyBoardContext::language()
{
    return "en_US";
}

void SoftKeyBoardContext::sendCharacter(int key)
{
    QPointer<QWidget> w = focusWidget();
    if (w)
    {
        QKeyEvent keyPress(QEvent::KeyPress, key, Qt::NoModifier, QString(key));
        QApplication::sendEvent(w, &keyPress);
    }
}

void SoftKeyBoardContext::updatePosition()//移动到焦点位置
{
    QWidget *widget = focusWidget();
    if (!widget)
        return;
	keyboard_->move(widget->mapToGlobal(QPoint(widget->rect().left(), widget->rect().bottom())));
}

bool SoftKeyBoardContext::close_keyboard()
{
//	QPoint pos;
//	QMouseEvent EventPress(QEvent::NonClientAreaMouseButtonPress, pos,Qt::LeftButton, 0, 0);
//	QApplication::sendEvent(this,&EventPress);   //发送键盘按下事件
	//qDebug()<<"^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^";
	return true;
}
