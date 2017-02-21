/*
 * Copyright (c) 2017, Bryan Haley
 * This code is dual licensed (GPLv2 and Simplified BSD). Use the license that works
 * best for you. Check LICENSE.GPL and LICENSE.BSD for more details.
 *
 * chip_gpio_callback_manager.c
 * Implementation of functions for registering and managing callback functions.
 */

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include "chip_gpio.h"
#include "chip_gpio_utils.h"
#include "chip_gpio_callback_manager.h"

#define NO_FUNC NULL
#define NEVER_READ -1

#ifndef TRUE
    #define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
#endif

//struct for storing data used to pass a callback function
typedef struct
{
    void* func;
    void* arg;
} callback_func_t;

//struct to hold data used to interpret pin value changes
typedef struct
{
    int  value;
    char flip; //bool indicating if the callback func for this pin a flip function
    char flipped_value; //opposite of the inital value
    char is_flipped; //bool indicating if the value flipped but has now returned
} pin_callback_t;

pthread_t manager_thread; //pins are polled on a separate thread
callback_func_t* callback_func; //array of callback functions
pin_callback_t* pin_val; //array of pin callback data
int manager_thread_finished; //bool used to indicate when the thread has closed
int delay; //optional polling delay
int stop_polling; //bool used to tell the polling thread to wrap it up
int first_start; //bool used to tell if start_callback_manager has been called yet
int paused; //bool indicating if the thread was paused externally
void* poll_values(void* arg); //function invoked on manager_thread

//Allocate memory for arrays, initialize structs, set booleans used for thread control
int initialize_callback_manager()
{
    callback_func = (callback_func_t*) 
                        malloc((NUM_PINS+FIRST_PIN)*sizeof(callback_func_t));
    pin_val = (pin_callback_t*) malloc((NUM_PINS+FIRST_PIN)*sizeof(pin_callback_t));

    for (int i = FIRST_PIN; i < NUM_PINS+FIRST_PIN; i++)
    {
        pin_val[i].value = NEVER_READ;
        pin_val[i].flip = FALSE;
        pin_val[i].flipped_value = NEVER_READ;
        pin_val[i].is_flipped = TRUE;
        callback_func[i].func = NO_FUNC;
    }

    manager_thread_finished = TRUE;
    stop_polling = FALSE;
    first_start = FALSE;
    paused = FALSE;

    return GPIO_OK;
}

//Convenience method; same as above, shorter name
int init_callback_manager()
{
    return initialize_callback_manager();
}

//create the thread that pins will be polled on
int start_callback_manager()
{
    // If this function is called, any thread assigned to manager_thread SHOULD
    // have already been destroyed, but let's just make sure.
    // (We should be fine even if pthread_cancel fails, so long as we make sure
    //  manager_thread is not NULL; if it -is- NULL, we'll accidentally close the thread
    //  that called this func; that's what this if statement prevents.)
    if (!manager_thread) { pthread_cancel(manager_thread); }
    
    pthread_create(&manager_thread, NULL, &poll_values, NULL);

    first_start = TRUE;

    return GPIO_OK;
}

//Convenience function; calls init and start together
int setup_callback_manager()
{
    if (initialize_callback_manager() < GPIO_OK)
    { return GPIO_ERR; }
    return start_callback_manager();
}

//Read values from pins with registered callback functions, and call their functions
void* poll_values(void* arg)
{
    manager_thread_finished = FALSE;
    pin_change_t change = { GPIO_ERR, NEVER_READ };

    while (!stop_polling)
    {
        for (int i = FIRST_PIN; i < NUM_PINS+FIRST_PIN; i++) //search through all pins
        {
            if (callback_func[i].func != NO_FUNC) //if the pin has a callback func
            {
                change.pin = i;
                change.new_val = read_gpio_val(i); //read in the pin's current value
                
                //assign function pointer user_func to the registered callback function
                int (*user_func)(pin_change_t, void*) = callback_func[i].func;
                
                //check for errors
                if (change.new_val < GPIO_PIN_LOW || change.new_val > GPIO_PIN_HIGH)
                {
                    fprintf(stderr, 
                        "Warning: read an impossible value from pin %d. Removing its callback function automatically. (Did you close pin %d before removing its callback function?)\n",
                            i);
                        
                    //Theoritically we should never get a value other than 0 or 1 from a
                    //digital IO pin, so if we do, be safe and remove the callback function
                    remove_callback_func(i);
                }

                //if there are no errors and the value has changed
                else if (pin_val[i].value != change.new_val)
                {
                    pin_val[i].value = change.new_val; //store the new value
                    
                    //if it isn't a flip function, we're done, just call user_func
                    if (!pin_val[i].flip)
                    { user_func(change, callback_func[i].arg); }

                    //if it is a flip function and the values flipped to and back, call
                    else if (pin_val[i].value != pin_val[i].flipped_value &&
                             pin_val[i].is_flipped)
                    { user_func(change, callback_func[i].arg); }
                    
                    //if the pin is currently flipping, set the bool to indicate this
                    else if (pin_val[i].value == pin_val[i].flipped_value)
                    { pin_val[i].is_flipped = TRUE; }
                }
            }
        } // done polling pins

        if (delay > 0) //optional delay
        { usleep(delay); }
    } // finished polling values

    //indicate we are finished with this thread
    manager_thread_finished = TRUE;

    return NULL;
}

//register a function to be called any time a pin's value changes
int register_callback_func(int pin, void* func, void* arg)
{
    int was_paused = paused;
    
    if (check_if_pin_exists < GPIO_OK)
    { return GPIO_ERR; }
    
    //if the polling thread has already started, we need to stop it before doing this
    if (first_start && !was_paused) { pause_callback_manager(); }

    //set the callback func for the pin to function pointer passed to us
    callback_func[pin].func = func;
    callback_func[pin].arg = arg;

    //set some initial values
    pin_val[pin].value = read_gpio_val(pin);
    pin_val[pin].flip = FALSE;
    pin_val[pin].is_flipped = FALSE;
    pin_val[pin].flipped_value = !pin_val[pin].value;

    //check for errors
    if (pin_val[pin].value < GPIO_PIN_LOW || pin_val[pin].value > GPIO_PIN_HIGH)
    {
        fprintf(stderr, "Unable to register a callback for pin %d\n", pin);
        callback_func[pin].func = NO_FUNC;
        callback_func[pin].arg = NULL;
    }

    if (first_start && !was_paused) { return unpause_callback_manager(); }

    return GPIO_OK;
}

//Convenience method; converts a string into a numerical pin and calls above
int register_callback_func_n(char* name, void* func, void* arg)
{
    int pin = get_gpio_pin_num_from_name(name);
    if (pin < GPIO_OK)
    {
        fprintf(stderr, "Could not register callback func for pin %s\n", name);
        return GPIO_ERR; 
    }
    return register_callback_func(pin, func, arg);
}

// Registers a callback function that is called when a pin's value changes, then changes
// back. So for example, imagine a pin connected to a button has a value of 1 when the
// button is not pressed. When pressed, the value changes to 0. When the button is
// released, the value flips back to 1. The callback function would then be called.
// This is effectively an on-release trigger, but might not always be used with buttons,
// so a more generic name is used.
int register_callback_flip_func(int pin, void* func, void* arg)
{
    //just register the callback function then set the flip flag to true
    int rc = register_callback_func(pin, func, arg);
    pin_val[pin].flip = TRUE;
    return rc;
}

int register_callback_flip_func_n(char* name, void* func, void* arg)
{
    int pin = get_gpio_num(name);
    if (pin < GPIO_OK) { return GPIO_ERR; }
    return register_callback_flip_func(pin, func, arg);
}

// Force the flipped_value of a pin
// This probably shouldn't be called before start or unexpected behavior may occur
int set_callback_flip_value(int pin, int val)
{
    if (check_if_pin_exists < GPIO_OK)
    { return GPIO_ERR; }

    if (is_valid_value(val, pin) < GPIO_OK)
    { return GPIO_ERR; }

    pin_val[pin].flipped_value = val;
    pin_val[pin].value = !val;

    return GPIO_OK;
}

int set_callback_flip_value_n(char* name, int val)
{
    int pin = get_gpio_num(name);
    if (pin < GPIO_OK) { return GPIO_ERR; }
    return set_callback_flip_value(pin, val);
}

//"Deregister" a callback function, stopping the pin from being polled. The pin should
// be closed by the programmer if it's done being used AFTER removing its callback func.
int remove_callback_func(int pin)
{
    if (check_if_pin_exists < GPIO_OK)
    { return GPIO_ERR; }

    pause_callback_manager();

    callback_func[pin].func = NO_FUNC;
    callback_func[pin].arg = NULL;

    pin_val[pin].value = NEVER_READ;

    return unpause_callback_manager();
}

int remove_callback_func_n(char* name)
{
    int pin = get_gpio_pin_num_from_name(name);
    if (pin < GPIO_OK)
    {
        fprintf(stderr, "Could not remove callback func for pin %s\n", name);
        return GPIO_ERR; 
    }
    return remove_callback_func(pin);
}

//Destroy/stop the polling thread without deregistering all callback functions
int pause_callback_manager()
{
    time_t start = time(NULL);
    time_t now = time(NULL);
    int timeout_in_seconds = 5;

    if (!manager_thread || manager_thread_finished)
    {
        fprintf(stderr, "Could not pause callback manager because it is not running. (Did you initialize and start callback manager first?)\n");
        return GPIO_ERR;
    }

    stop_polling = TRUE; //tell manager_thread to finish up
    while (!manager_thread_finished) //wait for manager thread to finish
    {
        now = time(NULL);
        
        if (now-start > timeout_in_seconds) //prevent an infinite loop using a timeout
        {
            fprintf(stderr, "Warning: attempting to pause the callback manager failed. Forcing it in order to prevent an infinite loop.\n");
            if (!manager_thread) 
            {
                pthread_cancel(manager_thread);
                manager_thread_finished = TRUE;
            }
            
            else //manager_thread SHOULDN'T ever be null
            {
                fprintf(stderr, "Could not force callback manager to pause; No valid handle for thread. (Are you being thread-safe?)\n");
            }

            return GPIO_ERR;
        }

        usleep(10);
    }
    
    paused = TRUE;

    return GPIO_OK;
}

//Recreate polling thread with previously registered callback funcs
int unpause_callback_manager()
{
    if (!manager_thread || !stop_polling)
    {
        fprintf(stderr, "Could not unpause callback manager because it is not paused. (Did you initialize and start callback manager first?)\n");
        return GPIO_ERR;
    }

    stop_polling = FALSE;
    paused = FALSE;

    return start_callback_manager();
}

// Destroy polling thread and free memory used by callback func definitions.
// Init would need to be called if the callback manager is to be used again.
int terminate_callback_manager()
{
    pause_callback_manager();
    first_start = FALSE;
    paused = FALSE;
    free(callback_func);
    free(pin_val);
    return GPIO_OK;
}

//Convenience method; shorter name, calls above
int term_callback_manager()
{
    return terminate_callback_manager();
}

//Set the optional delay if desired
int set_callback_polling_delay(int new_delay)
{
    delay = new_delay;
    return new_delay;
}
