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

#include "ac/common.h"
#include "ac/gamesetupstruct.h"
#include "ac/properties.h"
#include "ac/string.h"
#include "ac/dynobj/scriptstring.h"
#include "script/runtimescriptvalue.h"

using namespace AGS::Common;

extern GameSetupStruct game;
extern ScriptString myScriptStringImpl;

// begin custom property functions

bool get_property_desc(PropertyDesc &desc, const char *property, PropertyType want_type)
{
    PropertySchema::const_iterator sch_it = game.propSchema.find(property);
    if (sch_it == game.propSchema.end())
        quit("!GetProperty: no such property found in schema. Make sure you are using the property's name, and not its description, when calling this command.");

    desc = sch_it->second;
    if (want_type == kPropertyString && desc.Type != kPropertyString)
        quit("!GetTextProperty: need to use GetProperty for a non-text property");
    else if (want_type != kPropertyString && desc.Type == kPropertyString)
        quit("!GetProperty: need to use GetTextProperty for a text property");
    return true;
}

// Get an integer property
int get_int_property (const StringIMap &cprop, const char *property)
{
    PropertyDesc desc;
    if (!get_property_desc(desc, property, kPropertyInteger))
        return 0;

    StringIMap::const_iterator str_it = cprop.find(property);
    const char *cstr = str_it != cprop.end() ? str_it->second : desc.DefaultValue;
    return atoi(cstr);
}

// Get a string property
void get_text_property (const StringIMap &cprop, const char *property, char *bufer)
{
    PropertyDesc desc;
    if (!get_property_desc(desc, property, kPropertyString))
        return;

    StringIMap::const_iterator str_it = cprop.find(property);
    const char *cstr = str_it != cprop.end() ? str_it->second : desc.DefaultValue;
    strcpy(bufer, cstr);
}

const char* get_text_property_dynamic_string(const StringIMap &cprop, const char *property)
{
    PropertyDesc desc;
    if (!get_property_desc(desc, property, kPropertyString))
        return NULL;

    StringIMap::const_iterator str_it = cprop.find(property);
    const char *cstr = str_it != cprop.end() ? str_it->second : desc.DefaultValue;
    return CreateNewScriptString(cstr);
}

void set_int_property(AGS::Common::StringIMap &cprop, const char *property, int value)
{
    PropertyDesc desc;
    if (get_property_desc(desc, property, kPropertyInteger))
        cprop[desc.Name] = String::FromFormat("%d", value);
}

void set_text_property(AGS::Common::StringIMap &cprop, const char *property, const char* value)
{
    PropertyDesc desc;
    if (get_property_desc(desc, property, kPropertyString))
        cprop[desc.Name] = value;
}
