
#ifndef __SPI_H__
#define __SPI_H__

#include <stdint.h>
void spi_transfer(int fd, uint8_t *tx, uint8_t *rx, int size);
int spi_init(char *device);

#endif
