using AGS.Types;
using System;
using System.IO;
using System.Security.AccessControl;
using System.Security.Principal;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor.Components
{
    class BuildLinuxComponent : BaseComponent
    {
        private AGSEditor editor;
        private string[] libs;
        private string editorLinuxDir = null;
        private string editorLinuxAGS32Path = null;
        private string editorLinuxAGS64Path = null;
        private string editorLinuxLib32Dir = null;
        private string editorLinuxLib64Dir = null;
        private string editorLinuxLicensesDir = null;
        private string gameCompiledDir = null;
        private string gameLinuxDir = null;
        private string gameLinuxDataDir = null;
        private string gameLinuxDataAGS32Path = null;
        private string gameLinuxDataAGS64Path = null;
        private string gameLinuxDataLib32Dir = null;
        private string gameLinuxDataLib64Dir = null;
        private string gameLinuxDataLicensesDir = null;
        private static BuildLinuxComponent instance;

        public static BuildLinuxComponent Instance
        {
            get { return instance; }
        }

        public string LinuxOutputDirectory
        {
            get { return gameLinuxDir; }
        }

        public string LinuxDataDirectory
        {
            get { return gameLinuxDataDir; }
        }

        public BuildLinuxComponent(GUIController guiController, AGSEditor agsEditor) : base(guiController, agsEditor)
        {
            instance = this;
            editor = agsEditor;
            libs = new string[0];
            //libs = Targets.GetPlatformRequiredLibraryNames(Targets.Platforms.Linux);
        }

        void InitPaths()
        {
            if (editorLinuxDir != null) return; // paths already set
            editorLinuxDir = Path.Combine(editor.EditorDirectory, "linux");
            editorLinuxAGS32Path = Path.Combine(editorLinuxDir, "ags32");
            editorLinuxAGS64Path = Path.Combine(editorLinuxDir, "ags64");
            editorLinuxLib32Dir = Path.Combine(editorLinuxDir, "lib32");
            editorLinuxLib64Dir = Path.Combine(editorLinuxDir, "lib64");
            editorLinuxLicensesDir = Path.Combine(editorLinuxDir, "licenses");
            gameCompiledDir = Path.Combine(editor.CurrentGame.DirectoryPath, "Compiled");
            gameLinuxDir = Path.Combine(gameCompiledDir, "Linux");
            gameLinuxDataDir = Path.Combine(gameLinuxDir, "data");
            gameLinuxDataAGS32Path = Path.Combine(gameLinuxDataDir, "ags32");
            gameLinuxDataAGS64Path = Path.Combine(gameLinuxDataDir, "ags64");
            gameLinuxDataLib32Dir = Path.Combine(gameLinuxDataDir, "lib32");
            gameLinuxDataLib64Dir = Path.Combine(gameLinuxDataDir, "lib64");
            gameLinuxDataLicensesDir = Path.Combine(gameLinuxDataDir, "licenses");
        }

        public void BuildForLinux()
        {
            InitPaths();
            if (EnsureLibsExist().RaiseWarning() != DialogResult.Yes) return;
            if (EnsurePluginsExist().RaiseWarning() != DialogResult.Yes) return;
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

            public DialogResult RaiseWarning()
            {
                if (this == None) return DialogResult.Yes;
                return MessageBox.Show(
                    "Could not find " + Which.ToString().ToLower() + " '" + Name + "' (" + (Architecture == ArchType.Arch32 ? "32" : "64") + "-bit). Your game may not run properly without this file. Do you wish to continue building for Linux without it?",
                    "Warning!", MessageBoxButtons.YesNo, MessageBoxIcon.Warning);
            }

            public static readonly MissingLibOrPlugin None = new MissingLibOrPlugin(ArchType.Unknown, LibOrPlugin.Unknown, string.Empty);
        };

        private MissingLibOrPlugin EnsureLibsExist()
        {
            foreach (string lib in libs)
            {
                if (!File.Exists(Path.Combine(editorLinuxLib32Dir, lib)))
                    return new MissingLibOrPlugin(MissingLibOrPlugin.ArchType.Arch32, MissingLibOrPlugin.LibOrPlugin.Lib, lib);
                if (!File.Exists(Path.Combine(editorLinuxLib64Dir, lib)))
                    return new MissingLibOrPlugin(MissingLibOrPlugin.ArchType.Arch64, MissingLibOrPlugin.LibOrPlugin.Lib, lib);
            }
            if (!File.Exists(editorLinuxAGS32Path))
                return new MissingLibOrPlugin(MissingLibOrPlugin.ArchType.Arch32, MissingLibOrPlugin.LibOrPlugin.Engine, "ags32");
            if (!File.Exists(editorLinuxAGS64Path))
                return new MissingLibOrPlugin(MissingLibOrPlugin.ArchType.Arch64, MissingLibOrPlugin.LibOrPlugin.Engine, "ags64");
            return MissingLibOrPlugin.None;
        }

        private string GetPluginSOName(Plugin plugin)
        {
            return "lib" + plugin.FileName.Substring(0, plugin.FileName.Length - 3) + "so";
        }

        private MissingLibOrPlugin EnsurePluginsExist()
        {
            foreach (Plugin plugin in editor.CurrentGame.Plugins)
            {
                string soName = GetPluginSOName(plugin);
                if (!File.Exists(Path.Combine(editorLinuxLib32Dir, soName)))
                    return new MissingLibOrPlugin(MissingLibOrPlugin.ArchType.Arch32, MissingLibOrPlugin.LibOrPlugin.Plugin, soName);
                if (!File.Exists(Path.Combine(editorLinuxLib64Dir, soName)))
                    return new MissingLibOrPlugin(MissingLibOrPlugin.ArchType.Arch64, MissingLibOrPlugin.LibOrPlugin.Plugin, soName);
            }
            return MissingLibOrPlugin.None;
        }

        private string GetLibSymLinkScriptForEachPlugin(bool is64)
        {
            string results = "";
            string bit = (is64 ? "64" : "32");
            foreach (Plugin plugin in editor.CurrentGame.Plugins)
            {
                string soName = GetPluginSOName(plugin);
                results +=
@"
    ln -f -s ""$SCRIPTPATH/data/lib" + bit + @"/" + soName + @""" """ + soName + @"""";
            }
            results +=
@"
    ALLEGRO_MODULES=""$SCRIPTPATH/data/lib" + bit + @""" ""$SCRIPTPATH/data/ags" + bit + @""" ""$@"" ""$SCRIPTPATH/data/""";
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
                    foreach (Plugin plugin in editor.CurrentGame.Plugins)
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

        private void SetFilePermissions(string fileName)
        {
            FileSecurity fsec = File.GetAccessControl(fileName);
            fsec.AddAccessRule
            (
                new FileSystemAccessRule
                (
                    new SecurityIdentifier
                    (
                        WellKnownSidType.BuiltinUsersSid,
                        null
                    ),
                    FileSystemRights.Modify,
                    AccessControlType.Allow
                )
            );
            File.SetAccessControl(fileName, fsec);
        }

        private void CopyFilesFromCompiledDir()
        {
            if (!Directory.Exists(gameLinuxDir)) Directory.CreateDirectory(gameLinuxDir);
            if (!Directory.Exists(gameLinuxDataDir)) Directory.CreateDirectory(gameLinuxDataDir);
            if (!Directory.Exists(gameLinuxDataLib32Dir)) Directory.CreateDirectory(gameLinuxDataLib32Dir);
            if (!Directory.Exists(gameLinuxDataLib64Dir)) Directory.CreateDirectory(gameLinuxDataLib64Dir);
            if (!Directory.Exists(gameLinuxDataLicensesDir)) Directory.CreateDirectory(gameLinuxDataLicensesDir);
            string[] compiledFiles = Directory.GetFiles(gameCompiledDir);
            foreach (string file in compiledFiles)
            {
                if (Path.GetFileName(file).Equals("acsetup.cfg", StringComparison.OrdinalIgnoreCase))
                {
                    string acsetupPath = Path.Combine(gameLinuxDataDir, "acsetup.cfg");
                    if (!File.Exists(acsetupPath)) File.Copy(file, acsetupPath);
                }
                else if ((!file.EndsWith(".dll", StringComparison.OrdinalIgnoreCase)) &&
                    (!Path.GetFileName(file).Equals("winsetup.exe", StringComparison.OrdinalIgnoreCase)))
                {
                    Utilities.CreateHardLink(Path.Combine(gameLinuxDataDir, Path.GetFileName(file)), file, true);
                }
            }
            CopyFilesFromDir(editorLinuxLib32Dir, gameLinuxDataLib32Dir);
            CopyFilesFromDir(editorLinuxLib64Dir, gameLinuxDataLib64Dir);
            CopyFilesFromDir(editorLinuxLicensesDir, gameLinuxDataLicensesDir);
            File.Copy(editorLinuxAGS32Path, gameLinuxDataAGS32Path, true);
            SetFilePermissions(gameLinuxDataAGS32Path);
            File.Copy(editorLinuxAGS64Path, gameLinuxDataAGS64Path, true);
            SetFilePermissions(gameLinuxDataAGS64Path);
            string gamePathName = editor.BaseGameFileName.Replace(" ", ""); // strip whitespace
            string script = Path.Combine(gameLinuxDir, gamePathName);
            FileStream stream = File.Create(script);
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
  else" + GetLibSymLinkScriptForEachPlugin(false) +
@"
fi
";
            scriptContents = scriptContents.Replace("\r\n", "\n"); // make sure script has UNIX line endings
            byte[] bytes = Encoding.UTF8.GetBytes(scriptContents);
            stream.Write(bytes, 0, bytes.Length);
            stream.Close();
            SetFilePermissions(script);
        }
    }
}
