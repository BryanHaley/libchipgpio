/*
 * Copyright (c) 2017, Bryan Haley
 * This code is dual licensed (GPLv2 and Simplified BSD). Use the license that works
 * best for you. Check LICENSE.GPL and LICENSE.BSD for more details.
 *
 * morse.c
 * An example program utilizing libchipgpio to pulse an “SOS” signal via an LED.
 * By default, an LED should be connected to the XIO-P7 pin, and a button connected
 * to the LCD-VSYNC pin (this requires a pulp resistor). Of course, this program can
 * easily be edited to change which pins are used.
 */

#include <stdio.h>
#include <unistd.h>
#include "chip_gpio.h"
#include "chip_gpio_callback_manager.h"

// Signature of a callback function: int foo(pin_change_t, void*)
int led_power_print(pin_change_t pin, void* arg);
int led_power_on(pin_change_t pin, void* arg);
int led_power_toggle(pin_change_t pin, void* arg);

int led_power_pin;

int main (int argc, char **argv)
{
    int button_pin = GPIO_ERR; //gpio pin connected to a button
    int toggle_pin = GPIO_ERR; //gpio pin connected to another button
    led_power_pin = GPIO_ERR; //gpio pin connected to an led
	
    //You must call initialize_gpio_interface before use. Sub-zero values are errors.
    if (initialize_gpio_interface() < GPIO_OK)
    { fprintf(stderr, "GPIO Error. Shutting down.\n"); return GPIO_ERR; }

    // If desired, you may attempt to close pins before trying to open them to ensure
    // access. This is not recommended, however, as it may interfere with other programs.
    //if (is_gpio_pin_open_n("XIO-P7")) { close_gpio_pin_n("XIO-P7"); }
    //if (is_gpio_pin_open_n("XIO-P5")) { close_gpio_pin_n("XIO-P5"); }
    //if (is_gpio_pin_open_n("XIO-P3")) { close_gpio_pin_n("XIO_P3"); }

    //This is the ideal way to declare and open pins
    led_power_pin = get_gpio_pin_num_from_name("XIO-P7");
    if (setup_gpio_pin(led_power_pin, GPIO_DIR_OUT) < GPIO_OK)
    { fprintf(stderr, "GPIO Error. Shutting down.\n"); return GPIO_ERR; }

    button_pin = get_gpio_num("XIO-P5"); //same as get_gpio_pin_num_from_name
    if (setup_gpio_pin(button_pin, GPIO_DIR_IN) < GPIO_OK)
    { fprintf(stderr, "GPIO Error. Shutting down.\n"); return GPIO_ERR; }

    toggle_pin = get_gpio_num("XIO-P3");
    if (setup_gpio_pin(toggle_pin, GPIO_DIR_IN) < GPIO_OK)
    { fprintf(stderr, "GPIO Error. Shutting down.\n"); return GPIO_ERR; }

    /*
    *   If speed is not a concern, you may also open, read, write, and close pins using
    *   the pin's name with functions containing the _n suffix.
    *   E.g. setup_gpio_pin_n("XIO-P7", GPIO_DIR_OUT); //error checking recommended
    *   This is perfectly fine, but comes with a performance hit as the name must be
    *   compared against a list of strings for every function call. The method above is
    *   highly recommended, as it only compares the strings once.
    *   E.g. int on_button = get_gpio_pin_num_from_name("LCD-D11");
    *        setup_gpio_pin(on_button, GPIO_DIR_IN); //error checking recommended
    *
    *   However, !!DO NOT!! send pin numbers directly to open_gpio_pin or any other method.
    *   (E.g. !DO NOT! do setup_gpio_pin_n(37, GPIO_DIR_IN) or any other literal number)
    *
    *   The library is designed so that if the pinout of the CHIP were ever changed or
    *   expanded, the library can be modified and recompiled independently of the program.
    *   This way, existing pins can change location and new pins can be added WITHOUT
    *   breaking existing programs coded to run on old versions of the CHIP.
    *   Sending pin numbers directly WILL break that forwards-compatibility. In the interest
    *   of future-proofing your program, use the recommended method.
    */

    //Make sure the LED is off when we start
    set_gpio_val(led_power_pin, GPIO_PIN_LOW);

    // Set up some callbacks for button presses
    initialize_callback_manager();

    // Be aware when you register a callback function, the initial value of a pin is
    // taken at this time. This is important for register_callback_flip_func.
    //With register_callback_func, the function is called every time the value changes
    register_callback_func(led_power_pin, &led_power_print, NULL);
    register_callback_func(button_pin, &led_power_on, NULL);
    //With _flip_func, the function is only called when the pin's value changes, then
    //changes back. This is most useful with buttons (think of it as an on_release trigger)
    register_callback_flip_func(toggle_pin, &led_power_toggle, NULL);
    
    //Start polling the pins on a separate thread
    start_callback_manager();

    printf("Press one button to turn the LED on, press another to toggle the LED.\n");

    //You can register new callbacks after calling start/setup_callback_manager, but be
    //aware the thread will be destroyed and recreated. In addition, keep in mind that
    //the callback manager must be initialized BEFORE registering callback functions.
    
    //If desired, you can set a delay between each time callback manager polls the pins
    //set_callback_polling_delay(500); //delay of 500 nanoseconds

    //Let the callback manager thread run for a while
    sleep(20);
    printf("Your 20 seconds are up! Shutting down.\n");

    //If desired, you can pause and unpause callback manager's polling of pins
    //pause_callback_manager();
    //unpause_callback_manager();

    //You can manually remove callback functions
    //remove_callback_func(led_power_pin);
    //Or let terminate handle it
    terminate_callback_manager(); //ALWAYS call terminate when you're done

    //You may manually close pins
    //close_gpio_pin(led_power_pin);
    //Or automatically close all pins opened by open_gpio_pin
    //autoclose_gpio_pins();

    //Regardless, however, always call terminate_gpio_interface when done
    terminate_gpio_interface(); //this function also calls autoclose_gpio_pins

    return 0;
}

//Do keep in mind that callback functions run on a separate thread
int led_power_print(pin_change_t pin, void* arg)
{
    if (pin.new_val == GPIO_PIN_HIGH)
    { printf("LED is ON.\n"); }
    else
    { printf("LED is OFF.\n"); }

    return pin.new_val;
}

int led_power_on(pin_change_t pin, void* arg)
{
    return set_gpio_val(led_power_pin, !pin.new_val);
}

int led_power_toggle(pin_change_t pin, void* arg)
{
    return toggle_gpio_val(led_power_pin);
}
