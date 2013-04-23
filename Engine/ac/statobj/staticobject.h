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
//
// A stub class for "managing" static global objects exported to script.
// This may be temporary solution (oh no, not again :P) that could be
// replaced by the use of dynamic objects in the future.
//
//=============================================================================
#ifndef __AGS_EE_STATOBJ__STATICOBJECT_H
#define __AGS_EE_STATOBJ__STATICOBJECT_H

#include "core/types.h"

struct ICCStaticObject {
    virtual ~ICCStaticObject(){}

    // Legacy support for reading and writing object values by their relative offset
    virtual uint8_t GetPropertyUInt8(const char *address, intptr_t offset) = 0;
    virtual int16_t GetPropertyInt16(const char *address, intptr_t offset) = 0;
    virtual int32_t GetPropertyInt32(const char *address, intptr_t offset) = 0;
    virtual void    SetPropertyUInt8(const char *address, intptr_t offset, uint8_t value) = 0;
    virtual void    SetPropertyInt16(const char *address, intptr_t offset, int16_t value) = 0;
    virtual void    SetPropertyInt32(const char *address, intptr_t offset, int32_t value) = 0;
};

#endif // __AGS_EE_STATOBJ__STATICOBJECT_H
