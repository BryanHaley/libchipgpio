/*
 * Copyright (c) 2017, Bryan Haley
 * This code is dual licensed (GPLv2 and Simplified BSD). Use the license that works
 * best for you. Check LICENSE.GPL and LICENSE.BSD for more details.
 *
 * chip_gpio_rw.c
 * Implementations of the chip_gpio.h interface related to reading and writing to and
 * from the pins. (Remember: in UNIX everything is a file).
 */

#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include "chip_gpio.h"
#include "chip_gpio_utils.h"

//Set the value of a GPIO pin in the output direction to 1 or 0 (on/off) by writing
//'1' or '0' to its value file.
//  Note: those are the characters '1' and '0' (a.k.a. 0x30 and 0x31), not literal values
//  1 and 0.
int set_gpio_val(int pin, int val)
{
    int pin_kern = get_kern_num(pin);
    
    //Digital pins can only be on or off
    if (val < 0 || val > 1)
    {
        fprintf(stderr, "Invalid value (%d) passed to pin %d (%d)\n", val, pin, pin_kern);
        return -1;
    }
    
    //Open the value file in the pin directory and write the requested value
    char* path = get_gpio_path(pin_kern, "/value");
    int fd = open(path, O_RDWR);
    
    //free memory used by get_gpio_path
    free(path);
    path = NULL;

    if (fd < 0)
    {
        fprintf(stderr, "Could not open pin %d (%d) for writing\n", pin, pin_kern);
        close(fd);
        return -1;
    }

    if (val)
    { 
        if (write(fd, "1", 1) < 0) //ASCII character '1' aka 0x31
        {
            fprintf(stderr,
                "Could not write value %d to pin %d (%d)\n", val, pin, pin_kern);
            close(fd);
            return -1;
        }
    }
    else
    { 
        if (write(fd, "0", 1) < 0) //ASCII character '0' aka 0x30
        {
            fprintf(stderr,
                "Could not write value %d to pin %d (%d)\n", val, pin, pin_kern);
            close(fd);
            return -1;
        }
    }

    if (close(fd) < 0)
    {
        fprintf(stderr, "Could not close pin %d (%d) for writing\n", pin, pin_kern);
        close(fd);
        return -1;
    }

    //close the file descriptor
    close(fd);

    return val;
}

//Convenience function
int set_gpio_val_u(int U14, int pin, int val)
{
    if (U14 < 0 || U14 > 1) { fprintf(stderr, "Invalid value %d for U14 option.", U14); }
    return set_gpio_val(pin+(U14_OFFSET*U14), val);
}

//Convenience function
int set_gpio_val_n(char* name, int val)
{
    int pin = get_pin_from_name(name);
    if (pin < 0) { fprintf(stderr, "Could not find pin number for pin %s\n", name); }
    return set_gpio_val(pin, val);
}

//Return the value (1 aka HIGH or 0 aka LOW) of a GPIO pin in the input direction by
//reading its value file.
int read_gpio_val(int pin)
{
    //if -1 is returned an error occured
    char val = -1;
   
    //open the value file in the pin directory and read the requested value
    int pin_kern = get_kern_num(pin);
    char* path = get_gpio_path(pin_kern, "/value");
    int fd = open(path, O_RDWR);
    
    //free memory used by get_gpio_path
    free(path);
    path = NULL;

    if (fd < 0)
    {
        fprintf(stderr, "Could not open pin %d (%d) for reading\n", pin, pin_kern);
        close(fd);
        return -1;
    }

    if (read(fd, &val, 1) < 0)
    {
        fprintf(stderr, "Could not read from pin %d (%d)\n", pin, pin_kern);
        close(fd);
        return -1;
    }

    if (val != '0' && val != '1')
    {
        fprintf(stderr, "Invalid value read from pin %d (%d)\n", pin, pin_kern);
        close(fd);
        return -1;
    }
    
    val -= '0'; //An ASCII character is given (either '0' or '1'), so subtract it by
                //'0' (aka 0x30) to get the actual numerical value.

    if (close(fd) < 0)
    {
        fprintf(stderr, "Could not close pin %d (%d) for reading\n", pin, pin_kern);
        close(fd);
        return -1;
    }

    //close the file descriptor
    close(fd);

    return val;
}

//Convenience function
int read_gpio_val_u(int U14, int pin)
{
    if (U14 < 0 || U14 > 1) { fprintf(stderr, "Invalid value %d for U14 option.", U14); }
    return read_gpio_val(pin+(U14_OFFSET*U14));
}

//Convenience function
int read_gpio_val_n(char* name)
{
    int pin = get_pin_from_name(name);
    if (pin < 0) { fprintf(stderr, "Could not find pin number for pin %s\n", name); }
    return read_gpio_val(pin);
}
