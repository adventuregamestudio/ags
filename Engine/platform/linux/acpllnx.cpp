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

#include "platform/base/agsplatformdriver.h"
#include "platform/base/agsplatform_xdg_unix.h"

struct AGSLinux : AGSPlatformXDGUnix {
  eScriptSystemOSID GetSystemOSID() override;
};

eScriptSystemOSID AGSLinux::GetSystemOSID() {
  return eOS_Linux;
}

AGSPlatformDriver* AGSPlatformDriver::CreateDriver()
{
    return new AGSLinux();
}

#endif
