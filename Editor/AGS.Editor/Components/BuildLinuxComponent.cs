using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace AGS.Editor.Components
{
    class BuildLinuxComponent : BaseComponent
    {
        private AGSEditor _editor;
        private string[] libs;
        private string lib32Dir;
        private string lib64Dir;
        private string linuxDir;
        private string linuxDataDir;
        private string linuxDataLib32Dir;
        private string linuxDataLib64Dir;
        private string compiledDir;
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
            lib32Dir = Path.Combine(_editor.EditorDirectory, "lib32");
            lib64Dir = Path.Combine(_editor.EditorDirectory, "lib64");
            linuxDir = Path.Combine(_editor.CurrentGame.DirectoryPath, "Linux");
            linuxDataDir = Path.Combine(linuxDir, "data");
            linuxDataLib32Dir = Path.Combine(linuxDataDir, "lib32");
            linuxDataLib64Dir = Path.Combine(linuxDataDir, "lib64");
            compiledDir = Path.Combine(_editor.CurrentGame.DirectoryPath, "Compiled");
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
                if (!File.Exists(Path.Combine(lib32Dir, lib)))
                    return new MissingLibOrPlugin(MissingLibOrPlugin.ArchType.Arch32, MissingLibOrPlugin.LibOrPlugin.Lib, lib);
                if (!File.Exists(Path.Combine(lib64Dir, lib)))
                    return new MissingLibOrPlugin(MissingLibOrPlugin.ArchType.Arch64, MissingLibOrPlugin.LibOrPlugin.Lib, lib);
            }
            if (!File.Exists(Path.Combine(_editor.EditorDirectory, "ags32")))
                return new MissingLibOrPlugin(MissingLibOrPlugin.ArchType.Arch32, MissingLibOrPlugin.LibOrPlugin.Engine, "ags32");
            if (!File.Exists(Path.Combine(_editor.EditorDirectory, "ags64")))
                return new MissingLibOrPlugin(MissingLibOrPlugin.ArchType.Arch64, MissingLibOrPlugin.LibOrPlugin.Engine, "ags64");
            return MissingLibOrPlugin.Empty;
        }

        private MissingLibOrPlugin EnsurePluginsExist()
        {
            foreach (AGS.Types.Plugin plugin in _editor.CurrentGame.Plugins)
            {
                string soName = "lib" + plugin.FileName.Substring(0, plugin.FileName.Length - 3) + "so";
                if (!File.Exists(Path.Combine(lib32Dir, soName)))
                    return new MissingLibOrPlugin(MissingLibOrPlugin.ArchType.Arch32, MissingLibOrPlugin.LibOrPlugin.Plugin, soName);
                if (!File.Exists(Path.Combine(lib64Dir, soName)))
                    return new MissingLibOrPlugin(MissingLibOrPlugin.ArchType.Arch64, MissingLibOrPlugin.LibOrPlugin.Plugin, soName);
            }
            return MissingLibOrPlugin.Empty;
        }

        private string GetUnlinkScriptForEachPlugin()
        {
            string results = "";
            foreach (AGS.Types.Plugin plugin in _editor.CurrentGame.Plugins)
            {
                string pluginName = "lib" + plugin.FileName.Substring(0, plugin.FileName.Length - 3) + "so";
                results +=
@"
if [ -f """ + pluginName + @""" ]
  then
    rm """ + pluginName + @"""
fi
";
            }
            return results;
        }

        private string GetLibSymLinkScriptForEachPlugin(bool is64)
        {
            string results = "";
            foreach (AGS.Types.Plugin plugin in _editor.CurrentGame.Plugins)
            {
                string pluginName = "lib" + plugin.FileName.Substring(0, plugin.FileName.Length - 3) + "so";
                results +=
@"
    ln -s ""$SCRIPTPATH/data/lib" + (is64 ? "64" : "32") + @"/" + pluginName + @""" """ + pluginName + @"""";
            }
            return results;
        }

        private void CopyFilesFromCompiledDir()
        {
            if (!Directory.Exists(linuxDir)) Directory.CreateDirectory(linuxDir);
            if (!Directory.Exists(linuxDataDir)) Directory.CreateDirectory(linuxDataDir);
            if (!Directory.Exists(linuxDataLib32Dir)) Directory.CreateDirectory(linuxDataLib32Dir);
            if (!Directory.Exists(linuxDataLib64Dir)) Directory.CreateDirectory(linuxDataLib64Dir);
            string[] compiledFiles = Directory.GetFiles(compiledDir);
            foreach (string file in compiledFiles)
            {
                if ((!file.EndsWith(".dll", StringComparison.OrdinalIgnoreCase)) && (!file.Equals("winsetup.exe", StringComparison.OrdinalIgnoreCase))) File.Copy(file, Path.Combine(linuxDataDir, Path.GetFileName(file)), true);
            }
            foreach (string file in Directory.GetFiles(lib32Dir))
            {
                string fileName = Path.GetFileName(file);
                if (fileName.StartsWith("libags", StringComparison.OrdinalIgnoreCase))
                {
                    bool found = false;
                    foreach (AGS.Types.Plugin plugin in _editor.CurrentGame.Plugins)
                    {
                        string pluginFileName = "lib" + plugin.FileName.Substring(0, plugin.FileName.Length - 3) + "so";
                        if (pluginFileName.Equals(fileName, StringComparison.OrdinalIgnoreCase))
                        {
                            found = true;
                            break;
                        }
                    }
                    if (!found) continue;
                }
                File.Copy(file, Path.Combine(linuxDataLib32Dir, fileName), true);
            }
            foreach (string file in Directory.GetFiles(lib64Dir))
            {
                string fileName = Path.GetFileName(file);
                if (fileName.StartsWith("libags", StringComparison.OrdinalIgnoreCase))
                {
                    bool found = false;
                    foreach (AGS.Types.Plugin plugin in _editor.CurrentGame.Plugins)
                    {
                        string pluginFileName = "lib" + plugin.FileName.Substring(0, plugin.FileName.Length - 3) + "so";
                        if (pluginFileName.Equals(fileName, StringComparison.OrdinalIgnoreCase))
                        {
                            found = true;
                            break;
                        }
                    }
                    if (!found) continue;
                }
                File.Copy(file, Path.Combine(linuxDataLib64Dir, fileName), true);
            }
            File.Copy(Path.Combine(_editor.EditorDirectory, "ags32"), Path.Combine(linuxDataDir, "ags32"), true);
            File.Copy(Path.Combine(_editor.EditorDirectory, "ags64"), Path.Combine(linuxDataDir, "ags64"), true);
            string gamePathName = Path.Combine(_editor.CurrentGame.DirectoryPath, Path.PathSeparator.ToString());
            gamePathName = Path.GetDirectoryName(gamePathName);
            gamePathName = Path.GetFileName(gamePathName);
            gamePathName = gamePathName.Replace(" ", "");
            FileStream script = File.Create(Path.Combine(linuxDir, gamePathName));
            string scriptContents =
@"#!/bin/sh
SCRIPTPATH=""$(dirname ""$(readlink -f $0)"")""

if test ""x$@"" = ""x-h"" -o ""x$@"" = ""x--help""
  then
    echo ""Usage:"" ""$(basename ""$(readlink -f $0)"")"" ""[<ags options>]""
    echo """"
fi
" + GetUnlinkScriptForEachPlugin() +
@"
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
