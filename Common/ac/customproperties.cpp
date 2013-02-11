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
#include "ac/customproperties.h"
#include "util/string_utils.h"      // out->WriteString, etc
#include "util/stream.h"
#include "util/string.h"

using AGS::Common::String;
using AGS::Common::Stream;

// Find the index of the specified property
int CustomPropertySchema::findProperty (const char *pname) {
    for (int ii = 0; ii < numProps; ii++) {
        if (stricmp (pname, propName[ii]) == 0)
            return ii;
    }
    return -1;
}

void CustomPropertySchema::deleteProperty (int idx) {
    if ((idx < 0) || (idx >= numProps))
        return;

    if (defaultValue[idx])
        delete defaultValue[idx];

    numProps--;
    for (int qq = idx; qq < numProps; qq++) {
        strcpy (propName[qq], propName[qq+1]);
        strcpy (propDesc[qq], propDesc[qq+1]);
        propType[qq] = propType[qq+1];
        defaultValue[qq] = defaultValue[qq+1];
    }
    defaultValue[numProps] = NULL;
}

void CustomPropertySchema::resetProperty (int idx) {
    propName[idx][0] = 0;
    propDesc[idx][0] = 0;
    if (defaultValue[idx])
        delete defaultValue[idx];
    propType[idx] = PROP_TYPE_BOOL;
    defaultValue[idx] = new char[MAX_CUSTOM_PROPERTY_VALUE_LENGTH];
    defaultValue[idx][0] = 0;
}

CustomPropertySchema::CustomPropertySchema () {
    numProps = 0;
    for (int kk = 0; kk < MAX_CUSTOM_PROPERTIES; kk++) {
        defaultValue[kk] = NULL;
    }
}

// ** SCHEMA LOAD/SAVE FUNCTIONS
void CustomPropertySchema::Serialize (Stream *out) {
    out->WriteInt32 (1);  // version 1 at present
    out->WriteInt32 (numProps);
    for (int jj = 0; jj < numProps; jj++) {
        String::WriteString (propName[jj], out);
        String::WriteString (propDesc[jj], out);
        String::WriteString (defaultValue[jj], out);
        out->WriteInt32 (propType[jj]);
    }
}

int CustomPropertySchema::UnSerialize (Stream *in) {
    if (in->ReadInt32() != 1)
        return -1;
    numProps = in->ReadInt32();
    for (int kk = 0; kk < numProps; kk++) {
        this->resetProperty (kk);
        fgetstring_limit (propName[kk], in, 20);
        fgetstring_limit (propDesc[kk], in, 100);
        fgetstring_limit (defaultValue[kk], in, MAX_CUSTOM_PROPERTY_VALUE_LENGTH);
        propType[kk] = in->ReadInt32();
    }

    return 0;
}


CustomProperties::CustomProperties() {
    numProps = 0;
}

const char *CustomProperties::getPropertyValue (const char *pname) {
    int idxx = findProperty(pname);
    if (idxx < 0)
        return NULL;

    return propVal[idxx];
}

// Find the index of the specified property
int CustomProperties::findProperty (const char *pname) {
    for (int ii = 0; ii < numProps; ii++) {
        if (stricmp (pname, propName[ii]) == 0)
            return ii;
    }
    return -1;
}

void CustomProperties::reset () {
    for (int ii = 0; ii < numProps; ii++) {
        free (propName[ii]);
        free (propVal[ii]);
    }
    numProps = 0;
}

void CustomProperties::addProperty (const char *newname, const char *newval) {
    if (numProps >= MAX_CUSTOM_PROPERTIES) {
        return;
    }
    propName[numProps] = (char*)malloc(200);
    propVal[numProps] = (char*)malloc(MAX_CUSTOM_PROPERTY_VALUE_LENGTH);
    strcpy (propName[numProps], newname);
    strcpy (propVal[numProps], newval);
    numProps++;
}

// ** OBJECT PROPERTIES LOAD/SAVE FUNCTIONS
void CustomProperties::Serialize (Stream *out) {
    out->WriteInt32 (1);
    out->WriteInt32 (numProps);
    for (int ee = 0; ee < numProps; ee++) {
        String::WriteString (propName[ee], out);
        String::WriteString (propVal[ee], out);
    }
}

int CustomProperties::UnSerialize (Stream *in) {
    if (in->ReadInt32() != 1)
        return -1;
    numProps = in->ReadInt32();
    for (int ee = 0; ee < numProps; ee++) {
        propName[ee] = (char*)malloc(200);
        propVal[ee] = (char*)malloc(MAX_CUSTOM_PROPERTY_VALUE_LENGTH);
        fgetstring_limit (propName[ee], in, 200);
        fgetstring_limit (propVal[ee], in, MAX_CUSTOM_PROPERTY_VALUE_LENGTH);
    }
    return 0;
}
