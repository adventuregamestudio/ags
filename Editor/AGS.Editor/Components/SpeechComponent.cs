using System;
using System.Collections.Generic;
using System.Drawing;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows.Forms;
using System.Xml;
using AGS.Types;

namespace AGS.Editor.Components
{
    class SpeechComponent : BaseComponent
    {
        public const string SPEECH_DIRECTORY = "Speech";
        private const string PAMELA_FILE_FILTER = "*.pam";
        private const string PAPAGAYO_FILE_FILTER = "*.dat";
        private const string OGG_VORBIS_FILE_FILTER = "*.ogg";
        private const string MP3_FILE_FILTER = "*.mp3";
        private const string WAVEFORM_FILE_FILTER = "*.wav";
        private const string LIP_SYNC_DATA_OUTPUT = "syncdata.dat";
        private const string SPEECH_VOX_FILE_NAME = "speech.vox";

        // Source file timestamp records: keep timestamps of the source files used in speech.vox compilation;
        // used to test whether recompilation is necessary or may be skipped.
        private Dictionary<string, Dictionary<string, DateTime>> _speechVoxStatus = new Dictionary<string, Dictionary<string, DateTime>>();
        private Dictionary<string, Dictionary<string, DateTime>> _pamFileStatus = new Dictionary<string, Dictionary<string, DateTime>>();

        public SpeechComponent(GUIController guiController, AGSEditor agsEditor)
            : base(guiController, agsEditor)
        {
            _agsEditor.ExtraCompilationStep += new AGSEditor.ExtraCompilationStepHandler(_agsEditor_ExtraCompilationStep);
            _agsEditor.ExtraOutputCreationStep += new AGSEditor.ExtraOutputCreationStepHandler(_agsEditor_ExtraOutputCreationStep);
            _agsEditor.GetSourceControlFileList += new GetSourceControlFileListHandler(_agsEditor_GetSourceControlFileList);
        }

        private void _agsEditor_GetSourceControlFileList(IList<string> fileNames)
        {
            if (_agsEditor.CurrentGame.Settings.BinaryFilesInSourceControl)
            {
                Utilities.AddAllMatchingFiles(fileNames, OGG_VORBIS_FILE_FILTER, true);
                Utilities.AddAllMatchingFiles(fileNames, MP3_FILE_FILTER, true);
                Utilities.AddAllMatchingFiles(fileNames, WAVEFORM_FILE_FILTER, true);
                Utilities.AddAllMatchingFiles(fileNames, PAMELA_FILE_FILTER, true);
                Utilities.AddAllMatchingFiles(fileNames, PAPAGAYO_FILE_FILTER, true);
            }
        }

        private void _agsEditor_ExtraCompilationStep(CompileMessages errors)
        {
            MakeOneLipSyncDat(SPEECH_DIRECTORY, LIP_SYNC_DATA_OUTPUT, errors);
            // Also try compile corresponding lipsync dat for each top-level subfolder
            // inside the main Speech folder.
            foreach (string dir in Directory.GetDirectories(SPEECH_DIRECTORY))
            {
                MakeOneLipSyncDat(dir, LIP_SYNC_DATA_OUTPUT, errors);
            }
        }

        private void _agsEditor_ExtraOutputCreationStep(bool miniExeForDebug)
        {
            if (miniExeForDebug)
                return;
            MakeOneVOX(SPEECH_DIRECTORY, SPEECH_VOX_FILE_NAME);
            // For each top-level subfolder inside the main Speech folder,
            // make a VOX called "sp_[name].vox", where [name] is the lowercase subfolder name.
            var subdirs = Directory.GetDirectories(SPEECH_DIRECTORY);
            foreach (string dir in subdirs)
            {
                string outFileName = string.Format("sp_{0}.vox", dir.Substring(SPEECH_DIRECTORY.Length + 1).ToLower());
                MakeOneVOX(dir, outFileName);
            }
        }

        public override string ComponentID
        {
            get { return ComponentIDs.Speech; }
        }

        /// <summary>
        /// Compiles one lipsync dat file from the collection of Pamela files (*.pam).
        /// </summary>
        private void MakeOneLipSyncDat(string sourceDir, string outputName, CompileMessages errors)
        {
            string[] pamFileList = ConstructFileListForSyncData(sourceDir);
            string key = sourceDir.ToLower();
            if (!_pamFileStatus.ContainsKey(key))
                _pamFileStatus.Add(key, new Dictionary<string, DateTime>());
            Dictionary<string, DateTime> fileTimes = _pamFileStatus[key];

            string datFileName = Path.Combine(sourceDir, outputName);
            if (DoesTargetFileNeedRebuild(datFileName, pamFileList, fileTimes))
            {
                CompileLipSyncFiles(sourceDir, datFileName, errors);
                UpdateVOXFileStatusWithCurrentFileTimes(pamFileList, fileTimes);
            }
        }

        /// <summary>
        /// Compiles one VOX file from the collection of voice-over clips and lipsync dat files.
        /// </summary>
        private void MakeOneVOX(string sourceDir, string outputName)
        {
            string[] speechFileList = ConstructFileListForSpeechVOX(sourceDir);
            string key = sourceDir.ToLower();
            if (!_speechVoxStatus.ContainsKey(key))
                _speechVoxStatus.Add(key, new Dictionary<string, DateTime>());
            Dictionary<string, DateTime> fileTimes = _speechVoxStatus[key];

            string voxFileName = Path.Combine(AGSEditor.OUTPUT_DIRECTORY, Path.Combine(AGSEditor.DATA_OUTPUT_DIRECTORY, outputName));
            if (DoesTargetFileNeedRebuild(voxFileName, speechFileList, fileTimes))
            {
                RebuildVOXFile(voxFileName, speechFileList);
                UpdateVOXFileStatusWithCurrentFileTimes(speechFileList, fileTimes);
            }
        }

        private int FindFrameNumberForPhoneme(string phonemeCode)
        {
            int frameID = -1;
            for (int i = 0; i < (_agsEditor.CurrentGame.LipSync.CharactersPerFrame.Length) && (frameID < 0); i++)
            {
                string[] codesForThisFrame = _agsEditor.CurrentGame.LipSync.CharactersPerFrame[i].Split('/');
                foreach (string code in codesForThisFrame)
                {
                    if (code.ToUpper() == phonemeCode)
                    {
                        frameID = i;
                        break;
                    }
                }
            }
            return frameID;
        }

        private void AlignPhonemeOffsets(SpeechLipSyncLine syncDataForThisFile)
        {
            syncDataForThisFile.Phonemes.Sort();

            // The PAM/DAT files contain start times: Convert to end times
            for (int i = 0; i < syncDataForThisFile.Phonemes.Count - 1; i++)
            {
                syncDataForThisFile.Phonemes[i].EndTimeOffset = syncDataForThisFile.Phonemes[i + 1].EndTimeOffset;
            }

            if (syncDataForThisFile.Phonemes.Count > 1)
            {
                syncDataForThisFile.Phonemes[syncDataForThisFile.Phonemes.Count - 1].EndTimeOffset = syncDataForThisFile.Phonemes[syncDataForThisFile.Phonemes.Count - 2].EndTimeOffset + 1000;
            }
        }

        private SpeechLipSyncLine CompilePamelaFile(string fileName, CompileMessages errors)
        {
            SpeechLipSyncLine syncDataForThisFile = new SpeechLipSyncLine();
            syncDataForThisFile.FileName = Path.GetFileNameWithoutExtension(fileName);

            string thisLine;
            bool inMainSection = false;
            int lineNumber = 0;

            StreamReader sr = new StreamReader(fileName);
            while ((thisLine = sr.ReadLine()) != null)
            {
                lineNumber++;
                if (thisLine.ToLower().StartsWith("[speech]"))
                {
                    inMainSection = true;
                    continue;
                }
                if (inMainSection)
                {
                    if (thisLine.TrimStart().StartsWith("["))
                    {
                        // moved onto another section
                        break;
                    }
                    if (thisLine.IndexOf(':') > 0)
                    {
                        string[] parts = thisLine.Split(':');
                        int part0;
                        // Convert from Pamela XPOS into milliseconds
                        if (!Int32.TryParse(parts[0], out part0))
                        {
                            string friendlyFileName = Path.GetFileName(fileName);
                            errors.Add(new CompileError("Non-numeric phoneme offset '" + parts[0] + "'", friendlyFileName, lineNumber));
                            continue;
                        }
                        int milliSeconds = ((part0 / 15) * 1000) / 24;
                        string phonemeCode = parts[1].Trim().ToUpper();
                        int frameID = FindFrameNumberForPhoneme(phonemeCode);
                        if (frameID < 0)
                        {
                            string friendlyFileName = Path.GetFileName(fileName);
                            errors.Add(new CompileError("No frame found to match phoneme code '" + phonemeCode + "'", friendlyFileName, lineNumber));
                        }
                        else
                        {
                            syncDataForThisFile.Phonemes.Add(new SpeechLipSyncPhoneme(milliSeconds, (short)frameID));
                        }
                    }
                }
            }
            sr.Close();
			AlignPhonemeOffsets(syncDataForThisFile);

            return syncDataForThisFile;
        }

        private SpeechLipSyncLine CompilePapagayoFile(string fileName, CompileMessages errors)
        {
            SpeechLipSyncLine syncDataForThisFile = new SpeechLipSyncLine();
            syncDataForThisFile.FileName = Path.GetFileNameWithoutExtension(fileName);

            string thisLine;
            int lineNumber = 0;

            StreamReader sr = new StreamReader(fileName);
            if ((thisLine = sr.ReadLine()) != null) // Skip over the first line (always a heading)
                while ((thisLine = sr.ReadLine()) != null)
                {
                    lineNumber++;
                    if (thisLine.IndexOf(' ') > 0)
                    {
                        string[] parts = thisLine.Split(' ');
                        int part0;
                        if (!Int32.TryParse(parts[0], out part0))
                        {
                            string friendlyFileName = Path.GetFileName(fileName);
                            errors.Add(new CompileError("Non-numeric phoneme offset '" + parts[0] + "'", friendlyFileName, lineNumber));
                            continue;
                        }
                        int xpos = part0;
                        if (xpos < 0) // Clamp negative XPOS to 0
                            xpos = 0;
                        int milliSeconds = (part0 * 1000) / 24;
                        string phonemeCode = parts[1].Trim().ToUpper();
                        int frameID = FindFrameNumberForPhoneme(phonemeCode);
                        if (frameID < 0)
                        {
                            string friendlyFileName = Path.GetFileName(fileName);
                            errors.Add(new CompileError("No frame found to match phoneme code '" + phonemeCode + "'", friendlyFileName, lineNumber));
                        }
                        else
                        {
                            syncDataForThisFile.Phonemes.Add(new SpeechLipSyncPhoneme(milliSeconds, (short)frameID));
                        }
                    }
                }
            sr.Close();
			AlignPhonemeOffsets(syncDataForThisFile);

            return syncDataForThisFile;
        }

        private void CompileLipSyncFiles(string sourceDir, string outputName, CompileMessages errors)
        {
            List<SpeechLipSyncLine> lipSyncDataLines = new List<SpeechLipSyncLine>();

            foreach (string fileName in Utilities.GetDirectoryFileList(sourceDir, PAMELA_FILE_FILTER))
            {
                lipSyncDataLines.Add(CompilePamelaFile(fileName, errors));
            }

            foreach (string fileName in Utilities.GetDirectoryFileList(sourceDir, PAPAGAYO_FILE_FILTER))
            {
                lipSyncDataLines.Add(CompilePapagayoFile(fileName, errors));
            }

            if (File.Exists(outputName))
            {
                File.Delete(outputName);
            }

            if ((!errors.HasErrors) && (lipSyncDataLines.Count > 0))
            {
                BinaryWriter bw = new BinaryWriter(new FileStream(outputName, FileMode.Create, FileAccess.Write));
                bw.Write((int)4);
                bw.Write(lipSyncDataLines.Count);

                foreach (SpeechLipSyncLine line in lipSyncDataLines)
                {
					bw.Write((short)line.Phonemes.Count);

					byte[] fileNameBytes = Encoding.Default.GetBytes(line.FileName);
					byte[] paddedFileNameBytes = new byte[14];
					Array.Copy(fileNameBytes, paddedFileNameBytes, fileNameBytes.Length);
					paddedFileNameBytes[fileNameBytes.Length] = 0;
					bw.Write(paddedFileNameBytes);

                    for (int i = 0; i < line.Phonemes.Count; i++)
                    {
						bw.Write((int)line.Phonemes[i].EndTimeOffset);
                    }
					for (int i = 0; i < line.Phonemes.Count; i++)
					{
						bw.Write((short)line.Phonemes[i].Frame);
					}
				}

                bw.Close();
            }
        }

        private byte[] RawSerialize(object anything)
        {
            int rawsize = Marshal.SizeOf(anything);
            IntPtr buffer = Marshal.AllocHGlobal(rawsize);
            Marshal.StructureToPtr(anything, buffer, false);
            byte[] rawdatas = new byte[rawsize];
            Marshal.Copy(buffer, rawdatas, 0, rawsize);
            Marshal.FreeHGlobal(buffer);
            return rawdatas;
        }

        private string[] ConstructFileListForSpeechVOX(string sourceDir)
        {
            List<string> files = new List<string>();
            Utilities.AddAllMatchingFiles(files, sourceDir, LIP_SYNC_DATA_OUTPUT, true);
            Utilities.AddAllMatchingFiles(files, sourceDir, MP3_FILE_FILTER, true);
            Utilities.AddAllMatchingFiles(files, sourceDir, OGG_VORBIS_FILE_FILTER, true);
            Utilities.AddAllMatchingFiles(files, sourceDir, WAVEFORM_FILE_FILTER, true);
            return files.ToArray();
        }

		private string[] ConstructFileListForSyncData(string sourceDir)
		{
			List<string> files = new List<string>();
			Utilities.AddAllMatchingFiles(files, sourceDir, PAMELA_FILE_FILTER, true);
			Utilities.AddAllMatchingFiles(files, sourceDir, PAPAGAYO_FILE_FILTER, true);
			return files.ToArray();
		}

		private bool DoesTargetFileNeedRebuild(string targetFile, string[] filesOnDisk, Dictionary<string, DateTime> fileStatuses)
        {
			if (!File.Exists(targetFile))
			{
				return true;
			}

            bool needsRebuild = false;
            foreach (string file in filesOnDisk)
            {
                if (fileStatuses.ContainsKey(file.ToLower()))
                {
                    DateTime lastCheckFileTime = fileStatuses[file.ToLower()];
                    DateTime fileTimeNow = File.GetLastWriteTimeUtc(file);
                    if (fileTimeNow != lastCheckFileTime)
                    {
                        needsRebuild = true;
                        break;
                    }
                }
                else
                {
                    needsRebuild = true;
                    break;
                }
            }
            return needsRebuild;
        }

        private void UpdateVOXFileStatusWithCurrentFileTimes(string[] filesOnDisk, Dictionary<string, DateTime> fileStatuses)
        {
            fileStatuses.Clear();
            foreach (string file in filesOnDisk)
            {
                fileStatuses.Add(file.ToLower(), File.GetLastWriteTimeUtc(file));
            }
        }

		private void RebuildVOXFile(string voxFileName, string[] filesOnDisk)
		{
            if (File.Exists(voxFileName))
            {
                File.Delete(voxFileName);
            }
            if (filesOnDisk.Length > 0)
            {
                // Register speech assets under names = relative paths inside a Speech folder;
                // e.g. Speech/cEgo1.ogg => cEgo1.ogg;
                //      Speech/French/cEgo1.ogg => French/cEgo1.ogg;
                var assets = filesOnDisk.Select(
                    f => new Tuple<string, string>(f.Substring(SPEECH_DIRECTORY.Length + 1), f)).ToArray();
                DataFileWriter.MakeDataFile(assets, 0, voxFileName, false);
            }
        }

        public override void FromXml(XmlNode node)
        {
            if (node != null)
            {
                ReadFileTimes(node, "SpeechVoxFiles", _speechVoxStatus);
				ReadFileTimes(node, "PamFiles", _pamFileStatus);
            }
        }

        public override void ToXml(XmlTextWriter writer)
        {
            WriteFileTimes(writer, "SpeechVoxFiles", _speechVoxStatus);
			WriteFileTimes(writer, "PamFiles", _pamFileStatus);
        }

        /// <summary>
        /// Read a collection of file time dictionaries.
        /// </summary>
        private void ReadFileTimes(XmlNode node, string elementName, Dictionary<string, Dictionary<string, DateTime>> fileStatuses)
        {
            fileStatuses.Clear();
            XmlNode mainNode = node.SelectSingleNode(elementName);
            if (mainNode == null) return;
            // The main node suppose to contain list of sub-nodes defining source file folders
            foreach (XmlNode child in mainNode.ChildNodes)
            {
                if (child.Name != "Folder") continue;
                string folderName = SerializeUtils.GetAttributeString(child, "Name");
                var folderFileStatuses = new Dictionary<string, DateTime>();
                ReadFileTimes(child, folderName, folderFileStatuses);
                fileStatuses.Add(folderName, folderFileStatuses);
            }
        }

        /// <summary>
        /// Read a single file time dictionary.
        /// </summary>
        private void ReadFileTimes(XmlNode mainNode, string elementName, Dictionary<string, DateTime> fileStatuses)
        {
            fileStatuses.Clear();
            foreach (XmlNode child in mainNode.ChildNodes)
            {
                if (child.Name != "File") continue;
                string timeString = SerializeUtils.GetAttributeString(child, "FileTime");
                DateTime fileTime = DateTime.Parse(timeString, CultureInfo.InvariantCulture, DateTimeStyles.AssumeUniversal | DateTimeStyles.AdjustToUniversal);
                fileStatuses.Add(SerializeUtils.GetAttributeString(child, "Name"), fileTime);
            }
        }

        private void WriteFileTimes(XmlTextWriter writer, string elementName, Dictionary<string, Dictionary<string, DateTime>> fileStatuses)
        {
            writer.WriteStartElement(elementName);
            // Write as many "folder" sub-elements as there are dictionaries in the collection
            foreach (string folderName in fileStatuses.Keys)
            {
                WriteFileTimes(writer, folderName, fileStatuses[folderName]);
            }
            writer.WriteEndElement();
        }

        private void WriteFileTimes(XmlTextWriter writer, string elementName, Dictionary<string, DateTime> fileStatuses)
        {
            writer.WriteStartElement("Folder");
            writer.WriteAttributeString("Name", elementName);
            foreach (string file in fileStatuses.Keys)
            {
                writer.WriteStartElement("File");
                writer.WriteAttributeString("Name", file);
                writer.WriteAttributeString("FileTime", fileStatuses[file].ToString("u"));
                writer.WriteEndElement();
            }
            writer.WriteEndElement();
        }
    }
}
