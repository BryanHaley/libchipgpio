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

//find base number for xio pins and open files allowing opening and closing of gpio pins
int initialize_gpio_interface()
{
    char* path = NULL;
    int no_err = -1;
    int fd = -1;
    char* base_num_str = NULL;
    char* label_path = NULL;
    int label_fd = -1;
    char* correct_label = NULL;
    char* label = NULL;
	
    //Initialize pin identities
    if (initialize_gpio_pin_names() < 0)
    { fprintf(stderr, "Warning: could not initialize pin label names\n"); return -1; }

    //Find the base XIO pin number in a way that does not depend on the kernel
    xiopin_base = -1;

    for (int i = 0; i < ((int) pow(10,BASE_NUM_MAX_DIGITS)-1); i++)
    {
        path = get_gpiochip_path(i, "/base");
        
        if (access(path, F_OK) >= 0) //if file exists
        {
            //simple error checking
            no_err = 1;

            //get a file descriptor for the base file
            fd = open(path, O_RDONLY);
            if (fd < 0)
            {
                fprintf(stderr, "Could not open %s: %s\n", path, strerror(errno)); 
                no_err = 0;
            }
            
            //create a buffer for the base number
            //char base_num_str[BASE_NUM_MAX_DIGITS+1] = {'\0'};
	    base_num_str = (char*) calloc(BASE_NUM_MAX_DIGITS, sizeof(int));
            
            //read the base number from the file
            if (read(fd, base_num_str, BASE_NUM_MAX_DIGITS) < 0)
            {
                fprintf(stderr, "Could not get XIO base number from %s: %s\n", path,
                        strerror(errno));
                no_err = 0;
            }
            
            //get a file descriptor for the label file
            label_path = get_gpiochip_path(i, "/label");
            label_fd = open(label_path, O_RDONLY);
            if (label_fd < 0)
            {
                fprintf(stderr, "Could not open %s: %s\n", label_path, strerror(errno));
                no_err = 0;
            }
           
            //create a buffer for the label
            correct_label = XIO_CHIP_LABEL; //this is the label for the xio gpio chip
            //char label[sizeof(correct_label)];
            label = (char*) calloc(strlen(correct_label)+1, sizeof(char));
            
            label[strlen(correct_label)] = '\0'; //make sure label is null terminated

            //read the label from the file
            //note: watch out for possible endoffile or linefeed after label giving a
            //      false negative
            if (read(label_fd, label, strlen(correct_label)) < 0)
            {
                fprintf(stderr, "Could not read label from %s: %s", label_path,
                        strerror(errno));
                no_err = 0;
            }

            //if the label is correct and we didn't get any errors, set the base number and
            //get the number of xio pins
            if (strncmp(label, correct_label, strlen(correct_label)) == 0 && no_err)
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
    is_pin_open = (unsigned char*) calloc(1, (NUM_PINS+FIRST_PIN)*sizeof(char));

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
    char* pin_str = NULL;
    int pin_kern = 0;
    char* path = NULL;
	
    //get kernel-recognized pin number
    pin_str = get_kern_num_str(pin);
    
    //error-checking: see if the pin was already open before this was called
    pin_kern = get_kern_num(pin);
    path = get_gpio_path(pin_kern, "/value");
    
    if (access(path, F_OK) >= 0) //if file exists
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
    if (write(pin_fd[GPIO_OPEN_FD], pin_str, strlen(pin_str)) < 0)
    {
        fprintf(stderr,
            "Could not open pin %d (%s). Did you call gpio_init(), and are you root?: ",
            pin, pin_str);
        perror("");
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
/*int open_gpio_pin_u(int U14, int pin)
{
    if (U14 < 0 || U14 > 1) { fprintf(stderr, "Invalid value %d for U14 option.", U14); }
    return open_gpio_pin(pin+(U14_OFFSET*U14));
}*/

//Convenience function
int open_gpio_pin_n(char* name)
{
    int pin = get_pin_from_name(name);
    if (pin < 0) { fprintf(stderr, "Could not find pin number for pin %s\n", name); }
    return open_gpio_pin(pin);
}

//Convenience function to call open and set_dir in one line
int setup_gpio_pin(int pin, int out)
{
    if (open_gpio_pin(pin) < 0) { return -1; }
    return set_gpio_dir(pin, out);
}

/*int setup_gpio_pin_u(int U14, int pin, int out)
{
    if (open_gpio_pin_u(U14, pin) < 0) { return -1; }
    return set_gpio_dir_u(U14, pin, out);
}*/

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

/*int is_gpio_pin_open_u(int U14, int pin)
{
    if (U14 < 0 || U14 > 1) { fprintf(stderr, "Invalid value %d for U14 option.", U14); }
    return is_gpio_pin_open(pin+(U14_OFFSET*U14));
}*/

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
    char* pin_str = NULL;
	
    if (!is_pin_open[pin])
    {
        fprintf(stderr,
            "Warning: attempting to close a pin (%d) not managed by this program.\n", pin);
    }
    
    pin_str = get_kern_num_str(pin);
        
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
/*int close_gpio_pin_u(int U14, int pin)
{
    if (U14 < 0 || U14 > 1) { fprintf(stderr, "Invalid value %d for U14 option.", U14); }
    return close_gpio_pin(pin+(U14_OFFSET*U14));
}*/

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
    for (int i = FIRST_PIN; i < NUM_PINS+FIRST_PIN; i++)
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
