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


        public DebugLogComponent(GUIController guiController, AGSEditor agsEditor)
            : base(guiController, agsEditor)
        {
            _logPanel = new LogPanel();
            _logConfig = new DebugLog();

            _logPanel.LogConfig = _logConfig;

            _guiController.AddDockPane(_logPanel, new DockData(DockingState.DockBottom, Rectangle.Empty));
            _guiController.SetLogPanel(_logPanel);
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
                LogLevel logLevel = _logConfig.LogOutput.GetGroupLevel(group);

                cmd += group.ToString().ToLower() + ":" + logLevel.ToString().ToLower();

                if (i < ((int)LogGroup.NumGroups) - 1) cmd += ",";
            }

            return cmd;
        }

        public override void CommandClick(string controlID)
        {
        }

        public override void RefreshDataFromGame()
        {
            _logPanel.ApplyFilters(_logConfig);
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
