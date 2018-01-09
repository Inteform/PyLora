# PyLora
## What is it
**PyLora** is a Python extension module for sending and receiving data through a LoRa transceiver based on Semtech's SX127_ ICs.

The library itself is based on sandeepmistry's **arduino-LoRa** (https://github.com/sandeepmistry/arduino-LoRa) library for Arduino.

It is written as a user-space application over the standard linux drivers gpio and spidev, making the code easily portable among any architecture or distribution.

## How to install
Simply clone the repository and run the setup.py script. Someday maybe it will be available on PyPI.
```bash
git clone https://github.com/Inteform/PyLora.git
cd PyLora
python setup.py build
sudo python setup.py install
```
The library is dependent on the availability of the **gpio** and **spidev** drivers into the Linux system.

## Basic usage
A simple **sender** program...
```python
import PyLora
import time
PyLora.init()
PyLora.set_frequency(915e6)
PyLora.enable_crc()
while True:
    PyLora.send_packet('Hello')
    print 'Packet sent...'
    time.sleep(2)
```
Meanwhile in the **receiver** program...
```python
import PyLora
import time
PyLora.init()
PyLora.set_frequency(915e6)
PyLora.enable_crc()
while True:
    PyLora.receive()   # put into receive mode
    while not PyLora.packet_available():
        # wait for a package
        time.sleep(0)
    rec = PyLora.receive_packet()
    print 'Packet received: {}'.format(rec)
```

## Connection with the RF module
By default, the pins used to control the RF transceiver are those of the SPI channel 0.0 and, on the Raspberry Pi, the following GPIOs:
Pin | Signal
--- | ------
CS | GPIO25
RST | GPIO17
MISO | GPIO9 
MOSI | GPIO10
SCK | GPIO11
but you can reconfigure the pins and SPI channel to use by calling **PyLora.set_pins()** before **PyLora.init()**