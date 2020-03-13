#ifndef IO_OP_H
#define IO_OP_H

void beep_heartbeat();
void beep_timer();
void beep_none();
void beep_di();
void gpio_on();
void gpio_off();
void io_init();
void err_heartbeat();
void err_none();
void gpio_high(int num_io1,int num_io2);
void gpio_low(int num_io1,int num_io2);
int gpio5_5_read();
int gpio5_6_read();

#endif // IO_OP_H

