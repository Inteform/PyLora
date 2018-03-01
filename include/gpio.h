
#ifndef __GPIO_H__
#define __GPIO_H__

int gpio_open(int pin, int output);
int gpio_close(int pin, int fd);
void gpio_output(int fd, int val);
int gpio_input(int fd);
int gpio_wait(int pin, int fd, int rising, int timeout);

#endif
