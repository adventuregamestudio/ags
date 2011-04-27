/*
 * aastr.h --- anti-aliased stretching and rotation for Allegro
 *
 * This file is gift-ware.  This file is given to you freely
 * as a gift.  You may use, modify, redistribute, and generally hack
 * it about in any way you like, and you do not have to give anyone
 * anything in return.
 *
 * I do not accept any responsibility for any effects, adverse or
 * otherwise, that this code may have on just about anything that
 * you can think of.  Use it at your own risk.
 *
 * Copyright (C) 1998, 1999  Michael Bukin
 */

#ifndef __bma_aastr_h
#define __bma_aastr_h

#include <allegro.h>

#ifdef __cplusplus
extern "C" {
#endif

  /* Stretching.  */
  void aa_stretch_blit (BITMAP* src, BITMAP* dst,
			int sx, int sy, int sw, int sh,
			int dx, int dy, int dw, int dh);
  void aa_stretch_sprite (BITMAP* dst, BITMAP* src,
			  int dx, int dy, int dw, int dh);

  /* Rotation.  */
  void aa_rotate_scaled_bitmap (BITMAP* src, BITMAP* dst,
				int x, int y, fixed angle,
				fixed scalex, fixed scaley);
  void aa_rotate_scaled_sprite (BITMAP* dst, BITMAP* src,
				int x, int y, fixed angle,
				fixed scalex, fixed scaley);
  void aa_rotate_bitmap (BITMAP* src, BITMAP* dst,
			 int x, int y, fixed angle);
  void aa_rotate_sprite (BITMAP* dst, BITMAP* src,
			 int x, int y, fixed angle);

#ifdef __cplusplus
}
#endif

#endif /* !__bma_aastr_h */

/*
 * aastr.h ends here
 */
