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
#include <errno.h>
#include "chip_gpio.h"
#include "chip_gpio_utils.h"

#ifndef TRUE
    #define TRUE 1
#endif
#ifndef FALSE
    #define FALSE 0
#endif

//find base number for xio pins and open files allowing opening and closing of gpio pins
int initialize_gpio_interface()
{
    char* path = NULL;
    int no_err = GPIO_ERR;
    int fd = GPIO_ERR;
    char* base_num_str = NULL;
    char* label_path = NULL;
    int label_fd = GPIO_ERR;
    char* correct_label = NULL;
    char* label = NULL;
	
    //Initialize pin identities
    if (initialize_gpio_pin_names() < 0)
    { fprintf(stderr, "Warning: could not initialize pin label names\n"); return GPIO_ERR; }

    //Find the base XIO pin number in a way that does not depend on the kernel
    xiopin_base = GPIO_ERR;

    for (int i = 0; i < ((int) pow(10,BASE_NUM_MAX_DIGITS)-1); i++)
    {
        path = get_gpiochip_path(i, "/base");
        
        if (access(path, F_OK) >= 0) //if file exists
        {
            //simple error checking
            no_err = TRUE;

            //get a file descriptor for the base file
            fd = open(path, O_RDONLY);
            if (fd < GPIO_OK)
            {
                fprintf(stderr, "Could not open %s: %s\n", path, strerror(errno)); 
                no_err = FALSE;
            }
            
            //create a buffer for the base number
            //char base_num_str[BASE_NUM_MAX_DIGITS+1] = {'\0'};
	    base_num_str = (char*) calloc(BASE_NUM_MAX_DIGITS, sizeof(int));
            
            //read the base number from the file
            if (read(fd, base_num_str, BASE_NUM_MAX_DIGITS) < GPIO_OK)
            {
                fprintf(stderr, "Could not get XIO base number from %s: %s\n", path,
                        strerror(errno));
                no_err = FALSE;
            }
            
            //get a file descriptor for the label file
            label_path = get_gpiochip_path(i, "/label");
            label_fd = open(label_path, O_RDONLY);
            if (label_fd < GPIO_OK)
            {
                fprintf(stderr, "Could not open %s: %s\n", label_path, strerror(errno));
                no_err = FALSE;
            }
           
            //create a buffer for the label
            correct_label = XIO_CHIP_LABEL; //this is the label for the xio gpio chip
            //char label[sizeof(correct_label)];
            label = (char*) calloc(strlen(correct_label)+1, sizeof(char));
            
            label[strlen(correct_label)] = '\0'; //make sure label is null terminated

            //read the label from the file
            //note: watch out for possible endoffile or linefeed after label giving a
            //      false negative
            if (read(label_fd, label, strlen(correct_label)) < GPIO_OK)
            {
                fprintf(stderr, "Could not read label from %s: %s", label_path,
                        strerror(errno));
                no_err = FALSE;
            }

            //if the label is correct and we didn't get any errors, set the base number and
            //get the number of xio pins
            if (strncmp(label, correct_label, strlen(correct_label)) == GPIO_OK && no_err)
            {
                xiopin_base = atoi(base_num_str);

                //since we have our base number, we can break the loop now
                close(fd); //doesn't really matter if this fails, it won't be used again
                free(path);
                free(base_num_str);
                free(label);
                path = NULL; //make sure we don't free this twice somehow
                break;
            }
        }
        
        free(path);
    }

    //Error checking
    if (xiopin_base < GPIO_OK)
    {
        fprintf(stderr, "Failed to obtain XIO pin base number\n");
        return GPIO_ERR;
    }
    
    if (NUM_XIO_U14_PINS < GPIO_OK)
    {
        fprintf(stderr, "Failed to obtain number of XIO pins\n");
        return GPIO_ERR;
    }

    //for convenience, 0 is not used
    is_pin_open = (unsigned char*) calloc(NUM_PINS+FIRST_PIN, sizeof(char));

    //GPIO_CLOSE_FD should always be the last file descriptor in the array
    for (int i = 0; i <= GPIO_CLOSE_FD; i++)
    {
        //Any value less than 0 is considered an error
        pin_fd[i] = GPIO_ERR;
    }

    //Open the export and unexport files (these allow pins to be opened/closed)
    pin_fd[GPIO_OPEN_FD] = open("/sys/class/gpio/export", O_WRONLY);
    pin_fd[GPIO_CLOSE_FD] = open("/sys/class/gpio/unexport", O_WRONLY);

    //check for errors
    if (pin_fd[GPIO_OPEN_FD] < GPIO_OK)
    {
        fprintf(stderr, "Could not open GPIO Export file (are you root?)\n");
        return GPIO_ERR;
    }

    if (pin_fd[GPIO_CLOSE_FD] < GPIO_OK)
    {
        fprintf(stderr, "Could not open GPIO Unexport file (are you root?)\n");
        return GPIO_ERR;
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
    char* pin_str = NULL;
    int pin_kern = GPIO_ERR;
    char* path = NULL;
	
    //get kernel-recognized pin number
    pin_str = get_kern_num_str(pin);
    
    //error-checking: see if the pin was already open before this was called
    pin_kern = get_kern_num(pin);
    path = get_gpio_path(pin_kern, "/value");
    
    if (access(path, F_OK) >= GPIO_OK) //if file exists
    {
        fprintf(stderr,
            "Warning: pin %d (%s) may already be open. This will likely cause issues.\n",
            pin, pin_str);
    }

    if (is_gpio_pin_open(pin))
    {
        free(pin_str);
        return GPIO_OK;
    }

    free(path);
    //finished error checking

    //write to the export file to open to the gpio, check for error
    if (write(pin_fd[GPIO_OPEN_FD], pin_str, strlen(pin_str)) < GPIO_OK)
    {
        fprintf(stderr,
            "Could not open pin %d (%s). Did you call gpio_init(), and are you root?: ",
            pin, pin_str);
        perror("");
        free(pin_str);
        return GPIO_ERR;
    }

    //keep track of open pins for autoclose method
    is_pin_open[pin] = TRUE;

    //free mem used by get_kern_num_str
    free(pin_str);

    return GPIO_OK;
}

//Convenience function; converts pin's name to numerical value and passes it to above func
int open_gpio_pin_n(char* name)
{
    int pin = get_pin_from_name(name);
    if (pin < GPIO_OK) { return GPIO_ERR; }
    return open_gpio_pin(pin);
}

//Convenience function to call open and set_dir in one line
int setup_gpio_pin(int pin, int out)
{
    if (open_gpio_pin(pin) < GPIO_OK) { return GPIO_ERR; }
    return set_gpio_dir(pin, out);
}

int setup_gpio_pin_n(char* name, int out)
{
    if (open_gpio_pin_n(name) < GPIO_OK) { return GPIO_ERR; }
    return set_gpio_dir_n(name, out);
}

//Convenience function
int is_gpio_pin_open(int pin)
{
    if (check_if_pin_exists(pin) < GPIO_OK)
    {
        fprintf(stderr, "Could not check if pin %d is open\n", pin);
        return GPIO_ERR;
    }

    return is_pin_open[pin];
}

//Convenience function
int is_gpio_pin_open_n(char* name)
{
    int pin = get_pin_from_name(name);
    if (pin < GPIO_OK) { return GPIO_ERR; }
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
    char* pin_str = NULL;
	
    if (!is_pin_open[pin])
    {
        fprintf(stderr,
            "Warning: attempting to close a pin (%d) not managed by this program.\n", pin);
    }
    
    pin_str = get_kern_num_str(pin);
        
    if (write(pin_fd[GPIO_CLOSE_FD], pin_str, strlen(pin_str)) < GPIO_OK)
    {
        fprintf(stderr, "Could not close pin %d (%s) (Was it open?)\n", pin, pin_str);
        free(pin_str);
        return GPIO_ERR;
    }

    //free memory used by get_kern_num_str
    free(pin_str);

    //keep track of open pins for autoclose method
    is_pin_open[pin] = FALSE;

    return GPIO_OK;
}

//Convenience function
int close_gpio_pin_n(char* name)
{
    int pin = get_pin_from_name(name);
    if (pin < GPIO_OK) { return GPIO_ERR; }
    return close_gpio_pin(pin);
}

//Convenience function for speed
int get_gpio_pin_num_from_name(char* name)
{
    int pin = get_pin_from_name(name);
    if (pin < GPIO_OK) { return GPIO_ERR; }
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
    for (int i = FIRST_PIN; i < NUM_PINS+FIRST_PIN; i++)
    { if (is_gpio_pin_open(i)) { close_gpio_pin(i); } }
    
    return GPIO_OK;
}

//Close the export/unexport files and all pins we opened
int terminate_gpio_interface()
{
    int err = GPIO_OK;

    autoclose_gpio_pins();

    if (close(pin_fd[GPIO_OPEN_FD]) < GPIO_OK)
    {
        fprintf(stderr, "Could not close GPIO Export file.\n");
        //return GPIO_ERR; //Attempt to finish closing anyway
        err = GPIO_ERR;
    }
    
    if (close(pin_fd[GPIO_CLOSE_FD]) < GPIO_OK)
    {
        fprintf(stderr, "Could not close GPIO Unexport file.\n");
        //return GPIO_ERR; //Attempt to finish closing anyway
        err = GPIO_ERR;
    }

    free(is_pin_open);

    return err;
}

//Convenience function
int term_gpio_inf()
{
    return terminate_gpio_interface();
}
