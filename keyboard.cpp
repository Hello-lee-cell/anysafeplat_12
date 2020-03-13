#include <QtGui>
#include <QPalette>
#include "keyboard.h"

keyboard::keyboard()
{
    setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose); //关闭变量之后立即释放
    setWindowFlags(Qt::Tool|Qt::WindowStaysOnTopHint|Qt::FramelessWindowHint);
	QPalette p=palette();  //调色板
	//p.setColor(QPalette::Window,Qt::blue);
	p.setColor(QPalette::Background, QColor(40,100,190,200)); //46,110,200,200
	this-> setPalette(p);


	this->move(92,490);

    waitingForOperand = true;
    inputMode = iMode_Normal;
    caps_Lock = false;
	num_lock = true;

    signalMapper=new QSignalMapper(this);
    setMapper();
    connectMapper();

   // connect(signalMapper,SIGNAL(mapped(const QString&)),this,SLOT(setDispText(const QString&)));//将转发信号连接到对应的槽函数

    connect(toolButton_enter,SIGNAL(clicked()),this,SLOT(onEnter()));
    connect(toolButton_backspace,SIGNAL(clicked()),this,SLOT(onBackspace()));
    connect(toolButton_capslock,SIGNAL(clicked()),this,SLOT(onCapslock()));
	on_toolButton_num_clicked();//默认在数字界面
}

keyboard::~keyboard()
{

}

//void keyboard::paintEvent(QPaintEvent * event)
//{
//	QPainter painter(this);
//	painter.fillRect(this->rect(), QColor(51,84,100,100));  //QColor最后一个参数80代表背景的透明度
//}
void keyboard::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        dragPosition = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
    if (event->button() == Qt::RightButton)
    {
        close();
    }

}

/*
* Name : void setMapper()
* Type : function
* Func : mapping the toolButton text to signalMapper
* In   : Null
* Out  : Null
*/
void keyboard::setMapper()
{
    //number
//    signalMapper->setMapping(toolButton_0,toolButton_0->text());
//    signalMapper->setMapping(toolButton_1,toolButton_1->text());
//    signalMapper->setMapping(toolButton_2,toolButton_2->text());
//    signalMapper->setMapping(toolButton_3,toolButton_3->text());
//    signalMapper->setMapping(toolButton_4,toolButton_4->text());
//    signalMapper->setMapping(toolButton_5,toolButton_5->text());
//    signalMapper->setMapping(toolButton_6,toolButton_6->text());
//    signalMapper->setMapping(toolButton_7,toolButton_7->text());
//    signalMapper->setMapping(toolButton_8,toolButton_8->text());
//    signalMapper->setMapping(toolButton_9,toolButton_9->text());

    //letter
    signalMapper->setMapping(toolButton_a,toolButton_a->text());
    signalMapper->setMapping(toolButton_b,toolButton_b->text());
    signalMapper->setMapping(toolButton_c,toolButton_c->text());
    signalMapper->setMapping(toolButton_d,toolButton_d->text());
    signalMapper->setMapping(toolButton_e,toolButton_e->text());
    signalMapper->setMapping(toolButton_f,toolButton_f->text());
    signalMapper->setMapping(toolButton_g,toolButton_g->text());
    signalMapper->setMapping(toolButton_h,toolButton_h->text());
    signalMapper->setMapping(toolButton_i,toolButton_i->text());
    signalMapper->setMapping(toolButton_j,toolButton_j->text());
    signalMapper->setMapping(toolButton_k,toolButton_k->text());
    signalMapper->setMapping(toolButton_l,toolButton_l->text());
    signalMapper->setMapping(toolButton_m,toolButton_m->text());
    signalMapper->setMapping(toolButton_n,toolButton_n->text());
    signalMapper->setMapping(toolButton_o,toolButton_o->text());
    signalMapper->setMapping(toolButton_p,toolButton_p->text());
    signalMapper->setMapping(toolButton_q,toolButton_q->text());
    signalMapper->setMapping(toolButton_r,toolButton_r->text());
    signalMapper->setMapping(toolButton_s,toolButton_s->text());
    signalMapper->setMapping(toolButton_t,toolButton_t->text());
    signalMapper->setMapping(toolButton_u,toolButton_u->text());
    signalMapper->setMapping(toolButton_v,toolButton_v->text());
    signalMapper->setMapping(toolButton_w,toolButton_w->text());
    signalMapper->setMapping(toolButton_x,toolButton_x->text());
    signalMapper->setMapping(toolButton_y,toolButton_y->text());
    signalMapper->setMapping(toolButton_z,toolButton_z->text());
    signalMapper->setMapping(toolButton_dot,toolButton_dot->text());    //设置转发规则，将.text作为实参传递
    signalMapper->setMapping(toolButton_fu,toolButton_fu->text());
	signalMapper->setMapping(toolButton_space,toolButton_space->text()); //空格
	signalMapper->setMapping(toolButton_dou,toolButton_dou->text()); //逗号

}

/*
* Name : void connectMapper()
* Type : function
* Func : connect the toolButton signal(clicked()) to the signalMapper slots(map())
* In   : Null
* Out  : Null
*/
void keyboard::connectMapper()
{
    //number
//    connect(toolButton_0,SIGNAL(clicked()),signalMapper,SLOT(map()));   //将原始信号传递给qsignalMapper对象
//    connect(toolButton_1,SIGNAL(clicked()),signalMapper,SLOT(map()));
//    connect(toolButton_2,SIGNAL(clicked()),signalMapper,SLOT(map()));
//    connect(toolButton_3,SIGNAL(clicked()),signalMapper,SLOT(map()));
//    connect(toolButton_4,SIGNAL(clicked()),signalMapper,SLOT(map()));
//    connect(toolButton_5,SIGNAL(clicked()),signalMapper,SLOT(map()));
//    connect(toolButton_6,SIGNAL(clicked()),signalMapper,SLOT(map()));
//    connect(toolButton_7,SIGNAL(clicked()),signalMapper,SLOT(map()));
//    connect(toolButton_8,SIGNAL(clicked()),signalMapper,SLOT(map()));
//    connect(toolButton_9,SIGNAL(clicked()),signalMapper,SLOT(map()));

    //letter
    connect(toolButton_a,SIGNAL(clicked()),signalMapper,SLOT(map()));
    connect(toolButton_b,SIGNAL(clicked()),signalMapper,SLOT(map()));
    connect(toolButton_c,SIGNAL(clicked()),signalMapper,SLOT(map()));
    connect(toolButton_d,SIGNAL(clicked()),signalMapper,SLOT(map()));
    connect(toolButton_e,SIGNAL(clicked()),signalMapper,SLOT(map()));
    connect(toolButton_f,SIGNAL(clicked()),signalMapper,SLOT(map()));
    connect(toolButton_g,SIGNAL(clicked()),signalMapper,SLOT(map()));
    connect(toolButton_h,SIGNAL(clicked()),signalMapper,SLOT(map()));
    connect(toolButton_i,SIGNAL(clicked()),signalMapper,SLOT(map()));
    connect(toolButton_j,SIGNAL(clicked()),signalMapper,SLOT(map()));
    connect(toolButton_k,SIGNAL(clicked()),signalMapper,SLOT(map()));
    connect(toolButton_l,SIGNAL(clicked()),signalMapper,SLOT(map()));
    connect(toolButton_m,SIGNAL(clicked()),signalMapper,SLOT(map()));
    connect(toolButton_n,SIGNAL(clicked()),signalMapper,SLOT(map()));
    connect(toolButton_o,SIGNAL(clicked()),signalMapper,SLOT(map()));
    connect(toolButton_p,SIGNAL(clicked()),signalMapper,SLOT(map()));
    connect(toolButton_q,SIGNAL(clicked()),signalMapper,SLOT(map()));
    connect(toolButton_r,SIGNAL(clicked()),signalMapper,SLOT(map()));
    connect(toolButton_s,SIGNAL(clicked()),signalMapper,SLOT(map()));
    connect(toolButton_t,SIGNAL(clicked()),signalMapper,SLOT(map()));
    connect(toolButton_u,SIGNAL(clicked()),signalMapper,SLOT(map()));
    connect(toolButton_v,SIGNAL(clicked()),signalMapper,SLOT(map()));
    connect(toolButton_w,SIGNAL(clicked()),signalMapper,SLOT(map()));
    connect(toolButton_x,SIGNAL(clicked()),signalMapper,SLOT(map()));
    connect(toolButton_y,SIGNAL(clicked()),signalMapper,SLOT(map()));
    connect(toolButton_z,SIGNAL(clicked()),signalMapper,SLOT(map()));
    connect(toolButton_dot,SIGNAL(clicked()),signalMapper,SLOT(map()));
    connect(toolButton_fu,SIGNAL(clicked()),signalMapper,SLOT(map()));
	connect(toolButton_space,SIGNAL(clicked()),signalMapper,SLOT(map()));
	connect(toolButton_dou,SIGNAL(clicked()),signalMapper,SLOT(map()));
}

void keyboard::onCursorUp()
{
    QKeyEvent upPress(QEvent::KeyPress,Qt::Key_Up,Qt::NoModifier);
    QApplication::sendEvent(QApplication::focusWidget(),&upPress);
}

void keyboard::onCursorDown()
{
    QKeyEvent downPress(QEvent::KeyPress,Qt::Key_Down,Qt::NoModifier);
    QApplication::sendEvent(QApplication::focusWidget(),&downPress);
}


/*
* Name : void onCapslock()
* Type : slot
* Func : caps lock
* In   : Null
* Out  : Null
*/
void keyboard::onCapslock()
{
    caps_Lock = !caps_Lock;
    if(caps_Lock)
    {
        //letter
        toolButton_a->setText("A");
        toolButton_b->setText("B");
        toolButton_c->setText("C");
        toolButton_d->setText("D");
        toolButton_e->setText("E");
        toolButton_f->setText("F");
        toolButton_g->setText("G");
        toolButton_h->setText("H");
        toolButton_i->setText("I");
        toolButton_j->setText("J");
        toolButton_k->setText("K");
        toolButton_l->setText("L");
        toolButton_m->setText("M");
        toolButton_n->setText("N");
        toolButton_o->setText("O");
        toolButton_p->setText("P");
        toolButton_q->setText("Q");
        toolButton_r->setText("R");
        toolButton_s->setText("S");
        toolButton_t->setText("T");
        toolButton_u->setText("U");
        toolButton_v->setText("V");
        toolButton_w->setText("W");
        toolButton_x->setText("X");
        toolButton_y->setText("Y");
        toolButton_z->setText("Z");
		toolButton_fu->setText(":");
		toolButton_dou->setText("/");
		toolButton_num->setText("?123");

    }
    else
    {
        //letter
        toolButton_a->setText("a");
        toolButton_b->setText("b");
        toolButton_c->setText("c");
        toolButton_d->setText("d");
        toolButton_e->setText("e");
        toolButton_f->setText("f");
        toolButton_g->setText("g");
        toolButton_h->setText("h");
        toolButton_i->setText("i");
        toolButton_j->setText("j");
        toolButton_k->setText("k");
        toolButton_l->setText("l");
        toolButton_m->setText("m");
        toolButton_n->setText("n");
        toolButton_o->setText("o");
        toolButton_p->setText("p");
        toolButton_q->setText("q");
        toolButton_r->setText("r");
        toolButton_s->setText("s");
        toolButton_t->setText("t");
        toolButton_u->setText("u");
        toolButton_v->setText("v");
        toolButton_w->setText("w");
        toolButton_x->setText("x");
        toolButton_y->setText("y");
        toolButton_z->setText("z");
		toolButton_fu->setText(":");
		toolButton_dou->setText("/");
		toolButton_num->setText("?123");
    }
    setMapper();
}

/*
* Name : void onEnter()
* Type : slot
* Func : enter and emit the signal to editor to set text
* In   : Null
* Out  : Null
*/
void keyboard::onEnter()
{
    waitingForOperand = true;
    //emit setvalue(text);
    this->close();
}


/*
* Name : void onBackspace()
* Type : slot
* Func : backspace
* In   : Null
* Out  : Null
*/
void keyboard::onBackspace()
{
  //  display->backspace();
    emit display_backspace();
}



void keyboard::on_toolButton_capslock_2_clicked()
{
	onCapslock();
}

void keyboard::on_toolButton_num_clicked()
{
	num_lock = !num_lock;
	if(num_lock)
	{
		toolButton_a->setText("a");
		toolButton_b->setText("b");
		toolButton_c->setText("c");
		toolButton_d->setText("d");
		toolButton_e->setText("e");
		toolButton_f->setText("f");
		toolButton_g->setText("g");
		toolButton_h->setText("h");
		toolButton_i->setText("i");
		toolButton_j->setText("j");
		toolButton_k->setText("k");
		toolButton_l->setText("l");
		toolButton_m->setText("m");
		toolButton_n->setText("n");
		toolButton_o->setText("o");
		toolButton_p->setText("p");
		toolButton_q->setText("q");
		toolButton_r->setText("r");
		toolButton_s->setText("s");
		toolButton_t->setText("t");
		toolButton_u->setText("u");
		toolButton_v->setText("v");
		toolButton_w->setText("w");
		toolButton_x->setText("x");
		toolButton_y->setText("y");
		toolButton_z->setText("z");
		toolButton_dot->setText(".");
		toolButton_fu->setText(":");
		toolButton_dou->setText("/");
		toolButton_num->setText("?123");

	}
	else
	{
		//letter
		toolButton_a->setText("@");
		toolButton_b->setText(";");
		toolButton_c->setText("'");
		toolButton_d->setText("$");
		toolButton_e->setText("3");
		toolButton_f->setText("%");
		toolButton_g->setText("&&");
		toolButton_h->setText("~");
		toolButton_i->setText("8");
		toolButton_j->setText("+");
		toolButton_k->setText("(");
		toolButton_l->setText(")");
		toolButton_m->setText("?");
		toolButton_n->setText("!");
		toolButton_o->setText("9");
		toolButton_p->setText("0");
		toolButton_q->setText("1");
		toolButton_r->setText("4");
		toolButton_s->setText("#");
		toolButton_t->setText("5");
		toolButton_u->setText("7");
		toolButton_v->setText("=");
		toolButton_w->setText("2");
		toolButton_x->setText("\"");
		toolButton_y->setText("6");
		toolButton_z->setText("*");
		toolButton_dot->setText(".");
		toolButton_fu->setText("-");
		toolButton_dou->setText(",");
		toolButton_num->setText("ABC");
	}
	setMapper();
}

void keyboard::on_toolButton_close_clicked()
{
	this->close();
}
