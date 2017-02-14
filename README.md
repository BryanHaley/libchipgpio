# libchipgpio
Dynamic C Library for interfacing with the GPIO pins on the C.H.I.P. mini computer.
One of the main focuses behind this library is to provide "forward-compatibility" with possible future revisions of the CHIP. Accessible pins can be added, removed, and modified without breaking compatibility with programs written to run on older revisions of the CHIP.

BUILDING
--------

1. Install essential packages

  `sudo apt-get install build-essential git`

2. Clone this repository

  `git clone https://github.com/BryanHaley/libchipgpio.git`

3. Build and install

  `cd libchipgpio`
  
  `make all`
  
  `sudo make install`
  
4. (Optional) Run the example program

  `sudo ./example`
  
  + Connect an LED to pin XIO-P7 and a button to LCD-VSYNC (with a pull-up resistor) for the example program.
  
USAGE
-----

Preamble: this library does require some mindfulness from the coder using it to maintain forward compatibility. Please read *and follow* the best practices section.

libchipgpio provides the following functions to interface with the CHIP's GPIO pins. Include `chip_gpio.h` and link `lchipgpio` to use them.

+ `initialize_gpio_interface()`
  
  + Must be called before interfacing with any GPIO pins.
  
  + `init_gpio_inf()`

    + Same as above, except a shorter (less-explicit) name.
  
+ `open_gpio_pin(int pin)`

  + Pins must be opened before they can be read from or written to. This effectively tells the system to begin monitoring the pin. See best practices section for what argument to pass here.
  
+ `set_gpio_dir(int pin, int out)`

  + You must set a direction (input or output) for GPIO pins. Use GPIO_DIR_OUT and GPIO_DIR_IN.
  
+ `setup_gpio_pin(int pin, int out)`

  + Convenience function to call `open_gpio_pin` and `set_gpio_dir` in one line, since it is necessary to do both before a pin can be used.
  
+ `set_gpio_val(int pin, int val`

  + Write a value (1 or 0, GPIO_PIN_ON or GPIO_PIN_OFF, GPIO_PIN_HIGH or GPIO_PIN_LOW) to a pin. The pin must be in the *out* direction to do this. Attempting to do otherwise will cause an error.
  
+ `read_gpio_val(int pin)`

  + Returns the value (1 or 0, GPIO_PIN_ON or GPIO_PIN_OFF, GPIO_PIN_HIGH or GPIO_PIN_LOW) of a pin. The pin must be in the *in* direction to do this. Attempting to do otherwise will cause an error.
  
+ `close_gpio_pin(int pin)`

  + Pins should be closed when no longer in use. This effectively tells the system to stop monitoring the pin. You *can* attempt to close pins before opening them (this will cause a warning) to ensure you will be able to open the pin. However, this isn't recommended, as if the pins aren't closed before start-up it's most likely because you didn't close them properly the last time the program ran, or some other program/script is actively using them.
  
+ `is_gpio_pin_open(int pin)`

  + Returns 1 or 0 based on if the GPIO pin is currently open. Can be used to prevent errors on startup, and/or warn users.
  
+ `get_gpio_pin_num_from_name(char* name)`
  
  + You **must** use this to get the pin number that should be passed to the other functions in this list. Otherwise, forward compatibility will be broken. See the best practices section.

  +  `get_gpio_num(char* name)`
  
    + Same as above, except a shorter (less-explicit) name. Seriously, use one of these.

+ `get_gpio_xio_base()`

  + Not explicitly useful, but if you need the kernel-recognized identifier number for the first XIO pin, you can get it here. This is found based on files that exist in the `/sys/class/gpio` directory, and is kernel-version-agnostic.
  
+ `autoclose_gpio_pins()`

  + You don't *need* to keep track of all the pins you've opened - libchipgpio will do this automatically. You can close all of the pins opened by your program by calling this function.
  
+ `terminate_gpio_interface()`

  + Closes the GPIO file handles and calls autoclose_gpio_pins(). Call this before your program exits, or when you're completely done with GPIO.
  
  + `term_gpio_inf()`
  
    + Same as above, except a shorter (less-explicit) name.
    
+ `_n(char* name` variants
  
  + Most of the functions above have a variant with the suffix `_n` and accept a pin name string instead of an integer (e.g. `open_gpio_pin("XIO-P0");`). This is compliant with the best practices, however it comes at a performance cost (the pin name string must be compared to a list of pin name strings, which can be costly if done in a loop). Might be useful when grabbing user input or something, but otherwise, stick to the method outlined in the best practices section.
    
BEST PRACTICES
--------------

+ Use `get_gpio_num(char* name)` to get a GPIO pin's number.

  + **This is by far the most important one to follow.** Supply this function with a pin name string (e.g. "LCD-D4"), and it will return an integer you may use with the above functions (as the `int pin` argument). **Do not directly pass a pin number to the above functions.** In order to maintain forwards compatibility, the possibility of the GPIO pinout changing must be accounted for. If pin numbers are passed directly (e.g. do not do `open_gpio_pin(37);` for LCD-VSYNC) forwards compatibility will probably be broken, as libchipgpio has no way of knowing if Pin 37 will be a usable GPIO pin on future revisions of the CHIP.
  
  + Example of a proper way to setup a pin:
  
    + `int start_button_pin = get_gpio_num("XIO-P7");`
      
      `setup_gpio_pin(start_button_pin, GPIO_DIR_IN);`
    
  + start_button_pin can be reused; you only need to call get_gpio_num once for each pin.
  
  + As of February 2017, the current pinout of the current version of the CHIP is the following:

     ![alt text](https://docs.getchip.com/images/chip_pinouts.jpg)

     + Use these names when accessing the pins. According to the documentation, pins U13 18 to 40 (PWM0 to LCD-DE), U14 13-20 (XIO-P0 to XIO-P7), and U14 27-40 (CSIPCK to CSID7 plus two ground pins) are usable as GPIO. Note however, a few of those are ground pins (and as such are not usable as input/output), and I’m pretty sure pin 17 (LCD-D2) is perfectly acceptable to be used (typo?). There are, effectively, 44 usable GPIO pins at the current time. This is reflected in the current version of the library. Keep in mind if you use an HDMI or VGA adapter, many or all of the non-XIO pins will be unavailable. Use a TRRS->RCA cable or run the CHIP headless with SSH or VNC for projects that use many of GPIO pins.
  
+ Check for errors.

  + Every function in the libchipgpio interface will print information to stderr and return -1 if a major error occurs. Use that to troubleshoot and/or produce meaningful error messages.
  
  + Following the previous example:
  
    + `if (setup_gpio_pin(start_button_pin, GPIO_DIR_IN) < 0)`
      
      `{ fprintf(stderr, "Encountered GPIO on startup. Shutting down.”); return -1; }`
       
   + libchipgpio functions will generate meaningful error messages on stderr on their own, so it may be more helpful to specify *where* the error occurred than the error itself.
   
   + In particular, monitor `open_gpio_pin` for errors. If you get an error here, check if the pin is already open, prompt the user to allow you to close it (or at least warn them that you will), then try again to open the pin. Keep in mind other programs or scripts may be using GPIO pins, so only close a GPIO pin you didn't open if absolutely necessary.
       
+ Ensure you close the pins by the time your program ends.

  + Leaving a pin open can and will cause errors the next time you open your program. Libchipgpio needs to open pins itself in order to keep track of which pins it can close and to help ensure no other program or script will interfere while your program is running.
  
+ Your user will need this library installed to run your program.

  + This library is dynamically linked so that is may be updated independently of your program (again to help provide forwards compatibility). Therefore, it must be installed on the user's system. I am looking into making a ppa to make this a more streamlined process, but until then, consider automating the cloning, building, and installing of this library as shown in the Building section. Or, at the very least, link to this git repository and inform the user of its necessity.

+ If you need to change the available pins on the CHIP (perhaps a new revision has come out and I have not yet updated the library, for example), edit the `chip_gpio_pin_defs.h` file. Theoretically, it should be possible to make all of the necessary changes by only editing that file (unless some seriously drastic changes have been made to the CHIP pinout). Once you’re done making edits, build and install your new version of the library.
  
TODO
----

+ Add wrappers for interfacing with i2c, SPI, and PWM.
  
  + I'm currently awaiting a shipment containing an i2c/spi LCD screen to test with.

  + In the future, I’ll also look into making expanding GPIO via an i2c GPIO chip as easy as possible (ideally, integrating with the libchipgpio interface as if they were on the CHIP itself). It would be useful for those using VGA/HDMI adapters.

OPEN SOURCE LICENSE
-------------------

This software is dual-licensed (GPLv2 and SimplifiedBSD). Choose whichever works best for you.

```
GNU General Public License, version 2

LIBCHIPGPIO - Dynamic library to interface with GPIO pins on the C.H.I.P. mini computer.
Copyright (C) 2017 Bryan Haley

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
```

```
BSD 2-Clause License

Copyright (c) 2017, Bryan Haley
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```