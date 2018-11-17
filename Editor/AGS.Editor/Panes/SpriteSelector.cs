using Microsoft.Win32;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Windows.Forms;
using AGS.Types;
using AGS.Editor.Preferences;
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
        private const string MENU_ITEM_EXPORT_FOLDER = "ExportFolder";
        private const string MENU_ITEM_SORT_BY_NUMBER = "SortSpritesByNumber";
        private const string MENU_ITEM_REPLACE_FROM_SOURCE = "ReplaceSpriteFromSource";
        private const string MENU_ITEM_FIND_BY_NUMBER = "FindSpriteByNumber";

        private const string MENU_ITEM_USE_THIS_SPRITE = "UseThisSprite";
        private const string MENU_ITEM_EDIT_THIS_SPRITE = "EditThisSprite";
        private const string MENU_ITEM_COPY_TO_CLIPBOARD = "CopyToClipboard";
        private const string MENU_ITEM_EXPORT_SPRITE = "ExportSprite";
        private const string MENU_ITEM_REPLACE_FROM_FILE = "ReplaceFromFile";
        private const string MENU_ITEM_REPLACE_FROM_CLIPBOARD = "ReplaceFromClipboard";
        private const string MENU_ITEM_DELETE_SPRITE = "DeleteSprite";
        private const string MENU_ITEM_SHOW_USAGE = "ShowUsage";
        private const string MENU_ITEM_CROP_ASYMMETRIC = "CropAsymettric";
        private const string MENU_ITEM_CROP_SYMMETRIC = "CropSymettric";
        private const string MENU_ITEM_ASSIGN_TO_VIEW = "AssignToView";
        private const string MENU_ITEM_CHANGE_SPRITE_NUMBER = "ChangeSpriteNumber";

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

        public SpriteSelector()
        {
            InitializeComponent();
            Factory.GUIController.ColorThemes.Apply(LoadColorTheme);
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

        public Sprite SelectedSprite
        {
            get
            {
                if (spriteList.SelectedItems.Count == 1)
                {
                    return FindSpriteByNumber(Convert.ToInt32(spriteList.SelectedItems[0].Name));
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

        private void DisplaySpritesForFolder(SpriteFolder folder)
        {
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
            _spriteImages.ColorDepth = ColorDepth.Depth16Bit;
            _spriteImages.ImageSize = new Size(64, 64);
            _spriteImages.TransparentColor = Color.Pink;
            foreach (Sprite sprite in folder.Sprites)
            {
                Bitmap bmp = Utilities.GetBitmapForSpriteResizedKeepingAspectRatio(sprite, 64, 64, false, true, Color.Pink);
                _spriteImages.Images.Add(sprite.Number.ToString(), bmp);
                spriteList.Items.Add(sprite.Number.ToString(), sprite.Number.ToString(), sprite.Number.ToString());
            }
            spriteList.LargeImageList = _spriteImages;
            spriteList.EndUpdate();

            if (this.ParentForm != null)
            {
                this.ParentForm.Cursor = Cursors.Default;
                this.Cursor = Cursors.Default;
            }
        }

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
                if (Convert.ToInt32(listItem.Name) == spriteNumber)
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

        private Sprite CreateSpriteForBitmap(Bitmap bmp, SpriteImportTransparency method, bool remapColours, bool useRoomBackground, bool alphaChannel)
        {
            Sprite newSprite = Factory.NativeProxy.CreateSpriteFromBitmap(bmp, method, remapColours, useRoomBackground, alphaChannel);

            // added for sprite reload from source
            newSprite.TransparentColour = method;
            newSprite.RemapToGamePalette = remapColours;

            _currentFolder.Sprites.Add(newSprite);
            return newSprite;
        }

        private void ImportNewSprite(string filename)
        {
            ImportNewSprite(new string[] { filename });
        }

        private void ImportNewSprite(string[] filenames)
        {
            _lastImportedFilenames = filenames;
            SpriteImportWindow impWin = new SpriteImportWindow(filenames);

            if (impWin.ShowDialog() == DialogResult.OK)
            {
                foreach (string filename in filenames)
                {
                    int frame = 0;

                    foreach (Bitmap bmp in SpriteTools.LoadSpritesFromFile(filename))
                    {
                        frame ++;
                        bool useAlphaChannel = bmp.PixelFormat != PixelFormat.Format32bppArgb ? false : impWin.UseAlphaChannel;
                        SpriteImportTransparency method = impWin.SpriteImportMethod;

                        if (impWin.TiledImport)
                        {
                            foreach(Rectangle selection in SpriteTools.GetSpriteSelections(impWin.ImageSize, impWin.SelectionOffset,
                                impWin.SelectionSize, impWin.TilingMargin, impWin.TilingDirection, impWin.MaxTiles))
                            {
                                Bitmap import = bmp.Clone(selection, bmp.PixelFormat);
                                Sprite sprite = CreateSpriteForBitmap(import, method, impWin.RemapToGamePalette, impWin.UseBackgroundSlots, useAlphaChannel);
                                import.Dispose();

                                // set import options used for the sprite
                                sprite.TransparentColour = impWin.SpriteImportMethod;
                                sprite.OffsetX = selection.Left;
                                sprite.OffsetY = selection.Top;
                                sprite.RemapToGamePalette = impWin.RemapToGamePalette;
                                sprite.SourceFile = Utilities.GetRelativeToProjectPath(filename);
                                sprite.Frame = frame;
                            }
                        }
                        else
                        {
                            Sprite sprite = CreateSpriteForBitmap(bmp, method, impWin.RemapToGamePalette, impWin.UseBackgroundSlots, useAlphaChannel);
                            // set import options used for the sprite
                            sprite.TransparentColour = impWin.SpriteImportMethod;
                            sprite.OffsetX = impWin.TiledImport ? impWin.SelectionOffset.X : 0;
                            sprite.OffsetY = impWin.TiledImport ? impWin.SelectionOffset.Y : 0;
                            sprite.RemapToGamePalette = impWin.RemapToGamePalette;
                            sprite.SourceFile = Utilities.GetRelativeToProjectPath(filename);
                            sprite.Frame = frame;
                        }

                        bmp.Dispose();
                    }
                }

                RefreshSpriteDisplay();
            }

            impWin.Dispose();
        }

        private void ImportNewSprite(Bitmap bmp)
        {
            SpriteImportWindow impWin = new SpriteImportWindow(bmp);
            impWin.SpriteImportMethod = (SpriteImportTransparency)Factory.AGSEditor.Settings.SpriteImportMethod;

            if (impWin.ShowDialog() == DialogResult.OK)
            {
                bool useAlphaChannel = bmp.PixelFormat != PixelFormat.Format32bppArgb ? false : impWin.UseAlphaChannel;
                SpriteImportTransparency method = impWin.SpriteImportMethod;

                if (impWin.TiledImport)
                {
                    foreach (Rectangle selection in SpriteTools.GetSpriteSelections(impWin.ImageSize, impWin.SelectionOffset,
                        impWin.SelectionSize, impWin.TilingMargin, impWin.TilingDirection, impWin.MaxTiles))
                    {
                        Bitmap import = bmp.Clone(selection, bmp.PixelFormat);
                        Sprite sprite = CreateSpriteForBitmap(import, method, impWin.RemapToGamePalette, impWin.UseBackgroundSlots, useAlphaChannel);
                        import.Dispose();

                        // set import options used for the sprite
                        sprite.TransparentColour = impWin.SpriteImportMethod;
                        sprite.OffsetX = selection.Left;
                        sprite.OffsetY = selection.Top;
                        sprite.RemapToGamePalette = impWin.RemapToGamePalette;
                    }
                }
                else
                {
                    Sprite sprite = CreateSpriteForBitmap(bmp, method, impWin.RemapToGamePalette, impWin.UseBackgroundSlots, useAlphaChannel);

                    // set import options used for the sprite
                    sprite.TransparentColour = impWin.SpriteImportMethod;
                    sprite.OffsetX = impWin.TiledImport ? impWin.SelectionOffset.X : 0;
                    sprite.OffsetY = impWin.TiledImport ? impWin.SelectionOffset.Y : 0;
                    sprite.RemapToGamePalette = impWin.RemapToGamePalette;
                }

                RefreshSpriteDisplay();
            }

            impWin.Dispose();
        }

        private void ReplaceSprite(Sprite sprite, string filename)
        {
            _lastImportedFilenames = new string[] { filename };
            SpriteImportWindow impWin = new SpriteImportWindow(new string[] { filename });

            // get import options from the existing sprite
            impWin.SpriteImportMethod = sprite.TransparentColour;
            impWin.SelectionOffset = new Point(sprite.OffsetX, sprite.OffsetY);
            impWin.SelectionSize = new Size(sprite.Width, sprite.Height);
            impWin.UseAlphaChannel = sprite.AlphaChannel;
            impWin.RemapToGamePalette = sprite.RemapToGamePalette;

            if (impWin.ShowDialog() == DialogResult.OK)
            {
                Bitmap bmp = SpriteTools.LoadFirstImageFromFile(filename);
                Bitmap import;

                if (impWin.TiledImport)
                {
                    Rectangle selection = SpriteTools.GetFirstSpriteSelection(impWin.ImageSize, impWin.SelectionOffset,
                        impWin.SelectionSize, impWin.TilingMargin, impWin.TilingDirection, impWin.MaxTiles);
                    import = bmp.Clone(selection, bmp.PixelFormat);
                }
                else
                {
                    import = (Bitmap)bmp.Clone();
                }

                bmp.Dispose();
                bool useAlphaChannel = import.PixelFormat != PixelFormat.Format32bppArgb ? false : impWin.UseAlphaChannel;
                SpriteImportTransparency method = impWin.SpriteImportMethod;

                Factory.NativeProxy.ReplaceSpriteWithBitmap(sprite, import, method, impWin.RemapToGamePalette, impWin.UseBackgroundSlots, useAlphaChannel);
                import.Dispose();

                // set import options used for the sprite
                sprite.TransparentColour = impWin.SpriteImportMethod;
                sprite.OffsetX = impWin.TiledImport ? impWin.SelectionOffset.X : 0;
                sprite.OffsetY = impWin.TiledImport ? impWin.SelectionOffset.Y : 0;
                sprite.RemapToGamePalette = impWin.RemapToGamePalette;
                sprite.Frame = 1; // for direct replacement from a file we only ever take the first frame
                sprite.SourceFile = Utilities.GetRelativeToProjectPath(filename);

                RefreshSpriteDisplay();
            }

            impWin.Dispose();
        }

        private void ReplaceSprite(Sprite sprite, Bitmap bmp)
        {
            SpriteImportWindow impWin = new SpriteImportWindow(bmp);

            // get import options from the existing sprite
            impWin.SpriteImportMethod = sprite.TransparentColour;
            impWin.SelectionOffset = new Point(sprite.OffsetX, sprite.OffsetY);
            impWin.SelectionSize = new Size(sprite.Width, sprite.Height);
            impWin.UseAlphaChannel = sprite.AlphaChannel;
            impWin.RemapToGamePalette = sprite.RemapToGamePalette;

            if (impWin.ShowDialog() == DialogResult.OK)
            {
                Bitmap import;

                if (impWin.TiledImport)
                {
                    Rectangle selection = SpriteTools.GetFirstSpriteSelection(impWin.ImageSize, impWin.SelectionOffset,
                        impWin.SelectionSize, impWin.TilingMargin, impWin.TilingDirection, impWin.MaxTiles);
                    import = bmp.Clone(selection, bmp.PixelFormat);
                } else
                {
                    import = (Bitmap)bmp.Clone();
                }

                bool useAlphaChannel = import.PixelFormat != PixelFormat.Format32bppArgb ? false : impWin.UseAlphaChannel;
                SpriteImportTransparency method = impWin.SpriteImportMethod;

                Factory.NativeProxy.ReplaceSpriteWithBitmap(sprite, import, method, impWin.RemapToGamePalette, impWin.UseBackgroundSlots, useAlphaChannel);
                import.Dispose();

                // set import options used for the sprite
                sprite.TransparentColour = impWin.SpriteImportMethod;
                sprite.OffsetX = impWin.TiledImport ? impWin.SelectionOffset.X : 0;
                sprite.OffsetY = impWin.TiledImport ? impWin.SelectionOffset.Y : 0;
                sprite.RemapToGamePalette = impWin.RemapToGamePalette;
                sprite.Frame = 1; // for direct replacement from bmp there is only 1 frame

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
                        int spriteNum = Convert.ToInt32(listItem.Name);
                        Sprite sprite = FindSpriteByNumber(spriteNum);

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
                        ImportNewSprite(bmp);
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
                string[] filenames = Factory.GUIController.ShowOpenFileDialogMultipleFiles("Import new sprites...", GUIController.IMAGE_FILE_FILTER);

                if (filenames.Length > 0)
                {
                    ImportNewSprite(filenames);
                }
            }
            else if (item.Name == MENU_ITEM_NEW_FROM_PREVIOUS)
            {
                ImportNewSprite(_lastImportedFilenames);
            }
            else if (item.Name == MENU_ITEM_REPLACE_FROM_FILE)
            {
                string fileName = Factory.GUIController.ShowOpenFileDialog("Replace sprite...", GUIController.IMAGE_FILE_FILTER);
                if (fileName != null)
                {
                    Sprite sprite = FindSpriteByNumber(_spriteNumberOnMenuActivation);
                    ReplaceSpriteUsingImportWindow(fileName, sprite);
                }
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
            else if (item.Name == MENU_ITEM_DELETE_SPRITE)
            {
                DeleteSelectedSprites();
            }
            else if (item.Name == MENU_ITEM_EXPORT_FOLDER)
            {
                string exportFolder = Factory.GUIController.ShowSelectFolderOrNoneDialog("Export sprites to folder...", System.IO.Directory.GetCurrentDirectory());
                if (exportFolder != null)
                {
                    ExportAllSpritesInFolder(exportFolder);
                }
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
                string fileName = Factory.GUIController.ShowSaveFileDialog("Export sprite...", GUIController.IMAGE_FILE_FILTER);
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
                    if (Factory.GUIController.ShowQuestion("Your desktop colour depth is lower than this image. You may lose image detail if you copy this to the clipboard. Do you want to go ahead?") == DialogResult.Yes)
                    {
                        Clipboard.SetImage(bmp);
                    }
                }

                bmp.Dispose();
            }
            else if (item.Name == MENU_ITEM_CHANGE_SPRITE_NUMBER)
            {
                if (Factory.GUIController.ShowQuestion("Changing the sprite slot number is a specialized operation, for advanced users only.\n\nOnly re-number this sprite if you are ABSOLUTELY SURE it is not used AT ALL in your game. Any parts of your game that do use this sprite will cause the editor and engine to crash if you go ahead. Are you sure?") == DialogResult.Yes)
                {
                    string usage = SpriteTools.GetSpriteUsageReport(_spriteNumberOnMenuActivation, Factory.AGSEditor.CurrentGame);
                    if (usage != null)
                    {
                        Factory.GUIController.ShowMessage("Cannot change the sprite number because it is in use:" + Environment.NewLine + usage, MessageBoxIcon.Warning);
                        return;
                    }

                    Sprite sprite = FindSpriteByNumber(_spriteNumberOnMenuActivation);
                    int newNumber = NumberEntryDialog.Show("Change Sprite Number", "Enter the new sprite number in the box below:", sprite.Number, 0, NativeConstants.MAX_STATIC_SPRITES - 1);
                    if (newNumber == -1)
                    {
                        // Dialog cancelled
                    }
                    else if (Factory.NativeProxy.DoesSpriteExist(newNumber))
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
                        spriteNumbers.Add(Convert.ToInt32(selectedItem.Name.ToString()));
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
        }

        private void ReplaceSpritesFromSource()
        {

            List<Sprite> sprites = new List<Sprite>();
            foreach (ListViewItem listItem in spriteList.SelectedItems) //Check sources still exist
            {
                Sprite spr = FindSpriteByNumber(Convert.ToInt32(listItem.Name.ToString()));
                if (String.IsNullOrEmpty(spr.SourceFile))
                {
                    Factory.GUIController.ShowMessage(String.Format("Sprite {0} does not have a source file.", listItem.Name.ToString()), MessageBoxIcon.Error);
                    return;
                }
                else if (!File.Exists(spr.SourceFile))
                {
                    Factory.GUIController.ShowMessage(String.Format("File {0} does not exist.", spr.SourceFile), MessageBoxIcon.Error);
                    return;
                }
                sprites.Add(spr);
            }
            foreach (Sprite spr in sprites)
            {
                try
                {
                    Bitmap bmp = SpriteTools.LoadFrameImageFromFile(spr.SourceFile, spr.Frame);
                    Bitmap import;

                    // if offset would make a selection, use it
                    if (spr.OffsetX > 0 || spr.OffsetY > 0)
                    {
                        import = bmp.Clone(new Rectangle(spr.OffsetX, spr.OffsetY, spr.Width, spr.Height), bmp.PixelFormat);
                    }
                    else
                    {
                        import = (Bitmap)bmp.Clone();
                    }

                    bmp.Dispose();
                    bool alphaChannel = spr.AlphaChannel;
                    bool remap = spr.RemapToGamePalette;
                    SpriteImportTransparency method = spr.TransparentColour;
                    NativeProxy.Instance.ReplaceSpriteWithBitmap(spr, import, method, remap, false, alphaChannel);
                    import.Dispose();
                }
                catch (Exception ex)
                {
                    Factory.GUIController.ShowMessage(String.Format("There was an error importing the file, {0}. The error message was: '{1}' Please try again", spr.SourceFile, ex.Message), MessageBoxIcon.Warning);
                    RefreshSpriteDisplay();
                    return;
                }
            }
            RefreshSpriteDisplay();
            Factory.GUIController.ShowMessage("Import complete!", MessageBoxIcon.Information);

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
                    Factory.NativeProxy.ReplaceSpriteWithBitmap(sprite, newBmp,
                        SpriteImportTransparency.LeaveAsIs, false, false, sprite.AlphaChannel);
                    RefreshSpriteDisplay();
                }
                newBmp.Dispose();
            }
            bmp.Dispose();
            try
            {
                File.Delete(fileName);
            }
            catch (IOException ex)
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

        private object LaunchImageEditorThread(object parameter)
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
                    Sprite sprite = FindSpriteByNumber(Convert.ToInt32(listItem.Name));
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

        private void ExportAllSpritesInFolder(string exportToFolder)
        {
            try
            {
                foreach (Sprite sprite in _currentFolder.Sprites)
                {
                    Bitmap bmp = Factory.NativeProxy.GetBitmapForSprite(sprite.Number, sprite.Width, sprite.Height);
                    if ((sprite.ColorDepth < 32) && (!sprite.AlphaChannel))
                    {
                        bmp.Save(string.Format("{0}{1}spr{2:00000}.bmp", exportToFolder, Path.DirectorySeparatorChar, sprite.Number), ImageFormat.Bmp);
                    }
                    else
                    {
                        // export 32-bit images as PNG so no alpha channel is lost
                        bmp.Save(string.Format("{0}{1}spr{2:00000}.png", exportToFolder, Path.DirectorySeparatorChar, sprite.Number), ImageFormat.Png);
                    }
                    bmp.Dispose();
                }

                Factory.GUIController.ShowMessage("Sprites exported successfully.", MessageBoxIcon.Information);
            }
            catch (Exception ex)
            {
                Factory.GUIController.ShowMessage("There was an error exporting the files. The error message was: '" + ex.Message + "'. Please try again", MessageBoxIcon.Warning);
            }
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

        private void ReplaceSpriteUsingImportWindow(string fileName, Sprite sprite)
        {
            try
            {
                Bitmap bmp = SpriteTools.LoadFirstImageFromFile(fileName);
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
                _spriteNumberOnMenuActivation = Convert.ToInt32(itemAtLocation.Name);
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
                menu.Items.Add(new ToolStripMenuItem("Copy sprite to clipboard", null, onClick, MENU_ITEM_COPY_TO_CLIPBOARD));
                menu.Items.Add(new ToolStripMenuItem("Export sprite to file...", null, onClick, MENU_ITEM_EXPORT_SPRITE));
                menu.Items.Add(new ToolStripSeparator());
                menu.Items.Add(new ToolStripMenuItem("Replace sprite from file...", null, onClick, MENU_ITEM_REPLACE_FROM_FILE));

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

            menu.Items.Add(new ToolStripSeparator());
            menu.Items.Add(new ToolStripMenuItem("Export all sprites in folder...", null, onClick, MENU_ITEM_EXPORT_FOLDER));
            menu.Items.Add(new ToolStripMenuItem("Sort sprites by number", null, onClick, MENU_ITEM_SORT_BY_NUMBER));
            menu.Items.Add(new ToolStripSeparator());
            menu.Items.Add(new ToolStripMenuItem("Find sprite by number...", null, onClick, MENU_ITEM_FIND_BY_NUMBER));

            menu.Show(spriteList, menuPosition);
        }

        public Sprite FindSpriteByNumber(int spriteNum)
        {
            return _currentFolder.FindSpriteByID(spriteNum, false);
        }

        private void spriteList_ItemSelectionChanged(object sender, ListViewItemSelectionChangedEventArgs e)
        {
            // for multi-select, skip events generated by intermediate items by checking focus
            if (OnSelectionChanged != null && e.Item.Focused)
            {
                Sprite[] selectedSprites = new Sprite[spriteList.SelectedItems.Count];
                for (int i = 0; i < selectedSprites.Length; i++)
                {
                    selectedSprites[i] = FindSpriteByNumber(Convert.ToInt32(spriteList.SelectedItems[i].Name));
                    if (selectedSprites[i] == null)
                    {
                        throw new AGSEditorException("Internal error: selected sprite not in folder");
                    }
                }
                OnSelectionChanged(selectedSprites);
            }
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
                dragDropData.Sprites.Add(FindSpriteByNumber(Convert.ToInt32(selectedItem.Name)));
            }

            this.DoDragDrop(dragDropData, DragDropEffects.Move);
        }

        private void spriteList_DragDrop(object sender, DragEventArgs e)
        {
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
                int nearestSprite = Convert.ToInt32(nearestItem.Text);
                _currentFolder.Sprites = MoveSpritesIntoNewPositionInFolder(nearestSprite, putSpritesBeforeSelection, dragged);
                RefreshSpriteDisplay();
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
            BackColor = t.GetColor("sprite-selector/background");
            ForeColor = t.GetColor("sprite-selector/foreground");
            spriteList.BackColor = t.GetColor("sprite-selector/list/background");
            spriteList.ForeColor = t.GetColor("sprite-selector/list/foreground");
            folderList.BackColor = t.GetColor("sprite-selector/tree/background");
            folderList.ForeColor = t.GetColor("sprite-selector/tree/foreground");
            folderList.LineColor = t.GetColor("sprite-selector/tree/line");
        }
    }

    internal class SpriteManagerDragDropData
    {
        public List<Sprite> Sprites = new List<Sprite>();
    }
}
