#define WINDOWS_VERSION
#define USE_CLIB
#define SWAP_RB_HICOL  // win32 uses BGR not RGB
#include <stdio.h>
void serialize_room_interactions(FILE *);
void ThrowManagedException(const char *message);
#pragma unmanaged
#pragma warning (disable: 4996 4312)  // disable deprecation warnings
extern "C" {
  extern FILE *clibfopen(char *filnamm, char *fmt);
  extern int csetlib(char *fileName, char *password);
  extern int clibGetNumFiles();
  extern const char *clibGetFileName(int);
  extern const char *clibgetoriginalfilename();
  extern int cfopenpriority;
}
extern bool Scintilla_RegisterClasses(void *hInstance);
extern int Scintilla_LinkLexers();

int antiAliasFonts = 0;
#define SAVEBUFFERSIZE 5120
bool ShouldAntiAliasText() { return (antiAliasFonts != 0); }

int mousex, mousey;
#include "misc.h"
#include "wgt2allg_func.h"
#include "sprcache.h"
#include "acroom_func.h"
#include "acroom_savefunc.h"
#include "acgui.h"

#include "cs/cs_internal.h" // fputstring, etc

extern void Cstretch_blit(BITMAP *src, BITMAP *dst, int sx, int sy, int sw, int sh, int dx, int dy, int dw, int dh);
extern void Cstretch_sprite(BITMAP *dst, BITMAP *src, int x, int y, int w, int h);


int sxmult = 1, symult = 1;
int dsc_want_hires = 0;
bool enable_greyed_out_masks = true;
bool outlineGuiObjects;
color*palette;
GameSetupStruct thisgame;
SpriteCache spriteset(MAX_SPRITES + 2);
GUIMain tempgui;
const char*sprsetname = "acsprset.spr";
const char*clibendsig = "CLIB\x1\x2\x3\x4SIGE";
const char *old_editor_data_file = "editor.dat";
const char *new_editor_data_file = "game.agf";
const char *old_editor_main_game_file = "ac2game.dta";
const char *TEMPLATE_LOCK_FILE = "template.dta";
const char *ROOM_TEMPLATE_ID_FILE = "rtemplate.dat";
const int ROOM_TEMPLATE_ID_FILE_SIGNATURE = 0x74673812;
bool spritesModified = false;
roomstruct thisroom;
bool roomModified = false;
block drawBuffer = NULL;
block undoBuffer = NULL;
int loaded_room_number = -1;

// stuff for importing old games
int numScriptModules;
ScriptModule* scModules = NULL;
DialogTopic *dialog;
char*dlgscript[MAX_DIALOG];
GUIMain *guis;
ViewStruct272 *oldViews;
ViewStruct *newViews;
int numNewViews = 0;


bool reload_font(int curFont);
void drawBlockScaledAt(int hdc, block todraw ,int x, int y, int scaleFactor);
// this is to shut up the linker, it's used by CSRUN.CPP
void write_log(char *) { }

void GUIInv::Draw() {
  wsetcolor(15);
  wrectangle(x,y,x+wid,y+hit);
}

int multiply_up_coordinate(int coord)
{
	return coord * sxmult;
}

int get_fixed_pixel_size(int coord)
{
	return coord * sxmult;
}

// jibbles the sprite around to fix hi-color problems, by swapping
// the red and blue elements
#define fix_sprite(num) fix_block(spriteset[num])
void fix_block (block todraw) {
  int a,b,pixval;
  if (todraw == NULL)
    return;
  if (bitmap_color_depth(todraw) == 16) {
    for (a = 0; a < todraw->w; a++) {
      for (b = 0; b < todraw->h; b++) {
        pixval = _getpixel16 (todraw, a, b);
        _putpixel16 (todraw, a, b, makecol16 (getb16(pixval),getg16(pixval),getr16(pixval)));
      }
    }
  }
  else if (bitmap_color_depth(todraw) == 32) {
    for (a = 0; a < todraw->w; a++) {
      for (b = 0; b < todraw->h; b++) {
        pixval = _getpixel32 (todraw, a, b);
        _putpixel32 (todraw, a, b, makeacol32 (getb32(pixval),getg32(pixval),getr32(pixval), geta32(pixval)));
      }
    }
  }
}

void initialize_sprite(int spnum) {
  fix_sprite(spnum);
}

void pre_save_sprite(int spnum) {
  fix_sprite(spnum);
}

block get_sprite (int spnr) {
  if (spnr < 0)
    return NULL;
  if (spriteset[spnr] == NULL) {
    spnr = 0;
  }
  return spriteset[spnr];
}

void SetNewSprite(int slot, block sprit) {
  if (spriteset[slot] != NULL)
    wfreeblock(spriteset[slot]);

  spriteset.setNonDiscardable(slot, sprit);
  spritesModified = true;
}

void deleteSprite (int sprslot) {
  spriteset.removeSprite(sprslot, true);
  
  spritesModified = true;
}

void SetNewSpriteFromHBitmap(int slot, int hBmp) {
  block tempsprite = convert_hbitmap_to_bitmap((HBITMAP)hBmp);
  SetNewSprite(slot, tempsprite);
}

int GetSpriteAsHBitmap(int slot) {
  return (int)convert_bitmap_to_hbitmap(get_sprite(slot));
}

bool DoesSpriteExist(int slot) {
	return (spriteset[slot] != NULL);
}

int GetMaxSprites() {
	return MAX_SPRITES;
}

int GetSpriteWidth(int slot) {
	return get_sprite(slot)->w;
}

int GetSpriteHeight(int slot) {
	return get_sprite(slot)->h;
}

int GetRelativeSpriteWidth(int slot) {
	return GetSpriteWidth(slot) / ((thisgame.spriteflags[slot] & SPF_640x400) ? 2 : 1);
}

int GetRelativeSpriteHeight(int slot) {
	return GetSpriteHeight(slot) / ((thisgame.spriteflags[slot] & SPF_640x400) ? 2 : 1);
}

int GetSpriteResolutionMultiplier(int slot)
{
	return ((thisgame.spriteflags[slot] & SPF_640x400) ? 1 : 2);
}

unsigned char* GetRawSpriteData(int spriteSlot) {
  return &get_sprite(spriteSlot)->line[0][0];
}

int GetSpriteColorDepth(int slot) {
  return bitmap_color_depth(get_sprite(slot));
}

int GetPaletteAsHPalette() {
  return (int)convert_palette_to_hpalette(palette);
}

void transform_string(char *text) {
	encrypt_text(text);
}

int find_free_sprite_slot() {
  int rr = spriteset.findFreeSlot();
  if (rr < 0) {
    return -1;
  }
  spriteset.images[rr] = NULL;
  spriteset.offsets[rr] = 0;
  spriteset.sizes[rr] = 0;
  return rr;
}

void update_sprite_resolution(int spriteNum, bool isHighRes)
{
	thisgame.spriteflags[spriteNum] &= ~SPF_640x400;
	if (isHighRes)
	{
		thisgame.spriteflags[spriteNum] |= SPF_640x400;
	}
}

void change_sprite_number(int oldNumber, int newNumber) {

  spriteset.setNonDiscardable(newNumber, spriteset[oldNumber]);
  spriteset.removeSprite(oldNumber, false);

  thisgame.spriteflags[newNumber] = thisgame.spriteflags[oldNumber];
  thisgame.spriteflags[oldNumber] = 0;

  spritesModified = true;
}

int crop_sprite_edges(int numSprites, int *sprites, bool symmetric) {
  // this function has passed in a list of sprites, all the
  // same size, to crop to the size of the smallest
  int aa, xx, yy;
  int width = spriteset[sprites[0]]->w;
  int height = spriteset[sprites[0]]->h;
  int left = width, right = 0;
  int top = height, bottom = 0;

  for (aa = 0; aa < numSprites; aa++) {
    block sprit = get_sprite(sprites[aa]);
    int maskcol = bitmap_mask_color(sprit);

    // find the left hand side
    for (xx = 0; xx < width; xx++) {
      for (yy = 0; yy < height; yy++) {
        if (getpixel(sprit, xx, yy) != maskcol) {
          if (xx < left)
            left = xx;
          xx = width + 10;
          break;
        }
      }
    }
    // find the right hand side
    for (xx = width - 1; xx >= 0; xx--) {
      for (yy = 0; yy < height; yy++) {
        if (getpixel(sprit, xx, yy) != maskcol) {
          if (xx > right)
            right = xx;
          xx = -10;
          break;
        }
      }
    }
    // find the top side
    for (yy = 0; yy < height; yy++) {
      for (xx = 0; xx < width; xx++) {
        if (getpixel(sprit, xx, yy) != maskcol) {
          if (yy < top)
            top = yy;
          yy = height + 10;
          break;
        }
      }
    }
    // find the bottom side
    for (yy = height - 1; yy >= 0; yy--) {
      for (xx = 0; xx < width; xx++) {
        if (getpixel(sprit, xx, yy) != maskcol) {
          if (yy > bottom)
            bottom = yy;
          yy = -10;
          break;
        }
      }
    }
  }

  // Now, we have been through all the sprites and found the outer
  // edges that we need to keep. So, now crop everything down
  // to the smaller sizes
  if (symmetric) {
    // symmetric -- make sure that the left and right edge chopping
    // are equal
    int rightDist = (width - right) - 1;
    if (rightDist < left)
      left = rightDist;
    if (left < rightDist)
      right = (width - left) - 1;
  }
  int newWidth = (right - left) + 1;
  int newHeight = (bottom - top) + 1;

  if ((newWidth == width) && (newHeight == height)) {
    // no change in size
    return 0;
  }

  if ((newWidth < 1) || (newHeight < 1))
  {
	  // completely transparent sprite, don't attempt to crop
	  return 0;
  }

  for (aa = 0; aa < numSprites; aa++) {
    block sprit = get_sprite(sprites[aa]);
    // create a new, smaller sprite and copy across
    block newsprit = create_bitmap_ex(bitmap_color_depth(sprit), newWidth, newHeight);
    blit(sprit, newsprit, left, top, 0, 0, newWidth, newHeight);
    destroy_bitmap(sprit);

    spriteset.setNonDiscardable(sprites[aa], newsprit);
  }

  spritesModified = true;

  return 1;
}

int extract_room_template_files(const char *templateFileName, int newRoomNumber) 
{
  if (csetlib((char*)templateFileName, "")) 
  {
    return 0;
  }
  if (cliboffset((char*)ROOM_TEMPLATE_ID_FILE) < 1)
  {
    csetlib(NULL, "");
    return 0;
  }

  int numFile = clibGetNumFiles();

  for (int a = 0; a < numFile; a++) {
    const char *thisFile = clibGetFileName(a);
    if (thisFile == NULL) {
      csetlib(NULL, "");
      return 0;
    }

    // don't extract the template metadata file
    if (stricmp(thisFile, ROOM_TEMPLATE_ID_FILE) == 0)
      continue;

    FILE *readin = clibfopen ((char*)thisFile, "rb");
    char outputName[MAX_PATH];
    const char *extension = strchr(thisFile, '.');
    sprintf(outputName, "room%d%s", newRoomNumber, extension);
    FILE *wrout = fopen(outputName, "wb");
    if ((readin == NULL) || (wrout == NULL)) 
    {
      if (wrout != NULL) fclose(wrout);
      if (readin != NULL) fclose(readin);
      csetlib(NULL, "");
      return 0;
    }
    long size = clibfilesize((char*)thisFile);
    char *membuff = (char*)malloc (size);
    fread (membuff, 1, size, readin);
    fwrite (membuff, 1, size, wrout);
    fclose (readin);
    fclose (wrout);
    free (membuff);
  }

  csetlib(NULL, "");
  return 1;
}

int extract_template_files(const char *templateFileName) 
{
  if (csetlib((char*)templateFileName, "")) 
  {
    return 0;
  }
  
  if ((cliboffset((char*)old_editor_data_file) < 1) && (cliboffset((char*)new_editor_data_file) < 1))
  {
    csetlib(NULL, "");
    return 0;
  }

  int numFile = clibGetNumFiles();

  for (int a = 0; a < numFile; a++) {
    const char *thisFile = clibGetFileName (a);
    if (thisFile == NULL) {
      csetlib(NULL, "");
      return 0;
    }

    // don't extract the dummy template lock file
    if (stricmp(thisFile, TEMPLATE_LOCK_FILE) == 0)
      continue;

    FILE *readin = clibfopen ((char*)thisFile, "rb");
    FILE *wrout = fopen (thisFile, "wb");
    if ((wrout == NULL) && (strchr(thisFile, '\\') != NULL))
    {
      // an old template with Music/Sound folder, create the folder
      char folderName[MAX_PATH];
      strcpy(folderName, thisFile);
      *strchr(folderName, '\\') = 0;
      mkdir(folderName);
      wrout = fopen(thisFile, "wb");
    }
    if ((readin == NULL) || (wrout == NULL)) 
    {
      csetlib(NULL, "");
      return 0;
    }
    long size = clibfilesize((char*)thisFile);
    char *membuff = (char*)malloc (size);
    fread (membuff, 1, size, readin);
    fwrite (membuff, 1, size, wrout);
    fclose (readin);
    fclose (wrout);
    free (membuff);
  }

  csetlib(NULL, "");
  return 1;
}

void extract_icon_from_template(char *iconName, char **iconDataBuffer, long *bufferSize)
{
  // make sure we get the icon from the file
  cfopenpriority = 1;
  long sizey = clibfilesize(iconName);
  FILE* inpu = clibfopen (iconName, "rb");
  if ((inpu != NULL) && (sizey > 0))
  {
    char *iconbuffer = (char*)malloc(sizey);
    fread (iconbuffer, 1, sizey, inpu);
    fclose (inpu);
    *iconDataBuffer = iconbuffer;
    *bufferSize = sizey;
  }
  else
  {
    *iconDataBuffer = NULL;
    *bufferSize = 0;
  }
  // restore to normal setting after NewGameChooser changes it
  cfopenpriority = 2;
}

int load_template_file(const char *fileName, char **iconDataBuffer, long *iconDataSize, bool isRoomTemplate)
{
  if (csetlib((char*)fileName, "") == 0)
  {
    if (isRoomTemplate)
    {
      if (cliboffset((char*)ROOM_TEMPLATE_ID_FILE) > 0)
      {
        FILE *inpu = clibfopen((char*)ROOM_TEMPLATE_ID_FILE, "rb");
        if (getw(inpu) != ROOM_TEMPLATE_ID_FILE_SIGNATURE)
        {
          fclose(inpu);
		  csetlib(NULL, "");
          return 0;
        }
        int roomNumber = getw(inpu);
        fclose(inpu);
        char iconName[MAX_PATH];
        sprintf(iconName, "room%d.ico", roomNumber);
        if (cliboffset(iconName) > 0) 
        {
          extract_icon_from_template(iconName, iconDataBuffer, iconDataSize);
        }
		    csetlib(NULL, "");
        return 1;
      }
	  csetlib(NULL, "");
      return 0;
    }
	  else if ((cliboffset((char*)old_editor_data_file) > 0) || (cliboffset((char*)new_editor_data_file) > 0))
	  {
      const char *oriname = clibgetoriginalfilename();
      if ((strstr(oriname, ".exe") != NULL) ||
          (strstr(oriname, ".dat") != NULL) ||
          (strstr(oriname, ".ags") != NULL)) 
      {
        // wasn't originally meant as a template
		  csetlib(NULL, "");
	      return 0;
      }

	    FILE *inpu = clibfopen((char*)old_editor_main_game_file, "rb");
	    if (inpu != NULL) 
	    {
		    fseek(inpu, 30, SEEK_CUR);
		    int gameVersion = _getw(inpu);
		    fclose(inpu);
		    if (gameVersion != 32)
		    {
			    // older than 2.72 template
				csetlib(NULL, "");
			    return 0;
		    }
	    }

      int useIcon = 0;
      char *iconName = "template.ico";
      if (cliboffset (iconName) < 1)
        iconName = "user.ico";
      // the file is a CLIB file, so let's extract the icon to display
      if (cliboffset (iconName) > 0) 
      {
        extract_icon_from_template(iconName, iconDataBuffer, iconDataSize);
      }
	    csetlib(NULL, "");
      return 1;
    }
  }
  return 0;
}

const char* save_sprites(bool compressSprites) 
{
  const char *errorMsg = NULL;
  char backupname[100];
  sprintf(backupname, "backup_%s", sprsetname);

  if ((spritesModified) || (compressSprites != spriteset.spritesAreCompressed))
  {
    spriteset.detachFile();
    if (exists(backupname) && (unlink(backupname) != 0)) {
      errorMsg = "Unable to overwrite the old backup file. Make sure the backup sprite file is not read-only";
    }
    else if (rename(sprsetname, backupname)) {
      errorMsg = "Unable to create the backup sprite file. Make sure the backup sprite file is not read-only";
    }
    else if (spriteset.attachFile(backupname)) {
      errorMsg = "An error occurred attaching to the backup sprite file. Check write permissions on your game folder";
    }
    else if (spriteset.saveToFile(sprsetname, MAX_SPRITES, compressSprites)) {
      errorMsg = "Unable to save the sprites. An error occurred writing the sprite file.";
    }

    // reset the sprite cache
    spriteset.reset();
    if (spriteset.initFile(sprsetname))
    {
      if (errorMsg == NULL)
        errorMsg = "Unable to re-initialize sprite file after save.";
    }

    if (errorMsg == NULL)
      spritesModified = false;
  }
  return errorMsg;
}

void drawBlockDoubleAt (int hdc, block todraw ,int x, int y) {
  drawBlockScaledAt (hdc, todraw, x, y, 2);
}

void wputblock_stretch(int xpt,int ypt,block tblock,int nsx,int nsy) {
  if (bmp_bpp(tblock) != thisgame.color_depth) {
    block tempst=create_bitmap_ex(thisgame.color_depth*8,tblock->w,tblock->h);
    blit(tblock,tempst,0,0,0,0,tblock->w,tblock->h);
    int ww,vv;
    for (ww=0;ww<tblock->w;ww++) {
      for (vv=0;vv<tblock->h;vv++) {
        if (getpixel(tblock,ww,vv)==bitmap_mask_color(tblock))
          putpixel(tempst,ww,vv,bitmap_mask_color(tempst));
      }
    }
    stretch_sprite(abuf,tempst,xpt,ypt,nsx,nsy);
    wfreeblock(tempst);
  }
  else stretch_sprite(abuf,tblock,xpt,ypt,nsx,nsy);
}

void draw_sprite_compensate(int sprnum, int atxp, int atyp, int seethru) {
  block blptr = get_sprite(sprnum);
  block towrite=blptr;
  int needtofree=0, main_color_depth = thisgame.color_depth * 8;

  if ((bmp_bpp(blptr) > 1) & (main_color_depth==8)) {

    towrite=create_bitmap_ex(8,blptr->w,blptr->h);
    needtofree=1;
    clear_to_color(towrite,bitmap_mask_color(towrite));
    int xxp,yyp,tmv;
    for (xxp=0;xxp<blptr->w;xxp++) {
      for (yyp=0;yyp<blptr->h;yyp++) {
        tmv=getpixel(blptr,xxp,yyp);
        if (tmv != bitmap_mask_color(blptr))
          putpixel(towrite,xxp,yyp,makecol8(getr16(tmv),getg16(tmv),getb16(tmv)));
        }
      }

    }

  int nwid=towrite->w,nhit=towrite->h;
  if (thisgame.spriteflags[sprnum] & SPF_640x400) {
    if (dsc_want_hires == 0) {
      nwid/=2;
      nhit/=2;
    }
  }
  else if (dsc_want_hires) {
    nwid *= 2;
    nhit *= 2;
  }
  wputblock_stretch(atxp,atyp,towrite,nwid,nhit);
  if (needtofree) wfreeblock(towrite);
}

void drawBlock (HDC hdc, block todraw, int x, int y) {
  set_palette_to_hdc (hdc, palette);
  blit_to_hdc (todraw, hdc, 0,0,x,y,todraw->w,todraw->h);
}


enum RoomAreaMask
{
    None,
    Hotspots,
    WalkBehinds,
    WalkableAreas,
    Regions
};

block get_bitmap_for_mask(roomstruct *roomptr, RoomAreaMask maskType) 
{
	if (maskType == RoomAreaMask::None) 
	{
		return NULL;
	}

	block source = NULL;
	switch (maskType) 
	{
	case RoomAreaMask::Hotspots:
		source = roomptr->lookat;
		break;
	case RoomAreaMask::Regions:
		source = roomptr->regions;
		break;
	case RoomAreaMask::WalkableAreas:
		source = roomptr->walls;
		break;
	case RoomAreaMask::WalkBehinds:
		source = roomptr->object;
		break;
	}

	return source;
}

void copy_walkable_to_regions (void *roomptr) {
	roomstruct *theRoom = (roomstruct*)roomptr;
	blit(theRoom->walls, theRoom->regions, 0, 0, 0, 0, theRoom->regions->w, theRoom->regions->h);
}

int get_mask_pixel(void *roomptr, int maskType, int x, int y)
{
	block mask = get_bitmap_for_mask((roomstruct*)roomptr, (RoomAreaMask)maskType);
	return getpixel(mask, x, y);
}

void draw_line_onto_mask(void *roomptr, int maskType, int x1, int y1, int x2, int y2, int color)
{
	block mask = get_bitmap_for_mask((roomstruct*)roomptr, (RoomAreaMask)maskType);
	line(mask, x1, y1, x2, y2, color);
}

void draw_filled_rect_onto_mask(void *roomptr, int maskType, int x1, int y1, int x2, int y2, int color)
{
	block mask = get_bitmap_for_mask((roomstruct*)roomptr, (RoomAreaMask)maskType);
	rectfill(mask, x1, y1, x2, y2, color);
}

void draw_fill_onto_mask(void *roomptr, int maskType, int x1, int y1, int color)
{
	block mask = get_bitmap_for_mask((roomstruct*)roomptr, (RoomAreaMask)maskType);
	floodfill(mask, x1, y1, color);
}

void create_undo_buffer(void *roomptr, int maskType) 
{
	block mask = get_bitmap_for_mask((roomstruct*)roomptr, (RoomAreaMask)maskType);
  if (undoBuffer != NULL)
  {
    if ((undoBuffer->w != mask->w) || (undoBuffer->h != mask->h)) 
    {
      destroy_bitmap(undoBuffer);
      undoBuffer = NULL;
    }
  }
  if (undoBuffer == NULL)
  {
    undoBuffer = create_bitmap_ex(bitmap_color_depth(mask), mask->w, mask->h);
  }
  blit(mask, undoBuffer, 0, 0, 0, 0, mask->w, mask->h);
}

bool does_undo_buffer_exist()
{
  return (undoBuffer != NULL);
}

void clear_undo_buffer() 
{
  if (does_undo_buffer_exist()) 
  {
    destroy_bitmap(undoBuffer);
    undoBuffer = NULL;
  }
}

void restore_from_undo_buffer(void *roomptr, int maskType)
{
  if (does_undo_buffer_exist())
  {
  	block mask = get_bitmap_for_mask((roomstruct*)roomptr, (RoomAreaMask)maskType);
    blit(undoBuffer, mask, 0, 0, 0, 0, mask->w, mask->h);
  }
}

void setup_greyed_out_palette(int selCol) 
{
    color thisColourOnlyPal[256];

    // The code below makes it so that all the hotspot colours
    // except the selected one are greyed out. It doesn't work
    // in 256-colour games.

    // Blank out the temporary palette, and set a shade of grey
    // for all the hotspot colours
    for (int aa = 0; aa < 256; aa++) {
      int lumin = 0;
      if ((aa < MAX_HOTSPOTS) && (aa > 0))
        lumin = ((MAX_HOTSPOTS - aa) % 30) * 2;
      thisColourOnlyPal[aa].r = lumin;
      thisColourOnlyPal[aa].g = lumin;
      thisColourOnlyPal[aa].b = lumin;
    }
    // Highlight the currently selected area colour
    if (selCol > 0) {
      // if a bright colour, use it
      if ((selCol < 15) && (selCol != 7) && (selCol != 8))
        thisColourOnlyPal[selCol] = palette[selCol];
      else {
        // else, draw in red
        thisColourOnlyPal[selCol].r = 60;
        thisColourOnlyPal[selCol].g = 0;
        thisColourOnlyPal[selCol].b = 0;
      }
    }
    set_palette(thisColourOnlyPal);
}

BITMAP *recycle_bitmap(BITMAP* check, int colDepth, int w, int h)
{
  if ((check != NULL) && (check->w == w) && (check->h == h) &&
      (bitmap_color_depth(check) == colDepth))
  {
    return check;
  }
  if (check != NULL)
    destroy_bitmap(check);

  return create_bitmap_ex(colDepth, w, h);
}

block stretchedSprite = NULL, srcAtRightColDep = NULL;

void draw_area_mask(roomstruct *roomptr, block destination, RoomAreaMask maskType, int selectedArea, int transparency) 
{
	block source = get_bitmap_for_mask(roomptr, maskType);

	if (source == NULL) return;
	
	if (bitmap_color_depth(source) != bitmap_color_depth(destination)) 
	{
    block sourceSprite = source;

    if ((source->w != destination->w) || (source->h != destination->h))
    {
		  stretchedSprite = recycle_bitmap(stretchedSprite, bitmap_color_depth(source), destination->w, destination->h);
		  stretch_blit(source, stretchedSprite, 0, 0, source->w, source->h, 0, 0, stretchedSprite->w, stretchedSprite->h);
      sourceSprite = stretchedSprite;
    }

    if (enable_greyed_out_masks)
    {
      setup_greyed_out_palette(selectedArea);
    }

    if (transparency > 0)
    {
      srcAtRightColDep = recycle_bitmap(srcAtRightColDep, bitmap_color_depth(destination), destination->w, destination->h);
      
      int oldColorConv = get_color_conversion();
      set_color_conversion(oldColorConv | COLORCONV_KEEP_TRANS);

      blit(sourceSprite, srcAtRightColDep, 0, 0, 0, 0, sourceSprite->w, sourceSprite->h);
      set_trans_blender(0, 0, 0, (100 - transparency) + 155);
      draw_trans_sprite(destination, srcAtRightColDep, 0, 0);

      set_color_conversion(oldColorConv);
    }
    else
    {
		  draw_sprite(destination, sourceSprite, 0, 0);
    }

    set_palette(palette);
	}
	else
	{
		Cstretch_sprite(destination, source, 0, 0, destination->w, destination->h);
	}
}

void draw_room_background(void *roomvoidptr, int hdc, int x, int y, int bgnum, float scaleFactor, int maskType, int selectedArea, int maskTransparency) 
{
	roomstruct *roomptr = (roomstruct*)roomvoidptr;

  if (bgnum >= roomptr->num_bscenes)
    return;

  block srcBlock = roomptr->ebscene[bgnum];
  if (srcBlock == NULL)
    return;

	if (drawBuffer != NULL) 
	{
		block depthConverted = create_bitmap_ex(bitmap_color_depth(drawBuffer), srcBlock->w, srcBlock->h);
    if (bitmap_color_depth(srcBlock) == 8)
    {
      select_palette(roomptr->bpalettes[bgnum]);
    }

		blit(srcBlock, depthConverted, 0, 0, 0, 0, srcBlock->w, srcBlock->h);

    if (bitmap_color_depth(srcBlock) == 8)
    {
      unselect_palette();
    }

		draw_area_mask(roomptr, depthConverted, (RoomAreaMask)maskType, selectedArea, maskTransparency);

    int srcX = 0, srcY = 0;
    int srcWidth = srcBlock->w;
    int srcHeight = srcBlock->h;

    if (x < 0)
    {
      srcX = -x / scaleFactor;
      x = 0;
      srcWidth = drawBuffer->w / scaleFactor + 1;
      if (srcX + srcWidth > depthConverted->w)
      {
        srcWidth = depthConverted->w - srcX;
      }
    }
    if (y < 0)
    {
      srcY = -y / scaleFactor;
      y = 0;
      srcHeight = drawBuffer->h / scaleFactor + 1;
      if (srcY + srcHeight > depthConverted->h)
      {
        srcHeight = depthConverted->h - srcY;
      }
    }

		Cstretch_blit(depthConverted, drawBuffer, srcX, srcY, srcWidth, srcHeight, x, y, srcWidth * scaleFactor, srcHeight * scaleFactor);
		destroy_bitmap(depthConverted);
	}
	else {
		drawBlockScaledAt(hdc, srcBlock, x, y, scaleFactor);
	}
	
}

void update_font_sizes() {
  int multiplyWas = wtext_multiply;

  // scale up fonts if necessary
  wtext_multiply = 1;
  if ((thisgame.options[OPT_NOSCALEFNT] == 0) &&
      (thisgame.default_resolution >= 3)) {
    wtext_multiply = 2;
  }

  if (multiplyWas != wtext_multiply) {
    // resolution or Scale Up Fonts has changed, reload at new size
    for (int bb=0;bb<thisgame.numfonts;bb++)
      reload_font (bb);
  }

  if (thisgame.default_resolution >= 3) {
    sxmult = 2;
    symult = 2;
  }
  else {
    sxmult = 1;
    symult = 1;
  }

}

const char* import_sci_font(const char*fnn,int fslot) {
  char wgtfontname[100];
  sprintf(wgtfontname,"agsfnt%d.wfn",fslot);
  FILE*iii=fopen(fnn,"rb");
  if (iii==NULL) {
    return "File not found";
  }
  if (fgetc(iii)!=0x87) {
    fclose(iii);
    return "Not a valid SCI font file";
  }
  fseek(iii,3,SEEK_CUR);
  if (getshort(iii)!=0x80) {
    fclose(iii); 
	  return "Invalid SCI font"; 
  }
  int lineHeight = getshort(iii);
  short theiroffs[0x80];
  fread(theiroffs,2,0x80,iii);
  FILE*ooo=fopen(wgtfontname,"wb");
  fwrite("WGT Font File  ",15,1,ooo);
  putshort(0,ooo);  // will be table address
  short coffsets[0x80];
  char buffer[1000];
  int aa;
  for (aa=0;aa<0x80;aa++) 
  {
    if (theiroffs[aa] < 100)
    {
      fclose(iii);
      fclose(ooo);
      unlink(wgtfontname);
      return "Invalid character found in file";
    }
    fseek(iii,theiroffs[aa]+2,SEEK_SET);
    int wwi=fgetc(iii)-1;
    int hhi=fgetc(iii);
    coffsets[aa]=ftell(ooo);
    putshort(wwi+1,ooo);
    putshort(hhi,ooo);
    if ((wwi<1) | (hhi<1)) continue;
    memset(buffer,0,1000);
    int bytesPerRow = (wwi/8)+1;
    fread(buffer, bytesPerRow, hhi, iii);
    for (int bb=0;bb<hhi;bb++) { 
      int thisoffs = bb * bytesPerRow;
      fwrite(&buffer[thisoffs], bytesPerRow, 1, ooo);
    }
  }
  long tableat=ftell(ooo);
  fwrite(&coffsets[0],2,0x80,ooo);
  fclose(ooo); ooo=fopen(wgtfontname,"r+b");
  fseek(ooo,15,SEEK_SET); putshort(tableat,ooo); 
  fclose(ooo);
  fclose(iii);
  wfreefont(fslot);
  if (!wloadfont_size(fslot, 0))
  {
    return "Unable to load converted WFN file";
  }
  return NULL;
}


#define FONTGRIDSIZE 18*blockSize
void drawFontAt (int hdc, int fontnum, int x,int y) {
  
  if (fontnum >= thisgame.numfonts) 
  {
	  return;
  }

  update_font_sizes();

  int doubleSize = (thisgame.default_resolution < 3) ? 2 : 1;
  int blockSize = (thisgame.default_resolution < 3) ? 1 : 2;
  antiAliasFonts = thisgame.options[OPT_ANTIALIASFONTS];

  // we can't antialias font because changing col dep to 16 here causes
  // it to crash ... why?
  block tempblock = create_bitmap_ex(8, FONTGRIDSIZE*10, FONTGRIDSIZE*10);
  clear_to_color(tempblock, 0);
  block abufwas = abuf;
  abuf = tempblock;
  wtextcolor(15);
  for (int aa=0;aa<96;aa++)
    wgtprintf(5+(aa%10)*FONTGRIDSIZE,5+(aa/10)*FONTGRIDSIZE, fontnum, "%c",aa+32);
  abuf = abufwas;

  if (doubleSize > 1) 
    drawBlockDoubleAt(hdc, tempblock, x, y);
  else
    drawBlock((HDC)hdc, tempblock, x, y);
   
  wfreeblock(tempblock);
}

void proportionalDraw (int newwid, int sprnum, int*newx, int*newy) {
  int newhit = newwid;

  int newsizx=newwid,newsizy=newhit;
  int twid=get_sprite(sprnum)->w,thit = get_sprite(sprnum)->h;
  if ((twid < newwid/2) && (thit < newhit/2)) {
    newsizx = twid * 2;
    newsizy = thit * 2;
  }
  else {
    if (twid >= thit) newsizy=(int)((float)thit/((float)twid/(float)newwid));
    else if (twid < thit) newsizx=(int)((float)twid/((float)thit/(float)newhit));
  }
  newx[0] = newsizx;
  newy[0] = newsizy;
}

static void doDrawViewLoop (int hdc, int numFrames, ViewFrame *frames, int x, int y, int size, int cursel) {
  int wtoDraw = size * numFrames;
  
  if ((numFrames > 0) && (frames[numFrames-1].pic == -1))
    wtoDraw -= size;

  block todraw = create_bitmap_ex (thisgame.color_depth*8, wtoDraw, size);
  clear_to_color (todraw, bitmap_mask_color (todraw));
  int neww, newh;
  for (int i = 0; i < numFrames; i++) {
    // don't draw the Go-To-Next-Frame jibble
    if (frames[i].pic == -1)
      break;
    // work out the dimensions to stretch to
    proportionalDraw (size, frames[i].pic, &neww, &newh);
    block toblt = get_sprite(frames[i].pic);
    bool freeBlock = false;
    if (bitmap_color_depth (toblt) != bitmap_color_depth (todraw)) {
      // 256-col sprite in hi-col game, we need to copy first
      block oldBlt = toblt;
      toblt = create_bitmap_ex (bitmap_color_depth (todraw), toblt->w, toblt->h);
      blit (oldBlt, toblt, 0, 0, 0, 0, oldBlt->w, oldBlt->h);
      freeBlock = true;
    }
    block flipped = NULL;
    if (frames[i].flags & VFLG_FLIPSPRITE) {
      // mirror the sprite
      flipped = create_bitmap_ex (bitmap_color_depth (todraw), toblt->w, toblt->h);
      clear_to_color (flipped, bitmap_mask_color (flipped));
      draw_sprite_h_flip (flipped, toblt, 0, 0);
      if (freeBlock)
        wfreeblock(toblt);
      toblt = flipped;
      freeBlock = true;
    }
    //stretch_sprite(toblt, todraw, 0, 0, toblt->w, toblt->h, size*i, 0, neww, newh);
	Cstretch_sprite(todraw, toblt, size*i, 0, neww, newh);
    if (freeBlock)
      wfreeblock (toblt);
    if (i < numFrames-1) {
      int linecol = makecol_depth(thisgame.color_depth * 8, 0, 64, 200);
      if (thisgame.color_depth == 1)
        linecol = 12;

      // Draw dividing line
      line (todraw, size*(i+1) - 1, 0, size*(i+1) - 1, size-1, linecol);
    }
    if (i == cursel) {
      // Selected item
      int linecol = makecol_depth(thisgame.color_depth * 8, 255, 255,255);
      if (thisgame.color_depth == 1)
        linecol = 14;
      
      rect (todraw, size * i, 0, size * (i+1) - 1, size-1, linecol);
    }
  }
  drawBlock ((HDC)hdc, todraw, x, y);
  wfreeblock(todraw);
}

int get_adjusted_spritewidth(int spr) {
  block tsp = get_sprite(spr);
  if (tsp == NULL) return 0;

  int retval = tsp->w;

  if (thisgame.spriteflags[spr] & SPF_640x400) {
    if (sxmult == 1)
      retval /= 2;
  }
  else {
    if (sxmult == 2)
      retval *= 2;
  }
  return retval;
}

int get_adjusted_spriteheight(int spr) {
  block tsp = get_sprite(spr);
  if (tsp == NULL) return 0;

  int retval = tsp->h;

  if (thisgame.spriteflags[spr] & SPF_640x400) {
    if (symult == 1)
      retval /= 2;
  }
  else {
    if (symult == 2)
      retval *= 2;
  }
  return retval;
}

void drawBlockOfColour(int hdc, int x,int y, int width, int height, int colNum)
{
	__my_setcolor(&colNum, colNum);
  /*if (thisgame.color_depth > 2) {
    // convert to 24-bit colour
    int red = ((colNum >> 11) & 0x1f) * 8;
    int grn = ((colNum >> 5) & 0x3f) * 4;
    int blu = (colNum & 0x1f) * 8;
    colNum = (red << _rgb_r_shift_32) | (grn << _rgb_g_shift_32) | (blu << _rgb_b_shift_32);
  }*/

  block palbmp = create_bitmap_ex(thisgame.color_depth * 8, width, height);
  clear_to_color (palbmp, colNum);
  drawBlockScaledAt(hdc, palbmp, x, y, 1);
  wfreeblock(palbmp);
}

void NewInteractionCommand::remove () 
{
}

void new_font () {
  wloadfont_size(thisgame.numfonts, 0);
  thisgame.fontflags[thisgame.numfonts] = 0;
  thisgame.fontoutline[thisgame.numfonts] = -1;
  thisgame.numfonts++;
}

bool initialize_native()
{
  set_uformat(U_ASCII);  // required to stop ALFONT screwing up text
	install_allegro(SYSTEM_NONE, &errno, atexit);
	//set_gdi_color_format();
	palette = &thisgame.defpal[0];
	thisgame.color_depth = 2;
	abuf = create_bitmap_ex(32, 10, 10);
	thisgame.numfonts = 0;
	new_font();

	spriteset.reset();
	if (spriteset.initFile(sprsetname))
	  return false;
	spriteset.maxCacheSize = 100000000;  // 100 mb cache

	if (!Scintilla_RegisterClasses (GetModuleHandle(NULL)))
      return false;

  init_font_renderer();

	return true;
}

void shutdown_native()
{
  shutdown_font_renderer();
	allegro_exit();
}

void drawBlockScaledAt (int hdc, block todraw ,int x, int y, int scaleFactor) {
  if (bitmap_color_depth (todraw) == 8)
    set_palette_to_hdc ((HDC)hdc, palette);

  stretch_blit_to_hdc (todraw, (HDC)hdc, 0,0,todraw->w,todraw->h,
    x,y,todraw->w * scaleFactor, todraw->h * scaleFactor);
}

void drawSprite(int hdc, int x, int y, int spriteNum, bool flipImage) {
	int scaleFactor = ((thisgame.spriteflags[spriteNum] & SPF_640x400) != 0) ? 1 : 2;
	block theSprite = get_sprite(spriteNum);

  if (theSprite == NULL)
    return;

	if (flipImage) {
		block flipped = create_bitmap_ex (bitmap_color_depth(theSprite), theSprite->w, theSprite->h);
		clear_to_color (flipped, bitmap_mask_color (flipped));
		draw_sprite_h_flip (flipped, theSprite, 0, 0);
		drawBlockScaledAt(hdc, flipped, x, y, scaleFactor);
		wfreeblock(flipped);
	}
	else 
	{
		drawBlockScaledAt(hdc, theSprite, x, y, scaleFactor);
	}
}

void drawSpriteStretch(int hdc, int x, int y, int width, int height, int spriteNum) {
  block todraw = get_sprite(spriteNum);
  block tempBlock = NULL;
	
  if (bitmap_color_depth (todraw) == 8)
    set_palette_to_hdc ((HDC)hdc, palette);

  int hdcBpp = GetDeviceCaps((HDC)hdc, BITSPIXEL);
  if (hdcBpp != bitmap_color_depth(todraw))
  {
	  tempBlock = create_bitmap_ex(hdcBpp, todraw->w, todraw->h);
	  blit(todraw, tempBlock, 0, 0, 0, 0, todraw->w, todraw->h);
	  todraw = tempBlock;
  }

  stretch_blit_to_hdc (todraw, (HDC)hdc, 0,0,todraw->w,todraw->h, x,y, width, height);

  if (tempBlock != NULL)
  {
	  destroy_bitmap(tempBlock);
  }
}

void drawGUIAt (int hdc, int x,int y,int x1,int y1,int x2,int y2, int scaleFactor) {

  if ((tempgui.wid < 1) || (tempgui.hit < 1))
    return;

  //update_font_sizes();

  if (scaleFactor == 1) {
    dsc_want_hires = 1;
  }

  block tempblock = create_bitmap_ex (thisgame.color_depth*8, tempgui.wid, tempgui.hit);
  clear_to_color(tempblock, bitmap_mask_color (tempblock));
  block abufWas = abuf;
  abuf = tempblock;

  tempgui.draw_at (0, 0);

  dsc_want_hires = 0;

  if (x1 >= 0) {
    rect (abuf, x1, y1, x2, y2, 14);
  }
  abuf = abufWas;

  drawBlockScaledAt (hdc, tempblock, x, y, scaleFactor);
  //drawBlockDoubleAt (hdc, tempblock, x, y);
  wfreeblock(tempblock);
}

#define SIMP_INDEX0  0
#define SIMP_TOPLEFT 1
#define SIMP_BOTLEFT 2
#define SIMP_TOPRIGHT 3
#define SIMP_BOTRIGHT 4
#define SIMP_LEAVEALONE 5
#define SIMP_NONE     6

void sort_out_transparency(block toimp, int sprite_import_method, color*itspal, bool useBgSlots, int importedColourDepth) 
{
  if (sprite_import_method == SIMP_LEAVEALONE)
    return;

  int uu,tt;
  wsetpalette(0,255,palette);
  int transcol=bitmap_mask_color(toimp);
  // NOTE: This takes the pixel from the corner of the overall import
  // graphic, NOT just the image to be imported
  if (sprite_import_method == SIMP_TOPLEFT)
    transcol=getpixel(toimp,0,0);
  else if (sprite_import_method==SIMP_BOTLEFT)
    transcol=getpixel(toimp,0,(toimp->h)-1);
  else if (sprite_import_method == SIMP_TOPRIGHT)
    transcol = getpixel(toimp, (toimp->w)-1, 0);
  else if (sprite_import_method == SIMP_BOTRIGHT)
    transcol = getpixel(toimp, (toimp->w)-1, (toimp->h)-1);

  if (sprite_import_method == SIMP_NONE)
  {
    // remove all transparency pixels (change them to
    // a close non-trnasparent colour)
    int changeTransparencyTo;
    if (transcol == 0)
      changeTransparencyTo = 16;
    else
      changeTransparencyTo = transcol - 1;

    for (tt=0;tt<toimp->w;tt++) {
      for (uu=0;uu<toimp->h;uu++) {
        if (getpixel(toimp,tt,uu) == transcol)
          putpixel(toimp,tt,uu, changeTransparencyTo);
      }
    }
  }
  else
  {
	  int bitmapMaskColor = bitmap_mask_color(toimp);
    int replaceWithCol = 16;
	  if (bitmap_color_depth(toimp) > 8)
	  {
      if (importedColourDepth == 8)
        replaceWithCol = makecol_depth(bitmap_color_depth(toimp), itspal[0].r * 4, itspal[0].g * 4, itspal[0].b * 4);
      else
		    replaceWithCol = 0;
	  }
    // swap all transparent pixels with index 0 pixels
    for (tt=0;tt<toimp->w;tt++) {
      for (uu=0;uu<toimp->h;uu++) {
        if (getpixel(toimp,tt,uu)==transcol)
          putpixel(toimp,tt,uu, bitmapMaskColor);
        else if (getpixel(toimp,tt,uu) == bitmapMaskColor)
          putpixel(toimp,tt,uu, replaceWithCol);
      }
    }
  }

  if ((thisgame.color_depth == 1) && (itspal != NULL)) { 
    // 256-colour mode only
    if (transcol!=0)
      itspal[transcol] = itspal[0];
    wsetrgb(0,0,0,0,itspal); // set index 0 to black
    __wremap_keep_transparent = 1;
    color oldpale[256];
    for (uu=0;uu<255;uu++) {
      if (useBgSlots)  //  use background scene palette
        oldpale[uu]=palette[uu];
      else if (thisgame.paluses[uu]==PAL_BACKGROUND)
        wsetrgb(uu,0,0,0,oldpale);
      else 
        oldpale[uu]=palette[uu];
    }
    wremap(itspal,toimp,oldpale); 
    wsetpalette(0,255,palette);
  }
  else if (bitmap_color_depth(toimp) == 8) {  // hi-colour game
    set_palette(itspal);
  }
}

void update_abuf_coldepth() {
  wfreeblock(abuf);
  abuf = create_bitmap_ex (thisgame.color_depth * 8, 10, 10);
}

bool reload_font(int curFont)
{
  wfreefont(curFont);

  int fsize = thisgame.fontflags[curFont] & FFLG_SIZEMASK;
  // if the font is designed for 640x400, half it
  if (thisgame.options[OPT_NOSCALEFNT]) {
    if (thisgame.default_resolution <= 2)
      fsize /= 2;
  }
  else if (thisgame.default_resolution >= 3) {
    // designed for 320x200, double it up
    fsize *= 2;
  }
  return wloadfont_size(curFont, fsize);
}

void load_script_modules_compiled(FILE *inn) {

  numScriptModules = getw(inn);
  scModules = (ScriptModule*)realloc(scModules, sizeof(ScriptModule) * numScriptModules);
  for (int i = 0; i < numScriptModules; i++) {
    scModules[i].init();
    scModules[i].compiled = fread_script(inn);
  }

}

void read_dialogs(FILE*iii, int filever, bool encrypted) {
  int bb;
  dialog = (DialogTopic*)malloc(sizeof(DialogTopic) * thisgame.numdialog);
  fread(&dialog[0],sizeof(DialogTopic),thisgame.numdialog,iii);
  for (bb=0;bb<thisgame.numdialog;bb++) {
    if (dialog[bb].optionscripts!=NULL) {
      dialog[bb].optionscripts=(unsigned char*)malloc(dialog[bb].codesize+10);
      fread(&dialog[bb].optionscripts[0],dialog[bb].codesize,1,iii);
    }
    int lenof=getw(iii);
    if (lenof<=1) { fgetc(iii);
      dlgscript[bb]=NULL;
      continue;
    }
    // add a large buffer because it will get added to if another option is added
    dlgscript[bb]=(char*)malloc(lenof + 20000);
    fread(dlgscript[bb],lenof,1,iii);
    if (encrypted)
      decrypt_text(dlgscript[bb]);
  }
  char stringbuffer[1000];
  for (bb=0;bb<thisgame.numdlgmessage;bb++) {
    if ((filever >= 26) && (encrypted))
      read_string_decrypt(iii, stringbuffer);
    else
      fgetstring(stringbuffer, iii);

    // don't actually do anything with the dlgmessage (it's an obsolete compiled artefact)
  }
}

bool reset_sprite_file() {
  spriteset.reset();
  if (spriteset.initFile(sprsetname))
    return false;
  spriteset.maxCacheSize = 100000000;  // 100 mb cache
  return true;
}

#define MAX_PLUGINS 40
struct PluginData 
{
	char filename[50];
	char data[SAVEBUFFERSIZE];
	int dataSize;
};
PluginData thisgamePlugins[MAX_PLUGINS];
int numThisgamePlugins = 0;

void write_plugins_to_disk (FILE *ooo) {
  int a;
  // version of plugin saving format
  putw (1, ooo);
  putw (numThisgamePlugins, ooo);
  
  for (a = 0; a < numThisgamePlugins; a++) {
      fputstring(thisgamePlugins[a].filename, ooo);
      
      int savesize = thisgamePlugins[a].dataSize;
      
      if ((savesize > SAVEBUFFERSIZE) || (savesize < 0)) {
		  MessageBox(NULL, "Plugin tried to write too much data to game file.", "", MB_OK);
        savesize = 0;
      }

      putw (savesize, ooo);
      if (savesize > 0)
        fwrite (&thisgamePlugins[a].data[0], savesize, 1, ooo);
  }
}

const char * read_plugins_from_disk (FILE *iii) {
  if (getw(iii) != 1) {
    return "ERROR: unable to load game, invalid version of plugin data";
  }

  numThisgamePlugins = getw(iii);

  for (int a = 0; a < numThisgamePlugins; a++) {
    // read the plugin name
    fgetstring (thisgamePlugins[a].filename, iii);
    int datasize = getw(iii);
    if (datasize > SAVEBUFFERSIZE) {
      return "Invalid plugin save data format, plugin data is lost";
    }
    // we don't care if it's an editor-only plugin or not
    if (thisgamePlugins[a].filename[strlen(thisgamePlugins[a].filename) - 1] == '!')
		thisgamePlugins[a].filename[strlen(thisgamePlugins[a].filename) - 1] = 0;

	thisgamePlugins[a].dataSize = datasize;
	if (datasize > 0)
	  fread (thisgamePlugins[a].data, datasize, 1, iii);
  }
  return NULL;
}

void allocate_memory_for_views(int viewCount)
{
  numNewViews = 0;
	oldViews = (ViewStruct272*)calloc(sizeof(ViewStruct272) * viewCount, 1);
  newViews = (ViewStruct*)calloc(sizeof(ViewStruct) * viewCount, 1);
  thisgame.viewNames = (char**)malloc(sizeof(char*) * viewCount);
  thisgame.viewNames[0] = (char*)calloc(MAXVIEWNAMELENGTH * viewCount, 1);

  for (int i = 1; i < viewCount; i++)
  {
    thisgame.viewNames[i] = thisgame.viewNames[0] + (MAXVIEWNAMELENGTH * i);
  }
}

const char *load_dta_file_into_thisgame(const char *fileName)
{
  int bb;
  FILE*iii=fopen(fileName, "rb");
  if (iii == NULL)
    return "Unable to open file";

  char buffer[40];
  fread(buffer,30,1,iii); 
  buffer[30]=0;
  if (strcmp(buffer,game_file_sig)!=0) {
    fclose(iii);
    return "File contains invalid data and is not an AGS game.";
  }
  int filever = _getw(iii);
  if (filever != 32) 
  {
	  fclose(iii);
	  return "This game was saved by an old version of AGS. This version of the editor can only import games saved with AGS 2.72.";
  }

  // skip required engine version
  int stlen = _getw(iii);
  fseek(iii, stlen, SEEK_CUR);

  fread(&thisgame, sizeof (GameSetupStructBase), 1, iii);
  fread(&thisgame.fontflags[0], 1, thisgame.numfonts, iii);
  fread(&thisgame.fontoutline[0], 1, thisgame.numfonts, iii);

  int numSprites = _getw(iii);
  memset(&thisgame.spriteflags[0], 0, MAX_SPRITES);
  fread(&thisgame.spriteflags[0], 1, numSprites, iii);
  fread(&thisgame.invinfo[0], sizeof(InventoryItemInfo), thisgame.numinvitems, iii);
  fread(&thisgame.mcurs[0], sizeof(MouseCursor), thisgame.numcursors, iii);

  thisgame.intrChar = (NewInteraction**)calloc(thisgame.numcharacters, sizeof(NewInteraction*));
  for (bb = 0; bb < thisgame.numcharacters; bb++) {
    thisgame.intrChar[bb] = deserialize_new_interaction (iii);
  }
  for (bb = 0; bb < thisgame.numinvitems; bb++) {
    delete thisgame.intrInv[bb];
    thisgame.intrInv[bb] = deserialize_new_interaction (iii);
  }

  numGlobalVars = getw(iii);
  fread (&globalvars[0], sizeof (InteractionVariable), numGlobalVars, iii);

  if (thisgame.dict != NULL) {
    thisgame.dict = (WordsDictionary*)malloc(sizeof(WordsDictionary));
    read_dictionary (thisgame.dict, iii);
  }

  thisgame.globalscript = NULL;

  if (thisgame.compiled_script != NULL)
    thisgame.compiled_script = fread_script(iii);

  load_script_modules_compiled(iii);

  allocate_memory_for_views(thisgame.numviews);
  fread (&oldViews[0], sizeof(ViewStruct272), thisgame.numviews, iii);

  thisgame.chars = (CharacterInfo*)calloc(sizeof(CharacterInfo) * thisgame.numcharacters, 1);
  fread(&thisgame.chars[0],sizeof(CharacterInfo),thisgame.numcharacters,iii);

  fread (&thisgame.lipSyncFrameLetters[0][0], 20, 50, iii);

  for (bb=0;bb<MAXGLOBALMES;bb++) {
    if (thisgame.messages[bb]==NULL) continue;
    thisgame.messages[bb]=(char*)malloc(500);
    read_string_decrypt(iii, thisgame.messages[bb]);
  }

  read_dialogs(iii, filever, true);
  read_gui(iii,&guis[0],&thisgame, &guis);
  const char *pluginError = read_plugins_from_disk (iii);
  if (pluginError != NULL) return pluginError;

  thisgame.charProps = (CustomProperties*)calloc(thisgame.numcharacters, sizeof(CustomProperties));

  for (bb = 0; bb < thisgame.numcharacters; bb++)
    thisgame.charProps[bb].reset();
  for (bb = 0; bb < MAX_INV; bb++)
    thisgame.invProps[bb].reset();

  if (thisgame.propSchema.UnSerialize (iii))
    return "unable to deserialize prop schema";

  int errors = 0;

  for (bb = 0; bb < thisgame.numcharacters; bb++)
    errors += thisgame.charProps[bb].UnSerialize (iii);
  for (bb = 0; bb < thisgame.numinvitems; bb++)
    errors += thisgame.invProps[bb].UnSerialize (iii);

  if (errors > 0)
    return "errors encountered reading custom props";

  for (bb = 0; bb < thisgame.numviews; bb++)
    fgetstring_limit(thisgame.viewNames[bb], iii, MAXVIEWNAMELENGTH);

  for (bb = 0; bb < thisgame.numinvitems; bb++)
    fgetstring_limit(thisgame.invScriptNames[bb], iii, MAX_SCRIPT_NAME_LEN);

  for (bb = 0; bb < thisgame.numdialog; bb++)
    fgetstring_limit(thisgame.dialogScriptNames[bb], iii, MAX_SCRIPT_NAME_LEN);

  fclose(iii);

  for (bb = 0; bb < thisgame.numgui; bb++)
  {
	  guis[bb].rebuild_array();
  }

  // reset colour 0, it's possible for it to get corrupted
  palette[0].r = 0;
  palette[0].g = 0;
  palette[0].b = 0;
  wsetpalette(0,255,palette);

  if (!reset_sprite_file())
    return "The sprite file could not be loaded. Ensure that all your game files are intact and not corrupt. The game may require a newer version of AGS.";

  for (bb=0;bb<MAX_FONTS;bb++) {
    wfreefont(bb);
  }
  for (bb=0;bb<thisgame.numfonts;bb++) {
    reload_font (bb);
  }

  update_abuf_coldepth();
  spritesModified = false;

  thisgame.filever = filever;
  return NULL;
}

void free_script_module(int index) {
  free(scModules[index].name);
  free(scModules[index].author);
  free(scModules[index].version);
  free(scModules[index].description);
  free(scModules[index].script);
  free(scModules[index].scriptHeader);
  if (scModules[index].compiled != NULL)
    ccFreeScript(scModules[index].compiled);
}

void free_script_modules() {
  for (int i = 0; i < numScriptModules; i++)
    free_script_module(i);

  numScriptModules = 0;
}

void free_old_game_data()
{
  int bb;
  for (bb=0;bb<MAXGLOBALMES;bb++) {
    if (thisgame.messages[bb] != NULL)
      free(thisgame.messages[bb]);
  }
  for (bb = 0; bb < thisgame.numdialog; bb++) 
  {
	  if (dialog[bb].optionscripts != NULL)
		  free(dialog[bb].optionscripts);
  }
  if (thisgame.charProps != NULL)
  {
    for (bb = 0; bb < thisgame.numcharacters; bb++)
      thisgame.charProps[bb].reset();
    free(thisgame.charProps);
    thisgame.charProps = NULL;
  }
  if (thisgame.intrChar != NULL)
  {
    for (bb = 0; bb < thisgame.numcharacters; bb++)
      delete thisgame.intrChar[bb];
    
    free(thisgame.intrChar);
    thisgame.intrChar = NULL;
  }
  for (bb = 0; bb < numNewViews; bb++)
  {
    for (int cc = 0; cc < newViews[bb].numLoops; cc++)
    {
      newViews[bb].loops[cc].Dispose();
    }
    newViews[bb].Dispose();
  }
  free(thisgame.viewNames[0]);
  free(thisgame.viewNames);
  free(oldViews);
  free(newViews);
  free(guis);
  free(thisgame.chars);
  thisgame.dict->free_memory();
  free(thisgame.dict);
  free(dialog);
  free_script_modules();
}

// remap the scene, from its current palette oldpale to palette
void remap_background (block scene, color *oldpale, color*palette, int exactPal) {
  int a;  

  if (exactPal) {
    // exact palette import (for doing palette effects, don't change)
    for (a=0;a<256;a++) 
    {
      if (thisgame.paluses[a] == PAL_BACKGROUND)
      {
        palette[a] = oldpale[a];
      }
    }
    return;
  }

  // find how many slots there are reserved for backgrounds
  int numbgslots=0;
  for (a=0;a<256;a++) { oldpale[a].filler=0;
    if (thisgame.paluses[a]!=PAL_GAMEWIDE) numbgslots++;
  }
  // find which colours from the image palette are actually used
  int imgpalcnt[256],numimgclr=0;
  memset(&imgpalcnt[0],0,sizeof(int)*256);
  if (is_linear_bitmap(scene)==0)
    quit("mem bitmap non-linear?");

  for (a=0;a<(scene->w) * (scene->h);a++) {
    imgpalcnt[scene->line[0][a]]++;
  }
  for (a=0;a<256;a++) {
    if (imgpalcnt[a]>0) numimgclr++;
  }
  // count up the number of unique colours in the image
  int numclr=0,bb;
  color tpal[256];
  for (a=0;a<256;a++) {
    if (thisgame.paluses[a]==PAL_BACKGROUND)
      wsetrgb(a,0,0,0,palette);  // black out the bg slots before starting
    if ((oldpale[a].r==0) & (oldpale[a].g==0) & (oldpale[a].b==0)) {
      imgpalcnt[a]=0;
      continue;
    }
    for (bb=0;bb<numclr;bb++) {
      if ((oldpale[a].r==tpal[bb].r) &
        (oldpale[a].g==tpal[bb].g) &
        (oldpale[a].b==tpal[bb].b)) bb=1000;
    }
    if (bb>300) { 
      imgpalcnt[a]=0;
      continue;
    }
    if (imgpalcnt[a]==0)
      continue;
    tpal[numclr]=oldpale[a];
    numclr++;
  }
  if (numclr>numbgslots) {
    MessageBox(NULL, "WARNING: This image uses more colours than are allocated to backgrounds. Some colours will be lost.", "Warning", MB_OK);
  }

  // fill the background slots in the palette with the colours
  int palslt=255;  // start from end of palette and work backwards
  for (a=0;a<numclr;a++) {
    while (thisgame.paluses[palslt]!=PAL_BACKGROUND) {
      palslt--;
      if (palslt<0) break;
    }
    if (palslt<0) break;
    palette[palslt]=tpal[a];
    palslt--;
    if (palslt<0) break;
  }
  // blank out the sprite colours, then remap the picture
  for (a=0;a<256;a++) {
    if (thisgame.paluses[a]==PAL_GAMEWIDE) {
      tpal[a].r=0;
      tpal[a].g=0; tpal[a].b=0; 
    }
    else tpal[a]=palette[a];
  }
  wremapall(oldpale,scene,tpal); //palette);
}

void validate_mask(block toValidate, const char *name, int maxColour) {
  if ((toValidate == NULL) || (bitmap_color_depth(toValidate) != 8) ||
      (!is_memory_bitmap(toValidate))) {
    quit("Invalid mask passed to validate_mask");
    return;
  }

  bool errFound = false;
  int xx, yy;
  for (yy = 0; yy < toValidate->h; yy++) {
    for (xx = 0; xx < toValidate->w; xx++) {
      if (toValidate->line[yy][xx] >= maxColour) {
        errFound = true;
        toValidate->line[yy][xx] = 0;
      }
    }
  }

  if (errFound) {
	char errorBuf[1000];
    sprintf(errorBuf, "Invalid colours were found in the %s mask. They have now been removed."
       "\n\nWhen drawing a mask in an external paint package, you need to make "
       "sure that the image is set as 256-colour (Indexed Palette), and that "
       "you use the first 16 colours in the palette for drawing your areas. Palette "
       "entry 0 corresponds to No Area, palette index 1 corresponds to area 1, and "
       "so forth.", name);
	MessageBox(NULL, errorBuf, "Mask Error", MB_OK);
    roomModified = true;
  }
}

void copy_room_palette_to_global_palette()
{
  for (int ww = 0; ww < 256; ww++) 
  {
    if (thisgame.paluses[ww] == PAL_BACKGROUND)
    {
      thisroom.pal[ww] = thisroom.bpalettes[0][ww];
      palette[ww] = thisroom.bpalettes[0][ww];
    }
  }
}

void copy_global_palette_to_room_palette()
{
  for (int ww = 0; ww < 256; ww++) 
  {
    if (thisgame.paluses[ww] != PAL_BACKGROUND)
      thisroom.bpalettes[0][ww] = palette[ww];
  }
}

const char* load_room_file(const char*rtlo) {

  load_room((char*)rtlo, &thisroom, (thisgame.default_resolution > 2));

  if (thisroom.wasversion < 17) 
  {
	  return "This room was saved with an old version of the editor and cannot be opened. Use AGS 2.72 to upgrade this room file.";
  }

  //thisroom.numhotspots = MAX_HOTSPOTS;

  // Allocate enough memory to add extra variables
  InteractionVariable *ivv = (InteractionVariable*)malloc (sizeof(InteractionVariable) * MAX_GLOBAL_VARIABLES);
  if (thisroom.numLocalVars > 0) {
    memcpy (ivv, thisroom.localvars, sizeof(InteractionVariable) * thisroom.numLocalVars);
    free (thisroom.localvars);
  }
  thisroom.localvars = ivv;

  // Update room palette with gamewide colours
  copy_global_palette_to_room_palette();
  // Update current global palette with room background colours
  copy_room_palette_to_global_palette();
  int ww;
  for (ww = 0; ww < thisroom.numsprs; ww++) {
    // change invalid objects to blue cup
    if (spriteset[thisroom.sprs[ww].sprnum] == NULL)
      thisroom.sprs[ww].sprnum = 0;
  }
  // Fix hi-color screens
  for (ww = 0; ww < thisroom.num_bscenes; ww++)
    fix_block (thisroom.ebscene[ww]);

  if ((thisroom.resolution > 1) && (thisroom.object->w < thisroom.width)) {
    // 640x400 room with 320x200-res walkbehind
    // resize it up to 640x400-res
    int oldw = thisroom.object->w, oldh=thisroom.object->h;
    block tempb = create_bitmap_ex(bitmap_color_depth(thisroom.object), thisroom.width, thisroom.height);
    clear(tempb);
    stretch_blit(thisroom.object,tempb,0,0,oldw,oldh,0,0,tempb->w,tempb->h);
    destroy_bitmap(thisroom.object); 
    thisroom.object = tempb;
  }

  wsetpalette(0,255,palette);
  
  if ((bitmap_color_depth (thisroom.ebscene[0]) > 8) &&
      (thisgame.color_depth == 1))
    MessageBox(NULL,"WARNING: This room is hi-color, but your game is currently 256-colour. You will not be able to use this room in this game. Also, the room background will not look right in the editor.", "Colour depth warning", MB_OK);

  roomModified = false;

  validate_mask(thisroom.lookat, "hotspot", MAX_HOTSPOTS);
  validate_mask(thisroom.object, "walk-behind", MAX_WALK_AREAS + 1);
  validate_mask(thisroom.walls, "walkable area", MAX_WALK_AREAS + 1);
  validate_mask(thisroom.regions, "regions", MAX_REGIONS);
  return NULL;
}

void calculate_walkable_areas () {
  int ww, thispix;

  for (ww = 0; ww <= MAX_WALK_AREAS; ww++) {
    thisroom.walk_area_top[ww] = thisroom.height;
    thisroom.walk_area_bottom[ww] = 0;
  }
  for (ww = 0; ww < thisroom.walls->w; ww++) {
    for (int qq = 0; qq < thisroom.walls->h; qq++) {
      thispix = _getpixel (thisroom.walls, ww, qq);
      if (thispix > MAX_WALK_AREAS)
        continue;
      if (thisroom.walk_area_top[thispix] > qq)
        thisroom.walk_area_top[thispix] = qq;
      if (thisroom.walk_area_bottom[thispix] < qq)
        thisroom.walk_area_bottom[thispix] = qq;
    }
  }

}

// Note: we don't use GETW or PUTW for compatibility between 16-bit and
// 32-bit versions; hence all the FREAD/FWRITEs
void save_room(const char *files, roomstruct rstruc) {
  int               f;
  long              xoff, tesl;
  FILE              *opty;
  room_file_header  rfh;

  if (rstruc.wasversion < ROOM_FILE_VERSION)
    quit("save_room: can no longer save old format rooms");

  if (rstruc.wasversion < 9) {
    for (f = 0; f < 11; f++)
      rstruc.password[f] -= 60;
  }
  else
    for (f = 0; f < 11; f++)
      rstruc.password[f] -= passwencstring[f];

  opty = ci_fopen(const_cast<char*>(files), "wb");
  if (opty == NULL)
    quit("save_room: unable to open room file for writing.");

  rfh.version = rstruc.wasversion; //ROOM_FILE_VERSION;
  fwrite(&rfh,sizeof(room_file_header),1,opty);

  if (rfh.version >= 5) {
    long blsii = 0;

    fputc(BLOCKTYPE_MAIN, opty);
    fwrite(&blsii, 4, 1, opty);
  }

  putw(rstruc.bytes_per_pixel, opty);  // colour depth bytes per pixel
  fwrite(&rstruc.numobj, 2, 1, opty);
  fwrite(&rstruc.objyval[0], 2, rstruc.numobj, opty);

  fwrite(&rstruc.numhotspots, sizeof(int), 1, opty);
  fwrite(&rstruc.hswalkto[0], sizeof(_Point), rstruc.numhotspots, opty);
  for (f = 0; f < rstruc.numhotspots; f++)
  {
	  fputstring(rstruc.hotspotnames[f], opty);
  }

  if (rfh.version >= 24)
    fwrite(&rstruc.hotspotScriptNames[0], MAX_SCRIPT_NAME_LEN, rstruc.numhotspots, opty);

  fwrite(&rstruc.numwalkareas, 4, 1, opty);
  fwrite(&rstruc.wallpoints[0], sizeof(PolyPoints), rstruc.numwalkareas, opty);

  fwrite(&rstruc.top, 2, 1, opty);
  fwrite(&rstruc.bottom, 2, 1, opty);
  fwrite(&rstruc.left, 2, 1, opty);
  fwrite(&rstruc.right, 2, 1, opty);
  fwrite(&rstruc.numsprs, 2, 1, opty);
  fwrite(&rstruc.sprs[0], sizeof(sprstruc), rstruc.numsprs, opty);

  putw (rstruc.numLocalVars, opty);
  if (rstruc.numLocalVars > 0) 
    fwrite (&rstruc.localvars[0], sizeof(InteractionVariable), rstruc.numLocalVars, opty);
/*
  for (f = 0; f < rstruc.numhotspots; f++)
    serialize_new_interaction (rstruc.intrHotspot[f], opty);
  for (f = 0; f < rstruc.numsprs; f++)
    serialize_new_interaction (rstruc.intrObject[f], opty);
  serialize_new_interaction (rstruc.intrRoom, opty);
*/
  putw (MAX_REGIONS, opty);
  /*
  for (f = 0; f < MAX_REGIONS; f++)
    serialize_new_interaction (rstruc.intrRegion[f], opty);
	*/
  serialize_room_interactions(opty);

  fwrite(&rstruc.objbaseline[0], sizeof(int), rstruc.numsprs, opty);
  fwrite(&rstruc.width, 2, 1, opty);
  fwrite(&rstruc.height, 2, 1, opty);

  if (rfh.version >= 23)
    fwrite(&rstruc.objectFlags[0], sizeof(short), rstruc.numsprs, opty);

  if (rfh.version >= 11)
    fwrite(&rstruc.resolution,2,1,opty);

  // write the zoom and light levels
  putw (MAX_WALK_AREAS + 1, opty);
  fwrite(&rstruc.walk_area_zoom[0], sizeof(short), MAX_WALK_AREAS + 1, opty);
  fwrite(&rstruc.walk_area_light[0], sizeof(short), MAX_WALK_AREAS + 1, opty);
  fwrite(&rstruc.walk_area_zoom2[0], sizeof(short), MAX_WALK_AREAS + 1, opty);
  fwrite(&rstruc.walk_area_top[0], sizeof(short), MAX_WALK_AREAS + 1, opty);
  fwrite(&rstruc.walk_area_bottom[0], sizeof(short), MAX_WALK_AREAS + 1, opty);

  fwrite(&rstruc.password[0], 11, 1, opty);
  fwrite(&rstruc.options[0], 10, 1, opty);
  fwrite(&rstruc.nummes, 2, 1, opty);

  if (rfh.version >= 25)
    putw(rstruc.gameId, opty);
 
  if (rfh.version >= 3)
    fwrite(&rstruc.msgi[0], sizeof(MessageInfo), rstruc.nummes, opty);

  for (f = 0; f < rstruc.nummes; f++)
    write_string_encrypt(opty, rstruc.message[f]);
//    fputstring(rstruc.message[f], opty);

  if (rfh.version >= 6) {
    // we no longer use animations, remove them
    rstruc.numanims = 0;
    fwrite(&rstruc.numanims, 2, 1, opty);

    if (rstruc.numanims > 0)
      fwrite(&rstruc.anims[0], sizeof(FullAnimation), rstruc.numanims, opty);
  }

  if ((rfh.version >= 4) && (rfh.version < 16)) {
    save_script_configuration(opty);
    save_graphical_scripts(opty, &rstruc);
  }

  if (rfh.version >= 8)
    fwrite(&rstruc.shadinginfo[0], sizeof(short), 16, opty);

  if (rfh.version >= 21) {
    fwrite(&rstruc.regionLightLevel[0], sizeof(short), MAX_REGIONS, opty);
    fwrite(&rstruc.regionTintLevel[0], sizeof(int), MAX_REGIONS, opty);
  }

  xoff = ftell(opty);
  fclose(opty);

  tesl = save_lzw((char*)files, rstruc.ebscene[0], rstruc.pal, xoff);

  tesl = savecompressed_allegro((char*)files, rstruc.regions, rstruc.pal, tesl);
  tesl = savecompressed_allegro((char*)files, rstruc.walls, rstruc.pal, tesl);
  tesl = savecompressed_allegro((char*)files, rstruc.object, rstruc.pal, tesl);
  tesl = savecompressed_allegro((char*)files, rstruc.lookat, rstruc.pal, tesl);

  if (rfh.version >= 5) {
    long  lee;

    opty = ci_fopen(const_cast<char*>(files),"r+b");
    lee = filelength(fileno(opty))-7;

    fseek(opty, 3, SEEK_SET);
    fwrite(&lee, 4, 1, opty);
    fseek(opty, 0, SEEK_END);

    if (rstruc.scripts != NULL) {
      int hh;

      fputc(BLOCKTYPE_SCRIPT, opty);
      lee = (int)strlen(rstruc.scripts) + 4;
      fwrite(&lee, 4, 1, opty);
      lee -= 4;

      for (hh = 0; hh < lee; hh++)
        rstruc.scripts[hh]-=passwencstring[hh % 11];

      fwrite(&lee, 4, 1, opty);
      fwrite(rstruc.scripts, lee, 1, opty);

      for (hh = 0; hh < lee; hh++)
        rstruc.scripts[hh]+=passwencstring[hh % 11];

    }
   
    if (rstruc.compiled_script != NULL) {
      long  leeat, wasat;

      fputc(BLOCKTYPE_COMPSCRIPT3, opty);
      lee = 0;
      leeat = ftell(opty);
      fwrite(&lee, 4, 1, opty);
      fwrite_script(rstruc.compiled_script, opty);
     
      wasat = ftell(opty);
      fseek(opty, leeat, SEEK_SET);
      lee = (wasat - leeat) - 4;
      fwrite(&lee, 4, 1, opty);
      fseek(opty, 0, SEEK_END);
    }

    if (rstruc.numsprs > 0) {
      fputc(BLOCKTYPE_OBJECTNAMES, opty);
      lee=rstruc.numsprs * MAXOBJNAMELEN + 1;
      fwrite(&lee, 4, 1, opty);
      fputc(rstruc.numsprs, opty);
      fwrite(&rstruc.objectnames[0][0], MAXOBJNAMELEN, rstruc.numsprs, opty);

      fputc(BLOCKTYPE_OBJECTSCRIPTNAMES, opty);
      lee = rstruc.numsprs * MAX_SCRIPT_NAME_LEN + 1;
      fwrite(&lee, 4, 1, opty);
      fputc(rstruc.numsprs, opty);
      fwrite(&rstruc.objectscriptnames[0][0], MAX_SCRIPT_NAME_LEN, rstruc.numsprs, opty);
    }

    long lenpos, lenis;
    int gg;

    if (rstruc.num_bscenes > 1) {
      long  curoffs;

      fputc(BLOCKTYPE_ANIMBKGRND, opty);
      lenpos = ftell(opty);
      lenis = 0;
      fwrite(&lenis, 4, 1, opty);
      fputc(rstruc.num_bscenes, opty);
      fputc(rstruc.bscene_anim_speed, opty);
      
      fwrite (&rstruc.ebpalShared[0], 1, rstruc.num_bscenes, opty);

      fclose(opty);

      curoffs = lenpos + 6 + rstruc.num_bscenes;
      for (gg = 1; gg < rstruc.num_bscenes; gg++)
        curoffs = save_lzw((char*)files, rstruc.ebscene[gg], rstruc.bpalettes[gg], curoffs);

      opty = ci_fopen(const_cast<char*>(files), "r+b");
      lenis = (curoffs - lenpos) - 4;
      fseek(opty, lenpos, SEEK_SET);
      fwrite(&lenis, 4, 1, opty);
      fseek(opty, 0, SEEK_END);
    }

    // Write custom properties
    fputc (BLOCKTYPE_PROPERTIES, opty);
    lenpos = ftell(opty);
    lenis = 0;
    fwrite(&lenis, 4, 1, opty);
    putw (1, opty);  // Version 1 of properties block
    rstruc.roomProps.Serialize (opty);
    for (gg = 0; gg < rstruc.numhotspots; gg++)
      rstruc.hsProps[gg].Serialize (opty);
    for (gg = 0; gg < rstruc.numsprs; gg++)
      rstruc.objProps[gg].Serialize (opty);

    lenis = (ftell(opty) - lenpos) - 4;
    fseek(opty, lenpos, SEEK_SET);
    fwrite(&lenis, 4, 1, opty);
    fseek(opty, 0, SEEK_END);


    // Write EOF block
    fputc(BLOCKTYPE_EOF, opty);
    fclose(opty);
  }
 
  if (rfh.version < 9) {
    for (f = 0; f < 11; f++)
      rstruc.password[f]+=60;
  }
  else
    for (f = 0; f < 11; f++)
      rstruc.password[f] += passwencstring[f];

//  fclose(opty);
//  return SUCCESS;
}

void save_room_file(const char*rtsa) 
{
  thisroom.wasversion=ROOM_FILE_VERSION;
  copy_room_palette_to_global_palette();
  
  thisroom.password[0] = 0;

  calculate_walkable_areas();

  thisroom.bytes_per_pixel = bmp_bpp(thisroom.ebscene[0]);
  int ww;
  // Fix hi-color screens
  for (ww = 0; ww < thisroom.num_bscenes; ww++)
    fix_block (thisroom.ebscene[ww]);

  thisroom.numobj = MAX_OBJ;
  save_room((char*)rtsa,thisroom);

  // Fix hi-color screens back again
  for (ww = 0; ww < thisroom.num_bscenes; ww++)
    fix_block (thisroom.ebscene[ww]);
}



// ****** CLIB MAKER **** //

#define MAX_FILES 10000
#define MAXMULTIFILES 25
#define MAX_FILENAME_LENGTH 100
#define MAX_DATAFILENAME_LENGTH 50
struct MultiFileLibNew {
  char data_filenames[MAXMULTIFILES][MAX_DATAFILENAME_LENGTH];
  int  num_data_files;
  char filenames[MAX_FILES][MAX_FILENAME_LENGTH];
  long offset[MAX_FILES];
  long length[MAX_FILES];
  char file_datafile[MAX_FILES];  // number of datafile
  int  num_files;
  };
MultiFileLibNew ourlib;
static const char *tempSetting = "My\x1\xde\x4Jibzle";  // clib password
extern void init_pseudo_rand_gen(int seed);
extern int get_pseudo_rand();
const int RAND_SEED_SALT = 9338638;  // must update clib32.cpp if this changes

void fwrite_data_enc(const void *data, int dataSize, int dataCount, FILE *ooo)
{
  const unsigned char *dataChar = (const unsigned char*)data;
  for (int i = 0; i < dataSize * dataCount; i++)
  {
    fputc(dataChar[i] + get_pseudo_rand(), ooo);
  }
}

void fputstring_enc(const char *sss, FILE *ooo) 
{
  fwrite_data_enc(sss, 1, strlen(sss) + 1, ooo);
}

void putw_enc(int numberToWrite, FILE *ooo)
{
  fwrite_data_enc(&numberToWrite, 4, 1, ooo);
}

void write_clib_header(FILE*wout) {
  int ff;
  int randSeed = (int)time(NULL);
  putw(randSeed - RAND_SEED_SALT, wout);
  init_pseudo_rand_gen(randSeed);
  putw_enc(ourlib.num_data_files,wout);
  for (ff = 0; ff < ourlib.num_data_files; ff++)
  {
    fputstring_enc(ourlib.data_filenames[ff], wout);
  }
  putw_enc(ourlib.num_files, wout);
  for (ff = 0; ff < ourlib.num_files; ff++) 
  {
    fputstring_enc(ourlib.filenames[ff], wout);
  }
  fwrite_data_enc(&ourlib.offset[0],4,ourlib.num_files,wout);
  fwrite_data_enc(&ourlib.length[0],4,ourlib.num_files,wout);
  fwrite_data_enc(&ourlib.file_datafile[0],1,ourlib.num_files,wout);
}


#define CHUNKSIZE 256000
int copy_file_across(FILE*inlibb,FILE*coppy,long leftforthis) {
  int success = 1;
  char*diskbuffer=(char*)malloc(CHUNKSIZE+10);
  while (leftforthis>0) {
    if (leftforthis>CHUNKSIZE) {
      fread(diskbuffer,CHUNKSIZE,1,inlibb);
      success = fwrite(diskbuffer,CHUNKSIZE,1,coppy);
      leftforthis-=CHUNKSIZE;
    }
    else {
      fread(diskbuffer,leftforthis,1,inlibb);
      success = fwrite(diskbuffer,leftforthis,1,coppy);
      leftforthis=0;
    }
    if (success < 1)
      break;
  }
  free(diskbuffer);
  return success;
}

const char* make_old_style_data_file(const char* dataFileName, int numfile, char * const*filenames)
{
  const char *errorMsg = NULL;
  int a;
  int passwmod = 20;
  long *filesizes = (long*)malloc(4*numfile);
  char**writefname = (char**)malloc(4*numfile);
  writefname[0]=(char*)malloc(14*numfile);

  for (a=0;a<numfile;a++) {
    if (a>0) writefname[a]=&writefname[0][a*13];
	if (strrchr(filenames[a], '\\') != NULL)
		strcpy(writefname[a], strrchr(filenames[a], '\\') + 1);
	else if (strrchr(filenames[a], '/') != NULL)
		strcpy(writefname[a], strrchr(filenames[a], '/') + 1);
	else
		strcpy(writefname[a],filenames[a]);

	if (strlen(writefname[a]) > 12)
    {
		char buffer[500];
		sprintf(buffer, "Filename too long: %s", writefname[a]);
		free(filesizes);
		free(writefname);
		ThrowManagedException(buffer);
    }

    FILE*ddd = fopen(filenames[a],"rb");
    if (ddd==NULL) { 
      filesizes[a] = 0;
      continue;
    }
    filesizes[a] = _filelength(_fileno(ddd));
    fclose(ddd);

    for (int bb = 0; writefname[a][bb] != 0; bb++)
      writefname[a][bb] += passwmod;
  }
  // write the header
  FILE*wout=fopen(dataFileName, "wb");
  fwrite("CLIB\x1a",5,1,wout);
  fputc(6,wout);  // version
  fputc(passwmod,wout);  // password modifier
  fputc(0,wout);  // reserved
  fwrite(&numfile,2,1,wout);
  for (a=0;a<13;a++) fputc(0,wout);  // the password
  fwrite(&writefname[0][0],13,numfile,wout);
  fwrite(&filesizes[0],4,numfile,wout);
  for (a=0;a<2*numfile;a++) fputc(0,wout);  // comp.ratio

  // now copy the data
  for (a=0;a<numfile;a++) {

	FILE*iii = fopen(filenames[a],"rb");
    if (iii==NULL) {
      errorMsg = "unable to add one of the files to data file.";
      continue;
    }
    if (copy_file_across(iii,wout,filesizes[a]) < 1) {
      errorMsg = "Error writing file: possibly disk full";
      fclose(iii);
      break;
    }
    fclose(iii);
  }
  fclose(wout);
  free(filesizes);
  free(writefname[0]);
  free(writefname);

  if (errorMsg != NULL) 
  {
	unlink(dataFileName);
  }

  return errorMsg;
}

FILE* find_file_in_path(char *buffer, const char *fileName)
{
	char tomake[MAX_PATH];
	strcpy(tomake, fileName);
	FILE* iii = clibfopen(tomake, "rb");
	if (iii == NULL) {
	  // try in the Audio folder if not found
	  sprintf(tomake, "AudioCache\\%s", fileName);
	  iii = clibfopen(tomake, "rb");
	}
	if (iii == NULL) {
	  // no? maybe Speech then, templates include this
	  sprintf(tomake, "Speech\\%s", fileName);
	  iii = clibfopen(tomake, "rb");
	}

	if (buffer != NULL)
	  strcpy(buffer, tomake);

	return iii;
}

const char* make_data_file(int numFiles, char * const*fileNames, long splitSize, const char *baseFileName, bool makeFileNameAssumptionsForEXE)
{
  int a,b;
  FILE*wout;
  char tomake[MAX_PATH];
  ourlib.num_data_files = 0;
  ourlib.num_files = numFiles;
  cfopenpriority = 2;

  int currentDataFile = 0;
  long sizeSoFar = 0;
  bool doSplitting = false;

  for (a = 0; a < numFiles; a++)
  {
	  if (splitSize > 0)
	  {
		  if (stricmp(fileNames[a], sprsetname) == 0) 
		  {
			  // the sprite file's appearance signifies it's time to start splitting
			  doSplitting = true;
			  currentDataFile++;
			  sizeSoFar = 0;
		  }
		  else if ((sizeSoFar > splitSize) && (doSplitting) && 
			  (currentDataFile < MAXMULTIFILES - 1))
		  {
			  currentDataFile++;
			  sizeSoFar = 0;
		  }
	  }
	  long thisFileSize = 0;
	  FILE *tf = fopen(fileNames[a], "rb");
	  thisFileSize = _filelength(fileno(tf));
	  fclose(tf);
	  
	  sizeSoFar += thisFileSize;

    const char *fileNameSrc = fileNames[a];

  	if (strrchr(fileNames[a], '\\') != NULL)
		  fileNameSrc = strrchr(fileNames[a], '\\') + 1;
	  else if (strrchr(fileNames[a], '/') != NULL)
		  fileNameSrc = strrchr(fileNames[a], '/') + 1;

    if (strlen(fileNameSrc) >= MAX_FILENAME_LENGTH)
    {
      char buffer[500];
      sprintf(buffer, "Filename too long: %s", fileNames[a]);
      ThrowManagedException(buffer);
    }
		strcpy(ourlib.filenames[a], fileNameSrc);

	  ourlib.file_datafile[a] = currentDataFile;
	  ourlib.length[a] = thisFileSize;
  }

  ourlib.num_data_files = currentDataFile + 1;

  long startOffset = 0;
  long mainHeaderOffset = 0;
  char outputFileName[MAX_PATH];
  char firstDataFileFullPath[MAX_PATH];

  if (makeFileNameAssumptionsForEXE)
  {
	  _mkdir("Compiled");
  }

  // First, set up the ourlib.data_filenames array with all the filenames
  // so that write_clib_header will write the correct amount of data
  for (a = 0; a < ourlib.num_data_files; a++) 
  {
	  if (makeFileNameAssumptionsForEXE) 
	  {
		  sprintf(ourlib.data_filenames[a], "%s.%03d", baseFileName, a);
		  if (a == 0)
		  {
			  strcpy(&ourlib.data_filenames[a][strlen(ourlib.data_filenames[a]) - 3], "exe");
		  }
	  }
	  else 
	  {
    	if (strrchr(baseFileName, '\\') != NULL)
		    strcpy(ourlib.data_filenames[a], strrchr(baseFileName, '\\') + 1);
	    else if (strrchr(baseFileName, '/') != NULL)
		    strcpy(ourlib.data_filenames[a], strrchr(baseFileName, '/') + 1);
	    else
		    strcpy(ourlib.data_filenames[a], baseFileName);
	  }
  }

  // adjust the file paths if necessary, so that write_clib_header will
  // write the correct amount of data
  for (b = 0; b < ourlib.num_files; b++) 
  {
	FILE *iii = find_file_in_path(tomake, ourlib.filenames[b]);
	if (iii != NULL)
	{
		fclose(iii);

		if (!makeFileNameAssumptionsForEXE)
		  strcpy(ourlib.filenames[b], tomake);
	}
  }

  // now, create the actual files
  for (a = 0; a < ourlib.num_data_files; a++) 
  {
	  if (makeFileNameAssumptionsForEXE) 
	  {
		  sprintf(outputFileName, "Compiled\\%s", ourlib.data_filenames[a]);
	  }
	  else 
	  {
		  strcpy(outputFileName, baseFileName);
      }
      if (a == 0) strcpy(firstDataFileFullPath, outputFileName);

	  wout = fopen(outputFileName, (a == 0) ? "ab" : "wb");
	  if (wout == NULL) 
	  {
		  return "ERROR: unable to open file for writing";
	  }

	  startOffset = _filelength(_fileno(wout));
    fwrite("CLIB\x1a",5,1,wout);
    fputc(21, wout);  // version
    fputc(a, wout);   // file number

    if (a == 0) 
	{
      mainHeaderOffset = ftell(wout);
      write_clib_header(wout);
    }

    for (b=0;b<ourlib.num_files;b++) {
      if (ourlib.file_datafile[b] == a) {
        ourlib.offset[b] = ftell(wout) - startOffset;

		FILE *iii = find_file_in_path(NULL, ourlib.filenames[b]);
        if (iii == NULL) {
          fclose(wout);
          unlink(outputFileName);

		  char buffer[500];
		  sprintf(buffer, "Unable to find file '%s' for compilation. Do not remove files during the compilation process.", ourlib.filenames[b]);
		  ThrowManagedException(buffer);
        }

        if (copy_file_across(iii,wout,ourlib.length[b]) < 1) {
          fclose(iii);
          return "Error writing file: possibly disk full";
        }
        fclose(iii);
      }
    }
	if (startOffset > 0)
	{
		putw(startOffset, wout);
		fwrite(clibendsig, 12, 1, wout);
	}
    fclose(wout);
  }

  wout = fopen(firstDataFileFullPath, "r+b");
  fseek(wout, mainHeaderOffset, SEEK_SET);
  write_clib_header(wout);
  fclose(wout);
  return NULL;
}



// **** MANAGED CODE ****

#pragma managed
using namespace AGS::Types;
using namespace System;
using namespace System::Collections::Generic;
using namespace System::Drawing;
using namespace System::Drawing::Imaging;
using namespace System::Runtime::InteropServices;
#include "scripting.h"

public ref class TempDataStorage
{
public:
	static Room ^RoomBeingSaved;
};

void ConvertStringToCharArray(System::String^ clrString, char *textBuffer);
void ConvertStringToCharArray(System::String^ clrString, char *textBuffer, int maxLength);

void ThrowManagedException(const char *message) 
{
	throw gcnew AGS::Types::AGSEditorException(gcnew String((const char*)message));
}

void save_game(bool compressSprites)
{
	const char *errorMsg = save_sprites(compressSprites);
	if (errorMsg != NULL)
	{
		throw gcnew AGSEditorException(gcnew String(errorMsg));
	}
}

void CreateBuffer(int width, int height)
{
	drawBuffer = create_bitmap_ex(32, width, height);
	clear_to_color(drawBuffer, 0x00D0D0D0);
}

void DrawSpriteToBuffer(int sprNum, int x, int y, int scaleFactor) {
	block todraw = spriteset[sprNum];
	if (todraw == NULL)
	  todraw = spriteset[0];

	if (((thisgame.spriteflags[sprNum] & SPF_640x400) == 0) &&
		(thisgame.default_resolution > 2))
	{
		scaleFactor *= 2;
	}

	block imageToDraw = todraw;

	if (bitmap_color_depth(todraw) != bitmap_color_depth(drawBuffer)) 
	{
		int oldColorConv = get_color_conversion();
		set_color_conversion(oldColorConv | COLORCONV_KEEP_TRANS);
		block depthConverted = create_bitmap_ex(bitmap_color_depth(drawBuffer), todraw->w, todraw->h);
		blit(todraw, depthConverted, 0, 0, 0, 0, todraw->w, todraw->h);
		set_color_conversion(oldColorConv);

		imageToDraw = depthConverted;
	}

	int drawWidth = imageToDraw->w * scaleFactor;
	int drawHeight = imageToDraw->h * scaleFactor;

	if ((thisgame.spriteflags[sprNum] & SPF_ALPHACHANNEL) != 0)
	{
		if (scaleFactor > 1)
		{
			block resizedImage = create_bitmap_ex(bitmap_color_depth(imageToDraw), drawWidth, drawHeight);
			stretch_blit(imageToDraw, resizedImage, 0, 0, imageToDraw->w, imageToDraw->h, 0, 0, resizedImage->w, resizedImage->h);
			if (imageToDraw != todraw)
				destroy_bitmap(imageToDraw);
			imageToDraw = resizedImage;
		}
		set_alpha_blender();
		draw_trans_sprite(drawBuffer, imageToDraw, x, y);
	}
	else
	{
		Cstretch_sprite(drawBuffer, imageToDraw, x, y, drawWidth, drawHeight);
	}

	if (imageToDraw != todraw)
		destroy_bitmap(imageToDraw);
}

void RenderBufferToHDC(int hdc) 
{
	blit_to_hdc(drawBuffer, (HDC)hdc, 0, 0, 0, 0, drawBuffer->w, drawBuffer->h);
	destroy_bitmap(drawBuffer);
	drawBuffer = NULL;
}

void UpdateSpriteFlags(SpriteFolder ^folder) 
{
	for each (Sprite ^sprite in folder->Sprites)
	{
		thisgame.spriteflags[sprite->Number] = 0;
		if (sprite->Resolution == SpriteImportResolution::HighRes)
			thisgame.spriteflags[sprite->Number] |= SPF_640x400;
		if (sprite->AlphaChannel)
			thisgame.spriteflags[sprite->Number] |= SPF_ALPHACHANNEL;
	}

	for each (SpriteFolder^ subFolder in folder->SubFolders) 
	{
		UpdateSpriteFlags(subFolder);
	}
}

void GameUpdated(Game ^game) {
  thisgame.color_depth = (int)game->Settings->ColorDepth;
  thisgame.default_resolution = (int)game->Settings->Resolution;

  thisgame.options[OPT_NOSCALEFNT] = game->Settings->FontsForHiRes;
  thisgame.options[OPT_ANTIALIASFONTS] = game->Settings->AntiAliasFonts;
  antiAliasFonts = thisgame.options[OPT_ANTIALIASFONTS];
  update_font_sizes();

  destroy_bitmap(abuf);
  abuf = create_bitmap_ex(thisgame.color_depth * 8, 32, 32);

  // ensure that the sprite import knows about pal slots 
  for (int i = 0; i < 256; i++) {
	if (game->Palette[i]->ColourType == PaletteColourType::Background)
	{
	  thisgame.paluses[i] = PAL_BACKGROUND;
	}
	else 
	{
  	  thisgame.paluses[i] = PAL_GAMEWIDE;
    }
  }

  thisgame.numfonts = game->Fonts->Count;
  for (int i = 0; i < thisgame.numfonts; i++) 
  {
	  thisgame.fontflags[i] &= ~FFLG_SIZEMASK;
	  thisgame.fontflags[i] |= game->Fonts[i]->PointSize;
	  reload_font(i);
  }
}

void drawViewLoop (int hdc, ViewLoop^ loopToDraw, int x, int y, int size, int cursel)
{
  ::ViewFrame * frames = (::ViewFrame*)malloc(sizeof(::ViewFrame) * loopToDraw->Frames->Count);
	for (int i = 0; i < loopToDraw->Frames->Count; i++) 
	{
		frames[i].pic = loopToDraw->Frames[i]->Image;
		frames[i].flags = (loopToDraw->Frames[i]->Flipped) ? VFLG_FLIPSPRITE : 0;
	}
  // stretch_sprite is dodgy, retry a few times if it crashes
  int retries = 0;
  while (retries < 3)
  {
    try
    {
	    doDrawViewLoop(hdc, loopToDraw->Frames->Count, frames, x, y, size, cursel);
      break;
    }
    catch (AccessViolationException ^)
    {
      retries++;
    }
  }
  free(frames);
}

block CreateBlockFromBitmap(Bitmap ^bmp, color *imgpal, bool fixColourDepth, bool keepTransparency, int *originalColDepth) 
{
	int colDepth;
	if (bmp->PixelFormat == PixelFormat::Format8bppIndexed)
	{
		colDepth = 8;
	}
	else if (bmp->PixelFormat == PixelFormat::Format16bppRgb555)
	{
		colDepth = 15;
	}
	else if (bmp->PixelFormat == PixelFormat::Format16bppRgb565)
	{
		colDepth = 16;
	}
	else if (bmp->PixelFormat == PixelFormat::Format24bppRgb)
	{
		colDepth = 24;
	}
	else if (bmp->PixelFormat == PixelFormat::Format32bppRgb)
	{
		colDepth = 32;
	}
	else if (bmp->PixelFormat == PixelFormat::Format32bppArgb)
	{
		colDepth = 32;
	}
  else if ((bmp->PixelFormat == PixelFormat::Format48bppRgb) ||
           (bmp->PixelFormat == PixelFormat::Format64bppArgb) ||
           (bmp->PixelFormat == PixelFormat::Format64bppPArgb))
  {
    throw gcnew AGSEditorException("The source image is 48-bit or 64-bit colour. AGS does not support images with a colour depth higher than 32-bit. Make sure that your paint program is set to produce 32-bit images (8-bit per channel), not 48-bit images (16-bit per channel).");
  }
	else
	{
		throw gcnew AGSEditorException(gcnew System::String("Unknown pixel format"));
	}

  if ((thisgame.color_depth == 1) && (colDepth > 8))
  {
    throw gcnew AGSEditorException("You cannot import a hi-colour or true-colour image into a 256-colour game.");
  }

  if (originalColDepth != NULL)
    *originalColDepth = colDepth;

  bool needToFixColourDepth = false;
  if ((colDepth != thisgame.color_depth * 8) && (fixColourDepth))
  {
    needToFixColourDepth = true;
  }

	block tempsprite = create_bitmap_ex(colDepth, bmp->Width, bmp->Height);

	System::Drawing::Rectangle rect(0, 0, bmp->Width, bmp->Height);
	BitmapData ^bmpData = bmp->LockBits(rect, ImageLockMode::ReadWrite, bmp->PixelFormat);
	unsigned char *address = (unsigned char*)bmpData->Scan0.ToPointer();
	for (int y = 0; y < tempsprite->h; y++) {
	  memcpy(&tempsprite->line[y][0], address, bmp->Width * ((colDepth + 1) / 8));
	  address += bmpData->Stride;
	}
	bmp->UnlockBits(bmpData);

	if (colDepth == 8)
	{
		cli::array<System::Drawing::Color> ^bmpPalette = bmp->Palette->Entries;
		for (int i = 0; i < 256; i++) {
      if (i >= bmpPalette->Length)
      {
        // BMP files can have an arbitrary palette size, fill any
        // missing colours with black
			  imgpal[i].r = 1;
			  imgpal[i].g = 1;
			  imgpal[i].b = 1;
      }
      else
      {
			  imgpal[i].r = bmpPalette[i].R / 4;
			  imgpal[i].g = bmpPalette[i].G / 4;
			  imgpal[i].b = bmpPalette[i].B / 4;

        if ((needToFixColourDepth) && (i > 0) && 
            (imgpal[i].r == imgpal[0].r) &&
            (imgpal[i].g == imgpal[0].g) && 
            (imgpal[i].b == imgpal[0].b))
        {
          // convert any (0,0,0) colours to (1,1,1) since the image
          // is about to be converted to hi-colour; this will preserve
          // any transparency
          imgpal[i].r = (imgpal[0].r < 32) ? (imgpal[0].r + 1) : (imgpal[0].r - 1);
			    imgpal[i].g = (imgpal[0].g < 32) ? (imgpal[0].g + 1) : (imgpal[0].g - 1);
			    imgpal[i].b = (imgpal[0].b < 32) ? (imgpal[0].b + 1) : (imgpal[0].b - 1);
        }
      }
		}
	}

	if (needToFixColourDepth)
	{
		block spriteAtRightDepth = create_bitmap_ex(thisgame.color_depth * 8, tempsprite->w, tempsprite->h);
		if (colDepth == 8)
		{
			select_palette(imgpal);
		}

		int oldColorConv = get_color_conversion();
		if (keepTransparency)
		{
			set_color_conversion(oldColorConv | COLORCONV_KEEP_TRANS);
		}
		else
		{
			set_color_conversion(oldColorConv & ~COLORCONV_KEEP_TRANS);
		}

		blit(tempsprite, spriteAtRightDepth, 0, 0, 0, 0, tempsprite->w, tempsprite->h);

		set_color_conversion(oldColorConv);

		if (colDepth == 8) 
		{
			unselect_palette();
		}
		destroy_bitmap(tempsprite);
		tempsprite = spriteAtRightDepth;
	}

	if (colDepth > 8) 
	{
		fix_block(tempsprite);
	}

	return tempsprite;
}

void DeleteBackground(Room ^room, int backgroundNumber) 
{
	roomstruct *theRoom = (roomstruct*)(void*)room->_roomStructPtr;
	if (theRoom->ebscene[backgroundNumber] != NULL) 
	{
		destroy_bitmap(theRoom->ebscene[backgroundNumber]);
		theRoom->ebscene[backgroundNumber] = NULL;
	}
	theRoom->num_bscenes--;
	room->BackgroundCount--;
	for (int i = backgroundNumber; i < theRoom->num_bscenes; i++) 
	{
		theRoom->ebscene[i] = theRoom->ebscene[i + 1];
		theRoom->ebpalShared[i] = theRoom->ebpalShared[i + 1];
	}
}

void ImportBackground(Room ^room, int backgroundNumber, Bitmap ^bmp, bool useExactPalette, bool sharePalette) 
{
	color oldpale[256];
	block newbg = CreateBlockFromBitmap(bmp, oldpale, true, false, NULL);
	roomstruct *theRoom = (roomstruct*)(void*)room->_roomStructPtr;
	theRoom->width = room->Width;
	theRoom->height = room->Height;
	bool resolutionChanged = (theRoom->resolution != (int)room->Resolution);
	theRoom->resolution = (int)room->Resolution;

	if (bitmap_color_depth(newbg) == 8) 
	{
		for (int aa = 0; aa < 256; aa++) {
		  // make sure it maps to locked cols properly
		  if (thisgame.paluses[aa] == PAL_LOCKED)
			  theRoom->bpalettes[backgroundNumber][aa] = palette[aa];
		}

		// sharing palette with main background - so copy it across
		if (sharePalette) {
		  memcpy (&theRoom->bpalettes[backgroundNumber][0], &palette[0], sizeof(color) * 256);
		  theRoom->ebpalShared[backgroundNumber] = 1;
		  if (backgroundNumber >= theRoom->num_bscenes - 1)
		  	theRoom->ebpalShared[0] = 1;

		  if (!useExactPalette)
			wremapall(oldpale, newbg, palette);
		}
		else {
		  theRoom->ebpalShared[backgroundNumber] = 0;
		  remap_background (newbg, oldpale, theRoom->bpalettes[backgroundNumber], useExactPalette);
		}

    copy_room_palette_to_global_palette();
	}

	if (backgroundNumber >= theRoom->num_bscenes) 
	{
		theRoom->num_bscenes++;
	}
	else 
	{
		destroy_bitmap(theRoom->ebscene[backgroundNumber]);
	}
	theRoom->ebscene[backgroundNumber] = newbg;

  // if size or resolution has changed, reset masks
	if ((newbg->w != theRoom->object->w) || (newbg->h != theRoom->object->h) ||
      (theRoom->width != theRoom->object->w) || (resolutionChanged))
	{
		wfreeblock(theRoom->walls);
		wfreeblock(theRoom->lookat);
		wfreeblock(theRoom->object);
		wfreeblock(theRoom->regions);
		theRoom->walls = create_bitmap_ex(8,theRoom->width / theRoom->resolution, theRoom->height / theRoom->resolution);
		theRoom->lookat = create_bitmap_ex(8,theRoom->width / theRoom->resolution, theRoom->height / theRoom->resolution);
		theRoom->object = create_bitmap_ex(8,theRoom->width, theRoom->height);
		theRoom->regions = create_bitmap_ex(8,theRoom->width / theRoom->resolution, theRoom->height / theRoom->resolution);
		clear(theRoom->walls);
		clear(theRoom->lookat);
		clear(theRoom->object);
		clear(theRoom->regions);
	}

	room->BackgroundCount = theRoom->num_bscenes;
	room->ColorDepth = bitmap_color_depth(theRoom->ebscene[0]);
}

void import_area_mask(void *roomptr, int maskType, Bitmap ^bmp)
{
	color oldpale[256];
	block importedImage = CreateBlockFromBitmap(bmp, oldpale, false, false, NULL);
	block mask = get_bitmap_for_mask((roomstruct*)roomptr, (RoomAreaMask)maskType);

	if (mask->w != importedImage->w)
	{
		// allow them to import a double-size or half-size mask, adjust it as appropriate
		Cstretch_blit(importedImage, mask, 0, 0, importedImage->w, importedImage->h, 0, 0, mask->w, mask->h);
	}
	else
	{
		blit(importedImage, mask, 0, 0, 0, 0, importedImage->w, importedImage->h);
	}
	destroy_bitmap(importedImage);

	validate_mask(mask, "imported", (maskType == Hotspots) ? MAX_HOTSPOTS : (MAX_WALK_AREAS + 1));
}

void set_rgb_mask_from_alpha_channel(block image)
{
	for (int y = 0; y < image->h; y++)
	{
		unsigned long* thisLine = (unsigned long*)image->line[y];
		for (int x = 0; x < image->w; x++)
		{
			if ((thisLine[x] & 0xff000000) == 0)
			{
				thisLine[x] = MASK_COLOR_32;
			}
		}
	}
}

void set_opaque_alpha_channel(block image)
{
	for (int y = 0; y < image->h; y++)
	{
		unsigned long* thisLine = (unsigned long*)image->line[y];
		for (int x = 0; x < image->w; x++)
		{
			if (thisLine[x] != MASK_COLOR_32)
			  thisLine[x] |= 0xff000000;
		}
	}
}

int SetNewSpriteFromBitmap(int slot, Bitmap^ bmp, int spriteImportMethod, bool remapColours, bool useRoomBackgroundColours, bool alphaChannel) 
{
	color imgPalBuf[256];
  int importedColourDepth;
	block tempsprite = CreateBlockFromBitmap(bmp, imgPalBuf, true, (spriteImportMethod != SIMP_NONE), &importedColourDepth);

	if ((remapColours) || (thisgame.color_depth > 1)) 
	{
		sort_out_transparency(tempsprite, spriteImportMethod, imgPalBuf, useRoomBackgroundColours, importedColourDepth);
	}

	thisgame.spriteflags[slot] = 0;
	if (thisgame.default_resolution > 2)
	{
		thisgame.spriteflags[slot] |= SPF_640x400;
	}
	if (alphaChannel)
	{
		thisgame.spriteflags[slot] |= SPF_ALPHACHANNEL;

		if (bitmap_color_depth(tempsprite) == 32)
		{
			set_rgb_mask_from_alpha_channel(tempsprite);
		}
	}
	else if (bitmap_color_depth(tempsprite) == 32)
	{
		set_opaque_alpha_channel(tempsprite);
	}

	SetNewSprite(slot, tempsprite);

	return (thisgame.default_resolution > 2) ? 1 : 0;
}

void SetBitmapPaletteFromGlobalPalette(System::Drawing::Bitmap ^bmp)
{
	ColorPalette ^colorPal = bmp->Palette;
	cli::array<System::Drawing::Color> ^bmpPalette = colorPal->Entries;
	for (int i = 0; i < 256; i++) 
	{
		bmpPalette[i] = Color::FromArgb((i == 0) ? i : 255, palette[i].r * 4, palette[i].g * 4, palette[i].b * 4);
	}

	// Need to set this back to make it pick up the changes
	bmp->Palette = colorPal;
	//bmp->MakeTransparent(bmpPalette[0]);
}

System::Drawing::Bitmap^ ConvertBlockToBitmap(block todraw, bool useAlphaChannel) 
{
  fix_block(todraw);

  PixelFormat pixFormat = PixelFormat::Format32bppRgb;
  if ((bitmap_color_depth(todraw) == 32) && (useAlphaChannel))
	  pixFormat = PixelFormat::Format32bppArgb;
  else if (bitmap_color_depth(todraw) == 24)
    pixFormat = PixelFormat::Format24bppRgb;
  else if (bitmap_color_depth(todraw) == 16)
    pixFormat = PixelFormat::Format16bppRgb565;
  else if (bitmap_color_depth(todraw) == 15)
    pixFormat = PixelFormat::Format16bppRgb555;
  else if (bitmap_color_depth(todraw) == 8)
    pixFormat = PixelFormat::Format8bppIndexed;
  int bytesPerPixel = (bitmap_color_depth(todraw) + 1) / 8;

  Bitmap ^bmp = gcnew System::Drawing::Bitmap(todraw->w, todraw->h, pixFormat);
  System::Drawing::Rectangle rect(0, 0, bmp->Width, bmp->Height);
  BitmapData ^bmpData = bmp->LockBits(rect, ImageLockMode::WriteOnly, bmp->PixelFormat);
  unsigned char *address = (unsigned char*)bmpData->Scan0.ToPointer();
  for (int y = 0; y < todraw->h; y++) {
    memcpy(address, &todraw->line[y][0], bmp->Width * bytesPerPixel);
    address += bmpData->Stride;
  }
  bmp->UnlockBits(bmpData);

  if (pixFormat == PixelFormat::Format8bppIndexed)
    SetBitmapPaletteFromGlobalPalette(bmp);

  fix_block(todraw);
  return bmp;
}

System::Drawing::Bitmap^ ConvertBlockToBitmap32(block todraw, int width, int height, bool useAlphaChannel) 
{
  block tempBlock = create_bitmap_ex(32, todraw->w, todraw->h);
	
  if (bitmap_color_depth(todraw) == 8)
    select_palette(palette);

  blit(todraw, tempBlock, 0, 0, 0, 0, todraw->w, todraw->h);

  if (bitmap_color_depth(todraw) == 8)
	unselect_palette();

  if ((width != todraw->w) || (height != todraw->h)) 
  {
	  block newBlock = create_bitmap_ex(32, width, height);
	  Cstretch_blit(tempBlock, newBlock, 0, 0, todraw->w, todraw->h, 0, 0, width, height);
	  destroy_bitmap(tempBlock);
	  tempBlock = newBlock;
  }

  fix_block(tempBlock);

  PixelFormat pixFormat = PixelFormat::Format32bppRgb;
  if ((bitmap_color_depth(todraw) == 32) && (useAlphaChannel))
	  pixFormat = PixelFormat::Format32bppArgb;

  Bitmap ^bmp = gcnew System::Drawing::Bitmap(width, height, pixFormat);
  System::Drawing::Rectangle rect(0, 0, bmp->Width, bmp->Height);
  BitmapData ^bmpData = bmp->LockBits(rect, ImageLockMode::WriteOnly, bmp->PixelFormat);
  unsigned char *address = (unsigned char*)bmpData->Scan0.ToPointer();
  for (int y = 0; y < tempBlock->h; y++) {
    memcpy(address, &tempBlock->line[y][0], bmp->Width * 4);
    address += bmpData->Stride;
  }
  bmp->UnlockBits(bmpData);
  destroy_bitmap(tempBlock);
  return bmp;
}

System::Drawing::Bitmap^ ConvertAreaMaskToBitmap(block mask) 
{
	Bitmap ^bmp = gcnew System::Drawing::Bitmap(mask->w, mask->h, PixelFormat::Format8bppIndexed);
	System::Drawing::Rectangle rect(0, 0, bmp->Width, bmp->Height);
	BitmapData ^bmpData = bmp->LockBits(rect, ImageLockMode::WriteOnly, bmp->PixelFormat);
	unsigned char *address = (unsigned char*)bmpData->Scan0.ToPointer();
	for (int y = 0; y < mask->h; y++) 
	{
		memcpy(address, &mask->line[y][0], bmp->Width);
		address += bmpData->Stride;
	}
	bmp->UnlockBits(bmpData);

  SetBitmapPaletteFromGlobalPalette(bmp);

	return bmp;
}

System::Drawing::Bitmap^ getSpriteAsBitmap(int spriteNum) {
  block todraw = get_sprite(spriteNum);
  return ConvertBlockToBitmap(todraw, (thisgame.spriteflags[spriteNum] & SPF_ALPHACHANNEL) != 0);
}

System::Drawing::Bitmap^ getSpriteAsBitmap32bit(int spriteNum, int width, int height) {
  block todraw = get_sprite(spriteNum);
  if (todraw == NULL)
  {
	  throw gcnew AGSEditorException(String::Format("getSpriteAsBitmap32bit: Unable to find sprite {0}", spriteNum));
  }
  return ConvertBlockToBitmap32(todraw, width, height, (thisgame.spriteflags[spriteNum] & SPF_ALPHACHANNEL) != 0);
}

System::Drawing::Bitmap^ getBackgroundAsBitmap(Room ^room, int backgroundNumber) {

  roomstruct *roomptr = (roomstruct*)(void*)room->_roomStructPtr;
  return ConvertBlockToBitmap32(roomptr->ebscene[backgroundNumber], room->Width, room->Height, false);
}

void PaletteUpdated(cli::array<PaletteEntry^>^ newPalette) 
{  
	for each (PaletteEntry ^colour in newPalette) 
	{
		palette[colour->Index].r = colour->Colour.R / 4;
		palette[colour->Index].g = colour->Colour.G / 4;
		palette[colour->Index].b = colour->Colour.B / 4;
	}
	set_palette(palette);
  copy_global_palette_to_room_palette();
}

void ConvertGUIToBinaryFormat(GUI ^guiObj, GUIMain *gui) 
{
  NormalGUI^ normalGui = dynamic_cast<NormalGUI^>(guiObj);
  if (normalGui)
  {
	ConvertStringToCharArray(normalGui->OnClick, gui->clickEventHandler, 20);
	gui->x = normalGui->Left;
	gui->y = normalGui->Top;
	gui->wid = normalGui->Width;
	gui->hit = normalGui->Height;
	gui->flags = (normalGui->Clickable) ? 0 : GUIF_NOCLICK;
    gui->popupyp = normalGui->PopupYPos;
    gui->popup = (int)normalGui->Visibility;
    gui->zorder = normalGui->ZOrder;
	gui->vtext[0] = 0;
    gui->fgcol = normalGui->BorderColor;
    gui->SetTransparencyAsPercentage(normalGui->Transparency);
  }
  else
  {
    TextWindowGUI^ twGui = dynamic_cast<TextWindowGUI^>(guiObj);
	gui->wid = 200;
	gui->hit = 100;
    gui->flags = 0;
	gui->popup = POPUP_SCRIPT;
	gui->vtext[0] = GUI_TEXTWINDOW;
  gui->fgcol = twGui->TextColor;
  }
  gui->bgcol = guiObj->BackgroundColor;
  gui->bgpic = guiObj->BackgroundImage;
  
  ConvertStringToCharArray(guiObj->Name, gui->name);

  gui->numobjs = 0;

  for each (GUIControl^ control in guiObj->Controls)
  {
	  AGS::Types::GUIButton^ button = dynamic_cast<AGS::Types::GUIButton^>(control);
	  AGS::Types::GUILabel^ label = dynamic_cast<AGS::Types::GUILabel^>(control);
	  AGS::Types::GUITextBox^ textbox = dynamic_cast<AGS::Types::GUITextBox^>(control);
	  AGS::Types::GUIListBox^ listbox = dynamic_cast<AGS::Types::GUIListBox^>(control);
	  AGS::Types::GUISlider^ slider = dynamic_cast<AGS::Types::GUISlider^>(control);
	  AGS::Types::GUIInventory^ invwindow = dynamic_cast<AGS::Types::GUIInventory^>(control);
	  AGS::Types::GUITextWindowEdge^ textwindowedge = dynamic_cast<AGS::Types::GUITextWindowEdge^>(control);
	  if (button)
	  {
		  guibuts[numguibuts].textcol = button->TextColor;
		  guibuts[numguibuts].font = button->Font;
		  guibuts[numguibuts].pic = button->Image;
		  guibuts[numguibuts].usepic = guibuts[numguibuts].pic;
		  guibuts[numguibuts].overpic = button->MouseoverImage;
		  guibuts[numguibuts].pushedpic = button->PushedImage;
		  guibuts[numguibuts].textAlignment = (int)button->TextAlignment;
		  guibuts[numguibuts].leftclick = (int)button->ClickAction;
		  guibuts[numguibuts].lclickdata = button->NewModeNumber;
		  guibuts[numguibuts].flags = (button->ClipImage) ? GUIF_CLIP : 0;
		  ConvertStringToCharArray(button->Text, guibuts[numguibuts].text, 50);
		  ConvertStringToCharArray(button->OnClick, guibuts[numguibuts].eventHandlers[0], MAX_GUIOBJ_EVENTHANDLER_LEN + 1);
		  
		  gui->objrefptr[gui->numobjs] = (GOBJ_BUTTON << 16) | numguibuts;
		  gui->objs[gui->numobjs] = &guibuts[numguibuts];
		  gui->numobjs++;
		  numguibuts++;
	  }
	  else if (label)
	  {
		  guilabels[numguilabels].textcol = label->TextColor;
		  guilabels[numguilabels].font = label->Font;
		  guilabels[numguilabels].align = (int)label->TextAlignment;
		  guilabels[numguilabels].flags = 0;
		  char textBuffer[MAX_GUILABEL_TEXT_LEN];
		  ConvertStringToCharArray(label->Text, textBuffer, MAX_GUILABEL_TEXT_LEN);
		  guilabels[numguilabels].SetText(textBuffer);

		  gui->objrefptr[gui->numobjs] = (GOBJ_LABEL << 16) | numguilabels;
		  gui->objs[gui->numobjs] = &guilabels[numguilabels];
		  gui->numobjs++;
		  numguilabels++;
	  }
	  else if (textbox)
	  {
		  guitext[numguitext].textcol = textbox->TextColor;
		  guitext[numguitext].font = textbox->Font;
		  guitext[numguitext].flags = 0;
		  guitext[numguitext].exflags = (textbox->ShowBorder) ? 0 : GTF_NOBORDER;
		  guitext[numguitext].text[0] = 0;
		  ConvertStringToCharArray(textbox->OnActivate, guitext[numguitext].eventHandlers[0], MAX_GUIOBJ_EVENTHANDLER_LEN + 1);

		  gui->objrefptr[gui->numobjs] = (GOBJ_TEXTBOX << 16) | numguitext;
		  gui->objs[gui->numobjs] = &guitext[numguitext];
		  gui->numobjs++;
		  numguitext++;
	  }
	  else if (listbox)
	  {
		  guilist[numguilist].textcol = listbox->TextColor;
		  guilist[numguilist].font = listbox->Font;
		  guilist[numguilist].backcol = listbox->SelectedTextColor;
		  guilist[numguilist].selectedbgcol = listbox->SelectedBackgroundColor;
		  guilist[numguilist].alignment = (int)listbox->TextAlignment;
		  guilist[numguilist].exflags = (listbox->ShowBorder) ? 0 : GLF_NOBORDER;
		  guilist[numguilist].exflags |= (listbox->ShowScrollArrows) ? 0 : GLF_NOARROWS;
		  ConvertStringToCharArray(listbox->OnSelectionChanged, guilist[numguilist].eventHandlers[0], MAX_GUIOBJ_EVENTHANDLER_LEN + 1);

		  gui->objrefptr[gui->numobjs] = (GOBJ_LISTBOX << 16) | numguilist;
		  gui->objs[gui->numobjs] = &guilist[numguilist];
		  gui->numobjs++;
		  numguilist++;
	  }
	  else if (slider)
	  {
		  guislider[numguislider].min = slider->MinValue;
		  guislider[numguislider].max = slider->MaxValue;
		  guislider[numguislider].value = slider->Value;
		  guislider[numguislider].handlepic = slider->HandleImage;
		  guislider[numguislider].handleoffset = slider->HandleOffset;
		  guislider[numguislider].bgimage = slider->BackgroundImage;
		  ConvertStringToCharArray(slider->OnChange, guislider[numguislider].eventHandlers[0], MAX_GUIOBJ_EVENTHANDLER_LEN + 1);

		  gui->objrefptr[gui->numobjs] = (GOBJ_SLIDER << 16) | numguislider;
		  gui->objs[gui->numobjs] = &guislider[numguislider];
		  gui->numobjs++;
		  numguislider++;
	  }
	  else if (invwindow)
	  {
		  guiinv[numguiinv].charId = invwindow->CharacterID;
		  guiinv[numguiinv].itemWidth = invwindow->ItemWidth;
		  guiinv[numguiinv].itemHeight = invwindow->ItemHeight;

		  gui->objrefptr[gui->numobjs] = (GOBJ_INVENTORY << 16) | numguiinv;
		  gui->objs[gui->numobjs] = &guiinv[numguiinv];
		  gui->numobjs++;
		  numguiinv++;
	  }
	  else if (textwindowedge)
	  {
		  guibuts[numguibuts].pic = textwindowedge->Image;
		  guibuts[numguibuts].usepic = guibuts[numguibuts].pic;
		  guibuts[numguibuts].flags = 0;
		  guibuts[numguibuts].text[0] = 0;
		  
		  gui->objrefptr[gui->numobjs] = (GOBJ_BUTTON << 16) | numguibuts;
		  gui->objs[gui->numobjs] = &guibuts[numguibuts];
		  gui->numobjs++;
		  numguibuts++;
	  }

	  GUIObject *newObj = gui->objs[gui->numobjs - 1];
	  newObj->x = control->Left;
	  newObj->y = control->Top;
	  newObj->wid = control->Width;
	  newObj->hit = control->Height;
	  newObj->objn = control->ID;
	  newObj->zorder = control->ZOrder;
	  ConvertStringToCharArray(control->Name, newObj->scriptName, MAX_GUIOBJ_SCRIPTNAME_LEN + 1);
  }

  gui->rebuild_array();
  gui->resort_zorder();
}

void drawGUI(int hdc, int x,int y, GUI^ guiObj, int scaleFactor, int selectedControl) {
  numguibuts = 0;
  numguilabels = 0;
  numguitext = 0;
  numguilist = 0;
  numguislider = 0;
  numguiinv = 0;

  ConvertGUIToBinaryFormat(guiObj, &tempgui);

  tempgui.highlightobj = selectedControl;

  drawGUIAt(hdc, x, y, -1, -1, -1, -1, scaleFactor);
}

Dictionary<int, Sprite^>^ load_sprite_dimensions()
{
	Dictionary<int, Sprite^>^ sprites = gcnew Dictionary<int, Sprite^>();

	for (int i = 0; i < spriteset.elements; i++)
	{
		block spr = spriteset[i];
		if (spr != NULL)
		{
			sprites->Add(i, gcnew Sprite(i, spr->w, spr->h, bitmap_color_depth(spr), (thisgame.spriteflags[i] & SPF_640x400) ? SpriteImportResolution::HighRes : SpriteImportResolution::LowRes, (thisgame.spriteflags[i] & SPF_ALPHACHANNEL) ? true : false));
		}
	}

	return sprites;
}

void ConvertCustomProperties(AGS::Types::CustomProperties ^insertInto, ::CustomProperties *propToConvert)
{
	for (int j = 0; j < propToConvert->numProps; j++) 
	{
		CustomProperty ^newProp = gcnew CustomProperty();
		newProp->Name = gcnew String(propToConvert->propName[j]);
		newProp->Value = gcnew String(propToConvert->propVal[j]);
		insertInto->PropertyValues->Add(newProp->Name, newProp);
	}
}

void CompileCustomProperties(AGS::Types::CustomProperties ^convertFrom, ::CustomProperties *compileInto)
{
	compileInto->reset();
	int j = 0;
	char propName[200];
	char propVal[MAX_CUSTOM_PROPERTY_VALUE_LENGTH];
	for each (String ^key in convertFrom->PropertyValues->Keys)
	{
		ConvertStringToCharArray(convertFrom->PropertyValues[key]->Name, propName, 200);
		ConvertStringToCharArray(convertFrom->PropertyValues[key]->Value, propVal, MAX_CUSTOM_PROPERTY_VALUE_LENGTH);
		compileInto->addProperty(propName, propVal);
		j++;
	}
}

char charScriptNameBuf[100];
const char *GetCharacterScriptName(int charid, AGS::Types::Game ^game) 
{
	if ((charid >= 0) && (game != nullptr) &&
    (charid < game->Characters->Count) &&
		(game->Characters[charid]->ScriptName->Length > 0))
	{
		ConvertStringToCharArray(game->Characters[charid]->ScriptName,  charScriptNameBuf, 100); 
	}
	else 
	{
		sprintf(charScriptNameBuf, "character[%d]", charid);
	}
	return charScriptNameBuf;
}

void ConvertInteractionToScript(System::Text::StringBuilder ^sb, NewInteractionCommand *intrcmd, String^ scriptFuncPrefix, AGS::Types::Game ^game, int *runScriptCount, bool *onlyIfInvWasUseds, int commandOffset) 
{
  if (intrcmd->type != 1)
  {
    // if another type of interaction, we definately can't optimise
    // away the wrapper function
    runScriptCount[0] = 1000;
  }
  else
  {
    runScriptCount[0]++;
  }

  if (intrcmd->type != 20)
  {
	  *onlyIfInvWasUseds = false;
  }

	switch (intrcmd->type)
	{
	case 0:
		break;
	case 1:  // Run Script
		sb->Append(scriptFuncPrefix);
		sb->Append(System::Convert::ToChar(intrcmd->data[0].val + 'a'));
		sb->AppendLine("();");
		break;
	case 3: // Add Score
	case 4: // Display Message
	case 5: // Play Music
	case 6: // Stop Music
	case 7: // Play Sound
	case 8: // Play Flic
	case 9: // Run Dialog
	case 10: // Enable Dialog Option
	case 11: // Disalbe Dialog Option
	case 13: // Give player an inventory item
	case 15: // hide object
	case 16: // show object 
	case 17: // change object view
	case 20: // IF inv was used
	case 21: // IF player has inv
	case 22: // IF character is moving
	case 24: // stop moving
	case 25: // change room (at co-ords)
	case 26: // change room of NPC
	case 27: // lock character view
	case 28: // unlock character view
	case 29: // follow character
	case 30: // stop following
	case 31: // disable hotspot
	case 32: // enable hotspot
	case 36: // set idle
	case 37: // disable idle
	case 38: // lose inventory
	case 39: // show gui
	case 40: // hide gui
	case 41: // stop running commands
	case 42: // facelocation
	case 43: // wait()
	case 44: // change character view
	case 45: // IF player is
	case 46: // IF mouse cursor is
	case 47: // IF player has been in room
		// For these, the sample script code will work
		{
		String ^scriptCode = gcnew String(actions[intrcmd->type].textscript);
		if ((*onlyIfInvWasUseds) && (commandOffset > 0))
		{
			scriptCode = String::Concat("else ", scriptCode);
		}
		scriptCode = scriptCode->Replace("$$1", (gcnew Int32(intrcmd->data[0].val))->ToString() );
		scriptCode = scriptCode->Replace("$$2", (gcnew Int32(intrcmd->data[1].val))->ToString() );
		scriptCode = scriptCode->Replace("$$3", (gcnew Int32(intrcmd->data[2].val))->ToString() );
		scriptCode = scriptCode->Replace("$$4", (gcnew Int32(intrcmd->data[3].val))->ToString() );
		sb->AppendLine(scriptCode);
		}
		break;
	case 34: // animate character
		{
		char scriptCode[100];
		int charID = intrcmd->data[0].val;
		int loop = intrcmd->data[1].val;
		int speed = intrcmd->data[2].val;
		sprintf(scriptCode, "%s.Animate(%d, %d, eOnce, eBlock);", GetCharacterScriptName(charID, game), loop, speed);
		sb->AppendLine(gcnew String(scriptCode));
		}
		break;
	case 35: // quick animation
		{
		char scriptCode[300];
		int charID = intrcmd->data[0].val;
		int view = intrcmd->data[1].val;
		int loop = intrcmd->data[2].val;
		int speed = intrcmd->data[3].val;
		sprintf(scriptCode, "%s.LockView(%d);\n"
							"%s.Animate(%d, %d, eOnce, eBlock);\n"
							"%s.UnlockView();",
							GetCharacterScriptName(charID, game), view, 
							GetCharacterScriptName(charID, game), loop, speed, 
							GetCharacterScriptName(charID, game));
		sb->AppendLine(gcnew String(scriptCode));
		}
		break;
	case 14: // Move Object
		{
		char scriptCode[100];
		int objID = intrcmd->data[0].val;
		int x = intrcmd->data[1].val;
		int y = intrcmd->data[2].val;
		int speed = intrcmd->data[3].val;
		sprintf(scriptCode, "object[%d].Move(%d, %d, %d, %s);", objID, x, y, speed, (intrcmd->data[4].val) ? "eBlock" : "eNoBlock");
		sb->AppendLine(gcnew String(scriptCode));
		}
		break;
	case 19: // Move Character
		{
		char scriptCode[100];
		int charID = intrcmd->data[0].val;
		int x = intrcmd->data[1].val;
		int y = intrcmd->data[2].val;
		sprintf(scriptCode, "%s.Walk(%d, %d, %s);", GetCharacterScriptName(charID, game), x, y, (intrcmd->data[3].val) ? "eBlock" : "eNoBlock");
		sb->AppendLine(gcnew String(scriptCode));
		}
		break;
	case 18: // Animate Object
		{
		char scriptCode[100];
		int objID = intrcmd->data[0].val;
		int loop = intrcmd->data[1].val;
		int speed = intrcmd->data[2].val;
		sprintf(scriptCode, "object[%d].Animate(%d, %d, %s, eNoBlock);", objID, loop, speed, (intrcmd->data[3].val) ? "eRepeat" : "eOnce");
		sb->AppendLine(gcnew String(scriptCode));
		}
		break;
	case 23: // IF variable set to value
		{
		char scriptCode[100];
		int valueToCheck = intrcmd->data[1].val;
		if ((game == nullptr) || (intrcmd->data[0].val >= game->OldInteractionVariables->Count))
		{
			sprintf(scriptCode, "if (__INTRVAL$%d$ == %d) {", intrcmd->data[0].val, valueToCheck);
		}
		else
		{
			OldInteractionVariable^ variableToCheck = game->OldInteractionVariables[intrcmd->data[0].val];
			sprintf(scriptCode, "if (%s == %d) {", variableToCheck->ScriptName, valueToCheck);
		}
		sb->AppendLine(gcnew String(scriptCode));
		break;
		}
	case 33: // Set variable
		{
		char scriptCode[100];
		int valueToCheck = intrcmd->data[1].val;
		if ((game == nullptr) || (intrcmd->data[0].val >= game->OldInteractionVariables->Count))
		{
			sprintf(scriptCode, "__INTRVAL$%d$ = %d;", intrcmd->data[0].val, valueToCheck);
		}
		else
		{
			OldInteractionVariable^ variableToCheck = game->OldInteractionVariables[intrcmd->data[0].val];
			sprintf(scriptCode, "%s = %d;", variableToCheck->ScriptName, valueToCheck);
		}
		sb->AppendLine(gcnew String(scriptCode));
		break;
		}
	case 12: // Change Room
		{
		char scriptCode[200];
		int room = intrcmd->data[0].val;
		sprintf(scriptCode, "player.ChangeRoomAutoPosition(%d", room);
		if (intrcmd->data[1].val > 0) 
		{
			sprintf(&scriptCode[strlen(scriptCode)], ", %d", intrcmd->data[1].val);
		}
		strcat(scriptCode, ");");
		sb->AppendLine(gcnew String(scriptCode));
		}
		break;
	case 2: // Add Score On First Execution
		{
		  int points = intrcmd->data[0].val;
      String^ newGuid = System::Guid::NewGuid().ToString();
      String^ scriptCode = String::Format("if (Game.DoOnceOnly(\"{0}\"))", newGuid);
      scriptCode = String::Concat(scriptCode, " {\n  ");
      scriptCode = String::Concat(scriptCode, String::Format("GiveScore({0});", points.ToString()));
      scriptCode = String::Concat(scriptCode, "\n}");
		  sb->AppendLine(scriptCode);
		}
		break;
	default:
		throw gcnew InvalidDataException("Invalid interaction type found");
	}
}

void ConvertInteractionCommandList(System::Text::StringBuilder^ sb, NewInteractionCommandList *cmdList, String^ scriptFuncPrefix, AGS::Types::Game^ game, int *runScriptCount, int targetTypeForUnhandledEvent) 
{
	bool onlyIfInvWasUseds = true;

	for (int cmd = 0; cmd < cmdList->numCommands; cmd++)
	{
		ConvertInteractionToScript(sb, &cmdList->command[cmd], scriptFuncPrefix, game, runScriptCount, &onlyIfInvWasUseds, cmd);
		if (cmdList->command[cmd].get_child_list() != NULL) 
		{
			ConvertInteractionCommandList(sb, cmdList->command[cmd].get_child_list(), scriptFuncPrefix, game, runScriptCount, targetTypeForUnhandledEvent);
			sb->AppendLine("}");
		}
	}

	if ((onlyIfInvWasUseds) && (targetTypeForUnhandledEvent > 0) && 
		(cmdList->numCommands > 0))
	{
		sb->AppendLine("else {");
		sb->AppendLine(String::Format(" unhandled_event({0}, 3);", targetTypeForUnhandledEvent));
		sb->AppendLine("}");
	}
}

void CopyInteractions(AGS::Types::Interactions ^destination, ::InteractionScripts *source)
{
	if (source->numEvents > destination->ScriptFunctionNames->Length) 
	{
		throw gcnew AGS::Types::AGSEditorException("Invalid interaction funcs: too many interaction events");
	}

	for (int i = 0; i < source->numEvents; i++) 
	{
		destination->ScriptFunctionNames[i] = gcnew String(source->scriptFuncNames[i]);
	}
}

void ConvertInteractions(AGS::Types::Interactions ^interactions, ::NewInteraction *intr, String^ scriptFuncPrefix, AGS::Types::Game ^game, int targetTypeForUnhandledEvent)
{
	if (intr->numEvents > interactions->ScriptFunctionNames->Length) 
	{
		throw gcnew AGS::Types::AGSEditorException("Invalid interaction data: too many interaction events");
	}

	for (int i = 0; i < intr->numEvents; i++) 
	{
		if (intr->response[i] != NULL) 
		{
      int runScriptCount = 0;
			System::Text::StringBuilder^ sb = gcnew System::Text::StringBuilder();
			ConvertInteractionCommandList(sb, intr->response[i], scriptFuncPrefix, game, &runScriptCount, targetTypeForUnhandledEvent);
      if (runScriptCount == 1)
      {
        sb->Append("$$SINGLE_RUN_SCRIPT$$");
      }
			interactions->ImportedScripts[i] = sb->ToString();
		}
	}
}

Game^ load_old_game_dta_file(const char *fileName)
{
	const char *errorMsg = load_dta_file_into_thisgame(fileName);
	if (errorMsg != NULL)
	{
		throw gcnew AGS::Types::AGSEditorException(gcnew String(errorMsg));
	}

	Game^ game = gcnew Game();
	game->Settings->AlwaysDisplayTextAsSpeech = (thisgame.options[OPT_ALWAYSSPCH] != 0);
	game->Settings->AntiAliasFonts = (thisgame.options[OPT_ANTIALIASFONTS] != 0);
	game->Settings->AntiGlideMode = (thisgame.options[OPT_ANTIGLIDE] != 0);
	game->Settings->AutoMoveInWalkMode = !thisgame.options[OPT_NOWALKMODE];
	game->Settings->BackwardsText = (thisgame.options[OPT_RIGHTLEFTWRITE] != 0);
	game->Settings->ColorDepth = (GameColorDepth)thisgame.color_depth;
	game->Settings->CompressSprites = (thisgame.options[OPT_COMPRESSSPRITES] != 0);
	game->Settings->CrossfadeMusic = (CrossfadeSpeed)thisgame.options[OPT_CROSSFADEMUSIC];
	game->Settings->DebugMode = (thisgame.options[OPT_DEBUGMODE] != 0);
	game->Settings->DialogOptionsBackwards = (thisgame.options[OPT_DIALOGUPWARDS] != 0);
	game->Settings->DialogOptionsGap = thisgame.options[OPT_DIALOGGAP];
	game->Settings->DialogOptionsGUI = thisgame.options[OPT_DIALOGIFACE];
	game->Settings->DialogOptionsBullet = thisgame.dialog_bullet;
	game->Settings->DisplayMultipleInventory = (thisgame.options[OPT_DUPLICATEINV] != 0);
	game->Settings->EnforceNewStrings = (thisgame.options[OPT_STRICTSTRINGS] != 0);
  game->Settings->EnforceNewAudio = false;
	game->Settings->EnforceObjectBasedScript = (thisgame.options[OPT_STRICTSCRIPTING] != 0);
	game->Settings->FontsForHiRes = (thisgame.options[OPT_NOSCALEFNT] != 0);
	game->Settings->GameName = gcnew String(thisgame.gamename);
	game->Settings->GUIAlphaStyle = GUIAlphaStyle::Classic;
	game->Settings->HandleInvClicksInScript = (thisgame.options[OPT_HANDLEINVCLICKS] != 0);
	game->Settings->InventoryCursors = !thisgame.options[OPT_FIXEDINVCURSOR];
	game->Settings->LeftToRightPrecedence = (thisgame.options[OPT_LEFTTORIGHTEVAL] != 0);
	game->Settings->LetterboxMode = (thisgame.options[OPT_LETTERBOX] != 0);
	game->Settings->MaximumScore = thisgame.totalscore;
	game->Settings->MouseWheelEnabled = (thisgame.options[OPT_MOUSEWHEEL] != 0);
	game->Settings->NumberDialogOptions = (thisgame.options[OPT_DIALOGNUMBERED] != 0);
	game->Settings->PixelPerfect = (thisgame.options[OPT_PIXPERFECT] != 0);
	game->Settings->PlaySoundOnScore = thisgame.options[OPT_SCORESOUND];
	game->Settings->Resolution = (GameResolutions)thisgame.default_resolution;
	game->Settings->RoomTransition = (RoomTransitionStyle)thisgame.options[OPT_FADETYPE];
	game->Settings->SaveScreenshots = (thisgame.options[OPT_SAVESCREENSHOT] != 0);
	game->Settings->SkipSpeech = (SkipSpeechStyle)thisgame.options[OPT_NOSKIPTEXT];
	game->Settings->SpeechPortraitSide = (SpeechPortraitSide)thisgame.options[OPT_PORTRAITSIDE];
	game->Settings->SpeechStyle = (SpeechStyle)thisgame.options[OPT_SPEECHTYPE];
	game->Settings->SplitResources = thisgame.options[OPT_SPLITRESOURCES];
	game->Settings->TextWindowGUI = thisgame.options[OPT_TWCUSTOM];
	game->Settings->ThoughtGUI = thisgame.options[OPT_THOUGHTGUI];
	game->Settings->TurnBeforeFacing = (thisgame.options[OPT_TURNTOFACELOC] != 0);
	game->Settings->TurnBeforeWalking = (thisgame.options[OPT_ROTATECHARS] != 0);
	game->Settings->WalkInLookMode = (thisgame.options[OPT_WALKONLOOK] != 0);
	game->Settings->WhenInterfaceDisabled = (InterfaceDisabledAction)thisgame.options[OPT_DISABLEOFF];
	game->Settings->UniqueID = thisgame.uniqueid;
  game->Settings->SaveGameFolderName = gcnew String(thisgame.gamename);

	game->Settings->InventoryHotspotMarker->DotColor = thisgame.hotdot;
	game->Settings->InventoryHotspotMarker->CrosshairColor = thisgame.hotdotouter;
	game->Settings->InventoryHotspotMarker->Image = thisgame.invhotdotsprite;
	if (thisgame.invhotdotsprite) 
	{
		game->Settings->InventoryHotspotMarker->Style = InventoryHotspotMarkerStyle::Sprite;
	}
	else if (thisgame.hotdot) 
	{
		game->Settings->InventoryHotspotMarker->Style = InventoryHotspotMarkerStyle::Crosshair;
	}
	else 
	{
		game->Settings->InventoryHotspotMarker->Style = InventoryHotspotMarkerStyle::None;
	}

	int i;
	for (i = 0; i < 256; i++) 
	{
		if (thisgame.paluses[i] == PAL_BACKGROUND) 
		{
			game->Palette[i]->ColourType = PaletteColourType::Background;
		}
		else 
		{
			game->Palette[i]->ColourType = PaletteColourType::Gamewide; 
		}
		game->Palette[i]->Colour = Color::FromArgb(palette[i].r * 4, palette[i].g * 4, palette[i].b * 4);
	}

	for (i = 0; i < numThisgamePlugins; i++) 
	{
		cli::array<System::Byte> ^pluginData = gcnew cli::array<System::Byte>(thisgamePlugins[i].dataSize);
		for (int j = 0; j < thisgamePlugins[i].dataSize; j++) 
		{
			pluginData[j] = thisgamePlugins[i].data[j];
		}
		
		AGS::Types::Plugin ^plugin = gcnew AGS::Types::Plugin(gcnew String(thisgamePlugins[i].filename), pluginData);
		game->Plugins->Add(plugin);
	}

	for (i = 0; i < numGlobalVars; i++)
	{
		OldInteractionVariable ^intVar;
		intVar = gcnew OldInteractionVariable(gcnew String(globalvars[i].name), globalvars[i].value);
		game->OldInteractionVariables->Add(intVar);
	}
	
	AGS::Types::ViewFolder ^viewFolder = gcnew AGS::Types::ViewFolder();
	for (i = 0; i < thisgame.numviews; i++) 
	{
		AGS::Types::View ^view = gcnew AGS::Types::View();
		view->Name = gcnew String(thisgame.viewNames[i]);
		view->ID = i + 1;

		for (int j = 0; j < oldViews[i].numloops; j++) 
		{
			ViewLoop^ newLoop = gcnew ViewLoop();
			newLoop->ID = j;

			for (int k = 0; k < oldViews[i].numframes[j]; k++) 
			{
				if (oldViews[i].frames[j][k].pic == -1) 
				{
					newLoop->RunNextLoop = true;
				}
				else 
				{
					AGS::Types::ViewFrame^ newFrame = gcnew AGS::Types::ViewFrame();
					newFrame->ID = k;
					newFrame->Flipped = (oldViews[i].frames[j][k].flags & VFLG_FLIPSPRITE);
					newFrame->Image = oldViews[i].frames[j][k].pic;
					newFrame->Sound = oldViews[i].frames[j][k].sound;
					newFrame->Delay = oldViews[i].frames[j][k].speed;

					newLoop->Frames->Add(newFrame);
				}
			}
			
			view->Loops->Add(newLoop);
		}

		viewFolder->Views->Add(view);
	}
	game->RootViewFolder = viewFolder;

	for (i = 0; i < thisgame.numcharacters; i++) 
	{
		char jibbledScriptName[50] = "\0";
		if (strlen(thisgame.chars[i].scrname) > 0) 
		{
			sprintf(jibbledScriptName, "c%s", thisgame.chars[i].scrname);
			strlwr(jibbledScriptName);
			jibbledScriptName[1] = toupper(jibbledScriptName[1]);
		}
		AGS::Types::Character ^character = gcnew AGS::Types::Character();
		character->AdjustSpeedWithScaling = ((thisgame.chars[i].flags & CHF_SCALEMOVESPEED) != 0);
		character->AdjustVolumeWithScaling = ((thisgame.chars[i].flags & CHF_SCALEVOLUME) != 0);
		character->AnimationDelay = thisgame.chars[i].animspeed;
		character->BlinkingView = (thisgame.chars[i].blinkview < 1) ? 0 : (thisgame.chars[i].blinkview + 1);
		character->Clickable = !(thisgame.chars[i].flags & CHF_NOINTERACT);
		character->DiagonalLoops = !(thisgame.chars[i].flags & CHF_NODIAGONAL);
		character->ID = i;
		character->IdleView = (thisgame.chars[i].idleview < 1) ? 0 : (thisgame.chars[i].idleview + 1);
		character->MovementSpeed = thisgame.chars[i].walkspeed;
		character->MovementSpeedX = thisgame.chars[i].walkspeed;
		character->MovementSpeedY = thisgame.chars[i].walkspeed_y;
		character->NormalView = thisgame.chars[i].defview + 1;
		character->RealName = gcnew String(thisgame.chars[i].name);
		character->ScriptName = gcnew String(jibbledScriptName);
		character->Solid = !(thisgame.chars[i].flags & CHF_NOBLOCKING);
		character->SpeechColor = thisgame.chars[i].talkcolor;
		character->SpeechView = (thisgame.chars[i].talkview < 1) ? 0 : (thisgame.chars[i].talkview + 1);
		character->StartingRoom = thisgame.chars[i].room;
		character->StartX = thisgame.chars[i].x;
		character->StartY = thisgame.chars[i].y;
    character->ThinkingView = (thisgame.chars[i].thinkview < 1) ? 0 : (thisgame.chars[i].thinkview + 1);
		character->TurnBeforeWalking = !(thisgame.chars[i].flags & CHF_NOTURNING);
		character->UniformMovementSpeed = (thisgame.chars[i].walkspeed_y == UNIFORM_WALK_SPEED);
		character->UseRoomAreaLighting = !(thisgame.chars[i].flags & CHF_NOLIGHTING);
		character->UseRoomAreaScaling = !(thisgame.chars[i].flags & CHF_MANUALSCALING);

		game->Characters->Add(character);

		ConvertCustomProperties(character->Properties, &thisgame.charProps[i]);

		char scriptFuncPrefix[100];
		sprintf(scriptFuncPrefix, "character%d_", i);
		ConvertInteractions(character->Interactions, thisgame.intrChar[i], gcnew String(scriptFuncPrefix), game, 3);
	}
	game->PlayerCharacter = game->Characters[thisgame.playercharacter];

	game->TextParser->Words->Clear();
	for (i = 0; i < thisgame.dict->num_words; i++) 
	{
		AGS::Types::TextParserWord ^newWord = gcnew AGS::Types::TextParserWord();
		newWord->WordGroup = thisgame.dict->wordnum[i];
		newWord->Word = gcnew String(thisgame.dict->word[i]);
		newWord->SetWordTypeFromGroup();

		game->TextParser->Words->Add(newWord);
	}

	for (i = 0; i < MAXGLOBALMES; i++) 
	{
		if (thisgame.messages[i] != NULL) 
		{
			game->GlobalMessages[i] = gcnew String(thisgame.messages[i]);
		}
		else
		{
			game->GlobalMessages[i] = String::Empty;
		}
	}

	game->LipSync->Type = (thisgame.options[OPT_LIPSYNCTEXT] != 0) ? LipSyncType::Text : LipSyncType::None;
	game->LipSync->DefaultFrame = thisgame.default_lipsync_frame;
	for (i = 0; i < MAXLIPSYNCFRAMES; i++) 
	{
		game->LipSync->CharactersPerFrame[i] = gcnew String(thisgame.lipSyncFrameLetters[i]);
	}

	for (i = 0; i < thisgame.numdialog; i++) 
	{
		AGS::Types::Dialog ^newDialog = gcnew AGS::Types::Dialog();
		newDialog->ID = i;
		for (int j = 0; j < dialog[i].numoptions; j++) 
		{
			AGS::Types::DialogOption ^newOption = gcnew AGS::Types::DialogOption();
			newOption->ID = j + 1;
			newOption->Text = gcnew String(dialog[i].optionnames[j]);
			newOption->Say = !(dialog[i].optionflags[j] & DFLG_NOREPEAT);
			newOption->Show = (dialog[i].optionflags[j] & DFLG_ON);

			newDialog->Options->Add(newOption);
		}

		newDialog->Name = gcnew String(thisgame.dialogScriptNames[i]);
		newDialog->Script = gcnew String(dlgscript[i]);
		newDialog->ShowTextParser = (dialog[i].topicFlags & DTFLG_SHOWPARSER);

		game->Dialogs->Add(newDialog);
	}

	for (i = 0; i < thisgame.numcursors; i++)
	{
		AGS::Types::MouseCursor ^cursor = gcnew AGS::Types::MouseCursor();
		cursor->Animate = (thisgame.mcurs[i].view >= 0);
		cursor->AnimateOnlyOnHotspots = ((thisgame.mcurs[i].flags & MCF_HOTSPOT) != 0);
		cursor->AnimateOnlyWhenMoving = ((thisgame.mcurs[i].flags & MCF_ANIMMOVE) != 0);
		cursor->Image = thisgame.mcurs[i].pic;
		cursor->HotspotX = thisgame.mcurs[i].hotx;
		cursor->HotspotY = thisgame.mcurs[i].hoty;
		cursor->ID = i;
		cursor->Name = gcnew String(thisgame.mcurs[i].name);
		cursor->StandardMode = ((thisgame.mcurs[i].flags & MCF_STANDARD) != 0);
		cursor->View = thisgame.mcurs[i].view + 1;
		if (cursor->View < 1) cursor->View = 0;

		game->Cursors->Add(cursor);
	}

	for (i = 0; i < thisgame.numfonts; i++) 
	{
		AGS::Types::Font ^font = gcnew AGS::Types::Font();
		font->ID = i;
		font->OutlineFont = (thisgame.fontoutline[i] >= 0) ? thisgame.fontoutline[i] : 0;
		if (thisgame.fontoutline[i] == -1) 
		{
			font->OutlineStyle = FontOutlineStyle::None;
		}
		else if (thisgame.fontoutline[i] == FONT_OUTLINE_AUTO)
		{
			font->OutlineStyle = FontOutlineStyle::Automatic;
		}
		else 
		{
			font->OutlineStyle = FontOutlineStyle::UseOutlineFont;
		}
		font->PointSize = thisgame.fontflags[i] & FFLG_SIZEMASK;
		font->Name = gcnew String(String::Format("Font {0}", i));

		game->Fonts->Add(font);
	}

	for (i = 1; i < thisgame.numinvitems; i++)
	{
		InventoryItem^ invItem = gcnew InventoryItem();
    invItem->CursorImage = thisgame.invinfo[i].pic;
		invItem->Description = gcnew String(thisgame.invinfo[i].name);
		invItem->Image = thisgame.invinfo[i].pic;
		invItem->HotspotX = thisgame.invinfo[i].hotx;
		invItem->HotspotY = thisgame.invinfo[i].hoty;
		invItem->ID = i;
		invItem->Name = gcnew String(thisgame.invScriptNames[i]);
		invItem->PlayerStartsWithItem = (thisgame.invinfo[i].flags & IFLG_STARTWITH);

		ConvertCustomProperties(invItem->Properties, &thisgame.invProps[i]);

		char scriptFuncPrefix[100];
		sprintf(scriptFuncPrefix, "inventory%d_", i);
		ConvertInteractions(invItem->Interactions, thisgame.intrInv[i], gcnew String(scriptFuncPrefix), game, 5);

		game->InventoryItems->Add(invItem);
	}

	//AGS::Types::CustomPropertySchema ^schema = gcnew AGS::Types::CustomPropertySchema();
	for (i = 0; i < thisgame.propSchema.numProps; i++) 
	{
		CustomPropertySchemaItem ^schemaItem = gcnew CustomPropertySchemaItem();
		schemaItem->Name = gcnew String(thisgame.propSchema.propName[i]);
		schemaItem->Description = gcnew String(thisgame.propSchema.propDesc[i]);
		schemaItem->DefaultValue = gcnew String(thisgame.propSchema.defaultValue[i]);
		schemaItem->Type = (AGS::Types::CustomPropertyType)thisgame.propSchema.propType[i];

		game->PropertySchema->PropertyDefinitions->Add(schemaItem);
	}

	for (i = 0; i < thisgame.numgui; i++)
	{
		guis[i].rebuild_array();
	    guis[i].resort_zorder();

		GUI^ newGui;
		if (guis[i].is_textwindow()) 
		{
			newGui = gcnew TextWindowGUI();
			newGui->Controls->Clear();  // we'll add our own edges
      ((TextWindowGUI^)newGui)->TextColor = guis[i].fgcol;
		}
		else 
		{
			newGui = gcnew NormalGUI();
			((NormalGUI^)newGui)->Clickable = ((guis[i].flags & GUIF_NOCLICK) == 0);
			((NormalGUI^)newGui)->Top = guis[i].y;
			((NormalGUI^)newGui)->Left = guis[i].x;
			((NormalGUI^)newGui)->Width = (guis[i].wid > 0) ? guis[i].wid : 1;
			((NormalGUI^)newGui)->Height = (guis[i].hit > 0) ? guis[i].hit : 1;
			((NormalGUI^)newGui)->PopupYPos = guis[i].popupyp;
			((NormalGUI^)newGui)->Visibility = (GUIVisibility)guis[i].popup;
			((NormalGUI^)newGui)->ZOrder = guis[i].zorder;
			((NormalGUI^)newGui)->OnClick = gcnew String(guis[i].clickEventHandler);
      ((NormalGUI^)newGui)->BorderColor = guis[i].fgcol;
		}
		newGui->BackgroundColor = guis[i].bgcol;
		newGui->BackgroundImage = guis[i].bgpic;
		newGui->ID = i;
		newGui->Name = gcnew String(guis[i].get_objscript_name(guis[i].name));

		for (int j = 0; j < guis[i].numobjs; j++)
		{
			GUIObject* curObj = guis[i].objs[j];
			GUIControl ^newControl = nullptr;
			switch (guis[i].objrefptr[j] >> 16)
			{
			case GOBJ_BUTTON:
				{
				if (guis[i].is_textwindow())
				{
					AGS::Types::GUITextWindowEdge^ edge = gcnew AGS::Types::GUITextWindowEdge();
					::GUIButton *copyFrom = (::GUIButton*)curObj;
					newControl = edge;
					edge->Image = copyFrom->pic;
				}
				else
				{
					AGS::Types::GUIButton^ newButton = gcnew AGS::Types::GUIButton();
					::GUIButton *copyFrom = (::GUIButton*)curObj;
					newControl = newButton;
					newButton->TextColor = copyFrom->textcol;
					newButton->Font = copyFrom->font;
					newButton->Image = copyFrom->pic;
					newButton->MouseoverImage = copyFrom->overpic;
					newButton->PushedImage = copyFrom->pushedpic;
					newButton->TextAlignment = (TextAlignment)copyFrom->textAlignment;
					newButton->ClickAction = (GUIClickAction)copyFrom->leftclick;
					newButton->NewModeNumber = copyFrom->lclickdata;
					newButton->ClipImage = (copyFrom->flags & GUIF_CLIP) ? true : false;
					newButton->Text = gcnew String(copyFrom->text);
					newButton->OnClick = gcnew String(copyFrom->eventHandlers[0]);
				}
				break;
				}
			case GOBJ_LABEL:
				{
				AGS::Types::GUILabel^ newLabel = gcnew AGS::Types::GUILabel();
				::GUILabel *copyFrom = (::GUILabel*)curObj;
				newControl = newLabel;
				newLabel->TextColor = copyFrom->textcol;
				newLabel->Font = copyFrom->font;
				newLabel->TextAlignment = (LabelTextAlignment)copyFrom->align;
				newLabel->Text = gcnew String(copyFrom->GetText());
				break;
				}
			case GOBJ_TEXTBOX:
				{
				  AGS::Types::GUITextBox^ newTextbox = gcnew AGS::Types::GUITextBox();
				  ::GUITextBox *copyFrom = (::GUITextBox*)curObj;
				  newControl = newTextbox;
				  newTextbox->TextColor = copyFrom->textcol;
				  newTextbox->Font = copyFrom->font;
				  newTextbox->ShowBorder = (copyFrom->exflags & GTF_NOBORDER) ? false : true;
				  newTextbox->Text = gcnew String(copyFrom->text);
				  newTextbox->OnActivate = gcnew String(copyFrom->eventHandlers[0]);
				  break;
				}
			case GOBJ_LISTBOX:
				{
				  AGS::Types::GUIListBox^ newListbox = gcnew AGS::Types::GUIListBox();
				  ::GUIListBox *copyFrom = (::GUIListBox*)curObj;
				  newControl = newListbox;
				  newListbox->TextColor = copyFrom->textcol;
				  newListbox->Font = copyFrom->font; 
				  newListbox->SelectedTextColor = copyFrom->backcol;
				  newListbox->SelectedBackgroundColor = copyFrom->selectedbgcol;
				  newListbox->TextAlignment = (ListBoxTextAlignment)copyFrom->alignment;
				  newListbox->ShowBorder = ((copyFrom->exflags & GLF_NOBORDER) == 0);
				  newListbox->ShowScrollArrows = ((copyFrom->exflags & GLF_NOARROWS) == 0);
				  newListbox->OnSelectionChanged = gcnew String(copyFrom->eventHandlers[0]);
				  break;
				}
			case GOBJ_SLIDER:
				{
				  AGS::Types::GUISlider^ newSlider = gcnew AGS::Types::GUISlider();
				  ::GUISlider *copyFrom = (::GUISlider*)curObj;
				  newControl = newSlider;
				  newSlider->MinValue = copyFrom->min;
				  newSlider->MaxValue = copyFrom->max;
				  newSlider->Value = copyFrom->value;
				  newSlider->HandleImage = copyFrom->handlepic;
			  	  newSlider->HandleOffset = copyFrom->handleoffset;
				  newSlider->BackgroundImage = copyFrom->bgimage;
				  newSlider->OnChange = gcnew String(copyFrom->eventHandlers[0]);
				  break;
				}
			case GOBJ_INVENTORY:
				{
					AGS::Types::GUIInventory^ invwindow = gcnew AGS::Types::GUIInventory();
				    ::GUIInv *copyFrom = (::GUIInv*)curObj;
				    newControl = invwindow;
					invwindow->CharacterID = copyFrom->charId;
					invwindow->ItemWidth = copyFrom->itemWidth;
					invwindow->ItemHeight = copyFrom->itemHeight;
					break;
				}
			default:
				throw gcnew AGSEditorException("Unknown control type found: " + (guis[i].objrefptr[j] >> 16));
			}
			newControl->Width = (curObj->wid > 0) ? curObj->wid : 1;
			newControl->Height = (curObj->hit > 0) ? curObj->hit : 1;
			newControl->Left = curObj->x;
			newControl->Top = curObj->y;
			newControl->ZOrder = curObj->zorder;
			newControl->ID = j;
			newControl->Name = gcnew String(curObj->scriptName);
			newGui->Controls->Add(newControl);
		}
		
		game->GUIs->Add(newGui);
	}

	free_old_game_data();

	return game;
}

System::String ^load_room_script(System::String ^fileName)
{
	char roomFileNameBuffer[MAX_PATH];
	ConvertStringToCharArray(fileName, roomFileNameBuffer);

	FILE *opty = clibfopen(roomFileNameBuffer, "rb");
	if (opty == NULL) throw gcnew AGSEditorException("Unable to open room file");

	short version = getshort(opty);
	if (version < 17)
	{
    fclose(opty);
		throw gcnew AGSEditorException("Room file is from an old version of AGS and cannot be processed");
	}

	String ^scriptToReturn = nullptr;

	int thisblock = 0;
	while (thisblock != BLOCKTYPE_EOF) 
	{
		thisblock = fgetc(opty);
		if (thisblock == BLOCKTYPE_EOF) 
		{
			break;
		}

		int blockLen = getw(opty);

		if (thisblock == BLOCKTYPE_SCRIPT) 
		{
			int lee = getw(opty);
			int hh;

			char *scriptFile = (char *)malloc(lee + 5);
			fread(scriptFile, sizeof(char), lee, opty);
			scriptFile[lee] = 0;

			for (hh = 0; hh < lee; hh++)
			  scriptFile[hh] += passwencstring[hh % 11];

			scriptToReturn = gcnew String(scriptFile, 0, strlen(scriptFile), System::Text::Encoding::Default);
			free(scriptFile);
			break;
		}
		else 
		{
			fseek(opty, blockLen, SEEK_CUR);
		}
	}

	fclose(opty);

	return scriptToReturn;
}

int GetCurrentlyLoadedRoomNumber()
{
  return loaded_room_number;
}

AGS::Types::Room^ load_crm_file(UnloadedRoom ^roomToLoad)
{
	char roomFileNameBuffer[MAX_PATH];
	ConvertStringToCharArray(roomToLoad->FileName, roomFileNameBuffer);

	const char *errorMsg = load_room_file(roomFileNameBuffer);
	if (errorMsg != NULL) 
	{
		throw gcnew AGSEditorException(gcnew String(errorMsg));
	}

  loaded_room_number = roomToLoad->Number;

	Room ^room = gcnew Room(roomToLoad->Number);
	room->Description = roomToLoad->Description;
	room->Script = roomToLoad->Script;
	room->BottomEdgeY = thisroom.bottom;
	room->LeftEdgeX = thisroom.left;
	room->MusicVolumeAdjustment = (RoomVolumeAdjustment)thisroom.options[ST_VOLUME];
	room->PlayerCharacterView = thisroom.options[ST_MANVIEW];
	room->PlayMusicOnRoomLoad = thisroom.options[ST_TUNE];
	room->RightEdgeX = thisroom.right;
	room->SaveLoadEnabled = (thisroom.options[ST_SAVELOAD] == 0);
	room->ShowPlayerCharacter = (thisroom.options[ST_MANDISABLED] == 0);
	room->TopEdgeY = thisroom.top;
	room->Width = thisroom.width;
	room->Height = thisroom.height;
  if (thisroom.resolution > 2)
  {
    room->Resolution = RoomResolution::HighRes;
  }
  else
  {
  	room->Resolution = (RoomResolution)thisroom.resolution;
  }
	room->ColorDepth = bitmap_color_depth(thisroom.ebscene[0]);
	room->BackgroundAnimationDelay = thisroom.bscene_anim_speed;
	room->BackgroundCount = thisroom.num_bscenes;

	int i;
	for (i = 0; i < thisroom.numLocalVars; i++)
	{
		OldInteractionVariable ^intVar;
		intVar = gcnew OldInteractionVariable(gcnew String(thisroom.localvars[i].name), thisroom.localvars[i].value);
		room->OldInteractionVariables->Add(intVar);
	}

	for (i = 0; i < thisroom.nummes; i++) 
	{
		RoomMessage ^newMessage = gcnew RoomMessage(i);
		newMessage->Text = gcnew String(thisroom.message[i]);
		newMessage->ShowAsSpeech = (thisroom.msgi[i].displayas > 0);
		newMessage->CharacterID = (thisroom.msgi[i].displayas - 1);
		newMessage->DisplayNextMessageAfter = ((thisroom.msgi[i].flags & MSG_DISPLAYNEXT) != 0);
		newMessage->AutoRemoveAfterTime = ((thisroom.msgi[i].flags & MSG_TIMELIMIT) != 0);
		room->Messages->Add(newMessage);
	}

	for (i = 0; i < thisroom.numsprs; i++) 
	{
		char jibbledScriptName[50] = "\0";
		if (strlen(thisroom.objectscriptnames[i]) > 0) 
		{
			if (thisroom.wasversion < 26)
			{
				sprintf(jibbledScriptName, "o%s", thisroom.objectscriptnames[i]);
				strlwr(jibbledScriptName);
				jibbledScriptName[1] = toupper(jibbledScriptName[1]);
			}
			else 
			{			
				strcpy(jibbledScriptName, thisroom.objectscriptnames[i]);
			}
		}

		RoomObject ^obj = gcnew RoomObject(room);
		obj->ID = i;
		obj->Image = thisroom.sprs[i].sprnum;
		obj->StartX = thisroom.sprs[i].x;
		obj->StartY = thisroom.sprs[i].y;
    if (thisroom.wasversion <= 26)
      obj->StartY += GetSpriteHeight(thisroom.sprs[i].sprnum);
		obj->Visible = (thisroom.sprs[i].on != 0);
		obj->Baseline = thisroom.objbaseline[i];
		obj->Name = gcnew String(jibbledScriptName);
		obj->Description = gcnew String(thisroom.objectnames[i]);
		obj->UseRoomAreaScaling = ((thisroom.objectFlags[i] & OBJF_USEROOMSCALING) != 0);
		obj->UseRoomAreaLighting = ((thisroom.objectFlags[i] & OBJF_USEREGIONTINTS) != 0);
		ConvertCustomProperties(obj->Properties, &thisroom.objProps[i]);

		if (thisroom.wasversion < 26)
		{
			char scriptFuncPrefix[100];
			sprintf(scriptFuncPrefix, "object%d_", i);
			ConvertInteractions(obj->Interactions, thisroom.intrObject[i], gcnew String(scriptFuncPrefix), nullptr, 2);
		}
		else 
		{
			CopyInteractions(obj->Interactions, thisroom.objectScripts[i]);
		}

		room->Objects->Add(obj);
	}

	for (i = 0; i < thisroom.numhotspots; i++) 
	{
		RoomHotspot ^hotspot = room->Hotspots[i];
		hotspot->ID = i;
		hotspot->Description = gcnew String(thisroom.hotspotnames[i]);
		hotspot->Name = (gcnew String(thisroom.hotspotScriptNames[i]))->Trim();
		hotspot->WalkToPoint = Point(thisroom.hswalkto[i].x, thisroom.hswalkto[i].y);
		ConvertCustomProperties(hotspot->Properties, &thisroom.hsProps[i]);

		if (thisroom.wasversion < 26)
		{
			char scriptFuncPrefix[100];
			sprintf(scriptFuncPrefix, "hotspot%d_", i);
			ConvertInteractions(hotspot->Interactions, thisroom.intrHotspot[i], gcnew String(scriptFuncPrefix), nullptr, 1);
		}
		else 
		{
			CopyInteractions(hotspot->Interactions, thisroom.hotspotScripts[i]);
		}
	}

	for (i = 0; i <= MAX_WALK_AREAS; i++) 
	{
		RoomWalkableArea ^area = room->WalkableAreas[i];
		area->ID = i;
		area->AreaSpecificView = thisroom.shadinginfo[i];
		area->UseContinuousScaling = !(thisroom.walk_area_zoom2[i] == NOT_VECTOR_SCALED);
		area->ScalingLevel = thisroom.walk_area_zoom[i] + 100;
		area->MinScalingLevel = thisroom.walk_area_zoom[i] + 100;
		if (area->UseContinuousScaling) 
		{
			area->MaxScalingLevel = thisroom.walk_area_zoom2[i] + 100;
		}
		else
		{
			area->MaxScalingLevel = area->MinScalingLevel;
		}
	}

	for (i = 0; i < MAX_OBJ; i++) 
	{
		RoomWalkBehind ^area = room->WalkBehinds[i];
		area->ID = i;
		area->Baseline = thisroom.objyval[i];
	}

	for (i = 0; i < MAX_REGIONS; i++) 
	{
		RoomRegion ^area = room->Regions[i];
		area->ID = i;
		area->UseColourTint = ((thisroom.regionTintLevel[i] & TINT_IS_ENABLED) != 0);
		area->LightLevel = thisroom.regionLightLevel[i] + 100;
		area->BlueTint = (thisroom.regionTintLevel[i] >> 16) & 0x00ff;
		area->GreenTint = (thisroom.regionTintLevel[i] >> 8) & 0x00ff;
		area->RedTint = thisroom.regionTintLevel[i] & 0x00ff;
		area->TintSaturation = (thisroom.regionLightLevel[i] > 0) ? thisroom.regionLightLevel[i] : 50;

		if (thisroom.wasversion < 26)
		{
			char scriptFuncPrefix[100];
			sprintf(scriptFuncPrefix, "region%d_", i);
			ConvertInteractions(area->Interactions, thisroom.intrRegion[i], gcnew String(scriptFuncPrefix), nullptr, 0);
		}
		else 
		{
			CopyInteractions(area->Interactions, thisroom.regionScripts[i]);
		}
	}
/*
	if (thisroom.scripts != NULL) 
	{
		room->Script->Text = gcnew String(thisroom.scripts);
	}
*/
	room->_roomStructPtr = (IntPtr)&thisroom;

	ConvertCustomProperties(room->Properties, &thisroom.roomProps);

	if (thisroom.wasversion < 26)
	{
		ConvertInteractions(room->Interactions, thisroom.intrRoom, "room_", nullptr, 0);
	}
	else 
	{
		CopyInteractions(room->Interactions, thisroom.roomScripts);
	}

	room->GameID = thisroom.gameId;
  clear_undo_buffer();

	return room;
}

void save_crm_file(Room ^room)
{
	thisroom.gameId = room->GameID;
	thisroom.bottom = room->BottomEdgeY;
	thisroom.left = room->LeftEdgeX;
	thisroom.options[ST_VOLUME] = (int)room->MusicVolumeAdjustment;
	thisroom.options[ST_MANVIEW] = room->PlayerCharacterView;
	thisroom.options[ST_TUNE] = room->PlayMusicOnRoomLoad;
	thisroom.right = room->RightEdgeX;
	thisroom.options[ST_SAVELOAD] = room->SaveLoadEnabled ? 0 : 1;
	thisroom.options[ST_MANDISABLED] = room->ShowPlayerCharacter ? 0 : 1;
	thisroom.top = room->TopEdgeY;
	thisroom.width = room->Width;
	thisroom.height = room->Height;
	thisroom.resolution = (int)room->Resolution;
	thisroom.bscene_anim_speed = room->BackgroundAnimationDelay;
	thisroom.num_bscenes = room->BackgroundCount;

	int i;
	for (i = 0; i < thisroom.nummes; i++) 
	{
		free(thisroom.message[i]);
	}
	thisroom.nummes = room->Messages->Count;
	for (i = 0; i < thisroom.nummes; i++) 
	{
		RoomMessage ^newMessage = room->Messages[i];
		thisroom.message[i] = (char*)malloc(newMessage->Text->Length + 1);
		ConvertStringToCharArray(newMessage->Text, thisroom.message[i]);
		if (newMessage->ShowAsSpeech)
		{
			thisroom.msgi[i].displayas = newMessage->CharacterID + 1;
		}
		else
		{
			thisroom.msgi[i].displayas = 0;
		}
		thisroom.msgi[i].flags = 0;
		if (newMessage->DisplayNextMessageAfter) thisroom.msgi[i].flags |= MSG_DISPLAYNEXT;
		if (newMessage->AutoRemoveAfterTime) thisroom.msgi[i].flags |= MSG_TIMELIMIT;
	}

	thisroom.numsprs = room->Objects->Count;
	for (i = 0; i < thisroom.numsprs; i++) 
	{
		RoomObject ^obj = room->Objects[i];
		ConvertStringToCharArray(obj->Name, thisroom.objectscriptnames[i]);

		thisroom.sprs[i].sprnum = obj->Image;
		thisroom.sprs[i].x = obj->StartX;
		thisroom.sprs[i].y = obj->StartY;
		thisroom.sprs[i].on = obj->Visible;
		thisroom.objbaseline[i] = obj->Baseline;
		ConvertStringToCharArray(obj->Description, thisroom.objectnames[i], 30);
		thisroom.objectFlags[i] = 0;
		if (obj->UseRoomAreaScaling) thisroom.objectFlags[i] |= OBJF_USEROOMSCALING;
		if (obj->UseRoomAreaLighting) thisroom.objectFlags[i] |= OBJF_USEREGIONTINTS;
		CompileCustomProperties(obj->Properties, &thisroom.objProps[i]);
	}

	thisroom.numhotspots = room->Hotspots->Count;
	for (i = 0; i < thisroom.numhotspots; i++) 
	{
		RoomHotspot ^hotspot = room->Hotspots[i];
		thisroom.hotspotnames[i] = (char*)malloc(hotspot->Description->Length + 1);
		ConvertStringToCharArray(hotspot->Description, thisroom.hotspotnames[i]);
		ConvertStringToCharArray(hotspot->Name, thisroom.hotspotScriptNames[i], 20);
		thisroom.hswalkto[i].x = hotspot->WalkToPoint.X;
		thisroom.hswalkto[i].y = hotspot->WalkToPoint.Y;
		CompileCustomProperties(hotspot->Properties, &thisroom.hsProps[i]);
	}

	for (i = 0; i <= MAX_WALK_AREAS; i++) 
	{
		RoomWalkableArea ^area = room->WalkableAreas[i];
		thisroom.shadinginfo[i] = area->AreaSpecificView;
		
		if (area->UseContinuousScaling) 
		{
			thisroom.walk_area_zoom[i] = area->MinScalingLevel - 100;
			thisroom.walk_area_zoom2[i] = area->MaxScalingLevel - 100;
		}
		else
		{
			thisroom.walk_area_zoom[i] = area->ScalingLevel - 100;
			thisroom.walk_area_zoom2[i] = NOT_VECTOR_SCALED;
		}
	}

	for (i = 0; i < MAX_OBJ; i++) 
	{
		RoomWalkBehind ^area = room->WalkBehinds[i];
		thisroom.objyval[i] = area->Baseline;
	}

	for (i = 0; i < MAX_REGIONS; i++) 
	{
		RoomRegion ^area = room->Regions[i];
		thisroom.regionTintLevel[i] = 0;
		if (area->UseColourTint) 
		{
			thisroom.regionTintLevel[i] = TINT_IS_ENABLED;
			thisroom.regionTintLevel[i] |= area->RedTint | (area->GreenTint << 8) | (area->BlueTint << 16);
			thisroom.regionLightLevel[i] = area->TintSaturation;
		}
		else 
		{
			thisroom.regionLightLevel[i] = area->LightLevel - 100;
		}
	}

	CompileCustomProperties(room->Properties, &thisroom.roomProps);

	thisroom.scripts = NULL;
	thisroom.compiled_script = ((AGS::Native::CompiledScript^)room->Script->CompiledData)->Data;

	char roomFileNameBuffer[MAX_PATH];
	ConvertStringToCharArray(room->FileName, roomFileNameBuffer);

	TempDataStorage::RoomBeingSaved = room;

	save_room_file(roomFileNameBuffer);

	TempDataStorage::RoomBeingSaved = nullptr;

	for (i = 0; i < thisroom.numhotspots; i++) 
	{
		free(thisroom.hotspotnames[i]);
		thisroom.hotspotnames[i] = NULL;
	}
}

static int CountViews(ViewFolder ^folder) 
{
	int highestViewNumber = 0;
	for each (ViewFolder ^subFolder in folder->SubFolders)
	{
		int folderView = CountViews(subFolder);
		if (folderView > highestViewNumber) 
		{
			highestViewNumber = folderView;
		}
	}
	for each (View ^view in folder->Views)
	{
		if (view->ID > highestViewNumber)
		{
			highestViewNumber = view->ID;
		}
	}
	return highestViewNumber;
}

static void ConvertViewsToDTAFormat(ViewFolder ^folder, Game ^game) 
{
	for each (ViewFolder ^subFolder in folder->SubFolders)
	{
		ConvertViewsToDTAFormat(subFolder, game);
	}

	for each (View ^view in folder->Views)
	{
		int i = view->ID - 1;
		ConvertStringToCharArray(view->Name, thisgame.viewNames[i], MAXVIEWNAMELENGTH);

		newViews[i].Initialize(view->Loops->Count);
		for (int j = 0; j < newViews[i].numLoops; j++) 
		{
      newViews[i].loops[j].Initialize(view->Loops[j]->Frames->Count);
			for (int k = 0; k < newViews[i].loops[j].numFrames; k++) 
			{
				AGS::Types::ViewFrame ^frame = view->Loops[j]->Frames[k];
        int frameSound = -1;
        if (frame->Sound > 0) 
          frameSound = game->GetAudioArrayIndexFromAudioClipIndex(frame->Sound);

				newViews[i].loops[j].frames[k].flags = (frame->Flipped) ? VFLG_FLIPSPRITE : 0;
				newViews[i].loops[j].frames[k].pic = frame->Image;
        newViews[i].loops[j].frames[k].sound = frameSound;
				newViews[i].loops[j].frames[k].speed = frame->Delay;
			}
			
			if (view->Loops[j]->RunNextLoop)
			{
        newViews[i].loops[j].flags = LOOPFLAG_RUNNEXTLOOP;
			}
		}

	}
}

void write_compiled_script(FILE *ooo, Script ^script)
{
	if (script->CompiledData == nullptr)
	{
		throw gcnew CompileError(String::Format("Script has not been compiled: {0}", script->FileName));
	}

	fwrite_script(((AGS::Native::CompiledScript^)script->CompiledData)->Data, ooo);
}

void serialize_interaction_scripts(Interactions ^interactions, FILE *ooo)
{
	char textBuffer[256];
	putw(interactions->ScriptFunctionNames->Length, ooo);
	for each (String^ funcName in interactions->ScriptFunctionNames)
	{
		if (funcName == nullptr)
		{
			fputc(0, ooo);
		}
		else 
		{
			ConvertStringToCharArray(funcName, textBuffer, 256);
			fputstring(textBuffer, ooo);
		}
	}
}

void serialize_room_interactions(FILE *ooo) 
{
	Room ^roomBeingSaved = TempDataStorage::RoomBeingSaved;
	serialize_interaction_scripts(roomBeingSaved->Interactions, ooo);
	for each (RoomHotspot ^hotspot in roomBeingSaved->Hotspots) 
	{
		serialize_interaction_scripts(hotspot->Interactions, ooo);
	}
	for each (RoomObject ^obj in roomBeingSaved->Objects) 
	{
		serialize_interaction_scripts(obj->Interactions, ooo);
	}
	for each (RoomRegion ^region in roomBeingSaved->Regions) 
	{
		serialize_interaction_scripts(region->Interactions, ooo);
	}
}

void save_thisgame_to_file(const char *fileName, Game ^game)
{
	const char *AGS_VERSION = "3.2.0";
  char textBuffer[500];
	int bb;

	FILE*ooo = fopen(fileName, "wb");
	if (ooo == NULL) 
	{
		throw gcnew CompileError(String::Format("Cannot open file {0} for writing", gcnew String(fileName)));
	}

  fwrite(game_file_sig,30,1,ooo);
  putw(42, ooo);
  putw(strlen(AGS_VERSION), ooo);
  fwrite(AGS_VERSION, strlen(AGS_VERSION), 1, ooo);

  fwrite(&thisgame, sizeof (GameSetupStructBase), 1, ooo);
  fwrite(&thisgame.guid[0], 1, MAX_GUID_LENGTH, ooo);
  fwrite(&thisgame.saveGameFileExtension[0], 1, MAX_SG_EXT_LENGTH, ooo);
  fwrite(&thisgame.saveGameFolderName[0], 1, MAX_SG_FOLDER_LEN, ooo);
  fwrite(&thisgame.fontflags[0], 1, thisgame.numfonts, ooo);
  fwrite(&thisgame.fontoutline[0], 1, thisgame.numfonts, ooo);
  putw (MAX_SPRITES, ooo);
  fwrite(&thisgame.spriteflags[0], 1, MAX_SPRITES, ooo);
  fwrite(&thisgame.invinfo[0], sizeof(InventoryItemInfo), thisgame.numinvitems, ooo);
  fwrite(&thisgame.mcurs[0], sizeof(::MouseCursor), thisgame.numcursors, ooo);
  
  for (bb = 0; bb < thisgame.numcharacters; bb++)
  {
    serialize_interaction_scripts(game->Characters[bb]->Interactions, ooo);
  }
  for (bb = 1; bb < thisgame.numinvitems; bb++)
  {
    serialize_interaction_scripts(game->InventoryItems[bb - 1]->Interactions, ooo);
  }

  if (thisgame.dict != NULL) {
    write_dictionary (thisgame.dict, ooo);
  }

  write_compiled_script(ooo, game->ScriptsToCompile->GetScriptByFilename(Script::GLOBAL_SCRIPT_FILE_NAME));
  write_compiled_script(ooo, game->ScriptsToCompile->GetScriptByFilename(Script::DIALOG_SCRIPTS_FILE_NAME));

  // Extract all the scripts we want to persist (all the non-headers, except
  // the global script which was already written)
  List<AGS::Types::Script^>^ scriptsToWrite = gcnew List<AGS::Types::Script^>();
  for each (Script ^script in game->ScriptsToCompile)
  {
	  if ((!script->IsHeader) && 
      (!script->FileName->Equals(Script::GLOBAL_SCRIPT_FILE_NAME)) &&
      (!script->FileName->Equals(Script::DIALOG_SCRIPTS_FILE_NAME)))
	  {
		  scriptsToWrite->Add(script);
	  }
  }

  putw(scriptsToWrite->Count, ooo);

  for each (Script ^script in scriptsToWrite)
  {
	  write_compiled_script(ooo, script);
  }

  for (bb = 0; bb < thisgame.numviews; bb++)
  {
    newViews[bb].WriteToFile(ooo);
  }

  fwrite(&thisgame.chars[0],sizeof(CharacterInfo),thisgame.numcharacters,ooo);

  fwrite(&thisgame.lipSyncFrameLetters[0][0], MAXLIPSYNCFRAMES, 50, ooo);

  char *buffer;
  for (bb=0;bb<MAXGLOBALMES;bb++) 
  {
	  if ((game->GlobalMessages[bb] == nullptr) || (game->GlobalMessages[bb]->Length == 0))
		  continue;
      
	  buffer = (char*)malloc(game->GlobalMessages[bb]->Length + 1);
	  ConvertStringToCharArray(game->GlobalMessages[bb], buffer);
    write_string_encrypt(ooo, buffer);
	  free(buffer);
  }
  fwrite(&dialog[0], sizeof(DialogTopic), thisgame.numdialog, ooo);
  write_gui(ooo,&guis[0],&thisgame);
  write_plugins_to_disk(ooo);
  // write the custom properties & schema
  thisgame.propSchema.Serialize(ooo);
  for (bb = 0; bb < thisgame.numcharacters; bb++)
    thisgame.charProps[bb].Serialize (ooo);
  for (bb = 0; bb < thisgame.numinvitems; bb++)
    thisgame.invProps[bb].Serialize (ooo);

  for (bb = 0; bb < thisgame.numviews; bb++)
    fputstring(thisgame.viewNames[bb], ooo);

  for (bb = 0; bb < thisgame.numinvitems; bb++)
    fputstring(thisgame.invScriptNames[bb], ooo);

  for (bb = 0; bb < thisgame.numdialog; bb++)
    fputstring(thisgame.dialogScriptNames[bb], ooo);


  int audioClipTypeCount = game->AudioClipTypes->Count + 1;
  putw(audioClipTypeCount, ooo);
  ::AudioClipType *clipTypes = (::AudioClipType*)calloc(audioClipTypeCount, sizeof(::AudioClipType));
  // hard-coded SPEECH audio type 0
  clipTypes[0].id = 0;
  clipTypes[0].reservedChannels = 1;
  clipTypes[0].volume_reduction_while_speech_playing = 0;
  clipTypes[0].crossfadeSpeed = 0;

  for (bb = 1; bb < audioClipTypeCount; bb++)
  {
    clipTypes[bb].id = bb;
    clipTypes[bb].reservedChannels = game->AudioClipTypes[bb - 1]->MaxChannels;
    clipTypes[bb].volume_reduction_while_speech_playing = game->AudioClipTypes[bb - 1]->VolumeReductionWhileSpeechPlaying;
    clipTypes[bb].crossfadeSpeed = (int)game->AudioClipTypes[bb - 1]->CrossfadeClips;
  }
  fwrite(clipTypes, sizeof(::AudioClipType), audioClipTypeCount, ooo);
  free(clipTypes);

  IList<AudioClip^>^ allClips = game->CachedAudioClipListForCompile;
  putw(allClips->Count, ooo);
  ScriptAudioClip *compiledAudioClips = (ScriptAudioClip*)calloc(allClips->Count, sizeof(ScriptAudioClip));
  for (int i = 0; i < allClips->Count; i++)
  {
    AudioClip^ clip = allClips[i];
    ConvertStringToCharArray(clip->ScriptName, compiledAudioClips[i].scriptName, 30);
	  ConvertStringToCharArray(clip->CacheFileNameWithoutPath, compiledAudioClips[i].fileName, 15);
    compiledAudioClips[i].bundlingType = (int)clip->BundlingType;
    compiledAudioClips[i].defaultPriority = (int)clip->ActualPriority;
    compiledAudioClips[i].defaultRepeat = (clip->ActualRepeat) ? 1 : 0;
    compiledAudioClips[i].defaultVolume = clip->ActualVolume;
    compiledAudioClips[i].fileType = (int)clip->FileType;
    compiledAudioClips[i].type = clip->Type;
  }
  fwrite(compiledAudioClips, sizeof(ScriptAudioClip), allClips->Count, ooo);
  free(compiledAudioClips);
  putw(game->GetAudioArrayIndexFromAudioClipIndex(game->Settings->PlaySoundOnScore), ooo);

  if (game->Settings->DebugMode)
  {
    putw(game->Rooms->Count, ooo);
    for (bb = 0; bb < game->Rooms->Count; bb++)
    {
      IRoom ^room = game->Rooms[bb];
      putw(room->Number, ooo);
      if (room->Description != nullptr)
      {
        ConvertStringToCharArray(room->Description, textBuffer, 500);
      }
      else
      {
        textBuffer[0] = 0;
      }
      fputstring(textBuffer, ooo);
    }
  }

  fclose(ooo);
}

void save_game_to_dta_file(Game^ game, const char *fileName)
{
	thisgame.options[OPT_ALWAYSSPCH] = game->Settings->AlwaysDisplayTextAsSpeech;
	thisgame.options[OPT_ANTIALIASFONTS] = game->Settings->AntiAliasFonts;
	thisgame.options[OPT_ANTIGLIDE] = game->Settings->AntiGlideMode;
	thisgame.options[OPT_NOWALKMODE] = !game->Settings->AutoMoveInWalkMode;
	thisgame.options[OPT_RIGHTLEFTWRITE] = game->Settings->BackwardsText;
	thisgame.color_depth = (int)game->Settings->ColorDepth;
	thisgame.options[OPT_COMPRESSSPRITES] = game->Settings->CompressSprites;
	thisgame.options[OPT_CROSSFADEMUSIC] = (int)game->Settings->CrossfadeMusic;
	thisgame.options[OPT_DEBUGMODE] = game->Settings->DebugMode;
	thisgame.options[OPT_DIALOGUPWARDS] = game->Settings->DialogOptionsBackwards;
	thisgame.options[OPT_DIALOGGAP] = game->Settings->DialogOptionsGap;
	thisgame.options[OPT_DIALOGIFACE] = game->Settings->DialogOptionsGUI;
	thisgame.dialog_bullet = game->Settings->DialogOptionsBullet;
	thisgame.options[OPT_DUPLICATEINV] = game->Settings->DisplayMultipleInventory;
	thisgame.options[OPT_STRICTSTRINGS] = game->Settings->EnforceNewStrings;
	thisgame.options[OPT_STRICTSCRIPTING] = game->Settings->EnforceObjectBasedScript;
	thisgame.options[OPT_NOSCALEFNT] = game->Settings->FontsForHiRes;
	ConvertStringToCharArray(game->Settings->GameName, thisgame.gamename, 50);
	thisgame.options[OPT_NEWGUIALPHA] = (int)game->Settings->GUIAlphaStyle;
	thisgame.options[OPT_HANDLEINVCLICKS] = game->Settings->HandleInvClicksInScript;
	thisgame.options[OPT_FIXEDINVCURSOR] = !game->Settings->InventoryCursors;
  thisgame.options[OPT_OLDTALKANIMSPD] = game->Settings->LegacySpeechAnimationSpeed;
	thisgame.options[OPT_LEFTTORIGHTEVAL] = game->Settings->LeftToRightPrecedence;
	thisgame.options[OPT_LETTERBOX] = game->Settings->LetterboxMode;
  thisgame.totalscore = game->Settings->MaximumScore;
	thisgame.options[OPT_MOUSEWHEEL] = game->Settings->MouseWheelEnabled;
	thisgame.options[OPT_DIALOGNUMBERED] = game->Settings->NumberDialogOptions;
	thisgame.options[OPT_PIXPERFECT] = game->Settings->PixelPerfect;
	thisgame.options[OPT_SCORESOUND] = 0; // saved elsewhere now to make it 32-bit
	thisgame.default_resolution = (int)game->Settings->Resolution;
	thisgame.options[OPT_FADETYPE] = (int)game->Settings->RoomTransition;
	thisgame.options[OPT_RUNGAMEDLGOPTS] = game->Settings->RunGameLoopsWhileDialogOptionsDisplayed;
	thisgame.options[OPT_SAVESCREENSHOT] = game->Settings->SaveScreenshots;
	thisgame.options[OPT_NOSKIPTEXT] = (int)game->Settings->SkipSpeech;
	thisgame.options[OPT_PORTRAITSIDE] = (int)game->Settings->SpeechPortraitSide;
	thisgame.options[OPT_SPEECHTYPE] = (int)game->Settings->SpeechStyle;
	thisgame.options[OPT_SPLITRESOURCES] = game->Settings->SplitResources;
	thisgame.options[OPT_TWCUSTOM] = game->Settings->TextWindowGUI;
	thisgame.options[OPT_THOUGHTGUI] = game->Settings->ThoughtGUI;
	thisgame.options[OPT_TURNTOFACELOC] = game->Settings->TurnBeforeFacing;
	thisgame.options[OPT_ROTATECHARS] = game->Settings->TurnBeforeWalking;
	thisgame.options[OPT_NATIVECOORDINATES] = !game->Settings->UseLowResCoordinatesInScript;
	thisgame.options[OPT_WALKONLOOK] = game->Settings->WalkInLookMode;
	thisgame.options[OPT_DISABLEOFF] = (int)game->Settings->WhenInterfaceDisabled;
	thisgame.uniqueid = game->Settings->UniqueID;
  ConvertStringToCharArray(game->Settings->GUIDAsString, thisgame.guid, MAX_GUID_LENGTH);
  ConvertStringToCharArray(game->Settings->SaveGameFolderName, thisgame.saveGameFolderName, MAX_SG_FOLDER_LEN);

  if (game->Settings->EnhancedSaveGames)
  {
    ConvertStringToCharArray(game->Settings->SaveGameFileExtension, thisgame.saveGameFileExtension, MAX_SG_EXT_LENGTH);
  }
  else 
  {
    thisgame.saveGameFileExtension[0] = 0;
  }

	thisgame.hotdot = game->Settings->InventoryHotspotMarker->DotColor;
	thisgame.hotdotouter = game->Settings->InventoryHotspotMarker->CrosshairColor;
	thisgame.invhotdotsprite = game->Settings->InventoryHotspotMarker->Image;
	if (game->Settings->InventoryHotspotMarker->Style == InventoryHotspotMarkerStyle::Crosshair)
	{
		thisgame.invhotdotsprite = 0;
	}
	else if (game->Settings->InventoryHotspotMarker->Style == InventoryHotspotMarkerStyle::None)
	{
		thisgame.invhotdotsprite = 0;
		thisgame.hotdot = 0;
	}

	// ** Palette **
	int i;
	for (i = 0; i < 256; i++) 
	{
		if (game->Palette[i]->ColourType == PaletteColourType::Background) 
		{
			thisgame.paluses[i] = PAL_BACKGROUND;
		}
		else 
		{
			thisgame.paluses[i] = PAL_GAMEWIDE;
		}
		palette[i].r = game->Palette[i]->Colour.R / 4;
		palette[i].g = game->Palette[i]->Colour.G / 4;
		palette[i].b = game->Palette[i]->Colour.B / 4;
	}

	// ** Plugins **
	if (game->Plugins->Count > MAX_PLUGINS) 
	{
		throw gcnew CompileError("Too many plugins");
	}

	numThisgamePlugins = game->Plugins->Count;
	for (i = 0; i < game->Plugins->Count; i++) 
	{
		AGS::Types::Plugin ^plugin = game->Plugins[i];
		ConvertStringToCharArray(plugin->FileName, thisgamePlugins[i].filename, 50);
		
		thisgamePlugins[i].dataSize = plugin->SerializedData->Length;
		for (int j = 0; j < thisgamePlugins[i].dataSize; j++) 
		{
			thisgamePlugins[i].data[j] = plugin->SerializedData[j];
		}
	}

	// ** Views **
	int viewCount = CountViews(game->RootViewFolder);

	thisgame.numviews = viewCount;
  allocate_memory_for_views(viewCount);
  numNewViews = viewCount;

	ConvertViewsToDTAFormat(game->RootViewFolder, game);

	// ** Characters **
	thisgame.numcharacters = game->Characters->Count;
	thisgame.chars = (CharacterInfo*)calloc(sizeof(CharacterInfo) * thisgame.numcharacters, 1);
  thisgame.charProps = (::CustomProperties*)calloc(thisgame.numcharacters, sizeof(::CustomProperties));
	for (i = 0; i < thisgame.numcharacters; i++) 
	{
		AGS::Types::Character ^character = game->Characters[i];
		thisgame.chars[i].flags = 0;
		thisgame.chars[i].on = 1;
		if (character->AdjustSpeedWithScaling) thisgame.chars[i].flags |= CHF_SCALEMOVESPEED;
		if (character->AdjustVolumeWithScaling) thisgame.chars[i].flags |= CHF_SCALEVOLUME;
		if (!character->Clickable) thisgame.chars[i].flags |= CHF_NOINTERACT;
		if (!character->DiagonalLoops) thisgame.chars[i].flags |= CHF_NODIAGONAL;
    if (character->MovementLinkedToAnimation) thisgame.chars[i].flags |= CHF_ANTIGLIDE;
		if (!character->Solid) thisgame.chars[i].flags |= CHF_NOBLOCKING;
		if (!character->TurnBeforeWalking) thisgame.chars[i].flags |= CHF_NOTURNING;
		if (!character->UseRoomAreaLighting) thisgame.chars[i].flags |= CHF_NOLIGHTING;
		if (!character->UseRoomAreaScaling) thisgame.chars[i].flags |= CHF_MANUALSCALING;

		if (character->UniformMovementSpeed) 
		{
			thisgame.chars[i].walkspeed = character->MovementSpeed;
			thisgame.chars[i].walkspeed_y = UNIFORM_WALK_SPEED;
		}
		else 
		{
			thisgame.chars[i].walkspeed = character->MovementSpeedX;
			thisgame.chars[i].walkspeed_y = character->MovementSpeedY;
		}

		thisgame.chars[i].animspeed = character->AnimationDelay;
		thisgame.chars[i].blinkview = character->BlinkingView - 1;
		thisgame.chars[i].idleview = character->IdleView - 1;
		thisgame.chars[i].defview = character->NormalView - 1;
		thisgame.chars[i].view = thisgame.chars[i].defview;
		ConvertStringToCharArray(character->RealName, thisgame.chars[i].name, 40);
		ConvertStringToCharArray(character->ScriptName, thisgame.chars[i].scrname, MAX_SCRIPT_NAME_LEN);
		thisgame.chars[i].talkcolor = character->SpeechColor;
		thisgame.chars[i].talkview = character->SpeechView - 1;
		thisgame.chars[i].room = character->StartingRoom;
    thisgame.chars[i].speech_anim_speed = character->SpeechAnimationDelay;
		thisgame.chars[i].x = character->StartX;
		thisgame.chars[i].y = character->StartY;
		thisgame.chars[i].thinkview = character->ThinkingView - 1;

		CompileCustomProperties(character->Properties, &thisgame.charProps[i]);
	}
	thisgame.playercharacter = game->PlayerCharacter->ID;

	// ** Text Parser **
  thisgame.dict = (WordsDictionary*)malloc(sizeof(WordsDictionary));
  thisgame.dict->allocate_memory(game->TextParser->Words->Count);
  for (i = 0; i < thisgame.dict->num_words; i++)
	{
    ConvertStringToCharArray(game->TextParser->Words[i]->Word, thisgame.dict->word[i], MAX_PARSER_WORD_LENGTH);
    thisgame.dict->wordnum[i] = game->TextParser->Words[i]->WordGroup;
	}

	// ** Global Messages **
	for (i = 0; i < MAXGLOBALMES; i++) 
	{
		if (game->GlobalMessages[i] != String::Empty) 
		{
			thisgame.messages[i] = (char*)malloc(game->GlobalMessages[i]->Length + 1);
			ConvertStringToCharArray(game->GlobalMessages[i], thisgame.messages[i]);
		}
		else
		{
			thisgame.messages[i] = NULL;
		}
	}

	// ** Lip Sync **
	if (game->LipSync->Type == LipSyncType::Text)
	{
		thisgame.options[OPT_LIPSYNCTEXT] = 1;
	}
	else 
	{
		thisgame.options[OPT_LIPSYNCTEXT] = 0;
	}
	thisgame.default_lipsync_frame = game->LipSync->DefaultFrame;
	for (i = 0; i < MAXLIPSYNCFRAMES; i++) 
	{
		ConvertStringToCharArray(game->LipSync->CharactersPerFrame[i], thisgame.lipSyncFrameLetters[i], 50);
	}

	// ** Dialogs **
	if (game->Dialogs->Count > MAX_DIALOG) 
	{
		throw gcnew CompileError("Too many dialogs");
	}
	thisgame.numdialog = game->Dialogs->Count;
	thisgame.numdlgmessage = 0;
	dialog = (DialogTopic*)malloc(sizeof(DialogTopic) * thisgame.numdialog);
	for (i = 0; i < thisgame.numdialog; i++) 
	{
		Dialog ^curDialog = game->Dialogs[i];
		dialog[i].numoptions = curDialog->Options->Count;
		for (int j = 0; j < dialog[i].numoptions; j++) 
		{
			AGS::Types::DialogOption ^option = curDialog->Options[j];
			ConvertStringToCharArray(option->Text, dialog[i].optionnames[j], 150);
			//dialog[i].entrypoints[j] = option->EntryPointOffset;
			dialog[i].optionflags[j] = 0;
			if (!option->Say) 
			{
				dialog[i].optionflags[j] |= DFLG_NOREPEAT;
			}
			if (option->Show)
			{
				dialog[i].optionflags[j] |= DFLG_ON;
			}
		}
		
    dialog[i].optionscripts = NULL;
/*	    dialog[i].optionscripts = (unsigned char*)malloc(curDialog->CodeSize);
	    Marshal::Copy(curDialog->CompiledCode, 0, IntPtr(dialog[i].optionscripts), curDialog->CodeSize);

		dialog[i].codesize = curDialog->CodeSize;
		dialog[i].startupentrypoint = curDialog->StartupEntryPointOffset;

		dlgscript[i] = (char*)malloc(curDialog->Script->Length + 1);
		ConvertStringToCharArray(curDialog->Script, dlgscript[i]);*/

		ConvertStringToCharArray(curDialog->Name, thisgame.dialogScriptNames[i], MAX_SCRIPT_NAME_LEN);

		dialog[i].topicFlags = 0;
		if (curDialog->ShowTextParser) dialog[i].topicFlags |= DTFLG_SHOWPARSER;
	}

	// ** Cursors **
	if (game->Cursors->Count > MAX_CURSOR) 
	{
		throw gcnew CompileError("Too many cursors");
	}
	thisgame.numcursors = game->Cursors->Count;
	for (i = 0; i < thisgame.numcursors; i++)
	{
		AGS::Types::MouseCursor ^cursor = game->Cursors[i];
		thisgame.mcurs[i].flags = 0;
		if (cursor->Animate) 
		{
			thisgame.mcurs[i].view = cursor->View - 1;
			if (cursor->AnimateOnlyOnHotspots) thisgame.mcurs[i].flags |= MCF_HOTSPOT;
			if (cursor->AnimateOnlyWhenMoving) thisgame.mcurs[i].flags |= MCF_ANIMMOVE;
		}
		else 
		{
			thisgame.mcurs[i].view = -1;
		}
		if (cursor->StandardMode) thisgame.mcurs[i].flags |= MCF_STANDARD;
		thisgame.mcurs[i].pic = cursor->Image;
		thisgame.mcurs[i].hotx = cursor->HotspotX;
		thisgame.mcurs[i].hoty = cursor->HotspotY;
		ConvertStringToCharArray(cursor->Name, thisgame.mcurs[i].name, 10);
	}

	// ** Fonts **
	if (game->Fonts->Count > MAX_FONTS)
	{
		throw gcnew CompileError("Too many fonts");
	}
	thisgame.numfonts = game->Fonts->Count;
	for (i = 0; i < thisgame.numfonts; i++) 
	{
		AGS::Types::Font ^font = game->Fonts[i];
		thisgame.fontflags[i] = font->PointSize & FFLG_SIZEMASK;
		if (font->OutlineStyle == FontOutlineStyle::None) 
		{
			thisgame.fontoutline[i] = -1;
		}
		else if (font->OutlineStyle == FontOutlineStyle::Automatic) 
		{
			thisgame.fontoutline[i] = FONT_OUTLINE_AUTO;
		}
		else
		{
			thisgame.fontoutline[i] = font->OutlineFont;
		}
	}

	// ** Inventory items **
	if (game->InventoryItems->Count > MAX_INV)
	{
		throw gcnew CompileError("Too many inventory items");
	}
	thisgame.numinvitems = game->InventoryItems->Count + 1;
	for (i = 1; i < thisgame.numinvitems; i++)
	{
		InventoryItem^ invItem = game->InventoryItems[i - 1];
		ConvertStringToCharArray(invItem->Description, thisgame.invinfo[i].name, 25);
		ConvertStringToCharArray(invItem->Name, thisgame.invScriptNames[i], MAX_SCRIPT_NAME_LEN);
		thisgame.invinfo[i].pic = invItem->Image; 
    thisgame.invinfo[i].cursorPic = invItem->CursorImage;
		thisgame.invinfo[i].hotx = invItem->HotspotX;
		thisgame.invinfo[i].hoty = invItem->HotspotY;
		thisgame.invinfo[i].flags = 0;
		if (invItem->PlayerStartsWithItem) thisgame.invinfo[i].flags |= IFLG_STARTWITH;

		CompileCustomProperties(invItem->Properties, &thisgame.invProps[i]);
	}

	// ** Custom Properties Schema **
	List<AGS::Types::CustomPropertySchemaItem ^> ^schema = game->PropertySchema->PropertyDefinitions;
	if (schema->Count > MAX_CUSTOM_PROPERTIES)
	{
		throw gcnew CompileError("Too many custom properties defined");
	}
	thisgame.propSchema.numProps = schema->Count;
	for (i = 0; i < thisgame.propSchema.numProps; i++) 
	{
		CustomPropertySchemaItem ^schemaItem = schema[i];
		ConvertStringToCharArray(schemaItem->Name, thisgame.propSchema.propName[i], 20);
		ConvertStringToCharArray(schemaItem->Description, thisgame.propSchema.propDesc[i], 100);
		thisgame.propSchema.defaultValue[i] = (char*)malloc(schemaItem->DefaultValue->Length + 1);
		ConvertStringToCharArray(schemaItem->DefaultValue, thisgame.propSchema.defaultValue[i]);
		thisgame.propSchema.propType[i] = (int)schemaItem->Type;
	}

	// ** GUIs **
	numguibuts = 0;
	numguilabels = 0;
	numguitext = 0;
	numguilist = 0;
	numguislider = 0;
	numguiinv = 0;

	thisgame.numgui = game->GUIs->Count;
  guis = (GUIMain*)calloc(thisgame.numgui, sizeof(GUIMain));
	for (i = 0; i < thisgame.numgui; i++)
	{
		guis[i].init();
		ConvertGUIToBinaryFormat(game->GUIs[i], &guis[i]);
		guis[i].highlightobj = -1;
	}

	// this cannot be null, so set it randomly
	thisgame.compiled_script = (ccScript*)0x12345678;

	save_thisgame_to_file(fileName, game);
	
	free_old_game_data();
}



#pragma unmanaged


void quit(char * message) 
{
	ThrowManagedException((const char*)message);
}



// ** GRAPHICAL SCRIPT LOAD/SAVE ROUTINES ** //

long getlong(FILE*iii) {
  long tmm;
  fread(&tmm,4,1,iii);
  return tmm;
}

#define putlong putw

void save_script_configuration(FILE*iii) {
  // no variable names
  putlong (1, iii);
  putlong (0, iii);
}

void load_script_configuration(FILE*iii) { int aa;
  if (getlong(iii)!=1) quit("ScriptEdit: invliad config version");
  int numvarnames=getlong(iii);
  for (aa=0;aa<numvarnames;aa++) {
    int lenoft=getc(iii);
    fseek(iii,lenoft,SEEK_CUR);
  }
}

void save_graphical_scripts(FILE*fff,roomstruct*rss) {
  // no script
  putlong (-1, fff);
}

char*scripttempn="~acsc%d.tmp";
void load_graphical_scripts(FILE*iii,roomstruct*rst) {
  long ct;
  bool doneMsg = false;
  while (1) {
    fread(&ct,4,1,iii);
    if ((ct==-1) | (feof(iii)!=0)) break;
    if (!doneMsg) {
//      infoBox("WARNING: This room uses graphical scripts, which have been removed from this version. If you save the room now, all graphical scripts will be lost.");
      doneMsg = true;
    }
    // skip the data
    long lee; fread(&lee,4,1,iii);
    fseek (iii, lee, SEEK_CUR);
  }
}

void update_polled_stuff_if_runtime()
{
	// do nothing
}

// [IKM] 2012-06-07
// Had to copy this implementation from Engine/misc.cpp to evade including additional srcs & libs to AGS.Native.
FILE *ci_fopen(char *file_name, const char *mode)
{
    // Don't pass a NULL pointer to newlib on the PSP.
    if (file_name == NULL)
    {
        return NULL;
    }
    else
    {
        return fopen(file_name, mode);
    }
}

// [IKM] 2012-06-07
// Had to copy this variable definition from Engine/acc.cpp, since it is required in acgui.cpp // GUIInv::CalculateNumCells()
// due JJS's compatiblity fix for 2.70.
// This *must* be not less than 31, otherwise function will work in backward-compatibility mode.
int loaded_game_file_version = 31;
