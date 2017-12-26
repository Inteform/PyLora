
#ifndef __GPIO_H__
#define __GPIO_H__

int gpio_open(int pin, int output);
void gpio_output(int fd, int val);
int gpio_input(int fd);

#endif
