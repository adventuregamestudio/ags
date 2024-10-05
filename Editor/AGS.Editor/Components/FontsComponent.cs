using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Xml;
using AGS.Types;

namespace AGS.Editor.Components
{
    class FontsComponent : BaseComponent
    {
        private const string TOP_LEVEL_COMMAND_ID = "Fonts";
        private const string FONT_FILES_FOLDER_NODE_ID = "FontFilesFolderNode";
        private const string FONTS_FOLDER_NODE_ID = "FontsFolderNode";
        private const string FONT_FILE_ITEM_PREFIX = "FntFile:";
        private const string FONT_ITEM_PREFIX = "Fnt:";
        private const string FONT_ITEM_REF_PREFIX = "Fntref:";

        private const string COMMAND_NEW_FONTFILE = "NewFontFile";
        private const string COMMAND_DELETE_FONTFILE = "DeleteFontFile";
        private const string COMMAND_NEW_FONT = "NewFont";
        private const string COMMAND_NEW_FONT_FOR_FILE = "NewFontForFile";
        private const string COMMAND_DELETE_FONT = "DeleteFont";
        private const string ICON_KEY = "FontsIcon";

        private const string FONT_FILES_FILTER = "All supported fonts (*.ttf; *.wfn)|*.ttf;*.wfn|True-type font (*.ttf)|*.ttf|AGS Bitmap font (*.wfn)|*.wfn";
        private const int DEFAULT_IMPORTED_FONT_SIZE = 10;


        private Dictionary<AGS.Types.Font, ContentDocument> _documents;
        private object _itemRightClicked = null;

        public FontsComponent(GUIController guiController, AGSEditor agsEditor)
            : base(guiController, agsEditor)
        {
            _documents = new Dictionary<AGS.Types.Font, ContentDocument>();
            _guiController.RegisterIcon(ICON_KEY, Resources.ResourceManager.GetIcon("font.ico"));
            _guiController.RegisterIcon("FontIcon", Resources.ResourceManager.GetIcon("font-item.ico"));
            _guiController.ProjectTree.AddTreeRoot(this, TOP_LEVEL_COMMAND_ID, "Fonts", ICON_KEY);
            Factory.Events.GamePostLoad += ConvertAllFontsToFontAndFilePairs;
            FontSizeUIEditor.FontSizeGUI = FontImportSize;
            RePopulateTreeView();
        }

        public override string ComponentID
        {
            get { return ComponentIDs.Fonts; }
        }

        public override void CommandClick(string controlID)
        {
            if (controlID == COMMAND_NEW_FONTFILE)
            {
                AddNewFontFile(_agsEditor.CurrentGame);
            }
            else if (controlID == COMMAND_NEW_FONT)
            {
                AddNewFont(_agsEditor.CurrentGame);
            }
            else if (controlID == COMMAND_NEW_FONT_FOR_FILE)
            {
                AddNewFont(_agsEditor.CurrentGame, _itemRightClicked as AGS.Types.FontFile, true);
            }
            else if (controlID == COMMAND_DELETE_FONTFILE)
            {
                if (MessageBox.Show("Are you sure you want to remove this font file from the project? Doing so will also remove a SourceFile from all the Fonts that were using it.", "Confirm delete", MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.Yes)
                {
                    DeleteFontFile(_agsEditor.CurrentGame, _itemRightClicked as AGS.Types.FontFile);
                }
            }
            else if (controlID == COMMAND_DELETE_FONT)
            {
                if (MessageBox.Show("Are you sure you want to delete this font? Doing so could break any scripts that refer to fonts by number.", "Confirm delete", MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.Yes)
                {
                    DeleteFont(_agsEditor.CurrentGame, _itemRightClicked as AGS.Types.Font);
                }
            }
            else
            {
                if (controlID.StartsWith(FONT_FILE_ITEM_PREFIX))
                {
                    string fontFileName = controlID.Substring(FONT_FILE_ITEM_PREFIX.Length);
                    FontFile chosenFontFile = FindFontFileByName(fontFileName);
                    ShowOrAddPane(chosenFontFile);
                }
                else if (controlID.StartsWith(FONT_ITEM_PREFIX))
                {
                    AGS.Types.Font chosenFont = _agsEditor.CurrentGame.Fonts[Convert.ToInt32(controlID.Substring(FONT_ITEM_PREFIX.Length))];
                    ShowOrAddPane(chosenFont);
                }
                else if (controlID.StartsWith(FONT_ITEM_REF_PREFIX))
                {
                    AGS.Types.Font chosenFont = _agsEditor.CurrentGame.Fonts[Convert.ToInt32(controlID.Substring(FONT_ITEM_REF_PREFIX.Length))];
                    ShowOrAddPane(chosenFont);
                }
            }
        }

        private FontFile FindFontFileByName(string fontFileName)
        {
            return _agsEditor.CurrentGame.FontFiles.Where(ff => ff.FileName == fontFileName).FirstOrDefault();
        }

        private void ShowOrAddPane(FontFile chosenFontFile)
        {

        }

        private void ShowOrAddPane(AGS.Types.Font chosenFont)
        {
            ContentDocument document;
            if (!_documents.TryGetValue(chosenFont, out document)
                || document.Control.IsDisposed)
            {
                Dictionary<string, object> list = new Dictionary<string, object>();
                list.Add(chosenFont.Name + " (Font " + chosenFont.ID + ")", chosenFont);

                FontEditor editor = new FontEditor(chosenFont);
                document = new ContentDocument(editor,
                    chosenFont.WindowTitle, this, ICON_KEY, list);
                _documents[chosenFont] = document;
                document.SelectedPropertyGridObject = chosenFont;
            }
            document.TreeNodeID = GetNodeID(chosenFont);
            _guiController.AddOrShowPane(document);
        }

        public override IList<string> GetManagedScriptElements()
        {
            return new string[] { "FontType" };
        }

        public override void ShowItemPaneByName(string name)
        {
            foreach(AGS.Types.Font f in _agsEditor.CurrentGame.Fonts)
            {
                if(f.ScriptID == name)
                {
                    AGS.Types.Font chosenFont = f;
                    _guiController.ProjectTree.SelectNode(this, GetNodeID(chosenFont));
                    ShowOrAddPane(chosenFont);
                    return;
                }
            }
        }

        public override void PropertyChanged(string propertyName, object oldValue)
        {
            FontEditor editor = ((FontEditor)_guiController.ActivePane.Control);
            AGS.Types.Font itemBeingEdited = editor.ItemToEdit;

            if (propertyName == "Name")
            {
                RePopulateTreeView(GetNodeID(itemBeingEdited));

                foreach (ContentDocument doc in _documents.Values)
                {
                    doc.Name = ((FontEditor)doc.Control).ItemToEdit.WindowTitle;
                }

                FontTypeConverter.SetFontList(_agsEditor.CurrentGame.Fonts);
                return;
            }

            if (propertyName == "Source FontFile")
            {
                RePopulateTreeView(GetNodeID(itemBeingEdited));

                FontFile ff = FindFontFileByName(itemBeingEdited.SourceFilename);
                if (ff != null)
                {
                    if (ff.FileFormat == FontFileFormat.TTF && itemBeingEdited.PointSize == 0)
                        itemBeingEdited.PointSize = DEFAULT_IMPORTED_FONT_SIZE;
                }
            }

            bool shouldRepaint = (propertyName == "Source FontFile" || propertyName == "Font Size" || propertyName == "SizeMultiplier");
            Factory.NativeProxy.OnFontUpdated(Factory.AGSEditor.CurrentGame, itemBeingEdited.ID, false);
            if (shouldRepaint)
                editor.OnFontUpdated();
        }

        public override IList<MenuCommand> GetContextMenu(string controlID)
        {
            if (controlID == TOP_LEVEL_COMMAND_ID)
            {
                return null;
            }

            IList<MenuCommand> menu = new List<MenuCommand>();
            
            if (controlID == FONT_FILES_FOLDER_NODE_ID)
            {
                menu.Add(new MenuCommand(COMMAND_NEW_FONTFILE, "New Font File", null));
            }
            else if (controlID == FONTS_FOLDER_NODE_ID)
            {
                menu.Add(new MenuCommand(COMMAND_NEW_FONT, "New Font", null));
            }
            else if (controlID.StartsWith(FONT_FILE_ITEM_PREFIX))
            {
                string fontFileName = controlID.Substring(FONT_FILE_ITEM_PREFIX.Length);
                _itemRightClicked = FindFontFileByName(fontFileName);
                menu.Add(new MenuCommand(COMMAND_NEW_FONT_FOR_FILE, "New Font Style", null));
                menu.Add(new MenuCommand(COMMAND_DELETE_FONTFILE, "Delete this font file", null));
            }
            else if (controlID.StartsWith(FONT_ITEM_PREFIX))
            {
                int fontID = Convert.ToInt32(controlID.Substring(FONT_ITEM_PREFIX.Length));
                _itemRightClicked = _agsEditor.CurrentGame.Fonts[fontID];
                menu.Add(new MenuCommand(COMMAND_DELETE_FONT, "Delete this font", null));
            }
            else if (controlID.StartsWith(FONT_ITEM_REF_PREFIX))
            {
                int fontID = Convert.ToInt32(controlID.Substring(FONT_ITEM_REF_PREFIX.Length));
                _itemRightClicked = _agsEditor.CurrentGame.Fonts[fontID];
                menu.Add(new MenuCommand(COMMAND_DELETE_FONT, "Delete this font", null));
            }
            return menu;
        }

        public override void RefreshDataFromGame()
        {
            foreach (ContentDocument doc in _documents.Values)
            {
                _guiController.RemovePaneIfExists(doc);
                doc.Dispose();
            }
            _documents.Clear();

            RePopulateTreeView();
            FontFileTypeConverter.SetFontFileList(_agsEditor.CurrentGame.FontFiles);
            FontTypeConverter.SetFontList(_agsEditor.CurrentGame.Fonts);
        }

        private string GetNodeID(AGS.Types.FontFile item)
        {
            return FONT_FILE_ITEM_PREFIX + item.FileName;
        }

        private string GetNodeID(AGS.Types.Font item, bool isRefNode = false)
        {
            return isRefNode ?
                FONT_ITEM_REF_PREFIX + item.ID :
                FONT_ITEM_PREFIX + item.ID;
        }

        private void AddFontNode(AGS.Types.Font item)
        {
            // Add font node in the dedicated folder
            _guiController.ProjectTree.StartFromNode(this, FONTS_FOLDER_NODE_ID);
            _guiController.ProjectTree.AddTreeLeaf(this, GetNodeID(item), item.ID.ToString() + ": " + item.Name, "FontIcon");

            // Add font node as a sub-node to the FontFile
            // NOTE: this meant as an UI experiment, review later
            if (item.SourceFilename != null)
            {
                FontFile ff = FindFontFileByName(item.SourceFilename);
                if (ff != null)
                {
                    _guiController.ProjectTree.StartFromNode(this, GetNodeID(ff));
                    _guiController.ProjectTree.AddTreeLeaf(this, GetNodeID(item, true), item.ID.ToString() + ": " + item.Name, "FontIcon");
                }
            }
        }

        private void RePopulateTreeView(string selectedNodeID = null)
        {
            bool restoreNodeStates = !string.IsNullOrEmpty(selectedNodeID);
            List<string> savedExpansionState = null;
            if (restoreNodeStates)
                savedExpansionState = _guiController.ProjectTree.GetExpansionState();

            _guiController.ProjectTree.BeginUpdate();
            _guiController.ProjectTree.RemoveAllChildNodes(this, TOP_LEVEL_COMMAND_ID);
            _guiController.ProjectTree.StartFromNode(this, TOP_LEVEL_COMMAND_ID);

            _guiController.ProjectTree.AddTreeBranch(this, FONT_FILES_FOLDER_NODE_ID, "Font Files", "GenericFolderIcon");
            foreach (AGS.Types.FontFile item in _agsEditor.CurrentGame.FontFiles)
            {
                _guiController.ProjectTree.AddTreeLeaf(this, GetNodeID(item), item.FileName.ToString(), "FontIcon");
            }

            _guiController.ProjectTree.StartFromNode(this, TOP_LEVEL_COMMAND_ID);
            _guiController.ProjectTree.AddTreeBranch(this, FONTS_FOLDER_NODE_ID, "Fonts", "GenericFolderIcon");
            foreach (AGS.Types.Font item in _agsEditor.CurrentGame.Fonts)
            {
                AddFontNode(item);
            }

            if (restoreNodeStates)
                _guiController.ProjectTree.SetExpansionState(savedExpansionState);
            _guiController.ProjectTree.EndUpdate();

            if (!string.IsNullOrEmpty(selectedNodeID))
                _guiController.ProjectTree.SelectNode(this, selectedNodeID);
        }

        private void AddNewFontFile(Game game)
        {
            string selectedFile = _guiController.ShowOpenFileDialog("Select font file to add", FONT_FILES_FILTER);
            if (string.IsNullOrEmpty(selectedFile))
                return;

            // FIXME: store fonts in project in their own subfolder
            selectedFile = Utilities.GetRelativeToProjectPath(selectedFile);
            string destFile = selectedFile;
            if (Path.IsPathRooted(selectedFile))
            {
                destFile = Path.Combine(game.DirectoryPath, Path.GetFileName(selectedFile));
            }
            if (File.Exists(destFile))
            {
                _guiController.ShowMessage("The file of this name already exists in the project folder.", MessageBoxIconType.Warning);
                return; // TODO: suggest copy renamed?
            }
            if (destFile != selectedFile)
            {
                File.Copy(selectedFile, destFile);
            }

            IList<FontFile> items = game.FontFiles;
            FontFile newItem = new FontFile();
            newItem.FileName = Path.GetFileName(selectedFile);
            newItem.SourceFilename = selectedFile;
            if (newItem.FileName.EndsWith(".wfn"))
                newItem.FileFormat = FontFileFormat.WFN;
            else if(newItem.FileName.EndsWith(".ttf"))
                newItem.FileFormat = FontFileFormat.TTF;
            items.Add(newItem);
            _guiController.ProjectTree.StartFromNode(this, FONT_FILES_FOLDER_NODE_ID);
            _guiController.ProjectTree.AddTreeLeaf(this, GetNodeID(newItem), newItem.FileName, "FontIcon");
            _guiController.ProjectTree.SelectNode(this, GetNodeID(newItem));
            ShowOrAddPane(newItem);
            FontFileTypeConverter.SetFontFileList(game.FontFiles);
        }

        private void AddNewFont(Game game, FontFile sourceFontFile = null, bool selectRefNode = false)
        {
            IList<AGS.Types.Font> items = game.Fonts;
            AGS.Types.Font newItem = new AGS.Types.Font();
            newItem.ID = items.Count;
            newItem.Name = "Font " + newItem.ID;
            newItem.OutlineStyle = FontOutlineStyle.None;
            if (items.Count > 0)
            {
                newItem.SourceFilename = Utilities.GetRelativeToProjectPath(items[0].SourceFilename);
                newItem.PointSize = items[0].PointSize;
            }
            if (sourceFontFile != null)
            {
                newItem.SourceFilename = sourceFontFile.FileName;
            }
            if (newItem.PointSize == 0)
            {
                newItem.PointSize = DEFAULT_IMPORTED_FONT_SIZE;
            }
            newItem.TTFMetricsFixup = game.Settings.TTFMetricsFixup; // use defaults
            items.Add(newItem);
            Factory.NativeProxy.OnFontAdded(game, newItem.ID);
            AddFontNode(newItem);
            _guiController.ProjectTree.SelectNode(this, GetNodeID(newItem, selectRefNode));
            ShowOrAddPane(newItem);
            FontTypeConverter.SetFontList(game.Fonts);
        }

        private void DeleteFontFile(Game game, FontFile removedFile)
        {
            foreach (AGS.Types.Font font in game.Fonts)
            {
                if (font.SourceFilename == removedFile.FileName)
                {
                    font.SourceFilename = string.Empty;
                    Factory.NativeProxy.OnFontUpdated(game, font.ID, true);
                }
            }

            _agsEditor.DeleteFileOnDisk(removedFile.FileName);
            game.FontFiles.Remove(removedFile);
            game.FilesAddedOrRemoved = true;
            RePopulateTreeView(FONT_FILES_FOLDER_NODE_ID);
            FontFileTypeConverter.SetFontFileList(game.FontFiles);
        }

        private void DeleteFont(Game game, AGS.Types.Font removedFont)
        {
            int removingID = removedFont.ID;
            foreach (AGS.Types.Font item in _agsEditor.CurrentGame.Fonts)
            {
                if (item.ID > removingID)
                {
                    item.ID--;
                }
            }
            if (_documents.ContainsKey(removedFont))
            {
                _guiController.RemovePaneIfExists(_documents[removedFont]);
                _documents.Remove(removedFont);
            }
            game.Fonts.Remove(removedFont);
            Factory.NativeProxy.OnFontDeleted(game, removingID);
            RePopulateTreeView(FONTS_FOLDER_NODE_ID);
            FontTypeConverter.SetFontList(game.Fonts);
        }

        private int FontImportSize(AGS.Types.Font font)
        {
            // TODO: find a better solution for the font format check, perhaps store font type as Font's property,
            // or add a direct (unserialized) reference to related FontFile
            if (!font.SourceFilename.EndsWith(".ttf"))
            {
                _guiController.ShowMessage("You can only directly modify Font Size for TTF fonts. For bitmap fonts try adjusting Size Multiplier instead.", MessageBoxIconType.Information);
                return font.PointSize;
            }

            FontHeightDefinition sizeType;
            int sizeValue;
            if (!ImportTTFDialog.Show(out sizeType, out sizeValue,
                    font.PointSize > 0 ? font.PointSize : DEFAULT_IMPORTED_FONT_SIZE, Int32.MaxValue))
                return font.PointSize;

            // If user asked to get a certain pixel height, then try to change the font's
            // point size, until found the closest result
            if (sizeType == FontHeightDefinition.PixelHeight)
            {
                sizeValue = Factory.NativeProxy.FindTTFSizeForHeight(font.SourceFilename, sizeValue);
            }

            return sizeValue;
        }

        private void ConvertAllFontsToFontAndFilePairs(Game game)
        {
#pragma warning disable 0612, 0618
            if (_agsEditor.CurrentGame.SavedXmlVersionIndex >= AGSEditor.AGS_4_0_0_XML_VERSION_INDEX_FONT_SOURCES)
                return; // Upgrade already completed

            foreach (AGS.Types.Font font in game.Fonts)
            {
                FontFile ff = new FontFile();
                if (File.Exists(font.TTFFileName))
                {
                    ff.FileName = font.TTFFileName;
                    ff.FileFormat = FontFileFormat.TTF;
                }
                else if (File.Exists(font.WFNFileName))
                {
                    ff.FileName = font.WFNFileName;
                    ff.FileFormat = FontFileFormat.TTF;
                }

                ff.SourceFilename = font.SourceFilename;
                game.FontFiles.Add(ff);

                font.SourceFilename = ff.FileName;
            }
#pragma warning restore 0612, 0618
        }
    }
}
