//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#ifndef __AC_SERIALIZER_H
#define __AC_SERIALIZER_H

#include "ac/dynobj/cc_scriptobject.h"
#include "util/stream.h"

struct AGSDeSerializer : ICCObjectCollectionReader
{
public:
    void Unserialize(int index, const char *objectType, const char *serializedData, int dataSize) override;

private:
    // Unserializes an old GUIControl record
    void UnserializeGUIControl(int index, AGS::Common::Stream *in, size_t data_sz);
    int  RegisterGUIControl(int index, int guinum, int objnum);
};

#endif // __AC_SERIALIZER_H
