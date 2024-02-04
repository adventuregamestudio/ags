//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================

#include "core/platform.h"

#if AGS_PLATFORM_IS_XDG_UNIX
#include "platform/base/agsplatform_xdg_unix.h"

#include <cstdio>
#include <unistd.h>
#include <allegro.h>
#include "gfx/gfxdefines.h"
#include "util/string.h"
#include <pwd.h>
#include <sys/stat.h>

using AGS::Common::String;


FSLocation CommonDataDirectory;
FSLocation UserDataDirectory;


static FSLocation BuildXDGPath()
{
    // Check to see if XDG_DATA_HOME is set in the enviroment
    const char* home_dir = getenv("XDG_DATA_HOME");
    if (home_dir)
        return FSLocation(home_dir);
    // No evironment variable, so we fall back to home dir in /etc/passwd
    struct passwd *p = getpwuid(getuid());
    if (p)
        return FSLocation(p->pw_dir).Concat(".local/share");
    return FSLocation();
}

static void DetermineDataDirectories()
{
    if (UserDataDirectory.IsValid())
        return;
    FSLocation fsloc = BuildXDGPath();
    if (!fsloc.IsValid())
        fsloc = FSLocation("/tmp");
    UserDataDirectory = fsloc.Concat("ags");
    CommonDataDirectory = fsloc.Concat("ags-common");
}

FSLocation AGSPlatformXDGUnix::GetAllUsersDataDirectory()
{
    DetermineDataDirectories();
    return CommonDataDirectory;
}

FSLocation AGSPlatformXDGUnix::GetUserSavedgamesDirectory()
{
    DetermineDataDirectories();
    return UserDataDirectory;
}

FSLocation AGSPlatformXDGUnix::GetUserConfigDirectory()
{
    return GetUserSavedgamesDirectory();
}

FSLocation AGSPlatformXDGUnix::GetUserGlobalConfigDirectory()
{
    return GetUserSavedgamesDirectory();
}

FSLocation AGSPlatformXDGUnix::GetAppOutputDirectory()
{
    DetermineDataDirectories();
    return UserDataDirectory;
}

uint64_t AGSPlatformXDGUnix::GetDiskFreeSpaceMB() {
    // placeholder
    return 100;
}

const char* AGSPlatformXDGUnix::GetBackendFailUserHint()
{
    return "Make sure you have latest version of SDL2 libraries installed, and X server is running.";
}

#endif