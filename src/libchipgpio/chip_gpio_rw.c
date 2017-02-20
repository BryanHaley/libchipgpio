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
#include <errno.h>
#include "chip_gpio.h"
#include "chip_gpio_utils.h"

//Set the value of a GPIO pin in the output direction to 1 or 0 (on/off) by writing
//'1' or '0' to its value file.
//  Note: those are the characters '1' and '0' (a.k.a. 0x30 and 0x31), not literal values
//  1 and 0.
int set_gpio_val(int pin, int val)
{
    int pin_kern = -1;
    char* path = NULL;
    int fd = -1;
	
    pin_kern = get_kern_num(pin);
    
    //Digital pins can only be on or off
    if (val < 0 || val > 1)
    {
        fprintf(stderr, "Invalid value (%d) passed to pin %d (%d)\n", val, pin, pin_kern);
        return -1;
    }
    
    //Open the value file in the pin directory and write the requested value
    path = get_gpio_path(pin_kern, "/value");
    fd = open(path, O_RDWR);
    
    //free memory used by get_gpio_path
    free(path);
    path = NULL;

    if (fd < 0)
    {
        fprintf(stderr, "Could not open pin %d (%d) for writing: %s\n", 
                pin, pin_kern, strerror(errno));
        close(fd);
        return -1;
    }

    char val_ch = val + '0';

    if (write(fd, &val_ch, sizeof(char)) < 0)
    {
        fprintf(stderr,
                "Could not write value %d to pin %d (%d): %s\n", 
                val, pin, pin_kern, strerror(errno));
        close(fd);
        return -1;
    }

    if (close(fd) < 0)
    {
        fprintf(stderr, "Could not close pin %d (%d) for writing: %s\n", 
                pin, pin_kern, strerror(errno));
        //return -1; //try to continue anyway
    }

    return val;
}

//Convenience function
/*int set_gpio_val_u(int U14, int pin, int val)
{
    if (U14 < 0 || U14 > 1) { fprintf(stderr, "Invalid value %d for U14 option.", U14); }
    return set_gpio_val(pin+(U14_OFFSET*U14), val);
}*/

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
        fprintf(stderr, "Could not open pin %d (%d) for reading: %s\n", 
                pin, pin_kern, strerror(errno));
        close(fd);
        return -1;
    }

    if (read(fd, &val, 1) < 0)
    {
        fprintf(stderr, "Could not read from pin %d (%d): %s\n", 
                pin, pin_kern, strerror(errno));
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
        fprintf(stderr, "Could not close pin %d (%d) for reading: %s\n", 
                pin, pin_kern, strerror(errno));
        //return -1; try to continue anyway
    }

    return val;
}

//Convenience function
/*int read_gpio_val_u(int U14, int pin)
{
    if (U14 < 0 || U14 > 1) { fprintf(stderr, "Invalid value %d for U14 option.", U14); }
    return read_gpio_val(pin+(U14_OFFSET*U14));
}*/

//Convenience function
int read_gpio_val_n(char* name)
{
    int pin = get_pin_from_name(name);
    if (pin < 0) { fprintf(stderr, "Could not find pin number for pin %s\n", name); }
    return read_gpio_val(pin);
}

//Call read and send the opposite value to write
int toggle_gpio_val(int pin)
{
    int val = read_gpio_val(pin);
    if (val < GPIO_PIN_LOW || val > GPIO_PIN_HIGH) { return GPIO_ERR; }
    return set_gpio_val(pin, !val);
}

//Convenience function
int toggle_gpio_val_n(char* name)
{
    int pin = get_pin_from_name(name);
    if (pin < GPIO_OK) { return GPIO_ERR; }
    return toggle_gpio_val(pin);
}

//set a GPIO pin's direction (input/output) by writing "in" or "out" to its direction file
int set_gpio_dir(int pin, int out)
{
    //open the direction file in the pin directory to write the direction
    int pin_kern = get_kern_num(pin);
    char* path = get_gpio_path(pin_kern, "/direction");
    int fd = open(path, O_WRONLY);
    
    //free the mem used by get_gpio_path
    free(path);
    path = NULL;

    //err check
    if (fd < 0)
    {
        fprintf(stderr, "Could not open pin %d (%d)'s direction: %s\n", 
                pin, pin_kern, strerror(errno));
        close(fd);
        return -1;
    }

    //GPIO_DIR_OUT = 1
    //GPIO_DIR_IN = 0

    if (out)
    {
        if (write(fd, "out", sizeof("out")) < 0)
        {
            fprintf(stderr, "Failed to set %s to %s: %s\n", path, "out", strerror(errno));
            close(fd);
            return -1;
        }
    }
    else
    {
        if (write(fd, "in", sizeof("in")) < 0)
        {
            fprintf(stderr, "Failed to set %s to %s: %s\n", path, "in", strerror(errno));
            close(fd);
            return -1;
        }
    }

    if (close(fd) < 0)
    {
        fprintf(stderr, "Could not close pin %d (%d)'s direction: %s\n", 
                pin, pin_kern, strerror(errno));
        //return -1; //try to continue anyway
    }

    return out;
}

//Convenience function
/*int set_gpio_dir_u(int U14, int pin, int out)
{
    if (U14 < 0 || U14 > 1) { fprintf(stderr, "Invalid value %d for U14 option.", U14); }
    return set_gpio_dir(pin+(U14_OFFSET*U14), out);
}*/

//Convenience function
int set_gpio_dir_n(char* name, int out)
{
    int pin = get_pin_from_name(name);
    if (pin < 0) { fprintf(stderr, "Could not find pin number for pin %s\n", name); }
    return set_gpio_dir(pin, out);
}

int get_gpio_dir(int pin)
{
    int out = -1;
    
    //open the direction file in the pin directory to write the direction
    int pin_kern = get_kern_num(pin);
    char* path = get_gpio_path(pin_kern, "/direction");
    int fd = open(path, O_WRONLY);
    
    //free the mem used by get_gpio_path
    free(path);
    path = NULL;

    //err check
    if (fd < 0)
    {
        fprintf(stderr, "Could not open pin %d (%d)'s direction: %s\n", 
                pin, pin_kern, strerror(errno));
        close(fd);
        return -1;
    }

    //read the value from /direction
    if (read(fd, &out, 1) < 0)
    {
        fprintf(stderr, "Could not read from pin %d (%d): %s\n", 
                pin, pin_kern, strerror(errno));
        close(fd);
        return -1;
    }

    if (out != '0' && out != '1')
    {
        fprintf(stderr, "Invalid direction read from pin %d (%d)\n", pin, pin_kern);
        close(fd);
        return -1;
    }

    //Subtract by 0x30 to get numerical value
    out -= '0';

    if (close(fd) < 0)
    {
        fprintf(stderr, "Could not close pin %d (%d)'s direction: %s\n", 
                pin, pin_kern, strerror(errno));
        return -1;
    }

    return out;
}

int get_gpio_dir_n(char* name)
{
    int pin = get_pin_from_name(name);
    if (pin < 0) { fprintf(stderr, "Could not find pin number for pin %s\n", name); }
    return get_gpio_dir(pin);
}
