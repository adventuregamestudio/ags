using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Text;
using System.Windows.Forms;
using System.Xml;
using AGS.Types;

namespace AGS.Editor.Components
{
    class FontsComponent : BaseComponent
    {
        private const string TOP_LEVEL_COMMAND_ID = "Fonts";
        private const string COMMAND_NEW_ITEM = "NewFont";
        private const string COMMAND_DELETE_ITEM = "DeleteFont";
        private const int BUILT_IN_FONTS = 3;
        private const string ICON_KEY = "FontsIcon";

        private const int DEFAULT_IMPORTED_FONT_SIZE = 10;
        private Dictionary<AGS.Types.Font, ContentDocument> _documents;
        private AGS.Types.Font _itemRightClicked = null;

        public FontsComponent(GUIController guiController, AGSEditor agsEditor)
            : base(guiController, agsEditor)
        {
            _documents = new Dictionary<AGS.Types.Font, ContentDocument>();
            _guiController.RegisterIcon(ICON_KEY, Resources.ResourceManager.GetIcon("font.ico"));
            _guiController.RegisterIcon("FontIcon", Resources.ResourceManager.GetIcon("font-item.ico"));
            _guiController.ProjectTree.AddTreeRoot(this, TOP_LEVEL_COMMAND_ID, "Fonts", ICON_KEY);
            FontFileUIEditor.FontFileGUI = ImportFontFile;
            FontSizeUIEditor.FontSizeGUI = ReimportFontSize;
            RePopulateTreeView();
        }

        public override string ComponentID
        {
            get { return ComponentIDs.Fonts; }
        }

        public override void CommandClick(string controlID)
        {
            if (controlID == COMMAND_NEW_ITEM)
            {
                IList<AGS.Types.Font> items = _agsEditor.CurrentGame.Fonts;
                AGS.Types.Font newItem = new AGS.Types.Font();
                newItem.ID = items.Count;
                newItem.Name = "Font " + newItem.ID;
                newItem.OutlineStyle = FontOutlineStyle.None;
                newItem.PointSize = items[0].PointSize;
                newItem.SourceFilename = Utilities.GetRelativeToProjectPath(items[0].SourceFilename);
                newItem.TTFMetricsFixup = _agsEditor.CurrentGame.Settings.TTFMetricsFixup; // use defaults
                items.Add(newItem);
                Utilities.CopyFont(0, newItem.ID);
                Factory.NativeProxy.OnFontAdded(_agsEditor.CurrentGame, newItem.ID);
                _guiController.ProjectTree.StartFromNode(this, TOP_LEVEL_COMMAND_ID);
                _guiController.ProjectTree.AddTreeLeaf(this, GetNodeID(newItem), GetNodeLabel(newItem), "FontIcon");
                _guiController.ProjectTree.SelectNode(this, GetNodeID(newItem));
                ShowOrAddPane(newItem);
                FontTypeConverter.SetFontList(_agsEditor.CurrentGame.Fonts);
            }
            else if (controlID == COMMAND_DELETE_ITEM)
            {
                if (MessageBox.Show("Are you sure you want to delete this font? Doing so could break any scripts that refer to fonts by number.", "Confirm delete", MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.Yes)
                {
                    int removingID = _itemRightClicked.ID;
                    _agsEditor.DeleteFileOnDisk(_itemRightClicked.WFNFileName);
                    _agsEditor.DeleteFileOnDisk(_itemRightClicked.TTFFileName);

                    foreach (AGS.Types.Font item in _agsEditor.CurrentGame.Fonts)
                    {
                        if (item.ID > removingID)
                        {
                            if (File.Exists(item.WFNFileName))
                            {
                                _agsEditor.RenameFileOnDisk(item.WFNFileName, "agsfnt" + (item.ID - 1) + ".wfn");
                            }
                            if (File.Exists(item.TTFFileName))
                            {
                                _agsEditor.RenameFileOnDisk(item.TTFFileName, "agsfnt" + (item.ID - 1) + ".ttf");
                            }
                            item.ID--;
                        }
                    }
                    if (_documents.ContainsKey(_itemRightClicked))
                    {
                        _guiController.RemovePaneIfExists(_documents[_itemRightClicked]);
                        _documents.Remove(_itemRightClicked);
                    }
                    _agsEditor.CurrentGame.Fonts.Remove(_itemRightClicked);
                    _agsEditor.CurrentGame.FilesAddedOrRemoved = true;
                    Factory.NativeProxy.OnFontDeleted(_agsEditor.CurrentGame, removingID);
                    Factory.NativeProxy.GameSettingsChanged(_agsEditor.CurrentGame);
                    RePopulateTreeView();
                    FontTypeConverter.SetFontList(_agsEditor.CurrentGame.Fonts);
                }
            }
            else
            {
                if (controlID != TOP_LEVEL_COMMAND_ID)
                {
                    AGS.Types.Font chosenFont = _agsEditor.CurrentGame.Fonts[Convert.ToInt32(controlID.Substring(3))];
                    ShowOrAddPane(chosenFont);
                }
            }
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
                editor.ImportOverFont = ImportOverFont;
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
                RePopulateTreeView();

                foreach (ContentDocument doc in _documents.Values)
                {
                    doc.Name = ((FontEditor)doc.Control).ItemToEdit.WindowTitle;
                }

                FontTypeConverter.SetFontList(_agsEditor.CurrentGame.Fonts);
                return;
            }

            bool shouldRepaint = (propertyName == "SourceFilename" || propertyName == "Font Size" || propertyName == "SizeMultiplier");
            Factory.NativeProxy.OnFontUpdated(Factory.AGSEditor.CurrentGame, itemBeingEdited.ID, (propertyName == "SourceFilename"));
            if (shouldRepaint)
                editor.OnFontUpdated();
        }

        public override IList<MenuCommand> GetContextMenu(string controlID)
        {
            IList<MenuCommand> menu = new List<MenuCommand>();
            if (controlID == TOP_LEVEL_COMMAND_ID)
            {
                menu.Add(new MenuCommand(COMMAND_NEW_ITEM, "New Font", null));
            }
            else
            {
                int fontID = Convert.ToInt32(controlID.Substring(3));
                _itemRightClicked = _agsEditor.CurrentGame.Fonts[fontID];
                menu.Add(new MenuCommand(COMMAND_DELETE_ITEM, "Delete this font", null));
                if (fontID < BUILT_IN_FONTS)
                {
                    // can't delete built-in fonts
                    menu[menu.Count - 1].Enabled = false;
                }
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
            FontTypeConverter.SetFontList(_agsEditor.CurrentGame.Fonts);
        }

        private string GetNodeID(AGS.Types.Font item)
        {
            return "Fnt" + item.ID;
        }

        private string GetNodeLabel(AGS.Types.Font item)
        {
            return item.ID.ToString() + ": " + item.Name;
        }

        private void RePopulateTreeView()
        {
            _guiController.ProjectTree.RemoveAllChildNodes(this, TOP_LEVEL_COMMAND_ID);
            _guiController.ProjectTree.StartFromNode(this, TOP_LEVEL_COMMAND_ID);
            foreach (AGS.Types.Font item in _agsEditor.CurrentGame.Fonts)
            {
                _guiController.ProjectTree.AddTreeLeaf(this, GetNodeID(item), GetNodeLabel(item), "FontIcon");
            }

            if (_documents.ContainsValue(_guiController.ActivePane))
            {
                FontEditor editor = (FontEditor)_guiController.ActivePane.Control;
                _guiController.ProjectTree.SelectNode(this, GetNodeID(editor.ItemToEdit));
            }
            else if (_agsEditor.CurrentGame.Fonts.Count > 0)
            {
                _guiController.ProjectTree.SelectNode(this, "Fnt0");
            }
        }

        private int ReimportFontSize(AGS.Types.Font font)
        {
            // TODO: find a better solution for the font format check, perhaps store font type as Font's property,
            // or add a direct (unserialized) reference to related FontFile
            if (!font.SourceFilename.EndsWith(".ttf"))
            {
                _guiController.ShowMessage("You can only reimport TTF fonts with different size. For bitmap fonts try adjusting Size Multiplier instead.", MessageBoxIconType.Information);
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

        private void ImportTTFFont(AGS.Types.Font font, string fileName, string newTTFName, string newWFNName)
        {
            FontHeightDefinition sizeType;
            int sizeValue;
            if (!ImportTTFDialog.Show(out sizeType, out sizeValue,
                    font.PointSize > 0 ? font.PointSize : DEFAULT_IMPORTED_FONT_SIZE, Int32.MaxValue))
                return;
            File.Copy(fileName, newTTFName, true);
            try
            {
                if (File.Exists(newWFNName))
                {
                    Factory.AGSEditor.DeleteFileOnDisk(newWFNName);
                }
                Factory.NativeProxy.ReloadFont(font.ID);
                // If user asked to get a certain pixel height, then try to change the font's
                // point size, until found the closest result
                if (sizeType == FontHeightDefinition.PixelHeight)
                {
                    sizeValue = Factory.NativeProxy.FindTTFSizeForHeight(newTTFName, sizeValue);
                }
            }
            catch (AGSEditorException ex)
            {
                Factory.GUIController.ShowMessage("Unable to import the font.\n\n" + ex.Message, MessageBoxIcon.Warning);
                Utilities.TryDeleteFile(newTTFName);
            }

            font.PointSize = sizeValue;
            font.SizeMultiplier = 1;
            font.SourceFilename = Utilities.GetRelativeToProjectPath(fileName);
        }

        private void ImportWFNFont(AGS.Types.Font font, string fileName, string newTTFName, string newWFNName)
        {
            try
            {
                if (File.Exists(newTTFName))
                {
                    Factory.AGSEditor.DeleteFileOnDisk(newTTFName);
                }

                if (fileName.ToLower().EndsWith(".wfn"))
                {
                    File.Copy(fileName, newWFNName, true);
                }
                else
                {
                    Factory.NativeProxy.ImportSCIFont(fileName, font.ID);
                }
                Factory.NativeProxy.ReloadFont(font.ID);
                font.PointSize = 0;
                font.SizeMultiplier = 1;
                font.SourceFilename = Utilities.GetRelativeToProjectPath(fileName);
            }
            catch (AGSEditorException ex)
            {
                Factory.GUIController.ShowMessage("Unable to import the font.\n\n" + ex.Message, MessageBoxIcon.Warning);
            }
        }

        private void ImportFont(AGS.Types.Font item, string fileName)
        {
            try
            {
                string newTTFName = "agsfnt" + item.ID + ".ttf";
                string newWFNName = "agsfnt" + item.ID + ".wfn";

                List<string> filesToCheck = new List<string>();
                filesToCheck.Add(newTTFName);
                filesToCheck.Add(newWFNName);
                if (!Factory.AGSEditor.AttemptToGetWriteAccess(filesToCheck))
                {
                    return;
                }

                if (fileName.ToLower().EndsWith(".ttf"))
                {
                    ImportTTFFont(item, fileName, newTTFName, newWFNName);
                }
                else
                {
                    ImportWFNFont(item, fileName, newTTFName, newWFNName);
                }
                Factory.NativeProxy.OnFontUpdated(Factory.AGSEditor.CurrentGame, item.ID, true);
            }
            catch (Exception ex)
            {
                Factory.GUIController.ShowMessage("There was a problem importing the font. The error was: " + ex.Message, MessageBoxIcon.Warning);
            }
        }

        private string ImportFontFile(AGS.Types.Font item)
        {
            string fileName = Factory.GUIController.ShowOpenFileDialog("Select font to import...", Constants.FONT_FILE_FILTER);
            if (fileName != null)
            {
                ImportFont(item, fileName);
            }
            return item.SourceFilename;
        }

        public void ImportOverFont(AGS.Types.Font item)
        {
            ImportFontFile(item);
        }
    }
}
