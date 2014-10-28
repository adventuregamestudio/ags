using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types
{
    public interface IBuildTarget
    {
        IDictionary<string, string> GetRequiredLibraryPaths();
        string GetCompiledPath(params string[] parts);
        string[] GetPlatformStandardSubfolders();
        string[] GetRequiredLibraryNames();
        void EnsureStandardSubfoldersExist();
        bool Build(CompileMessages errors, bool forceRebuild);

        bool IsAvailable
        {
            get;
        }

        bool IsTargetedForBuild
        {
            get;
        }

        bool IsOnlyBuildTarget
        {
            get;
        }

        string Name
        {
            get;
        }

        string OutputDirectory
        {
            get;
        }

        string OutputDirectoryFullPath
        {
            get;
        }

        BuildTargetPlatform Platform
        {
            get;
        }
    }
}
