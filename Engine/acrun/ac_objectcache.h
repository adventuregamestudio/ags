
// stores cached object info
struct ObjectCache {
    block image;
    int   sppic;
    short tintredwas, tintgrnwas, tintbluwas, tintamntwas, tintlightwas;
    short lightlevwas, mirroredWas, zoomWas;
    // The following are used to determine if the character has moved
    int   xwas, ywas;
};
