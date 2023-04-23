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
#ifndef __AC_CCDYNAMICOBJECT_H
#define __AC_CCDYNAMICOBJECT_H

#include "ac/dynobj/cc_basicobject.h"

namespace AGS { namespace Common { class Stream; } }


struct AGSCCDynamicObject : CCBasicObject {
protected:
    virtual ~AGSCCDynamicObject() = default;

public:
    // TODO: how to pass savegame format version (???)
    int Serialize(const char *address, char *buffer, int bufsize) override;
    // Try unserializing the object from the given input stream
    virtual void Unserialize(int index, AGS::Common::Stream *in, size_t data_sz) = 0;

protected:
    // Savegame serialization
    // Calculate and return required space for serialization, in bytes
    virtual size_t CalcSerializeSize() = 0;
    // Write object data into the provided stream
    virtual void Serialize(const char *address, AGS::Common::Stream *out) = 0;
};

#endif // __AC_CCDYNAMICOBJECT_H