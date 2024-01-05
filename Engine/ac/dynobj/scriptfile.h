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
//
//
//=============================================================================
#ifndef __AGS_EE_DYNOBJ__SCRIPTFILE_H
#define __AGS_EE_DYNOBJ__SCRIPTFILE_H

#include "ac/dynobj/cc_agsdynamicobject.h"
#include "util/file.h"
#include "util/stream.h"

using namespace AGS; // FIXME later

#define scFileRead   1
#define scFileWrite  2
#define scFileAppend 3

struct sc_File final : CCBasicObject {
    int32_t             handle;

    static const Common::FileOpenMode fopenModes[];
    static const Common::StreamMode fworkModes[];

    int Dispose(void *address, bool force) override;

    const char *GetType() override;

    int Serialize(void *address, uint8_t *buffer, int bufsize) override;

    int OpenFile(const char *filename, int mode);
    void Close();

    sc_File();
};

#endif // __AGS_EE_DYNOBJ__SCRIPTFILE_H
