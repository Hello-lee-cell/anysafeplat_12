#ifndef HISTORY_H
#define HISTORY_H

#include <QDialog>
#include<QTimer>
#include"myqsqlrelationmodel_centerdisp.h"
namespace Ui {
class history;
}

class history : public QDialog
{
    Q_OBJECT

public:
    explicit history(QWidget *parent = 0);
    ~history();
private:
    QTimer *label_hide;
    MyQSqlRelationModel_Centerdisp *model;

    QString Filter;
private slots:
    void which_guncheckbox_show();
    void on_pushButton_3_clicked();     //关闭
    void on_pushButton_xielou_warn_clicked();     //报警记录
    void on_pushButton_xielou_net_clicked();     //通信日志
    void on_pushButton_xielou_op_clicked();     //操作日志
    void on_pushButton_copy_clicked();

    void disp_noU();
    void disp_ing();
    void disp_over();

    void on_pushButton_radar_clicked();
    void on_pushButton_jingdian_clicked();

    void on_tabWidget_currentChanged(int index);

    void on_pushButton_gaswarn_clicked();

    void on_pushButton_gasre_huanjing_clicked();

    void on_pushButton_gasre_oilgun_clicked();

    void on_toolButton_reoilgas_detail_clicked();

    void on_toolButton_reoilgas_detail_output_clicked();

    void on_pushButton_gasre_warn_clicked();

    void on_pushButton_IIE_clicked();

    void on_pushButton_pump_clicked();

    void on_pushButton_liquid_clicked();


    void on_pushButton_crash_clicked();



    void on_pushButton_conditional_query_clicked();

    void on_toolButton_reoilgas_detail_2_clicked();

signals:
    void pushButton_history_enable();

    void export_noU();
    void export_ing();

private:
    Ui::history *ui;
};

#endif // HISTORY_H
