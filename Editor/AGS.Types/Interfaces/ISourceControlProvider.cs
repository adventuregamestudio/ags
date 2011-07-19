using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types
{
    public interface ISourceControlProvider : IDisposable
    {
        bool Initialize(IntPtr mainWindowHandle);

        string ProviderName
        {
            get;
        }

        bool Available
        {
            get;
        }

        bool ProjectUnderControl
        {
            get;
        }

        string ProjectSourceControlPath
        {
            get;
        }

        string ProjectSourceControlName
        {
            get;
        }

        bool AddToSourceControl();

        bool OpenProject(string sourceControlPath, string projectName);

        void CloseProject();

        bool IsFileUnderSourceControl(string fileName);

        SourceControlFileStatus[] GetFileStatuses(string[] fileNames);

        void AddFilesToSourceControl(string[] fileNames, string comment);

        void CheckInFiles(string[] fileNames, string comment);

        void CheckOutFiles(string[] fileNames, string comment);

        void RenameFileOnDiskAndInSourceControl(string currentName, string newName);

        void RemoveFiles(string[] fileNames);

        void RenameFile(string currentPath, string newPath);
    }
}
