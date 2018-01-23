
from distutils.core import setup, Extension

mod = Extension("PyLora", 
                sources = ["src/PyLora.c", 
                           "src/lora.c",
                           "src/gpio.c",
                           "src/spi.c"],
                extra_compile_args = ["-std=c99"],
                include_dirs = ["./include"])

setup(
    name = "PyLora",
    version = "1.0",
    description = "Python interface with LoRa Radio Transceiver",
    author = "Bruno Abrantes Basseto",
    author_email = "bruno.basseto@inteform.com.br",
    url = "https://",
    ext_modules = [mod])

