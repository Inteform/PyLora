
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

/**
 * Perform retries to open a system file.
 * @param fn Filename to open.
 * @param mode Mode to open (as in UNIX open())
 * @return Positive handler if succesful, negative if failure.
 */
int 
try_open(char *fn, int mode)
{
   int fd, retries = 100;
   while(retries-- > 0) {
      fd = open(fn, mode);
      if(fd >= 0) break;
      usleep(1000);
   }

   return fd;
}

/**
 * Open a device file for GPIO control.
 * @param pin Pin number to control.
 * @param output Control direction: 0 = input, 1 = output.
 * @return Positive handler if succesful, negative if failure.
 */
int 
gpio_open(int pin, int output)
{
   char fn[80];
   int fd;
   sprintf(fn, "/sys/class/gpio/gpio%d/value", pin);
   if(access(fn, F_OK) == -1) {
      /*
       * Pin has to be exported from kernel.
       */
      fd = open("/sys/class/gpio/export", O_WRONLY);
      if(fd < 0) return fd;
      sprintf(fn, "%d", pin);
      write(fd, fn, strlen(fn));
      close(fd);
   }
   
   /*
    * Configure pin direction.
    */
   sprintf(fn, "/sys/class/gpio/gpio%d/direction", pin);
   fd = try_open(fn, O_WRONLY);
   if(fd < 0) return fd;

   if(output) write(fd, "out", 3);
   else write(fd, "in", 2);
   close(fd);

   /*
    * Open /sys control file for pin.
    */
   sprintf(fn, "/sys/class/gpio/gpio%d/value", pin);
   if(output) fd = try_open(fn, O_WRONLY);
   else fd = try_open(fn, O_RDONLY);
   return fd;
}

/**
 * Change the value of an output pin.
 * @param fd Control file handler for the pin.
 * @param val New value (0-1).
 */
void 
gpio_output(int fd, int val)
{
   if(fd < 0) return;
   lseek(fd, 0, SEEK_SET);
   if(val) write(fd, "1", 1);
   else write(fd, "0", 1);
}

/**
 * Read the status of an input pin.
 * @param fd Control file handler for the pin.
 * @return 0-1, current pin status, -1 = error.
 */
int
gpio_input(int fd)
{
   char v;
   if(fd < 0) return fd;
   lseek(fd, 0, SEEK_SET);
   if(1 != read(fd, &v, 1)) return -1;
   if(v == '1') return 1;
   return 0;
}

