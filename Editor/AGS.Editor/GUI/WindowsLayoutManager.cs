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
        public enum LayoutResult
        {
            OK,
            NoFile,
            LayoutException
        }

        private DockPanel _dockPanel;
        private List<DockContent> _dockPanes;
        private const string LAYOUT_FILENAME = "Layout.xml";
        private const string LAYOUT_RESOURCE = "LayoutDefault.xml";

        public WindowsLayoutManager(DockPanel dockPanel,
            List<DockContent> dockPanes)
        {
            _dockPanel = dockPanel;
            _dockPanes = dockPanes;
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

        public LayoutResult LoadLayout()
        {
            string configFile = GetLayoutFile();
            return LoadLayout(configFile);
        }

        public LayoutResult LoadLayout(string path)
        {
            LayoutResult res;
            try
            {
                if (!File.Exists(path))
                    return LayoutResult.NoFile;
                DetachExistingPanes();
                _dockPanel.LoadFromXml(path, new
                    DeserializeDockContent(DeserializeContents));
                res = LayoutResult.OK;
            }
            catch (Exception)
            {
                res = LayoutResult.LayoutException;
            }
            finally
            {
                RestoreDetachedPanes();
            }
            return res;
        }

        public LayoutResult ResetToDefaults()
        {
            LayoutResult res;
            try
            {
                string layout = Resources.ResourceManager.GetResourceAsString(LAYOUT_RESOURCE, Encoding.Unicode);
                if (string.IsNullOrEmpty(layout))
                    return LayoutResult.NoFile;
                byte[] byteArray = Encoding.Unicode.GetBytes(layout);
                Stream mems = new MemoryStream(byteArray, false);
                DetachExistingPanes();
                _dockPanel.LoadFromXml(mems, new DeserializeDockContent(DeserializeContents));
                res = LayoutResult.OK;
            }
            catch (Exception)
            {
                res = LayoutResult.LayoutException;
            }
            finally
            {
                RestoreDetachedPanes();
            }
            return res;
        }

        /// <summary>
        /// Detaches all the persistent panes from the parent dock.
        /// </summary>
        public void DetachAll()
        {
            DetachExistingPanes();
        }

        /// <summary>
        /// Restores all the detached panes to the parent dock.
        /// Note that this only links the panes to the parent pane, this
        /// does not make them shown yet.
        /// </summary>
        public void RestoreDetached()
        {
            RestoreDetachedPanes();
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

        private void RestoreDetachedPanes()
        {
            foreach (DockContent pane in _dockPanes)
            {
                if (pane.DockPanel == null)
                {
                    // TODO: find a more elegant way to connect to _dockPanel without calling Show/Hide?
                    // pane.DockPanel = _dockPanel does not work and causes exceptions later on
                    pane.Show(_dockPanel, pane.ShowHint);
                    pane.Hide();
                }
            }
        }

        private string GetLayoutFile()
        {
            return Path.Combine(Factory.AGSEditor.LocalAppData, LAYOUT_FILENAME);
        }

        private IDockContent DeserializeContents(string type)
        {
            foreach (DockContent pane in _dockPanes)
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
