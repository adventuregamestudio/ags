//=============================================================================
//
// A mini-extension to miniz library. Contains expanded function for writing
// PNG file with palette data, which is not included into miniz itself (yet).
// This code is redistributed in accordance with SDL3 license:
// https://github.com/libsdl-org/SDL/blob/release-3.4.x/LICENSE.txt
//
// TODO: remove this unit when (if) an equivalent function is added to the
// miniz library itself.
// 
//=============================================================================
#pragma once
#include <miniz.h>

#ifdef __cplusplus
extern "C" {
#endif

/* An extended variant of writing a PNG file in memory. */
/* Ported from a modified miniz.h/c of SDL3 library */
/* See: https://github.com/libsdl-org/SDL/blob/release-3.4.x/src/video/miniz.h */
/* where it was replacing original "tdefl_write_image_to_png_file_in_memory_ex" */
MINIZ_EXPORT void *tdefl_write_image_to_png_file_in_memory_ex2(const void *pImage, int w, int h, int num_chans, int bpl, size_t *pLen_out, mz_uint level, mz_bool flip, mz_uint8 *plte, int plte_size, mz_uint8 *trns, int trns_size);

#ifdef __cplusplus
}
#endif
