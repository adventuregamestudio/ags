using System;
using System.Collections.Generic;
using System.Text;
using WeifenLuo.WinFormsUI.Docking;
using System.IO;
using System.Windows.Forms;

namespace AGS.Editor
{
    public class WindowsLayoutManager
    {
        private DockPanel _dockPanel;
        private List<DockContent> _startupPanes;
        private const string LAYOUT_FILENAME = "Layout.xml";

        public WindowsLayoutManager(DockPanel dockPanel,
            List<DockContent> startupPanes)
        {
            _dockPanel = dockPanel;
            _startupPanes = startupPanes;            
        }

        public void SaveLayout()
        {
            string configFile = GetLayoutFile();
            _dockPanel.SaveAsXml(configFile);
        }

        public bool LoadLayout()
        {
            string configFile = GetLayoutFile();
            if (File.Exists(configFile))
            {
                DetachExistingPanes();            
                _dockPanel.LoadFromXml(configFile, new
                    DeserializeDockContent(DeserializeContents));
                return true;
            }
            return false;
        }

        private void DetachExistingPanes()
        {
            for (int i = _dockPanel.Contents.Count - 1; i >= 0; i--)
            {
                IDockContent iContent = _dockPanel.Contents[i];
                DockContent content = (DockContent)iContent;
                content.DockPanel = null;
            }
        }

        private string GetLayoutFile()
        {
            return Path.Combine(
                Path.GetDirectoryName(Application.ExecutablePath),
                LAYOUT_FILENAME);
        }

        private IDockContent DeserializeContents(string type)
        {
            foreach (DockContent pane in _startupPanes)
            {
                if (pane.GetType().ToString().Equals(type))
                {
                    return pane;
                }
            }
            return null;
        }
    }
}
