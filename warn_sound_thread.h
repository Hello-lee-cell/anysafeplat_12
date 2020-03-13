#ifndef WARN_SOUND_THREAD_H
#define WARN_SOUND_THREAD_H


#include <QtGui/QMainWindow>
#include <QSharedMemory>
#include <QSystemSemaphore>
#include<qthread.h>
#include<QThread>
#include<mainwindow.h>
#include<qwidget.h>

class warn_sound_thread : public QThread
{
    Q_OBJECT

public:
    explicit warn_sound_thread(QWidget *parent = 0);

    void run();

public slots:
   void write();

private:

    QSharedMemory sharedMem;

};

#endif // WARN_SOUND_THREAD_H
