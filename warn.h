#ifndef WARN_H
#define WARN_H

#include <QDialog>

namespace Ui {
class warn;
}

class warn : public QDialog
{
    Q_OBJECT

public:
    explicit warn(QWidget *parent = 0);
    ~warn();

private slots:
    void on_pushButton_3_clicked();

    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

    void hide_warn_button_set(int t);
private:
    Ui::warn *ui;
signals:
    void history_ipautoset();
    void history_ipset();

};

#endif // WARN_H
