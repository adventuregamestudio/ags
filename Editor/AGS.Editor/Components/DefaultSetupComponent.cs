using System;
using System.Collections.Generic;
using System.Text;
using AGS.Types;

namespace AGS.Editor.Components
{
    class DefaultSetupComponent : BaseComponent
    {
        private const string ICON_KEY = "SettingsIcon";
        private DefaultRuntimeSetupPane _settingsPane;
        private ContentDocument _document;

        public DefaultSetupComponent(GUIController guiController, AGSEditor agsEditor)
            : base(guiController, agsEditor)
        {
            RecreateDocument();
            _guiController.RegisterIcon(ICON_KEY, Resources.ResourceManager.GetIcon("iconsett.ico"));
            _guiController.ProjectTree.AddTreeRoot(this, ComponentID, "Default Setup", ICON_KEY);
        }

        private void RecreateDocument()
        {
            if (_document != null)
            {
                _guiController.RemovePaneIfExists(_document);
                _document.Dispose();
            }
            _settingsPane = new DefaultRuntimeSetupPane();
            _document = new ContentDocument(_settingsPane, "Default Setup", this, ICON_KEY);
        }

        public override string ComponentID
        {
            get { return ComponentIDs.DefaultSetup; }
        }

        public override void CommandClick(string controlID)
        {
            _document.TreeNodeID = controlID;
            _guiController.AddOrShowPane(_document);
        }

        public override void GameSettingsChanged()
        {
            _settingsPane.RefreshData();
        }

        public override void RefreshDataFromGame()
        {
            RecreateDocument();
        }
    }
}
