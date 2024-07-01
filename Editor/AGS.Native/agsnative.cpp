//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include <stdio.h>
void ThrowManagedException(const char *message);
#pragma unmanaged
#pragma warning (disable: 4996 4312)  // disable deprecation warnings
extern "C" bool Scintilla_RegisterClasses(void *hInstance);

#include "agsnative.h"
#include <allegro.h>
#include <winalleg.h>
#undef CreateFile
#undef DeleteFile
#include "util/wgt2allg.h"
#include "ac/actiontype.h"
#include "ac/common.h"
#include "ac/dialogtopic.h"
#include "ac/gamesetupstruct.h"
#include "ac/scriptmodule.h"
#include "ac/spritecache.h"
#include "ac/view.h"
#include "font/agsfontrenderer.h"
#include "font/fonts.h"
#include "game/main_game_file.h"
#include "game/plugininfo.h"
#include "game/room_file.h"
#include "game/roomstruct.h"
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
#include "util/directory.h"
#include "util/filestream.h"
#include "util/path.h"
#include "gfx/bitmap.h"
#include "core/assetmanager.h"
#include "NativeUtils.h"

using AGS::Types::AGSEditorException;

using AGS::Common::Stream;
using AGS::Common::AssetLibInfo;
using AGS::Common::AssetManager;
namespace AGSProps = AGS::Common::Properties;
namespace BitmapHelper = AGS::Common::BitmapHelper;
using AGS::Common::GUIMain;
using AGS::Common::GUIButton;
using AGS::Common::GUIInvWindow;
using AGS::Common::GUILabel;
using AGS::Common::GUIListBox;
using AGS::Common::GUISlider;
using AGS::Common::GUITextBox;
using AGS::Common::RoomStruct;
using AGS::Common::PInteractionScripts;
using AGS::Common::Interaction;
using AGS::Common::InteractionCommand;
using AGS::Common::InteractionCommandList;
typedef AGS::Common::String AGSString;
namespace AGSDirectory = AGS::Common::Directory;
namespace AGSFile = AGS::Common::File;
namespace AGSPath = AGS::Common::Path;

typedef System::Drawing::Bitmap SysBitmap;
typedef AGS::Common::Bitmap AGSBitmap;
typedef AGS::Common::PBitmap PBitmap;

typedef AGS::Common::Error AGSError;
typedef AGS::Common::HError HAGSError;

void save_room_file(RoomStruct &rs, const AGSString &path);

AGSBitmap *initialize_sprite(AGS::Common::sprkey_t, AGSBitmap*, uint32_t &sprite_flags);
void pre_save_sprite(AGSBitmap*);


std::unique_ptr<AssetManager> AssetMgr;
int mousex = 0, mousey = 0;
int antiAliasFonts = 0;
int dsc_want_hires = 0;
bool enable_greyed_out_masks = true;
RGB*palette = NULL;
GameSetupStruct thisgame;
AGS::Common::SpriteCache::Callbacks spritecallbacks = {
    nullptr,
    initialize_sprite,
    nullptr,
    pre_save_sprite
};
AGS::Common::SpriteCache spriteset(thisgame.SpriteInfos, spritecallbacks);
GUIMain tempgui; // for drawing a GUI preview
const char *sprsetname = "acsprset.spr";
const char *sprindexname = "sprindex.dat";
const char *old_editor_data_file = "editor.dat";
const char *new_editor_data_file = "game.agf";
const char *old_editor_main_game_file = "ac2game.dta";
const char *TEMPLATE_LOCK_FILE = "template.dta";
const char *ROOM_TEMPLATE_ID_FILE = "rtemplate.dat";
const int ROOM_TEMPLATE_ID_FILE_SIGNATURE = 0x74673812;
bool spritesModified = false;
RoomStruct thisroom;

GameDataVersion loaded_game_file_version = kGameVersion_Current;
AGS::Common::Version game_compiled_version;

// stuff for importing old games
int numScriptModules = 0;
std::vector<ScriptModule> scModules;
std::vector<DialogTopic> dialog;
std::vector<AGSString> dlgscript;
std::vector<GUIMain> guis;
std::vector<GUIButton> guibuts;
std::vector<GUIInvWindow> guiinv;
std::vector<GUILabel> guilabels;
std::vector<GUIListBox> guilist;
std::vector<GUISlider> guislider;
std::vector<GUITextBox> guitext;
std::vector<ViewStruct> newViews;

// A reference color depth, for correct color selection;
// originally was defined by 'abuf' bitmap.
int BaseColorDepth = 0;


struct NativeRoomTools
{
    bool roomModified = false;
    int loaded_room_number = -1;
    std::unique_ptr<AGSBitmap> drawBuffer;
    std::unique_ptr<AGSBitmap> undoBuffer;
    std::unique_ptr<AGSBitmap> roomBkgBuffer;
};
std::unique_ptr<NativeRoomTools> RoomTools;


HAGSError reset_sprite_file();
HAGSError reset_sprite_file(const AGSString &spritefile, const AGSString &indexfile);
bool reload_font(int curFont);
void drawBlockScaledAt(int hdc, Common::Bitmap *todraw ,int x, int y, float scaleFactor);
// this is to shut up the linker, it's used by CSRUN.CPP
void write_log(const char *) { }
SysBitmap^ ConvertBlockToBitmap(Common::Bitmap *todraw, bool useAlphaChannel);

int data_to_game_coord(int coord)
{
	return coord * thisgame.GetDataUpscaleMult();
}

int get_fixed_pixel_size(int coord)
{
	return coord * thisgame.GetDataUpscaleMult();
}

// jibbles the sprite around to fix hi-color problems, by swapping
// the red and blue elements
// TODO: find out if this is actually needed. I guess this may have something to do
// with Allegro keeping 16-bit bitmaps in an unusual format internally.
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

AGSBitmap *initialize_sprite(AGS::Common::sprkey_t /*index*/, AGSBitmap *image, uint32_t &/*sprite_flags*/)
{
    fix_block(image);
    return image;
}

void pre_save_sprite(AGSBitmap *image)
{
    // NOTE: this is a nasty hack:
    // because Editor expects slightly different RGB order, it swaps colors
    // when loading them (call to initialize_sprite), so here we basically
    // unfix that fix to save the data in a way that engine will expect.
    // TODO: perhaps adjust the editor to NOT need this?!
    fix_block(image);
}

// Safely gets a sprite, if the sprite does not exist then returns a placeholder (sprite 0)
Common::Bitmap *get_sprite (int spnr) {
  if ((spnr < 0) || !spriteset[spnr])
    return spriteset[0]; // return a placeholder
  return spriteset[spnr];
}

void SetNewSprite(int slot, AGSBitmap *sprit, int flags) {
  spriteset.SetSprite(slot, std::unique_ptr<AGSBitmap>(sprit), flags);
  spritesModified = true;
}

void deleteSprite (int sprslot) {
  spriteset.DisposeSprite(sprslot);
  spritesModified = true;
}

int GetSpriteAsHBitmap(int slot) {
  // FIXME later
  return (int)convert_bitmap_to_hbitmap(get_sprite(slot)->GetAllegroBitmap());
}

bool DoesSpriteExist(int slot) {
    return spriteset.DoesSpriteExist(slot);
}

int GetMaxSprites() {
	return MAX_STATIC_SPRITES;
}

int GetSpriteWidth(int slot) {
    assert((slot >= 0) && ((size_t)slot < thisgame.SpriteInfos.size()));
    if (slot < 0 || (size_t)slot >= thisgame.SpriteInfos.size())
        return 0;
    return thisgame.SpriteInfos[slot].Width;
}

int GetSpriteHeight(int slot) {
    assert((slot >= 0) && ((size_t)slot < thisgame.SpriteInfos.size()));
    if (slot < 0 || (size_t)slot >= thisgame.SpriteInfos.size())
        return 0;
    return thisgame.SpriteInfos[slot].Height;
}

void GetSpriteInfo(int slot, ::SpriteInfo &info) {
    assert((slot >= 0) && ((size_t)slot < thisgame.SpriteInfos.size()));
    if (slot < 0 || (size_t)slot >= thisgame.SpriteInfos.size())
        return;
    info.Width = thisgame.SpriteInfos[slot].Width;
    info.Height = thisgame.SpriteInfos[slot].Height;
    info.Flags = thisgame.SpriteInfos[slot].Flags;
}

int GetSpriteColorDepth(int slot) {
  return get_sprite(slot)->GetColorDepth();
}

int GetPaletteAsHPalette() {
  return (int)convert_palette_to_hpalette(palette);
}

int find_free_sprite_slot() {
  return spriteset.GetFreeIndex();
}

void update_sprite_resolution(int spriteNum, bool isVarRes, bool isHighRes)
{
	thisgame.SpriteInfos[spriteNum].Flags &= ~(SPF_HIRES | SPF_VAR_RESOLUTION);
    if (isVarRes)
    {
        thisgame.SpriteInfos[spriteNum].Flags |= SPF_VAR_RESOLUTION;
        if (isHighRes)
            thisgame.SpriteInfos[spriteNum].Flags |= SPF_HIRES;
    }
}

void change_sprite_number(int oldNumber, int newNumber) {

  if (!spriteset.DoesSpriteExist(oldNumber))
    return;

  std::unique_ptr<AGSBitmap> bitmap(spriteset.RemoveSprite(oldNumber));
  spriteset.SetSprite(newNumber, std::move(bitmap), thisgame.SpriteInfos[oldNumber].Flags);
  spritesModified = true;
}

int crop_sprite_edges(const std::vector<int> &sprites, bool symmetric, Rect *crop_rect) {
  // this function has passed in a list of sprites, all the
  // same size, to crop to the size of the smallest
  int xx, yy;
  int width = spriteset[sprites[0]]->GetWidth();
  int height = spriteset[sprites[0]]->GetHeight();
  int left = width, right = 0;
  int top = height, bottom = 0;

  for (const int sprnum : sprites) {
    AGSBitmap *sprit = get_sprite(sprnum);
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
  if (crop_rect)
    *crop_rect = Rect(left, top, right, bottom);

  if ((newWidth == width) && (newHeight == height))
  {
    // no change in size
    return 0;
  }

  if ((newWidth < 1) || (newHeight < 1))
  {
	  // completely transparent sprite, don't attempt to crop
	  return 0;
  }

  for (const int sprnum : sprites) {
    AGSBitmap *sprit = get_sprite(sprnum);
    // create a new, smaller sprite and copy across
	std::unique_ptr<AGSBitmap> newsprit(new AGSBitmap(newWidth, newHeight, sprit->GetColorDepth()));
    newsprit->Blit(sprit, left, top, 0, 0, newWidth, newHeight);
    // set new image and keep old flags
    spriteset.SetSprite(sprnum, std::move(newsprit), thisgame.SpriteInfos[sprnum].Flags);
  }

  spritesModified = true;

  return 1;
}

HAGSError extract_template_files_impl(const AGSString &templateFileName, const std::vector<AGSString> &check_list,
    const AGSString &check_list_error,
    const std::vector<AGSString> &exclude_list, const AGSString &asset_output_format)
{
    const AssetLibInfo *lib = nullptr;
    std::unique_ptr<AssetManager> templateMgr(new AssetManager());
    auto err = templateMgr->AddLibrary(templateFileName, &lib);
    if (err != Common::kAssetNoError) 
        return new AGSError("Failed to read the template file.", Common::GetAssetErrorText(err));

    // If check_list is provided, then the package must include at least one of the files
    if (!check_list.empty())
    {
        bool at_least_one_check = false;
        for (auto check = check_list.begin();
            !at_least_one_check && (check < check_list.end()); ++check)
        {
            at_least_one_check |= templateMgr->DoesAssetExist(*check);
        }
        if (!at_least_one_check)
            return new AGSError(check_list_error);
    }

    for (const auto &asset : lib->AssetInfos)
    {
        const AGSString thisFile = asset.FileName;
        if (thisFile.IsEmpty())
            continue; // should not normally happen

        // Don't extract excluded files
        if (std::find_if(exclude_list.begin(), exclude_list.end(), AGS::Common::StrEqNoCasePred(thisFile))
                != exclude_list.end())
            continue;

        std::unique_ptr<Stream> readin(templateMgr->OpenAsset(thisFile));
        if (!readin)
            return new AGSError(AGSString::FromFormat("Failed to open template asset '%s' for reading.", thisFile.GetCStr()));

        AGSString outputName = thisFile;
        // If format pattern is provided, then rename the output file
        if (!asset_output_format.IsEmpty())
        {
            AGSString ext = AGSPath::GetFileExtension(thisFile);
            outputName = AGSString::FromFormat(asset_output_format.GetCStr(), ext.GetCStr());
        }

        // Make sure to create necessary subfolders
        AGSDirectory::CreateAllDirectories(".", AGSPath::GetDirectoryPath(outputName));
        std::unique_ptr<Stream> wrout(AGSFile::CreateFile(outputName));
        if (!wrout)
            return new AGSError(AGSString::FromFormat("Failed to open file '%s' for writing.", outputName));

        const soff_t src_len = readin->GetLength();
        soff_t result = AGS::Common::CopyStream(readin.get(), wrout.get(), src_len);
        if (result < src_len)
            return new AGSError(AGSString::FromFormat("Failed to extract file '%s'.", thisFile.GetCStr()));
    }

    return HAGSError::None();
}

HAGSError extract_room_template_files(const AGSString &templateFileName, int newRoomNumber)
{
  std::vector<AGSString> check_list = { ROOM_TEMPLATE_ID_FILE };
  return extract_template_files_impl(templateFileName, check_list,
      "Template file does not contain necessary metadata.",
      check_list, AGSString::FromFormat("room%d.%%s", newRoomNumber));
}

HAGSError extract_template_files(const AGSString &templateFileName)
{
  std::vector<AGSString> check_list = { old_editor_data_file, new_editor_data_file };
  std::vector<AGSString> exclude_list = { TEMPLATE_LOCK_FILE };
  return extract_template_files_impl(templateFileName, check_list,
      "Template file does not contain main project data.",
      exclude_list, {});
}

void extract_icon_from_template(AssetManager *templateMgr, char *iconName, char **iconDataBuffer, size_t *bufferSize)
{
  // make sure we get the icon from the file
  templateMgr->SetSearchPriority(Common::kAssetPriorityLib);
  auto inpu = templateMgr->OpenAsset (iconName);
  const size_t sizey = inpu->GetLength();
  if ((inpu != NULL) && (sizey > 0))
  {
    char *iconbuffer = (char*)malloc(sizey);
    inpu->Read (iconbuffer, sizey);
    *iconDataBuffer = iconbuffer;
    *bufferSize = sizey;
  }
  else
  {
    *iconDataBuffer = NULL;
    *bufferSize = 0u;
  }
}

int load_template_file(const AGSString &fileName, char **iconDataBuffer, size_t *iconDataSize, bool isRoomTemplate)
{
  const AssetLibInfo *lib = nullptr;
  std::unique_ptr<AssetManager> templateMgr(new AssetManager());
  if (templateMgr->AddLibrary(fileName, &lib) == Common::kAssetNoError)
  {
    if (isRoomTemplate)
    {
      if (templateMgr->DoesAssetExist(ROOM_TEMPLATE_ID_FILE))
      {
        auto inpu = templateMgr->OpenAsset(ROOM_TEMPLATE_ID_FILE);
        if (inpu->ReadInt32() != ROOM_TEMPLATE_ID_FILE_SIGNATURE)
        {
          return 0;
        }
        int roomNumber = inpu->ReadInt32();
        char iconName[MAX_PATH];
        sprintf(iconName, "room%d.ico", roomNumber);
        if (templateMgr->DoesAssetExist(iconName))
        {
          extract_icon_from_template(templateMgr.get(), iconName, iconDataBuffer, iconDataSize);
        }
        return 1;
      }
      return 0;
    }
	  else if ((templateMgr->DoesAssetExist(old_editor_data_file)) || (templateMgr->DoesAssetExist(new_editor_data_file)))
	  {
      Common::String oriname = lib->BaseFileName;
      if ((oriname.FindString(".exe") != -1) ||
          (oriname.FindString(".dat") != -1) ||
          (oriname.FindString(".ags") != -1))
      {
        // wasn't originally meant as a template
	      return 0;
      }

	    auto inpu = templateMgr->OpenAsset(old_editor_main_game_file);
	    if (inpu != NULL) 
	    {
		    inpu->Seek(30);
		    int gameVersion = inpu->ReadInt32();
            // TODO: check this out, in theory we still support pre-2.72 game import
		    if (gameVersion != 32)
		    {
			    // older than 2.72 template
			    return 0;
		    }
	    }

      int useIcon = 0;
      char *iconName = "template.ico";
      if (!templateMgr->DoesAssetExist(iconName))
        iconName = "user.ico";
      // the file is a CLIB file, so let's extract the icon to display
      if (templateMgr->DoesAssetExist(iconName))
      {
        extract_icon_from_template(templateMgr.get(), iconName, iconDataBuffer, iconDataSize);
      }
      return 1;
    }
  }
  return 0;
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

// draw_gui_sprite is supported formally, without actual blending and other effects
// This is one ugly function... a "simplified" alternative of the engine's one;
// but with extra hacks, due to how sprites are stored while working in editor.
void draw_gui_sprite_impl(Common::Bitmap *g, int sprnum, Common::Bitmap *blptr, int atxp, int atyp)
{
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
  if ((sprnum >= 0) && thisgame.AllowRelativeRes() && thisgame.SpriteInfos[sprnum].IsRelativeRes()) {
    if (thisgame.SpriteInfos[sprnum].IsLegacyHiRes()) {
      if (dsc_want_hires == 0) {
        nwid/=2;
        nhit/=2;
      }
    }
    else if (dsc_want_hires) {
      nwid *= 2;
      nhit *= 2;
    }
  }
  wputblock_stretch(g, atxp,atyp,towrite,nwid,nhit);
  if (needtofree) delete towrite;
}

void draw_gui_sprite(Common::Bitmap *g, int sprnum, int atxp, int atyp, bool use_alpha, Common::BlendMode blend_mode)
{
    draw_gui_sprite_impl(g, sprnum, get_sprite(sprnum), atxp, atyp);
}

void draw_gui_sprite(Common::Bitmap *g, bool use_alpha, int atxp, int atyp,
    Common::Bitmap *blptr, bool src_has_alpha, Common::BlendMode blend_mode, int alpha)
{
    draw_gui_sprite_impl(g, -1, blptr, atxp, atyp);
}

void drawBlock (HDC hdc, Common::Bitmap *todraw, int x, int y) {
  set_palette_to_hdc (hdc, palette);
  // FIXME later
  blit_to_hdc (todraw->GetAllegroBitmap(), hdc, 0,0,x,y,todraw->GetWidth(),todraw->GetHeight());
}

void copy_walkable_to_regions (void *roomptr) {
    RoomStruct *theRoom = (RoomStruct*)roomptr;
	theRoom->RegionMask->Blit(theRoom->WalkAreaMask.get(), 0, 0, 0, 0, theRoom->RegionMask->GetWidth(), theRoom->RegionMask->GetHeight());
}

int get_mask_pixel(void *roomptr, int maskType, int x, int y)
{
    Common::Bitmap *mask = ((RoomStruct*)roomptr)->GetMask((RoomAreaMask)maskType);
    float scale = ((RoomStruct*)roomptr)->GetMaskScale((RoomAreaMask)maskType);
	return mask->GetPixel(x * scale, y * scale);
}

void draw_line_onto_mask(void *roomptr, int maskType, int x1, int y1, int x2, int y2, int color)
{
	Common::Bitmap *mask = ((RoomStruct*)roomptr)->GetMask((RoomAreaMask)maskType);
    float scale = ((RoomStruct*)roomptr)->GetMaskScale((RoomAreaMask)maskType);
	mask->DrawLine(Line(x1 * scale, y1 * scale, x2 * scale, y2 * scale), color);
}

void draw_filled_rect_onto_mask(void *roomptr, int maskType, int x1, int y1, int x2, int y2, int color)
{
	Common::Bitmap *mask = ((RoomStruct*)roomptr)->GetMask((RoomAreaMask)maskType);
    float scale = ((RoomStruct*)roomptr)->GetMaskScale((RoomAreaMask)maskType);
    mask->FillRect(Rect(x1 * scale, y1 * scale, x2 * scale, y2 * scale), color);
}

void draw_fill_onto_mask(void *roomptr, int maskType, int x1, int y1, int color)
{
	Common::Bitmap *mask = ((RoomStruct*)roomptr)->GetMask((RoomAreaMask)maskType);
    float scale = ((RoomStruct*)roomptr)->GetMaskScale((RoomAreaMask)maskType);
    mask->FloodFill(x1 * scale, y1 * scale, color);
}

void create_undo_buffer(void *roomptr, int maskType) 
{
	Common::Bitmap *mask = ((RoomStruct*)roomptr)->GetMask((RoomAreaMask)maskType);
    auto &undoBuffer = RoomTools->undoBuffer;
  if (undoBuffer != NULL)
  {
    if ((undoBuffer->GetWidth() != mask->GetWidth()) || (undoBuffer->GetHeight() != mask->GetHeight())) 
    {
      undoBuffer.reset();
    }
  }
  if (undoBuffer == NULL)
  {
    undoBuffer.reset(Common::BitmapHelper::CreateBitmap(mask->GetWidth(), mask->GetHeight(), mask->GetColorDepth()));
  }
  undoBuffer->Blit(mask, 0, 0, 0, 0, mask->GetWidth(), mask->GetHeight());
}

bool does_undo_buffer_exist()
{
  return (RoomTools->undoBuffer != NULL);
}

void clear_undo_buffer() 
{
  if (does_undo_buffer_exist()) 
  {
      RoomTools->undoBuffer.reset();
  }
}

void restore_from_undo_buffer(void *roomptr, int maskType)
{
  if (does_undo_buffer_exist())
  {
  	Common::Bitmap *mask = ((RoomStruct*)roomptr)->GetMask((RoomAreaMask)maskType);
    mask->Blit(RoomTools->undoBuffer.get(), 0, 0, 0, 0, mask->GetWidth(), mask->GetHeight());
  }
}

void setup_greyed_out_palette(int selCol) 
{
    RGB thisColourOnlyPal[256];

    // The code below makes it so that all the hotspot colours
    // except the selected one are greyed out. It doesn't work
    // in 256-colour games.

    // Blank out the temporary palette, and set a shade of grey
    // for all the hotspot colours
    for (int aa = 0; aa < 256; aa++) {
      int lumin = 0;
      if ((aa < MAX_ROOM_HOTSPOTS) && (aa > 0))
        lumin = ((MAX_ROOM_HOTSPOTS - aa) % 30) * 2;
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

void draw_area_mask(RoomStruct *roomptr, Common::Bitmap *ds, RoomAreaMask maskType, int selectedArea, int transparency) 
{
	Common::Bitmap *source = roomptr->GetMask(maskType);

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
        ds->StretchBlt(source, RectWH(0, 0, dest_width, dest_height));
	}
}

void draw_room_background(void *roomvoidptr, int hdc, int x, int y, int bgnum, float scaleFactor, int maskType, int selectedArea, int maskTransparency) 
{
	RoomStruct *roomptr = (RoomStruct*)roomvoidptr;

  if (bgnum < 0 || (size_t)bgnum >= roomptr->BgFrameCount)
    return;

  PBitmap srcBlock = roomptr->BgFrames[bgnum].Graphic;
  if (srcBlock == NULL)
    return;

    auto &drawBuffer = RoomTools->drawBuffer;
    auto &roomBkgBuffer = RoomTools->roomBkgBuffer;
	if (drawBuffer != NULL) 
	{
        if (!roomBkgBuffer || roomBkgBuffer->GetSize() != srcBlock->GetSize() || roomBkgBuffer->GetColorDepth() != srcBlock->GetColorDepth())
		    roomBkgBuffer.reset(new AGSBitmap(srcBlock->GetWidth(), srcBlock->GetHeight(), drawBuffer->GetColorDepth()));
    if (srcBlock->GetColorDepth() == 8)
    {
      select_palette(roomptr->BgFrames[bgnum].Palette);
    }

    roomBkgBuffer->Blit(srcBlock.get(), 0, 0, 0, 0, srcBlock->GetWidth(), srcBlock->GetHeight());

    if (srcBlock->GetColorDepth() == 8)
    {
      unselect_palette();
    }

	draw_area_mask(roomptr, roomBkgBuffer.get(), (RoomAreaMask)maskType, selectedArea, maskTransparency);

    int srcX = 0, srcY = 0;
    int srcWidth = srcBlock->GetWidth();
    int srcHeight = srcBlock->GetHeight();

    if (x < 0)
    {
      srcX = (int)(-x / scaleFactor);
      x += (int)(srcX * scaleFactor);
      srcWidth = drawBuffer->GetWidth() / scaleFactor;
      if (srcX + srcWidth > roomBkgBuffer->GetWidth())
      {
        srcWidth = roomBkgBuffer->GetWidth() - srcX;
      }
    }
    if (y < 0)
    {
      srcY = (int)(-y / scaleFactor);
      y += (int)(srcY * scaleFactor);
      srcHeight = drawBuffer->GetHeight() / scaleFactor;
      if (srcY + srcHeight > roomBkgBuffer->GetHeight())
      {
        srcHeight = roomBkgBuffer->GetHeight() - srcY;
      }
    }
        drawBuffer->StretchBlt(roomBkgBuffer.get(),
            RectWH(srcX, srcY, srcWidth, srcHeight),
            RectWH(x, y, (int)(srcWidth * scaleFactor), (int)(srcHeight * scaleFactor)));
	}
	else {
		drawBlockScaledAt(hdc, srcBlock.get(), x, y, scaleFactor);
	}
	
}

HAGSError import_sci_font(const AGSString &filename, int fslot)
{
    // Read SCI font into memory
    std::unique_ptr<Stream> in(AGSFile::OpenFileRead(filename));
    if (!in)
        return new AGSError("File not found");
    if (in->ReadByte() != 0x87) // format check
        return new AGSError("Not a valid SCI font file");
    in->Seek(3);
    const size_t char_count = in->ReadInt16(); // number of characters
    in->ReadInt16(); // line height (?)
    std::vector<int16_t> sci_offs(char_count); // table of contents
    in->ReadArrayOfInt16(&sci_offs.front(), char_count);
    const soff_t char_data_at = in->GetPosition();
    std::vector<uint8_t> widths(char_count); // char widths
    std::vector<uint8_t> heights(char_count); // char heights
    std::vector<uint8_t> pxbuf; // pixel data (stored in one line)
    for (size_t i = 0; i < char_count; ++i)
    {
        // TODO: find the SCI specs, why offsets have to be shifted by 2 bytes?
        soff_t off = sci_offs[i] + 2;
        if (off < char_data_at)
            return new AGSError("Invalid character offset found in SCI file");
        in->Seek(off, Common::kSeekBegin);
        int w = in->ReadInt8() - 1; // CHECKME: why has +1 in file?
        int h = in->ReadInt8();
        widths[i] = w;
        heights[i] = h;
        if ((w < 1) || (h < 1))
            continue; // character has zero size
        size_t row_size = (w / 8) + 1;
        size_t px_size = row_size * h;
        pxbuf.resize(pxbuf.size() + px_size);
        in->Read(&pxbuf[0] + (pxbuf.size() - px_size), px_size);
    }
    in.reset();

    // Write as WFN format on disk
    // TODO: merge this code with WFNFont class
    AGSString dst_fn = AGSString::FromFormat("agsfnt%d.wfn", fslot);
    std::unique_ptr<Stream> out(AGSFile::CreateFile(dst_fn));
    out->Write("WGT Font File  ", 15);
    out->WriteInt16(0);  // will be table address
    std::vector<uint16_t> wfn_offs(char_count);
    for (size_t i = 0, buf_off = 0; i < char_count; ++i)
    {
        wfn_offs[i] = out->GetPosition();
        int w = widths[i];
        int h = heights[i];
        out->WriteInt16(w + 1); // CHECKME: why has +1 in file?
        out->WriteInt16(h);
        if ((w < 1) || (h < 1))
            continue; // character has zero size
        size_t row_size = (w / 8) + 1;
        size_t px_size = row_size * h;
        out->Write(&pxbuf[0] + buf_off, px_size);
        buf_off += px_size;
    }
    // Seek back to the header, and write table position
    uint16_t tableat = static_cast<uint16_t>(out->GetPosition());
    out->WriteArrayOfInt16(reinterpret_cast<int16_t*>(&wfn_offs.front()), char_count);
    out->Seek(15, Common::kSeekBegin);
    out->WriteInt16(tableat);
    out.reset();

    // Load and register WFN font
    FontInfo fi;
    if (!load_font_size(fslot, fi))
        return new AGSError("Unable to load converted WFN file");
    return HAGSError::None();
}


int drawFontAt (int hdc, int fontnum, int x, int y, int width) {
  assert(fontnum < thisgame.numfonts);
  if (fontnum >= thisgame.numfonts)
  {
    return 0;
  }

  assert(width > 0);
  if (width <= 0)
    return 0;

  if (!is_font_loaded(fontnum))
    reload_font(fontnum);

  // TODO: rewrite this, use actual font size (maybe related to window size) and not game's resolution type
  int doubleSize = (!thisgame.IsLegacyHiRes()) ? 2 : 1;
  int blockSize = (!thisgame.IsLegacyHiRes()) ? 1 : 2;
  antiAliasFonts = thisgame.options[OPT_ANTIALIASFONTS];

  int char_height = thisgame.fonts[fontnum].Size * thisgame.fonts[fontnum].SizeMultiplier;
  int grid_size   = std::max(10, char_height);
  int grid_margin = std::max(4, grid_size / 4);
  grid_size += grid_margin * 2;
  grid_size *= blockSize;
  int first_char = 0;
  int num_chars  = 256;
  int padding = 5;

  if (doubleSize > 1)
      width /= 2;
  int chars_per_row = std::max(1, (width - (padding * 2)) / grid_size);
  int height = (num_chars / chars_per_row + 1) * grid_size + padding * 2;

  if (!hdc)
    return height * doubleSize;

  // we can't antialias font because changing col dep to 16 here causes
  // it to crash ... why?
  Common::Bitmap *tempblock = Common::BitmapHelper::CreateBitmap(width, height, 8);
  tempblock->Fill(0);
  color_t text_color = tempblock->GetCompatibleColor(15); // fixed white color
  int old_uformat = get_uformat();
  set_uformat(U_ASCII); // we won't be able to print 128-255 chars otherwise!
  for (int c = first_char; c < num_chars; ++c)
  {
    wgtprintf(tempblock,
                padding + (c % chars_per_row) * grid_size + grid_margin,
                padding + (c / chars_per_row) * grid_size + grid_margin,
                fontnum, text_color, "%c", c);
  }
  set_uformat(old_uformat);

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

  double ratioX = ((double) newwid) / ((double) twid);
  double ratioY = ((double) newhit )/ ((double) thit);
  double ratio = MIN(ratioX, ratioY);

  newsizx = (int)(twid * ratio);
  newsizy = (int)(thit * ratio);

  newx[0] = newsizx;
  newy[0] = newsizy;
}

static void doDrawViewLoop(int hdc, const std::vector<::ViewFrame> &frames,
    int x, int y, int size, const std::vector<int> &cursel)
{
    int wtoDraw = size * frames.size();
    if ((frames.size() > 0) && (frames.back().pic == -1))
        wtoDraw -= size;

    std::unique_ptr<AGSBitmap> todraw(Common::BitmapHelper::CreateTransparentBitmap(wtoDraw, size, thisgame.GetColorDepth()));
    std::unique_ptr<AGSBitmap> bmpbuf;

    int linecol = makecol_depth(thisgame.GetColorDepth(), 0, 64, 200);
    if (thisgame.GetColorDepth() == 8)
        linecol = 12;
  
    for (size_t i = 0; i < frames.size(); ++i)
    {
        // don't draw the Go-To-Next-Frame jibble
        if (frames[i].pic == -1)
            break;
        // work out the dimensions to stretch to
        int neww, newh;
        proportionalDraw (size, frames[i].pic, &neww, &newh);
        AGSBitmap *toblt = get_sprite(frames[i].pic);
        if (toblt->GetColorDepth () != todraw->GetColorDepth ())
        {
            // Different color depth? make a copy
            bmpbuf.reset(Common::BitmapHelper::CreateBitmap(toblt->GetWidth(), toblt->GetHeight(), todraw->GetColorDepth()));
            bmpbuf->Blit (toblt, 0, 0, 0, 0, toblt->GetWidth(), toblt->GetHeight());
            toblt = bmpbuf.get();
        }
        if (frames[i].flags & VFLG_FLIPSPRITE)
        {
            // Flipped bitmap? mirror the sprite
            AGSBitmap *flipped = Common::BitmapHelper::CreateBitmap (toblt->GetWidth(), toblt->GetHeight(), todraw->GetColorDepth ());
            flipped->Clear (flipped->GetMaskColor ());
            flipped->FlipBlt(toblt, 0, 0, Common::kFlip_Horizontal);
            bmpbuf.reset(flipped);
            toblt = flipped;
        }
        todraw->StretchBlt(toblt, RectWH(size*i, 0, neww, newh), Common::kBitmap_Transparency);
        bmpbuf.reset();
        if (i < frames.size() - 1)
        {
            // Draw dividing line
	        todraw->DrawLine (Line(size*(i+1) - 1, 0, size*(i+1) - 1, size-1), linecol);
        }
    }

    // Paint selection boxes over selected items
    linecol = makecol_depth(thisgame.GetColorDepth(), 255, 255,255);
    if (thisgame.GetColorDepth() == 8)
        linecol = 14;
    for (int sel : cursel)
    {
        todraw->DrawRect(Rect(size * sel, 0, size * (sel+1) - 1, size-1), linecol);
    }

    // Paint result on the final GDI surface
    drawBlock((HDC)hdc, todraw.get(), x, y);
}

// TODO: these functions duplicate ones in the engine, because game struct
// is referenced differently. Refactor this somehow.
int ctx_data_to_game_size(int val, bool hires_ctx)
{
    if (hires_ctx && !thisgame.IsLegacyHiRes())
        return std::max(1, (val / HIRES_COORD_MULTIPLIER));
    if (!hires_ctx && thisgame.IsLegacyHiRes())
        return val * HIRES_COORD_MULTIPLIER;
    return val;
}

int get_adjusted_spritewidth(int spr) {
  Common::Bitmap *tsp = get_sprite(spr);
  if (tsp == NULL)
      return 0;
  int retval = tsp->GetWidth();
  if (!thisgame.AllowRelativeRes() || !thisgame.SpriteInfos[spr].IsRelativeRes())
      return retval;
  return ctx_data_to_game_size(retval, thisgame.SpriteInfos[spr].IsLegacyHiRes());
}

int get_adjusted_spriteheight(int spr) {
  Common::Bitmap *tsp = get_sprite(spr);
  if (tsp == NULL)
      return 0;
  int retval = tsp->GetHeight();
  if (!thisgame.AllowRelativeRes() || !thisgame.SpriteInfos[spr].IsRelativeRes())
      return retval;
  return ctx_data_to_game_size(retval, thisgame.SpriteInfos[spr].IsLegacyHiRes());
}

void drawBlockOfColour(int hdc, int x,int y, int width, int height, int colNum)
{
	__my_setcolor(&colNum, colNum, BaseColorDepth);
  Common::Bitmap *palbmp = Common::BitmapHelper::CreateBitmap(width, height, thisgame.color_depth * 8);
  palbmp->Clear (colNum);
  drawBlockScaledAt(hdc, palbmp, x, y, 1);
  delete palbmp;
}

void new_font () {
  FontInfo fi;
  load_font_size(thisgame.numfonts, fi);
  thisgame.fonts.push_back(FontInfo());
  thisgame.numfonts++;
}

bool initialize_native()
{
    // Set text encoding and init allegro
    set_uformat(U_UTF8);
    set_filename_encoding(U_UNICODE);
    install_allegro(SYSTEM_NONE, &errno, atexit);

    AssetMgr.reset(new AssetManager());
    AssetMgr->AddLibrary("."); // TODO: this is for search in editor program folder, but maybe don't use implicit cwd?

	palette = &thisgame.defpal[0];
	thisgame.color_depth = 2;
    BaseColorDepth = 32;
	thisgame.numfonts = 0;

    HAGSError err = reset_sprite_file();
	if (!err)
	  return false;

	if (!Scintilla_RegisterClasses (GetModuleHandle(NULL)))
      return false;

	init_font_renderer(AssetMgr.get());
    new_font();

	RoomTools.reset(new NativeRoomTools());
	return true;
}

void shutdown_native()
{
    RoomTools.reset();
    // We must dispose all native bitmaps before shutting down the library
    thisroom.Free();
    shutdown_font_renderer();
    spriteset.Reset();
    allegro_exit();
    AssetMgr.reset();
}

void drawBlockScaledAt (int hdc, Common::Bitmap *todraw ,int x, int y, float scaleFactor) {
  if (todraw->GetColorDepth () == 8)
    set_palette_to_hdc ((HDC)hdc, palette);

  // FIXME later
  stretch_blit_to_hdc (todraw->GetAllegroBitmap(), (HDC)hdc, 0,0,todraw->GetWidth(),todraw->GetHeight(),
    x,y,todraw->GetWidth() * scaleFactor, todraw->GetHeight() * scaleFactor);
}

void drawSprite(int hdc, int x, int y, int spriteNum, bool flipImage) {
	Common::Bitmap *theSprite = get_sprite(spriteNum);

  if (theSprite == NULL)
    return;

	if (flipImage) {
		Common::Bitmap *flipped = Common::BitmapHelper::CreateBitmap (theSprite->GetWidth(), theSprite->GetHeight(), theSprite->GetColorDepth());
		flipped->FillTransparent();
		flipped->FlipBlt(theSprite, 0, 0, Common::kFlip_Horizontal);
		drawBlockScaledAt(hdc, flipped, x, y, 1);
		delete flipped;
	}
	else 
	{
		drawBlockScaledAt(hdc, theSprite, x, y, 1);
	}
}

void drawSpriteStretch(int hdc, int x, int y, int width, int height, int spriteNum, bool flipImage) {
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

  if (flipImage) {
    Common::Bitmap* flipped = Common::BitmapHelper::CreateBitmap(todraw->GetWidth(), todraw->GetHeight(), todraw->GetColorDepth());
    flipped->FillTransparent();
    flipped->FlipBlt(todraw, 0, 0, Common::kFlip_Horizontal);
    stretch_blit_to_hdc(flipped->GetAllegroBitmap(), (HDC)hdc, 0, 0, flipped->GetWidth(), flipped->GetHeight(), x, y, width, height);
    delete flipped;
  } else {
    // FIXME later
    stretch_blit_to_hdc(todraw->GetAllegroBitmap(), (HDC)hdc, 0, 0, todraw->GetWidth(), todraw->GetHeight(), x, y, width, height);
  }

  delete tempBlock;
}

void drawGUIAt(int hdc, int x, int y, int x1, int y1,int x2, int y2, int resolutionFactor, float scale, int ctrl_trans)
{
  if ((tempgui.Width < 1) || (tempgui.Height < 1))
    return;

  if (resolutionFactor == 1) {
    dsc_want_hires = 1;
  }

  std::unique_ptr<AGSBitmap> tempblock(new AGSBitmap(tempgui.Width, tempgui.Height, thisgame.color_depth * 8));
  tempblock->Clear(tempblock->GetMaskColor());

  tempgui.DrawSelf(tempblock.get());
  if (ctrl_trans == 0)
  {
      tempgui.DrawControls(tempblock.get());
  }
  else if (ctrl_trans < 100)
  {
      std::unique_ptr<AGSBitmap> ctrl_bmp(new AGSBitmap(tempgui.Width, tempgui.Height, thisgame.color_depth * 8));
      ctrl_bmp->Clear(ctrl_bmp->GetMaskColor());
      tempgui.DrawControls(ctrl_bmp.get());
      set_trans_blender(0, 0, 0, AGS::Common::GfxDef::Trans100ToAlpha255(ctrl_trans));
      tempblock->TransBlendBlt(ctrl_bmp.get(), 0, 0);
  }

  dsc_want_hires = 0;

  if (x1 >= 0) {
    tempblock->DrawRect(Rect (x1, y1, x2, y2), 14);
  }

  drawBlockScaledAt(hdc, tempblock.get(), x, y, scale);
}

#define SIMP_INDEX0  0
#define SIMP_TOPLEFT 1
#define SIMP_BOTLEFT 2
#define SIMP_TOPRIGHT 3
#define SIMP_BOTRIGHT 4
#define SIMP_LEAVEALONE 5
#define SIMP_NONE     6

// Adjusts sprite's transparency using the chosen method
void sort_out_transparency(Common::Bitmap *toimp, int sprite_import_method, RGB*itspal, int importedColourDepth,
    int &transcol)
{
  if (sprite_import_method == SIMP_LEAVEALONE)
  {
    transcol = 0;
    return;
  }

  set_palette_range(palette, 0, 255, 0);
  transcol=toimp->GetMaskColor();
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

    for (int tt=0;tt<toimp->GetWidth();tt++) {
      for (int uu=0;uu<toimp->GetHeight();uu++) {
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
    for (int tt=0;tt<toimp->GetWidth();tt++) {
      for (int uu=0;uu<toimp->GetHeight();uu++) {
        if (toimp->GetPixel(tt,uu)==transcol)
          toimp->PutPixel(tt,uu, bitmapMaskColor);
        else if (toimp->GetPixel(tt,uu) == bitmapMaskColor)
          toimp->PutPixel(tt,uu, replaceWithCol);
      }
    }
  }
}

// Adjusts 8-bit sprite's palette
void sort_out_palette(Common::Bitmap *toimp, RGB*itspal, bool useBgSlots, int transcol)
{
  set_palette_range(palette, 0, 255, 0);
  if ((thisgame.color_depth == 1) && (itspal != NULL)) { 
    // 256-colour mode only
    if (transcol!=0)
      itspal[transcol] = itspal[0];
    wsetrgb(0,0,0,0,itspal); // set index 0 to black
    RGB oldpale[256];
    for (int uu=0;uu<255;uu++) {
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
    BaseColorDepth = thisgame.color_depth * 8;
}

bool reload_font(int curFont)
{
    return load_font_size(curFont, thisgame.fonts[curFont]);
}

bool measure_font_height(const AGSString &filename, int pixel_height, int &formal_height)
{
    FontMetrics metrics;
    if (!load_font_metrics(filename, pixel_height, metrics))
        return false;
    formal_height = metrics.NominalHeight;
    return true;
}

HAGSError reset_sprite_file()
{
    return reset_sprite_file(AGS::Common::SpriteFile::DefaultSpriteFileName,
        AGS::Common::SpriteFile::DefaultSpriteIndexName);
}

HAGSError reset_sprite_file(const AGSString &spritefile, const AGSString &indexfile)
{
    auto sprite_file = AssetMgr->OpenAsset(spritefile);
    if (!sprite_file)
        return new AGSError(AGSString::FromFormat("Failed to open spriteset file '%s'.",
            spritefile.GetCStr()));
    auto index_file = AssetMgr->OpenAsset(indexfile);
	HAGSError err = spriteset.InitFile(std::move(sprite_file), std::move(index_file));
    if (!err)
        return err;
    // 100 mb cache // TODO: set this up in preferences?
	spriteset.SetMaxCacheSize(100 * 1024 * 1024);
    return HAGSError::None();
}

std::vector<Common::PluginInfo> thisgamePlugins;

HAGSError init_game_after_import(const AGS::Common::LoadedGameEntities &ents, GameDataVersion data_ver)
{
    newViews = std::move(ents.Views);
    dialog = std::move(ents.Dialogs);
    dlgscript = std::move(ents.OldDialogSources);
    numScriptModules = (int)ents.ScriptModules.size();
    scModules.resize(numScriptModules);
    for (int i = 0; i < numScriptModules; i++)
    {
        scModules[i].init();
        scModules[i].compiled = ents.ScriptModules[i];
    }

    thisgamePlugins = ents.PluginInfos;
    for (size_t i = 0; i < thisgamePlugins.size(); ++i)
    {
        // we don't care if it's an editor-only plugin or not
        if (thisgamePlugins[i].Name.GetLast() == '!')
            thisgamePlugins[i].Name.ClipRight(1);
    }

    AGS::Common::GUIRefCollection guictrl_refs(guibuts, guiinv, guilabels, guilist, guislider, guitext);
    for (int i = 0; i < thisgame.numgui; ++i)
    {
        HAGSError err = guis[i].RebuildArray(guictrl_refs);
        if (!err)
            return err;
    }

    // reset colour 0, it's possible for it to get corrupted
    palette[0].r = 0;
    palette[0].g = 0;
    palette[0].b = 0;
    set_palette_range(palette, 0, 255, 0);

    HAGSError err = reset_sprite_file();
    if (!err)
        return err;

    free_all_fonts();
    for (int i = 0; i < thisgame.numfonts; ++i)
        reload_font(i);

    update_abuf_coldepth();
    spritesModified = false;
    thisgame.filever = data_ver;
    return HAGSError::None();
}

HAGSError load_dta_file_into_thisgame(const AGSString &filename)
{
    AGS::Common::MainGameSource src;
    AGS::Common::LoadedGameEntities ents(thisgame);
    HGameFileError load_err = AGS::Common::OpenMainGameFile(filename, src);
    if (load_err)
    {
        load_err = AGS::Common::ReadGameData(ents, std::move(src.InputStream), src.DataVersion);
        if (load_err)
            load_err = AGS::Common::UpdateGameData(ents, src.DataVersion);
    }
    if (!load_err)
        return HAGSError(load_err);
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
  newViews.clear();
  guis.clear();
  dialog.clear();
  free_script_modules();

  // free game struct last because it contains object counts
  thisgame.Free();
}

// remap the scene, from its current palette oldpale to palette
void remap_background (Common::Bitmap *scene, const RGB *oldpale, RGB*palette, int exactPal) {

  if (exactPal) {
    // exact palette import (for doing palette effects, don't change)
    for (int a=0;a<256;a++) 
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
  for (int a=0;a<256;a++) {
    if (thisgame.paluses[a]!=PAL_GAMEWIDE) numbgslots++;
  }
  // find which colours from the image palette are actually used
  int imgpalcnt[256],numimgclr=0;
  memset(&imgpalcnt[0],0,sizeof(int)*256);

  for (int a=0;a<(scene->GetWidth()) * (scene->GetHeight());a++) {
    imgpalcnt[scene->GetScanLine(0)[a]]++;
  }
  for (int a=0;a<256;a++) {
    if (imgpalcnt[a]>0) numimgclr++;
  }
  // count up the number of unique colours in the image
  int numclr=0,bb;
  RGB tpal[256];
  for (int a=0;a<256;a++) {
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
  for (int a=0;a<numclr;a++) {
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
  for (int a=0;a<256;a++) {
    if (thisgame.paluses[a]==PAL_GAMEWIDE) {
      tpal[a].r=0;
      tpal[a].g=0; tpal[a].b=0; 
    }
    else tpal[a]=palette[a];
  }
  wremapall(oldpale,scene,tpal); //palette);
}

void validate_mask(Common::Bitmap *toValidate, const char *name, int maxColour) {
  if ((toValidate == NULL) || (toValidate->GetColorDepth() != 8)) {
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
	AGSString err = AGSString::FromFormat(
       "Invalid colours were found in the %s mask. They have now been removed."
       "\n\nWhen drawing a mask in an external image editor, you need to make "
       "sure that the image is set as 256-colour (Indexed Palette), and that "
       "you use the first %d colours in the palette for drawing %s. Palette "
       "entry 0 corresponds to No Area, palette index 1 corresponds to area 1, and "
       "so forth.", name, maxColour, name);
	MessageBox(NULL, err.GetCStr(), "Mask Error", MB_OK | MB_ICONWARNING);
    RoomTools->roomModified = true;
  }
}

void copy_room_palette_to_global_palette(const RoomStruct &rs)
{
  for (int ww = 0; ww < 256; ww++) 
  {
    if (thisgame.paluses[ww] == PAL_BACKGROUND)
    {
      palette[ww] = rs.Palette[ww];
    }
  }
}

void copy_global_palette_to_room_palette(RoomStruct &rs)
{
  for (int ww = 0; ww < 256; ww++) 
  {
    if (thisgame.paluses[ww] != PAL_BACKGROUND)
    {
      rs.Palette[ww] = palette[ww];
      rs.BgFrames[0].Palette[ww] = palette[ww];
    }
  }
}

const char *get_mask_name(RoomAreaMask mask)
{
    switch (mask)
    {
    case kRoomAreaHotspot: return "Hotspots";
    case kRoomAreaWalkBehind: return "Walk-behinds";
    case kRoomAreaWalkable: return "Walkable areas";
    case kRoomAreaRegion: return "Regions";
    default: return "unknown";
    }
}

AGSString load_room_file(RoomStruct &rs, const AGSString &filename) {
  HAGSError err = LoadRoom(filename, &rs, AssetMgr.get(), thisgame.IsLegacyHiRes(), thisgame.SpriteInfos);
  if (!err)
      return err->FullMessage();

  // Update room palette with gamewide colours
  copy_global_palette_to_room_palette(rs);
  // Update current global palette with room background colours
  copy_room_palette_to_global_palette(rs);
  for (size_t i = 0; i < rs.Objects.size(); ++i) {
    // change invalid objects to blue cup
    // TODO: should this be done in the common native lib?
    if (spriteset[rs.Objects[i].Sprite] == NULL)
      rs.Objects[i].Sprite = 0;
  }
  // Fix hi-color screens
  // TODO: find out why this is necessary. Probably has something to do with
  // Allegro switching color components at certain version update long time ago.
  // do we need to do this in the engine too?
  for (size_t i = 0; i < rs.BgFrameCount; ++i)
    fix_block (rs.BgFrames[i].Graphic.get());

  set_palette_range(palette, 0, 255, 0);
  
  if ((rs.BgFrames[0].Graphic->GetColorDepth() > 8) &&
      (thisgame.color_depth == 1))
  {
      MessageBox(NULL, "WARNING: This room is hi-color, but your game is currently 256-colour. You will not be able to use this room in this game. Also, the room background will not look right in the editor.", "Colour depth warning", MB_OK);
  }

  RoomTools->roomModified = false;

  validate_mask(rs.HotspotMask.get(), get_mask_name(kRoomAreaHotspot), MAX_ROOM_HOTSPOTS);
  validate_mask(rs.WalkBehindMask.get(), get_mask_name(kRoomAreaWalkBehind), MAX_WALK_AREAS);
  validate_mask(rs.WalkAreaMask.get(), get_mask_name(kRoomAreaWalkable), MAX_WALK_AREAS);
  validate_mask(rs.RegionMask.get(), get_mask_name(kRoomAreaRegion), MAX_ROOM_REGIONS);
  return AGSString();
}

void calculate_walkable_areas (RoomStruct &rs) {
  int ww, thispix;

  for (ww = 0; ww < MAX_WALK_AREAS; ww++) {
    rs.WalkAreas[ww].Top = rs.Height;
    rs.WalkAreas[ww].Bottom = 0;
  }
  for (ww = 0; ww < rs.WalkAreaMask->GetWidth(); ww++) {
    for (int qq = 0; qq < rs.WalkAreaMask->GetHeight(); qq++) {
      thispix = rs.WalkAreaMask->GetPixel (ww, qq);
      if (thispix >= MAX_WALK_AREAS)
        continue;
      if (rs.WalkAreas[thispix].Top > qq)
        rs.WalkAreas[thispix].Top = qq;
      if (rs.WalkAreas[thispix].Bottom < qq)
        rs.WalkAreas[thispix].Bottom= qq;
    }
  }

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

void ThrowManagedException(const char *message) 
{
	throw gcnew AGS::Types::AGSEditorException(gcnew String(message));
}

void CreateBuffer(int width, int height)
{
    auto &drawBuffer = RoomTools->drawBuffer;
    if (!drawBuffer || drawBuffer->GetWidth() != width || drawBuffer->GetHeight() != height || drawBuffer->GetColorDepth() != 32)
        drawBuffer.reset(new AGSBitmap(width, height, 32));
    drawBuffer->Clear(0x00D0D0D0);
}

void DrawSpriteToBuffer(int sprNum, int x, int y, float scale) {
    Common::Bitmap *todraw = get_sprite(sprNum);

	if (thisgame.AllowRelativeRes() && thisgame.SpriteInfos[sprNum].IsRelativeRes() &&
        !thisgame.SpriteInfos[sprNum].IsLegacyHiRes() && thisgame.IsLegacyHiRes())
	{
		scale *= 2.0f;
	}

	Common::Bitmap *imageToDraw = todraw;
    auto &drawBuffer = RoomTools->drawBuffer;
	if (todraw->GetColorDepth() != drawBuffer->GetColorDepth()) 
	{
		int oldColorConv = get_color_conversion();
		set_color_conversion(oldColorConv | COLORCONV_KEEP_TRANS);
		Common::Bitmap *depthConverted = Common::BitmapHelper::CreateBitmapCopy(todraw, drawBuffer->GetColorDepth());
		set_color_conversion(oldColorConv);

		imageToDraw = depthConverted;
	}

	int drawWidth = imageToDraw->GetWidth() * scale;
	int drawHeight = imageToDraw->GetHeight() * scale;

	if ((thisgame.SpriteInfos[sprNum].Flags & SPF_ALPHACHANNEL) != 0)
	{
		if (scale != 1.0f)
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
        drawBuffer->StretchBlt(imageToDraw, RectWH(x, y, drawWidth, drawHeight), Common::kBitmap_Transparency);
	}

	if (imageToDraw != todraw)
		delete imageToDraw;
}

void RenderBufferToHDC(int hdc) 
{
    auto &drawBuffer = RoomTools->drawBuffer;
	blit_to_hdc(drawBuffer->GetAllegroBitmap(), (HDC)hdc, 0, 0, 0, 0, drawBuffer->GetWidth(), drawBuffer->GetHeight());
}

void UpdateNativeSprites(SpriteFolder ^folder, std::vector<int> &missing)
{
	for each (Sprite ^sprite in folder->Sprites)
	{
        if (!spriteset.DoesSpriteExist(sprite->Number))
        {
            missing.push_back(sprite->Number);
            spriteset.SetEmptySprite(sprite->Number, true); // mark as an asset to prevent disposal on reload
        }

        int flags = 0;
        if (sprite->Resolution != SpriteImportResolution::Real)
        {
            flags |= SPF_VAR_RESOLUTION;
            if (sprite->Resolution == SpriteImportResolution::HighRes)
                flags |= SPF_HIRES;
        }
		if (sprite->AlphaChannel)
            flags |= SPF_ALPHACHANNEL;
        thisgame.SpriteInfos[sprite->Number].Flags = flags;
	}

	for each (SpriteFolder^ subFolder in folder->SubFolders) 
	{
        UpdateNativeSprites(subFolder, missing);
	}
}

int RemoveLeftoverSprites(SpriteFolder ^folder)
{
    int removed = 0;
    // NOTE: we do not ever remove sprite 0, because it's used as a placeholder too
    for (AGS::Common::sprkey_t i = 1; (size_t)i < spriteset.GetSpriteSlotCount(); ++i)
    {
        if (!spriteset.DoesSpriteExist(i)) continue;
        if (folder->FindSpriteByID(i, true) == nullptr)
        {
            spriteset.DisposeSprite(i);
            removed++;
        }
    }
    return removed;
}

void UpdateNativeSpritesToGame(Game ^game, CompileMessages ^errors)
{
    // Test for missing sprites: when the game has a sprite ref,
    // but the sprite file does not have respective data
    std::vector<int> missing;
    UpdateNativeSprites(game->RootSpriteFolder, missing);
    if (missing.size() > 0)
    {
        const size_t max_nums = 40;
        auto sprnum = gcnew System::Text::StringBuilder();
        sprnum->Append(missing[0]);
        for (size_t i = 1; i < missing.size() && i < max_nums; i++)
        {
            sprnum->Append(", ");
            sprnum->Append(missing[i]);
        }
        if (missing.size() > max_nums)
            sprnum->AppendFormat(", and {0} more.", missing.size() - max_nums);

        spritesModified = true;
        errors->Add(gcnew CompileWarning(String::Format(
            "Sprite file (acsprset.spr) contained less sprites than the game project referenced ({0} sprites were missing). "
            "This could happen if it was not saved properly last time. Some sprites could be missing actual images. "
            "You may try restoring them by reimporting from the source files.{1}{2}Affected sprites:{3}{4}",
            missing.size(), Environment::NewLine, Environment::NewLine, Environment::NewLine, sprnum->ToString())));
    }

    // Test for leftovers: when the game does NOT have a sprite ref,
    // but the sprite file has the data in the slot.
    if (RemoveLeftoverSprites(game->RootSpriteFolder) > 0)
    {
        spritesModified = true;
        errors->Add(gcnew CompileWarning(String::Format(
            "Sprite file (acsprset.spr) contained extra data that is not referenced by the game project. "
            "This could happen if it was not saved properly last time. This leftover data will be removed completely "
            "next time you save your project.")));
    }
}

// Attempts to save the current spriteset contents into the temporary file
// provided by the system API. On success assigns saved_filename.
void SaveTempSpritefile(int store_flags, AGS::Common::SpriteCompression compressSprites,
    AGSString &saved_spritefile, AGSString &saved_indexfile)
{
    // First save new sprite set into the temporary file
    String ^temp_spritefile = nullptr;
    String ^temp_indexfile = nullptr;
    try
    {
        temp_spritefile = IO::Path::GetTempFileName();
        temp_indexfile = IO::Path::GetTempFileName();
    }
    catch (Exception ^e)
    {
        throw gcnew AGSEditorException("Unable to create a temporary file to save sprites to.", e);
    }

    AGSString n_temp_spritefile = TextHelper::ConvertUTF8(temp_spritefile);
    AGSString n_temp_indexfile = TextHelper::ConvertUTF8(temp_indexfile);
    AGS::Common::SpriteFileIndex index;
    if (spriteset.SaveToFile(n_temp_spritefile, store_flags, compressSprites, index) != 0)
        throw gcnew AGSEditorException(String::Format("Unable to save the sprites. An error occurred whilst writing the sprite file.{0}Temp path: {1}",
            Environment::NewLine, temp_spritefile));
    saved_spritefile = n_temp_spritefile;
    if (AGS::Common::SaveSpriteIndex(n_temp_indexfile, index) == 0)
        saved_indexfile = n_temp_indexfile;
}

// Updates the backup and spritefile, moving it from the temp location.
void PutNewSpritefileIntoProject(const AGSString &temp_spritefile, const AGSString &temp_indexfile)
{
    spriteset.DetachFile();
    // Now when sprites are safe, move last sprite file to backup file
    String ^sprfilename = gcnew String(sprsetname);
    String ^backupname = String::Format("backup_{0}", sprfilename);
    try
    {
        if (IO::File::Exists(backupname))
            IO::File::Delete(backupname);
        if (IO::File::Exists(sprfilename))
            IO::File::Move(sprfilename, backupname);
    }
    catch (Exception^)
    {// TODO: ignore for now, but proper warning output system in needed here
    }

    // And then temp file to its final location
    String ^sprindexfilename = gcnew String(sprindexname);
    try
    {
        if (IO::File::Exists(sprfilename))
            IO::File::Delete(sprfilename);
        String^ path = TextHelper::ConvertUTF8(temp_spritefile);
        IO::File::Move(path, sprfilename);
    }
    catch (Exception ^e)
    {
        throw gcnew AGSEditorException("Unable to replace the previous sprite file in your project folder.", e);
    }

    // Sprite index is wanted but optional, so react to exceptions separately
    try
    {
        if (IO::File::Exists(sprindexfilename))
            IO::File::Delete(sprindexfilename);
        if (!temp_indexfile.IsEmpty())
            IO::File::Move(TextHelper::ConvertUTF8(temp_indexfile), sprindexfilename);
    }
    catch (Exception^)
    {// TODO: ignore for now, but proper warning output system in needed here
    }
}

void ReplaceSpriteFile(const AGSString &new_spritefile, const AGSString &new_indexfile, bool fallback_tempfiles)
{
    AGSString use_spritefile = sprsetname;
    AGSString use_indexfile = sprindexname;

    Exception ^main_exception;
    try
    {
        PutNewSpritefileIntoProject(new_spritefile, new_indexfile);
    }
    catch (Exception ^e)
    {
        main_exception = e;
        if (fallback_tempfiles)
        {
            use_spritefile = new_spritefile;
            use_indexfile = new_indexfile;
        }
    }
    finally
    {
        // Reset the sprite cache to whichever file was successfully saved
        HAGSError err = reset_sprite_file(use_spritefile, use_indexfile);
        if (!err)
        {
            throw gcnew AGSEditorException(
                String::Format("Unable to re-initialize sprite file after save.{0}{1}",
                    Environment::NewLine, gcnew String(err->FullMessage().GetCStr())), main_exception);
        }
        else if (main_exception != nullptr)
        {
            if (fallback_tempfiles)
                throw gcnew AGSEditorException(
                    String::Format("Unable to save sprites in your project folder. The sprites were saved to a temporary location:{0}{1}",
                        Environment::NewLine, TextHelper::ConvertUTF8(use_spritefile)), main_exception);
            else
                throw gcnew AGSEditorException(
                    String::Format("Unable to save sprites in your project folder."), main_exception);
        }
    }
    spritesModified = false;
}

void SaveNativeSprites(Settings^ gameSettings)
{
    int storeFlags = 0;
    if (gameSettings->OptimizeSpriteStorage)
        storeFlags |= AGS::Common::kSprStore_OptimizeForSize;
    AGS::Common::SpriteCompression compressSprites =
        (AGS::Common::SpriteCompression)gameSettings->CompressSpritesType;
    if (!spritesModified && (compressSprites == spriteset.GetSpriteCompression()) &&
        storeFlags == spriteset.GetStoreFlags())
        return;

    AGSString saved_spritefile;
    AGSString saved_indexfile;
    SaveTempSpritefile(storeFlags, compressSprites, saved_spritefile, saved_indexfile);

    ReplaceSpriteFile(saved_spritefile, saved_indexfile, true);
}

void SetGameResolution(Game ^game)
{
    // For backwards compatibility, save letterbox-by-design games as having non-custom resolution
    thisgame.options[OPT_LETTERBOX] = game->Settings->LetterboxMode;
    if (game->Settings->LetterboxMode)
        thisgame.SetDefaultResolution((GameResolutionType)game->Settings->LegacyLetterboxResolution);
    else
        thisgame.SetDefaultResolution(::Size(game->Settings->CustomResolution.Width, game->Settings->CustomResolution.Height));
}

void GameDirChanged(String ^workingDir)
{
    AssetMgr->RemoveAllLibraries();
    AssetMgr->AddLibrary(TextHelper::ConvertUTF8(workingDir));
}

void GameFontUpdated(Game ^game, int fontNumber, bool forceUpdate);

void GameUpdated(Game ^game, bool forceUpdate) {
  set_uformat(game->UnicodeMode ? U_UTF8 : U_ASCII);
  // TODO: this function may get called when only one item is added/removed or edited;
  // probably it would be best to split it up into several callbacks at some point.
  thisgame.color_depth = (int)game->Settings->ColorDepth;
  SetGameResolution(game);

  thisgame.options[OPT_RELATIVEASSETRES] = game->Settings->AllowRelativeAssetResolutions;
  thisgame.options[OPT_ANTIALIASFONTS] = game->Settings->AntiAliasFonts;
  thisgame.options[OPT_CLIPGUICONTROLS] = game->Settings->ClipGUIControls;
  thisgame.options[OPT_GAMETEXTENCODING] = game->TextEncoding->CodePage;
  antiAliasFonts = thisgame.options[OPT_ANTIALIASFONTS];

  AGS::Common::GUI::Options.ClipControls = thisgame.options[OPT_CLIPGUICONTROLS] != 0;

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
  thisgame.fonts.resize(thisgame.numfonts);
  for (int i = 0; i < thisgame.numfonts; i++) 
  {
      GameFontUpdated(game, i, forceUpdate);
  }
}

void GameFontUpdated(Game ^game, int fontNumber, bool forceUpdate)
{
    FontInfo &font_info = thisgame.fonts[fontNumber];
    AGS::Types::Font ^font = game->Fonts[fontNumber];

    int old_size = font_info.Size;
    int old_scaling = font_info.SizeMultiplier;
    int old_flags = font_info.Flags;

    font_info.Size = font->PointSize;
    font_info.SizeMultiplier = font->SizeMultiplier;
    font_info.YOffset = font->VerticalOffset;
    font_info.LineSpacing = font->LineSpacing;
    if (game->Settings->TTFHeightDefinedBy == FontHeightDefinition::PixelHeight)
        font_info.Flags &= ~FFLG_REPORTNOMINALHEIGHT;
    else
        font_info.Flags |= FFLG_REPORTNOMINALHEIGHT;
    if (font->TTFMetricsFixup == FontMetricsFixup::None)
        font_info.Flags &= ~FFLG_ASCENDERFIXUP;
    else
        font_info.Flags |= FFLG_ASCENDERFIXUP;

    if (forceUpdate ||
        font_info.Size != old_size ||
        font_info.SizeMultiplier != old_scaling ||
        font_info.Flags != old_flags)
    {
        reload_font(fontNumber);
    }
    else
    {
        set_fontinfo(fontNumber, font_info);
    }

    font->FamilyName = gcnew String(get_font_name(fontNumber));
    font->Height = get_font_surface_height(fontNumber);
}

void drawViewLoop (int hdc, ViewLoop^ loopToDraw, int x, int y, int size, List<int>^ cursel)
{
    std::vector<::ViewFrame> frames(loopToDraw->Frames->Count);
    for (int i = 0; i < loopToDraw->Frames->Count; ++i) 
    {
	    frames[i].pic = loopToDraw->Frames[i]->Image;
	    frames[i].flags = (loopToDraw->Frames[i]->Flipped) ? VFLG_FLIPSPRITE : 0;
    }
    std::vector<int> selected(cursel->Count);
    for (int i = 0; i < cursel->Count; ++i)
        selected[i] = cursel[i];
    doDrawViewLoop(hdc, frames, x, y, size, selected);
}

Common::Bitmap *CreateBlockFromBitmap(System::Drawing::Bitmap ^bmp, RGB *imgpal, int *srcPalLen,
    bool fixColourDepth, bool keepTransparency, int *originalColDepth)
{
    int src_depth, dst_depth;
    switch (bmp->PixelFormat)
    {
    case PixelFormat::Format1bppIndexed:
        src_depth = 1; dst_depth = 8; // convert 1-bit to 8-bit
        break;
    case PixelFormat::Format4bppIndexed:
        src_depth = 4; dst_depth = 8; // convert 4-bit to 8-bit
        break;
    case PixelFormat::Format8bppIndexed:
        src_depth = dst_depth = 8;
        break;
    case PixelFormat::Format16bppRgb555:
        src_depth = dst_depth = 15; // FIXME: convert to 16-bit instead?
        break;
    case PixelFormat::Format16bppRgb565:
        src_depth = dst_depth = 16;
        break;
    case PixelFormat::Format24bppRgb:
        src_depth = dst_depth = 24; // FIXME: convert to 32-bit instead?
        break;
    case PixelFormat::Format32bppRgb:
    case PixelFormat::Format32bppArgb:
        src_depth = dst_depth = 32;
        break;
    case PixelFormat::Format48bppRgb:
    case PixelFormat::Format64bppArgb:
    case PixelFormat::Format64bppPArgb:
        throw gcnew AGSEditorException("The source image is 48-bit or 64-bit colour. AGS does not support images with a colour depth higher than 32-bit. Make sure that your paint program is set to produce 32-bit images (8-bit per channel), not 48-bit images (16-bit per channel).");
    default:
        throw gcnew AGSEditorException(System::String::Format("Unsupported pixel format: \"{0}\"", bmp->PixelFormat.ToString()));
    }

    if ((thisgame.color_depth == 1) && (src_depth > 8))
    {
        throw gcnew AGSEditorException("You cannot import a hi-colour or true-colour image into a 256-colour game.");
    }

    if (srcPalLen)
        *srcPalLen = bmp->Palette ? bmp->Palette->Entries->Length : 0;
    if (originalColDepth)
        *originalColDepth = src_depth;

    // Test if destination bitmap will be not suitable for the game's color depth
    const bool needToFixColourDepth = (dst_depth != thisgame.color_depth * 8) && (fixColourDepth);

	System::Drawing::Rectangle rect(0, 0, bmp->Width, bmp->Height);
	BitmapData ^bmpData = bmp->LockBits(rect, ImageLockMode::ReadOnly, bmp->PixelFormat);
	const uint8_t *address = static_cast<const uint8_t*>(bmpData->Scan0.ToPointer());
    AGSBitmap *tempsprite = BitmapHelper::CreateBitmapFromPixels(bmp->Width, bmp->Height, dst_depth,
        address, src_depth, bmpData->Stride);
	bmp->UnlockBits(bmpData);

    if (!tempsprite)
    {
        throw gcnew AGSEditorException("Failed to create bitmap of compatible color depth. Could be unsupported image format.");
    }

	if (src_depth <= 8)
	{
		cli::array<System::Drawing::Color> ^bmpPalette = bmp->Palette->Entries;
    
    // determine actual transparency index, in case of GIF or other formats that can mark a specific palette entry
    int transparency_index = 0;
    for (int i = 0; i < bmpPalette->Length; ++i) {
      if (bmpPalette[i].A == 0) {
        transparency_index = i;
        System::Drawing::Color toswap = bmpPalette[0];
        bmpPalette[0] = bmpPalette[i];
        bmpPalette[i] = toswap;
        break; // to avoid blank palette entries
      }
    }
    
    // normalize tranparency to palette entry zero
    if (transparency_index != 0)
    {
      for (int tt = 0; tt<tempsprite->GetWidth(); tt++) {
        for (int uu = 0; uu<tempsprite->GetHeight(); uu++) {
          const int pixel = tempsprite->GetPixel(tt, uu);
          if (pixel == 0)
            tempsprite->PutPixel(tt, uu, transparency_index);
          else if (pixel == transparency_index)
            tempsprite->PutPixel(tt, uu, 0);
        }
      }
    }
    
		for (int i = 0; i < 256; i++) {
      if (i >= bmpPalette->Length)
      {
        // BMP files can have an arbitrary palette size, fill any
        // missing colours with black
			  imgpal[i].r = 0;
			  imgpal[i].g = 0;
			  imgpal[i].b = 0;
      }
      else if (thisgame.color_depth == 1)
      {
        // allegro palette is 0-63
        imgpal[i].r = bmpPalette[i].R / 4;
        imgpal[i].g = bmpPalette[i].G / 4;
        imgpal[i].b = bmpPalette[i].B / 4;
      }
      else
      {
			  imgpal[i].r = bmpPalette[i].R;
			  imgpal[i].g = bmpPalette[i].G;
			  imgpal[i].b = bmpPalette[i].B;
      }
		}
	}

	if (needToFixColourDepth)
	{
		Common::Bitmap *spriteAtRightDepth = Common::BitmapHelper::CreateBitmap(tempsprite->GetWidth(), tempsprite->GetHeight(), thisgame.color_depth * 8);
        if (!spriteAtRightDepth)
        {
            delete tempsprite;
            return nullptr; // out of mem?
        }

		if (dst_depth == 8)
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

    if (dst_depth == 8 && thisgame.color_depth > 1)
    {
      // manually compose to use the full palette instead of allegro 0-63 restricted one
      const int maskcolor = spriteAtRightDepth->GetMaskColor();
      // define a safe magenta color to use to preserve opacity in colors that match maskcolor
      const int safe_magenta = (thisgame.color_depth == 2)
        ? makecol_depth(thisgame.color_depth * 8, 255, 4, 255) // 16bit
        : makecol_depth(thisgame.color_depth * 8, 255, 1, 255); // 24-32 bit

      for (int ww = 0; ww<tempsprite->GetWidth(); ww++) {
        for (int vv = 0; vv<tempsprite->GetHeight(); vv++) {
          int px = tempsprite->GetPixel(ww, vv);
          RGB pal_color = imgpal[px];
          int color = makecol_depth(thisgame.color_depth * 8, pal_color.r, pal_color.g, pal_color.b);
          if (keepTransparency && px == 0)
            spriteAtRightDepth->PutPixel(ww, vv, spriteAtRightDepth->GetMaskColor() );
          else if (keepTransparency && color == maskcolor) // replace magenta with close match
            spriteAtRightDepth->PutPixel(ww, vv, safe_magenta);
          else
            spriteAtRightDepth->PutPixel(ww, vv, color);
        }
      }
    }
    else // let allegro do its own conversions
    {
      spriteAtRightDepth->Blit(tempsprite, 0, 0, 0, 0, tempsprite->GetWidth(), tempsprite->GetHeight());
    }
		set_color_conversion(oldColorConv);

		if (dst_depth == 8) 
		{
			unselect_palette();
		}
		delete tempsprite;
		tempsprite = spriteAtRightDepth;
	}

	if (dst_depth > 8) 
	{
		fix_block(tempsprite);
	}

	return tempsprite;
}

void DeleteBackground(Room ^room, int backgroundNumber) 
{
	RoomStruct *theRoom = (RoomStruct*)(void*)room->_roomStructPtr;
    theRoom->BgFrames[backgroundNumber].Graphic.reset();
	
	theRoom->BgFrameCount--;
	room->BackgroundCount--;
	for (size_t i = backgroundNumber; i < theRoom->BgFrameCount; i++) 
	{
		theRoom->BgFrames[i] = theRoom->BgFrames[i + 1];
		theRoom->BgFrames[i].IsPaletteShared = theRoom->BgFrames[i + 1].IsPaletteShared;
	}
}

void ImportBackground(Room ^room, int backgroundNumber, System::Drawing::Bitmap ^bmp, bool useExactPalette, bool sharePalette) 
{
    RGB oldpale[256];
	Common::Bitmap *newbg = CreateBlockFromBitmap(bmp, oldpale, nullptr, true, false, nullptr);
	RoomStruct *theRoom = (RoomStruct*)(void*)room->_roomStructPtr;
	theRoom->Width = room->Width;
	theRoom->Height = room->Height;
	bool resolutionChanged = (theRoom->GetResolutionType() != (AGS::Common::RoomResolutionType)room->Resolution);
	theRoom->SetResolution((AGS::Common::RoomResolutionType)room->Resolution);
    theRoom->MaskResolution = room->MaskResolution;

	if (newbg->GetColorDepth() == 8) 
	{
		for (int aa = 0; aa < 256; aa++) {
		  // make sure it maps to locked cols properly
		  if (thisgame.paluses[aa] == PAL_LOCKED)
			  theRoom->BgFrames[backgroundNumber].Palette[aa] = palette[aa];
		}

		// sharing palette with main background - so copy it across
		if (sharePalette) {
		  memcpy (&theRoom->BgFrames[backgroundNumber].Palette[0], &palette[0], sizeof(RGB) * 256);
		  theRoom->BgFrames[backgroundNumber].IsPaletteShared = 1;
		  if ((size_t)backgroundNumber >= theRoom->BgFrameCount - 1)
		  	theRoom->BgFrames[0].IsPaletteShared = 1;

		  if (!useExactPalette)
			wremapall(oldpale, newbg, palette);
		}
		else {
		  theRoom->BgFrames[backgroundNumber].IsPaletteShared = 0;
		  remap_background (newbg, oldpale, theRoom->BgFrames[backgroundNumber].Palette, useExactPalette);
		}

    copy_room_palette_to_global_palette(*theRoom);
	}

	if ((size_t)backgroundNumber >= theRoom->BgFrameCount)
	{
		theRoom->BgFrameCount++;
	}
	theRoom->BgFrames[backgroundNumber].Graphic.reset(newbg);

  // if size or resolution has changed, reset masks
	if ((newbg->GetWidth() != theRoom->WalkBehindMask->GetWidth()) || (newbg->GetHeight() != theRoom->WalkBehindMask->GetHeight()) ||
      (theRoom->Width != theRoom->WalkBehindMask->GetWidth()) || (resolutionChanged))
	{
        theRoom->WalkAreaMask.reset(new AGSBitmap(theRoom->Width / theRoom->MaskResolution, theRoom->Height / theRoom->MaskResolution, 8));
        theRoom->HotspotMask.reset(new AGSBitmap(theRoom->Width / theRoom->MaskResolution, theRoom->Height / theRoom->MaskResolution, 8));
        theRoom->WalkBehindMask.reset(new AGSBitmap(theRoom->Width, theRoom->Height, 8));
        theRoom->RegionMask.reset(new AGSBitmap(theRoom->Width / theRoom->MaskResolution, theRoom->Height / theRoom->MaskResolution, 8));
        theRoom->WalkAreaMask->Clear();
        theRoom->HotspotMask->Clear();
        theRoom->WalkBehindMask->Clear();
        theRoom->RegionMask->Clear();
	}

	room->BackgroundCount = theRoom->BgFrameCount;
	room->ColorDepth = theRoom->BgFrames[0].Graphic->GetColorDepth();
}

void import_area_mask(void *roomptr, int maskType, System::Drawing::Bitmap ^bmp)
{
    RGB oldpale[256];
	Common::Bitmap *importedImage = CreateBlockFromBitmap(bmp, oldpale, nullptr, false, false, nullptr);
	Common::Bitmap *mask = ((RoomStruct*)roomptr)->GetMask((RoomAreaMask)maskType);

	if (mask->GetWidth() != importedImage->GetWidth())
	{
		// allow them to import a double-size or half-size mask, adjust it as appropriate
        mask->StretchBlt(importedImage,
            RectWH(0, 0, importedImage->GetWidth(), importedImage->GetHeight()),
            RectWH(0, 0, mask->GetWidth(), mask->GetHeight()));
	}
	else
	{
		mask->Blit(importedImage, 0, 0, 0, 0, importedImage->GetWidth(), importedImage->GetHeight());
	}
	delete importedImage;

	validate_mask(mask, get_mask_name((RoomAreaMask)maskType), (maskType == kRoomAreaHotspot) ? MAX_ROOM_HOTSPOTS : MAX_WALK_AREAS);
}

SysBitmap ^export_area_mask(void *roomptr, int maskType)
{
    AGSBitmap *mask = ((RoomStruct*)roomptr)->GetMask((RoomAreaMask)maskType);
    return ConvertBlockToBitmap(mask, false);
}

Common::Bitmap *CreateNativeBitmap(System::Drawing::Bitmap^ bmp, int spriteImportMethod, bool remapColours,
    bool useRoomBackgroundColours, bool alphaChannel, int *out_flags)
{
    // If the input format does not have alpha - disable alpha channel flag
    alphaChannel &= System::Drawing::Bitmap::IsAlphaPixelFormat(bmp->PixelFormat);

    RGB imgPalBuf[256];
    int importedColourDepth;
    Common::Bitmap *tempsprite = CreateBlockFromBitmap(bmp, imgPalBuf, nullptr, true, (spriteImportMethod != SIMP_NONE), &importedColourDepth);

    int transcol;
    sort_out_transparency(tempsprite, spriteImportMethod, imgPalBuf, importedColourDepth, transcol);
    if (thisgame.color_depth == 1)
    {
        if (remapColours)
            sort_out_palette(tempsprite, imgPalBuf, useRoomBackgroundColours, transcol);
    }

    int flags = 0;
    if (alphaChannel)
    {
        flags |= SPF_ALPHACHANNEL;
        if (tempsprite->GetColorDepth() == 32)
        {
            BitmapHelper::ReplaceAlphaWithRGBMask(tempsprite);
        }
    }
    else if (tempsprite->GetColorDepth() == 32)
    {
        BitmapHelper::MakeOpaqueSkipMask(tempsprite);
    }

    if (out_flags)
        *out_flags = flags;
    return tempsprite;
}

AGS::Types::SpriteImportResolution SetNewSpriteFromBitmap(int slot, System::Drawing::Bitmap^ bmp, int spriteImportMethod,
    bool remapColours, bool useRoomBackgroundColours, bool alphaChannel)
{
    int flags;
    Common::Bitmap *tempsprite = CreateNativeBitmap(bmp, spriteImportMethod, remapColours,
        useRoomBackgroundColours, alphaChannel, &flags);

	SetNewSprite(slot, tempsprite, flags);

	return AGS::Types::SpriteImportResolution::Real;
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
  if (!tempBlock)
    return nullptr; // out of mem?

  if (todraw->GetColorDepth() == 8)
    select_palette(palette);

  tempBlock->Blit(todraw, 0, 0, 0, 0, todraw->GetWidth(), todraw->GetHeight());

  if (todraw->GetColorDepth() == 8)
	unselect_palette();

  if ((width != todraw->GetWidth()) || (height != todraw->GetHeight())) 
  {
	  Common::Bitmap *newBlock = Common::BitmapHelper::CreateBitmap(width, height, 32);
      if (!newBlock)
      {
          delete tempBlock;
          return nullptr; // out of mem?
      }
      newBlock->StretchBlt(tempBlock,
          RectWH(0, 0, todraw->GetWidth(), todraw->GetHeight()),
          RectWH(0, 0, width, height));
	  delete tempBlock;
	  tempBlock = newBlock;
  }

  fix_block(tempBlock);

  PixelFormat pixFormat = PixelFormat::Format32bppRgb;
  if ((todraw->GetColorDepth() == 32) && (useAlphaChannel))
	  pixFormat = PixelFormat::Format32bppArgb;

  System::Drawing::Bitmap ^bmp = gcnew System::Drawing::Bitmap(width, height, pixFormat);
  if (!bmp)
  {
      delete tempBlock;
      return nullptr; // out of mem?
  }
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
  return ConvertBlockToBitmap(todraw, (thisgame.SpriteInfos[spriteNum].Flags & SPF_ALPHACHANNEL) != 0);
}

System::Drawing::Bitmap^ getSpriteAsBitmap32bit(int spriteNum, int width, int height) {
  Common::Bitmap *todraw = get_sprite(spriteNum);
  if (todraw == NULL)
  {
	  throw gcnew AGSEditorException(String::Format("getSpriteAsBitmap32bit: Unable to find sprite {0}", spriteNum));
  }
  return ConvertBlockToBitmap32(todraw, width, height, (thisgame.SpriteInfos[spriteNum].Flags & SPF_ALPHACHANNEL) != 0);
}

System::Drawing::Bitmap^ getBackgroundAsBitmap(Room ^room, int backgroundNumber) {

  RoomStruct *roomptr = (RoomStruct*)(void*)room->_roomStructPtr;
  return ConvertBlockToBitmap(roomptr->BgFrames[backgroundNumber].Graphic.get(), false);
}

System::Drawing::Bitmap^ getBackgroundAsBitmap32(Room ^room, int backgroundNumber) {

  RoomStruct *roomptr = (RoomStruct*)(void*)room->_roomStructPtr;
  return ConvertBlockToBitmap32(roomptr->BgFrames[backgroundNumber].Graphic.get(), room->Width, room->Height, false);
}

void AdjustRoomResolution(Room ^room)
{
    RoomStruct *roomptr = (RoomStruct*)(void*)room->_roomStructPtr;
    AGS::Common::UpscaleRoomBackground(roomptr, thisgame.IsLegacyHiRes());
}

void FixRoomMasks(Room ^room)
{
    RoomStruct *roomptr = (RoomStruct*)(void*)room->_roomStructPtr;
    roomptr->MaskResolution = room->MaskResolution;
    AGS::Common::FixRoomMasks(roomptr);
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
  copy_global_palette_to_room_palette(thisroom);
}

void ConvertGUIToBinaryFormat(GUI ^guiObj, GUIMain *gui) 
{
  TextConverter^ tcv = TextHelper::GetGameTextConverter();

  NormalGUI^ normalGui = dynamic_cast<NormalGUI^>(guiObj);
  if (normalGui)
  {
	gui->OnClickHandler = TextHelper::ConvertASCII(normalGui->OnClick);
	gui->X = normalGui->Left;
	gui->Y = normalGui->Top;
	gui->Width = normalGui->Width;
	gui->Height = normalGui->Height;
    gui->SetClickable(normalGui->Clickable);
    gui->SetVisible(normalGui->Visible);
    gui->PopupAtMouseY = normalGui->PopupYPos;
    gui->PopupStyle = (Common::GUIPopupStyle)normalGui->PopupStyle;
    gui->ZOrder = normalGui->ZOrder;
    gui->FgColor = normalGui->BorderColor;
    gui->SetTransparencyAsPercentage(normalGui->Transparency);
  }
  else
  {
    TextWindowGUI^ twGui = dynamic_cast<TextWindowGUI^>(guiObj);
	gui->Width = twGui->EditorWidth;
	gui->Height = twGui->EditorHeight;
    gui->SetTextWindow(true);
    gui->PopupStyle = Common::kGUIPopupModal;
	gui->Padding = twGui->Padding;
    gui->FgColor = twGui->TextColor;
  }
  gui->BgColor = guiObj->BackgroundColor;
  gui->BgImage = guiObj->BackgroundImage;
  
  gui->Name = TextHelper::ConvertASCII(guiObj->Name);

  gui->RemoveAllControls();

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
          Common::GUIButton nbut;
          nbut.TextColor = button->TextColor;
          nbut.Font = button->Font;
          nbut.SetNormalImage(button->Image);
          nbut.SetCurrentImage(button->Image);
          nbut.SetMouseOverImage(button->MouseoverImage);
          nbut.SetPushedImage(button->PushedImage);
          nbut.TextAlignment = (::FrameAlignment)button->TextAlignment;
          nbut.ClickAction[Common::kGUIClickLeft] = (Common::GUIClickAction)button->ClickAction;
          nbut.ClickData[Common::kGUIClickLeft] = button->NewModeNumber;
          nbut.SetClipImage(button->ClipImage);
          nbut.SetText(tcv->ConvertTextProperty(button->Text));
          nbut.EventHandlers[0] = TextHelper::ConvertASCII(button->OnClick);
          guibuts.push_back(nbut);
		  
          gui->AddControl(Common::kGUIButton, guibuts.size() - 1, &guibuts.back());
	  }
	  else if (label)
	  {
          Common::GUILabel nlabel;
          nlabel.TextColor = label->TextColor;
          nlabel.Font = label->Font;
          nlabel.TextAlignment = (::HorAlignment)label->TextAlignment;
          Common::String text = tcv->ConvertTextProperty(label->Text);
          nlabel.SetText(text);
          guilabels.push_back(nlabel);

          gui->AddControl(Common::kGUILabel, guilabels.size() - 1, &guilabels.back());
	  }
	  else if (textbox)
	  {
          Common::GUITextBox ntext;
          ntext.TextColor = textbox->TextColor;
          ntext.Font = textbox->Font;
          ntext.SetShowBorder(textbox->ShowBorder);
          ntext.EventHandlers[0] = TextHelper::ConvertASCII(textbox->OnActivate);
          guitext.push_back(ntext);

          gui->AddControl(Common::kGUITextBox, guitext.size() - 1, &guitext.back());
	  }
	  else if (listbox)
	  {
          Common::GUIListBox nlist;
          nlist.TextColor = listbox->TextColor;
          nlist.Font = listbox->Font;
          nlist.SelectedTextColor = listbox->SelectedTextColor;
          nlist.SelectedBgColor = listbox->SelectedBackgroundColor;
          nlist.TextAlignment = (::HorAlignment)listbox->TextAlignment;
          nlist.SetTranslated(listbox->Translated);
          nlist.SetShowBorder(listbox->ShowBorder);
          nlist.SetShowArrows(listbox->ShowScrollArrows);
          nlist.EventHandlers[0] = TextHelper::ConvertASCII(listbox->OnSelectionChanged);
          guilist.push_back(nlist);

          gui->AddControl(Common::kGUIListBox, guilist.size() - 1, &guilist.back());
	  }
	  else if (slider)
	  {
          Common::GUISlider nslider;
		  nslider.MinValue = slider->MinValue;
		  nslider.MaxValue = slider->MaxValue;
		  nslider.Value = slider->Value;
		  nslider.HandleImage = slider->HandleImage;
		  nslider.HandleOffset = slider->HandleOffset;
		  nslider.BgImage = slider->BackgroundImage;
          nslider.EventHandlers[0] = TextHelper::ConvertASCII(slider->OnChange);
          guislider.push_back(nslider);

          gui->AddControl(Common::kGUISlider, guislider.size() - 1, &guislider.back());
	  }
	  else if (invwindow)
	  {
          Common::GUIInvWindow ninv;
          ninv.CharId = invwindow->CharacterID;
          ninv.ItemWidth = invwindow->ItemWidth;
          ninv.ItemHeight = invwindow->ItemHeight;
          guiinv.push_back(ninv);

          gui->AddControl(Common::kGUIInvWindow, guiinv.size() - 1, &guiinv.back());
	  }
	  else if (textwindowedge)
	  {
          Common::GUIButton nbut;
          nbut.SetNormalImage(textwindowedge->Image);
          nbut.SetCurrentImage(textwindowedge->Image);
          guibuts.push_back(nbut);
		  
          gui->AddControl(Common::kGUIButton, guibuts.size() - 1, &guibuts.back());
	  }

      Common::GUIObject *newObj = gui->GetControl(gui->GetControlCount() - 1);
	  newObj->X = control->Left;
	  newObj->Y = control->Top;
	  newObj->SetSize(control->Width, control->Height);
	  newObj->Id = control->ID;
	  newObj->ZOrder = control->ZOrder;
      newObj->Name = TextHelper::ConvertASCII(control->Name);
  }

  AGS::Common::GUIRefCollection guictrl_refs(guibuts, guiinv, guilabels, guilist, guislider, guitext);
  gui->RebuildArray(guictrl_refs);
}

void drawGUI(int hdc, int x, int y, GUI^ guiObj, int resolutionFactor, float scale, int ctrl_trans, int selectedControl) {
  guibuts.clear();
  guilabels.clear();
  guitext.clear();
  guilist.clear();
  guislider.clear();
  guiinv.clear();

  // Setup GuiContext
  AGS::Common::GUI::Context.Spriteset = &spriteset;

  ConvertGUIToBinaryFormat(guiObj, &tempgui);
  // Add dummy items to all listboxes, let user preview the fonts
  for (auto &lb : guilist)
  {
      lb.AddItem("Sample selected");
      lb.AddItem("Sample item");
  }

  tempgui.HighlightCtrl = selectedControl;

  drawGUIAt(hdc, x, y, -1, -1, -1, -1, resolutionFactor, scale, ctrl_trans);
}

Dictionary<int, Sprite^>^ load_sprite_dimensions()
{
	Dictionary<int, Sprite^>^ sprites = gcnew Dictionary<int, Sprite^>();

	for (size_t i = 0; i < spriteset.GetSpriteSlotCount(); i++)
	{
		Common::Bitmap *spr = spriteset[i];
		if (spr != NULL)
		{
			sprites->Add(i, gcnew Sprite(i, spr->GetWidth(), spr->GetHeight(), spr->GetColorDepth(),
                thisgame.SpriteInfos[i].IsRelativeRes() ? (thisgame.SpriteInfos[i].IsLegacyHiRes() ? SpriteImportResolution::HighRes : SpriteImportResolution::LowRes) : SpriteImportResolution::Real,
                (thisgame.SpriteInfos[i].Flags & SPF_ALPHACHANNEL) ? true : false));
		}
	}

	return sprites;
}

void ConvertCustomProperties(AGS::Types::CustomProperties ^insertInto, const AGS::Common::StringIMap *propToConvert)
{
    TextConverter^ tcv = TextHelper::GetGameTextConverter();
    for (AGS::Common::StringIMap::const_iterator it = propToConvert->begin();
         it != propToConvert->end(); ++it)
	{
		CustomProperty ^newProp = gcnew CustomProperty();
		newProp->Name = TextHelper::ConvertASCII(it->first); // property name is always ASCII
		newProp->Value = tcv->Convert(it->second);
		insertInto->PropertyValues->Add(newProp->Name, newProp);
	}
}

void CompileCustomProperties(AGS::Types::CustomProperties ^convertFrom, AGS::Common::StringIMap *compileInto)
{
    TextConverter^ tcv = TextHelper::GetGameTextConverter();
	compileInto->clear();
	for each (String ^key in convertFrom->PropertyValues->Keys)
	{
        AGS::Common::String name, value;
		name = TextHelper::ConvertASCII(convertFrom->PropertyValues[key]->Name); // property name is ASCII
		value = tcv->ConvertTextProperty(convertFrom->PropertyValues[key]->Value);
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
		TextHelper::ConvertASCIIToArray(game->Characters[charid]->ScriptName, charScriptNameBuf, 100); 
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
			AGSString str = TextHelper::ConvertASCII(variableToCheck->ScriptName);
			sprintf(scriptCode, "if (%s == %d) {", str.GetCStr(), valueToCheck);
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
			AGSString str = TextHelper::ConvertASCII(variableToCheck->ScriptName);
			sprintf(scriptCode, "%s = %d;", str.GetCStr(), valueToCheck);
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

void CopyInteractions(AGS::Types::Interactions ^destination, AGS::Common::InteractionScripts *source)
{
    size_t evt_count = std::min(source->ScriptFuncNames.size(), (size_t)destination->ScriptFunctionNames->Length);
    // TODO: add a warning? if warning list would be passed in here

	for (size_t i = 0; i < evt_count; i++) 
	{
		destination->ScriptFunctionNames[i] = TextHelper::ConvertASCII(source->ScriptFuncNames[i]);
	}
}

void ConvertInteractions(AGS::Types::Interactions ^interactions, Interaction *intr, String^ scriptFuncPrefix, AGS::Types::Game ^game, int targetTypeForUnhandledEvent)
{
    size_t evt_count = std::min(intr->Events.size(), (size_t)interactions->ScriptFunctionNames->Length);
    // TODO: add a warning? if warning list would be passed in here

	for (size_t i = 0; i < evt_count; i++) 
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
Game^ import_compiled_game_dta(const AGSString &filename)
{
	HAGSError err = load_dta_file_into_thisgame(filename);
    loaded_game_file_version = kGameVersion_Current;
	if (!err)
	{
		throw gcnew AGS::Types::AGSEditorException(TextHelper::ConvertUTF8(err->FullMessage()));
	}

	Game^ game = gcnew Game();
    game->Settings->GameTextEncoding = thisgame.options[OPT_GAMETEXTENCODING] == 65001 ? "UTF-8" : "ANSI";
    game->Settings->AlwaysDisplayTextAsSpeech = (thisgame.options[OPT_ALWAYSSPCH] != 0);
	game->Settings->AntiAliasFonts = (thisgame.options[OPT_ANTIALIASFONTS] != 0);
	game->Settings->AntiGlideMode = (thisgame.options[OPT_ANTIGLIDE] != 0);
	game->Settings->AutoMoveInWalkMode = !thisgame.options[OPT_NOWALKMODE];
	game->Settings->BackwardsText = (thisgame.options[OPT_RIGHTLEFTWRITE] != 0);
	game->Settings->ColorDepth = (GameColorDepth)thisgame.color_depth;
	game->Settings->CompressSpritesType = (SpriteCompression)thisgame.options[OPT_COMPRESSSPRITES];
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
	game->Settings->FontsForHiRes = (thisgame.options[OPT_HIRES_FONTS] != 0);
	game->Settings->GameName = gcnew String(thisgame.gamename.GetCStr());
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
	game->Settings->Resolution = (GameResolutions)thisgame.GetResolutionType();
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
    game->Settings->SaveGameFolderName = gcnew String(thisgame.gamename.GetCStr());
    game->Settings->RenderAtScreenResolution = (RenderAtScreenResolution)thisgame.options[OPT_RENDERATSCREENRES];
    game->Settings->AllowRelativeAssetResolutions = (thisgame.options[OPT_RELATIVEASSETRES] != 0);
    game->Settings->ScaleMovementSpeedWithMaskResolution = (thisgame.options[OPT_WALKSPEEDABSOLUTE] == 0);
    game->Settings->UseOldKeyboardHandling = (thisgame.options[OPT_KEYHANDLEAPI] == 0);
    game->Settings->ScaleCharacterSpriteOffsets = (thisgame.options[OPT_SCALECHAROFFSETS] != 0);

    TextConverter^ tcv = gcnew TextConverter(game->TextEncoding);

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
	
	for (int i = 0; i < 256; i++) 
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

	for (size_t i = 0; i < thisgamePlugins.size(); ++i) 
	{
		cli::array<System::Byte> ^pluginData = gcnew cli::array<System::Byte>(thisgamePlugins[i].Data.size());
		for (size_t j = 0; j < thisgamePlugins[i].Data.size(); j++) 
		{
			pluginData[j] = thisgamePlugins[i].Data[j];
		}
		
		AGS::Types::Plugin ^plugin = gcnew AGS::Types::Plugin(TextHelper::ConvertASCII(thisgamePlugins[i].Name), pluginData);
		game->Plugins->Add(plugin);
	}

	for (int i = 0; i < thisgame.numIntrVars; i++)
	{
		OldInteractionVariable ^intVar;
		intVar = gcnew OldInteractionVariable(TextHelper::ConvertASCII(thisgame.intrVars[i].Name), thisgame.intrVars[i].Value);
		game->OldInteractionVariables->Add(intVar);
	}
	
    AGS::Types::IViewFolder ^viewFolder = AGS::Types::FolderHelper::CreateDefaultViewFolder();
	for (int i = 0; i < thisgame.numviews; i++) 
	{
		AGS::Types::View ^view = gcnew AGS::Types::View();
		view->Name = TextHelper::ConvertASCII(thisgame.viewNames[i]);
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

	for (int i = 0; i < thisgame.numcharacters; i++) 
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
		character->RealName = gcnew String(thisgame.chars2[i].name_new.GetCStr());
		character->ScriptName = gcnew String(thisgame.chars2[i].scrname_new.GetCStr());
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

        // Custom properties are only for 2.6.0 and higher
        if (thisgame.filever >= kGameVersion_260)
        {
            ConvertCustomProperties(character->Properties, &thisgame.charProps[i]);
        }

		char scriptFuncPrefix[100];
		sprintf(scriptFuncPrefix, "character%d_", i);
		ConvertInteractions(character->Interactions, thisgame.intrChar[i].get(), gcnew String(scriptFuncPrefix), game, 3);
	}
	game->PlayerCharacter = game->Characters[thisgame.playercharacter];

	game->TextParser->Words->Clear();
	for (int i = 0; i < thisgame.dict->num_words; i++) 
	{
		AGS::Types::TextParserWord ^newWord = gcnew AGS::Types::TextParserWord();
		newWord->WordGroup = thisgame.dict->wordnum[i];
		newWord->Word = gcnew String(thisgame.dict->word[i]);
		newWord->SetWordTypeFromGroup();

		game->TextParser->Words->Add(newWord);
	}

	for (int i = 0; i < MAXGLOBALMES; i++) 
	{
		if (thisgame.messages[i] != NULL) 
		{
			game->GlobalMessages[i] = gcnew String(thisgame.messages[i].GetCStr());
		}
		else
		{
			game->GlobalMessages[i] = String::Empty;
		}
	}

	game->LipSync->Type = (thisgame.options[OPT_LIPSYNCTEXT] != 0) ? LipSyncType::Text : LipSyncType::None;
	game->LipSync->DefaultFrame = thisgame.default_lipsync_frame;
	for (int i = 0; i < MAXLIPSYNCFRAMES; i++) 
	{
		game->LipSync->CharactersPerFrame[i] = gcnew String(thisgame.lipSyncFrameLetters[i]);
	}

	for (int i = 0; i < thisgame.numdialog; i++) 
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

		newDialog->Name = TextHelper::ConvertASCII(thisgame.dialogScriptNames[i]);
		newDialog->Script = tcv->Convert(dlgscript[i]);
		newDialog->ShowTextParser = (dialog[i].topicFlags & DTFLG_SHOWPARSER);

		game->Dialogs->Add(newDialog);
	}

	for (int i = 0; i < thisgame.numcursors; i++)
	{
		AGS::Types::MouseCursor ^cursor = gcnew AGS::Types::MouseCursor();
		cursor->Animate = (thisgame.mcurs[i].view >= 0);
		cursor->AnimateOnlyOnHotspots = ((thisgame.mcurs[i].flags & MCF_HOTSPOT) != 0);
		cursor->AnimateOnlyWhenMoving = ((thisgame.mcurs[i].flags & MCF_ANIMMOVE) != 0);
		cursor->Image = thisgame.mcurs[i].pic;
		cursor->HotspotX = thisgame.mcurs[i].hotx;
		cursor->HotspotY = thisgame.mcurs[i].hoty;
		cursor->ID = i;
		cursor->Name = gcnew String(thisgame.mcurs[i].name.GetCStr());
		cursor->StandardMode = ((thisgame.mcurs[i].flags & MCF_STANDARD) != 0);
		cursor->View = thisgame.mcurs[i].view + 1;
		if (cursor->View < 1) cursor->View = 0;

		game->Cursors->Add(cursor);
	}

	for (int i = 0; i < thisgame.numfonts; i++) 
	{
		AGS::Types::Font ^font = gcnew AGS::Types::Font();
		font->ID = i;
		font->OutlineFont = (thisgame.fonts[i].Outline >= 0) ? thisgame.fonts[i].Outline : 0;
		if (thisgame.fonts[i].Outline == -1)
		{
			font->OutlineStyle = FontOutlineStyle::None;
		}
		else if (thisgame.fonts[i].Outline == FONT_OUTLINE_AUTO)
		{
			font->OutlineStyle = FontOutlineStyle::Automatic;
		}
		else 
		{
			font->OutlineStyle = FontOutlineStyle::UseOutlineFont;
		}
		font->PointSize = thisgame.fonts[i].Size;
		font->Name = gcnew String(String::Format("Font {0}", i));

		game->Fonts->Add(font);
	}

	for (int i = 1; i < thisgame.numinvitems; i++)
	{
		InventoryItem^ invItem = gcnew InventoryItem();
    invItem->CursorImage = thisgame.invinfo[i].pic;
		invItem->Description = gcnew String(thisgame.invinfo[i].name.GetCStr());
		invItem->Image = thisgame.invinfo[i].pic;
		invItem->HotspotX = thisgame.invinfo[i].hotx;
		invItem->HotspotY = thisgame.invinfo[i].hoty;
		invItem->ID = i;
		invItem->Name = TextHelper::ConvertASCII(thisgame.invScriptNames[i]);
		invItem->PlayerStartsWithItem = (thisgame.invinfo[i].flags & IFLG_STARTWITH);

		ConvertCustomProperties(invItem->Properties, &thisgame.invProps[i]);

		char scriptFuncPrefix[100];
		sprintf(scriptFuncPrefix, "inventory%d_", i);
		ConvertInteractions(invItem->Interactions, thisgame.intrInv[i].get(), gcnew String(scriptFuncPrefix), game, 5);

		game->InventoryItems->Add(invItem);
	}

    for (AGS::Common::PropertySchema::const_iterator it = thisgame.propSchema.begin();
         it != thisgame.propSchema.end(); ++it)
	{
		CustomPropertySchemaItem ^schemaItem = gcnew CustomPropertySchemaItem();
		schemaItem->Name = TextHelper::ConvertASCII(it->second.Name); // property name is always ASCII
		schemaItem->Description = tcv->Convert(it->second.Description);
		schemaItem->DefaultValue = tcv->Convert(it->second.DefaultValue);
		schemaItem->Type = (AGS::Types::CustomPropertyType)it->second.Type;

		game->PropertySchema->PropertyDefinitions->Add(schemaItem);
	}

    AGS::Common::GUIRefCollection guictrl_refs(guibuts, guiinv, guilabels, guilist, guislider, guitext);
	for (int i = 0; i < thisgame.numgui; i++)
	{
		guis[i].RebuildArray(guictrl_refs);

		GUI^ newGui;
		if (guis[i].IsTextWindow()) 
		{
			newGui = gcnew TextWindowGUI();
			newGui->Controls->Clear();  // we'll add our own edges
      ((TextWindowGUI^)newGui)->TextColor = guis[i].FgColor;
		}
		else 
		{
			newGui = gcnew NormalGUI(1, 1);
            ((NormalGUI^)newGui)->Clickable = guis[i].IsClickable();
            ((NormalGUI^)newGui)->Visible = guis[i].IsVisible();
			((NormalGUI^)newGui)->Top = guis[i].Y;
			((NormalGUI^)newGui)->Left = guis[i].X;
			((NormalGUI^)newGui)->Width = (guis[i].Width > 0) ? guis[i].Width : 1;
			((NormalGUI^)newGui)->Height = (guis[i].Height > 0) ? guis[i].Height : 1;
			((NormalGUI^)newGui)->PopupYPos = guis[i].PopupAtMouseY;
			((NormalGUI^)newGui)->PopupStyle = (GUIPopupStyle)guis[i].PopupStyle;
			((NormalGUI^)newGui)->ZOrder = guis[i].ZOrder;
			((NormalGUI^)newGui)->OnClick = TextHelper::ConvertASCII(guis[i].OnClickHandler);
      ((NormalGUI^)newGui)->BorderColor = guis[i].FgColor;
		}
		newGui->BackgroundColor = guis[i].BgColor;
		newGui->BackgroundImage = guis[i].BgImage;
		newGui->ID = i;
		newGui->Name = TextHelper::ConvertASCII(guis[i].Name);

		for (int j = 0; j < guis[i].GetControlCount(); j++)
		{
            Common::GUIObject* curObj = guis[i].GetControl(j);
			GUIControl ^newControl = nullptr;
            Common::GUIControlType ctrl_type = guis[i].GetControlType(j);
			switch (ctrl_type)
			{
			case Common::kGUIButton:
				{
				if (guis[i].IsTextWindow())
				{
					AGS::Types::GUITextWindowEdge^ edge = gcnew AGS::Types::GUITextWindowEdge();
					Common::GUIButton *copyFrom = (Common::GUIButton*)curObj;
					newControl = edge;
					edge->Image = copyFrom->GetNormalImage();
				}
				else
				{
					AGS::Types::GUIButton^ newButton = gcnew AGS::Types::GUIButton();
					Common::GUIButton *copyFrom = (Common::GUIButton*)curObj;
					newControl = newButton;
					newButton->TextColor = copyFrom->TextColor;
					newButton->Font = copyFrom->Font;
					newButton->Image = copyFrom->GetNormalImage();
					newButton->MouseoverImage = copyFrom->GetMouseOverImage();
					newButton->PushedImage = copyFrom->GetPushedImage();
					newButton->TextAlignment = (AGS::Types::FrameAlignment)copyFrom->TextAlignment;
                    newButton->ClickAction = (GUIClickAction)copyFrom->ClickAction[Common::kGUIClickLeft];
					newButton->NewModeNumber = copyFrom->ClickData[Common::kGUIClickLeft];
                    newButton->ClipImage = copyFrom->IsClippingImage();
					newButton->Text = tcv->Convert(copyFrom->GetText());
					newButton->OnClick = TextHelper::ConvertASCII(copyFrom->EventHandlers[0]);
				}
				break;
				}
			case Common::kGUILabel:
				{
				AGS::Types::GUILabel^ newLabel = gcnew AGS::Types::GUILabel();
				Common::GUILabel *copyFrom = (Common::GUILabel*)curObj;
				newControl = newLabel;
				newLabel->TextColor = copyFrom->TextColor;
				newLabel->Font = copyFrom->Font;
				newLabel->TextAlignment = (AGS::Types::HorizontalAlignment)copyFrom->TextAlignment;
				newLabel->Text = tcv->Convert(copyFrom->GetText());
				break;
				}
			case Common::kGUITextBox:
				{
				  AGS::Types::GUITextBox^ newTextbox = gcnew AGS::Types::GUITextBox();
				  Common::GUITextBox *copyFrom = (Common::GUITextBox*)curObj;
				  newControl = newTextbox;
				  newTextbox->TextColor = copyFrom->TextColor;
				  newTextbox->Font = copyFrom->Font;
                  newTextbox->ShowBorder = copyFrom->IsBorderShown();
				  newTextbox->Text = tcv->Convert(copyFrom->Text);
				  newTextbox->OnActivate = TextHelper::ConvertASCII(copyFrom->EventHandlers[0]);
				  break;
				}
			case Common::kGUIListBox:
				{
				  AGS::Types::GUIListBox^ newListbox = gcnew AGS::Types::GUIListBox();
				  Common::GUIListBox *copyFrom = (Common::GUIListBox*)curObj;
				  newControl = newListbox;
				  newListbox->TextColor = copyFrom->TextColor;
				  newListbox->Font = copyFrom->Font; 
				  newListbox->SelectedTextColor = copyFrom->SelectedTextColor;
				  newListbox->SelectedBackgroundColor = copyFrom->SelectedBgColor;
				  newListbox->TextAlignment = (AGS::Types::HorizontalAlignment)copyFrom->TextAlignment;
				  newListbox->ShowBorder = copyFrom->IsBorderShown();
				  newListbox->ShowScrollArrows = copyFrom->AreArrowsShown();
                  newListbox->Translated = copyFrom->IsTranslated();
				  newListbox->OnSelectionChanged = TextHelper::ConvertASCII(copyFrom->EventHandlers[0]);
				  break;
				}
			case Common::kGUISlider:
				{
				  AGS::Types::GUISlider^ newSlider = gcnew AGS::Types::GUISlider();
				  Common::GUISlider *copyFrom = (Common::GUISlider*)curObj;
				  newControl = newSlider;
				  newSlider->MinValue = copyFrom->MinValue;
				  newSlider->MaxValue = copyFrom->MaxValue;
				  newSlider->Value = copyFrom->Value;
				  newSlider->HandleImage = copyFrom->HandleImage;
			  	  newSlider->HandleOffset = copyFrom->HandleOffset;
				  newSlider->BackgroundImage = copyFrom->BgImage;
				  newSlider->OnChange = TextHelper::ConvertASCII(copyFrom->EventHandlers[0]);
				  break;
				}
			case Common::kGUIInvWindow:
				{
					AGS::Types::GUIInventory^ invwindow = gcnew AGS::Types::GUIInventory();
				    Common::GUIInvWindow *copyFrom = (Common::GUIInvWindow*)curObj;
				    newControl = invwindow;
					invwindow->CharacterID = copyFrom->CharId;
					invwindow->ItemWidth = copyFrom->ItemWidth;
					invwindow->ItemHeight = copyFrom->ItemHeight;
					break;
				}
			default:
				throw gcnew AGSEditorException("Unknown control type found: " + ((int)ctrl_type).ToString());
			}
            ::Size size = curObj->GetSize();
			newControl->Width = (size.Width > 0) ? size.Width : 1;
			newControl->Height = (size.Height > 0) ? size.Height : 1;
			newControl->Left = curObj->X;
			newControl->Top = curObj->Y;
			newControl->ZOrder = curObj->ZOrder;
            newControl->Clickable = curObj->IsClickable();
            newControl->Enabled = curObj->IsEnabled();
            newControl->Visible = curObj->IsVisible();
			newControl->ID = j;
			newControl->Name = TextHelper::ConvertASCII(curObj->Name);
			newGui->Controls->Add(newControl);
		}
		
		game->GUIs->Add(newGui);
	}

	free_old_game_data();

	return game;
}

System::String ^load_room_script(System::String ^fileName)
{
    AGSString roomFileName = TextHelper::ConvertUTF8(fileName);

    AGSString scriptText;
    AGS::Common::RoomDataSource src;
    AGS::Common::HRoomFileError err = OpenRoomFile(roomFileName, src);
    if (err)
    {
        err = AGS::Common::ExtractScriptText(scriptText, std::move(src.InputStream), src.DataVersion);
        if (err.HasError() && err->Code() == AGS::Common::kRoomFileErr_BlockNotFound)
            return nullptr; // simply did not find the script text
    }
    if (!err)
        quit(AGSString::FromFormat("Unable to load room script source from '%s', error was:\r\n%s", roomFileName.GetCStr(), err->FullMessage()));

	return gcnew String(scriptText.GetCStr(), 0, scriptText.GetLength(), System::Text::Encoding::Default);;
}

int GetCurrentlyLoadedRoomNumber()
{
  return RoomTools->loaded_room_number;
}

void convert_room_from_native(const RoomStruct &rs, Room ^room, System::Text::Encoding ^defEncoding)
{
    // Use local converter to account for room encoding (could be imported from another game)
    TextConverter^ tcv = defEncoding ? gcnew TextConverter(defEncoding) : TextHelper::GetGameTextConverter();
    try
    {
        auto enc_opt = rs.StrOptions.at("textencoding");
        tcv = gcnew TextConverter(System::Text::Encoding::GetEncoding(enc_opt.ToInt()));
    }
    catch (...) {}

    room->GameID = rs.GameID;
    room->BottomEdgeY = rs.Edges.Bottom;
    room->LeftEdgeX = rs.Edges.Left;
    room->MusicVolumeAdjustment = (AGS::Types::RoomVolumeAdjustment)rs.Options.MusicVolume;
    room->PlayerCharacterView = rs.Options.PlayerView;
    room->PlayMusicOnRoomLoad = rs.Options.StartupMusic;
    room->RightEdgeX = rs.Edges.Right;
    room->SaveLoadEnabled = (rs.Options.SaveLoadDisabled == 0);
    room->ShowPlayerCharacter = (rs.Options.PlayerCharOff == 0);
    room->TopEdgeY = rs.Edges.Top;
    room->Width = rs.Width;
    room->Height = rs.Height;
    room->ColorDepth = rs.BgFrames[0].Graphic->GetColorDepth();
    room->BackgroundAnimationDelay = rs.BgAnimSpeed;
    room->BackgroundAnimationEnabled = (rs.Options.Flags & kRoomFlag_BkgFrameLocked) == 0;
    room->BackgroundCount = rs.BgFrameCount;
    room->Resolution = (AGS::Types::RoomResolution)rs.GetResolutionType();
    room->MaskResolution = rs.MaskResolution;

	for (size_t i = 0; i < rs.LocalVariables.size(); ++i)
	{
		OldInteractionVariable ^intVar;
		intVar = gcnew OldInteractionVariable(TextHelper::ConvertASCII(rs.LocalVariables[i].Name), rs.LocalVariables[i].Value);
		room->OldInteractionVariables->Add(intVar);
	}

    for (size_t i = 0; i < rs.MessageCount; ++i)
	{
		RoomMessage ^newMessage = gcnew RoomMessage(i);
		newMessage->Text = tcv->Convert(rs.Messages[i]);
		newMessage->ShowAsSpeech = (rs.MessageInfos[i].DisplayAs > 0);
		newMessage->CharacterID = (rs.MessageInfos[i].DisplayAs - 1);
		newMessage->DisplayNextMessageAfter = ((rs.MessageInfos[i].Flags & MSG_DISPLAYNEXT) != 0);
		newMessage->AutoRemoveAfterTime = ((rs.MessageInfos[i].Flags & MSG_TIMELIMIT) != 0);
		room->Messages->Add(newMessage);
	}

	for (size_t i = 0; i < rs.Objects.size(); ++i) 
	{
		RoomObject ^obj = gcnew RoomObject(room);
		obj->ID = i;
		obj->Image = rs.Objects[i].Sprite;
		obj->StartX = rs.Objects[i].X;
		obj->StartY = rs.Objects[i].Y;
		obj->Visible = (rs.Objects[i].IsOn != 0);
		obj->Clickable = ((rs.Objects[i].Flags & OBJF_NOINTERACT) == 0);
		obj->Baseline = rs.Objects[i].Baseline;
		obj->Name = TextHelper::ConvertASCII(rs.Objects[i].ScriptName);
		obj->Description = tcv->Convert(rs.Objects[i].Name);
		obj->UseRoomAreaScaling = ((rs.Objects[i].Flags & OBJF_USEROOMSCALING) != 0);
		obj->UseRoomAreaLighting = ((rs.Objects[i].Flags & OBJF_USEREGIONTINTS) != 0);
		ConvertCustomProperties(obj->Properties, &rs.Objects[i].Properties);

		if (rs.DataVersion < kRoomVersion_300a)
		{
			char scriptFuncPrefix[100];
			sprintf(scriptFuncPrefix, "object%zu_", i);
			ConvertInteractions(obj->Interactions, rs.Objects[i].Interaction.get(), gcnew String(scriptFuncPrefix), nullptr, 2);
		}
		else 
		{
			CopyInteractions(obj->Interactions, rs.Objects[i].EventHandlers.get());
		}

		room->Objects->Add(obj);
	}

	for (size_t i = 0; i < rs.HotspotCount; ++i) 
	{
		RoomHotspot ^hotspot = room->Hotspots[i];
		hotspot->ID = i;
		hotspot->Description = tcv->Convert(rs.Hotspots[i].Name);
		hotspot->Name = TextHelper::ConvertASCII(rs.Hotspots[i].ScriptName);
        hotspot->WalkToPoint = System::Drawing::Point(rs.Hotspots[i].WalkTo.X, rs.Hotspots[i].WalkTo.Y);
		ConvertCustomProperties(hotspot->Properties, &rs.Hotspots[i].Properties);

		if (rs.DataVersion < kRoomVersion_300a)
		{
			char scriptFuncPrefix[100];
			sprintf(scriptFuncPrefix, "hotspot%zu_", i);
			ConvertInteractions(hotspot->Interactions, rs.Hotspots[i].Interaction.get(), gcnew String(scriptFuncPrefix), nullptr, 1);
		}
		else 
		{
			CopyInteractions(hotspot->Interactions, rs.Hotspots[i].EventHandlers.get());
		}
	}

	for (size_t i = 0; i < MAX_WALK_AREAS; ++i) 
	{
		RoomWalkableArea ^area = room->WalkableAreas[i];
		area->ID = i;
		area->AreaSpecificView = rs.WalkAreas[i].PlayerView;
		area->UseContinuousScaling = !(rs.WalkAreas[i].ScalingNear == NOT_VECTOR_SCALED);
		area->ScalingLevel = rs.WalkAreas[i].ScalingFar + 100;
		area->MinScalingLevel = rs.WalkAreas[i].ScalingFar + 100;
		if (area->UseContinuousScaling) 
		{
			area->MaxScalingLevel = rs.WalkAreas[i].ScalingNear + 100;
		}
		else
		{
			area->MaxScalingLevel = area->MinScalingLevel;
		}
	}

	for (size_t i = 0; i < MAX_WALK_BEHINDS; ++i) 
	{
		RoomWalkBehind ^area = room->WalkBehinds[i];
		area->ID = i;
		area->Baseline = rs.WalkBehinds[i].Baseline;
	}

	for (size_t i = 0; i < MAX_ROOM_REGIONS; ++i)
	{
		RoomRegion ^area = room->Regions[i];
		area->ID = i;
		// NOTE: Region's light level value exposed in editor is always 100 units higher,
		// for compatibility with older versions of the editor.
		// TODO: probably we could remove this behavior? Need to consider possible compat mode
		area->LightLevel = rs.GetRegionLightLevel(i) + 100;
		area->UseColourTint = rs.HasRegionTint(i);
		area->BlueTint = (rs.Regions[i].Tint >> 16) & 0x00ff;
		area->GreenTint = (rs.Regions[i].Tint >> 8) & 0x00ff;
		area->RedTint = rs.Regions[i].Tint & 0x00ff;
		// Set saturation's and luminance's default values in the editor if it is disabled in room data
		int saturation = (rs.Regions[i].Tint >> 24) & 0xFF;
		area->TintSaturation = (saturation > 0 && area->UseColourTint) ? saturation :
			Utilities::GetDefaultValue(area->GetType(), "TintSaturation", 0);
		int luminance = rs.GetRegionTintLuminance(i);
		area->TintLuminance = area->UseColourTint ? luminance :
			Utilities::GetDefaultValue(area->GetType(), "TintLuminance", 0);

		if (rs.DataVersion < kRoomVersion_300a)
		{
			char scriptFuncPrefix[100];
			sprintf(scriptFuncPrefix, "region%zu_", i);
			ConvertInteractions(area->Interactions, rs.Regions[i].Interaction.get(), gcnew String(scriptFuncPrefix), nullptr, 0);
		}
		else 
		{
			CopyInteractions(area->Interactions, rs.Regions[i].EventHandlers.get());
		}
	}

	ConvertCustomProperties(room->Properties, &rs.Properties);

	if (rs.DataVersion < kRoomVersion_300a)
	{
		ConvertInteractions(room->Interactions, rs.Interaction.get(), "room_", nullptr, 0);
	}
	else 
	{
		CopyInteractions(room->Interactions, rs.EventHandlers.get());
	}
}

AGS::Types::Room^ load_crm_file(UnloadedRoom ^roomToLoad, System::Text::Encoding ^defEncoding)
{
    AGSString roomFileName = TextHelper::ConvertUTF8(roomToLoad->FileName);

    AGSString errorMsg = load_room_file(thisroom, roomFileName);
    if (!errorMsg.IsEmpty())
    {
        throw gcnew AGSEditorException(TextHelper::ConvertUTF8(errorMsg));
    }

    RoomTools->loaded_room_number = roomToLoad->Number;

    Room ^room = gcnew Room(roomToLoad->Number);
    convert_room_from_native(thisroom, room, defEncoding);
    room->_roomStructPtr = (IntPtr)&thisroom;

    room->Description = roomToLoad->Description;
    room->Script = roomToLoad->Script;

    clear_undo_buffer();
    return room;
}

void convert_room_interactions_to_native(Room ^room, RoomStruct &rs);

void convert_room_to_native(Room ^room, RoomStruct &rs)
{
    //
    // Convert managed Room object into the native roomstruct that is going
    // to be saved using native procedure.
    //
    TextConverter^ tcv = TextHelper::GetGameTextConverter();
	rs.SetResolution((AGS::Common::RoomResolutionType)room->Resolution);
    rs.MaskResolution = room->MaskResolution;

	rs.GameID = room->GameID;
	rs.Edges.Bottom = room->BottomEdgeY;
	rs.Edges.Left = room->LeftEdgeX;
	rs.Options.MusicVolume = (RoomVolumeMod)room->MusicVolumeAdjustment;
	rs.Options.PlayerView = room->PlayerCharacterView;
	rs.Options.StartupMusic = 0; // [OBSOLETE]
	rs.Edges.Right = room->RightEdgeX;
	rs.Options.SaveLoadDisabled = false; // [OBSOLETE]
	rs.Options.PlayerCharOff = room->ShowPlayerCharacter ? 0 : 1;
	rs.Edges.Top = room->TopEdgeY;
	rs.Width = room->Width;
	rs.Height = room->Height;
	rs.BgAnimSpeed = room->BackgroundAnimationDelay;
	rs.BgFrameCount = room->BackgroundCount;
    rs.Options.Flags = 0;
    if (!room->BackgroundAnimationEnabled)
        rs.Options.Flags |= kRoomFlag_BkgFrameLocked;

	rs.MessageCount = room->Messages->Count;
	for (size_t i = 0; i < rs.MessageCount; ++i)
	{
		RoomMessage ^newMessage = room->Messages[i];
		rs.Messages[i] = tcv->ConvertTextProperty(newMessage->Text);
		if (newMessage->ShowAsSpeech)
		{
			rs.MessageInfos[i].DisplayAs = newMessage->CharacterID + 1;
		}
		else
		{
			rs.MessageInfos[i].DisplayAs = 0;
		}
		rs.MessageInfos[i].Flags = 0;
		if (newMessage->DisplayNextMessageAfter) rs.MessageInfos[i].Flags |= MSG_DISPLAYNEXT;
		if (newMessage->AutoRemoveAfterTime) rs.MessageInfos[i].Flags |= MSG_TIMELIMIT;
	}

	rs.Objects.resize(room->Objects->Count);
	for (size_t i = 0; i < rs.Objects.size(); ++i)
	{
		RoomObject ^obj = room->Objects[i];
		rs.Objects[i].ScriptName = TextHelper::ConvertASCII(obj->Name);

		rs.Objects[i].Sprite = obj->Image;
		rs.Objects[i].X = obj->StartX;
		rs.Objects[i].Y = obj->StartY;
		rs.Objects[i].IsOn = obj->Visible;
		rs.Objects[i].Baseline = obj->Baseline;
		rs.Objects[i].Name = tcv->ConvertTextProperty(obj->Description);
		rs.Objects[i].Flags = 0;
		if (obj->UseRoomAreaScaling) rs.Objects[i].Flags |= OBJF_USEROOMSCALING;
		if (obj->UseRoomAreaLighting) rs.Objects[i].Flags |= OBJF_USEREGIONTINTS;
		if (!obj->Clickable) rs.Objects[i].Flags |= OBJF_NOINTERACT;
		CompileCustomProperties(obj->Properties, &rs.Objects[i].Properties);
	}

	rs.HotspotCount = room->Hotspots->Count;
	for (size_t i = 0; i < rs.HotspotCount; ++i)
	{
		RoomHotspot ^hotspot = room->Hotspots[i];
		rs.Hotspots[i].Name = tcv->ConvertTextProperty(hotspot->Description);
		rs.Hotspots[i].ScriptName = TextHelper::ConvertASCII(hotspot->Name);
		rs.Hotspots[i].WalkTo.X = hotspot->WalkToPoint.X;
		rs.Hotspots[i].WalkTo.Y = hotspot->WalkToPoint.Y;
		CompileCustomProperties(hotspot->Properties, &rs.Hotspots[i].Properties);
	}

	rs.WalkAreaCount = room->WalkableAreas->Count;
	for (size_t i = 0; i < rs.WalkAreaCount; ++i)
	{
		RoomWalkableArea ^area = room->WalkableAreas[i];
		rs.WalkAreas[i].PlayerView = area->AreaSpecificView;
		
		if (area->UseContinuousScaling) 
		{
			rs.WalkAreas[i].ScalingFar = area->MinScalingLevel - 100;
			rs.WalkAreas[i].ScalingNear = area->MaxScalingLevel - 100;
		}
		else
		{
			rs.WalkAreas[i].ScalingFar = area->ScalingLevel - 100;
			rs.WalkAreas[i].ScalingNear = NOT_VECTOR_SCALED;
		}
	}

	rs.WalkBehindCount = room->WalkBehinds->Count;
	for (size_t i = 0; i < rs.WalkBehindCount; ++i)
	{
		RoomWalkBehind ^area = room->WalkBehinds[i];
		rs.WalkBehinds[i].Baseline = area->Baseline;
	}

	rs.RegionCount = room->Regions->Count;
	for (size_t i = 0; i < rs.RegionCount; ++i)
	{
		RoomRegion ^area = room->Regions[i];
		rs.Regions[i].Tint = 0;
		if (area->UseColourTint) 
		{
            rs.Regions[i].Tint  = area->RedTint | (area->GreenTint << 8) | (area->BlueTint << 16) | (area->TintSaturation << 24);
            rs.Regions[i].Light = (area->TintLuminance * 25) / 10;
		}
		else 
		{
            rs.Regions[i].Tint = 0;
			// NOTE: Region's light level value exposed in editor is always 100 units higher,
			// for compatibility with older versions of the editor.
			rs.Regions[i].Light = area->LightLevel - 100;
		}
	}

	CompileCustomProperties(room->Properties, &rs.Properties);

    // Prepare script links
    convert_room_interactions_to_native(room, rs);
    if (room->Script && room->Script->CompiledData)
	    rs.CompiledScript = ((AGS::Native::CompiledScript^)room->Script->CompiledData)->Data;

    // Encoding hint
    rs.StrOptions["textencoding"].Format("%d", tcv->GetEncoding()->CodePage);
}

void save_crm_file(Room ^room)
{
    convert_room_to_native(room, thisroom);
    AGSString roomFileName = TextHelper::ConvertUTF8(room->FileName);
    save_room_file(thisroom, roomFileName);
}

void save_default_crm_file(Room ^room)
{
    RoomStruct rs;
    convert_room_to_native(room, rs);
    // Insert default backgrounds and masks
    for (size_t i = 0; i < rs.BgFrameCount; ++i) // FIXME use of thisgame.color_depth
        rs.BgFrames[i].Graphic.reset(BitmapHelper::CreateClearBitmap(rs.Width, rs.Height, thisgame.color_depth * 8));
    rs.WalkAreaMask.reset(BitmapHelper::CreateClearBitmap(rs.Width / rs.MaskResolution, rs.Height / rs.MaskResolution, 8));
    rs.HotspotMask.reset(BitmapHelper::CreateClearBitmap(rs.Width / rs.MaskResolution, rs.Height / rs.MaskResolution, 8));
    rs.RegionMask.reset(BitmapHelper::CreateClearBitmap(rs.Width / rs.MaskResolution, rs.Height / rs.MaskResolution, 8));
    rs.WalkBehindMask.reset(BitmapHelper::CreateClearBitmap(rs.Width, rs.Height, 8));
    // Now save the resulting CRM
    AGSString roomFileName = TextHelper::ConvertUTF8(room->FileName);
    save_room_file(rs, roomFileName);
}

PInteractionScripts convert_interaction_scripts(Interactions ^interactions)
{
    AGS::Common::InteractionScripts *native_scripts = new AGS::Common::InteractionScripts();
	for each (String^ funcName in interactions->ScriptFunctionNames)
	{
        native_scripts->ScriptFuncNames.push_back(TextHelper::ConvertASCII(funcName));
	}
    return PInteractionScripts(native_scripts);
}

void convert_room_interactions_to_native(Room ^room, RoomStruct &rs)
{
    rs.EventHandlers = convert_interaction_scripts(room->Interactions);
	for (int i = 0; i < room->Hotspots->Count; ++i)
	{
        rs.Hotspots[i].EventHandlers = convert_interaction_scripts(room->Hotspots[i]->Interactions);
	}
    for (int i = 0; i < room->Objects->Count; ++i)
	{
        rs.Objects[i].EventHandlers = convert_interaction_scripts(room->Objects[i]->Interactions);
	}
    for (int i = 0; i < room->Regions->Count; ++i)
	{
        rs.Regions[i].EventHandlers = convert_interaction_scripts(room->Regions[i]->Interactions);
	}
}



#pragma unmanaged

// Fixups and saves the native room struct into the file
void save_room_file(RoomStruct &rs, const AGSString &path)
{
    rs.DataVersion = kRoomVersion_Current;
    calculate_walkable_areas(rs);

    rs.BackgroundBPP = rs.BgFrames[0].Graphic->GetBPP();
    for (int i = 0; i < 256; ++i)
        thisroom.Palette[i] = thisroom.BgFrames[0].Palette[i];
    // Fix hi-color screens
    // TODO: find out why this is needed; may be related to the Allegro internal 16-bit bitmaps format
    for (size_t i = 0; i < (size_t)rs.BgFrameCount; ++i)
        fix_block(rs.BgFrames[i].Graphic.get());

    std::unique_ptr<Stream> out(AGSFile::CreateFile(path));
    if (out == NULL)
        quit("save_room: unable to open room file for writing.");

    AGS::Common::HRoomFileError err = AGS::Common::WriteRoomData(&rs, out.get(), kRoomVersion_Current);
    if (!err)
        quit(AGSString::FromFormat("save_room: unable to write room data, error was:\r\n%s", err->FullMessage()));

    // Fix hi-color screens back again
    // TODO: find out why this is needed; may be related to the Allegro internal 16-bit bitmaps format
    for (size_t i = 0; i < rs.BgFrameCount; ++i)
        fix_block(rs.BgFrames[i].Graphic.get());
}


// Reimplementation of project-dependent functions from Common
#include "script/cc_common.h"

AGSString cc_format_error(const AGSString &message)
{
    if (currentline > 0)
        return AGSString::FromFormat("Error (line %d): %s", currentline, message.GetCStr());
    else
        return AGSString::FromFormat("Error (line unknown): %s", message.GetCStr());
}

AGSString cc_get_callstack(int max_lines)
{
    return "";
}

void quit(const char * message) 
{
	ThrowManagedException((const char*)message);
}
