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
    class SourceControlComponent : BaseComponent, IPersistUserData
    {
        private const string ADD_TO_SOURCE_CONTROL_COMMAND = "AddSourceControl";
        private const string SHOW_PENDING_CHECKINS_COMMAND = "ShowPendingCheckins";
        private const string DISCONNECT_SOURCE_CONTROL_COMMAND = "DisconnectSourceControl";

        private const string XML_ELEMENT_PROVIDER = "SourceControlProvider";
        private const string XML_ELEMENT_PROJECT_NAME = "SourceControlProjectName";
        private const string XML_ELEMENT_PATH = "SourceControlPath";
        private const string XML_ELEMENT_USING_SCC = "ProjectUnderControl";

        private ISourceControlProvider _sourceControl = null;
        private bool _menuShowsUnderControl = false;
        private MenuCommands _menuCommands = new MenuCommands(GUIController.FILE_MENU_ID, 900);
        private string _sourceControlPathLoadedFromUserData = null;
        private string _sourceControlProjectLoadedFromUserData = null;

        public SourceControlComponent(GUIController guiController, AGSEditor agsEditor)
            : base(guiController, agsEditor)
        {
            _sourceControl = _agsEditor.SourceControlProvider;

            try
            {
                if (!_sourceControl.Initialize(_guiController.TopLevelWindowHandle))
                {
                    _guiController.ShowMessage("Your source code control provider '" + _sourceControl.ProviderName + "' failed to initialize.", MessageBoxIcon.Warning);
                }
                if (_sourceControl.Available)
                {
                    _menuCommands.Commands.Add(new MenuCommand(ADD_TO_SOURCE_CONTROL_COMMAND, "Add to source control"));
                    _guiController.AddMenuItems(this, _menuCommands);
                }
            }
            catch (Exception ex)
            {
                _guiController.ShowMessage("A serious error occurred attempting to connect to your source control provider. Please ensure that '" + _sourceControl.ProviderName + "' is properly installed.\n\nError details: " + ex.ToString(), MessageBoxIcon.Stop);
            }
        }

        public override void EditorShutdown()
        {
            if (_sourceControl != null)
            {
                _sourceControl.Dispose();
                _sourceControl = null;
            }

        }

        public override string ComponentID
        {
            get { return ComponentIDs.SourceControl; }
        }

        public override void CommandClick(string controlID)
        {
            if (controlID == ADD_TO_SOURCE_CONTROL_COMMAND)
            {
                if (!_sourceControl.AddToSourceControl())
                {
                    _guiController.ShowMessage("Add to source control failed.", MessageBoxIcon.Warning);
                }
                else if (_sourceControl.ProjectUnderControl)
                {
                    _agsEditor.SaveGameFiles();
                    UpdateMenuCommandVisibility();
                    _guiController.ShowPendingCheckinsDialog();
                }
            }
            else if (controlID == SHOW_PENDING_CHECKINS_COMMAND)
            {
                _guiController.ShowPendingCheckinsDialog();
            }
            else if (controlID == DISCONNECT_SOURCE_CONTROL_COMMAND)
            {
                if (_guiController.ShowQuestion("Are you sure you want to disconnect this project from source control?") == DialogResult.Yes)
                {
                    _sourceControl.CloseProject();
                    UpdateMenuCommandVisibility();
                    _agsEditor.SaveGameFiles();
                }
            }
        }

        private void UpdateMenuCommandVisibility()
        {
            if ((_sourceControl.ProjectUnderControl) && (!_menuShowsUnderControl))
            {
                _guiController.RemoveMenuItems(_menuCommands);
                _menuCommands.Commands.Clear();
                _menuCommands.Commands.Add(new MenuCommand(SHOW_PENDING_CHECKINS_COMMAND, "Show Pending Checkins"));
                _menuCommands.Commands.Add(new MenuCommand(DISCONNECT_SOURCE_CONTROL_COMMAND, "Disconnect source control"));
                _guiController.AddMenuItems(this, _menuCommands);
                _menuShowsUnderControl = true;
            }
            else if ((!_sourceControl.ProjectUnderControl) && (_menuShowsUnderControl))
            {
                _guiController.RemoveMenuItems(_menuCommands);
                _menuCommands.Commands.Clear();
                _menuCommands.Commands.Add(new MenuCommand(ADD_TO_SOURCE_CONTROL_COMMAND, "Add to source control"));
                _guiController.AddMenuItems(this, _menuCommands);
                _menuShowsUnderControl = false;
            }

        }

        public override void RefreshDataFromGame()
        {
            if (_sourceControl.Available)
            {
                UpdateMenuCommandVisibility();
                if (_sourceControl.ProjectUnderControl)
                {
                    // do we need to do anything here?
                }
            }
        }

        public override void ToXml(XmlTextWriter writer)
        {
            writer.WriteElementString(XML_ELEMENT_USING_SCC, _sourceControl.ProjectUnderControl.ToString());
            if (_sourceControl.ProjectUnderControl)
            {
                writer.WriteElementString(XML_ELEMENT_PROJECT_NAME, _sourceControl.ProjectSourceControlName);
                writer.WriteElementString(XML_ELEMENT_PROVIDER, _sourceControl.ProviderName);
            }
        }

        public override void FromXml(XmlNode node)
        {
            bool isUnderControl = false;
            if (node != null)
            {
                isUnderControl = Convert.ToBoolean(node.SelectSingleNode(XML_ELEMENT_USING_SCC).InnerXml);
            }

            if (_sourceControl.Available)
            {
                _sourceControl.CloseProject();
                if ((node != null) && (isUnderControl))
                {
                    OpenProjectFromSourceControl(node);
                }
            }
            else if (isUnderControl)
            {
                _guiController.ShowMessage("This project is under source control, but AGS was not able to initialize any source control provider. If you save the game now, it will be disconnected from source control.", MessageBoxIcon.Warning);
            }

            _sourceControlPathLoadedFromUserData = null;
            _sourceControlProjectLoadedFromUserData = null;
        }

        private void OpenProjectFromSourceControl(XmlNode node)
        {
            if (_sourceControlPathLoadedFromUserData == null)
            {
                XmlNode pathNode = node.SelectSingleNode(XML_ELEMENT_PATH);
                if (pathNode != null)
                {
                    // Older versions stored the path in the .AGF file
                    _sourceControlPathLoadedFromUserData = pathNode.InnerText;
                }
                else
                {
                    // No user data file yet, allow SCC to prompt for login info
                    _sourceControlPathLoadedFromUserData = string.Empty;
                }
            }

            string sourceControlProject = node.SelectSingleNode(XML_ELEMENT_PROJECT_NAME).InnerText;

            if ((_sourceControlProjectLoadedFromUserData != null) &&
                (_sourceControlProjectLoadedFromUserData != sourceControlProject))
            {
                if (_guiController.ShowQuestion("The source control information stored with this game is inconsistent. Do you want to bind to source control anyway?", MessageBoxIcon.Warning) == DialogResult.No)
                {
                    return;
                }
            }

            string provider = SerializeUtils.GetElementStringOrDefault(node, XML_ELEMENT_PROVIDER, string.Empty);
            if ((!string.IsNullOrEmpty(provider)) &&
                (provider != _sourceControl.ProviderName))
            {
                _guiController.ShowMessage("The source control provider you are using (" + _sourceControl.ProviderName + ") is not the same as the one that this game is configured for (" + provider + "). Either switch your local source control provider, or reconnect the project to your new source control provider.", MessageBoxIcon.Warning);
                return;
            }

            if (!_sourceControl.OpenProject(_sourceControlPathLoadedFromUserData, sourceControlProject))
            {
                _guiController.ShowMessage("AGS was unable to connect to the project in source control. If you save the game, it will be disconnected from source control.", MessageBoxIcon.Warning);
            }
        }

        void IPersistUserData.Serialize(XmlTextWriter writer)
        {
            if ((_sourceControl != null) && (_sourceControl.ProjectUnderControl))
            {
                writer.WriteElementString(XML_ELEMENT_PATH, _sourceControl.ProjectSourceControlPath);
                writer.WriteElementString(XML_ELEMENT_PROJECT_NAME, _sourceControl.ProjectSourceControlName);
            }
        }

        void IPersistUserData.DeSerialize(XmlNode node)
        {
            if (node != null)
            {
                XmlNode pathNode = node.SelectSingleNode(XML_ELEMENT_PATH);
                if (pathNode != null)
                {
                    _sourceControlPathLoadedFromUserData = pathNode.InnerText;
                    _sourceControlProjectLoadedFromUserData = node.SelectSingleNode(XML_ELEMENT_PROJECT_NAME).InnerText;
                }
            }
        }
    }
}
