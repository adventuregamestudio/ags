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
#pragma once
#include <string.h>
#include "NativeUtils.h"
#include "IScriptCompiler.h"

using namespace AGS::Types;
using namespace System;
using namespace System::Collections::Generic;
using namespace System::Drawing::Imaging;
using namespace System::IO;

namespace AGS
{
namespace Native 
{
    using Bitmap = System::Drawing::Bitmap;

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
			void ApplyPalette(cli::array<PaletteEntry^> ^palette);
			void DrawGUI(int hDC, int x, int y, GUI^ gui, int resolutionFactor, float scale, int controlTransparency, int selectedControl);
			void DrawSprite(int hDC, int x, int y, int width, int height, int spriteNum, bool flipImage);
			void DrawSprite(int hDC, int x, int y, int spriteNum, bool flipImage);
			// Draws font char sheet on the provided context and returns the height of drawn object;
			// may be called with hDC = 0 to get required height without drawing anything
			int  DrawFont(int hDC, int x, int y, int width, int fontNum);
			void DrawViewLoop(int hdc, ViewLoop^ loopToDraw, int x, int y, int size, List<int>^ cursel);
			Sprite^ SetSpriteFromBitmap(int spriteSlot, Bitmap^ bmp, int destColorDepth, int spriteImportMethod, bool remapColours, bool useRoomBackgroundColours, bool alphaChannel);
			void ReplaceSpriteWithBitmap(Sprite ^spr, Bitmap^ bmp, int destColorDepth, int spriteImportMethod, bool remapColours, bool useRoomBackgroundColours, bool alphaChannel);
            Bitmap^ GetSpriteBitmap(int spriteSlot);
            Bitmap^ GetSpriteBitmapAs32Bit(int spriteSlot, int width, int height);
			void DeleteSprite(int spriteSlot);
			int  GetFreeSpriteSlot();
            Types::SpriteInfo^ GetSpriteInfo(int spriteSlot);
			int  GetSpriteWidth(int spriteSlot);
			int  GetSpriteHeight(int spriteSlot);
            Drawing::Size GetMaxSpriteSize(array<int>^ sprites);
			bool CropSpriteEdges(System::Collections::Generic::IList<Sprite^>^ sprites, bool symmetric);
			bool DoesSpriteExist(int spriteNumber);
			void ChangeSpriteNumber(Sprite^ sprite, int newNumber);
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
			String ^LoadRoomScript(String ^roomFileName);
            // Returns a list of IScriptCompiler implementations embedded in AGS.Native
            List<IScriptCompiler^>^ GetEmbeddedScriptCompilers();
			GameTemplate^ LoadTemplateFile(String ^fileName);
      RoomTemplate^ LoadRoomTemplateFile(String ^fileName);
			void ExtractTemplateFiles(String ^templateFileName);
		  void ExtractRoomTemplateFiles(String ^templateFileName, int newRoomNumber, List<String^> ^extractedFiles);
			void UpdateFileIcon(String ^fileToUpdate, String ^iconFileName);
      void UpdateFileVersionInfo(String ^fileToUpdate, cli::array<System::Byte> ^authorNameUnicode, cli::array<System::Byte> ^gameNameUnicode);
			bool HaveSpritesBeenModified();
            Object^ GetNativeConstant(String ^name);
            void ReadIniFile(String ^fileName, Dictionary<String^, Dictionary<String^, String^>^>^ sections);
            void WriteIniFile(String ^fileName, Dictionary<String^, Dictionary<String^, String^>^>^ sections, bool mergeExisting);
		};
} // namespace Native 
} // namespace AGS
