
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <poll.h>

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
 * Finish using a pin from user space.
 * @param pin Pin number to close.
 * @param fd Control file handler for the pin.
 * @return 1 if successful.
 */
int 
gpio_close(int pin, int fd)
{
   char fn[80];
   
   close(fd);
   
   fd = open("/sys/class/gpio/unexport", O_WRONLY);
   if(fd < 0) return fd;
   sprintf(fn, "%d", pin);
   write(fd, fn, strlen(fn));
   close(fd);

   return 1;
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

/**
 * Suspends the process/thread until a rising/falling edge is detected
 * in a input pin.
 * @param pin Input pin number.
 * @param fd Control file handler for the pin (as returned by gpio_open).
 * @param rising Detect falling edge if zero, rising edge if not.
 * @param timeout Timeout for waiting the transition in ms; -1 means no timeout at all.
 */
int 
gpio_wait(int pin, int fd, int rising, int timeout)
{
   char fn[80];
   int f;
   struct pollfd pfd;
 
   sprintf(fn, "/sys/class/gpio/gpio%d/edge", pin);
   f = open(fn, O_WRONLY);
   if(f < 0) return f;
   if(rising) write(f, "rising", 6);
   else write(f, "falling", 7);
   close(f);

   pfd.fd = fd;
   pfd.events = POLLPRI | POLLERR;
   lseek(fd, 0, SEEK_SET);
   read(fd, fn, sizeof(fn));

   f = poll(&pfd, 1, timeout);
   if(f <= 0) return f;

   lseek(fd, 0, SEEK_SET);
   read(fd, fn, sizeof(fn));
   
   if(pfd.revents & pfd.events) return 1;
   return -1;
}
