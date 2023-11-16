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

        private string GetScriptFileNameFromBasename(string basename)
        {
            return basename.Replace(" ", ""); // strip whitespace from script name
        }

        public override IDictionary<string, string> GetRequiredLibraryPaths()
        {
            Dictionary<string, string> paths = new Dictionary<string, string>();
            string[] libs =
            {
                "libSDL2-2.0.so.0",
                "libogg.so.0",
                "libtheora.so.0",
                "libvorbis.so.0",
                "libvorbisfile.so.3"
            };
            string[] licenses =
            {
                "ags-copyright",
                "libSDL2-2.0-copyright",
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

        public override void DeleteMainGameData(string name)
        {
            DeleteCommonGameFiles(Path.Combine(OutputDirectoryFullPath, LINUX_DATA_DIR), name);

            string script_filename = Path.Combine(OutputDirectoryFullPath, GetScriptFileNameFromBasename(name));
            Utilities.TryDeleteFile(script_filename);
        }

        private void CheckPluginsHaveSharedLibraries(CompileMessages errors)
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
                if(has32bit || has64bit)
                {
                    _plugins.Add(soName);
                    if (!has32bit) errors.Add(new CompileWarning("Linux: plugin " + soName + " is missing for 32-bit."));
                    if (!has64bit) errors.Add(new CompileWarning("Linux: plugin " + soName + " is missing for 64-bit."));
                }
                else
                {
                    errors.Add(new CompileWarning("Linux: plugin " + soName + " not found for any arch."));
                }                
            }
        }

        private string GetSymLinkScriptForEachPlugin(bool is64bit)
        {
            string results = "";
            string bit = (is64bit ? "64" : "32");
            foreach (string soName in _plugins)
            {
                results +=
@"
    ln -f -s ""$scriptdir/data/lib" + bit + @"/" + soName + @""" """ + soName + @"""";
            }
            results +=
@"
    ""$scriptdir/data/ags" + bit + @""" ""$@"" ""$scriptdir/data/""";
            return results;
        }

        public override bool Build(CompileMessages errors, bool forceRebuild)
        {
            if (!base.Build(errors, forceRebuild)) return false;
            CheckPluginsHaveSharedLibraries(errors);
            foreach (string fileName in Directory.GetFiles(Path.Combine(AGSEditor.OUTPUT_DIRECTORY, AGSEditor.DATA_OUTPUT_DIRECTORY)))
            {
                if ((File.GetAttributes(fileName) & (FileAttributes.Hidden | FileAttributes.System | FileAttributes.Temporary)) != 0)
                    continue;
                if ((!fileName.EndsWith(".dll", StringComparison.OrdinalIgnoreCase)) &&
                    (!Path.GetFileName(fileName).Equals("winsetup.exe", StringComparison.OrdinalIgnoreCase)) &&
                    (!Path.GetFileName(fileName).Equals(AGSEditor.CONFIG_FILE_NAME, StringComparison.OrdinalIgnoreCase)))
                {
                    Utilities.HardlinkOrCopy(GetCompiledPath(LINUX_DATA_DIR, Path.GetFileName(fileName)), fileName, true);
                }
            }
            // Update config file with current game parameters
            GenerateConfigFile(GetCompiledPath(LINUX_DATA_DIR));

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
                Utilities.HardlinkOrCopy(GetCompiledPath(LINUX_DATA_DIR, folderName, Path.GetFileName(fileName)),
                    fileName, true);
            }
            string linuxDataLib32Dir = GetCompiledPath(LINUX_DATA_DIR, LINUX_LIB32_DIR);
            string linuxDataLib64Dir = GetCompiledPath(LINUX_DATA_DIR, LINUX_LIB64_DIR);
            string editorLinuxDir = Path.Combine(Factory.AGSEditor.EditorDirectory, LINUX_DIR);
            string editorLinuxLib32Dir = Path.Combine(editorLinuxDir, LINUX_LIB32_DIR);
            string editorLinuxLib64Dir = Path.Combine(editorLinuxDir, LINUX_LIB64_DIR);
            foreach (string soName in _plugins)
            {
                string pathSoEditor32bit = Path.Combine(editorLinuxLib32Dir, soName);
                string pathSoEditor64bit = Path.Combine(editorLinuxLib64Dir, soName);
                string pathSoData32bit = Path.Combine(linuxDataLib32Dir, soName);
                string pathSoData64bit = Path.Combine(linuxDataLib64Dir, soName);
                if (File.Exists(pathSoEditor32bit))
                {
                    Utilities.HardlinkOrCopy(pathSoData32bit, pathSoEditor32bit, true);
                }
                if(File.Exists(pathSoEditor64bit))
                {
                    Utilities.HardlinkOrCopy(pathSoData64bit, pathSoEditor64bit, true);
                }
                
            }
            string scriptFileName = GetCompiledPath(GetScriptFileNameFromBasename(Factory.AGSEditor.BaseGameFileName)); 
            string scriptText =
@"#!/bin/sh
scriptpath=$(readlink -f ""$0"")
scriptdir=$(dirname ""$scriptpath"")

for arg; do
    if [ ""$arg"" = ""--help"" ]; then
        printf ""Usage: %s [<ags options>]\n\n"" ""$(basename ""$0"")""
        break
    fi
done

## Old versions of Mesa can hang when using DRI3
## https://bugs.freedesktop.org/show_bug.cgi?id=106404
export LIBGL_DRI3_DISABLE=true

if [ ""$(uname -m)"" = ""x86_64"" ]; then" + GetSymLinkScriptForEachPlugin(true) +
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
