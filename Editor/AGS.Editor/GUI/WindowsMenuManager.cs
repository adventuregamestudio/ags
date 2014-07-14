using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;
using WeifenLuo.WinFormsUI.Docking;
using AGS.Types;

namespace AGS.Editor
{
    public class WindowsMenuManager
    {
        private ToolStripMenuItem _windowsMenu;
        private DockPanel _dockPanel;
        private const string MENU_SEPARATOR = "-";

        internal WindowsMenuManager(ToolStripMenuItem windowsMenu,
            List<DockContent> startupPanes, DockPanel dockPanel)
        {
            _windowsMenu = windowsMenu;
            _dockPanel = dockPanel;
            InitItems(startupPanes);
        }

        private void InitItems(List<DockContent> startupPanes)
        {
            foreach (DockContent dockContent in startupPanes)
            {
                ToolStripMenuItem menuItem = new ToolStripMenuItem(dockContent.Text);
                menuItem.CheckOnClick = true;
                UpdateMenuItemChecked(menuItem, dockContent);
                menuItem.Tag = dockContent;
                menuItem.CheckedChanged += StartupWindow_CheckedChanged;
                dockContent.DockStateChanged += StartupWindow_DockStateChanged;
                _windowsMenu.DropDownItems.Add(menuItem);
            }
            _windowsMenu.DropDownItems.Add(MENU_SEPARATOR);
        }

        private void StartupWindow_DockStateChanged(object sender, EventArgs e)
        {
            DockContent dockContent = sender as DockContent;
            if (dockContent != null)
            {
                ToolStripMenuItem menuItem = GetMenuItem(dockContent);
                UpdateMenuItemChecked(menuItem, dockContent);
            }
        }

        private void UpdateMenuItemChecked(ToolStripMenuItem menuItem, DockContent dockContent)
        {
            if (menuItem != null)
            {
                menuItem.Checked = (dockContent.DockState != DockState.Hidden);
            }
        }

        private ToolStripMenuItem GetMenuItem(DockContent dockContent)
        {
            foreach (ToolStripItem item in _windowsMenu.DropDownItems)
            {
                if (item.Tag == dockContent)
                {
                    return item as ToolStripMenuItem;
                }
            }
            return null;
        }

        private void StartupWindow_CheckedChanged(object sender, EventArgs e)
        {
            ToolStripMenuItem menuItem = sender as ToolStripMenuItem;
            if (menuItem != null)
            {
                DockContent dockContent = menuItem.Tag as DockContent;
                if (dockContent != null)
                {
                    if (menuItem.Checked)
                    {
                        dockContent.Show();
                        if (dockContent.IsHidden)
                        {
                            //todo: Find a better solution.
                            //This is a hack since apparently the docking suite doesn't save/load
                            //the layout correctly when the window was floating.
                            //Loading the editor will hide all windows that were floating
                            //and the previous dockContent.Show will not bring them back and the window stays hidden!
                            dockContent.Show(_dockPanel, DockState.Float);
                        }
                    }
                    else
                    {
                        dockContent.Hide();
                    }
                }
            }
        }

        public void Refresh(List<ContentDocument> documents, ContentDocument activeDocument)
        {
            RemoveAllButStartupWindows();
            AddDocuments(documents, activeDocument);
        }

        private void AddDocuments(List<ContentDocument> documents, ContentDocument activeDocument)
        {
            foreach (ContentDocument document in documents)
            {
                ToolStripMenuItem menuItem = new ToolStripMenuItem(document.Control.DockingContainer.Text);
                menuItem.CheckOnClick = false;
                menuItem.Checked = (document == activeDocument);
                menuItem.Tag = document;
                menuItem.Click += MenuItem_Click;
                _windowsMenu.DropDownItems.Add(menuItem);
            }
        }

        private void MenuItem_Click(object sender, EventArgs e)
        {
            ToolStripMenuItem menuItem = sender as ToolStripMenuItem;
            if (menuItem != null && !menuItem.Checked)
            {
                ContentDocument document = menuItem.Tag as ContentDocument;
                if (document != null)
                {
                    Factory.GUIController.AddOrShowPane(document);
                }
            }
        }

        private void RemoveAllButStartupWindows()
        {
            for (int i = _windowsMenu.DropDownItems.Count - 1; i >= 0; i--)
            {
                ToolStripItem menuItem = _windowsMenu.DropDownItems[i];
                if (menuItem is ToolStripSeparator) return;
                menuItem.Click -= MenuItem_Click;
                _windowsMenu.DropDownItems.RemoveAt(i);
            }
        }
    }
}
