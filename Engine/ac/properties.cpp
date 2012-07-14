
#include "wgt2allg.h"
#include "ac/common.h"
#include "ac/gamesetupstruct.h"
#include "ac/properties.h"
#include "ac/string.h"

extern GameSetupStruct game;

// begin custom property functions

// Get an integer property
int get_int_property (CustomProperties *cprop, const char *property) {
    int idx = game.propSchema.findProperty(property);

    if (idx < 0)
        quit("!GetProperty: no such property found in schema. Make sure you are using the property's name, and not its description, when calling this command.");

    if (game.propSchema.propType[idx] == PROP_TYPE_STRING)
        quit("!GetProperty: need to use GetPropertyString for a text property");

    const char *valtemp = cprop->getPropertyValue(property);
    if (valtemp == NULL) {
        valtemp = game.propSchema.defaultValue[idx];
    }
    return atoi(valtemp);
}

// Get a string property
void get_text_property (CustomProperties *cprop, const char *property, char *bufer) {
    int idx = game.propSchema.findProperty(property);

    if (idx < 0)
        quit("!GetPropertyText: no such property found in schema. Make sure you are using the property's name, and not its description, when calling this command.");

    if (game.propSchema.propType[idx] != PROP_TYPE_STRING)
        quit("!GetPropertyText: need to use GetProperty for a non-text property");

    const char *valtemp = cprop->getPropertyValue(property);
    if (valtemp == NULL) {
        valtemp = game.propSchema.defaultValue[idx];
    }
    strcpy (bufer, valtemp);
}

const char* get_text_property_dynamic_string(CustomProperties *cprop, const char *property) {
    int idx = game.propSchema.findProperty(property);

    if (idx < 0)
        quit("!GetTextProperty: no such property found in schema. Make sure you are using the property's name, and not its description, when calling this command.");

    if (game.propSchema.propType[idx] != PROP_TYPE_STRING)
        quit("!GetTextProperty: need to use GetProperty for a non-text property");

    const char *valtemp = cprop->getPropertyValue(property);
    if (valtemp == NULL) {
        valtemp = game.propSchema.defaultValue[idx];
    }

    return CreateNewScriptString(valtemp);
}
