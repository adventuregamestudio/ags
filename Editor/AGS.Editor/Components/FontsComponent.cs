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
        
        private Dictionary<AGS.Types.Font, ContentDocument> _documents;
        private AGS.Types.Font _itemRightClicked = null;

        public FontsComponent(GUIController guiController, AGSEditor agsEditor)
            : base(guiController, agsEditor)
        {
            _documents = new Dictionary<AGS.Types.Font, ContentDocument>();
            _guiController.RegisterIcon(ICON_KEY, Resources.ResourceManager.GetIcon("font.ico"));
            _guiController.RegisterIcon("FontIcon", Resources.ResourceManager.GetIcon("font-item.ico"));
            _guiController.ProjectTree.AddTreeRoot(this, TOP_LEVEL_COMMAND_ID, "Fonts", ICON_KEY);
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
                if (_agsEditor.CurrentGame.Fonts.Count == Game.MAX_FONTS)
                {
                    Factory.GUIController.ShowMessage("You already have the maximum number of fonts in your game, and cannot add any more.", MessageBoxIcon.Warning);
                    return;
                }
                IList<AGS.Types.Font> items = _agsEditor.CurrentGame.Fonts;
                AGS.Types.Font newItem = new AGS.Types.Font();
                newItem.ID = items.Count;
                newItem.Name = "Font " + newItem.ID;
                newItem.OutlineStyle = FontOutlineStyle.None;
				newItem.PointSize = items[0].PointSize;
				newItem.SourceFilename = items[0].SourceFilename;
                items.Add(newItem);
                Utilities.CopyFont(0, newItem.ID);
                Factory.NativeProxy.GameSettingsChanged(_agsEditor.CurrentGame);
                _guiController.ProjectTree.StartFromNode(this, TOP_LEVEL_COMMAND_ID);
                _guiController.ProjectTree.AddTreeLeaf(this, GetNodeID(newItem), newItem.ID.ToString() + ": " + newItem.Name, "FontIcon");
                _guiController.ProjectTree.SelectNode(this, GetNodeID(newItem));
				ShowOrAddPane(newItem);
                FontTypeConverter.SetFontList(_agsEditor.CurrentGame.Fonts);
            }
            else if (controlID == COMMAND_DELETE_ITEM)
            {
                if (MessageBox.Show("Are you sure you want to delete this font? Doing so could break any scripts that refer to fonts by number.", "Confirm delete", MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.Yes)
                {
					int removingID = _itemRightClicked.ID;
					_agsEditor.DeleteFileOnDiskAndSourceControl(_itemRightClicked.WFNFileName);
					_agsEditor.DeleteFileOnDiskAndSourceControl(_itemRightClicked.TTFFileName);

                    foreach (AGS.Types.Font item in _agsEditor.CurrentGame.Fonts)
                    {
                        if (item.ID > removingID)
                        {
							if (File.Exists(item.WFNFileName))
							{
								_agsEditor.SourceControlProvider.RenameFileOnDiskAndInSourceControl(item.WFNFileName, "agsfnt" + (item.ID - 1) + ".wfn");
							}
							if (File.Exists(item.TTFFileName))
							{
								_agsEditor.SourceControlProvider.RenameFileOnDiskAndInSourceControl(item.TTFFileName, "agsfnt" + (item.ID - 1) + ".ttf");
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

                document = new ContentDocument(new FontEditor(chosenFont),
                    chosenFont.WindowTitle, this, ICON_KEY, list);
                _documents[chosenFont] = document;
                document.SelectedPropertyGridObject = chosenFont;
			}
            document.TreeNodeID = GetNodeID(chosenFont);
            _guiController.AddOrShowPane(document);
			_guiController.ShowCuppit("The Font Editor allows you to import fonts into your game. Windows TTF fonts are supported, as are SCI fonts which can be created with Radiant FontEdit.", "Fonts introduction");
		}

        public override void PropertyChanged(string propertyName, object oldValue)
        {
            if (propertyName == "Name")
            {
                RePopulateTreeView();

                foreach (ContentDocument doc in _documents.Values)
                {
                    doc.Name = ((FontEditor)doc.Control).ItemToEdit.WindowTitle;
                }

                FontTypeConverter.SetFontList(_agsEditor.CurrentGame.Fonts);
            }
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

        private void RePopulateTreeView()
        {
            _guiController.ProjectTree.RemoveAllChildNodes(this, TOP_LEVEL_COMMAND_ID);
            _guiController.ProjectTree.StartFromNode(this, TOP_LEVEL_COMMAND_ID);
            foreach (AGS.Types.Font item in _agsEditor.CurrentGame.Fonts)
            {
                _guiController.ProjectTree.AddTreeLeaf(this, GetNodeID(item), item.ID.ToString() + ": " + item.Name, "FontIcon");
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

    }
}
