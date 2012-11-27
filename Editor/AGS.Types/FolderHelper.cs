using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types
{
    /// <summary>
    /// Set of static helper functions for working with folders.
    /// [IKM] 2012-11-13
    /// Was added to provide a workaround over MS C++/CLR compiler bug that did
    /// not properly recognize generic classes with constraints when used in
    /// C++/CLR code.
    /// </summary>
    public static class FolderHelper
    {
        public static IViewFolder CreateDefaultViewFolder()
        {
            return new ViewFolder();
        }

        public static IViewFolder GetRootViewFolder(Game game)
        {
            return game.RootViewFolder;
        }

        public static void SetRootViewFolder(Game game, IViewFolder folder)
        {
            ViewFolder default_folder = (ViewFolder)folder;
            if (default_folder != null)
            {
                game.RootViewFolder = default_folder;
            }
        }

        // Copied from AGS.Native
        public static int CountViews(IViewFolder folder) 
        {
	        int highestViewNumber = 0;
	        foreach (ViewFolder subFolder in folder.SubFolders)
	        {
		        int folderView = CountViews(subFolder);
		        if (folderView > highestViewNumber) 
		        {
			        highestViewNumber = folderView;
		        }
	        }
	        foreach (View view in folder.Views)
	        {
		        if (view.ID > highestViewNumber)
		        {
			        highestViewNumber = view.ID;
		        }
	        }
	        return highestViewNumber;
        }

        public delegate void ViewFolderProcessing(IViewFolder folder, Game game);

        public static void ForEachViewFolder(IViewFolder parentFolder, Game game, ViewFolderProcessing proc)
        {
            foreach (ViewFolder subFolder in parentFolder.SubFolders)
	        {
                proc(subFolder, game);
	        }
        }
    }
}
