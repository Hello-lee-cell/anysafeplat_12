#ifndef DRAWHEIGHT_H
#define DRAWHEIGHT_H

#include <QWidget>
#include <QtGui>

class DrawHeight : public QWidget
{
    Q_OBJECT
public:
    explicit DrawHeight(QWidget *parent = 0);

    ~DrawHeight(){}

    void setUsedColor (const QColor color);

protected:
    void paintEvent(QPaintEvent *ev);

private:
    //画背景
    void drawBg(QPainter *painter);

    //画水波
    void draw_OilHeight(QPainter *painter);
    void draw_WaterProcess(QPainter *painter);

    //画文字
    void drawValue(QPainter *painter);

public Q_SLOTS:
    void setMinValue(int value);
    void setMaxValue(int value);
    void setValue(int v_Max,int v_Min,int v_O,int v_W);
    void setOil_Value(int v);
    void setWater_Value(int v);

    void setBorderWidth(int width);
    void setWaterHeight(int height);

    void start();
    void stop();

private:
    int     m_minValue;       //最小值
    int     m_maxValue;       //最大值
    int     m_Oilvalue;          //当前油值
    int     m_Watervalue;          //当前水值

    double  m_borderWidth;    //边框宽度
    double  m_waveHeight;    //水波高度
    double  m_offset;         //水波偏移量

    QColor  m_bgColor;        //背景颜色
    QColor  m_borderColor;    //边框颜色
    QColor  m_usedOil_Color;  //油颜色
    QColor  m_usedWater_Color;//水颜色
    QColor  m_textColor;      //文字颜色

    QTimer  *m_timer;         //水波定时器
};

#endif // DRAWHEIGHT_H
