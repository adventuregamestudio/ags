using AGS.Types;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Xml;

namespace AGS.Editor.Components
{
    class SettingsComponent : BaseComponent
    {
        private const string ICON_KEY = "SettingsIcon";
        private GeneralSettings _settingsPane;
        private ContentDocument _document;

        public SettingsComponent(GUIController guiController, AGSEditor agsEditor)
            : base(guiController, agsEditor)
        {
            _settingsPane = new GeneralSettings();
            _document = new ContentDocument(_settingsPane, "General Settings", this, ICON_KEY);
            _guiController.RegisterIcon(ICON_KEY, Resources.ResourceManager.GetIcon("iconsett.ico"));
            _guiController.ProjectTree.AddTreeRoot(this, "GeneralSettings", "General Settings", ICON_KEY);
        }

        public override string ComponentID
        {
            get { return ComponentIDs.GeneralSettings; }
        }

        public override void CommandClick(string controlID)
        {
            _document.TreeNodeID = controlID;
            _guiController.AddOrShowPane(_document);
			_guiController.ShowCuppit("This is the Game Settings window. It's where you set various options for your game, such as the name, resolution and dialog type.", "Game Settings introduction");
        }

        public override void RefreshDataFromGame()
        {
            _settingsPane.RefreshData();

			if (_agsEditor.Preferences.StartupPane == EditorStartupPane.GeneralSettings)
			{
				_guiController.AddOrShowPane(_document);
			}
        }

    }
}
