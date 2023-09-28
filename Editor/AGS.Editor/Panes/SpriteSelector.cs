using Microsoft.Win32;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Windows.Forms;
using System.Linq;
using AGS.Types;
using AGS.Editor.Utils;

namespace AGS.Editor
{
    public partial class SpriteSelector : UserControl
    {
        public delegate void SelectionChangedHandler(Sprite[] newSelection);
        [Description("Occurs when the selected sprite changes")]
        public event SelectionChangedHandler OnSelectionChanged;

        public delegate void SpriteActivatedHandler(Sprite activatedSprite);
        [Description("Occurs when a sprite is double-clicked")]
        public event SpriteActivatedHandler OnSpriteActivated;

        private const string GIF_FILTER = "Compuserve Graphics Interchange (*.gif)|*.gif";

        private const string MENU_ITEM_RENAME = "Rename";
        private const string MENU_ITEM_CREATE_SUB_FOLDER = "NewFolder";
        private const string MENU_ITEM_DELETE_FOLDER = "DeleteFolder";

        private const string MENU_ITEM_IMPORT_NEW = "ImportNewSprite";
        private const string MENU_ITEM_PASTE_NEW = "PasteNewSprite";
        private const string MENU_ITEM_NEW_FROM_PREVIOUS = "NewSpriteFromPrevious";
        private const string MENU_ITEM_REPLACE_FROM_SOURCE_ALL = "ReplaceAllSpritesFromSource";
        private const string MENU_ITEM_EXPORT_FOLDER = "ExportFolder";
        private const string MENU_ITEM_EXPORT_FIXUP_SOURCES = "ExportFixupSources";
        private const string MENU_ITEM_SORT_BY_NUMBER = "SortSpritesByNumber";
        private const string MENU_ITEM_REPLACE_FROM_SOURCE = "ReplaceSpriteFromSource";
        private const string MENU_ITEM_FIND_BY_NUMBER = "FindSpriteByNumber";

        private const string MENU_ITEM_USE_THIS_SPRITE = "UseThisSprite";
        private const string MENU_ITEM_EDIT_THIS_SPRITE = "EditThisSprite";
        private const string MENU_ITEM_COPY_TO_CLIPBOARD = "CopyToClipboard";
        private const string MENU_ITEM_EXPORT_SPRITE = "ExportSprite";
        private const string MENU_ITEM_REPLACE_FROM_FILE = "ReplaceFromFile";
        private const string MENU_ITEM_REPLACE_FROM_PREVIOUS = "ReplaceFromPreviousFiles";
        private const string MENU_ITEM_REPLACE_FROM_CLIPBOARD = "ReplaceFromClipboard";
        private const string MENU_ITEM_OPEN_FILE_EXPLORER = "OpenFileExplorer";
        private const string MENU_ITEM_DELETE_SPRITE = "DeleteSprite";
        private const string MENU_ITEM_SHOW_USAGE = "ShowUsage";
        private const string MENU_ITEM_CROP_ASYMMETRIC = "CropAsymettric";
        private const string MENU_ITEM_CROP_SYMMETRIC = "CropSymettric";
        private const string MENU_ITEM_ASSIGN_TO_VIEW = "AssignToView";
        private const string MENU_ITEM_CHANGE_SPRITE_NUMBER = "ChangeSpriteNumber";

        private const string MENU_ITEM_PREVIEW_SIZE_1X = "PreviewSizeSmall";
        private const string MENU_ITEM_PREVIEW_SIZE_2X = "PreviewSizeMedium";
        private const string MENU_ITEM_PREVIEW_SIZE_3X = "PreviewSizeLarge";
        private const string MENU_ITEM_PREVIEW_SIZE_4X = "PreviewSizeExtraLarge";
        private const string MENU_ITEM_PREVIEW_TEXT_FILENAME = "PreviewTextFilename";
        
        private const int SPRITE_BASE_SIZE = 32;

        private static ImageList _spManagerIcons;
        private Dictionary<string, SpriteFolder> _folders;
        private Dictionary<SpriteFolder, TreeNode> _folderNodeMapping;
        private Dictionary<TreeNode, SpriteFolder> _nodeFolderMapping;
        private SpriteFolder _currentFolder;
        private SpriteFolder _rootFolder;
        private ImageList _spriteImages = new ImageList();
        private int _spriteNumberOnMenuActivation;
        private bool _showUseThisSpriteOption = false;
        private bool _sendUpdateNotifications = false;
        private string[] _lastImportedFilenames = null;
        private Timer _timer;
        private TreeNode _dropHighlight;
        private int _spriteSizeMultiplier = 1;
        private bool _idleHandlerSet = false;
        // Which information to display under sprites
        // TODO: extract into flags?
        private bool _showSpriteFilenames = false;
        // A formatting string used to make sprite item's text
        private string _itemTextFormat = "{0}";

        public SpriteSelector()
        {
            InitializeComponent();
            _folders = new Dictionary<string, SpriteFolder>(
                // The TreeNodeCollection uses case-insensitive string comparer
                StringComparer.Create(System.Globalization.CultureInfo.CurrentCulture, true));
            _folderNodeMapping = new Dictionary<SpriteFolder, TreeNode>();
            _nodeFolderMapping = new Dictionary<TreeNode, SpriteFolder>();

            if (_spManagerIcons == null)
            {
                _spManagerIcons = new ImageList();
                _spManagerIcons.Images.Add("Folder", Resources.ResourceManager.GetIcon("folder.ico"));
                _spManagerIcons.Images.Add("OpenFolder", Resources.ResourceManager.GetIcon("openfldr.ico"));
            }
            folderList.ImageList = _spManagerIcons;
            folderList.KeyDown += new System.Windows.Forms.KeyEventHandler(this.projectTree_KeyDown);
            SetSpritePreviewMultiplier(2); // default value for sprite multiplier
        }

        /// <summary>
        /// Gets the sprite out of the sprite list item 
        /// </summary>
        private Sprite GetSprite(ListViewItem item)
        {
            return item.Tag as Sprite;
        }

        public bool ShowUseThisSpriteOption
        {
            get { return _showUseThisSpriteOption; }
            set { _showUseThisSpriteOption = value; }
        }

        public bool SendUpdateNotifications
        {
            get { return _sendUpdateNotifications; }
            set { _sendUpdateNotifications = value; }
        }

        public bool ShowSpriteFilenames
        {
            get { return _showSpriteFilenames; }
            set
            {
                _showSpriteFilenames = value;
                _itemTextFormat = _showSpriteFilenames ? "{0}\n{1}" : "{0}";
            }
        }

        public Sprite SelectedSprite
        {
            get
            {
                if (spriteList.SelectedItems.Count == 1)
                {
                    return GetSprite(spriteList.SelectedItems[0]);
                }
                return null;
            }
        }

        public void SetDataSource(SpriteFolder rootFolder)
        {
            // store the hash for each expanded sprite folder
            List<int> expanded = new List<int>();
            for (int i = 0; i < folderList.Nodes.Count; i++)
            {
                AddNodeState(_nodeFolderMapping[folderList.Nodes[i]], expanded);
            }

            // reset - this could be adding a sprite or loading another game
            _rootFolder = rootFolder;
            folderList.Nodes.Clear();
            _folders.Clear();
            _folderNodeMapping.Clear();
            _nodeFolderMapping.Clear();
            BuildNodeTree(rootFolder, folderList.Nodes);

            // re-expand nodes where they look to be the same
            foreach (SpriteFolder folder in _folderNodeMapping.Keys)
            {
                if (expanded.Contains(folder.GetHashCode()))
                {
                    _folderNodeMapping[folder].Expand();
                }
            }

            if (_currentFolder != null && _folderNodeMapping.ContainsKey(_currentFolder))
            {
                // reselect the previous node
                folderList.SelectedNode = _folderNodeMapping[_currentFolder];
	            DisplaySpritesForFolder(_currentFolder);
            }
            else
            {
                // default to expanded root node
                folderList.SelectedNode = folderList.Nodes[0];
                DisplaySpritesForFolder(rootFolder);
                folderList.Nodes[0].Expand();
            }
        }

        private void AddNodeState(SpriteFolder folder, List<int> expanded)
        {
            if (_folderNodeMapping.ContainsKey(folder) &&
                _folderNodeMapping[folder].IsExpanded)
            {
                expanded.Add(folder.GetHashCode());
            }

            foreach (SpriteFolder subfolder in folder.SubFolders)
            {
                AddNodeState(subfolder, expanded);
            }
        }

        private void BuildNodeTree(SpriteFolder folder, TreeNodeCollection parent)
        {
            TreeNode newNode = AddTreeNode(folder, parent);

            foreach (SpriteFolder subFolder in folder.SubFolders)
            {
                BuildNodeTree(subFolder, newNode.Nodes);
            }
        }

        private TreeNode AddTreeNode(SpriteFolder folder, TreeNodeCollection parent)
        {
            string nodeID = folder.Name;
            while (_folders.ContainsKey(nodeID))
            {
                nodeID = nodeID + "A";
            }
            TreeNode addedNode = parent.Add(nodeID, folder.Name, "Folder", "OpenFolder");
            _folders.Add(nodeID, folder);
            _folderNodeMapping.Add(folder, addedNode);
            _nodeFolderMapping.Add(addedNode, folder);
            return addedNode;
        }

        private string GetTextForItem(Sprite sprite)
        {
            return string.Format(_itemTextFormat, sprite.Number, Path.GetFileName(sprite.SourceFile));
        }

        private void DisplaySpritesForFolder(SpriteFolder folder)
        {
            if (folder == null) return;

            if (OnSelectionChanged != null)
            {
                // this means the previously selected sprite is un-selected
                // from the property grid
                OnSelectionChanged(new Sprite[0]);
            }

            if (this.ParentForm != null)
            {
                this.ParentForm.Cursor = Cursors.WaitCursor;
                this.Cursor = Cursors.WaitCursor;
            }

            _currentFolder = folder;
            spriteList.BeginUpdate();
            spriteList.Clear();
            _spriteImages.Images.Clear();
            _spriteImages.ColorDepth = ColorDepth.Depth32Bit;
            _spriteImages.ImageSize = new Size(SPRITE_BASE_SIZE * _spriteSizeMultiplier, SPRITE_BASE_SIZE * _spriteSizeMultiplier);
            _spriteImages.TransparentColor = Color.Pink;
            List<ListViewItem> itemsToAdd = new List<ListViewItem>();

            Progress progress = new Progress(folder.Sprites.Count, "Refreshing folder...");
            progress.Show();

            for (int index = 0; index < folder.Sprites.Count; index ++)
            {
                progress.SetProgressValue(index);
                Sprite sprite = folder.Sprites[index];

                int newSize = SPRITE_BASE_SIZE * _spriteSizeMultiplier;

                Bitmap bmp = Utilities.GetBitmapForSpriteResizedKeepingAspectRatio(sprite, newSize, newSize, true, true, Color.Pink);

                // we are already indexing from 0 and this ImageList was cleared,
                // so just adding the image doesn't need a modified index
                _spriteImages.Images.Add(bmp);

                // adding items individually in this loop is extremely slow, so build
                // a List of items and use AddRange instead
                var item = new ListViewItem(GetTextForItem(sprite), index);
                item.Tag = sprite;
                itemsToAdd.Add(item);
            }

            progress.Hide();
            progress.Dispose();

            spriteList.Items.AddRange(itemsToAdd.ToArray());
            spriteList.LargeImageList = _spriteImages;
            spriteList.EndUpdate();

            if (this.ParentForm != null)
            {
                this.ParentForm.Cursor = Cursors.Default;
                this.Cursor = Cursors.Default;
            }
        }

        /// <summary>
        /// Updates the listview item texts according to the current settings.
        /// </summary>
        private void RefreshSpriteTexts()
        {
            spriteList.BeginUpdate();
            foreach (ListViewItem item in spriteList.Items)
            {
                var sprite = item.Tag as Sprite;
                item.Text = GetTextForItem(sprite);
            }
            spriteList.EndUpdate();
        }

        /// <summary>
        /// Fully repopulates the sprite listview.
        /// </summary>
        private void RefreshSpriteDisplay()
        {
            DisplaySpritesForFolder(_currentFolder);

            if (_sendUpdateNotifications)
            {
                _rootFolder.NotifyClientsOfUpdate();
            }
        }

        public void SelectSprite(Sprite sprite)
        {
            SelectSprite(sprite.Number);
        }

        public void SelectSprite(int spriteNumber)
        {
            spriteList.SelectedItems.Clear();
            foreach (ListViewItem listItem in spriteList.Items)
            {
                if (GetSprite(listItem).Number == spriteNumber)
                {
                    listItem.Selected = true;
                    listItem.Focused = true;
                    listItem.EnsureVisible();
                    spriteList.Focus();
                    break;
                }
            }
        }

        public void EnsureSpriteListFocused()
        {
            spriteList.Focus();
        }

        public bool OpenFolderForSprite(int spriteNumber)
        {
            foreach (KeyValuePair<string, SpriteFolder> entry in _folders)
            {
                foreach (Sprite sprite in entry.Value.Sprites)
                {
                    if (sprite.Number == spriteNumber)
                    {
                        folderList.SelectedNode = folderList.Nodes.Find(entry.Key, true)[0];
                        // the SelectedNode needs to process message loop before we
                        // can focus to the new list of sprites
                        if (_timer == null)
                        {
                            _timer = new Timer();
                            _timer.Interval = 50;
                            _timer.Tick += new EventHandler(_timer_Tick);
                            _timer.Tag = spriteNumber;
                            _timer.Start();
                        }
                        return true;
                    }
                }
            }
            return false;
        }

        private void _timer_Tick(object sender, EventArgs e)
        {
            _timer.Stop();
            this.SelectSprite((int)_timer.Tag);
            this.EnsureSpriteListFocused();
            _timer.Dispose();
            _timer = null;
        }

        private void folderList_AfterSelect(object sender, TreeViewEventArgs e)
        {
            DisplaySpritesForFolder(_folders[e.Node.Name]);
        }

        private void folderList_MouseUp(object sender, MouseEventArgs e)
        {
            if (e.Button == MouseButtons.Right)
            {
                TreeNode clickedNode = folderList.HitTest(e.Location).Node;
                if (clickedNode != null)
                {
                    folderList.SelectedNode = clickedNode;
                    ShowTreeContextMenu(clickedNode, e.Location);
                }
            }
        }

        private void TreeContextMenuEventHandler(object sender, EventArgs e)
        {
            ToolStripMenuItem item = (ToolStripMenuItem)sender;
            TreeNode node = (TreeNode)item.Owner.Tag;
            if (item.Name == MENU_ITEM_CREATE_SUB_FOLDER)
            {
                SpriteFolder newFolder = new SpriteFolder("New folder");
                _folders[node.Name].SubFolders.Add(newFolder);
                TreeNode newNode = AddTreeNode(newFolder, node.Nodes);
                node.Expand();
                folderList.SelectedNode = newNode;
                newNode.BeginEdit();
            }
            else if (item.Name == MENU_ITEM_RENAME)
            {
                node.BeginEdit();
            }
            else if (item.Name == MENU_ITEM_DELETE_FOLDER)
            {
                DeleteFolder(node);
            }
        }

        private void DeleteFolder(TreeNode node)
        {
            if (Factory.GUIController.ShowQuestion("Are you sure you want to delete the folder '" + node.Text + "' and all its subfolders?") == DialogResult.Yes)
            {
                SpriteFolder folderToDelete = _folders[node.Name];
                try
                {
                    VerifySpriteFolderTreeCanBeDeleted(folderToDelete);
                    node.Parent.Nodes.Remove(node);
                    DeleteSpriteFolder(folderToDelete, Factory.AGSEditor.CurrentGame.RootSpriteFolder);
                }
                catch (AGSEditorException ex)
                {
                    Factory.GUIController.ShowMessage(ex.Message, MessageBoxIcon.Warning);
                }
            }
        }

        private void DeleteAllSpritesInTree(SpriteFolder topFolder)
        {
            if (_folderNodeMapping.ContainsKey(topFolder))
            {
                _folderNodeMapping.Remove(topFolder);
            }

            foreach (Sprite sprite in topFolder.Sprites)
            {
                Factory.NativeProxy.DeleteSprite(sprite);
            }

            foreach (SpriteFolder subFolder in topFolder.SubFolders)
            {
                DeleteAllSpritesInTree(subFolder);
            }
        }

        private void VerifySpriteFolderTreeCanBeDeleted(SpriteFolder folderToDelete)
        {
            foreach (Sprite sprite in folderToDelete.Sprites)
            {
                string usageReport = SpriteTools.GetSpriteUsageReport(sprite.Number, Factory.AGSEditor.CurrentGame);
                if (usageReport != null)
                {
                    throw new AGSEditorException("Folder cannot be deleted because sprite " + sprite.Number + " is in use:" + Environment.NewLine + usageReport);
                }

                if (!Factory.AGSEditor.AboutToDeleteSprite(sprite.Number))
                {
                    throw new AGSEditorException("Folder cannot be deleted because sprite " + sprite.Number + " could not be marked for deletion");
                }
            }
        }

        private void DeleteSpriteFolder(SpriteFolder folderToDelete, SpriteFolder folderToCheck)
        {
            foreach (SpriteFolder folder in folderToCheck.SubFolders)
            {
                if (folder == folderToDelete)
                {
                    folderToCheck.SubFolders.Remove(folderToDelete);
                    DeleteAllSpritesInTree(folderToDelete);
                    break;
                }
                DeleteSpriteFolder(folderToDelete, folder);
            }
        }

        private void ShowTreeContextMenu(TreeNode forNode, Point menuPosition)
        {
            EventHandler onClick = new EventHandler(TreeContextMenuEventHandler);
            ContextMenuStrip menu = new ContextMenuStrip();
            menu.Tag = forNode;
            menu.Items.Add(new ToolStripMenuItem("Rename", null, onClick, MENU_ITEM_RENAME));
            menu.Items.Add(new ToolStripMenuItem("Create sub-folder", null, onClick, MENU_ITEM_CREATE_SUB_FOLDER));

            if (forNode.Level > 0)
            {
                menu.Items.Add(new ToolStripSeparator());
                menu.Items.Add(new ToolStripMenuItem("Delete", null, onClick, MENU_ITEM_DELETE_FOLDER));
            }

            menu.Show(folderList, menuPosition);
        }

        private void folderList_AfterLabelEdit(object sender, NodeLabelEditEventArgs e)
        {
            if ((!e.CancelEdit) && (e.Label != null))
            {
                _folders[e.Node.Name].Name = e.Label;
            }
        }

        private void spriteList_MouseUp(object sender, MouseEventArgs e)
        {
            if (e.Button == MouseButtons.Right)
            {
                ShowSpriteContextMenu(e.Location);
            }
        }

        private void ImportNewSprite(SpriteFolder folder, string filename)
        {
            ImportNewSprite(folder, new string[] { filename });
        }

        private void ImportNewSprite(SpriteFolder folder, string[] filenames)
        {
            _lastImportedFilenames = filenames;
            SpriteImportWindow impWin = new SpriteImportWindow(filenames, folder);

            if (impWin.ShowDialog() == DialogResult.OK)
            {
                RefreshSpriteDisplay();
            }

            impWin.Dispose();
        }

        private void ImportNewSprite(SpriteFolder folder, Bitmap bmp)
        {
            SpriteImportWindow impWin = new SpriteImportWindow(bmp, folder);

            if (impWin.ShowDialog() == DialogResult.OK)
            {
                RefreshSpriteDisplay();
            }

            impWin.Dispose();
        }

        private void ReplaceSprite(Sprite sprite, string[] filenames)
        {
            _lastImportedFilenames = filenames;
            SpriteImportWindow impWin = new SpriteImportWindow(filenames, sprite);

            if (impWin.ShowDialog() == DialogResult.OK)
            {
                RefreshSpriteDisplay();
            }

            impWin.Dispose();
        }

        private void ReplaceSprite(Sprite sprite, string filename)
        {
            _lastImportedFilenames = new string[] { filename };
            SpriteImportWindow impWin = new SpriteImportWindow(new string[] { filename }, sprite);

            if (impWin.ShowDialog() == DialogResult.OK)
            {
                RefreshSpriteDisplay();
            }

            impWin.Dispose();
        }

        private void ReplaceSprite(Sprite sprite, Bitmap bmp)
        {
            SpriteImportWindow impWin = new SpriteImportWindow(bmp, sprite);

            if (impWin.ShowDialog() == DialogResult.OK)
            {
                RefreshSpriteDisplay();
            }

            impWin.Dispose();
        }

        public void DeleteKeyPressed()
        {
            if ((folderList.Focused) && (folderList.SelectedNode != null))
            {
                DeleteFolder(folderList.SelectedNode);
            }
            else if ((spriteList.Focused) && (spriteList.SelectedItems.Count > 0))
            {
                DeleteSelectedSprites();
            }
        }

        private void DeleteSelectedSprites()
        {
            if (Factory.GUIController.ShowQuestion("Only delete this sprite(s) if you are ABSOLUTELY SURE it is not used AT ALL in your game. AGS cannot automatically detect usage of the sprite within room files or script function calls.\n\nAny parts of your game that do use this sprite will cause the editor and engine to crash if you go ahead. Are you sure?") == DialogResult.Yes)
            {
                foreach (ListViewItem listItem in spriteList.Items)
                {
                    if (listItem.Selected)
                    {
                        Sprite sprite = GetSprite(listItem);

                        try
                        {
                            Factory.AGSEditor.DeleteSprite(sprite);
                        }
                        catch (SpriteInUseException ex)
                        {
                            Factory.GUIController.ShowMessage(ex.Message, MessageBoxIcon.Warning);
                        }
                    }
                }
                RefreshSpriteDisplay();
            }
        }

        private int GetDesktopColourDepth()
        {
            Graphics desktopHandle = Graphics.FromHwnd(IntPtr.Zero);
            Bitmap desktopBitmap = new Bitmap(1, 1, desktopHandle);
            PixelFormat formatToReturn = desktopBitmap.PixelFormat;
            desktopBitmap.Dispose();
            desktopHandle.Dispose();
            if ((formatToReturn == PixelFormat.Format32bppArgb) ||
                (formatToReturn == PixelFormat.Format32bppPArgb) ||
                (formatToReturn == PixelFormat.Format32bppRgb))
            {
                return 32;
            }
            return 16;
        }

        private void SetSpritePreviewMultiplier(int multiplier)
        {
            if (_spriteSizeMultiplier != multiplier)
            {
                sliderPreviewSize.Value = multiplier;
                _spriteSizeMultiplier = multiplier;
                RefreshSpriteDisplay();
            }
        }

        private void SpriteContextMenuEventHandler(object sender, EventArgs e)
        {
            ToolStripMenuItem item = (ToolStripMenuItem)sender;

            if (item.Name == MENU_ITEM_PASTE_NEW)
            {
                if ((Clipboard.ContainsImage()) && (Clipboard.GetImage() is Bitmap))
                {
                    Bitmap bmp = (Bitmap)Clipboard.GetImage();
                    if ((bmp.PixelFormat == PixelFormat.Format32bppRgb) ||
                        (bmp.PixelFormat == PixelFormat.Format16bppRgb565))
                    {
                        ImportNewSprite(_currentFolder, bmp);
                    }
                    else
                    {
                        Factory.GUIController.ShowMessage("The image on the clipboard is in an unrecognised format: " + bmp.PixelFormat, MessageBoxIcon.Warning);
                        bmp.Dispose();
                    }
                }
                else
                {
                    Factory.GUIController.ShowMessage("The clipboard does not currently contain a supported image format.", MessageBoxIcon.Warning);
                }
            }
            else if (item.Name == MENU_ITEM_IMPORT_NEW)
            {
                string[] filenames = Factory.GUIController.ShowOpenFileDialogMultipleFiles("Import new sprites...", Constants.IMAGE_FILE_FILTER);

                if (filenames.Length > 0)
                {
                    ImportNewSprite(_currentFolder, filenames);
                }
            }
            else if (item.Name == MENU_ITEM_REPLACE_FROM_SOURCE_ALL)
            {
                if (Factory.GUIController.ShowQuestion("This will recreate game's spritefile using sprite source files if they are available. All sprites will be updated from their sources.\n\nNOTE: sprites that don't have source file references, or which source files are missing, - will remain untouched.\n\nAre you sure you want to do this?",
                    MessageBoxIcon.Warning) == DialogResult.Yes)
                {
                    Tasks.RecreateSpriteFileFromSources();
                }
            }
            else if (item.Name == MENU_ITEM_NEW_FROM_PREVIOUS)
            {
                ImportNewSprite(_currentFolder, _lastImportedFilenames);
            }
            else if (item.Name == MENU_ITEM_REPLACE_FROM_FILE)
            {
                string fileName = Factory.GUIController.ShowOpenFileDialog("Replace sprite...", Constants.IMAGE_FILE_FILTER);
                if (fileName != null)
                {
                    Sprite sprite = FindSpriteByNumber(_spriteNumberOnMenuActivation);
                    ReplaceSpriteUsingImportWindow(fileName, sprite);
                }
            }
            else if (item.Name == MENU_ITEM_REPLACE_FROM_PREVIOUS)
            {
                Sprite sprite = FindSpriteByNumber(_spriteNumberOnMenuActivation);
                ReplaceSpriteUsingImportWindow(_lastImportedFilenames, sprite);
            }
            else if (item.Name == MENU_ITEM_REPLACE_FROM_CLIPBOARD)
            {
                if ((Clipboard.ContainsImage()) && (Clipboard.GetImage() is Bitmap))
                {
                    Bitmap bmp = (Bitmap)Clipboard.GetImage();
                    if ((bmp.PixelFormat == PixelFormat.Format32bppRgb) ||
                        (bmp.PixelFormat == PixelFormat.Format16bppRgb565))
                    {
                        Sprite sprite = FindSpriteByNumber(_spriteNumberOnMenuActivation);
                        ReplaceSprite(sprite, bmp);
                    }
                    else
                    {
                        Factory.GUIController.ShowMessage("The image on the clipboard is in an unrecognised format: " + bmp.PixelFormat, MessageBoxIcon.Warning);
                        bmp.Dispose();
                    }
                }
                else
                {
                    Factory.GUIController.ShowMessage("The clipboard does not currently contain a supported image format.", MessageBoxIcon.Warning);
                }
            }
            else if (item.Name == MENU_ITEM_OPEN_FILE_EXPLORER)
            {
                Sprite sprite = FindSpriteByNumber(_spriteNumberOnMenuActivation);
                string path = Utilities.ResolveSourcePath(sprite.SourceFile);

                Utilities.OpenFileOrDirInFileExplorer(path);
            }
            else if (item.Name == MENU_ITEM_DELETE_SPRITE)
            {
                DeleteSelectedSprites();
            }
            else if (item.Name == MENU_ITEM_EXPORT_FOLDER)
            {
                ExportAllSprites();
            }
            else if (item.Name == MENU_ITEM_EXPORT_FIXUP_SOURCES)
            {
                ExportFixupSources();
            }
            else if (item.Name == MENU_ITEM_SORT_BY_NUMBER)
            {
                SortAllSpritesInCurrentFolderByNumber();
            }
            else if (item.Name == MENU_ITEM_FIND_BY_NUMBER)
            {
                PromptUserForSpriteNumberAndFindSprite();
            }
            else if (item.Name == MENU_ITEM_EXPORT_SPRITE)
            {
                string fileName = Factory.GUIController.ShowSaveFileDialog("Export sprite...", Constants.IMAGE_FILE_FILTER);
                if (fileName != null)
                {
                    Sprite sprite = FindSpriteByNumber(_spriteNumberOnMenuActivation);
                    ExportSprite(fileName, sprite);
                }
            }
            else if (item.Name == MENU_ITEM_USE_THIS_SPRITE)
            {
                if (OnSpriteActivated != null)
                {
                    OnSpriteActivated(this.SelectedSprite);
                }
            }
            else if (item.Name == MENU_ITEM_EDIT_THIS_SPRITE)
            {
                LaunchImageEditorForSprite(this.SelectedSprite);
            }
            else if (item.Name == MENU_ITEM_COPY_TO_CLIPBOARD)
            {
                Sprite sprite = FindSpriteByNumber(_spriteNumberOnMenuActivation);
                Bitmap bmp = Factory.NativeProxy.GetBitmapForSprite(sprite.Number, sprite.Width, sprite.Height);

                if (GetDesktopColourDepth() < 32 &&
                    (bmp.PixelFormat != PixelFormat.Format32bppArgb || bmp.PixelFormat == PixelFormat.Format32bppRgb))
                {
                    if (Factory.GUIController.ShowQuestion("Your desktop colour depth is lower than this image. You may lose image detail if you copy this to the clipboard. Do you want to go ahead?") != DialogResult.Yes)
                    {
                        bmp.Dispose();
                        return;
                    }
                }

                Clipboard.SetImage(bmp);
                bmp.Dispose();
            }
            else if (item.Name == MENU_ITEM_CHANGE_SPRITE_NUMBER)
            {
                ChangeSpriteNumber(_spriteNumberOnMenuActivation);
            }
            else if (item.Name == MENU_ITEM_SHOW_USAGE)
            {
                string usage = SpriteTools.GetSpriteUsageReport(_spriteNumberOnMenuActivation, Factory.AGSEditor.CurrentGame);
                if (usage == null)
                {
                    Factory.GUIController.ShowMessage("No uses of this sprite could be found automatically. HOWEVER, it may be used in scripts or as a room object image; these uses cannot be detected automatically.", MessageBoxIcon.Information);
                }
                else
                {
                    Factory.GUIController.ShowMessage(usage, MessageBoxIcon.Information);
                }
            }
            else if (item.Name == MENU_ITEM_ASSIGN_TO_VIEW)
            {
                AssignToView dialog = new AssignToView();
                if (dialog.ShowDialog(this) == DialogResult.OK)
                {
                    List<int> spriteNumbers = new List<int>();
                    foreach (ListViewItem selectedItem in spriteList.SelectedItems)
                    {
                        spriteNumbers.Add(GetSprite(selectedItem).Number);
                    }
                    AssignSpritesToView(spriteNumbers, dialog);
                }
                dialog.Dispose();
            }
            else if (item.Name == MENU_ITEM_CROP_ASYMMETRIC)
            {
                if (Factory.GUIController.ShowQuestion("Cropping the selected sprites will trim off the edges to reduce all the selected sprites to the size required by the largest. Are you sure you want to proceed?") == DialogResult.Yes)
                {
                    CropSelectedSprites(false);
                }
            }
            else if (item.Name == MENU_ITEM_CROP_SYMMETRIC)
            {
                if (Factory.GUIController.ShowQuestion("Cropping the selected sprites will trim off the edges to reduce all the selected sprites to the size required by the largest, but ensuring that the central pivot point of the sprites remains unchanged. Are you sure you want to proceed?") == DialogResult.Yes)
                {
                    CropSelectedSprites(true);
                }
            }
            else if (item.Name == MENU_ITEM_REPLACE_FROM_SOURCE)
            {
                ReplaceSpritesFromSource();
            }
            else if (item.Name == MENU_ITEM_PREVIEW_SIZE_1X)
            {
                SetSpritePreviewMultiplier(2);
            }
            else if (item.Name == MENU_ITEM_PREVIEW_SIZE_2X)
            {
                SetSpritePreviewMultiplier(4);
            }
            else if (item.Name == MENU_ITEM_PREVIEW_SIZE_3X)
            {
                SetSpritePreviewMultiplier(6);
            }
            else if (item.Name == MENU_ITEM_PREVIEW_SIZE_4X)
            {
                SetSpritePreviewMultiplier(8);
            }
            else if (item.Name == MENU_ITEM_PREVIEW_TEXT_FILENAME)
            {
                ShowSpriteFilenames = !ShowSpriteFilenames;
                RefreshSpriteTexts();
            }
        }

        private void ReplaceSpritesFromSource()
        {
            List<string> errors = new List<string>();
            List<Sprite> sprites = new List<Sprite>();

            foreach (ListViewItem listItem in spriteList.SelectedItems) //Check sources still exist
            {
                Sprite spr = GetSprite(listItem);
                if (String.IsNullOrEmpty(spr.SourceFile))
                {
                    Factory.GUIController.ShowMessage(String.Format("Sprite {0} does not have a source file.", spr.Number), MessageBoxIcon.Error);
                    return;
                }
                else if (!File.Exists(spr.SourceFile))
                {
                    Factory.GUIController.ShowMessage(String.Format("Sprite {0}: source file {1} does not exist.", spr.Number, spr.SourceFile), MessageBoxIcon.Error);
                    return;
                }
                sprites.Add(spr);
            }

            Progress progress = new Progress(sprites.Count, "Re-importing from source files...");
            progress.Show();

            for (int index = 0; index < sprites.Count; index ++)
            {
                Sprite spr = sprites[index];
                progress.SetProgressValue(index);

                try
                {
                    SpriteSheet spritesheet;

                    if (spr.ImportAsTile)
                    {
                        spritesheet = new SpriteSheet(new Point(spr.OffsetX, spr.OffsetY), new Size(spr.ImportWidth, spr.ImportHeight));
                    }
                    else
                    {
                        spritesheet = null;
                    }

                    // take the alpha channel preference from the specified import option
                    // (instead of using whether the old sprite has an alpha channel)
                    SpriteTools.ReplaceSprite(spr, spr.SourceFile, spr.Frame, spr.ImportAlphaChannel, spr.RemapToGamePalette, spr.RemapToRoomPalette, spr.TransparentColour, spritesheet);
                }
                catch (Exception ex)
                {
                    if (ex is InvalidOperationException || ex is Types.InvalidDataException)
                    {
                        errors.Add(ex.Message);
                    }
                    else
                    {
                        throw;
                    }
                }
            }

            progress.Hide();
            progress.Dispose();
            RefreshSpriteDisplay();
            Factory.GUIController.ClearOutputPanel();

            if (errors.Count == 1)
            {
                Factory.GUIController.ShowMessage(errors[0], MessageBoxIcon.Warning);
            }
            else if (errors.Count > 1)
            {
                Factory.GUIController.ShowOutputPanel(errors.ToArray(), "SpriteIcon");
                Factory.GUIController.ShowMessage("Sprite replacement complete, with some errors", MessageBoxIcon.Warning);
            }
        }

        private string GetTempFileNameForSprite(Sprite sprite, out ImageFormat fileFormat)
        {
            string fileName;
            try
            {
                fileName = System.IO.Path.GetTempFileName();
            }
            catch (IOException ex)
            {
                Factory.GUIController.ShowMessage("Unable to create temporary file. Your TEMP directory could be full. Open your temp folder in explorer (" + System.IO.Path.GetTempPath() + ") and delete any unnecessary files.\n\nError: " + ex.Message, MessageBoxIcon.Warning);
                fileName = System.IO.Path.Combine(System.IO.Path.GetTempPath(), "agsimg.tmp");
            }

            if (sprite.ColorDepth < 15)
            {
                fileFormat = ImageFormat.Bmp;
                fileName += ".bmp";
            }
            else
            {
                fileFormat = ImageFormat.Png;
                fileName += ".png";
            }
            return fileName;
        }

        private void LaunchImageEditorForSprite(Sprite sprite)
        {
            ImageFormat fileFormat;
            string fileName = GetTempFileNameForSprite(sprite, out fileFormat);

            Bitmap bmp = Factory.NativeProxy.GetBitmapForSprite(sprite.Number);
            bmp.Save(fileName, fileFormat);

            DateTime fileLastModified = System.IO.File.GetLastWriteTimeUtc(fileName);

            try
            {
                BusyDialog.Show("Launching your image editor. Close it to return to AGS...", new BusyDialog.ProcessingHandler(LaunchImageEditorThread), fileName);
            }
            catch (Exception ex)
            {
                Factory.GUIController.ShowMessage("Unable to launch your image editor. Make sure you have an application installed to handle PNG and BMP files, and that it is not already running. If it is already running, close it and then try again.\n\nError: " + ex.Message, MessageBoxIcon.Warning);
            }

            DateTime fileNowModified = System.IO.File.GetLastWriteTimeUtc(fileName);
            if (fileNowModified.CompareTo(fileLastModified) != 0)
            {
                Bitmap newBmp = new Bitmap(fileName);
                if ((newBmp.PixelFormat == PixelFormat.Format8bppIndexed) !=
                    (bmp.PixelFormat == PixelFormat.Format8bppIndexed))
                {
                    Factory.GUIController.ShowMessage("The colour depth of the image has changed. You cannot change this with an in-place edit.", MessageBoxIcon.Warning);
                }
                else
                {
                    Factory.NativeProxy.ReplaceSpriteWithBitmap(sprite, newBmp, sprite.TransparentColour,
                        sprite.RemapToGamePalette, sprite.RemapToRoomPalette, sprite.AlphaChannel);
                    RefreshSpriteDisplay();
                }
                newBmp.Dispose();
            }
            bmp.Dispose();
            try
            {
                Utilities.TryDeleteFile(fileName);
            }
            catch (Exception ex)
            {
                Factory.GUIController.ShowMessage("AGS was not able to delete the temporary sprite file. It could be that the image editor is still running and that AGS has not been able to properly detect when it shuts down. Please report this problem on the AGS Forums." + Environment.NewLine + Environment.NewLine + "Error: " + ex.Message, MessageBoxIcon.Warning);
            }
        }

        private string GetAssociatedProgramForFileExtension(string extension)
        {
            RegistryKey key = Registry.ClassesRoot.OpenSubKey(extension);
            if (key != null)
            {
                string fileHandlerType = key.GetValue(null).ToString();
                key.Close();
                key = Registry.ClassesRoot.OpenSubKey(fileHandlerType + @"\shell\open\command");
                if (key != null)
                {
                    string launchPath = key.GetValue(null).ToString();
                    key.Close();
                    if (launchPath.StartsWith("\""))
                    {
                        launchPath = launchPath.Substring(1);
                        int endIndex = launchPath.IndexOf("\"");
                        if (endIndex > 0)
                        {
                            launchPath = launchPath.Substring(0, endIndex);
                        }
                    }
                    else if (launchPath.Contains(" "))
                    {
                        launchPath = launchPath.Substring(0, launchPath.IndexOf(' '));
                    }
                    return launchPath;
                }
            }
            throw new AGSEditorException("No default application registered to handle file type " + extension);
        }

        private object LaunchImageEditorThread(IWorkProgress progress, object parameter)
        {
            string fileName = (string)parameter;
            Process imageEditor = new Process();

            string paintProgramPath = Factory.AGSEditor.Settings.PaintProgramPath;
            if (string.IsNullOrEmpty(paintProgramPath))
            {
                imageEditor.StartInfo.FileName = GetAssociatedProgramForFileExtension(System.IO.Path.GetExtension(fileName));
                imageEditor.StartInfo.Arguments = string.Format("\"{0}\"", fileName);
            }
            else
            {
                imageEditor.StartInfo.FileName = paintProgramPath;
                imageEditor.StartInfo.Arguments = string.Format("\"{0}\"", fileName);
            }

            imageEditor.Start();
            try
            {
                imageEditor.WaitForInputIdle(15000);
            }
            catch (InvalidOperationException)
            {
                // if it's a console app, WaitForInputIdle doesn't work
            }
            imageEditor.WaitForExit();
            imageEditor.Dispose();

            return null;
        }

        private void AssignSpritesToView(List<int> spriteNumbers, AssignToView dialog)
        {
            int loop = dialog.LoopNumber;

            if (dialog.ReverseFrames)
            {
                spriteNumbers.Reverse();
            }

            AGS.Types.View view = Factory.AGSEditor.CurrentGame.FindViewByID(dialog.ViewNumber);
            if (view == null)
            {
                Factory.GUIController.ShowMessage("The view number you selected (" + dialog.ViewNumber + ") does not exist.", MessageBoxIcon.Warning);
                return;
            }

            while (loop >= view.Loops.Count)
            {
                view.AddNewLoop();
            }

            if (!dialog.AddFramesToExistingLoop)
            {
                view.Loops[loop].Frames.Clear();
            }

            foreach (int spriteNum in spriteNumbers)
            {
                if (view.Loops[loop].Full)
                {
                    if (!dialog.ContinueIntoNextLoop)
                    {
                        Factory.GUIController.ShowMessage("The selected loop is now full. Not all the selected sprites were assigned.", MessageBoxIcon.Information);
                        view.NotifyClientsOfUpdate();
                        return;
                    }
                    loop++;
                    if (loop >= view.Loops.Count)
                    {
                        view.AddNewLoop();
                    }
                    view.Loops[loop - 1].RunNextLoop = true;
                }
                ViewFrame newFrame = new ViewFrame();
                newFrame.ID = view.Loops[loop].Frames.Count;
                newFrame.Image = spriteNum;
                if (dialog.FlipFrames)
                {
                    newFrame.Flipped = true;
                }
                view.Loops[loop].Frames.Add(newFrame);
            }
            Factory.GUIController.ShowMessage("The selected sprites were assigned successfully.", MessageBoxIcon.Information);
            view.NotifyClientsOfUpdate();
        }

        private void CropSelectedSprites(bool symettric)
        {
            List<Sprite> sprites = new List<Sprite>();
            int width = 0, height = 0;
            foreach (ListViewItem listItem in spriteList.Items)
            {
                if (listItem.Selected)
                {
                    Sprite sprite = GetSprite(listItem);
                    if (sprites.Count > 0)
                    {
                        if (sprite.Width != width || sprite.Height != height)
                        {
                            Factory.GUIController.ShowMessage("All sprites to be cropped must have the same dimensions. If you want to crop unrelated sprites, do them separately.", MessageBoxIcon.Warning);
                            return;
                        }
                    }
                    else
                    {
                        width = sprite.Width;
                        height = sprite.Height;
                    }
                    sprites.Add(sprite);
                }
            }
            if (sprites.Count > 0 && Factory.NativeProxy.CropSpriteEdges(sprites, symettric))
            {
                Factory.GUIController.ShowMessage("The selected sprites were cropped down to " + sprites[0].Width + " x " + sprites[0].Height + " successfully.", MessageBoxIcon.Information);
                RefreshSpriteDisplay();
            }
            else
            {
                Factory.GUIController.ShowMessage("The selected sprites could not be cropped any further.", MessageBoxIcon.Information);
            }
        }

        private void ExportSprite(string fileName, Sprite sprite)
        {
            Bitmap bmp = Factory.NativeProxy.GetBitmapForSprite(sprite.Number, sprite.Width, sprite.Height);
            ImportExport.ExportBitmapToFile(fileName, bmp);
            bmp.Dispose();
        }

        private void ExportAllSprites()
        {
            SpriteExportDialog dialog = new SpriteExportDialog(_currentFolder);

            if (dialog.ShowDialog() != DialogResult.OK)
                return;

            SpriteFolder startFolder = dialog.UseRootFolder ?
                Factory.AGSEditor.CurrentGame.RootSpriteFolder : _currentFolder;
            var opts = new SpriteTools.ExportSpritesOptions(
                dialog.ExportPath,
                dialog.Recurse,
                dialog.SkipIf,
                dialog.UpdateSpriteSource,
                dialog.ResetTileSettings);
            dialog.Dispose();

            Tasks.ExportSprites(startFolder, opts);
        }

        private void ExportFixupSources()
        {
            string folder = Factory.GUIController.ShowSelectFolderOrNoneDialog("Create sprite source in folder...",
                Factory.AGSEditor.CurrentGame.DirectoryPath);
            if (folder == null)
                return;

            var opts = new SpriteTools.ExportSpritesOptions(
                    Path.Combine(folder, "%Number%"),
                    recurse: true,
                    skipIf: SpriteTools.SkipIf.SourceLocal,
                    updateSourcePath: true,
                    resetTileSettings: true
                );
            Tasks.ExportSprites(opts);
        }

        private void SortAllSpritesInCurrentFolderByNumber()
        {
            ((List<Sprite>)_currentFolder.Sprites).Sort();
            RefreshSpriteDisplay();
        }

        private void PromptUserForSpriteNumberAndFindSprite()
        {
            string spriteToFind = TextEntryDialog.Show("Find sprite", "Type the number of the sprite that you want to find:", "");
            if (!string.IsNullOrEmpty(spriteToFind))
            {
                int spriteNumberToFind;
                if (int.TryParse(spriteToFind, out spriteNumberToFind))
                {
                    if (!OpenFolderForSprite(spriteNumberToFind))
                    {
                        Factory.GUIController.ShowMessage("Unable to display sprite " + spriteNumberToFind.ToString() + ". Could not find a sprite with that number.", MessageBoxIcon.Warning);
                    }
                }
            }
        }

        private void ReplaceSpriteUsingImportWindow(string[] fileNames, Sprite sprite)
        {
            try
            {
                ReplaceSprite(sprite, fileNames);
            }
            catch (Exception ex)
            {
                Factory.GUIController.ShowMessage("There was an error importing the file. The error message was: '" + ex.Message + "'. Please try again", MessageBoxIcon.Warning);
            }
        }

        private void ReplaceSpriteUsingImportWindow(string fileName, Sprite sprite)
        {
            try
            {
                ReplaceSprite(sprite, fileName);
            }
            catch (Exception ex)
            {
                Factory.GUIController.ShowMessage("There was an error importing the file. The error message was: '" + ex.Message + "'. Please try again", MessageBoxIcon.Warning);
            }
        }

        private void ShowSpriteContextMenu(Point menuPosition)
        {
            _spriteNumberOnMenuActivation = -1;
            ListViewItem itemAtLocation = spriteList.HitTest(menuPosition).Item;
            EventHandler onClick = new EventHandler(SpriteContextMenuEventHandler);
            ContextMenuStrip menu = new ContextMenuStrip();

            if (itemAtLocation != null)
            {
                _spriteNumberOnMenuActivation = GetSprite(itemAtLocation).Number;
                ToolStripMenuItem newItem;
                if (_showUseThisSpriteOption)
                {
                    // add a bold default option for Use This Sprite
                    newItem = new ToolStripMenuItem("Use this sprite", null, onClick, MENU_ITEM_USE_THIS_SPRITE);
                }
                else
                {
                    newItem = new ToolStripMenuItem("Edit in default image editor", null, onClick, MENU_ITEM_EDIT_THIS_SPRITE);
                }
                newItem.Font = new System.Drawing.Font(newItem.Font.Name, newItem.Font.Size, FontStyle.Bold);
                menu.Items.Add(newItem);
                menu.Items.Add(new ToolStripSeparator());
                menu.Items.Add(new ToolStripMenuItem("Open File Explorer at sprite source", null, onClick, MENU_ITEM_OPEN_FILE_EXPLORER));
                menu.Items[menu.Items.Count - 1].Enabled = File.Exists(Utilities.ResolveSourcePath(FindSpriteByNumber(_spriteNumberOnMenuActivation).SourceFile));
                menu.Items.Add(new ToolStripMenuItem("Copy sprite to clipboard", null, onClick, MENU_ITEM_COPY_TO_CLIPBOARD));
                menu.Items.Add(new ToolStripMenuItem("Export sprite to file...", null, onClick, MENU_ITEM_EXPORT_SPRITE));
                menu.Items.Add(new ToolStripSeparator());
                menu.Items.Add(new ToolStripMenuItem("Replace sprite from file...", null, onClick, MENU_ITEM_REPLACE_FROM_FILE));
                menu.Items.Add(new ToolStripMenuItem("Replace sprite using previous files...", null, onClick, MENU_ITEM_REPLACE_FROM_PREVIOUS));
                if (_lastImportedFilenames == null)
                {
                    menu.Items[menu.Items.Count - 1].Enabled = false;
                }
                if (Factory.AGSEditor.CurrentGame.Settings.ColorDepth != GameColorDepth.Palette)
                {
                    menu.Items.Add(new ToolStripMenuItem("Replace sprite from clipboard...", null, onClick, MENU_ITEM_REPLACE_FROM_CLIPBOARD));
                    if (!Clipboard.ContainsImage())
                    {
                        menu.Items[menu.Items.Count - 1].Enabled = false;
                    }
                }
                if (spriteList.SelectedItems.Count > 1)
                {
                    foreach (ToolStripItem menuItem in menu.Items)
                    {
                        menuItem.Enabled = false;
                    }
                }
                menu.Items.Add(new ToolStripMenuItem("Replace sprite(s) from source...", null, onClick, MENU_ITEM_REPLACE_FROM_SOURCE));
                menu.Items.Add(new ToolStripSeparator());
                menu.Items.Add(new ToolStripMenuItem("Show usage...", null, onClick, MENU_ITEM_SHOW_USAGE));
                menu.Items.Add(new ToolStripMenuItem("Change sprite number...", null, onClick, MENU_ITEM_CHANGE_SPRITE_NUMBER));
                if (spriteList.SelectedItems.Count > 1)
                {
                    menu.Items[menu.Items.Count - 2].Enabled = false;
                    menu.Items[menu.Items.Count - 1].Enabled = false;
                }
                menu.Items.Add(new ToolStripMenuItem("Assign to view...", null, onClick, MENU_ITEM_ASSIGN_TO_VIEW));
                menu.Items.Add(new ToolStripSeparator());
                menu.Items.Add(new ToolStripMenuItem("Crop sprite edges", null, onClick, MENU_ITEM_CROP_ASYMMETRIC));
                menu.Items.Add(new ToolStripMenuItem("Crop sprite edges (symmetric)", null, onClick, MENU_ITEM_CROP_SYMMETRIC));
                menu.Items.Add(new ToolStripMenuItem("Delete", null, onClick, MENU_ITEM_DELETE_SPRITE));
                if (_spriteNumberOnMenuActivation == 0)
                {
                    // can't delete sprite number 0
                    menu.Items[menu.Items.Count - 1].Enabled = false;
                }
                menu.Items.Add(new ToolStripSeparator());
            }

            menu.Items.Add(new ToolStripMenuItem("Import new sprite(s) from files...", null, onClick, MENU_ITEM_IMPORT_NEW));

            menu.Items.Add(new ToolStripMenuItem("Import new sprite(s) using previous files...", null, onClick, MENU_ITEM_NEW_FROM_PREVIOUS));
            if (_lastImportedFilenames == null)
            {
                menu.Items[menu.Items.Count - 1].Enabled = false;
            }

            if (Factory.AGSEditor.CurrentGame.Settings.ColorDepth != GameColorDepth.Palette)
            {
                menu.Items.Add(new ToolStripMenuItem("Paste new sprite from clipboard...", null, onClick, MENU_ITEM_PASTE_NEW));
                if (!Clipboard.ContainsImage())
                {
                    menu.Items[menu.Items.Count - 1].Enabled = false;
                }
            }

            menu.Items.Add(new ToolStripMenuItem("Restore all sprites from sources", null, onClick, MENU_ITEM_REPLACE_FROM_SOURCE_ALL));

            menu.Items.Add(new ToolStripSeparator());
            menu.Items.Add(new ToolStripMenuItem("Export all sprites...", null, onClick, MENU_ITEM_EXPORT_FOLDER));
            menu.Items.Add(new ToolStripMenuItem("Create source files for all sprites with missing / external sources...", null, onClick, MENU_ITEM_EXPORT_FIXUP_SOURCES));
            menu.Items.Add(new ToolStripSeparator());
            menu.Items.Add(new ToolStripMenuItem("Find sprite by number...", null, onClick, MENU_ITEM_FIND_BY_NUMBER));
            menu.Items.Add(new ToolStripMenuItem("Sort sprites by number", null, onClick, MENU_ITEM_SORT_BY_NUMBER));

            ToolStripMenuItem viewMenu = new ToolStripMenuItem();
            viewMenu.Text = "View";

            viewMenu.DropDownItems.Add(new ToolStripMenuItem("Small icons", null, onClick, MENU_ITEM_PREVIEW_SIZE_1X));
            viewMenu.DropDownItems.Add(new ToolStripMenuItem("Medium icons", null, onClick, MENU_ITEM_PREVIEW_SIZE_2X));
            viewMenu.DropDownItems.Add(new ToolStripMenuItem("Large icons", null, onClick, MENU_ITEM_PREVIEW_SIZE_3X));
            viewMenu.DropDownItems.Add(new ToolStripMenuItem("Extra large icons", null, onClick, MENU_ITEM_PREVIEW_SIZE_4X));
            viewMenu.DropDownItems.Add(new ToolStripSeparator());
            var item = new ToolStripMenuItem("Show filenames", null, onClick, MENU_ITEM_PREVIEW_TEXT_FILENAME);
            item.Checked = ShowSpriteFilenames; // NOTE: the menu is recreated, so CheckOnClick would be useless
            viewMenu.DropDownItems.Add(item);

            menu.Items.Add(viewMenu);

            menu.Show(spriteList, menuPosition);
        }

        public Sprite FindSpriteByNumber(int spriteNum)
        {
            return _currentFolder.FindSpriteByID(spriteNum, false);
        }

        private void ChangeSpriteNumber(int spriteNum)
        {
            string usage = SpriteTools.GetSpriteUsageReport(spriteNum, Factory.AGSEditor.CurrentGame);
            if (usage != null)
            {
                Factory.GUIController.ShowMessage("Cannot change the sprite number because it is in use:" + Environment.NewLine + usage, MessageBoxIcon.Warning);
                return;
            }

            Sprite sprite = FindSpriteByNumber(spriteNum);
            int newNumber = NumberEntryWithInfoDialog.Show("Change Sprite Number",
                String.Format("Enter the new sprite number in the box below ({0}-{1}):", 0, NativeConstants.MAX_STATIC_SPRITES - 1),
                "WARNING: Changing the sprite slot number is a specialized operation, for advanced users only.\n\nOnly re - number this sprite if you are ABSOLUTELY SURE it is not used AT ALL in your game. Any parts of your game that do use this sprite will cause the editor and engine to crash if you go ahead.",
                sprite.Number, 0, NativeConstants.MAX_STATIC_SPRITES - 1);
            if (newNumber < 0)
                return;
            if (Factory.NativeProxy.DoesSpriteExist(newNumber))
            {
                Factory.GUIController.ShowMessage("The destination sprite number " + newNumber + " already exists.", MessageBoxIcon.Stop);
            }
            else
            {
                try
                {
                    Factory.NativeProxy.ChangeSpriteNumber(sprite, newNumber);
                    RefreshSpriteDisplay();
                    SelectSprite(sprite);
                }
                catch (AGSEditorException ex)
                {
                    Factory.GUIController.ShowMessage("Unable to change the sprite number: " + ex.Message, MessageBoxIcon.Warning);
                }
            }
        }

        /// <summary>
        /// Controls the custom SelectionChanged event, when enabled schedules it
        /// to Application.Idle, and unschedules it when disabled.
        /// </summary>
        private bool SelectionIdleHandler
        {
            get { return _idleHandlerSet; }
            set
            {
                if (_idleHandlerSet != value)
                {
                    _idleHandlerSet = value;
                    if (value)
                        Application.Idle += spriteList_SelectionChanged;
                    else
                        Application.Idle -= spriteList_SelectionChanged;
                }
            }
        }

        private void spriteList_ItemSelectionChanged(object sender, ListViewItemSelectionChangedEventArgs e)
        {
            // Schedule OnSelectionChanged invocation until after the whole range of items is processed.
            // The trick is explained in this stackoverflow thread:
            // https://stackoverflow.com/questions/1191920/net-listview-event-after-changing-selection/1195583#1195583
            SelectionIdleHandler = true;
        }

        /// <summary>
        /// This custom callback is called by Application.Idle right after the ListView
        /// finishes processing a range of (de)selected items. Here we invoke OnSelectionChanged.
        /// </summary>
        private void spriteList_SelectionChanged(object sender, EventArgs e)
        {
            SelectionIdleHandler = false; // reset the event handler

            // TODO: ways to optimize this further:
            // * dont GetSpriteID/FindSpriteByNumber, attach Sprite as a custom obj to ListView items

            Sprite[] selectedSprites = new Sprite[spriteList.SelectedItems.Count];
            for (int i = 0; i < selectedSprites.Length; i++)
            {
                selectedSprites[i] = spriteList.SelectedItems[i].Tag as Sprite;
                if (selectedSprites[i] == null)
                {
                    throw new AGSEditorException("Internal error: selected sprite not in folder");
                }
            }

            OnSelectionChanged?.Invoke(selectedSprites);
        }

        private void spriteList_ItemActivate(object sender, EventArgs e)
        {
            Sprite selected = this.SelectedSprite;
            if (selected != null)
            {
                if (OnSpriteActivated != null)
                {
                    OnSpriteActivated(selected);
                }
                else if (!_showUseThisSpriteOption)
                {
                    LaunchImageEditorForSprite(selected);
                }
            }
        }

        private void spriteList_ItemDrag(object sender, ItemDragEventArgs e)
        {
            SpriteManagerDragDropData dragDropData = new SpriteManagerDragDropData();

            foreach (ListViewItem selectedItem in spriteList.SelectedItems)
            {
                dragDropData.Sprites.Add(GetSprite(selectedItem));
            }

            this.DoDragDrop(dragDropData, DragDropEffects.Move);
        }

        private void spriteList_DragDrop(object sender, DragEventArgs e)
        {
            if(e.Data.GetDataPresent(typeof(SpriteManagerDragDropData)))
            {
                // Moving a sprite already imported
                SpriteManagerDragDropData dragged = (SpriteManagerDragDropData)e.Data.GetData(typeof(SpriteManagerDragDropData));
                Point locationInControl = spriteList.PointToClient(new Point(e.X, e.Y));
                bool putSpritesBeforeSelection = true;
                ListViewItem nearestItem = spriteList.HitTest(locationInControl).Item;
                if (nearestItem == null)
                {
                    putSpritesBeforeSelection = false;
                    nearestItem = spriteList.FindNearestItem(SearchDirectionHint.Left, locationInControl);

                    if (nearestItem == null)
                    {
                        putSpritesBeforeSelection = true;
                        nearestItem = spriteList.FindNearestItem(SearchDirectionHint.Right, locationInControl);
                    }
                }
                if (nearestItem != null)
                {
                    int nearestSprite = GetSprite(nearestItem).Number;
                    _currentFolder.Sprites = MoveSpritesIntoNewPositionInFolder(nearestSprite, putSpritesBeforeSelection, dragged);
                    RefreshSpriteDisplay();
                }
            }
            else if(e.Data.GetDataPresent(DataFormats.FileDrop))
            {
                string[] filePaths = (string[])e.Data.GetData(DataFormats.FileDrop, false);

                // don't block the UI thread, see asyncFileDropWorker_DoWork
                asyncFileDropWorker.RunWorkerAsync(filePaths);
            }
        }

        /// <summary>
        /// Moves the set of supplied sprites to be before (or after) the specified
        /// sprite in the folder. This takes account of the fact that NearestSprite
        /// could be one of the ones being moved.
        /// </summary>
        private List<Sprite> MoveSpritesIntoNewPositionInFolder(int nearestSprite, bool putSpritesBeforeSelection, SpriteManagerDragDropData dragged)
        {
            List<Sprite> newFolderContents = new List<Sprite>();
            foreach (Sprite sprite in _currentFolder.Sprites)
            {
                bool addThisSpriteToNewList = true;

                if (sprite.Number == nearestSprite)
                {
                    if (!putSpritesBeforeSelection)
                    {
                        newFolderContents.Add(sprite);
                        addThisSpriteToNewList = false;
                    }
                    foreach (Sprite draggedSprite in dragged.Sprites)
                    {
                        if ((draggedSprite.Number != sprite.Number) || (putSpritesBeforeSelection))
                        {
                            newFolderContents.Add(draggedSprite);
                        }
                    }
                }
                foreach (Sprite draggedSprite in dragged.Sprites)
                {
                    if (sprite.Number == draggedSprite.Number)
                    {
                        addThisSpriteToNewList = false;
                        break;
                    }
                }
                if (addThisSpriteToNewList)
                {
                    newFolderContents.Add(sprite);
                }
            }
            return newFolderContents;
        }

        private void spriteList_DragOver(object sender, DragEventArgs e)
        {
            if (e.Data.GetDataPresent(typeof(SpriteManagerDragDropData)))
            {
                e.Effect = DragDropEffects.Move;
            }
        }

        private void folderList_DragOver(object sender, DragEventArgs e)
        {
			TreeNode target;

            if (e.Data.GetDataPresent(typeof(SpriteManagerDragDropData)))
            {
				target = GetMouseOverTreeNode(e.X, e.Y);
				SetFolderListDropHighlight(target);
				if (target != null)
                {
					target.Expand();
                    e.Effect = DragDropEffects.Move;
                }
                else
                {
                    e.Effect = DragDropEffects.None;
                }
            }
        }

		private void SetFolderListDropHighlight(TreeNode target)
		{
			if (_dropHighlight != target)
			{
				if (_dropHighlight != null)
				{
					_dropHighlight.BackColor = Color.Empty;
					_dropHighlight.ForeColor = Color.Empty;
				}
				if (target != null)
				{
					folderList.HideSelection = target == folderList.SelectedNode;
					target.BackColor = SystemColors.Highlight;
					target.ForeColor = SystemColors.HighlightText;
				}
				else
					if (_dropHighlight == folderList.SelectedNode)
						folderList.HideSelection = false;
				_dropHighlight = target;
			}
		}

        private void RemoveSpritesFromFolder(SpriteFolder folder, List<Sprite> spritesToRemove)
        {
            foreach (Sprite draggedSprite in spritesToRemove)
            {
                folder.Sprites.Remove(draggedSprite);
            }
        }

        private void folderList_DragDrop(object sender, DragEventArgs e)
        {
            SpriteFolder draggedInto = GetMouseOverFolder(e.X, e.Y);
            SpriteManagerDragDropData dragged = (SpriteManagerDragDropData)e.Data.GetData(typeof(SpriteManagerDragDropData));
            RemoveSpritesFromFolder(_currentFolder, dragged.Sprites);
            foreach (Sprite draggedSprite in dragged.Sprites)
            {
                draggedInto.Sprites.Add(draggedSprite);
            }
			SetFolderListDropHighlight(null);
            RefreshSpriteDisplay();
        }

		private void folderList_DragLeave(object sender, EventArgs e)
		{
			SetFolderListDropHighlight(null);
		}

		private TreeNode GetMouseOverTreeNode(int screenX, int screenY)
        {
            Point locationInControl = folderList.PointToClient(new Point(screenX, screenY));

			return folderList.HitTest(locationInControl).Node;
		}

		private SpriteFolder GetMouseOverFolder(int screenX, int screenY)
		{
			TreeNode draggedIntoFolder = GetMouseOverTreeNode(screenX, screenY);
            if (draggedIntoFolder == null)
            {
                return null;
            }
            return _nodeFolderMapping[draggedIntoFolder];
        }

        private void LoadColorTheme(ColorTheme t)
        {
            t.ControlHelper(this, "sprite-selector");

            t.SetColor("sprite-selector/list/background", c => spriteList.BackColor = c);
            t.SetColor("sprite-selector/list/foreground", c => spriteList.ForeColor = c);
            t.SetColor("sprite-selector/tree/background", c => folderList.BackColor = c);
            t.SetColor("sprite-selector/tree/foreground", c => folderList.ForeColor = c);
            t.SetColor("sprite-selector/tree/line", c => folderList.LineColor = c);
            t.ButtonHelper(button_importNew, "sprite-selector/btn-import-new");
        }

        private void sliderPreviewSize_ValueChanged(object sender, EventArgs e)
        {
            SetSpritePreviewMultiplier(sliderPreviewSize.Value);
        }

        private void button_importNew_Click(object sender, EventArgs e)
        {
            string[] filenames = Factory.GUIController.ShowOpenFileDialogMultipleFiles("Import new sprites...", Constants.IMAGE_FILE_FILTER);

            if (filenames.Length > 0)
            {
                ImportNewSprite(_currentFolder, filenames);
            }
        }

        private void spriteList_DragEnter(object sender, DragEventArgs e)
        {
            if(e.Data.GetDataPresent(DataFormats.FileDrop))
            {
                e.Effect = DragDropEffects.Copy;
            }
            else if(e.Data.GetDataPresent(typeof(SpriteManagerDragDropData)))
            {
                e.Effect = DragDropEffects.Move;
            }
            else
            {
                e.Effect = DragDropEffects.None;
            }
        }

        private void spriteList_MouseWheel(object sender, MouseEventArgs e)
        {
            if (ModifierKeys.HasFlag(Keys.Control))
            {
                int movement = e.Delta;
                if (movement > 0)
                {
                    if (sliderPreviewSize.Value < sliderPreviewSize.Maximum)
                    {
                        sliderPreviewSize.Value++;
                    }
                }
                else
                {
                    if (sliderPreviewSize.Value > sliderPreviewSize.Minimum)
                    {
                        sliderPreviewSize.Value--;
                    }
                }
                SetSpritePreviewMultiplier(sliderPreviewSize.Value);
            }
        }

        protected override bool ProcessCmdKey(ref Message msg, Keys keyData)
        {
            if (keyData == (Keys.Control | Keys.D0))
            {
                SetSpritePreviewMultiplier(2);
                return true;
            }
            return base.ProcessCmdKey(ref msg, keyData);
        }

        private void asyncFileDropWorker_DoWork(object sender, DoWorkEventArgs e)
        {
            string[] filePaths = (string[])e.Argument; // from RunWorkerAsync

            string[] possiblyValidFiles = filePaths.Where(a =>
               a.EndsWith(".png", StringComparison.OrdinalIgnoreCase) ||
               a.EndsWith(".gif", StringComparison.OrdinalIgnoreCase) ||
               a.EndsWith(".bmp", StringComparison.OrdinalIgnoreCase) ||
               a.EndsWith(".jpg", StringComparison.OrdinalIgnoreCase) ||
               a.EndsWith(".tif", StringComparison.OrdinalIgnoreCase)).ToArray();

            e.Result = possiblyValidFiles;
        }

        private void asyncFileDropWorker_RunWorkerCompleted(object sender, RunWorkerCompletedEventArgs e)
        {
            string[] possiblyValidFiles = (string[])e.Result;

            if (possiblyValidFiles.Length > 0)
            {
                ImportNewSprite(_currentFolder, possiblyValidFiles);
            }            
        }

        private void SpriteSelector_Load(object sender, EventArgs e)
        {
            if (!DesignMode)
            {
                Factory.GUIController.ColorThemes.Apply(LoadColorTheme);
            }
        }

        private void projectTree_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.F2)
            {
                if (folderList.SelectedNode != null)
                    folderList.SelectedNode.BeginEdit();
            }
        }

        private void SpriteSelector_Leave(object sender, EventArgs e)
        {
            SelectionIdleHandler = false;
        }

        private void SpriteSelector_VisibleChanged(object sender, EventArgs e)
        {
            SelectionIdleHandler = false;
        }

        private void panel1_Layout(object sender, LayoutEventArgs e)
        {
            splitContainer1.SplitterDistance = 2 * (button_importNew.Font.Height) + button_importNew.Margin.Top + button_importNew.Margin.Bottom - 4;
        }
    }

    internal class SpriteManagerDragDropData
    {
        public List<Sprite> Sprites = new List<Sprite>();
    }
}
