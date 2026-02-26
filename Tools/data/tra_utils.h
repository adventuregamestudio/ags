//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
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

#include <memory>
#include "data/tra_file.h"
#include "util/stream.h"

namespace AGS
{
namespace DataUtil
{

using AGS::Common::HError;
using AGS::Common::Stream;
using AGS::Common::String;
using AGS::Common::Translation;

// Parses a TRS format and fills Translation data
HError ReadTRS(Translation &tra, std::unique_ptr<Stream> &&in);
HError WriteTRA(const Translation &tra, std::unique_ptr<Stream> &&out);

} // namespace DataUtil
} // namespace AGS

#endif // __AGS_TOOL_DATA__CRMUTIL_H
