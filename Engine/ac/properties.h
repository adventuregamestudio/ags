
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__PROPERTIES_H
#define __AGS_EE_AC__PROPERTIES_H

#include "ac/customproperties.h"

int get_int_property (CustomProperties *cprop, const char *property);
void get_text_property (CustomProperties *cprop, const char *property, char *bufer);
const char* get_text_property_dynamic_string(CustomProperties *cprop, const char *property);

#endif // __AGS_EE_AC__PROPERTIES_H
