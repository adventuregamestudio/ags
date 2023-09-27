using AGS.Types;
using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace AGS.Editor
{
    public abstract class BuildTargetBase : IBuildTarget
    {
        private string missingFile;

        public abstract IDictionary<string, string> GetRequiredLibraryPaths();
        public abstract string[] GetPlatformStandardSubfolders();

        public virtual bool Build(CompileMessages errors, bool forceRebuild)
        {
            if (!IsAvailable)
            {
                errors.Add(new CompileError("Could not build target platform " + Name + ", because the file '" +
                    missingFile + "' is unavailable."));
                return false;
            }
            EnsureStandardSubfoldersExist();
            return true;
        }

        public virtual string GetCompiledPath(params string[] parts)
        {
            StringBuilder sb = new StringBuilder(Path.Combine(AGSEditor.OUTPUT_DIRECTORY, OutputDirectory));
            if (parts.Length == 0) return sb.ToString();
            int i = 0;
            if (parts[0] == AGSEditor.OUTPUT_DIRECTORY)
            {
                if (parts[1] == OutputDirectory) i++;
                i++;
            }
            else if (parts[0] == OutputDirectory) i++;
            for (; i < parts.Length; ++i)
            {
                if (!string.IsNullOrEmpty(parts[i])) sb.Append(Path.DirectorySeparatorChar + parts[i]);
            }
            return sb.ToString();
        }

        public virtual string[] GetRequiredLibraryNames()
        {
            return new List<string>(GetRequiredLibraryPaths().Keys).ToArray();
        }

        public virtual void EnsureStandardSubfoldersExist()
        {
            foreach (string subfolder in GetPlatformStandardSubfolders())
            {
                Directory.CreateDirectory(Path.Combine(Factory.AGSEditor.CurrentGame.DirectoryPath, subfolder)); // automatically checks if directories exist
            }
        }

        public virtual void DeleteMainGameData(string name)
        {
        }

        public virtual bool IsAvailable
        {
            get
            {
                missingFile = null; // checking IsAvailable will also update the missing file for error messages
                IDictionary<string, string> paths = GetRequiredLibraryPaths();
                if (paths == null) return false;
                foreach (KeyValuePair<string, string> pair in paths)
                {
                    string fullPath = pair.Value;
                    if (!fullPath.EndsWith(pair.Key)) fullPath = Path.Combine(fullPath, pair.Key);
                    if (!File.Exists(fullPath))
                    {
                        missingFile = pair.Key;
                        return false;
                    }
                }
                return true;
            }
        }

        public virtual bool IsTargetedForBuild
        {
            get
            {
                List<string> targets = new List<string>(
                    Factory.AGSEditor.CurrentGame.Settings.BuildTargets.Split(StringListUIEditor.Separators,
                    StringSplitOptions.RemoveEmptyEntries));
                return targets.Contains(Name);
            }
        }

        public virtual bool IsOnlyBuildTarget
        {
            get
            {
                return (Factory.AGSEditor.CurrentGame.Settings.BuildTargets == Name);
            }
        }

        public virtual bool IsNormalBuildTarget
        {
            get
            {
                return true;
            }
        }

        public abstract string Name
        {
            get;
        }

        public abstract string OutputDirectory
        {
            get;
        }

        public virtual string OutputDirectoryFullPath
        {
            get
            {
                return Path.Combine(Path.Combine(Factory.AGSEditor.CurrentGame.DirectoryPath, AGSEditor.OUTPUT_DIRECTORY), OutputDirectory);
            }
        }

        /// <summary>
        /// Generates a common config file in two steps:
        /// 1) copies a custom config from the project root, if one was provided by user;
        /// 2) writes over DefaultSetup, overwriting any matching entries, but not removing other contents.
        /// </summary>
        protected void GenerateConfigFile(string destDir)
        {
            string gameDir = AGSEditor.Instance.GameDirectory;
            destDir = Path.Combine(gameDir, destDir); // resolve to the absolute path for Uri check
            string customCfg = Path.Combine(gameDir, AGSEditor.CONFIG_FILE_NAME);
            string destPath = Path.Combine(destDir, AGSEditor.CONFIG_FILE_NAME);
            if (!Utilities.PathsAreEqual(destDir, gameDir))
            {
                try
                {
                    if (File.Exists(customCfg))
                        File.Copy(customCfg, destPath, true);
                    else
                        Utilities.TryDeleteFile(destPath);
                }
                catch (Exception) { }
            }
            AGSEditor.Instance.WriteConfigFile(destPath, false);
        }

        /// <summary>
        /// Deletes all the common game's data files:
        ///  * primary: <name>.ags,
        ///  * split resources: <name>.001, <name>.002, etc.
        /// </summary>
        protected void DeleteCommonGameFiles(string dir, string gamename)
        {
            string filename = Path.Combine(dir, gamename + ".ags");
            Utilities.TryDeleteFile(filename);

            // Delete split resources (if any)
            foreach (string fileName in Utilities.GetDirectoryFileList(dir, gamename + ".0*"))
            {
                Utilities.TryDeleteFile(fileName);
            }
        }
    }
}
