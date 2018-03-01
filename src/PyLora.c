
#include <Python.h>
#include "lora.h"

int check(void)
{
   if(lora_initialized()) return 1;
   PyErr_SetString(PyExc_RuntimeError, "Lora not initialized");
   return 0;
}

static PyObject *
reset(PyObject *self)
{
   if(!check()) return NULL;
   lora_reset();
   Py_RETURN_NONE;
}

static PyObject *
explicit_header_mode(PyObject *self)
{
   if(!check()) return NULL;
   lora_explicit_header_mode();
   Py_RETURN_NONE;
}

static PyObject *
implicit_header_mode(PyObject *self, PyObject *args)
{
   int size;
   if(!check()) return NULL;
   if(!PyArg_ParseTuple(args, "i", &size)) return NULL;
   lora_implicit_header_mode(size);
   Py_RETURN_NONE;
}

static PyObject *
idle(PyObject *self)
{
   if(!check()) return NULL;
   lora_idle();
   Py_RETURN_NONE;
}

static PyObject *
_sleep(PyObject *self)
{
   if(!check()) return NULL;
   lora_sleep();
   Py_RETURN_NONE;
}

static PyObject *
receive(PyObject *self)
{
   if(!check()) return NULL;
   lora_receive();
   Py_RETURN_NONE;
}

static PyObject *
set_tx_power(PyObject *self, PyObject *args)
{
   int power;
   if(!check()) return NULL;
   if(!PyArg_ParseTuple(args, "i", &power)) return NULL;
   lora_set_tx_power(power);
   Py_RETURN_NONE;
}

static PyObject *
set_frequency(PyObject *self, PyObject *args)
{
   long freq;
   if(!check()) return NULL;
   if(!PyArg_ParseTuple(args, "l", &freq)) return NULL;
   lora_set_frequency(freq);
   Py_RETURN_NONE;
}

static PyObject *
set_spreading_factor(PyObject *self, PyObject *args)
{
   int sf;
   if(!check()) return NULL;
   if(!PyArg_ParseTuple(args, "i", &sf)) return NULL;
   lora_set_spreading_factor(sf);
   Py_RETURN_NONE;
}

static PyObject *
set_bandwidth(PyObject *self, PyObject *args)
{
   long bw;
   if(!check()) return NULL;
   if(!PyArg_ParseTuple(args, "l", &bw)) return NULL;
   lora_set_bandwidth(bw);
   Py_RETURN_NONE;
}

static PyObject *
set_coding_rate(PyObject *self, PyObject *args)
{
   int cr;
   if(!check()) return NULL;
   if(!PyArg_ParseTuple(args, "i", &cr)) return NULL;
   lora_set_coding_rate(cr);
   Py_RETURN_NONE;
}

static PyObject *
set_preamble_length(PyObject *self, PyObject *args)
{
   long pre;
   if(!check()) return NULL;
   if(!PyArg_ParseTuple(args, "l", &pre)) return NULL;
   lora_set_preamble_length(pre);
   Py_RETURN_NONE;
}

static PyObject *
set_sync_word(PyObject *self, PyObject *args)
{
   int w;
   if(!check()) return NULL;
   if(!PyArg_ParseTuple(args, "i", &w)) return NULL;
   lora_set_sync_word(w);
   Py_RETURN_NONE;
}

static PyObject *
enable_crc(PyObject *self)
{
   if(!check()) return NULL;
   lora_enable_crc();
   Py_RETURN_NONE;
}

static PyObject *
disable_crc(PyObject *self)
{
   if(!check()) return NULL;
   lora_disable_crc();
   Py_RETURN_NONE;
}

static PyObject *
set_pins(PyObject *self, PyObject *args, PyObject *keywords)
{
   if(lora_initialized()) {
      PyErr_SetString(PyExc_RuntimeError, "set_pins() has no effect after initialization");
      return NULL;
   }

   char *keys[] = { "spi_device", "cs_pin", "rst_pin", "irq_pin", NULL };
   char *spidev = NULL;
   int cs = -1;
   int rst = -1;
   int irq = -1;

   if(!PyArg_ParseTupleAndKeywords(args, keywords, "|siii", keys, &spidev, &cs, &rst, &irq)) 
      return NULL;

   lora_set_pins(spidev, cs, rst, irq);
   Py_RETURN_NONE;
}

static PyObject *
init(PyObject *self)
{
   int res = lora_init();
   PyEval_InitThreads();                     // it seems to be necessary for using the global interpreter lock (GIL)
   return PyInt_FromLong(res);
}

static PyObject *
packet_rssi(PyObject *self)
{
   if(!check()) return NULL;
   int res = lora_packet_rssi();
   return PyInt_FromLong(res);
}

static PyObject *
packet_snr(PyObject *self)
{
   if(!check()) return NULL;
   float res = lora_packet_snr();
   return PyFloat_FromDouble(res);
}

static PyObject *
_close(PyObject *self)
{
   lora_close();
   Py_RETURN_NONE;
}

static PyObject *
send_packet(PyObject *self, PyObject *args)
{
   PyObject *arg, *msg;
   if(!check()) return NULL;

   /*
    * Check parameter count
    */
   if(PyTuple_Size(args) != 1) {
      PyErr_SetString(PyExc_RuntimeError, "Packet data not provided");
      return NULL;
   }

   /*
    * Check parameter type (bytearray).
    * Convert types if necessary.
    */
   arg = PyTuple_GetItem(args, 0);
   Py_XINCREF(arg);
   if(PyByteArray_Check(arg)) msg = arg;
   else {
      msg = PyByteArray_FromObject(arg);
      Py_XDECREF(arg);
      if(msg == NULL) return NULL;
   }

   /*
    * Use data from bytearray to send.
    */
   uint8_t *buf = PyByteArray_AsString(msg);
   int size = PyByteArray_Size(msg);
   Py_BEGIN_ALLOW_THREADS
   lora_send_packet(buf, size);
   Py_END_ALLOW_THREADS

   Py_XDECREF(msg);
   Py_RETURN_NONE;
}

static PyObject *
packet_available(PyObject *self)
{
   if(lora_received()) Py_RETURN_TRUE;
   Py_RETURN_FALSE;
}

static PyObject *
receive_packet(PyObject *self)
{
   if(!lora_received()) Py_RETURN_NONE;

   /*
    * Create temporary buffer.
    */
   char *buf = malloc(255);
   if(buf == NULL) return PyErr_NoMemory();
   
   /*
    * Read packet and convert data into bytearray object.
    */
   int len = lora_receive_packet(buf, 255);
   PyObject *res = PyByteArray_FromStringAndSize(buf, len);
   free(buf);
   return res;
}

static PyObject *callback_function = NULL;

static void __packet_received(void)
{
   if(callback_function == NULL) return;
   
   PyGILState_STATE gstate = PyGILState_Ensure();
   if(!PyCallable_Check(callback_function)) {
      PyGILState_Release(gstate);
      return;
   }

   PyObject *args = Py_BuildValue("()");
   PyObject *res = PyEval_CallObject(callback_function, args);
   Py_XDECREF(args);
   Py_XDECREF(res);
   PyGILState_Release(gstate);
}

static PyObject *
on_receive(PyObject *self, PyObject *args)
{
   PyObject *funct;
   if(!check()) return NULL;
   
   if(!PyArg_ParseTuple(args, "O", &funct)) return NULL;
   
   if(funct == Py_None) {
      Py_XDECREF(callback_function);
      callback_function = NULL;
      lora_on_receive(NULL);
      Py_RETURN_NONE;
   } 
   
   if((funct == NULL) || (!PyCallable_Check(funct))) {
      PyErr_SetString(PyExc_RuntimeError, "Parameter for on_receive() must be callable");
      return NULL;
   }
   
   Py_XINCREF(funct);
   Py_XDECREF(callback_function);
   callback_function = funct;
   lora_on_receive(__packet_received);
   Py_RETURN_NONE;
}

static PyObject *
wait_for_packet(PyObject *self, PyObject *args)
{
   int timeout = -1;
   if(!check()) return NULL;
   if(!PyArg_ParseTuple(args, "|i", &timeout)) return NULL;
 
   Py_BEGIN_ALLOW_THREADS
   lora_wait_for_packet(timeout);
   Py_END_ALLOW_THREADS
 
   Py_RETURN_NONE;
}

/**
 * Method list for PyLora module
 */
static PyMethodDef metodos[] = {
   { "reset", reset, METH_NOARGS, "Physical reset of the module" },
   { "explicit_header_mode", explicit_header_mode, METH_NOARGS, "Set explicit header mode for the next messages" },
   { "implicit_header_mode", implicit_header_mode, METH_VARARGS, "Set implicit header mode for the next messages, with size bytes" },
   { "idle", idle, METH_NOARGS, "Put the radio in idle mode" },
   { "sleep", _sleep, METH_NOARGS, "Put the radio in sleep mode" },
   { "receive", receive, METH_NOARGS, "Put the radio in RX mode" },
   { "set_tx_power", set_tx_power, METH_VARARGS, "Set output power" },
   { "set_frequency", set_frequency, METH_VARARGS, "Set channel frequency" },
   { "set_spreading_factor", set_spreading_factor, METH_VARARGS, "Set spreading factor" },
   { "set_bandwidth", set_bandwidth, METH_VARARGS, "Set signal bandwidth" },
   { "set_coding_rate", set_coding_rate, METH_VARARGS, "Set coding rate denominator" },
   { "set_preamble_length", set_preamble_length, METH_VARARGS, "Set message preamble length in symbols" },
   { "set_sync_word", set_sync_word, METH_VARARGS, "Set sync word for messages" },
   { "enable_crc", enable_crc, METH_NOARGS, "Enable CRC in message frame" },
   { "disable_crc", disable_crc, METH_NOARGS, "Disable CRC in message frame" },
   { "set_pins", set_pins, METH_VARARGS | METH_KEYWORDS, "Configure interface with transceiver" },
   { "init", init, METH_NOARGS, "Radio transceiver initialization" },
   { "packet_rssi", packet_rssi, METH_NOARGS, "Returns last packet RSSI" },
   { "packet_snr", packet_snr, METH_NOARGS, "Returns last packet SNR" },
   { "close", _close, METH_NOARGS, "End radio library" },
   { "send_packet", send_packet, METH_VARARGS, "Broadcast a message" },
   { "packet_available", packet_available, METH_NOARGS, "Check if data is received" },
   { "receive_packet", receive_packet, METH_NOARGS, "Read the last received packet" },
   { "on_receive", on_receive, METH_VARARGS, "Register a callback function for packet reception" },
   { "wait_for_packet", wait_for_packet, METH_VARARGS, "Suspend execution until a packet arrives or a timeout occurs" },
   { NULL, NULL, 0, NULL }
};

/**
 * Initialization function for the Python interpreter.
 */
void
initPyLora(void)
{
   Py_InitModule("PyLora", metodos);
}
