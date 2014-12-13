﻿using AGS.Types;
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

        private void CreateCompiledSetupProgram()
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

        public override bool Build(CompileMessages errors, bool forceRebuild)
        {
            if (!base.Build(errors, forceRebuild)) return false;
            string newExeName = Path.Combine(Path.Combine(AGSEditor.OUTPUT_DIRECTORY, WINDOWS_DIRECTORY),
                Factory.AGSEditor.BaseGameFileName + ".exe");
            string sourceEXE = Path.Combine(Factory.AGSEditor.EditorDirectory, AGSEditor.ENGINE_EXE_FILE_NAME);
            File.Copy(sourceEXE, newExeName, true);
            if (File.Exists(AGSEditor.CUSTOM_ICON_FILE_NAME))
            {
                try
                {
                    Factory.NativeProxy.UpdateFileIcon(newExeName, AGSEditor.CUSTOM_ICON_FILE_NAME);
                }
                catch (AGSEditorException ex)
                {
                    Factory.GUIController.ShowMessage("An problem occurred setting your custom icon onto the EXE file. The error was: " + ex.Message, MessageBoxIcon.Warning);
                }
            }
            try
            {
                UpdateVistaGameExplorerResources(newExeName);
            }
            catch (Exception ex)
            {
                errors.Add(new CompileError("Unable to register for Vista Game Explorer: " + ex.Message));
            }
            try
            {
                Factory.NativeProxy.UpdateFileVersionInfo(newExeName, Factory.AGSEditor.CurrentGame.Settings.DeveloperName, Factory.AGSEditor.CurrentGame.Settings.GameName);
            }
            catch (Exception ex)
            {
                errors.Add(new CompileError("Unable to set EXE name/description: " + ex.Message));
            }
            CreateCompiledSetupProgram();
            Environment.CurrentDirectory = Factory.AGSEditor.CurrentGame.DirectoryPath;
            string compiledDir = AGSEditor.OUTPUT_DIRECTORY;
            if (!File.Exists(GetCompiledPath(AGSEditor.CONFIG_FILE_NAME)))
            {
                // don't hard-link config file
                File.Copy(Path.Combine(compiledDir, AGSEditor.CONFIG_FILE_NAME),
                    GetCompiledPath(AGSEditor.CONFIG_FILE_NAME));
            }
            foreach (string fileName in Utilities.GetDirectoryFileList(compiledDir, "*.vox"))
            {
                Utilities.CreateHardLink(GetCompiledPath(fileName.Split('\\')), fileName, true);
            }
            foreach (string fileName in Utilities.GetDirectoryFileList(compiledDir, "*.tra"))
            {
                Utilities.CreateHardLink(GetCompiledPath(fileName.Split('\\')), fileName, true);
            }
            string baseGameFileName = Factory.AGSEditor.BaseGameFileName;
            using (FileStream ostream = File.Open(GetCompiledPath(baseGameFileName + ".exe"), FileMode.Append,
                FileAccess.Write))
            {
                int startPosition = (int)ostream.Position;
                bool has000File = false;
                foreach (string fileName in Utilities.GetDirectoryFileList(compiledDir, baseGameFileName + ".0*"))
                {
                    if (fileName.EndsWith(".000"))
                    {
                        using (FileStream istream = File.Open(fileName, FileMode.Open, FileAccess.Read))
                        {
                            byte[] buffer = new byte[4096];
                            for (int count = istream.Read(buffer, 0, 4096); count > 0;
                                count = istream.Read(buffer, 0, 4096))
                            {
                                ostream.Write(buffer, 0, count);
                            }
                        }
                        has000File = true;
                    }
                    else Utilities.CreateHardLink(GetCompiledPath(fileName), fileName, true);
                }
                if (has000File)
                {
                    // write the offset into the EXE where the first data file resides
                    ostream.Write(BitConverter.GetBytes(startPosition), 0, 4);
                    // write the CLIB end signature so the engine knows this is a valid EXE
                    ostream.Write(Encoding.UTF8.GetBytes(NativeConstants.CLIB_END_SIGNATURE.ToCharArray()), 0,
                        NativeConstants.CLIB_END_SIGNATURE.Length);
                }
            }
            return true;
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
