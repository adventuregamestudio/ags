using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace AGS.Types
{
    public class BuildTargetInfo
    {
        private static IDictionary<BuildTargetPlatform, IBuildTarget> _buildTargets;

        static BuildTargetInfo()
        {
            _buildTargets = new Dictionary<BuildTargetPlatform, IBuildTarget>(Enum.GetNames(typeof(BuildTargetPlatform)).Length);
        }

        public static void RegisterBuildTarget(IBuildTarget target)
        {
            if (_buildTargets.Values.Contains(target)) return;
            if (_buildTargets.Keys.Contains(target.Platform)) return;
            _buildTargets.Add(target.Platform, target);
        }

        public static IList<IBuildTarget> GetRegisteredBuildTargets()
        {
            return new List<IBuildTarget>(_buildTargets.Values);
        }

        public static IList<IBuildTarget> GetSelectedBuildTargets()
        {
            List<IBuildTarget> targets = new List<IBuildTarget>();
            foreach (BuildTargetPlatform platform in _buildTargets.Keys)
            {
                if (_buildTargets[platform].IsTargetedForBuild)
                {
                    targets.Add(_buildTargets[platform]);
                }
            }
            return targets;
        }

        public static IBuildTarget FindBuildTargetByName(string name)
        {
            if ((string.IsNullOrEmpty(name)) || (_buildTargets == null)) return null;
            try
            {
                BuildTargetPlatform p = (BuildTargetPlatform)Enum.Parse(typeof(BuildTargetPlatform), name, true);
                return _buildTargets[p];
            }
            catch
            {
                return null;
            }
        }

        public static IBuildTarget FindBuildTargetByPlatform(BuildTargetPlatform platform)
        {
            if (_buildTargets.Keys.Contains(platform)) return _buildTargets[platform];
            return null;
        }

        /// <summary>
        /// Helper function for FlagsUIEditorControl.ValueExclusionCheck delegate.
        /// </summary>
        public static bool IsBuildTargetAvailable(Enum e)
        {
            if (e.GetType() != typeof(BuildTargetPlatform)) return false;
            return IsBuildTargetAvailable((BuildTargetPlatform)e);
        }

        /// <summary>
        /// Returns whether all files are available to build targeting the specified platform.
        /// </summary>
        public static bool IsBuildTargetAvailable(BuildTargetPlatform platform)
        {
            IBuildTarget target = FindBuildTargetByPlatform(platform);
            if (target == null) return false;
            IDictionary<string, string> paths = target.GetRequiredLibraryPaths();
            if (paths == null) return false;
            foreach (KeyValuePair<string, string> pair in paths)
            {
                string fullPath = pair.Value;
                if (!fullPath.EndsWith(pair.Key)) fullPath = Path.Combine(fullPath, pair.Key);
                if (!File.Exists(fullPath)) return false;
            }
            return true;
        }
    }
}
