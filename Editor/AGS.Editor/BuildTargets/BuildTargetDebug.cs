﻿using AGS.Types;
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

        public override void DeleteMainGameData(string name, CompileMessages errors)
        {
            string filename = Path.Combine(Path.Combine(OutputDirectoryFullPath, DEBUG_DIRECTORY), name + ".exe");
            Utilities.ExecuteOrError(() => { Utilities.TryDeleteFile(filename); }, $"Failed to delete an old file {filename}.", errors);
        }

        private object CreateDebugFiles(IWorkProgress progress, object parameter)
        {
            CompileMessages errors = (parameter as CompileMessages);
            if (!DataFileWriter.SaveThisGameToFile(AGSEditor.COMPILED_DTA_FILE_NAME, Factory.AGSEditor.CurrentGame, errors))
            {
                return null;
            }
            Factory.NativeProxy.CreateDebugMiniEXE(new string[] { AGSEditor.COMPILED_DTA_FILE_NAME },
                Factory.AGSEditor.BaseGameFileName + ".exe");
            Utilities.TryDeleteFile(AGSEditor.COMPILED_DTA_FILE_NAME);
            return null;
        }

        public override bool Build(CompileMessages errors, bool forceRebuild)
        {
            if (!base.Build(errors, forceRebuild)) return false;
            try
            {
                string baseGameFileName = Factory.AGSEditor.BaseGameFileName;
                string exeFileName = baseGameFileName + ".exe";
                BuildTargetWindows targetWin = BuildTargetsInfo.FindBuildTargetByName("Windows") as BuildTargetWindows;
                if (targetWin == null)
                {
                    errors.Add(new CompileError("Debug build depends on Windows build target being available! Your AGS installation may be corrupted!"));
                    return false;
                }
                string sourceEXE = Path.Combine(Factory.AGSEditor.EditorDirectory, AGSEditor.ENGINE_EXE_FILE_NAME);
                File.Copy(sourceEXE, exeFileName, true);
                BusyDialog.Show("Please wait while we prepare to run the game...", new BusyDialog.ProcessingHandler(CreateDebugFiles), errors);
                if (errors.HasErrors)
                {
                    return false;
                }
                string exePath = GetDebugPath(exeFileName);
                if (!Utilities.ExecuteOrError(() =>
                    {
                        Utilities.TryDeleteFile(exePath);
                        File.Move(exeFileName, exePath);
                    },
                    $"Failed to replace an old file {exePath}", errors))
                {
                    return false;
                }
                
                // copy configuration from Compiled folder to use with Debugging
                string cfgFilePath = targetWin.GetCompiledPath(AGSEditor.CONFIG_FILE_NAME);
                if (File.Exists(cfgFilePath))
                {
                    File.Copy(cfgFilePath, GetDebugPath(AGSEditor.CONFIG_FILE_NAME), true);
                }
                else
                {
                    cfgFilePath = Path.Combine(AGSEditor.OUTPUT_DIRECTORY, Path.Combine(AGSEditor.DATA_OUTPUT_DIRECTORY, AGSEditor.CONFIG_FILE_NAME));
                    if (File.Exists(cfgFilePath))
                    {
                        File.Copy(cfgFilePath, GetDebugPath(AGSEditor.CONFIG_FILE_NAME), true);
                    }
                }
                // Copy DLLs
                File.Copy(Path.Combine(Factory.AGSEditor.EditorDirectory, "SDL2.dll"), GetDebugPath("SDL2.dll"), true);
                // Copy plugins
                foreach (Plugin plugin in Factory.AGSEditor.CurrentGame.Plugins)
                {
                    File.Copy(Path.Combine(Factory.AGSEditor.EditorDirectory, plugin.FileName), GetDebugPath(plugin.FileName), true);
                }

                // Copy files from Compiled/Data to Compiled/Windows, because this is currently where game will be looking them up
                targetWin.CopyAuxiliaryGameFiles(Path.Combine(AGSEditor.OUTPUT_DIRECTORY, AGSEditor.DATA_OUTPUT_DIRECTORY), false, errors);
                // Update config file with current game parameters
                GenerateConfigFile(GetCompiledPath());
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
