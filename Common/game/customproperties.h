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
// Custom property structs
//
//-----------------------------------------------------------------------------
//
// Custom property schema is kept by GameInfo object as a single instance and
// defines property type and default value. Every game entity that has
// properties implemented keeps CustomProperties object, which stores actual
// property values only if ones are different from defaults.
//
// TODO: use binary tree or hash map to store properties.
//
//=============================================================================
#ifndef __AGS_CN_GAME__CUSTOMPROPERTIES_H
#define __AGS_CN_GAME__CUSTOMPROPERTIES_H

#include "util/array.h"
#include "util/string.h"

namespace AGS
{
namespace Common
{

class Stream;

enum CustomPropertyVersion
{
    kCustomPropertyVersion_v321,
    kCustomPropertyVersion_v340,
    kCustomPropertyVersion_Current = kCustomPropertyVersion_v340
};

enum CustomPropertyType
{
    kCustomPropertyUndefined = 0,
    kCustomPropertyBoolean,
    kCustomPropertyInteger,
    kCustomPropertyString
};

enum CustomPropertyError
{
    kCustomPropertyErr_NoError,
    kCustomPropertyErr_UnsupportedFormat
};

struct CustomPropertyInfo
{
    String              Name;
    CustomPropertyType  Type;
    String              Description;
    String              DefaultValue;

    CustomPropertyInfo()
    {
        Type = kCustomPropertyBoolean;
    }
    CustomPropertyInfo(const String &name, CustomPropertyType type,
        const String &description, const String &def_value)
    {
        Name = name;
        Type = type;
        Description = description;
        DefaultValue = def_value;
    }
};

class CustomPropertySchema
{
public:
    void                AddProperty(const String &name, CustomPropertyType type,
                                    const String &description, const String &def_value);
    CustomPropertyInfo  *FindProperty(const String &name);
    void                Free();
    CustomPropertyInfo  *GetProperty(int index);
    int                 GetPropertyCount() const;
    void                Serialize(Stream *out) const;
    CustomPropertyError UnSerialize(Stream *in);

private:
    ObjectArray<CustomPropertyInfo> Properties;
};

struct CustomPropertyState
{
    String Name;
    String Value;

    CustomPropertyState(){}
    CustomPropertyState(const String name, const String &value)
    {
        Name = name;
        Value = value;
    }
};

class CustomProperties
{
public:
    void                AddProperty(const String &name, const String &value);
    CustomPropertyState *FindProperty(const String &name);
    void                Free();
    CustomPropertyState *GetProperty(int index);
    int                 GetPropertyCount() const;
    void                Serialize(Stream *out) const;
    CustomPropertyError UnSerialize(Stream *in);

private:
    ObjectArray<CustomPropertyState> Properties;
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_GAME__CUSTOMPROPERTIES_H
