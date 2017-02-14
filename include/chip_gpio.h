/*
 * Copyright (c) 2017, Bryan Haley
 * This code is dual licensed (GPLv2 and Simplified BSD). Use the license that works
 * best for you. Check LICENSE.GPL and LICENSE.BSD for more details.
 *
 * chip_gpio.h
 * The libchipgpio interface. Check the documentation (README.md) for info on usage.
 */

#ifndef CHIP_GPIO_H
#define CHIP_GPIO_H

#include "chip_gpio_pin_defs.h" //read this as well

#define GPIO_DIR_OUT 1
#define GPIO_DIR_IN 0
#define DIR_GPIO_OUT GPIO_DIR_OUT
#define DIR_GPIO_IN GPIO_DIR_IN
#define is_U13 0
#define is_U14 1
#define GPIO_PIN_OFF 0
#define GPIO_PIN_ON 1
#define GPIO_PIN_LOW 0
#define GPIO_PIN_HIGH 1
#define GPIO_PIN_OUT GPIO_DIR_OUT
#define GPIO_PIN_IN GPIO_DIR_IN
#define NUM_LCD_U13_PINS LCD_U13_LAST_PIN-LCD_U13_FIRST_PIN+1
#define NUM_LCD_U14_PINS LCD_U14_LAST_PIN-LCD_U13_FIRST_PIN+1
#define LCD_U14_FIRST_PIN_ALL LCD_U14_FIRST_PIN+U14_OFFSET
#define LCD_U14_LAST_PIN_ALL LCD_U14_LAST_PIN+U14_OFFSET
#define NUM_LCD_PINS NUM_LCD_U13_PINS+NUM_LCD_U14_PINS

#define NUM_XIO_PINS NUM_XIO_U14_PINS
#define XIO_U14_LAST_PIN XIO_U14_FIRST_PIN+NUM_XIO_U14_PINS-1
#define XIO_U14_FIRST_PIN_ALL XIO_U14_FIRST_PIN+U14_OFFSET
#define XIO_U14_LAST_PIN_ALL XIO_U14_LAST_PIN+U14_OFFSET

extern int initialize_gpio_interface();

//  _u functions have been removed, as they are not conducive to having the GPIO pins 
//  be extendable in the future. Pins should not be accessed directly by their pin
//  number.

extern int open_gpio_pin(int pin); 
//extern int open_gpio_pin_u(int U14, int pin);
extern int open_gpio_pin_n(char* pin_name);

extern int set_gpio_dir(int pin, int out);
//extern int set_gpio_dir_u(int U14, int pin, int out);
extern int set_gpio_dir_n(char* pin_name, int out);

extern int setup_gpio_pin(int pin, int out);
//extern int setup_gpio_pin_u(int U14, int pin, int out);
extern int setup_gpio_pin_n(char* pin_name, int out);

extern int set_gpio_val(int pin, int val);
//extern int set_gpio_val_u(int U14, int pin, int val);
extern int set_gpio_val_n(char* pin_name, int val);

extern int read_gpio_val(int pin);
//extern int read_gpio_val_u(int U14, int pin);
extern int read_gpio_val_n(char* pin_name);

extern int close_gpio_pin(int pin);
//extern int close_gpio_pin_u(int U14, int pin);
extern int close_gpio_pin_n(char* pin_name);

extern int is_gpio_pin_open(int pin);
//extern int is_gpio_pin_open_u(int U14, int pin);
extern int is_gpio_pin_open_n(char* pin_name);

extern int get_gpio_pin_num_from_name(char* pin_name);

extern int get_gpio_xio_base();

extern int autoclose_gpio_pins();

extern int terminate_gpio_interface();

#endif
