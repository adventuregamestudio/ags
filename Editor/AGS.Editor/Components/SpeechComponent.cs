using System;
using System.Collections.Generic;
using System.Drawing;
using System.Globalization;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows.Forms;
using System.Xml;
using AGS.Types;

namespace AGS.Editor.Components
{
    class SpeechComponent : BaseComponent
    {
        private static readonly string PAMELA_FILE_FILTER = "Speech" + Path.DirectorySeparatorChar + "*.pam";
        private static readonly string PAPAGAYO_FILE_FILTER = "Speech" + Path.DirectorySeparatorChar + "*.dat";
        private static readonly string OGG_VORBIS_FILE_FILTER = "Speech" + Path.DirectorySeparatorChar + "*.ogg";
        private static readonly string MP3_FILE_FILTER = "Speech" + Path.DirectorySeparatorChar + "*.mp3";
        private static readonly string WAVEFORM_FILE_FILTER = "Speech" + Path.DirectorySeparatorChar + "*.wav";
        private const string LIP_SYNC_DATA_OUTPUT = "syncdata.dat";
        private static readonly string SPEECH_VOX_FILE_NAME = Path.Combine(AGSEditor.OUTPUT_DIRECTORY, "speech.vox");

        private Dictionary<string, DateTime> _speechVoxStatus = new Dictionary<string, DateTime>();
		private Dictionary<string, DateTime> _pamFileStatus = new Dictionary<string, DateTime>();

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
			string[] pamFileList = ConstructFileListForSyncData();

			if (DoesTargetFileNeedRebuild(LIP_SYNC_DATA_OUTPUT, pamFileList, _pamFileStatus))
			{
				CompileLipSyncFiles(errors);

				UpdateVOXFileStatusWithCurrentFileTimes(pamFileList, _pamFileStatus);
			}
        }

        private void _agsEditor_ExtraOutputCreationStep()
        {
            string[] speechFileList = ConstructFileListForSpeechVOX();
            RebuildVOXFileIfRequired(SPEECH_VOX_FILE_NAME, speechFileList, _speechVoxStatus);
        }

        public override string ComponentID
        {
            get { return ComponentIDs.Speech; }
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

        private void CompileLipSyncFiles(CompileMessages errors)
        {
            List<SpeechLipSyncLine> lipSyncDataLines = new List<SpeechLipSyncLine>();

            foreach (string fileName in Utilities.GetDirectoryFileList(Directory.GetCurrentDirectory(), PAMELA_FILE_FILTER))
            {
                lipSyncDataLines.Add(CompilePamelaFile(fileName, errors));
            }

            foreach (string fileName in Utilities.GetDirectoryFileList(Directory.GetCurrentDirectory(), PAPAGAYO_FILE_FILTER))
            {
                lipSyncDataLines.Add(CompilePapagayoFile(fileName, errors));
            }

            if (File.Exists(LIP_SYNC_DATA_OUTPUT))
            {
                File.Delete(LIP_SYNC_DATA_OUTPUT);
            }

            if ((!errors.HasErrors) && (lipSyncDataLines.Count > 0))
            {
                BinaryWriter bw = new BinaryWriter(new FileStream(LIP_SYNC_DATA_OUTPUT, FileMode.Create, FileAccess.Write));
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

        private string[] ConstructFileListForSpeechVOX()
        {
            List<string> files = new List<string>();
            Utilities.AddAllMatchingFiles(files, LIP_SYNC_DATA_OUTPUT);
            Utilities.AddAllMatchingFiles(files, MP3_FILE_FILTER);
            Utilities.AddAllMatchingFiles(files, OGG_VORBIS_FILE_FILTER);
            Utilities.AddAllMatchingFiles(files, WAVEFORM_FILE_FILTER);
            return files.ToArray();
        }

		private string[] ConstructFileListForSyncData()
		{
			List<string> files = new List<string>();
			Utilities.AddAllMatchingFiles(files, PAMELA_FILE_FILTER);
			Utilities.AddAllMatchingFiles(files, PAPAGAYO_FILE_FILTER);
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
                    }
                }
                else
                {
                    needsRebuild = true;
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

		private void RebuildVOXFileIfRequired(string voxFileName, string[] filesOnDisk, Dictionary<string,DateTime> sourceFileTimes)
		{
			if (DoesTargetFileNeedRebuild(voxFileName, filesOnDisk, sourceFileTimes))
			{
				if (File.Exists(voxFileName))
				{
					File.Delete(voxFileName);
				}
				if (filesOnDisk.Length > 0)
				{
					Factory.NativeProxy.CreateVOXFile(voxFileName, filesOnDisk);
				}
				UpdateVOXFileStatusWithCurrentFileTimes(filesOnDisk, sourceFileTimes);
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

        private void ReadFileTimes(XmlNode node, string elementName, Dictionary<string, DateTime> fileStatuses)
        {
            fileStatuses.Clear();

            XmlNode mainNode = node.SelectSingleNode(elementName);
            if (mainNode != null)
            {
                foreach (XmlNode child in mainNode.ChildNodes)
                {
					string timeString = SerializeUtils.GetAttributeString(child, "FileTime");
					DateTime fileTime = DateTime.Parse(timeString, CultureInfo.InvariantCulture, DateTimeStyles.AssumeUniversal | DateTimeStyles.AdjustToUniversal);
                    fileStatuses.Add(SerializeUtils.GetAttributeString(child, "Name"), fileTime);
                }
            }
        }

        private void WriteFileTimes(XmlTextWriter writer, string elementName, Dictionary<string, DateTime> fileStatuses)
        {
            writer.WriteStartElement(elementName);
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
