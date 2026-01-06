//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// AGS Native interface to .NET
//
//=============================================================================
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define BITMAP WINDOWS_BITMAP
#include <windows.h>
#undef BITMAP
#include <stdlib.h>
#include "ac/characterinfo.h"
#include "ac/dialogtopic.h"
#include "ac/game_version.h"
#include "ac/inventoryiteminfo.h"
#include "ac/mousecursor.h"
#include "ac/view.h"
#include "ac/wordsdictionary.h"
#include "font/fonts.h"
#include "game/customproperties.h"
#include "game/main_game_file.h"
#include "game/plugininfo.h"
#include "util/error.h"
#include "util/ini_util.h"
#include "util/multifilelib.h"
#include "util/string_utils.h"
// IMPORTANT: NativeMethods.h must be included AFTER native headers,
// otherwise there will be naming conflicts with System:: and AGS::Types
#include "NativeMethods.h"
#include "NativeUtils.h"
#include "agsnative.h"

using namespace System::Runtime::InteropServices;
typedef AGS::Common::HError HAGSError;

extern bool initialize_native();
extern void shutdown_native();
extern AGS::Types::Game^ import_compiled_game_dta(const AGSString &filename);
extern void free_old_game_data();
extern AGS::Types::Room^ load_crm_file(UnloadedRoom ^roomToLoad, System::Text::Encoding ^defEncoding);
extern void save_crm_file(Room ^roomToSave);
extern void save_default_crm_file(Room ^roomToSave);
extern HAGSError import_sci_font(const AGSString &filename, int fslot);
extern bool reload_font(int curFont);
extern bool measure_font_height(const AGSString &filename, int pixel_height, int &formal_height);
// Get the loaded font's metrics (note: expand if necessary)
extern void GetFontMetrics(int fontnum, int &last_charcode, Rect &char_bbox);
extern void GetFontValidCharacters(int fontnum, std::vector<int> &char_codes);
// Draws font char sheet on the provided context
extern void DrawFontAt(HDC hdc, int fontnum, bool ansi_mode, bool only_valid_chars,
    int dc_atx, int dc_aty, int draw_atx, int draw_aty,
    int cell_w, int cell_h, int cell_space_x, int cell_space_y,
    int col_count, int row_count, int first_cell,
    float scaling);
extern void DrawTextUsingFontAt(HDC hdc, String^ text, int fontnum, bool draw_outline,
    int dc_atx, int dc_aty, int dc_width, int dc_height,
    int text_atx, int text_aty, int max_width, float scaling);
extern Dictionary<int, Sprite^>^ load_sprite_dimensions();
extern void drawGUI(HDC hdc, int x, int y, GUI^ gui, int resolutionFactor, float scale, int control_transparency, int selectedControl);
extern void drawSprite(HDC hdc, int x,int y, int spriteNum, bool flipImage);
extern void drawSpriteStretch(HDC hdc, int x,int y, int width, int height, int spriteNum, bool flipImage);
extern void drawBlockOfColour(HDC hdc, int x,int y, int width, int height, int colNum);
extern void drawViewLoop (HDC hdc, ViewLoop^ loopToDraw, int x, int y, int size, List<int>^ cursel);
extern AGS::Types::SpriteImportResolution SetNewSpriteFromBitmap(int slot, Bitmap^ bmp, int spriteImportMethod,
    int transColour, bool remapColours, bool useRoomBackgroundColours, bool alphaChannel);
extern Bitmap^ getSpriteAsBitmap(int spriteNum);
extern Bitmap^ getSpriteAsBitmap32bit(int spriteNum, int width, int height);
extern Bitmap^ getBackgroundAsBitmap(Room ^room, int backgroundNumber);
extern Bitmap^ getBackgroundAsBitmap32(Room ^room, int backgroundNumber);
extern int find_free_sprite_slot();
extern int crop_sprite_edges(const std::vector<int> &sprites, bool symmetric, Rect *crop_rect = nullptr);
extern void deleteSprite(int sprslot);
extern void GetSpriteInfo(int slot, ::SpriteInfo &info);
extern int GetSpriteWidth(int slot);
extern int GetSpriteHeight(int slot);
extern int GetSpriteColorDepth(int slot);
extern int GetPaletteAsHPalette();
extern bool DoesSpriteExist(int slot);
extern int GetMaxSprites();
extern int GetCurrentlyLoadedRoomNumber();
extern bool load_template_file(const AGSString &fileName, AGSString &description,
    std::vector<char> &iconDataBuffer, bool isRoomTemplate);
extern HAGSError extract_template_files(const AGSString &templateFileName);
extern HAGSError extract_room_template_files(const AGSString &templateFileName, int newRoomNumber);
extern void change_sprite_number(int oldNumber, int newNumber);
extern void update_sprite_resolution(int spriteNum, bool isVarRes, bool isHighRes);
extern void SaveNativeSprites(Settings^ gameSettings);
extern void ReplaceSpriteFile(const AGSString &new_spritefile, const AGSString &new_indexfile, bool fallback_tempfiles);
extern HAGSError reset_sprite_file();
extern void PaletteUpdated(cli::array<PaletteEntry^>^ newPalette);
extern void GameDirChanged(String ^workingDir);
extern void GameUpdated(Game ^game, bool forceUpdate);
extern void GameFontAdded(Game ^game, int fontNumber);
extern void GameFontDeleted(Game ^game, int fontNumber);
extern void GameFontUpdated(Game ^game, int fontNumber, bool forceUpdate);
extern void UpdateNativeSpritesToGame(Game ^game, CompileMessages ^errors);
extern void draw_room_background(void *roomptr, HDC hdc, int x, int y, int bgnum, float scaleFactor, int maskType, int selectedArea, int maskTransparency);
extern void ImportBackground(Room ^room, int backgroundNumber, Bitmap ^bmp, bool useExactPalette, bool sharePalette);
extern void DeleteBackground(Room ^room, int backgroundNumber);
extern void CreateBuffer(int width, int height);
extern void RenderBufferToHDC(HDC hdc);
extern void DrawSpriteToBuffer(int sprNum, int x, int y, float scale);
extern void draw_line_onto_mask(void *roomptr, int maskType, int x1, int y1, int x2, int y2, int color);
extern void draw_filled_rect_onto_mask(void *roomptr, int maskType, int x1, int y1, int x2, int y2, int color);
extern void draw_fill_onto_mask(void *roomptr, int maskType, int x1, int y1, int color);
extern void AdjustRoomResolution(Room ^room);
extern void FixRoomMasks(Room ^room);
extern int get_mask_pixel(void *roomptr, int maskType, int x, int y);
extern void import_area_mask(void *roomptr, int maskType, Bitmap ^bmp);
extern Bitmap ^export_area_mask(void *roomptr, int maskType);
extern void create_undo_buffer(void *roomptr, int maskType) ;
extern bool does_undo_buffer_exist();
extern void clear_undo_buffer() ;
extern void restore_from_undo_buffer(void *roomptr, int maskType);
extern System::String ^load_room_script(System::String ^fileName);
extern bool enable_greyed_out_masks;
extern bool spritesModified;

AGSString editorVersionNumber;

TextConverter::TextConverter(System::Text::Encoding^ enc)
    : _encoding(enc) {}

System::Text::Encoding^ TextConverter::GetEncoding() { return _encoding; }

System::String^ TextConverter::Convert(const AGS::Common::String &str)
{
    return TextHelper::Convert(str, _encoding);
}

System::String^ TextHelper::ConvertASCII(const AGS::Common::String &str)
{
    return gcnew String(str.GetCStr());
}

System::String^ TextHelper::ConvertUTF8(const AGS::Common::String &str)
{
    return Convert(str, System::Text::Encoding::UTF8);
}

System::String^ TextHelper::Convert(const AGS::Common::String &str, System::Text::Encoding^ enc)
{
    size_t len = strlen(str.GetCStr()); // we need number of bytes, not chars
    array<Byte>^ buf = gcnew array<Byte>(len);
    Marshal::Copy((IntPtr)(void*)str.GetCStr(), buf, 0, (int)len);
    return enc->GetString(buf);
}

AGSString TextConverter::Convert(System::String^ clr_str)
{
    if (clr_str == nullptr)
        return AGSString();
    return TextHelper::Convert(clr_str, _encoding);
}

AGSString TextConverter::ConvertTextProperty(System::String^ clr_str)
{
    if (clr_str == nullptr)
        return AGSString();
    AGSString str = TextHelper::Convert(clr_str, _encoding);
    // Escape backslashes before brackets: for '\[' support;
    // this is needed because Unescape will delete '\' in unrecognized sequence.
    str.Replace("\\[", "\\\\[");
    return AGS::Common::StrUtil::Unescape(str);
}

AGSString TextHelper::ConvertASCII(System::String^ clr_str)
{
    if (clr_str == nullptr)
        return AGSString();
    char* stringPointer = (char*)Marshal::StringToHGlobalAnsi(clr_str).ToPointer();
    AGSString str = stringPointer;
    Marshal::FreeHGlobal(IntPtr(stringPointer));
    return str;
}

void TextHelper::ConvertASCIIToArray(System::String^ clr_str, char *buf, size_t buf_len)
{
    char* stringPointer = (char*)Marshal::StringToHGlobalAnsi(clr_str).ToPointer();
    size_t ansi_len = std::min(strlen(stringPointer) + 1, buf_len);
    memcpy(buf, stringPointer, ansi_len);
    buf[ansi_len - 1] = 0;
    Marshal::FreeHGlobal(IntPtr(stringPointer));
}

AGSString TextHelper::ConvertUTF8(System::String^ clr_str)
{
    if (clr_str == nullptr)
        return AGSString();
    return Convert(clr_str, System::Text::Encoding::UTF8);
}

AGSString TextHelper::Convert(System::String^ clr_str, System::Text::Encoding^ enc)
{
    int len = enc->GetByteCount(clr_str);
    cli::array<unsigned char>^ buf = gcnew cli::array<unsigned char>(len + 1);
    enc->GetBytes(clr_str, 0, clr_str->Length, buf, 0);
    IntPtr dest_ptr = Marshal::AllocHGlobal(buf->Length);
    Marshal::Copy(buf, 0, dest_ptr, buf->Length);
    AGSString str = (const char*)dest_ptr.ToPointer();
    Marshal::FreeHGlobal(dest_ptr);
    return str;
}

TextConverter^ TextHelper::GetGameTextConverter()
{
    return AGS::Native::NativeMethods::GetGameTextConverter();
}

AGSString WinAPIHelper::GetErrorUTF8(uint32_t errcode)
{
    if (errcode == 0)
        errcode = GetLastError();

    WCHAR buffer[1024];
    if (FormatMessageW(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        /*source*/ NULL, /*dwMessageId*/ errcode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        buffer, sizeof(buffer) / sizeof(WCHAR), NULL) == 0)
    {
        return AGSString();
    }
    char message[1024];
    AGS::Common::StrUtil::ConvertWstrToUtf8(buffer, message, sizeof(message));
    return message;
}

AGSString WinAPIHelper::MakeErrorUTF8(const AGSString &error_title, uint32_t errcode)
{
    return AGSString::FromFormat("%s \n%s", error_title.GetCStr(), GetErrorUTF8(errcode).GetCStr());
}

System::String^ WinAPIHelper::MakeErrorManaged(const AGSString &error_title, uint32_t errcode)
{
    return TextHelper::ConvertUTF8(MakeErrorUTF8(error_title, errcode));
}


namespace AGS
{
	namespace Native
	{
		NativeMethods::NativeMethods(String ^editorVersion)
		{
			lastPaletteSet = nullptr;
			editorVersionNumber = TextHelper::ConvertASCII(editorVersion);
		}

        TextConverter^ NativeMethods::GetGameTextConverter()
        {
            return _gameTextConverter;
        }

		void NativeMethods::Initialize()
		{
            _gameTextConverter = gcnew TextConverter(System::Text::Encoding::UTF8);
			if (!initialize_native())
			{
				throw gcnew AGS::Types::InvalidDataException("Native initialization failed.");
			}
		}

        void NativeMethods::NewWorkingDirSet(String^ workingDir)
        {
            GameDirChanged(workingDir);
        }

		void NativeMethods::NewGameLoaded(Game ^game, CompileMessages ^errors)
		{
            _gameTextConverter = gcnew TextConverter(game->TextEncoding);
			this->PaletteColoursUpdated(game);
			GameUpdated(game, true);
			UpdateNativeSpritesToGame(game, errors);
		}

		void NativeMethods::PaletteColoursUpdated(Game ^game)
		{
			lastPaletteSet = game->Palette;
			PaletteUpdated(game->Palette);
		}

		void NativeMethods::LoadNewSpriteFile() 
		{
			HAGSError err = reset_sprite_file();
			if (!err)
			{
				throw gcnew AGSEditorException(gcnew String("Unable to load spriteset from ACSPRSET.SPR.\n") + TextHelper::ConvertUTF8(err->FullMessage()));
			}
		}

        void NativeMethods::ReplaceSpriteFile(String ^srcFileName)
        {
            AGSString temp_filename = TextHelper::ConvertUTF8(srcFileName);
            ::ReplaceSpriteFile(temp_filename, "", false);
        }

		void NativeMethods::SaveGame(Game ^game)
		{
			::SaveNativeSprites(game->Settings);
		}

		void NativeMethods::GameSettingsChanged(Game ^game)
		{
            _gameTextConverter = gcnew TextConverter(game->TextEncoding);
			GameUpdated(game, false);
		}

		void NativeMethods::DrawGUI(int hDC, int x, int y, GUI^ gui, int resolutionFactor, float scale, int controlTransparency, int selectedControl)
		{
			drawGUI((HDC)hDC, x, y, gui, resolutionFactor, scale, controlTransparency, selectedControl);
		}

		void NativeMethods::DrawSprite(int hDC, int x, int y, int spriteNum, bool flipImage)
		{
			drawSprite((HDC)hDC, x, y, spriteNum, flipImage);
		}

        Native::FontMetrics ^NativeMethods::GetFontMetrics(int fontNum)
        {
            int last_char;
            Rect bbox;
            ::GetFontMetrics(fontNum, last_char, bbox);
            return gcnew Native::FontMetrics(0, last_char,
                System::Drawing::Rectangle(bbox.Left, bbox.Top, bbox.GetWidth(), bbox.GetHeight()));
        }

        cli::array<int> ^NativeMethods::GetFontValidCharacters(int fontNum)
        {
            std::vector<int> char_codes;
            ::GetFontValidCharacters(fontNum, char_codes);
            cli::array<int> ^arr = gcnew cli::array<int>(char_codes.size());
            for (size_t i = 0; i < char_codes.size(); ++i)
                arr[i] = char_codes[i];
            return arr;
        }

        void NativeMethods::DrawFont(int hDC, int fontNum, bool ansi_mode, bool only_valid_chars,
            int dc_atx, int dc_aty, int draw_atx, int draw_aty,
            int cell_w, int cell_h, int cell_space_x, int cell_space_y,
            int col_count, int row_count, int first_cell,
            float scaling)
        {
            return DrawFontAt((HDC)hDC, fontNum, ansi_mode, only_valid_chars,
                dc_atx, dc_aty, draw_atx, draw_aty,
                cell_w, cell_h, cell_space_x, cell_space_y,
                col_count, row_count, first_cell, scaling);
        }

        void NativeMethods::DrawTextUsingFont(int hDC, String ^text, int fontNum, bool draw_outline,
            int dc_atx, int dc_aty, int dc_width, int dc_height,
            int text_atx, int text_aty, int max_width, float scaling)
        {
            DrawTextUsingFontAt((HDC)hDC, text, fontNum, draw_outline, dc_atx, dc_aty, dc_width, dc_height, text_atx, text_aty, max_width, scaling);
        }

		void NativeMethods::DrawSprite(int hDC, int x, int y, int width, int height, int spriteNum, bool flipImage)
		{
			drawSpriteStretch((HDC)hDC, x, y, width, height, spriteNum, flipImage);
		}

		void NativeMethods::DrawBlockOfColour(int hDC, int x, int y, int width, int height, int colourNum)
		{
			drawBlockOfColour((HDC)hDC, x, y, width, height, colourNum);
		}

		void NativeMethods::DrawViewLoop(int hdc, ViewLoop^ loopToDraw, int x, int y, int size, List<int>^ cursel)
		{
			drawViewLoop((HDC)hdc, loopToDraw, x, y, size, cursel);
		}

		bool NativeMethods::DoesSpriteExist(int spriteNumber)
        {
			if ((spriteNumber < 0) || (spriteNumber >= GetMaxSprites()))
			{
				return false;
			}
			return ::DoesSpriteExist(spriteNumber);
        }

		void NativeMethods::ImportSCIFont(String ^fileName, int fontSlot) 
		{
			AGSString fileNameAnsi = TextHelper::ConvertUTF8(fileName);
			HAGSError err = import_sci_font(fileNameAnsi, fontSlot);
			if (!err) 
			{
				throw gcnew AGSEditorException(TextHelper::ConvertUTF8(err->FullMessage()));
			}
		}

    void NativeMethods::ReloadFont(int fontSlot)
    {
      if (!reload_font(fontSlot))
      {
        throw gcnew AGSEditorException(String::Format("Unable to load font {0}. No supported font renderer was able to load the font.", fontSlot));
      }
    }

    int NativeMethods::FindTTFSizeForHeight(String ^fileName, int pixelHeight)
    {
        AGSString filename = TextHelper::ConvertUTF8(fileName);
        int height;
        if (!measure_font_height(filename, pixelHeight, height))
        {
            throw gcnew AGSEditorException(String::Format("Unable to load font {0}. Not a TTF font, or there an error occured while loading it.", fileName));
        }
        return height;
    }

    void NativeMethods::OnGameFontAdded(Game^ game, int fontSlot)
    {
        ::GameFontAdded(game, fontSlot);
    }

    void NativeMethods::OnGameFontDeleted(Game^ game, int fontSlot)
    {
        ::GameFontDeleted(game, fontSlot);
    }

    void NativeMethods::OnGameFontUpdated(Game^ game, int fontSlot, bool forceUpdate)
    {
        ::GameFontUpdated(game, fontSlot, forceUpdate);
    }

        AGS::Types::SpriteInfo^ NativeMethods::GetSpriteInfo(int spriteSlot)
        {
            ::SpriteInfo info;
            ::GetSpriteInfo(spriteSlot, info);
            return gcnew AGS::Types::SpriteInfo(info.Width, info.Height,
                info.IsRelativeRes() ? (info.IsLegacyHiRes() ? SpriteImportResolution::HighRes : SpriteImportResolution::LowRes) : SpriteImportResolution::Real);
        }

		int NativeMethods::GetSpriteWidth(int spriteSlot) 
		{
			return ::GetSpriteWidth(spriteSlot);
		}

		int NativeMethods::GetSpriteHeight(int spriteSlot) 
		{
			return ::GetSpriteHeight(spriteSlot);
		}

        Drawing::Size NativeMethods::GetMaxSpriteSize(array<int>^ sprites,
            [Runtime::InteropServices::Out] bool% hasLowResSprites)
        {
            int width = 0, height = 0;
            bool has_lowres = false;
            ::SpriteInfo info;
            for (int i = 0; i < sprites->Length; ++i)
            {
                ::GetSpriteInfo(sprites[i], info);
                width = std::max(width, info.Width);
                height = std::max(height, info.Height);
                has_lowres |= info.IsRelativeRes() && !info.IsLegacyHiRes();
            }
            hasLowResSprites = has_lowres;
            return Drawing::Size(width, height);
        }

		void NativeMethods::ChangeSpriteNumber(Sprite^ sprite, int newNumber)
		{
			if ((newNumber < 0) || (newNumber >= GetMaxSprites()))
			{
				throw gcnew AGSEditorException(gcnew String("Invalid sprite number"));
			}
			change_sprite_number(sprite->Number, newNumber);
			sprite->Number = newNumber;
		}

		void NativeMethods::SpriteResolutionsChanged(cli::array<Sprite^>^ sprites)
		{
			for each (Sprite^ sprite in sprites)
			{
				update_sprite_resolution(sprite->Number, sprite->Resolution != SpriteImportResolution::Real, sprite->Resolution == SpriteImportResolution::HighRes);
			}
		}

        Sprite^ NativeMethods::SetSpriteFromBitmap(int spriteSlot, Bitmap^ bmp, int spriteImportMethod, int transColour, bool remapColours, bool useRoomBackgroundColours, bool alphaChannel)
        {
            SpriteImportResolution spriteRes = SetNewSpriteFromBitmap(spriteSlot, bmp, spriteImportMethod, transColour, remapColours, useRoomBackgroundColours, alphaChannel);
            int colDepth = GetSpriteColorDepth(spriteSlot);
            Sprite^ newSprite = gcnew Sprite(spriteSlot, bmp->Width, bmp->Height, colDepth, spriteRes, alphaChannel);
            int roomNumber = GetCurrentlyLoadedRoomNumber();
            if ((colDepth == 8) && (useRoomBackgroundColours) && (roomNumber >= 0))
            {
                newSprite->ColoursLockedToRoom = roomNumber;
            }
            return newSprite;
        }

        void NativeMethods::ReplaceSpriteWithBitmap(Sprite ^spr, Bitmap^ bmp, int spriteImportMethod, int transColour, bool remapColours, bool useRoomBackgroundColours, bool alphaChannel)
        {
            SpriteImportResolution spriteRes = SetNewSpriteFromBitmap(spr->Number, bmp, spriteImportMethod, transColour, remapColours, useRoomBackgroundColours, alphaChannel);
            spr->Resolution = spriteRes;
            spr->ColorDepth = GetSpriteColorDepth(spr->Number);
            spr->Width = bmp->Width;
            spr->Height = bmp->Height;
            spr->AlphaChannel = alphaChannel;
            spr->ColoursLockedToRoom = System::Nullable<int>();
            int roomNumber = GetCurrentlyLoadedRoomNumber();
            if ((spr->ColorDepth == 8) && (useRoomBackgroundColours) && (roomNumber >= 0))
            {
                spr->ColoursLockedToRoom = roomNumber;
            }
        }

        Bitmap^ NativeMethods::GetSpriteBitmap(int spriteSlot)
        {
            return getSpriteAsBitmap(spriteSlot);
        }

        Bitmap^ NativeMethods::GetSpriteBitmapAs32Bit(int spriteSlot, int width, int height)
        {
	        return getSpriteAsBitmap32bit(spriteSlot, width, height);
        }

		void NativeMethods::DeleteSprite(int spriteSlot)
		{
			deleteSprite(spriteSlot);
		}

		int NativeMethods::GetFreeSpriteSlot()
		{
			return find_free_sprite_slot();
		}

		bool NativeMethods::CropSpriteEdges(System::Collections::Generic::IList<Sprite^>^ sprites, bool symmetric)
        {
            std::vector<int> spr_list;
            std::vector<Rect> spr_tile;
            spr_list.reserve(sprites->Count);
            spr_tile.reserve(sprites->Count);
			for each (Sprite^ sprite in sprites)
			{
				spr_list.push_back(sprite->Number);
                spr_tile.push_back(RectWH(sprite->OffsetX, sprite->OffsetY, sprite->ImportWidth, sprite->ImportHeight));
			}

            Rect crop_rect;
			bool result = crop_sprite_edges(spr_list, symmetric, &crop_rect) != 0;

            int newWidth = GetSpriteWidth(sprites[0]->Number);
			int newHeight = GetSpriteHeight(sprites[0]->Number);
            if (newWidth == sprites[0]->Width && newHeight == sprites[0]->Height)
                return false; // no change

			for each (Sprite^ sprite in sprites)
			{
                sprite->ImportAsTile = true;
                // Remember that this sprite may have been a tile already
                sprite->OffsetX += crop_rect.Left;
                sprite->OffsetY += crop_rect.Top;
                sprite->ImportWidth = crop_rect.GetWidth();
                sprite->ImportHeight = crop_rect.GetHeight();
				sprite->Width = newWidth;
				sprite->Height = newHeight;
			}
			return result;
		}

		void NativeMethods::Shutdown()
		{
			shutdown_native();
		}

		AGS::Types::Game^ NativeMethods::ImportOldGameFile(String^ fileName)
		{
			AGSString fileNameAnsi = TextHelper::ConvertUTF8(fileName);
			Game ^game = import_compiled_game_dta(fileNameAnsi);
			return game;
		}

		Dictionary<int,Sprite^>^ NativeMethods::LoadAllSpriteDimensions()
		{
			return load_sprite_dimensions();
		}

		AGS::Types::Room^ NativeMethods::LoadRoomFile(UnloadedRoom^ roomToLoad, System::Text::Encoding ^defEncoding)
		{
			return load_crm_file(roomToLoad, defEncoding);
		}

		void NativeMethods::SaveRoomFile(AGS::Types::Room ^roomToSave)
		{
			save_crm_file(roomToSave);
		}

        void NativeMethods::SaveDefaultRoomFile(AGS::Types::Room ^roomToSave)
        {
            save_default_crm_file(roomToSave);
        }

		void NativeMethods::CreateBuffer(int width, int height) 
		{
			::CreateBuffer(width, height);
		}

		void NativeMethods::DrawSpriteToBuffer(int sprNum, int x, int y, float scale) 
		{
			::DrawSpriteToBuffer(sprNum, x, y, scale);
		}

		void NativeMethods::RenderBufferToHDC(int hDC) 
		{
			::RenderBufferToHDC((HDC)hDC);
		}

		void NativeMethods::DrawRoomBackground(int hDC, Room ^room, int x, int y, int backgroundNumber, float scaleFactor, RoomAreaMaskType maskType, int selectedArea, int maskTransparency)
		{
			draw_room_background((void*)room->_roomStructPtr, (HDC)hDC, x, y, backgroundNumber, scaleFactor, (int)maskType, selectedArea, maskTransparency);
		}

		void NativeMethods::ImportBackground(Room ^room, int backgroundNumber, Bitmap ^bmp, bool useExactPalette, bool sharePalette)
		{
			::ImportBackground(room, backgroundNumber, bmp, useExactPalette, sharePalette);
		}

		void NativeMethods::DeleteBackground(Room ^room, int backgroundNumber)
		{
			::DeleteBackground(room, backgroundNumber);
		}

		Bitmap^ NativeMethods::GetRoomBackgroundForPreview(Room ^room, int backgroundNumber)
		{
			return getBackgroundAsBitmap32(room, backgroundNumber);
		}

        Bitmap^ NativeMethods::ExportRoomBackground(Room ^room, int backgroundNumber)
        {
            return getBackgroundAsBitmap(room, backgroundNumber);
        }

        void NativeMethods::AdjustRoomResolution(Room ^room)
        {
            ::AdjustRoomResolution(room);
        }

        void NativeMethods::AdjustRoomMaskResolution(Room ^room)
        {
            FixRoomMasks(room);
        }

		void NativeMethods::DrawLineOntoMask(Room ^room, RoomAreaMaskType maskType, int x1, int y1, int x2, int y2, int color)
		{
			draw_line_onto_mask((void*)room->_roomStructPtr, (int)maskType, x1, y1, x2, y2, color);
		}

		void NativeMethods::DrawFilledRectOntoMask(Room ^room, RoomAreaMaskType maskType, int x1, int y1, int x2, int y2, int color)
		{
			draw_filled_rect_onto_mask((void*)room->_roomStructPtr, (int)maskType, x1, y1, x2, y2, color);
		}

		void NativeMethods::DrawFillOntoMask(Room ^room, RoomAreaMaskType maskType, int x1, int y1, int color)
		{
			draw_fill_onto_mask((void*)room->_roomStructPtr, (int)maskType, x1, y1, color);
		}

		int NativeMethods::GetAreaMaskPixel(Room ^room, RoomAreaMaskType maskType, int x, int y)
		{
			return get_mask_pixel((void*)room->_roomStructPtr, (int)maskType, x, y);
		}

    void NativeMethods::ImportAreaMask(Room ^room, RoomAreaMaskType maskType, Bitmap ^bmp)
    {
      import_area_mask((void*)room->_roomStructPtr, (int)maskType, bmp);
    }

    Bitmap ^NativeMethods::ExportAreaMask(Room ^room, RoomAreaMaskType maskType)
    {
        return export_area_mask((void*)room->_roomStructPtr, (int)maskType);
    }

    void NativeMethods::CreateUndoBuffer(Room ^room, RoomAreaMaskType maskType)
		{
			create_undo_buffer((void*)room->_roomStructPtr, (int)maskType);
		}

    bool NativeMethods::DoesUndoBufferExist()
		{
			return does_undo_buffer_exist();
		}

    void NativeMethods::ClearUndoBuffer()
		{
			clear_undo_buffer();
		}

    void NativeMethods::RestoreFromUndoBuffer(Room ^room, RoomAreaMaskType maskType)
		{
			restore_from_undo_buffer((void*)room->_roomStructPtr, (int)maskType);
		}

    void NativeMethods::SetGreyedOutMasksEnabled(bool enabled)
    {
      enable_greyed_out_masks = enabled;
    }

		String ^NativeMethods::LoadRoomScript(String ^roomFileName) 
		{
			return load_room_script(roomFileName);
		}

    BaseTemplate^ NativeMethods::LoadTemplateFile(String ^fileName, bool isRoomTemplate)
    {
      AGSString fileNameAnsi = TextHelper::ConvertUTF8(fileName);
      AGSString description;
      std::vector<char> iconDataBuffer;

      int success = load_template_file(fileNameAnsi, description, iconDataBuffer, isRoomTemplate);
			if (success) 
			{
				Icon ^icon = nullptr;
				if (!iconDataBuffer.empty())
				{
          cli::array<unsigned char>^ managedArray = gcnew cli::array<unsigned char>(iconDataBuffer.size());
          Marshal::Copy(IntPtr(&iconDataBuffer.front()), managedArray, 0, iconDataBuffer.size());
          System::IO::MemoryStream^ ms = gcnew System::IO::MemoryStream(managedArray);
          try 
          {
					  icon = gcnew Icon(ms);
          } 
          catch (ArgumentException^) 
          {
            // it is not a valid .ICO file, ignore it
            icon = nullptr;
          }
				}
        if (isRoomTemplate)
        {
          return gcnew RoomTemplate(fileName, icon);
        }
        else
        {
            // Bring linebreaks in description to the uniform "\r\n" format.
            String ^uniDescription = TextHelper::ConvertUTF8(description);
            uniDescription = System::Text::RegularExpressions::Regex
                ::Replace(uniDescription, "(?<!\r)\n", "\r\n");
            return gcnew GameTemplate(fileName, uniDescription, icon);
        }
			}
			return nullptr;
    }

		GameTemplate^ NativeMethods::LoadTemplateFile(String ^fileName)
		{
      return (GameTemplate^)LoadTemplateFile(fileName, false);
		}

    RoomTemplate^ NativeMethods::LoadRoomTemplateFile(String ^fileName)
		{
      return (RoomTemplate^)LoadTemplateFile(fileName, true);
		}

		void NativeMethods::ExtractTemplateFiles(String ^templateFileName) 
		{
			AGSString fileNameAnsi = TextHelper::ConvertUTF8(templateFileName);
            HAGSError err = extract_template_files(fileNameAnsi);
			if (!err)
			{
				throw gcnew AGSEditorException("Unable to extract template files.\n" + TextHelper::ConvertUTF8(err->FullMessage()));
			}
		}

		void NativeMethods::ExtractRoomTemplateFiles(String ^templateFileName, int newRoomNumber) 
		{
			AGSString fileNameAnsi = TextHelper::ConvertUTF8(templateFileName);
            HAGSError err = extract_room_template_files(fileNameAnsi, newRoomNumber);
			if (!err)
			{
				throw gcnew AGSEditorException("Unable to extract template files.\n" + TextHelper::ConvertUTF8(err->FullMessage()));
			}
		}

		bool NativeMethods::HaveSpritesBeenModified()
		{
			return spritesModified;
		}

        /// <summary>
        /// Allows the Editor to reuse constants from the native code. If a constant required by the Editor
        /// is not also required by the Engine, then it should instead by moved into AGS.Types (AGS.Native
        /// references the AGS.Types assembly). Note that this method returns only System::Int32 and
        /// System::String objects -- it is up to the user to determine if the value should be used as a
        /// smaller integral type (additional casting may be required to cast to a non-int integral type).
        /// </summary>
        Object^ NativeMethods::GetNativeConstant(String ^name)
        {
            if (name == nullptr) return nullptr;
            if (name->Equals("GAME_FILE_SIG")) return gcnew String(AGS::Common::MainGameSource::Signature.GetCStr());
            if (name->Equals("GAME_DATA_VERSION_CURRENT")) return (int)kGameVersion_Current;
            if (name->Equals("MAX_GUID_LENGTH")) return MAX_GUID_LENGTH;
            if (name->Equals("MAX_SG_EXT_LENGTH")) return MAX_SG_EXT_LENGTH;
            if (name->Equals("MAX_SG_FOLDER_LEN")) return LEGACY_MAX_SG_FOLDER_LEN;
            if (name->Equals("MAX_SCRIPT_NAME_LEN")) return LEGACY_MAX_SCRIPT_NAME_LEN;
            if (name->Equals("FFLG_SIZEMULTIPLIER")) return FFLG_SIZEMULTIPLIER;
            if (name->Equals("IFLG_STARTWITH")) return IFLG_STARTWITH;
            if (name->Equals("MCF_ANIMMOVE")) return MCF_ANIMMOVE;
            if (name->Equals("MCF_STANDARD")) return MCF_STANDARD;
            if (name->Equals("MCF_HOTSPOT")) return MCF_HOTSPOT;
            if (name->Equals("CHF_MANUALSCALING")) return CHF_MANUALSCALING;
            if (name->Equals("CHF_NOINTERACT")) return CHF_NOINTERACT;
            if (name->Equals("CHF_NODIAGONAL")) return CHF_NODIAGONAL;
            if (name->Equals("CHF_NOLIGHTING")) return CHF_NOLIGHTING;
            if (name->Equals("CHF_NOTURNWHENWALK")) return CHF_NOTURNWHENWALK;
            if (name->Equals("CHF_NOBLOCKING")) return CHF_NOBLOCKING;
            if (name->Equals("CHF_SCALEMOVESPEED")) return CHF_SCALEMOVESPEED;
            if (name->Equals("CHF_SCALEVOLUME")) return CHF_SCALEVOLUME;
            if (name->Equals("CHF_ANTIGLIDE")) return CHF_ANTIGLIDE;
            if (name->Equals("CHF_TURNWHENFACE")) return CHF_TURNWHENFACE;
            if (name->Equals("DFLG_ON")) return DFLG_ON;
            if (name->Equals("DFLG_NOREPEAT")) return DFLG_NOREPEAT;
            if (name->Equals("DTFLG_SHOWPARSER")) return DTFLG_SHOWPARSER;
            if (name->Equals("FONT_OUTLINE_AUTO")) return FONT_OUTLINE_AUTO;
            if (name->Equals("MAX_STATIC_SPRITES")) return MAX_STATIC_SPRITES;
            if (name->Equals("MAX_PARSER_WORD_LENGTH")) return MAX_PARSER_WORD_LENGTH;
            if (name->Equals("MAX_INV")) return MAX_INV;
            if (name->Equals("MAXLIPSYNCFRAMES")) return MAXLIPSYNCFRAMES;
            if (name->Equals("MAXGLOBALMES")) return MAXGLOBALMES;
            if (name->Equals("LEGACY_MAXTOPICOPTIONS")) return LEGACY_MAXTOPICOPTIONS;
            if (name->Equals("UNIFORM_WALK_SPEED")) return safe_cast<Object^>(UNIFORM_WALK_SPEED);
            if (name->Equals("GAME_RESOLUTION_CUSTOM")) return (int)kGameResolution_Custom;
            if (name->Equals("SPRSET_NAME")) return gcnew String(sprsetname);
            if (name->Equals("SPF_VAR_RESOLUTION")) return SPF_VAR_RESOLUTION;
            if (name->Equals("SPF_HIRES")) return SPF_HIRES;
            if (name->Equals("SPF_ALPHACHANNEL")) return SPF_ALPHACHANNEL;
            if (name->Equals("PASSWORD_ENC_STRING"))
            {
                int len = (int)strlen(passwencstring);
                array<System::Byte>^ bytes = gcnew array<System::Byte>(len);
                System::Runtime::InteropServices::Marshal::Copy( IntPtr( ( char* ) passwencstring ), bytes, 0, len );
                return bytes;
            }
            if (name->Equals("LOOPFLAG_RUNNEXTLOOP")) return LOOPFLAG_RUNNEXTLOOP;
            if (name->Equals("VFLG_FLIPSPRITE")) return VFLG_FLIPSPRITE;
            if (name->Equals("GUIMAGIC")) return GUIMAGIC;
            if (name->Equals("SAVEBUFFERSIZE")) return PLUGIN_SAVEBUFFERSIZE;
            if (name->Equals("GUIMAIN_CLICKABLE")) return (int)Common::kGUIMain_Clickable;
            if (name->Equals("GUIMAIN_VISIBLE")) return (int)Common::kGUIMain_Visible;
            if (name->Equals("GUIF_ENABLED")) return (int)Common::kGUICtrl_Enabled;
            if (name->Equals("GUIF_CLICKABLE")) return (int)Common::kGUICtrl_Clickable;
            if (name->Equals("GUIF_VISIBLE")) return (int)Common::kGUICtrl_Visible;
            if (name->Equals("GUIF_CLIP")) return (int)Common::kGUICtrl_Clip;
            if (name->Equals("GUIF_TRANSLATED")) return (int)Common::kGUICtrl_Translated;
            if (name->Equals("GUIF_WRAPTEXT")) return (int)Common::kGUICtrl_WrapText;
            if (name->Equals("GLF_SHOWBORDER")) return (int)Common::kListBox_ShowBorder;
            if (name->Equals("GLF_SHOWARROWS")) return (int)Common::kListBox_ShowArrows;
            if (name->Equals("GUI_POPUP_MODAL")) return (int)Common::kGUIPopupModal;
            if (name->Equals("GUIMAIN_TEXTWINDOW")) return (int)Common::kGUIMain_TextWindow;
            if (name->Equals("GUIMAIN_LEGACYTEXTWINDOW")) return (int)Common::kGUIMain_LegacyTextWindow;
            if (name->Equals("GTF_SHOWBORDER")) return (int)Common::kTextBox_ShowBorder;
            if (name->Equals("GOBJ_BUTTON")) return (int)Common::kGUIButton;
            if (name->Equals("GOBJ_LABEL")) return (int)Common::kGUILabel;
            if (name->Equals("GOBJ_INVENTORY")) return (int)Common::kGUIInvWindow;
            if (name->Equals("GOBJ_SLIDER")) return (int)Common::kGUISlider;
            if (name->Equals("GOBJ_TEXTBOX")) return (int)Common::kGUITextBox;
            if (name->Equals("GOBJ_LISTBOX")) return (int)Common::kGUIListBox;
            if (name->Equals("TEXTWINDOW_PADDING_DEFAULT")) return TEXTWINDOW_PADDING_DEFAULT;
            if (name->Equals("GUI_VERSION_CURRENT")) return (int)kGuiVersion_Current;
            if (name->Equals("CUSTOM_PROPERTY_SCHEMA_VERSION")) return (int)AGS::Common::kPropertyVersion_Current;
            if (name->Equals("FFLG_LOGICALNOMINALHEIGHT")) return FFLG_LOGICALNOMINALHEIGHT;
            if (name->Equals("FFLG_LOGICALCUSTOMHEIGHT")) return FFLG_LOGICALCUSTOMHEIGHT;
            if (name->Equals("FFLG_ASCENDERFIXUP")) return FFLG_ASCENDERFIXUP;
            if (name->Equals("OPT_DEBUGMODE")) return safe_cast<Object^>(OPT_DEBUGMODE);
            if (name->Equals("OPT_WALKONLOOK")) return OPT_WALKONLOOK;
            if (name->Equals("OPT_DIALOGIFACE")) return OPT_DIALOGIFACE;
            if (name->Equals("OPT_ANTIGLIDE")) return OPT_ANTIGLIDE;
            if (name->Equals("OPT_TWCUSTOM")) return OPT_TWCUSTOM;
            if (name->Equals("OPT_DIALOGGAP")) return OPT_DIALOGGAP;
            if (name->Equals("OPT_NOSKIPTEXT")) return OPT_NOSKIPTEXT;
            if (name->Equals("OPT_DISABLEOFF")) return OPT_DISABLEOFF;
            if (name->Equals("OPT_ALWAYSSPCH")) return OPT_ALWAYSSPCH;
            if (name->Equals("OPT_SPEECHTYPE")) return OPT_SPEECHTYPE;
            if (name->Equals("OPT_PIXPERFECT")) return OPT_PIXPERFECT;
            if (name->Equals("OPT_NOWALKMODE")) return OPT_NOWALKMODE;
            if (name->Equals("OPT_LETTERBOX")) return OPT_LETTERBOX;
            if (name->Equals("OPT_FIXEDINVCURSOR")) return OPT_FIXEDINVCURSOR;
            if (name->Equals("OPT_HIRES_FONTS")) return OPT_HIRES_FONTS;
            if (name->Equals("OPT_SPLITRESOURCES")) return OPT_SPLITRESOURCES;
            if (name->Equals("OPT_CHARTURNWHENWALK")) return OPT_CHARTURNWHENWALK;
            if (name->Equals("OPT_FADETYPE")) return OPT_FADETYPE;
            if (name->Equals("OPT_HANDLEINVCLICKS")) return OPT_HANDLEINVCLICKS;
            if (name->Equals("OPT_MOUSEWHEEL")) return OPT_MOUSEWHEEL;
            if (name->Equals("OPT_DIALOGNUMBERED")) return OPT_DIALOGNUMBERED;
            if (name->Equals("OPT_DIALOGUPWARDS")) return OPT_DIALOGUPWARDS;
            if (name->Equals("OPT_ANTIALIASFONTS")) return OPT_ANTIALIASFONTS;
            if (name->Equals("OPT_THOUGHTGUI")) return OPT_THOUGHTGUI;
            if (name->Equals("OPT_CHARTURNWHENFACE")) return OPT_CHARTURNWHENFACE;
            if (name->Equals("OPT_RIGHTLEFTWRITE")) return OPT_RIGHTLEFTWRITE;
            if (name->Equals("OPT_DUPLICATEINV")) return OPT_DUPLICATEINV;
            if (name->Equals("OPT_SAVESCREENSHOT")) return OPT_SAVESCREENSHOT;
            if (name->Equals("OPT_SAVESCREENSHOTLAYER")) return OPT_SAVESCREENSHOTLAYER;
            if (name->Equals("OPT_PORTRAITSIDE")) return OPT_PORTRAITSIDE;
            if (name->Equals("OPT_STRICTSCRIPTING")) return OPT_STRICTSCRIPTING;
            if (name->Equals("OPT_LEFTTORIGHTEVAL")) return OPT_LEFTTORIGHTEVAL;
            if (name->Equals("OPT_COMPRESSSPRITES")) return OPT_COMPRESSSPRITES;
            if (name->Equals("OPT_STRICTSTRINGS")) return OPT_STRICTSTRINGS;
            if (name->Equals("OPT_NEWGUIALPHA")) return OPT_NEWGUIALPHA;
            if (name->Equals("OPT_RUNGAMEDLGOPTS")) return OPT_RUNGAMEDLGOPTS;
            if (name->Equals("OPT_NATIVECOORDINATES")) return OPT_NATIVECOORDINATES;
            if (name->Equals("OPT_GLOBALTALKANIMSPD")) return OPT_GLOBALTALKANIMSPD;
            if (name->Equals("OPT_SPRITEALPHA")) return OPT_SPRITEALPHA;
            if (name->Equals("OPT_SAFEFILEPATHS")) return OPT_SAFEFILEPATHS;
            if (name->Equals("OPT_DIALOGOPTIONSAPI")) return OPT_DIALOGOPTIONSAPI;
            if (name->Equals("OPT_BASESCRIPTAPI")) return OPT_BASESCRIPTAPI;
            if (name->Equals("OPT_SCRIPTCOMPATLEV")) return OPT_SCRIPTCOMPATLEV;
            if (name->Equals("OPT_RENDERATSCREENRES")) return OPT_RENDERATSCREENRES;
            if (name->Equals("OPT_RELATIVEASSETRES")) return OPT_RELATIVEASSETRES;
            if (name->Equals("OPT_WALKSPEEDABSOLUTE")) return OPT_WALKSPEEDABSOLUTE;
            if (name->Equals("OPT_CLIPGUICONTROLS")) return OPT_CLIPGUICONTROLS;
            if (name->Equals("OPT_GAMETEXTENCODING")) return OPT_GAMETEXTENCODING;
            if (name->Equals("OPT_KEYHANDLEAPI")) return OPT_KEYHANDLEAPI;
            if (name->Equals("OPT_SCALECHAROFFSETS")) return OPT_SCALECHAROFFSETS;
            if (name->Equals("OPT_VOICECLIPNAMERULE")) return OPT_VOICECLIPNAMERULE;
            if (name->Equals("OPT_GAMEFPS")) return OPT_GAMEFPS;
            if (name->Equals("OPT_GUICONTROLMOUSEBUT")) return OPT_GUICONTROLMOUSEBUT;
            if (name->Equals("OPT_LIPSYNCTEXT")) return OPT_LIPSYNCTEXT;
            return nullptr;
        }

        void NativeMethods::ReadIniFile(String ^fileName, Dictionary<String^, Dictionary<String^, String^>^>^ sections)
        {
            AGSString filename = TextHelper::ConvertUTF8(fileName);
            AGS::Common::ConfigTree cfg;
            if (!AGS::Common::IniUtil::Read(filename, cfg))
                return;

            sections->Clear();
            for (const auto &section : cfg)
            {
                String ^secname = TextHelper::ConvertASCII(section.first);
                Dictionary<String^, String^>^ secmap = gcnew Dictionary<String^, String^>();
                for (const auto &item : section.second)
                {
                    String ^key = TextHelper::ConvertASCII(item.first);
                    String ^value = TextHelper::ConvertUTF8(item.second);
                    secmap[key] = value;
                }
                sections[secname] = secmap;
            }
        }

        void NativeMethods::WriteIniFile(String ^fileName, Dictionary<String^, Dictionary<String^, String^>^>^ sections, bool mergeExisting)
        {
            AGSString filename = TextHelper::ConvertUTF8(fileName);
            AGS::Common::ConfigTree cfg;
            for each (auto section in sections)
            {
                AGSString secname = TextHelper::ConvertASCII(section.Key);
                AGS::Common::StringOrderMap secmap;
                for each (auto item in section.Value)
                {
                    AGSString key = TextHelper::ConvertASCII(item.Key);
                    AGSString value = TextHelper::ConvertUTF8(item.Value);
                    secmap[key] = value;
                }
                cfg[secname] = std::move(secmap);
            }

            if (mergeExisting)
                AGS::Common::IniUtil::Merge(filename, cfg);
            else
                AGS::Common::IniUtil::Write(filename, cfg);
        }
	}
}