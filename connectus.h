#ifndef CONNECTUS_H
#define CONNECTUS_H

#include <QDialog>

namespace Ui {
class connectus;
}

class connectus : public QDialog
{
    Q_OBJECT

public:
    explicit connectus(QWidget *parent = 0);
    ~connectus();

private slots:
    void on_pushButton_clicked();

private:
    Ui::connectus *ui;
signals:
    void pushButton_connect_enable();
};

#endif // CONNECTUS_H
