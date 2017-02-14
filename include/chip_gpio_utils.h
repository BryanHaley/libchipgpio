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
    if (n < 0) n = (n == INT_MIN) ? INT_MAX : -n;
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
    if (pin < FIRST_PIN || pin > NUM_PINS)
    {
        fprintf(stderr, "Tried to access non-existant pin %d\n", pin);
        return -1;
    }

    //if -1 gets returned, an error occured
    int pin_kern = -1;
    
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
	//	official CHIP documentation.

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
    //create a buffer of the number of digits in pin_kern + 1 for a null terminator
    char* pin_str = (char*) calloc(1, (num_places(pin)+1)*sizeof(char));
    sprintf(pin_str, "%d", pin_kern);

    return pin_str;
}

//get the full path to a gpio file using its chip-assigned number
static inline char* get_gpio_related_path(char* dir, int kern_pin, char* file)
{
    //create string of the kernel pin number
    char pin_str[num_places(kern_pin)];
    sprintf(pin_str, "%d", kern_pin);

    //concat a string that leads to the value file for the gpio pin
    //char path_beg[] = "/sys/class/gpio/gpio";
    char* path_beg = dir;
    char* path_end = file;
    char* pin_path_full = (char*) calloc(1, (strlen(path_beg)+num_places(kern_pin)+
                                            strlen(path_end)) * sizeof(char));
    strcat(pin_path_full, path_beg);
    strcat(pin_path_full, pin_str);
    strcat(pin_path_full, path_end);
    
    return pin_path_full;
}

//Convenience function for accessing files in GPIO pin directories
static inline char* get_gpio_path(int kern_pin, char* file)
{
    return get_gpio_related_path("/sys/class/gpio/gpio", kern_pin, file);
}

//Convenience function for accessing files in GPIO chip directories
static inline char* get_gpiochip_path(int kern_pin, char* file)
{
    return get_gpio_related_path("/sys/class/gpio/gpiochip", kern_pin, file);
}

static inline int get_pin_from_name(char* name)
{
	//Compare name to list defined in xio_pin_defs.h
    int pin = -1;

    for (int i = 0; i <= NUM_PINS; i++)
    {
        if (strncmp(name, p_ident[i].name, strlen(p_ident[i].name)-1) == 0)
        { pin = i; }
    }

    return pin;
}

#endif
