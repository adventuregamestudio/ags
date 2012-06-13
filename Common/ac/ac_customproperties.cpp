
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ac_customproperties.h"
#include "cs/cs_utils.h"            // fputstring, etc

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
void CustomPropertySchema::Serialize (FILE *outto) {
    putw (1, outto);  // version 1 at present
    putw (numProps, outto);
    for (int jj = 0; jj < numProps; jj++) {
        fputstring (propName[jj], outto);
        fputstring (propDesc[jj], outto);
        fputstring (defaultValue[jj], outto);
        putw (propType[jj], outto);
    }

}

int CustomPropertySchema::UnSerialize (FILE *infrom) {
    if (getw(infrom) != 1)
        return -1;
    numProps = getw(infrom);
    for (int kk = 0; kk < numProps; kk++) {
        this->resetProperty (kk);
        fgetstring_limit (propName[kk], infrom, 20);
        fgetstring_limit (propDesc[kk], infrom, 100);
        fgetstring_limit (defaultValue[kk], infrom, MAX_CUSTOM_PROPERTY_VALUE_LENGTH);
        propType[kk] = getw(infrom);
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
void CustomProperties::Serialize (FILE *outto) {
    putw (1, outto);
    putw (numProps, outto);
    for (int ee = 0; ee < numProps; ee++) {
        fputstring (propName[ee], outto);
        fputstring (propVal[ee], outto);
    }
}

int CustomProperties::UnSerialize (FILE *infrom) {
    if (getw(infrom) != 1)
        return -1;
    numProps = getw(infrom);
    for (int ee = 0; ee < numProps; ee++) {
        propName[ee] = (char*)malloc(200);
        propVal[ee] = (char*)malloc(MAX_CUSTOM_PROPERTY_VALUE_LENGTH);
        fgetstring_limit (propName[ee], infrom, 200);
        fgetstring_limit (propVal[ee], infrom, MAX_CUSTOM_PROPERTY_VALUE_LENGTH);
    }
    return 0;
}
