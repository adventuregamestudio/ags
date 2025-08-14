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
#pragma once
#include <string.h>
#include "NativeUtils.h"
#include "IScriptCompiler.h"

using namespace AGS::Types;
using namespace System;
using namespace System::Collections::Generic;
using namespace System::Drawing;
using namespace System::Drawing::Imaging;
using namespace System::IO;

namespace AGS
{
	namespace Native 
	{
		public ref class NativeMethods
		{
		private:
			cli::array<PaletteEntry^>^ lastPaletteSet;
            static TextConverter^ _gameTextConverter;

      BaseTemplate^ LoadTemplateFile(String ^fileName, bool isRoomTemplate);
      void FindAndUpdateMemory(unsigned char *data, int dataLen, const unsigned char *searchFor, int searchForLen, const unsigned char *replaceWith);
      void ReplaceStringInMemory(unsigned char *memory, int memorySize, const char *searchFor, const unsigned char *replaceWithData);

		public:
			NativeMethods(String ^version);

            static TextConverter^ GetGameTextConverter();

			void Initialize();
            void NewWorkingDirSet(String^ workingDir);
			void NewGameLoaded(Game^ game, CompileMessages ^errors);
			void SaveGame(Game^ game);
			void GameSettingsChanged(Game^ game);
			void PaletteColoursUpdated(Game ^game);
			void DrawGUI(int hDC, int x, int y, GUI^ gui, int resolutionFactor, float scale, int controlTransparency, int selectedControl);
			void DrawSprite(int hDC, int x, int y, int width, int height, int spriteNum, bool flipImage);
			void DrawSprite(int hDC, int x, int y, int spriteNum, bool flipImage);
			// Draws font char sheet on the provided context and returns the height of drawn object;
			// may be called with hDC = 0 to get required height without drawing anything
			int  DrawFont(int hDC, int fontNum, int draw_atx, int draw_aty, int width, int height, int scroll_y);
            void DrawTextUsingFont(int hDC, String ^text, int fontNum,
                int dc_atx, int dc_aty, int dc_width, int dc_height,
                int text_atx, int text_aty, int max_width);
			void DrawBlockOfColour(int hDC, int x, int y, int width, int height, int colourNum);
			void DrawViewLoop(int hdc, ViewLoop^ loopToDraw, int x, int y, int size, List<int>^ cursel);
			Sprite^ SetSpriteFromBitmap(int spriteSlot, Bitmap^ bmp, int spriteImportMethod, int transColour, bool remapColours, bool useRoomBackgroundColours, bool alphaChannel);
			void ReplaceSpriteWithBitmap(Sprite ^spr, Bitmap^ bmp, int spriteImportMethod, int transColour, bool remapColours, bool useRoomBackgroundColours, bool alphaChannel);
            Bitmap^ GetSpriteBitmap(int spriteSlot);
            Bitmap^ GetSpriteBitmapAs32Bit(int spriteSlot, int width, int height);
			void DeleteSprite(int spriteSlot);
			int  GetFreeSpriteSlot();
            Types::SpriteInfo^ GetSpriteInfo(int spriteSlot);
			int  GetSpriteWidth(int spriteSlot);
			int  GetSpriteHeight(int spriteSlot);
            Drawing::Size GetMaxSpriteSize(array<int>^ sprites, [Runtime::InteropServices::Out] bool% hasLowResSprites);
			bool CropSpriteEdges(System::Collections::Generic::IList<Sprite^>^ sprites, bool symmetric);
			bool DoesSpriteExist(int spriteNumber);
			void ChangeSpriteNumber(Sprite^ sprite, int newNumber);
			void SpriteResolutionsChanged(cli::array<Sprite^>^ sprites);
			void Shutdown();
			Game^ ImportOldGameFile(String^ fileName);
			void ImportSCIFont(String ^fileName, int fontSlot);
            void ReloadFont(int fontSlot);
            // Measures the TTF font from the given file, and tries to find a point size corresponding
            // to the closest pixel height match, returns the found point size, or 0 in case of error.
            int FindTTFSizeForHeight(String ^fileName, int size);
            void OnGameFontAdded(Game^ game, int fontSlot);
            void OnGameFontDeleted(Game^ game, int fontSlot);
            void OnGameFontUpdated(Game^ game, int fontSlot, bool forceUpdate);
			Dictionary<int,Sprite^>^ LoadAllSpriteDimensions();
			void LoadNewSpriteFile();
            void ReplaceSpriteFile(String ^srcFileName);
			Room^ LoadRoomFile(UnloadedRoom ^roomToLoad, System::Text::Encoding ^defEncoding);
			void SaveRoomFile(Room ^roomToSave);
            void SaveDefaultRoomFile(Room ^roomToSave);
			void DrawRoomBackground(int hDC, Room ^room, int x, int y, int backgroundNumber, float scaleFactor, RoomAreaMaskType maskType, int selectedArea, int maskTransparency);
			void ImportBackground(Room ^room, int backgroundNumber, Bitmap ^bmp, bool useExactPalette, bool sharePalette);
			void DeleteBackground(Room ^room, int backgroundNumber);
			Bitmap^ GetRoomBackgroundForPreview(Room ^room, int backgroundNumber);
            Bitmap^ ExportRoomBackground(Room ^room, int backgroundNumber);
            void AdjustRoomResolution(Room ^room);
            void AdjustRoomMaskResolution(Room ^room);
			void DrawLineOntoMask(Room ^room, RoomAreaMaskType maskType, int x1, int y1, int x2, int y2, int color);
			void DrawFilledRectOntoMask(Room ^room, RoomAreaMaskType maskType, int x1, int y1, int x2, int y2, int color);
			void DrawFillOntoMask(Room ^room, RoomAreaMaskType maskType, int x1, int y1, int color);
			int  GetAreaMaskPixel(Room ^room, RoomAreaMaskType maskType, int x, int y);
      void ImportAreaMask(Room ^room, RoomAreaMaskType maskType, Bitmap ^bmp);
      Bitmap ^ExportAreaMask(Room ^room, RoomAreaMaskType maskType);
      void CreateUndoBuffer(Room ^room, RoomAreaMaskType maskType);
      bool DoesUndoBufferExist();
      void ClearUndoBuffer();
      void RestoreFromUndoBuffer(Room ^room, RoomAreaMaskType maskType);
      void SetGreyedOutMasksEnabled(bool enabled);
			void CreateBuffer(int width, int height) ;
			void DrawSpriteToBuffer(int sprNum, int x, int y, float scale) ;
			void RenderBufferToHDC(int hDC) ;
			String ^LoadRoomScript(String ^roomFileName);
            // Returns a list of IScriptCompiler implementations embedded in AGS.Native
            List<IScriptCompiler^>^ GetEmbeddedScriptCompilers();
			GameTemplate^ LoadTemplateFile(String ^fileName);
      RoomTemplate^ LoadRoomTemplateFile(String ^fileName);
			void ExtractTemplateFiles(String ^templateFileName);
		  void ExtractRoomTemplateFiles(String ^templateFileName, int newRoomNumber);
			void UpdateFileIcon(String ^fileToUpdate, String ^iconFileName);
      void UpdateFileVersionInfo(String ^fileToUpdate, cli::array<System::Byte> ^authorNameUnicode, cli::array<System::Byte> ^gameNameUnicode);
			bool HaveSpritesBeenModified();
            Object^ GetNativeConstant(String ^name);
            void ReadIniFile(String ^fileName, Dictionary<String^, Dictionary<String^, String^>^>^ sections);
            void WriteIniFile(String ^fileName, Dictionary<String^, Dictionary<String^, String^>^>^ sections, bool mergeExisting);
		};
	}
}
