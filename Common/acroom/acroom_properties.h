#ifndef __CROOM_PROPERTIES_H
#define __CROOM_PROPERTIES_H



#define MAX_CUSTOM_PROPERTIES 30
#define MAX_CUSTOM_PROPERTY_VALUE_LENGTH 500
#define PROP_TYPE_BOOL   1
#define PROP_TYPE_INT    2
#define PROP_TYPE_STRING 3
struct CustomPropertySchema {
    char  propName[MAX_CUSTOM_PROPERTIES][20];
    char  propDesc[MAX_CUSTOM_PROPERTIES][100];
    char *defaultValue[MAX_CUSTOM_PROPERTIES];
    int   propType[MAX_CUSTOM_PROPERTIES];
    int   numProps;

    // Find the index of the specified property
    int findProperty (const char *pname) {
        for (int ii = 0; ii < numProps; ii++) {
            if (stricmp (pname, propName[ii]) == 0)
                return ii;
        }
        return -1;
    }

    void deleteProperty (int idx) {
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

    void resetProperty (int idx) {
        propName[idx][0] = 0;
        propDesc[idx][0] = 0;
        if (defaultValue[idx])
            delete defaultValue[idx];
        propType[idx] = PROP_TYPE_BOOL;
        defaultValue[idx] = new char[MAX_CUSTOM_PROPERTY_VALUE_LENGTH];
        defaultValue[idx][0] = 0;
    }

    CustomPropertySchema () {
        numProps = 0;
        for (int kk = 0; kk < MAX_CUSTOM_PROPERTIES; kk++) {
            defaultValue[kk] = NULL;
        }
    }

    void Serialize (FILE *outto);
    int UnSerialize (FILE *infrom);

};


struct CustomProperties {
    char *propName[MAX_CUSTOM_PROPERTIES];
    char *propVal[MAX_CUSTOM_PROPERTIES];
    int   numProps;

    CustomProperties() {
        numProps = 0;
    }

    const char *getPropertyValue (const char *pname) {
        int idxx = findProperty(pname);
        if (idxx < 0)
            return NULL;

        return propVal[idxx];
    }

    // Find the index of the specified property
    int findProperty (const char *pname) {
        for (int ii = 0; ii < numProps; ii++) {
            if (stricmp (pname, propName[ii]) == 0)
                return ii;
        }
        return -1;
    }

    void reset () {
        for (int ii = 0; ii < numProps; ii++) {
            free (propName[ii]);
            free (propVal[ii]);
        }
        numProps = 0;
    }

    void addProperty (const char *newname, const char *newval) {
        if (numProps >= MAX_CUSTOM_PROPERTIES) {
            return;
        }
        propName[numProps] = (char*)malloc(200);
        propVal[numProps] = (char*)malloc(MAX_CUSTOM_PROPERTY_VALUE_LENGTH);
        strcpy (propName[numProps], newname);
        strcpy (propVal[numProps], newval);
        numProps++;
    }

    void Serialize (FILE *outto);
    int  UnSerialize (FILE *infrom);
};

#endif // __CROOM_PROPERTIES_H