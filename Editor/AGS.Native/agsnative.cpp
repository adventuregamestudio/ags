#define USE_CLIB
#include <stdio.h>
void ThrowManagedException(const char *message);
#pragma unmanaged
#pragma warning (disable: 4996 4312)  // disable deprecation warnings
extern bool Scintilla_RegisterClasses(void *hInstance);
extern int Scintilla_LinkLexers();

int antiAliasFonts = 0;
#define SAVEBUFFERSIZE 5120
bool ShouldAntiAliasText() { return (antiAliasFonts != 0); }

int mousex, mousey;
#include "util/wgt2allg.h"
#include "util/misc.h"
#include "ac/spritecache.h"
#include "ac/actiontype.h"
#include "ac/common.h"
#include "ac/scriptmodule.h"
#include "ac/view.h"
#include "ac/wordsdictionary.h"
#include "font/fonts.h"
#include "game/dialogtopicinfo.h"
#include "game/gameinfo.h"
#include "game/roominfo.h"
#include "gui/guimain.h"
#include "gui/guiinv.h"
#include "gui/guibutton.h"
#include "gui/guilabel.h"
#include "gui/guitextbox.h"
#include "gui/guilistbox.h"
#include "gui/guislider.h"
#include "util/compress.h"
#include "util/string_utils.h"    // fputstring, etc
#include "util/alignedstream.h"
#include "util/filestream.h"
#include "util/geometry.h"
#include "gfx/bitmap.h"
#include "core/assetmanager.h"

using AGS::Common::Stream;
namespace BitmapHelper = AGS::Common::BitmapHelper;

//-----------------------------------------------------------------------------
// [IKM] 2012-09-07
// TODO: need a way to make conversions between Common::Bitmap and Windows bitmap;
// Possible plan:
// - Common::Bitmap *ConvertToBitmapClass(int class_type) method in Common::Bitmap;
// - WindowsBitmap implementation of Common::Bitmap;
// - AllegroBitmap and WindowsBitmap know of each other and may convert to
// each other.
//
//-----------------------------------------------------------------------------

// TODO: do something with this later
// (those are from 'cstretch' unit)
extern void Cstretch_blit(BITMAP *src, BITMAP *dst, int sx, int sy, int sw, int sh, int dx, int dy, int dw, int dh);
extern void Cstretch_sprite(BITMAP *dst, BITMAP *src, int x, int y, int w, int h);

void serialize_room_interactions(Stream *);

inline void Cstretch_blit(Common::Bitmap *src, Common::Bitmap *dst, int sx, int sy, int sw, int sh, int dx, int dy, int dw, int dh)
{
	Cstretch_blit(src->GetAllegroBitmap(), dst->GetAllegroBitmap(), sx, sy, sw, sh, dx, dy, dw, dh);
}
inline void Cstretch_sprite(Common::Bitmap *dst, Common::Bitmap *src, int x, int y, int w, int h)
{
	Cstretch_sprite(dst->GetAllegroBitmap(), src->GetAllegroBitmap(), x, y, w, h);
}


int sxmult = 1, symult = 1;
int dsc_want_hires = 0;
bool enable_greyed_out_masks = true;
bool outlineGuiObjects;
color*palette;
AGS::Common::GameInfo thisgame;
SpriteCache spriteset(MAX_SPRITES + 2);
AGS::Common::GuiMain tempgui;
const char*sprsetname = "acsprset.spr";
const char*clibendsig = "CLIB\x1\x2\x3\x4SIGE";
const char *old_editor_data_file = "editor.dat";
const char *new_editor_data_file = "game.agf";
const char *old_editor_main_game_file = "ac2game.dta";
const char *TEMPLATE_LOCK_FILE = "template.dta";
const char *ROOM_TEMPLATE_ID_FILE = "rtemplate.dat";
const int ROOM_TEMPLATE_ID_FILE_SIGNATURE = 0x74673812;
bool spritesModified = false;
AGS::Common::RoomInfo thisroom;
bool roomModified = false;
Common::Bitmap *drawBuffer = NULL;
Common::Bitmap *undoBuffer = NULL;
int loaded_room_number = -1;

// stuff for importing old games
int numScriptModules;
ScriptModule* scModules = NULL;
AGS::Common::ObjectArray<AGS::Common::DialogTopicInfo> dialog;
AGS::Common::Array<char*> dlgscript;
AGS::Common::ObjectArray<AGS::Common::GuiMain> guis;
ViewStruct272 *oldViews;
ViewStruct *newViews;
int numNewViews = 0;

// A reference color depth, for correct color selection;
// originally was defined by 'abuf' bitmap.
int BaseColorDepth;


bool reload_font(int curFont);
void drawBlockScaledAt(int hdc, Common::Bitmap *todraw ,int x, int y, int scaleFactor);
// this is to shut up the linker, it's used by CSRUN.CPP
void write_log(char *) { }

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
void fix_block (Common::Bitmap *todraw) {
  int a,b,pixval;
  if (todraw == NULL)
    return;
  // TODO: redo this using direct bitmap data access for the sake of speed
  if (todraw->GetColorDepth() == 16) {
    for (a = 0; a < todraw->GetWidth(); a++) {
      for (b = 0; b < todraw->GetHeight(); b++) {
        pixval = todraw->GetPixel (a, b);
        todraw->PutPixel (a, b, makecol16 (getb16(pixval),getg16(pixval),getr16(pixval)));
      }
    }
  }
  else if (todraw->GetColorDepth() == 32) {
    for (a = 0; a < todraw->GetWidth(); a++) {
      for (b = 0; b < todraw->GetHeight(); b++) {
        pixval = todraw->GetPixel (a, b);
        todraw->PutPixel (a, b, makeacol32 (getb32(pixval),getg32(pixval),getr32(pixval), geta32(pixval)));
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

Common::Bitmap *get_sprite (int spnr) {
  if (spnr < 0)
    return NULL;
  if (spriteset[spnr] == NULL) {
    spnr = 0;
  }
  return spriteset[spnr];
}

void SetNewSprite(int slot, Common::Bitmap *sprit) {
  delete spriteset[slot];

  spriteset.setNonDiscardable(slot, sprit);
  spritesModified = true;
}

void deleteSprite (int sprslot) {
  spriteset.removeSprite(sprslot, true);
  
  spritesModified = true;
}

void SetNewSpriteFromHBitmap(int slot, int hBmp) {
  // FIXME later
  Common::Bitmap *tempsprite = Common::BitmapHelper::CreateRawBitmapOwner(convert_hbitmap_to_bitmap((HBITMAP)hBmp));
  SetNewSprite(slot, tempsprite);
}

int GetSpriteAsHBitmap(int slot) {
  // FIXME later
  return (int)convert_bitmap_to_hbitmap(get_sprite(slot)->GetAllegroBitmap());
}

bool DoesSpriteExist(int slot) {
	return (spriteset[slot] != NULL);
}

int GetMaxSprites() {
	return MAX_SPRITES;
}

int GetSpriteWidth(int slot) {
	return get_sprite(slot)->GetWidth();
}

int GetSpriteHeight(int slot) {
	return get_sprite(slot)->GetHeight();
}

int GetRelativeSpriteWidth(int slot) {
	return GetSpriteWidth(slot) / ((thisgame.SpriteFlags[slot] & SPF_640x400) ? 2 : 1);
}

int GetRelativeSpriteHeight(int slot) {
	return GetSpriteHeight(slot) / ((thisgame.SpriteFlags[slot] & SPF_640x400) ? 2 : 1);
}

int GetSpriteResolutionMultiplier(int slot)
{
	return ((thisgame.SpriteFlags[slot] & SPF_640x400) ? 1 : 2);
}

unsigned char* GetRawSpriteData(int spriteSlot) {
  return &get_sprite(spriteSlot)->GetScanLineForWriting(0)[0];
}

int GetSpriteColorDepth(int slot) {
  return get_sprite(slot)->GetColorDepth();
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
	thisgame.SpriteFlags[spriteNum] &= ~SPF_640x400;
	if (isHighRes)
	{
		thisgame.SpriteFlags[spriteNum] |= SPF_640x400;
	}
}

void change_sprite_number(int oldNumber, int newNumber) {

  spriteset.setNonDiscardable(newNumber, spriteset[oldNumber]);
  spriteset.removeSprite(oldNumber, false);

  thisgame.SpriteFlags[newNumber] = thisgame.SpriteFlags[oldNumber];
  thisgame.SpriteFlags[oldNumber] = 0;

  spritesModified = true;
}

int crop_sprite_edges(int numSprites, int *sprites, bool symmetric) {
  // this function has passed in a list of sprites, all the
  // same size, to crop to the size of the smallest
  int aa, xx, yy;
  int width = spriteset[sprites[0]]->GetWidth();
  int height = spriteset[sprites[0]]->GetHeight();
  int left = width, right = 0;
  int top = height, bottom = 0;

  for (aa = 0; aa < numSprites; aa++) {
    Common::Bitmap *sprit = get_sprite(sprites[aa]);
    int maskcol = sprit->GetMaskColor();

    // find the left hand side
    for (xx = 0; xx < width; xx++) {
      for (yy = 0; yy < height; yy++) {
        if (sprit->GetPixel(xx, yy) != maskcol) {
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
        if (sprit->GetPixel(xx, yy) != maskcol) {
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
        if (sprit->GetPixel(xx, yy) != maskcol) {
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
        if (sprit->GetPixel(xx, yy) != maskcol) {
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
    Common::Bitmap *sprit = get_sprite(sprites[aa]);
    // create a new, smaller sprite and copy across
	Common::Bitmap *newsprit = Common::BitmapHelper::CreateBitmap(newWidth, newHeight, sprit->GetColorDepth());
    newsprit->Blit(sprit, left, top, 0, 0, newWidth, newHeight);
    delete sprit;

    spriteset.setNonDiscardable(sprites[aa], newsprit);
  }

  spritesModified = true;

  return 1;
}

int extract_room_template_files(const char *templateFileName, int newRoomNumber) 
{
    if (Common::AssetManager::SetDataFile(templateFileName) != Common::kAssetNoError) 
  {
    return 0;
  }
  if (Common::AssetManager::GetAssetOffset(ROOM_TEMPLATE_ID_FILE) < 1)
  {
    Common::AssetManager::SetDataFile(NULL);
    return 0;
  }

  int numFile = Common::AssetManager::GetAssetCount();

  for (int a = 0; a < numFile; a++) {
      const char *thisFile = Common::AssetManager::GetAssetFileByIndex(a);
    if (thisFile == NULL) {
      Common::AssetManager::SetDataFile(NULL);
      return 0;
    }

    // don't extract the template metadata file
    if (stricmp(thisFile, ROOM_TEMPLATE_ID_FILE) == 0)
      continue;

    Stream *readin = Common::AssetManager::OpenAsset ((char*)thisFile);
    char outputName[MAX_PATH];
    const char *extension = strchr(thisFile, '.');
    sprintf(outputName, "room%d%s", newRoomNumber, extension);
    Stream *wrout = Common::File::CreateFile(outputName);
    if ((readin == NULL) || (wrout == NULL)) 
    {
      delete wrout;
      delete readin;
      Common::AssetManager::SetDataFile(NULL);
      return 0;
    }
    long size = Common::AssetManager::GetAssetSize((char*)thisFile);
    char *membuff = (char*)malloc (size);
    readin->Read(membuff, size);
    wrout->Write (membuff, size );
    delete readin;
    delete wrout;
    free (membuff);
  }

  Common::AssetManager::SetDataFile(NULL);
  return 1;
}

int extract_template_files(const char *templateFileName) 
{
  if (Common::AssetManager::SetDataFile(templateFileName) != Common::kAssetNoError) 
  {
    return 0;
  }
  
  if ((Common::AssetManager::GetAssetOffset((char*)old_editor_data_file) < 1) && (Common::AssetManager::GetAssetOffset((char*)new_editor_data_file) < 1))
  {
    Common::AssetManager::SetDataFile(NULL);
    return 0;
  }

  int numFile = Common::AssetManager::GetAssetCount();

  for (int a = 0; a < numFile; a++) {
    const char *thisFile = Common::AssetManager::GetAssetFileByIndex (a);
    if (thisFile == NULL) {
      Common::AssetManager::SetDataFile(NULL);
      return 0;
    }

    // don't extract the dummy template lock file
    if (stricmp(thisFile, TEMPLATE_LOCK_FILE) == 0)
      continue;

    Stream *readin = Common::AssetManager::OpenAsset ((char*)thisFile);
    Stream *wrout = Common::File::CreateFile (thisFile);
    if ((wrout == NULL) && (strchr(thisFile, '\\') != NULL))
    {
      // an old template with Music/Sound folder, create the folder
      char folderName[MAX_PATH];
      strcpy(folderName, thisFile);
      *strchr(folderName, '\\') = 0;
      mkdir(folderName);
      wrout = Common::File::CreateFile(thisFile);
    }
    if ((readin == NULL) || (wrout == NULL)) 
    {
      Common::AssetManager::SetDataFile(NULL);
      return 0;
    }
    long size = Common::AssetManager::GetAssetSize((char*)thisFile);
    char *membuff = (char*)malloc (size);
    readin->Read (membuff, size);
    wrout->Write (membuff, size);
    delete readin;
    delete wrout;
    free (membuff);
  }

  Common::AssetManager::SetDataFile(NULL);
  return 1;
}

void extract_icon_from_template(char *iconName, char **iconDataBuffer, long *bufferSize)
{
  // make sure we get the icon from the file
  Common::AssetManager::SetSearchPriority(Common::kAssetPriorityLib);
  long sizey = Common::AssetManager::GetAssetSize(iconName);
  Stream* inpu = Common::AssetManager::OpenAsset (iconName);
  if ((inpu != NULL) && (sizey > 0))
  {
    char *iconbuffer = (char*)malloc(sizey);
    inpu->Read (iconbuffer, sizey);
    delete inpu;
    *iconDataBuffer = iconbuffer;
    *bufferSize = sizey;
  }
  else
  {
    *iconDataBuffer = NULL;
    *bufferSize = 0;
  }
  // restore to normal setting after NewGameChooser changes it
  Common::AssetManager::SetSearchPriority(Common::kAssetPriorityDir);
}

int load_template_file(const char *fileName, char **iconDataBuffer, long *iconDataSize, bool isRoomTemplate)
{
  if (Common::AssetManager::SetDataFile(fileName) == Common::kAssetNoError)
  {
    if (isRoomTemplate)
    {
      if (Common::AssetManager::GetAssetOffset((char*)ROOM_TEMPLATE_ID_FILE) > 0)
      {
        Stream *inpu = Common::AssetManager::OpenAsset((char*)ROOM_TEMPLATE_ID_FILE);
        if (inpu->ReadInt32() != ROOM_TEMPLATE_ID_FILE_SIGNATURE)
        {
          delete inpu;
		  Common::AssetManager::SetDataFile(NULL);
          return 0;
        }
        int roomNumber = inpu->ReadInt32();
        delete inpu;
        char iconName[MAX_PATH];
        sprintf(iconName, "room%d.ico", roomNumber);
        if (Common::AssetManager::GetAssetOffset(iconName) > 0) 
        {
          extract_icon_from_template(iconName, iconDataBuffer, iconDataSize);
        }
		    Common::AssetManager::SetDataFile(NULL);
        return 1;
      }
	  Common::AssetManager::SetDataFile(NULL);
      return 0;
    }
	  else if ((Common::AssetManager::GetAssetOffset((char*)old_editor_data_file) > 0) || (Common::AssetManager::GetAssetOffset((char*)new_editor_data_file) > 0))
	  {
      Common::String oriname = Common::AssetManager::GetLibraryBaseFile();
      if ((strstr(oriname, ".exe") != NULL) ||
          (strstr(oriname, ".dat") != NULL) ||
          (strstr(oriname, ".ags") != NULL)) 
      {
        // wasn't originally meant as a template
		  Common::AssetManager::SetDataFile(NULL);
	      return 0;
      }

	    Stream *inpu = Common::AssetManager::OpenAsset((char*)old_editor_main_game_file);
	    if (inpu != NULL) 
	    {
		    inpu->Seek(Common::kSeekCurrent, 30);
		    int gameVersion = inpu->ReadInt32();
		    delete inpu;
		    if (gameVersion != 32)
		    {
			    // older than 2.72 template
				Common::AssetManager::SetDataFile(NULL);
			    return 0;
		    }
	    }

      int useIcon = 0;
      char *iconName = "template.ico";
      if (Common::AssetManager::GetAssetOffset (iconName) < 1)
        iconName = "user.ico";
      // the file is a CLIB file, so let's extract the icon to display
      if (Common::AssetManager::GetAssetOffset (iconName) > 0) 
      {
        extract_icon_from_template(iconName, iconDataBuffer, iconDataSize);
      }
	    Common::AssetManager::SetDataFile(NULL);
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

void drawBlockDoubleAt (int hdc, Common::Bitmap *todraw ,int x, int y) {
  drawBlockScaledAt (hdc, todraw, x, y, 2);
}

void wputblock_stretch(Common::Bitmap *g, int xpt,int ypt,Common::Bitmap *tblock,int nsx,int nsy) {
  if (tblock->GetBPP() != thisgame.ColorDepth) {
    Common::Bitmap *tempst=Common::BitmapHelper::CreateBitmapCopy(tblock, thisgame.ColorDepth*8);
    int ww,vv;
    for (ww=0;ww<tblock->GetWidth();ww++) {
      for (vv=0;vv<tblock->GetHeight();vv++) {
        if (tblock->GetPixel(ww,vv)==tblock->GetMaskColor())
          tempst->PutPixel(ww,vv,tempst->GetMaskColor());
      }
    }
    g->StretchBlt(tempst,RectWH(xpt,ypt,nsx,nsy), Common::kBitmap_Transparency);
    delete tempst;
  }
  else g->StretchBlt(tblock,RectWH(xpt,ypt,nsx,nsy), Common::kBitmap_Transparency);
}

void draw_sprite_compensate(Common::Bitmap *g, int sprnum, int atxp, int atyp, int seethru) {
  Common::Bitmap *blptr = get_sprite(sprnum);
  Common::Bitmap *towrite=blptr;
  int needtofree=0, main_color_depth = thisgame.ColorDepth * 8;

  if ((blptr->GetBPP() > 1) & (main_color_depth==8)) {

    towrite=Common::BitmapHelper::CreateBitmap(blptr->GetWidth(),blptr->GetHeight(), 8);
    needtofree=1;
    towrite->Clear(towrite->GetMaskColor());
    int xxp,yyp,tmv;
    for (xxp=0;xxp<blptr->GetWidth();xxp++) {
      for (yyp=0;yyp<blptr->GetHeight();yyp++) {
        tmv=blptr->GetPixel(xxp,yyp);
        if (tmv != blptr->GetMaskColor())
          towrite->PutPixel(xxp,yyp,makecol8(getr16(tmv),getg16(tmv),getb16(tmv)));
        }
      }

    }

  int nwid=towrite->GetWidth(),nhit=towrite->GetHeight();
  if (thisgame.SpriteFlags[sprnum] & SPF_640x400) {
    if (dsc_want_hires == 0) {
      nwid/=2;
      nhit/=2;
    }
  }
  else if (dsc_want_hires) {
    nwid *= 2;
    nhit *= 2;
  }
  wputblock_stretch(g, atxp,atyp,towrite,nwid,nhit);
  if (needtofree) delete towrite;
}

void drawBlock (HDC hdc, Common::Bitmap *todraw, int x, int y) {
  set_palette_to_hdc (hdc, palette);
  // FIXME later
  blit_to_hdc (todraw->GetAllegroBitmap(), hdc, 0,0,x,y,todraw->GetWidth(),todraw->GetHeight());
}


enum RoomAreaMask
{
    None,
    Hotspots,
    WalkBehinds,
    WalkableAreas,
    Regions
};

Common::Bitmap *get_bitmap_for_mask(Common::RoomInfo *roomptr, RoomAreaMask maskType) 
{
	if (maskType == RoomAreaMask::None) 
	{
		return NULL;
	}

	Common::Bitmap *source = NULL;
	switch (maskType) 
	{
	case RoomAreaMask::Hotspots:
		source = roomptr->HotspotMask;
		break;
	case RoomAreaMask::Regions:
		source = roomptr->RegionMask;
		break;
	case RoomAreaMask::WalkableAreas:
		source = roomptr->WalkAreaMask;
		break;
	case RoomAreaMask::WalkBehinds:
		source = roomptr->WalkBehindMask;
		break;
	}

	return source;
}

void copy_walkable_to_regions (void *roomptr) {
	Common::RoomInfo *theRoom = (Common::RoomInfo*)roomptr;
	theRoom->RegionMask->Blit(theRoom->WalkAreaMask, 0, 0, 0, 0, theRoom->RegionMask->GetWidth(), theRoom->RegionMask->GetHeight());
}

int get_mask_pixel(void *roomptr, int maskType, int x, int y)
{
	Common::Bitmap *mask = get_bitmap_for_mask((Common::RoomInfo*)roomptr, (RoomAreaMask)maskType);
	return mask->GetPixel(x, y);
}

void draw_line_onto_mask(void *roomptr, int maskType, int x1, int y1, int x2, int y2, int color)
{
	Common::Bitmap *mask = get_bitmap_for_mask((Common::RoomInfo*)roomptr, (RoomAreaMask)maskType);
	mask->DrawLine(Line(x1, y1, x2, y2), color);
}

void draw_filled_rect_onto_mask(void *roomptr, int maskType, int x1, int y1, int x2, int y2, int color)
{
	Common::Bitmap *mask = get_bitmap_for_mask((Common::RoomInfo*)roomptr, (RoomAreaMask)maskType);
    mask->FillRect(Rect(x1, y1, x2, y2), color);
}

void draw_fill_onto_mask(void *roomptr, int maskType, int x1, int y1, int color)
{
	Common::Bitmap *mask = get_bitmap_for_mask((Common::RoomInfo*)roomptr, (RoomAreaMask)maskType);
    mask->FloodFill(x1, y1, color);
}

void create_undo_buffer(void *roomptr, int maskType) 
{
	Common::Bitmap *mask = get_bitmap_for_mask((Common::RoomInfo*)roomptr, (RoomAreaMask)maskType);
  if (undoBuffer != NULL)
  {
    if ((undoBuffer->GetWidth() != mask->GetWidth()) || (undoBuffer->GetHeight() != mask->GetHeight())) 
    {
      delete undoBuffer;
      undoBuffer = NULL;
    }
  }
  if (undoBuffer == NULL)
  {
    undoBuffer = Common::BitmapHelper::CreateBitmap(mask->GetWidth(), mask->GetHeight(), mask->GetColorDepth());
  }
  undoBuffer->Blit(mask, 0, 0, 0, 0, mask->GetWidth(), mask->GetHeight());
}

bool does_undo_buffer_exist()
{
  return (undoBuffer != NULL);
}

void clear_undo_buffer() 
{
  if (does_undo_buffer_exist()) 
  {
    delete undoBuffer;
    undoBuffer = NULL;
  }
}

void restore_from_undo_buffer(void *roomptr, int maskType)
{
  if (does_undo_buffer_exist())
  {
  	Common::Bitmap *mask = get_bitmap_for_mask((Common::RoomInfo*)roomptr, (RoomAreaMask)maskType);
    mask->Blit(undoBuffer, 0, 0, 0, 0, mask->GetWidth(), mask->GetHeight());
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
      if ((aa < LEGACY_MAX_ROOM_HOTSPOTS) && (aa > 0))
        lumin = ((LEGACY_MAX_ROOM_HOTSPOTS - aa) % 30) * 2;
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

Common::Bitmap *recycle_bitmap(Common::Bitmap* check, int colDepth, int w, int h)
{
  if ((check != NULL) && (check->GetWidth() == w) && (check->GetHeight() == h) &&
      (check->GetColorDepth() == colDepth))
  {
    return check;
  }
  delete check;

  return Common::BitmapHelper::CreateBitmap(w, h, colDepth);
}

Common::Bitmap *stretchedSprite = NULL, *srcAtRightColDep = NULL;

void draw_area_mask(Common::RoomInfo *roomptr, Common::Bitmap *ds, RoomAreaMask maskType, int selectedArea, int transparency) 
{
	Common::Bitmap *source = get_bitmap_for_mask(roomptr, maskType);

	if (source == NULL) return;

    int dest_width = ds->GetWidth();
    int dest_height = ds->GetHeight();
    int dest_depth =  ds->GetColorDepth();
	
	if (source->GetColorDepth() != dest_depth) 
	{
    Common::Bitmap *sourceSprite = source;
    if ((source->GetWidth() != dest_width) || (source->GetHeight() != dest_height))
    {
		  stretchedSprite = recycle_bitmap(stretchedSprite, source->GetColorDepth(), dest_width, dest_height);
		  stretchedSprite->StretchBlt(source, RectWH(0, 0, source->GetWidth(), source->GetHeight()),
			  RectWH(0, 0, stretchedSprite->GetWidth(), stretchedSprite->GetHeight()));
      sourceSprite = stretchedSprite;
    }

    if (enable_greyed_out_masks)
    {
      setup_greyed_out_palette(selectedArea);
    }

    if (transparency > 0)
    {
      srcAtRightColDep = recycle_bitmap(srcAtRightColDep, dest_depth, dest_width, dest_height);
      
      int oldColorConv = get_color_conversion();
      set_color_conversion(oldColorConv | COLORCONV_KEEP_TRANS);

      srcAtRightColDep->Blit(sourceSprite, 0, 0, 0, 0, sourceSprite->GetWidth(), sourceSprite->GetHeight());
      set_trans_blender(0, 0, 0, (100 - transparency) + 155);
      ds->TransBlendBlt(srcAtRightColDep, 0, 0);
      set_color_conversion(oldColorConv);
    }
    else
    {
        ds->Blit(sourceSprite, 0, 0, Common::kBitmap_Transparency);
    }

    set_palette(palette);
	}
	else
	{
		Cstretch_sprite(ds, source, 0, 0, dest_width, dest_height);
	}
}

void draw_room_background(void *roomvoidptr, int hdc, int x, int y, int bgnum, float scaleFactor, int maskType, int selectedArea, int maskTransparency) 
{
	Common::RoomInfo *roomptr = (Common::RoomInfo*)roomvoidptr;

  if (bgnum >= roomptr->BkgSceneCount)
    return;

  Common::Bitmap *srcBlock = roomptr->Backgrounds[bgnum].Graphic;
  if (srcBlock == NULL)
    return;

	if (drawBuffer != NULL) 
	{
		Common::Bitmap *depthConverted = Common::BitmapHelper::CreateBitmap(srcBlock->GetWidth(), srcBlock->GetHeight(), drawBuffer->GetColorDepth());
    if (srcBlock->GetColorDepth() == 8)
    {
      select_palette(roomptr->Backgrounds[bgnum].Palette);
    }

    depthConverted->Blit(srcBlock, 0, 0, 0, 0, srcBlock->GetWidth(), srcBlock->GetHeight());

    if (srcBlock->GetColorDepth() == 8)
    {
      unselect_palette();
    }

	draw_area_mask(roomptr, depthConverted, (RoomAreaMask)maskType, selectedArea, maskTransparency);

    int srcX = 0, srcY = 0;
    int srcWidth = srcBlock->GetWidth();
    int srcHeight = srcBlock->GetHeight();

    if (x < 0)
    {
      srcX = -x / scaleFactor;
      x = 0;
      srcWidth = drawBuffer->GetWidth() / scaleFactor + 1;
      if (srcX + srcWidth > depthConverted->GetWidth())
      {
        srcWidth = depthConverted->GetWidth() - srcX;
      }
    }
    if (y < 0)
    {
      srcY = -y / scaleFactor;
      y = 0;
      srcHeight = drawBuffer->GetHeight() / scaleFactor + 1;
      if (srcY + srcHeight > depthConverted->GetHeight())
      {
        srcHeight = depthConverted->GetHeight() - srcY;
      }
    }

		Cstretch_blit(depthConverted, drawBuffer, srcX, srcY, srcWidth, srcHeight, x, y, srcWidth * scaleFactor, srcHeight * scaleFactor);
		delete depthConverted;
	}
	else {
		drawBlockScaledAt(hdc, srcBlock, x, y, scaleFactor);
	}
	
}

void update_font_sizes() {
  int multiplyWas = wtext_multiply;

  // scale up fonts if necessary
  wtext_multiply = 1;
  if ((thisgame.Options[OPT_NOSCALEFNT] == 0) &&
      (thisgame.DefaultResolution >= 3)) {
    wtext_multiply = 2;
  }

  if (multiplyWas != wtext_multiply) {
    // resolution or Scale Up Fonts has changed, reload at new size
    for (int bb=0;bb<thisgame.FontCount;bb++)
      reload_font (bb);
  }

  if (thisgame.DefaultResolution >= 3) {
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
  Stream*iii=Common::File::OpenFileRead(fnn);
  if (iii==NULL) {
    return "File not found";
  }
  if (iii->ReadByte()!=0x87) {
    delete iii;
    return "Not a valid SCI font file";
  }
  iii->Seek(Common::kSeekCurrent,3);
  if (iii->ReadInt16()!=0x80) {
    delete iii; 
	  return "Invalid SCI font"; 
  }
  int lineHeight = iii->ReadInt16();
  short theiroffs[0x80];
  iii->ReadArrayOfInt16(theiroffs,0x80);
  Stream*ooo=Common::File::CreateFile(wgtfontname);
  ooo->Write("WGT Font File  ",15);
  ooo->WriteInt16(0);  // will be table address
  short coffsets[0x80];
  char buffer[1000];
  int aa;
  for (aa=0;aa<0x80;aa++) 
  {
    if (theiroffs[aa] < 100)
    {
      delete iii;
      delete ooo;
      unlink(wgtfontname);
      return "Invalid character found in file";
    }
    iii->Seek(Common::kSeekBegin,theiroffs[aa]+2);
    int wwi=iii->ReadByte()-1;
    int hhi=iii->ReadByte();
    coffsets[aa]=ooo->GetPosition();
    ooo->WriteInt16(wwi+1);
    ooo->WriteInt16(hhi);
    if ((wwi<1) | (hhi<1)) continue;
    memset(buffer,0,sizeof(buffer));
    int bytesPerRow = (wwi/8)+1;
    iii->ReadArray(buffer, bytesPerRow, hhi);
    for (int bb=0;bb<hhi;bb++) { 
      int thisoffs = bb * bytesPerRow;
      ooo->Write(&buffer[thisoffs], bytesPerRow);
    }
  }
  long tableat=ooo->GetPosition();
  ooo->WriteArrayOfInt16(&coffsets[0],0x80);
  delete ooo;
  ooo=Common::File::OpenFile(wgtfontname,Common::kFile_Open,Common::kFile_ReadWrite);
  ooo->Seek(Common::kSeekBegin,15);
  ooo->WriteInt16(tableat); 
  delete ooo;
  delete iii;
  wfreefont(fslot);
  if (!wloadfont_size(fslot, 0))
  {
    return "Unable to load converted WFN file";
  }
  return NULL;
}


#define FONTGRIDSIZE 18*blockSize
void drawFontAt (int hdc, int fontnum, int x,int y) {
  
  if (fontnum >= thisgame.FontCount) 
  {
	  return;
  }

  update_font_sizes();

  int doubleSize = (thisgame.DefaultResolution < 3) ? 2 : 1;
  int blockSize = (thisgame.DefaultResolution < 3) ? 1 : 2;
  antiAliasFonts = thisgame.Options[OPT_ANTIALIASFONTS];

  // we can't antialias font because changing col dep to 16 here causes
  // it to crash ... why?
  Common::Bitmap *tempblock = Common::BitmapHelper::CreateBitmap(FONTGRIDSIZE*10, FONTGRIDSIZE*10, 8);
  tempblock->Fill(0);
  //Common::Bitmap *abufwas = abuf;
  //abuf = tempblock;
  color_t text_color = tempblock->GetCompatibleColor(15);
  for (int aa=0;aa<96;aa++)
    wgtprintf(tempblock, 5+(aa%10)*FONTGRIDSIZE,5+(aa/10)*FONTGRIDSIZE, fontnum, text_color, "%c",aa+32);
  //abuf = abufwas;

  if (doubleSize > 1) 
    drawBlockDoubleAt(hdc, tempblock, x, y);
  else
    drawBlock((HDC)hdc, tempblock, x, y);
   
  delete tempblock;
}

void proportionalDraw (int newwid, int sprnum, int*newx, int*newy) {
  int newhit = newwid;

  int newsizx=newwid,newsizy=newhit;
  int twid=get_sprite(sprnum)->GetWidth(),thit = get_sprite(sprnum)->GetHeight();
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

  Common::Bitmap *todraw = Common::BitmapHelper::CreateBitmap (wtoDraw, size, thisgame.ColorDepth*8);
  todraw->Clear (todraw->GetMaskColor ());
  int neww, newh;
  for (int i = 0; i < numFrames; i++) {
    // don't draw the Go-To-Next-Frame jibble
    if (frames[i].pic == -1)
      break;
    // work out the dimensions to stretch to
    proportionalDraw (size, frames[i].pic, &neww, &newh);
    Common::Bitmap *toblt = get_sprite(frames[i].pic);
    bool freeBlock = false;
    if (toblt->GetColorDepth () != todraw->GetColorDepth ()) {
      // 256-col sprite in hi-col game, we need to copy first
      Common::Bitmap *oldBlt = toblt;
      toblt = Common::BitmapHelper::CreateBitmap (toblt->GetWidth(), toblt->GetHeight(), todraw->GetColorDepth ());
      toblt->Blit (oldBlt, 0, 0, 0, 0, oldBlt->GetWidth(), oldBlt->GetHeight());
      freeBlock = true;
    }
    Common::Bitmap *flipped = NULL;
    if (frames[i].flags & VFLG_FLIPSPRITE) {
      // mirror the sprite
      flipped = Common::BitmapHelper::CreateBitmap (toblt->GetWidth(), toblt->GetHeight(), todraw->GetColorDepth ());
      flipped->Clear (flipped->GetMaskColor ());
      flipped->FlipBlt(toblt, 0, 0, Common::kBitmap_HFlip);
      if (freeBlock)
        delete toblt;
      toblt = flipped;
      freeBlock = true;
    }
    //->StretchBlt(toblt, todraw, 0, 0, toblt->GetWidth(), toblt->GetHeight(), size*i, 0, neww, newh);
	Cstretch_sprite(todraw, toblt, size*i, 0, neww, newh);
    if (freeBlock)
      delete toblt;
    if (i < numFrames-1) {
      int linecol = makecol_depth(thisgame.ColorDepth * 8, 0, 64, 200);
      if (thisgame.ColorDepth == 1)
        linecol = 12;

      // Draw dividing line
	  todraw->DrawLine (Line(size*(i+1) - 1, 0, size*(i+1) - 1, size-1), linecol);
    }
    if (i == cursel) {
      // Selected item
      int linecol = makecol_depth(thisgame.ColorDepth * 8, 255, 255,255);
      if (thisgame.ColorDepth == 1)
        linecol = 14;
      
      todraw->DrawRect(Rect (size * i, 0, size * (i+1) - 1, size-1), linecol);
    }
  }
  drawBlock ((HDC)hdc, todraw, x, y);
  delete todraw;
}

int get_adjusted_spritewidth(int spr) {
  Common::Bitmap *tsp = get_sprite(spr);
  if (tsp == NULL) return 0;

  int retval = tsp->GetWidth();

  if (thisgame.SpriteFlags[spr] & SPF_640x400) {
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
  Common::Bitmap *tsp = get_sprite(spr);
  if (tsp == NULL) return 0;

  int retval = tsp->GetHeight();

  if (thisgame.SpriteFlags[spr] & SPF_640x400) {
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
	__my_setcolor(&colNum, colNum, BaseColorDepth);
  /*if (thisgame.ColorDepth > 2) {
    // convert to 24-bit colour
    int red = ((colNum >> 11) & 0x1f) * 8;
    int grn = ((colNum >> 5) & 0x3f) * 4;
    int blu = (colNum & 0x1f) * 8;
    colNum = (red << _rgb_r_shift_32) | (grn << _rgb_g_shift_32) | (blu << _rgb_b_shift_32);
  }*/

  Common::Bitmap *palbmp = Common::BitmapHelper::CreateBitmap(width, height, thisgame.ColorDepth * 8);
  palbmp->Clear (colNum);
  drawBlockScaledAt(hdc, palbmp, x, y, 1);
  delete palbmp;
}

/* [IKM] 2012-07-08: use the Common implementation
void NewInteractionCommand::remove () 
{
}
*/

void new_font () {
  wloadfont_size(thisgame.FontCount, 0);
  thisgame.FontFlags.Append(0);
  thisgame.FontOutline.Append(-1);
  thisgame.FontCount++;
}

bool initialize_native()
{
    Common::AssetManager::CreateInstance();

  set_uformat(U_ASCII);  // required to stop ALFONT screwing up text
	install_allegro(SYSTEM_NONE, &errno, atexit);
	//set_gdi_color_format();
	palette = &thisgame.DefaultPalette[0];
	thisgame.ColorDepth = 2;
	//abuf = Common::BitmapHelper::CreateBitmap(10, 10, 32);
    BaseColorDepth = 32;
	thisgame.FontCount = 0;
	new_font();

	spriteset.reset();
	if (spriteset.initFile(sprsetname))
	  return false;
	spriteset.maxCacheSize = 100000000;  // 100 mb cache
    thisgame.SpriteFlags.New(MAX_SPRITES, 0);

	if (!Scintilla_RegisterClasses (GetModuleHandle(NULL)))
      return false;

  init_font_renderer();

	return true;
}

void shutdown_native()
{
  shutdown_font_renderer();
  // This MUST be called before Allegro is deinitialized.
  thisroom.Free();
  allegro_exit();
  Common::AssetManager::DestroyInstance();
}

void drawBlockScaledAt (int hdc, Common::Bitmap *todraw ,int x, int y, int scaleFactor) {
  if (todraw->GetColorDepth () == 8)
    set_palette_to_hdc ((HDC)hdc, palette);

  // FIXME later
  stretch_blit_to_hdc (todraw->GetAllegroBitmap(), (HDC)hdc, 0,0,todraw->GetWidth(),todraw->GetHeight(),
    x,y,todraw->GetWidth() * scaleFactor, todraw->GetHeight() * scaleFactor);
}

void drawSprite(int hdc, int x, int y, int spriteNum, bool flipImage) {
	int scaleFactor = ((thisgame.SpriteFlags[spriteNum] & SPF_640x400) != 0) ? 1 : 2;
	Common::Bitmap *theSprite = get_sprite(spriteNum);

  if (theSprite == NULL)
    return;

	if (flipImage) {
		Common::Bitmap *flipped = Common::BitmapHelper::CreateBitmap (theSprite->GetWidth(), theSprite->GetHeight(), theSprite->GetColorDepth());
		flipped->FillTransparent();
		flipped->FlipBlt(theSprite, 0, 0, Common::kBitmap_HFlip);
		drawBlockScaledAt(hdc, flipped, x, y, scaleFactor);
		delete flipped;
	}
	else 
	{
		drawBlockScaledAt(hdc, theSprite, x, y, scaleFactor);
	}
}

void drawSpriteStretch(int hdc, int x, int y, int width, int height, int spriteNum) {
  Common::Bitmap *todraw = get_sprite(spriteNum);
  Common::Bitmap *tempBlock = NULL;
	
  if (todraw->GetColorDepth () == 8)
    set_palette_to_hdc ((HDC)hdc, palette);

  int hdcBpp = GetDeviceCaps((HDC)hdc, BITSPIXEL);
  if (hdcBpp != todraw->GetColorDepth())
  {
	  tempBlock = Common::BitmapHelper::CreateBitmapCopy(todraw, hdcBpp);
	  todraw = tempBlock;
  }

  // FIXME later
  stretch_blit_to_hdc (todraw->GetAllegroBitmap(), (HDC)hdc, 0,0,todraw->GetWidth(),todraw->GetHeight(), x,y, width, height);

  delete tempBlock;
}

void drawGUIAt (int hdc, int x,int y,int x1,int y1,int x2,int y2, int scaleFactor) {

  if ((tempgui.GetWidth() < 1) || (tempgui.GetHeight() < 1))
    return;

  //update_font_sizes();

  if (scaleFactor == 1) {
    dsc_want_hires = 1;
  }

  Common::Bitmap *tempblock = Common::BitmapHelper::CreateBitmap(tempgui.GetWidth(), tempgui.GetHeight(), thisgame.ColorDepth*8);
  tempblock->Clear(tempblock->GetMaskColor ());
  //Common::Bitmap *abufWas = abuf;
  //abuf = tempblock;

  tempgui.DrawAt (tempblock, 0, 0);

  dsc_want_hires = 0;

  if (x1 >= 0) {
    tempblock->DrawRect(Rect (x1, y1, x2, y2), 14);
  }
  //abuf = abufWas;

  drawBlockScaledAt (hdc, tempblock, x, y, scaleFactor);
  //drawBlockDoubleAt (hdc, tempblock, x, y);
  delete tempblock;
}

#define SIMP_INDEX0  0
#define SIMP_TOPLEFT 1
#define SIMP_BOTLEFT 2
#define SIMP_TOPRIGHT 3
#define SIMP_BOTRIGHT 4
#define SIMP_LEAVEALONE 5
#define SIMP_NONE     6

void sort_out_transparency(Common::Bitmap *toimp, int sprite_import_method, color*itspal, bool useBgSlots, int importedColourDepth) 
{
  if (sprite_import_method == SIMP_LEAVEALONE)
    return;

  int uu,tt;
  set_palette_range(palette, 0, 255, 0);
  int transcol=toimp->GetMaskColor();
  // NOTE: This takes the pixel from the corner of the overall import
  // graphic, NOT just the image to be imported
  if (sprite_import_method == SIMP_TOPLEFT)
    transcol=toimp->GetPixel(0,0);
  else if (sprite_import_method==SIMP_BOTLEFT)
    transcol=toimp->GetPixel(0,(toimp->GetHeight())-1);
  else if (sprite_import_method == SIMP_TOPRIGHT)
    transcol = toimp->GetPixel((toimp->GetWidth())-1, 0);
  else if (sprite_import_method == SIMP_BOTRIGHT)
    transcol = toimp->GetPixel((toimp->GetWidth())-1, (toimp->GetHeight())-1);

  if (sprite_import_method == SIMP_NONE)
  {
    // remove all transparency pixels (change them to
    // a close non-trnasparent colour)
    int changeTransparencyTo;
    if (transcol == 0)
      changeTransparencyTo = 16;
    else
      changeTransparencyTo = transcol - 1;

    for (tt=0;tt<toimp->GetWidth();tt++) {
      for (uu=0;uu<toimp->GetHeight();uu++) {
        if (toimp->GetPixel(tt,uu) == transcol)
          toimp->PutPixel(tt,uu, changeTransparencyTo);
      }
    }
  }
  else
  {
	  int bitmapMaskColor = toimp->GetMaskColor();
    int replaceWithCol = 16;
	  if (toimp->GetColorDepth() > 8)
	  {
      if (importedColourDepth == 8)
        replaceWithCol = makecol_depth(toimp->GetColorDepth(), itspal[0].r * 4, itspal[0].g * 4, itspal[0].b * 4);
      else
		    replaceWithCol = 0;
	  }
    // swap all transparent pixels with index 0 pixels
    for (tt=0;tt<toimp->GetWidth();tt++) {
      for (uu=0;uu<toimp->GetHeight();uu++) {
        if (toimp->GetPixel(tt,uu)==transcol)
          toimp->PutPixel(tt,uu, bitmapMaskColor);
        else if (toimp->GetPixel(tt,uu) == bitmapMaskColor)
          toimp->PutPixel(tt,uu, replaceWithCol);
      }
    }
  }

  if ((thisgame.ColorDepth == 1) && (itspal != NULL)) { 
    // 256-colour mode only
    if (transcol!=0)
      itspal[transcol] = itspal[0];
    wsetrgb(0,0,0,0,itspal); // set index 0 to black
    __wremap_keep_transparent = 1;
    color oldpale[256];
    for (uu=0;uu<255;uu++) {
      if (useBgSlots)  //  use background scene palette
        oldpale[uu]=palette[uu];
      else if (thisgame.PaletteUses[uu]==PAL_BACKGROUND)
        wsetrgb(uu,0,0,0,oldpale);
      else 
        oldpale[uu]=palette[uu];
    }
    wremap(itspal,toimp,oldpale); 
    set_palette_range(palette, 0, 255, 0);
  }
  else if (toimp->GetColorDepth() == 8) {  // hi-colour game
    set_palette(itspal);
  }
}

void update_abuf_coldepth() {
//  delete abuf;
//  abuf = Common::BitmapHelper::CreateBitmap(10, 10, thisgame.ColorDepth * 8);
    BaseColorDepth = thisgame.ColorDepth * 8;
}

bool reload_font(int curFont)
{
  wfreefont(curFont);

  int fsize = thisgame.FontFlags[curFont] & FFLG_SIZEMASK;
  // if the font is designed for 640x400, half it
  if (thisgame.Options[OPT_NOSCALEFNT]) {
    if (thisgame.DefaultResolution <= 2)
      fsize /= 2;
  }
  else if (thisgame.DefaultResolution >= 3) {
    // designed for 320x200, double it up
    fsize *= 2;
  }
  return wloadfont_size(curFont, fsize);
}

void load_script_modules_compiled(Stream *inn) {

  numScriptModules = inn->ReadInt32();
  scModules = (ScriptModule*)realloc(scModules, sizeof(ScriptModule) * numScriptModules);
  for (int i = 0; i < numScriptModules; i++) {
    scModules[i].init();
    scModules[i].compiled = ccScript::CreateFromStream(inn);
  }

}

void read_dialogs(Stream*iii, int filever, bool encrypted) {
  int bb;
  dialog.New(thisgame.DialogCount);

  Common::DialogVersion dlg_version;
  if (filever < kGameVersion_340_alpha)
  {
    dlg_version = Common::kDialogVersion_pre340;
  }
  else
  {
    dlg_version = (Common::DialogVersion)iii->ReadInt32();
  }

  //iii->ReadArray(&dialog[0],sizeof(DialogTopicInfo),thisgame.DialogCount);
  for (int i = 0; i < thisgame.DialogCount; ++i)
  {
    dialog[i].ReadFromFile(iii, dlg_version);
  }

  for (bb=0;bb<thisgame.DialogCount;bb++) {
    if (dialog[bb].OldCodeSize > 0) {
      //dialog[bb].optionscripts=(unsigned char*)malloc(dialog[bb].OldCodeSize+10);
      //iii->Read(&dialog[bb].optionscripts[0],dialog[bb].OldCodeSize);
        iii->Seek(Common::kSeekCurrent, dialog[bb].OldCodeSize);
    }
    int lenof=iii->ReadInt32();
    if (lenof<=1) { iii->ReadByte();
      dlgscript[bb]=NULL;
      continue;
    }
    // add a large buffer because it will get added to if another option is added
    dlgscript[bb]=(char*)malloc(lenof + 20000);
    iii->Read(dlgscript[bb],lenof);
    if (encrypted)
      decrypt_text(dlgscript[bb]);
  }
  char stringbuffer[1000];
  for (bb=0;bb<thisgame.DialogMessageCount;bb++) {
    if ((filever >= kGameVersion_261) && (encrypted))
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

void write_plugins_to_disk (Stream *ooo) {
  int a;
  // version of plugin saving format
  ooo->WriteInt32 (1);
  ooo->WriteInt32 (numThisgamePlugins);
  
  for (a = 0; a < numThisgamePlugins; a++) {
      fputstring(thisgamePlugins[a].filename, ooo);
      
      int savesize = thisgamePlugins[a].dataSize;
      
      if ((savesize > SAVEBUFFERSIZE) || (savesize < 0)) {
		  MessageBox(NULL, "Plugin tried to write too much data to game file.", "", MB_OK);
        savesize = 0;
      }

      ooo->WriteInt32 (savesize);
      if (savesize > 0)
        ooo->Write (&thisgamePlugins[a].data[0], savesize);
  }
}

const char * read_plugins_from_disk (Stream *iii) {
  if (iii->ReadInt32() != 1) {
    return "ERROR: unable to load game, invalid version of plugin data";
  }

  numThisgamePlugins = iii->ReadInt32();

  for (int a = 0; a < numThisgamePlugins; a++) {
    // read the plugin name
    fgetstring (thisgamePlugins[a].filename, iii);
    int datasize = iii->ReadInt32();
    if (datasize > SAVEBUFFERSIZE) {
      return "Invalid plugin save data format, plugin data is lost";
    }
    // we don't care if it's an editor-only plugin or not
    if (thisgamePlugins[a].filename[strlen(thisgamePlugins[a].filename) - 1] == '!')
		thisgamePlugins[a].filename[strlen(thisgamePlugins[a].filename) - 1] = 0;

	thisgamePlugins[a].dataSize = datasize;
	if (datasize > 0)
	  iii->Read (thisgamePlugins[a].data, datasize);
  }
  return NULL;
}

void allocate_memory_for_views(int viewCount)
{
  numNewViews = 0;
	oldViews = (ViewStruct272*)calloc(sizeof(ViewStruct272) * viewCount, 1);
  newViews = (ViewStruct*)calloc(sizeof(ViewStruct) * viewCount, 1);
  thisgame.ViewNames.New(viewCount);
}

const char *load_dta_file_into_thisgame(const char *fileName)
{
  int bb;
  Stream*iii=Common::File::OpenFileRead(fileName);
  if (iii == NULL)
    return "Unable to open file";

  char buffer[40];
  iii->Read(buffer,30);
  buffer[30]=0;
  if (strcmp(buffer,game_file_sig)!=0) {
    delete iii;
    return "File contains invalid data and is not an AGS game.";
  }
  int filever = iii->ReadInt32();
  if (filever != kGameVersion_272) 
  {
	  delete iii;
	  return "This game was saved by an old version of AGS. This version of the editor can only import games saved with AGS 2.72.";
  }

  // skip required engine version
  int stlen = iii->ReadInt32();
  iii->Seek(Common::kSeekCurrent, stlen);

  {
    Common::AlignedStream align_s(iii, Common::kAligned_Read);
    thisgame.ReadBaseFromFile(&align_s);
    align_s.Close();
  }

  thisgame.FontFlags.ReadRaw(iii, thisgame.FontCount);
  thisgame.FontOutline.ReadRaw(iii, thisgame.FontCount);

  int numSprites = iii->ReadInt32();
  memset(&thisgame.SpriteFlags[0], 0, MAX_SPRITES);
  thisgame.SpriteFlags.ReadRawOver(iii, numSprites);
  thisgame.ReadInvInfo_Aligned(iii);
  thisgame.ReadMouseCursors_Aligned(iii);

  thisgame.CharacterInteractions.New(thisgame.CharacterCount, NULL);
  for (bb = 0; bb < thisgame.CharacterCount; bb++) {
    thisgame.CharacterInteractions[bb] = deserialize_new_interaction (iii);
  }
  for (bb = 0; bb < thisgame.InvItemCount; bb++) {
    delete thisgame.InvItemInteractions[bb];
    thisgame.InvItemInteractions[bb] = deserialize_new_interaction (iii);
  }

  numGlobalVars = iii->ReadInt32();
  iii->ReadArray (&globalvars[0], sizeof (InteractionVariable), numGlobalVars);

  if (thisgame.LoadDictionary != NULL) {
    thisgame.Dictionary = (WordsDictionary*)malloc(sizeof(WordsDictionary));
    read_dictionary (thisgame.Dictionary, iii);
  }

  if (thisgame.LoadCompiledScript != NULL)
    thisgame.CompiledScript = ccScript::CreateFromStream(iii);

  load_script_modules_compiled(iii);

  allocate_memory_for_views(thisgame.ViewCount);
  iii->ReadArray (&oldViews[0], sizeof(ViewStruct272), thisgame.ViewCount);

  thisgame.ReadCharacters_Aligned(iii);

  iii->ReadArray (&thisgame.LipSyncFrameLetters[0][0], 20, 50);

  for (bb=0;bb<MAXGLOBALMES;bb++) {
    if (!thisgame.MessageToLoad[bb]) continue;
    read_string_decrypt(iii, thisgame.GlobalMessages[bb]);
  }

  read_dialogs(iii, filever, true);
  Common::Gui::ReadGui(guis, iii);
  thisgame.GuiCount = guis.GetCount();
  const char *pluginError = read_plugins_from_disk (iii);
  if (pluginError != NULL) return pluginError;

  thisgame.CharacterProperties.New(thisgame.CharacterCount);

  if (thisgame.PropertySchema.UnSerialize (iii))
    return "unable to deserialize prop schema";

  int errors = 0;

  for (bb = 0; bb < thisgame.CharacterCount; bb++)
    errors += thisgame.CharacterProperties[bb].UnSerialize (iii);
  for (bb = 0; bb < thisgame.InvItemCount; bb++)
    errors += thisgame.InvItemProperties[bb].UnSerialize (iii);

  if (errors > 0)
    return "errors encountered reading custom props";

  for (bb = 0; bb < thisgame.ViewCount; bb++)
    thisgame.ViewNames[bb].ReadCount(iii, MAXVIEWNAMELENGTH);

  for (bb = 0; bb < thisgame.InvItemCount; bb++)
    thisgame.InventoryScriptNames[bb].ReadCount(iii, MAX_SCRIPT_NAME_LEN);

  for (bb = 0; bb < thisgame.DialogCount; bb++)
    thisgame.DialogScriptNames[bb].ReadCount(iii, MAX_SCRIPT_NAME_LEN);

  delete iii;

  for (bb = 0; bb < thisgame.GuiCount; bb++)
  {
	  guis[bb].RebuildArray();
  }

  // reset colour 0, it's possible for it to get corrupted
  palette[0].r = 0;
  palette[0].g = 0;
  palette[0].b = 0;
  set_palette_range(palette, 0, 255, 0);

  if (!reset_sprite_file())
    return "The sprite file could not be loaded. Ensure that all your game files are intact and not corrupt. The game may require a newer version of AGS.";

  for (bb=0;bb<MAX_FONTS;bb++) {
    wfreefont(bb);
  }
  for (bb=0;bb<thisgame.FontCount;bb++) {
    reload_font (bb);
  }

  update_abuf_coldepth();
  spritesModified = false;

  thisgame.FileVersion = filever;
  return NULL;
}

void free_script_module(int index) {
  free(scModules[index].name);
  free(scModules[index].author);
  free(scModules[index].version);
  free(scModules[index].description);
  free(scModules[index].script);
  free(scModules[index].scriptHeader);
  delete scModules[index].compiled;
  scModules[index].compiled = NULL;
}

void free_script_modules() {
  for (int i = 0; i < numScriptModules; i++)
    free_script_module(i);

  numScriptModules = 0;
}

void free_old_game_data()
{
  int bb;
  thisgame.GlobalMessages.Free();
  thisgame.CharacterProperties.Free();
  
  for (bb = 0; bb < thisgame.CharacterCount; bb++)
    delete thisgame.CharacterInteractions[bb];
  thisgame.CharacterInteractions.Free();

  for (bb = 0; bb < numNewViews; bb++)
  {
    for (int cc = 0; cc < newViews[bb].numLoops; cc++)
    {
      newViews[bb].loops[cc].Dispose();
    }
    newViews[bb].Dispose();
  }
  thisgame.ViewNames.Free();
  free(oldViews);
  free(newViews);
  guis.Free();
  thisgame.Characters.Free();
  thisgame.Dictionary->free_memory();
  free(thisgame.Dictionary);
  dialog.Free();
  free_script_modules();
}

// remap the scene, from its current palette oldpale to palette
void remap_background (Common::Bitmap *scene, color *oldpale, color*palette, int exactPal) {
  int a;  

  if (exactPal) {
    // exact palette import (for doing palette effects, don't change)
    for (a=0;a<256;a++) 
    {
      if (thisgame.PaletteUses[a] == PAL_BACKGROUND)
      {
        palette[a] = oldpale[a];
      }
    }
    return;
  }

  // find how many slots there are reserved for backgrounds
  int numbgslots=0;
  for (a=0;a<256;a++) { oldpale[a].filler=0;
    if (thisgame.PaletteUses[a]!=PAL_GAMEWIDE) numbgslots++;
  }
  // find which colours from the image palette are actually used
  int imgpalcnt[256],numimgclr=0;
  memset(&imgpalcnt[0],0,sizeof(imgpalcnt));
  if (scene->IsLinearBitmap()==0)
    quit("mem bitmap non-linear?");

  for (a=0;a<(scene->GetWidth()) * (scene->GetHeight());a++) {
    imgpalcnt[scene->GetScanLine(0)[a]]++;
  }
  for (a=0;a<256;a++) {
    if (imgpalcnt[a]>0) numimgclr++;
  }
  // count up the number of unique colours in the image
  int numclr=0,bb;
  color tpal[256];
  for (a=0;a<256;a++) {
    if (thisgame.PaletteUses[a]==PAL_BACKGROUND)
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
    while (thisgame.PaletteUses[palslt]!=PAL_BACKGROUND) {
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
    if (thisgame.PaletteUses[a]==PAL_GAMEWIDE) {
      tpal[a].r=0;
      tpal[a].g=0; tpal[a].b=0; 
    }
    else tpal[a]=palette[a];
  }
  wremapall(oldpale,scene,tpal); //palette);
}

void validate_mask(Common::Bitmap *toValidate, const char *name, int maxColour) {
  if ((toValidate == NULL) || (toValidate->GetColorDepth() != 8) ||
      (!toValidate->IsMemoryBitmap())) {
    quit("Invalid mask passed to validate_mask");
    return;
  }

  bool errFound = false;
  int xx, yy;
  for (yy = 0; yy < toValidate->GetHeight(); yy++) {
    for (xx = 0; xx < toValidate->GetWidth(); xx++) {
      if (toValidate->GetScanLine(yy)[xx] >= maxColour) {
        errFound = true;
        toValidate->GetScanLineForWriting(yy)[xx] = 0;
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
    if (thisgame.PaletteUses[ww] == PAL_BACKGROUND)
    {
      thisroom.Palette[ww] = thisroom.Backgrounds[0].Palette[ww];
      palette[ww] = thisroom.Backgrounds[0].Palette[ww];
    }
  }
}

void copy_global_palette_to_room_palette()
{
  for (int ww = 0; ww < 256; ww++) 
  {
    if (thisgame.PaletteUses[ww] != PAL_BACKGROUND)
      thisroom.Backgrounds[0].Palette[ww] = palette[ww];
  }
}

const char* load_room_file(const char*rtlo, int id) {

  AGS::Common::RoomInfo::Load(thisroom, (char*)rtlo, id, (thisgame.DefaultResolution > 2));

  if (thisroom.LoadedVersion < kRoomVersion_250b) 
  {
	  return "This room was saved with an old version of the editor and cannot be opened. Use AGS 2.72 to upgrade this room file.";
  }

  //thisroom.HotspotCount = LEGACY_MAX_ROOM_HOTSPOTS;

  // Allocate enough memory to add extra variables
  thisroom.LocalVariables.SetLength(MAX_GLOBAL_VARIABLES);

  // Update room palette with gamewide colours
  copy_global_palette_to_room_palette();
  // Update current global palette with room background colours
  copy_room_palette_to_global_palette();
  int ww;
  for (ww = 0; ww < thisroom.ObjectCount; ww++) {
    // change invalid objects to blue cup
    if (spriteset[thisroom.Objects[ww].SpriteIndex] == NULL)
      thisroom.Objects[ww].SpriteIndex = 0;
  }
  // Fix hi-color screens
  for (ww = 0; ww < thisroom.BkgSceneCount; ww++)
    fix_block (thisroom.Backgrounds[ww].Graphic);

  if ((thisroom.Resolution > 1) && (thisroom.WalkBehindMask->GetWidth() < thisroom.Width)) {
    // 640x400 room with 320x200-res walkbehind
    // resize it up to 640x400-res
    int oldw = thisroom.WalkBehindMask->GetWidth(), oldh=thisroom.WalkBehindMask->GetHeight();
    Common::Bitmap *tempb = Common::BitmapHelper::CreateBitmap(thisroom.Width, thisroom.Height, thisroom.WalkBehindMask->GetColorDepth());
    tempb->Fill(0);
    tempb->StretchBlt(thisroom.WalkBehindMask,RectWH(0,0,oldw,oldh),RectWH(0,0,tempb->GetWidth(),tempb->GetHeight()));
    delete thisroom.WalkBehindMask; 
    thisroom.WalkBehindMask = tempb;
  }

  set_palette_range(palette, 0, 255, 0);
  
  if ((thisroom.Backgrounds[0].Graphic->GetColorDepth () > 8) &&
      (thisgame.ColorDepth == 1))
    MessageBox(NULL,"WARNING: This room is hi-color, but your game is currently 256-colour. You will not be able to use this room in this game. Also, the room background will not look right in the editor.", "Colour depth warning", MB_OK);

  roomModified = false;

  validate_mask(thisroom.HotspotMask, "hotspot", LEGACY_MAX_ROOM_HOTSPOTS);
  validate_mask(thisroom.WalkBehindMask, "walk-behind", LEGACY_MAX_ROOM_WALKAREAS + 1);
  validate_mask(thisroom.WalkAreaMask, "walkable area", LEGACY_MAX_ROOM_WALKAREAS + 1);
  validate_mask(thisroom.RegionMask, "regions", LEGACY_MAX_ROOM_REGIONS);
  return NULL;
}

void calculate_walkable_areas () {
  int ww, thispix;

  for (ww = 0; ww <= LEGACY_MAX_ROOM_WALKAREAS; ww++) {
    thisroom.WalkAreas[ww].Top = thisroom.Height;
    thisroom.WalkAreas[ww].Bottom = 0;
  }
  for (ww = 0; ww < thisroom.WalkAreaMask->GetWidth(); ww++) {
    for (int qq = 0; qq < thisroom.WalkAreaMask->GetHeight(); qq++) {
      thispix = thisroom.WalkAreaMask->GetPixel (ww, qq);
      if (thispix > LEGACY_MAX_ROOM_WALKAREAS)
        continue;
      if (thisroom.WalkAreas[thispix].Top > qq)
        thisroom.WalkAreas[thispix].Top = qq;
      if (thisroom.WalkAreas[thispix].Bottom < qq)
        thisroom.WalkAreas[thispix].Bottom = qq;
    }
  }

}

void save_room(const char *files, const Common::RoomInfo &rstruc) {
  int               f;
  long              xoff, tesl;
  Stream            *opty;

  // TODO: if we don't support saving in old formats we should remove all
  // related conditional forks from serialization routine
  if (rstruc.LoadedVersion < kRoomVersion_Current)
    quit("save_room: can no longer save old format rooms");

  opty = ci_fopen(const_cast<char*>(files), Common::kFile_CreateAlways, Common::kFile_Write);
  if (opty == NULL)
    quit("save_room: unable to open room file for writing.");

  opty->WriteInt16(rstruc.LoadedVersion);

  if (rstruc.LoadedVersion >= kRoomVersion_pre114_5) {
    long blsii = 0;

    opty->WriteByte(Common::kRoomBlock_Main);
    opty->WriteInt32(blsii);
  }

  opty->WriteInt32(rstruc.BytesPerPixel);  // colour depth bytes per pixel
  opty->WriteInt16(rstruc.WalkBehindCount);
  for (int i = 0; i < rstruc.WalkBehindCount; ++i)
  {
    opty->WriteInt16(rstruc.WalkBehinds[i].Baseline);
  }

  opty->WriteInt32(rstruc.HotspotCount);
  for (int i = 0; i < rstruc.HotspotCount; ++i)
  {
      opty->WriteInt16(rstruc.Hotspots[i].WalkToPoint.x);
      opty->WriteInt16(rstruc.Hotspots[i].WalkToPoint.y);
  }
  for (f = 0; f < rstruc.HotspotCount; f++)
  {
	  fputstring(rstruc.Hotspots[f].Name, opty);
  }

  if (rstruc.LoadedVersion >= kRoomVersion_270)
  {
      for (int i = 0; i < rstruc.HotspotCount; ++i)
      {
          rstruc.Hotspots[i].ScriptName.WriteCount(opty, MAX_SCRIPT_NAME_LEN);
      }
  }

  opty->WriteInt32(rstruc.WalkAreaCount);
  for (int i = 0; i < rstruc.WalkAreaCount; ++i)
  {
      rstruc.WalkAreas[i].WallPoints.WriteToFile(opty);
  }

  opty->WriteInt16(rstruc.Edges.Top);
  opty->WriteInt16(rstruc.Edges.Bottom);
  opty->WriteInt16(rstruc.Edges.Left);
  opty->WriteInt16(rstruc.Edges.Right);
  opty->WriteInt16(rstruc.ObjectCount);
  for (int i = 0; i < rstruc.ObjectCount; ++i)
  {
      rstruc.Objects[i].WriteToFile(opty);
  }

  opty->WriteInt32 (rstruc.LocalVariableCount);
  if (rstruc.LocalVariableCount > 0) 
    opty->WriteArray (&rstruc.LocalVariables[0], sizeof(InteractionVariable), rstruc.LocalVariableCount);
/*
  for (f = 0; f < rstruc.HotspotCount; f++)
    serialize_new_interaction (rstruc.Hotspots[].EventHandlers.Interaction[f]);
  for (f = 0; f < rstruc.ObjectCount; f++)
    serialize_new_interaction (rstruc.Objects[].EventHandlers.Interaction[f]);
  serialize_new_interaction (rstruc.EventHandlers.Interaction);
*/
  opty->WriteInt32 (LEGACY_MAX_ROOM_REGIONS);
  /*
  for (f = 0; f < LEGACY_MAX_ROOM_REGIONS; f++)
    serialize_new_interaction (rstruc.Regions[].EventHandlers.Interaction[f]);
	*/
  serialize_room_interactions(opty);

  for (int i = 0; i < rstruc.ObjectCount; ++i)
  {
    opty->WriteInt32(rstruc.Objects[i].Baseline);
  }
  opty->WriteInt16(rstruc.Width);
  opty->WriteInt16(rstruc.Height);

  if (rstruc.LoadedVersion >= kRoomVersion_262)
  {
    for (int i = 0; i < rstruc.ObjectCount; ++i)
    {
        opty->WriteInt16(rstruc.Objects[i].Flags);
    }
  }

  if (rstruc.LoadedVersion >= kRoomVersion_200_final)
    opty->WriteInt16(rstruc.Resolution);

  // write the zoom and light levels
  opty->WriteInt32 (LEGACY_MAX_ROOM_WALKAREAS + 1);
  for (int i = 0; i < LEGACY_MAX_ROOM_WALKAREAS + 1; ++i)
  {
      opty->WriteInt16(rstruc.WalkAreas[i].Zoom);
  }
  for (int i = 0; i < LEGACY_MAX_ROOM_WALKAREAS + 1; ++i)
  {
      opty->WriteInt16(rstruc.WalkAreas[i].Light);
  }
  for (int i = 0; i < LEGACY_MAX_ROOM_WALKAREAS + 1; ++i)
  {
      opty->WriteInt16(rstruc.WalkAreas[i].Zoom2);
  }
  for (int i = 0; i < LEGACY_MAX_ROOM_WALKAREAS + 1; ++i)
  {
      opty->WriteInt16(rstruc.WalkAreas[i].Top);
  }
  for (int i = 0; i < LEGACY_MAX_ROOM_WALKAREAS + 1; ++i)
  {
      opty->WriteInt16(rstruc.WalkAreas[i].Bottom);
  }

  opty->WriteBool(rstruc.IsPersistent);
  opty->Write(&rstruc.Options, 10);
  opty->WriteInt16(rstruc.MessageCount);

  if (rstruc.LoadedVersion >= kRoomVersion_272)
    opty->WriteInt32(rstruc.GameId);
 
  if (rstruc.LoadedVersion >= kRoomVersion_pre114_3)
  {
    for (int i = 0; i < rstruc.MessageCount; ++i)
    {
      rstruc.MessageInfos[i].WriteToFile(opty);
    }
  }

  for (f = 0; f < rstruc.MessageCount; f++)
    write_string_encrypt(opty, rstruc.Messages[f]);
//    fputstring(rstruc.Messages[f]);

  if (rstruc.LoadedVersion >= kRoomVersion_pre114_6) {
    // we no longer use animations, remove them
    //rstruc.AnimationCount = 0;
    opty->WriteInt16(0);

    /*
    if (rstruc.AnimationCount > 0)
    {
      // CHECKME: what versions were those animations used in?
      opty->WriteByteCount(0, sizeof(FullAnimation) * rstruc.AnimationCount);
    }
    */
  }

  if ((rstruc.LoadedVersion >= kRoomVersion_pre114_4) && (rstruc.LoadedVersion < kRoomVersion_250a)) {
    rstruc.SaveScriptConfiguration(opty);
    rstruc.SaveGraphicalScripts(opty);
  }

  if (rstruc.LoadedVersion >= kRoomVersion_114)
  {
    for (int i = 0; i < LEGACY_MAX_ROOM_WALKAREAS + 1; ++i)
    {
      opty->WriteInt16(rstruc.WalkAreas[i].ShadingView);
    }
  }

  if (rstruc.LoadedVersion >= kRoomVersion_255b)
  {
    for (int i = 0; i < LEGACY_MAX_ROOM_REGIONS; ++i)
    {
      opty->WriteInt16(rstruc.Regions[i].Light);
    }
    for (int i = 0; i < LEGACY_MAX_ROOM_REGIONS; ++i)
    {
        opty->WriteInt32(rstruc.Regions[i].Tint);
    }
  }

  xoff = opty->GetPosition();
  delete opty;

  tesl = save_lzw((char*)files, rstruc.Backgrounds[0].Graphic, rstruc.Palette, xoff);

  tesl = savecompressed_allegro((char*)files, rstruc.RegionMask, rstruc.Palette, tesl);
  tesl = savecompressed_allegro((char*)files, rstruc.WalkAreaMask, rstruc.Palette, tesl);
  tesl = savecompressed_allegro((char*)files, rstruc.WalkBehindMask, rstruc.Palette, tesl);
  tesl = savecompressed_allegro((char*)files, rstruc.HotspotMask, rstruc.Palette, tesl);

  if (rstruc.LoadedVersion >= kRoomVersion_pre114_5) {
    long  lee;

    opty = ci_fopen(files, Common::kFile_Open, Common::kFile_ReadWrite);
    lee = opty->GetLength()-7;

    opty->Seek(Common::kSeekBegin, 3);
    opty->WriteInt32(lee);
    opty->Seek(Common::kSeekEnd, 0);

    if (rstruc.TextScript != NULL) {
      int hh;

      opty->WriteByte(Common::kRoomBlock_Script);
      lee = (int)strlen(rstruc.TextScript) + 4;
      opty->WriteInt32(lee);
      lee -= 4;

      for (hh = 0; hh < lee; hh++)
        rstruc.TextScript[hh]-=passwencstring[hh % 11];

      opty->WriteInt32(lee);
      opty->Write(rstruc.TextScript, lee);

      for (hh = 0; hh < lee; hh++)
        rstruc.TextScript[hh]+=passwencstring[hh % 11];

    }
   
    if (rstruc.CompiledScript != NULL) {
      long  leeat, wasat;

      opty->WriteByte(Common::kRoomBlock_CompScript3);
      lee = 0;
      leeat = opty->GetPosition();
      opty->WriteInt32(lee);
      rstruc.CompiledScript->Write(opty);
     
      wasat = opty->GetPosition();
      opty->Seek(Common::kSeekBegin, leeat);
      lee = (wasat - leeat) - 4;
      opty->WriteInt32(lee);
      opty->Seek(Common::kSeekEnd, 0);
    }

    if (rstruc.ObjectCount > 0) {
      opty->WriteByte(Common::kRoomBlock_ObjectNames);
      lee=rstruc.ObjectCount * MAXOBJNAMELEN + 1;
      opty->WriteInt32(lee);
      opty->WriteByte(rstruc.ObjectCount);
      for (int i = 0; i < rstruc.ObjectCount; ++i)
      {
        rstruc.Objects[i].Name.WriteCount(opty, MAXOBJNAMELEN);
      }

      opty->WriteByte(Common::kRoomBlock_ObjectScriptNames);
      lee = rstruc.ObjectCount * MAX_SCRIPT_NAME_LEN + 1;
      opty->WriteInt32(lee);
      opty->WriteByte(rstruc.ObjectCount);
      for (int i = 0; i < rstruc.ObjectCount; ++i)
      {
          rstruc.Objects[i].ScriptName.WriteCount(opty, MAX_SCRIPT_NAME_LEN);
      }
    }

    long lenpos, lenis;
    int gg;

    if (rstruc.BkgSceneCount > 1) {
      long  curoffs;

      opty->WriteByte(Common::kRoomBlock_AnimBkg);
      lenpos = opty->GetPosition();
      lenis = 0;
      opty->WriteInt32(lenis);
      opty->WriteByte(rstruc.BkgSceneCount);
      opty->WriteByte(rstruc.BkgSceneAnimSpeed);
      for (int i = 0; i < rstruc.BkgSceneCount; ++i)
      {
          opty->WriteInt8(rstruc.Backgrounds[i].PaletteShared);
      }

      delete opty;

      curoffs = lenpos + 6 + rstruc.BkgSceneCount;
      for (gg = 1; gg < rstruc.BkgSceneCount; gg++)
        curoffs = save_lzw((char*)files, rstruc.Backgrounds[gg].Graphic, rstruc.Backgrounds[gg].Palette, curoffs);

      opty = ci_fopen(const_cast<char*>(files), Common::kFile_Open, Common::kFile_ReadWrite);
      lenis = (curoffs - lenpos) - 4;
      opty->Seek(Common::kSeekBegin, lenpos);
      opty->WriteInt32(lenis);
      opty->Seek(Common::kSeekEnd, 0);
    }

    // Write custom properties
    opty->WriteByte (Common::kRoomBlock_Properties);
    lenpos = opty->GetPosition();
    lenis = 0;
    opty->WriteInt32(lenis);
    opty->WriteInt32 (1);  // Version 1 of properties block
    rstruc.Properties.Serialize (opty);
    for (gg = 0; gg < rstruc.HotspotCount; gg++)
      rstruc.Hotspots[gg].Properties.Serialize (opty);
    for (gg = 0; gg < rstruc.ObjectCount; gg++)
      rstruc.Objects[gg].Properties.Serialize (opty);

    lenis = (opty->GetPosition() - lenpos) - 4;
    opty->Seek(Common::kSeekBegin, lenpos);
    opty->WriteInt32(lenis);
    opty->Seek(Common::kSeekEnd, 0);

    // Write EOF block
    opty->WriteByte(Common::kRoomBlock_End);
    delete opty;
  }

//  fclose(opty);
//  return SUCCESS;
}

void save_room_file(const char*rtsa) 
{
  thisroom.LoadedVersion=kRoomVersion_Current;
  copy_room_palette_to_global_palette();

  calculate_walkable_areas();

  thisroom.BytesPerPixel = thisroom.Backgrounds[0].Graphic->GetBPP();
  int ww;
  // Fix hi-color screens
  for (ww = 0; ww < thisroom.BkgSceneCount; ww++)
    fix_block (thisroom.Backgrounds[ww].Graphic);

  thisroom.WalkBehindCount = LEGACY_MAX_ROOM_WALKBEHINDS;
  save_room((char*)rtsa,thisroom);

  // Fix hi-color screens back again
  for (ww = 0; ww < thisroom.BkgSceneCount; ww++)
    fix_block (thisroom.Backgrounds[ww].Graphic);
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

//static const char *tempSetting = "My\x1\xde\x4Jibzle";  // clib password
//extern void init_pseudo_rand_gen(int seed);
//extern int get_pseudo_rand();
const int RAND_SEED_SALT = 9338638;  // must update clib32.cpp if this changes

void fwrite_data_enc(const void *data, int dataSize, int dataCount, Stream *ooo)
{
  const unsigned char *dataChar = (const unsigned char*)data;
  for (int i = 0; i < dataSize * dataCount; i++)
  {
    ooo->WriteByte(dataChar[i] + Common::AssetManager::GetNextPseudoRand());
  }
}

void fputstring_enc(const char *sss, Stream *ooo) 
{
  fwrite_data_enc(sss, 1, strlen(sss) + 1, ooo);
}

void putw_enc(int numberToWrite, Stream *ooo)
{
  fwrite_data_enc(&numberToWrite, 4, 1, ooo);
}

void write_clib_header(Stream*wout) {
  int ff;
  int randSeed = (int)time(NULL);
  wout->WriteInt32(randSeed - RAND_SEED_SALT);
  Common::AssetManager::InitPseudoRand(randSeed);
  putw_enc(ourlib.num_data_files, wout);
  for (ff = 0; ff < ourlib.num_data_files; ff++)
  {
    fputstring_enc(ourlib.data_filenames[ff], wout);
  }
  putw_enc(ourlib.num_files, wout);
  for (ff = 0; ff < ourlib.num_files; ff++) 
  {
    fputstring_enc(ourlib.filenames[ff], wout);
  }
  fwrite_data_enc(&ourlib.offset[0],4,ourlib.num_files, wout);
  fwrite_data_enc(&ourlib.length[0],4,ourlib.num_files, wout);
  fwrite_data_enc(&ourlib.file_datafile[0],1,ourlib.num_files, wout);
}


#define CHUNKSIZE 256000
int copy_file_across(Stream*inlibb,Stream*coppy,long leftforthis) {
  int success = 1;
  char*diskbuffer=(char*)malloc(CHUNKSIZE+10);
  while (leftforthis>0) {
    if (leftforthis>CHUNKSIZE) {
      inlibb->Read(diskbuffer,CHUNKSIZE);
      success = coppy->Write(diskbuffer,CHUNKSIZE);
      leftforthis-=CHUNKSIZE;
    }
    else {
      inlibb->Read(diskbuffer,leftforthis);
      success = coppy->Write(diskbuffer,leftforthis);
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

    Stream*ddd = Common::File::OpenFileRead(filenames[a]);
    if (ddd==NULL) { 
      filesizes[a] = 0;
      continue;
    }
    filesizes[a] = ddd->GetLength();
    delete ddd;

    for (int bb = 0; writefname[a][bb] != 0; bb++)
      writefname[a][bb] += passwmod;
  }
  // write the header
  Stream*wout=Common::File::CreateFile(dataFileName);
  wout->Write("CLIB\x1a",5);
  wout->WriteByte(6);  // version
  wout->WriteByte(passwmod);  // password modifier
  wout->WriteByte(0);  // reserved
  wout->WriteInt16(numfile);
  for (a=0;a<13;a++) wout->WriteByte(0);  // the password
  wout->WriteArray(&writefname[0][0],13,numfile);
  wout->WriteArrayOfInt32((int32_t*)&filesizes[0],numfile);
  for (a=0;a<2*numfile;a++) wout->WriteByte(0);  // comp.ratio

  // now copy the data
  for (a=0;a<numfile;a++) {

	Stream*iii = Common::File::OpenFileRead(filenames[a]);
    if (iii==NULL) {
      errorMsg = "unable to add one of the files to data file.";
      continue;
    }
    if (copy_file_across(iii,wout,filesizes[a]) < 1) {
      errorMsg = "Error writing file: possibly disk full";
      delete iii;
      break;
    }
    delete iii;
  }
  delete wout;
  free(filesizes);
  free(writefname[0]);
  free(writefname);

  if (errorMsg != NULL) 
  {
	unlink(dataFileName);
  }

  return errorMsg;
}

Stream* find_file_in_path(char *buffer, const char *fileName)
{
	char tomake[MAX_PATH];
	strcpy(tomake, fileName);
	Stream* iii = Common::AssetManager::OpenAsset(tomake);
	if (iii == NULL) {
	  // try in the Audio folder if not found
	  sprintf(tomake, "AudioCache\\%s", fileName);
	  iii = Common::AssetManager::OpenAsset(tomake);
	}
	if (iii == NULL) {
	  // no? maybe Speech then, templates include this
	  sprintf(tomake, "Speech\\%s", fileName);
	  iii = Common::AssetManager::OpenAsset(tomake);
	}

	if (buffer != NULL)
	  strcpy(buffer, tomake);

	return iii;
}

const char* make_data_file(int numFiles, char * const*fileNames, long splitSize, const char *baseFileName, bool makeFileNameAssumptionsForEXE)
{
  int a,b;
  Stream*wout;
  char tomake[MAX_PATH];
  ourlib.num_data_files = 0;
  ourlib.num_files = numFiles;
  Common::AssetManager::SetSearchPriority(Common::kAssetPriorityDir);

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
	  Stream *tf = Common::File::OpenFileRead(fileNames[a]);
	  thisFileSize = tf->GetLength();
	  delete tf;
	  
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
	Stream *iii = find_file_in_path(tomake, ourlib.filenames[b]);
	if (iii != NULL)
	{
		delete iii;

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

	  wout = Common::File::OpenFile(outputFileName,
          (a == 0) ? Common::kFile_Create : Common::kFile_CreateAlways, Common::kFile_Write);
	  if (wout == NULL) 
	  {
		  return "ERROR: unable to open file for writing";
	  }

	  startOffset = wout->GetLength();
    wout->Write("CLIB\x1a",5);
    wout->WriteByte(21);  // version
    wout->WriteByte(a);   // file number

    if (a == 0) 
	{
      mainHeaderOffset = wout->GetPosition();
      write_clib_header(wout);
    }

    for (b=0;b<ourlib.num_files;b++) {
      if (ourlib.file_datafile[b] == a) {
        ourlib.offset[b] = wout->GetPosition() - startOffset;

		Stream *iii = find_file_in_path(NULL, ourlib.filenames[b]);
        if (iii == NULL) {
          delete wout;
          unlink(outputFileName);

		  char buffer[500];
		  sprintf(buffer, "Unable to find file '%s' for compilation. Do not remove files during the compilation process.", ourlib.filenames[b]);
		  ThrowManagedException(buffer);
        }

        if (copy_file_across(iii,wout,ourlib.length[b]) < 1) {
          delete iii;
          return "Error writing file: possibly disk full";
        }
        delete iii;
      }
    }
	if (startOffset > 0)
	{
		wout->WriteInt32(startOffset);
		wout->Write(clibendsig, 12);
	}
    delete wout;
  }

  wout = Common::File::OpenFile(firstDataFileFullPath, Common::kFile_Open, Common::kFile_ReadWrite);
  wout->Seek(Common::kSeekBegin, mainHeaderOffset);
  write_clib_header(wout);
  delete wout;
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
void ConvertStringToNativeString(System::String^ clrString, Common::String &destStr);
void ConvertStringToNativeString(System::String^ clrString, Common::String &destStr, int maxLength);

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
	drawBuffer = Common::BitmapHelper::CreateBitmap( width, height, 32);
	drawBuffer->Clear(0x00D0D0D0);
}

void DrawSpriteToBuffer(int sprNum, int x, int y, int scaleFactor) {
	Common::Bitmap *todraw = spriteset[sprNum];
	if (todraw == NULL)
	  todraw = spriteset[0];

	if (((thisgame.SpriteFlags[sprNum] & SPF_640x400) == 0) &&
		(thisgame.DefaultResolution > 2))
	{
		scaleFactor *= 2;
	}

	Common::Bitmap *imageToDraw = todraw;

	if (todraw->GetColorDepth() != drawBuffer->GetColorDepth()) 
	{
		int oldColorConv = get_color_conversion();
		set_color_conversion(oldColorConv | COLORCONV_KEEP_TRANS);
		Common::Bitmap *depthConverted = Common::BitmapHelper::CreateBitmapCopy(todraw, drawBuffer->GetColorDepth());
		set_color_conversion(oldColorConv);

		imageToDraw = depthConverted;
	}

	int drawWidth = imageToDraw->GetWidth() * scaleFactor;
	int drawHeight = imageToDraw->GetHeight() * scaleFactor;

	if ((thisgame.SpriteFlags[sprNum] & SPF_ALPHACHANNEL) != 0)
	{
		if (scaleFactor > 1)
		{
			Common::Bitmap *resizedImage = Common::BitmapHelper::CreateBitmap(drawWidth, drawHeight, imageToDraw->GetColorDepth());
			resizedImage->StretchBlt(imageToDraw, RectWH(0, 0, imageToDraw->GetWidth(), imageToDraw->GetHeight()),
				RectWH(0, 0, resizedImage->GetWidth(), resizedImage->GetHeight()));
			if (imageToDraw != todraw)
				delete imageToDraw;
			imageToDraw = resizedImage;
		}
		set_alpha_blender();
		drawBuffer->TransBlendBlt(imageToDraw, x, y);
	}
	else
	{
		Cstretch_sprite(drawBuffer, imageToDraw, x, y, drawWidth, drawHeight);
	}

	if (imageToDraw != todraw)
		delete imageToDraw;
}

void RenderBufferToHDC(int hdc) 
{
	blit_to_hdc(drawBuffer->GetAllegroBitmap(), (HDC)hdc, 0, 0, 0, 0, drawBuffer->GetWidth(), drawBuffer->GetHeight());
	delete drawBuffer;
	drawBuffer = NULL;
}

void UpdateSpriteFlags(SpriteFolder ^folder) 
{
	for each (Sprite ^sprite in folder->Sprites)
	{
		thisgame.SpriteFlags[sprite->Number] = 0;
		if (sprite->Resolution == SpriteImportResolution::HighRes)
			thisgame.SpriteFlags[sprite->Number] |= SPF_640x400;
		if (sprite->AlphaChannel)
			thisgame.SpriteFlags[sprite->Number] |= SPF_ALPHACHANNEL;
	}

	for each (SpriteFolder^ subFolder in folder->SubFolders) 
	{
		UpdateSpriteFlags(subFolder);
	}
}

void GameUpdated(Game ^game) {
  thisgame.ColorDepth = (int)game->Settings->ColorDepth;
  thisgame.DefaultResolution = (int)game->Settings->Resolution;

  thisgame.Options[OPT_NOSCALEFNT] = game->Settings->FontsForHiRes;
  thisgame.Options[OPT_ANTIALIASFONTS] = game->Settings->AntiAliasFonts;
  antiAliasFonts = thisgame.Options[OPT_ANTIALIASFONTS];
  update_font_sizes();

  //delete abuf;
  //abuf = Common::BitmapHelper::CreateBitmap(32, 32, thisgame.ColorDepth * 8);
  BaseColorDepth = thisgame.ColorDepth * 8;

  // ensure that the sprite import knows about pal slots 
  for (int i = 0; i < 256; i++) {
	if (game->Palette[i]->ColourType == PaletteColourType::Background)
	{
	  thisgame.PaletteUses[i] = PAL_BACKGROUND;
	}
	else 
	{
  	  thisgame.PaletteUses[i] = PAL_GAMEWIDE;
    }
  }

  thisgame.FontCount = game->Fonts->Count;
  thisgame.FontFlags.SetLength(thisgame.FontCount);
  thisgame.FontOutline.SetLength(thisgame.FontCount);
  for (int i = 0; i < thisgame.FontCount; i++) 
  {
	  thisgame.FontFlags[i] &= ~FFLG_SIZEMASK;
	  thisgame.FontFlags[i] |= game->Fonts[i]->PointSize;
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

Common::Bitmap *CreateBlockFromBitmap(System::Drawing::Bitmap ^bmp, color *imgpal, bool fixColourDepth, bool keepTransparency, int *originalColDepth) 
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

  if ((thisgame.ColorDepth == 1) && (colDepth > 8))
  {
    throw gcnew AGSEditorException("You cannot import a hi-colour or true-colour image into a 256-colour game.");
  }

  if (originalColDepth != NULL)
    *originalColDepth = colDepth;

  bool needToFixColourDepth = false;
  if ((colDepth != thisgame.ColorDepth * 8) && (fixColourDepth))
  {
    needToFixColourDepth = true;
  }

	Common::Bitmap *tempsprite = Common::BitmapHelper::CreateBitmap(bmp->Width, bmp->Height, colDepth);

	System::Drawing::Rectangle rect(0, 0, bmp->Width, bmp->Height);
	BitmapData ^bmpData = bmp->LockBits(rect, ImageLockMode::ReadWrite, bmp->PixelFormat);
	unsigned char *address = (unsigned char*)bmpData->Scan0.ToPointer();
	for (int y = 0; y < tempsprite->GetHeight(); y++) {
	  memcpy(&tempsprite->GetScanLineForWriting(y)[0], address, bmp->Width * ((colDepth + 1) / 8));
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
		Common::Bitmap *spriteAtRightDepth = Common::BitmapHelper::CreateBitmap(tempsprite->GetWidth(), tempsprite->GetHeight(), thisgame.ColorDepth * 8);
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

		spriteAtRightDepth->Blit(tempsprite, 0, 0, 0, 0, tempsprite->GetWidth(), tempsprite->GetHeight());

		set_color_conversion(oldColorConv);

		if (colDepth == 8) 
		{
			unselect_palette();
		}
		delete tempsprite;
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
	Common::RoomInfo *theRoom = (Common::RoomInfo*)(void*)room->_roomStructPtr;
	delete theRoom->Backgrounds[backgroundNumber].Graphic;
	theRoom->Backgrounds[backgroundNumber].Graphic = NULL;
	
	theRoom->BkgSceneCount--;
	room->BackgroundCount--;
	for (int i = backgroundNumber; i < theRoom->BkgSceneCount; i++) 
	{
		theRoom->Backgrounds[i].Graphic = theRoom->Backgrounds[i + 1].Graphic;
		theRoom->Backgrounds[i].PaletteShared = theRoom->Backgrounds[i + 1].PaletteShared;
	}
}

void ImportBackground(Room ^room, int backgroundNumber, System::Drawing::Bitmap ^bmp, bool useExactPalette, bool sharePalette) 
{
	color oldpale[256];
	Common::Bitmap *newbg = CreateBlockFromBitmap(bmp, oldpale, true, false, NULL);
	Common::RoomInfo *theRoom = (Common::RoomInfo*)(void*)room->_roomStructPtr;
	theRoom->Width = room->Width;
	theRoom->Height = room->Height;
	bool resolutionChanged = (theRoom->Resolution != (int)room->Resolution);
	theRoom->Resolution = (int)room->Resolution;

	if (newbg->GetColorDepth() == 8) 
	{
		for (int aa = 0; aa < 256; aa++) {
		  // make sure it maps to locked cols properly
		  if (thisgame.PaletteUses[aa] == PAL_LOCKED)
			  theRoom->Backgrounds[backgroundNumber].Palette[aa] = palette[aa];
		}

		// sharing palette with main background - so copy it across
		if (sharePalette) {
		  memcpy (&theRoom->Backgrounds[backgroundNumber].Palette[0], &palette[0], sizeof(color) * 256);
		  theRoom->Backgrounds[backgroundNumber].PaletteShared = 1;
		  if (backgroundNumber >= theRoom->BkgSceneCount - 1)
		  	theRoom->Backgrounds[0].PaletteShared = 1;

		  if (!useExactPalette)
			wremapall(oldpale, newbg, palette);
		}
		else {
		  theRoom->Backgrounds[backgroundNumber].PaletteShared = 0;
		  remap_background (newbg, oldpale, theRoom->Backgrounds[backgroundNumber].Palette, useExactPalette);
		}

    copy_room_palette_to_global_palette();
	}

	if (backgroundNumber >= theRoom->BkgSceneCount) 
	{
		theRoom->BkgSceneCount++;
	}
	else 
	{
		delete theRoom->Backgrounds[backgroundNumber].Graphic;
	}
	theRoom->Backgrounds[backgroundNumber].Graphic = newbg;

  // if size or resolution has changed, reset masks
	if ((newbg->GetWidth() != theRoom->WalkBehindMask->GetWidth()) || (newbg->GetHeight() != theRoom->WalkBehindMask->GetHeight()) ||
      (theRoom->Width != theRoom->WalkBehindMask->GetWidth()) || (resolutionChanged))
	{
		delete theRoom->WalkAreaMask;
		delete theRoom->HotspotMask;
		delete theRoom->WalkBehindMask;
		delete theRoom->RegionMask;
		theRoom->WalkAreaMask = Common::BitmapHelper::CreateBitmap(theRoom->Width / theRoom->Resolution, theRoom->Height / theRoom->Resolution,8);
		theRoom->HotspotMask = Common::BitmapHelper::CreateBitmap(theRoom->Width / theRoom->Resolution, theRoom->Height / theRoom->Resolution,8);
		theRoom->WalkBehindMask = Common::BitmapHelper::CreateBitmap(theRoom->Width, theRoom->Height,8);
		theRoom->RegionMask = Common::BitmapHelper::CreateBitmap(theRoom->Width / theRoom->Resolution, theRoom->Height / theRoom->Resolution,8);
		theRoom->WalkAreaMask->Clear();
		theRoom->HotspotMask->Clear();
		theRoom->WalkBehindMask->Clear();
		theRoom->RegionMask->Clear();
	}

	room->BackgroundCount = theRoom->BkgSceneCount;
	room->ColorDepth = theRoom->Backgrounds[0].Graphic->GetColorDepth();
}

void import_area_mask(void *roomptr, int maskType, System::Drawing::Bitmap ^bmp)
{
	color oldpale[256];
	Common::Bitmap *importedImage = CreateBlockFromBitmap(bmp, oldpale, false, false, NULL);
	Common::Bitmap *mask = get_bitmap_for_mask((Common::RoomInfo*)roomptr, (RoomAreaMask)maskType);

	if (mask->GetWidth() != importedImage->GetWidth())
	{
		// allow them to import a double-size or half-size mask, adjust it as appropriate
		Cstretch_blit(importedImage, mask, 0, 0, importedImage->GetWidth(), importedImage->GetHeight(), 0, 0, mask->GetWidth(), mask->GetHeight());
	}
	else
	{
		mask->Blit(importedImage, 0, 0, 0, 0, importedImage->GetWidth(), importedImage->GetHeight());
	}
	delete importedImage;

	validate_mask(mask, "imported", (maskType == Hotspots) ? LEGACY_MAX_ROOM_HOTSPOTS : (LEGACY_MAX_ROOM_WALKAREAS + 1));
}

void set_rgb_mask_from_alpha_channel(Common::Bitmap *image)
{
	for (int y = 0; y < image->GetHeight(); y++)
	{
		unsigned long* thisLine = (unsigned long*)image->GetScanLine(y);
		for (int x = 0; x < image->GetWidth(); x++)
		{
			if ((thisLine[x] & 0xff000000) == 0)
			{
				thisLine[x] = MASK_COLOR_32;
			}
		}
	}
}

void set_opaque_alpha_channel(Common::Bitmap *image)
{
	for (int y = 0; y < image->GetHeight(); y++)
	{
		unsigned long* thisLine = (unsigned long*)image->GetScanLine(y);
		for (int x = 0; x < image->GetWidth(); x++)
		{
			if (thisLine[x] != MASK_COLOR_32)
			  thisLine[x] |= 0xff000000;
		}
	}
}

int SetNewSpriteFromBitmap(int slot, System::Drawing::Bitmap^ bmp, int spriteImportMethod, bool remapColours, bool useRoomBackgroundColours, bool alphaChannel) 
{
	color imgPalBuf[256];
  int importedColourDepth;
	Common::Bitmap *tempsprite = CreateBlockFromBitmap(bmp, imgPalBuf, true, (spriteImportMethod != SIMP_NONE), &importedColourDepth);

	if ((remapColours) || (thisgame.ColorDepth > 1)) 
	{
		sort_out_transparency(tempsprite, spriteImportMethod, imgPalBuf, useRoomBackgroundColours, importedColourDepth);
	}

	thisgame.SpriteFlags[slot] = 0;
	if (thisgame.DefaultResolution > 2)
	{
		thisgame.SpriteFlags[slot] |= SPF_640x400;
	}
	if (alphaChannel)
	{
		thisgame.SpriteFlags[slot] |= SPF_ALPHACHANNEL;

		if (tempsprite->GetColorDepth() == 32)
		{
			set_rgb_mask_from_alpha_channel(tempsprite);
		}
	}
	else if (tempsprite->GetColorDepth() == 32)
	{
		set_opaque_alpha_channel(tempsprite);
	}

	SetNewSprite(slot, tempsprite);

	return (thisgame.DefaultResolution > 2) ? 1 : 0;
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

System::Drawing::Bitmap^ ConvertBlockToBitmap(Common::Bitmap *todraw, bool useAlphaChannel) 
{
  fix_block(todraw);

  PixelFormat pixFormat = PixelFormat::Format32bppRgb;
  if ((todraw->GetColorDepth() == 32) && (useAlphaChannel))
	  pixFormat = PixelFormat::Format32bppArgb;
  else if (todraw->GetColorDepth() == 24)
    pixFormat = PixelFormat::Format24bppRgb;
  else if (todraw->GetColorDepth() == 16)
    pixFormat = PixelFormat::Format16bppRgb565;
  else if (todraw->GetColorDepth() == 15)
    pixFormat = PixelFormat::Format16bppRgb555;
  else if (todraw->GetColorDepth() == 8)
    pixFormat = PixelFormat::Format8bppIndexed;
  int bytesPerPixel = (todraw->GetColorDepth() + 1) / 8;

  System::Drawing::Bitmap ^bmp = gcnew System::Drawing::Bitmap(todraw->GetWidth(), todraw->GetHeight(), pixFormat);
  System::Drawing::Rectangle rect(0, 0, bmp->Width, bmp->Height);
  BitmapData ^bmpData = bmp->LockBits(rect, ImageLockMode::WriteOnly, bmp->PixelFormat);
  unsigned char *address = (unsigned char*)bmpData->Scan0.ToPointer();
  for (int y = 0; y < todraw->GetHeight(); y++) {
    memcpy(address, &todraw->GetScanLine(y)[0], bmp->Width * bytesPerPixel);
    address += bmpData->Stride;
  }
  bmp->UnlockBits(bmpData);

  if (pixFormat == PixelFormat::Format8bppIndexed)
    SetBitmapPaletteFromGlobalPalette(bmp);

  fix_block(todraw);
  return bmp;
}

System::Drawing::Bitmap^ ConvertBlockToBitmap32(Common::Bitmap *todraw, int width, int height, bool useAlphaChannel) 
{
  Common::Bitmap *tempBlock = Common::BitmapHelper::CreateBitmap(todraw->GetWidth(), todraw->GetHeight(), 32);
	
  if (todraw->GetColorDepth() == 8)
    select_palette(palette);

  tempBlock->Blit(todraw, 0, 0, 0, 0, todraw->GetWidth(), todraw->GetHeight());

  if (todraw->GetColorDepth() == 8)
	unselect_palette();

  if ((width != todraw->GetWidth()) || (height != todraw->GetHeight())) 
  {
	  Common::Bitmap *newBlock = Common::BitmapHelper::CreateBitmap(width, height, 32);
	  Cstretch_blit(tempBlock, newBlock, 0, 0, todraw->GetWidth(), todraw->GetHeight(), 0, 0, width, height);
	  delete tempBlock;
	  tempBlock = newBlock;
  }

  fix_block(tempBlock);

  PixelFormat pixFormat = PixelFormat::Format32bppRgb;
  if ((todraw->GetColorDepth() == 32) && (useAlphaChannel))
	  pixFormat = PixelFormat::Format32bppArgb;

  System::Drawing::Bitmap ^bmp = gcnew System::Drawing::Bitmap(width, height, pixFormat);
  System::Drawing::Rectangle rect(0, 0, bmp->Width, bmp->Height);
  BitmapData ^bmpData = bmp->LockBits(rect, ImageLockMode::WriteOnly, bmp->PixelFormat);
  unsigned char *address = (unsigned char*)bmpData->Scan0.ToPointer();
  for (int y = 0; y < tempBlock->GetHeight(); y++) {
    memcpy(address, &tempBlock->GetScanLine(y)[0], bmp->Width * 4);
    address += bmpData->Stride;
  }
  bmp->UnlockBits(bmpData);
  delete tempBlock;
  return bmp;
}

System::Drawing::Bitmap^ ConvertAreaMaskToBitmap(Common::Bitmap *mask) 
{
	System::Drawing::Bitmap ^bmp = gcnew System::Drawing::Bitmap(mask->GetWidth(), mask->GetHeight(), PixelFormat::Format8bppIndexed);
	System::Drawing::Rectangle rect(0, 0, bmp->Width, bmp->Height);
	BitmapData ^bmpData = bmp->LockBits(rect, ImageLockMode::WriteOnly, bmp->PixelFormat);
	unsigned char *address = (unsigned char*)bmpData->Scan0.ToPointer();
	for (int y = 0; y < mask->GetHeight(); y++) 
	{
		memcpy(address, &mask->GetScanLine(y)[0], bmp->Width);
		address += bmpData->Stride;
	}
	bmp->UnlockBits(bmpData);

  SetBitmapPaletteFromGlobalPalette(bmp);

	return bmp;
}

System::Drawing::Bitmap^ getSpriteAsBitmap(int spriteNum) {
  Common::Bitmap *todraw = get_sprite(spriteNum);
  return ConvertBlockToBitmap(todraw, (thisgame.SpriteFlags[spriteNum] & SPF_ALPHACHANNEL) != 0);
}

System::Drawing::Bitmap^ getSpriteAsBitmap32bit(int spriteNum, int width, int height) {
  Common::Bitmap *todraw = get_sprite(spriteNum);
  if (todraw == NULL)
  {
	  throw gcnew AGSEditorException(String::Format("getSpriteAsBitmap32bit: Unable to find sprite {0}", spriteNum));
  }
  return ConvertBlockToBitmap32(todraw, width, height, (thisgame.SpriteFlags[spriteNum] & SPF_ALPHACHANNEL) != 0);
}

System::Drawing::Bitmap^ getBackgroundAsBitmap(Room ^room, int backgroundNumber) {

  Common::RoomInfo *roomptr = (Common::RoomInfo*)(void*)room->_roomStructPtr;
  return ConvertBlockToBitmap32(roomptr->Backgrounds[backgroundNumber].Graphic, room->Width, room->Height, false);
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

void ConvertGUIToBinaryFormat(GUI ^guiObj, Common::GuiMain *gui) 
{
  NormalGUI^ normalGui = dynamic_cast<NormalGUI^>(guiObj);
  if (normalGui)
  {
	ConvertStringToNativeString(normalGui->OnClick, gui->OnClickHandler);
	gui->SetX(normalGui->Left);
	gui->SetY(normalGui->Top);
	gui->SetWidth(normalGui->Width);
	gui->SetHeight(normalGui->Height);
	gui->Flags = (normalGui->Clickable) ? 0 : Common::kGuiMain_NoClick;
    gui->PopupAtMouseY = normalGui->PopupYPos;
    gui->PopupStyle = (Common::GuiPopupStyle)normalGui->Visibility;
    gui->ZOrder = normalGui->ZOrder;
    gui->ForegroundColor = normalGui->BorderColor;
    gui->SetTransparencyAsPercentage(normalGui->Transparency);
  }
  else
  {
    TextWindowGUI^ twGui = dynamic_cast<TextWindowGUI^>(guiObj);
	gui->SetWidth(200);
	gui->SetHeight(100);
    gui->Flags = Common::kGuiMain_TextWindow;
	gui->PopupStyle = Common::kGuiPopupScript;
    gui->ForegroundColor = twGui->TextColor;
  }
  gui->BackgroundColor = guiObj->BackgroundColor;
  gui->BackgroundImage = guiObj->BackgroundImage;
  
  ConvertStringToNativeString(guiObj->Name, gui->Name);

  gui->ControlCount = 0;

  int btn_index = numguibuts;
  int label_index = numguilabels;
  int textbox_index = numguitext;
  int listbox_index = numguilist;
  int slider_index = numguislider;
  int inv_index = numguiinv;

  for each (GUIControl^ control in guiObj->Controls)
  {
      if (dynamic_cast<AGS::Types::GUIButton^>(control))
      {
          numguibuts++;
      }
      else if (dynamic_cast<AGS::Types::GUILabel^>(control))
      {
          numguilabels++;
      }
      else if (dynamic_cast<AGS::Types::GUITextBox^>(control))
      {
          numguitext++;
      }
      else if (dynamic_cast<AGS::Types::GUIListBox^>(control))
      {
          numguilist++;
      }
      else if (dynamic_cast<AGS::Types::GUISlider^>(control))
      {
          numguislider++;
      }
      else if (dynamic_cast<AGS::Types::GUIInventory^>(control))
      {
          numguiinv++;
      }
      else if (dynamic_cast<AGS::Types::GUITextWindowEdge^>(control))
      {
          numguibuts++;
      }
  }

  guibuts.SetLength(numguibuts);
  guilabels.SetLength(numguilabels);
  guitext.SetLength(numguitext);
  guilist.SetLength(numguilist);
  guislider.SetLength(numguislider);
  guiinv.SetLength(numguiinv);

  gui->ControlRefs.SetLength(guiObj->Controls->Count);
  gui->Controls.SetLength(guiObj->Controls->Count);

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
          Common::GuiButton &btn = guibuts[btn_index];
		  btn.TextColor = button->TextColor;
		  btn.TextFont = button->Font;
		  btn.NormalImage = button->Image;
		  btn.CurrentImage = btn.NormalImage;
		  btn.MouseOverImage = button->MouseoverImage;
		  btn.PushedImage = button->PushedImage;
          btn.TextAlignment = (Alignment)button->TextAlignment;
          btn.ClickAction = (Common::GuiButtonClickAction)button->ClickAction;
		  btn.ClickActionData = button->NewModeNumber;
		  btn.Flags = (button->ClipImage) ? Common::kGuiCtrl_Clip : 0;
          Common::String btn_text;
		  ConvertStringToNativeString(button->Text, btn_text);
          btn.SetText(btn_text);
		  ConvertStringToNativeString(button->OnClick, btn.EventHandlers[0]);
		  
		  gui->ControlRefs[gui->ControlCount] = (Common::kGuiButton << 16) | btn_index;
		  gui->Controls[gui->ControlCount] = &btn;
		  gui->ControlCount++;
		  btn_index++;
	  }
	  else if (label)
	  {
          Common::GuiLabel &lbl = guilabels[label_index];
		  lbl.TextColor = label->TextColor;
		  lbl.TextFont = label->Font;
		  lbl.TextAlignment = (Alignment)label->TextAlignment;
		  lbl.Flags = 0;
		  ConvertStringToNativeString(label->Text, lbl.Text);

		  gui->ControlRefs[gui->ControlCount] = (Common::kGuiLabel << 16) | label_index;
		  gui->Controls[gui->ControlCount] = &lbl;
		  gui->ControlCount++;
		  label_index++;
	  }
	  else if (textbox)
	  {
          Common::GuiTextBox &tbox = guitext[textbox_index];
		  tbox.TextColor = textbox->TextColor;
		  tbox.TextFont = textbox->Font;
		  tbox.Flags = 0;
		  tbox.TextBoxFlags = (textbox->ShowBorder) ? 0 : Common::kGuiTextBox_NoBorder;
		  tbox.Text.Empty();
		  ConvertStringToNativeString(textbox->OnActivate, tbox.EventHandlers[0]);

		  gui->ControlRefs[gui->ControlCount] = (Common::kGuiTextBox << 16) | textbox_index;
		  gui->Controls[gui->ControlCount] = &tbox;
		  gui->ControlCount++;
		  textbox_index++;
	  }
	  else if (listbox)
	  {
          Common::GuiListBox &lbox = guilist[listbox_index];
		  lbox.TextColor = listbox->TextColor;
		  lbox.TextFont = listbox->Font;
		  lbox.BackgroundColor = listbox->SelectedTextColor;
		  lbox.SelectedBkgColor = listbox->SelectedBackgroundColor;
		  lbox.TextAlignment = (Alignment)listbox->TextAlignment;
          lbox.Flags = listbox->Translated ? Common::kGuiCtrl_Translated : 0;
		  lbox.ListBoxFlags = (listbox->ShowBorder) ? 0 : Common::kGuiListBox_NoBorder;
		  lbox.ListBoxFlags |= (listbox->ShowScrollArrows) ? 0 : Common::kGuiListBox_NoArrows;
		  ConvertStringToNativeString(listbox->OnSelectionChanged, lbox.EventHandlers[0]);

		  gui->ControlRefs[gui->ControlCount] = (Common::kGuiListBox << 16) | listbox_index;
		  gui->Controls[gui->ControlCount] = &lbox;
		  gui->ControlCount++;
		  listbox_index++;
	  }
	  else if (slider)
	  {
          Common::GuiSlider &sld = guislider[slider_index];
		  sld.MinValue = slider->MinValue;
		  sld.MaxValue = slider->MaxValue;
		  sld.Value = slider->Value;
		  sld.HandleImage = slider->HandleImage;
		  sld.HandleOffset = slider->HandleOffset;
		  sld.BackgroundImage = slider->BackgroundImage;
		  ConvertStringToNativeString(slider->OnChange, sld.EventHandlers[0]);

		  gui->ControlRefs[gui->ControlCount] = (Common::kGuiSlider << 16) | slider_index;
		  gui->Controls[gui->ControlCount] = &sld;
		  gui->ControlCount++;
		  slider_index++;
	  }
	  else if (invwindow)
	  {
          Common::GuiInvWindow &inv = guiinv[inv_index];
		  inv.CharacterId = invwindow->CharacterID;
		  inv.ItemWidth = invwindow->ItemWidth;
		  inv.ItemHeight = invwindow->ItemHeight;

		  gui->ControlRefs[gui->ControlCount] = (Common::kGuiInvWindow << 16) | inv_index;
		  gui->Controls[gui->ControlCount] = &inv;
		  gui->ControlCount++;
		  inv_index++;
	  }
	  else if (textwindowedge)
	  {
          Common::GuiButton &btn = guibuts[btn_index];
		  btn.NormalImage = textwindowedge->Image;
		  btn.CurrentImage = btn.NormalImage;
		  btn.Flags = 0;
		  btn.SetText("");
		  
		  gui->ControlRefs[gui->ControlCount] = (Common::kGuiButton << 16) | btn_index;
		  gui->Controls[gui->ControlCount] = &btn;
		  gui->ControlCount++;
		  btn_index++;
	  }

      Common::GuiObject *newObj = gui->Controls[gui->ControlCount - 1];
	  newObj->SetFrame(RectWH(control->Left, control->Top, control->Width, control->Height));
	  newObj->Id = control->ID;
	  newObj->ZOrder = control->ZOrder;
	  ConvertStringToNativeString(control->Name, newObj->ScriptName);
  }

  gui->RebuildArray();
}

void drawGUI(int hdc, int x,int y, GUI^ guiObj, int scaleFactor, int selectedControl) {
  numguibuts = 0;
  numguilabels = 0;
  numguitext = 0;
  numguilist = 0;
  numguislider = 0;
  numguiinv = 0;

  ConvertGUIToBinaryFormat(guiObj, &tempgui);

  tempgui.HighlightControl = selectedControl;

  drawGUIAt(hdc, x, y, -1, -1, -1, -1, scaleFactor);
}

Dictionary<int, Sprite^>^ load_sprite_dimensions()
{
	Dictionary<int, Sprite^>^ sprites = gcnew Dictionary<int, Sprite^>();

	for (int i = 0; i < spriteset.elements; i++)
	{
		Common::Bitmap *spr = spriteset[i];
		if (spr != NULL)
		{
			sprites->Add(i, gcnew Sprite(i, spr->GetWidth(), spr->GetHeight(), spr->GetColorDepth(), (thisgame.SpriteFlags[i] & SPF_640x400) ? SpriteImportResolution::HighRes : SpriteImportResolution::LowRes, (thisgame.SpriteFlags[i] & SPF_ALPHACHANNEL) ? true : false));
		}
	}

	return sprites;
}

void ConvertCustomProperties(AGS::Types::CustomProperties ^insertInto, AGS::Common::CustomProperties *propToConvert)
{
	for (int j = 0; j < propToConvert->GetPropertyCount(); j++) 
	{
        AGS::Common::CustomPropertyState *prop_info = propToConvert->GetProperty(j);
		CustomProperty ^newProp = gcnew CustomProperty();
		newProp->Name = gcnew String(prop_info->Name);
		newProp->Value = gcnew String(prop_info->Value);
		insertInto->PropertyValues->Add(newProp->Name, newProp);
	}
}

void CompileCustomProperties(AGS::Types::CustomProperties ^convertFrom, AGS::Common::CustomProperties *compileInto)
{
	compileInto->Free();
    AGS::Common::String prop_name;
    AGS::Common::String prop_value;
	for each (String ^key in convertFrom->PropertyValues->Keys)
	{
		ConvertStringToNativeString(convertFrom->PropertyValues[key]->Name, prop_name);
		ConvertStringToNativeString(convertFrom->PropertyValues[key]->Value, prop_value);
		compileInto->AddProperty(prop_name, prop_value);
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
		if (cmdList->command[cmd].children != NULL) 
		{
			ConvertInteractionCommandList(sb, cmdList->command[cmd].children, scriptFuncPrefix, game, runScriptCount, targetTypeForUnhandledEvent);
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
	game->Settings->AlwaysDisplayTextAsSpeech = (thisgame.Options[OPT_ALWAYSSPCH] != 0);
	game->Settings->AntiAliasFonts = (thisgame.Options[OPT_ANTIALIASFONTS] != 0);
	game->Settings->AntiGlideMode = (thisgame.Options[OPT_ANTIGLIDE] != 0);
	game->Settings->AutoMoveInWalkMode = !thisgame.Options[OPT_NOWALKMODE];
	game->Settings->BackwardsText = (thisgame.Options[OPT_RIGHTLEFTWRITE] != 0);
	game->Settings->ColorDepth = (GameColorDepth)thisgame.ColorDepth;
	game->Settings->CompressSprites = (thisgame.Options[OPT_COMPRESSSPRITES] != 0);
	game->Settings->CrossfadeMusic = (CrossfadeSpeed)thisgame.Options[OPT_CROSSFADEMUSIC];
	game->Settings->DebugMode = (thisgame.Options[OPT_DEBUGMODE] != 0);
	game->Settings->DialogOptionsBackwards = (thisgame.Options[OPT_DIALOGUPWARDS] != 0);
	game->Settings->DialogOptionsGap = thisgame.Options[OPT_DIALOGGAP];
	game->Settings->DialogOptionsGUI = thisgame.Options[OPT_DIALOGIFACE];
	game->Settings->DialogOptionsBullet = thisgame.DialogBulletSprIndex;
	game->Settings->DisplayMultipleInventory = (thisgame.Options[OPT_DUPLICATEINV] != 0);
	game->Settings->EnforceNewStrings = (thisgame.Options[OPT_STRICTSTRINGS] != 0);
  game->Settings->EnforceNewAudio = false;
	game->Settings->EnforceObjectBasedScript = (thisgame.Options[OPT_STRICTSCRIPTING] != 0);
	game->Settings->FontsForHiRes = (thisgame.Options[OPT_NOSCALEFNT] != 0);
	game->Settings->GameName = gcnew String(thisgame.GameName);
	game->Settings->GUIAlphaStyle = GUIAlphaStyle::Classic;
	game->Settings->HandleInvClicksInScript = (thisgame.Options[OPT_HANDLEINVCLICKS] != 0);
	game->Settings->InventoryCursors = !thisgame.Options[OPT_FIXEDINVCURSOR];
	game->Settings->LeftToRightPrecedence = (thisgame.Options[OPT_LEFTTORIGHTEVAL] != 0);
	game->Settings->LetterboxMode = (thisgame.Options[OPT_LETTERBOX] != 0);
	game->Settings->MaximumScore = thisgame.TotalScore;
	game->Settings->MouseWheelEnabled = (thisgame.Options[OPT_MOUSEWHEEL] != 0);
	game->Settings->NumberDialogOptions = (thisgame.Options[OPT_DIALOGNUMBERED] != 0);
	game->Settings->PixelPerfect = (thisgame.Options[OPT_PIXPERFECT] != 0);
	game->Settings->PlaySoundOnScore = thisgame.Options[OPT_SCORESOUND];
	game->Settings->Resolution = (GameResolutions)thisgame.DefaultResolution;
	game->Settings->RoomTransition = (RoomTransitionStyle)thisgame.Options[OPT_FADETYPE];
	game->Settings->SaveScreenshots = (thisgame.Options[OPT_SAVESCREENSHOT] != 0);
	game->Settings->SkipSpeech = (SkipSpeechStyle)thisgame.Options[OPT_NOSKIPTEXT];
	game->Settings->SpeechPortraitSide = (SpeechPortraitSide)thisgame.Options[OPT_PORTRAITSIDE];
	game->Settings->SpeechStyle = (SpeechStyle)thisgame.Options[OPT_SPEECHTYPE];
	game->Settings->SplitResources = thisgame.Options[OPT_SPLITRESOURCES];
	game->Settings->TextWindowGUI = thisgame.Options[OPT_TWCUSTOM];
	game->Settings->ThoughtGUI = thisgame.Options[OPT_THOUGHTGUI];
	game->Settings->TurnBeforeFacing = (thisgame.Options[OPT_TURNTOFACELOC] != 0);
	game->Settings->TurnBeforeWalking = (thisgame.Options[OPT_ROTATECHARS] != 0);
	game->Settings->WalkInLookMode = (thisgame.Options[OPT_WALKONLOOK] != 0);
	game->Settings->WhenInterfaceDisabled = (InterfaceDisabledAction)thisgame.Options[OPT_DISABLEOFF];
	game->Settings->UniqueID = thisgame.UniqueId;
  game->Settings->SaveGameFolderName = gcnew String(thisgame.GameName);

	game->Settings->InventoryHotspotMarker->DotColor = thisgame.InvItemHotDotColor;
	game->Settings->InventoryHotspotMarker->CrosshairColor = thisgame.InvItemHotDotOuterColor;
	game->Settings->InventoryHotspotMarker->Image = thisgame.InvItemHotDotSprIndex;
	if (thisgame.InvItemHotDotSprIndex) 
	{
		game->Settings->InventoryHotspotMarker->Style = InventoryHotspotMarkerStyle::Sprite;
	}
	else if (thisgame.InvItemHotDotColor) 
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
		if (thisgame.PaletteUses[i] == PAL_BACKGROUND) 
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
	
    AGS::Types::IViewFolder ^viewFolder = AGS::Types::FolderHelper::CreateDefaultViewFolder();
	for (i = 0; i < thisgame.ViewCount; i++) 
	{
		AGS::Types::View ^view = gcnew AGS::Types::View();
		view->Name = gcnew String(thisgame.ViewNames[i]);
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
    AGS::Types::FolderHelper::SetRootViewFolder(game, viewFolder);

	for (i = 0; i < thisgame.CharacterCount; i++) 
	{
		char jibbledScriptName[50] = "\0";
		if (strlen(thisgame.Characters[i].scrname) > 0) 
		{
			sprintf(jibbledScriptName, "c%s", thisgame.Characters[i].scrname);
			strlwr(jibbledScriptName);
			jibbledScriptName[1] = toupper(jibbledScriptName[1]);
		}
		AGS::Types::Character ^character = gcnew AGS::Types::Character();
		character->AdjustSpeedWithScaling = ((thisgame.Characters[i].flags & CHF_SCALEMOVESPEED) != 0);
		character->AdjustVolumeWithScaling = ((thisgame.Characters[i].flags & CHF_SCALEVOLUME) != 0);
		character->AnimationDelay = thisgame.Characters[i].animspeed;
		character->BlinkingView = (thisgame.Characters[i].blinkview < 1) ? 0 : (thisgame.Characters[i].blinkview + 1);
		character->Clickable = !(thisgame.Characters[i].flags & CHF_NOINTERACT);
		character->DiagonalLoops = !(thisgame.Characters[i].flags & CHF_NODIAGONAL);
		character->ID = i;
		character->IdleView = (thisgame.Characters[i].idleview < 1) ? 0 : (thisgame.Characters[i].idleview + 1);
		character->MovementSpeed = thisgame.Characters[i].walkspeed;
		character->MovementSpeedX = thisgame.Characters[i].walkspeed;
		character->MovementSpeedY = thisgame.Characters[i].walkspeed_y;
		character->NormalView = thisgame.Characters[i].defview + 1;
		character->RealName = gcnew String(thisgame.Characters[i].name);
		character->ScriptName = gcnew String(jibbledScriptName);
		character->Solid = !(thisgame.Characters[i].flags & CHF_NOBLOCKING);
		character->SpeechColor = thisgame.Characters[i].talkcolor;
		character->SpeechView = (thisgame.Characters[i].talkview < 1) ? 0 : (thisgame.Characters[i].talkview + 1);
		character->StartingRoom = thisgame.Characters[i].room;
		character->StartX = thisgame.Characters[i].x;
		character->StartY = thisgame.Characters[i].y;
    character->ThinkingView = (thisgame.Characters[i].thinkview < 1) ? 0 : (thisgame.Characters[i].thinkview + 1);
		character->TurnBeforeWalking = !(thisgame.Characters[i].flags & CHF_NOTURNING);
		character->UniformMovementSpeed = (thisgame.Characters[i].walkspeed_y == UNIFORM_WALK_SPEED);
		character->UseRoomAreaLighting = !(thisgame.Characters[i].flags & CHF_NOLIGHTING);
		character->UseRoomAreaScaling = !(thisgame.Characters[i].flags & CHF_MANUALSCALING);

		game->Characters->Add(character);

		ConvertCustomProperties(character->Properties, &thisgame.CharacterProperties[i]);

		char scriptFuncPrefix[100];
		sprintf(scriptFuncPrefix, "character%d_", i);
		ConvertInteractions(character->Interactions, thisgame.CharacterInteractions[i], gcnew String(scriptFuncPrefix), game, 3);
	}
	game->PlayerCharacter = game->Characters[thisgame.PlayerCharacterIndex];

	game->TextParser->Words->Clear();
	for (i = 0; i < thisgame.Dictionary->num_words; i++) 
	{
		AGS::Types::TextParserWord ^newWord = gcnew AGS::Types::TextParserWord();
		newWord->WordGroup = thisgame.Dictionary->wordnum[i];
		newWord->Word = gcnew String(thisgame.Dictionary->word[i]);
		newWord->SetWordTypeFromGroup();

		game->TextParser->Words->Add(newWord);
	}

	for (i = 0; i < MAXGLOBALMES; i++) 
	{
		if (!thisgame.GlobalMessages[i].IsEmpty()) 
		{
			game->GlobalMessages[i] = gcnew String(thisgame.GlobalMessages[i]);
		}
		else
		{
			game->GlobalMessages[i] = String::Empty;
		}
	}

	game->LipSync->Type = (thisgame.Options[OPT_LIPSYNCTEXT] != 0) ? LipSyncType::Text : LipSyncType::None;
	game->LipSync->DefaultFrame = thisgame.DefaultLipSyncFrame;
	for (i = 0; i < MAXLIPSYNCFRAMES; i++) 
	{
		game->LipSync->CharactersPerFrame[i] = gcnew String(thisgame.LipSyncFrameLetters[i]);
	}

	for (i = 0; i < thisgame.DialogCount; i++) 
	{
		AGS::Types::Dialog ^newDialog = gcnew AGS::Types::Dialog();
		newDialog->ID = i;
		for (int j = 0; j < dialog[i].OptionCount; j++) 
		{
			AGS::Types::DialogOption ^newOption = gcnew AGS::Types::DialogOption();
			newOption->ID = j + 1;
			newOption->Text = gcnew String(dialog[i].Options[j].Name);
			newOption->Say = !(dialog[i].Options[j].Flags & Common::kDialogOption_NoRepeat);
			newOption->Show = (dialog[i].Options[j].Flags & Common::kDialogOption_IsOn);

			newDialog->Options->Add(newOption);
		}

		newDialog->Name = gcnew String(thisgame.DialogScriptNames[i]);
		newDialog->Script = gcnew String(dlgscript[i]);
		newDialog->ShowTextParser = (dialog[i].Flags & Common::kDialogTopic_ShowParser);

		game->Dialogs->Add(newDialog);
	}

	for (i = 0; i < thisgame.MouseCursorCount; i++)
	{
		AGS::Types::MouseCursor ^cursor = gcnew AGS::Types::MouseCursor();
		cursor->Animate = (thisgame.MouseCursors[i].view >= 0);
		cursor->AnimateOnlyOnHotspots = ((thisgame.MouseCursors[i].flags & MCF_HOTSPOT) != 0);
		cursor->AnimateOnlyWhenMoving = ((thisgame.MouseCursors[i].flags & MCF_ANIMMOVE) != 0);
		cursor->Image = thisgame.MouseCursors[i].pic;
		cursor->HotspotX = thisgame.MouseCursors[i].hotx;
		cursor->HotspotY = thisgame.MouseCursors[i].hoty;
		cursor->ID = i;
		cursor->Name = gcnew String(thisgame.MouseCursors[i].name);
		cursor->StandardMode = ((thisgame.MouseCursors[i].flags & MCF_STANDARD) != 0);
		cursor->View = thisgame.MouseCursors[i].view + 1;
		if (cursor->View < 1) cursor->View = 0;

		game->Cursors->Add(cursor);
	}

	for (i = 0; i < thisgame.FontCount; i++) 
	{
		AGS::Types::Font ^font = gcnew AGS::Types::Font();
		font->ID = i;
		font->OutlineFont = (thisgame.FontOutline[i] >= 0) ? thisgame.FontOutline[i] : 0;
		if (thisgame.FontOutline[i] == -1) 
		{
			font->OutlineStyle = FontOutlineStyle::None;
		}
		else if (thisgame.FontOutline[i] == FONT_OUTLINE_AUTO)
		{
			font->OutlineStyle = FontOutlineStyle::Automatic;
		}
		else 
		{
			font->OutlineStyle = FontOutlineStyle::UseOutlineFont;
		}
		font->PointSize = thisgame.FontFlags[i] & FFLG_SIZEMASK;
		font->Name = gcnew String(String::Format("Font {0}", i));

		game->Fonts->Add(font);
	}

	for (i = 1; i < thisgame.InvItemCount; i++)
	{
		InventoryItem^ invItem = gcnew InventoryItem();
    invItem->CursorImage = thisgame.InventoryItems[i].pic;
		invItem->Description = gcnew String(thisgame.InventoryItems[i].name);
		invItem->Image = thisgame.InventoryItems[i].pic;
		invItem->HotspotX = thisgame.InventoryItems[i].hotx;
		invItem->HotspotY = thisgame.InventoryItems[i].hoty;
		invItem->ID = i;
		invItem->Name = gcnew String(thisgame.InventoryScriptNames[i]);
		invItem->PlayerStartsWithItem = (thisgame.InventoryItems[i].flags & IFLG_STARTWITH);

		ConvertCustomProperties(invItem->Properties, &thisgame.InvItemProperties[i]);

		char scriptFuncPrefix[100];
		sprintf(scriptFuncPrefix, "inventory%d_", i);
		ConvertInteractions(invItem->Interactions, thisgame.InvItemInteractions[i], gcnew String(scriptFuncPrefix), game, 5);

		game->InventoryItems->Add(invItem);
	}

	//AGS::Types::PropertySchema ^schema = gcnew AGS::Types::PropertySchema();
    for (i = 0; i < thisgame.PropertySchema.GetPropertyCount(); i++) 
	{
		CustomPropertySchemaItem ^schemaItem = gcnew CustomPropertySchemaItem();
        const AGS::Common::CustomPropertyInfo *prop_info = thisgame.PropertySchema.GetProperty(i);
        schemaItem->Name = gcnew String(prop_info->Name);
        schemaItem->Description = gcnew String(prop_info->Description);
		schemaItem->DefaultValue = gcnew String(prop_info->DefaultValue);
		schemaItem->Type = (AGS::Types::CustomPropertyType)prop_info->Type;

		game->PropertySchema->PropertyDefinitions->Add(schemaItem);
	}

	for (i = 0; i < thisgame.GuiCount; i++)
	{
		guis[i].RebuildArray();

		GUI^ newGui;
		if (guis[i].IsTextWindow()) 
		{
			newGui = gcnew TextWindowGUI();
			newGui->Controls->Clear();  // we'll add our own edges
            ((TextWindowGUI^)newGui)->TextColor = guis[i].ForegroundColor;
		}
		else 
		{
			newGui = gcnew NormalGUI();
			((NormalGUI^)newGui)->Clickable = ((guis[i].Flags & Common::kGuiMain_NoClick) == 0);
			((NormalGUI^)newGui)->Top = guis[i].GetY();
			((NormalGUI^)newGui)->Left = guis[i].GetX();
			((NormalGUI^)newGui)->Width = (guis[i].GetWidth() > 0) ? guis[i].GetWidth() : 1;
			((NormalGUI^)newGui)->Height = (guis[i].GetHeight() > 0) ? guis[i].GetHeight() : 1;
			((NormalGUI^)newGui)->PopupYPos = guis[i].PopupAtMouseY;
			((NormalGUI^)newGui)->Visibility = (GUIVisibility)guis[i].PopupStyle;
			((NormalGUI^)newGui)->ZOrder = guis[i].ZOrder;
			((NormalGUI^)newGui)->OnClick = gcnew String(guis[i].OnClickHandler);
      ((NormalGUI^)newGui)->BorderColor = guis[i].ForegroundColor;
		}
		newGui->BackgroundColor = guis[i].BackgroundColor;
		newGui->BackgroundImage = guis[i].BackgroundImage;
		newGui->ID = i;
        newGui->Name = gcnew String(Common::GuiMain::MakeScriptName(guis[i].Name));

		for (int j = 0; j < guis[i].ControlCount; j++)
		{
            Common::GuiObject* curObj = guis[i].Controls[j];
			GUIControl ^newControl = nullptr;
			switch (guis[i].ControlRefs[j] >> 16)
			{
			case Common::kGuiButton:
				{
				if (guis[i].IsTextWindow())
				{
					AGS::Types::GUITextWindowEdge^ edge = gcnew AGS::Types::GUITextWindowEdge();
					Common::GuiButton *copyFrom = (Common::GuiButton*)curObj;
					newControl = edge;
					edge->Image = copyFrom->NormalImage;
				}
				else
				{
					AGS::Types::GUIButton^ newButton = gcnew AGS::Types::GUIButton();
					Common::GuiButton *copyFrom = (Common::GuiButton*)curObj;
					newControl = newButton;
					newButton->TextColor = copyFrom->TextColor;
					newButton->Font = copyFrom->TextFont;
					newButton->Image = copyFrom->NormalImage;
					newButton->MouseoverImage = copyFrom->MouseOverImage;
					newButton->PushedImage = copyFrom->PushedImage;
					newButton->TextAlignment = (FrameAlignment)copyFrom->TextAlignment;
					newButton->ClickAction = (GUIClickAction)copyFrom->ClickAction;
					newButton->NewModeNumber = copyFrom->ClickActionData;
					newButton->ClipImage = (copyFrom->Flags & Common::kGuiCtrl_Clip) ? true : false;
					newButton->Text = gcnew String(copyFrom->GetText());
					newButton->OnClick = gcnew String(copyFrom->EventHandlers[0]);
				}
				break;
				}
			case Common::kGuiLabel:
				{
				AGS::Types::GUILabel^ newLabel = gcnew AGS::Types::GUILabel();
				Common::GuiLabel *copyFrom = (Common::GuiLabel*)curObj;
				newControl = newLabel;
				newLabel->TextColor = copyFrom->TextColor;
				newLabel->Font = copyFrom->TextFont;
				newLabel->TextAlignment = (FrameAlignment)copyFrom->TextAlignment;
				newLabel->Text = gcnew String(copyFrom->GetText());
				break;
				}
			case Common::kGuiTextBox:
				{
				  AGS::Types::GUITextBox^ newTextbox = gcnew AGS::Types::GUITextBox();
				  Common::GuiTextBox *copyFrom = (Common::GuiTextBox*)curObj;
				  newControl = newTextbox;
				  newTextbox->TextColor = copyFrom->TextColor;
				  newTextbox->Font = copyFrom->TextFont;
				  newTextbox->ShowBorder = (copyFrom->TextBoxFlags & Common::kGuiTextBox_NoBorder) ? false : true;
				  newTextbox->Text = gcnew String(copyFrom->Text);
				  newTextbox->OnActivate = gcnew String(copyFrom->EventHandlers[0]);
				  break;
				}
			case Common::kGuiListBox:
				{
				  AGS::Types::GUIListBox^ newListbox = gcnew AGS::Types::GUIListBox();
				  Common::GuiListBox *copyFrom = (Common::GuiListBox*)curObj;
				  newControl = newListbox;
				  newListbox->TextColor = copyFrom->TextColor;
				  newListbox->Font = copyFrom->TextFont; 
				  newListbox->SelectedTextColor = copyFrom->BackgroundColor;
				  newListbox->SelectedBackgroundColor = copyFrom->SelectedBkgColor;
				  newListbox->TextAlignment = (FrameAlignment)copyFrom->TextAlignment;
				  newListbox->ShowBorder = ((copyFrom->ListBoxFlags & Common::kGuiListBox_NoBorder) == 0);
				  newListbox->ShowScrollArrows = ((copyFrom->ListBoxFlags & Common::kGuiListBox_NoArrows) == 0);
                  newListbox->Translated = (copyFrom->Flags & Common::kGuiCtrl_Translated) != 0;
				  newListbox->OnSelectionChanged = gcnew String(copyFrom->EventHandlers[0]);
				  break;
				}
			case Common::kGuiSlider:
				{
				  AGS::Types::GUISlider^ newSlider = gcnew AGS::Types::GUISlider();
				  Common::GuiSlider *copyFrom = (Common::GuiSlider*)curObj;
				  newControl = newSlider;
				  newSlider->MinValue = copyFrom->MinValue;
				  newSlider->MaxValue = copyFrom->MaxValue;
				  newSlider->Value = copyFrom->Value;
				  newSlider->HandleImage = copyFrom->HandleImage;
			  	  newSlider->HandleOffset = copyFrom->HandleOffset;
				  newSlider->BackgroundImage = copyFrom->BackgroundImage;
				  newSlider->OnChange = gcnew String(copyFrom->EventHandlers[0]);
				  break;
				}
			case Common::kGuiInvWindow:
				{
					AGS::Types::GUIInventory^ invwindow = gcnew AGS::Types::GUIInventory();
				    Common::GuiInvWindow *copyFrom = (Common::GuiInvWindow*)curObj;
				    newControl = invwindow;
					invwindow->CharacterID = copyFrom->CharacterId;
					invwindow->ItemWidth = copyFrom->ItemWidth;
					invwindow->ItemHeight = copyFrom->ItemHeight;
					break;
				}
			default:
				throw gcnew AGSEditorException("Unknown control type found: " + (guis[i].ControlRefs[j] >> 16));
			}
			newControl->Width = (curObj->GetWidth() > 0) ? curObj->GetWidth() : 1;
			newControl->Height = (curObj->GetHeight() > 0) ? curObj->GetHeight() : 1;
			newControl->Left = curObj->GetX();
			newControl->Top = curObj->GetY();
			newControl->ZOrder = curObj->ZOrder;
			newControl->ID = j;
			newControl->Name = gcnew String(curObj->ScriptName);
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

	Stream *opty = Common::AssetManager::OpenAsset(roomFileNameBuffer);
	if (opty == NULL) throw gcnew AGSEditorException("Unable to open room file");

	short version = opty->ReadInt16();
	if (version < 17)
	{
    delete opty;
		throw gcnew AGSEditorException("Room file is from an old version of AGS and cannot be processed");
	}

	String ^scriptToReturn = nullptr;

	int thisblock = 0;
	while (thisblock != Common::kRoomBlock_End) 
	{
		thisblock = opty->ReadByte();
		if (thisblock == Common::kRoomBlock_End) 
		{
			break;
		}

		int blockLen = opty->ReadInt32();

		if (thisblock == Common::kRoomBlock_Script) 
		{
			int lee = opty->ReadInt32();
			int hh;

			char *scriptFile = (char *)malloc(lee + 5);
			opty->Read(scriptFile, lee);
			scriptFile[lee] = 0;

			for (hh = 0; hh < lee; hh++)
			  scriptFile[hh] += passwencstring[hh % 11];

			scriptToReturn = gcnew String(scriptFile, 0, strlen(scriptFile), System::Text::Encoding::Default);
			free(scriptFile);
			break;
		}
		else 
		{
			opty->Seek(Common::kSeekCurrent, blockLen);
		}
	}

	delete opty;

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

	const char *errorMsg = load_room_file(roomFileNameBuffer, roomToLoad->Number);
	if (errorMsg != NULL) 
	{
		throw gcnew AGSEditorException(gcnew String(errorMsg));
	}

  loaded_room_number = roomToLoad->Number;

	Room ^room = gcnew Room(roomToLoad->Number);
	room->Description = roomToLoad->Description;
    room->Script = roomToLoad->Script;
    room->StateSaving = thisroom.IsPersistent;
	room->BottomEdgeY = thisroom.Edges.Bottom;
	room->LeftEdgeX = thisroom.Edges.Left;
	room->MusicVolumeAdjustment = (RoomVolumeAdjustment)thisroom.Options[kRoomBaseOpt_MusicVolume];
	room->PlayerCharacterView = thisroom.Options[kRoomBaseOpt_PlayerCharacterView];
	room->PlayMusicOnRoomLoad = thisroom.Options[kRoomBaseOpt_StartUpMusic];
	room->RightEdgeX = thisroom.Edges.Right;
	room->SaveLoadEnabled = (thisroom.Options[kRoomBaseOpt_SaveLoadDisabled] == 0);
	room->ShowPlayerCharacter = (thisroom.Options[kRoomBaseOpt_PlayerCharacterDisabled] == 0);
	room->TopEdgeY = thisroom.Edges.Top;
	room->Width = thisroom.Width;
	room->Height = thisroom.Height;
  if (thisroom.Resolution > 2)
  {
    room->Resolution = RoomResolution::HighRes;
  }
  else
  {
  	room->Resolution = (RoomResolution)thisroom.Resolution;
  }
	room->ColorDepth = thisroom.Backgrounds[0].Graphic->GetColorDepth();
	room->BackgroundAnimationDelay = thisroom.BkgSceneAnimSpeed;
	room->BackgroundCount = thisroom.BkgSceneCount;

	int i;
	for (i = 0; i < thisroom.LocalVariableCount; i++)
	{
		OldInteractionVariable ^intVar;
		intVar = gcnew OldInteractionVariable(gcnew String(thisroom.LocalVariables[i].name), thisroom.LocalVariables[i].value);
		room->OldInteractionVariables->Add(intVar);
	}

	for (i = 0; i < thisroom.MessageCount; i++) 
	{
		RoomMessage ^newMessage = gcnew RoomMessage(i);
		newMessage->Text = gcnew String(thisroom.Messages[i]);
		newMessage->ShowAsSpeech = (thisroom.MessageInfos[i].displayas > 0);
		newMessage->CharacterID = (thisroom.MessageInfos[i].displayas - 1);
		newMessage->DisplayNextMessageAfter = ((thisroom.MessageInfos[i].flags & MSG_DISPLAYNEXT) != 0);
		newMessage->AutoRemoveAfterTime = ((thisroom.MessageInfos[i].flags & MSG_TIMELIMIT) != 0);
		room->Messages->Add(newMessage);
	}

	for (i = 0; i < thisroom.ObjectCount; i++) 
	{
		char jibbledScriptName[50] = "\0";
		if (strlen(thisroom.Objects[i].ScriptName) > 0) 
		{
			if (thisroom.LoadedVersion < kRoomVersion_300a)
			{
				sprintf(jibbledScriptName, "o%s", thisroom.Objects[i].ScriptName);
				strlwr(jibbledScriptName);
				jibbledScriptName[1] = toupper(jibbledScriptName[1]);
			}
			else 
			{			
				strcpy(jibbledScriptName, thisroom.Objects[i].ScriptName);
			}
		}

		RoomObject ^obj = gcnew RoomObject(room);
		obj->ID = i;
		obj->Image = thisroom.Objects[i].SpriteIndex;
		obj->StartX = thisroom.Objects[i].X;
		obj->StartY = thisroom.Objects[i].Y;
    if (thisroom.LoadedVersion <= kRoomVersion_300a)
      obj->StartY += GetSpriteHeight(thisroom.Objects[i].SpriteIndex);
		obj->Visible = (thisroom.Objects[i].IsOn != 0);
		obj->Baseline = thisroom.Objects[i].Baseline;
		obj->Name = gcnew String(jibbledScriptName);
		obj->Description = gcnew String(thisroom.Objects[i].Name);
		obj->UseRoomAreaScaling = ((thisroom.Objects[i].Flags & OBJF_USEROOMSCALING) != 0);
		obj->UseRoomAreaLighting = ((thisroom.Objects[i].Flags & OBJF_USEREGIONTINTS) != 0);
		ConvertCustomProperties(obj->Properties, &thisroom.Objects[i].Properties);

		if (thisroom.LoadedVersion < kRoomVersion_300a)
		{
			char scriptFuncPrefix[100];
			sprintf(scriptFuncPrefix, "object%d_", i);
			ConvertInteractions(obj->Interactions, thisroom.Objects[i].EventHandlers.Interaction, gcnew String(scriptFuncPrefix), nullptr, 2);
		}
		else 
		{
			CopyInteractions(obj->Interactions, thisroom.Objects[i].EventHandlers.ScriptFnRef);
		}

		room->Objects->Add(obj);
	}

	for (i = 0; i < thisroom.HotspotCount; i++) 
	{
		RoomHotspot ^hotspot = room->Hotspots[i];
		hotspot->ID = i;
		hotspot->Description = gcnew String(thisroom.Hotspots[i].Name);
		hotspot->Name = (gcnew String(thisroom.Hotspots[i].ScriptName))->Trim();
        hotspot->WalkToPoint = Drawing::Point(thisroom.Hotspots[i].WalkToPoint.x, thisroom.Hotspots[i].WalkToPoint.y);
		ConvertCustomProperties(hotspot->Properties, &thisroom.Hotspots[i].Properties);

		if (thisroom.LoadedVersion < kRoomVersion_300a)
		{
			char scriptFuncPrefix[100];
			sprintf(scriptFuncPrefix, "hotspot%d_", i);
			ConvertInteractions(hotspot->Interactions, thisroom.Hotspots[i].EventHandlers.Interaction, gcnew String(scriptFuncPrefix), nullptr, 1);
		}
		else 
		{
			CopyInteractions(hotspot->Interactions, thisroom.Hotspots[i].EventHandlers.ScriptFnRef);
		}
	}

	for (i = 0; i <= LEGACY_MAX_ROOM_WALKAREAS; i++) 
	{
		RoomWalkableArea ^area = room->WalkableAreas[i];
		area->ID = i;
		area->AreaSpecificView = thisroom.WalkAreas[i].ShadingView;
		area->UseContinuousScaling = !(thisroom.WalkAreas[i].Zoom2 == NOT_VECTOR_SCALED);
		area->ScalingLevel = thisroom.WalkAreas[i].Zoom + 100;
		area->MinScalingLevel = thisroom.WalkAreas[i].Zoom + 100;
		if (area->UseContinuousScaling) 
		{
			area->MaxScalingLevel = thisroom.WalkAreas[i].Zoom2 + 100;
		}
		else
		{
			area->MaxScalingLevel = area->MinScalingLevel;
		}
	}

	for (i = 0; i < LEGACY_MAX_ROOM_WALKBEHINDS; i++) 
	{
		RoomWalkBehind ^area = room->WalkBehinds[i];
		area->ID = i;
		area->Baseline = thisroom.WalkBehinds[i].Baseline;
	}

	for (i = 0; i < LEGACY_MAX_ROOM_REGIONS; i++) 
	{
		RoomRegion ^area = room->Regions[i];
		area->ID = i;
		area->UseColourTint = ((thisroom.Regions[i].Tint & TINT_IS_ENABLED) != 0);
		area->LightLevel = thisroom.Regions[i].Light + 100;
		area->BlueTint = (thisroom.Regions[i].Tint >> 16) & 0x00ff;
		area->GreenTint = (thisroom.Regions[i].Tint >> 8) & 0x00ff;
		area->RedTint = thisroom.Regions[i].Tint & 0x00ff;
		area->TintSaturation = (thisroom.Regions[i].Light > 0) ? thisroom.Regions[i].Light : 50;

		if (thisroom.LoadedVersion < kRoomVersion_300a)
		{
			char scriptFuncPrefix[100];
			sprintf(scriptFuncPrefix, "region%d_", i);
			ConvertInteractions(area->Interactions, thisroom.Regions[i].EventHandlers.Interaction, gcnew String(scriptFuncPrefix), nullptr, 0);
		}
		else 
		{
			CopyInteractions(area->Interactions, thisroom.Regions[i].EventHandlers.ScriptFnRef);
		}
	}
/*
	if (thisroom.TextScript != NULL) 
	{
		room->Script->Text = gcnew String(thisroom.TextScript);
	}
*/
	room->_roomStructPtr = (IntPtr)&thisroom;

	ConvertCustomProperties(room->Properties, &thisroom.Properties);

	if (thisroom.LoadedVersion < kRoomVersion_300a)
	{
		ConvertInteractions(room->Interactions, thisroom.EventHandlers.Interaction, "room_", nullptr, 0);
	}
	else 
	{
		CopyInteractions(room->Interactions, thisroom.EventHandlers.ScriptFnRef);
	}

	room->GameID = thisroom.GameId;
  clear_undo_buffer();

	return room;
}

void save_crm_file(Room ^room)
{
	thisroom.GameId = room->GameID;
	thisroom.Edges.Bottom = room->BottomEdgeY;
	thisroom.Edges.Left = room->LeftEdgeX;
    thisroom.IsPersistent = room->StateSaving;
	thisroom.Options[kRoomBaseOpt_MusicVolume] = (int)room->MusicVolumeAdjustment;
	thisroom.Options[kRoomBaseOpt_PlayerCharacterView] = room->PlayerCharacterView;
	thisroom.Options[kRoomBaseOpt_StartUpMusic] = room->PlayMusicOnRoomLoad;
	thisroom.Edges.Right = room->RightEdgeX;
	thisroom.Options[kRoomBaseOpt_SaveLoadDisabled] = room->SaveLoadEnabled ? 0 : 1;
	thisroom.Options[kRoomBaseOpt_PlayerCharacterDisabled] = room->ShowPlayerCharacter ? 0 : 1;
	thisroom.Edges.Top = room->TopEdgeY;
	thisroom.Width = room->Width;
	thisroom.Height = room->Height;
	thisroom.Resolution = (int)room->Resolution;
	thisroom.BkgSceneAnimSpeed = room->BackgroundAnimationDelay;
	thisroom.BkgSceneCount = room->BackgroundCount;

	int i;
	for (i = 0; i < thisroom.MessageCount; i++) 
	{
		thisroom.Messages[i].Free();
	}
	thisroom.MessageCount = room->Messages->Count;
	for (i = 0; i < thisroom.MessageCount; i++) 
	{
		RoomMessage ^newMessage = room->Messages[i];
		//thisroom.Messages[i] = (char*)malloc(newMessage->Text->Length + 1);
		//ConvertStringToCharArray(newMessage->Text, thisroom.Messages[i]);
        ConvertStringToNativeString(newMessage->Text, thisroom.Messages[i]);
		if (newMessage->ShowAsSpeech)
		{
			thisroom.MessageInfos[i].displayas = newMessage->CharacterID + 1;
		}
		else
		{
			thisroom.MessageInfos[i].displayas = 0;
		}
		thisroom.MessageInfos[i].flags = 0;
		if (newMessage->DisplayNextMessageAfter) thisroom.MessageInfos[i].flags |= MSG_DISPLAYNEXT;
		if (newMessage->AutoRemoveAfterTime) thisroom.MessageInfos[i].flags |= MSG_TIMELIMIT;
	}

	thisroom.ObjectCount = room->Objects->Count;
    thisroom.Objects.SetLength(room->Objects->Count);
	for (i = 0; i < thisroom.ObjectCount; i++) 
	{
		RoomObject ^obj = room->Objects[i];
		ConvertStringToNativeString(obj->Name, thisroom.Objects[i].ScriptName);

		thisroom.Objects[i].SpriteIndex = obj->Image;
		thisroom.Objects[i].X = obj->StartX;
		thisroom.Objects[i].Y = obj->StartY;
		thisroom.Objects[i].IsOn = obj->Visible;
		thisroom.Objects[i] .Baseline = obj->Baseline;
		ConvertStringToNativeString(obj->Description, thisroom.Objects[i].Name, 30);
		thisroom.Objects[i].Flags = 0;
		if (obj->UseRoomAreaScaling) thisroom.Objects[i].Flags |= OBJF_USEROOMSCALING;
		if (obj->UseRoomAreaLighting) thisroom.Objects[i].Flags |= OBJF_USEREGIONTINTS;
		CompileCustomProperties(obj->Properties, &thisroom.Objects[i].Properties);
	}

	thisroom.HotspotCount = room->Hotspots->Count;
    thisroom.Hotspots.SetLength(room->Hotspots->Count);
	for (i = 0; i < thisroom.HotspotCount; i++) 
	{
		RoomHotspot ^hotspot = room->Hotspots[i];
		thisroom.Hotspots[i].Name = (char*)malloc(hotspot->Description->Length + 1);
		ConvertStringToNativeString(hotspot->Description, thisroom.Hotspots[i].Name);
		ConvertStringToNativeString(hotspot->Name, thisroom.Hotspots[i].ScriptName, 20);
		thisroom.Hotspots[i].WalkToPoint.x = hotspot->WalkToPoint.X;
		thisroom.Hotspots[i].WalkToPoint.y = hotspot->WalkToPoint.Y;
		CompileCustomProperties(hotspot->Properties, &thisroom.Hotspots[i].Properties);
	}

    thisroom.WalkAreas.SetLength(LEGACY_MAX_ROOM_WALKAREAS + 1);
	for (i = 0; i <= LEGACY_MAX_ROOM_WALKAREAS; i++) 
	{
		RoomWalkableArea ^area = room->WalkableAreas[i];
		thisroom.WalkAreas[i].ShadingView = area->AreaSpecificView;
		
		if (area->UseContinuousScaling) 
		{
			thisroom.WalkAreas[i].Zoom = area->MinScalingLevel - 100;
			thisroom.WalkAreas[i].Zoom2 = area->MaxScalingLevel - 100;
		}
		else
		{
			thisroom.WalkAreas[i].Zoom = area->ScalingLevel - 100;
			thisroom.WalkAreas[i].Zoom2 = NOT_VECTOR_SCALED;
		}
	}

    thisroom.WalkBehinds.SetLength(LEGACY_MAX_ROOM_WALKBEHINDS);
	for (i = 0; i < LEGACY_MAX_ROOM_WALKBEHINDS; i++) 
	{
		RoomWalkBehind ^area = room->WalkBehinds[i];
		thisroom.WalkBehinds[i].Baseline = area->Baseline;
	}

    thisroom.Regions.SetLength(LEGACY_MAX_ROOM_REGIONS);
	for (i = 0; i < LEGACY_MAX_ROOM_REGIONS; i++) 
	{
		RoomRegion ^area = room->Regions[i];
		thisroom.Regions[i].Tint = 0;
		if (area->UseColourTint) 
		{
			thisroom.Regions[i].Tint = TINT_IS_ENABLED;
			thisroom.Regions[i].Tint |= area->RedTint | (area->GreenTint << 8) | (area->BlueTint << 16);
			thisroom.Regions[i].Light = area->TintSaturation;
		}
		else 
		{
			thisroom.Regions[i].Light = area->LightLevel - 100;
		}
	}

	CompileCustomProperties(room->Properties, &thisroom.Properties);

	thisroom.TextScript = NULL;
	thisroom.CompiledScript = ((AGS::Native::CompiledScript^)room->Script->CompiledData)->Data;
    thisroom.CompiledScriptShared = true;

	char roomFileNameBuffer[MAX_PATH];
	ConvertStringToCharArray(room->FileName, roomFileNameBuffer);

	TempDataStorage::RoomBeingSaved = room;

	save_room_file(roomFileNameBuffer);

	TempDataStorage::RoomBeingSaved = nullptr;

	for (i = 0; i < thisroom.HotspotCount; i++) 
	{
		thisroom.Hotspots[i].Name.Free();
	}
}

// [IKM] 2012-11-13: code moved to AGS.Types.FolderHelper
/*
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
*/

ref class ManagedViewProcessing
{
public:
    static void ConvertViewsToDTAFormat(IViewFolder ^folder, Game ^game) 
    {
        /*
	    for each (ViewFolder ^subFolder in folder->SubFolders)
	    {
		    ConvertViewsToDTAFormat(subFolder, game);
	    }
        */
        AGS::Types::FolderHelper::ViewFolderProcessing ^del = 
            gcnew AGS::Types::FolderHelper::ViewFolderProcessing(ConvertViewsToDTAFormat);
        AGS::Types::FolderHelper::ForEachViewFolder(folder, game, del);

	    for each (View ^view in folder->Views)
	    {
		    int i = view->ID - 1;
		    ConvertStringToNativeString(view->Name, thisgame.ViewNames[i], MAXVIEWNAMELENGTH);

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
};

void write_compiled_script(Stream *ooo, Script ^script)
{
	if (script->CompiledData == nullptr)
	{
		throw gcnew CompileError(String::Format("Script has not been compiled: {0}", script->FileName));
	}

	((AGS::Native::CompiledScript^)script->CompiledData)->Data->Write(ooo);
}

void serialize_interaction_scripts(Interactions ^interactions, Stream *ooo)
{
	char textBuffer[256];
	ooo->WriteInt32(interactions->ScriptFunctionNames->Length);
	for each (String^ funcName in interactions->ScriptFunctionNames)
	{
		if (funcName == nullptr)
		{
			ooo->WriteByte(0);
		}
		else 
		{
			ConvertStringToCharArray(funcName, textBuffer, 256);
			fputstring(textBuffer, ooo);
		}
	}
}

void serialize_room_interactions(Stream *ooo) 
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
	const char *AGS_VERSION = "3.3.0";
  char textBuffer[500];
	int bb;

	Stream*ooo = Common::File::CreateFile(fileName);
	if (ooo == NULL) 
	{
		throw gcnew CompileError(String::Format("Cannot open file {0} for writing", gcnew String(fileName)));
	}

  ooo->Write(game_file_sig,30);
  ooo->WriteInt32(kGameVersion_Current);
  ooo->WriteInt32(strlen(AGS_VERSION));
  ooo->Write(AGS_VERSION, strlen(AGS_VERSION));

  {
    Common::AlignedStream align_s(ooo, Common::kAligned_Write);
    thisgame.WriteBaseToFile(&align_s);
    align_s.Close();
  }

  thisgame.Guid.WriteCount(ooo, MAX_GUID_LENGTH);
  thisgame.SavedGameFileExtension.WriteCount(ooo, MAX_SG_EXT_LENGTH);
  thisgame.SavedGameFolderName.WriteCount(ooo, MAX_SG_FOLDER_LEN);
  thisgame.FontFlags.WriteRaw(ooo);
  thisgame.FontOutline.WriteRaw(ooo);
  ooo->WriteInt32(MAX_SPRITES);
  thisgame.SpriteFlags.WriteRaw(ooo);
  if (thisgame.SpriteFlags.GetCount() < MAX_SPRITES)
  {
    ooo->WriteByteCount(0, (MAX_SPRITES - thisgame.SpriteFlags.GetCount()));
  }

  thisgame.WriteInvInfo_Aligned(ooo);
  thisgame.WriteMouseCursors_Aligned(ooo);
  
  for (bb = 0; bb < thisgame.CharacterCount; bb++)
  {
    serialize_interaction_scripts(game->Characters[bb]->Interactions, ooo);
  }
  for (bb = 1; bb < thisgame.InvItemCount; bb++)
  {
    serialize_interaction_scripts(game->InventoryItems[bb - 1]->Interactions, ooo);
  }

  if (thisgame.Dictionary != NULL) {
    write_dictionary (thisgame.Dictionary, ooo);
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

  ooo->WriteInt32(scriptsToWrite->Count);

  for each (Script ^script in scriptsToWrite)
  {
	  write_compiled_script(ooo, script);
  }

  for (bb = 0; bb < thisgame.ViewCount; bb++)
  {
    newViews[bb].WriteToFile(ooo);
  }

  thisgame.WriteCharacters_Aligned(ooo);

  for (int i = 0; i < MAXLIPSYNCFRAMES; ++i)
  {
    ooo->Write(&thisgame.LipSyncFrameLetters[0], 50);
  }

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

  ooo->WriteInt32(Common::kDialogVersion_Current);
  for (int i = 0; i < thisgame.DialogCount; ++i)
  {
      dialog[i].WriteToFile(ooo);
  }

  Common::Gui::WriteGui(guis, ooo);
  write_plugins_to_disk(ooo);
  // write the custom properties & schema
  thisgame.PropertySchema.Serialize(ooo);
  for (bb = 0; bb < thisgame.CharacterCount; bb++)
    thisgame.CharacterProperties[bb].Serialize (ooo);
  for (bb = 0; bb < thisgame.InvItemCount; bb++)
    thisgame.InvItemProperties[bb].Serialize (ooo);

  for (bb = 0; bb < thisgame.ViewCount; bb++)
    fputstring(thisgame.ViewNames[bb], ooo);

  for (bb = 0; bb < thisgame.InvItemCount; bb++)
    fputstring(thisgame.InventoryScriptNames[bb], ooo);

  for (bb = 0; bb < thisgame.DialogCount; bb++)
    fputstring(thisgame.DialogScriptNames[bb], ooo);


  int audioClipTypeCount = game->AudioClipTypes->Count + 1;
  ooo->WriteInt32(audioClipTypeCount);
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
  ooo->WriteArray(clipTypes, sizeof(::AudioClipType), audioClipTypeCount);
  free(clipTypes);

  IList<AudioClip^>^ allClips = game->CachedAudioClipListForCompile;
  ooo->WriteInt32(allClips->Count);
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

  {
    Common::AlignedStream align_s(ooo, Common::kAligned_Write);
    for (int i = 0; i < allClips->Count; ++i)
    {
      compiledAudioClips[i].WriteToFile(&align_s);
      align_s.Reset();
    }
    align_s.Close();
  }

  free(compiledAudioClips);
  ooo->WriteInt32(game->GetAudioArrayIndexFromAudioClipIndex(game->Settings->PlaySoundOnScore));

  if (game->Settings->DebugMode)
  {
    ooo->WriteInt32(game->Rooms->Count);
    for (bb = 0; bb < game->Rooms->Count; bb++)
    {
      IRoom ^room = game->Rooms[bb];
      ooo->WriteInt32(room->Number);
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

  delete ooo;
}

void save_game_to_dta_file(Game^ game, const char *fileName)
{
	thisgame.Options[OPT_ALWAYSSPCH] = game->Settings->AlwaysDisplayTextAsSpeech;
	thisgame.Options[OPT_ANTIALIASFONTS] = game->Settings->AntiAliasFonts;
	thisgame.Options[OPT_ANTIGLIDE] = game->Settings->AntiGlideMode;
	thisgame.Options[OPT_NOWALKMODE] = !game->Settings->AutoMoveInWalkMode;
	thisgame.Options[OPT_RIGHTLEFTWRITE] = game->Settings->BackwardsText;
	thisgame.ColorDepth = (int)game->Settings->ColorDepth;
	thisgame.Options[OPT_COMPRESSSPRITES] = game->Settings->CompressSprites;
	thisgame.Options[OPT_CROSSFADEMUSIC] = (int)game->Settings->CrossfadeMusic;
	thisgame.Options[OPT_DEBUGMODE] = game->Settings->DebugMode;
	thisgame.Options[OPT_DIALOGUPWARDS] = game->Settings->DialogOptionsBackwards;
	thisgame.Options[OPT_DIALOGGAP] = game->Settings->DialogOptionsGap;
	thisgame.Options[OPT_DIALOGIFACE] = game->Settings->DialogOptionsGUI;
	thisgame.DialogBulletSprIndex = game->Settings->DialogOptionsBullet;
	thisgame.Options[OPT_DUPLICATEINV] = game->Settings->DisplayMultipleInventory;
	thisgame.Options[OPT_STRICTSTRINGS] = game->Settings->EnforceNewStrings;
	thisgame.Options[OPT_STRICTSCRIPTING] = game->Settings->EnforceObjectBasedScript;
	thisgame.Options[OPT_NOSCALEFNT] = game->Settings->FontsForHiRes;
	ConvertStringToNativeString(game->Settings->GameName, thisgame.GameName, 50);
	thisgame.Options[OPT_NEWGUIALPHA] = (int)game->Settings->GUIAlphaStyle;
	thisgame.Options[OPT_HANDLEINVCLICKS] = game->Settings->HandleInvClicksInScript;
	thisgame.Options[OPT_FIXEDINVCURSOR] = !game->Settings->InventoryCursors;
  thisgame.Options[OPT_OLDTALKANIMSPD] = game->Settings->LegacySpeechAnimationSpeed;
	thisgame.Options[OPT_LEFTTORIGHTEVAL] = game->Settings->LeftToRightPrecedence;
	thisgame.Options[OPT_LETTERBOX] = game->Settings->LetterboxMode;
  thisgame.TotalScore = game->Settings->MaximumScore;
	thisgame.Options[OPT_MOUSEWHEEL] = game->Settings->MouseWheelEnabled;
	thisgame.Options[OPT_DIALOGNUMBERED] = game->Settings->NumberDialogOptions;
	thisgame.Options[OPT_PIXPERFECT] = game->Settings->PixelPerfect;
	thisgame.Options[OPT_SCORESOUND] = 0; // saved elsewhere now to make it 32-bit
	thisgame.DefaultResolution = (int)game->Settings->Resolution;
	thisgame.Options[OPT_FADETYPE] = (int)game->Settings->RoomTransition;
	thisgame.Options[OPT_RUNGAMEDLGOPTS] = game->Settings->RunGameLoopsWhileDialogOptionsDisplayed;
	thisgame.Options[OPT_SAVESCREENSHOT] = game->Settings->SaveScreenshots;
	thisgame.Options[OPT_NOSKIPTEXT] = (int)game->Settings->SkipSpeech;
	thisgame.Options[OPT_PORTRAITSIDE] = (int)game->Settings->SpeechPortraitSide;
	thisgame.Options[OPT_SPEECHTYPE] = (int)game->Settings->SpeechStyle;
	thisgame.Options[OPT_SPLITRESOURCES] = game->Settings->SplitResources;
	thisgame.Options[OPT_TWCUSTOM] = game->Settings->TextWindowGUI;
	thisgame.Options[OPT_THOUGHTGUI] = game->Settings->ThoughtGUI;
	thisgame.Options[OPT_TURNTOFACELOC] = game->Settings->TurnBeforeFacing;
	thisgame.Options[OPT_ROTATECHARS] = game->Settings->TurnBeforeWalking;
	thisgame.Options[OPT_NATIVECOORDINATES] = !game->Settings->UseLowResCoordinatesInScript;
	thisgame.Options[OPT_WALKONLOOK] = game->Settings->WalkInLookMode;
	thisgame.Options[OPT_DISABLEOFF] = (int)game->Settings->WhenInterfaceDisabled;
	thisgame.UniqueId = game->Settings->UniqueID;
  ConvertStringToNativeString(game->Settings->GUIDAsString, thisgame.Guid, MAX_GUID_LENGTH);
  ConvertStringToNativeString(game->Settings->SaveGameFolderName, thisgame.SavedGameFolderName, MAX_SG_FOLDER_LEN);

  if (game->Settings->EnhancedSaveGames)
  {
    ConvertStringToNativeString(game->Settings->SaveGameFileExtension, thisgame.SavedGameFileExtension, MAX_SG_EXT_LENGTH);
  }
  else 
  {
    thisgame.SavedGameFileExtension.Empty();
  }

	thisgame.InvItemHotDotColor = game->Settings->InventoryHotspotMarker->DotColor;
	thisgame.InvItemHotDotOuterColor = game->Settings->InventoryHotspotMarker->CrosshairColor;
	thisgame.InvItemHotDotSprIndex = game->Settings->InventoryHotspotMarker->Image;
	if (game->Settings->InventoryHotspotMarker->Style == InventoryHotspotMarkerStyle::Crosshair)
	{
		thisgame.InvItemHotDotSprIndex = 0;
	}
	else if (game->Settings->InventoryHotspotMarker->Style == InventoryHotspotMarkerStyle::None)
	{
		thisgame.InvItemHotDotSprIndex = 0;
		thisgame.InvItemHotDotColor = 0;
	}

	// ** Palette **
	int i;
	for (i = 0; i < 256; i++) 
	{
		if (game->Palette[i]->ColourType == PaletteColourType::Background) 
		{
			thisgame.PaletteUses[i] = PAL_BACKGROUND;
		}
		else 
		{
			thisgame.PaletteUses[i] = PAL_GAMEWIDE;
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
    int viewCount = AGS::Types::FolderHelper::CountViews(AGS::Types::FolderHelper::GetRootViewFolder(game));

	thisgame.ViewCount = viewCount;
  allocate_memory_for_views(viewCount);
  numNewViews = viewCount;

  ManagedViewProcessing::ConvertViewsToDTAFormat(AGS::Types::FolderHelper::GetRootViewFolder(game), game);

	// ** Characters **
	thisgame.CharacterCount = game->Characters->Count;
	thisgame.Characters.New(thisgame.CharacterCount);
    thisgame.CharacterProperties.New(thisgame.CharacterCount);
    thisgame.CharacterInteractions.New(thisgame.CharacterCount, NULL);
    thisgame.CharacterInteractionScripts.New(thisgame.CharacterCount, NULL);
	for (i = 0; i < thisgame.CharacterCount; i++) 
	{
		AGS::Types::Character ^character = game->Characters[i];
		thisgame.Characters[i].flags = 0;
		thisgame.Characters[i].on = 1;
		if (character->AdjustSpeedWithScaling) thisgame.Characters[i].flags |= CHF_SCALEMOVESPEED;
		if (character->AdjustVolumeWithScaling) thisgame.Characters[i].flags |= CHF_SCALEVOLUME;
		if (!character->Clickable) thisgame.Characters[i].flags |= CHF_NOINTERACT;
		if (!character->DiagonalLoops) thisgame.Characters[i].flags |= CHF_NODIAGONAL;
    if (character->MovementLinkedToAnimation) thisgame.Characters[i].flags |= CHF_ANTIGLIDE;
		if (!character->Solid) thisgame.Characters[i].flags |= CHF_NOBLOCKING;
		if (!character->TurnBeforeWalking) thisgame.Characters[i].flags |= CHF_NOTURNING;
		if (!character->UseRoomAreaLighting) thisgame.Characters[i].flags |= CHF_NOLIGHTING;
		if (!character->UseRoomAreaScaling) thisgame.Characters[i].flags |= CHF_MANUALSCALING;

		if (character->UniformMovementSpeed) 
		{
			thisgame.Characters[i].walkspeed = character->MovementSpeed;
			thisgame.Characters[i].walkspeed_y = UNIFORM_WALK_SPEED;
		}
		else 
		{
			thisgame.Characters[i].walkspeed = character->MovementSpeedX;
			thisgame.Characters[i].walkspeed_y = character->MovementSpeedY;
		}

		thisgame.Characters[i].animspeed = character->AnimationDelay;
		thisgame.Characters[i].blinkview = character->BlinkingView - 1;
		thisgame.Characters[i].idleview = character->IdleView - 1;
		thisgame.Characters[i].defview = character->NormalView - 1;
		thisgame.Characters[i].view = thisgame.Characters[i].defview;
		ConvertStringToCharArray(character->RealName, thisgame.Characters[i].name, 40);
		ConvertStringToCharArray(character->ScriptName, thisgame.Characters[i].scrname, MAX_SCRIPT_NAME_LEN);
		thisgame.Characters[i].talkcolor = character->SpeechColor;
		thisgame.Characters[i].talkview = character->SpeechView - 1;
		thisgame.Characters[i].room = character->StartingRoom;
    thisgame.Characters[i].speech_anim_speed = character->SpeechAnimationDelay;
		thisgame.Characters[i].x = character->StartX;
		thisgame.Characters[i].y = character->StartY;
		thisgame.Characters[i].thinkview = character->ThinkingView - 1;

		CompileCustomProperties(character->Properties, &thisgame.CharacterProperties[i]);
	}
	thisgame.PlayerCharacterIndex = game->PlayerCharacter->ID;

	// ** Text Parser **
  thisgame.Dictionary = (WordsDictionary*)malloc(sizeof(WordsDictionary));
  thisgame.Dictionary->allocate_memory(game->TextParser->Words->Count);
  for (i = 0; i < thisgame.Dictionary->num_words; i++)
	{
    ConvertStringToCharArray(game->TextParser->Words[i]->Word, thisgame.Dictionary->word[i], MAX_PARSER_WORD_LENGTH);
    thisgame.Dictionary->wordnum[i] = game->TextParser->Words[i]->WordGroup;
	}

	// ** Global Messages **
  thisgame.GlobalMessages.New(MAXGLOBALMES);
	for (i = 0; i < MAXGLOBALMES; i++) 
	{
		if (game->GlobalMessages[i] != String::Empty) 
		{
			ConvertStringToNativeString(game->GlobalMessages[i], thisgame.GlobalMessages[i]);
		}
	}

	// ** Lip Sync **
	if (game->LipSync->Type == LipSyncType::Text)
	{
		thisgame.Options[OPT_LIPSYNCTEXT] = 1;
	}
	else 
	{
		thisgame.Options[OPT_LIPSYNCTEXT] = 0;
	}
	thisgame.DefaultLipSyncFrame = game->LipSync->DefaultFrame;
    thisgame.LipSyncFrameLetters.New(MAXLIPSYNCFRAMES);
	for (i = 0; i < MAXLIPSYNCFRAMES; i++) 
	{
		ConvertStringToCharArray(game->LipSync->CharactersPerFrame[i], thisgame.LipSyncFrameLetters[i], 50);
	}

	// ** Dialogs **
	thisgame.DialogCount = game->Dialogs->Count;
	thisgame.DialogMessageCount = 0;
	dialog.New(thisgame.DialogCount);
    thisgame.DialogScriptNames.New(thisgame.DialogCount);
	for (i = 0; i < thisgame.DialogCount; i++) 
	{
		Dialog ^curDialog = game->Dialogs[i];
		dialog[i].OptionCount = curDialog->Options->Count;
        dialog[i].Options.New(dialog[i].OptionCount);
		for (int j = 0; j < dialog[i].OptionCount; j++) 
		{
			AGS::Types::DialogOption ^option = curDialog->Options[j];
			ConvertStringToNativeString(option->Text, dialog[i].Options[j].Name, 150);
			//dialog[i].entrypoints[j] = option->EntryPointOffset;
			dialog[i].Options[j].Flags = 0;
			if (!option->Say) 
			{
				dialog[i].Options[j].Flags |= Common::kDialogOption_NoRepeat;
			}
			if (option->Show)
			{
				dialog[i].Options[j].Flags |= Common::kDialogOption_IsOn;
			}
		}
		
/*	    dialog[i].optionscripts = (unsigned char*)malloc(curDialog->CodeSize);
	    Marshal::Copy(curDialog->CompiledCode, 0, IntPtr(dialog[i].optionscripts), curDialog->CodeSize);

		dialog[i].OldCodeSize = curDialog->CodeSize;
		dialog[i].startupentrypoint = curDialog->StartupEntryPointOffset;

		dlgscript[i] = (char*)malloc(curDialog->Script->Length + 1);
		ConvertStringToCharArray(curDialog->Script, dlgscript[i]);*/

		ConvertStringToNativeString(curDialog->Name, thisgame.DialogScriptNames[i], MAX_SCRIPT_NAME_LEN);

		dialog[i].Flags = 0;
		if (curDialog->ShowTextParser) dialog[i].Flags |= Common::kDialogTopic_ShowParser;
	}

	// ** Cursors **
	thisgame.MouseCursorCount = game->Cursors->Count;
    thisgame.MouseCursors.New(thisgame.MouseCursorCount);
	for (i = 0; i < thisgame.MouseCursorCount; i++)
	{
		AGS::Types::MouseCursor ^cursor = game->Cursors[i];
		thisgame.MouseCursors[i].flags = 0;
		if (cursor->Animate) 
		{
			thisgame.MouseCursors[i].view = cursor->View - 1;
			if (cursor->AnimateOnlyOnHotspots) thisgame.MouseCursors[i].flags |= MCF_HOTSPOT;
			if (cursor->AnimateOnlyWhenMoving) thisgame.MouseCursors[i].flags |= MCF_ANIMMOVE;
		}
		else 
		{
			thisgame.MouseCursors[i].view = -1;
		}
		if (cursor->StandardMode) thisgame.MouseCursors[i].flags |= MCF_STANDARD;
		thisgame.MouseCursors[i].pic = cursor->Image;
		thisgame.MouseCursors[i].hotx = cursor->HotspotX;
		thisgame.MouseCursors[i].hoty = cursor->HotspotY;
		ConvertStringToCharArray(cursor->Name, thisgame.MouseCursors[i].name, 10);
	}

	// ** Fonts **
	if (game->Fonts->Count > MAX_FONTS)
	{
		throw gcnew CompileError("Too many fonts");
	}
	thisgame.FontCount = game->Fonts->Count;
    thisgame.FontFlags.New(thisgame.FontCount);
	for (i = 0; i < thisgame.FontCount; i++) 
	{
		AGS::Types::Font ^font = game->Fonts[i];
		thisgame.FontFlags[i] = font->PointSize & FFLG_SIZEMASK;
		if (font->OutlineStyle == FontOutlineStyle::None) 
		{
			thisgame.FontOutline[i] = -1;
		}
		else if (font->OutlineStyle == FontOutlineStyle::Automatic) 
		{
			thisgame.FontOutline[i] = FONT_OUTLINE_AUTO;
		}
		else
		{
			thisgame.FontOutline[i] = font->OutlineFont;
		}
	}

	// ** Inventory items **
	if (game->InventoryItems->Count > MAX_INV)
	{
		throw gcnew CompileError("Too many inventory items");
	}
	thisgame.InvItemCount = game->InventoryItems->Count + 1;
    thisgame.InventoryItems.New(thisgame.InvItemCount);
    thisgame.InventoryScriptNames.New(thisgame.InvItemCount);
    thisgame.InvItemProperties.New(thisgame.InvItemCount);
    thisgame.InvItemInteractions.New(thisgame.InvItemCount, NULL);
    thisgame.InvItemInteractionScripts.New(thisgame.InvItemCount, NULL);
	for (i = 1; i < thisgame.InvItemCount; i++)
	{
		InventoryItem^ invItem = game->InventoryItems[i - 1];
		ConvertStringToCharArray(invItem->Description, thisgame.InventoryItems[i].name, 25);
		ConvertStringToNativeString(invItem->Name, thisgame.InventoryScriptNames[i], MAX_SCRIPT_NAME_LEN);
		thisgame.InventoryItems[i].pic = invItem->Image; 
    thisgame.InventoryItems[i].cursorPic = invItem->CursorImage;
		thisgame.InventoryItems[i].hotx = invItem->HotspotX;
		thisgame.InventoryItems[i].hoty = invItem->HotspotY;
		thisgame.InventoryItems[i].flags = 0;
		if (invItem->PlayerStartsWithItem) thisgame.InventoryItems[i].flags |= IFLG_STARTWITH;

		CompileCustomProperties(invItem->Properties, &thisgame.InvItemProperties[i]);
	}

	// ** Custom Properties Schema **
	List<AGS::Types::CustomPropertySchemaItem ^> ^schema = game->PropertySchema->PropertyDefinitions;
	thisgame.PropertySchema.Free();
    AGS::Common::String prop_name;
    AGS::Common::String prop_desc;
    AGS::Common::String prop_defval;
	for (i = 0; i < schema->Count; i++) 
	{
		CustomPropertySchemaItem ^schemaItem = schema[i];
		ConvertStringToNativeString(schemaItem->Name, prop_name);
		ConvertStringToNativeString(schemaItem->Description, prop_desc);
		ConvertStringToNativeString(schemaItem->DefaultValue, prop_defval);
        thisgame.PropertySchema.AddProperty(prop_name,
            (AGS::Common::CustomPropertyType)schemaItem->Type, prop_desc, prop_defval);
	}

	// ** GUIs **
	numguibuts = 0;
	numguilabels = 0;
	numguitext = 0;
	numguilist = 0;
	numguislider = 0;
	numguiinv = 0;

	thisgame.GuiCount = game->GUIs->Count;
    guis.SetLength(thisgame.GuiCount);
	for (i = 0; i < thisgame.GuiCount; i++)
	{
		guis[i].Init();
		ConvertGUIToBinaryFormat(game->GUIs[i], &guis[i]);
		guis[i].HighlightControl = -1;
	}

	thisgame.LoadCompiledScript = true;

	save_thisgame_to_file(fileName, game);
	
	free_old_game_data();
}



#pragma unmanaged


void quit(const char * message) 
{
	ThrowManagedException((const char*)message);
}



// ** GRAPHICAL SCRIPT LOAD/SAVE ROUTINES ** //

long getlong(Stream*iii) {
  long tmm;
  tmm = iii->ReadInt32();
  return tmm;
}

void AGS::Common::RoomInfo::SaveScriptConfiguration(Stream*iii) const
{
  // no variable names
  iii->WriteInt32 (1);
  iii->WriteInt32 (0);
}

void AGS::Common::RoomInfo::LoadScriptConfiguration(Stream*iii)
{
  int aa;
  if (getlong(iii)!=1) quit("ScriptEdit: invliad config version");
  int numvarnames=getlong(iii);
  for (aa=0;aa<numvarnames;aa++) {
    int lenoft=iii->ReadByte();
    iii->Seek(Common::kSeekCurrent,lenoft);
  }
}

void AGS::Common::RoomInfo::SaveGraphicalScripts(Stream*fff) const
{
  // no script
  fff->WriteInt32 (-1);
}

char*scripttempn="~acsc%d.tmp";
void AGS::Common::RoomInfo::LoadGraphicalScripts(Stream*iii)
{
  long ct;
  bool doneMsg = false;
  while (1) {
    ct = iii->ReadInt32();
    if ((ct==-1) | (iii->EOS()!=0)) break;
    if (!doneMsg) {
//      infoBox("WARNING: This room uses graphical scripts, which have been removed from this version. If you save the room now, all graphical scripts will be lost.");
      doneMsg = true;
    }
    // skip the data
    long lee = iii->ReadInt32();
    iii->Seek (Common::kSeekCurrent, lee);
  }
}

void update_polled_stuff_if_runtime()
{
	// do nothing
}

// [IKM] 2012-06-07
// Had to copy this variable definition from Engine/ac.cpp, since it is required in acgui.cpp // GUIInv::CalculateNumCells()
// due JJS's compatiblity fix for 2.70.
// This *must* be not less than 31 (v270), otherwise function will work in backward-compatibility mode.
GameDataVersion loaded_game_file_version = kGameVersion_270;
