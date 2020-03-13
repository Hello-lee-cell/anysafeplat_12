#ifndef REOILGAS_POP_H
#define REOILGAS_POP_H

#include <QWidget>

namespace Ui {
class reoilgas_pop;
}

class reoilgas_pop : public QWidget
{
    Q_OBJECT

public:
    explicit reoilgas_pop(QWidget *parent = 0);
    ~reoilgas_pop();

private slots:
    void on_toolButton_close_clicked();

    void on_tabWidget_currentChanged(int index);

    void select_gun_sta();

    void select_gun_wrong();

    void show_dispener();

    void refresh_dispener_data();

private:
    Ui::reoilgas_pop *ui;

};

#endif // REOILGAS_POP_H
