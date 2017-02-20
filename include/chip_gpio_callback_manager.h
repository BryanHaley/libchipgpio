/*
 * Copyright (c) 2017, Bryan Haley
 * This code is dual licensed (GPLv2 and Simplified BSD). Use the license that works
 * best for you. Check LICENSE.GPL and LICENSE.BSD for more details.
 *
 * chip_gpio_callback_manager.h
 * Interface for registering callbacks and starting the callback manager.
 */

#ifndef CHIP_GPIO_CALLBACK_MANAGER_H
#define CHIP_GPIO_CALLBACK_MANAGER_H

typedef struct
{
    int pin;
    int new_val;
} pin_change_t;

extern int initialize_callback_manager();
extern int init_callback_manager();

extern int start_callback_manager();

extern int setup_callback_manager();

// Signature of a callback function: int foo(pin_change_t, void*)

extern int register_callback_func(int pin, void* func, void* arg);
extern int register_callback_func_n(char* pin_name, void* func, void* arg);

extern int register_callback_flip_func(int pin, void* func, void* arg);
extern int register_callback_flip_func_n(char* pin_name, void* func, void* arg);

extern int remove_callback_func(int pin);
extern int remove_callback_func_n(char* pin_name);

extern int pause_callback_manager();
extern int unpause_callback_manager();

extern int terminate_callback_manager();
extern int term_callback_manager();

extern int set_callback_polling_delay(int new_delay);

#endif
