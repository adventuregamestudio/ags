using AGS.Types;
using System;
using System.Collections.Generic;
using System.Text;
using AGS.Editor.Preferences;
using System.Xml;
using System.Drawing;
using AGS.Types.Enums;

namespace AGS.Editor.Components
{
    internal class DebugLogComponent : BaseComponent, IPersistUserData
    {
        private LogPanel _logPanel;
        private DebugLog _logConfig;  // Particular settings on logging from the game when running the game in debug mode in the Editor
        private ContentDocument _document;
        private MenuCommands _menuCommands = new MenuCommands(GUIController.HELP_MENU_ID, 500);

        private const string ICON_KEY = "MenuIconShowDebugLog";
        private const string SHOW_DEBUG_LOG_COMMAND = "ShowDebugLog";


        public DebugLogComponent(GUIController guiController, AGSEditor agsEditor)
            : base(guiController, agsEditor)
        {
            _logPanel = new LogPanel(guiController);
            _document = new ContentDocument(_logPanel, "Debug Log", this, ICON_KEY);
            _document.PreferredDockData = new DockData(DockingState.DockBottom, Rectangle.Empty);
            _logConfig = new DebugLog();

            _document.SelectedPropertyGridObject = _logConfig;

            _guiController.RegisterIcon(ICON_KEY, Resources.ResourceManager.GetIcon("debug_log.ico"));

            _menuCommands.Commands.Add(new MenuCommand(SHOW_DEBUG_LOG_COMMAND, "Show Debug Log", ICON_KEY));
            _guiController.AddMenuItems(this, _menuCommands);
            _guiController.SetLogPanel(_logPanel);

            _logPanel.ApplyFilters(_logConfig);
        }

        public override void PropertyChanged(string propertyName, object oldValue)
        {
            _logPanel.ApplyFilters(_logConfig);
        }

        public override string ComponentID
        {
            get { return ComponentIDs.DebugLog; }
        }
        
        public string GetCmdLineLogGroupsAndLevels()
        {
            string cmd = "--log-debugger=";

            for (int i = 0; i < (int)LogGroup.NumGroups; i++)
            {
                LogGroup group = (LogGroup)i;
                LogLevel logLevel = _logConfig.GetGroupLogLevel(group);

                cmd += group.ToString().ToLower() + ":" + logLevel.ToString().ToLower();

                if (i < ((int)LogGroup.NumGroups) - 1) cmd += ",";
            }

            return cmd;
        }

        public override void CommandClick(string controlID)
        {
            if (controlID == SHOW_DEBUG_LOG_COMMAND)
            {
                _guiController.AddOrShowPane(_document);
            }
        }

        public override void RefreshDataFromGame()
        {
            // Game has just been loaded
            if (Factory.AGSEditor.Settings.StartupPane == StartupPane.StartPage)
            {
                _guiController.AddOrShowPane(_document);
            }
        }

        public void Serialize(XmlTextWriter writer)
        {
            _logConfig.ToXml(writer);
        }

        public void DeSerialize(XmlNode node)
        {
            if (node != null)
            {
                try
                {
                    _logConfig.FromXml(node);
                }
                catch
                {
                    _logConfig.SetDefaults();
                }
            }
        }
    }
}
