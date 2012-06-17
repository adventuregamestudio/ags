
struct SpriteListEntry {
    IDriverDependantBitmap *bmp;
    block pic;
    int baseline;
    int x,y;
    int transparent;
    bool takesPriorityIfEqual;
    bool hasAlphaChannel;
};
