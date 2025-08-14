using AGS.Native;
using AGS.Types;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Runtime.InteropServices;
using System.Text;

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

        public void NewWorkingDirSet(String workingDir)
        {
            _native.NewWorkingDirSet(workingDir);
        }

        public void NewGameLoaded(Game game, CompileMessages errors)
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

        public void ReplaceSpriteFile(string srcFilename)
        {
            lock (_spriteSetLock)
            {
                _native.ReplaceSpriteFile(srcFilename);
            }
        }

        public void DrawGUI(IntPtr hdc, int x, int y, GUI gui, int resolutionFactor, float scale, int ctrlTransparency, int selectedControl)
        {
            _native.DrawGUI((int)hdc, x, y, gui, resolutionFactor, scale, ctrlTransparency, selectedControl);
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

        public Native.FontMetrics GetFontMetrics(int fontNum)
        {
            return _native.GetFontMetrics(fontNum);
        }

        public void DrawFont(IntPtr hdc, int fontNum, int dc_atx, int dc_aty, int dc_width, int dc_height, int padding,
                int cell_w, int cell_h, int cell_space_x, int cell_space_y, float scaling,
                int scroll_y)
        {
            _native.DrawFont((int)hdc, fontNum, dc_atx, dc_aty, dc_width, dc_height, padding,
                cell_w, cell_h, cell_space_x, cell_space_y, scaling,
                scroll_y);
        }

        public void DrawTextUsingFont(IntPtr hdc, string text, int fontNum,
            int dc_atx, int dc_aty, int dc_width, int dc_height,
            int text_atx, int text_aty, int max_width, float scaling)
        {
            _native.DrawTextUsingFont((int)hdc, text, fontNum, dc_atx, dc_aty, dc_width, dc_height, text_atx, text_aty, max_width, scaling);
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

        public void DrawViewLoop(IntPtr hdc, ViewLoop loop, int x, int y, int sizeInPixels, List<int> selectedFrames)
        {
			lock (_spriteSetLock)
			{
				_native.DrawViewLoop((int)hdc, loop, x, y, sizeInPixels, selectedFrames);
			}
        }

        public Sprite CreateSpriteFromBitmap(Bitmap bmp, SpriteImportTransparency transparency, int transColour, bool remapColours, bool useRoomBackgroundColours, bool alphaChannel)
        {
            int spriteSlot = _native.GetFreeSpriteSlot();
            lock (_spriteSetLock)
            {
                return _native.SetSpriteFromBitmap(spriteSlot, bmp, (int)transparency, transColour, remapColours, useRoomBackgroundColours, alphaChannel);
            }
        }

        public void ReplaceSpriteWithBitmap(Sprite spr, Bitmap bmp, SpriteImportTransparency transparency, int transColour, bool remapColours, bool useRoomBackgroundColours, bool alphaChannel)
        {
            lock (_spriteSetLock)
            {
                _native.ReplaceSpriteWithBitmap(spr, bmp, (int)transparency, transColour, remapColours, useRoomBackgroundColours, alphaChannel);
            }
        }

        public bool CropSpriteEdges(IList<Sprite> sprites, bool symettric)
        {
            lock (_spriteSetLock)
            {
                return _native.CropSpriteEdges(sprites, symettric);
            }
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
            lock (_spriteSetLock)
            {
                _native.ChangeSpriteNumber(sprite, newNumber);
            }
        }

        public void SpriteResolutionsChanged(Sprite[] sprites)
        {
            _native.SpriteResolutionsChanged(sprites);
        }

        public Bitmap GetSpriteBitmap(int spriteSlot)
        {
			lock (_spriteSetLock)
			{
				return _native.GetSpriteBitmap(spriteSlot);
			}
        }

        public Bitmap GetSpriteBitmapAs32Bit(int spriteSlot, int width, int height)
        {
            lock (_spriteSetLock)
            {
                return _native.GetSpriteBitmapAs32Bit(spriteSlot, width, height);
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

        /// <summary>
        /// Measures the TTF font from the given file, and tries to find a point size corresponding
        /// to the closest pixel height match, returns the found point size, or 0 in case of error.
        /// </summary>
        public int FindTTFSizeForHeight(string fileName, int size)
        {
            return _native.FindTTFSizeForHeight(fileName, size);
        }

        public void OnFontAdded(Game game, int fontSlot)
        {
            _native.OnGameFontAdded(game, fontSlot);
        }

        public void OnFontDeleted(Game game, int fontSlot)
        {
            _native.OnGameFontDeleted(game, fontSlot);
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

        public Size GetMaxViewFrameSize(View view, out bool hasLowResSprites)
        {
            lock (_spriteSetLock)
            {
                List<int> sprites = new List<int>();
                foreach (ViewLoop loop in view.Loops)
                {
                    foreach (ViewFrame frame in loop.Frames)
                    {
                        sprites.Add(frame.Image);
                    }
                }
                return _native.GetMaxSpriteSize(sprites.ToArray(), out hasLowResSprites);
            }
        }

        public Room LoadRoom(UnloadedRoom roomToLoad, Encoding defEncoding = null)
        {
            return _native.LoadRoomFile(roomToLoad, defEncoding);
        }

        public void SaveRoom(Room roomToSave)
        {
            _native.SaveRoomFile(roomToSave);
        }

        public void SaveDefaultRoom(Room roomToSave)
        {
            _native.SaveDefaultRoomFile(roomToSave);
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

        /// <summary>
        /// Gets current Room's background for preview.
        /// Bitmap is always returned as a 32-bit image.
        /// </summary>
        public Bitmap GetRoomBackgroundForPreview(Room room, int backgroundNumber)
        {
            return _native.GetRoomBackgroundForPreview(room, backgroundNumber);
        }

        /// <summary>
        /// Gets current Room's background in its native colour depth.
        /// </summary>
        public Bitmap ExportRoomBackground(Room room, int backgroundNumber)
        {
            return _native.ExportRoomBackground(room, backgroundNumber);
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

        public List<IScriptCompiler> GetEmbeddedScriptCompilers()
        {
            return _native.GetEmbeddedScriptCompilers();
        }

        public void CreateDebugMiniEXE(string[] fileList, string exeFileName)
        {
            DataFileWriter.MakeFlatDataFile(fileList, 0, exeFileName, false);
        }

        public void CreateTemplateFile(string templateFileName, string[] fileList)
        {
            DataFileWriter.MakeDataFile(fileList, 0, templateFileName, false);
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

        /// <summary>
        /// Reads the ini file and stores found options in the provided dictionary.
        /// The dictionary has 2 levels:
        /// * sections
        /// * key-value pairs
        /// </summary>
        public void ReadIniFile(string fileName, Dictionary<string, Dictionary<string, string>> sections)
        {
            _native.ReadIniFile(fileName, sections);
        }

        /// <summary>
        /// Writes "sections" dictionary into the ini file, optionally either merging or
        /// completely replacing any existing contents.
        /// The dictionary has 2 levels:
        /// * sections
        /// * key-value pairs
        /// </summary>
        public void WriteIniFile(string fileName, Dictionary<string, Dictionary<string, string>> sections, bool mergeExisting)
        {
            _native.WriteIniFile(fileName, sections, mergeExisting);
        }

        public void Dispose()
        {
            _native.Shutdown();
        }

    }
}
