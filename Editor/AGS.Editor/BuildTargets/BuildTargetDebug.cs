using AGS.Types;
using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace AGS.Editor
{
    public class BuildTargetDebug : BuildTargetBase
    {
        public const string DEBUG_DIRECTORY = AGSEditor.DEBUG_OUTPUT_DIRECTORY;
        public const string DEBUG_TARGET_NAME = "_Debug";

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

        public override string GetCompiledPath(params string[] parts)
        {
            return base.GetCompiledPath(parts).Replace(AGSEditor.OUTPUT_DIRECTORY, DEBUG_DIRECTORY);
        }

        public virtual string GetDebugPath(params string[] parts)
        {
            if (parts[0] == DEBUG_DIRECTORY) parts[0] = AGSEditor.OUTPUT_DIRECTORY;
            return GetCompiledPath(parts);
        }

        private object CreateDebugFiles(object parameter)
        {
            Factory.AGSEditor.SetMODMusicFlag();
            if (!Factory.AGSEditor.Preferences.UseLegacyCompiler)
            {
                CompileMessages errors = (parameter as CompileMessages);
                if (!DataFileWriter.SaveThisGameToFile(AGSEditor.COMPILED_DTA_FILE_NAME, Factory.AGSEditor.CurrentGame, errors))
                {
                    return null;
                }
            }
            else Factory.NativeProxy.CompileGameToDTAFile(Factory.AGSEditor.CurrentGame, AGSEditor.COMPILED_DTA_FILE_NAME);
            Factory.NativeProxy.CreateDebugMiniEXE(new string[] { AGSEditor.COMPILED_DTA_FILE_NAME },
                Factory.AGSEditor.BaseGameFileName + ".exe");
            File.Delete(AGSEditor.COMPILED_DTA_FILE_NAME);
            return null;
        }

        public override bool Build(CompileMessages errors, bool forceRebuild)
        {
            if (!base.Build(errors, forceRebuild)) return false;
            try
            {
                string baseGameFileName = Factory.AGSEditor.BaseGameFileName;
                string exeFileName = baseGameFileName + ".exe";
                string compiledEXE = Path.Combine(Path.Combine(AGSEditor.OUTPUT_DIRECTORY,
                    BuildTargetWindows.WINDOWS_DIRECTORY), exeFileName);
                string sourceEXE = Path.Combine(Factory.AGSEditor.EditorDirectory, AGSEditor.ENGINE_EXE_FILE_NAME);
                Utilities.DeleteFileIfExists(compiledEXE);
                File.Copy(sourceEXE, exeFileName, true);
                BusyDialog.Show("Please wait while we prepare to run the game...", new BusyDialog.ProcessingHandler(CreateDebugFiles), errors);
                if (errors.HasErrors)
                {
                    return false;
                }
                Utilities.DeleteFileIfExists(GetDebugPath(exeFileName));
                File.Move(exeFileName, GetDebugPath(exeFileName));
                // copy configuration from Compiled folder to use with Debugging
                string cfgFilePath = Path.Combine(AGSEditor.OUTPUT_DIRECTORY, AGSEditor.CONFIG_FILE_NAME);
                if (!Factory.AGSEditor.Preferences.UseLegacyCompiler)
                {
                    IBuildTarget targetWin = BuildTargetsInfo.FindBuildTargetByName("Windows");
                    if (targetWin != null)
                        cfgFilePath = targetWin.GetCompiledPath(AGSEditor.OUTPUT_DIRECTORY, AGSEditor.CONFIG_FILE_NAME);
                }
                if (File.Exists(cfgFilePath))
                {
                    File.Copy(cfgFilePath, GetDebugPath(AGSEditor.CONFIG_FILE_NAME), true);
                }
                foreach (Plugin plugin in Factory.AGSEditor.CurrentGame.Plugins)
                {
                    File.Copy(Path.Combine(Factory.AGSEditor.EditorDirectory, plugin.FileName), GetDebugPath(plugin.FileName), true);
                }
            }
            catch (Exception ex)
            {
                errors.Add(new CompileError("Unexpected error: " + ex.Message));
                return false;
            }
            return true;
        }

        public override bool IsNormalBuildTarget
        {
            get
            {
                return false;
            }
        }

        public override string Name
        {
            get
            {
                return DEBUG_TARGET_NAME;
            }
        }

        public override string OutputDirectory
        {
            get
            {
                return "";
            }
        }

        public override string OutputDirectoryFullPath
        {
            get
            {
                return Path.Combine(Factory.AGSEditor.CurrentGame.DirectoryPath, OutputDirectory);
            }
        }
    }
}
