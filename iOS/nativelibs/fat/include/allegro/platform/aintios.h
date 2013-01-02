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
 *      Internal header file for the Android Allegro library port.
 *
 *      By JJS for the Adventure Game Studio runtime port.
 *      Based on the Allegro PSP port.
 *
 *      See readme.txt for copyright information.
 */


#ifndef AINTIOS_H
#define AINTIOS_H

AL_FUNC(void *, _ios_create_mutex, (void));
AL_FUNC(void, _ios_destroy_mutex, (void *handle));
AL_FUNC(void, _ios_lock_mutex, (void *handle));
AL_FUNC(void, _ios_unlock_mutex, (void *handle));

void ios_mouse_setup(int left, int right, int top, int bottom, float scaling_x, float scaling_y);

#endif

