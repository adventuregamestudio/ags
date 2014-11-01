using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Text;

namespace AGS.Types
{
    public class BuildTargetsInfo
    {
        private static IList<IBuildTarget> _buildTargets;

        static BuildTargetsInfo()
        {
            _buildTargets = new List<IBuildTarget>();
        }

        /// <summary>
        /// Includes an IBuildTarget in the list of potential target build platforms.
        /// </summary>
        public static void RegisterBuildTarget(IBuildTarget target)
        {
            if (target == null) throw new ArgumentNullException("Build targets cannot be null!");
            if (string.IsNullOrEmpty(target.Name)) throw new ArgumentException("Build target names cannot be null or empty!");
            foreach (IBuildTarget bt in _buildTargets)
            {
                if (bt == target) return;   // build target already registered, ignore it
                if (bt.Name == target.Name) // otherwise, if two targets share the same name
                {
                    throw new ArgumentException("Another build target with the same name ('" + target.Name + "') already exists.");
                }
            }
            _buildTargets.Add(target);
        }

        /// <summary>
        /// Returns a read-only list of the IBuildTargets that are currently registered,
        /// regardless of whether they are selected or available for building.
        /// </summary>
        public static IList<IBuildTarget> GetRegisteredBuildTargets()
        {
            return new ReadOnlyCollection<IBuildTarget>(_buildTargets);
        }

        /// <summary>
        /// Returns a list of the names of all registered build targets.
        /// </summary>
        public static string[] GetRegisteredBuildTargetNames()
        {
            string[] names = new string[_buildTargets.Count];
            for (int i = 0; i < _buildTargets.Count; ++i)
            {
                names[i] = _buildTargets[i].Name;
            }
            return names;
        }

        /// <summary>
        /// Returns a list of the IBuildTargets that are currently available and selected
        /// for building.
        /// </summary>
        public static IList<IBuildTarget> GetSelectedBuildTargets()
        {
            List<IBuildTarget> targets = new List<IBuildTarget>(_buildTargets.Count);
            foreach (IBuildTarget target in _buildTargets)
            {
                if (target.IsTargetedForBuild)
                {
                    targets.Add(target);
                }
            }
            return targets;
        }

        /// <summary>
        /// Locates a registered IBuildTarget by its name, (e.g, "Windows").
        /// </summary>
        public static IBuildTarget FindBuildTargetByName(string name)
        {
            if (string.IsNullOrEmpty(name)) return null;
            foreach (IBuildTarget target in _buildTargets)
            {
                if (target.Name == name)
                {
                    return target;
                }
            }
            return null;
        }

        /// <summary>
        /// Returns whether all files are available to build targeting the specified platform.
        /// </summary>
        public static bool IsBuildTargetAvailable(IBuildTarget target)
        {
            if ((target == null) || (!_buildTargets.Contains(target))) return false;
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
