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
#ifndef __AC_SCRIPTCONTAINERS_H
#define __AC_SCRIPTCONTAINERS_H

class ScriptDictBase;
class ScriptSetBase;

// Create and register new dictionary
ScriptDictBase *Dict_Create(bool sorted, bool case_sensitive);
// Unserialize dictionary from the memory stream
ScriptDictBase *Dict_Unserialize(int index, AGS::Common::Stream *in, size_t data_sz);
// Create and register new set
ScriptSetBase *Set_Create(bool sorted, bool case_sensitive);
// Unserialize set from the memory stream
ScriptSetBase *Set_Unserialize(int index, AGS::Common::Stream *in, size_t data_sz);

#endif // __AC_SCRIPTCONTAINERS_H
