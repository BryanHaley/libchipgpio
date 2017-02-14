/*
 * Copyright (c) 2017, Bryan Haley
 * This code is dual licensed (GPLv2 and Simplified BSD). Use the license that works
 * best for you. Check LICENSE.GPL and LICENSE.BSD for more details.
 *
 * chip_gpio_oc.c
 * Implementations of the chip_gpio.h interface related to initializing and terminating
 * the interface, as well as “opening” and “closing” the pins.
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

//find base number for xio pins and open files allowing opening and closing of gpio pins
int initialize_gpio_interface()
{
    //Initialize pin identities
    if (initialize_gpio_pin_names() < 0)
    { fprintf(stderr, "Warning: could not initialize pin label names\n"); return -1; }

    //Find the base XIO pin number in a way that does not depend on the kernel
    xiopin_base = -1;

    for (int i = 0; i < ((int) pow(10,BASE_NUM_MAX_DIGITS)-1); i++)
    {
        char* path = get_gpiochip_path(i, "/base");
        
        if (access(path, F_OK) >= 0) //if file exists
        {
            //simple error checking
            int no_err = 1;

            //get a file descriptor for the base file
            int fd = open(path, O_RDONLY);
            if (fd < 0) { fprintf(stderr, "Could not open %s\n", path); no_err = 0; }
            
            //create a buffer for the base number
            char base_num_str[BASE_NUM_MAX_DIGITS+1] = {'\0'};
            
            //read the base number from the file
            if (read(fd, &base_num_str, BASE_NUM_MAX_DIGITS) < 0)
            {
                fprintf(stderr, "Could not get XIO base number from %s\n", path);
                no_err = 0;
            }
            
            //get a file descriptor for the label file
            char* label_path = get_gpiochip_path(i, "/label");
            int label_fd = open(label_path, O_RDONLY);
            if (label_fd < 0)
            {
                fprintf(stderr, "Could not open %s\n", label_path);
                no_err = 0;
            }
           
            //create a buffer for the label
            char* correct_label = XIO_CHIP_LABEL; //this is the label for the xio gpio chip
            char label[sizeof(correct_label)];
            
            //read the label from the file
            //note: watch out for possible endoffile or linefeed after label giving a
            //      false negative
            if (read(label_fd, &label, sizeof(correct_label)-1) < 0)
            { fprintf(stderr, "Could not read label from %s", label_path); no_err = 0;}

            //if the label is correct and we didn't get any errors, set the base number and
            //get the number of xio pins
            if (strncmp(label, correct_label, sizeof(correct_label)) == 0 && no_err)
            {
                xiopin_base = atoi(base_num_str);

                //Get the number of XIO pins from the ngpio file
                //  This is unimplemented until I figure out a better way to handle
                //  the possibility of the CHIP having more or less XIO pins. There's
                //  no documented way to get the total number of gpio pins, or the pin
                //  where U13 ends, or the arbitrary pin label representing where XIO-P0
                //  starts. As it's currently implemented it seems recompilation will be
                //  necessary no matter what, so for now, edit chip_gpio_pin_defs.h if
                //  changes need to be made.
                /*char* ngpio_path = get_gpiochip_path(i, "/ngpio");
                int ngpio_fd = open(ngpio_path, O_RDONLY);
                
                if (ngpio_fd < 0)
                { fprintf(stderr, "Warning: could not open %s\n", ngpio_path); }
                
                char ngpio_str[BASE_NUM_MAX_DIGITS+1] = {'\0'}; //probably a safe assumption
                
                if (read(ngpio_fd, &ngpio_str, BASE_NUM_MAX_DIGITS) < 0)
                {
                    //just in case we don't get a number for some reason
                    fprintf(stderr, 
                        "Warning: could not get number of xio pins from %s; assuming 8.\n",
                        ngpio_path);
                    NUM_XIO_U14_PINS = 8; // The current version of the CHIP has 8 XIO pins
                }
                
                else // if we are able to get the number of xio pins, which we always should
                {
                    NUM_XIO_U14_PINS = atoi(ngpio_str);
                }

                free(ngpio_path);
                close(ngpio_fd);*/

                //since we have our base number, we can break the loop now
                close(fd); //doesn't really matter if this fails, it won't be used again
                free(path);
                path = NULL; //make sure we don't free this twice somehow
                break;
            }
        }
        
        free(path);
    }

    //Error checking
    if (xiopin_base < 0)
    {
        fprintf(stderr, "Failed to obtain XIO pin base number\n");
        return -1;
    }
    
    if (NUM_XIO_U14_PINS < 0)
    {
        fprintf(stderr, "Failed to obtain number of XIO pins\n");
        return -1;
    }

    //for convenience, 0 is not used
    is_pin_open = (unsigned char*) calloc(1, (NUM_PINS+1)*sizeof(char));

    //GPIO_CLOSE_FD should always be the last file descriptor in the array
    for (int i = 0; i <= GPIO_CLOSE_FD; i++)
    {
        //Any value less than 0 is considered an error
        pin_fd[i] = -1;
    }

    //Open the export and unexport files (these allow pins to be opened/closed)
    pin_fd[GPIO_OPEN_FD] = open("/sys/class/gpio/export", O_WRONLY);
    pin_fd[GPIO_CLOSE_FD] = open("/sys/class/gpio/unexport", O_WRONLY);

    //check for errors
    if (pin_fd[GPIO_OPEN_FD] < 0)
    {
        fprintf(stderr, "Could not open GPIO Export file (are you root?)\n");
        return -1;
    }

    if (pin_fd[GPIO_CLOSE_FD] < 0)
    {
        fprintf(stderr, "Could not open GPIO Unexport file (are you root?)\n");
        return -1;
    }

    return xiopin_base;
}

//Convenience function
int init_gpio_inf()
{
	return initialize_gpio_interface();
}

//open a GPIO pin by writing its chip-assigned number to the export file
int open_gpio_pin(int pin)
{
    //get kernel-recognized pin number
    char* pin_str = get_kern_num_str(pin);
    
    //error-checking: see if the pin was already open before this was called
    int pin_kern = get_kern_num(pin);
    char* path = get_gpio_path(pin_kern, "/value");
    
    if (access(path, F_OK) >= 0 || is_gpio_pin_open(pin)) //if file exists
    {
        fprintf(stderr,
            "Warning: pin %d (%s) may already be open. This will likely cause issues.\n",
            pin, pin_str);
    }

    free(path);
    //finished error checking

    //write to the export file to open to the gpio, check for error
    if (write(pin_fd[GPIO_OPEN_FD], pin_str, strlen(pin_str)) < 0)
    {
        fprintf(stderr,
            "Could not open pin %d (%s). Did you call gpio_init(), and are you root?\n",
            pin, pin_str);
        free(pin_str);
        return -1;
    }

    //keep track of open pins for autoclose method
    is_pin_open[pin] = 1;

    //free mem used by get_kern_num_str
    free(pin_str);

    return 0;
}

//Convenience function
int open_gpio_pin_u(int U14, int pin)
{
    if (U14 < 0 || U14 > 1) { fprintf(stderr, "Invalid value %d for U14 option.", U14); }
    return open_gpio_pin(pin+(U14_OFFSET*U14));
}

//Convenience function
int open_gpio_pin_n(char* name)
{
    int pin = get_pin_from_name(name);
    if (pin < 0) { fprintf(stderr, "Could not find pin number for pin %s\n", name); }
    return open_gpio_pin(pin);
}

//set a GPIO pin's direction (input/output) by writing "in" or "out" to its direction file
int set_gpio_dir(int pin, int out)
{
    //open the direction file in the pin directory to write the direction
    int pin_kern = get_kern_num(pin);
    char* path = get_gpio_path(pin_kern, "/direction");
    int fd = open(path, O_WRONLY);

    //err check
    if (fd < 0)
    {
        fprintf(stderr, "Could not open pin %d (%d)'s direction\n", pin, pin_kern);
        free(path);
        close(fd);
        return -1;
    }

    //GPIO_DIR_OUT = 1
    //GPIO_DIR_IN = 0

    if (out)
    {
        if (write(fd, "out", sizeof("out")) < 0)
        {
            fprintf(stderr, "Failed to set %s to %s\n", path, "out");
            free(path);
            close(fd);
            return -1;
        }
    }
    else
    {
        if (write(fd, "in", sizeof("in")) < 0)
        {
            fprintf(stderr, "Failed to set %s to %s\n", path, "in");
            close(fd);
            free(path);
            return -1;
        }
    }

    if (close(fd) < 0)
    {
        fprintf(stderr, "Could not close pin %d (%d)'s direction\n", pin, pin_kern);
        free(path);
        close(fd);
        return -1;
    }

    //free mem used by get_gpio_path
    free(path);
    close(fd);

    return out;
}

//Convenience function
int set_gpio_dir_u(int U14, int pin, int out)
{
    if (U14 < 0 || U14 > 1) { fprintf(stderr, "Invalid value %d for U14 option.", U14); }
    return set_gpio_dir(pin+(U14_OFFSET*U14), out);
}

//Convenience function
int set_gpio_dir_n(char* name, int out)
{
    int pin = get_pin_from_name(name);
    if (pin < 0) { fprintf(stderr, "Could not find pin number for pin %s\n", name); }
    return set_gpio_dir(pin, out);
}

//Convenience function to call open and set_dir in one line
int setup_gpio_pin(int pin, int out)
{
    if (open_gpio_pin(pin) < 0) { return -1; }
    return set_gpio_dir(pin, out);
}

int setup_gpio_pin_u(int U14, int pin, int out)
{
    if (open_gpio_pin_u(U14, pin) < 0) { return -1; }
    return set_gpio_dir_u(U14, pin, out);
}

int setup_gpio_pin_n(char* name, int out)
{
    if (open_gpio_pin_n(name) < 0) { return -1; }
    return set_gpio_dir_n(name, out);
}

//Convenience function
int is_gpio_pin_open(int pin)
{
    return is_pin_open[pin];
}

int is_gpio_pin_open_u(int U14, int pin)
{
    if (U14 < 0 || U14 > 1) { fprintf(stderr, "Invalid value %d for U14 option.", U14); }
    return is_gpio_pin_open(pin+(U14_OFFSET*U14));
}

//Convenience function
int is_gpio_pin_open_n(char* name)
{
    int pin = get_pin_from_name(name);
    if (pin < 0) { fprintf(stderr, "Could not find pin number for pin %s\n", name); }
    return is_gpio_pin_open(pin);
}

//Return the XIO base number assigned by the kernel
//Note: init_gpio() MUST be called first
int get_gpio_xio_base()
{
	return xiopin_base;
}

//Close a GPIO pin by writing its chip-assigned number to the unexport file
int close_gpio_pin(int pin)
{
    if (!is_pin_open[pin])
    {
        fprintf(stderr,
            "Warning: attempting to close a pin (%d) not managed by this program.\n", pin);
    }
    
    char* pin_str = get_kern_num_str(pin);
        
    if (write(pin_fd[GPIO_CLOSE_FD], pin_str, strlen(pin_str)) < 0)
    {
        fprintf(stderr, "Could not close pin %d (%s) (Was it open?)\n", pin, pin_str);
        free(pin_str);
        return -1;
    }

    //free memory used by get_kern_num_str
    free(pin_str);

    //keep track of open pins for autoclose method
    is_pin_open[pin] = 0;

    return 0;
}

//Convenience function
int close_gpio_pin_u(int U14, int pin)
{
    if (U14 < 0 || U14 > 1) { fprintf(stderr, "Invalid value %d for U14 option.", U14); }
    return close_gpio_pin(pin+(U14_OFFSET*U14));
}

//Convenience function
int close_gpio_pin_n(char* name)
{
    int pin = get_pin_from_name(name);
    if (pin < 0) { fprintf(stderr, "Could not find pin number for pin %s\n", name); }
    return close_gpio_pin(pin);
}

//Convenience function for speed
int get_gpio_pin_num_from_name(char* name)
{
    int pin = -1;
    pin = get_pin_from_name(name);
    if (pin < 0) { fprintf(stderr, "Could not find pin number for pin %s\n", name); }
    return pin;
}

//Same thing, shorter name
int get_gpio_num(char* name)
{
	return get_gpio_pin_num_from_name(name);
}

//This will close only pins that were opened by the program that evoked it
int autoclose_gpio_pins()
{
    for (int i = 1; i < NUM_PINS+1; i++) //pins start at 1
    { if (is_gpio_pin_open(i)) { close_gpio_pin(i); } }
    
    return 0;
}

//Close the export/unexport files and all pins we opened
int terminate_gpio_interface()
{
    int noerr = 0;

    autoclose_gpio_pins();

    if (close(pin_fd[GPIO_OPEN_FD]) < 0)
    {
        fprintf(stderr, "Could not close GPIO Export file.\n");
        //return -1; //Attempt to finish closing anyway
        noerr = -1;
    }
    
    if (close(pin_fd[GPIO_CLOSE_FD]) < 0)
    {
        fprintf(stderr, "Could not close GPIO Unexport file.\n");
        //return -1; //Attempt to finish closing anyway
        noerr = -1;
    }

    free(is_pin_open);

    return noerr;
}

//Convenience function
int term_gpio_inf()
{
	return terminate_gpio_interface();
}
