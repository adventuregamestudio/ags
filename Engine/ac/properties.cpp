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
#include "ac/string.h"
#include "ac/dynobj/scriptstring.h"
#include "game/game_objects.h"
#include "script/runtimescriptvalue.h"

using Common::CustomProperties;
using Common::CustomPropertyInfo;
using Common::CustomPropertyState;
using Common::String;

extern ScriptString myScriptStringImpl;

// begin custom property functions

// Get an integer property
int get_int_property(CustomProperties *cprop, const char *property) {
    CustomPropertyInfo *prop_info = game.PropertySchema.FindProperty(property);

    if (!prop_info)
        quit("!GetProperty: no such property found in schema. Make sure you are using the property's name, and not its description, when calling this command.");

    if (prop_info->Type == Common::kCustomPropertyString)
        quit("!GetProperty: need to use GetPropertyString for a text property");

    AGS::Common::CustomPropertyState *prop_state = cprop->FindProperty(property);
    return prop_state ? prop_state->Value.ToInt() : prop_info->DefaultValue.ToInt();
}

// Get a string property
void get_text_property(CustomProperties *cprop, const char *property, char *bufer) {
    CustomPropertyInfo *prop_info = game.PropertySchema.FindProperty(property);

    if (!prop_info)
        quit("!GetPropertyText: no such property found in schema. Make sure you are using the property's name, and not its description, when calling this command.");

    if (prop_info->Type != Common::kCustomPropertyString)
        quit("!GetPropertyText: need to use GetProperty for a non-text property");

    AGS::Common::CustomPropertyState *prop_state = cprop->FindProperty(property);
    String val_str = prop_state ? prop_state->Value : prop_info->DefaultValue;
    strcpy (bufer, val_str);
}

const char* get_text_property_dynamic_string(CustomProperties *cprop, const char *property) {
    CustomPropertyInfo *prop_info = game.PropertySchema.FindProperty(property);

    if (!prop_info)
        quit("!GetPropertyText: no such property found in schema. Make sure you are using the property's name, and not its description, when calling this command.");

    if (prop_info->Type != Common::kCustomPropertyString)
        quit("!GetPropertyText: need to use GetProperty for a non-text property");

    AGS::Common::CustomPropertyState *prop_state = cprop->FindProperty(property);
    String val_str = prop_state ? prop_state->Value : prop_info->DefaultValue;
    return CreateNewScriptString(val_str);
}
