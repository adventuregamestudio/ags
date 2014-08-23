using AGS.Types;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Text;

namespace AGS.Editor
{
    public abstract class BuildTarget
    {
        private static IList<BuildTarget> _targets = null;

        static BuildTarget()
        {
            _targets = new List<BuildTarget>(sizeof(BuildTargetPlatform));
            _targets[(int)BuildTargetPlatform.DataFileOnly] = new BuildTargetDataFileOnly();
            _targets[(int)BuildTargetPlatform.Windows] = new BuildTargetWindows();
            _targets[(int)BuildTargetPlatform.Linux] = new BuildTargetLinux();
        }

        /// <summary>
        /// Returns a read-only list of all known targets.
        /// </summary>
        public static IList<BuildTarget> Targets
        {
            get
            {
                return new ReadOnlyCollection<BuildTarget>(_targets);
            }
        }

        /// <summary>
        /// Returns target platform by its name.
        /// </summary>
        public static BuildTarget FindTargetByName(string name)
        {
            if ((string.IsNullOrEmpty(name)) || (_targets == null)) return null;
            BuildTargetPlatform? p = null;
            try
            {
                p = (BuildTargetPlatform)Enum.Parse(typeof(BuildTargetPlatform), name, true);
                return _targets[(int)p];
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
            if (_targets == null) return null;
            try
            {
                return _targets[(int)platform];
            }
            catch
            {
                return null;
            }
        }

        public static bool IsBuildTargetAvailable(BuildTargetPlatform platform)
        {
            BuildTarget target = FindTargetByPlatform(platform);
            return (target == null ? false : target.Available);
        }

        /// <summary>
        /// Returns whether target platform is available for building.
        /// </summary>
        public virtual bool Available
        {
            get
            {
                return BuildTargetInfo.IsBuildTargetAvailable(Platform);
            }
        }

        /// <summary>
        /// Returns whether the target is currently selected for building.
        /// </summary>
        public abstract bool IsTargetForBuild
        {
            get;
        }

        /// <summary>
        /// Returns the name of the target platform.
        /// </summary>
        public virtual string Name
        {
            get
            {
                return Platform.ToString();
            }
        }

        /// <summary>
        /// Returns the full path of this target's output directory.
        /// </summary>
        public abstract string OutputDirectory
        {
            get;
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
        public abstract BuildTargetPlatform Platform
        {
            get;
        }

        /// <summary>
        /// If available, builds the target.
        /// </summary>
        public virtual void Build()
        {
            if (!Available) return;
            EnsureStandardSubfoldersExist();
        }

        /// <summary>
        /// Ensures that standard folders exist for this target.
        /// </summary>
        public abstract void EnsureStandardSubfoldersExist();

        /// <summary>
        /// Returns a list of required files to build this target.
        /// </summary>
        public string[] GetRequiredLibraryNames()
        {
            return new List<string>(RequiredLibraryPaths.Keys).ToArray();
        }
    }
}
