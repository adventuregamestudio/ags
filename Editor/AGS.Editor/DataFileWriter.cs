using AGS.Types;
using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;
using System.Runtime.Serialization;
using System.Text;

namespace AGS.Editor
{
    // TODO: split onto main game file reader/writer and asset library reader/writer
    // TODO: separate data format writers for various game components (GUI etc)
    public class DataFileWriter
    {
        // Signatures of the asset library
        public const string CLIB_BEGIN_SIGNATURE = "CLIB\x1a";
        public const string CLIB_END_SIGNATURE = "CLIB\x1\x2\x3\x4SIGE";
        public const int CLIB_VERSION = 30; // large file support, non-encoded
        public const int MAXMULTIFILES = 256; // 1-byte index
        public const int MAX_PATH = 260; // corresponds to WinAPI definition

        // TODO: probably move these GetAnsiBytes functions to some utility class

        // Converts unicode string into ANSI array of bytes; the returned array
        // will NOT have a null-terminator appended.
        private static byte[] GetAnsiBytes(string text)
        {
            return GetAnsiBytes(text, -1, -1, 0);
        }

        // Converts unicode string into ANSI array of bytes; the returned array
        // will contain converted string with the null-terminator appended.
        private static byte[] GetAnsiBytesTerminated(string text)
        {
            return GetAnsiBytes(text, -1, -1, 1);
        }

        // Converts unicode string into ANSI array of bytes; the returned array
        // will contain converted string with the null-terminator appended.
        // The resulting array will be no longer than max_len bytes in size.
        private static byte[] GetAnsiBytesTerminated(string text, int max_len)
        {
            return GetAnsiBytes(text, -1, max_len, 1);
        }

        // Converts unicode string into ANSI array of bytes.
        // If fixed_length parameter has positive value, the returned array will
        // be exactly 'fixed_length' in size. In such case the larger string
        // gets truncated, and in case of shorter string all the unused bytes
        // are zeroed. No null terminator is appended explicitly though.
        private static byte[] GetAnsiBytes(string text, int fixed_length)
        {
            return GetAnsiBytes(text, fixed_length, fixed_length, 0);
        }

        // Converts unicode string into ANSI array of bytes.
        // If min_size and max_size parameters have positive values, the returned
        // array's length will be ensured to be in min_size to max_size range..
        // If reserve_bytes is positive, that number of bytes will be reserved
        // in the end of array, regardless of whether converted chars fit or not
        // (useful to ensure that string is null-terminated).
        // If converted string appears larger than array is allowed to accomodate,
        // then it gets truncated; if it is shorter, all the unused bytes are
        // zeroed.
        private static byte[] GetAnsiBytes(string text, int min_size, int max_size, int reserve_bytes)
        {
            // We must convert original Unicode string into ANSI string,
            // because AGS engine currently supports only these.
            // When doing so we should use current system's codepage, to meet
            // common expectations of the game developers (and comply to how
            // the Editor behaved in the past).
            //
            // The output array will be at least min_size in size.
            byte[] bytes = new byte[Math.Max(min_size, Encoding.Default.GetByteCount(text) + reserve_bytes)];
            Encoding.Default.GetBytes(text, 0, text.Length, bytes, 0);
            if (max_size <= 0 || bytes.Length <= max_size)
                return bytes;
            // If the output array is larger than requested length, then truncate it;
            // must remember to preseve some bytes in the end, if requested by caller.
            byte[] fixed_bytes = new byte[max_size];
            Array.Copy(bytes, fixed_bytes, fixed_bytes.Length - reserve_bytes);
            return fixed_bytes;
        }


        private class MultiFileLibNew
        {
            public class MultiFile
            {
                public string Filename;
                public long Offset;
                public long Length;
                public byte Datafile;

                public MultiFile(string fileName, byte dataFile, long length)
                {
                    Filename = fileName;
                    Datafile = dataFile;
                    Length = length;
                    Offset = 0;
                }
            }

            public List<string> DataFilenames;
            public List<MultiFile> Files;

            public MultiFileLibNew()
            {
                DataFilenames = new List<string>();
                Files = new List<MultiFile>();
            }
        }

        static MultiFileLibNew ourlib;

        static DataFileWriter()
        {
            ourlib = new MultiFileLibNew();
        }

        static Stream TryFileOpen(string fileName, FileAccess access)
        {
            return TryFileOpen(fileName, FileMode.Open, access);
        }

        static Stream TryFileOpen(string fileName, FileMode mode, FileAccess access)
        {
            Stream stream = null;
            try
            {
                stream = File.Open(fileName, mode, access, FileShare.ReadWrite); // even if we're only reading don't disallow writing from other processes
            }
            catch
            {
            }
            return stream;
        }

        static Stream FindFileInPath(out string buffer, string fileName)
        {
            string tomake = fileName;
            Stream stream = TryFileOpen(tomake, FileAccess.Read);
            // TODO: get audio and speech paths from a kind of shared config
            if (stream == null)
            {
                // try in the Audio folder if not found
                tomake = Path.Combine("AudioCache", fileName);
                stream = TryFileOpen(tomake, FileAccess.Read);
            }
            if (stream == null)
            {
                // no? maybe Speech then, templates include this
                tomake = Path.Combine("Speech", fileName);
                stream = TryFileOpen(tomake, FileAccess.Read);
            }
            buffer = tomake;
            return stream;
        }

        static void FilePutInt8(byte val, BinaryWriter writer)
        {
            writer.Write((byte)val);
        }

        static void FilePutInt32(int val, BinaryWriter writer)
        {
            writer.Write((int)val);
        }

        static void FilePutInt64(long val, BinaryWriter writer)
        {
            writer.Write((long)val);
        }

        class PseudoRandInt
        {
            private static PseudoRandInt _instance;
            private int _lastRandValue;

            public static PseudoRandInt Instance
            {
                get
                {
                    if (_instance == null) _instance = new PseudoRandInt((int)(DateTime.UtcNow - new DateTime(1970, 1, 1)).TotalSeconds);
                    return _instance;
                }
            }

            public static void InitializeInstance(int seed)
            {
                _instance = new PseudoRandInt(seed);
            }

            private PseudoRandInt(int seed)
            {
                _lastRandValue = seed;
            }

            public int GetNextRand()
            {
                _lastRandValue = (int)((_lastRandValue * 214013L) + 2531011L);
                return (_lastRandValue >> 16) & 0x7FFF;
            }
        }

        static void FileWriteDataEncrypted(byte[] data, BinaryWriter writer)
        {
            for (int i = 0; i < data.Length; ++i)
            {
                writer.Write((byte)((int)data[i] + PseudoRandInt.Instance.GetNextRand()));
            }
        }

        static void FilePutStringEncrypted(string text, BinaryWriter writer)
        {
            byte[] bytes = GetAnsiBytesTerminated(text);
            FileWriteDataEncrypted(bytes, writer);
        }

        static void FilePutIntEncrypted(int numberToWrite, BinaryWriter writer)
        {
            FileWriteDataEncrypted(BitConverter.GetBytes(numberToWrite), writer);
        }

        /// <summary>
        /// Writes string preceding it with 32-bit length value.
        /// </summary>
        /// <param name="text"></param>
        /// <param name="writer"></param>
        static void FilePutString(string text, BinaryWriter writer)
        {
            if (String.IsNullOrEmpty(text))
                writer.Write((int)0);
            else
            {
                byte[] bytes = GetAnsiBytes(text);
                writer.Write((int)bytes.Length);
                writer.Write(bytes);
            }
        }

        /// <summary>
        /// Writes null-terminated string, limited by the given number of bytes.
        /// </summary>
        /// <param name="text"></param>
        /// <param name="maxLen"></param>
        /// <param name="writer"></param>
        static void FilePutNullTerminatedString(string text, int maxLen, BinaryWriter writer)
        {
            if (maxLen <= 0) return;
            if ((string.IsNullOrEmpty(text)) || (maxLen == 1))
            {
                writer.Write((byte)0);
                return;
            }
            // This does not make much sense, but trying to match ANSI AGS logic
            // as close as possible, we will both truncate string to maxLen chars,
            // and truncate resulting byte array to maxLen bytes.
            int len = text.IndexOf('\0');
            if (len == -1) len += maxLen;
            if (len < text.Length) text = text.Substring(0, len);
            writer.Write(GetAnsiBytesTerminated(text, maxLen));
        }

        /// <summary>
        /// Writes null-terminated string.
        /// </summary>
        /// <param name="text"></param>
        /// <param name="writer"></param>
        static void FilePutNullTerminatedString(string text, BinaryWriter writer)
        {
            if (string.IsNullOrEmpty(text))
                writer.Write((byte)0);
            else
                writer.Write(GetAnsiBytesTerminated(text));
        }

        /// <summary>
        /// Write asset library header with the table of contents.
        /// Currently corresponds to writing main lib file in chain in format version 30.
        /// </summary>
        /// <param name="writer"></param>
        static void WriteCLIBHeader(BinaryWriter writer)
        {
            FilePutInt32(0, writer); // reserved options
            FilePutInt32(ourlib.DataFilenames.Count, writer);
            for (int i = 0; i < ourlib.DataFilenames.Count; ++i)
            {
                FilePutNullTerminatedString(ourlib.DataFilenames[i], writer);
            }
            FilePutInt32(ourlib.Files.Count, writer);
            for (int i = 0; i < ourlib.Files.Count; ++i)
            {
                FilePutNullTerminatedString(ourlib.Files[i].Filename, writer);
                FilePutInt8(ourlib.Files[i].Datafile, writer);
                FilePutInt64(ourlib.Files[i].Offset, writer);
                FilePutInt64(ourlib.Files[i].Length, writer);
            }
        }

        static int CopyFileAcross(Stream instream, Stream copystream, long leftforthis)
        {
            int success = 1;
            byte[] diskbuffer = new byte[NativeConstants.CHUNKSIZE + 10];
            while (leftforthis > 0)
            {
                if (leftforthis > NativeConstants.CHUNKSIZE)
                {
                    instream.Read(diskbuffer, 0, NativeConstants.CHUNKSIZE);
                    try
                    {
                        copystream.Write(diskbuffer, 0, NativeConstants.CHUNKSIZE);
                    }
                    catch
                    {
                        success = 0;
                    }
                    finally
                    {
                        success = 1;
                    }
                    leftforthis -= NativeConstants.CHUNKSIZE;
                }
                else
                {
                    instream.Read(diskbuffer, 0, (int)leftforthis);
                    try
                    {
                        copystream.Write(diskbuffer, 0, (int)leftforthis);
                    }
                    catch
                    {
                        success = 0;
                    }
                    finally
                    {
                        success = 1;
                    }
                    leftforthis = 0;
                }
                if (success < 1) break;
            }
            return success;
        }

        public static string MakeDataFile(string[] fileNames, int splitSize, string baseFileName, bool makeFileNameAssumptions)
        {
            Environment.CurrentDirectory = Factory.AGSEditor.CurrentGame.DirectoryPath;
            ourlib.DataFilenames.Clear();
            ourlib.Files.Clear();
            ourlib.Files.Capacity = fileNames.Length;
            int currentDataFile = 0;
            long sizeSoFar = 0;
            bool doSplitting = false;
            for (int i = 0; i < fileNames.Length; ++i)
            {
                if (splitSize > 0)
                {
                    if (string.Compare(fileNames[i], NativeConstants.SPRSET_NAME, true) == 0)
                    {
                        // the sprite file's appearance signifies it's time to start splitting
                        doSplitting = true;
                        currentDataFile++;
                        sizeSoFar = 0;
                    }
                    else if ((sizeSoFar > splitSize) && (doSplitting) && (currentDataFile < (DataFileWriter.MAXMULTIFILES - 1)))
                    {
                        currentDataFile++;
                        sizeSoFar = 0;
                    }
                }
                long thisFileSize = 0;
                using (Stream tf = File.OpenRead(fileNames[i]))
                {
                    thisFileSize = tf.Length;
                }
                sizeSoFar += thisFileSize;
                string fileNameSrc = Path.GetFileName(fileNames[i]);
                if (fileNameSrc.Length >= DataFileWriter.MAX_PATH)
                {
                    return "Filename too long: " + fileNames[i];
                }
                ourlib.Files.Add(new MultiFileLibNew.MultiFile(fileNameSrc, (byte)currentDataFile, thisFileSize));
            }
            ourlib.DataFilenames.Capacity = currentDataFile + 1;
            long startOffset = 0;
            long mainHeaderOffset = 0;
            string outputFileName;
            string firstDataFileFullPath = null;
            string outputDir = Path.Combine(AGSEditor.OUTPUT_DIRECTORY, AGSEditor.DATA_OUTPUT_DIRECTORY);
            if (makeFileNameAssumptions)
            {
                Directory.CreateDirectory(outputDir);
            }
            // First, set up ourlib.data_filenames array with all the filenames
            // so that write_clib_header will write the correct amount of data
            for (int i = 0, cap = ourlib.DataFilenames.Capacity; i < cap; ++i)
            {
                if (makeFileNameAssumptions)
                {
                    ourlib.DataFilenames.Add(baseFileName + "." + 
                        (i == 0 ? "ags" : i.ToString("D3")));
                }
                else
                {
                    ourlib.DataFilenames.Add(Path.GetFileName(baseFileName));
                }
            }
            // adjust the file paths if necessary, so that write_clib_header will
            // write the correct amount of data
            string tomake;
            for (int i = 0; i < ourlib.Files.Count; ++i)
            {
                using (Stream stream = FindFileInPath(out tomake, ourlib.Files[i].Filename))
                {
                    if (stream != null)
                    {
                        stream.Close();
                        if (!makeFileNameAssumptions) ourlib.Files[i].Filename = tomake;
                    }
                }
            }
            // now, create the actual files
            for (int i = 0; i < ourlib.DataFilenames.Count; ++i)
            {
                if (makeFileNameAssumptions)
                {
                    outputFileName = Path.Combine(outputDir, ourlib.DataFilenames[i]);
                }
                else
                {
                    outputFileName = baseFileName;
                }
                if (i == 0) firstDataFileFullPath = outputFileName;
                using (Stream wout = TryFileOpen(outputFileName,
                    (makeFileNameAssumptions ? FileMode.Create : FileMode.Append), FileAccess.Write))
                {
                    if (wout == null) return "ERROR: unable to open file '" + outputFileName + "' for writing";
                    BinaryWriter writer = new BinaryWriter(wout);
                    startOffset = writer.BaseStream.Length;
                    writer.Write(CLIB_BEGIN_SIGNATURE.ToCharArray());
                    writer.Write((byte)CLIB_VERSION);
                    writer.Write((byte)i);
                    if (i == 0)
                    {
                        mainHeaderOffset = writer.BaseStream.Position;
                        WriteCLIBHeader(writer);
                    }
                    string buffer;
                    for (int j = 0; j < ourlib.Files.Count; ++j)
                    {
                        if (ourlib.Files[j].Datafile == i)
                        {
                            ourlib.Files[j].Offset = (writer.BaseStream.Position - startOffset);
                            using (Stream stream = FindFileInPath(out buffer, ourlib.Files[j].Filename))
                            {
                                if (stream == null)
                                {
                                    try
                                    {
                                        File.Delete(outputFileName);
                                    }
                                    catch
                                    {
                                    }
                                    return "Unable to find file '" + ourlib.Files[j].Filename + "' for compilation in directory '" + Directory.GetCurrentDirectory() + "'. Do not remove files during the compilation process.";
                                }
                                if (CopyFileAcross(stream, writer.BaseStream, ourlib.Files[j].Length) < 1)
                                {
                                    return "Error writing file '" + ourlib.Files[j].Filename + "': possibly disk full";
                                }
                            }
                        }
                    }
                    if (startOffset > 0)
                    {
                        FilePutInt64(startOffset, writer);
                        writer.Write(DataFileWriter.CLIB_END_SIGNATURE.ToCharArray());
                    }
                }
            }
            using (Stream wout = TryFileOpen(firstDataFileFullPath, FileMode.Open, FileAccess.Write))
            {
                wout.Seek(mainHeaderOffset, SeekOrigin.Begin);
                WriteCLIBHeader(new BinaryWriter(wout));
            }
            return null;
        }

        private static void WriteGameSetupStructBase_Aligned(BinaryWriter writer, Game game)
        {
            // assume stream is aligned at start
            WriteString(SafeTruncate(game.Settings.GameName, 49), 50, writer);
            writer.Write(new byte[2]); // alignment padding
            int[] options = new int[100];
            options[NativeConstants.GameOptions.OPT_ALWAYSSPCH] = (game.Settings.AlwaysDisplayTextAsSpeech ? 1 : 0);
            options[NativeConstants.GameOptions.OPT_ANTIALIASFONTS] = (game.Settings.AntiAliasFonts ? 1 : 0);
            options[NativeConstants.GameOptions.OPT_ANTIGLIDE] = (game.Settings.AntiGlideMode ? 1 : 0);
            options[NativeConstants.GameOptions.OPT_NOWALKMODE] = (game.Settings.AutoMoveInWalkMode ? 0 : 1);
            options[NativeConstants.GameOptions.OPT_RIGHTLEFTWRITE] = (game.Settings.BackwardsText ? 1 : 0);
            options[NativeConstants.GameOptions.OPT_COMPRESSSPRITES] = (game.Settings.CompressSprites ? 1 : 0);
            options[NativeConstants.GameOptions.OPT_DEBUGMODE] = (game.Settings.DebugMode ? 1 : 0);
            options[NativeConstants.GameOptions.OPT_DIALOGUPWARDS] = (game.Settings.DialogOptionsBackwards ? 1 : 0);
            options[NativeConstants.GameOptions.OPT_DIALOGGAP] = game.Settings.DialogOptionsGap;
            options[NativeConstants.GameOptions.OPT_DIALOGIFACE] = game.Settings.DialogOptionsGUI;
            options[NativeConstants.GameOptions.OPT_DUPLICATEINV] = (game.Settings.DisplayMultipleInventory ? 1 : 0);
            options[NativeConstants.GameOptions.OPT_STRICTSTRINGS] = (game.Settings.EnforceNewStrings ? 1 : 0);
            options[NativeConstants.GameOptions.OPT_STRICTSCRIPTING] = (game.Settings.EnforceObjectBasedScript ? 1 : 0);
            options[NativeConstants.GameOptions.OPT_HIRES_FONTS] = 0; // always ignore this setting
            options[NativeConstants.GameOptions.OPT_NEWGUIALPHA] = (int)game.Settings.GUIAlphaStyle;
            options[NativeConstants.GameOptions.OPT_SPRITEALPHA] = (int)game.Settings.SpriteAlphaStyle;
            options[NativeConstants.GameOptions.OPT_HANDLEINVCLICKS] = (game.Settings.HandleInvClicksInScript ? 1 : 0);
            options[NativeConstants.GameOptions.OPT_FIXEDINVCURSOR] = (game.Settings.InventoryCursors ? 0 : 1);
            options[NativeConstants.GameOptions.OPT_GLOBALTALKANIMSPD] = (game.Settings.UseGlobalSpeechAnimationDelay ?
                game.Settings.GlobalSpeechAnimationDelay : (-game.Settings.GlobalSpeechAnimationDelay - 1));
            options[NativeConstants.GameOptions.OPT_LEFTTORIGHTEVAL] = (game.Settings.LeftToRightPrecedence ? 1 : 0);
            options[NativeConstants.GameOptions.OPT_LETTERBOX] = (game.Settings.LetterboxMode ? 1 : 0);
            options[NativeConstants.GameOptions.OPT_MOUSEWHEEL] = (game.Settings.MouseWheelEnabled ? 1 : 0);
            options[NativeConstants.GameOptions.OPT_DIALOGNUMBERED] = (int)game.Settings.NumberDialogOptions;
            options[NativeConstants.GameOptions.OPT_PIXPERFECT] = (game.Settings.PixelPerfect ? 1 : 0);
            options[NativeConstants.GameOptions.OPT_FADETYPE] = (int)game.Settings.RoomTransition;
            options[NativeConstants.GameOptions.OPT_RUNGAMEDLGOPTS] = (game.Settings.RunGameLoopsWhileDialogOptionsDisplayed ? 1 : 0);
            options[NativeConstants.GameOptions.OPT_SAVESCREENSHOT] = (game.Settings.SaveScreenshots ? 1 : 0);
            options[NativeConstants.GameOptions.OPT_NOSKIPTEXT] = (int)game.Settings.SkipSpeech;
            options[NativeConstants.GameOptions.OPT_PORTRAITSIDE] = (int)game.Settings.SpeechPortraitSide;
            options[NativeConstants.GameOptions.OPT_SPEECHTYPE] = (int)game.Settings.SpeechStyle;
            options[NativeConstants.GameOptions.OPT_SPLITRESOURCES] = game.Settings.SplitResources;
            options[NativeConstants.GameOptions.OPT_TWCUSTOM] = game.Settings.TextWindowGUI;
            options[NativeConstants.GameOptions.OPT_THOUGHTGUI] = game.Settings.ThoughtGUI;
            options[NativeConstants.GameOptions.OPT_TURNTOFACELOC] = (game.Settings.TurnBeforeFacing ? 1 : 0);
            options[NativeConstants.GameOptions.OPT_ROTATECHARS] = (game.Settings.TurnBeforeWalking ? 1 : 0);
            options[NativeConstants.GameOptions.OPT_NATIVECOORDINATES] = (game.Settings.UseLowResCoordinatesInScript ? 0 : 1);
            options[NativeConstants.GameOptions.OPT_WALKONLOOK] = (game.Settings.WalkInLookMode ? 1 : 0);
            options[NativeConstants.GameOptions.OPT_DISABLEOFF] = (int)game.Settings.WhenInterfaceDisabled;
            options[NativeConstants.GameOptions.OPT_SAFEFILEPATHS] = 1; // always use safe file paths in new games
            options[NativeConstants.GameOptions.OPT_DIALOGOPTIONSAPI] = (game.Settings.UseOldCustomDialogOptionsAPI ? -1 : 1);
            options[NativeConstants.GameOptions.OPT_BASESCRIPTAPI] = (int)game.Settings.ScriptAPIVersionReal;
            options[NativeConstants.GameOptions.OPT_SCRIPTCOMPATLEV] = (int)game.Settings.ScriptCompatLevelReal;
            options[NativeConstants.GameOptions.OPT_RENDERATSCREENRES] = (int)game.Settings.RenderAtScreenResolution;
            options[NativeConstants.GameOptions.OPT_RELATIVEASSETRES] = (game.Settings.AllowRelativeAssetResolutions ? 1 : 0);
            options[NativeConstants.GameOptions.OPT_LIPSYNCTEXT] = (game.LipSync.Type == LipSyncType.Text ? 1 : 0);
            for (int i = 0; i < options.Length; ++i) // writing only ints, alignment preserved
            {
                writer.Write(options[i]);
            }
            for (int i = 0; i < 256; ++i) // writing 256 bytes, alignment preserved
            {
                if (game.Palette[i].ColourType == PaletteColourType.Background)
                {
                    writer.Write((byte)2); // PAL_BACKGROUND
                }
                else writer.Write((byte)0); // PAL_GAMEWIDE
            }
            for (int i = 0; i < 256; ++i) // writing 256*4 bytes, alignment preserved
            {
                writer.Write((byte)(game.Palette[i].Colour.R / 4));
                writer.Write((byte)(game.Palette[i].Colour.G / 4));
                writer.Write((byte)(game.Palette[i].Colour.B / 4));
                writer.Write((byte)0); // filler
            }
            writer.Write(game.ViewCount);
            writer.Write(game.Characters.Count);
            writer.Write(game.PlayerCharacter.ID);
            writer.Write(game.Settings.MaximumScore);
            writer.Write((short)(game.InventoryItems.Count + 1));
            writer.Write(new byte[2]); // alignment padding
            writer.Write(game.Dialogs.Count);
            writer.Write(0); // numdlgmessage
            writer.Write(game.Fonts.Count);
            writer.Write((int)game.Settings.ColorDepth);
            writer.Write(0); // target_win
            writer.Write(game.Settings.DialogOptionsBullet); // aligned so far
            writer.Write(game.Settings.InventoryHotspotMarker.Style != InventoryHotspotMarkerStyle.None ?
                (short)game.Settings.InventoryHotspotMarker.DotColor : (short)0); // need 2 bytes for alignment
            writer.Write((short)game.Settings.InventoryHotspotMarker.CrosshairColor); // aligned again
            writer.Write(game.Settings.UniqueID);
            writer.Write(game.GUIs.Count);
            writer.Write(game.Cursors.Count);
            if (game.Settings.LetterboxMode)
            {
                writer.Write((int)game.Settings.LegacyLetterboxResolution);
            }
            else
            {
                writer.Write(NativeConstants.GAME_RESOLUTION_CUSTOM);
                writer.Write(game.Settings.CustomResolution.Width);
                writer.Write(game.Settings.CustomResolution.Height);
            }
            writer.Write(game.LipSync.DefaultFrame);
            writer.Write(game.Settings.InventoryHotspotMarker.Style == InventoryHotspotMarkerStyle.Sprite ?
                game.Settings.InventoryHotspotMarker.Image : 0);
            writer.Write(new byte[17 * sizeof(int)]); // reserved; 17 ints, alignment preserved
            for (int i = 0; i < 500; ++i) // MAXGLOBALMES; write 500 ints, alignment preserved
            {
                writer.Write(string.IsNullOrEmpty(game.GlobalMessages[i]) ? 0 : 1);
            }
            // the rest are ints, alignment is correct
            writer.Write(1); // dict != null
            writer.Write(0); // globalscript != null
            writer.Write(0); // chars != null
            writer.Write(1); // compiled_script != null
            // no final padding required
        }

        private static void UpdateSpriteFlags(SpriteFolder folder, byte[] flags, out int mostTopmost)
        {
            mostTopmost = -1;
            foreach (Sprite sprite in folder.Sprites)
            {
                mostTopmost = Math.Max(sprite.Number, mostTopmost);
                flags[sprite.Number] = 0;
                if (sprite.Resolution != SpriteImportResolution.Real) flags[sprite.Number] |= NativeConstants.SPF_VAR_RESOLUTION;
                if (sprite.Resolution == SpriteImportResolution.HighRes) flags[sprite.Number] |= NativeConstants.SPF_HIRES;
                if (sprite.AlphaChannel) flags[sprite.Number] |= NativeConstants.SPF_ALPHACHANNEL;
            }
            foreach (SpriteFolder subfolder in folder.SubFolders)
            {
                int topmost;
                UpdateSpriteFlags(subfolder, flags, out topmost);
                mostTopmost = Math.Max(topmost, mostTopmost);
            }
        }

        private static string SafeTruncate(string src, int maxLength)
        {
            // TODO: I think this should actually NOT be safe and should throw an error
            // if maxLength is exceeded.
            if ((src == null) || (maxLength <= 0)) return "";
            if (maxLength >= src.Length) return src;
            return src.Substring(0, maxLength);
        }

        /// <summary>
        /// Writes a string to the file as bytes. Length must be provided
        /// and will pad or truncate the text as necessary.
        /// </summary>
        private static void WriteString(string src, int length, BinaryWriter writer)
        {
            if ((writer == null) || (length <= 0)) return;
            byte[] bytes = GetAnsiBytes(src, length);
            writer.Write(bytes);
        }

        private static string ReadString(int length, BinaryReader reader)
        {
            return new string(reader.ReadChars(length));
        }

        private static string ReadString(BinaryReader reader)
        {
            int len = reader.ReadInt32();
            return new string(reader.ReadChars(len));
        }

        private class CustomPropertiesWriter
        {
            public static void Write(BinaryWriter writer, CustomProperties properties)
            {
                writer.Write(NativeConstants.CustomPropertyVersion.Current);
                writer.Write(properties.PropertyValues.Count);
                foreach (KeyValuePair<string, CustomProperty> pair in properties.PropertyValues)
                {
                    FilePutString(pair.Value.Name, writer);
                    FilePutString(pair.Value.Value, writer);
                }
            }
        }

        private class CompiledCustomProperties
        {
            private List<string> _names;
            private List<string> _values;

            public CompiledCustomProperties()
            {
                _names = new List<string>();
                _values = new List<string>();
            }

            public string[] Names
            {
                get
                {
                    return _names.ToArray();
                }
            }

            public string[] Values
            {
                get
                {
                    return _values.ToArray();
                }
            }

            public int PropertyCount
            {
                get
                {
                    return _names.Count;
                }
            }

            public string this[string name]
            {
                get
                {
                    int idx = _names.IndexOf(name);
                    if (idx == -1) return null;
                    return _values[idx];
                }
            }

            public int IndexOfName(string name)
            {
                return _names.IndexOf(name);
            }

            public int IndexOfValue(string value)
            {
                return _values.IndexOf(value);
            }

            public void Reset()
            {
                _names.Clear();
                _values.Clear();
            }

            public void AddProperty(string name, string value)
            {
                _names.Add(name);
                _values.Add(value);
            }

            public void Serialize(BinaryWriter writer)
            {
                writer.Write(NativeConstants.CustomPropertyVersion.Current);
                writer.Write(PropertyCount);
                for (int i = 0; i < PropertyCount; ++i)
                {
                    FilePutString(Names[i], writer);
                    FilePutString(Values[i], writer);
                }
            }

            int UnSerialize(BinaryReader reader)
            {
                if (reader.ReadInt32() != NativeConstants.CustomPropertyVersion.Current) return -1;
                int count = reader.ReadInt32();
                for (int i = 0; i < count; ++i)
                {
                    string name = ReadString(reader);
                    string value = ReadString(reader);
                    AddProperty(name, value);
                }
                return 0;
            }
        }

        private static void CompileCustomProperties(CustomProperties convertFrom, CompiledCustomProperties compileInto)
        {
            compileInto.Reset();
            foreach (string key in convertFrom.PropertyValues.Keys)
            {
                compileInto.AddProperty(convertFrom.PropertyValues[key].Name, convertFrom.PropertyValues[key].Value);
            }
        }

        static void SerializeInteractionScripts(Interactions interactions, BinaryWriter writer)
        {
            writer.Write(interactions.ScriptFunctionNames.Length);
            foreach (string funcName in interactions.ScriptFunctionNames)
            {
                if (funcName == null)
                {
                    writer.Write((byte)0);
                }
                else
                {
                    WriteString(funcName, funcName.Length, writer);
                    writer.Write((byte)0);
                }
            }
        }

        // Encrypts an ANSI string
        static void EncryptText(byte[] toEncrypt)
        {
            int p = 0;
            for (int i = 0; i < toEncrypt.Length; ++i)
            {
                toEncrypt[i] += NativeConstants.PASSWORD_ENC_STRING[p];
                if (++p == NativeConstants.PASSWORD_ENC_STRING.Length)
                    p = 0;
            }
        }

        static void WriteStringEncrypted(BinaryWriter writer, string text)
        {
            byte[] bytes = GetAnsiBytesTerminated(text);
            EncryptText(bytes);
            writer.Write(bytes.Length);
            writer.Write(bytes);
        }

        static bool WriteCompiledScript(FileStream ostream, Script script, CompileMessages errors)
        {
            if (script.CompiledData == null)
            {
                errors.Add(new CompileError(string.Format("Script has not been compiled: {0}", script.FileName)));
                return false;
            }
            script.CompiledData.Write(ostream, script.FileName);
            return true;
        }

        class ViewsWriter
        {
            private BinaryWriter writer;
            private View[] views;

            public ViewsWriter(BinaryWriter writer, Game game)
            {
                this.writer = writer;
                views = new View[game.ViewCount];
                PopulateViews(game.RootViewFolder, game);
            }

            public void PopulateViews(IViewFolder folder, Game game)
            {
                FolderHelper.ForEachViewFolder(folder, game, new FolderHelper.ViewFolderProcessing(PopulateViews));
                foreach (View view in folder.Views)
                {
                    views[view.ID - 1] = view;
                }
            }

            public bool WriteViews(IViewFolder folder, Game game, CompileMessages errors)
            {
                if (writer == null)
                {
                    errors.Add(new CompileError("Could not write views: Invalid stream (NULL)"));
                    return false;
                }
                foreach (View view in views)
                {
                    // views are not always sequential, so we may have some null entries;
                    // but even in that case we must write number of loops (0) to conform
                    // to the data format
                    short numLoops = (short)(view != null ? view.Loops.Count : 0);
                    writer.Write(numLoops);
                    for (int i = 0; i < numLoops; ++i)
                    {
                        short numFrames = (short)view.Loops[i].Frames.Count;
                        writer.Write(numFrames);
                        writer.Write(view.Loops[i].RunNextLoop ? NativeConstants.LOOPFLAG_RUNNEXTLOOP : 0);
                        for (int j = 0; j < numFrames; ++j)
                        {
                            ViewFrame frame = view.Loops[i].Frames[j];
                            writer.Write(frame.Image);
                            writer.Write((short)0); // unused x-offset
                            writer.Write((short)0); // unused y-offset
                            writer.Write((short)frame.Delay);
                            writer.Write((short)0); // struct alignment padding
                            writer.Write(frame.Flipped ? NativeConstants.VFLG_FLIPSPRITE : 0);
                            writer.Write(frame.Sound > 0 ? game.GetAudioArrayIndexFromAudioClipIndex(frame.Sound) : -1);
                            writer.Write(0); // unused reservedForFuture[0]
                            writer.Write(0); // unused reservedForFuture[1]
                        }
                    }
                }
                return true;
            }
        }

        class GUIsWriter
        {
            private List<GUIButtonOrTextWindowEdge> GUIButtonsAndTextWindowEdges = new List<GUIButtonOrTextWindowEdge>();
            private List<GUILabel> GUILabels = new List<GUILabel>();
            private List<GUIInventory> GUIInvWindows = new List<GUIInventory>();
            private List<GUISlider> GUISliders = new List<GUISlider>();
            private List<GUITextBox> GUITextBoxes = new List<GUITextBox>();
            private List<GUIListBox> GUIListBoxes = new List<GUIListBox>();
            private BinaryWriter writer;
            private Game game;

            public GUIsWriter(BinaryWriter writer, Game game)
            {
                this.writer = writer;
                this.game = game;
            }

            /// <summary>
            /// The engine treats GUITextWindowEdges as GUIButtons, and they
            /// need to be written to the game data file as the same type. For
            /// consistency with other GUIControl types, there are no public
            /// constructors for this type. Instead you must use a cast on a
            /// GUIControl to this type (the as operator may not be used
            /// because it does not support user-defined conversions). If a
            /// GUIControl that is neither a GUIButton nor a GUITextWindowEdge
            /// is cast to this type then null is returned.
            /// </summary>
            private class GUIButtonOrTextWindowEdge
            {
                private GUIControl _ctrl;

                private GUIButtonOrTextWindowEdge(GUIControl control)
                {
                    _ctrl = control;
                }

                public static implicit operator GUIButtonOrTextWindowEdge(GUIControl ctrl)
                {
                    if (ctrl == null) return null;
                    if (((ctrl as GUIButton) == null) && ((ctrl as GUITextWindowEdge) == null)) return null;
                    return new GUIButtonOrTextWindowEdge(ctrl);
                }

                public static implicit operator GUIButtonOrTextWindowEdge(GUIButton button)
                {
                    if (button == null) return null;
                    return new GUIButtonOrTextWindowEdge(button);
                }

                public static implicit operator GUIButtonOrTextWindowEdge(GUITextWindowEdge textWindowEdge)
                {
                    if (textWindowEdge == null) return null;
                    return new GUIButtonOrTextWindowEdge(textWindowEdge);
                }

                public static implicit operator GUIControl(GUIButtonOrTextWindowEdge ctrl)
                {
                    return (ctrl == null ? null : ctrl._ctrl);
                }

                public static implicit operator GUIButton(GUIButtonOrTextWindowEdge ctrl)
                {
                    return ((GUIControl)ctrl) as GUIButton;
                }

                public static implicit operator GUITextWindowEdge(GUIButtonOrTextWindowEdge ctrl)
                {
                    return ((GUIControl)ctrl) as GUITextWindowEdge;
                }

                public int Image
                {
                    get
                    {
                        GUIButton button = (GUIButton)this;
                        GUITextWindowEdge textWindowEdge = (GUITextWindowEdge)this;
                        if (button != null) return button.Image;
                        if (textWindowEdge != null) return textWindowEdge.Image;
                        return -1;
                    }
                }

                public int MouseoverImage
                {
                    get
                    {
                        GUIButton button = (GUIButton)this;
                        if (button != null) return button.MouseoverImage;
                        return -1;
                    }
                }

                public int PushedImage
                {
                    get
                    {
                        GUIButton button = (GUIButton)this;
                        if (button != null) return button.PushedImage;
                        return -1;
                    }
                }

                public int Font
                {
                    get
                    {
                        GUIButton button = (GUIButton)this;
                        if (button != null) return button.Font;
                        return 0;
                    }
                }

                public int TextColor
                {
                    get
                    {
                        GUIButton button = (GUIButton)this;
                        if (button != null) return button.TextColor;
                        return 0;
                    }
                }

                public GUIClickAction ClickAction
                {
                    get
                    {
                        GUIButton button = (GUIButton)this;
                        if (button != null) return button.ClickAction;
                        return GUIClickAction.RunScript;
                    }
                }

                public int NewModeNumber
                {
                    get
                    {
                        GUIButton button = (GUIButton)this;
                        if (button != null) return button.NewModeNumber;
                        return 0;
                    }
                }

                public string Text
                {
                    get
                    {
                        GUIButton button = (GUIButton)this;
                        GUITextWindowEdge textWindowEdge = (GUITextWindowEdge)this;
                        if (button != null) return button.Text;
                        if (textWindowEdge != null) return "";
                        return null;
                    }
                }

                public FrameAlignment TextAlignment
                {
                    get
                    {
                        GUIButton button = (GUIButton)this;
                        if (button != null) return button.TextAlignment;
                        return FrameAlignment.TopCenter;
                    }
                }

                public string OnClick
                {
                    get
                    {
                        GUIButton button = (GUIButton)this;
                        if (button != null) return button.OnClick;
                        return null;
                    }
                }

                public bool ClipImage
                {
                    get
                    {
                        GUIButton button = (GUIButton)this;
                        if (button != null) return button.ClipImage;
                        return false;
                    }
                }
            }

            private int MakeCommonGUIControlFlags(GUIControl control)
            {
                return (control.Clickable ? NativeConstants.GUIF_CLICKABLE : 0) |
                    (control.Enabled ? NativeConstants.GUIF_ENABLED : 0) |
                    (control.Visible ? NativeConstants.GUIF_VISIBLE : 0)
                    ;
            }

            /// <summary>
            /// Writes the common elements of this GUIControl to the file. Type-specific
            /// data is written only by the respective method for that type.
            /// </summary>
            private void WriteGUIControl(GUIControl control, int flags, string[] events)
            {
                flags |= MakeCommonGUIControlFlags(control);
                writer.Write(flags); // flags
                writer.Write(control.Left);
                writer.Write(control.Top);
                writer.Write(control.Width);
                writer.Write(control.Height);
                writer.Write(control.ZOrder);
                FilePutNullTerminatedString(control.Name, writer);
                writer.Write(events.Length); // numSupportedEvents
                foreach (string sevent in events)
                {
                    FilePutNullTerminatedString(sevent, writer);
                }
            }

            private void WriteGUIControl(GUIControl control, int flags)
            {
                WriteGUIControl(control, flags, new string[0]);
            }

            private void WriteAllButtonsAndTextWindowEdges()
            {
                writer.Write(GUIButtonsAndTextWindowEdges.Count);
                foreach (GUIButtonOrTextWindowEdge ctrl in GUIButtonsAndTextWindowEdges)
                {
                    int flags;
                    flags = (ctrl.ClipImage ? NativeConstants.GUIF_CLIP : 0);
                    WriteGUIControl(ctrl, flags, new string[] { ctrl.OnClick });
                    writer.Write(ctrl.Image); // pic
                    writer.Write(ctrl.MouseoverImage); // overpic
                    writer.Write(ctrl.PushedImage); // pushedpic
                    writer.Write(ctrl.Font); // font
                    writer.Write(ctrl.TextColor); // textcol
                    writer.Write((int)ctrl.ClickAction); // leftclick
                    writer.Write(0); // rightclick
                    writer.Write(ctrl.NewModeNumber); // lclickdata
                    writer.Write(0); // rclickdata
                    FilePutString(ctrl.Text, writer); // text
                    writer.Write((int)ctrl.TextAlignment); // textAlignment
                }
            }

            private void WriteAllLabels()
            {
                writer.Write(GUILabels.Count);
                foreach (GUILabel label in GUILabels)
                {
                    WriteGUIControl(label, 0);
                    string text = label.Text;
                    FilePutString(text, writer);
                    writer.Write(label.Font);
                    writer.Write(label.TextColor);
                    writer.Write((int)label.TextAlignment);
                }
            }

            private void WriteAllInvWindows()
            {
                writer.Write(GUIInvWindows.Count);
                foreach (GUIInventory invWindow in GUIInvWindows)
                {
                    WriteGUIControl(invWindow, 0);
                    writer.Write(invWindow.CharacterID);
                    writer.Write(invWindow.ItemWidth);
                    writer.Write(invWindow.ItemHeight);
                }
            }

            private void WriteAllSliders()
            {
                writer.Write(GUISliders.Count);
                foreach (GUISlider slider in GUISliders)
                {
                    WriteGUIControl(slider, 0, new string[] { slider.OnChange });
                    writer.Write(slider.MinValue);
                    writer.Write(slider.MaxValue);
                    writer.Write(slider.Value);
                    writer.Write(slider.HandleImage);
                    writer.Write(slider.HandleOffset);
                    writer.Write(slider.BackgroundImage);
                }
            }

            private void WriteAllTextBoxes()
            {
                writer.Write(GUITextBoxes.Count);
                foreach (GUITextBox textBox in GUITextBoxes)
                {
                    WriteGUIControl(textBox, 0, new string[] { textBox.OnActivate });
                    FilePutString(textBox.Text, writer);
                    writer.Write(textBox.Font);
                    writer.Write(textBox.TextColor);
                    writer.Write(MakeTextBoxFlags(textBox));
                }
            }

            private int MakeTextBoxFlags(GUITextBox textBox)
            {
                return textBox.ShowBorder ? NativeConstants.GTF_SHOWBORDER : 0;
            }

            private void WriteAllListBoxes()
            {
                writer.Write(GUIListBoxes.Count);
                foreach (GUIListBox listBox in GUIListBoxes)
                {
                    int flags = (listBox.Translated ? NativeConstants.GUIF_TRANSLATED : 0);
                    WriteGUIControl(listBox, flags, new string[] { listBox.OnSelectionChanged });
                    writer.Write(0); // numItems
                    writer.Write(listBox.Font);
                    writer.Write(listBox.TextColor);
                    writer.Write(listBox.SelectedTextColor);
                    writer.Write(MakeListBoxFlags(listBox));
                    writer.Write((int)listBox.TextAlignment);
                    writer.Write(listBox.SelectedBackgroundColor);
                }
            }

            private int MakeListBoxFlags(GUIListBox listBox)
            {
                return (listBox.ShowBorder ? NativeConstants.GLF_SHOWBORDER : 0) |
                       (listBox.ShowScrollArrows ? NativeConstants.GLF_SHOWARROWS : 0)
                       ;
            }

            /// <summary>
            /// Fill the lists of the GUIControl types.
            /// </summary>
            /// <returns>
            /// Returns a jagged array indexed by GUI.ID then GUIControl.ID, which holds
            /// the index of each control in the global list of the same GUIControl type.
            /// </returns>
            private int[][] PopulateGUIControls()
            {
                //int[][] objrefptrs = new int[game.GUIs.Count][];
                List<int[]> objrefptrs = new List<int[]>();
                foreach (GUI gui in game.GUIs)
                {
                    //objrefptrs[gui.ID] = new int[Constants.MAX_OBJS_ON_GUI];
                    objrefptrs.Add(new int[gui.Controls.Count]);
                    int numobjs = 0; // number of processed controls on this GUI
                    foreach (GUIControl control in gui.Controls)
                    {
                        GUIButton button = control as GUIButton;
                        GUILabel label = control as GUILabel;
                        GUIInventory invWindow = control as GUIInventory;
                        GUISlider slider = control as GUISlider;
                        GUITextBox textBox = control as GUITextBox;
                        GUIListBox listBox = control as GUIListBox;
                        GUITextWindowEdge textWindowEdge = control as GUITextWindowEdge;
                        if ((button != null) || (textWindowEdge != null))
                        {
                            objrefptrs[gui.ID][numobjs] = (NativeConstants.GOBJ_BUTTON << 16) | GUIButtonsAndTextWindowEdges.Count;
                            GUIButtonsAndTextWindowEdges.Add(button != null ?
                                (GUIButtonOrTextWindowEdge)button :
                                (GUIButtonOrTextWindowEdge)textWindowEdge);
                        }
                        else if (label != null)
                        {
                            objrefptrs[gui.ID][numobjs] = (NativeConstants.GOBJ_LABEL << 16) | GUILabels.Count;
                            GUILabels.Add(label);
                        }
                        else if (invWindow != null)
                        {
                            objrefptrs[gui.ID][numobjs] = (NativeConstants.GOBJ_INVENTORY << 16) | GUIInvWindows.Count;
                            GUIInvWindows.Add(invWindow);
                        }
                        else if (slider != null)
                        {
                            objrefptrs[gui.ID][numobjs] = (NativeConstants.GOBJ_SLIDER << 16) | GUISliders.Count;
                            GUISliders.Add(slider);
                        }
                        else if (textBox != null)
                        {
                            objrefptrs[gui.ID][numobjs] = (NativeConstants.GOBJ_TEXTBOX << 16) | GUITextBoxes.Count;
                            GUITextBoxes.Add(textBox);
                        }
                        else if (listBox != null)
                        {
                            objrefptrs[gui.ID][numobjs] = (NativeConstants.GOBJ_LISTBOX << 16) | GUIListBoxes.Count;
                            GUIListBoxes.Add(listBox);
                        }
                        ++numobjs;
                    }
                }
                return objrefptrs.ToArray();
            }

            private void WriteNormalGUI(NormalGUI gui)
            {
                FilePutString(gui.Name, writer); // name
                FilePutString(gui.OnClick, writer); // clickEventHandler
                writer.Write(gui.Left); // x
                writer.Write(gui.Top); // y
                writer.Write(gui.Width); // wid
                writer.Write(gui.Height); // hit
                writer.Write(gui.Controls.Count); // numobjs
                writer.Write((int)gui.PopupStyle); // gui style
                writer.Write(gui.PopupYPos); // popupyp
                writer.Write(gui.BackgroundColor); // bgcol
                writer.Write(gui.BackgroundImage); // bgpic
                writer.Write(gui.BorderColor); // fgcol
                // GUI Flags
                writer.Write(MakeGUIFlags(gui));
                int transparency = gui.Transparency;
                if (transparency <= 0) transparency = 0;
                else if (transparency >= 100) transparency = 255;
                else transparency = ((100 - transparency) * 25) / 10;
                writer.Write(transparency); // transparency
                writer.Write(gui.ZOrder); // zorder
                writer.Write(0); // guiId
                writer.Write(NativeConstants.TEXTWINDOW_PADDING_DEFAULT); // padding
            }

            private int MakeGUIFlags(NormalGUI gui)
            {
                int flags =
                    (gui.Clickable ? NativeConstants.GUIMAIN_CLICKABLE : 0) |
                    (gui.Visible ? NativeConstants.GUIMAIN_VISIBLE : 0);
                return flags;
            }

            private void WriteTextWindowGUI(TextWindowGUI gui)
            {
                FilePutString(gui.Name, writer); // name
                FilePutString(null, writer); // clickEventHandler
                writer.Write(0); // x
                writer.Write(0); // y
                writer.Write(200); // wid
                writer.Write(100); // hit
                writer.Write(gui.Controls.Count); // numobjs
                writer.Write(NativeConstants.GUI_POPUP_MODAL); // popup
                writer.Write(-1); // popupyp
                writer.Write(gui.BackgroundColor); // bgcol
                writer.Write(gui.BackgroundImage); // bgpic
                writer.Write(gui.TextColor); // fgcol
                writer.Write(NativeConstants.GUIMAIN_TEXTWINDOW); // flags
                writer.Write(0); // transparency
                writer.Write(-1); // zorder
                writer.Write(0); // guiId
                writer.Write(gui.Padding); // padding
            }

            public void WriteAllGUIs()
            {
                int version = NativeConstants.GUIVersion.Current;
                int[][] objrefptrs = PopulateGUIControls();
                writer.Write(NativeConstants.GUIMAGIC);
                writer.Write(version);
                writer.Write(game.GUIs.Count);
                foreach (GUI gui in game.GUIs)
                {
                    NormalGUI normalGUI = gui as NormalGUI;
                    if (normalGUI != null) WriteNormalGUI(normalGUI);
                    else WriteTextWindowGUI(gui as TextWindowGUI);
                    for (int i = 0; i < game.GUIs[gui.ID].Controls.Count; ++i)
                    {
                        writer.Write(objrefptrs[gui.ID][i]); // objrefptr
                    }
                }
                WriteAllButtonsAndTextWindowEdges();
                WriteAllLabels();
                WriteAllInvWindows();
                WriteAllSliders();
                WriteAllTextBoxes();
                WriteAllListBoxes();
            }
        }

        private static bool WritePluginsToDisk(BinaryWriter writer, Game game, CompileMessages errors)
        {
            if (game.Plugins.Count > NativeConstants.MAX_PLUGINS)
            {
                errors.Add(new CompileError("Too many plugins"));
                return false;
            }
            writer.Write(1); // version
            writer.Write(game.Plugins.Count);
            foreach (Plugin plugin in game.Plugins)
            {
                FilePutNullTerminatedString(plugin.FileName, writer);
                int savesize = plugin.SerializedData.Length;
                if (savesize > NativeConstants.SAVEBUFFERSIZE)
                {
                    System.Windows.Forms.MessageBox.Show("Plugin tried to write too much data to game file.");
                    savesize = 0;
                }
                writer.Write(savesize);
                if (savesize > 0) writer.Write(plugin.SerializedData);
            }
            return true;
        }

        public static bool SaveThisGameToFile(string fileName, Game game, CompileMessages errors)
        {
            FileStream ostream = File.Create(fileName);
            if (ostream == null)
            {
                errors.Add(new CompileError(string.Format("Cannot open file {0} for writing", fileName)));
                return false;
            }
            BinaryWriter writer = new BinaryWriter(ostream);
            WriteString(NativeConstants.GAME_FILE_SIG, NativeConstants.GAME_FILE_SIG.Length, writer);
            writer.Write(NativeConstants.GAME_DATA_VERSION_CURRENT);
            writer.Write(AGS.Types.Version.AGS_EDITOR_VERSION.Length);
            WriteString(AGS.Types.Version.AGS_EDITOR_VERSION, AGS.Types.Version.AGS_EDITOR_VERSION.Length, writer);
            // Write extended engine caps; none for this version
            writer.Write((int)0);
            // An example of writing caps (pseudo-code):
            //   writer.Write(caps.Count);
            //   foreach (cap in caps)
            //       FilePutString(cap.Name);
            //
            WriteGameSetupStructBase_Aligned(writer, game);
            WriteString(game.Settings.GUIDAsString, NativeConstants.MAX_GUID_LENGTH, writer);
            WriteString(game.Settings.SaveGameFileExtension, NativeConstants.MAX_SG_EXT_LENGTH, writer);
            WriteString(game.Settings.SaveGameFolderName, NativeConstants.MAX_SG_FOLDER_LEN, writer);
            for (int i = 0; i < game.Fonts.Count; ++i)
            {
                int flags = 0;
                if (game.Fonts[i].PointSize == 0)
                {
                    flags = NativeConstants.FFLG_SIZEMULTIPLIER;
                }
                writer.Write(flags);
                if ((flags & NativeConstants.FFLG_SIZEMULTIPLIER) == 0)
                    writer.Write(game.Fonts[i].PointSize * game.Fonts[i].SizeMultiplier);
                else
                    writer.Write(game.Fonts[i].SizeMultiplier);

                int outline = -1;
                if (game.Fonts[i].OutlineStyle == FontOutlineStyle.Automatic)
                    outline = NativeConstants.FONT_OUTLINE_AUTO;
                else if (game.Fonts[i].OutlineStyle != FontOutlineStyle.None)
                    outline = game.Fonts[i].OutlineFont;
                writer.Write(outline);

                writer.Write(game.Fonts[i].VerticalOffset);
                writer.Write(game.Fonts[i].LineSpacing);
            }
            int topmostSprite;
            byte[] spriteFlags = new byte[NativeConstants.MAX_STATIC_SPRITES];
            UpdateSpriteFlags(game.RootSpriteFolder, spriteFlags, out topmostSprite);
            writer.Write(topmostSprite + 1);
            for (int i = 0; i <= topmostSprite; ++i)
            {
                writer.Write(spriteFlags[i]);
            }
            if (game.InventoryItems.Count > NativeConstants.MAX_INV)
            {
                errors.Add(new CompileError("Too many inventory items"));
                return false;
            }
            writer.Write(new byte[68]); // inventory item slot 0 is unused
            for (int i = 0; i < game.InventoryItems.Count; ++i)
            {
                WriteString(game.InventoryItems[i].Description, 24, writer);
                writer.Write(new byte[4]); // null terminator plus 3 bytes padding
                writer.Write(game.InventoryItems[i].Image);
                writer.Write(game.InventoryItems[i].CursorImage);
                writer.Write(game.InventoryItems[i].HotspotX);
                writer.Write(game.InventoryItems[i].HotspotY);
                for (int j = 0; j < 5; ++j) // write "reserved", currently unused
                {
                    writer.Write(0);
                }
                writer.Write(game.InventoryItems[i].PlayerStartsWithItem ? NativeConstants.IFLG_STARTWITH : (char)0);
                writer.Write(new byte[3]); // 3 bytes padding
            }
            if (game.Cursors.Count > NativeConstants.MAX_CURSOR)
            {
                errors.Add(new CompileError("Too many cursors"));
                return false;
            }
            for (int i = 0; i < game.Cursors.Count; ++i)
            {
                char flags = (char)0;
                writer.Write(game.Cursors[i].Image);
                writer.Write((short)game.Cursors[i].HotspotX);
                writer.Write((short)game.Cursors[i].HotspotY);
                if (game.Cursors[i].Animate)
                {
                    writer.Write((short)(game.Cursors[i].View - 1));
                    if (game.Cursors[i].AnimateOnlyOnHotspots) flags |= NativeConstants.MCF_HOTSPOT;
                    if (game.Cursors[i].AnimateOnlyWhenMoving) flags |= NativeConstants.MCF_ANIMMOVE;
                }
                else writer.Write((short)-1);
                WriteString(game.Cursors[i].Name, 9, writer);
                writer.Write((byte)0); // null terminator
                if (game.Cursors[i].StandardMode) flags |= NativeConstants.MCF_STANDARD;
                writer.Write(flags);
                writer.Write(new byte[3]); // 3 bytes padding
            }
            for (int i = 0; i < game.Characters.Count; ++i)
            {
                SerializeInteractionScripts(game.Characters[i].Interactions, writer);
            }
            for (int i = 1; i <= game.InventoryItems.Count; ++i)
            {
                SerializeInteractionScripts(game.InventoryItems[i - 1].Interactions, writer);
            }
            writer.Write(game.TextParser.Words.Count);
            for (int i = 0; i < game.TextParser.Words.Count; ++i)
            {
                WriteStringEncrypted(writer, SafeTruncate(game.TextParser.Words[i].Word, NativeConstants.MAX_PARSER_WORD_LENGTH));
                writer.Write((short)game.TextParser.Words[i].WordGroup);
            }
            if (!WriteCompiledScript(ostream, game.ScriptsToCompile.GetScriptByFilename(Script.GLOBAL_SCRIPT_FILE_NAME), errors) ||
                !WriteCompiledScript(ostream, game.ScriptsToCompile.GetScriptByFilename(Script.DIALOG_SCRIPTS_FILE_NAME), errors))
            {
                return false;
            }
            // Extract all the scripts we want to persist (all the non-headers, except
            // the global script which was already written)
            List<Script> scriptsToWrite = new List<Script>();
            foreach (ScriptAndHeader scriptAndHeader in game.ScriptsToCompile)
            {
                Script script = scriptAndHeader.Script;
                if (script != null)
                {
                    if ((!script.FileName.Equals(Script.GLOBAL_SCRIPT_FILE_NAME)) &&
                        (!script.FileName.Equals(Script.DIALOG_SCRIPTS_FILE_NAME)))
                    {
                        scriptsToWrite.Add(script);
                    }
                }
            }
            writer.Write(scriptsToWrite.Count);
            foreach (Script script in scriptsToWrite)
            {
                if (!WriteCompiledScript(ostream, script, errors))
                {
                    return false;
                }
            }
            ViewsWriter viewsWriter = new ViewsWriter(writer, game);
            if (!viewsWriter.WriteViews(FolderHelper.GetRootViewFolder(game), game, errors))
            {
                return false;
            }
            foreach (Character character in game.Characters)
            {
                int flags = 0;
                if (character.AdjustSpeedWithScaling) flags |= NativeConstants.CHF_SCALEMOVESPEED;
                if (character.AdjustVolumeWithScaling) flags |= NativeConstants.CHF_SCALEVOLUME;
                if (!character.Clickable) flags |= NativeConstants.CHF_NOINTERACT;
                if (!character.DiagonalLoops) flags |= NativeConstants.CHF_NODIAGONAL;
                if (character.MovementLinkedToAnimation) flags |= NativeConstants.CHF_ANTIGLIDE;
                if (!character.Solid) flags |= NativeConstants.CHF_NOBLOCKING;
                if (!character.TurnBeforeWalking) flags |= NativeConstants.CHF_NOTURNING;
                if (!character.UseRoomAreaLighting) flags |= NativeConstants.CHF_NOLIGHTING;
                if (!character.UseRoomAreaScaling) flags |= NativeConstants.CHF_MANUALSCALING;
                writer.Write(character.NormalView - 1);                // defview
                writer.Write(character.SpeechView - 1);                // talkview
                writer.Write(character.NormalView - 1);                // view
                writer.Write(character.StartingRoom);                  // room
                writer.Write(0);                                       // prevroom
                writer.Write(character.StartX);                        // x
                writer.Write(character.StartY);                        // y
                writer.Write(0);                                       // wait
                writer.Write(flags);                                   // flags
                writer.Write((short)0);                                // following
                writer.Write((short)0);                                // followinfo
                writer.Write(character.IdleView - 1);                  // idleview
                writer.Write((short)0);                                // idletime
                writer.Write((short)0);                                // idleleft
                writer.Write((short)0);                                // transparency
                writer.Write((short)0);                                // baseline
                writer.Write(0);                                       // activeinv
                writer.Write(character.SpeechColor);                   // talkcolor
                writer.Write(character.ThinkingView - 1);              // thinkview
                writer.Write((short)(character.BlinkingView - 1));     // blinkview
                writer.Write((short)0);                                // blinkinterval
                writer.Write((short)0);                                // blinktimer
                writer.Write((short)0);                                // blinkframe
                writer.Write(character.UniformMovementSpeed ?          // walkspeed_y
                    NativeConstants.UNIFORM_WALK_SPEED :
                    (short)character.MovementSpeedY);
                writer.Write((short)0);                                // pic_yoffs
                writer.Write(0);                                       // z
                writer.Write(0);                                       // walkwait
                writer.Write((short)character.SpeechAnimationDelay);   // speech_anim_speed
                writer.Write((short)0);                                // reserved1
                writer.Write((short)0);                                // blocking_width
                writer.Write((short)0);                                // blocking_height
                writer.Write(0);                                       // index_id
                writer.Write((short)0);                                // pic_xoffs
                writer.Write((short)0);                                // walkwaitcounter
                writer.Write((short)0);                                // loop
                writer.Write((short)0);                                // frame
                writer.Write((short)0);                                // walking
                writer.Write((short)0);                                // animating
                writer.Write(character.UniformMovementSpeed ?          // walkspeed
                    (short)character.MovementSpeed :
                    (short)character.MovementSpeedX);
                writer.Write((short)character.AnimationDelay);         // animspeed
                bool isPlayer = (character == game.PlayerCharacter);
                foreach (InventoryItem invItem in game.InventoryItems) // inv[MAX_INV]
                {
                    if ((isPlayer) && (invItem.PlayerStartsWithItem)) writer.Write((short)1);
                    else writer.Write((short)0);
                }
                if (game.InventoryItems.Count < NativeConstants.MAX_INV)
                {
                    writer.Write(new byte[(NativeConstants.MAX_INV - game.InventoryItems.Count) * sizeof(short)]);
                }
                writer.Write((short)0);                                // actx
                writer.Write((short)0);                                // acty
                WriteString(character.RealName, 40, writer);           // name
                WriteString(character.ScriptName, NativeConstants.MAX_SCRIPT_NAME_LEN, writer); // scrname
                writer.Write((char)1);                                 // on
                writer.Write((byte)0);                                 // alignment padding
            }
            for (int i = 0; i < NativeConstants.MAXLIPSYNCFRAMES; ++i)
            {
                WriteString(game.LipSync.CharactersPerFrame[i], 50, writer);
            }
            for (int i = 0; i < NativeConstants.MAXGLOBALMES; ++i)
            {
                if (string.IsNullOrEmpty(game.GlobalMessages[i])) continue;
                WriteStringEncrypted(writer, game.GlobalMessages[i]);
            }
            foreach (Dialog curDialog in game.Dialogs)
            {
                for (int i = 0; (i < NativeConstants.MAXTOPICOPTIONS) && (i < curDialog.Options.Count); ++i)
                {
                    WriteString(curDialog.Options[i].Text, 150, writer); // optionnames
                }
                for (int i = curDialog.Options.Count; i < NativeConstants.MAXTOPICOPTIONS; ++i)
                {
                    WriteString("", 150, writer);
                }
                for (int i = 0; (i < NativeConstants.MAXTOPICOPTIONS) && (i < curDialog.Options.Count); ++i)
                {
                    DialogOption option = curDialog.Options[i];
                    int flags = 0;
                    if (!option.Say) flags |= NativeConstants.DFLG_NOREPEAT;
                    if (option.Show) flags |= NativeConstants.DFLG_ON;
                    writer.Write(flags); // optionflags
                }
                for (int i = curDialog.Options.Count; i < NativeConstants.MAXTOPICOPTIONS; ++i)
                {
                    writer.Write(0);
                }
                writer.Write(new byte[4]); // optionscripts
                writer.Write(new byte[NativeConstants.MAXTOPICOPTIONS * sizeof(short)]); // entrypoints
                writer.Write((short)0); // startupentrypoint
                writer.Write((short)0); // codesize
                writer.Write(curDialog.Options.Count); // numoptions
                writer.Write(curDialog.ShowTextParser ? NativeConstants.DTFLG_SHOWPARSER : 0); // topicflags
            }
            GUIsWriter guisWriter = new GUIsWriter(writer, game);
            guisWriter.WriteAllGUIs();
            if (!WritePluginsToDisk(writer, game, errors))
            {
                return false;
            }
            writer.Write(NativeConstants.CustomPropertyVersion.Current);
            writer.Write(game.PropertySchema.PropertyDefinitions.Count);
            foreach (CustomPropertySchemaItem schemaItem in game.PropertySchema.PropertyDefinitions)
            {
                FilePutString(schemaItem.Name, writer);
                writer.Write((int)schemaItem.Type);
                FilePutString(schemaItem.Description, writer);
                FilePutString(schemaItem.DefaultValue, writer);
            }
            for (int i = 0; i < game.Characters.Count; ++i)
            {
                CustomPropertiesWriter.Write(writer, game.Characters[i].Properties);
            }
            writer.Write(1); // inv slot 0 is unused, write the property header (int 1)
            writer.Write(0); // then write the number of props used by this inv item (int 0)
            for (int i = 0; i < game.InventoryItems.Count; ++i)
            {
                CustomPropertiesWriter.Write(writer, game.InventoryItems[i].Properties);
            }
            for (int i = 0; i < game.ViewCount; ++i) // ViewCount is highest numbered view
            {
                View view = game.FindViewByID(i + 1);
                if (view != null)
                    FilePutNullTerminatedString(view.Name, writer);
                else
                    writer.Write((byte)0); // view is null, so its name is just a single NUL byte
            }
            writer.Write((byte)0); // inventory slot 0 is unused, so its name is just a single NUL byte
            for (int i = 0; i < game.InventoryItems.Count; ++i)
            {
                FilePutNullTerminatedString(game.InventoryItems[i].Name, writer);
            }
            for (int i = 0; i < game.Dialogs.Count; ++i)
            {
                FilePutNullTerminatedString(game.Dialogs[i].Name, writer);
            }
            writer.Write(game.AudioClipTypes.Count + 1);
            // hard coded SPEECH audio type 0
            writer.Write(0); // id
            writer.Write(1); // reservedChannels
            writer.Write(0); // volume_reduction_while_speech_playing
            writer.Write(0); // crossfadeSpeed
            writer.Write(0); // reservedForFuture
            for (int i = 1; i < (game.AudioClipTypes.Count + 1); ++i)
            {
                writer.Write(i); // id
                writer.Write(game.AudioClipTypes[i - 1].MaxChannels); // reservedChannels
                writer.Write(game.AudioClipTypes[i - 1].VolumeReductionWhileSpeechPlaying); // volume_reduction_while_speech_playing
                writer.Write((int)game.AudioClipTypes[i - 1].CrossfadeClips); // crossfadeSpeed
                writer.Write(0);
            }
            IList<AudioClip> allClips = game.CachedAudioClipListForCompile;
            writer.Write(allClips.Count);
            for (int i = 0; i < allClips.Count; ++i)
            {
                AudioClip clip = allClips[i];
                writer.Write(0); // id
                WriteString(SafeTruncate(clip.ScriptName, 29), 30, writer); // scriptName
                WriteString(SafeTruncate(clip.CacheFileNameWithoutPath, 14), 15, writer); // fileName
                writer.Write((byte)clip.BundlingType); // bundlingType
                writer.Write((byte)clip.Type); // type
                writer.Write((byte)clip.FileType); // fileType
                writer.Write(clip.ActualRepeat ? (byte)1 : (byte)0); // defaultRepeat
                writer.Write((byte)0); // struct alignment padding
                writer.Write((short)clip.ActualPriority); // defaultPriority
                writer.Write((short)clip.ActualVolume); // defaultVolume
                writer.Write(new byte[2]); // struct alignment padding
                writer.Write(0); // reserved
            }
            writer.Write(game.GetAudioArrayIndexFromAudioClipIndex(game.Settings.PlaySoundOnScore));
            if (game.Settings.DebugMode)
            {
                writer.Write(game.Rooms.Count);
                for (int i = 0; i < game.Rooms.Count; ++i)
                {
                    IRoom room = game.Rooms[i];
                    writer.Write(room.Number);
                    if (room.Description != null)
                    {
                        FilePutNullTerminatedString(room.Description, 500, writer);
                    }
                    else writer.Write((byte)0);
                }
            }
            writer.Close();
            GC.Collect();
            return true;
        }
    }
}
