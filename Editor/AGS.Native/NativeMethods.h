#pragma once
#include <string.h>

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
      void UpdateResourceInFile(String ^fileToUpdate, const char *resourceName, cli::array<System::Byte> ^newData);
      BaseTemplate^ LoadTemplateFile(String ^fileName, bool isRoomTemplate);
      void FindAndUpdateMemory(unsigned char *data, int dataLen, const unsigned char *searchFor, int searchForLen, const unsigned char *replaceWith);
      void ReplaceStringInMemory(unsigned char *memory, int memorySize, const char *searchFor, const unsigned char *replaceWithData);

		public:
			NativeMethods(String ^version);

			void Initialize();
			void NewGameLoaded(Game^ game);
			void SaveGame(Game^ game);
			void GameSettingsChanged(Game^ game);
			void PaletteColoursUpdated(Game ^game);
			void DrawGUI(int hDC, int x, int y, GUI^ gui, int scaleFactor, int selectedControl);
			void DrawSprite(int hDC, int x, int y, int width, int height, int spriteNum);
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
			int  GetSpriteWidth(int spriteSlot);
			int  GetSpriteHeight(int spriteSlot);
            int  GetResolutionMultiplier();
			bool CropSpriteEdges(System::Collections::Generic::IList<Sprite^>^ sprites, bool symmetric);
			bool DoesSpriteExist(int spriteNumber);
			void ChangeSpriteNumber(Sprite^ sprite, int newNumber);
			void SpriteResolutionsChanged(cli::array<Sprite^>^ sprites);
			void Shutdown();
			Game^ ImportOldGameFile(String^ fileName);
			void ImportSCIFont(String ^fileName, int fontSlot);
      void ReloadTTFFont(int fontSlot);
            void OnGameFontUpdated(Game^ game, int fontSlot);
			Dictionary<int,Sprite^>^ LoadAllSpriteDimensions();
			void LoadNewSpriteFile();
			Room^ LoadRoomFile(UnloadedRoom ^roomToLoad);
			void SaveRoomFile(Room ^roomToSave);
			void DrawRoomBackground(int hDC, Room ^room, int x, int y, int backgroundNumber, float scaleFactor, RoomAreaMaskType maskType, int selectedArea, int maskTransparency);
			void ImportBackground(Room ^room, int backgroundNumber, Bitmap ^bmp, bool useExactPalette, bool sharePalette);
			void DeleteBackground(Room ^room, int backgroundNumber);
			Bitmap^ GetBitmapForBackground(Room ^room, int backgroundNumber);
			void DrawLineOntoMask(Room ^room, RoomAreaMaskType maskType, int x1, int y1, int x2, int y2, int color);
			void DrawFilledRectOntoMask(Room ^room, RoomAreaMaskType maskType, int x1, int y1, int x2, int y2, int color);
			void DrawFillOntoMask(Room ^room, RoomAreaMaskType maskType, int x1, int y1, int color);
			void CopyWalkableMaskToRegions(Room ^room);
			int  GetAreaMaskPixel(Room ^room, RoomAreaMaskType maskType, int x, int y);
      void ImportAreaMask(Room ^room, RoomAreaMaskType maskType, Bitmap ^bmp);
      Bitmap ^ExportAreaMask(Room ^room, RoomAreaMaskType maskType);
      void CreateUndoBuffer(Room ^room, RoomAreaMaskType maskType);
      bool DoesUndoBufferExist();
      void ClearUndoBuffer();
      void RestoreFromUndoBuffer(Room ^room, RoomAreaMaskType maskType);
      void SetGreyedOutMasksEnabled(bool enabled);
			void CreateBuffer(int width, int height) ;
			void DrawSpriteToBuffer(int sprNum, int x, int y, int scaleFactor) ;
			void RenderBufferToHDC(int hDC) ;
			String ^LoadRoomScript(String ^roomFileName);
			void CompileScript(Script ^script, cli::array<String^> ^preProcessedScripts, Game ^game, bool isRoomScript);
			void CreateDataFile(cli::array<String^> ^fileList, long splitSize, String ^baseFileName, bool isGameEXE);
			void CreateVOXFile(String ^fileName, cli::array<String^> ^fileList);
			GameTemplate^ LoadTemplateFile(String ^fileName);
      RoomTemplate^ LoadRoomTemplateFile(String ^fileName);
			void ExtractTemplateFiles(String ^templateFileName);
		  void ExtractRoomTemplateFiles(String ^templateFileName, int newRoomNumber);
			void UpdateFileIcon(String ^fileToUpdate, String ^iconFileName);
      void UpdateGameExplorerXML(String ^fileToUpdate, cli::array<unsigned char> ^data);
      void UpdateGameExplorerThumbnail(String ^fileToUpdate, cli::array<unsigned char> ^data);
      void UpdateFileVersionInfo(String ^fileToUpdate, cli::array<System::Byte> ^authorNameUnicode, cli::array<System::Byte> ^gameNameUnicode);
			cli::array<unsigned char>^ TransformStringToBytes(String ^text);
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

	}
}
