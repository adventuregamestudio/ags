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
    public class DataFileWriter
    {
        private class NativeConstants
        {
            // NOTE: GetNativeConstant returns only int or string, so some additional casting may be required
            public static readonly string GAME_FILE_SIG = (string)Factory.NativeProxy.GetNativeConstant("GAME_FILE_SIG");
            public static readonly int GAME_DATA_VERSION_CURRENT = (int)Factory.NativeProxy.GetNativeConstant("GAME_DATA_VERSION_CURRENT");
            public static readonly int MAX_GUID_LENGTH = (int)Factory.NativeProxy.GetNativeConstant("MAX_GUID_LENGTH");
            public static readonly int MAX_SG_EXT_LENGTH = (int)Factory.NativeProxy.GetNativeConstant("MAX_SG_EXT_LENGTH");
            public static readonly int MAX_SG_FOLDER_LEN = (int)Factory.NativeProxy.GetNativeConstant("MAX_SG_FOLDER_LEN");
            public static readonly int MAX_SCRIPT_NAME_LEN = (int)Factory.NativeProxy.GetNativeConstant("MAX_SCRIPT_NAME_LEN");
            public static readonly int FFLG_SIZEMASK = (int)Factory.NativeProxy.GetNativeConstant("FFLG_SIZEMASK");
            public static readonly char IFLG_STARTWITH = (char)(int)Factory.NativeProxy.GetNativeConstant("IFLG_STARTWITH");
            public static readonly char MCF_ANIMMOVE = (char)(int)Factory.NativeProxy.GetNativeConstant("MCF_ANIMMOVE");
            public static readonly char MCF_STANDARD = (char)(int)Factory.NativeProxy.GetNativeConstant("MCF_STANDARD");
            public static readonly char MCF_HOTSPOT = (char)(int)Factory.NativeProxy.GetNativeConstant("MCF_HOTSPOT");
            public static readonly int CHF_MANUALSCALING = (int)Factory.NativeProxy.GetNativeConstant("CHF_MANUALSCALING");
            public static readonly int CHF_NOINTERACT = (int)Factory.NativeProxy.GetNativeConstant("CHF_NOINTERACT");
            public static readonly int CHF_NODIAGONAL = (int)Factory.NativeProxy.GetNativeConstant("CHF_NODIAGONAL");
            public static readonly int CHF_NOLIGHTING = (int)Factory.NativeProxy.GetNativeConstant("CHF_NOLIGHTING");
            public static readonly int CHF_NOTURNING = (int)Factory.NativeProxy.GetNativeConstant("CHF_NOTURNING");
            public static readonly int CHF_NOBLOCKING = (int)Factory.NativeProxy.GetNativeConstant("CHF_NOBLOCKING");
            public static readonly int CHF_SCALEMOVESPEED = (int)Factory.NativeProxy.GetNativeConstant("CHF_SCALEMOVESPEED");
            public static readonly int CHF_SCALEVOLUME = (int)Factory.NativeProxy.GetNativeConstant("CHF_SCALEVOLUME");
            public static readonly int CHF_ANTIGLIDE = (int)Factory.NativeProxy.GetNativeConstant("CHF_ANTIGLIDE");
            public static readonly int DFLG_ON = (int)Factory.NativeProxy.GetNativeConstant("DFLG_ON");
            public static readonly int DFLG_NOREPEAT = (int)Factory.NativeProxy.GetNativeConstant("DFLG_NOREPEAT");
            public static readonly int DTFLG_SHOWPARSER = (int)Factory.NativeProxy.GetNativeConstant("DTFLG_SHOWPARSER");
            public static readonly sbyte FONT_OUTLINE_AUTO = (sbyte)(int)Factory.NativeProxy.GetNativeConstant("FONT_OUTLINE_AUTO");
            public static readonly int MAX_FONTS = (int)Factory.NativeProxy.GetNativeConstant("MAX_FONTS");
            public static readonly int MAX_SPRITES = (int)Factory.NativeProxy.GetNativeConstant("MAX_SPRITES");
            public static readonly int MAX_CURSOR = (int)Factory.NativeProxy.GetNativeConstant("MAX_CURSOR");
            public static readonly int MAX_PARSER_WORD_LENGTH = (int)Factory.NativeProxy.GetNativeConstant("MAX_PARSER_WORD_LENGTH");
            public static readonly int MAX_INV = (int)Factory.NativeProxy.GetNativeConstant("MAX_INV");
            public static readonly int MAXLIPSYNCFRAMES = (int)Factory.NativeProxy.GetNativeConstant("MAXLIPSYNCFRAMES");
            public static readonly int MAXGLOBALMES = (int)Factory.NativeProxy.GetNativeConstant("MAXGLOBALMES");
            public static readonly int MAXTOPICOPTIONS = (int)Factory.NativeProxy.GetNativeConstant("MAXTOPICOPTIONS");
            public static readonly short UNIFORM_WALK_SPEED = (short)(int)Factory.NativeProxy.GetNativeConstant("UNIFORM_WALK_SPEED");
            public static readonly int GAME_RESOLUTION_CUSTOM = (int)Factory.NativeProxy.GetNativeConstant("GAME_RESOLUTION_CUSTOM");
            public static readonly int MAXMULTIFILES = (int)Factory.NativeProxy.GetNativeConstant("MAXMULTIFILES");
            public static readonly int RAND_SEED_SALT = (int)Factory.NativeProxy.GetNativeConstant("RAND_SEED_SALT");
            public static readonly int CHUNKSIZE = (int)Factory.NativeProxy.GetNativeConstant("CHUNKSIZE");
            public static readonly string CLIB_END_SIGNATURE = (string)Factory.NativeProxy.GetNativeConstant("CLIB_END_SIGNATURE");
            public static readonly int MAX_FILENAME_LENGTH = (int)Factory.NativeProxy.GetNativeConstant("MAX_FILENAME_LENGTH");
            public static readonly string SPRSET_NAME = (string)Factory.NativeProxy.GetNativeConstant("SPRSET_NAME");
            public static readonly byte SPF_640x400 = (byte)(int)Factory.NativeProxy.GetNativeConstant("SPF_640x400");
            public static readonly byte SPF_ALPHACHANNEL = (byte)(int)Factory.NativeProxy.GetNativeConstant("SPF_ALPHACHANNEL");
            public static readonly int MAX_CUSTOM_PROPERTIES = (int)Factory.NativeProxy.GetNativeConstant("MAX_CUSTOM_PROPERTIES");
            public static readonly int MAX_CUSTOM_PROPERTY_NAME_LENGTH = (int)Factory.NativeProxy.GetNativeConstant("MAX_CUSTOM_PROPERTY_NAME_LENGTH");
            public static readonly int MAX_CUSTOM_PROPERTY_VALUE_LENGTH = (int)Factory.NativeProxy.GetNativeConstant("MAX_CUSTOM_PROPERTY_VALUE_LENGTH");
            public static readonly string PASSWORD_ENC_STRING = (string)Factory.NativeProxy.GetNativeConstant("PASSWORD_ENC_STRING");
            public static readonly int LOOPFLAG_RUNNEXTLOOP = (int)Factory.NativeProxy.GetNativeConstant("LOOPFLAG_RUNNEXTLOOP");
            public static readonly int VFLG_FLIPSPRITE = (int)Factory.NativeProxy.GetNativeConstant("VFLG_FLIPSPRITE");
            public static readonly uint GUIMAGIC = (uint)Factory.NativeProxy.GetNativeConstant("GUIMAGIC");
            public static readonly int SAVEBUFFERSIZE = (int)Factory.NativeProxy.GetNativeConstant("SAVEBUFFERSIZE");
            public static readonly int GUIMAIN_NOCLICK = (int)Factory.NativeProxy.GetNativeConstant("GUIMAIN_NOCLICK");
            public static readonly int GUIF_CLIP = (int)Factory.NativeProxy.GetNativeConstant("GUIF_CLIP");
            public static readonly int GUIF_TRANSLATED = (int)Factory.NativeProxy.GetNativeConstant("GUIF_TRANSLATED");
            public static readonly int GLF_NOBORDER = (int)Factory.NativeProxy.GetNativeConstant("GLF_NOBORDER");
            public static readonly int GLF_NOARROWS = (int)Factory.NativeProxy.GetNativeConstant("GLF_NOARROWS");
            public static readonly int GUI_POPUP_MODAL = (int)Factory.NativeProxy.GetNativeConstant("GUI_POPUP_MODAL");
            public static readonly byte GUIMAIN_TEXTWINDOW = (byte)(int)Factory.NativeProxy.GetNativeConstant("GUIMAIN_TEXTWINDOW");
            public static readonly int GTF_NOBORDER = (int)Factory.NativeProxy.GetNativeConstant("GTF_NOBORDER");
            public static readonly int MAX_GUILABEL_TEXT_LEN = (int)Factory.NativeProxy.GetNativeConstant("MAX_GUILABEL_TEXT_LEN");
            public static readonly int MAX_GUIOBJ_SCRIPTNAME_LEN = (int)Factory.NativeProxy.GetNativeConstant("MAX_GUIOBJ_SCRIPTNAME_LEN");
            public static readonly int MAX_GUIOBJ_EVENTHANDLER_LEN = (int)Factory.NativeProxy.GetNativeConstant("MAX_GUIOBJ_EVENTHANDLER_LEN");
            public static readonly int GOBJ_BUTTON = (int)Factory.NativeProxy.GetNativeConstant("GOBJ_BUTTON");
            public static readonly int GOBJ_LABEL = (int)Factory.NativeProxy.GetNativeConstant("GOBJ_LABEL");
            public static readonly int GOBJ_INVENTORY = (int)Factory.NativeProxy.GetNativeConstant("GOBJ_INVENTORY");
            public static readonly int GOBJ_SLIDER = (int)Factory.NativeProxy.GetNativeConstant("GOBJ_SLIDER");
            public static readonly int GOBJ_TEXTBOX = (int)Factory.NativeProxy.GetNativeConstant("GOBJ_TEXTBOX");
            public static readonly int GOBJ_LISTBOX = (int)Factory.NativeProxy.GetNativeConstant("GOBJ_LISTBOX");
            public static readonly int TEXTWINDOW_PADDING_DEFAULT = (int)Factory.NativeProxy.GetNativeConstant("TEXTWINDOW_PADDING_DEFAULT");

            public class GUIVersion
            {
                public static readonly int Current = (int)Factory.NativeProxy.GetNativeConstant("GUI_VERSION_CURRENT");
            }

            public class GameOptions
            {
                public static readonly int OPT_DEBUGMODE = (int)Factory.NativeProxy.GetNativeConstant("OPT_DEBUGMODE");
                public static readonly int OPT_WALKONLOOK = (int)Factory.NativeProxy.GetNativeConstant("OPT_WALKONLOOK");
                public static readonly int OPT_DIALOGIFACE = (int)Factory.NativeProxy.GetNativeConstant("OPT_DIALOGIFACE");
                public static readonly int OPT_ANTIGLIDE = (int)Factory.NativeProxy.GetNativeConstant("OPT_ANTIGLIDE");
                public static readonly int OPT_TWCUSTOM = (int)Factory.NativeProxy.GetNativeConstant("OPT_TWCUSTOM");
                public static readonly int OPT_DIALOGGAP = (int)Factory.NativeProxy.GetNativeConstant("OPT_DIALOGGAP");
                public static readonly int OPT_NOSKIPTEXT = (int)Factory.NativeProxy.GetNativeConstant("OPT_NOSKIPTEXT");
                public static readonly int OPT_DISABLEOFF = (int)Factory.NativeProxy.GetNativeConstant("OPT_DISABLEOFF");
                public static readonly int OPT_ALWAYSSPCH = (int)Factory.NativeProxy.GetNativeConstant("OPT_ALWAYSSPCH");
                public static readonly int OPT_SPEECHTYPE = (int)Factory.NativeProxy.GetNativeConstant("OPT_SPEECHTYPE");
                public static readonly int OPT_PIXPERFECT = (int)Factory.NativeProxy.GetNativeConstant("OPT_PIXPERFECT");
                public static readonly int OPT_NOWALKMODE = (int)Factory.NativeProxy.GetNativeConstant("OPT_NOWALKMODE");
                public static readonly int OPT_LETTERBOX = (int)Factory.NativeProxy.GetNativeConstant("OPT_LETTERBOX");
                public static readonly int OPT_FIXEDINVCURSOR = (int)Factory.NativeProxy.GetNativeConstant("OPT_FIXEDINVCURSOR");
                public static readonly int OPT_NOSCALEFNT = (int)Factory.NativeProxy.GetNativeConstant("OPT_NOSCALEFNT");
                public static readonly int OPT_SPLITRESOURCES = (int)Factory.NativeProxy.GetNativeConstant("OPT_SPLITRESOURCES");
                public static readonly int OPT_ROTATECHARS = (int)Factory.NativeProxy.GetNativeConstant("OPT_ROTATECHARS");
                public static readonly int OPT_FADETYPE = (int)Factory.NativeProxy.GetNativeConstant("OPT_FADETYPE");
                public static readonly int OPT_HANDLEINVCLICKS = (int)Factory.NativeProxy.GetNativeConstant("OPT_HANDLEINVCLICKS");
                public static readonly int OPT_MOUSEWHEEL = (int)Factory.NativeProxy.GetNativeConstant("OPT_MOUSEWHEEL");
                public static readonly int OPT_DIALOGNUMBERED = (int)Factory.NativeProxy.GetNativeConstant("OPT_DIALOGNUMBERED");
                public static readonly int OPT_DIALOGUPWARDS = (int)Factory.NativeProxy.GetNativeConstant("OPT_DIALOGUPWARDS");
                public static readonly int OPT_ANTIALIASFONTS = (int)Factory.NativeProxy.GetNativeConstant("OPT_ANTIALIASFONTS");
                public static readonly int OPT_THOUGHTGUI = (int)Factory.NativeProxy.GetNativeConstant("OPT_THOUGHTGUI");
                public static readonly int OPT_TURNTOFACELOC = (int)Factory.NativeProxy.GetNativeConstant("OPT_TURNTOFACELOC");
                public static readonly int OPT_RIGHTLEFTWRITE = (int)Factory.NativeProxy.GetNativeConstant("OPT_RIGHTLEFTWRITE");
                public static readonly int OPT_DUPLICATEINV = (int)Factory.NativeProxy.GetNativeConstant("OPT_DUPLICATEINV");
                public static readonly int OPT_SAVESCREENSHOT = (int)Factory.NativeProxy.GetNativeConstant("OPT_SAVESCREENSHOT");
                public static readonly int OPT_PORTRAITSIDE = (int)Factory.NativeProxy.GetNativeConstant("OPT_PORTRAITSIDE");
                public static readonly int OPT_STRICTSCRIPTING = (int)Factory.NativeProxy.GetNativeConstant("OPT_STRICTSCRIPTING");
                public static readonly int OPT_LEFTTORIGHTEVAL = (int)Factory.NativeProxy.GetNativeConstant("OPT_LEFTTORIGHTEVAL");
                public static readonly int OPT_COMPRESSSPRITES = (int)Factory.NativeProxy.GetNativeConstant("OPT_COMPRESSSPRITES");
                public static readonly int OPT_STRICTSTRINGS = (int)Factory.NativeProxy.GetNativeConstant("OPT_STRICTSTRINGS");
                public static readonly int OPT_NEWGUIALPHA = (int)Factory.NativeProxy.GetNativeConstant("OPT_NEWGUIALPHA");
                public static readonly int OPT_RUNGAMEDLGOPTS = (int)Factory.NativeProxy.GetNativeConstant("OPT_RUNGAMEDLGOPTS");
                public static readonly int OPT_NATIVECOORDINATES = (int)Factory.NativeProxy.GetNativeConstant("OPT_NATIVECOORDINATES");
                public static readonly int OPT_GLOBALTALKANIMSPD = (int)Factory.NativeProxy.GetNativeConstant("OPT_GLOBALTALKANIMSPD");
                public static readonly int OPT_SPRITEALPHA = (int)Factory.NativeProxy.GetNativeConstant("OPT_SPRITEALPHA");
            }
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
            int randSeed = (int)DateTime.Now.Ticks;
            writer.Write(randSeed - NativeConstants.RAND_SEED_SALT);
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

        public static string MakeDataFile(string[] fileNames, int splitSize, string baseFileName, bool makeFileNameAssumptionsForEXE)
        {
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
                    if (string.Compare(fileNames[i], NativeConstants.SPRSET_NAME, true) == 0)
                    {
                        // the sprite file's appearance signifies it's time to start splitting
                        doSplitting = true;
                        currentDataFile++;
                        sizeSoFar = 0;
                    }
                    else if ((sizeSoFar > splitSize) && (doSplitting) && (currentDataFile < (NativeConstants.MAXMULTIFILES - 1)))
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
                if (fileNameSrc.Length >= NativeConstants.MAX_FILENAME_LENGTH)
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
                        writer.Write(NativeConstants.CLIB_END_SIGNATURE.ToCharArray());
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
            options[NativeConstants.GameOptions.OPT_NOSCALEFNT] = (game.Settings.FontsForHiRes ? 1 : 0);
            options[NativeConstants.GameOptions.OPT_NEWGUIALPHA] = (int)game.Settings.GUIAlphaStyle;
            options[NativeConstants.GameOptions.OPT_SPRITEALPHA] = (int)game.Settings.SpriteAlphaStyle;
            options[NativeConstants.GameOptions.OPT_HANDLEINVCLICKS] = (game.Settings.HandleInvClicksInScript ? 1 : 0);
            options[NativeConstants.GameOptions.OPT_FIXEDINVCURSOR] = (game.Settings.InventoryCursors ? 0 : 1);
            options[NativeConstants.GameOptions.OPT_GLOBALTALKANIMSPD] = (game.Settings.UseGlobalSpeechAnimationDelay ?
                game.Settings.GlobalSpeechAnimationDelay : (-game.Settings.GlobalSpeechAnimationDelay - 1));
            options[NativeConstants.GameOptions.OPT_LEFTTORIGHTEVAL] = (game.Settings.LeftToRightPrecedence ? 1 : 0);
            options[NativeConstants.GameOptions.OPT_LETTERBOX] = (game.Settings.LetterboxMode ? 1 : 0);
            options[NativeConstants.GameOptions.OPT_MOUSEWHEEL] = (game.Settings.MouseWheelEnabled ? 1 : 0);
            options[NativeConstants.GameOptions.OPT_DIALOGNUMBERED] = (game.Settings.NumberDialogOptions ? 1 : 0);
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
            if ((game.Settings.LegacyLetterboxAble) && (game.Settings.LetterboxMode))
            {
#pragma warning disable 612, 618 // we know this is the old resolution member, don't warn about this usage
                writer.Write((int)game.Settings.Resolution);
#pragma warning restore 612, 618
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

        private static void UpdateSpriteFlags(SpriteFolder folder, byte[] flags)
        {
            foreach (Sprite sprite in folder.Sprites)
            {
                flags[sprite.Number] = 0;
                if (sprite.Resolution == SpriteImportResolution.HighRes) flags[sprite.Number] |= NativeConstants.SPF_640x400;
                if (sprite.AlphaChannel) flags[sprite.Number] |= NativeConstants.SPF_ALPHACHANNEL;
            }
            foreach (SpriteFolder subfolder in folder.SubFolders)
            {
                UpdateSpriteFlags(subfolder, flags);
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
                if (PropertyCount >= NativeConstants.MAX_CUSTOM_PROPERTIES) return;
                _names.Add(SafeTruncate(name, NativeConstants.MAX_CUSTOM_PROPERTY_NAME_LENGTH - 1));
                _values.Add(SafeTruncate(value, NativeConstants.MAX_CUSTOM_PROPERTY_VALUE_LENGTH - 1));
            }

            public void Serialize(BinaryWriter writer)
            {
                writer.Write(1);
                writer.Write(PropertyCount);
                for (int i = 0; i < PropertyCount; ++i)
                {
                    FilePutNullTerminatedString(Names[i], NativeConstants.MAX_CUSTOM_PROPERTY_NAME_LENGTH, writer);
                    FilePutNullTerminatedString(Values[i], NativeConstants.MAX_CUSTOM_PROPERTY_VALUE_LENGTH, writer);
                }
            }

            int UnSerialize(BinaryReader reader)
            {
                if (reader.ReadInt32() != 1) return -1;
                int count = reader.ReadInt32();
                for (int i = 0; i < count; ++i)
                {
                    string name = ReadString(NativeConstants.MAX_CUSTOM_PROPERTY_NAME_LENGTH, reader);
                    string value = ReadString(NativeConstants.MAX_CUSTOM_PROPERTY_VALUE_LENGTH, reader);
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
            StringBuilder sb = new StringBuilder(toEncrypt);
            int p = 0;
            for (int i = 0; i < toEncrypt.Length; ++i, ++p)
            {
                if (p == NativeConstants.PASSWORD_ENC_STRING.Length) p = 0;
                sb[i] += NativeConstants.PASSWORD_ENC_STRING[p];
            }
            sb.Append(NativeConstants.PASSWORD_ENC_STRING[p]);
            return sb.ToString();
        }

        static void WriteStringEncrypted(BinaryWriter writer, string text)
        {
            int stlent = text.Length + 1;
            writer.Write(stlent);
            text = EncryptText(text);
            for (int i = 0; i < text.Length; ++i) // NOTE: Using ASCII encoding here actually breaks the encryption and gives wrong result
            {
                writer.Write((byte)text[i]);
            }
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
                string buffer = SafeTruncate(control.Name, NativeConstants.MAX_GUIOBJ_SCRIPTNAME_LEN);
                FilePutNullTerminatedString(buffer, buffer.Length + 1, writer);
                writer.Write(events.Length); // numSupportedEvents
                foreach (string sevent in events)
                {
                    buffer = SafeTruncate(sevent, NativeConstants.MAX_GUIOBJ_EVENTHANDLER_LEN);
                    FilePutNullTerminatedString(buffer, buffer.Length + 1, writer);
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
                    flags = (ctrl.ClipImage ? NativeConstants.GUIF_CLIP : 0);
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
                    string text = SafeTruncate(label.Text, NativeConstants.MAX_GUILABEL_TEXT_LEN);
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
                    writer.Write(textBox.ShowBorder ? 0 : NativeConstants.GTF_NOBORDER);
                }
            }

            private void WriteAllListBoxes()
            {
                writer.Write(GUIListBoxes.Count);
                foreach (GUIListBox listBox in GUIListBoxes)
                {
                    int flags = (listBox.Translated ? NativeConstants.GUIF_TRANSLATED : 0);
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
                    int exflags = (listBox.ShowBorder ? 0 : NativeConstants.GLF_NOBORDER);
                    exflags |= (listBox.ShowScrollArrows ? 0 : NativeConstants.GLF_NOARROWS);
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
                writer.Write(new byte[4]); // vtext
                WriteString(gui.Name, gui.Name.Length + 1, writer); // name
                WriteString(gui.OnClick, gui.OnClick.Length + 1, writer); // clickEventHandler
                //WriteString(SafeTruncate(gui.Name, 15), 16, writer); // name
                //WriteString(SafeTruncate(gui.OnClick, 19), 20, writer); // clickEventHandler
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
                writer.Write(gui.Clickable ? 0 : NativeConstants.GUIMAIN_NOCLICK); // flags
                int transparency = gui.Transparency;
                if (transparency <= 0) transparency = 0;
                else if (transparency >= 100) transparency = 255;
                else transparency = ((100 - transparency) * 25) / 10;
                writer.Write(transparency); // transparency
                writer.Write(gui.ZOrder); // zorder
                writer.Write(0); // guiId, TODO: should this be gui.ID?
                writer.Write(NativeConstants.TEXTWINDOW_PADDING_DEFAULT); // padding
                writer.Write(new byte[5 * sizeof(int)]); // reserved
                writer.Write(1); // on
            }

            private void WriteTextWindowGUI(TextWindowGUI gui)
            {
                writer.Write(NativeConstants.GUIMAIN_TEXTWINDOW); // vtext...
                writer.Write(new byte[3]); // ...vtext
                WriteString(gui.Name, gui.Name.Length + 1, writer); // name
                writer.Write((byte)0); // clickEventHandler
                //WriteString(SafeTruncate(gui.Name, 15), 16, writer); // name
                //writer.Write(new byte[20]); // clickEventHandler
                writer.Write(0); // x
                writer.Write(0); // y
                writer.Write(200); // wid
                writer.Write(100); // hit
                writer.Write(0); // focus
                writer.Write(gui.Controls.Count); // numobjs
                writer.Write(NativeConstants.GUI_POPUP_MODAL); // popup
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

        private static void WritePluginsToDisk(BinaryWriter writer, Game game)
        {
            writer.Write(1); // version
            writer.Write(game.Plugins.Count);
            foreach (Plugin plugin in game.Plugins)
            {
                FilePutNullTerminatedString(plugin.FileName, plugin.FileName.Length + 1, writer);
                int savesize = plugin.SerializedData.Length;
                if (savesize > NativeConstants.SAVEBUFFERSIZE)
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
            FileStream ostream = File.Create(fileName);
            if (ostream == null)
            {
                throw new CompileError(string.Format("Cannot open file {0} for writing", fileName));
            }
            BinaryWriter writer = new BinaryWriter(ostream);
            WriteString(NativeConstants.GAME_FILE_SIG, NativeConstants.GAME_FILE_SIG.Length, writer);
            writer.Write(NativeConstants.GAME_DATA_VERSION_CURRENT);
            writer.Write(AGS.Types.Version.AGS_EDITOR_VERSION.Length);
            WriteString(AGS.Types.Version.AGS_EDITOR_VERSION, AGS.Types.Version.AGS_EDITOR_VERSION.Length, writer);
            WriteGameSetupStructBase_Aligned(writer, game);
            WriteString(game.Settings.GUIDAsString, NativeConstants.MAX_GUID_LENGTH, writer);
            WriteString(game.Settings.SaveGameFileExtension, NativeConstants.MAX_SG_EXT_LENGTH, writer);
            WriteString(game.Settings.SaveGameFolderName, NativeConstants.MAX_SG_FOLDER_LEN, writer);
            if (game.Fonts.Count > NativeConstants.MAX_FONTS)
            {
                throw new CompileError("Too many fonts");
            }
            for (int i = 0; i < game.Fonts.Count; ++i)
            {
                writer.Write((byte)(game.Fonts[i].PointSize & NativeConstants.FFLG_SIZEMASK));
            }
            for (int i = 0; i < game.Fonts.Count; ++i)
            {
                if (game.Fonts[i].OutlineStyle == FontOutlineStyle.None)
                {
                    writer.Write((sbyte)-1);
                }
                else if (game.Fonts[i].OutlineStyle == FontOutlineStyle.Automatic)
                {
                    writer.Write(NativeConstants.FONT_OUTLINE_AUTO);
                }
                else
                {
                    writer.Write((byte)game.Fonts[i].OutlineFont);
                }
            }
            writer.Write(NativeConstants.MAX_SPRITES);
            byte[] spriteFlags = new byte[NativeConstants.MAX_SPRITES];
            UpdateSpriteFlags(game.RootSpriteFolder, spriteFlags);
            for (int i = 0; i < NativeConstants.MAX_SPRITES; ++i)
            {
                writer.Write(spriteFlags[i]);
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
                throw new CompileError("Too many cursors");
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
                if (character.AdjustSpeedWithScaling) flags |= NativeConstants.CHF_SCALEMOVESPEED;
                if (character.AdjustVolumeWithScaling) flags |= NativeConstants.CHF_SCALEVOLUME;
                if (!character.Clickable) flags |= NativeConstants.CHF_NOINTERACT;
                if (!character.DiagonalLoops) flags |= NativeConstants.CHF_NODIAGONAL;
                if (character.MovementLinkedToAnimation) flags |= NativeConstants.CHF_ANTIGLIDE;
                if (!character.Solid) flags |= NativeConstants.CHF_NOBLOCKING;
                if (!character.TurnBeforeWalking) flags |= NativeConstants.CHF_NOTURNING;
                if (!character.UseRoomAreaLighting) flags |= NativeConstants.CHF_NOLIGHTING;
                if (!character.UseRoomAreaScaling) flags |= NativeConstants.CHF_MANUALSCALING;
                writer.Write(character.NormalView - 1);             // defview
                writer.Write(character.SpeechView - 1);             // talkview
                writer.Write(character.NormalView - 1);             // view
                writer.Write(character.StartingRoom);               // room
                writer.Write(0);                               // prevroom
                writer.Write(character.StartX);                     // x
                writer.Write(character.StartY);                     // y
                writer.Write(0);                                // wait
                writer.Write(flags);                            // flags
                writer.Write((short)0);                        // following
                writer.Write((short)0);          // followinfo
                writer.Write(character.IdleView - 1);               // idleview
                writer.Write((short)0);                        // idletime
                writer.Write((short)0);                        // idleleft
                writer.Write((short)0);                         // transparency
                writer.Write((short)0);                        // baseline
                writer.Write(0);                               // activeinv
                writer.Write(character.SpeechColor);                // talkcolor
                writer.Write(character.ThinkingView - 1);           // thinkview
                writer.Write((short)(character.BlinkingView - 1));  // blinkview
                writer.Write((short)0);                       // blinkinterval
                writer.Write((short)0);                       // blinktimer
                writer.Write((short)0);                         // blinkframe
                writer.Write(character.UniformMovementSpeed ? NativeConstants.UNIFORM_WALK_SPEED : (short)character.MovementSpeedY); // walkspeed_y
                writer.Write((short)0); // pic_yoffs
                writer.Write(0); // z
                writer.Write(0); // walkwait
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
                if (game.InventoryItems.Count < NativeConstants.MAX_INV)
                {
                    writer.Write(new byte[(NativeConstants.MAX_INV - game.InventoryItems.Count) * sizeof(short)]);
                }
                writer.Write((short)0); // actx
                writer.Write((short)0); // acty
                WriteString(character.RealName, 40, writer); // name
                WriteString(character.ScriptName, NativeConstants.MAX_SCRIPT_NAME_LEN, writer); // scrname
                writer.Write((char)1); // on
                writer.Write((byte)0); // alignment padding
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
            WritePluginsToDisk(writer, game);
            if (game.PropertySchema.PropertyDefinitions.Count > NativeConstants.MAX_CUSTOM_PROPERTIES)
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
            new CompiledCustomProperties().Serialize(writer); // data file is writing 1 extra slot for inventory properties
            for (int i = 0; i <= game.ViewCount; ++i) // ViewCount is highest numbered view
            {
                View view = game.FindViewByID(i);
                if (view != null) FilePutNullTerminatedString(view.Name, view.Name.Length + 1, writer);
            }
            writer.Write((byte)0); // inventory slot 0 is unused, so its name is just a single NUL byte
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
