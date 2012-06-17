
// stores cached info about the character
struct CharacterCache {
    block image;
    int sppic;
    int scaling;
    int inUse;
    short tintredwas, tintgrnwas, tintbluwas, tintamntwas;
    short lightlevwas, tintlightwas;
    // no mirroredWas is required, since the code inverts the sprite number
};
