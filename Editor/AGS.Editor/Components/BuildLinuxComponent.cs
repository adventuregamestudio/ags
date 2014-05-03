﻿using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace AGS.Editor.Components
{
    class BuildLinuxComponent : BaseComponent
    {
        private AGSEditor _editor;
        private string[] libs;
        private string editorLinuxDir = null;
        private string editorAGS32Path = null;
        private string editorAGS64Path = null;
        private string editorLib32Dir = null;
        private string editorLib64Dir = null;
        private string gameLinuxDir = null;
        private string gameLinuxAGS32Path = null;
        private string gameLinuxAGS64Path = null;
        private string gameLinuxDataDir = null;
        private string gameLinuxDataLib32Dir = null;
        private string gameLinuxDataLib64Dir = null;
        private string gameCompiledDir = null;
        private static BuildLinuxComponent _instance;

        public static BuildLinuxComponent Instance
        {
            get { return _instance; }
        }

        public BuildLinuxComponent(GUIController guiController, AGSEditor agsEditor) : base(guiController, agsEditor)
        {
            _instance = this;
            _editor = agsEditor;
            libs = new string[]
            {
                "alleg-alsadigi.so",
                "alleg-alsamidi.so",
                "libaldmb.so.1",
                "liballeg.so.4.4",
                "libdumb.so.1",
                "libfreetype.so.6",
                "libogg.so.0",
                "libtheora.so.0",
                "libvorbis.so.0",
                "libvorbisfile.so.3",
                "modules.lst"
            };
            _editor.Preferences.BuildForAllPorts = _editor.Preferences.BuildForAllPorts; // to initialize the event!
        }

        void InitPaths()
        {
            if (editorLinuxDir != null) return; // paths already set
            editorLinuxDir = Path.Combine(_editor.EditorDirectory, "linux");
            editorAGS32Path = Path.Combine(editorLinuxDir, "ags32");
            editorAGS64Path = Path.Combine(editorLinuxDir, "ags64");
            editorLib32Dir = Path.Combine(editorLinuxDir, "lib32");
            editorLib64Dir = Path.Combine(editorLinuxDir, "lib64");
            gameLinuxDir = Path.Combine(_editor.CurrentGame.DirectoryPath, "linux");
            gameLinuxDataDir = Path.Combine(gameLinuxDir, "data");
            gameLinuxAGS32Path = Path.Combine(gameLinuxDataDir, "ags32");
            gameLinuxAGS64Path = Path.Combine(gameLinuxDataDir, "ags64");
            gameLinuxDataLib32Dir = Path.Combine(gameLinuxDataDir, "lib32");
            gameLinuxDataLib64Dir = Path.Combine(gameLinuxDataDir, "lib64");
            gameCompiledDir = Path.Combine(_editor.CurrentGame.DirectoryPath, "Compiled");
        }

        public void BuildForLinux()
        {
            InitPaths();
            if (EnsureLibsExist().RaiseWarning() != System.Windows.Forms.DialogResult.Yes) return;
            if (EnsurePluginsExist().RaiseWarning() != System.Windows.Forms.DialogResult.Yes) return;
            CopyFilesFromCompiledDir();
        }

        public override string ComponentID
        {
            get { return ComponentIDs.BuildLinux; }
        }

        public override void CommandClick(string controlID)
        {
            base.CommandClick(controlID);
        }

        public override void RefreshDataFromGame()
        {
            base.RefreshDataFromGame();
        }

        private class MissingLibOrPlugin
        {
            public enum ArchType
            {
                Arch32,
                Arch64,
                Unknown
            };

            public enum LibOrPlugin
            {
                Lib,
                Plugin,
                Engine,
                Unknown
            };

            public ArchType Architecture;
            public LibOrPlugin Which;
            public string Name;

            public MissingLibOrPlugin(ArchType arch, LibOrPlugin which, string name)
            {
                Architecture = arch;
                Which = which;
                Name = name;
            }

            public System.Windows.Forms.DialogResult RaiseWarning()
            {
                if (this == Empty) return System.Windows.Forms.DialogResult.Yes;
                return System.Windows.Forms.MessageBox.Show(
                    "Could not find " + Which.ToString().ToLower() + " '" + Name + "' (" + (Architecture == ArchType.Arch32 ? "32" : "64") + "-bit). Your game may not run properly without this file. Do you wish to continue building for Linux without it?",
                    "Warning!", System.Windows.Forms.MessageBoxButtons.YesNo, System.Windows.Forms.MessageBoxIcon.Warning);
            }

            public static readonly MissingLibOrPlugin Empty = new MissingLibOrPlugin(ArchType.Unknown, LibOrPlugin.Unknown, string.Empty);
        };

        private MissingLibOrPlugin EnsureLibsExist()
        {
            foreach (string lib in libs)
            {
                if (!File.Exists(Path.Combine(editorLib32Dir, lib)))
                    return new MissingLibOrPlugin(MissingLibOrPlugin.ArchType.Arch32, MissingLibOrPlugin.LibOrPlugin.Lib, lib);
                if (!File.Exists(Path.Combine(editorLib64Dir, lib)))
                    return new MissingLibOrPlugin(MissingLibOrPlugin.ArchType.Arch64, MissingLibOrPlugin.LibOrPlugin.Lib, lib);
            }
            if (!File.Exists(editorAGS32Path))
                return new MissingLibOrPlugin(MissingLibOrPlugin.ArchType.Arch32, MissingLibOrPlugin.LibOrPlugin.Engine, "ags32");
            if (!File.Exists(editorAGS64Path))
                return new MissingLibOrPlugin(MissingLibOrPlugin.ArchType.Arch64, MissingLibOrPlugin.LibOrPlugin.Engine, "ags64");
            return MissingLibOrPlugin.Empty;
        }

        private string GetPluginSOName(AGS.Types.Plugin plugin)
        {
            return "lib" + plugin.FileName.Substring(0, plugin.FileName.Length - 3) + "so";
        }

        private MissingLibOrPlugin EnsurePluginsExist()
        {
            foreach (AGS.Types.Plugin plugin in _editor.CurrentGame.Plugins)
            {
                string soName = GetPluginSOName(plugin);
                if (!File.Exists(Path.Combine(editorLib32Dir, soName)))
                    return new MissingLibOrPlugin(MissingLibOrPlugin.ArchType.Arch32, MissingLibOrPlugin.LibOrPlugin.Plugin, soName);
                if (!File.Exists(Path.Combine(editorLib64Dir, soName)))
                    return new MissingLibOrPlugin(MissingLibOrPlugin.ArchType.Arch64, MissingLibOrPlugin.LibOrPlugin.Plugin, soName);
            }
            return MissingLibOrPlugin.Empty;
        }

        private string GetLibSymLinkScriptForEachPlugin(bool is64)
        {
            string results = "";
            foreach (AGS.Types.Plugin plugin in _editor.CurrentGame.Plugins)
            {
                string soName = GetPluginSOName(plugin);
                results +=
@"
    ln -f -s ""$SCRIPTPATH/data/lib" + (is64 ? "64" : "32") + @"/" + soName + @""" """ + soName + @"""";
            }
            return results;
        }

        private void CopyFilesFromDir(string inDir, string outDir)
        {
            // inDir and outDir must be validated first
            foreach (string file in Directory.GetFiles(inDir))
            {
                string fileName = Path.GetFileName(file);
                if (fileName.StartsWith("libags", StringComparison.OrdinalIgnoreCase))
                {
                    bool found = false;
                    foreach (AGS.Types.Plugin plugin in _editor.CurrentGame.Plugins)
                    {
                        string soName = GetPluginSOName(plugin);
                        if (soName.Equals(fileName, StringComparison.OrdinalIgnoreCase))
                        {
                            found = true;
                            break;
                        }
                    }
                    if (!found) continue;
                }
                File.Copy(file, Path.Combine(outDir, fileName), true);
            }
        }

        private void CopyFilesFromCompiledDir()
        {
            if (!Directory.Exists(gameLinuxDir)) Directory.CreateDirectory(gameLinuxDir);
            if (!Directory.Exists(gameLinuxDataDir)) Directory.CreateDirectory(gameLinuxDataDir);
            if (!Directory.Exists(gameLinuxDataLib32Dir)) Directory.CreateDirectory(gameLinuxDataLib32Dir);
            if (!Directory.Exists(gameLinuxDataLib64Dir)) Directory.CreateDirectory(gameLinuxDataLib64Dir);
            string[] compiledFiles = Directory.GetFiles(gameCompiledDir);
            foreach (string file in compiledFiles)
            {
                if ((!file.EndsWith(".dll", StringComparison.OrdinalIgnoreCase)) && (!Path.GetFileName(file).Equals("winsetup.exe", StringComparison.OrdinalIgnoreCase))) File.Copy(file, Path.Combine(gameLinuxDataDir, Path.GetFileName(file)), true);
            }
            CopyFilesFromDir(editorLib32Dir, gameLinuxDataLib32Dir);
            CopyFilesFromDir(editorLib64Dir, gameLinuxDataLib64Dir);
            File.Copy(editorAGS32Path, Path.Combine(gameLinuxDataDir, "ags32"), true);
            File.Copy(editorAGS64Path, Path.Combine(gameLinuxDataDir, "ags64"), true);
            string gamePathName = Path.Combine(_editor.CurrentGame.DirectoryPath, Path.PathSeparator.ToString()); // make sure string ends with path separator so GetDirectoryName returns the correct path
            gamePathName = Path.GetDirectoryName(gamePathName); // strips the trailing path separator
            gamePathName = Path.GetFileName(gamePathName); // returns the name of the last directory in the path (e.g., the game directory)
            gamePathName = gamePathName.Replace(" ", ""); // strips whitespace
            FileStream script = File.Create(Path.Combine(gameLinuxDir, gamePathName));
            string scriptContents =
@"#!/bin/sh
SCRIPTPATH=""$(dirname ""$(readlink -f $0)"")""

if test ""x$@"" = ""x-h"" -o ""x$@"" = ""x--help""
  then
    echo ""Usage:"" ""$(basename ""$(readlink -f $0)"")"" ""[<ags options>]""
    echo """"
fi

if test $(uname -m) = x86_64
  then" + GetLibSymLinkScriptForEachPlugin(true) +
@"
    ALLEGRO_MODULES=""$SCRIPTPATH/data/lib64"" ""$SCRIPTPATH/data/ags64"" ""$@"" ""$SCRIPTPATH/data/""
  else" + GetLibSymLinkScriptForEachPlugin(false) +
@"
    ALLEGRO_MODULES=""$SCRIPTPATH/data/lib32"" ""$SCRIPTPATH/data/ags32"" ""$@"" ""$SCRIPTPATH/data/""
fi
";
            script.Write(Encoding.ASCII.GetBytes(scriptContents), 0, scriptContents.Length);
            script.Close();
        }
    }
}
