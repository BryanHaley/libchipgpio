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

DEBUG=-g

CFLAGS=-std=gnu11 -fPIC $(DEBUG) -I$(IDIR)
LFLAGS=-shared $(DEBUG)
LIBS=-lpthread

SDIR=./src/libchipgpio
SRC=chip_gpio_oc.c chip_gpio_rw.c chip_gpio_callback_manager.c
ODIR=./bin
OBJS=$(ODIR)/chip_gpio_oc.o $(ODIR)/chip_gpio_rw.o $(ODIR)/chip_gpio_callback_manager.o
EXE=$(ODIR)/libchipgpio.so
EXEDIR=./lib
DELMACGARB=-find . -name ._\* -delete

lib: mkbin $(OBJS)
	$(CC) $(LFLAGS) -o $(EXE) $(OBJS) $(LIBS)
	cp $(EXE) $(EXEDIR)/
$(ODIR)/%.o: $(SDIR)/%.c
	$(CC) $(CFLAGS) -c $^ -o $@
mkbin:
	-mkdir $(ODIR)

EX_CFLAGS=-std=gnu11 $(DEBUG) -I$(IDIR)
EX_LFLAGS=-L./lib $(DEBUG)
EX_LIBS=-lchipgpio

MORSE_SRC=./src/example/morse.c
MORSE_OBJ=./bin/morse.o
MORSE_EXE=./morse_example

TOGGLE_SRC=./src/example/toggle.c
TOGGLE_OBJ=./bin/toggle.o
TOGGLE_EXE=./toggle_example

morse_example: morse.o
	$(CC) $(EX_LFLAGS) -o $(MORSE_EXE) $(MORSE_OBJ) $(EX_LIBS)
morse.o:
	$(CC) $(EX_CFLAGS) -c $(MORSE_SRC) -o $(MORSE_OBJ)

toggle_example: toggle.o
	$(CC) $(EX_LFLAGS) -o $(TOGGLE_EXE) $(TOGGLE_OBJ) $(EX_LIBS)
toggle.o:
	$(CC) $(EX_CFLAGS) -c $(TOGGLE_SRC) -o $(TOGGLE_OBJ)

all: lib morse_example toggle_example

install:
	cp $(EXE) /usr/lib/
	cp $(IDIR)/* /usr/include

uninstall:
	-rm /usr/lib/libchipgpio.so
	-rm /usr/include/chip_gpio_utils.h
	-rm /usr/include/chip_gpio.h
	-rm /usr/include/chip_gpio_pin_defs.h

clean:
	-rm -r $(ODIR) $(EXEDIR)/*
	-rm $(MORSE_EXE) $(TOGGLE_EXE)
	$(DELMACGARB)
