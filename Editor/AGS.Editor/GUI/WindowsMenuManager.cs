using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
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
        // Indexes of the first and last persistent items in the menu,
        // and an index where current documents should be added beneath
        private int _firstPaneIndex, _lastPaneIndex, _documentInsertIndex;
        private const string MENU_SEPARATOR = "-";
        private const string XML_FILE_FILTER = "Xml files (*.xml)|*.xml";

        internal WindowsMenuManager(ToolStripMenuItem windowsMenu,
            IList<DockContent> dockPanes, DockPanel dockPanel, WindowsLayoutManager layoutManager)
        {
            _windowsMenu = windowsMenu;
            _dockPanel = dockPanel;
            _layoutManager = layoutManager;
            InitLayoutSubmenu();
            InitItems(dockPanes);
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

        /// <summary>
        /// Inits Window items with the list of the persistent panes.
        /// </summary>
        private void InitItems(IList<DockContent> dockPanes)
        {
            _firstPaneIndex = _windowsMenu.DropDownItems.Count;
            _lastPaneIndex = _firstPaneIndex;
            RegisterPersistentItems(dockPanes);
            _windowsMenu.DropDownItems.Add(MENU_SEPARATOR);
        }

        /// <summary>
        /// Adds persistent Window items, that is the items that should
        /// always be available in the menu, even when the pane is closed.
        /// </summary>
        public void AddPersistentItems(IList<DockContent> panes)
        {
            RegisterPersistentItems(panes);
        }

        // Register persistent Window items
        private void RegisterPersistentItems(IList<DockContent> panes)
        {
            int itemIndex = _lastPaneIndex;

            foreach (DockContent dockContent in panes)
            {
                if (_windowsMenu.DropDownItems.Cast<ToolStripItem>().Count(item => item.Tag == dockContent) > 0)
                    continue;

                ToolStripMenuItem menuItem = new ToolStripMenuItem(dockContent.Text);
                menuItem.CheckOnClick = true;
                UpdateMenuItemChecked(menuItem, dockContent);
                menuItem.Tag = dockContent;
                menuItem.CheckedChanged += DockPane_CheckedChanged;
                dockContent.DockStateChanged += DockPane_DockStateChanged;
                _windowsMenu.DropDownItems.Insert(itemIndex++, menuItem);
            }

            _lastPaneIndex = itemIndex;
            _documentInsertIndex = itemIndex + 1; // skip MENU_SEPARATOR
        }

        private void DockPane_DockStateChanged(object sender, EventArgs e)
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
                menuItem.Checked =
                    dockContent.DockState != DockState.Unknown &&
                    dockContent.DockState != DockState.Hidden;
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

        private void DockPane_CheckedChanged(object sender, EventArgs e)
        {
            ToolStripMenuItem menuItem = sender as ToolStripMenuItem;
            if (menuItem == null)
                return;

            DockContent dockContent = menuItem.Tag as DockContent;
            if (dockContent == null)
                return;

            // Safety check; should add a error popup?
            if (dockContent.DockPanel == null)
                return;

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

        /// <summary>
        /// Resets the list of Documents in the menu.
        /// </summary>
        public void Refresh(List<ContentDocument> documents, ContentDocument activeDocument)
        {
            RemoveAllDocuments();
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

        // Removes all menu items refering to the Content Documents
        // added after the fixed Pane items.
        private void RemoveAllDocuments()
        {
            for (int i = _windowsMenu.DropDownItems.Count - 1; i >= _documentInsertIndex; i--)
            {
                ToolStripItem menuItem = _windowsMenu.DropDownItems[i];
                menuItem.Click -= MenuItem_Click;
                _windowsMenu.DropDownItems.RemoveAt(i);
            }
        }
    }
}
