#pragma once
#include <string.h>
#include "NativeUtils.h"

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
			void NewGameLoaded(Game^ game, List<String^> ^errors);
			void SaveGame(Game^ game);
			void GameSettingsChanged(Game^ game);
			void PaletteColoursUpdated(Game ^game);
			void DrawGUI(int hDC, int x, int y, GUI^ gui, int resolutionFactor, float scale, int selectedControl);
			void DrawSprite(int hDC, int x, int y, int width, int height, int spriteNum, bool flipImage);
			void DrawSprite(int hDC, int x, int y, int spriteNum, bool flipImage);
			// Draws font char sheet on the provided context and returns the height of drawn object;
			// may be called with hDC = 0 to get required height without drawing anything
			int  DrawFont(int hDC, int x, int y, int width, int fontNum);
			void DrawBlockOfColour(int hDC, int x, int y, int width, int height, int colourNum);
			void DrawViewLoop(int hdc, ViewLoop^ loopToDraw, int x, int y, int size, int cursel);
			Sprite^ SetSpriteFromBitmap(int spriteSlot, Bitmap^ bmp, int spriteImportMethod, bool remapColours, bool useRoomBackgroundColours, bool alphaChannel);
			void ReplaceSpriteWithBitmap(Sprite ^spr, Bitmap^ bmp, int spriteImportMethod, bool remapColours, bool useRoomBackgroundColours, bool alphaChannel);
			Bitmap^ GetBitmapForSprite(int spriteSlot, int width, int height);
      Bitmap^ GetBitmapForSpritePreserveColDepth(int spriteSlot);
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
            void OnGameFontUpdated(Game^ game, int fontSlot, bool forceUpdate);
			Dictionary<int,Sprite^>^ LoadAllSpriteDimensions();
			void LoadNewSpriteFile();
            void ReplaceSpriteFile(String ^srcFileName);
            void SaveDefaultRoomFile(Room ^roomToSave);
			String ^LoadRoomScript(String ^roomFileName);
            List<String^>^ GetCompilerExtensions(bool new_compiler);
			void CompileScript(Script ^script, cli::array<String^> ^preProcessedScripts, Game ^game, CompileMessages ^errors);
			GameTemplate^ LoadTemplateFile(String ^fileName);
      RoomTemplate^ LoadRoomTemplateFile(String ^fileName);
			void ExtractTemplateFiles(String ^templateFileName);
		  void ExtractRoomTemplateFiles(String ^templateFileName, int newRoomNumber);
			void UpdateFileIcon(String ^fileToUpdate, String ^iconFileName);
      void UpdateFileVersionInfo(String ^fileToUpdate, cli::array<System::Byte> ^authorNameUnicode, cli::array<System::Byte> ^gameNameUnicode);
			bool HaveSpritesBeenModified();
            Object^ GetNativeConstant(String ^name);
		};

		public ref class SourceCodeControl
		{
		public:
			SourceCodeControl(void);

			bool Initialize(System::String^ dllName, int mainWindowHwnd);
			void Shutdown();
			SourceControlProject^ AddToSourceControl();
			bool OpenProject(SourceControlProject^ project);
			void CloseProject();
			cli::array<AGS::Types::SourceControlFileStatus>^ GetFileStatuses(cli::array<System::String^> ^fileNames);
			void AddFilesToSourceControl(cli::array<System::String^> ^fileNames, System::String ^comment);
			void CheckInFiles(cli::array<System::String^> ^fileNames, System::String ^comment);
			void CheckOutFiles(cli::array<System::String^> ^fileNames, System::String ^comment);
			void RenameFile(System::String ^currentPath, System::String ^newPath);
			void DeleteFiles(cli::array<System::String^> ^fileNames, System::String ^comment);
		};

} // namespace Native 
} // namespace AGS
