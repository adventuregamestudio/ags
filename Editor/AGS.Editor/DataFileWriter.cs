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
    class DataFileWriter
    {
        const int MAXMULTIFILES = 25;

        class MultiFileLibNew
        {
            const int MAX_FILES = 10000;

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

        class PseudoRandInt
        {
            private static PseudoRandInt _instance;
            private int _seed;
            private int _lastRandValue;

            public static PseudoRandInt Instance
            {
                get
                {
                    if (_instance == null) _instance = new PseudoRandInt((int)DateTime.Now.Ticks);
                    return _instance;
                }
            }

            public static void InitializeInstance(int value)
            {
                _instance = new PseudoRandInt(value);
            }

            private PseudoRandInt(int value)
            {
                _seed = value;
                _lastRandValue = value;
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

        static void FilePutNullTerminatedString(string text, int maxLen, BinaryWriter writer)
        {
            if ((text != null) && (maxLen >= 2))
            {
                int len = text.IndexOf('\0');
                text = text.Substring(0, Math.Min(len == -1 ? text.Length : len, maxLen - 1));
                writer.Write(Encoding.ASCII.GetBytes(text));
            }
            writer.Write(new byte[] { (byte)0 });
        }

        static void FilePutStringEncrypted(string text, BinaryWriter writer)
        {
            byte[] bytes = Encoding.ASCII.GetBytes(text);
            FileWriteDataEncrypted(bytes, writer);
            FileWriteDataEncrypted(new byte[] { (byte)0 }, writer);
        }

        static void FilePutIntEncrypted(int numberToWrite, BinaryWriter writer)
        {
            FileWriteDataEncrypted(BitConverter.GetBytes(numberToWrite), writer);
        }

        static void WriteCLIBHeader(BinaryWriter writer)
        {
            const int RAND_SEED_SALT = 9338638;
            int randSeed = (int)DateTime.Now.Ticks;
            writer.Write(randSeed - RAND_SEED_SALT);
            PseudoRandInt.InitializeInstance(randSeed);
            FilePutIntEncrypted(ourlib.DataFilenames.Count, writer);
            for (int i = 0; i < ourlib.DataFilenames.Count; ++i)
            {
                FilePutStringEncrypted(ourlib.DataFilenames[i], writer);
            }
            FilePutIntEncrypted(ourlib.Files.Count, writer);
            for (int i = 0; i < ourlib.Files.Count; ++i)
            {
                FilePutStringEncrypted(ourlib.Files[i].Filename, writer);
            }
            for (int i = 0; i < ourlib.Files.Count; ++i)
            {
                FileWriteDataEncrypted(BitConverter.GetBytes((int)ourlib.Files[i].Offset), writer);
            }
            for (int i = 0; i < ourlib.Files.Count; ++i)
            {
                FileWriteDataEncrypted(BitConverter.GetBytes((int)ourlib.Files[i].Length), writer);
            }
            for (int i = 0; i < ourlib.Files.Count; ++i)
            {
                FileWriteDataEncrypted(new byte[] { ourlib.Files[i].Datafile }, writer);
            }
        }

        static int CopyFileAcross(Stream instream, Stream copystream, long leftforthis)
        {
            const int CHUNKSIZE = 256000;
            int success = 1;
            byte[] diskbuffer = new byte[CHUNKSIZE + 10];
            while (leftforthis > 0)
            {
                if (leftforthis > CHUNKSIZE)
                {
                    instream.Read(diskbuffer, 0, CHUNKSIZE);
                    try
                    {
                        copystream.Write(diskbuffer, 0, CHUNKSIZE);
                    }
                    catch
                    {
                        success = 0;
                    }
                    finally
                    {
                        success = 1;
                    }
                    leftforthis -= CHUNKSIZE;
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

        public static string MakeDataFile(string[] fileNames, int splitSize, string baseFileName, bool makeFileNameAssumptionsForEXE)
        {
            const string CLIB_END_SIGNATURE = "CLIB\x1\x2\x3\x4SIGE";
            const int MAX_FILENAME_LENGTH = 100;
            const string SPRSET_NAME = "acsprset.spr";
            Environment.CurrentDirectory = AGSEditor.Instance.CurrentGame.DirectoryPath;
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
                    if (string.Compare(fileNames[i], SPRSET_NAME, true) == 0)
                    {
                        // the sprite file's appearance signifies it's time to start splitting
                        doSplitting = true;
                        currentDataFile++;
                        sizeSoFar = 0;
                    }
                    else if ((sizeSoFar > splitSize) && (doSplitting) && (currentDataFile < (MAXMULTIFILES - 1)))
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
                if (fileNameSrc.Length >= MAX_FILENAME_LENGTH)
                {
                    throw new AGSEditorException("Filename too long: " + fileNames[i]);
                }
                ourlib.Files.Add(new MultiFileLibNew.MultiFile(fileNameSrc, (byte)currentDataFile, thisFileSize));
            }
            ourlib.DataFilenames.Capacity = currentDataFile + 1;
            long startOffset = 0;
            long mainHeaderOffset = 0;
            string outputFileName;
            string firstDataFileFullPath = null;
            if (makeFileNameAssumptionsForEXE)
            {
                Directory.CreateDirectory("Compiled");
            }
            // First, set up ourlib.data_filenames array with all the filenames
            // so that write_clib_header will write the correct amount of data
            for (int i = 0, cap = ourlib.DataFilenames.Capacity; i < cap; ++i)
            {
                if (makeFileNameAssumptionsForEXE)
                {
                    ourlib.DataFilenames.Add(baseFileName + "." + (i == 0 ? "exe" : i.ToString("D3")));
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
                        if (!makeFileNameAssumptionsForEXE) ourlib.Files[i].Filename = tomake;
                    }
                }
            }
            // now, create the actual files
            for (int i = 0; i < ourlib.DataFilenames.Count; ++i)
            {
                if (makeFileNameAssumptionsForEXE)
                {
                    outputFileName = Path.Combine("Compiled", ourlib.DataFilenames[i]);
                }
                else
                {
                    outputFileName = baseFileName;
                }
                if (i == 0) firstDataFileFullPath = outputFileName;
                using (Stream wout = TryFileOpen(outputFileName,
                    i == 0 ? FileMode.Append : FileMode.Create, FileAccess.Write))
                {
                    if (wout == null) return "ERROR: unable to open file for writing";
                    BinaryWriter writer = new BinaryWriter(wout);
                    startOffset = writer.BaseStream.Length;
                    writer.Write("CLIB\x1A".ToCharArray());
                    writer.Write((byte)21);
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
                                    throw new AGSEditorException("Unable to find file '" + ourlib.Files[j].Filename + "' for compilation. Do not remove files during the compilation process." + Environment.NewLine + Directory.GetCurrentDirectory());
                                }
                                if (CopyFileAcross(stream, writer.BaseStream, ourlib.Files[j].Length) < 1)
                                {
                                    return "Error writing file: possibly disk full";
                                }
                            }
                        }
                    }
                    if (startOffset > 0)
                    {
                        writer.Write((int)startOffset);
                        writer.Write(CLIB_END_SIGNATURE.ToCharArray());
                    }
                }
            }
            using (Stream wout = TryFileOpen(firstDataFileFullPath, FileMode.Open, FileAccess.ReadWrite))
            {
                wout.Seek(mainHeaderOffset, SeekOrigin.Begin);
                WriteCLIBHeader(new BinaryWriter(wout));
            }
            return null;
        }

        /*
typedef struct RGB
{
   unsigned char r, g, b;
   unsigned char filler; // ALIGNED
} RGB; // 4 bytes
         */
        /*
struct GameSetupStructBase {
    static const int  MAX_OPTIONS = 100;

    char              gamename[50]; // 50/4 = 12R2
    char _padding1[2]; // aligned
    int32             options[MAX_OPTIONS]; // aligned
    unsigned char     paluses[256]; // 256/4 = 64R0, aligned
    color             defpal[256]; // aligned
    int32             numviews;
    int32             numcharacters;
    int32             playercharacter;
    int32             totalscore;
    short             numinvitems; // missing 2
    char _padding2[2]; // aligned
    int32             numdialog, numdlgmessage;
    int32             numfonts;
    int32             color_depth;          // in bytes per pixel (ie. 1 or 2)
    int32             target_win;
    int32             dialog_bullet;        // 0 for none, otherwise slot num of bullet point
    unsigned short    hotdot, hotdotouter;  // inv cursor hotspot dot     // aligned
    int32             uniqueid;    // random key identifying the game
    int32             numgui;
    int32             numcursors;
    GameResolutionType default_resolution; // 4 bytes, aligned
    int32             default_lipsync_frame; // used for unknown chars
    int32             invhotdotsprite;
    int32             reserved[17];
    char             *messages[MAXGLOBALMES]; // char*, aligned
    WordsDictionary  *dict; // pointer, aligned
    char             *globalscript; // pointer, aligned
    CharacterInfo    *chars; // pointer, aligned
    ccScript         *compiled_script; // pointer, aligned

    int32_t          *load_messages; // pointer, aligned
    bool             load_dictionary;
    bool             load_compiled_script;
    // [IKM] 2013-03-30
    // NOTE: it looks like nor 'globalscript', not 'compiled_script' are used
    // to store actual script data anytime; 'ccScript* gamescript' global
    // pointer is used for that instead.

    GameSetupStructBase();
    virtual ~GameSetupStructBase();
    void ReadFromFile(Common::Stream *in);
    void WriteToFile(Common::Stream *out);

    inline bool IsHiRes() const
    {
        return ::IsHiRes(default_resolution);
    }
};
         */
        /*
         * void WriteGameSetupStructBase_Aligned(Stream *out)
{
    GameSetupStructBase *gameBase = (GameSetupStructBase *)&thisgame;
    AlignedStream align_s(out, Common::kAligned_Write);
    gameBase->WriteToFile(&align_s);
}*/
        private enum GameOptions
        {
            OPT_DEBUGMODE = 0,
            OPT_SCORESOUND = 1,
            OPT_WALKONLOOK = 2,
            OPT_DIALOGIFACE = 3,
            OPT_ANTIGLIDE = 4,
            OPT_TWCUSTOM = 5,
            OPT_DIALOGGAP = 6,
            OPT_NOSKIPTEXT = 7,
            OPT_DISABLEOFF = 8,
            OPT_ALWAYSSPCH = 9,
            OPT_SPEECHTYPE = 10,
            OPT_PIXPERFECT = 11,
            OPT_NOWALKMODE = 12,
            OPT_LETTERBOX = 13,
            OPT_FIXEDINVCURSOR = 14,
            OPT_NOLOSEINV = 15,
            OPT_NOSCALEFNT = 16,
            OPT_SPLITRESOURCES = 17,
            OPT_ROTATECHARS = 18,
            OPT_FADETYPE = 19,
            OPT_HANDLEINVCLICKS = 20,
            OPT_MOUSEWHEEL = 21,
            OPT_DIALOGNUMBERED = 22,
            OPT_DIALOGUPWARDS = 23,
            OPT_CROSSFADEMUSIC = 24,
            OPT_ANTIALIASFONTS = 25,
            OPT_THOUGHTGUI = 26,
            OPT_TURNTOFACELOC = 27,
            OPT_RIGHTLEFTWRITE = 28,
            OPT_DUPLICATEINV = 29,
            OPT_SAVESCREENSHOT = 30,
            OPT_PORTRAITSIDE = 31,
            OPT_STRICTSCRIPTING = 32,
            OPT_LEFTTORIGHTEVAL = 33,
            OPT_COMPRESSSPRITES = 34,
            OPT_STRICTSTRINGS = 35,
            OPT_NEWGUIALPHA = 36,
            OPT_RUNGAMEDLGOPTS = 37,
            OPT_NATIVECOORDINATES = 38,
            OPT_GLOBALTALKANIMSPD = 39,
            OPT_SPRITEALPHA = 40,
            OPT_NOMODMUSIC = 98,
            OPT_LIPSYNCTEXT = 99
        }

        private static void WriteGameSetupStructBase_Aligned(BinaryWriter writer, Game game)
        {
            // assume stream is aligned at start
            WriteString(SafeTruncate(game.Settings.GameName, 49), 50, writer);
            writer.Write(new byte[2]); // alignment padding
            int[] options = new int[100];
            options[(int)GameOptions.OPT_ALWAYSSPCH] = (game.Settings.AlwaysDisplayTextAsSpeech ? 1 : 0);
            options[(int)GameOptions.OPT_ANTIALIASFONTS] = (game.Settings.AntiAliasFonts ? 1 : 0);
            options[(int)GameOptions.OPT_ANTIGLIDE] = (game.Settings.AntiGlideMode ? 1 : 0);
            options[(int)GameOptions.OPT_NOWALKMODE] = (game.Settings.AutoMoveInWalkMode ? 0 : 1);
            options[(int)GameOptions.OPT_RIGHTLEFTWRITE] = (game.Settings.BackwardsText ? 1 : 0);
            options[(int)GameOptions.OPT_COMPRESSSPRITES] = (game.Settings.CompressSprites ? 1 : 0);
            options[(int)GameOptions.OPT_DEBUGMODE] = (game.Settings.DebugMode ? 1 : 0);
            options[(int)GameOptions.OPT_DIALOGUPWARDS] = (game.Settings.DialogOptionsBackwards ? 1 : 0);
            options[(int)GameOptions.OPT_DIALOGGAP] = game.Settings.DialogOptionsGap;
            options[(int)GameOptions.OPT_DIALOGIFACE] = game.Settings.DialogOptionsGUI;
            options[(int)GameOptions.OPT_DUPLICATEINV] = (game.Settings.DisplayMultipleInventory ? 1 : 0);
            options[(int)GameOptions.OPT_STRICTSTRINGS] = (game.Settings.EnforceNewStrings ? 1 : 0);
            options[(int)GameOptions.OPT_STRICTSCRIPTING] = (game.Settings.EnforceObjectBasedScript ? 1 : 0);
            options[(int)GameOptions.OPT_NOSCALEFNT] = (game.Settings.FontsForHiRes ? 1 : 0);
            options[(int)GameOptions.OPT_NEWGUIALPHA] = (int)game.Settings.GUIAlphaStyle;
            options[(int)GameOptions.OPT_SPRITEALPHA] = (int)game.Settings.SpriteAlphaStyle;
            options[(int)GameOptions.OPT_HANDLEINVCLICKS] = (game.Settings.HandleInvClicksInScript ? 1 : 0);
            options[(int)GameOptions.OPT_FIXEDINVCURSOR] = (game.Settings.InventoryCursors ? 0 : 1);
            options[(int)GameOptions.OPT_GLOBALTALKANIMSPD] = (game.Settings.UseGlobalSpeechAnimationDelay ?
                game.Settings.GlobalSpeechAnimationDelay : (-game.Settings.GlobalSpeechAnimationDelay - 1));
            options[(int)GameOptions.OPT_LEFTTORIGHTEVAL] = (game.Settings.LeftToRightPrecedence ? 1 : 0);
            options[(int)GameOptions.OPT_LETTERBOX] = (game.Settings.LetterboxMode ? 1 : 0);
            options[(int)GameOptions.OPT_MOUSEWHEEL] = (game.Settings.MouseWheelEnabled ? 1 : 0);
            options[(int)GameOptions.OPT_DIALOGNUMBERED] = (game.Settings.NumberDialogOptions ? 1 : 0);
            options[(int)GameOptions.OPT_PIXPERFECT] = (game.Settings.PixelPerfect ? 1 : 0);
            options[(int)GameOptions.OPT_FADETYPE] = (int)game.Settings.RoomTransition;
            options[(int)GameOptions.OPT_RUNGAMEDLGOPTS] = (game.Settings.RunGameLoopsWhileDialogOptionsDisplayed ? 1 : 0);
            options[(int)GameOptions.OPT_SAVESCREENSHOT] = (game.Settings.SaveScreenshots ? 1 : 0);
            options[(int)GameOptions.OPT_NOSKIPTEXT] = (int)game.Settings.SkipSpeech;
            options[(int)GameOptions.OPT_PORTRAITSIDE] = (int)game.Settings.SpeechPortraitSide;
            options[(int)GameOptions.OPT_SPEECHTYPE] = (int)game.Settings.SpeechStyle;
            options[(int)GameOptions.OPT_SPLITRESOURCES] = game.Settings.SplitResources;
            options[(int)GameOptions.OPT_TWCUSTOM] = game.Settings.TextWindowGUI;
            options[(int)GameOptions.OPT_THOUGHTGUI] = game.Settings.ThoughtGUI;
            options[(int)GameOptions.OPT_TURNTOFACELOC] = (game.Settings.TurnBeforeFacing ? 1 : 0);
            options[(int)GameOptions.OPT_ROTATECHARS] = (game.Settings.TurnBeforeWalking ? 1 : 0);
            options[(int)GameOptions.OPT_NATIVECOORDINATES] = (game.Settings.UseLowResCoordinatesInScript ? 0 : 1);
            options[(int)GameOptions.OPT_WALKONLOOK] = (game.Settings.WalkInLookMode ? 1 : 0);
            options[(int)GameOptions.OPT_DISABLEOFF] = (int)game.Settings.WhenInterfaceDisabled;
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
            writer.Write((short)game.InventoryItems.Count);
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
            writer.Write((int)game.Settings.Resolution);
            writer.Write(game.LipSync.DefaultFrame);
            writer.Write(game.Settings.InventoryHotspotMarker.Style == InventoryHotspotMarkerStyle.Sprite ?
                game.Settings.InventoryHotspotMarker.Image : 0);
            writer.Write(new byte[17 * sizeof(int)]); // reserved; 17 ints, alignment preserved
            for (int i = 0; i < 500; ++i) // MAXGLOBALMES; write 500 ints, alignment preserved
            {
                writer.Write(game.GlobalMessages[i] != null ? 1 : 0);
            }
            // the rest are ints, alignment is correct
            writer.Write(1); // dict != null
            writer.Write(0); // globalscript != null
            writer.Write(0); // chars != null
            writer.Write(1); // compiled_script != null
            // no final padding required
        }

        [Flags]
        private enum SpriteFlags
        {
            HighRes = 1,
            HighColor = 2,
            TrueColor = 8,
            AlphaChannel = 0x10
        }

        private static SpriteFlags GetSpriteFlags(Sprite sprite)
        {
            SpriteFlags flags = (SpriteFlags)0;
            if (sprite.Resolution == SpriteImportResolution.HighRes) flags |= SpriteFlags.HighRes;
            if (sprite.ColorDepth == 32) flags |= SpriteFlags.TrueColor;
            else if (sprite.ColorDepth >= 15) flags |= SpriteFlags.HighColor;
            if (sprite.AlphaChannel) flags |= SpriteFlags.AlphaChannel;
            return flags;
        }

        private static List<SpriteFlags> GetAllSpriteFlags(SpriteFolder rootFolder)
        {
            List<SpriteFlags> flags = new List<SpriteFlags>();
            foreach (Sprite sprite in rootFolder.Sprites)
            {
                flags.Add(GetSpriteFlags(sprite));
            }
            foreach (SpriteFolder folder in rootFolder.SubFolders)
            {
                flags.AddRange(GetAllSpriteFlags(folder));
            }
            return flags;
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
        /// Writes a string to the file as ASCII bytes. Length must be provided
        /// and will pad or truncate the text as necessary.
        /// </summary>
        private static void WriteString(string src, int length, BinaryWriter writer)
        {
            if ((writer == null) || (length <= 0)) return;
            byte[] bytes = null;
            if (string.IsNullOrEmpty(src)) bytes = new byte[length];
            else
            {
                src = SafeTruncate(src, length);
                if (src.Length < length)
                {
                    List<char> chars = new List<char>(src.ToCharArray());
                    chars.AddRange(new char[length - src.Length]);
                    bytes = Encoding.ASCII.GetBytes(chars.ToArray());
                }
                else bytes = Encoding.ASCII.GetBytes(src);
            }
            writer.Write(bytes);
        }

        private static string ReadString(int length, BinaryReader reader)
        {
            return new string(reader.ReadChars(length));
        }

        private class CompiledCustomProperties
        {
            public const int MAX_CUSTOM_PROPERTIES = 30;
            public const int MAX_CUSTOM_PROPERTY_NAME_LENGTH = 200;
            public const int MAX_CUSTOM_PROPERTY_VALUE_LENGTH = 500;
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
                if (PropertyCount >= MAX_CUSTOM_PROPERTIES) return;
                _names.Add(SafeTruncate(name, MAX_CUSTOM_PROPERTY_NAME_LENGTH - 1));
                _values.Add(SafeTruncate(value, MAX_CUSTOM_PROPERTY_VALUE_LENGTH - 1));
            }

            public void Serialize(BinaryWriter writer)
            {
                writer.Write(1);
                writer.Write(PropertyCount);
                for (int i = 0; i < PropertyCount; ++i)
                {
                    FilePutNullTerminatedString(Names[i], MAX_CUSTOM_PROPERTY_NAME_LENGTH, writer);
                    FilePutNullTerminatedString(Values[i], MAX_CUSTOM_PROPERTY_VALUE_LENGTH, writer);
                }
            }

            int UnSerialize(BinaryReader reader)
            {
                if (reader.ReadInt32() != 1) return -1;
                int count = reader.ReadInt32();
                for (int i = 0; i < count; ++i)
                {
                    string name = ReadString(MAX_CUSTOM_PROPERTY_NAME_LENGTH, reader);
                    string value = ReadString(MAX_CUSTOM_PROPERTY_VALUE_LENGTH, reader);
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

        static string EncryptText(string toEncrypt)
        {
            const string PASSWORD_ENC_STRING = "Avis Durgan";
            StringBuilder sb = new StringBuilder(toEncrypt);
            int p = 0;
            for (int i = 0; i < toEncrypt.Length; ++i, ++p)
            {
                if (p == PASSWORD_ENC_STRING.Length) p = 0;
                sb[i] += PASSWORD_ENC_STRING[p];
            }
            sb.Append(PASSWORD_ENC_STRING[p]);
            return sb.ToString();
        }

        static void WriteStringEncrypted(BinaryWriter writer, string text)
        {
            int stlent = text.Length + 1;
            writer.Write(stlent);
            text = EncryptText(text);
            WriteString(text, stlent, writer);
        }

        static void WriteCompiledScript(FileStream ostream, Script script)
        {
            if (script.CompiledData == null)
            {
                throw new CompileError(string.Format("Script has not been compiled: {0}", script.FileName));
            }
            script.CompiledData.Write(ostream, script.FileName);
        }

        class ViewsWriter
        {
            private FolderHelper.ViewFolderProcessing processingDelegate;
            private BinaryWriter writer;

            public ViewsWriter(BinaryWriter writer)
            {
                this.writer = writer;
                processingDelegate = new FolderHelper.ViewFolderProcessing(WriteViews);
            }

            public void WriteViews(IViewFolder folder, Game game)
            {
                const int LOOPFLAG_RUNNEXTLOOP = 1;
                const int VFLG_FLIPSPRITE = 1;
                if (writer == null)
                {
                    throw new CompileError("Could not write views: Invalid stream (NULL)");
                }
                FolderHelper.ForEachViewFolder(folder, game, processingDelegate);
                foreach (View view in folder.Views)
                {
                    short numLoops = (short)view.Loops.Count;
                    writer.Write(numLoops);
                    for (int i = 0; i < numLoops; ++i)
                    {
                        short numFrames = (short)view.Loops[i].Frames.Count;
                        writer.Write(numFrames);
                        writer.Write(view.Loops[i].RunNextLoop ? LOOPFLAG_RUNNEXTLOOP : 0);
                        for (int j = 0; j < numFrames; ++j)
                        {
                            ViewFrame frame = view.Loops[i].Frames[j];
                            writer.Write(frame.Image);
                            writer.Write((short)0); // unused x-offset
                            writer.Write((short)0); // unused y-offset
                            writer.Write((short)frame.Delay);
                            writer.Write((short)0); // struct alignment padding
                            writer.Write(frame.Flipped ? VFLG_FLIPSPRITE : 0);
                            writer.Write(frame.Sound > 0 ? game.GetAudioArrayIndexFromAudioClipIndex(frame.Sound) : -1);
                            writer.Write(0); // unused reservedForFuture[0]
                            writer.Write(0); // unused reservedForFuture[1]
                        }
                    }
                }
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

            private enum GUIVersion
            {
                VersionInitial = 0,
                /*Version214 = 100,
                Version222 = 101,
                Version230 = 102,
                VersionUnknown103 = 103,
                VersionUnknown104 = 104,
                Version260 = 105,
                VersionUnknown106 = 106,
                VersionUnknown107 = 107,
                VersionUnknown108 = 108,
                VersionUnknown109 = 109,
                Version270 = 110,
                Version272a = 111,
                Version272b = 112,
                Version272c = 113,
                Version272d = 114,*/
                Version272e = 115,
                Version330 = 116,
                Version331 = 117,
                VersionCurrent = Version331,
                VersionForwardCompatible = Version272e
            }

            private enum GUIFlags
            {
                // TODO: This should probably be a "Constants" class to avoid having to cast
                // every time one of these is used.
                GUIF_NOCLICK = 1,
                GUIF_CLIP = 0x0020,
                GUIF_TRANSLATED = 0x0080,
                GLF_NOBORDER = 1,
                GLF_NOARROWS = 2,
                POPUP_NONE = 0,
                POPUP_MOUSEY = 1,
                POPUP_SCRIPT = 2,
                POPUP_NOAUTOREM = 3,
                POPUP_NONEINITIALLYOFF = 4,
                GUI_TEXTWINDOW = 0x05,
                GTF_NOBORDER = 1,
                MAX_GUILABEL_TEXT_LEN = 2048,
                GALIGN_LEFT = 0,
                GALIGN_RIGHT = 1,
                GALIGN_CENTRE = 2,
                MAX_GUIOBJ_SCRIPTNAME_LEN = 25,
                MAX_GUIOBJ_EVENTHANDLER_LEN = 30,
                MAX_GUIOBJ_EVENTS = 10,
                MAX_OBJS_ON_GUI = 30,
                GOBJ_BUTTON = 1,
                GOBJ_LABEL = 2,
                GOBJ_INVENTORY = 3,
                GOBJ_SLIDER = 4,
                GOBJ_TEXTBOX = 5,
                GOBJ_LISTBOX = 6
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

                public TextAlignment TextAlignment
                {
                    get
                    {
                        GUIButton button = (GUIButton)this;
                        if (button != null) return button.TextAlignment;
                        return TextAlignment.TopMiddle;
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

            /// <summary>
            /// Writes the common elements of this GUIControl to the file. Type-specific
            /// data is written only by the respective method for that type.
            /// </summary>
            private void WriteGUIControl(GUIControl control, int flags, string[] events)
            {
                writer.Write(flags); // flags
                writer.Write(control.Left);
                writer.Write(control.Top);
                writer.Write(control.Width);
                writer.Write(control.Height);
                writer.Write(control.ZOrder);
                writer.Write(0); // activated
                string buffer = SafeTruncate(control.Name, (int)GUIFlags.MAX_GUIOBJ_SCRIPTNAME_LEN);
                FilePutNullTerminatedString(buffer, buffer.Length, writer);
                writer.Write(events.Length); // numSupportedEvents
                foreach (string sevent in events)
                {
                    buffer = SafeTruncate(sevent, (int)GUIFlags.MAX_GUIOBJ_EVENTHANDLER_LEN);
                    FilePutNullTerminatedString(buffer, buffer.Length, writer);
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
                    string[] events;
                    flags = (ctrl.ClipImage ? (int)GUIFlags.GUIF_CLIP : 0);
                    events = (ctrl.OnClick == null ? new string[0] : new string[] { ctrl.OnClick });
                    WriteGUIControl(ctrl, flags, events);
                    writer.Write(ctrl.Image); // pic
                    writer.Write(ctrl.MouseoverImage); // overpic
                    writer.Write(ctrl.PushedImage); // pushedpic
                    writer.Write(ctrl.Image); // usepic
                    writer.Write(0); // ispushed
                    writer.Write(0); // isover
                    writer.Write(ctrl.Font); // font
                    writer.Write(ctrl.TextColor); // textcol
                    writer.Write((int)ctrl.ClickAction); // leftclick
                    writer.Write(0); // rightclick
                    writer.Write(ctrl.NewModeNumber); // lclickdata
                    writer.Write(0); // rclickdata
                    WriteString(ctrl.Text, 50, writer); // text
                    writer.Write((int)ctrl.TextAlignment); // textAlignment
                    writer.Write(0); // reserved1
                }
            }

            private void WriteAllLabels()
            {
                writer.Write(GUILabels.Count);
                foreach (GUILabel label in GUILabels)
                {
                    WriteGUIControl(label, 0);
                    string text = SafeTruncate(label.Text, (int)GUIFlags.MAX_GUILABEL_TEXT_LEN);
                    writer.Write(text.Length + 1);
                    FilePutNullTerminatedString(text, text.Length + 1, writer);
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
                    writer.Write(0); // topIndex
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
                    writer.Write(0); // mpressed
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
                    WriteString(textBox.Text, 200, writer);
                    writer.Write(textBox.Font);
                    writer.Write(textBox.TextColor);
                    writer.Write(textBox.ShowBorder ? 0 : (int)GUIFlags.GTF_NOBORDER);
                }
            }

            private void WriteAllListBoxes()
            {
                writer.Write(GUIListBoxes.Count);
                foreach (GUIListBox listBox in GUIListBoxes)
                {
                    int flags = (listBox.Translated ? (int)GUIFlags.GUIF_TRANSLATED : 0);
                    WriteGUIControl(listBox, flags, new string[] { listBox.OnSelectionChanged });
                    writer.Write(0); // numItems
                    writer.Write(0); // selected
                    writer.Write(0); // topItem
                    writer.Write(0); // mousexp
                    writer.Write(0); // mouseyp
                    writer.Write(0); // rowheight
                    writer.Write(0); // num_items_fit
                    writer.Write(listBox.Font);
                    writer.Write(listBox.TextColor);
                    writer.Write(listBox.SelectedTextColor);
                    int exflags = (listBox.ShowBorder ? 0 : (int)GUIFlags.GLF_NOBORDER);
                    exflags |= (listBox.ShowScrollArrows ? 0 : (int)GUIFlags.GLF_NOARROWS);
                    writer.Write(exflags);
                    writer.Write((int)listBox.TextAlignment);
                    writer.Write(0); // reserved1
                    writer.Write(listBox.SelectedBackgroundColor);
                }
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
                int[][] objrefptrs = new int[game.GUIs.Count][];
                foreach (GUI gui in game.GUIs)
                {
                    objrefptrs[gui.ID] = new int[(int)GUIFlags.MAX_OBJS_ON_GUI];
                    int numobjs = 0; // number of processed controls on this GUI
                    foreach (GUIControl control in gui.Controls)
                    {
                        // TODO: Raise error if too many GUIControls
                        GUIButton button = control as GUIButton;
                        GUILabel label = control as GUILabel;
                        GUIInventory invWindow = control as GUIInventory;
                        GUISlider slider = control as GUISlider;
                        GUITextBox textBox = control as GUITextBox;
                        GUIListBox listBox = control as GUIListBox;
                        GUITextWindowEdge textWindowEdge = control as GUITextWindowEdge;
                        if ((button != null) || (textWindowEdge != null))
                        {
                            objrefptrs[gui.ID][numobjs] = ((int)GUIFlags.GOBJ_BUTTON << 16) | GUIButtonsAndTextWindowEdges.Count;
                            GUIButtonsAndTextWindowEdges.Add(button != null ?
                                (GUIButtonOrTextWindowEdge)button :
                                (GUIButtonOrTextWindowEdge)textWindowEdge);
                        }
                        else if (label != null)
                        {
                            objrefptrs[gui.ID][numobjs] = ((int)GUIFlags.GOBJ_LABEL << 16) | GUILabels.Count;
                            GUILabels.Add(label);
                        }
                        else if (invWindow != null)
                        {
                            objrefptrs[gui.ID][numobjs] = ((int)GUIFlags.GOBJ_INVENTORY << 16) | GUIInvWindows.Count;
                            GUIInvWindows.Add(invWindow);
                        }
                        else if (slider != null)
                        {
                            objrefptrs[gui.ID][numobjs] = ((int)GUIFlags.GOBJ_SLIDER << 16) | GUISliders.Count;
                            GUISliders.Add(slider);
                        }
                        else if (textBox != null)
                        {
                            objrefptrs[gui.ID][numobjs] = ((int)GUIFlags.GOBJ_TEXTBOX << 16) | GUITextBoxes.Count;
                            GUITextBoxes.Add(textBox);
                        }
                        else if (listBox != null)
                        {
                            objrefptrs[gui.ID][numobjs] = ((int)GUIFlags.GOBJ_LISTBOX << 16) | GUIListBoxes.Count;
                            GUIListBoxes.Add(listBox);
                        }
                        ++numobjs;
                    }
                }
                return objrefptrs;
            }

            private void WriteNormalGUI(NormalGUI gui)
            {
                writer.Write(new byte[4]); // vtext
                WriteString(SafeTruncate(gui.Name, 15), 16, writer); // name
                WriteString(SafeTruncate(gui.OnClick, 19), 20, writer); // clickEventHandler
                writer.Write(gui.Left); // x
                writer.Write(gui.Top); // y
                writer.Write(gui.Width); // wid
                writer.Write(gui.Height); // hit
                writer.Write(0); // focus
                writer.Write(gui.Controls.Count); // numobjs
                writer.Write((int)gui.Visibility); // popup
                writer.Write(gui.PopupYPos); // popupyp
                writer.Write(gui.BackgroundColor); // bgcol
                writer.Write(gui.BackgroundImage); // bgpic
                writer.Write(gui.BorderColor); // fgcol
                writer.Write(-1); // mouseover
                writer.Write(-1); // mousewasx
                writer.Write(-1); // mousewasy
                writer.Write(-1); // mousedownon
                writer.Write(-1); // highlightobj
                writer.Write(gui.Clickable ? 0 : (int)GUIFlags.GUIF_NOCLICK); // flags
                int transparency = gui.Transparency;
                if (transparency <= 0) transparency = 0;
                else if (transparency >= 100) transparency = 255;
                else transparency = ((100 - transparency) * 25) / 10;
                writer.Write(transparency); // transparency
                writer.Write(gui.ZOrder); // zorder
                writer.Write(0); // guiId, TODO: should this be gui.ID?
                writer.Write(0); // padding
                writer.Write(new byte[5 * sizeof(int)]); // reserved
                writer.Write(1); // on
                writer.Write(new byte[(int)GUIFlags.MAX_OBJS_ON_GUI * sizeof(int)]); // dummy 32-bit pointers
            }

            private void WriteTextWindowGUI(TextWindowGUI gui)
            {
                writer.Write((byte)GUIFlags.GUI_TEXTWINDOW); // vtext...
                writer.Write(new byte[3]); // ...vtext
                WriteString(SafeTruncate(gui.Name, 15), 16, writer); // name
                writer.Write(new byte[20]); // clickEventHandler
                writer.Write(0); // x
                writer.Write(0); // y
                writer.Write(200); // wid
                writer.Write(100); // hit
                writer.Write(0); // focus
                writer.Write(gui.Controls.Count); // numobjs
                writer.Write((int)GUIFlags.POPUP_SCRIPT); // popup
                writer.Write(0); // popupyp
                writer.Write(gui.BackgroundColor); // bgcol
                writer.Write(gui.BackgroundImage); // bgpic
                writer.Write(gui.TextColor); // fgcol
                writer.Write(-1); // mouseover
                writer.Write(-1); // mousewasx
                writer.Write(-1); // mousewasy
                writer.Write(-1); // mousedownon
                writer.Write(-1); // highlightobj
                writer.Write(0); // flags
                writer.Write(0); // transparency
                writer.Write(0); // zorder
                writer.Write(0); // guiId, TODO: should this be gui.ID?
                writer.Write(gui.Padding); // padding
                writer.Write(new byte[5 * sizeof(int)]); // reserved
                writer.Write(1); // on
                writer.Write(new byte[(int)GUIFlags.MAX_OBJS_ON_GUI * sizeof(int)]); // dummy 32-bit pointers
            }

            public void WriteAllGUIs()
            {
                const uint GUIMAGIC = 0xCAFEBEEF;
                GUIVersion version = GUIVersion.VersionCurrent;
                int[][] objrefptrs = PopulateGUIControls();
                writer.Write(GUIMAGIC);
                writer.Write((int)version);
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

        private static void WritePluginsToDisk(BinaryWriter writer, Game game)
        {
            const int SAVEBUFFERSIZE = 5120;
            writer.Write(1); // version
            writer.Write(game.Plugins.Count);
            foreach (Plugin plugin in game.Plugins)
            {
                FilePutNullTerminatedString(plugin.FileName, plugin.FileName.Length + 1, writer);
                int savesize = plugin.SerializedData.Length;
                if (savesize > SAVEBUFFERSIZE)
                {
                    System.Windows.Forms.MessageBox.Show("Plugin tried to write too much data to game file.");
                    savesize = 0;
                }
                writer.Write(savesize);
                if (savesize > 0) writer.Write(plugin.SerializedData);
            }
        }

        public static void SaveThisGameToFile(string fileName, Game game)
        {
            const string GAME_FILE_SIG = "Adventure Creator Game File v2";
            const int GAME_DATA_VERSION_CURRENT = 44;
            const int MAX_GUID_LENGTH = 40;
            const int MAX_SG_EXT_LENGTH = 20;
            const int MAX_SG_FOLDER_LEN = 50;
            const int MAX_SCRIPT_NAME_LEN = 20;
            const int FFLG_SIZEMASK = 0x003F;
            const char IFLG_STARTWITH = (char)1;
            const char MCF_ANIMMOVE = (char)1;
            const char MCF_STANDARD = (char)4;
            const char MCF_HOTSPOT = (char)8; // only animate when over hotspot
            const int CHF_MANUALSCALING = 1;
            const int CHF_NOINTERACT = 4;
            const int CHF_NODIAGONAL = 8;
            const int CHF_NOLIGHTING = 0x20;
            const int CHF_NOTURNING = 0x40;
            const int CHF_NOBLOCKING = 0x200;
            const int CHF_SCALEMOVESPEED = 0x400;
            const int CHF_SCALEVOLUME = 0x1000;
            const int CHF_ANTIGLIDE = 0x20000;
            const int DFLG_ON = 1; // currently enabled
            const int DFLG_NOREPEAT = 4; // character doesn't repeat it when clicked
            const int DTFLG_SHOWPARSER = 1; // show parser in this topic
            const int FONT_OUTLINE_AUTO = -10;
            const int MAX_SPRITES = 30000;
            const int MAX_CURSOR = 20;
            const int MAX_PARSER_WORD_LENGTH = 30;
            const int MAX_INV = 301;
            const int MAXLIPSYNCFRAMES = 20;
            const int MAXGLOBALMES = 500;
            const int MAXTOPICOPTIONS = 30;
            const int MAX_CUSTOM_PROPERTIES = 30;
            const short UNIFORM_WALK_SPEED = 0;
            const string AGS_VERSION = AGS.Types.Version.AGS_EDITOR_VERSION;
            FileStream ostream = File.Create(fileName);
            if (ostream == null)
            {
                throw new CompileError(string.Format("Cannot open file {0} for writing", fileName));
            }
            BinaryWriter writer = new BinaryWriter(ostream);
            WriteString(GAME_FILE_SIG, GAME_FILE_SIG.Length, writer);
            writer.Write(GAME_DATA_VERSION_CURRENT);
            writer.Write(AGS_VERSION.Length);
            WriteString(AGS_VERSION, AGS_VERSION.Length, writer);
            WriteGameSetupStructBase_Aligned(writer, game);
            WriteString(game.Settings.GUIDAsString, MAX_GUID_LENGTH, writer);
            WriteString(game.Settings.SaveGameFileExtension, MAX_SG_EXT_LENGTH, writer);
            WriteString(game.Settings.SaveGameFolderName, MAX_SG_FOLDER_LEN, writer);
            for (int i = 0; i < game.Fonts.Count; ++i)
            {
                writer.Write(game.Fonts[i].PointSize & FFLG_SIZEMASK);
            }
            for (int i = 0; i < game.Fonts.Count; ++i)
            {
                if (game.Fonts[i].OutlineStyle == FontOutlineStyle.None)
                {
                    writer.Write(-1);
                }
                else if (game.Fonts[i].OutlineStyle == FontOutlineStyle.Automatic)
                {
                    writer.Write(FONT_OUTLINE_AUTO);
                }
                else
                {
                    writer.Write(game.Fonts[i].OutlineFont);
                }
            }
            writer.Write(MAX_SPRITES);
            List<SpriteFlags> spriteFlags = GetAllSpriteFlags(game.RootSpriteFolder);
            if (spriteFlags.Count > MAX_SPRITES)
            {
                throw new CompileError("Too many sprites");
            }
            foreach (SpriteFlags flags in spriteFlags)
            {
                writer.Write((int)flags);
            }
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
                writer.Write(game.InventoryItems[i].PlayerStartsWithItem ? IFLG_STARTWITH : (char)0);
                writer.Write(new byte[3]); // 3 bytes padding
            }
            if (game.Cursors.Count > MAX_CURSOR)
            {
                throw new CompileError("Too many cursors");
            }
            for (int i = 0; i < game.Cursors.Count; ++i)
            {
                char flags = (char)0;
                writer.Write(game.Cursors[i].Image);
                writer.Write((short)game.Cursors[i].HotspotX);
                writer.Write(new byte[2]); // 2 bytes padding
                writer.Write((short)game.Cursors[i].HotspotY);
                writer.Write(new byte[2]); // 2 bytes padding
                if (game.Cursors[i].Animate)
                {
                    writer.Write((short)(game.Cursors[i].View - 1));
                    if (game.Cursors[i].AnimateOnlyOnHotspots) flags |= MCF_HOTSPOT;
                    if (game.Cursors[i].AnimateOnlyWhenMoving) flags |= MCF_ANIMMOVE;
                }
                else writer.Write((short)-1);
                writer.Write(new byte[2]); // 2 bytes padding
                WriteString(game.Cursors[i].Name, 9, writer);
                writer.Write(new byte[3]); // null terminator and 2 bytes padding
                if (game.Cursors[i].StandardMode) flags |= MCF_STANDARD;
                writer.Write(flags);
                writer.Write(new byte[3]); // 3 bytes padding
            }
            for (int i = 0; i < game.Characters.Count; ++i)
            {
                SerializeInteractionScripts(game.Characters[i].Interactions, writer);
            }
            for (int i = 0; i < game.InventoryItems.Count; ++i)
            {
                SerializeInteractionScripts(game.InventoryItems[i].Interactions, writer);
            }
            writer.Write(game.TextParser.Words.Count);
            for (int i = 0; i < game.TextParser.Words.Count; ++i)
            {
                WriteStringEncrypted(writer, SafeTruncate(game.TextParser.Words[i].Word, MAX_PARSER_WORD_LENGTH));
                writer.Write((short)game.TextParser.Words[i].WordGroup);
            }
            WriteCompiledScript(ostream, game.ScriptsToCompile.GetScriptByFilename(Script.GLOBAL_SCRIPT_FILE_NAME));
            WriteCompiledScript(ostream, game.ScriptsToCompile.GetScriptByFilename(Script.DIALOG_SCRIPTS_FILE_NAME));
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
                WriteCompiledScript(ostream, script);
            }
            ViewsWriter viewsWriter = new ViewsWriter(writer);
            viewsWriter.WriteViews(FolderHelper.GetRootViewFolder(game), game);
            foreach (Character character in game.Characters)
            {
                int flags = 0;
                if (character.AdjustSpeedWithScaling) flags |= CHF_SCALEMOVESPEED;
                if (character.AdjustVolumeWithScaling) flags |= CHF_SCALEVOLUME;
                if (!character.Clickable) flags |= CHF_NOINTERACT;
                if (!character.DiagonalLoops) flags |= CHF_NODIAGONAL;
                if (character.MovementLinkedToAnimation) flags |= CHF_ANTIGLIDE;
                if (!character.Solid) flags |= CHF_NOBLOCKING;
                if (!character.TurnBeforeWalking) flags |= CHF_NOTURNING;
                if (!character.UseRoomAreaLighting) flags |= CHF_NOLIGHTING;
                if (!character.UseRoomAreaScaling) flags |= CHF_MANUALSCALING;
                writer.Write(character.NormalView - 1);             // defview
                writer.Write(character.SpeechView - 1);             // talkview
                writer.Write(character.NormalView - 1);             // view
                writer.Write(character.StartingRoom);               // room
                writer.Write(-1);                               // prevroom
                writer.Write(character.StartX);                     // x
                writer.Write(character.StartY);                     // y
                writer.Write(0);                                // wait
                writer.Write(flags);                            // flags
                writer.Write((short)-1);                        // following
                writer.Write((short)(97 | (10 << 8)));          // followinfo
                writer.Write(character.IdleView - 1);               // idleview
                writer.Write((short)20);                        // idletime
                writer.Write((short)20);                        // idleleft
                writer.Write((short)0);                         // transparency
                writer.Write((short)-1);                        // baseline
                writer.Write(-1);                               // activeinv
                writer.Write(character.SpeechColor);                // talkcolor
                writer.Write(character.ThinkingView - 1);           // thinkview
                writer.Write((short)(character.BlinkingView - 1));  // blinkview
                writer.Write((short)140);                       // blinkinterval
                writer.Write((short)140);                       // blinktimer
                writer.Write((short)0);                         // blinkframe
                writer.Write(character.UniformMovementSpeed ? UNIFORM_WALK_SPEED : (short)character.MovementSpeedY); // walkspeed_y
                writer.Write((short)0); // pic_yoffs
                writer.Write(0); // z
                writer.Write(-1); // walkwait
                writer.Write((short)character.SpeechAnimationDelay); // speech_anim_speed
                writer.Write((short)0); // reserved1
                writer.Write((short)0); // blocking_width
                writer.Write((short)0); // blocking_height
                writer.Write(character.ID); // index_id
                writer.Write((short)0); // pic_xoffs
                writer.Write((short)0); // walkwaitcounter
                writer.Write(game.FindViewByID(character.NormalView).Loops[0].Frames.Count < 1 ? (short)1 : (short)0); // loop
                writer.Write((short)0); // frame
                writer.Write((short)0); // walking
                writer.Write((short)0); // animating
                writer.Write(character.UniformMovementSpeed ? (short)character.MovementSpeed : (short)character.MovementSpeedX); // walkspeed
                writer.Write((short)character.AnimationDelay); // animspeed
                bool isPlayer = (character == game.PlayerCharacter);
                foreach (InventoryItem invItem in game.InventoryItems) // inv[MAX_INV]
                {
                    if ((isPlayer) && (invItem.PlayerStartsWithItem)) writer.Write((short)1);
                    else writer.Write((short)0);
                }
                if (game.InventoryItems.Count < MAX_INV)
                {
                    writer.Write(new byte[(MAX_INV - game.InventoryItems.Count) * sizeof(short)]);
                }
                writer.Write((short)0); // actx
                writer.Write((short)0); // acty
                WriteString(character.RealName, 40, writer); // name
                WriteString(character.ScriptName, MAX_SCRIPT_NAME_LEN, writer); // scrname
                writer.Write((char)1); // on
                writer.Write(new byte[3]); // padding
            }
            for (int i = 0; i < MAXLIPSYNCFRAMES; ++i)
            {
                WriteString(game.LipSync.CharactersPerFrame[i], 50, writer);
            }
            for (int i = 0; i < MAXGLOBALMES; ++i)
            {
                if (string.IsNullOrEmpty(game.GlobalMessages[i])) continue;
                WriteStringEncrypted(writer, game.GlobalMessages[i]);
            }
            foreach (Dialog curDialog in game.Dialogs)
            {
                for (int i = 0; (i < MAXTOPICOPTIONS) && (i < curDialog.Options.Count); ++i)
                {
                    WriteString(curDialog.Options[i].Text, 150, writer); // optionnames
                }
                for (int i = curDialog.Options.Count + 1; i < MAXTOPICOPTIONS; ++i)
                {
                    WriteString("", 150, writer);
                }
                for (int i = 0; (i < MAXTOPICOPTIONS) && (i < curDialog.Options.Count); ++i)
                {
                    DialogOption option = curDialog.Options[i];
                    int flags = 0;
                    if (!option.Say) flags |= DFLG_NOREPEAT;
                    if (option.Show) flags |= DFLG_ON;
                    writer.Write(flags); // optionflags
                }
                for (int i = curDialog.Options.Count + 1; i < MAXTOPICOPTIONS; ++i)
                {
                    writer.Write(0);
                }
                writer.Write(new byte[4]); // optionscripts
                writer.Write(new byte[MAXTOPICOPTIONS * sizeof(short)]); // entrypoints
                writer.Write((short)0); // startupentrypoint
                writer.Write((short)0); // codesize
                writer.Write(curDialog.Options.Count); // numoptions
                writer.Write(curDialog.ShowTextParser ? 0 : DTFLG_SHOWPARSER); // topicflags
            }
            GUIsWriter guisWriter = new GUIsWriter(writer, game);
            guisWriter.WriteAllGUIs();
            WritePluginsToDisk(writer, game);
            if (game.PropertySchema.PropertyDefinitions.Count > MAX_CUSTOM_PROPERTIES)
            {
                throw new CompileError("Too many custom properties defined");
            }
            writer.Write(1); // properties schema version 1 at present
            writer.Write(game.PropertySchema.PropertyDefinitions.Count);
            foreach (CustomPropertySchemaItem schemaItem in game.PropertySchema.PropertyDefinitions)
            {
                FilePutNullTerminatedString(SafeTruncate(schemaItem.Name, 19), 20, writer);
                FilePutNullTerminatedString(SafeTruncate(schemaItem.Description, 99), 100, writer);
                FilePutNullTerminatedString(schemaItem.DefaultValue, schemaItem.DefaultValue.Length + 1, writer);
                writer.Write((int)schemaItem.Type);
            }
            for (int i = 0; i < game.Characters.Count; ++i)
            {
                CompiledCustomProperties item = new CompiledCustomProperties();
                CompileCustomProperties(game.Characters[i].Properties, item);
                item.Serialize(writer);
            }
            for (int i = 0; i < game.InventoryItems.Count; ++i)
            {
                CompiledCustomProperties item = new CompiledCustomProperties();
                CompileCustomProperties(game.InventoryItems[i].Properties, item);
                item.Serialize(writer);
            }
            for (int i = 0; i < game.ViewCount; ++i)
            {
                View view = game.FindViewByID(i);
                if (view != null) FilePutNullTerminatedString(view.Name, view.Name.Length + 1, writer);
            }
            for (int i = 0; i < game.InventoryItems.Count; ++i)
            {
                string buffer = game.InventoryItems[i].Name;
                FilePutNullTerminatedString(buffer, buffer.Length + 1, writer);
            }
            for (int i = 0; i < game.Dialogs.Count; ++i)
            {
                string buffer = game.Dialogs[i].Name;
                FilePutNullTerminatedString(buffer, buffer.Length + 1, writer);
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
                writer.Write(new byte[3]); // struct alignment padding
                writer.Write((short)clip.ActualPriority); // defaultPriority
                writer.Write((short)clip.ActualVolume); // defaultVolume
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
        }
    }
}
