
#include "gpio.h"
#include "spi.h"
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>

/*
 * Hardware definitions
 */
#define DEFAULT_SPI_DEVICE_NAME        "/dev/spidev0.0"
#define DEFAULT_CS_PIN_NUMBER          25
#define DEFAULT_RST_PIN_NUMBER         17
#define DEFAULT_IRQ_PIN_NUMBER         4

/*
 * Register definitions
 */
#define REG_FIFO                       0x00
#define REG_OP_MODE                    0x01
#define REG_FRF_MSB                    0x06
#define REG_FRF_MID                    0x07
#define REG_FRF_LSB                    0x08
#define REG_PA_CONFIG                  0x09
#define REG_LNA                        0x0c
#define REG_FIFO_ADDR_PTR              0x0d
#define REG_FIFO_TX_BASE_ADDR          0x0e
#define REG_FIFO_RX_BASE_ADDR          0x0f
#define REG_FIFO_RX_CURRENT_ADDR       0x10
#define REG_IRQ_FLAGS_MASK             0x11
#define REG_IRQ_FLAGS                  0x12
#define REG_RX_NB_BYTES                0x13
#define REG_PKT_SNR_VALUE              0x19
#define REG_PKT_RSSI_VALUE             0x1a
#define REG_MODEM_CONFIG_1             0x1d
#define REG_MODEM_CONFIG_2             0x1e
#define REG_PREAMBLE_MSB               0x20
#define REG_PREAMBLE_LSB               0x21
#define REG_PAYLOAD_LENGTH             0x22
#define REG_MODEM_CONFIG_3             0x26
#define REG_RSSI_WIDEBAND              0x2c
#define REG_DETECTION_OPTIMIZE         0x31
#define REG_DETECTION_THRESHOLD        0x37
#define REG_SYNC_WORD                  0x39
#define REG_DIO_MAPPING_1              0x40
#define REG_VERSION                    0x42

/*
 * Transceiver modes
 */
#define MODE_LONG_RANGE_MODE           0x80
#define MODE_SLEEP                     0x00
#define MODE_STDBY                     0x01
#define MODE_TX                        0x03
#define MODE_RX_CONTINUOUS             0x05
#define MODE_RX_SINGLE                 0x06

/*
 * PA configuration
 */
#define PA_BOOST                       0x80

/*
 * IRQ masks
 */
#define IRQ_TX_DONE_MASK               0x08
#define IRQ_PAYLOAD_CRC_ERROR_MASK     0x20
#define IRQ_RX_DONE_MASK               0x40

#define PA_OUTPUT_RFO_PIN              0
#define PA_OUTPUT_PA_BOOST_PIN         1

/*
 * File descriptors for the gpios and spi channel
 */
static int __spi = -1;
static int __cs = -1;
static int __rst = -1;
static int __irq = -1;

static char __spi_device_name[80] = DEFAULT_SPI_DEVICE_NAME;
static int __cs_pin_number = DEFAULT_CS_PIN_NUMBER;
static int __rst_pin_number = DEFAULT_RST_PIN_NUMBER;
static int __irq_pin_number = DEFAULT_IRQ_PIN_NUMBER;

static int __implicit;
static long __frequency;

/*
 * Asynchronous API
 */
static void (*__callback)(void) = NULL;
static pthread_t __thid;
static pthread_mutex_t __mutex;

#define lock()          pthread_mutex_lock(&__mutex)
#define unlock()        pthread_mutex_unlock(&__mutex)

/**
 * Returns non-zero value if the hardware had been initialized
 */
int
lora_initialized(void)
{
   if(__spi <= 0) return 0;
   if(__cs <= 0) return 0;
   if(__rst <= 0) return 0;
   if(__irq <= 0) return 0;
   return 1;
}

/**
 * Define pins and spi channel for interfacing with the transceiver.
 * @param spidev SPI device name (like /dev/spidev0.0) or NULL
 * @param cs Chip-select pin number if not negative.
 * @param rst Negative Reset pin number if not negative.
 */
void
lora_set_pins(char *spidev, int cs, int rst, int irq)
{
   if(spidev != NULL) strncpy(__spi_device_name, spidev, sizeof(__spi_device_name));
   if(cs >= 0) __cs_pin_number = cs;
   if(rst >= 0) __rst_pin_number = rst;
   if(irq >= 0) __irq_pin_number = irq;
}

/**
 * Write a value to a register.
 * @param reg Register index.
 * @param val Value to write.
 */
void 
lora_write_reg(int reg, int val)
{
   uint8_t out[2] = { 0x80 | reg, val };
   uint8_t in[2];
   gpio_output(__cs, 0);
   spi_transfer(__spi, out, in, sizeof(out));
   gpio_output(__cs, 1);
}

/**
 * Read the current value of a register.
 * @param reg Register index.
 * @return Value of the register.
 */
int
lora_read_reg(int reg)
{
   uint8_t out[2] = { reg, 0xff };
   uint8_t in[2];
   gpio_output(__cs, 0);
   spi_transfer(__spi, out, in, sizeof(out));
   gpio_output(__cs, 1);
   
   return in[1];
}

/**
 * Perform physical reset on the Lora chip
 */
void 
lora_reset(void)
{
   gpio_output(__cs, 1);
   gpio_output(__rst, 0);
   usleep(300);
   gpio_output(__rst, 1);
   usleep(10000);
}

/**
 * Configure explicit header mode.
 * Packet size will be included in the frame.
 */
void 
lora_explicit_header_mode(void)
{
   __implicit = 0;
   lock();
   lora_write_reg(REG_MODEM_CONFIG_1, lora_read_reg(REG_MODEM_CONFIG_1) & 0xfe);
   unlock();
}

/**
 * Configure implicit header mode.
 * All packets will have a predefined size.
 * @param size Size of the packets.
 */
void 
lora_implicit_header_mode(int size)
{
   __implicit = 1;
   lock();
   lora_write_reg(REG_MODEM_CONFIG_1, lora_read_reg(REG_MODEM_CONFIG_1) | 0x01);
   lora_write_reg(REG_PAYLOAD_LENGTH, size);
   unlock();
}

/**
 * Sets the radio transceiver in idle mode.
 * Must be used to change registers and access the FIFO.
 */
void 
lora_idle(void)
{
   lock();
   lora_write_reg(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_STDBY);
   unlock();
}

/**
 * Sets the radio transceiver in sleep mode.
 * Low power consumption and FIFO is lost.
 */
void 
lora_sleep(void)
{
   lock(); 
   lora_write_reg(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_SLEEP);
   unlock();
}

/**
 * Sets the radio transceiver in receive mode.
 * Incoming packets will be received.
 */
void 
lora_receive(void)
{
   lock();
   lora_write_reg(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_RX_CONTINUOUS);
   unlock();
}

/**
 * Configure power level for transmission
 * @param level 2-17, from least to most power
 */
void 
lora_set_tx_power(int level)
{
   // RF9x module uses PA_BOOST pin
   if (level < 2) level = 2;
   else if (level > 17) level = 17;
   lock();
   lora_write_reg(REG_PA_CONFIG, PA_BOOST | (level - 2));
   unlock();
}

/**
 * Set carrier frequency.
 * @param frequency Frequency in Hz
 */
void 
lora_set_frequency(long frequency)
{
   __frequency = frequency;

   uint64_t frf = ((uint64_t)frequency << 19) / 32000000;

   lock();
   lora_write_reg(REG_FRF_MSB, (uint8_t)(frf >> 16));
   lora_write_reg(REG_FRF_MID, (uint8_t)(frf >> 8));
   lora_write_reg(REG_FRF_LSB, (uint8_t)(frf >> 0));
   unlock();
}

/**
 * Set spreading factor.
 * @param sf 6-12, Spreading factor to use.
 */
void 
lora_set_spreading_factor(int sf)
{
   if (sf < 6) sf = 6;
   else if (sf > 12) sf = 12;

   lock();
   if (sf == 6) {
      lora_write_reg(REG_DETECTION_OPTIMIZE, 0xc5);
      lora_write_reg(REG_DETECTION_THRESHOLD, 0x0c);
   } else {
      lora_write_reg(REG_DETECTION_OPTIMIZE, 0xc3);
      lora_write_reg(REG_DETECTION_THRESHOLD, 0x0a);
   }

   lora_write_reg(REG_MODEM_CONFIG_2, (lora_read_reg(REG_MODEM_CONFIG_2) & 0x0f) | ((sf << 4) & 0xf0));
   unlock();
}

/**
 * Set bandwidth (bit rate)
 * @param sbw Bandwidth in Hz (up to 500000)
 */
void 
lora_set_bandwidth(long sbw)
{
   int bw;

   if (sbw <= 7.8E3) bw = 0;
   else if (sbw <= 10.4E3) bw = 1;
   else if (sbw <= 15.6E3) bw = 2;
   else if (sbw <= 20.8E3) bw = 3;
   else if (sbw <= 31.25E3) bw = 4;
   else if (sbw <= 41.7E3) bw = 5;
   else if (sbw <= 62.5E3) bw = 6;
   else if (sbw <= 125E3) bw = 7;
   else if (sbw <= 250E3) bw = 8;
   else bw = 9;
   lock();
   lora_write_reg(REG_MODEM_CONFIG_1, (lora_read_reg(REG_MODEM_CONFIG_1) & 0x0f) | (bw << 4));
   unlock();
}

/**
 * Set coding rate 
 * @param denominator 5-8, Denominator for the coding rate 4/x
 */ 
void 
lora_set_coding_rate(int denominator)
{
   if (denominator < 5) denominator = 5;
   else if (denominator > 8) denominator = 8;

   int cr = denominator - 4;
   lock();
   lora_write_reg(REG_MODEM_CONFIG_1, (lora_read_reg(REG_MODEM_CONFIG_1) & 0xf1) | (cr << 1));
   unlock();
}

/**
 * Set the size of preamble.
 * @param length Preamble length in symbols.
 */
void 
lora_set_preamble_length(long length)
{
   lock();
   lora_write_reg(REG_PREAMBLE_MSB, (uint8_t)(length >> 8));
   lora_write_reg(REG_PREAMBLE_LSB, (uint8_t)(length >> 0));
   unlock();
}

/**
 * Change radio sync word.
 * @param sw New sync word to use.
 */
void 
lora_set_sync_word(int sw)
{
   lock();
   lora_write_reg(REG_SYNC_WORD, sw);
   unlock();
}

/**
 * Enable appending/verifying packet CRC.
 */
void 
lora_enable_crc(void)
{
   lock();
   lora_write_reg(REG_MODEM_CONFIG_2, lora_read_reg(REG_MODEM_CONFIG_2) | 0x04);
   unlock();
}

/**
 * Disable appending/verifying packet CRC.
 */
void 
lora_disable_crc(void)
{
   lock();
   lora_write_reg(REG_MODEM_CONFIG_2, lora_read_reg(REG_MODEM_CONFIG_2) & 0xfb);
   unlock();
}

/**
 * Perform hardware initialization.
 */
int 
lora_init(void)
{
   /*
    * Configure CPU hardware to communicate with the radio chip
    */
   __spi = spi_init(__spi_device_name);
   if(__spi < 0) return __spi;

   __cs = gpio_open(__cs_pin_number, 1);
   if(__cs < 0) {
      close(__spi);
      return __cs;
   }

   __rst = gpio_open(__rst_pin_number, 1);
   if(__rst < 0) {
      close(__spi);
      close(__cs);
      return __rst;
   }

   __irq = gpio_open(__irq_pin_number, 0);
   if(__irq < 0) {
      close(__spi);
      close(__cs);
      close(__rst);
      return __irq;
   }

   /*
    * Init mutex and callback
    */
   __callback = NULL;
   pthread_mutex_init(&__mutex, NULL);

   /*
    * Perform hardware reset.
    */
   lora_reset();

   /*
    * Check version.
    */
   uint8_t version = lora_read_reg(REG_VERSION);
   assert(version == 0x12);

   /*
    * Default configuration.
    */
   lora_sleep();
 
   lock();
   lora_write_reg(REG_FIFO_RX_BASE_ADDR, 0);
   lora_write_reg(REG_FIFO_TX_BASE_ADDR, 0);
   lora_write_reg(REG_LNA, lora_read_reg(REG_LNA) | 0x03);
   lora_write_reg(REG_MODEM_CONFIG_3, 0x04);
   unlock();
 
   lora_set_tx_power(17);

   lora_idle();
   return 1;
}

/**
 * Send a packet.
 * @param buf Data to be sent
 * @param size Size of data.
 */
void 
lora_send_packet(uint8_t *buf, int size)
{
   int i;

   /*
    * Transfer data to radio.
    */
   lock();
   lora_write_reg(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_STDBY);
   lora_write_reg(REG_FIFO_ADDR_PTR, 0);

   for(i=0; i<size; i++) 
      lora_write_reg(REG_FIFO, *buf++);
   
   lora_write_reg(REG_PAYLOAD_LENGTH, size);
   
   /*
    * Start transmission and wait for conclusion.
    */
   lora_write_reg(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_TX);
   while((lora_read_reg(REG_IRQ_FLAGS) & IRQ_TX_DONE_MASK) == 0)
      usleep(100);

   lora_write_reg(REG_IRQ_FLAGS, IRQ_TX_DONE_MASK);
   unlock();
}

/**
 * Read a received packet.
 * @param buf Buffer for the data.
 * @param size Available size in buffer (bytes).
 * @return Number of bytes received (zero if no packet available).
 */
int 
lora_receive_packet(uint8_t *buf, int size)
{
   int i, len = 0;
 
   /*
    * Check interrupts.
    */
   lock();
   int irq = lora_read_reg(REG_IRQ_FLAGS);
   lora_write_reg(REG_IRQ_FLAGS, irq);
   unlock();
   if((irq & IRQ_RX_DONE_MASK) == 0) return 0;
   if(irq & IRQ_PAYLOAD_CRC_ERROR_MASK) return 0;

   /*
    * Find packet size.
    */
   lock();
   lora_write_reg(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_STDBY);
   if (__implicit) len = lora_read_reg(REG_PAYLOAD_LENGTH);
   else len = lora_read_reg(REG_RX_NB_BYTES);

   /*
    * Transfer data from radio.
    */
   lora_write_reg(REG_FIFO_ADDR_PTR, lora_read_reg(REG_FIFO_RX_CURRENT_ADDR));
   if(len > size) len = size;
   for(i=0; i<len; i++) 
      *buf++ = lora_read_reg(REG_FIFO);
   unlock();
   return len;
}

/**
 * Returns non-zero if there is data to read (packet received).
 */
int
lora_received(void)
{
   lock();
   int m = lora_read_reg(REG_IRQ_FLAGS) & IRQ_RX_DONE_MASK;
   unlock();
   if(m) return 1;
   return 0;
}

/**
 * Suspend the current thread until a packet arrives or a timeout occurs.
 * @param timeout Timeout in ms.
 */
void
lora_wait_for_packet(int timeout)
{
   lock();
   lora_write_reg(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_STDBY);
   lora_write_reg(REG_IRQ_FLAGS_MASK, 0x9f);
   lora_write_reg(REG_DIO_MAPPING_1, 0x00);
   unlock();
   lora_receive();
   gpio_wait(__irq_pin_number, __irq, 1, timeout);
}

/**
 * Secondary thread entry point.
 */
void *__thread_wait(void *p)
{
   p = p;
   pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
   pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

   while(1) {
      lora_wait_for_packet(-1);
      if(__callback != NULL)
         __callback();
   }
   return NULL;
}

/**
 * Define a callback function for packet reception.
 * @param cb Callback function to use (NULL to cancel callbacks).
 */
void lora_on_receive(void (*cb)(void))
{
   if((cb == NULL) && (__callback != NULL)) {
      pthread_cancel(__thid);
      pthread_join(__thid, NULL);
      __callback = NULL;
      return;
   }

   if(__callback == NULL) {
      __callback = cb;
      pthread_create(&__thid, NULL, __thread_wait, NULL);
      return;
   }

   __callback = cb;
}

/**
 * Return last packet's RSSI.
 */
int 
lora_packet_rssi(void)
{
   lock();
   int v = lora_read_reg(REG_PKT_RSSI_VALUE);
   unlock();
   return v - (__frequency < 868E6 ? 164 : 157);
}

/**
 * Return last packet's SNR (signal to noise ratio).
 */
float 
lora_packet_snr(void)
{
   lock();
   int v = lora_read_reg(REG_PKT_SNR_VALUE);
   unlock();
   return ((int8_t)v) * 0.25;
}

/**
 * Shutdown hardware.
 */
void 
lora_close(void)
{
   lora_sleep();
   
   if(__callback != NULL) {
      pthread_cancel(__thid);
      pthread_join(__thid, NULL);
      __callback = NULL;
   }

   close(__spi);
   gpio_close(__cs_pin_number, __cs);
   gpio_close(__rst_pin_number, __rst);
   gpio_close(__irq_pin_number, __irq);
   __spi = -1;
   __cs = -1;
   __rst = -1;
   __irq = -1;
}

void 
lora_dump_registers(void)
{
   int i;
   for(i=0; i<0x26; i++) {
      printf("%02x -> %02x\n", i, lora_read_reg(i));
   }
}

