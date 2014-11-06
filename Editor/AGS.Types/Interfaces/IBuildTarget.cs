using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types
{
    public interface IBuildTarget
    {
        /// <summary>
        /// Returns a set of the required library names and paths.
        /// The VALUE may or may not end with the KEY string, but
        /// the two joined together should result in the full path
        /// to the library file (usually in the editor directory).
        /// </summary>
        IDictionary<string, string> GetRequiredLibraryPaths();
        /// <summary>
        /// Helper function to join together parts of a full path
        /// to the Compiled folder. For normal build targets this
        /// automatically adds the Compiled folder and target
        /// OutputDirectory (e.g., GetCompiledPath("MyGame.exe")
        /// would return the full path to "Compiled/Windows/MyGame.exe"
        /// for a Windows build target).
        /// </summary>
        string GetCompiledPath(params string[] parts);
        /// <summary>
        /// Returns a list of the standard folders which are created
        /// when this target is built. This always includes the
        /// OutputDirectory, but may contain other subfolders as
        /// well.
        /// </summary>
        string[] GetPlatformStandardSubfolders();
        /// <summary>
        /// Returns a list of the files required for this target
        /// to be built. This is the same as the Key list returned
        /// by GetRequiredLibraryPaths, so if you need full paths
        /// use that function instead.
        /// </summary>
        string[] GetRequiredLibraryNames();
        /// <summary>
        /// Calls GetPlatformStandardSubfolders() and then makes
        /// sure that each of those folders exist. This is always
        /// called as part of the normal Build process.
        /// </summary>
        void EnsureStandardSubfoldersExist();
        /// <summary>
        /// Attempts to build the target. Compilation errors will
        /// be added to the ERRORS collection. Returns whether or
        /// not the target was successfully built. You should not
        /// normally need to call this yourself.
        /// </summary>
        bool Build(CompileMessages errors, bool forceRebuild);

        /// <summary>
        /// Returns whether all necessary files are available for
        /// this target to be successfully built.
        /// </summary>
        bool IsAvailable
        {
            get;
        }

        /// <summary>
        /// Returns whether this build target is currently selected
        /// for building in the General Settings pane. See the
        /// Settings.BuildTargets property to check all selected
        /// targets.
        /// </summary>
        bool IsTargetedForBuild
        {
            get;
        }

        /// <summary>
        /// Returns whether this is the only selected build
        /// target in the General Settings pane (excludes the
        /// DataFile target unless all other targets are
        /// deselected). See the Settings.BuildTargets property
        /// to check all selected targets.
        /// </summary>
        bool IsOnlyBuildTarget
        {
            get;
        }

        /// <summary>
        /// Returns whether this is a normal build target. A
        /// normal build target is one which can be selected
        /// from the General Settings pane, and builds to the
        /// Compiled folder. An example of a non-normal build
        /// target would be the built-in "_Debug" target,
        /// whose files are built to the "_Debug" folder
        /// instead of the "Compiled" folder, and is not
        /// listed in the General Settings pane.
        /// </summary>
        bool IsNormalBuildTarget
        {
            get;
        }

        /// <summary>
        /// Gets the name of this build target (e.g., "Windows").
        /// </summary>
        string Name
        {
            get;
        }

        /// <summary>
        /// Gets the subfolder where this target's files are
        /// built (relative to Compiled folder, see
        /// OutputDirectoryFullPath for a full path).
        /// </summary>
        string OutputDirectory
        {
            get;
        }

        /// <summary>
        /// Gets the full path to this target's OutputDirectory.
        /// </summary>
        string OutputDirectoryFullPath
        {
            get;
        }
    }
}
