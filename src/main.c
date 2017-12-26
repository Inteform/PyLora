
#include "gpio.h"
#include "spi.h"
#include "lora.h"
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

uint8_t buf1[8], buf2[8];

void teste1(void)
{
   memset(buf1, 0x55, sizeof(buf1));

   int fd = spi_init("/dev/spidev0.0");
   assert(fd >= 0);

   for(;;) {
      usleep(2000000);
      spi_transfer(fd, buf1, buf2, 8);
   }

   close(fd);
}

void teste2(void)
{
   int fd = gpio_open(21, 1);
   assert(fd >= 0);

   for(;;) {
      gpio_output(fd, 1);
      usleep(2000000);
      gpio_output(fd, 0);
      usleep(2000000);
   }

   close(fd);
}

void teste3(void)
{
   lora_init();
   lora_dump_registers();

   for(;;) {
      usleep(2000000);
      printf("Enviando pacote...");
      lora_send_packet("Hello", 5);
      printf("ok\n");
   }
}

int 
main(int argc, char **argv)
{
   teste3();
   return 0;
}
