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

#include "game/customproperties.h"
#include "util/stream.h"
#include "util/string_utils.h"

namespace AGS
{
namespace Common
{

PropertyDesc::PropertyDesc()
{
    Type = kPropertyBoolean;
}

PropertyDesc::PropertyDesc(const String &name, PropertyType type, const String &desc, const String &def_value)
{
    Name = name;
    Type = type;
    Description = desc;
    DefaultValue = def_value;
}


namespace Properties
{

PropertyError ReadSchema(PropertySchema &schema, Stream *in)
{
    PropertyVersion version = (PropertyVersion)in->ReadInt32();
    if (version < kPropertyVersion_340 || version > kPropertyVersion_Current)
    {
        return kPropertyErr_UnsupportedFormat;
    }

    PropertyDesc prop;
    int count = in->ReadInt32();
    for (int i = 0; i < count; ++i)
    {
        prop.Name = StrUtil::ReadString(in);
        prop.Type = (PropertyType)in->ReadInt32();
        prop.Description = StrUtil::ReadString(in);
        prop.DefaultValue = StrUtil::ReadString(in);
        schema[prop.Name] = prop;
    }
    return kPropertyErr_NoError;
}

void WriteSchema(const PropertySchema &schema, Stream *out)
{
    out->WriteInt32(kPropertyVersion_Current);
    out->WriteInt32(static_cast<uint32_t>(schema.size()));
    for (PropertySchema::const_iterator it = schema.begin();
         it != schema.end(); ++it)
    {
        const PropertyDesc &prop = it->second;
        StrUtil::WriteString(prop.Name, out);
        out->WriteInt32(prop.Type);
        StrUtil::WriteString(prop.Description, out);
        StrUtil::WriteString(prop.DefaultValue, out);
    }
}

PropertyError ReadValues(StringIMap &map, Stream *in)
{
    PropertyVersion version = (PropertyVersion)in->ReadInt32();
    if (version < kPropertyVersion_Initial || version > kPropertyVersion_Current)
    {
        return kPropertyErr_UnsupportedFormat;
    }

    int count = in->ReadInt32();
    // NOTE: handle Editor's mistake where it could save empty property bag with version 1
    if ((version == kPropertyVersion_Initial) && count > 0)
        return kPropertyErr_UnsupportedFormat;
    for (int i = 0; i < count; ++i)
    {
        String name  = StrUtil::ReadString(in);
        map[name] = StrUtil::ReadString(in);
    }
    return kPropertyErr_NoError;
}

PropertyError SkipValues(Stream *in)
{
    PropertyVersion version = (PropertyVersion)in->ReadInt32();
    if (version < kPropertyVersion_Initial || version > kPropertyVersion_Current)
    {
        return kPropertyErr_UnsupportedFormat;
    }

    int count = in->ReadInt32();
    // NOTE: handle Editor's mistake where it could save empty property bag with version 1
    if ((version == kPropertyVersion_Initial) && count > 0)
        return kPropertyErr_UnsupportedFormat;
    for (int i = 0; i < count; ++i)
    {
        StrUtil::SkipString(in); // name
        StrUtil::SkipString(in); // value
    }
    return kPropertyErr_NoError;
}

void WriteValues(const StringIMap &map, Stream *out)
{
    out->WriteInt32(kPropertyVersion_Current);
    out->WriteInt32(static_cast<uint32_t>(map.size()));
    for (StringIMap::const_iterator it = map.begin();
         it != map.end(); ++it)
    {
        StrUtil::WriteString(it->first, out);
        StrUtil::WriteString(it->second, out);
    }
}

} // namespace Properties

} // namespace Common
} // namespace AGS
