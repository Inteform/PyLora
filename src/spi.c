
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

/**
 * Perform a full-duplex transfer on the SPI channel.
 * @param fd File handler of the SPI device.
 * @param tx Buffer with data to send.
 * @param rx Buffer to store received data.
 * @param size Size in bytes for the transfer.
 */
void 
spi_transfer(int fd, uint8_t *tx, uint8_t *rx, int size)
{
   struct spi_ioc_transfer tr = {
      .tx_buf = (unsigned long)tx,
      .rx_buf = (unsigned long)rx,
      .len = size,
      .delay_usecs = 5,
      .speed_hz = 1000000,
      .bits_per_word = 8
   };

   ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
}

/**
 * Open and configure a SPI channel for use.
 * @param device Device file name, like /dev/spidev0.0
 * @return Positive file handler if sucessful, negative if error.
 */
int 
spi_init(char *device)
{
   int fd = open(device, O_RDWR);
   if(fd < 0) return fd;

   int res;
   uint8_t val = SPI_NO_CS;
   res = ioctl(fd, SPI_IOC_WR_MODE, &val);
   if(res < 0) {
      close(fd);
      return -1;
   }

   val = 8;
   res = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &val);
   if(res < 0) {
      close(fd);
      return -1;
   }

   uint32_t spd = 16000000;
   res = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &val);
   if(res < 0) {
      close(fd);
      return -1;
   }

   return fd;
}

