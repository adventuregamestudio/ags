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

            GuiComponent guiComponent = ComponentController.Instance.FindComponent<GuiComponent>();
            // FIXME: following delegate rem/add is a nasty hack, but we do not have proper
            // callbacks like "after all components attached", "on game loaded", "on game unloaded",
            // so have to use RefreshDataFromGame which may be called multiple times.
            if (guiComponent != null)
            {
                guiComponent.GUIChangedID -= GuiComponent_GUIChangedID;
                guiComponent.GUIChangedID += GuiComponent_GUIChangedID;
            }
            Factory.AGSEditor.CurrentGame.GUIAddedOrRemoved -= CurrentGame_GUIRemoved;
            Factory.AGSEditor.CurrentGame.GUIAddedOrRemoved += CurrentGame_GUIRemoved;
        }

        private int UpdatePropertyOnGuiMove(int propertyValue, int guiOldID, int guiNewID, bool wasRemoved)
        {
            if (propertyValue == guiOldID)
            {
                return guiNewID;
            }
            else if (wasRemoved && (propertyValue > guiOldID))
            {
                return propertyValue - 1;
            }
            return propertyValue;
        }

        private void CurrentGame_GUIRemoved(GUI gui)
        {
            // Pass 0 as a "new id", since 0 is used as "no gui"
            _agsEditor.CurrentGame.Settings.TextWindowGUI = UpdatePropertyOnGuiMove(_agsEditor.CurrentGame.Settings.TextWindowGUI, gui.ID, 0, true);
            _agsEditor.CurrentGame.Settings.DialogOptionsGUI = UpdatePropertyOnGuiMove(_agsEditor.CurrentGame.Settings.DialogOptionsGUI, gui.ID, 0, true);
            _agsEditor.CurrentGame.Settings.ThoughtGUI = UpdatePropertyOnGuiMove(_agsEditor.CurrentGame.Settings.ThoughtGUI, gui.ID, 0, true);
            _settingsPane.RefreshData();
        }

        private void GuiComponent_GUIChangedID(GUI gui, int oldID)
        {
            _agsEditor.CurrentGame.Settings.TextWindowGUI = UpdatePropertyOnGuiMove(_agsEditor.CurrentGame.Settings.TextWindowGUI, oldID, gui.ID, false);
            _agsEditor.CurrentGame.Settings.DialogOptionsGUI = UpdatePropertyOnGuiMove(_agsEditor.CurrentGame.Settings.DialogOptionsGUI, oldID, gui.ID, false);
            _agsEditor.CurrentGame.Settings.ThoughtGUI = UpdatePropertyOnGuiMove(_agsEditor.CurrentGame.Settings.ThoughtGUI, oldID, gui.ID, false);
            _settingsPane.RefreshData();
        }
    }
}
