/*
 * Copyright (c) 2017, Bryan Haley
 * This code is dual licensed (GPLv2 and Simplified BSD). Use the license that works
 * best for you. Check LICENSE.GPL and LICENSE.BSD for more details.
 *
 * chip_gpio_utils.h
 * Utility functions used internally by libchipgpio. These shouldn’t be called by a
 * program using the library.
 */

#ifndef CHIP_GPIO_UTILS_H
#define CHIP_GPIO_UTILS_H

#include <string.h>
#include <stdlib.h>

#define GPIO_OPEN_FD 0
#define GPIO_CLOSE_FD 1
#define BASE_NUM_MAX_DIGITS 5

//XIO GPIO pins start at an unknown base number
/* static */ int xiopin_base;
//Store pointers for gpio export and unexport files here
//  At one point, it was planned to store file descriptors for GPIO value files in this
//  array for performance considerations; however I have since changed this.
static int pin_fd[GPIO_CLOSE_FD+1];
static unsigned char* is_pin_open;

/* Taken from http://stackoverflow.com/questions/1068849/how-do-i-determine-the-number-of-digits-of-an-integer-in-c */
// Quick method to determine the number of digits in an int
static inline int num_places (int n) 
{
    if (n < 10) return 1;
    if (n < 100) return 2;
    if (n < 1000) return 3;
    if (n < 10000) return 4;
    if (n < 100000) return 5;
    if (n < 1000000) return 6;
    if (n < 10000000) return 7;
    if (n < 100000000) return 8;
    if (n < 1000000000) return 9;
    return 10;
}

//find chip-assigned number from Allwinner label
static inline int decode_r8_pin(char multiple, int offset)
{
    // CHIP-assigned number = (32*multiple)+offset
    // Ex: LCD_D4 is PD4 in Allwinner documentation.
    // P = "Pin" presumably (safely ignored)
    // D = multiple (A = 0, B = 1, so on. Subtract by 'A' to get value).
    // 4 = offset
    return (32*(multiple-'A'))+offset;
}

//find the kernel-recognized number for a pin based on the label numbers
static inline int get_kern_num(int pin) //U14 pins = label number + U14_OFFSET
{
    //if -1 gets returned, an error occured
    int pin_kern = GPIO_ERR;
	
    if (pin < FIRST_PIN || pin > NUM_PINS+FIRST_PIN)
    {
        fprintf(stderr, "Tried to access non-existant pin %d\n", pin);
        return GPIO_ERR;
    }
    
    //  CHIP creates directories for GPIO files based on a number assigned to each
    //  GPIO pin. For XIO pins, the number is the offset from the first XIO pin
    //  added to the number assigned to the first XIO pin. The first XIO pin's
    //  number (the base number, get_gpio_xio_base()) changes between kernel versions,
    //  so it must be "found" by probing files/directories in the /sys/class/gpio/
    //  directory.
    //  For all other general purpose GPIO pins (18-40 on U13 and 27-40 on U14,
    //  according to official CHIP documentation), the assigned number can be
    //  calculated by decoding (see https://docs.getchip.com/chip.html#gpio-types)
    //  the pin's label as seen in the Allwinner R8 documentation (goo.gl/cQLRuW,
    //  pages 18-20). I believe Pin 17 is also usable; this may be a typo on the
    //  official CHIP documentation.

    //If a pointer to a custom function for finding the kernel number exists, that
    //takes precedence over anything else.
    if (p_ident[pin].func) //if the function pointer isn't null
    {
        int (*pin_kern_func)(int, void*) = p_ident[pin].func;
        pin_kern = pin_kern_func(pin, p_ident[pin].arg);
    }

    //Reference: https://docs.getchip.com/chip.html#gpio
    if (pin >= XIO_U14_FIRST_PIN_ALL && pin <= XIO_U14_LAST_PIN_ALL)
    {
        pin_kern = get_gpio_xio_base()+(pin-U14_OFFSET)-XIO_U14_FIRST_PIN;
    }
    
    //Kernel pin number CAN be hard coded, but this is not recommended
    else if (p_ident[pin].hard_coded_kern_pin >= 0)
    {
        pin_kern = p_ident[pin].hard_coded_kern_pin;
    }

    //If it's not an XIO or hard-coded pin, it's probably an R8 pin
    //References: goo.gl/cQLRuW (pages 18-20)
    else if (p_ident[pin].mult != GPIO_UNUSED)
    {
        pin_kern = decode_r8_pin(p_ident[pin].mult, p_ident[pin].off);
    }

    else
    {
        fprintf(stderr, "Could not identify kernel-assigned identifier for pin %d\n", pin);
    }
    
    return pin_kern;
}

//convert kernel pin number to a string
static inline char* get_kern_num_str(int pin)
{
    int pin_kern = get_kern_num(pin);
    int pin_kern_len = num_places(pin_kern)+1;
    //create a buffer of the number of digits in pin_kern + a null terminator
    char* pin_str = (char*) calloc(pin_kern_len, sizeof(char));
    snprintf(pin_str, pin_kern_len, "%d", pin_kern);

    //it's your responsibility to free this when it's returned to you
    return pin_str;
}

//get the full path to a gpio file using its chip-assigned number
static inline char* get_gpio_related_path(char* dir, int kern_pin, char* file)
{
    int pin_str_len = num_places(kern_pin)+1; //number of digits + null terminator
    int pin_path_full_len = GPIO_ERR;
    //create string of the kernel pin number
    char pin_str[pin_str_len];
    snprintf(pin_str, pin_str_len, "%d", kern_pin);

    //concat a string that leads to the value file for the gpio pin
    pin_path_full_len = strlen(dir)+strlen(pin_str)+strlen(file)+1;
    char* pin_path_full = (char*) calloc(pin_path_full_len, sizeof(char));
    
    //this is way better than using strncpy
    snprintf(pin_path_full, pin_path_full_len, "%s%s%s", dir, pin_str, file);

    return pin_path_full;
}

//Convenience function for accessing files in GPIO pin directories
static inline char* get_gpio_path(int kern_pin, char* file)
{
    return get_gpio_related_path(GPIO_SYSFS_PATH, kern_pin, file);
}

//Convenience function for accessing files in GPIO chip directories
static inline char* get_gpiochip_path(int kern_pin, char* file)
{
    return get_gpio_related_path(GPIOCHIP_SYSFS_PATH, kern_pin, file);
}

static inline int get_pin_from_name(char* name)
{
    //Compare name to list defined in xio_pin_defs.h
    int pin = GPIO_ERR;

    for (int i = FIRST_PIN; i < NUM_PINS+FIRST_PIN; i++)
    {
        if (strncmp(name, p_ident[i].name, strlen(name)) == GPIO_OK)
        { pin = i; }
    }

    return pin;
}

static inline int does_pin_exist(int pin)
{
    if (pin < FIRST_PIN || pin > NUM_PINS+FIRST_PIN)
    { return 0; } //pin does not exist

    return 1; //pin exists
}

//Convenience error checking form of does_pin_exist
// Note: unlike does pin exist, returning 0 means it DOES exist
static inline int check_if_pin_exists(int pin)
{
    if (!does_pin_exist(pin))
    {
        fprintf(stderr, "Pin %d does not exist.", pin);
        return GPIO_ERR; //pin does not exist
    }

    return GPIO_OK; //pin exists
}

//Convenience error checking method
static inline int is_valid_value(int val, int pin)
{
    if (val < GPIO_PIN_LOW || val > GPIO_PIN_HIGH)
    {
        fprintf(stderr, "Invalid pin (%d) value: %d", pin, val);
        return GPIO_ERR;
    }

    return GPIO_OK;
}

#endif
