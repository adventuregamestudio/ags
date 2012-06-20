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


#ifndef AINTAND_H
#define AINTAND_H

#include <jni.h>

void android_allegro_initialize_jni(JNIEnv* env, jclass java_class, jobject java_object);

void android_debug_printf(char* format, ...);

void android_attach_current_thread();
void android_detach_current_thread();

int android_poll_keyboard();

int android_poll_mouse_absolute();
int android_poll_mouse_relative();
int android_poll_mouse_buttons();
void android_mouse_setup(int left, int right, int top, int bottom, float scaling_x, float scaling_y);

void android_initialize_sound(int size);
void android_update_sound(char* buffer, unsigned int size);

void android_create_screen(int width, int height, int color_depth);
void android_initialize_renderer(JNIEnv* env, jobject self, jint screen_width, jint screen_height);

void android_swap_buffers();

BITMAP* displayed_video_bitmap;

#endif

