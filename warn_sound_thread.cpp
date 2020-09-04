#include "warn_sound_thread.h"
#include "mainwindow.h"
#include "mythread.h"
#include"uartthread.h"
#include"config.h"
#include"io_op.h"

#include<semaphore.h>
#include<sys/mman.h>
#include<fcntl.h>
#include<unistd.h>

#define SHASIZE_SOUND   100
#define SHANAME_SOUND   "shareMem_sound"
#define SEMSIG_SOUND    "signalMem_sound"
int shmid_sound;
char *p_sound;
sem_t *semid_sound;

warn_sound_thread::warn_sound_thread(QWidget *parent)
    : QThread(parent)
{
    shmid_sound = shm_open(SHANAME_SOUND,O_RDWR,0);
    ftruncate(shmid_sound,SHASIZE_SOUND);
    semid_sound = sem_open(SEMSIG_SOUND,0);
    p_sound = (char *)mmap(NULL,SHASIZE_SOUND,PROT_READ|PROT_WRITE,MAP_SHARED,shmid_sound,0);
}

void warn_sound_thread::run()
{
    while(1)
    {
        msleep(100);
        write();
    }
}

void warn_sound_thread::write()
{
    unsigned char flag_sound[30] = {0};

    for(unsigned char i = 0;i < 20;i++)
    {
        flag_sound[i] = Flag_Sound_Xielou[i];
    }
    for(unsigned char i = 20;i < 23;i++)
    {
        flag_sound[i] = Flag_Sound_Radar[i-20];
    }
    if(flag_silent == 1)  //如果静音的话全部置0
    {
        for(int i = 0; i < 28; i++)
        {
            flag_sound[i] = 0;
        }
    }
    flag_sound[28] = 0xa0;
    flag_sound[29] = 0x0a;
    int value_sem_sound;
    sem_getvalue(semid_sound,&value_sem_sound);
//    printf("semid_sound:%d\n",value_sem_sound);
    if(!(value_sem_sound > 0))
    {
        for(uchar i = 0;i < 30;i++)
        {
            p_sound[i] = flag_sound[i];
        }
        sem_post(semid_sound);
    }
}
