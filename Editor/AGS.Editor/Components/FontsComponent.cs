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
        public const string FONT_FILES_DIRECTORY = "Fonts";

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
            _guiController.RegisterIcon("FontFileIcon", Resources.ResourceManager.GetIcon("font-file.ico"));
            _guiController.RegisterIcon(ICON_KEY, Resources.ResourceManager.GetIcon("font.ico"));
            _guiController.RegisterIcon("FontIcon", Resources.ResourceManager.GetIcon("font-item.ico"));
            _guiController.ProjectTree.AddTreeRoot(this, TOP_LEVEL_COMMAND_ID, "Fonts", ICON_KEY);
            Factory.Events.GamePrepareUpgrade += Events_GamePrepareUpgrade;
            Factory.Events.GamePostLoad += PostGameLoadInit;
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
            // TODO: add a FontFile preview pane
        }

        private void ShowOrAddPane(AGS.Types.Font chosenFont)
        {
            ContentDocument document;
            if (!_documents.TryGetValue(chosenFont, out document)
                || document.Control.IsDisposed)
            {
                Dictionary<string, object> list = new Dictionary<string, object>();
                list.Add(chosenFont.PropertyGridTitle, chosenFont);

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

        public override bool ShowItemPaneByName(string name)
        {
            foreach(AGS.Types.Font f in _agsEditor.CurrentGame.Fonts)
            {
                if(f.ScriptID == name)
                {
                    AGS.Types.Font chosenFont = f;
                    _guiController.ProjectTree.SelectNode(this, GetNodeID(chosenFont));
                    ShowOrAddPane(chosenFont);
                    return true;
                }
            }
            return false;
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

                UpdateFont(itemBeingEdited); // update font which has a new source file
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
                menu.Add(new MenuCommand(COMMAND_NEW_FONT_FOR_FILE, "New Font", null));
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

        private string GetNodeLabel(AGS.Types.FontFile item)
        {
            return item.FileName;
        }

        private string GetNodeID(AGS.Types.Font item, bool isRefNode = false)
        {
            return isRefNode ?
                FONT_ITEM_REF_PREFIX + item.ID :
                FONT_ITEM_PREFIX + item.ID;
        }

        private string GetNodeLabel(AGS.Types.Font item)
        {
            return item.ID.ToString() + ": " + item.Name;
        }

        private void AddFontNode(AGS.Types.Font item)
        {
            // Add font node in the dedicated folder
            _guiController.ProjectTree.StartFromNode(this, FONTS_FOLDER_NODE_ID);
            _guiController.ProjectTree.AddTreeLeaf(this, GetNodeID(item), GetNodeLabel(item), "FontIcon");

            // Add font node as a sub-node to the FontFile
            // NOTE: this meant as an UI experiment, review later
            if (item.FontFileName != null)
            {
                FontFile ff = FindFontFileByName(item.FontFileName);
                if (ff != null)
                {
                    _guiController.ProjectTree.StartFromNode(this, GetNodeID(ff));
                    _guiController.ProjectTree.AddTreeLeaf(this, GetNodeID(item, true), GetNodeLabel(item), "FontIcon");
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

            _guiController.ProjectTree.AddTreeBranch(this, FONT_FILES_FOLDER_NODE_ID, "Font Files", "FixedFolderIcon");
            foreach (AGS.Types.FontFile item in _agsEditor.CurrentGame.FontFiles)
            {
                _guiController.ProjectTree.AddTreeLeaf(this, GetNodeID(item), GetNodeLabel(item), "FontFileIcon");
            }

            _guiController.ProjectTree.StartFromNode(this, TOP_LEVEL_COMMAND_ID);
            _guiController.ProjectTree.AddTreeBranch(this, FONTS_FOLDER_NODE_ID, "Fonts", "FixedFolderIcon");
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

        private FontFile NewFontFile(string filename, string sourceFilename = null)
        {
            if (sourceFilename == null)
                sourceFilename = filename;
            FontFile newItem = new FontFile();
            newItem.FileName = Path.GetFileName(filename);
            newItem.SourceFilename = sourceFilename;
            if (newItem.FileName.EndsWith(".wfn", StringComparison.OrdinalIgnoreCase))
                newItem.FileFormat = FontFileFormat.WFN;
            else if (newItem.FileName.EndsWith(".ttf", StringComparison.OrdinalIgnoreCase))
                newItem.FileFormat = FontFileFormat.TTF;
            return newItem;
        }

        private void AddNewFontFile(Game game)
        {
            string selectedFile = _guiController.ShowOpenFileDialog("Select font file to add", FONT_FILES_FILTER);
            if (string.IsNullOrEmpty(selectedFile))
                return;

            string fontsDirectory = Path.Combine(game.DirectoryPath, FONT_FILES_DIRECTORY);
            string fontFileName = Path.GetFileName(selectedFile);
            string destFilePath = Path.Combine(game.DirectoryPath, FONT_FILES_DIRECTORY, fontFileName);

            if (!Directory.Exists(FONT_FILES_DIRECTORY))
            {
                Directory.CreateDirectory(FONT_FILES_DIRECTORY);
            }

            // Test if they selected a file which is already registered
            if (Utilities.PathIsParentOf(fontsDirectory, selectedFile))
            {
                if (FindFontFileByName(fontFileName) != null)
                {
                    _guiController.ShowMessage("Selected font file is already added to the project.", MessageBoxIconType.Warning);
                    return;
                }
            }
            // Test if a file of such name already exists
            else if (File.Exists(destFilePath))
            {
                if (_guiController.ShowQuestion("The font file of the same name already exists in the project folder. Would you like to still add this file (the name will be changed)?", MessageBoxIcon.Warning)
                    == System.Windows.Forms.DialogResult.No)
                {
                    return;
                }
                fontFileName = Utilities.MakeUniqueFileName(fontsDirectory, fontFileName, "-", 2);
                destFilePath = Path.Combine(game.DirectoryPath, FONT_FILES_DIRECTORY, fontFileName);
            }

            if (!Utilities.PathsAreEqual(selectedFile, destFilePath))
            {
                File.Copy(selectedFile, destFilePath);
            }

            IList<FontFile> items = game.FontFiles;
            FontFile newItem = NewFontFile(fontFileName, Utilities.GetRelativeToProjectPath(selectedFile));
            items.Add(newItem);
            _guiController.ProjectTree.StartFromNode(this, FONT_FILES_FOLDER_NODE_ID);
            _guiController.ProjectTree.AddTreeLeaf(this, GetNodeID(newItem), GetNodeLabel(newItem), "FontFileIcon");
            _guiController.ProjectTree.SelectNode(this, GetNodeID(newItem));
            ShowOrAddPane(newItem);
            FontFileTypeConverter.SetFontFileList(game.FontFiles);
        }

        private void UpdateFontFile(FontFile fontFile)
        {
            // NOTE: if we decide to get format by loading and testing actual file contents,
            // then this may be a good place to do that.
            if (fontFile.FileName.EndsWith(".wfn", StringComparison.OrdinalIgnoreCase))
                fontFile.FileFormat = FontFileFormat.WFN;
            else if (fontFile.FileName.EndsWith(".ttf", StringComparison.OrdinalIgnoreCase))
                fontFile.FileFormat = FontFileFormat.TTF;
        }

        private AGS.Types.Font NewFont(Game game, int id, string fontName, FontFile sourceFontFile)
        {
            AGS.Types.Font newItem = new AGS.Types.Font();
            newItem.ID = id;
            newItem.Name = fontName;
            newItem.OutlineStyle = FontOutlineStyle.None;
            newItem.PointSize = 0;
            if (sourceFontFile != null)
            {
                AssignFontFileToFont(newItem, sourceFontFile);
            }
            newItem.TTFMetricsFixup = game.Settings.TTFMetricsFixup; // use defaults
            return newItem;
        }

        private void AssignFontFileToFont(AGS.Types.Font font, FontFile fontFile)
        {
            font.FontFile = fontFile;
            if (fontFile != null)
            {
                font.FontFileName = fontFile.FileName;
                font.SourceFilename = fontFile.SourceFilename;
                font.FamilyName = fontFile.FamilyName;
                if (fontFile.FileFormat == FontFileFormat.TTF && font.PointSize == 0)
                    font.PointSize = DEFAULT_IMPORTED_FONT_SIZE;
                else if (fontFile.FileFormat == FontFileFormat.WFN)
                    font.PointSize = 0; // CHECKME: should keep PointSize instead, just in case they switch back to TTF?
            }
            else
            {
                font.FontFileName = string.Empty;
                font.SourceFilename = string.Empty;
                font.FamilyName = string.Empty;
                font.PointSize = 0;
            }
        }

        private void UpdateFont(AGS.Types.Font font)
        {
            AssignFontFileToFont(font, FindFontFileByName(font.FontFileName));
        }

        private void AddNewFont(Game game, FontFile sourceFontFile = null, bool selectRefNode = false)
        {
            IList<AGS.Types.Font> items = game.Fonts;
            AGS.Types.Font newItem = NewFont(game, items.Count, "Font " + items.Count, sourceFontFile);
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
                if (font.FontFileName == removedFile.FileName)
                {
                    AssignFontFileToFont(font, null);
                    Factory.NativeProxy.OnFontUpdated(game, font.ID, true);
                }
            }

            _agsEditor.DeleteFileOnDisk(Path.Combine(FONT_FILES_DIRECTORY, removedFile.FileName));
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
            if (!(font.FontFile != null && font.FontFile.FileFormat == FontFileFormat.TTF))
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
                sizeValue = Factory.NativeProxy.FindTTFSizeForHeight(font.FontFileName, sizeValue);
            }

            return sizeValue;
        }

        private void Events_GamePrepareUpgrade(UpgradeGameEventArgs args)
        {
            args.Tasks.Add(new UpgradeGameFontsToFontFilesTask(ConvertAllFontsToFontAndFilePairs));
        }

        private void PostGameLoadInit(Game game)
        {
            AddDefaultFontsIfMissing(game);

            // Update deserialized FontFiles, setup non-serialized properties, sync with assets on disk etc
            foreach (FontFile ff in game.FontFiles)
            {
                UpdateFontFile(ff);
            }

            // Bind all deserialized Fonts to their FontFiles
            foreach (AGS.Types.Font font in game.Fonts)
            {
                UpdateFont(font);
            }
        }

        /// <summary>
        /// Ensures that the game has at least a single fontfile and font,
        /// so that it could display text.
        /// </summary>
        private void AddDefaultFontsIfMissing(Game game)
        {
            if (!Directory.Exists(FONT_FILES_DIRECTORY))
            {
                Directory.CreateDirectory(FONT_FILES_DIRECTORY);
            }

            if (game.FontFiles.Count == 0)
            {
                string destFilepath = Path.Combine(FONT_FILES_DIRECTORY, "AGSFNT0.WFN");
                if (!File.Exists(destFilepath))
                    Resources.ResourceManager.CopyFileFromResourcesToDisk("AGSFNT0.WFN", destFilepath);
                FontFile fontfile = NewFontFile("AGSFNT0.WFN");
                game.FontFiles.Add(fontfile);
            }
            if (game.Fonts.Count == 0)
            {
                AGS.Types.Font font = NewFont(game, 0, "Default", game.FontFiles[0]);
                game.Fonts.Add(font);
            }
        }

        private void ConvertAllFontsToFontAndFilePairs(Game game, IWorkProgress progress, CompileMessages errors)
        {
            if (_agsEditor.CurrentGame.SavedXmlVersionIndex >= AGSEditor.AGS_4_0_0_XML_VERSION_INDEX_FONT_SOURCES)
                return; // already converted

            // If the fonts directory we want to write to already exists then backup
            if (Directory.Exists(FONT_FILES_DIRECTORY))
            {
                string backupRootDir = Utilities.MakeUniqueDirectory(_agsEditor.CurrentGame.DirectoryPath, FONT_FILES_DIRECTORY, "Backup-");
                Utilities.SafeMoveDirectoryFiles(FONT_FILES_DIRECTORY, backupRootDir);
            }

            Directory.CreateDirectory(FONT_FILES_DIRECTORY);

#pragma warning disable 0612, 0618
            foreach (AGS.Types.Font font in game.Fonts)
            {
                FontFile ff;
                if (File.Exists(font.TTFFileName))
                {
                    File.Move(font.TTFFileName, Path.Combine(FONT_FILES_DIRECTORY, font.TTFFileName));
                    ff = NewFontFile(font.TTFFileName);
                }
                else if (File.Exists(font.WFNFileName))
                {
                    File.Move(font.WFNFileName, Path.Combine(FONT_FILES_DIRECTORY, font.WFNFileName));
                    ff = NewFontFile(font.WFNFileName);
                }
                else
                {
                    // TODO: add a warning? perhaps will have to pass a list of warnings/errors to this method, or invent another way
                    continue;
                }

                // Copy SourceFilename over to FontFile, this may be still used as a reference to original asset
                // when upgrading an older project, where font files are renamed to agsfntN.ext.
                ff.SourceFilename = font.SourceFilename;

                game.FontFiles.Add(ff);
                AssignFontFileToFont(font, ff);
            }
#pragma warning restore 0612, 0618
        }
    }
}
