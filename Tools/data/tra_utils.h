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
#ifndef __AGS_TOOL_DATA__CRMUTIL_H
#define __AGS_TOOL_DATA__CRMUTIL_H

#include "game/tra_file.h"

namespace AGS
{
namespace DataUtil
{

using AGS::Common::HError;
using AGS::Common::Stream;
using AGS::Common::String;
using AGS::Common::Translation;

// Parses a TRS format and fills Translation data
HError ReadTRS(Translation &tra, Stream *in);
HError WriteTRA(const Translation &tra, Stream *out);

} // namespace DataUtil
} // namespace AGS

#endif // __AGS_TOOL_DATA__CRMUTIL_H
