using AGS.Types;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Xml;
using AGS.Editor.Preferences;

namespace AGS.Editor.Components
{
    class SettingsComponent : BaseComponent
    {
        private const string ICON_KEY = "SettingsIcon";
        private GeneralSettingsPane _settingsPane;
        private ContentDocument _document;

        public SettingsComponent(GUIController guiController, AGSEditor agsEditor)
            : base(guiController, agsEditor)
        {
            _settingsPane = new GeneralSettingsPane();
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
        }

        public override void RefreshDataFromGame()
        {
            _settingsPane.RefreshData();

			if (Factory.AGSEditor.Settings.StartupPane == StartupPane.GeneralSettings)
			{
				_guiController.AddOrShowPane(_document);
			}
        }

    }
}
