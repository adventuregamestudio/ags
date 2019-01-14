﻿using System;
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
            SaveLayout(configFile);
        }

        public void SaveLayout(string path)
        {
            string folder = Path.GetDirectoryName(path);
            Directory.CreateDirectory(folder);
            File.SetAttributes(folder, FileAttributes.Normal);
            _dockPanel.SaveAsXml(path);
        }

        public bool LoadLayout()
        {
            string configFile = GetLayoutFile();
            return LoadLayout(configFile);
        }

        public bool LoadLayout(string path)
        {
            if (File.Exists(path))
            {
                DetachExistingPanes();
                _dockPanel.LoadFromXml(path, new
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
            string baseFolder = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);
            const string agsFolder = "AGS";
            return Path.Combine(Path.Combine(baseFolder, agsFolder), LAYOUT_FILENAME);
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
