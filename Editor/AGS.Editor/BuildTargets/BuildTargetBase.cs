﻿using AGS.Types;
using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace AGS.Editor
{
    public abstract class BuildTargetBase : IBuildTarget
    {
        public abstract IDictionary<string, string> GetRequiredLibraryPaths();
        public abstract string[] GetPlatformStandardSubfolders();

        public virtual bool Build(CompileMessages errors, bool forceRebuild)
        {
            if (!IsAvailable)
            {
                errors.Add(new CompileError("Could not build target platform " + Platform + ", because it is unavailable."));
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
                sb.Append(Path.DirectorySeparatorChar + parts[i]);
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
                Directory.CreateDirectory(Utilities.GetFullPathFromProjectRelative(subfolder)); // automatically checks if directories exist
            }
        }

        public virtual bool IsAvailable
        {
            get
            {
                return BuildTargetInfo.IsBuildTargetAvailable(Platform);
            }
        }

        public virtual bool IsTargetedForBuild
        {
            get
            {
                return ((Factory.AGSEditor.CurrentGame.Settings.BuildTargets & Platform) != 0);
            }
        }

        public virtual bool IsOnlyBuildTarget
        {
            get
            {
                return (Factory.AGSEditor.CurrentGame.Settings.BuildTargets == Platform);
            }
        }

        public virtual string Name
        {
            get
            {
                return Platform.ToString();
            }
        }

        public abstract string OutputDirectory
        {
            get;
        }

        public virtual string OutputDirectoryFullPath
        {
            get
            {
                return Utilities.GetFullPathFromProjectRelative(OutputDirectory);
            }
        }

        public abstract BuildTargetPlatform Platform
        {
            get;
        }
    }
}
