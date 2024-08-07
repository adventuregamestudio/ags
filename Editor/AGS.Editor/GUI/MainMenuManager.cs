using AGS.Types;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    public class MainMenuManager
    {
        public delegate void MenuClickHandler(string menuItemID);
        public event MenuClickHandler OnMenuClick;

        private MenuStrip _mainMenu;
        private EventHandler _onClick;
        private ToolStripMenuItem _currentPaneMenu = null;
		private Dictionary<string, List<MenuCommands>> _menuCommandGroups = new Dictionary<string, List<MenuCommands>>();
        private WindowsMenuManager _windowsMenuManager;

        internal MainMenuManager(MenuStrip mainMenu, WindowsMenuManager windowsMenuManager)
        {
            _mainMenu = mainMenu;
            _windowsMenuManager = windowsMenuManager;
            _onClick = new EventHandler(MenuEventHandler);
        }

        public void AddMenu(string id, string title)
        {
			AddMenu(id, title, null);
        }

		public void AddMenu(string id, string title, string insertAfterMenuID)
		{
			ToolStripMenuItem newMenu = new ToolStripMenuItem(title);
			newMenu.Name = id;
			AddOrInsertMenu(newMenu, insertAfterMenuID);
		}

        private ToolStripItem AddMenuItem(MenuCommand command)
        {
            string id = command.ID;
            string name = command.Name;
            Keys shortcutKeys = command.ShortcutKey;
            string iconKey = command.IconKey;
            bool enabled = command.Enabled;

            ToolStripItem newItem;

            if (name == MenuCommand.MENU_TEXT_SEPARATOR)
            {
                newItem = new ToolStripSeparator();
            }
            else if (command.SubCommands != null && command.SubCommands.Count > 0)
            {
                ToolStripMenuItem subMenu = new ToolStripMenuItem(name, null, null, id);
                subMenu.Enabled = enabled;
                foreach (var subCommand in command.SubCommands)
                {
                    subMenu.DropDownItems.Add(AddMenuItem(subCommand));
                }
                newItem = subMenu;
            }
            else
            {
                newItem = new ToolStripMenuItem(name, null, _onClick, id);
                ((ToolStripMenuItem)newItem).ShortcutKeys = shortcutKeys;
                if (iconKey != null)
                {
                    newItem.Image = Factory.GUIController.ImageList.Images[iconKey];
                }
                newItem.Enabled = enabled;
            }
            return newItem;
        }

        private ToolStripItem AddTopMenuItem(string menu, MenuCommand command)
        {
            ToolStripItem[] results = _mainMenu.Items.Find(menu, false);
            if (results.Length == 0)
            {
                throw new AGSEditorException("Menu " + menu + " not found");
            }
            ToolStripMenuItem topMenu = (ToolStripMenuItem)results[0];
            ToolStripItem newItem = AddMenuItem(command);

            topMenu.DropDownItems.Add(newItem);
            return newItem;
        }

		private void RefreshMenu(string menuName)
		{
			if (!_menuCommandGroups.ContainsKey(menuName))
			{
				throw new AGSEditorException("Attempted to refresh non existant menu: " + menuName);
			}

			ToolStripItem[] results = _mainMenu.Items.Find(menuName, false);
			if (results.Length == 0)
			{
				throw new AGSEditorException("Menu " + menuName + " not found");
			}
			ToolStripMenuItem topMenu = (ToolStripMenuItem)results[0];
			topMenu.DropDownItems.Clear();

			foreach (MenuCommands commandGroup in _menuCommandGroups[menuName])
			{
				if (topMenu.DropDownItems.Count > 0)
				{
					topMenu.DropDownItems.Add(new ToolStripSeparator());
				}
				foreach (MenuCommand command in commandGroup.Commands)
				{
					ToolStripItem newItem = AddTopMenuItem(menuName, command);
					newItem.Tag = command;
				}
			}
		}

        public void RefreshWindowsMenu(List<ContentDocument> documents,
            ContentDocument activeDocument)
        {
            _windowsMenuManager.Refresh(documents, activeDocument); 
        }

		public void AddMenuCommandGroup(MenuCommands commands)
		{
			if (!_menuCommandGroups.ContainsKey(commands.MenuName))
			{
				_menuCommandGroups.Add(commands.MenuName, new List<MenuCommands>());
			}
			_menuCommandGroups[commands.MenuName].Add(commands);
			_menuCommandGroups[commands.MenuName].Sort();

			RefreshMenu(commands.MenuName);
		}

        public void RemoveMenuCommandGroup(MenuCommands commands)
        {
			if (_menuCommandGroups.ContainsKey(commands.MenuName))
			{
				if (_menuCommandGroups[commands.MenuName].Contains(commands))
				{
					_menuCommandGroups[commands.MenuName].Remove(commands);
					RefreshMenu(commands.MenuName);
				}
			}
        }

        // TO-DO: this can't go down in subcommands
        public MenuCommand GetCommandById(string commandId)
        {
            List<string> keyList = new List<string>(_menuCommandGroups.Keys);
            foreach (string key in keyList)
            {
                foreach (MenuCommands menuCommands in _menuCommandGroups[key])
                {
                    for (int i = 0; i < menuCommands.Commands.Count; i++)
                    {
                        if (menuCommands.Commands[i].ID == commandId)
                        {
                            return menuCommands.Commands[i];
                        }
                    }
                }
            }
            return null;
        }

        public void SetMenuItemEnabled(string id, bool enabled)
        {
            ToolStripItem[] results = _mainMenu.Items.Find(id, true);
            if (results.Length == 0)
            {
                throw new AGSEditorException("Menu item does not exist: " + id);
            }
            results[0].Enabled = enabled;

			if (results[0].Tag is MenuCommand)
			{
				((MenuCommand)results[0].Tag).Enabled = enabled;
			}
        }

        public void RefreshCurrentPane()
        {
            ShowPane(Factory.GUIController.ActivePane);
        }

        public void ShowPane(ContentDocument doc)
        {
            RemoveItemsFromLastPane();

            if ((doc == null) || (doc.MainMenu == null))
            {
                return;
            }

            ToolStripMenuItem newMenu = new ToolStripMenuItem(doc.MainMenu.MenuName);
            foreach (MenuCommand command in doc.MainMenu.Commands)
            {
                if (command.IsSeparator)
                {
                    newMenu.DropDownItems.Add(new ToolStripSeparator());
                }
                else
                {
                    ToolStripMenuItem newItem = new ToolStripMenuItem(command.Name, null, _onClick, command.ID);
                    newItem.Enabled = command.Enabled;
                    newItem.Checked = command.Checked;
                    newItem.Tag = doc;
                    newItem.ShortcutKeys = command.ShortcutKey;
                    newItem.ShortcutKeyDisplayString = command.ShortcutKeyDisplayString;
                    if (command.IconKey != null)
                    {
                        newItem.Image = Factory.GUIController.ImageList.Images[command.IconKey];
                    }
                    newMenu.DropDownItems.Add(newItem);
                }
            }

			AddOrInsertMenu(newMenu, doc.MainMenu.InsertAfterMenu);

            _currentPaneMenu = newMenu;
        }

		private void AddOrInsertMenu(ToolStripMenuItem newMenu, string insertAfterMenu)
		{
			if (insertAfterMenu != null)
			{
				ToolStripMenuItem nextToMenu = null;
				int nextToIndex = 0;
				foreach (ToolStripMenuItem menu in _mainMenu.Items)
				{
					if (menu.Name == insertAfterMenu)
					{
						nextToMenu = menu;
						break;
					}
					nextToIndex++;
				}
				if (nextToMenu == null)
				{
					throw new AGSEditorException("Menu not found: " + insertAfterMenu);
				}
				_mainMenu.Items.Insert(nextToIndex + 1, newMenu);
			}
			else
			{
				_mainMenu.Items.Add(newMenu);
			}
		}

        public void ReplaceMenuItemSubcommands(string commandId, IList<MenuCommand> commands)
        {
            // Find the existing menu item
            ToolStripItem[] results = _mainMenu.Items.Find(commandId, true);
            if (results.Length == 0)
            {
                throw new AGSEditorException("Menu item does not exist: " + commandId);
            }

            ToolStripItem oldItem = results[0];
            ToolStripMenuItem parentMenu = (ToolStripMenuItem)oldItem.OwnerItem;

            MenuCommand parentCommand = GetCommandById(commandId);
            parentCommand.SubCommands = commands;
            int oldIndex = parentMenu.DropDownItems.IndexOf(oldItem);
            parentMenu.DropDownItems.Remove(oldItem);

            ToolStripItem newItem = AddMenuItem(parentCommand);
            parentMenu.DropDownItems.Insert(oldIndex, newItem);
        }

        private void RemoveItemsFromLastPane()
        {
            if (_currentPaneMenu != null)
            {
                _mainMenu.Items.Remove(_currentPaneMenu);
                _currentPaneMenu = null;
            }
        }

        private void MenuEventHandler(object sender, EventArgs e)
        {
            ToolStripMenuItem item = (ToolStripMenuItem)sender;
            if (item.Tag is ContentDocument)
            {
                ((ContentDocument)item.Tag).Control.CommandClick(item.Name);
            }
            else if (OnMenuClick != null)
            {
                OnMenuClick(item.Name);
            }
        }
    }
}
