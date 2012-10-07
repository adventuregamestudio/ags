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
// Internal definitions for ex_* units
//
//=============================================================================
#ifndef __AGS_EE_SCRIPT__EX_DEFINES_H
#define __AGS_EE_SCRIPT__EX_DEFINES_H

// defined in cs_runtime
extern void ccAddExternalSymbol(char *namof, void *addrof);

#define scAdd_External_Symbol ccAddExternalSymbol

#endif // __AGS_EE_SCRIPT__EX_DEFINES_H
