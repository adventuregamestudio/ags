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

        public void DrawViewLoop(IntPtr hdc, ViewLoop loop, int x, int y, int sizeInPixels, List<int> selectedFrames)
        {
			lock (_spriteSetLock)
			{
				_native.DrawViewLoop((int)hdc, loop, x, y, sizeInPixels, selectedFrames);
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

        /// <summary>
        /// Measures the TTF font from the given file, and tries to find a point size corresponding
        /// to the closest pixel height match, returns the found point size, or 0 in case of error.
        /// </summary>
        public int FindTTFSizeForHeight(string fileName, int size)
        {
            return _native.FindTTFSizeForHeight(fileName, size);
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

        public Size GetMaxViewFrameSize(View view)
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
                return _native.GetMaxSpriteSize(sprites.ToArray());
            }
        }

        public void SaveDefaultRoom(Room roomToSave)
        {
            _native.SaveDefaultRoomFile(roomToSave);
        }

        public string LoadRoomScript(string roomFileName)
        {
            return _native.LoadRoomScript(roomFileName);
        }

        public List<string> GetCompilerExtensions(Game game)
        {
            return _native.GetCompilerExtensions(game.Settings.ExtendedCompiler);
        }

        public void CompileScript(Script script, string[] preProcessedData, Game game, CompileMessages messages)
        {
            _native.CompileScript(script, preProcessedData, game, messages);
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
