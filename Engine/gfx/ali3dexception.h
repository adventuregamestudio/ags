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
//
// Graphics driver exception class
//
//=============================================================================
#ifndef __AGS_EE_GFX__ALI3DEXCEPTION_H
#define __AGS_EE_GFX__ALI3DEXCEPTION_H

#include "util/string.h"

namespace AGS
{
namespace Engine
{

class Ali3DException
{
public:
    Ali3DException(const AGS::Common::String &message)
        : Message(message)
    {
    }

    const AGS::Common::String Message;
};

class Ali3DFullscreenLostException : public Ali3DException
{
public:
    Ali3DFullscreenLostException() : Ali3DException("Direct3D device is lost")
    {
    }
};

} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_GFX__ALI3DEXCEPTION_H
