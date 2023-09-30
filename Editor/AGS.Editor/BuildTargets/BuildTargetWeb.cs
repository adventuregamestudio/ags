using AGS.Types;
using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    class BuildTargetWeb : BuildTargetBase
    {
        public const string WEB_DIR = "Web";
        public const string MY_GAME_FILES_JS = "my_game_files.js";

        public override IDictionary<string, string> GetRequiredLibraryPaths()
        {
            Dictionary<string, string> paths = new Dictionary<string, string>();

            string webDir = Path.Combine(Factory.AGSEditor.EditorDirectory, WEB_DIR);
       
            paths.Add("ags.js", webDir);
            paths.Add("ags.wasm", webDir);
            paths.Add("index.html", webDir);
            return paths;
        }

        public override string[] GetPlatformStandardSubfolders()
        {
            return new string[] { GetCompiledPath() };
        }

        public override void DeleteMainGameData(string name)
        {
            DeleteCommonGameFiles(OutputDirectoryFullPath, name);
        }

        public override bool Build(CompileMessages errors, bool forceRebuild)
        {
            if (!base.Build(errors, forceRebuild)) return false;
            string my_game_files_text = "var gamefiles = [";

            foreach (string fileName in Directory.GetFiles(Path.Combine(AGSEditor.OUTPUT_DIRECTORY, AGSEditor.DATA_OUTPUT_DIRECTORY)))
            {
                if ((File.GetAttributes(fileName) & (FileAttributes.Hidden | FileAttributes.System | FileAttributes.Temporary)) != 0)
                    continue;
                if ((!fileName.EndsWith(".dll", StringComparison.OrdinalIgnoreCase)) &&
                    (!Path.GetFileName(fileName).Equals("winsetup.exe", StringComparison.OrdinalIgnoreCase)) &&
                    (!Path.GetFileName(fileName).Equals(AGSEditor.CONFIG_FILE_NAME, StringComparison.OrdinalIgnoreCase)))
                {
                    Utilities.HardlinkOrCopy(GetCompiledPath(WEB_DIR, Path.GetFileName(fileName)), fileName, true);

                    my_game_files_text += "'" + Path.GetFileName(fileName) + "', ";
                }
            }
            my_game_files_text += "'" + AGSEditor.CONFIG_FILE_NAME + "'";

            my_game_files_text += "];";

            // Update config file with current game parameters
            GenerateConfigFile(GetCompiledPath(WEB_DIR));

            string compiled_web_path = GetCompiledPath(WEB_DIR);
            string editor_web_prebuilt = Path.Combine(Factory.AGSEditor.EditorDirectory, WEB_DIR);

            foreach (KeyValuePair<string, string> pair in GetRequiredLibraryPaths())
            {
                string fileName = pair.Key;
                string originDir = pair.Value;

                string destFile = GetCompiledPath(WEB_DIR, fileName);
                string originFile = Path.Combine(originDir, fileName);

                if (fileName.EndsWith(".wasm"))
                {
                    Utilities.HardlinkOrCopy(destFile, originFile, true);
                }
                else
                {
                    string destFileName = Utilities.ResolveSourcePath(destFile);
                    string sourceFileName = Utilities.ResolveSourcePath(originFile);
                    File.Copy(sourceFileName, destFileName, true);
                }
            }

            FileStream stream = File.Create(GetCompiledPath(WEB_DIR, MY_GAME_FILES_JS));
            byte[] bytes = Encoding.UTF8.GetBytes(my_game_files_text);
            stream.Write(bytes, 0, bytes.Length);
            stream.Close();

            return true;
        }

        public override string Name
        {
            get
            {
                return "Web";
            }
        }

        public override string OutputDirectory
        {
            get
            {
                return WEB_DIR;
            }
        }
    }
}
