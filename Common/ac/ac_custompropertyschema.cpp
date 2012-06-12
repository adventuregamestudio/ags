
#include <stdio.h>
#include <string.h>
#include "ac_custompropertyschema.h"
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
