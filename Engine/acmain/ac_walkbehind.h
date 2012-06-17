
enum WalkBehindMethodEnum
{
    DrawOverCharSprite,
    DrawAsSeparateSprite,
    DrawAsSeparateCharSprite
};

extern char *walkBehindExists;  // whether a WB area is in this column
extern int *walkBehindStartY, *walkBehindEndY ;
extern char noWalkBehindsAtAll;
extern int walkBehindLeft[MAX_OBJ], walkBehindTop[MAX_OBJ];
extern int walkBehindRight[MAX_OBJ], walkBehindBottom[MAX_OBJ];
extern IDriverDependantBitmap *walkBehindBitmap[MAX_OBJ];
extern int walkBehindsCachedForBgNum;
extern WalkBehindMethodEnum walkBehindMethod;
