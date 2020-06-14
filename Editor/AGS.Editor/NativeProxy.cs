using AGS.Native;
using AGS.Types;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Runtime.InteropServices;
using AGS.Editor.Preferences;

namespace AGS.Editor
{
    class NativeProxy : IDisposable
    {
        [DllImport("kernel32.dll")]
        public static extern IntPtr LoadLibrary(string dllToLoad);
        [DllImport("kernel32.dll")]
        public static extern IntPtr GetProcAddress(IntPtr hModule, string procedureName);
        [DllImport("kernel32.dll")]
        public static extern bool FreeLibrary(IntPtr hModule);
        [DllImport("user32.dll")]
        public static extern bool SetForegroundWindow(IntPtr hWnd);
		[DllImport("kernel32.dll")]
		public static extern long WritePrivateProfileString(string section, string key, string val, string filePath);
		[DllImport("kernel32.dll")]
		public static extern int GetPrivateProfileString(string section, string key, string def, System.Text.StringBuilder retVal, int size, string filePath);

        public const uint WM_MOUSEACTIVATE = 0x21;
        public const uint MA_ACTIVATE = 1;
        public const uint MA_ACTIVATEANDEAT = 2; 

        // IMPORTANT: These lengths reflect the resource file in ACWIN.EXE
        private const int AUTHOR_NAME_LEN = 30;
        private const int FILE_DESCRIPTION_LEN = 40;

        private static NativeProxy _instance;

        public static NativeProxy Instance
        {
            get
            {
                if (_instance == null)
                {
                    _instance = new NativeProxy();
                }
                return _instance;
            }
        }

        private NativeMethods _native;

		// The sprite set lock is used to prevent a crash if the window
		// thread tries to redraw while the game is being saved/loaded
		private object _spriteSetLock = new object();

        private NativeProxy()
        {
			_native = new NativeMethods(AGS.Types.Version.AGS_EDITOR_VERSION);
            _native.Initialize();
        }

        public void NewGameLoaded(Game game, List<string> errors)
        {
            _native.NewGameLoaded(game, errors);
        }

        public void GameSettingsChanged(Game game)
        {
            _native.GameSettingsChanged(game);
        }

        public void PaletteColoursChanged(Game game)
        {
            _native.PaletteColoursUpdated(game);
        }

        public void SaveGame(Game game)
        {
			lock (_spriteSetLock)
			{
				_native.SaveGame(game);
			}
        }

        public void DrawGUI(IntPtr hdc, int x, int y, GUI gui, int resolutionFactor, float scale, int selectedControl)
        {
            _native.DrawGUI((int)hdc, x, y, gui, resolutionFactor, scale, selectedControl);
        }

        public void DrawSprite(IntPtr hdc, int x, int y, int spriteNum)
        {
			lock (_spriteSetLock)
			{
				_native.DrawSprite((int)hdc, x, y, spriteNum, false);
			}
        }

        public void DrawSprite(IntPtr hdc, int x, int y, int spriteNum, bool flipImage)
        {
			lock (_spriteSetLock)
			{
				_native.DrawSprite((int)hdc, x, y, spriteNum, flipImage);
			}
        }

        public int DrawFont(IntPtr hdc, int x, int y, int width, int fontNum)
        {
            return _native.DrawFont((int)hdc, x, y, width, fontNum);
        }

        public void DrawSprite(IntPtr hdc, int x, int y, int width, int height, int spriteNum, bool flipImage = false)
        {
			lock (_spriteSetLock)
			{
				_native.DrawSprite((int)hdc, x, y, width, height, spriteNum, flipImage);
			}
        }

        public void DrawBlockOfColour(IntPtr hdc, int x, int y, int width, int height, int colourNum)
        {
            _native.DrawBlockOfColour((int)hdc, x, y, width, height, colourNum);
        }

        public void DrawViewLoop(IntPtr hdc, ViewLoop loop, int x, int y, int sizeInPixels, int selectedFrame)
        {
			lock (_spriteSetLock)
			{
				_native.DrawViewLoop((int)hdc, loop, x, y, sizeInPixels, selectedFrame);
			}
        }

        public Sprite CreateSpriteFromBitmap(Bitmap bmp, SpriteImportTransparency transparency, bool remapColours, bool useRoomBackgroundColours, bool alphaChannel)
        {
            int spriteSlot = _native.GetFreeSpriteSlot();
            return _native.SetSpriteFromBitmap(spriteSlot, bmp, (int)transparency, remapColours, useRoomBackgroundColours, alphaChannel);
        }

        public void ReplaceSpriteWithBitmap(Sprite spr, Bitmap bmp, SpriteImportTransparency transparency, bool remapColours, bool useRoomBackgroundColours, bool alphaChannel)
        {
            _native.ReplaceSpriteWithBitmap(spr, bmp, (int)transparency, remapColours, useRoomBackgroundColours, alphaChannel);
        }

        public bool CropSpriteEdges(IList<Sprite> sprites, bool symettric)
        {
            return _native.CropSpriteEdges(sprites, symettric);
        }

        public bool DoesSpriteExist(int spriteNumber)
        {
			lock (_spriteSetLock)
			{
				return _native.DoesSpriteExist(spriteNumber);
			}
        }

        public void ChangeSpriteNumber(Sprite sprite, int newNumber)
        {
            _native.ChangeSpriteNumber(sprite, newNumber);
        }

        public void SpriteResolutionsChanged(Sprite[] sprites)
        {
            _native.SpriteResolutionsChanged(sprites);
        }

        public Bitmap GetBitmapForSprite(int spriteSlot, int width, int height)
        {
			lock (_spriteSetLock)
			{
				return _native.GetBitmapForSprite(spriteSlot, width, height);
			}
        }

        public Bitmap GetBitmapForSprite(int spriteSlot)
        {
			lock (_spriteSetLock)
			{
				return _native.GetBitmapForSpritePreserveColDepth(spriteSlot);
			}
        }

        public void DeleteSprite(Sprite sprite)
        {
			lock (_spriteSetLock)
			{
				_native.DeleteSprite(sprite.Number);
			}
        }

        public Game ImportOldGame(string fileName)
        {
            return _native.ImportOldGameFile(fileName);
        }

        public void ImportSCIFont(string fileName, int fontSlot)
        {
            _native.ImportSCIFont(fileName, fontSlot);
        }

        public void ReloadFont(int fontSlot)
        {
            _native.ReloadFont(fontSlot);
        }

        public void OnFontUpdated(Game game, int fontSlot, bool forceUpdate)
        {
            _native.OnGameFontUpdated(game, fontSlot, forceUpdate);
        }

        public Dictionary<int, Sprite> LoadSpriteDimensions()
        {
            return _native.LoadAllSpriteDimensions();
        }

        public void LoadNewSpriteFile()
        {
			lock (_spriteSetLock)
			{
				_native.LoadNewSpriteFile();
			}
        }

        public SpriteInfo GetSpriteInfo(int spriteSlot)
        {
            lock (_spriteSetLock)
            {
                return _native.GetSpriteInfo(spriteSlot);
            }
        }

        public int GetSpriteWidth(int spriteSlot)
        {
            lock (_spriteSetLock)
            {
                return _native.GetSpriteWidth(spriteSlot);
            }
        }

        public int GetSpriteHeight(int spriteSlot)
        {
            lock (_spriteSetLock)
            {
                return _native.GetSpriteHeight(spriteSlot);
            }
        }

        public Room LoadRoom(UnloadedRoom roomToLoad)
        {
            return _native.LoadRoomFile(roomToLoad);
        }

        public void SaveRoom(Room roomToSave)
        {
            _native.SaveRoomFile(roomToSave);
        }

        public void DrawRoomBackground(IntPtr hDC, Room room, int x, int y, int backgroundNumber, float scaleFactor, RoomAreaMaskType maskType, int selectedArea, int maskTransparency)
        {
            _native.DrawRoomBackground((int)hDC, room, x, y, backgroundNumber, scaleFactor, maskType, selectedArea, maskTransparency);
        }

        public void ImportBackground(Room room, int backgroundNumber, Bitmap bmp, bool useExactPalette, bool sharePalette)
        {
            _native.ImportBackground(room, backgroundNumber, bmp, useExactPalette, sharePalette);
        }

        public void DeleteBackground(Room room, int backgroundNumber)
        {
            _native.DeleteBackground(room, backgroundNumber);
        }

        public Bitmap GetBitmapForBackground(Room room, int backgroundNumber)
        {
            return _native.GetBitmapForBackground(room, backgroundNumber);
        }

        public void AdjustRoomResolution(Room room)
        {
            _native.AdjustRoomResolution(room);
        }

        public void AdjustRoomMaskResolution(Room room)
        {
            _native.AdjustRoomMaskResolution(room);
        }

        public void CreateBuffer(int width, int height)
        {
            _native.CreateBuffer(width, height);
        }

        public void RenderBufferToHDC(IntPtr hDC)
        {
            _native.RenderBufferToHDC((int)hDC);
        }

        public void DrawSpriteToBuffer(int spriteNum, int x, int y, float scale)
        {
            _native.DrawSpriteToBuffer(spriteNum, x, y, scale);
        }

        public void DrawLineOntoMask(Room room, RoomAreaMaskType mask, int x1, int y1, int x2, int y2, int color)
        {
            _native.DrawLineOntoMask(room, mask, x1, y1, x2, y2, color);
        }

		public void DrawFilledRectOntoMask(Room room, RoomAreaMaskType mask, int x1, int y1, int x2, int y2, int color)
		{
			_native.DrawFilledRectOntoMask(room, mask, x1, y1, x2, y2, color);
		}

        public void DrawFillOntoMask(Room room, RoomAreaMaskType mask, int x1, int y1, int color)
        {
            _native.DrawFillOntoMask(room, mask, x1, y1, color);
        }

        public void CopyWalkableAreaMaskToRegions(Room room)
        {
            _native.CopyWalkableMaskToRegions(room);
        }

		public bool GreyOutNonSelectedMasks
		{
			set { _native.SetGreyedOutMasksEnabled(value); }
		}

        public int GetAreaMaskPixel(Room room, RoomAreaMaskType mask, int x, int y)
        {
            int pixel = _native.GetAreaMaskPixel(room, mask, x, y);
            // if it lies outside the bitmap, just return 0
            if (pixel < 0)
            {
                pixel = 0;
            }
            return pixel;
        }

        public void CreateUndoBuffer(Room room, RoomAreaMaskType mask)
        {
            _native.CreateUndoBuffer(room, mask);
        }

        public bool DoesUndoBufferExist()
        {
            return _native.DoesUndoBufferExist();
        }

        public void ClearUndoBuffer()
        {
            _native.ClearUndoBuffer();
        }

        public void RestoreFromUndoBuffer(Room room, RoomAreaMaskType mask)
        {
            _native.RestoreFromUndoBuffer(room, mask);
        }

        public void ImportAreaMask(Room room, RoomAreaMaskType mask, Bitmap bmp)
        {
            _native.ImportAreaMask(room, mask, bmp);
        }

        public Bitmap ExportAreaMask(Room room, RoomAreaMaskType mask)
        {
            return _native.ExportAreaMask(room, mask);
        }

        public string LoadRoomScript(string roomFileName)
        {
            return _native.LoadRoomScript(roomFileName);
        }

        public void CompileScript(Script script, string[] preProcessedData, Game game, bool isRoomScript)
        {
            _native.CompileScript(script, preProcessedData, game, isRoomScript);
        }

        public void CreateDataFile(string[] fileList, int splitSize, string baseFileName, bool isGameEXE)
        {
            _native.CreateDataFile(fileList, splitSize, baseFileName, isGameEXE);
        }

        public void CreateDebugMiniEXE(string[] fileList, string exeFileName)
        {
            CreateDataFile(fileList, 0, exeFileName, false);
        }

        public void CreateVOXFile(string fileName, string[] fileList)
        {
            _native.CreateVOXFile(fileName, fileList);
        }

        public void CreateTemplateFile(string templateFileName, string[] fileList)
        {
            CreateDataFile(fileList, 0, templateFileName, false);
        }

        public GameTemplate LoadTemplateFile(string fileName)
        {
            return _native.LoadTemplateFile(fileName);
        }

		public RoomTemplate LoadRoomTemplateFile(string fileName)
		{
			return _native.LoadRoomTemplateFile(fileName);
		}

        public void ExtractTemplateFiles(string templateFileName)
        {
            _native.ExtractTemplateFiles(templateFileName);
        }

		public void ExtractRoomTemplateFiles(string templateFileName, int newRoomNumber)
		{
			_native.ExtractRoomTemplateFiles(templateFileName, newRoomNumber);
		}

        public void UpdateFileIcon(string fileToUpdate, string newIconToUse)
        {
            _native.UpdateFileIcon(fileToUpdate, newIconToUse);
        }

		public void UpdateGameExplorerXML(string fileToUpdate, byte[] newData)
		{
			_native.UpdateGameExplorerXML(fileToUpdate, newData);
		}

		public void UpdateGameExplorerThumbnail(string fileToUpdate, byte[] newData)
		{
			_native.UpdateGameExplorerThumbnail(fileToUpdate, newData);
		}

        public void UpdateFileVersionInfo(string fileToUpdate, string authorName, string gameName)
        {
            _native.UpdateFileVersionInfo(fileToUpdate, 
                System.Text.Encoding.Unicode.GetBytes(authorName.PadRight(AUTHOR_NAME_LEN, ' ')), 
                System.Text.Encoding.Unicode.GetBytes(gameName.PadRight(FILE_DESCRIPTION_LEN, ' ')));
        }

        public bool AreSpritesModified
        {
            get { return _native.HaveSpritesBeenModified(); }
        }

        public byte[] TransformStringToBytes(string text)
        {
            return _native.TransformStringToBytes(text);
        }

        /// <summary>
        /// Allows the Editor to reuse constants from the native code. If a constant required by the Editor
        /// is not also required by the Engine, then it should instead by moved into AGS.Types (AGS.Native
        /// references the AGS.Types assembly). Note that this method returns only System::Int32 and
        /// System::String objects -- it is up to the user to determine if the value should be used as a
        /// smaller integral type (additional casting may be required to cast to a non-int integral type).
        /// </summary>
        public object GetNativeConstant(string name)
        {
            return _native.GetNativeConstant(name);
        }

        public void Dispose()
        {
            _native.Shutdown();
        }

    }
}
