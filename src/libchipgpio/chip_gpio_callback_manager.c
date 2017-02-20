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

typedef struct
{
    void* func;
    void* arg;
} callback_func_t;

typedef struct
{
    int value;
    char flip;
    char flipped_value;
    char is_flipped;
} pin_callback_t;

pthread_t manager_thread;
callback_func_t* callback_func;
pin_callback_t* pin_val;
int manager_thread_finished;
int delay;
int stop_polling;
int first_start;
void* poll_values(void* arg);
int stop_callback_manager();

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

    return GPIO_OK;
}

int init_callback_manager()
{
    return initialize_callback_manager();
}

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

int setup_callback_manager()
{
    if (initialize_callback_manager() < GPIO_OK)
    { return GPIO_ERR; }
    return start_callback_manager();
}

void* poll_values(void* arg)
{
    manager_thread_finished = FALSE;
    pin_change_t change = { GPIO_ERR, NEVER_READ };

    while (!stop_polling)
    {
        for (int i = FIRST_PIN; i < NUM_PINS+FIRST_PIN; i++)
        {
            if (callback_func[i].func != NO_FUNC)
            {
                change.pin = i;
                change.new_val = read_gpio_val(i);
                
                int (*user_func)(pin_change_t, void*) = callback_func[i].func;
                
                if (change.new_val < GPIO_PIN_LOW || change.new_val > GPIO_PIN_HIGH)
                {
                    fprintf(stderr, 
                        "Warning: read an impossible value from pin %d. Removing its callback function automatically. (Did you close pin %d before removing its callback function?)\n",
                            i);
                        
                    remove_callback_func(i);
                }

                else if (pin_val[i].value != change.new_val)
                {
                    pin_val[i].value = change.new_val;
                    
                    if (!pin_val[i].flip)
                    { user_func(change, callback_func[i].arg); }

                    else if (pin_val[i].value != pin_val[i].flipped_value &&
                             pin_val[i].is_flipped)
                    { user_func(change, callback_func[i].arg); }
                    
                    else if (pin_val[i].value == pin_val[i].flipped_value)
                    { pin_val[i].is_flipped = TRUE; }
                }
            }
        } // done polling pins

        if (delay > 0)
        { usleep(delay); }
    } // finished polling values

    manager_thread_finished = TRUE;

    return NULL;
}

int register_callback_func(int pin, void* func, void* arg)
{
    if (check_if_pin_exists < GPIO_OK)
    { return GPIO_ERR; }
    
    if (first_start) { pause_callback_manager(); }

    callback_func[pin].func = func;
    callback_func[pin].arg = arg;

    pin_val[pin].value = read_gpio_val(pin);
    pin_val[pin].flip = FALSE;
    pin_val[pin].is_flipped = FALSE;
    pin_val[pin].flipped_value = !pin_val[pin].value;

    if (pin_val[pin].value < GPIO_PIN_LOW || pin_val[pin].value > GPIO_PIN_HIGH)
    {
        fprintf(stderr, "Unable to register a callback for pin %d\n", pin);
        callback_func[pin].func = NO_FUNC;
        callback_func[pin].arg = NULL;
    }

    if (first_start) { return unpause_callback_manager(); }

    return GPIO_OK;
}

int register_callback_func_n(char* name, void* func, void* arg)
{
    int pin = get_gpio_pin_num_from_name(name);
    if (pin < GPIO_OK)
    {
        fprintf(stderr, "Could not register callback func for pin %s\n", name);
        return pin; 
    }
    return register_callback_func(pin, func, arg);
}

int register_callback_flip_func(int pin, void* func, void* arg)
{
    int rc = register_callback_func(pin, func, arg);
    pin_val[pin].flip = TRUE;
    return rc;
}

int register_callback_flip_func_n(char* name, void* func, void* arg)
{
    int pin = get_gpio_pin_num_from_name(name);
    if (pin < GPIO_OK)
    {
        fprintf(stderr, "Could not register callback func for pin %s\n", name);
        return pin; 
    }
    return register_callback_flip_func(pin, func, arg);
}

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
        return pin; 
    }
    return remove_callback_func(pin);
}

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
        
        if (now-start > timeout_in_seconds) //prevent an infinite loop
        {
            fprintf(stderr, "Warning: attempting to pause the callback manager failed. Forcing it in order to prevent an infinite loop.\n");
            if (!manager_thread) 
            {
                pthread_cancel(manager_thread);
                manager_thread_finished = TRUE;
            }
            
            else 
            {
                fprintf(stderr, "Could not force callback manager to pause; No valid handle for thread. (Are you being thread-safe?)\n");
            }

            return GPIO_ERR;
        }

        usleep(10);
    }
    
    return GPIO_OK;
}

int unpause_callback_manager()
{
    if (!manager_thread || !stop_polling)
    {
        fprintf(stderr, "Could not unpause callback manager because it is not paused. (Did you initialize and start callback manager first?)\n");
        return GPIO_ERR;
    }

    stop_polling = FALSE;
    return start_callback_manager();
}

int terminate_callback_manager()
{
    pause_callback_manager();
    free(callback_func);
    free(pin_val);
    return GPIO_OK;
}

int term_callback_manager()
{
    return terminate_callback_manager();
}

int set_callback_polling_delay(int new_delay)
{
    delay = new_delay;
    return new_delay;
}
