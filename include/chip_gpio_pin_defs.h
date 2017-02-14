/*
 * Copyright (c) 2017, Bryan Haley
 * This code is dual licensed (GPLv2 and Simplified BSD). Use the license that works
 * best for you. Check LICENSE.GPL and LICENSE.BSD for more details.
 *
 * chip_gpio_pin_defs.h
 * Header file used to define pins in a way we can easily interpret. This file can be
 * edited to add, remove, or modify pins recognized by the library with relative ease.
 * Doing so should not break programs compiled against older versions of the library,
 * unless you remove a pin it relied upon.
 */

#ifndef CHIP_GPIO_PIN_DEFS_H
#define CHIP_GPIO_PIN_DEFS_H

//Some definitions that detail what the CHIP pinout is like in a way we can interpret

#define FIRST_PIN          1 // First pin is 1 since that's how it's labeled. 0 is
#define NUM_PINS          80 // unused. Keep that in mind when using NUM_PINS.
#define U13_END           40
#define U14_OFFSET        40 //These are the same now, but may not always be.

//If anything has changed with the XIO pins, it's important to reflect those
#define NUM_XIO_U14_PINS   8 //                                 changes here
#define XIO_U14_FIRST_PIN 13 //Use the _ALL variant for pre-baked U14 offset

#define LCD_U13_FIRST_PIN 17
#define LCD_U13_LAST_PIN  40

//Use LCD_U14_(FIRST/LAST)_PIN_ALL for pre-baked U14 offset
#define LCD_U14_FIRST_PIN 27
#define LCD_U14_LAST_PIN  38 //39, 40 is GND

//If the multiplier of a pin is this, that means it's definitely not an R8 pin
#define GPIO_UNUSED '\0'

// The following are definitions assigning pin numbers to pin names as seen here:
// goo.gl/zpt4ge (for the current version of the CHIP, anyway).
// They are to be used ONLY by libchipgpio internally. Since these are pre-
// processor definitions, they will be baked into whatever includes this file
// and cannot be changed without recompiling. Therefore, if a program uses
// these, it will break forwards-compatibility and the program will need to be
// recompiled if a pinout change breaks it. When best practices are followed,
// only libchipgpio should need to be recompiled.

//U13
#define     GPIO_LCD_D2     17
#define     GPIO_PWM0       18
#define     GPIO_LCD_D4     19
#define     GPIO_LCD_D3     20
#define     GPIO_LCD_D6     21
#define     GPIO_LCD_D5     22
#define     GPIO_LCD_D10    23
#define     GPIO_LCD_D7     24
#define     GPIO_LCD_D12    25
#define     GPIO_LCD_D11    26
#define     GPIO_LCD_D14    27
#define     GPIO_LCD_D13    28
#define     GPIO_LCD_D18    29
#define     GPIO_LCD_D15    30
#define     GPIO_LCD_D20    31
#define     GPIO_LCD_D19    32
#define     GPIO_LCD_D22    33
#define     GPIO_LCD_D21    34
#define     GPIO_LCD_CLK    35
#define     GPIO_LCD_D23    36
#define     GPIO_LCD_VSYNC  37
#define     GPIO_LCD_HSYNC  38
#define     GPIO_LCD_DE     40

//U14
#define     GPIO_XIO_P0     13 + U14_OFFSET
#define     GPIO_XIO_P1     14 + U14_OFFSET
#define     GPIO_XIO_P2     15 + U14_OFFSET
#define     GPIO_XIO_P3     16 + U14_OFFSET
#define     GPIO_XIO_P4     17 + U14_OFFSET
#define     GPIO_XIO_P5     18 + U14_OFFSET
#define     GPIO_XIO_P6     19 + U14_OFFSET
#define     GPIO_XIO_P7     20 + U14_OFFSET

#define	    GPIO_CSIPCK	    27 + U14_OFFSET
#define	    GPIO_CSICK	    28 + U14_OFFSET
#define	    GPIO_CSIHSYNC   29 + U14_OFFSET
#define	    GPIO_CSIVSYNC   30 + U14_OFFSET
#define	    GPIO_CSID0	    31 + U14_OFFSET
#define	    GPIO_CSID1	    32 + U14_OFFSET
#define	    GPIO_CSID2	    33 + U14_OFFSET
#define	    GPIO_CSID3	    34 + U14_OFFSET
#define	    GPIO_CSID4	    35 + U14_OFFSET
#define	    GPIO_CSID5	    36 + U14_OFFSET
#define	    GPIO_CSID6	    37 + U14_OFFSET
#define	    GPIO_CSID7	    38 + U14_OFFSET

char* PIN_UNUSED;
char* XIO_CHIP_LABEL;

//struct containing information to identify a gpio pin
//see: goo.gl/vQLRuW (pages 18 to 20), goo.gl/1cGAZw
typedef struct
{
    char* name;
    char mult;
    int off; 
    //Use of the following is not recommended, but may be necessary.
    int hard_coded_kern_pin;
} pin_identifier;

//  Using hard_coded_kern_pin would not be kernel-agnostic. However, it appears only
//  XIO pins get new kernel identifier numbers with each kernel version, so it may
//  not be an issue. R8 (LCD and CSID) pins do not depend on the kernel, for example.
//  Regardless, avoid using hard_coded_kern_pin if at all possible.

pin_identifier p_ident[NUM_PINS+1];

//This is called in initialize_gpio_interface() before anything else
static int initialize_gpio_pin_names()
{
    PIN_UNUSED = "DO NOT USE";
    // this is the i2c chip handling the XIO pins
    XIO_CHIP_LABEL = "pcf8574a"; // Currently, the CHIP uses this label to identify
                                 // the correct directory containing the xio base
                                 // number, so we just need to know this beforehand.

    for (int i = 0; i < NUM_PINS+1; i++)
    {
	//Unused pins will all have this info
        p_ident[i].name = PIN_UNUSED;
        p_ident[i].mult =  GPIO_UNUSED;
        p_ident[i].off  = -1;
        p_ident[i].hard_coded_kern_pin = -1;
    }
	
    // Here, we define the name (by which users of this library should access
    // the pins by) and other identifying information. XIO pins should only
    // need the name.
    
    //U13
    p_ident[GPIO_LCD_D2].name        = "LCD-D2";
        p_ident[GPIO_LCD_D2].mult    = 'D'; p_ident[GPIO_LCD_D2].off    =  2;
    p_ident[GPIO_PWM0].name          = "PWM0";
        p_ident[GPIO_PWM0].mult      = 'B'; p_ident[GPIO_PWM0].off      =  2;
    p_ident[GPIO_LCD_D4].name        = "LCD-D4";
        p_ident[GPIO_LCD_D4].mult    = 'D'; p_ident[GPIO_LCD_D4].off    =  4;
    p_ident[GPIO_LCD_D3].name        = "LCD-D3";
        p_ident[GPIO_LCD_D3].mult    = 'D'; p_ident[GPIO_LCD_D3].off    =  3;
    p_ident[GPIO_LCD_D6].name        = "LCD-D6";
        p_ident[GPIO_LCD_D6].mult    = 'D'; p_ident[GPIO_LCD_D6].off    =  6;
    p_ident[GPIO_LCD_D5].name        = "LCD-D5";
        p_ident[GPIO_LCD_D5].mult    = 'D'; p_ident[GPIO_LCD_D5].off    =  5;
    p_ident[GPIO_LCD_D10].name       = "LCD-D10";
        p_ident[GPIO_LCD_D10].mult   = 'D'; p_ident[GPIO_LCD_D10].off   = 10;
    p_ident[GPIO_LCD_D7].name        = "LCD-D7";
        p_ident[GPIO_LCD_D7].mult    = 'D'; p_ident[GPIO_LCD_D7].off    =  7;
    p_ident[GPIO_LCD_D12].name       = "LCD-D12";
        p_ident[GPIO_LCD_D12].mult   = 'D'; p_ident[GPIO_LCD_D12].off   = 12;
    p_ident[GPIO_LCD_D11].name       = "LCD-D11";
        p_ident[GPIO_LCD_D11].mult   = 'D'; p_ident[GPIO_LCD_D11].off   = 11;
    p_ident[GPIO_LCD_D14].name       = "LCD-D14";
        p_ident[GPIO_LCD_D14].mult   = 'D'; p_ident[GPIO_LCD_D14].off   = 14;
    p_ident[GPIO_LCD_D13].name       = "LCD-D13";
        p_ident[GPIO_LCD_D13].mult   = 'D'; p_ident[GPIO_LCD_D13].off   = 13;
    p_ident[GPIO_LCD_D18].name       = "LCD-D18";
        p_ident[GPIO_LCD_D18].mult   = 'D'; p_ident[GPIO_LCD_D18].off   = 18;
    p_ident[GPIO_LCD_D15].name       = "LCD-D15";
        p_ident[GPIO_LCD_D15].mult   = 'D'; p_ident[GPIO_LCD_D15].off   = 15;
    p_ident[GPIO_LCD_D20].name       = "LCD-D20";
        p_ident[GPIO_LCD_D20].mult   = 'D'; p_ident[GPIO_LCD_D20].off   = 20;
    p_ident[GPIO_LCD_D19].name       = "LCD-D19";
        p_ident[GPIO_LCD_D19].mult   = 'D'; p_ident[GPIO_LCD_D19].off   = 19;
    p_ident[GPIO_LCD_D22].name       = "LCD-D22";
        p_ident[GPIO_LCD_D22].mult   = 'D'; p_ident[GPIO_LCD_D22].off   = 22;
    p_ident[GPIO_LCD_D21].name       = "LCD-D21";
        p_ident[GPIO_LCD_D21].mult   = 'D'; p_ident[GPIO_LCD_D21].off   = 21;
    p_ident[GPIO_LCD_CLK].name       = "LCD-CLK";
        p_ident[GPIO_LCD_CLK].mult   = 'D'; p_ident[GPIO_LCD_CLK].off   = 24;
    p_ident[GPIO_LCD_D23].name       = "LCD-D23";
        p_ident[GPIO_LCD_D23].mult   = 'D'; p_ident[GPIO_LCD_D23].off   = 23;
    p_ident[GPIO_LCD_VSYNC].name     = "LCD-VSYNC";
        p_ident[GPIO_LCD_VSYNC].mult = 'D'; p_ident[GPIO_LCD_VSYNC].off = 27;
    p_ident[GPIO_LCD_HSYNC].name     = "LCD-HSYNC";
        p_ident[GPIO_LCD_HSYNC].mult = 'D'; p_ident[GPIO_LCD_HSYNC].off = 26;
    p_ident[GPIO_LCD_DE].name        = "LCD-DE";
        p_ident[GPIO_LCD_DE].mult    = 'D'; p_ident[GPIO_LCD_DE].off    = 25;

    //U14
    p_ident[GPIO_XIO_P0].name = "XIO-P0";
    p_ident[GPIO_XIO_P1].name = "XIO-P1";
    p_ident[GPIO_XIO_P2].name = "XIO-P2";
    p_ident[GPIO_XIO_P3].name = "XIO-P3";
    p_ident[GPIO_XIO_P4].name = "XIO-P4";
    p_ident[GPIO_XIO_P5].name = "XIO-P5";
    p_ident[GPIO_XIO_P6].name = "XIO-P6";
    p_ident[GPIO_XIO_P7].name = "XIO-P7";
	

    p_ident[GPIO_CSIPCK].name       = "CSIPCK";
        p_ident[GPIO_CSIPCK].mult   = 'E'; p_ident[GPIO_CSIPCK].off   =  0;
    p_ident[GPIO_CSICK].name        = "CSICK";
        p_ident[GPIO_CSICK].mult    = 'E'; p_ident[GPIO_CSICK].off    =  1;
    p_ident[GPIO_CSIHSYNC].name     = "CSIHSYNC";
        p_ident[GPIO_CSIHSYNC].mult = 'E'; p_ident[GPIO_CSIHSYNC].off =  2;
    p_ident[GPIO_CSIVSYNC].name     = "CSIVSYNC";
        p_ident[GPIO_CSIVSYNC].mult = 'E'; p_ident[GPIO_CSIVSYNC].off =  3;
    p_ident[GPIO_CSID0].name        = "CSID0";
        p_ident[GPIO_CSID0].mult    = 'E'; p_ident[GPIO_CSID0].off    =  4;
    p_ident[GPIO_CSID1].name        = "CSID1";
        p_ident[GPIO_CSID1].mult    = 'E'; p_ident[GPIO_CSID1].off    =  5;
    p_ident[GPIO_CSID2].name        = "CSID2";
        p_ident[GPIO_CSID2].mult    = 'E'; p_ident[GPIO_CSID2].off    =  6;
    p_ident[GPIO_CSID3].name        = "CSID3";
        p_ident[GPIO_CSID3].mult    = 'E'; p_ident[GPIO_CSID3].off    =  7;
    p_ident[GPIO_CSID4].name        = "CSID4";
        p_ident[GPIO_CSID4].mult    = 'E'; p_ident[GPIO_CSID4].off    =  8;
    p_ident[GPIO_CSID5].name        = "CSID5";
        p_ident[GPIO_CSID5].mult    = 'E'; p_ident[GPIO_CSID5].off    =  9;
    p_ident[GPIO_CSID6].name        = "CSID6";
        p_ident[GPIO_CSID6].mult    = 'E'; p_ident[GPIO_CSID6].off    = 10;
    p_ident[GPIO_CSID7].name        = "CSID7";
        p_ident[GPIO_CSID7].mult    = 'E'; p_ident[GPIO_CSID7].off    = 11;
    
    return 0;
}

#endif
