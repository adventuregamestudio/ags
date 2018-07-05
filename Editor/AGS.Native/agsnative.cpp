#define USE_CLIB
#include <stdio.h>
void ThrowManagedException(const char *message);
#pragma unmanaged
#pragma warning (disable: 4996 4312)  // disable deprecation warnings
extern bool Scintilla_RegisterClasses(void *hInstance);
extern int Scintilla_LinkLexers();

int antiAliasFonts = 0;
bool ShouldAntiAliasText() { return (antiAliasFonts != 0); }

int mousex, mousey;
#include "agsnative.h"
#include "util/wgt2allg.h"
#include "util/misc.h"
#include "ac/spritecache.h"
#include "ac/actiontype.h"
#include "ac/roomstruct.h"
#include "ac/scriptmodule.h"
#include "ac/gamesetupstruct.h"
#include "font/fonts.h"
#include "game/main_game_file.h"
#include "game/plugininfo.h"
#include "gui/guimain.h"
#include "gui/guiinv.h"
#include "gui/guibutton.h"
#include "gui/guilabel.h"
#include "gui/guitextbox.h"
#include "gui/guilistbox.h"
#include "gui/guislider.h"
#include "util/compress.h"
#include "util/string_types.h"
#include "util/string_utils.h"    // fputstring, etc
#include "util/alignedstream.h"
#include "util/filestream.h"
#include "gfx/bitmap.h"
#include "core/assetmanager.h"
#include "NativeUtils.h"

using AGS::Common::AlignedStream;
using AGS::Common::Stream;
namespace AGSProps = AGS::Common::Properties;
namespace BitmapHelper = AGS::Common::BitmapHelper;
namespace AGSProps = AGS::Common::Properties;
using AGS::Common::GUIMain;
using AGS::Common::Interaction;
using AGS::Common::InteractionCommand;
using AGS::Common::InteractionCommandList;
typedef AGS::Common::String AGSString;

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
GameSetupStruct thisgame;
SpriteCache spriteset(MAX_SPRITES + 2);
GUIMain tempgui;
const char*sprsetname = "acsprset.spr";
const char *old_editor_data_file = "editor.dat";
const char *new_editor_data_file = "game.agf";
const char *old_editor_main_game_file = "ac2game.dta";
const char *TEMPLATE_LOCK_FILE = "template.dta";
const char *ROOM_TEMPLATE_ID_FILE = "rtemplate.dat";
const int ROOM_TEMPLATE_ID_FILE_SIGNATURE = 0x74673812;
bool spritesModified = false;
roomstruct thisroom;
bool roomModified = false;
Common::Bitmap *drawBuffer = NULL;
Common::Bitmap *undoBuffer = NULL;
int loaded_room_number = -1;

GameDataVersion loaded_game_file_version = kGameVersion_Current;

// stuff for importing old games
int numScriptModules;
ScriptModule* scModules = NULL;
DialogTopic *dialog;
std::vector<Common::String> dlgscript;
std::vector<GUIMain> guis;
ViewStruct *newViews;
int numNewViews = 0;

// A reference color depth, for correct color selection;
// originally was defined by 'abuf' bitmap.
int BaseColorDepth;


bool reload_font(int curFont);
void drawBlockScaledAt(int hdc, Common::Bitmap *todraw ,int x, int y, int scaleFactor);
// this is to shut up the linker, it's used by CSRUN.CPP
void write_log(const char *) { }

void GUIInv::Draw(Common::Bitmap *ds) {
  color_t draw_color = ds->GetCompatibleColor(15);
  ds->DrawRect(Rect(x,y,x+wid,y+hit), draw_color);
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
		    inpu->Seek(30);
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
  if (tblock->GetBPP() != thisgame.color_depth) {
    Common::Bitmap *tempst=Common::BitmapHelper::CreateBitmapCopy(tblock, thisgame.color_depth*8);
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

void draw_gui_sprite(Common::Bitmap *g, int sprnum, int atxp, int atyp, bool use_alpha, Common::BlendMode blend_mode) {
  Common::Bitmap *blptr = get_sprite(sprnum);
  Common::Bitmap *towrite=blptr;
  int needtofree=0, main_color_depth = thisgame.color_depth * 8;

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

Common::Bitmap *get_bitmap_for_mask(roomstruct *roomptr, RoomAreaMask maskType) 
{
	if (maskType == RoomAreaMask::None) 
	{
		return NULL;
	}

	Common::Bitmap *source = NULL;
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
	theRoom->regions->Blit(theRoom->walls, 0, 0, 0, 0, theRoom->regions->GetWidth(), theRoom->regions->GetHeight());
}

int get_mask_pixel(void *roomptr, int maskType, int x, int y)
{
	Common::Bitmap *mask = get_bitmap_for_mask((roomstruct*)roomptr, (RoomAreaMask)maskType);
	return mask->GetPixel(x, y);
}

void draw_line_onto_mask(void *roomptr, int maskType, int x1, int y1, int x2, int y2, int color)
{
	Common::Bitmap *mask = get_bitmap_for_mask((roomstruct*)roomptr, (RoomAreaMask)maskType);
	mask->DrawLine(Line(x1, y1, x2, y2), color);
}

void draw_filled_rect_onto_mask(void *roomptr, int maskType, int x1, int y1, int x2, int y2, int color)
{
	Common::Bitmap *mask = get_bitmap_for_mask((roomstruct*)roomptr, (RoomAreaMask)maskType);
    mask->FillRect(Rect(x1, y1, x2, y2), color);
}

void draw_fill_onto_mask(void *roomptr, int maskType, int x1, int y1, int color)
{
	Common::Bitmap *mask = get_bitmap_for_mask((roomstruct*)roomptr, (RoomAreaMask)maskType);
    mask->FloodFill(x1, y1, color);
}

void create_undo_buffer(void *roomptr, int maskType) 
{
	Common::Bitmap *mask = get_bitmap_for_mask((roomstruct*)roomptr, (RoomAreaMask)maskType);
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
  	Common::Bitmap *mask = get_bitmap_for_mask((roomstruct*)roomptr, (RoomAreaMask)maskType);
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

void draw_area_mask(roomstruct *roomptr, Common::Bitmap *ds, RoomAreaMask maskType, int selectedArea, int transparency) 
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
	roomstruct *roomptr = (roomstruct*)roomvoidptr;

  if (bgnum >= roomptr->num_bscenes)
    return;

  Common::Bitmap *srcBlock = roomptr->ebscene[bgnum];
  if (srcBlock == NULL)
    return;

	if (drawBuffer != NULL) 
	{
		Common::Bitmap *depthConverted = Common::BitmapHelper::CreateBitmap(srcBlock->GetWidth(), srcBlock->GetHeight(), drawBuffer->GetColorDepth());
    if (srcBlock->GetColorDepth() == 8)
    {
      select_palette(roomptr->bpalettes[bgnum]);
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
  if ((thisgame.options[OPT_NOSCALEFNT] == 0) &&
      thisgame.IsHiRes()) {
    wtext_multiply = 2;
  }

  if (multiplyWas != wtext_multiply) {
    // resolution or Scale Up Fonts has changed, reload at new size
    for (int bb=0;bb<thisgame.numfonts;bb++)
      reload_font (bb);
  }

  if (thisgame.IsHiRes()) {
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
  iii->Seek(3);
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
    iii->Seek(theiroffs[aa]+2, Common::kSeekBegin);
    int wwi=iii->ReadByte()-1;
    int hhi=iii->ReadByte();
    coffsets[aa]=ooo->GetPosition();
    ooo->WriteInt16(wwi+1);
    ooo->WriteInt16(hhi);
    if ((wwi<1) | (hhi<1)) continue;
    memset(buffer,0,1000);
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
  ooo->Seek(15, Common::kSeekBegin);
  ooo->WriteInt16(tableat); 
  delete ooo;
  delete iii;
  wfreefont(fslot);
  FontInfo fi;
  if (!wloadfont_size(fslot, fi))
  {
    return "Unable to load converted WFN file";
  }
  return NULL;
}


int drawFontAt (int hdc, int fontnum, int x, int y, int width) {
  
  if (fontnum >= thisgame.numfonts)
  {
    return 0;
  }

  update_font_sizes();

  int doubleSize = (!thisgame.IsHiRes()) ? 2 : 1;
  int blockSize = (!thisgame.IsHiRes()) ? 1 : 2;
  antiAliasFonts = thisgame.options[OPT_ANTIALIASFONTS];

  int char_height = thisgame.fontflags[fontnum] & FFLG_SIZEMASK;
  int grid_size   = max(10, char_height);
  int grid_margin = max(4, grid_size / 4);
  grid_size += grid_margin * 2;
  grid_size *= blockSize;
  int first_char = 0;
  int num_chars  = 256;
  int padding = 5;

  if (doubleSize > 1)
      width /= 2;
  int chars_per_row = max(1, (width - (padding * 2)) / grid_size);
  int height = (num_chars / chars_per_row + 1) * grid_size + padding * 2;

  if (!hdc)
    return height * doubleSize;

  // we can't antialias font because changing col dep to 16 here causes
  // it to crash ... why?
  Common::Bitmap *tempblock = Common::BitmapHelper::CreateBitmap(width, height, 8);
  tempblock->Fill(0);
  color_t text_color = tempblock->GetCompatibleColor(15); // fixed white color
  for (int c = first_char; c < num_chars; ++c)
  {
    wgtprintf(tempblock,
                padding + (c % chars_per_row) * grid_size + grid_margin,
                padding + (c / chars_per_row) * grid_size + grid_margin,
                fontnum, text_color, "%c", c);
  }

  if (doubleSize > 1) 
    drawBlockDoubleAt(hdc, tempblock, x, y);
  else
    drawBlock((HDC)hdc, tempblock, x, y);
   
  delete tempblock;
  return height * doubleSize;
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

  Common::Bitmap *todraw = Common::BitmapHelper::CreateBitmap (wtoDraw, size, thisgame.color_depth*8);
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
      int linecol = makecol_depth(thisgame.color_depth * 8, 0, 64, 200);
      if (thisgame.color_depth == 1)
        linecol = 12;

      // Draw dividing line
	  todraw->DrawLine (Line(size*(i+1) - 1, 0, size*(i+1) - 1, size-1), linecol);
    }
    if (i == cursel) {
      // Selected item
      int linecol = makecol_depth(thisgame.color_depth * 8, 255, 255,255);
      if (thisgame.color_depth == 1)
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
  Common::Bitmap *tsp = get_sprite(spr);
  if (tsp == NULL) return 0;

  int retval = tsp->GetHeight();

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
	__my_setcolor(&colNum, colNum, BaseColorDepth);
  /*if (thisgame.color_depth > 2) {
    // convert to 24-bit colour
    int red = ((colNum >> 11) & 0x1f) * 8;
    int grn = ((colNum >> 5) & 0x3f) * 4;
    int blu = (colNum & 0x1f) * 8;
    colNum = (red << _rgb_r_shift_32) | (grn << _rgb_g_shift_32) | (blu << _rgb_b_shift_32);
  }*/

  Common::Bitmap *palbmp = Common::BitmapHelper::CreateBitmap(width, height, thisgame.color_depth * 8);
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
  FontInfo fi;
  wloadfont_size(thisgame.numfonts, fi);
  thisgame.fontflags[thisgame.numfonts] = 0;
  thisgame.fontoutline[thisgame.numfonts] = -1;
  thisgame.fontvoffset[thisgame.numfonts] = 0;
  thisgame.fontlnspace[thisgame.numfonts] = 0;
  thisgame.numfonts++;
}

bool initialize_native()
{
    Common::AssetManager::CreateInstance();

  set_uformat(U_ASCII);  // required to stop ALFONT screwing up text
	install_allegro(SYSTEM_NONE, &errno, atexit);
	//set_gdi_color_format();
	palette = &thisgame.defpal[0];
	thisgame.color_depth = 2;
	//abuf = Common::BitmapHelper::CreateBitmap(10, 10, 32);
    BaseColorDepth = 32;
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
	int scaleFactor = ((thisgame.spriteflags[spriteNum] & SPF_640x400) != 0) ? 1 : 2;
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

  if ((tempgui.Width < 1) || (tempgui.Height < 1))
    return;

  //update_font_sizes();

  if (scaleFactor == 1) {
    dsc_want_hires = 1;
  }

  Common::Bitmap *tempblock = Common::BitmapHelper::CreateBitmap(tempgui.Width, tempgui.Height, thisgame.color_depth*8);
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
    set_palette_range(palette, 0, 255, 0);
  }
  else if (toimp->GetColorDepth() == 8) {  // hi-colour game
    set_palette(itspal);
  }
}

void update_abuf_coldepth() {
//  delete abuf;
//  abuf = Common::BitmapHelper::CreateBitmap(10, 10, thisgame.color_depth * 8);
    BaseColorDepth = thisgame.color_depth * 8;
}

bool reload_font(int curFont)
{
  wfreefont(curFont);

  FontInfo fi;
  make_fontinfo(thisgame, curFont, fi);

  // TODO: for some reason these compat fixes are different in the engine, investigate
  // if the font is designed for 640x400, half it
  if (thisgame.options[OPT_NOSCALEFNT]) {
    if (!thisgame.IsHiRes())
      fi.SizePt /= 2;
  }
  else if (thisgame.IsHiRes()) {
    // designed for 320x200, double it up
    fi.SizePt *= 2;
  }
  return wloadfont_size(curFont, fi);
}

bool reset_sprite_file() {
  spriteset.reset();
  if (spriteset.initFile(sprsetname))
    return false;
  spriteset.maxCacheSize = 100000000;  // 100 mb cache
  return true;
}

Common::PluginInfo thisgamePlugins[MAX_PLUGINS];
int numThisgamePlugins = 0;

const char *init_game_after_import(const AGS::Common::LoadedGameEntities &ents, GameDataVersion data_ver)
{
    dlgscript = ents.OldDialogSources;
    numNewViews = thisgame.numviews;
    numScriptModules = (int)ents.ScriptModules.size();
    scModules = (ScriptModule*)realloc(scModules, sizeof(ScriptModule) * numScriptModules);
    for (int i = 0; i < numScriptModules; i++)
    {
        scModules[i].init();
        scModules[i].compiled = ents.ScriptModules[i];
    }

    numThisgamePlugins = (int)ents.PluginInfos.size();
    for (int i = 0; i < numThisgamePlugins; ++i)
    {
        thisgamePlugins[i] = ents.PluginInfos[i];
        // we don't care if it's an editor-only plugin or not
        if (thisgamePlugins[i].Name.GetLast() == '!')
            thisgamePlugins[i].Name.ClipRight(1);
    }

    for (int i = 0; i < thisgame.numgui; ++i)
    {
        guis[i].RebuildArray();
    }

    // reset colour 0, it's possible for it to get corrupted
    palette[0].r = 0;
    palette[0].g = 0;
    palette[0].b = 0;
    set_palette_range(palette, 0, 255, 0);

    if (!reset_sprite_file())
        return "The sprite file could not be loaded. Ensure that all your game files are intact and not corrupt. The game may require a newer version of AGS.";

    for (int i = 0; i < MAX_FONTS; ++i)
        wfreefont(i);
    for (int i = 0; i < thisgame.numfonts; ++i)
        reload_font(i);

    update_abuf_coldepth();
    spritesModified = false;
    thisgame.filever = data_ver;
    return NULL;
}

const char *load_dta_file_into_thisgame(const char *fileName)
{
    AGS::Common::MainGameSource src;
    AGS::Common::LoadedGameEntities ents(thisgame, dialog, newViews);
    MainGameFileError load_err = AGS::Common::OpenMainGameFile(fileName, src);
    if (load_err == Common::kMGFErr_NoError)
    {
        load_err = AGS::Common::ReadGameData(ents, src.InputStream.get(), src.DataVersion);
        if (load_err == Common::kMGFErr_NoError)
            load_err = AGS::Common::UpdateGameData(ents, src.DataVersion);
    }
    if (load_err != Common::kMGFErr_NoError)
    {
        return AGS::Common::GetMainGameFileErrorText(load_err);
    }
    return init_game_after_import(ents, src.DataVersion);
}

void free_script_module(int index) {
  free(scModules[index].name);
  free(scModules[index].author);
  free(scModules[index].version);
  free(scModules[index].description);
  free(scModules[index].script);
  free(scModules[index].scriptHeader);
  scModules[index].compiled.reset();
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
  thisgame.charProps.clear();
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
  thisgame.viewNames.clear();
  free(newViews);
  guis.clear();
  free(thisgame.chars);
  thisgame.dict->free_memory();
  free(thisgame.dict);
  free(dialog);
  free_script_modules();
}

// remap the scene, from its current palette oldpale to palette
void remap_background (Common::Bitmap *scene, color *oldpale, color*palette, int exactPal) {
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

  load_room((char*)rtlo, &thisroom, thisgame.IsHiRes());

  if (thisroom.wasversion < kRoomVersion_250b) 
  {
	  return "This room was saved with an old version of the editor and cannot be opened. Use AGS 2.72 to upgrade this room file.";
  }

  //thisroom.numhotspots = MAX_HOTSPOTS;

  // Allocate enough memory to add extra variables
  InteractionVariable *ivv = new InteractionVariable[MAX_GLOBAL_VARIABLES];
  for (int i = 0; i < thisroom.numLocalVars; ++i)
    ivv[i] = thisroom.localvars[i];
  delete [] thisroom.localvars;
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

  if ((thisroom.resolution > 1) && (thisroom.object->GetWidth() < thisroom.width)) {
    // 640x400 room with 320x200-res walkbehind
    // resize it up to 640x400-res
    int oldw = thisroom.object->GetWidth(), oldh=thisroom.object->GetHeight();
    Common::Bitmap *tempb = Common::BitmapHelper::CreateBitmap(thisroom.width, thisroom.height, thisroom.object->GetColorDepth());
    tempb->Fill(0);
    tempb->StretchBlt(thisroom.object,RectWH(0,0,oldw,oldh),RectWH(0,0,tempb->GetWidth(),tempb->GetHeight()));
    delete thisroom.object; 
    thisroom.object = tempb;
  }

  set_palette_range(palette, 0, 255, 0);
  
  if ((thisroom.ebscene[0]->GetColorDepth () > 8) &&
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
  for (ww = 0; ww < thisroom.walls->GetWidth(); ww++) {
    for (int qq = 0; qq < thisroom.walls->GetHeight(); qq++) {
      thispix = thisroom.walls->GetPixel (ww, qq);
      if (thispix > MAX_WALK_AREAS)
        continue;
      if (thisroom.walk_area_top[thispix] > qq)
        thisroom.walk_area_top[thispix] = qq;
      if (thisroom.walk_area_bottom[thispix] < qq)
        thisroom.walk_area_bottom[thispix] = qq;
    }
  }

}

void save_room(const char *files, roomstruct rstruc) {
  int               f;
  long              xoff, tesl;
  Stream       *opty;
  room_file_header  rfh;

  if (rstruc.wasversion < kRoomVersion_Current)
    quit("save_room: can no longer save old format rooms");

  if (rstruc.wasversion < kRoomVersion_200_alpha) {
    for (f = 0; f < 11; f++)
      rstruc.password[f] -= 60;
  }
  else
    for (f = 0; f < 11; f++)
      rstruc.password[f] -= passwencstring[f];

  opty = ci_fopen(const_cast<char*>(files), Common::kFile_CreateAlways, Common::kFile_Write);
  if (opty == NULL)
    quit("save_room: unable to open room file for writing.");

  rfh.version = (RoomFileVersion)rstruc.wasversion; //ROOM_FILE_VERSION;
  rfh.WriteFromFile(opty);

  if (rfh.version >= 5) {
    opty->WriteByte(BLOCKTYPE_MAIN);
    opty->WriteInt32(0);
  }

  opty->WriteInt32(rstruc.bytes_per_pixel);  // colour depth bytes per pixel
  opty->WriteInt16(rstruc.numobj);
  opty->WriteArrayOfInt16(&rstruc.objyval[0], rstruc.numobj);

  opty->WriteInt32(rstruc.numhotspots);
  opty->WriteArray(&rstruc.hswalkto[0], sizeof(_Point), rstruc.numhotspots);
  for (f = 0; f < rstruc.numhotspots; f++)
    Common::StrUtil::WriteString(rstruc.hotspotnames[f], opty);

  // TODO: checking version here makes little sense now because save_room does not 
  // properly support export to lower data version
  if (rfh.version >= 24)
    for (f = 0; f < rstruc.numhotspots; f++)
      Common::StrUtil::WriteString(rstruc.hotspotScriptNames[f], opty);

  opty->WriteInt32(rstruc.numwalkareas);
  opty->WriteArray(&rstruc.wallpoints[0], sizeof(PolyPoints), rstruc.numwalkareas);

  opty->WriteInt16(rstruc.top);
  opty->WriteInt16(rstruc.bottom);
  opty->WriteInt16(rstruc.left);
  opty->WriteInt16(rstruc.right);
  opty->WriteInt16(rstruc.numsprs);
  opty->WriteArray(&rstruc.sprs[0], sizeof(sprstruc), rstruc.numsprs);

  opty->WriteInt32 (rstruc.numLocalVars);
  for (int i = 0; i < rstruc.numLocalVars; ++i)
    rstruc.localvars[i].Write(opty);
/*
  for (f = 0; f < rstruc.numhotspots; f++)
    serialize_new_interaction (rstruc.intrHotspot[f]);
  for (f = 0; f < rstruc.numsprs; f++)
    serialize_new_interaction (rstruc.intrObject[f]);
  serialize_new_interaction (rstruc.intrRoom);
*/
  opty->WriteInt32 (MAX_REGIONS);
  /*
  for (f = 0; f < MAX_REGIONS; f++)
    serialize_new_interaction (rstruc.intrRegion[f]);
	*/
  serialize_room_interactions(opty);

  opty->WriteArrayOfInt32(&rstruc.objbaseline[0], rstruc.numsprs);
  opty->WriteInt16(rstruc.width);
  opty->WriteInt16(rstruc.height);

  if (rfh.version >= 23)
    opty->WriteArrayOfInt16(&rstruc.objectFlags[0], rstruc.numsprs);

  if (rfh.version >= 11)
    opty->WriteInt16(rstruc.resolution);

  // write the zoom and light levels
  opty->WriteInt32 (MAX_WALK_AREAS + 1);
  opty->WriteArrayOfInt16(&rstruc.walk_area_zoom[0], MAX_WALK_AREAS + 1);
  opty->WriteArrayOfInt16(&rstruc.walk_area_light[0], MAX_WALK_AREAS + 1);
  opty->WriteArrayOfInt16(&rstruc.walk_area_zoom2[0], MAX_WALK_AREAS + 1);
  opty->WriteArrayOfInt16(&rstruc.walk_area_top[0], MAX_WALK_AREAS + 1);
  opty->WriteArrayOfInt16(&rstruc.walk_area_bottom[0], MAX_WALK_AREAS + 1);

  opty->Write(&rstruc.password[0], 11);
  opty->Write(&rstruc.options[0], 10);
  opty->WriteInt16(rstruc.nummes);

  if (rfh.version >= 25)
    opty->WriteInt32(rstruc.gameId);
 
  if (rfh.version >= 3)
    opty->WriteArray(&rstruc.msgi[0], sizeof(MessageInfo), rstruc.nummes);

  for (f = 0; f < rstruc.nummes; f++)
    write_string_encrypt(opty, rstruc.message[f]);
//    fputstring(rstruc.message[f]);

  if (rfh.version >= 6) {
    // we no longer use animations, remove them
    rstruc.numanims = 0;
    opty->WriteInt16(rstruc.numanims);

    if (rstruc.numanims > 0)
      opty->WriteArray(&rstruc.anims[0], sizeof(FullAnimation), rstruc.numanims);
  }

  if ((rfh.version >= 4) && (rfh.version < 16)) {
    save_script_configuration(opty);
    save_graphical_scripts(opty, &rstruc);
  }

  if (rfh.version >= 8)
    opty->WriteArrayOfInt16(&rstruc.shadinginfo[0], 16);

  if (rfh.version >= 21) {
    opty->WriteArrayOfInt16(&rstruc.regionLightLevel[0], MAX_REGIONS);
    opty->WriteArrayOfInt32(&rstruc.regionTintLevel[0], MAX_REGIONS);
  }

  xoff = opty->GetPosition();
  delete opty;

  tesl = save_lzw((char*)files, rstruc.ebscene[0], rstruc.pal, xoff);

  tesl = savecompressed_allegro((char*)files, rstruc.regions, rstruc.pal, tesl);
  tesl = savecompressed_allegro((char*)files, rstruc.walls, rstruc.pal, tesl);
  tesl = savecompressed_allegro((char*)files, rstruc.object, rstruc.pal, tesl);
  tesl = savecompressed_allegro((char*)files, rstruc.lookat, rstruc.pal, tesl);

  if (rfh.version >= 5) {
    long  lee;

    opty = ci_fopen(files, Common::kFile_Open, Common::kFile_ReadWrite);
    lee = opty->GetLength()-7;

    opty->Seek(3, Common::kSeekBegin);
    opty->WriteInt32(lee);
    opty->Seek(0, Common::kSeekEnd);

    if (rstruc.scripts != NULL) {
      int hh;

      opty->WriteByte(BLOCKTYPE_SCRIPT);
      lee = (int)strlen(rstruc.scripts) + 4;
      opty->WriteInt32(lee);
      lee -= 4;

      for (hh = 0; hh < lee; hh++)
        rstruc.scripts[hh]-=passwencstring[hh % 11];

      opty->WriteInt32(lee);
      opty->Write(rstruc.scripts, lee);

      for (hh = 0; hh < lee; hh++)
        rstruc.scripts[hh]+=passwencstring[hh % 11];

    }
   
    if (rstruc.compiled_script != NULL) {
      long  leeat, wasat;

      opty->WriteByte(BLOCKTYPE_COMPSCRIPT3);
      lee = 0;
      leeat = opty->GetPosition();
      opty->WriteInt32(lee);
      rstruc.compiled_script->Write(opty);
     
      wasat = opty->GetPosition();
      opty->Seek(leeat, Common::kSeekBegin);
      lee = (wasat - leeat) - 4;
      opty->WriteInt32(lee);
      opty->Seek(0, Common::kSeekEnd);
    }

    if (rstruc.numsprs > 0) {
      // TODO: need generic algorithm to write block sizes back after their contents are written
      long  leeat, wasat;

      opty->WriteByte(BLOCKTYPE_OBJECTNAMES);
      lee = 0;
      leeat = opty->GetPosition();
      opty->WriteInt32(lee);

      opty->WriteByte(rstruc.numsprs);
      for (int i = 0; i < rstruc.numsprs; ++i)
        Common::StrUtil::WriteString(rstruc.objectnames[i], opty);

      wasat = opty->GetPosition();
      opty->Seek(leeat, Common::kSeekBegin);
      lee = (wasat - leeat) - sizeof(int32_t);
      opty->WriteInt32(lee);
      opty->Seek(0, Common::kSeekEnd);


      opty->WriteByte(BLOCKTYPE_OBJECTSCRIPTNAMES);
      lee = 0;
      leeat = opty->GetPosition();
      opty->WriteInt32(lee);

      opty->WriteByte(rstruc.numsprs);
      for (int i = 0; i < rstruc.numsprs; ++i)
        Common::StrUtil::WriteString(rstruc.objectscriptnames[i], opty);

      wasat = opty->GetPosition();
      opty->Seek(leeat, Common::kSeekBegin);
      lee = (wasat - leeat) - sizeof(int32_t);
      opty->WriteInt32(lee);
      opty->Seek(0, Common::kSeekEnd);
    }

    long lenpos, lenis;
    int gg;

    if (rstruc.num_bscenes > 1) {
      long  curoffs;

      opty->WriteByte(BLOCKTYPE_ANIMBKGRND);
      lenpos = opty->GetPosition();
      lenis = 0;
      opty->WriteInt32(lenis);
      opty->WriteByte(rstruc.num_bscenes);
      opty->WriteByte(rstruc.bscene_anim_speed);
      
      opty->WriteArrayOfInt8 ((int8_t*)&rstruc.ebpalShared[0], rstruc.num_bscenes);

      delete opty;

      curoffs = lenpos + 6 + rstruc.num_bscenes;
      for (gg = 1; gg < rstruc.num_bscenes; gg++)
        curoffs = save_lzw((char*)files, rstruc.ebscene[gg], rstruc.bpalettes[gg], curoffs);

      opty = ci_fopen(const_cast<char*>(files), Common::kFile_Open, Common::kFile_ReadWrite);
      lenis = (curoffs - lenpos) - 4;
      opty->Seek(lenpos, Common::kSeekBegin);
      opty->WriteInt32(lenis);
      opty->Seek(0, Common::kSeekEnd);
    }

    // Write custom properties
    opty->WriteByte (BLOCKTYPE_PROPERTIES);
    lenpos = opty->GetPosition();
    lenis = 0;
    opty->WriteInt32(lenis);
    opty->WriteInt32 (1);  // Version 1 of properties block
    AGSProps::WriteValues(rstruc.roomProps, opty);
    for (gg = 0; gg < rstruc.numhotspots; gg++)
      AGSProps::WriteValues(rstruc.hsProps[gg], opty);
    for (gg = 0; gg < rstruc.numsprs; gg++)
      AGSProps::WriteValues(rstruc.objProps[gg], opty);

    lenis = (opty->GetPosition() - lenpos) - 4;
    opty->Seek(lenpos, Common::kSeekBegin);
    opty->WriteInt32(lenis);
    opty->Seek(0, Common::kSeekEnd);


    // Write EOF block
    opty->WriteByte(BLOCKTYPE_EOF);
    delete opty;
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
  thisroom.wasversion=kRoomVersion_Current;
  copy_room_palette_to_global_palette();
  
  thisroom.password[0] = 0;

  calculate_walkable_areas();

  thisroom.bytes_per_pixel = thisroom.ebscene[0]->GetBPP();
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
#include "util/multifilelib.h"

namespace MFLUtil = AGS::Common::MFLUtil;

#define MAX_FILES 10000
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

void fwrite_data_enc(const void *data, int dataSize, int dataCount, Stream *ooo, int &rand_val)
{
  const unsigned char *dataChar = (const unsigned char*)data;
  for (int i = 0; i < dataSize * dataCount; i++)
  {
    ooo->WriteByte(dataChar[i] + MFLUtil::GetNextPseudoRand(rand_val));
  }
}

void fputstring_enc(const char *sss, Stream *ooo, int &rand_val) 
{
  fwrite_data_enc(sss, 1, strlen(sss) + 1, ooo, rand_val);
}

void putw_enc(int numberToWrite, Stream *ooo, int &rand_val)
{
  fwrite_data_enc(&numberToWrite, 4, 1, ooo, rand_val);
}

void write_clib_header(Stream*wout) {
  int ff;
  int randSeed = (int)time(NULL);
  wout->WriteInt32(randSeed - MFLUtil::EncryptionRandSeed);
  putw_enc(ourlib.num_data_files, wout, randSeed);
  for (ff = 0; ff < ourlib.num_data_files; ff++)
  {
    fputstring_enc(ourlib.data_filenames[ff], wout, randSeed);
  }
  putw_enc(ourlib.num_files, wout, randSeed);
  for (ff = 0; ff < ourlib.num_files; ff++) 
  {
    fputstring_enc(ourlib.filenames[ff], wout, randSeed);
  }
  fwrite_data_enc(&ourlib.offset[0],4,ourlib.num_files, wout, randSeed);
  fwrite_data_enc(&ourlib.length[0],4,ourlib.num_files, wout, randSeed);
  fwrite_data_enc(&ourlib.file_datafile[0],1,ourlib.num_files, wout, randSeed);
}


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

// TODO: upgrade this "old style data file" to "new style data file" (would require engine update too)
void make_old_style_data_file(const AGSString &dataFileName, const std::vector<AGSString> &filenames)
{
    const int passwmod = 20;
    std::vector<int32_t> filesizes(filenames.size());
    std::vector<AGSString> writefname(filenames.size());
    size_t numfile = filenames.size();

    for (size_t fi = 0; fi < numfile; ++fi)
    {
        writefname[fi] = get_filename(filenames[fi]);

        if (strlen(writefname[fi]) > 12)
            ThrowManagedException(AGSString::FromFormat("Filename too long (limit is 12 chars): '%s'", writefname[fi].GetCStr()));
        int filesize = Common::File::GetFileSize(filenames[fi]);
        if (filesize < 0)
            ThrowManagedException(AGSString::FromFormat("Unable to retrieve file size: '%s'", writefname[fi].GetCStr()));
        filesizes[fi] = filesize;

        for (size_t c = 0; c < writefname[fi].GetLength(); ++c)
            writefname[fi].SetAt(c, writefname[fi][c] + passwmod);
    }

    // Write the header
    Stream *wout = Common::File::CreateFile(dataFileName);
    if (!wout)
        ThrowManagedException(AGSString::FromFormat("Failed to open data file for writing: '%s'", dataFileName.GetCStr()));
    wout->Write(MFLUtil::HeadSig, MFLUtil::HeadSig.GetLength());
    wout->WriteByte(6);  // version
    wout->WriteByte(passwmod);  // password modifier
    wout->WriteByte(0);  // reserved
    wout->WriteInt16((int16_t)numfile);
    for (int i = 0; i < 13; ++i) wout->WriteByte(0);  // the password
    for (size_t i = 0; i < numfile; ++i)
        writefname[i].WriteCount(wout, 13);
    for (size_t i = 0; i < numfile; ++i)
        wout->WriteInt32(filesizes[i]);
    wout->WriteByteCount(0, 2 * numfile);  // comp.ratio

    // now copy the data
    AGSString err_msg;
    for (size_t i = 0; i < numfile; ++i)
    {
        Stream *in = Common::File::OpenFileRead(filenames[i]);
        if (!in)
        {
            err_msg.Format("Unable to open asset file for reading: '%s'", filenames[i].GetCStr());
            break;
        }
        int res = copy_file_across(in, wout, filesizes[i]);
        delete in;
        if (res < 1)
        {
            err_msg.Format("Error writing asset into package, possibly disk full: '%s'", filenames[i].GetCStr());
            break;
        }
    }
    delete wout;

    if (!err_msg.IsEmpty())
    {
        unlink(dataFileName);
        ThrowManagedException(err_msg);
    }
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
    wout->Write(MFLUtil::HeadSig, MFLUtil::HeadSig.GetLength());
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
        wout->Write(MFLUtil::TailSig, MFLUtil::TailSig.GetLength());
	}
    delete wout;
  }

  wout = Common::File::OpenFile(firstDataFileFullPath, Common::kFile_Open, Common::kFile_ReadWrite);
  wout->Seek(mainHeaderOffset, Common::kSeekBegin);
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

	if (((thisgame.spriteflags[sprNum] & SPF_640x400) == 0) &&
		thisgame.IsHiRes())
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

	if ((thisgame.spriteflags[sprNum] & SPF_ALPHACHANNEL) != 0)
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

void SetGameResolution(Game ^game)
{
    // For backwards compatibility, save letterbox-by-design games as having non-custom resolution
    thisgame.options[OPT_LETTERBOX] = game->Settings->LetterboxMode;
    if (game->Settings->LetterboxMode)
        thisgame.SetDefaultResolution((GameResolutionType)game->Settings->LegacyLetterboxResolution);
    else
        thisgame.SetCustomResolution(::Size(game->Settings->CustomResolution.Width, game->Settings->CustomResolution.Height));
}

void GameUpdated(Game ^game) {
  // TODO: this function may get called when only one item is added/removed or edited;
  // probably it would be best to split it up into several callbacks at some point.
  thisgame.color_depth = (int)game->Settings->ColorDepth;
  SetGameResolution(game);

  thisgame.options[OPT_NOSCALEFNT] = game->Settings->FontsForHiRes;
  thisgame.options[OPT_ANTIALIASFONTS] = game->Settings->AntiAliasFonts;
  antiAliasFonts = thisgame.options[OPT_ANTIALIASFONTS];
  update_font_sizes();

  //delete abuf;
  //abuf = Common::BitmapHelper::CreateBitmap(32, 32, thisgame.color_depth * 8);
  BaseColorDepth = thisgame.color_depth * 8;

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

  // Reload native fonts and update font information in the managed component
  thisgame.numfonts = game->Fonts->Count;
  for (int i = 0; i < thisgame.numfonts; i++) 
  {
	  thisgame.fontflags[i] &= ~FFLG_SIZEMASK;
	  thisgame.fontflags[i] |= game->Fonts[i]->PointSize;
      thisgame.fontvoffset[i] = game->Fonts[i]->VerticalOffset;
      thisgame.fontlnspace[i] = game->Fonts[i]->LineSpacing;
	  reload_font(i);
	  game->Fonts[i]->Height = getfontheight(i);
  }
}

void GameFontUpdated(Game ^game, int fontNumber)
{
    thisgame.fontvoffset[fontNumber] = game->Fonts[fontNumber]->VerticalOffset;
    thisgame.fontlnspace[fontNumber] = game->Fonts[fontNumber]->LineSpacing;
    FontInfo fi;
    make_fontinfo(thisgame, fontNumber, fi);
    set_fontinfo(fontNumber, fi);
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
		Common::Bitmap *spriteAtRightDepth = Common::BitmapHelper::CreateBitmap(tempsprite->GetWidth(), tempsprite->GetHeight(), thisgame.color_depth * 8);
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
	roomstruct *theRoom = (roomstruct*)(void*)room->_roomStructPtr;
	delete theRoom->ebscene[backgroundNumber];
	theRoom->ebscene[backgroundNumber] = NULL;
	
	theRoom->num_bscenes--;
	room->BackgroundCount--;
	for (int i = backgroundNumber; i < theRoom->num_bscenes; i++) 
	{
		theRoom->ebscene[i] = theRoom->ebscene[i + 1];
		theRoom->ebpalShared[i] = theRoom->ebpalShared[i + 1];
	}
}

void ImportBackground(Room ^room, int backgroundNumber, System::Drawing::Bitmap ^bmp, bool useExactPalette, bool sharePalette) 
{
	color oldpale[256];
	Common::Bitmap *newbg = CreateBlockFromBitmap(bmp, oldpale, true, false, NULL);
	roomstruct *theRoom = (roomstruct*)(void*)room->_roomStructPtr;
	theRoom->width = room->Width;
	theRoom->height = room->Height;
	bool resolutionChanged = (theRoom->resolution != (int)room->Resolution);
	theRoom->resolution = (int)room->Resolution;

	if (newbg->GetColorDepth() == 8) 
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
		delete theRoom->ebscene[backgroundNumber];
	}
	theRoom->ebscene[backgroundNumber] = newbg;

  // if size or resolution has changed, reset masks
	if ((newbg->GetWidth() != theRoom->object->GetWidth()) || (newbg->GetHeight() != theRoom->object->GetHeight()) ||
      (theRoom->width != theRoom->object->GetWidth()) || (resolutionChanged))
	{
		delete theRoom->walls;
		delete theRoom->lookat;
		delete theRoom->object;
		delete theRoom->regions;
		theRoom->walls = Common::BitmapHelper::CreateBitmap(theRoom->width / theRoom->resolution, theRoom->height / theRoom->resolution,8);
		theRoom->lookat = Common::BitmapHelper::CreateBitmap(theRoom->width / theRoom->resolution, theRoom->height / theRoom->resolution,8);
		theRoom->object = Common::BitmapHelper::CreateBitmap(theRoom->width, theRoom->height,8);
		theRoom->regions = Common::BitmapHelper::CreateBitmap(theRoom->width / theRoom->resolution, theRoom->height / theRoom->resolution,8);
		theRoom->walls->Clear();
		theRoom->lookat->Clear();
		theRoom->object->Clear();
		theRoom->regions->Clear();
	}

	room->BackgroundCount = theRoom->num_bscenes;
	room->ColorDepth = theRoom->ebscene[0]->GetColorDepth();
}

void import_area_mask(void *roomptr, int maskType, System::Drawing::Bitmap ^bmp)
{
	color oldpale[256];
	Common::Bitmap *importedImage = CreateBlockFromBitmap(bmp, oldpale, false, false, NULL);
	Common::Bitmap *mask = get_bitmap_for_mask((roomstruct*)roomptr, (RoomAreaMask)maskType);

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

	validate_mask(mask, "imported", (maskType == Hotspots) ? MAX_HOTSPOTS : (MAX_WALK_AREAS + 1));
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

	if ((remapColours) || (thisgame.color_depth > 1)) 
	{
		sort_out_transparency(tempsprite, spriteImportMethod, imgPalBuf, useRoomBackgroundColours, importedColourDepth);
	}

	thisgame.spriteflags[slot] = 0;
	if (thisgame.IsHiRes())
	{
		thisgame.spriteflags[slot] |= SPF_640x400;
	}
	if (alphaChannel)
	{
		thisgame.spriteflags[slot] |= SPF_ALPHACHANNEL;

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

	return thisgame.IsHiRes() ? 1 : 0;
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
  return ConvertBlockToBitmap(todraw, (thisgame.spriteflags[spriteNum] & SPF_ALPHACHANNEL) != 0);
}

System::Drawing::Bitmap^ getSpriteAsBitmap32bit(int spriteNum, int width, int height) {
  Common::Bitmap *todraw = get_sprite(spriteNum);
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
	gui->OnClickHandler = ConvertStringToNativeString(normalGui->OnClick);
	gui->X = normalGui->Left;
	gui->Y = normalGui->Top;
	gui->Width = normalGui->Width;
	gui->Height = normalGui->Height;
    gui->Flags = (normalGui->Clickable) ? 0 : Common::kGUIMain_NoClick;
    gui->PopupAtMouseY = normalGui->PopupYPos;
    gui->PopupStyle = (Common::GUIPopupStyle)normalGui->Visibility;
    gui->ZOrder = normalGui->ZOrder;
    gui->FgColor = normalGui->BorderColor;
    gui->SetTransparencyAsPercentage(normalGui->Transparency);
  }
  else
  {
    TextWindowGUI^ twGui = dynamic_cast<TextWindowGUI^>(guiObj);
	gui->Width = twGui->EditorWidth;
	gui->Height = twGui->EditorHeight;
    gui->Flags = Common::kGUIMain_TextWindow;
    gui->PopupStyle = Common::kGUIPopupModal;
	gui->Padding = twGui->Padding;
    gui->FgColor = twGui->TextColor;
  }
  gui->BgColor = guiObj->BackgroundColor;
  gui->BgImage = guiObj->BackgroundImage;
  
  gui->Name = ConvertStringToNativeString(guiObj->Name);

  gui->ControlCount = 0;
  gui->CtrlRefs.resize(guiObj->Controls->Count);
  gui->Controls.resize(guiObj->Controls->Count);

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
          guibuts.push_back(::GUIButton());
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
		  guibuts[numguibuts].eventHandlers[0] = ConvertStringToNativeString(button->OnClick);
		  
          gui->CtrlRefs[gui->ControlCount] = (Common::kGUIButton << 16) | numguibuts;
		  gui->Controls[gui->ControlCount] = &guibuts[numguibuts];
		  gui->ControlCount++;
		  numguibuts++;
	  }
	  else if (label)
	  {
          guilabels.push_back(::GUILabel());
		  guilabels[numguilabels].textcol = label->TextColor;
		  guilabels[numguilabels].font = label->Font;
		  guilabels[numguilabels].align = (int)label->TextAlignment;
		  guilabels[numguilabels].flags = 0;
		  char textBuffer[MAX_GUILABEL_TEXT_LEN];
		  ConvertStringToCharArray(label->Text, textBuffer, MAX_GUILABEL_TEXT_LEN);
		  guilabels[numguilabels].SetText(textBuffer);

		  gui->CtrlRefs[gui->ControlCount] = (Common::kGUILabel << 16) | numguilabels;
		  gui->Controls[gui->ControlCount] = &guilabels[numguilabels];
		  gui->ControlCount++;
		  numguilabels++;
	  }
	  else if (textbox)
	  {
          guitext.push_back(::GUITextBox());
		  guitext[numguitext].textcol = textbox->TextColor;
		  guitext[numguitext].font = textbox->Font;
		  guitext[numguitext].flags = 0;
		  guitext[numguitext].exflags = (textbox->ShowBorder) ? 0 : GTF_NOBORDER;
		  guitext[numguitext].text[0] = 0;
		  guitext[numguitext].eventHandlers[0] = ConvertStringToNativeString(textbox->OnActivate);

		  gui->CtrlRefs[gui->ControlCount] = (Common::kGUITextBox << 16) | numguitext;
		  gui->Controls[gui->ControlCount] = &guitext[numguitext];
		  gui->ControlCount++;
		  numguitext++;
	  }
	  else if (listbox)
	  {
          guilist.push_back(::GUIListBox());
		  guilist[numguilist].textcol = listbox->TextColor;
		  guilist[numguilist].font = listbox->Font;
		  guilist[numguilist].backcol = listbox->SelectedTextColor;
		  guilist[numguilist].selectedbgcol = listbox->SelectedBackgroundColor;
		  guilist[numguilist].alignment = (int)listbox->TextAlignment;
          guilist[numguilist].flags = listbox->Translated ? GUIF_TRANSLATED : 0;
		  guilist[numguilist].exflags = (listbox->ShowBorder) ? 0 : GLF_NOBORDER;
		  guilist[numguilist].exflags |= (listbox->ShowScrollArrows) ? 0 : GLF_NOARROWS;
		  guilist[numguilist].eventHandlers[0] = ConvertStringToNativeString(listbox->OnSelectionChanged);

		  gui->CtrlRefs[gui->ControlCount] = (Common::kGUIListBox << 16) | numguilist;
		  gui->Controls[gui->ControlCount] = &guilist[numguilist];
		  gui->ControlCount++;
		  numguilist++;
	  }
	  else if (slider)
	  {
          guislider.push_back(::GUISlider());
		  guislider[numguislider].min = slider->MinValue;
		  guislider[numguislider].max = slider->MaxValue;
		  guislider[numguislider].value = slider->Value;
		  guislider[numguislider].handlepic = slider->HandleImage;
		  guislider[numguislider].handleoffset = slider->HandleOffset;
		  guislider[numguislider].bgimage = slider->BackgroundImage;
		  guislider[numguislider].eventHandlers[0] = ConvertStringToNativeString(slider->OnChange);

		  gui->CtrlRefs[gui->ControlCount] = (Common::kGUISlider << 16) | numguislider;
		  gui->Controls[gui->ControlCount] = &guislider[numguislider];
		  gui->ControlCount++;
		  numguislider++;
	  }
	  else if (invwindow)
	  {
          guiinv.push_back(::GUIInv());
		  guiinv[numguiinv].charId = invwindow->CharacterID;
		  guiinv[numguiinv].itemWidth = invwindow->ItemWidth;
		  guiinv[numguiinv].itemHeight = invwindow->ItemHeight;

		  gui->CtrlRefs[gui->ControlCount] = (Common::kGUIInvWindow << 16) | numguiinv;
		  gui->Controls[gui->ControlCount] = &guiinv[numguiinv];
		  gui->ControlCount++;
		  numguiinv++;
	  }
	  else if (textwindowedge)
	  {
          guibuts.push_back(::GUIButton());
		  guibuts[numguibuts].pic = textwindowedge->Image;
		  guibuts[numguibuts].usepic = guibuts[numguibuts].pic;
		  guibuts[numguibuts].flags = 0;
		  guibuts[numguibuts].text[0] = 0;
		  
		  gui->CtrlRefs[gui->ControlCount] = (Common::kGUIButton << 16) | numguibuts;
		  gui->Controls[gui->ControlCount] = &guibuts[numguibuts];
		  gui->ControlCount++;
		  numguibuts++;
	  }

	  GUIObject *newObj = gui->Controls[gui->ControlCount - 1];
	  newObj->x = control->Left;
	  newObj->y = control->Top;
	  newObj->wid = control->Width;
	  newObj->hit = control->Height;
	  newObj->objn = control->ID;
	  newObj->zorder = control->ZOrder;
	  newObj->scriptName = ConvertStringToNativeString(control->Name);
  }

  gui->RebuildArray();
  gui->ResortZOrder();
}

void drawGUI(int hdc, int x,int y, GUI^ guiObj, int scaleFactor, int selectedControl) {
  numguibuts = 0;
  numguilabels = 0;
  numguitext = 0;
  numguilist = 0;
  numguislider = 0;
  numguiinv = 0;
  guibuts.resize(0);
  guilabels.resize(0);
  guitext.resize(0);
  guilist.resize(0);
  guislider.resize(0);
  guiinv.resize(0);

  ConvertGUIToBinaryFormat(guiObj, &tempgui);

  tempgui.HighlightCtrl = selectedControl;

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
			sprites->Add(i, gcnew Sprite(i, spr->GetWidth(), spr->GetHeight(), spr->GetColorDepth(), (thisgame.spriteflags[i] & SPF_640x400) ? SpriteImportResolution::HighRes : SpriteImportResolution::LowRes, (thisgame.spriteflags[i] & SPF_ALPHACHANNEL) ? true : false));
		}
	}

	return sprites;
}

void ConvertCustomProperties(AGS::Types::CustomProperties ^insertInto, AGS::Common::StringIMap *propToConvert)
{
    for (AGS::Common::StringIMap::const_iterator it = propToConvert->begin();
         it != propToConvert->end(); ++it)
	{
		CustomProperty ^newProp = gcnew CustomProperty();
		newProp->Name = gcnew String(it->first);
		newProp->Value = gcnew String(it->second);
		insertInto->PropertyValues->Add(newProp->Name, newProp);
	}
}

void CompileCustomProperties(AGS::Types::CustomProperties ^convertFrom, AGS::Common::StringIMap *compileInto)
{
	compileInto->clear();
	for each (String ^key in convertFrom->PropertyValues->Keys)
	{
        AGS::Common::String name, value;
		name = ConvertStringToNativeString(convertFrom->PropertyValues[key]->Name);
		value = ConvertStringToNativeString(convertFrom->PropertyValues[key]->Value);
		(*compileInto)[name] = value;
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

void ConvertInteractionToScript(System::Text::StringBuilder ^sb, InteractionCommand *intrcmd, String^ scriptFuncPrefix, AGS::Types::Game ^game, int *runScriptCount, bool *onlyIfInvWasUseds, int commandOffset) 
{
  if (intrcmd->Type != 1)
  {
    // if another type of interaction, we definately can't optimise
    // away the wrapper function
    runScriptCount[0] = 1000;
  }
  else
  {
    runScriptCount[0]++;
  }

  if (intrcmd->Type != 20)
  {
	  *onlyIfInvWasUseds = false;
  }

	switch (intrcmd->Type)
	{
	case 0:
		break;
	case 1:  // Run Script
		sb->Append(scriptFuncPrefix);
		sb->Append(System::Convert::ToChar(intrcmd->Data[0].Value + 'a'));
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
		String ^scriptCode = gcnew String(actions[intrcmd->Type].textscript);
		if ((*onlyIfInvWasUseds) && (commandOffset > 0))
		{
			scriptCode = String::Concat("else ", scriptCode);
		}
		scriptCode = scriptCode->Replace("$$1", (gcnew Int32(intrcmd->Data[0].Value))->ToString() );
		scriptCode = scriptCode->Replace("$$2", (gcnew Int32(intrcmd->Data[1].Value))->ToString() );
		scriptCode = scriptCode->Replace("$$3", (gcnew Int32(intrcmd->Data[2].Value))->ToString() );
		scriptCode = scriptCode->Replace("$$4", (gcnew Int32(intrcmd->Data[3].Value))->ToString() );
		sb->AppendLine(scriptCode);
		}
		break;
	case 34: // animate character
		{
		char scriptCode[100];
		int charID = intrcmd->Data[0].Value;
		int loop = intrcmd->Data[1].Value;
		int speed = intrcmd->Data[2].Value;
		sprintf(scriptCode, "%s.Animate(%d, %d, eOnce, eBlock);", GetCharacterScriptName(charID, game), loop, speed);
		sb->AppendLine(gcnew String(scriptCode));
		}
		break;
	case 35: // quick animation
		{
		char scriptCode[300];
		int charID = intrcmd->Data[0].Value;
		int view = intrcmd->Data[1].Value;
		int loop = intrcmd->Data[2].Value;
		int speed = intrcmd->Data[3].Value;
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
		int objID = intrcmd->Data[0].Value;
		int x = intrcmd->Data[1].Value;
		int y = intrcmd->Data[2].Value;
		int speed = intrcmd->Data[3].Value;
		sprintf(scriptCode, "object[%d].Move(%d, %d, %d, %s);", objID, x, y, speed, (intrcmd->Data[4].Value) ? "eBlock" : "eNoBlock");
		sb->AppendLine(gcnew String(scriptCode));
		}
		break;
	case 19: // Move Character
		{
		char scriptCode[100];
		int charID = intrcmd->Data[0].Value;
		int x = intrcmd->Data[1].Value;
		int y = intrcmd->Data[2].Value;
		sprintf(scriptCode, "%s.Walk(%d, %d, %s);", GetCharacterScriptName(charID, game), x, y, (intrcmd->Data[3].Value) ? "eBlock" : "eNoBlock");
		sb->AppendLine(gcnew String(scriptCode));
		}
		break;
	case 18: // Animate Object
		{
		char scriptCode[100];
		int objID = intrcmd->Data[0].Value;
		int loop = intrcmd->Data[1].Value;
		int speed = intrcmd->Data[2].Value;
		sprintf(scriptCode, "object[%d].Animate(%d, %d, %s, eNoBlock);", objID, loop, speed, (intrcmd->Data[3].Value) ? "eRepeat" : "eOnce");
		sb->AppendLine(gcnew String(scriptCode));
		}
		break;
	case 23: // IF variable set to value
		{
		char scriptCode[100];
		int valueToCheck = intrcmd->Data[1].Value;
		if ((game == nullptr) || (intrcmd->Data[0].Value >= game->OldInteractionVariables->Count))
		{
			sprintf(scriptCode, "if (__INTRVAL$%d$ == %d) {", intrcmd->Data[0].Value, valueToCheck);
		}
		else
		{
			OldInteractionVariable^ variableToCheck = game->OldInteractionVariables[intrcmd->Data[0].Value];
			sprintf(scriptCode, "if (%s == %d) {", variableToCheck->ScriptName, valueToCheck);
		}
		sb->AppendLine(gcnew String(scriptCode));
		break;
		}
	case 33: // Set variable
		{
		char scriptCode[100];
		int valueToCheck = intrcmd->Data[1].Value;
		if ((game == nullptr) || (intrcmd->Data[0].Value >= game->OldInteractionVariables->Count))
		{
			sprintf(scriptCode, "__INTRVAL$%d$ = %d;", intrcmd->Data[0].Value, valueToCheck);
		}
		else
		{
			OldInteractionVariable^ variableToCheck = game->OldInteractionVariables[intrcmd->Data[0].Value];
			sprintf(scriptCode, "%s = %d;", variableToCheck->ScriptName, valueToCheck);
		}
		sb->AppendLine(gcnew String(scriptCode));
		break;
		}
	case 12: // Change Room
		{
		char scriptCode[200];
		int room = intrcmd->Data[0].Value;
		sprintf(scriptCode, "player.ChangeRoomAutoPosition(%d", room);
		if (intrcmd->Data[1].Value > 0) 
		{
			sprintf(&scriptCode[strlen(scriptCode)], ", %d", intrcmd->Data[1].Value);
		}
		strcat(scriptCode, ");");
		sb->AppendLine(gcnew String(scriptCode));
		}
		break;
	case 2: // Add Score On First Execution
		{
		  int points = intrcmd->Data[0].Value;
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

void ConvertInteractionCommandList(System::Text::StringBuilder^ sb, InteractionCommandList *cmdList, String^ scriptFuncPrefix, AGS::Types::Game^ game, int *runScriptCount, int targetTypeForUnhandledEvent) 
{
	bool onlyIfInvWasUseds = true;

    for (size_t cmd = 0; cmd < cmdList->Cmds.size(); cmd++)
	{
		ConvertInteractionToScript(sb, &cmdList->Cmds[cmd], scriptFuncPrefix, game, runScriptCount, &onlyIfInvWasUseds, cmd);
		if (cmdList->Cmds[cmd].Children.get() != NULL) 
		{
			ConvertInteractionCommandList(sb, cmdList->Cmds[cmd].Children.get(), scriptFuncPrefix, game, runScriptCount, targetTypeForUnhandledEvent);
			sb->AppendLine("}");
		}
	}

	if ((onlyIfInvWasUseds) && (targetTypeForUnhandledEvent > 0) && 
		(cmdList->Cmds.size() > 0))
	{
		sb->AppendLine("else {");
		sb->AppendLine(String::Format(" unhandled_event({0}, 3);", targetTypeForUnhandledEvent));
		sb->AppendLine("}");
	}
}

void CopyInteractions(AGS::Types::Interactions ^destination, ::InteractionScripts *source)
{
    if (source->ScriptFuncNames.size() > (size_t)destination->ScriptFunctionNames->Length) 
	{
		throw gcnew AGS::Types::AGSEditorException("Invalid interaction funcs: too many interaction events");
	}

	for (size_t i = 0; i < source->ScriptFuncNames.size(); i++) 
	{
		destination->ScriptFunctionNames[i] = gcnew String(source->ScriptFuncNames[i]);
	}
}

void ConvertInteractions(AGS::Types::Interactions ^interactions, Interaction *intr, String^ scriptFuncPrefix, AGS::Types::Game ^game, int targetTypeForUnhandledEvent)
{
	if (intr->Events.size() > (size_t)interactions->ScriptFunctionNames->Length) 
	{
		throw gcnew AGS::Types::AGSEditorException("Invalid interaction data: too many interaction events");
	}

	for (size_t i = 0; i < intr->Events.size(); i++) 
	{
        if (intr->Events[i].Response.get() != NULL) 
		{
      int runScriptCount = 0;
			System::Text::StringBuilder^ sb = gcnew System::Text::StringBuilder();
			ConvertInteractionCommandList(sb, intr->Events[i].Response.get(), scriptFuncPrefix, game, &runScriptCount, targetTypeForUnhandledEvent);
      if (runScriptCount == 1)
      {
        sb->Append("$$SINGLE_RUN_SCRIPT$$");
      }
			interactions->ImportedScripts[i] = sb->ToString();
		}
	}
}

// Load compiled game's main data file and use it to create AGS::Types::Game.
// TODO: originally this function was meant import strictly 2.72 games;
// although technically it can now load game files of any version, more work is
// required to properly fill in editor's Game object's fields depending on
// which version is being loaded.
Game^ import_compiled_game_dta(const char *fileName)
{
	const char *errorMsg = load_dta_file_into_thisgame(fileName);
    loaded_game_file_version = kGameVersion_Current;
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
	game->Settings->UseGlobalSpeechAnimationDelay = true; // this was always on in pre-3.0 games
	game->Settings->GUIAlphaStyle = GUIAlphaStyle::Classic;
    game->Settings->SpriteAlphaStyle = SpriteAlphaStyle::Classic;
	game->Settings->HandleInvClicksInScript = (thisgame.options[OPT_HANDLEINVCLICKS] != 0);
	game->Settings->InventoryCursors = !thisgame.options[OPT_FIXEDINVCURSOR];
	game->Settings->LeftToRightPrecedence = (thisgame.options[OPT_LEFTTORIGHTEVAL] != 0);
	game->Settings->LetterboxMode = (thisgame.options[OPT_LETTERBOX] != 0);
	game->Settings->MaximumScore = thisgame.totalscore;
	game->Settings->MouseWheelEnabled = (thisgame.options[OPT_MOUSEWHEEL] != 0);
    game->Settings->NumberDialogOptions = (thisgame.options[OPT_DIALOGNUMBERED] != 0) ? DialogOptionsNumbering::Normal : DialogOptionsNumbering::KeyShortcutsOnly;
	game->Settings->PixelPerfect = (thisgame.options[OPT_PIXPERFECT] != 0);
	game->Settings->PlaySoundOnScore = thisgame.options[OPT_SCORESOUND];
	game->Settings->Resolution = (GameResolutions)thisgame.GetDefaultResolution();
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
  game->Settings->RenderAtScreenResolution = (RenderAtScreenResolution)thisgame.options[OPT_RENDERATSCREENRES];

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
		cli::array<System::Byte> ^pluginData = gcnew cli::array<System::Byte>(thisgamePlugins[i].DataLen);
		const char *data_ptr = thisgamePlugins[i].Data.get();
		for (size_t j = 0; j < thisgamePlugins[i].DataLen; j++) 
		{
			pluginData[j] = data_ptr[j];
		}
		
		AGS::Types::Plugin ^plugin = gcnew AGS::Types::Plugin(gcnew String(thisgamePlugins[i].Name), pluginData);
		game->Plugins->Add(plugin);
	}

	for (i = 0; i < numGlobalVars; i++)
	{
		OldInteractionVariable ^intVar;
		intVar = gcnew OldInteractionVariable(gcnew String(globalvars[i].Name), globalvars[i].Value);
		game->OldInteractionVariables->Add(intVar);
	}
	
    AGS::Types::IViewFolder ^viewFolder = AGS::Types::FolderHelper::CreateDefaultViewFolder();
	for (i = 0; i < thisgame.numviews; i++) 
	{
		AGS::Types::View ^view = gcnew AGS::Types::View();
		view->Name = gcnew String(thisgame.viewNames[i]);
		view->ID = i + 1;

		for (int j = 0; j < newViews[i].numLoops; j++) 
		{
			ViewLoop^ newLoop = gcnew ViewLoop();
			newLoop->ID = j;
			newLoop->RunNextLoop = newViews[i].loops[j].flags & LOOPFLAG_RUNNEXTLOOP;

			for (int k = 0; k < newViews[i].loops[j].numFrames; k++) 
			{
				AGS::Types::ViewFrame^ newFrame = gcnew AGS::Types::ViewFrame();
				newFrame->ID = k;
				newFrame->Flipped = (newViews[i].loops[j].frames[k].flags & VFLG_FLIPSPRITE);
				newFrame->Image = newViews[i].loops[j].frames[k].pic;
				newFrame->Sound = newViews[i].loops[j].frames[k].sound;
				newFrame->Delay = newViews[i].loops[j].frames[k].speed;
				newLoop->Frames->Add(newFrame);
			}
			
			view->Loops->Add(newLoop);
		}

		viewFolder->Views->Add(view);
	}
    AGS::Types::FolderHelper::SetRootViewFolder(game, viewFolder);

	for (i = 0; i < thisgame.numcharacters; i++) 
	{
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
		character->ScriptName = gcnew String(thisgame.chars[i].scrname);
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

    for (AGS::Common::PropertySchema::const_iterator it = thisgame.propSchema.begin();
         it != thisgame.propSchema.end(); ++it)
	{
		CustomPropertySchemaItem ^schemaItem = gcnew CustomPropertySchemaItem();
		schemaItem->Name = gcnew String(it->second.Name);
		schemaItem->Description = gcnew String(it->second.Description);
		schemaItem->DefaultValue = gcnew String(it->second.DefaultValue);
		schemaItem->Type = (AGS::Types::CustomPropertyType)it->second.Type;

		game->PropertySchema->PropertyDefinitions->Add(schemaItem);
	}

	for (i = 0; i < thisgame.numgui; i++)
	{
		guis[i].RebuildArray();
	    guis[i].ResortZOrder();

		GUI^ newGui;
		if (guis[i].IsTextWindow()) 
		{
			newGui = gcnew TextWindowGUI();
			newGui->Controls->Clear();  // we'll add our own edges
      ((TextWindowGUI^)newGui)->TextColor = guis[i].FgColor;
		}
		else 
		{
			newGui = gcnew NormalGUI();
			((NormalGUI^)newGui)->Clickable = ((guis[i].Flags & Common::kGUIMain_NoClick) == 0);
			((NormalGUI^)newGui)->Top = guis[i].Y;
			((NormalGUI^)newGui)->Left = guis[i].X;
			((NormalGUI^)newGui)->Width = (guis[i].Width > 0) ? guis[i].Width : 1;
			((NormalGUI^)newGui)->Height = (guis[i].Height > 0) ? guis[i].Height : 1;
			((NormalGUI^)newGui)->PopupYPos = guis[i].PopupAtMouseY;
			((NormalGUI^)newGui)->Visibility = (GUIVisibility)guis[i].PopupStyle;
			((NormalGUI^)newGui)->ZOrder = guis[i].ZOrder;
			((NormalGUI^)newGui)->OnClick = gcnew String(guis[i].OnClickHandler);
      ((NormalGUI^)newGui)->BorderColor = guis[i].FgColor;
		}
		newGui->BackgroundColor = guis[i].BgColor;
		newGui->BackgroundImage = guis[i].BgImage;
		newGui->ID = i;
		newGui->Name = gcnew String(guis[i].Name);

		for (int j = 0; j < guis[i].ControlCount; j++)
		{
			GUIObject* curObj = guis[i].Controls[j];
			GUIControl ^newControl = nullptr;
			switch (guis[i].CtrlRefs[j] >> 16)
			{
			case Common::kGUIButton:
				{
				if (guis[i].IsTextWindow())
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
			case Common::kGUILabel:
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
			case Common::kGUITextBox:
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
			case Common::kGUIListBox:
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
                  newListbox->Translated = (copyFrom->flags & GUIF_TRANSLATED) != 0;
				  newListbox->OnSelectionChanged = gcnew String(copyFrom->eventHandlers[0]);
				  break;
				}
			case Common::kGUISlider:
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
			case Common::kGUIInvWindow:
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
				throw gcnew AGSEditorException("Unknown control type found: " + (guis[i].CtrlRefs[j] >> 16));
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
    AGSString roomFileName = ConvertFileNameToNativeString(fileName);

	Stream *opty = Common::AssetManager::OpenAsset(roomFileName);
	if (opty == NULL) throw gcnew AGSEditorException("Unable to open room file");

	short version = opty->ReadInt16();
	if (version < 17)
	{
    delete opty;
		throw gcnew AGSEditorException("Room file is from an old version of AGS and cannot be processed");
	}

	String ^scriptToReturn = nullptr;

	int thisblock = 0;
	while (thisblock != BLOCKTYPE_EOF) 
	{
		thisblock = opty->ReadByte();
		if (thisblock == BLOCKTYPE_EOF) 
		{
			break;
		}

		int blockLen = opty->ReadInt32();

		if (thisblock == BLOCKTYPE_SCRIPT) 
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
			opty->Seek(blockLen);
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
    AGSString roomFileName = ConvertFileNameToNativeString(roomToLoad->FileName);

	const char *errorMsg = load_room_file(roomFileName);
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
    room->MusicVolumeAdjustment = (AGS::Types::RoomVolumeAdjustment)thisroom.options[ST_VOLUME];
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
	room->ColorDepth = thisroom.ebscene[0]->GetColorDepth();
	room->BackgroundAnimationDelay = thisroom.bscene_anim_speed;
	room->BackgroundCount = thisroom.num_bscenes;

	int i;
	for (i = 0; i < thisroom.numLocalVars; i++)
	{
		OldInteractionVariable ^intVar;
		intVar = gcnew OldInteractionVariable(gcnew String(thisroom.localvars[i].Name), thisroom.localvars[i].Value);
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
			if (thisroom.wasversion < kRoomVersion_300a)
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
    if (thisroom.wasversion <= kRoomVersion_300a)
      obj->StartY += GetSpriteHeight(thisroom.sprs[i].sprnum);
		obj->Visible = (thisroom.sprs[i].on != 0);
		obj->Clickable = ((thisroom.objectFlags[i] & OBJF_NOINTERACT) == 0);
		obj->Locked = ((thisroom.objectFlags[i] & OBJF_LOCKED) != 0);
		obj->Baseline = thisroom.objbaseline[i];
		obj->Name = gcnew String(jibbledScriptName);
		obj->Description = gcnew String(thisroom.objectnames[i]);
		obj->UseRoomAreaScaling = ((thisroom.objectFlags[i] & OBJF_USEROOMSCALING) != 0);
		obj->UseRoomAreaLighting = ((thisroom.objectFlags[i] & OBJF_USEREGIONTINTS) != 0);
		ConvertCustomProperties(obj->Properties, &thisroom.objProps[i]);

		if (thisroom.wasversion < kRoomVersion_300a)
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
        hotspot->WalkToPoint = System::Drawing::Point(thisroom.hswalkto[i].x, thisroom.hswalkto[i].y);
		ConvertCustomProperties(hotspot->Properties, &thisroom.hsProps[i]);

		if (thisroom.wasversion < kRoomVersion_300a)
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
		// NOTE: Region's light level value exposed in editor is always 100 units higher,
		// for compatibility with older versions of the editor.
		// TODO: probably we could remove this behavior? Need to consider possible compat mode
		area->LightLevel = thisroom.get_region_lightlevel(i) + 100;
		area->UseColourTint = thisroom.has_region_tint(i);
		area->BlueTint = (thisroom.regionTintLevel[i] >> 16) & 0x00ff;
		area->GreenTint = (thisroom.regionTintLevel[i] >> 8) & 0x00ff;
		area->RedTint = thisroom.regionTintLevel[i] & 0x00ff;
		// Set saturation's and luminance's default values in the editor if it is disabled in room data
		int saturation = (thisroom.regionTintLevel[i] >> 24) & 0xFF;
		area->TintSaturation = (saturation > 0 && area->UseColourTint) ? saturation :
			Utilities::GetDefaultValue(area->GetType(), "TintSaturation", 0);
		int luminance = thisroom.get_region_tintluminance(i);
		area->TintLuminance = area->UseColourTint ? luminance :
			Utilities::GetDefaultValue(area->GetType(), "TintLuminance", 0);

		if (thisroom.wasversion < kRoomVersion_300a)
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

	if (thisroom.wasversion < kRoomVersion_300a)
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
    thisroom.freemessage();
    thisroom.freescripts();

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

	thisroom.nummes = room->Messages->Count;
	for (int i = 0; i < thisroom.nummes; i++) 
	{
		RoomMessage ^newMessage = room->Messages[i];
		thisroom.message[i] = (char*)malloc(newMessage->Text->Length + 1);
		ConvertStringToCharArray(newMessage->Text, thisroom.message[i], newMessage->Text->Length + 1);
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
	for (int i = 0; i < thisroom.numsprs; i++) 
	{
		RoomObject ^obj = room->Objects[i];
		thisroom.objectscriptnames[i] = ConvertStringToNativeString(obj->Name);

		thisroom.sprs[i].sprnum = obj->Image;
		thisroom.sprs[i].x = obj->StartX;
		thisroom.sprs[i].y = obj->StartY;
		thisroom.sprs[i].on = obj->Visible;
		thisroom.objbaseline[i] = obj->Baseline;
		thisroom.objectnames[i] = ConvertStringToNativeString(obj->Description);
		thisroom.objectFlags[i] = 0;
		if (obj->UseRoomAreaScaling) thisroom.objectFlags[i] |= OBJF_USEROOMSCALING;
		if (obj->UseRoomAreaLighting) thisroom.objectFlags[i] |= OBJF_USEREGIONTINTS;
		if (!obj->Clickable) thisroom.objectFlags[i] |= OBJF_NOINTERACT;
		if (obj->Locked) thisroom.objectFlags[i] |= OBJF_LOCKED;
		CompileCustomProperties(obj->Properties, &thisroom.objProps[i]);
	}

	thisroom.numhotspots = room->Hotspots->Count;
	for (int i = 0; i < thisroom.numhotspots; i++) 
	{
		RoomHotspot ^hotspot = room->Hotspots[i];
		thisroom.hotspotnames[i] = ConvertStringToNativeString(hotspot->Description);
		thisroom.hotspotScriptNames[i] = ConvertStringToNativeString(hotspot->Name);
		thisroom.hswalkto[i].x = hotspot->WalkToPoint.X;
		thisroom.hswalkto[i].y = hotspot->WalkToPoint.Y;
		CompileCustomProperties(hotspot->Properties, &thisroom.hsProps[i]);
	}

	for (int i = 0; i <= MAX_WALK_AREAS; i++) 
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

	for (int i = 0; i < MAX_OBJ; i++) 
	{
		RoomWalkBehind ^area = room->WalkBehinds[i];
		thisroom.objyval[i] = area->Baseline;
	}

	for (int i = 0; i < MAX_REGIONS; i++) 
	{
		RoomRegion ^area = room->Regions[i];
		thisroom.regionTintLevel[i] = 0;
		if (area->UseColourTint) 
		{
            thisroom.regionTintLevel[i]  = area->RedTint | (area->GreenTint << 8) | (area->BlueTint << 16) | (area->TintSaturation << 24);
            thisroom.regionLightLevel[i] = (area->TintLuminance * 25) / 10;
		}
		else 
		{
            thisroom.regionTintLevel[i] = 0;
			// NOTE: Region's light level value exposed in editor is always 100 units higher,
			// for compatibility with older versions of the editor.
			thisroom.regionLightLevel[i] = area->LightLevel - 100;
		}
	}

	CompileCustomProperties(room->Properties, &thisroom.roomProps);

	thisroom.scripts = NULL;
	thisroom.compiled_script = ((AGS::Native::CompiledScript^)room->Script->CompiledData)->Data;

	AGSString roomFileName = ConvertFileNameToNativeString(room->FileName);

	TempDataStorage::RoomBeingSaved = room;

	save_room_file(roomFileName);

	TempDataStorage::RoomBeingSaved = nullptr;

	for (int i = 0; i < thisroom.numhotspots; i++) 
	{
		thisroom.hotspotnames[i].Free(); // TODO: not sure if makes sense here
	}
}

void serialize_interaction_scripts(Interactions ^interactions, Stream *ooo)
{
	ooo->WriteInt32(interactions->ScriptFunctionNames->Length);
	for each (String^ funcName in interactions->ScriptFunctionNames)
	{
		if (funcName == nullptr)
		{
			ooo->WriteByte(0);
		}
		else 
		{
			AGSString fname = ConvertStringToNativeString(funcName);
			fname.Write(ooo);
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

void save_script_configuration(Stream*iii) {
  // no variable names
  iii->WriteInt32 (1);
  iii->WriteInt32 (0);
}

void load_script_configuration(Stream*iii) { int aa;
  if (getlong(iii)!=1) quit("ScriptEdit: invliad config version");
  int numvarnames=getlong(iii);
  for (aa=0;aa<numvarnames;aa++) {
    int lenoft=iii->ReadByte();
    iii->Seek(lenoft);
  }
}

void save_graphical_scripts(Stream*fff,roomstruct*rss) {
  // no script
  fff->WriteInt32 (-1);
}

char*scripttempn="~acsc%d.tmp";
void load_graphical_scripts(Stream*iii,roomstruct*rst) {
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
    iii->Seek (lee);
  }
}

void update_polled_stuff_if_runtime()
{
	// do nothing
}
