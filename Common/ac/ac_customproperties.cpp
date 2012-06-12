
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ac_customproperties.h"
#include "cs/cs_utils.h"            // fputstring, etc

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
