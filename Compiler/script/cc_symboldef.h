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
#ifndef __CC_SYMBOLDEF_H
#define __CC_SYMBOLDEF_H

#include "cs_parser_common.h"   // macro definitions

#define STYPE_DYNARRAY  (0x10000000)
#define STYPE_CONST     (0x20000000)
#define STYPE_POINTER   (0x40000000)

#define STYPE_MASK       (0xFFFFFFF)

#define SYM_TEMPORARYTYPE -99


// Contains function parameter description
struct FuncParamInfo
{
    uint32_t Type = 0u;
    int32_t DefaultValue = 0;
    bool HasDefaultValue = false;
    std::string Name;
};

struct SymbolDef {
    int16_t  stype;
    int32_t  flags;
    int32_t  ssize;  // or return type size for function
    int16_t  sscope;  // or num arguments for function
    int32_t  arrsize;
    // return value is at index 0, actual args begin with 1
    FuncParamInfo funcparams[MAX_FUNCTION_PARAMETERS + 1];
};

#endif // __CC_SYMBOLDEF_H
