set(ALLEGRO_SRC_FILES
        src/allegro.c
        src/blit.c
        src/colblend.c
        src/color.c
        src/dither.c
        src/file.c
        src/fli.c
        src/flood.c
        src/gfx.c
        src/graphics.c
        src/inline.c
        src/libc.c
        src/math.c
        src/polygon.c
        src/quantize.c
        src/readbmp.c
        src/rotate.c
        src/unicode.c
        src/vtable15.c
        src/vtable16.c
        src/vtable24.c
        src/vtable32.c
        src/vtable8.c
        src/vtable.c
        )

set(ALLEGRO_SRC_C_FILES
        src/c/cblit8.c
        src/c/cblit16.c
        src/c/cblit24.c
        src/c/cblit32.c
        src/c/cgfx8.c
        src/c/cgfx15.c
        src/c/cgfx16.c
        src/c/cgfx24.c
        src/c/cgfx32.c
        src/c/cmisc.c
        src/c/cspr8.c
        src/c/cspr15.c
        src/c/cspr16.c
        src/c/cspr24.c
        src/c/cspr32.c
        src/c/cstretch.c
        )

set(ALLEGRO_SRC_UNIX_FILES
        src/unix/ufile.c
        )

set(ALLEGRO_SRC_WIN_FILES
        src/win/gdi.c
        src/win/wfile.c
        )

set(ALLEGRO_INCLUDE_ALLEGRO_FILES
        include/allegro/base.h
        include/allegro/color.h
        include/allegro/debug.h
        include/allegro/draw.h
        include/allegro/file.h
        include/allegro/fixed.h
        include/allegro/fli.h
        include/allegro/fmaths.h
        include/allegro/gfx.h
        include/allegro/palette.h
        include/allegro/system.h
        include/allegro/unicode.h
        )

set(ALLEGRO_INCLUDE_ALLEGRO_INLINE_FILES
        include/allegro/inline/color.inl
        include/allegro/inline/draw.inl
        include/allegro/inline/fix.inl
        include/allegro/inline/fmaths.inl
        include/allegro/inline/gfx.inl
        )

set(ALLEGRO_INCLUDE_ALLEGRO_INTERNAL_FILES
        include/allegro/internal/aintern.h
        include/allegro/internal/alconfig.h
        )

set(ALLEGRO_INCLUDE_ALLEGRO_PLATFORM_FILES
       include/allegro/platform/astdint.h
       include/allegro/platform/alucfg.h
       )

#-----------------------------------------------------------------------------#
# vim: set sts=4 sw=4 et:
