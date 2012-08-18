//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__TOPBARSETTINGS_H
#define __AGS_EE_AC__TOPBARSETTINGS_H

struct TopBarSettings {
    int wantIt;
    int height;
    int font;
    char text[200];

    TopBarSettings() {
        wantIt = 0;
        height = 0;
        font = 0;
        text[0] = 0;
    }
};

#endif // __AGS_EE_AC__TOPBARSETTINGS_H
