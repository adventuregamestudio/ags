using AGS.Types;
using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Editor.Components
{
    internal class WelcomeComponent : BaseComponent
    {
        private WelcomePane _welcomePane;
        private ContentDocument _document;
        private MenuCommands _menuCommands = new MenuCommands(GUIController.HELP_MENU_ID, 500);

        private const string ICON_KEY = "MenuIconShowStartPage";
        private const string SHOW_START_PAGE_COMMAND = "ShowStartPage";

        public WelcomeComponent(GUIController guiController, AGSEditor agsEditor)
            : base(guiController, agsEditor)
        {
            _welcomePane = new WelcomePane(guiController);
            _document = new ContentDocument(_welcomePane, "Start Page", this, ICON_KEY);

            _guiController.RegisterIcon(ICON_KEY, Resources.ResourceManager.GetIcon("menu_help_showstart.ico"));

            _menuCommands.Commands.Add(new MenuCommand(SHOW_START_PAGE_COMMAND, "Show Start Page", ICON_KEY));
            _guiController.AddMenuItems(this, _menuCommands);
        }

        public override string ComponentID
        {
            get { return ComponentIDs.Welcome; }
        }

        public override void CommandClick(string controlID)
        {
            if (controlID == SHOW_START_PAGE_COMMAND)
            {
                _guiController.AddOrShowPane(_document);
            }
        }

        public override void RefreshDataFromGame()
        {
            // Game has just been loaded
			if (_agsEditor.Preferences.StartupPane == EditorStartupPane.StartPage)
			{
				_guiController.AddOrShowPane(_document);
			}
        }
    }
}
