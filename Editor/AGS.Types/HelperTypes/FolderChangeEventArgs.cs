using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types
{
    public delegate void FolderChangeEventHandler<TFolderItem>(object sender, FolderChangeEventArgs<TFolderItem> e);

    public enum FolderChange
    {
        ItemAdded,
        ItemRemoved,
    }

    public class FolderChangeEventArgs<TFolderItem> : EventArgs
    {
        private TFolderItem _folderItem;
        private FolderChange _folderChange;

        public FolderChangeEventArgs(TFolderItem folderItem, FolderChange folderChange)
        {
            _folderItem = folderItem;
            _folderChange = folderChange;
        }

        public TFolderItem FolderItem { get { return _folderItem; } }

        public FolderChange FolderChange { get { return _folderChange; } }
    }
}
