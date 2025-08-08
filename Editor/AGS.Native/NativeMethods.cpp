//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
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
using namespace AGS::Native;
using Icon = System::Drawing::Icon;
typedef AGS::Common::HError HAGSError;

extern bool initialize_native();
extern void shutdown_native();
extern AGS::Types::Game^ import_compiled_game_dta(const AGSString &filename);
extern void free_old_game_data();
extern HAGSError import_sci_font(const AGSString &filename, int fslot);
extern bool reload_font(int curFont);
extern bool measure_font_height(const AGSString &filename, int pixel_height, int &formal_height);
// Draws font char sheet on the provided context and returns the height of drawn object;
// may be called with hdc = 0 to get required height without drawing anything
extern int drawFontAt(HDC hdc, int fontnum, int draw_atx, int draw_aty, int width, int height, int scroll_y);
extern Dictionary<int, Sprite^>^ load_sprite_dimensions();
extern void drawGUI(HDC hdc, int x, int y, GUI^ gui, int resolutionFactor, float scale, int control_transparency, int selectedControl);
extern void drawSprite(HDC hdc, int x,int y, int spriteNum, bool flipImage);
extern void drawSpriteStretch(HDC hdc, int x,int y, int width, int height, int spriteNum, bool flipImage);
extern void drawViewLoop (HDC hdc, ViewLoop^ loopToDraw, int x, int y, int size, List<int>^ cursel);
extern void SetNewSpriteFromBitmap(int slot, Bitmap^ bmp, int destColorDepth,
    int spriteImportMethod, int transColour, bool remapColours, bool useRoomBackgroundColours, bool alphaChannel);
extern Bitmap^ getSpriteAsBitmap(int spriteNum);
extern Bitmap^ getSpriteAsBitmap32bit(int spriteNum, int width, int height);
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
extern bool load_template_file(const AGSString &fileName, AGSString &description,
    std::vector<char> &iconDataBuffer, bool isRoomTemplate);
extern HAGSError extract_template_files(const AGSString &templateFileName, std::vector<AGSString> *out_files = nullptr);
extern HAGSError extract_room_template_files(const AGSString &templateFileName, int newRoomNumber, std::vector<AGSString> *out_files = nullptr);
extern void change_sprite_number(int oldNumber, int newNumber);
extern void SaveNativeSprites(Settings^ gameSettings);
extern void ReplaceSpriteFile(const AGSString &new_spritefile, const AGSString &new_indexfile, bool fallback_tempfiles);
extern HAGSError reset_sprite_file();
extern void ApplyPalette(cli::array<PaletteEntry^>^ newPalette);
extern void GameDirChanged(String ^workingDir);
extern void GameUpdated(Game ^game, bool forceUpdate);
extern void GameFontAdded(Game ^game, int fontNumber);
extern void GameFontDeleted(Game ^game, int fontNumber);
extern void GameFontUpdated(Game ^game, int fontNumber, bool forceUpdate);
extern void UpdateNativeSpritesToGame(Game ^game, CompileMessages ^errors);
extern System::String ^load_room_script(System::String ^fileName);
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

std::string TextConverter::ConvertToStd(System::String^ clr_str)
{
    if (clr_str == nullptr)
        return std::string();
    return TextHelper::ConvertToStd(clr_str, _encoding);
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

std::string TextHelper::ConvertASCIIToStd(System::String^ clr_str)
{
    if (clr_str == nullptr)
        return std::string();
    char* stringPointer = (char*)Marshal::StringToHGlobalAnsi(clr_str).ToPointer();
    std::string str = stringPointer;
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

static IntPtr ConvertImpl(System::String^ clr_str, System::Text::Encoding^ enc)
{
    int len = enc->GetByteCount(clr_str);
    cli::array<unsigned char>^ buf = gcnew cli::array<unsigned char>(len + 1);
    enc->GetBytes(clr_str, 0, clr_str->Length, buf, 0);
    IntPtr dest_ptr = Marshal::AllocHGlobal(buf->Length);
    Marshal::Copy(buf, 0, dest_ptr, buf->Length);
    return dest_ptr;
}

AGSString TextHelper::Convert(System::String^ clr_str, System::Text::Encoding^ enc)
{
    IntPtr intptr = ConvertImpl(clr_str, enc);
    AGSString str = (const char*)intptr.ToPointer();
    Marshal::FreeHGlobal(intptr);
    return str;
}

std::string TextHelper::ConvertToStd(System::String^ clr_str, System::Text::Encoding^ enc)
{
    IntPtr intptr = ConvertImpl(clr_str, enc);
    std::string str = (const char*)intptr.ToPointer();
    Marshal::FreeHGlobal(intptr);
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
			PaletteColoursUpdated(game);
            // Update asset library in case there were new asset directories created
            GameDirChanged(game->DirectoryPath);
            // Update native data from the loaded game
			GameUpdated(game, true);
			UpdateNativeSpritesToGame(game, errors);
		}

		void NativeMethods::PaletteColoursUpdated(Game ^game)
		{
            ApplyPalette(game->Palette);
		}

        void NativeMethods::ApplyPalette(cli::array<PaletteEntry^> ^palette)
		{
			lastPaletteSet = palette;
			::ApplyPalette(palette);
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

		int NativeMethods::DrawFont(int hDC, int fontNum, int draw_atx, int draw_aty, int width, int height, int scroll_y)
		{
			return drawFontAt((HDC)hDC, fontNum, draw_atx, draw_aty, width, height, scroll_y);
		}

		void NativeMethods::DrawSprite(int hDC, int x, int y, int width, int height, int spriteNum, bool flipImage)
		{
			drawSpriteStretch((HDC)hDC, x, y, width, height, spriteNum, flipImage);
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
            return gcnew AGS::Types::SpriteInfo(info.Width, info.Height, SpriteImportResolution::Real);
        }

		int NativeMethods::GetSpriteWidth(int spriteSlot) 
		{
			return ::GetSpriteWidth(spriteSlot);
		}

		int NativeMethods::GetSpriteHeight(int spriteSlot) 
		{
			return ::GetSpriteHeight(spriteSlot);
		}

        Drawing::Size NativeMethods::GetMaxSpriteSize(array<int>^ sprites)
        {
            int width = 0, height = 0;
            ::SpriteInfo info;
            for (int i = 0; i < sprites->Length; ++i)
            {
                ::GetSpriteInfo(sprites[i], info);
                width = std::max(width, info.Width);
                height = std::max(height, info.Height);
            }
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

        static int GetCurrentlyLoadedRoomNumber()
        {
            return 0; // FIXME: not working after moved to open room format
        }

        Sprite^ NativeMethods::SetSpriteFromBitmap(int spriteSlot, Bitmap^ bmp, int destColorDepth, int spriteImportMethod, int transColour, bool remapColours, bool useRoomBackgroundColours, bool alphaChannel)
        {
            SetNewSpriteFromBitmap(spriteSlot, bmp, destColorDepth, spriteImportMethod, transColour, remapColours, useRoomBackgroundColours, alphaChannel);
            int colDepth = GetSpriteColorDepth(spriteSlot);
            Sprite^ newSprite = gcnew Sprite(spriteSlot, bmp->Width, bmp->Height, colDepth, alphaChannel);
            int roomNumber = GetCurrentlyLoadedRoomNumber();
            if ((colDepth == 8) && (useRoomBackgroundColours) && (roomNumber >= 0))
            {
                newSprite->ColoursLockedToRoom = roomNumber;
            }
            return newSprite;
        }

        void NativeMethods::ReplaceSpriteWithBitmap(Sprite ^spr, Bitmap^ bmp, int destColorDepth, int spriteImportMethod, int transColour, bool remapColours, bool useRoomBackgroundColours, bool alphaChannel)
        {
            SetNewSpriteFromBitmap(spr->Number, bmp, destColorDepth, spriteImportMethod, transColour, remapColours, useRoomBackgroundColours, alphaChannel);
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

		void NativeMethods::ExtractRoomTemplateFiles(String ^templateFileName, int newRoomNumber, List<String^> ^extractedFiles) 
		{
			AGSString fileNameAnsi = TextHelper::ConvertUTF8(templateFileName);
            std::vector<AGSString> files;
            HAGSError err = extract_room_template_files(fileNameAnsi, newRoomNumber, &files);
            for (const auto &file : files)
            {
                extractedFiles->Add(TextHelper::ConvertUTF8(file));
            }
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
            if (name->Equals("CHF_ENABLED")) return CHF_ENABLED;
            if (name->Equals("CHF_VISIBLE")) return CHF_VISIBLE;
            if (name->Equals("DFLG_ON")) return DFLG_ON;
            if (name->Equals("DFLG_NOREPEAT")) return DFLG_NOREPEAT;
            if (name->Equals("DTFLG_SHOWPARSER")) return DTFLG_SHOWPARSER;
            if (name->Equals("FONT_OUTLINE_AUTO")) return FONT_OUTLINE_AUTO;
            if (name->Equals("MAX_STATIC_SPRITES")) return MAX_STATIC_SPRITES;
            if (name->Equals("MAX_PARSER_WORD_LENGTH")) return MAX_PARSER_WORD_LENGTH;
            if (name->Equals("MAX_INV")) return MAX_INV;
            if (name->Equals("MAXLIPSYNCFRAMES")) return MAXLIPSYNCFRAMES;
            if (name->Equals("MAXTOPICOPTIONS")) return MAXTOPICOPTIONS;
            if (name->Equals("UNIFORM_WALK_SPEED")) return safe_cast<Object^>(UNIFORM_WALK_SPEED);
            if (name->Equals("GAME_RESOLUTION_CUSTOM")) return (int)kGameResolution_Custom;
            if (name->Equals("SPRSET_NAME")) return gcnew String(sprsetname);
            if (name->Equals("SPF_KEEPDEPTH")) return SPF_KEEPDEPTH;
            if (name->Equals("PASSWORD_ENC_STRING"))
            {
                int len = (int)strlen(passwencstring);
                array<System::Byte>^ bytes = gcnew array<System::Byte>(len);
                System::Runtime::InteropServices::Marshal::Copy( IntPtr( ( char* ) passwencstring ), bytes, 0, len );
                return bytes;
            }
            if (name->Equals("LOOPFLAG_RUNNEXTLOOP")) return LOOPFLAG_RUNNEXTLOOP;
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
            if (name->Equals("FFLG_REPORTNOMINALHEIGHT")) return FFLG_REPORTNOMINALHEIGHT;
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
            if (name->Equals("OPT_FIXEDINVCURSOR")) return OPT_FIXEDINVCURSOR;
            if (name->Equals("OPT_SPLITRESOURCES")) return OPT_SPLITRESOURCES;
            if (name->Equals("OPT_CHARTURNWHENWALK")) return OPT_CHARTURNWHENWALK;
            if (name->Equals("OPT_FADETYPE")) return OPT_FADETYPE;
            if (name->Equals("OPT_HANDLEINVCLICKS")) return OPT_HANDLEINVCLICKS;
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
            if (name->Equals("OPT_COMPRESSSPRITES")) return OPT_COMPRESSSPRITES;
            if (name->Equals("OPT_RUNGAMEDLGOPTS")) return OPT_RUNGAMEDLGOPTS;
            if (name->Equals("OPT_GLOBALTALKANIMSPD")) return OPT_GLOBALTALKANIMSPD;
            if (name->Equals("OPT_DIALOGOPTIONSAPI")) return OPT_DIALOGOPTIONSAPI;
            if (name->Equals("OPT_BASESCRIPTAPI")) return OPT_BASESCRIPTAPI;
            if (name->Equals("OPT_SCRIPTCOMPATLEV")) return OPT_SCRIPTCOMPATLEV;
            if (name->Equals("OPT_RENDERATSCREENRES")) return OPT_RENDERATSCREENRES;
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
