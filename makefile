#
# Copyright (c) 2017, Bryan Haley
# This code is dual licensed (GPLv2 and Simplified BSD). Use the license that works
# best for you. Check LICENSE.GPL and LICENSE.BSD for more details.
#
# makefile
# Use this to easily compile the project. General steps:
# $ make all
# $ sudo make install
# $ sudo ./example
#

CC=gcc
IDIR=./include
CFLAGS=-std=gnu11 -fPIC -I$(IDIR)
LFLAGS=-shared

SDIR=./src/libchipgpio
SRC=chip_gpio_oc.c chip_gpio_rw.c
ODIR=./bin
OBJS=$(ODIR)/chip_gpio_oc.o $(ODIR)/chip_gpio_rw.o
EXE=$(ODIR)/libchipgpio.so
EXEDIR=./lib
DELMACGARB=-find . -name ._\* -delete

lib: mkbin $(OBJS)
	$(CC) $(LFLAGS) -o $(EXE) $(OBJS)
	cp $(EXE) $(EXEDIR)/
$(ODIR)/%.o: $(SDIR)/%.c
	$(CC) $(CFLAGS) -c $^ -o $@
mkbin:
	-mkdir $(ODIR)

EX_CFLAGS=-std=gnu11 -I$(IDIR)
EX_LFLAGS=-L./lib
EX_LIBS=-lchipgpio

EX_SRC=./src/example/morse.c
EX_OBJ=./bin/morse.o
EX_EXE=./example

example: morse.o
	$(CC) $(EX_LFLAGS) -o $(EX_EXE) $(EX_OBJ) $(EX_LIBS)
morse.o:
	$(CC) $(EX_CFLAGS) -c $(EX_SRC) -o $(EX_OBJ)

all: lib example

install:
	cp $(EXE) /usr/lib/
	cp $(IDIR)/* /usr/include

uninstall:
	-rm /usr/lib/libchipgpio.so
	-rm /usr/include/chip_gpio_utils.h
	-rm /usr/include/chip_gpio.h
	-rm /usr/include/chip_gpio_pin_defs.h

clean:
	-rm -r $(EX_EXE) $(ODIR) $(EXEDIR)/*
	-rm ./example
	$(DELMACGARB)
