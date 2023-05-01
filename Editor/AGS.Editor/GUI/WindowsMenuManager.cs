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
        private WindowsLayoutManager _layoutManager;
        private const string MENU_SEPARATOR = "-";
        private const string XML_FILE_FILTER = "Xml files (*.xml)|*.xml";

        internal WindowsMenuManager(ToolStripMenuItem windowsMenu,
            List<DockContent> startupPanes, DockPanel dockPanel, WindowsLayoutManager layoutManager)
        {
            _windowsMenu = windowsMenu;
            _dockPanel = dockPanel;
            _layoutManager = layoutManager;
            InitLayoutSubmenu();
            InitItems(startupPanes);
        }

        private void InitLayoutSubmenu()
        {
            ToolStripMenuItem layoutMenuItem = new ToolStripMenuItem("Layout");
            _windowsMenu.DropDownItems.Add(layoutMenuItem);
            _windowsMenu.DropDownItems.Add(MENU_SEPARATOR);

            ToolStripMenuItem saveLayoutMenuItem = new ToolStripMenuItem("Save");
            ToolStripMenuItem loadLayoutMenuItem = new ToolStripMenuItem("Load");
            ToolStripMenuItem resetLayoutMenuItem = new ToolStripMenuItem("Reset to Defaults");
            layoutMenuItem.DropDownItems.Add(saveLayoutMenuItem);
            layoutMenuItem.DropDownItems.Add(loadLayoutMenuItem);
            layoutMenuItem.DropDownItems.Add(resetLayoutMenuItem);

            saveLayoutMenuItem.Click += saveLayoutMenuItem_Click;
            loadLayoutMenuItem.Click += loadLayoutMenuItem_Click;
            resetLayoutMenuItem.Click += resetLayoutMenuItem_Click;
        }

        private void resetLayoutMenuItem_Click(object sender, EventArgs e)
        {
            if (Factory.GUIController.ShowQuestion("Do you really want to reset the Editor window layout? This will close all the windows and rearrange the panels to their default positions.",
                MessageBoxIcon.Warning) == DialogResult.Yes)
            {
                Factory.GUIController.ResetWindowPanes();
            }
        }

        private void saveLayoutMenuItem_Click(object sender, EventArgs e)
        {
            string fileName = Factory.GUIController.ShowSaveFileDialog("Save layout...", XML_FILE_FILTER);
            if (fileName == null)
            {
                return;
            }
            _layoutManager.SaveLayout(fileName);
            Factory.GUIController.ShowMessage("Done!", MessageBoxIcon.Information);
        }

        private void loadLayoutMenuItem_Click(object sender, EventArgs e)
        {
            string fileName = Factory.GUIController.ShowOpenFileDialog("Load layout...", XML_FILE_FILTER);
            if (fileName == null)
            {
                return;
            }
            if (_layoutManager.LoadLayout(fileName) == WindowsLayoutManager.LayoutResult.OK)
            {
                Factory.GUIController.ShowMessage("Done!", MessageBoxIcon.Information);
            }
            else
            {
                Factory.GUIController.ShowMessage("Failed to load layout!", MessageBoxIcon.Error);
                Factory.GUIController.ResetWindowPanes();
            }
        }

        private void InitItems(List<DockContent> startupPanes)
        {
            _windowsMenu.DropDownItems.Add(MENU_SEPARATOR);
            RegisterPersistentItems(startupPanes);
        }

        public void AddPersistentItems(IList<DockContent> panes)
        {
            RegisterPersistentItems(panes);
        }

        public void AddPersistentItems(IList<ContentDocument> panes)
        {
            RegisterPersistentItems(panes);
        }

        // Find a place above the separator that splits persistent items
        // and dynamic document list
        private int GetPersistentItemInsertIndex()
        {
            int itemIndex = _windowsMenu.DropDownItems.Count;
            for (--itemIndex; itemIndex >= 0; --itemIndex)
            {
                if (_windowsMenu.DropDownItems[itemIndex] is ToolStripSeparator)
                    break;
            }
            return itemIndex;
        }

        private void RegisterPersistentItems(IList<DockContent> panes)
        {
            int itemIndex = GetPersistentItemInsertIndex();

            foreach (DockContent dockContent in panes)
            {
                ToolStripMenuItem menuItem = new ToolStripMenuItem(dockContent.Text);
                menuItem.CheckOnClick = true;
                UpdateMenuItemChecked(menuItem, dockContent);
                menuItem.Tag = dockContent;
                menuItem.CheckedChanged += StartupWindow_CheckedChanged;
                dockContent.DockStateChanged += StartupWindow_DockStateChanged;
                _windowsMenu.DropDownItems.Insert(itemIndex++, menuItem);
            }
        }

        private void RegisterPersistentItems(IList<ContentDocument> panes)
        {
            int itemIndex = GetPersistentItemInsertIndex();

            foreach (ContentDocument document in panes)
            {
                ToolStripMenuItem menuItem = new ToolStripMenuItem(document.Name);
                menuItem.CheckOnClick = true;
                UpdateMenuItemChecked(menuItem, document);
                menuItem.Tag = document;
                menuItem.CheckedChanged += StartupWindow_CheckedChanged;
                document.PanelClosed += Document_PanelClosed;
                _windowsMenu.DropDownItems.Insert(itemIndex++, menuItem);
            }
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

        private void Document_PanelClosed(object sender, EventArgs e)
        {
            ContentDocument document = sender as ContentDocument;
            if (document != null)
            {
                ToolStripMenuItem menuItem = GetMenuItem(document);
                UpdateMenuItemChecked(menuItem, document);
            }
        }

        private void UpdateMenuItemChecked(ToolStripMenuItem menuItem, DockContent dockContent)
        {
            if (menuItem != null)
            {
                menuItem.Checked = (dockContent.DockState != DockState.Hidden);
            }
        }

        private void UpdateMenuItemChecked(ToolStripMenuItem menuItem, ContentDocument document)
        {
            if (menuItem != null)
            {
                menuItem.Checked = document.Visible;
            }
        }

        private ToolStripMenuItem GetMenuItem(object pane)
        {
            foreach (ToolStripItem item in _windowsMenu.DropDownItems)
            {
                if (item.Tag == pane)
                {
                    return item as ToolStripMenuItem;
                }
            }
            return null;
        }

        private void StartupWindow_CheckedChanged(object sender, EventArgs e)
        {
            ToolStripMenuItem menuItem = sender as ToolStripMenuItem;
            if (menuItem == null || menuItem.Tag == null)
                return;

            if (menuItem.Tag is DockContent)
            {
                DockContent dockContent = menuItem.Tag as DockContent;
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
            else if (menuItem.Tag is ContentDocument)
            {
                ContentDocument document = menuItem.Tag as ContentDocument;
                if (menuItem.Checked)
                {
                    Factory.GUIController.AddOrShowPane(document);
                }
                else
                {
                    Factory.GUIController.RemovePaneIfExists(document);
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

        // Searches existing menu items from bottom up and removes them
        // until the first separator is found.
        // This is meant to remove Document items, which are normally
        // added after the fixed Pane items.
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
