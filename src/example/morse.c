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

int main (int argc, char **argv)
{
    //You must call initialize_gpio_interface before use. Sub-zero values are errors.
    if (initialize_gpio_interface() < 0)
    { fprintf(stderr, "GPIO Error. Shutting down.\n"); return -1; }

    //If desired, you may attempt to close pins before trying to open them to ensure
    //access. This is not recommended, however, as it may interfere with other programs.

    //This is the ideal way to declare and open pins
    int lcd_power_pin = get_gpio_pin_num_from_name("XIO-P7");
    if (setup_gpio_pin(lcd_power_pin, GPIO_DIR_OUT) < 0)
    { fprintf(stderr, "GPIO Error. Shutting down.\n"); return -1; }

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
    *   (E.g. !DO NOT! do setup_gpio_pin_n(37, GPIO_DIR_IN) or any other plain number)
    *
    *   The library is designed so that if the pinout of the CHIP were ever changed or
    *   expanded, the library can be modified and recompiled independently of the program.
    *   This way, existing pins can change location and new pins can be added WITHOUT
    *   breaking existing programs coded to run on old version of the CHIP.
    *   Sending pin numbers directly WILL break that forwards-compatibility. In the interest
    *   of future-proofing your program, use the recommended method.
    */

    //NOTE: LCD pins require pull-up resistors. A 10k Ohm Resistor connected to 3.3V works
    int button_pin = get_gpio_pin_num_from_name("LCD-VSYNC");
    //setup_gpio_pin is a convenience function that calls open_gpio_pin and set_gpio_dir
    //You may do so separately if you please.
    if (open_gpio_pin(button_pin) < 0)
    { fprintf(stderr, "GPIO Error. Shutting down.\n"); return -1; }
    if (set_gpio_dir(button_pin, GPIO_DIR_IN) < 0) //Don't assume the direction of pins
    { fprintf(stderr, "GPIO Error. Shutting down.\n"); return -1; }
    
    //This is "SOS" in morse code - three shorts, three longs, three shorts
    int morse_message[] = {0,0,0,1,1,1,0,0,0};

    printf("Waiting for user to press the button.\n");
    
    //Pins are HIGH until pulled LOW (to ground) e.g. by a jumper or a pressed button
    //Busy loop waiting for a button press to start
    while (read_gpio_val(button_pin) != GPIO_PIN_LOW)
    { usleep(10); }
    //Wait for button to be released
    while (read_gpio_val(button_pin) != GPIO_PIN_HIGH)
    { usleep(10); }

    //Note: You can and should check for errors on reading and writing. I have ommitted
    //      this for the sake of brevity.

    //Decide on how long a short and a long will be for morse code
    int short_pulse = 500000; // half a second with usleep()
    int long_pulse = 1; // one second with sleep()

    printf("Hold the button to stop\n");

    //keep going until we get another button press to stop
    while (read_gpio_val(button_pin) != 0)
    {
        for (int i = 0; i < sizeof(morse_message); i++)
        {
            //Power the LED to send the signal
            set_gpio_val(lcd_power_pin, 1);
            if (!morse_message[i]) { usleep(short_pulse); }
            else { sleep(long_pulse); }

            //Wait for the length of a short pulse before next pulse
            set_gpio_val(lcd_power_pin, 0);
            usleep(short_pulse);

            //allow the user to break early
            if (read_gpio_val(button_pin) == 0)
            { break; }
        }
    }
    
    //You may manually close pins
    //close_gpio_pin(lcd_power_pin);
    //Or automatically close all pins opened by open_gpio_pin
    //autoclose_gpio_pins();

    //Regardless, however, always call terminate_gpio_interface when done
    terminate_gpio_interface(); //this function also calls autoclose_gpio_pins

    return 0;
}
