using AGS.Types;
using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Xml;
using System.Windows.Forms;

namespace AGS.Editor
{
    public class BuildTargetWindows : BuildTargetBase
    {
        public const string WINDOWS_DIRECTORY = "Windows";

        public override IDictionary<string, string> GetRequiredLibraryPaths()
        {
            Dictionary<string, string> paths = new Dictionary<string, string>();
            paths.Add(AGSEditor.ENGINE_EXE_FILE_NAME, Factory.AGSEditor.EditorDirectory);
            return paths;
        }

        public override string[] GetPlatformStandardSubfolders()
        {
            return new string[] { GetCompiledPath() };
        }

        public void CopyPlugins(CompileMessages errors)
        {
            try
            {
                string outputDir = GetCompiledPath();
                foreach (Plugin plugin in Factory.AGSEditor.CurrentGame.Plugins)
                {
                    File.Copy(Path.Combine(Factory.AGSEditor.EditorDirectory, plugin.FileName), Path.Combine(outputDir, plugin.FileName), true);
                }
            }
            catch (Exception ex)
            {
                errors.Add(new CompileError("Unexpected error: " + ex.Message));
            }
        }

        public void CreateCompiledSetupProgram()
        {
            string setupFileName = GetCompiledPath(AGSEditor.COMPILED_SETUP_FILE_NAME);
            Resources.ResourceManager.CopyFileFromResourcesToDisk(AGSEditor.SETUP_PROGRAM_SOURCE_FILE, setupFileName);

            if (File.Exists(AGSEditor.SETUP_ICON_FILE_NAME))
            {
                try
                {
                    Factory.NativeProxy.UpdateFileIcon(setupFileName, AGSEditor.SETUP_ICON_FILE_NAME);
                }
                catch (AGSEditorException ex)
                {
                    Factory.GUIController.ShowMessage("An problem occurred setting your custom icon onto the setup file. The error was: " + ex.Message, MessageBoxIcon.Warning);
                }
            }

            string gameFileName = Factory.AGSEditor.BaseGameFileName + ".exe";

            BinaryWriter sw = new BinaryWriter(File.Open(setupFileName, FileMode.Append, FileAccess.Write));
            sw.Write(Encoding.ASCII.GetBytes(gameFileName));
            sw.Write((byte)0);
            sw.Write(gameFileName.Length + 1);
            sw.Write(Encoding.ASCII.GetBytes("STCUSTOM"));
            sw.Close();
        }

        private string GenerateGameExplorerXML()
        {
            Game _game = Factory.AGSEditor.CurrentGame;
            StringWriter sw = new StringWriter();
            XmlTextWriter writer = new XmlTextWriter(sw);
            writer.Formatting = Formatting.Indented;

            writer.WriteProcessingInstruction("xml", "version=\"1.0\" encoding=\"utf-8\"");

            writer.WriteStartElement("GameDefinitionFile");
            writer.WriteAttributeString("xmlns:baseTypes", "urn:schemas-microsoft-com:GamesExplorerBaseTypes.v1");
            writer.WriteAttributeString("xmlns", "urn:schemas-microsoft-com:GameDescription.v1");

            writer.WriteStartElement("GameDefinition");
            writer.WriteAttributeString("gameID", _game.Settings.GUIDAsString);
            writer.WriteElementString("Name", _game.Settings.GameName);
            writer.WriteElementString("Description", _game.Settings.Description);
            writer.WriteElementString("ReleaseDate", _game.Settings.ReleaseDate.ToString("yyyy-MM-dd"));

            writer.WriteStartElement("Genres");
            writer.WriteElementString("Genre", _game.Settings.Genre);
            writer.WriteEndElement();

            if (!string.IsNullOrEmpty(_game.Settings.SaveGameFolderName))
            {
                writer.WriteStartElement("SavedGames");
                writer.WriteAttributeString("baseKnownFolderID", "{4C5C32FF-BB9D-43B0-B5B4-2D72E54EAAA4}");
                writer.WriteAttributeString("path", _game.Settings.SaveGameFolderName);
                writer.WriteEndElement();
            }

            writer.WriteStartElement("Version");
            writer.WriteStartElement("VersionNumber");
            writer.WriteAttributeString("versionNumber", _game.Settings.Version);
            writer.WriteEndElement();
            writer.WriteEndElement();

            writer.WriteStartElement("WindowsSystemPerformanceRating");
            writer.WriteAttributeString("minimum", _game.Settings.WindowsExperienceIndex.ToString());
            writer.WriteAttributeString("recommended", _game.Settings.WindowsExperienceIndex.ToString());
            writer.WriteEndElement();

            if (!string.IsNullOrEmpty(_game.Settings.DeveloperName))
            {
                writer.WriteStartElement("Developers");
                writer.WriteStartElement("Developer");
                writer.WriteAttributeString("URI", _game.Settings.DeveloperURL);
                writer.WriteString(_game.Settings.DeveloperName);
                writer.WriteEndElement();
                writer.WriteEndElement();
            }

            writer.WriteEndElement();
            writer.WriteEndElement();
            writer.Flush();

            string xml = sw.ToString();
            writer.Close();
            return xml;
        }

        private void UpdateVistaGameExplorerResources(string newExeName)
        {
            if (Factory.AGSEditor.CurrentGame.Settings.GameExplorerEnabled)
            {
                string xml = GenerateGameExplorerXML();
                Factory.NativeProxy.UpdateGameExplorerXML(newExeName, Encoding.UTF8.GetBytes(xml));

                if (File.Exists(AGSEditor.GAME_EXPLORER_THUMBNAIL_FILE_NAME))
                {
                    BinaryReader br = new BinaryReader(new FileStream(AGSEditor.GAME_EXPLORER_THUMBNAIL_FILE_NAME, FileMode.Open, FileAccess.Read));
                    byte[] data = br.ReadBytes((int)br.BaseStream.Length);
                    br.Close();

                    Factory.NativeProxy.UpdateGameExplorerThumbnail(newExeName, data);
                }
            }
            else
            {
                Factory.NativeProxy.UpdateGameExplorerXML(newExeName, null);
            }
        }

        public void UpdateWindowsEXE(string filename)
        {
            UpdateWindowsEXE(filename, null);
        }

        public void UpdateWindowsEXE(string filename, CompileMessages errors)
        {
            if (!File.Exists(filename)) return;
            if (errors == null) errors = new CompileMessages();
            if (File.Exists(AGSEditor.CUSTOM_ICON_FILE_NAME))
            {
                try
                {
                    Factory.NativeProxy.UpdateFileIcon(filename, AGSEditor.CUSTOM_ICON_FILE_NAME);
                }
                catch (AGSEditorException ex)
                {
                    Factory.GUIController.ShowMessage("An problem occurred setting your custom icon onto the EXE file. The error was: " + ex.Message, MessageBoxIcon.Warning);
                }
            }
            try
            {
                UpdateVistaGameExplorerResources(filename);
            }
            catch (Exception ex)
            {
                errors.Add(new CompileError("Unable to register for Windows Game Explorer: " + ex.Message));
            }
            try
            {
                Factory.NativeProxy.UpdateFileVersionInfo(filename, Factory.AGSEditor.CurrentGame.Settings.DeveloperName, Factory.AGSEditor.CurrentGame.Settings.GameName);
            }
            catch (Exception ex)
            {
                errors.Add(new CompileError("Unable to set EXE name/description: " + ex.Message));
            }
        }

        public override bool Build(CompileMessages errors, bool forceRebuild)
        {
            if (!base.Build(errors, forceRebuild)) return false;
            string compiledDataDir = Path.Combine(AGSEditor.OUTPUT_DIRECTORY, AGSEditor.DATA_OUTPUT_DIRECTORY);
            string baseGameFileName = Factory.AGSEditor.BaseGameFileName;
            string newExeName = GetCompiledPath(baseGameFileName + ".exe");
            string sourceEXE = Path.Combine(Factory.AGSEditor.EditorDirectory, AGSEditor.ENGINE_EXE_FILE_NAME);
            File.Copy(sourceEXE, newExeName, true);
            UpdateWindowsEXE(newExeName, errors);
            CreateCompiledSetupProgram();
            Environment.CurrentDirectory = Factory.AGSEditor.CurrentGame.DirectoryPath;
            string mainGameData = Path.Combine(compiledDataDir, baseGameFileName + ".ags");
            if (File.Exists(mainGameData))
                AttachDataToEXE(mainGameData, newExeName);
            else
                errors.Add(new CompileError(String.Format("Unable to locate main game data file at '{0}'", mainGameData)));
            CopyAuxiliaryGameFiles(compiledDataDir, true);
            // Update config file with current game parameters
            Factory.AGSEditor.WriteConfigFile(GetCompiledPath());
            // Copy Windows plugins
            CopyPlugins(errors);
            return true;
        }

        private void AttachDataToEXE(string sourceFile, string destFile)
        {
            using (FileStream ostream = File.Open(destFile, FileMode.Append, FileAccess.Write))
            {
                long startPosition = ostream.Position;
                using (FileStream istream = File.Open(sourceFile, FileMode.Open, FileAccess.Read))
                {
                    const int bufferSize = 4096;
                    byte[] buffer = new byte[bufferSize];
                    for (int count = istream.Read(buffer, 0, bufferSize); count > 0;
                        count = istream.Read(buffer, 0, bufferSize))
                    {
                        ostream.Write(buffer, 0, count);
                    }
                }
                // TODO: use functions shared with DataFileWriter
                // write the offset into the EXE where the first data file resides
                ostream.Write(BitConverter.GetBytes(startPosition), 0, 8);
                // write the CLIB end signature so the engine knows this is a valid EXE
                ostream.Write(Encoding.UTF8.GetBytes(DataFileWriter.CLIB_END_SIGNATURE.ToCharArray()), 0,
                    DataFileWriter.CLIB_END_SIGNATURE.Length);
            }
        }

        /// <summary>
        /// Copies all files that could potentially be a part of the game
        /// into the final compiled directory.
        /// </summary>
        public void CopyAuxiliaryGameFiles(string sourcePath, bool alwaysOverwrite)
        {
            List<string> files = GetAuxiliaryGameFiles(sourcePath);
            foreach (string fileName in files)
            {
                string destFile = GetCompiledPath(Path.GetFileName(fileName));
                if (alwaysOverwrite ||
                    !File.Exists(destFile) ||
                    File.GetLastWriteTimeUtc(destFile).CompareTo(File.GetLastWriteTimeUtc(fileName)) < 0)
                {
                    Utilities.HardlinkOrCopy(destFile, fileName, true);
                }
            }
        }

        /// <summary>
        /// Gathers a list of files that could potentially be a part of the game.
        /// Skips main game data (*.ags) and config (these are treated separately).
        /// Returns an array of file paths.
        /// </summary>
        private List<string> GetAuxiliaryGameFiles(string sourcePath)
        {
            List<string> files = new List<string>();
            foreach (string fileName in Utilities.GetDirectoryFileList(sourcePath, "*"))
            {
                // TODO: this attributes check was added as a part of a hotfix.
                // Constructing a list of game files for distribution package
                // should be shared between build targets and done in a more
                // elaborate way.
                if ((File.GetAttributes(fileName) & (FileAttributes.Hidden | FileAttributes.System | FileAttributes.Temporary)) != 0)
                    continue;
                if (fileName.EndsWith(".ags") || fileName.EndsWith(AGSEditor.CONFIG_FILE_NAME))
                    continue;
                files.Add(fileName);
            }
            return files;
        }

        public override string Name
        {
            get
            {
                return "Windows";
            }
        }

        public override string OutputDirectory
        {
            get
            {
                return WINDOWS_DIRECTORY;
            }
        }
    }
}
