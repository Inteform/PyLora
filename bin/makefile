#
# Nome do arquivo binário a ser gerado.
#
PROGRAM=teste_spi

#
# Relação dos arquivos objeto.
#
OBJS=main.o gpio.o spi.o lora.o

#
# Caminhos para o código fonte.
#
SRC= ../src
INCLUDE= ../include

#
# Caminho para os compiladores.
#
#TOOLROOT=/home/bruno/x-tools/pi/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian/bin/
#CROSS_COMPILE=arm-linux-gnueabihf-
TOOLPREFIX=$(TOOLROOT)$(CROSS_COMPILE)

#
# Definição dos aplicativos.
#
CC=$(TOOLPREFIX)gcc
LD=$(TOOLPREFIX)gcc
CCC=$(TOOLPREFIX)g++
AR=$(TOOLPREFIX)ar
AS=$(TOOLPREFIX)as
OCPY=$(TOOLPREFIX)objcopy

#
# Flags para depuração (usar GDB).
#
#CFLAGS  = -O2 -g -std=gnu++11
CFLAGS  = -O2 -g
ASFLAGS = -g 

#
# Flags de compilação e montagem.
#
CFLAGS+= -I. -I$(INCLUDE)
CXXFLAGS+= -I. -I$(INCLUDE)
#LDFLAGS+= -lpthread -lconfig++ -lpigpio -lrt
#LDFLAGS+= -lpthread -lpigpio -lrt `mysql_config --libs`

#
# Máquina de destino
#
INSTALL_HOST=raspberrypi.local
INSTALL_USER=pi
INSTALL_PATH=./teste

#
# Definição dos alvos.
#
.phony: all
all: $(PROGRAM)

#
# Linker.
#
$(PROGRAM) : $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

# 
# Gerar arquivos .o a partir dos .c
# Usa comando -MM para gerar dependências.
#
%.o: $(SRC)/%.c
	$(CC) -c $(CFLAGS) $< -o $@
	$(CC) -MM $(CFLAGS) $< > $*.d

#
# Assembler.
#
%.o: $(SRC)/%.s
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -f $(OBJS) $(OBJS:.o=.d) $(ELF) $(CLEANOTHER)

debug: $(ELF)
	arm-none-eabi-gdb $(ELF)

install: $(PROGRAM)
	scp $(PROGRAM) $(INSTALL_USER)@$(INSTALL_HOST):$(INSTALL_PATH)

#
# Inclui os arquivos .d para estender as dependências aos includes.
#
-include $(OBJS:.o=.d)

