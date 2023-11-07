//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2023 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================

#include "core/platform.h"

#if AGS_PLATFORM_OS_FREEBSD

// *************** FREEBSD DRIVER ***************

#include "platform/base/agsplatformdriver.h"
#include "platform/base/agsplatform_xdg_unix.h"

struct AGSFreeBSD : AGSPlatformXDGUnix {
    eScriptSystemOSID GetSystemOSID() override;
    int  CDPlayerCommand(int cmdd, int datt) override;
    int  InitializeCDPlayer() override;
    void ShutdownCDPlayer() override;
};

int AGSFreeBSD::CDPlayerCommand(int cmdd, int datt) {
    return 0;
}

int AGSFreeBSD::InitializeCDPlayer() {
    return 0;
}

void AGSFreeBSD::ShutdownCDPlayer() { }

eScriptSystemOSID AGSFreeBSD::GetSystemOSID() {
  return eOS_FreeBSD;
}

AGSPlatformDriver* AGSPlatformDriver::CreateDriver() {
    return new AGSFreeBSD();
}

#endif // AGS_PLATFORM_OS_FREEBSD
