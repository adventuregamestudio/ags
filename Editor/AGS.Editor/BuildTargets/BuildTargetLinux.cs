using AGS.Types;
using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    public class BuildTargetLinux : BuildTargetBase
    {
        public const string LINUX_DIR = "Linux";
        public const string LINUX_LIB32_DIR = "lib32";
        public const string LINUX_LIB64_DIR = "lib64";
        public const string LINUX_LICENSES_DIR = "licenses";
        public const string LINUX_DATA_DIR = "data";
        private IList<string> _plugins = new List<string>();

        public override IDictionary<string, string> GetRequiredLibraryPaths()
        {
            Dictionary<string, string> paths = new Dictionary<string, string>();
            string[] libs =
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
            string[] licenses =
            {
                "ags-copyright",
                "liballegro4.4-copyright",
                "libdumb1-copyright",
                "libfreetype6-copyright",
                "libogg0-copyright",
                "libtheora0-copyright",
                "libvorbis0a-copyright"
            };
            string linuxDir = Path.Combine(Factory.AGSEditor.EditorDirectory, LINUX_DIR);
            foreach (string lib in libs)
            {
                paths.Add(Path.Combine(LINUX_LIB32_DIR, lib), linuxDir);
                paths.Add(Path.Combine(LINUX_LIB64_DIR, lib), linuxDir);
            }
            foreach (string lic in licenses)
            {
                paths.Add(Path.Combine(LINUX_LICENSES_DIR, lic), linuxDir);
            }
            paths.Add("ags32", linuxDir);
            paths.Add("ags64", linuxDir);
            return paths;
        }

        public override string[] GetPlatformStandardSubfolders()
        {
            return new string[]
            {
                GetCompiledPath(LINUX_DATA_DIR, LINUX_LIB32_DIR),
                GetCompiledPath(LINUX_DATA_DIR, LINUX_LIB64_DIR),
                GetCompiledPath(LINUX_DATA_DIR, LINUX_LICENSES_DIR)
            };
        }

        private bool CheckPluginsHaveSharedLibraries()
        {
            _plugins.Clear();
            string linuxDir = Path.Combine(Factory.AGSEditor.EditorDirectory, LINUX_DIR);
            string lib32Dir = Path.Combine(linuxDir, LINUX_LIB32_DIR);
            string lib64Dir = Path.Combine(linuxDir, LINUX_LIB64_DIR);
            foreach (Plugin plugin in Factory.AGSEditor.CurrentGame.Plugins)
            {
                string soName = "lib" + plugin.FileName.Substring(0, plugin.FileName.Length - 3) + "so";
                bool has32bit = File.Exists(Path.Combine(lib32Dir, soName));
                bool has64bit = File.Exists(Path.Combine(lib64Dir, soName));
                if ((!has32bit) || (!has64bit))
                {
                    DialogResult ignore = MessageBox.Show("WARNING! The plugin '" + plugin.FileName +
                        "' does not have a Linux equivalent ('" + soName +
                        "' (" + (has32bit ? "" : "32-bit") + (has64bit ? "" : (has32bit ? ", " : "") + "64-bit") + ")" +
                        ") available. Your game may not run or function properly without it. Do you wish to continue building for Linux anyway?",
                        "Missing plugin for Linux",
                        MessageBoxButtons.YesNo,
                        MessageBoxIcon.Warning);
                    if (ignore == DialogResult.Yes) continue;
                    return false;
                }
                _plugins.Add(soName);
            }
            return true;
        }

        private string GetSymLinkScriptForEachPlugin(bool is64bit)
        {
            string results = "";
            string bit = (is64bit ? "64" : "32");
            foreach (string soName in _plugins)
            {
                results +=
@"
    ln -f -s ""$SCRIPTPATH/data/lib" + bit + @"/" + soName + @""" """ + soName + @"""";
            }
            results +=
@"
    ALLEGRO_MODULES=""$SCRIPTPATH/data/lib" + bit + @""" ""$SCRIPTPATH/data/ags" + bit + @""" ""$@"" ""$SCRIPTPATH/data/""";
            return results;
        }

        public override bool Build(CompileMessages errors, bool forceRebuild)
        {
            if (!base.Build(errors, forceRebuild)) return false;
            if (!CheckPluginsHaveSharedLibraries())
            {
                errors.Add(new CompileError("Could not build for Linux due to missing plugins."));
                return false;
            }
            foreach (string fileName in Directory.GetFiles(Factory.AGSEditor.CompiledDirectory))
            {
                if (Path.GetFileName(fileName).Equals(AGSEditor.CONFIG_FILE_NAME, StringComparison.OrdinalIgnoreCase))
                {
                    // don't hard-link config file
                    string acsetupPath = GetCompiledPath(LINUX_DATA_DIR, AGSEditor.CONFIG_FILE_NAME);
                    if (!File.Exists(acsetupPath)) File.Copy(fileName, acsetupPath);
                }
                else if ((!fileName.EndsWith(".dll", StringComparison.OrdinalIgnoreCase)) &&
                    (!Path.GetFileName(fileName).Equals("winsetup.exe", StringComparison.OrdinalIgnoreCase)))
                {
                    Utilities.CreateHardLink(GetCompiledPath(LINUX_DATA_DIR, Path.GetFileName(fileName)),
                        fileName, true);
                }
            }
            foreach (KeyValuePair<string, string> pair in GetRequiredLibraryPaths())
            {
                string fileName = pair.Value;
                if (!fileName.EndsWith(pair.Key)) fileName = Path.Combine(fileName, pair.Key);
                string folderName = null;
                if ((!fileName.EndsWith("ags32")) && (!fileName.EndsWith("ags64")))
                {
                    // the engine files belong in the LINUX_DATA_DIR, but the other libs
                    // should have lib32 or lib64 subdirectories as part of their name
                    folderName = Path.GetFileName(Path.GetDirectoryName(fileName).TrimEnd(Path.DirectorySeparatorChar));
                }
                Utilities.CreateHardLink(GetCompiledPath(LINUX_DATA_DIR, folderName, Path.GetFileName(fileName)),
                    fileName, true);
            }
            string linuxDataLib32Dir = GetCompiledPath(LINUX_DATA_DIR, LINUX_LIB32_DIR);
            string linuxDataLib64Dir = GetCompiledPath(LINUX_DATA_DIR, LINUX_LIB64_DIR);
            string editorLinuxDir = Path.Combine(Factory.AGSEditor.EditorDirectory, LINUX_DIR);
            string editorLinuxLib32Dir = Path.Combine(editorLinuxDir, LINUX_LIB32_DIR);
            string editorLinuxLib64Dir = Path.Combine(editorLinuxDir, LINUX_LIB64_DIR);
            foreach (string soName in _plugins)
            {
                Utilities.CreateHardLink(Path.Combine(linuxDataLib32Dir, soName),
                    Path.Combine(editorLinuxLib32Dir, soName), true);
                Utilities.CreateHardLink(Path.Combine(linuxDataLib64Dir, soName),
                    Path.Combine(editorLinuxLib64Dir, soName), true);
            }
            string scriptFileName = GetCompiledPath(Factory.AGSEditor.BaseGameFileName.Replace(" ", "")); // strip whitespace from script name
            string scriptText =
@"#!/bin/sh
SCRIPTPATH=""$(dirname ""$(readlink -f $0)"")""

if test ""x$@"" = ""x-h"" -o ""x$@"" = ""x--help""
  then
    echo ""Usage:"" ""$(basename ""$(readlink -f $0)"")"" ""[<ags options>]""
    echo """"
fi

if test $(uname -m) = x86_64
  then" + GetSymLinkScriptForEachPlugin(true) +
@"
  else" + GetSymLinkScriptForEachPlugin(false) +
@"
fi
";
            scriptText = scriptText.Replace("\r\n", "\n"); // make sure script has UNIX line endings
            FileStream stream = File.Create(scriptFileName);
            byte[] bytes = Encoding.UTF8.GetBytes(scriptText);
            stream.Write(bytes, 0, bytes.Length);
            stream.Close();
            Utilities.SetFileAccessAllowUsersToModify(scriptFileName);
            return true;
        }

        public override string Name
        {
            get
            {
                return "Linux";
            }
        }

        public override string OutputDirectory
        {
            get
            {
                return LINUX_DIR;
            }
        }
    }
}
