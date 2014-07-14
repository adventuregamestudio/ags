using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types
{
    public delegate void GetSourceControlFileListHandler(IList<string> fileNames);

    /// <summary>
    /// Operations to handle files under Source Control. If the current game
    /// is not under Source Control, these operations will all work on the
    /// local file system only.
    /// </summary>
    public interface ISourceControlIntegration
    {
        /// <summary>
        /// Attempt to get write access to the specified file, checking out
        /// the file if necessary. If this fails, a dialog will be displayed
        /// to the user and false will be returned.
        /// </summary>
        bool AttemptToGetWriteAccess(string fileName);

        /// <summary>
        /// Attempt to get write access to all of the specified files, checking
        /// them out if necessary. If one of them fails, a dialog will be
        /// displayed to the user and false will be returned.
        /// </summary>
        bool AttemptToGetWriteAccess(IList<string> fileNames);

        /// <summary>
        /// Deletes the specified files from the disk. If they are under
        /// source control, prompts the user and asks them whether they also
        /// want to delete the files from source control.
        /// </summary>
        void DeleteFileOnDiskAndSourceControl(string[] fileNames);

        /// <summary>
        /// Renames the specified file on the disk and in the source control
        /// repository, if applicable.
        /// </summary>
        void RenameFileOnDiskAndSourceControl(string currentName, string newName);

        /// <summary>
        /// Links the specified file extension with the specified icon key. The
        /// Pending Checkins window will use this to determine what icon to
        /// display for the files.
        /// </summary>
        /// <param name="fileExtension">The file extension to register. Should start with a period.</param>
        /// <param name="iconKey">The icon key, previously registered with the GUIController's RegisterIcon method.</param>
        void RegisterFileIconAssociation(string fileExtension, string iconKey);

        /// <summary>
        /// Fired by the Pending Checkins window to get a list of files that
        /// comprise the game source code. All source-controllable files should
        /// be returned, and the Pending Checkins window will then display
        /// any that are not currently checked in.
        /// </summary>
        event GetSourceControlFileListHandler GetSourceControlFileList;
    }
}
