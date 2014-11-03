/* AGS Native interface to .NET

Adventure Game Studio Editor Source Code
Copyright (c) 2006-2010 Chris Jones
------------------------------------------------------

The AGS Editor Source Code is provided under the Artistic License 2.0,
see the license.txt for details.
*/
#include "agsnative.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include "NativeMethods.h"
#include "util/string.h"

using namespace System::Runtime::InteropServices;

extern bool initialize_native();
extern void shutdown_native();
extern AGS::Types::Game^ load_old_game_dta_file(const char *fileName);
extern void free_old_game_data();
extern AGS::Types::Room^ load_crm_file(UnloadedRoom ^roomToLoad);
extern void save_crm_file(Room ^roomToSave);
extern const char* import_sci_font(const char*fnn,int fslot);
extern bool reload_font(int curFont);
extern void drawFontAt (int hdc, int fontnum, int x,int y);
extern Dictionary<int, Sprite^>^ load_sprite_dimensions();
extern void drawGUI(int hdc, int x,int y, GUI^ gui, int scaleFactor, int selectedControl);
extern void drawSprite(int hdc, int x,int y, int spriteNum, bool flipImage);
extern void drawSpriteStretch(int hdc, int x,int y, int width, int height, int spriteNum);
extern void drawBlockOfColour(int hdc, int x,int y, int width, int height, int colNum);
extern void drawViewLoop (int hdc, ViewLoop^ loopToDraw, int x, int y, int size, int cursel);
extern void SetNewSpriteFromHBitmap(int slot, int hBmp);
extern int SetNewSpriteFromBitmap(int slot, Bitmap^ bmp, int spriteImportMethod, bool remapColours, bool useRoomBackgroundColours, bool alphaChannel);
extern int GetSpriteAsHBitmap(int spriteSlot);
extern Bitmap^ getSpriteAsBitmap32bit(int spriteNum, int width, int height);
extern Bitmap^ getSpriteAsBitmap(int spriteNum);
extern Bitmap^ getBackgroundAsBitmap(Room ^room, int backgroundNumber);
extern unsigned char* GetRawSpriteData(int spriteSlot);
extern int find_free_sprite_slot();
extern int crop_sprite_edges(int numSprites, int *sprites, bool symmetric);
extern void deleteSprite(int sprslot);
extern int GetSpriteWidth(int slot);
extern int GetSpriteHeight(int slot);
extern int GetRelativeSpriteWidth(int slot);
extern int GetRelativeSpriteHeight(int slot);
extern int GetSpriteColorDepth(int slot);
extern int GetPaletteAsHPalette();
extern bool DoesSpriteExist(int slot);
extern int GetMaxSprites();
extern int GetCurrentlyLoadedRoomNumber();
extern int load_template_file(const char *fileName, char **iconDataBuffer, long *iconDataSize, bool isRoomTemplate);
extern int extract_template_files(const char *templateFileName);
extern int extract_room_template_files(const char *templateFileName, int newRoomNumber);
extern void change_sprite_number(int oldNumber, int newNumber);
extern void update_sprite_resolution(int spriteNum, bool isHighRes);
extern void save_game(bool compressSprites);
extern bool reset_sprite_file();
extern int GetSpriteResolutionMultiplier(int slot);
extern void PaletteUpdated(cli::array<PaletteEntry^>^ newPalette);
extern void GameUpdated(Game ^game);
extern void UpdateSpriteFlags(SpriteFolder ^folder) ;
extern void draw_room_background(void *roomptr, int hdc, int x, int y, int bgnum, float scaleFactor, int maskType, int selectedArea, int maskTransparency);
extern void ImportBackground(Room ^room, int backgroundNumber, Bitmap ^bmp, bool useExactPalette, bool sharePalette);
extern void DeleteBackground(Room ^room, int backgroundNumber);
extern void CreateBuffer(int width, int height);
extern void RenderBufferToHDC(int hdc);
extern void DrawSpriteToBuffer(int sprNum, int x, int y, int scaleFactor);
extern void draw_line_onto_mask(void *roomptr, int maskType, int x1, int y1, int x2, int y2, int color);
extern void draw_filled_rect_onto_mask(void *roomptr, int maskType, int x1, int y1, int x2, int y2, int color);
extern void draw_fill_onto_mask(void *roomptr, int maskType, int x1, int y1, int color);
extern void copy_walkable_to_regions(void *roomptr);
extern int get_mask_pixel(void *roomptr, int maskType, int x, int y);
extern void import_area_mask(void *roomptr, int maskType, Bitmap ^bmp);
extern void create_undo_buffer(void *roomptr, int maskType) ;
extern bool does_undo_buffer_exist();
extern void clear_undo_buffer() ;
extern void restore_from_undo_buffer(void *roomptr, int maskType);
extern System::String ^load_room_script(System::String ^fileName);
extern void transform_string(char *text);
extern bool enable_greyed_out_masks;
extern bool spritesModified;

char editorVersionNumber[50];

void ConvertStringToCharArray(System::String^ clrString, char *textBuffer)
{
	char* stringPointer = (char*)Marshal::StringToHGlobalAnsi(clrString).ToPointer();
	
	strcpy(textBuffer, stringPointer);

  Marshal::FreeHGlobal(IntPtr(stringPointer));
}

void ConvertFileNameToCharArray(System::String^ clrString, char *textBuffer)
{
  ConvertStringToCharArray(clrString, textBuffer);
  if (strchr(textBuffer, '?') != NULL)
  {
    throw gcnew AGSEditorException(String::Format("Filename contains invalid unicode characters: {0}", clrString));
  }
}

void ConvertStringToNativeString(System::String^ clrString, AGS::Common::String &destStr)
{
    char* stringPointer = (char*)Marshal::StringToHGlobalAnsi(clrString).ToPointer();

    destStr = stringPointer;

    Marshal::FreeHGlobal(IntPtr(stringPointer));
}

void ConvertStringToNativeString(System::String^ clrString, AGS::Common::String &destStr, int maxLength)
{
    if (clrString->Length >= maxLength) 
    {
        throw gcnew AGSEditorException(String::Format("String is too long: {0} (max length={1})", clrString, maxLength - 1));
    }
    char* stringPointer = (char*)System::Runtime::InteropServices::Marshal::StringToHGlobalAnsi(clrString).ToPointer();

    destStr = stringPointer;

    System::Runtime::InteropServices::Marshal::FreeHGlobal(IntPtr(stringPointer));
}

void ConvertStringToCharArray(System::String^ clrString, char *textBuffer, int maxLength)
{
	if (clrString->Length >= maxLength) 
	{
		throw gcnew AGSEditorException(String::Format("String is too long: {0} (max length={1})", clrString, maxLength - 1));
	}
	char* stringPointer = (char*)System::Runtime::InteropServices::Marshal::StringToHGlobalAnsi(clrString).ToPointer();
	
	strcpy(textBuffer, stringPointer);

    System::Runtime::InteropServices::Marshal::FreeHGlobal(IntPtr(stringPointer));
}

namespace AGS
{
	namespace Native
	{
		NativeMethods::NativeMethods(String ^editorVersion)
		{
			lastPaletteSet = nullptr;
			ConvertStringToCharArray(editorVersion, editorVersionNumber);
		}

		void NativeMethods::Initialize()
		{
			if (!initialize_native())
			{
				throw gcnew AGS::Types::InvalidDataException("Native initialization failed.");
			}
		}

		void NativeMethods::NewGameLoaded(Game ^game)
		{
			this->PaletteColoursUpdated(game);
			GameUpdated(game);
			UpdateSpriteFlags(game->RootSpriteFolder);
		}

		void NativeMethods::PaletteColoursUpdated(Game ^game)
		{
			lastPaletteSet = game->Palette;
			PaletteUpdated(game->Palette);
		}

		void NativeMethods::LoadNewSpriteFile() 
		{
			if (!reset_sprite_file())
			{
				throw gcnew AGSEditorException("Unable to load the sprite file ACSPRSET.SPR. The file may be missing, corrupt or it may require a newer version of AGS.");
			}
		}

		void NativeMethods::SaveGame(Game ^game)
		{
			save_game(game->Settings->CompressSprites);
		}

		void NativeMethods::GameSettingsChanged(Game ^game)
		{
			GameUpdated(game);
		}

		void NativeMethods::DrawGUI(int hDC, int x, int y, GUI^ gui, int scaleFactor, int selectedControl)
		{
			drawGUI(hDC, x, y, gui, scaleFactor, selectedControl);
		}

		void NativeMethods::DrawSprite(int hDC, int x, int y, int spriteNum, bool flipImage)
		{
			drawSprite(hDC, x, y, spriteNum, flipImage);
		}

		void NativeMethods::DrawFont(int hDC, int x, int y, int fontNum)
		{
			drawFontAt(hDC, fontNum, x, y);
		}

		void NativeMethods::DrawSprite(int hDC, int x, int y, int width, int height, int spriteNum)
		{
			drawSpriteStretch(hDC, x, y, width, height, spriteNum);
		}

		void NativeMethods::DrawBlockOfColour(int hDC, int x, int y, int width, int height, int colourNum)
		{
			drawBlockOfColour(hDC, x, y, width, height, colourNum);
		}

		void NativeMethods::DrawViewLoop(int hdc, ViewLoop^ loopToDraw, int x, int y, int size, int cursel)
		{
			drawViewLoop(hdc, loopToDraw, x, y, size, cursel);
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
			char fileNameBuf[MAX_PATH];
      ConvertFileNameToCharArray(fileName, fileNameBuf);
			const char *errorMsg = import_sci_font(fileNameBuf, fontSlot);
			if (errorMsg != NULL) 
			{
				throw gcnew AGSEditorException(gcnew String(errorMsg));
			}
		}

    void NativeMethods::ReloadTTFFont(int fontSlot)
    {
      if (!reload_font(fontSlot))
      {
        throw gcnew AGSEditorException("Unable to load the TTF font file. The renderer was unable to load the font.");
      }
    }

		// Gets sprite height in 320x200-res co-ordinates
		int NativeMethods::GetRelativeSpriteHeight(int spriteSlot) 
		{
			return ::GetRelativeSpriteHeight(spriteSlot);
		}

		// Gets sprite width in 320x200-res co-ordinates
		int NativeMethods::GetRelativeSpriteWidth(int spriteSlot) 
		{
			return ::GetRelativeSpriteWidth(spriteSlot);
		}

		int NativeMethods::GetActualSpriteWidth(int spriteSlot) 
		{
			return ::GetSpriteWidth(spriteSlot);
		}

		int NativeMethods::GetActualSpriteHeight(int spriteSlot) 
		{
			return ::GetSpriteHeight(spriteSlot);
		}

		int NativeMethods::GetSpriteResolutionMultiplier(int spriteSlot)
		{
			return ::GetSpriteResolutionMultiplier(spriteSlot);
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
				update_sprite_resolution(sprite->Number, sprite->Resolution == SpriteImportResolution::HighRes);
			}
		}

		Sprite^ NativeMethods::SetSpriteFromBitmap(int spriteSlot, Bitmap^ bmp, int spriteImportMethod, bool remapColours, bool useRoomBackgroundColours, bool alphaChannel)
		{
			int spriteRes = SetNewSpriteFromBitmap(spriteSlot, bmp, spriteImportMethod, remapColours, useRoomBackgroundColours, alphaChannel);
      int colDepth = GetSpriteColorDepth(spriteSlot);
			Sprite^ newSprite = gcnew Sprite(spriteSlot, bmp->Width, bmp->Height, colDepth, (SpriteImportResolution)spriteRes, alphaChannel);
      int roomNumber = GetCurrentlyLoadedRoomNumber();
      if ((colDepth == 8) && (useRoomBackgroundColours) && (roomNumber >= 0))
      {
        newSprite->ColoursLockedToRoom = roomNumber;
      }
      return newSprite;
		}

		void NativeMethods::ReplaceSpriteWithBitmap(Sprite ^spr, Bitmap^ bmp, int spriteImportMethod, bool remapColours, bool useRoomBackgroundColours, bool alphaChannel)
		{
			int spriteRes = SetNewSpriteFromBitmap(spr->Number, bmp, spriteImportMethod, remapColours, useRoomBackgroundColours, alphaChannel);
			spr->Resolution = (SpriteImportResolution)spriteRes;
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

		Bitmap^ NativeMethods::GetBitmapForSprite(int spriteSlot, int width, int height)
		{
/*			int spriteWidth = GetSpriteWidth(spriteSlot);
			int spriteHeight = GetSpriteHeight(spriteSlot);
			int colDepth = GetSpriteColorDepth(spriteSlot);
/*			int stride = spriteWidth * ((colDepth + 1) / 8);
			PixelFormat pFormat;
			if (colDepth == 8)
			{
				pFormat = PixelFormat::Format8bppIndexed;
			}
			else if (colDepth == 15)
			{
				pFormat = PixelFormat::Format16bppRgb555;
			}
			else if (colDepth == 16)
			{
				pFormat = PixelFormat::Format16bppRgb565;
			}
			else
			{
				pFormat = PixelFormat::Format32bppRgb;
			}

			unsigned char *spriteData = GetRawSpriteData(spriteSlot);
			/*IntPtr intPtr(spriteData);
			Bitmap ^newBitmap = gcnew Bitmap(spriteWidth, spriteHeight, stride, pFormat, intPtr);*/
/*			Bitmap ^newBitmap = gcnew Bitmap(spriteWidth, spriteHeight, pFormat);
			System::Drawing::Rectangle rect(0, 0, spriteWidth, spriteHeight);
			BitmapData ^bmpData = newBitmap->LockBits(rect, ImageLockMode::WriteOnly, pFormat);
			memcpy(bmpData->Scan0.ToPointer(), spriteData, stride * spriteHeight);
			newBitmap->UnlockBits(bmpData);

			if (pFormat == PixelFormat::Format8bppIndexed)
			{
				for each (PaletteEntry^ palEntry in lastPaletteSet)
				{
					newBitmap->Palette->Entries[palEntry->Index] = palEntry->Colour;
				}
				newBitmap->Palette = newBitmap->Palette;
			}* /
			int hBmp = GetSpriteAsHBitmap(spriteSlot);
			Bitmap^ newBitmap;
			if (GetSpriteColorDepth(spriteSlot) == 8) 
			{
				int hPal = GetPaletteAsHPalette();
				newBitmap = Bitmap::FromHbitmap((IntPtr)hBmp, (IntPtr)hPal);
				DeleteObject((HPALETTE)hPal);
			}
			else
			{
				newBitmap = Bitmap::FromHbitmap((IntPtr)hBmp);
			}
			DeleteObject((HBITMAP)hBmp);
			return newBitmap;
*/
			return getSpriteAsBitmap32bit(spriteSlot, width, height);
		}

		Bitmap^ NativeMethods::GetBitmapForSpritePreserveColDepth(int spriteSlot)
		{
      return getSpriteAsBitmap(spriteSlot);
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
			int *spriteSlotList = new int[sprites->Count];
			int i = 0;
			for each (Sprite^ sprite in sprites)
			{
				spriteSlotList[i] = sprite->Number;
				i++;
			}
			bool result = crop_sprite_edges(sprites->Count, spriteSlotList, symmetric) != 0;
			delete spriteSlotList;

			int newWidth = GetSpriteWidth(sprites[0]->Number);
			int newHeight = GetSpriteHeight(sprites[0]->Number);
			for each (Sprite^ sprite in sprites)
			{
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
			char fileNameBuf[MAX_PATH];
			ConvertFileNameToCharArray(fileName, fileNameBuf);

			Game ^game = load_old_game_dta_file(fileNameBuf);

			return game;
		}

		Dictionary<int,Sprite^>^ NativeMethods::LoadAllSpriteDimensions()
		{
			return load_sprite_dimensions();
		}

		AGS::Types::Room^ NativeMethods::LoadRoomFile(UnloadedRoom^ roomToLoad)
		{
			return load_crm_file(roomToLoad);
		}

		void NativeMethods::SaveRoomFile(AGS::Types::Room ^roomToSave)
		{
			save_crm_file(roomToSave);
		}

		void NativeMethods::CreateBuffer(int width, int height) 
		{
			::CreateBuffer(width, height);
		}

		void NativeMethods::DrawSpriteToBuffer(int sprNum, int x, int y, int scaleFactor) 
		{
			::DrawSpriteToBuffer(sprNum, x, y, scaleFactor);
		}

		void NativeMethods::RenderBufferToHDC(int hDC) 
		{
			::RenderBufferToHDC(hDC);
		}

		void NativeMethods::DrawRoomBackground(int hDC, Room ^room, int x, int y, int backgroundNumber, float scaleFactor, RoomAreaMaskType maskType, int selectedArea, int maskTransparency)
		{
			draw_room_background((void*)room->_roomStructPtr, hDC, x, y, backgroundNumber, scaleFactor, (int)maskType, selectedArea, maskTransparency);
		}

		void NativeMethods::ImportBackground(Room ^room, int backgroundNumber, Bitmap ^bmp, bool useExactPalette, bool sharePalette)
		{
			::ImportBackground(room, backgroundNumber, bmp, useExactPalette, sharePalette);
		}

		void NativeMethods::DeleteBackground(Room ^room, int backgroundNumber)
		{
			::DeleteBackground(room, backgroundNumber);
		}

		Bitmap^ NativeMethods::GetBitmapForBackground(Room ^room, int backgroundNumber)
		{
			return getBackgroundAsBitmap(room, backgroundNumber);
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

		void NativeMethods::CopyWalkableMaskToRegions(Room ^room) 
		{
			copy_walkable_to_regions((void*)room->_roomStructPtr);
		}

		int NativeMethods::GetAreaMaskPixel(Room ^room, RoomAreaMaskType maskType, int x, int y)
		{
			return get_mask_pixel((void*)room->_roomStructPtr, (int)maskType, x, y);
		}

    void NativeMethods::ImportAreaMask(Room ^room, RoomAreaMaskType maskType, Bitmap ^bmp)
    {
      import_area_mask((void*)room->_roomStructPtr, (int)maskType, bmp);
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
      char fileNameBuf[MAX_PATH];
			ConvertFileNameToCharArray(fileName, fileNameBuf);
      char *iconDataBuffer = NULL;
      long iconDataSize = 0;

      int success = load_template_file(fileNameBuf, &iconDataBuffer, &iconDataSize, isRoomTemplate);
			if (success) 
			{
				Icon ^icon = nullptr;
				if (iconDataBuffer != NULL)
				{
          cli::array<unsigned char>^ managedArray = gcnew cli::array<unsigned char>(iconDataSize);
          Marshal::Copy(IntPtr(iconDataBuffer), managedArray, 0, iconDataSize);
          ::free(iconDataBuffer);
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
				  return gcnew GameTemplate(fileName, icon);
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
			char fileNameBuf[MAX_PATH];
			ConvertFileNameToCharArray(templateFileName, fileNameBuf);

			if (!extract_template_files(fileNameBuf))
			{
				throw gcnew AGSEditorException("Unable to extract template files.");
			}
		}

		void NativeMethods::ExtractRoomTemplateFiles(String ^templateFileName, int newRoomNumber) 
		{
			char fileNameBuf[MAX_PATH];
			ConvertFileNameToCharArray(templateFileName, fileNameBuf);

			if (!extract_room_template_files(fileNameBuf, newRoomNumber))
			{
				throw gcnew AGSEditorException("Unable to extract template files.");
			}
		}
				
		cli::array<unsigned char>^ NativeMethods::TransformStringToBytes(String ^text) 
		{
			char* stringPointer = (char*)Marshal::StringToHGlobalAnsi(text).ToPointer();
			int textLength = text->Length + 1;
			cli::array<unsigned char>^ toReturn = gcnew cli::array<unsigned char>(textLength + 4);
			toReturn[0] = textLength % 256;
			toReturn[1] = textLength / 256;
			toReturn[2] = 0;
			toReturn[3] = 0;
	
			transform_string(stringPointer);

			{ pin_ptr<unsigned char> nativeBytes = &toReturn[4];
				memcpy(nativeBytes, stringPointer, textLength);
			}

			Marshal::FreeHGlobal(IntPtr(stringPointer));

			return toReturn;
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
            if (name->Equals("GAME_FILE_SIG")) return gcnew String(game_file_sig);
            if (name->Equals("GAME_DATA_VERSION_CURRENT")) return (int)kGameVersion_Current;
            if (name->Equals("MAX_GUID_LENGTH")) return MAX_GUID_LENGTH;
            if (name->Equals("MAX_SG_EXT_LENGTH")) return MAX_SG_EXT_LENGTH;
            if (name->Equals("MAX_SG_FOLDER_LEN")) return MAX_SG_FOLDER_LEN;
            if (name->Equals("MAX_SCRIPT_NAME_LEN")) return MAX_SCRIPT_NAME_LEN;
            if (name->Equals("FFLG_SIZEMASK")) return FFLG_SIZEMASK;
            if (name->Equals("IFLG_STARTWITH")) return IFLG_STARTWITH;
            if (name->Equals("MCF_ANIMMOVE")) return MCF_ANIMMOVE;
            if (name->Equals("MCF_STANDARD")) return MCF_STANDARD;
            if (name->Equals("MCF_HOTSPOT")) return MCF_HOTSPOT;
            if (name->Equals("CHF_MANUALSCALING")) return CHF_MANUALSCALING;
            if (name->Equals("CHF_NOINTERACT")) return CHF_NOINTERACT;
            if (name->Equals("CHF_NODIAGONAL")) return CHF_NODIAGONAL;
            if (name->Equals("CHF_NOLIGHTING")) return CHF_NOLIGHTING;
            if (name->Equals("CHF_NOTURNING")) return CHF_NOTURNING;
            if (name->Equals("CHF_NOBLOCKING")) return CHF_NOBLOCKING;
            if (name->Equals("CHF_SCALEMOVESPEED")) return CHF_SCALEMOVESPEED;
            if (name->Equals("CHF_SCALEVOLUME")) return CHF_SCALEVOLUME;
            if (name->Equals("CHF_ANTIGLIDE")) return CHF_ANTIGLIDE;
            if (name->Equals("DFLG_ON")) return DFLG_ON;
            if (name->Equals("DFLG_NOREPEAT")) return DFLG_NOREPEAT;
            if (name->Equals("DTFLG_SHOWPARSER")) return DTFLG_SHOWPARSER;
            if (name->Equals("FONT_OUTLINE_AUTO")) return FONT_OUTLINE_AUTO;
            if (name->Equals("MAX_FONTS")) return MAX_FONTS;
            if (name->Equals("MAX_SPRITES")) return MAX_SPRITES;
            if (name->Equals("MAX_CURSOR")) return MAX_CURSOR;
            if (name->Equals("MAX_PARSER_WORD_LENGTH")) return MAX_PARSER_WORD_LENGTH;
            if (name->Equals("MAX_INV")) return MAX_INV;
            if (name->Equals("MAXLIPSYNCFRAMES")) return MAXLIPSYNCFRAMES;
            if (name->Equals("MAXGLOBALMES")) return MAXGLOBALMES;
            if (name->Equals("MAXTOPICOPTIONS")) return MAXTOPICOPTIONS;
            if (name->Equals("UNIFORM_WALK_SPEED")) return UNIFORM_WALK_SPEED;
            if (name->Equals("GAME_RESOLUTION_CUSTOM")) return (int)kGameResolution_Custom;
            if (name->Equals("MAXMULTIFILES")) return MAXMULTIFILES;
            if (name->Equals("RAND_SEED_SALT")) return RAND_SEED_SALT;
            if (name->Equals("CHUNKSIZE")) return CHUNKSIZE;
            if (name->Equals("CLIB_END_SIGNATURE")) return gcnew String(clibendsig);
            if (name->Equals("MAX_FILENAME_LENGTH")) return MAX_FILENAME_LENGTH;
            if (name->Equals("SPRSET_NAME")) return gcnew String(sprsetname);
            if (name->Equals("SPF_640x400")) return SPF_640x400;
            if (name->Equals("SPF_ALPHACHANNEL")) return SPF_ALPHACHANNEL;
            if (name->Equals("MAX_CUSTOM_PROPERTIES")) return MAX_CUSTOM_PROPERTIES;
            if (name->Equals("MAX_CUSTOM_PROPERTY_NAME_LENGTH")) return MAX_CUSTOM_PROPERTY_NAME_LENGTH;
            if (name->Equals("MAX_CUSTOM_PROPERTY_VALUE_LENGTH")) return MAX_CUSTOM_PROPERTY_VALUE_LENGTH;
            if (name->Equals("PASSWORD_ENC_STRING")) return gcnew String(passwencstring);
            if (name->Equals("LOOPFLAG_RUNNEXTLOOP")) return LOOPFLAG_RUNNEXTLOOP;
            if (name->Equals("VFLG_FLIPSPRITE")) return VFLG_FLIPSPRITE;
            if (name->Equals("GUIMAGIC")) return GUIMAGIC;
            if (name->Equals("SAVEBUFFERSIZE")) return SAVEBUFFERSIZE;
            if (name->Equals("GUIMAIN_NOCLICK")) return (int)Common::kGUIMain_NoClick;
            if (name->Equals("GUIF_CLIP")) return GUIF_CLIP;
            if (name->Equals("GUIF_TRANSLATED")) return GUIF_TRANSLATED;
            if (name->Equals("GLF_NOBORDER")) return GLF_NOBORDER;
            if (name->Equals("GLF_NOARROWS")) return GLF_NOARROWS;
            if (name->Equals("GUI_POPUP_MODAL")) return (int)Common::kGUIPopupModal;
            if (name->Equals("GUIMAIN_TEXTWINDOW")) return (int)Common::kGUIMain_TextWindow;
            if (name->Equals("GUIMAIN_LEGACYTEXTWINDOW")) return (int)Common::kGUIMain_LegacyTextWindow;
            if (name->Equals("GTF_NOBORDER")) return GTF_NOBORDER;
            if (name->Equals("MAX_GUILABEL_TEXT_LEN")) return MAX_GUILABEL_TEXT_LEN;
            if (name->Equals("MAX_GUIOBJ_SCRIPTNAME_LEN")) return MAX_GUIOBJ_SCRIPTNAME_LEN;
            if (name->Equals("MAX_GUIOBJ_EVENTHANDLER_LEN")) return MAX_GUIOBJ_EVENTHANDLER_LEN;
            if (name->Equals("GOBJ_BUTTON")) return (int)Common::kGUIButton;
            if (name->Equals("GOBJ_LABEL")) return (int)Common::kGUILabel;
            if (name->Equals("GOBJ_INVENTORY")) return (int)Common::kGUIInvWindow;
            if (name->Equals("GOBJ_SLIDER")) return (int)Common::kGUISlider;
            if (name->Equals("GOBJ_TEXTBOX")) return (int)Common::kGUITextBox;
            if (name->Equals("GOBJ_LISTBOX")) return (int)Common::kGUIListBox;
            if (name->Equals("TEXTWINDOW_PADDING_DEFAULT")) return TEXTWINDOW_PADDING_DEFAULT;
            if (name->Equals("GUI_VERSION_CURRENT")) return (int)kGuiVersion_Current;
            if (name->Equals("OPT_DEBUGMODE")) return OPT_DEBUGMODE;
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
            if (name->Equals("OPT_NOSCALEFNT")) return OPT_NOSCALEFNT;
            if (name->Equals("OPT_SPLITRESOURCES")) return OPT_SPLITRESOURCES;
            if (name->Equals("OPT_ROTATECHARS")) return OPT_ROTATECHARS;
            if (name->Equals("OPT_FADETYPE")) return OPT_FADETYPE;
            if (name->Equals("OPT_HANDLEINVCLICKS")) return OPT_HANDLEINVCLICKS;
            if (name->Equals("OPT_MOUSEWHEEL")) return OPT_MOUSEWHEEL;
            if (name->Equals("OPT_DIALOGNUMBERED")) return OPT_DIALOGNUMBERED;
            if (name->Equals("OPT_DIALOGUPWARDS")) return OPT_DIALOGUPWARDS;
            if (name->Equals("OPT_ANTIALIASFONTS")) return OPT_ANTIALIASFONTS;
            if (name->Equals("OPT_THOUGHTGUI")) return OPT_THOUGHTGUI;
            if (name->Equals("OPT_TURNTOFACELOC")) return OPT_TURNTOFACELOC;
            if (name->Equals("OPT_RIGHTLEFTWRITE")) return OPT_RIGHTLEFTWRITE;
            if (name->Equals("OPT_DUPLICATEINV")) return OPT_DUPLICATEINV;
            if (name->Equals("OPT_SAVESCREENSHOT")) return OPT_SAVESCREENSHOT;
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
            if (name->Equals("OPT_LIPSYNCTEXT")) return OPT_LIPSYNCTEXT;
            return nullptr;
        }
	}
}