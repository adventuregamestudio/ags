/*         ______   ___    ___
 *        /\  _  \ /\_ \  /\_ \
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      Android specific header defines.
 *
 *      By JJS for the Adventure Game Studio runtime port.
 *      Based on the Allegro PSP port.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALIOS_H
#define ALIOS_H

#ifndef ALLEGRO_IOS
   #error bad include
#endif

#include <stdio.h>


/* The Android C library doesn't include stricmp */
#define stricmp strcasecmp

/* System driver */
#define SYSTEM_IOS              AL_ID('I','O','S',' ')
AL_VAR(SYSTEM_DRIVER, system_ios);

/* Timer driver */
#define TIMER_IOS               AL_ID('I','O','S','T')
AL_VAR(TIMER_DRIVER, timer_ios);

/* Keyboard driver */
#define KEYBOARD_IOS            AL_ID('I','O','S','K')
AL_VAR(KEYBOARD_DRIVER, keyboard_ios);

/* Mouse drivers */
#define MOUSE_IOS               AL_ID('I','O','S','M')
AL_VAR(MOUSE_DRIVER, mouse_ios);

/* Gfx driver */
#define GFX_IOS                 AL_ID('I','O','S','G')
AL_VAR(GFX_DRIVER, gfx_ios);

/* Digital sound driver */
#define DIGI_IOS                AL_ID('I','O','S','S')
AL_VAR(DIGI_DRIVER, digi_ios);

/* Joystick drivers */
#define JOYSTICK_IOS            AL_ID('I','O','S','J')
AL_VAR(JOYSTICK_DRIVER, joystick_ios);





#endif
