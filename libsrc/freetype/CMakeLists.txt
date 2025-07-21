add_library(freetype)

set_target_properties(freetype PROPERTIES
        C_STANDARD 11
        C_EXTENSIONS NO
)

target_compile_definitions(freetype PRIVATE FT2_BUILD_LIBRARY)
target_include_directories(freetype PUBLIC include )

target_sources(freetype
        PRIVATE
        src/base/ftbase.c
        src/base/ftglyph.c
        src/base/ftinit.c
        src/base/ftmm.c

        src/base/ftfstype.c
        src/base/ftgasp.c
        src/base/ftgxval.c
        src/base/ftotval.c
        src/base/ftpatent.c
        src/base/ftstroke.c
        src/base/ftsynth.c
        src/base/ftwinfnt.c
        src/base/ftcid.c
        src/base/ftbitmap.c

        src/autofit/autofit.c
        src/bdf/bdf.c
        src/cache/ftcache.c
        src/cff/cff.c
        src/cid/type1cid.c
        src/lzw/ftlzw.c
        src/gzip/ftgzip.c
        src/pcf/pcf.c
        src/pfr/pfr.c
        src/psaux/psaux.c
        src/pshinter/pshinter.c
        src/psnames/psmodule.c
        src/raster/raster.c
        src/sfnt/sfnt.c
        src/sdf/sdf.c
        src/smooth/smooth.c
        src/svg/svg.c
        src/truetype/truetype.c
        src/type1/type1.c
        src/type42/type42.c
        src/cid/type1cid.c
        src/winfonts/winfnt.c

        # the following files may be unix only:
        src/base/ftbbox.c
        src/base/ftbdf.c
        src/base/ftpfr.c
        src/base/fttype1.c
)

if (WIN32)
  target_sources(freetype PRIVATE
          src/base/ftdebug.c
          builds/windows/ftdebug.c
          builds/windows/ftsystem.c
  )
else()
  # Currently assume all non-windows platforms are unix-y
  target_sources(freetype PRIVATE
          builds/unix/ftsystem.c
          src/base/ftdebug.c
  )
  target_compile_definitions(freetype PRIVATE HAVE_UNISTD_H=1)
  target_compile_definitions(freetype PRIVATE HAVE_FCNTL_H=1)
endif()

#target_link_libraries(freetype PUBLIC ZLIB::ZLIB BZip2::BZip2)

add_library(FreeType::FreeType ALIAS freetype)