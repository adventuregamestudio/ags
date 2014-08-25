using AGS.Types;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Text;

namespace AGS.Editor
{
    public class BuildTarget
    {
        private static TargetsIndexer _targets;
        private static IList<BuildTarget> _allTargets;
        private BuildTargetPlatform _platform;

        static BuildTarget()
        {
            _targets = new TargetsIndexer();
            _allTargets = new List<BuildTarget>(Enum.GetNames(typeof(BuildTargetPlatform)).Length);
            _allTargets[(int)BuildTargetPlatform.DataFileOnly] = new BuildTarget(BuildTargetPlatform.DataFileOnly);
            _allTargets[(int)BuildTargetPlatform.Windows] = new BuildTarget(BuildTargetPlatform.Windows);
            _allTargets[(int)BuildTargetPlatform.Linux] = new BuildTarget(BuildTargetPlatform.Linux);
        }

        private BuildTarget(BuildTargetPlatform platform)
        {
            _platform = platform;
        }

        /// <summary>
        /// Returns a read-only list of all known targets.
        /// </summary>
        public static IList<BuildTarget> AllTargets
        {
            get
            {
                return new ReadOnlyCollection<BuildTarget>(_allTargets);
            }
        }

        public class TargetsIndexer
        {
            public BuildTarget this[BuildTargetPlatform platform]
            {
                get
                {
                    return FindTargetByPlatform(platform);
                }
            }
        }

        /// <summary>
        /// Provides an indexer that takes a BuildTargetPlatform as an index.
        /// Returns the BuildTarget represented by the specified platform.
        /// </summary>
        public static TargetsIndexer Targets
        {
            get
            {
                return _targets;
            }
        }

        /// <summary>
        /// Returns target platform by its name.
        /// </summary>
        public static BuildTarget FindTargetByName(string name)
        {
            if ((string.IsNullOrEmpty(name)) || (_allTargets == null)) return null;
            BuildTargetPlatform? p = null;
            try
            {
                p = (BuildTargetPlatform)Enum.Parse(typeof(BuildTargetPlatform), name, true);
                return _allTargets[(int)p];
            }
            catch
            {
                return null;
            }
        }

        /// <summary>
        /// Returns target by its platform.
        /// </summary>
        public static BuildTarget FindTargetByPlatform(BuildTargetPlatform platform)
        {
            if (_allTargets == null) return null;
            try
            {
                return _allTargets[(int)platform];
            }
            catch
            {
                return null;
            }
        }

        /// <summary>
        /// Returns whether specified platform is available for building.
        /// </summary>
        public static bool IsBuildTargetAvailable(BuildTargetPlatform platform)
        {
            return BuildTargetInfo.IsBuildTargetAvailable(platform);
        }

        /// <summary>
        /// Returns the target platforms selected for building, or null if no game is loaded yet.
        /// </summary>
        public static BuildTargetPlatform? TargetedPlatforms
        {
            get
            {
                return (AGSEditor.Instance.CurrentGame == null ? (BuildTargetPlatform?)null : AGSEditor.Instance.CurrentGame.Settings.BuildTargets);
            }
        }

        /// <summary>
        /// Returns whether target platform is available for building.
        /// </summary>
        public bool Available
        {
            get
            {
                return IsBuildTargetAvailable(Platform);
            }
        }

        /// <summary>
        /// Returns whether the target is currently selected for building.
        /// </summary>
        public bool IsTargetedForBuild
        {
            get
            {
                return (TargetedPlatforms == BuildTargetPlatform.DataFileOnly ? true :
                    TargetedPlatforms == null ? false : (TargetedPlatforms & Platform) != 0);
            }
        }

        /// <summary>
        /// Returns whether this is the only selected target for building other than the data file.
        /// </summary>
        public bool IsOnlyBuildTarget
        {
            get
            {
                return (TargetedPlatforms == Platform);
            }
        }

        /// <summary>
        /// Returns the name of the target platform.
        /// </summary>
        public string Name
        {
            get
            {
                return Platform.ToString();
            }
        }

        /// <summary>
        /// Returns the full path of this target's output directory.
        /// </summary>
        public string OutputDirectory
        {
            get
            {
                return BuildTargetInfo.GetCompiledPath(Platform, IsOnlyBuildTarget);
            }
        }

        /// <summary>
        /// Returns the folder where the target outputs its files when built.
        /// </summary>
        public string OutputDirectoryFullPath
        {
            get
            {
                return Utilities.GetFullPathFromProjectRelative(OutputDirectory);
            }
        }

        /// <summary>
        /// Returns required libraries and their paths.
        /// </summary>
        public Dictionary<string, string> RequiredLibraryPaths
        {
            get
            {
                return BuildTargetInfo.GetRequiredLibraryPaths(Platform);
            }
        }

        /// <summary>
        /// Returns the platform this target builds for.
        /// </summary>
        public BuildTargetPlatform Platform
        {
            get
            {
                return _platform;
            }
        }

        /// <summary>
        /// If available, builds the target.
        /// </summary>
        public void Build()
        {
            if (!Available) return;
            EnsureStandardSubfoldersExist();
        }

        /// <summary>
        /// Ensures that standard folders exist for this target.
        /// </summary>
        public void EnsureStandardSubfoldersExist()
        {
            foreach (string subfolder in BuildTargetInfo.GetPlatformStandardSubfolders(Platform, IsOnlyBuildTarget))
            {
                Directory.CreateDirectory(Utilities.GetFullPathFromProjectRelative(subfolder)); // automatically checks if directories exist
            }
        }

        /// <summary>
        /// Returns a list of required files to build this target.
        /// </summary>
        public string[] GetRequiredLibraryNames()
        {
            return new List<string>(RequiredLibraryPaths.Keys).ToArray();
        }
    }
}
