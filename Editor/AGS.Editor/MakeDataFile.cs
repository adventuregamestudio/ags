using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace AGS.Editor
{
    class MakeDataFile
    {
        const string sprsetname = "acsprset.spr";
        const int MAX_FILES = 10000;
        const int MAXMULTIFILES = 25;
        const int MAX_FILENAME_LENGTH = 100;
        const int MAX_DATAFILENAME_LENGTH = 50;
        const int RAND_SEED_SALT = 9338638;
        const int CHUNKSIZE = 256000;
        const string clibendsig = "CLIB\x1\x2\x3\x4SIGE";

        struct MultiFileLibNew
        {
            public string[] data_filenames = new string[MAXMULTIFILES];
            public int num_data_files;
            public string[] filenames = new string[MAX_FILES];
            public long[] offset = new long[MAX_FILES];
            public long[] length = new long[MAX_FILES];
            public byte[] file_datafile = new byte[MAX_FILES];
            public int num_files;
        };

        static MultiFileLibNew ourlib;

        static Stream TryFileOpen(string fileName, FileAccess access)
        {
            return TryFileOpen(fileName, FileMode.Open, access);
        }

        static Stream TryFileOpen(string fileName, FileMode mode, FileAccess access)
        {
            Stream stream = null;
            try
            {
                stream = File.Open(fileName, mode, access);
            }
            catch
            {
            }
            return stream;
        }

        static Stream find_file_in_path(out string buffer, string fileName)
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
                return (((_lastRandValue = (int)(_lastRandValue * 214013L + 2531011L)) >> 16) & 0x7FFF);
            }
        }

        static void fwrite_data_enc(byte[] data, BinaryWriter writer)
        {
            for (int i = 0; i < data.Length; ++i)
            {
                writer.Write((int)data[i] + PseudoRandInt.Instance.GetNextRand());
            }
        }

        static void fputstring_enc(string text, BinaryWriter writer)
        {
            writer.Write(text.ToCharArray());
            writer.Write((byte)0);
        }

        static void putw_enc(int numberToWrite, BinaryWriter writer)
        {
            fwrite_data_enc(BitConverter.GetBytes(numberToWrite), writer);
        }

        static void write_clib_header(BinaryWriter writer)
        {
            int randSeed = (int)DateTime.Now.Ticks;
            writer.Write(randSeed - RAND_SEED_SALT);
            PseudoRandInt.InitializeInstance(randSeed);
            putw_enc(ourlib.num_data_files, writer);
            for (int i = 0; i < ourlib.num_data_files; ++i)
            {
                fputstring_enc(ourlib.data_filenames[i], writer);
            }
            putw_enc(ourlib.num_data_files, writer);
            for (int i = 0; i < ourlib.num_files; ++i)
            {
                fputstring_enc(ourlib.filenames[i], writer);
            }
            for (int i = 0; i < ourlib.num_files; ++i)
            {
                fwrite_data_enc(BitConverter.GetBytes((int)ourlib.offset[i]), writer);
            }
            for (int i = 0; i < ourlib.num_files; ++i)
            {
                fwrite_data_enc(BitConverter.GetBytes((int)ourlib.length[i]), writer);
            }
            for (int i = 0; i < ourlib.num_files; ++i)
            {
                fwrite_data_enc(new byte[] { ourlib.file_datafile[i] }, writer);
            }
        }

        static int copy_file_across(Stream instream, Stream copystream, long leftforthis)
        {
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

        static string DoMakeDataFile(int numFiles, string[] fileNames, long splitSize, string baseFileName, bool makeFileNameAssumptionsForEXE)
        {
            Environment.CurrentDirectory = AGSEditor.Instance.CurrentGame.DirectoryPath;
            ourlib.num_data_files = 0;
            ourlib.num_files = numFiles;
            int currentDataFile = 0;
            long sizeSoFar = 0;
            bool doSplitting = false;
            for (int i = 0; i < numFiles; ++i)
            {
                if (splitSize > 0)
                {
                    if (string.Compare(fileNames[i], sprsetname, true) == 0)
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
                ourlib.filenames[i] = fileNameSrc;
                ourlib.file_datafile[i] = (byte)currentDataFile;
                ourlib.length[i] = thisFileSize;
            }
            ourlib.num_data_files = currentDataFile + 1;
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
            for (int i = 0; i < ourlib.num_data_files; ++i)
            {
                if (makeFileNameAssumptionsForEXE)
                {
                    ourlib.data_filenames[i] = baseFileName + i.ToString("D3");
                    if (i == 0)
                    {
                        ourlib.data_filenames[i] = ourlib.data_filenames[i].Remove(ourlib.data_filenames[i].Length - 3) + "exe";
                    }
                }
                else
                {
                    ourlib.data_filenames[i] = Path.GetFileName(baseFileName);
                }
            }
            // adjust the file paths if necessary, so that write_clib_header will
            // write the correct amount of data
            string tomake;
            for (int i = 0; i < ourlib.num_files; i++)
            {
                using (Stream stream = find_file_in_path(out tomake, ourlib.filenames[i]))
                {
                    if (stream != null)
                    {
                        stream.Close();
                        if (!makeFileNameAssumptionsForEXE) ourlib.filenames[i] = tomake;
                    }
                }
            }
            // now, create the actual files
            for (int i = 0; i < ourlib.num_data_files; ++i)
            {
                if (makeFileNameAssumptionsForEXE)
                {
                    outputFileName = Path.Combine("Compiled", ourlib.data_filenames[i]);
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
                        write_clib_header(writer);
                    }
                    string buffer;
                    for (int j = 0; j < ourlib.num_files; ++j)
                    {
                        if (ourlib.file_datafile[j] == i)
                        {
                            using (Stream stream = find_file_in_path(out buffer, ourlib.filenames[j]))
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
                                    throw new AGS.Types.AGSEditorException("Unable to find file '" + ourlib.filenames[j] + "' for compilation. Do not remove files during the compilation process.");
                                }
                                if (copy_file_across(stream, writer.BaseStream, ourlib.length[j]) < 1)
                                {
                                    return "Error writing file: possibly disk full";
                                }
                            }
                        }
                    }
                    if (startOffset > 0)
                    {
                        writer.Write(startOffset);
                        writer.Write(clibendsig.ToCharArray());
                    }
                }
            }
            using (Stream wout = TryFileOpen(firstDataFileFullPath, FileMode.Open, FileAccess.ReadWrite))
            {
                wout.Seek(mainHeaderOffset, SeekOrigin.Begin);
                write_clib_header(new BinaryWriter(wout));
            }
            return null;
        }
    }
}
