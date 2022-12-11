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
 *      Some definitions for internal use by the library code.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef AINTERN_H
#define AINTERN_H

#ifndef ALLEGRO_H
   #error must include allegro.h first
#endif

#ifdef __cplusplus
   extern "C" {
#endif


/* length in bytes of the cpu_vendor string */
#define _AL_CPU_VENDOR_SIZE 32


/* flag for how many times we have been initialised */
AL_VAR(int, _allegro_count);


/* flag to know whether we are being called by the exit mechanism */
AL_VAR(int, _allegro_in_exit);


AL_FUNCPTR(int, _al_trace_handler, (AL_CONST char *msg));


/* malloc wrappers */
/* The 4.3 branch uses the following macro names to allow the user to customise
 * the memory management routines.  We don't have that feature in 4.2, but we
 * use the same macros names in order to reduce divergence of the codebases.
 */
#define _AL_MALLOC(SIZE)         (_al_malloc(SIZE))
#define _AL_MALLOC_ATOMIC(SIZE)  (_al_malloc(SIZE))
#define _AL_FREE(PTR)            (_al_free(PTR))
#define _AL_REALLOC(PTR, SIZE)   (_al_realloc(PTR, SIZE))

AL_FUNC(void *, _al_malloc, (size_t size));
AL_FUNC(void, _al_free, (void *mem));
AL_FUNC(void *, _al_realloc, (void *mem, size_t size));
AL_FUNC(char *, _al_strdup, (AL_CONST char *string));
AL_FUNC(char *, _al_ustrdup, (AL_CONST char *string));



/* some Allegro functions need a block of scratch memory */
AL_VAR(void *, _scratch_mem);
AL_VAR(int, _scratch_mem_size);


AL_INLINE(void, _grow_scratch_mem, (int size),
{
   if (size > _scratch_mem_size) {
      size = (size+1023) & 0xFFFFFC00;
      _scratch_mem = _AL_REALLOC(_scratch_mem, size);
      _scratch_mem_size = size;
   }
})


/* list of functions to call at program cleanup */
AL_FUNC(void, _add_exit_func, (AL_METHOD(void, func, (void)), AL_CONST char *desc));
AL_FUNC(void, _remove_exit_func, (AL_METHOD(void, func, (void))));


/* helper structure for talking to Unicode strings */
typedef struct UTYPE_INFO
{
   int id;
   AL_METHOD(int, u_getc, (AL_CONST char *s));
   AL_METHOD(int, u_getx, (char **s));
   AL_METHOD(int, u_setc, (char *s, int c));
   AL_METHOD(int, u_width, (AL_CONST char *s));
   AL_METHOD(int, u_cwidth, (int c));
   AL_METHOD(int, u_isok, (int c));
   int u_width_max;
} UTYPE_INFO;

AL_FUNC(UTYPE_INFO *, _find_utype, (int type));


/* message stuff */
#define ALLEGRO_MESSAGE_SIZE  4096


/* wrappers for implementing disk I/O on different platforms */
AL_FUNC(int, _al_file_isok, (AL_CONST char *filename));
AL_FUNC(uint64_t, _al_file_size_ex, (AL_CONST char *filename));
AL_FUNC(time_t, _al_file_time, (AL_CONST char *filename));
AL_FUNC(int, _al_drive_exists, (int drive));
AL_FUNC(int, _al_getdrive, (void));
AL_FUNC(void, _al_getdcwd, (int drive, char *buf, int size));



#if (defined ALLEGRO_WINDOWS)

   AL_FUNC(int, _al_win_open, (const char *filename, int mode, int perm));


   #define _al_open(filename, mode, perm)   _al_win_open(filename, mode, perm)

#else

   #define _al_open(filename, mode, perm)   open(filename, mode, perm)

#endif



/* stuff for setting up bitmaps */
AL_FUNC(GFX_VTABLE *, _get_vtable, (int color_depth));

AL_VAR(int, _sub_bitmap_id_count);


#ifdef ALLEGRO_I386
   #define BYTES_PER_PIXEL(bpp)     (((int)(bpp) + 7) / 8)
#else
   #ifdef ALLEGRO_MPW 
      /* in Mac 24 bit is a unsigned long */
      #define BYTES_PER_PIXEL(bpp)  (((bpp) <= 8) ? 1					\
				     : (((bpp) <= 16) ? 2		\
					: 4))
   #else
      #define BYTES_PER_PIXEL(bpp)  (((bpp) <= 8) ? 1					\
				     : (((bpp) <= 16) ? 2		\
					: (((bpp) <= 24) ? 3 : 4)))
   #endif
#endif

AL_FUNC(int, _color_load_depth, (int depth, int hasalpha));

AL_VAR(int, _color_conv);

AL_FUNC(int, _bitmap_has_alpha, (BITMAP *bmp));

/* default truecolor pixel format */
#define DEFAULT_RGB_R_SHIFT_15  0
#define DEFAULT_RGB_G_SHIFT_15  5
#define DEFAULT_RGB_B_SHIFT_15  10
#define DEFAULT_RGB_R_SHIFT_16  0
#define DEFAULT_RGB_G_SHIFT_16  5
#define DEFAULT_RGB_B_SHIFT_16  11
#define DEFAULT_RGB_R_SHIFT_24  0
#define DEFAULT_RGB_G_SHIFT_24  8
#define DEFAULT_RGB_B_SHIFT_24  16
#define DEFAULT_RGB_R_SHIFT_32  0
#define DEFAULT_RGB_G_SHIFT_32  8
#define DEFAULT_RGB_B_SHIFT_32  16
#define DEFAULT_RGB_A_SHIFT_32  24



/* current drawing mode */
AL_VAR(int, _drawing_mode);
AL_VAR(BITMAP *, _drawing_pattern);
AL_VAR(int, _drawing_x_anchor);
AL_VAR(int, _drawing_y_anchor);
AL_VAR(unsigned int, _drawing_x_mask);
AL_VAR(unsigned int, _drawing_y_mask);

AL_FUNCPTR(int *, _palette_expansion_table, (int bpp));

AL_VAR(int, _color_depth);

AL_VAR(int, _current_palette_changed);
AL_VAR(PALETTE, _prev_current_palette);
AL_VAR(int, _got_prev_current_palette);

AL_ARRAY(int, _palette_color8);
AL_ARRAY(int, _palette_color15);
AL_ARRAY(int, _palette_color16);
AL_ARRAY(int, _palette_color24);
AL_ARRAY(int, _palette_color32);

/* truecolor blending functions */
AL_VAR(BLENDER_FUNC, _blender_func15);
AL_VAR(BLENDER_FUNC, _blender_func16);
AL_VAR(BLENDER_FUNC, _blender_func24);
AL_VAR(BLENDER_FUNC, _blender_func32);

AL_VAR(BLENDER_FUNC, _blender_func15x);
AL_VAR(BLENDER_FUNC, _blender_func16x);
AL_VAR(BLENDER_FUNC, _blender_func24x);

AL_VAR(int, _blender_col_15);
AL_VAR(int, _blender_col_16);
AL_VAR(int, _blender_col_24);
AL_VAR(int, _blender_col_32);

AL_VAR(int, _blender_alpha);

AL_FUNC(unsigned long, _blender_black, (unsigned long x, unsigned long y, unsigned long n));

#ifdef ALLEGRO_COLOR16

AL_FUNC(unsigned long, _blender_trans15, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_add15, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_burn15, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_color15, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_difference15, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_dissolve15, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_dodge15, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_hue15, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_invert15, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_luminance15, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_multiply15, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_saturation15, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_screen15, (unsigned long x, unsigned long y, unsigned long n));

AL_FUNC(unsigned long, _blender_trans16, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_add16, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_burn16, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_color16, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_difference16, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_dissolve16, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_dodge16, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_hue16, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_invert16, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_luminance16, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_multiply16, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_saturation16, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_screen16, (unsigned long x, unsigned long y, unsigned long n));

#endif

#if (defined ALLEGRO_COLOR24) || (defined ALLEGRO_COLOR32)

AL_FUNC(unsigned long, _blender_trans24, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_add24, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_burn24, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_color24, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_difference24, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_dissolve24, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_dodge24, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_hue24, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_invert24, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_luminance24, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_multiply24, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_saturation24, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_screen24, (unsigned long x, unsigned long y, unsigned long n));

#endif

AL_FUNC(unsigned long, _blender_alpha15, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_alpha16, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_alpha24, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_alpha32, (unsigned long x, unsigned long y, unsigned long n));

AL_FUNC(unsigned long, _blender_write_alpha, (unsigned long x, unsigned long y, unsigned long n));


/* graphics drawing routines */
AL_FUNC(void, _normal_line, (BITMAP *bmp, int x1, int y_1, int x2, int y2, int color));
AL_FUNC(void, _fast_line, (BITMAP *bmp, int x1, int y_1, int x2, int y2, int color));
AL_FUNC(void, _normal_rectfill, (BITMAP *bmp, int x1, int y_1, int x2, int y2, int color));

#ifdef ALLEGRO_COLOR8

AL_FUNC(int,  _linear_getpixel8, (BITMAP *bmp, int x, int y));
AL_FUNC(void, _linear_putpixel8, (BITMAP *bmp, int x, int y, int color));
AL_FUNC(void, _linear_vline8, (BITMAP *bmp, int x, int y_1, int y2, int color));
AL_FUNC(void, _linear_hline8, (BITMAP *bmp, int x1, int y, int x2, int color));
AL_FUNC(void, _linear_draw_sprite8, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_sprite_ex8, (BITMAP *bmp, BITMAP *sprite, int x, int y, int mode, int flip));
AL_FUNC(void, _linear_draw_sprite_v_flip8, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_sprite_h_flip8, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_sprite_vh_flip8, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_trans_sprite8, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_lit_sprite8, (BITMAP *bmp, BITMAP *sprite, int x, int y, int color));
AL_FUNC(void, _linear_draw_rle_sprite8, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y));
AL_FUNC(void, _linear_draw_trans_rle_sprite8, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y));
AL_FUNC(void, _linear_draw_lit_rle_sprite8, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y, int color));
AL_FUNC(void, _linear_draw_character8, (BITMAP *bmp, BITMAP *sprite, int x, int y, int color, int bg));
AL_FUNC(void, _linear_blit8, (BITMAP *source,BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
AL_FUNC(void, _linear_blit_backward8, (BITMAP *source,BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
AL_FUNC(void, _linear_masked_blit8, (BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
AL_FUNC(void, _linear_clear_to_color8, (BITMAP *bitmap, int color));

#endif

#ifdef ALLEGRO_COLOR16

AL_FUNC(void, _linear_putpixel15, (BITMAP *bmp, int x, int y, int color));
AL_FUNC(void, _linear_vline15, (BITMAP *bmp, int x, int y_1, int y2, int color));
AL_FUNC(void, _linear_hline15, (BITMAP *bmp, int x1, int y, int x2, int color));
AL_FUNC(void, _linear_draw_trans_sprite15, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_trans_rgba_sprite15, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_lit_sprite15, (BITMAP *bmp, BITMAP *sprite, int x, int y, int color));
AL_FUNC(void, _linear_draw_rle_sprite15, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y));
AL_FUNC(void, _linear_draw_trans_rle_sprite15, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y));
AL_FUNC(void, _linear_draw_trans_rgba_rle_sprite15, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y));
AL_FUNC(void, _linear_draw_lit_rle_sprite15, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y, int color));

AL_FUNC(int,  _linear_getpixel16, (BITMAP *bmp, int x, int y));
AL_FUNC(void, _linear_putpixel16, (BITMAP *bmp, int x, int y, int color));
AL_FUNC(void, _linear_vline16, (BITMAP *bmp, int x, int y_1, int y2, int color));
AL_FUNC(void, _linear_hline16, (BITMAP *bmp, int x1, int y, int x2, int color));
AL_FUNC(void, _linear_draw_sprite16, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_sprite_ex16, (BITMAP *bmp, BITMAP *sprite, int x, int y, int mode, int flip));
AL_FUNC(void, _linear_draw_256_sprite16, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_sprite_v_flip16, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_sprite_h_flip16, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_sprite_vh_flip16, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_trans_sprite16, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_trans_rgba_sprite16, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_lit_sprite16, (BITMAP *bmp, BITMAP *sprite, int x, int y, int color));
AL_FUNC(void, _linear_draw_rle_sprite16, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y));
AL_FUNC(void, _linear_draw_trans_rle_sprite16, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y));
AL_FUNC(void, _linear_draw_trans_rgba_rle_sprite16, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y));
AL_FUNC(void, _linear_draw_lit_rle_sprite16, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y, int color));
AL_FUNC(void, _linear_draw_character16, (BITMAP *bmp, BITMAP *sprite, int x, int y, int color, int bg));
AL_FUNC(void, _linear_blit16, (BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
AL_FUNC(void, _linear_blit_backward16, (BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
AL_FUNC(void, _linear_masked_blit16, (BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
AL_FUNC(void, _linear_clear_to_color16, (BITMAP *bitmap, int color));

#endif

#ifdef ALLEGRO_COLOR24

AL_FUNC(int,  _linear_getpixel24, (BITMAP *bmp, int x, int y));
AL_FUNC(void, _linear_putpixel24, (BITMAP *bmp, int x, int y, int color));
AL_FUNC(void, _linear_vline24, (BITMAP *bmp, int x, int y_1, int y2, int color));
AL_FUNC(void, _linear_hline24, (BITMAP *bmp, int x1, int y, int x2, int color));
AL_FUNC(void, _linear_draw_sprite24, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_sprite_ex24, (BITMAP *bmp, BITMAP *sprite, int x, int y, int mode, int flip));
AL_FUNC(void, _linear_draw_256_sprite24, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_sprite_v_flip24, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_sprite_h_flip24, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_sprite_vh_flip24, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_trans_sprite24, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_trans_rgba_sprite24, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_lit_sprite24, (BITMAP *bmp, BITMAP *sprite, int x, int y, int color));
AL_FUNC(void, _linear_draw_rle_sprite24, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y));
AL_FUNC(void, _linear_draw_trans_rle_sprite24, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y));
AL_FUNC(void, _linear_draw_trans_rgba_rle_sprite24, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y));
AL_FUNC(void, _linear_draw_lit_rle_sprite24, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y, int color));
AL_FUNC(void, _linear_draw_character24, (BITMAP *bmp, BITMAP *sprite, int x, int y, int color, int bg));
AL_FUNC(void, _linear_blit24, (BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
AL_FUNC(void, _linear_blit_backward24, (BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
AL_FUNC(void, _linear_masked_blit24, (BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
AL_FUNC(void, _linear_clear_to_color24, (BITMAP *bitmap, int color));

#endif

#ifdef ALLEGRO_COLOR32

AL_FUNC(int,  _linear_getpixel32, (BITMAP *bmp, int x, int y));
AL_FUNC(void, _linear_putpixel32, (BITMAP *bmp, int x, int y, int color));
AL_FUNC(void, _linear_vline32, (BITMAP *bmp, int x, int y_1, int y2, int color));
AL_FUNC(void, _linear_hline32, (BITMAP *bmp, int x1, int y, int x2, int color));
AL_FUNC(void, _linear_draw_sprite32, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_sprite_ex32, (BITMAP *bmp, BITMAP *sprite, int x, int y, int mode, int flip));
AL_FUNC(void, _linear_draw_256_sprite32, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_sprite_v_flip32, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_sprite_h_flip32, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_sprite_vh_flip32, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_trans_sprite32, (BITMAP *bmp, BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_lit_sprite32, (BITMAP *bmp, BITMAP *sprite, int x, int y, int color));
AL_FUNC(void, _linear_draw_rle_sprite32, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y));
AL_FUNC(void, _linear_draw_trans_rle_sprite32, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y));
AL_FUNC(void, _linear_draw_lit_rle_sprite32, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y, int color));
AL_FUNC(void, _linear_draw_character32, (BITMAP *bmp, BITMAP *sprite, int x, int y, int color, int bg));
AL_FUNC(void, _linear_blit32, (BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
AL_FUNC(void, _linear_blit_backward32, (BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
AL_FUNC(void, _linear_masked_blit32, (BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
AL_FUNC(void, _linear_clear_to_color32, (BITMAP *bitmap, int color));

#endif



/* color conversion routines */
typedef struct GRAPHICS_RECT {
   int width;
   int height;
   int pitch;
   void *data;
} GRAPHICS_RECT;

typedef void (COLORCONV_BLITTER_FUNC)(GRAPHICS_RECT *src_rect, GRAPHICS_RECT *dest_rect);

AL_FUNC(COLORCONV_BLITTER_FUNC *, _get_colorconv_blitter, (int from_depth, int to_depth));
AL_FUNC(void, _release_colorconv_blitter, (COLORCONV_BLITTER_FUNC *blitter));
AL_FUNC(void, _set_colorconv_palette, (AL_CONST struct RGB *p, int from, int to));
AL_FUNC(unsigned char *, _get_colorconv_map, (void));

#ifdef ALLEGRO_COLOR8

AL_FUNC(void, _colorconv_blit_8_to_8, (GRAPHICS_RECT *src_rect, GRAPHICS_RECT *dest_rect));
AL_FUNC(void, _colorconv_blit_8_to_15, (GRAPHICS_RECT *src_rect, GRAPHICS_RECT *dest_rect));
AL_FUNC(void, _colorconv_blit_8_to_16, (GRAPHICS_RECT *src_rect, GRAPHICS_RECT *dest_rect));
AL_FUNC(void, _colorconv_blit_8_to_24, (GRAPHICS_RECT *src_rect, GRAPHICS_RECT *dest_rect));
AL_FUNC(void, _colorconv_blit_8_to_32, (GRAPHICS_RECT *src_rect, GRAPHICS_RECT *dest_rect));

#endif

#ifdef ALLEGRO_COLOR16

AL_FUNC(void, _colorconv_blit_15_to_8, (GRAPHICS_RECT *src_rect, GRAPHICS_RECT *dest_rect));
AL_FUNC(void, _colorconv_blit_15_to_16, (GRAPHICS_RECT *src_rect, GRAPHICS_RECT *dest_rect));
AL_FUNC(void, _colorconv_blit_15_to_24, (GRAPHICS_RECT *src_rect, GRAPHICS_RECT *dest_rect));
AL_FUNC(void, _colorconv_blit_15_to_32, (GRAPHICS_RECT *src_rect, GRAPHICS_RECT *dest_rect));

AL_FUNC(void, _colorconv_blit_16_to_8, (GRAPHICS_RECT *src_rect, GRAPHICS_RECT *dest_rect));
AL_FUNC(void, _colorconv_blit_16_to_15, (GRAPHICS_RECT *src_rect, GRAPHICS_RECT *dest_rect));
AL_FUNC(void, _colorconv_blit_16_to_24, (GRAPHICS_RECT *src_rect, GRAPHICS_RECT *dest_rect));
AL_FUNC(void, _colorconv_blit_16_to_32, (GRAPHICS_RECT *src_rect, GRAPHICS_RECT *dest_rect));

#endif

#ifdef ALLEGRO_COLOR24

AL_FUNC(void, _colorconv_blit_24_to_8, (GRAPHICS_RECT *src_rect, GRAPHICS_RECT *dest_rect));
AL_FUNC(void, _colorconv_blit_24_to_15, (GRAPHICS_RECT *src_rect, GRAPHICS_RECT *dest_rect));
AL_FUNC(void, _colorconv_blit_24_to_16, (GRAPHICS_RECT *src_rect, GRAPHICS_RECT *dest_rect));
AL_FUNC(void, _colorconv_blit_24_to_32, (GRAPHICS_RECT *src_rect, GRAPHICS_RECT *dest_rect));

#endif

#ifdef ALLEGRO_COLOR32

AL_FUNC(void, _colorconv_blit_32_to_8, (GRAPHICS_RECT *src_rect, GRAPHICS_RECT *dest_rect));
AL_FUNC(void, _colorconv_blit_32_to_15, (GRAPHICS_RECT *src_rect, GRAPHICS_RECT *dest_rect));
AL_FUNC(void, _colorconv_blit_32_to_16, (GRAPHICS_RECT *src_rect, GRAPHICS_RECT *dest_rect));
AL_FUNC(void, _colorconv_blit_32_to_24, (GRAPHICS_RECT *src_rect, GRAPHICS_RECT *dest_rect));

#endif



/* generic color conversion blitter */
AL_FUNC(void, _blit_between_formats, (BITMAP *src, BITMAP *dest, int s_x, int s_y, int d_x, int d_y, int w, int h));


/* asm helper for stretch_blit() */
#ifndef SCAN_EXPORT
AL_FUNC(void, _do_stretch, (BITMAP *source, BITMAP *dest, void *drawer, int sx, fixed sy, fixed syd, int dx, int dy, int dh, int color_depth));
#endif


/* lower level functions for rotation */
AL_FUNC(void, _parallelogram_map, (BITMAP *bmp, BITMAP *spr, fixed xs[4], fixed ys[4], void (*draw_scanline)(BITMAP *bmp, BITMAP *spr, fixed l_bmp_x, int bmp_y, fixed r_bmp_x, fixed l_spr_x, fixed l_spr_y, fixed spr_dx, fixed spr_dy), int sub_pixel_accuracy));
AL_FUNC(void, _parallelogram_map_standard, (BITMAP *bmp, BITMAP *sprite, fixed xs[4], fixed ys[4]));
AL_FUNC(void, _rotate_scale_flip_coordinates, (fixed w, fixed h, fixed x, fixed y, fixed cx, fixed cy, fixed angle, fixed scale_x, fixed scale_y, int h_flip, int v_flip, fixed xs[4], fixed ys[4]));
AL_FUNC(void, _pivot_scaled_sprite_flip, (struct BITMAP *bmp, struct BITMAP *sprite, fixed x, fixed y, fixed cx, fixed cy, fixed angle, fixed scale, int v_flip));


/* number of fractional bits used by the polygon rasteriser */
#define POLYGON_FIX_SHIFT     18




/* information for polygon scanline fillers */
typedef struct POLYGON_SEGMENT
{
   fixed u, v, du, dv;              /* fixed point u/v coordinates */
   fixed c, dc;                     /* single color gouraud shade values */
   fixed r, g, b, dr, dg, db;       /* RGB gouraud shade values */
   float z, dz;                     /* polygon depth (1/z) */
   float fu, fv, dfu, dfv;          /* floating point u/v coordinates */
   unsigned char *texture;          /* the texture map */
   int umask, vmask, vshift;        /* texture map size information */
   int seg;                         /* destination bitmap selector */
   uintptr_t zbuf_addr;		    /* Z-buffer address */
   uintptr_t read_addr;		    /* reading address for transparency modes */
} POLYGON_SEGMENT;


/* an active polygon edge */
typedef struct POLYGON_EDGE 
{
   int top;                         /* top y position */
   int bottom;                      /* bottom y position */
   fixed x, dx;                     /* fixed point x position and gradient */
   fixed w;                         /* width of line segment */
   POLYGON_SEGMENT dat;             /* texture/gouraud information */
   struct POLYGON_EDGE *prev;       /* doubly linked list */
   struct POLYGON_EDGE *next;
   struct POLYGON_INFO *poly;	    /* father polygon */
} POLYGON_EDGE;


/* various libc stuff */
AL_FUNC(void *, _al_sane_realloc, (void *ptr, size_t size));
AL_FUNC(char *, _al_sane_strncpy, (char *dest, const char *src, size_t n));


#define _AL_RAND_MAX  0xFFFF
AL_FUNC(void, _al_srand, (int seed));
AL_FUNC(int, _al_rand, (void));


#ifdef __cplusplus
   }
#endif

#endif          /* ifndef AINTERN_H */
