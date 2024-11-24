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
// Custom property structs
//
//-----------------------------------------------------------------------------
//
// Custom property schema is kept by GameSetupStruct object as a single
// instance and defines property type and default value. Every game entity that
// has properties implemented keeps CustomProperties object, which stores
// actual property values only if ones are different from defaults.
//
//=============================================================================
#ifndef __AGS_CN_GAME__CUSTOMPROPERTIES_H
#define __AGS_CN_GAME__CUSTOMPROPERTIES_H

#include <unordered_map>
#include "util/string.h"
#include "util/string_types.h"

namespace AGS
{
namespace Common
{

enum PropertyVersion
{
    kPropertyVersion_Initial = 1, // [DEPRECATED]
    kPropertyVersion_340 = 2,
    kPropertyVersion_Current = kPropertyVersion_340
};

enum PropertyType
{
    kPropertyUndefined = 0,
    kPropertyBoolean,
    kPropertyInteger,
    kPropertyString
};

enum PropertyError
{
    kPropertyErr_NoError,
    kPropertyErr_UnsupportedFormat
};

//
// PropertyDesc - a description of a single custom property
//
struct PropertyDesc
{
    String       Name;
    PropertyType Type;
    String       Description;
    String       DefaultValue;

    PropertyDesc();
    PropertyDesc(const String &name, PropertyType type, const String &desc, const String &def_value);
};

// NOTE: AGS has case-insensitive property IDs
// Schema - a map of property descriptions
typedef std::unordered_map<String, PropertyDesc, HashStrNoCase, StrEqNoCase> PropertySchema;


namespace Properties
{
    PropertyError ReadSchema(PropertySchema &schema, Stream *in);
    void          WriteSchema(const PropertySchema &schema, Stream *out);

    // Reads property values from the stream and assign them to map.
    // The non-matching existing map items, if any, are NOT erased.
    PropertyError ReadValues(StringIMap &map, Stream *in);
    // Writes property values chunk to the stream
    void          WriteValues(const StringIMap &map, Stream *out);

} // namespace Properties

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_GAME__CUSTOMPROPERTIES_H
