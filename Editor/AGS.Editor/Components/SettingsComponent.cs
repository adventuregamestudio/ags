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
            RecreateDocument();
            _guiController.RegisterIcon(ICON_KEY, Resources.ResourceManager.GetIcon("iconsett.ico"));
            _guiController.ProjectTree.AddTreeRoot(this, "GeneralSettings", "General Settings", ICON_KEY);

            Factory.Events.GamePostLoad += Events_GamePostLoad;
            Factory.Events.GameSettingsChanged += Events_GameSettingsChanged;
        }

        private void RecreateDocument()
        {
            if (_document != null)
            {
                _guiController.RemovePaneIfExists(_document);
                _document.Dispose();
            }
            _settingsPane = new GeneralSettingsPane();
            _document = new ContentDocument(_settingsPane, "General Settings", this, ICON_KEY);
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
            RecreateDocument();

			if (Factory.AGSEditor.Settings.StartupPane == StartupPane.GeneralSettings)
			{
				_guiController.AddOrShowPane(_document);
			}
        }

        private void Events_GamePostLoad(Game game)
        {
            ColorUIEditor.ColorMode = game.Settings.ColorDepth;
            CustomColorConverter.ColorMode = game.Settings.ColorDepth;
        }

        private void Events_GameSettingsChanged()
        {
            ColorUIEditor.ColorMode = _agsEditor.CurrentGame.Settings.ColorDepth;
            CustomColorConverter.ColorMode = _agsEditor.CurrentGame.Settings.ColorDepth;
        }
    }
}
