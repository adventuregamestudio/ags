//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================

#include "core/platform.h"

#if AGS_PLATFORM_OS_LINUX

// *************** LINUX DRIVER ***************
#include <unordered_map>
#include <SDL.h>
#include "platform/base/agsplatformdriver.h"
#include "platform/base/agsplatform_xdg_unix.h"
#include "libsrc/libcda-0.5/libcda.h"

struct AGSLinux : AGSPlatformXDGUnix {
  int  CDPlayerCommand(int cmdd, int datt) override;
  eScriptSystemOSID GetSystemOSID() override;
  int  InitializeCDPlayer() override;
  void ShutdownCDPlayer() override;
  SDL_Surface *CreateWindowIcon() override;
};


int AGSLinux::CDPlayerCommand(int cmdd, int datt) {
  return cd_player_control(cmdd, datt);
}

eScriptSystemOSID AGSLinux::GetSystemOSID() {
  return eOS_Linux;
}

int AGSLinux::InitializeCDPlayer() {
  return cd_player_init();
}

void AGSLinux::ShutdownCDPlayer() {
  cd_exit();
}

struct XpmInfo
{
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t palsize = 0;
    uint32_t chpp = 0; // characters per pixel
    static const int shift_r = 16;
    static const int shift_g = 8;
    static const int shift_b = 0;
    static const int shift_a = 24;

    std::unordered_map<uint32_t, uint32_t> pal;
    const char **pixels = nullptr;
} xpminfo;

static bool OpenXpm(const char *xpm[], XpmInfo &xpminfo)
{
    sscanf(xpm[0], "%u %u %u %u", &xpminfo.width, &xpminfo.height, &xpminfo.palsize, &xpminfo.chpp);
    if (xpminfo.width == 0 || xpminfo.height == 0 || xpminfo.palsize == 0 || xpminfo.chpp == 0)
        return false;
    xpminfo.pal.reserve(xpminfo.palsize);
    for (uint32_t col = 1; col < xpminfo.palsize + 1; ++col)
    {
        uint32_t colid = 0;
        for (uint32_t cc = 0; cc < xpminfo.chpp; ++cc)
            colid |= xpm[col][cc] << (cc * 8);
        uint32_t colval = 0;
        const char *p_value = xpm[col] + xpminfo.chpp + 3;
        if (*p_value == '#')
        {
            sscanf(p_value + 1, "%x", &colval);
            colval |= 0xFF << XpmInfo::shift_a;
        }
        xpminfo.pal.insert(std::make_pair(colid, colval));
    }
    xpminfo.pixels = &xpm[xpminfo.palsize + 1];
    return true;
}

static void XpmToArgb(const XpmInfo &xpminfo, uint8_t *dst_buf, size_t dst_pitch)
{
    const uint32_t w = xpminfo.width, h = xpminfo.height, chpp = xpminfo.chpp;
    const char **pixels = xpminfo.pixels;
    for (uint32_t y = 0; y < h; ++y)
    {
        const char *src = pixels[y];
        const char *src_end = src + chpp * w;
        uint32_t *dst = (uint32_t*)(dst_buf + (dst_pitch * y));
        for (; src < src_end;)
        {
            uint32_t colid = 0;
            for (size_t cc = 0; cc < chpp; ++cc)
                colid |= (*src++) << (cc * 8);
            uint32_t colval = xpminfo.pal.at(colid);
            (*dst++) =
                (((colval >> XpmInfo::shift_r) & 0xFF) << 16) |
                (((colval >> XpmInfo::shift_g) & 0xFF) << 8) |
                (((colval >> XpmInfo::shift_b) & 0xFF) << 0) |
                (((colval >> XpmInfo::shift_a) & 0xFF) << 24);
        }
    }
}

#include "platform/linux/icon.xpm"

SDL_Surface *AGSLinux::CreateWindowIcon() {
    XpmInfo xpminfo;
    if (!OpenXpm(icon_xpm, xpminfo)) return nullptr;
    SDL_Surface *sdl_icon = SDL_CreateRGBSurfaceWithFormat(
        0, xpminfo.width, xpminfo.height, 32, SDL_PIXELFORMAT_ARGB8888);
    XpmToArgb(xpminfo, (uint8_t*)sdl_icon->pixels, sdl_icon->pitch);
    return sdl_icon;
}

AGSPlatformDriver* AGSPlatformDriver::CreateDriver()
{
    return new AGSLinux();
}

#endif
