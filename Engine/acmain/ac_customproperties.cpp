
#include <stdio.h>
#include "wgt2allg.h"
#include "ac/roomstruct.h"
#include "ac/gamesetupstruct.h"
#include "acmain/ac_maindefines.h"
#include "acmain/ac_customproperties.h"
#include "acmain/ac_commonheaders.h"

extern GameSetupStruct game;
extern roomstruct thisroom;

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

int GetInvProperty (int item, const char *property) {
  return get_int_property (&game.invProps[item], property);
}
int InventoryItem_GetProperty(ScriptInvItem *scii, const char *property) {
  return get_int_property (&game.invProps[scii->id], property);
}











void GetInvPropertyText (int item, const char *property, char *bufer) {
  get_text_property (&game.invProps[item], property, bufer);
}
void InventoryItem_GetPropertyText(ScriptInvItem *scii, const char *property, char *bufer) {
  get_text_property(&game.invProps[scii->id], property, bufer);
}
const char* InventoryItem_GetTextProperty(ScriptInvItem *scii, const char *property) {
  return get_text_property_dynamic_string(&game.invProps[scii->id], property);
}





// end custom property functions

