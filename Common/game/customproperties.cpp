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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game/customproperties.h"
#include "util/string_utils.h"      // out->WriteString, etc
#include "util/stream.h"
#include "util/string.h"

#define LEGACY_MAX_CUSTOM_PROPERTY_SCHEMA_NAME_LENGTH 20
#define LEGACY_MAX_CUSTOM_PROPERTY_NAME_LENGTH        200
#define LEGACY_MAX_CUSTOM_PROPERTY_DESCRIPTION_LENGTH 100
#define LEGACY_MAX_CUSTOM_PROPERTY_VALUE_LENGTH       500

namespace AGS
{
namespace Common
{

void CustomPropertySchema::AddProperty(const String &name, CustomPropertyType type,
                                const String &description, const String &def_value)
{
    Properties.Append(CustomPropertyInfo(name, type, description, def_value));
}

CustomPropertyInfo *CustomPropertySchema::FindProperty(const String &name)
{
    for (int i = 0; i < Properties.GetCount(); ++i)
    {
        if (Properties[i].Name.CompareNoCase(name) == 0)
        {
            return &Properties[i];
        }
    }
    return NULL;
}

void CustomPropertySchema::Free()
{
    Properties.Free();
}

CustomPropertyInfo *CustomPropertySchema::GetProperty(int index)
{
    if (index >= 0 && index < Properties.GetCount())
    {
        return &Properties[index];
    }
    return NULL;
}

int CustomPropertySchema::GetPropertyCount() const
{
    return Properties.GetCount();
}

void CustomPropertySchema::Serialize(Common::Stream *out) const
{
    out->WriteInt32(kCustomPropertyVersion_Current);
    out->WriteInt32(Properties.GetCount());
    for (int i = 0; i < Properties.GetCount(); ++i)
    {
        Properties[i].Name.Write(out);
        out->WriteInt32(Properties[i].Type);
        Properties[i].Description.Write(out);
        Properties[i].DefaultValue.Write(out);
    }
}

CustomPropertyError CustomPropertySchema::UnSerialize(Common::Stream *in)
{
    CustomPropertyVersion version = (CustomPropertyVersion)in->ReadInt32();
    if (version < kCustomPropertyVersion_v321 ||
        version > kCustomPropertyVersion_Current)
    {
        return kCustomPropertyErr_UnsupportedFormat;
    }

    Properties.SetLength(in->ReadInt32());
    if (version == kCustomPropertyVersion_v321)
    {
        for (int i = 0; i < Properties.GetCount(); ++i)
        {
            Properties[i].Name.Read(in, LEGACY_MAX_CUSTOM_PROPERTY_SCHEMA_NAME_LENGTH);
            Properties[i].Description.Read(in, LEGACY_MAX_CUSTOM_PROPERTY_DESCRIPTION_LENGTH);
            Properties[i].DefaultValue.Read(in, LEGACY_MAX_CUSTOM_PROPERTY_VALUE_LENGTH);
            Properties[i].Type = (CustomPropertyType)in->ReadInt32();
        }
    }
    else
    {
        for (int i = 0; i < Properties.GetCount(); ++i)
        {
            Properties[i].Name.Read(in);
            Properties[i].Type = (CustomPropertyType)in->ReadInt32();
            Properties[i].Description.Read(in);
            Properties[i].DefaultValue.Read(in);
        }
    }
    return kCustomPropertyErr_NoError;
}

void CustomProperties::AddProperty(const String &name, const String &value)
{
    Properties.Append(CustomPropertyState(name, value));
}

CustomPropertyState *CustomProperties::FindProperty(const String &name)
{
    for (int i = 0; i < Properties.GetCount(); ++i)
    {
        if (Properties[i].Name.CompareNoCase(name) == 0)
        {
            return &Properties[i];
        }
    }
    return NULL;
}

void CustomProperties::Free()
{
    Properties.Free();
}

CustomPropertyState *CustomProperties::GetProperty(int index)
{
    if (index >= 0 && index < Properties.GetCount())
    {
        return &Properties[index];
    }
    return NULL;
}

int CustomProperties::GetPropertyCount() const
{
    return Properties.GetCount();
}

void CustomProperties::Serialize(Common::Stream *out) const
{
    out->WriteInt32(kCustomPropertyVersion_Current);
    out->WriteInt32(Properties.GetCount());
    for (int i = 0; i < Properties.GetCount(); ++i)
    {
        Properties[i].Name.Write(out);
        Properties[i].Value.Write(out);
    }
}

CustomPropertyError CustomProperties::UnSerialize(Common::Stream *in)
{
    CustomPropertyVersion version = (CustomPropertyVersion)in->ReadInt32();
    if (version < kCustomPropertyVersion_v321 ||
        version > kCustomPropertyVersion_Current)
    {
        return kCustomPropertyErr_UnsupportedFormat;
    }

    Properties.SetLength(in->ReadInt32());
    if (version == kCustomPropertyVersion_v321)
    {
        for (int i = 0; i < Properties.GetCount(); ++i)
        {
            Properties[i].Name.Read(in, LEGACY_MAX_CUSTOM_PROPERTY_NAME_LENGTH);
            Properties[i].Value.Read(in, LEGACY_MAX_CUSTOM_PROPERTY_VALUE_LENGTH);
        }
    }
    else
    {
        for (int i = 0; i < Properties.GetCount(); ++i)
        {
            Properties[i].Name.Read(in);
            Properties[i].Value.Read(in);
        }
    }
    return kCustomPropertyErr_NoError;
}

} // namespace Common
} // namespace AGS
