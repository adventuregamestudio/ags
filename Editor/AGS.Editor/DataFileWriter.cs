using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace AGS.Editor
{
    class DataFileWriter
    {
        const int MAXMULTIFILES = 25;
        //const int MAX_DATAFILENAME_LENGTH = 50;

        class MultiFileLibNew
        {
            const int MAX_FILES = 10000;

            public class MultiFile
            {
                public string Filename;
                public int Offset;
                public int Length;
                public byte Datafile;

                public MultiFile(string fileName, byte dataFile, int length)
                {
                    Filename = fileName;
                    Datafile = dataFile;
                    Length = length;
                    Offset = 0;
                }
            };

            public List<string> DataFilenames;
            public List<MultiFile> Files;

            public MultiFileLibNew()
            {
                DataFilenames = new List<string>();
                Files = new List<MultiFile>();
            }
        };

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
                stream = File.Open(fileName, mode, access, FileShare.ReadWrite); // we're only reading, but don't disallow writing from other processes
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
                FileWriteDataEncrypted(BitConverter.GetBytes(ourlib.Files[i].Offset), writer);
            }
            for (int i = 0; i < ourlib.Files.Count; ++i)
            {
                FileWriteDataEncrypted(BitConverter.GetBytes(ourlib.Files[i].Length), writer);
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
                    throw new AGS.Types.AGSEditorException("Filename too long: " + fileNames[i]);
                }
                ourlib.Files.Add(new MultiFileLibNew.MultiFile(fileNameSrc, (byte)currentDataFile, (int)thisFileSize));
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
                    i == 0 ? FileMode.OpenOrCreate : FileMode.Create, FileAccess.Write))
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
                            ourlib.Files[j].Offset = (int)(writer.BaseStream.Position - startOffset);
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
                                    throw new AGS.Types.AGSEditorException("Unable to find file '" + ourlib.Files[j].Filename + "' for compilation. Do not remove files during the compilation process." + Environment.NewLine + Directory.GetCurrentDirectory());
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
                        writer.Write(startOffset);
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
    }
}
