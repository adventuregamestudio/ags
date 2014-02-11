using AGS.Types;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Data;
using System.Text;
using System.Windows.Forms;
using WeifenLuo.WinFormsUI.Docking;
using System.IO;
using System.Diagnostics;

namespace AGS.Editor
{
    public partial class TabbedDocumentManager
    {
        public delegate void ActiveDocumentChangeHandler(ContentDocument newActiveDocument);
        public event ActiveDocumentChangeHandler ActiveDocumentChanged;
        public event ActiveDocumentChangeHandler ActiveDocumentChanging;

        private delegate void DocumentsTitledChangedHandler();

        private const int TAB_HEIGHT = 18;
        private const string MENU_ITEM_CLOSE = "Close";
        private const string MENU_ITEM_CLOSE_ALL_BUT_THIS = "CloseAllOthers";
        private const string MENU_ITEM_NAVIGATE = "Navigate";

        private ContentDocument _currentPane;
        private List<ContentDocument> _panes;
        private List<ContentDocument> _panesInOrderUsed;
		//private System.Drawing.Font _selectedPaneFont = new System.Drawing.Font("Tahoma", 8, FontStyle.Bold);
		//private System.Drawing.Font _unselectedPaneFont = new System.Drawing.Font("Tahoma", 8, FontStyle.Regular);
        private IDockingPanel _dockPanel;
        
        public TabbedDocumentManager(DockPanel dockPanel)
            :this()
        {
            _dockPanel = new DockingPanel(dockPanel);            
        }

        public void Init()
        {
            _dockPanel.ActiveContentChanged += _dockPanel_ActiveContentChanged;            
        }

        void _dockPanel_ActiveContentChanged(object sender, EventArgs e)
        {
            /* When Ctrl+Tabbing, ActiveDocument is still the previously active tab, so let's double check which tab is actually active (i.e. has focus). */
            foreach (ContentDocument focusDocument in _panes)
                if (focusDocument.Control.ContainsFocus || focusDocument.Control.DockingContainer.ContainsFocus)
                {
                    if (focusDocument != ActiveDocument)
                    {
                        SetActiveDocument(focusDocument, false);
                        focusDocument.Control.DockingContainer.Refresh();
                        _dockPanel.ActivePane.Refresh();
                    }
                    break;
                }

            return;
        }        

        public TabbedDocumentManager()
        {
            //InitializeComponent();
            //btnClose.Image = Resources.ResourceManager.GetBitmap("CloseButton.ico");
            //btnListAll.Image = Resources.ResourceManager.GetBitmap("ListAllButton.ico");
            _panes = new List<ContentDocument>();
            _panesInOrderUsed = new List<ContentDocument>();
            //ShowControlAsEmpty();
            //toolTipManager.SetToolTip(btnClose, "Close");
            //toolTipManager.SetToolTip(btnListAll, "List active windows");
        }

        public static bool HoveringTabs { get; private set; }

        public bool ContainsDocument(ContentDocument pane)
        {
            return _panes.Contains(pane);
        }

        public ContentDocument ActiveDocument
        {
            get { return _currentPane; }
        }

        public void SetActiveDocument(ContentDocument pane)
        {
            SetActiveDocument(pane, true);
        }

        public void DocumentTitlesChanged()
        {
            if (Factory.GUIController.InvokeRequired)
            {
                Factory.GUIController.Invoke(new DocumentsTitledChangedHandler(DocumentTitlesChanged));
                return;
            }

            foreach (ContentDocument document in _panes)
            {
                document.Control.DockingContainer.Text = document.Name;
            }
            RefreshWindowsMenu();
            if (ActiveDocument != null)
            {
                ActiveDocument.Control.DockingContainer.Refresh();                
            }
        }

        private void RefreshWindowsMenu()
        {
            Factory.MenuManager.RefreshWindowsMenu(_panes, _currentPane); 
        }

        private void SetIcon(ContentDocument pane)
        {
            if (string.IsNullOrEmpty(pane.IconKey))
                pane.Control.DockingContainer.Icon = Factory.GUIController.MainIcon;
            else
            {
                Image iconImage = 
                    Factory.GUIController.ImageList.Images[pane.IconKey];
                pane.Control.DockingContainer.Icon = Utilities.ImageToIcon(iconImage);                    
            }
        }

        private void SetActiveDocument(ContentDocument pane, bool updatePaneOrder)
        {
            SetIcon(pane);
            if (!_panes.Contains(pane))
            {
                AddDocument(pane);
            }
            if (_currentPane != pane)
            {
                if (ActiveDocumentChanging != null)
                {
                    ActiveDocumentChanging(pane);
                }
                if (updatePaneOrder)
                {
                    _panesInOrderUsed.Remove(pane);
                    _panesInOrderUsed.Insert(0, pane);
                }
                if ((pane != _panes[0]) && (pane.TabWidth == 0))
                {
                    // if this pane's tab is not visible, move it to the start
                    _panes.Remove(pane);
                    _panes.Insert(0, pane);
                }
                if (_currentPane != null && _currentPane.Control != null)
                {
                    _currentPane.Control.WindowDeactivated();
                }
                _currentPane = pane;
                
                AttachTabContextMenu(pane);
                pane.Control.DockingContainer.Text = pane.Name;
                DockData dockData = GetDockData(pane);
                if (pane.Control.DockingContainer == null || pane.Control.DockingContainer.IsDisposed)
                {
                    pane.Control.DockingContainer = new DockingContainer(pane.Control);                    
                }                
                pane.Control.DockingContainer.Show(_dockPanel, dockData);

                pane.Control.DockingContainer.Focus();
                pane.Control.WindowActivated();
            }

			// Do this even if the pane hasn't changed, to ensure
			// that the property grid is refreshed
			if (ActiveDocumentChanged != null)
			{
				ActiveDocumentChanged(pane);
			}
            RefreshWindowsMenu();
		}

        private DockData GetDockData(ContentDocument pane)
        {
            if (pane.PreferredDockData != null) return pane.PreferredDockData;
            DockingState dockState = pane.Control.DockingContainer.DockState == DockingState.Unknown ||
                    pane.Control.DockingContainer.DockState == DockingState.Hidden ?
                    DockingState.Document : pane.Control.DockingContainer.DockState;
            return new DockData(dockState, Rectangle.Empty);
        }

        public void AddDocument(ContentDocument pane)
        {
            pane.Visible = true;
            pane.Control.Tag = pane;
            if (pane.Control.DockingContainer == null || pane.Control.DockingContainer.IsDisposed)
            {
                pane.Control.DockingContainer = new DockingContainer(pane.Control);                
            }            
            pane.Control.DockingContainer.DockStateChanged += Document_DockStateChanged;
            _panes.Insert(0, pane);
            _panesInOrderUsed.Insert(0, pane);            
        }

        void Document_DockStateChanged(object sender, EventArgs e)
        {
            if (_dockPanel.IsDisposed) return;
            DockingContainer dockingContainer = (DockingContainer)sender;
            EditorContentPanel panel = dockingContainer.Panel;
            ContentDocument docToRemove = null;
            if (panel.DockingContainer.DockState == DockingState.Unknown ||
                panel.DockingContainer.DockState == DockingState.Hidden)
            {
                foreach (ContentDocument document in _panes)
                {
                    if (document.Control == panel)
                    {
                        if (dockingContainer.IsShowing) dockingContainer.IsShowing = false;
                        else docToRemove = document;
                        break;
                    }
                }
            }
            
            if (docToRemove != null)
            {
                RemoveDocument(docToRemove);
            }
            else panel.DockingContainer.Refresh();
        }

        public void RemoveAllDocumentsExcept(ContentDocument pane)
        {
            List<ContentDocument> newPaneList = new List<ContentDocument>();

            ContentDocument[] copyOfPaneList = _panes.ToArray();
            foreach (ContentDocument doc in copyOfPaneList)
            {
                if (doc != pane)
                {
                    bool cancelled = false;
                    doc.Control.PanelClosing(true, ref cancelled);
                    if (cancelled)
                    {
                        newPaneList.Add(doc);
                    }
                    else
                    {
                        doc.Control.DockingContainer.Hide();
                        doc.Visible = false;                        
                    }
                }
            }

            _panes = newPaneList;
            _panes.Add(pane);
            _panesInOrderUsed = new List<ContentDocument>();
            foreach (ContentDocument doc in _panes) 
            {
                _panesInOrderUsed.Add(doc);
            }
            if (pane != _currentPane)
            {
                SetActiveDocument(pane);
            }
            else
            {
                RefreshWindowsMenu();
            }
            //tabsPanel.Invalidate();
        }

        public void RemoveDocument(ContentDocument pane)
        {
            RemoveDocument(pane, false);
        }

        private void RemoveDocument(ContentDocument pane, bool canCancel)
        {
            bool cancelled = false;
            pane.Control.PanelClosing(canCancel, ref cancelled);
            if (canCancel && cancelled)
            {
                return;
            }
            if (pane.Control.DockingContainer.DockState != DockingState.Hidden &&
                pane.Control.DockingContainer.DockState != DockingState.Unknown)
            {
                pane.Control.DockingContainer.Hide();
            }
            
            pane.Visible = false;
            _panes.Remove(pane);
            _panesInOrderUsed.Remove(pane);

            if (pane == _currentPane)
            {
                if (_panes.Count > 0)
                {
                    SetActiveDocument(_panesInOrderUsed[0]);
                }
                else
                {
                    _currentPane = null;
                    
                    if (ActiveDocumentChanged != null)
                    {
                        ActiveDocumentChanged(null);
                    }                    
                }
            }
            RefreshWindowsMenu();            
        }

        private int _flipThroughPanesIndex = 0;

        public bool ProcessKeyDown(Keys key)
        {
            if ((key == (Keys.Control | Keys.F4)) ||
				(key == (Keys.Control | Keys.W)))
            {
                if (_currentPane != null)
                {
                    RemoveDocument(_currentPane, true);
                }
                return true;
            }
            else if (key == (Keys.Tab | Keys.Control))
            {
                // Cycle through forwards
                if (_panes.Count > 1)
                {
                    _flipThroughPanesIndex++;
                    if (_flipThroughPanesIndex >= _panes.Count)
                    {
                        _flipThroughPanesIndex = 0;
                    }
                    SetActiveDocument(_panesInOrderUsed[_flipThroughPanesIndex], false);
                }
                return true;
            }
            else if (key == (Keys.Tab | Keys.Control | Keys.Shift))
            {
                // Cycle through backwards
                if (_panes.Count > 1)
                {
                    _flipThroughPanesIndex--;
                    if (_flipThroughPanesIndex < 0)
                    {
                        _flipThroughPanesIndex = _panes.Count - 1;
                    }
                    SetActiveDocument(_panesInOrderUsed[_flipThroughPanesIndex], false);
                }
                return true;
            }
			else if (_currentPane != null)
			{
				return _currentPane.Control.KeyPressed(key);
			}
			return false;
        }

        public bool ProcessKeyUp(Keys key)
        {
            if (key == Keys.ControlKey)
            {
                if (_flipThroughPanesIndex < _panes.Count)
                {
                    _panesInOrderUsed.Remove(_currentPane);
                    _panesInOrderUsed.Insert(0, _currentPane);
                }
                _flipThroughPanesIndex = 0;
            }
            return false;
        }

        /*private void ShowControlAsEmpty()
        {
            contentPane1.Visible = false;
            tabsPanel.Visible = false;
            this.BackColor = Color.FromKnownColor(KnownColor.Gray);
        }*/

        /*private void UpdateSize()
        {
            if (_currentPane != null)
            {
                UpdateSize(_currentPane);
            }
        }*/

        /*private void UpdateSize(ContentDocument pane)
        {
            pane.Control.Size = contentPane1.ClientSize;
        }

        private void TabbedDocumentContainer_Resize(object sender, EventArgs e)
        {
            UpdateSize();
        }*/

        /*private void tabsPanel_Paint(object sender, PaintEventArgs e)
        {
            int x = 0, y = 3;
            foreach (ContentDocument pane in _panes)
            {
                pane.TabWidth = 0;
                pane.TabXOffset = -1;
            }

            foreach (ContentDocument pane in _panes)
            {
                if (!DrawPane(e.Graphics, pane, (pane == _currentPane), x, y))
                {
                    break;
                }
                x += pane.TabWidth;
            }
        }*/

        /*private bool DrawPane(Graphics graphics, ContentDocument pane, bool selected, int x, int y)
        {
            Pen borderPen = Pens.Gray;
            int textWidth = (int)graphics.MeasureString(pane.Name, _selectedPaneFont).Width + 5;
            if (x + TAB_HEIGHT + textWidth >= btnListAll.Left)
            {
                // going off the edge of the screen
                return false;
            }
            GraphicsPath path = new GraphicsPath();
            path.AddLine(x, y + TAB_HEIGHT, x + TAB_HEIGHT, y);
            path.AddLine(x + TAB_HEIGHT, y, x + TAB_HEIGHT + textWidth, y);
            path.AddLine(x + TAB_HEIGHT + textWidth, y, x + TAB_HEIGHT + textWidth, y + TAB_HEIGHT);
            path.CloseFigure();
            if (selected)
            {
                graphics.FillPath(Brushes.White, path);
            }
            graphics.DrawPath(borderPen, path);

			System.Drawing.Font fontToUse = _unselectedPaneFont;
            if (selected)
            {
                fontToUse = _selectedPaneFont;
            }
            graphics.DrawString(pane.Name, fontToUse, Brushes.Black, x + TAB_HEIGHT, y + 2);

            graphics.DrawLine(borderPen, 0, tabsPanel.ClientSize.Height - 1, tabsPanel.ClientSize.Width - 1, tabsPanel.ClientSize.Height - 1);

            pane.TabXOffset = x;
            pane.TabWidth = TAB_HEIGHT + textWidth;
            return true;
        }*/

        /*private void tabsPanel_MouseUp(object sender, MouseEventArgs e)
        {
            foreach (ContentDocument pane in _panes)
            {
                if (e.X < pane.TabXOffset + pane.TabWidth)
                {
                    SetActiveDocument(pane);
                    if (e.Button == MouseButtons.Right)
                    {
                        ShowTabContextMenu(pane, e.Location);
                    }
                    else if (e.Button == MouseButtons.Middle)
                    {
                        if (_currentPane != null)
                        {
                            RemoveDocument(_currentPane, true);
                        }
                    }
                    break;
                }
            }
        }*/

        private void AttachTabContextMenu(ContentDocument document)
        {            
            EventHandler onClick = new EventHandler(TreeContextMenuEventHandler);
            ContextMenuStrip menu = new ContextMenuStrip();
            menu.Tag = document;
            menu.Items.Add(new ToolStripMenuItem("Close", null, onClick, MENU_ITEM_CLOSE));
            menu.Items.Add(new ToolStripMenuItem("Close all others", null, onClick, MENU_ITEM_CLOSE_ALL_BUT_THIS));
            if (document.TreeNodeID != null)
            {
                menu.Items.Add(new ToolStripMenuItem("Navigate (In Tree)", null, onClick, MENU_ITEM_NAVIGATE));
            }
            document.Control.DockingContainer.TabPageContextMenuStrip = menu;
        }

        private void TreeContextMenuEventHandler(object sender, EventArgs e)
        {
            ToolStripMenuItem item = (ToolStripMenuItem)sender;
            ContentDocument document = (ContentDocument)item.Owner.Tag;
            if (item.Name == MENU_ITEM_CLOSE)
            {
                RemoveDocument(document, true);
            }
            else if (item.Name == MENU_ITEM_CLOSE_ALL_BUT_THIS)
            {
                if (MessageBox.Show("Are you sure you want to close all other tabs?", "Confirm close", MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.Yes)
                {
                    RemoveAllDocumentsExcept(document);
                }
            }
            else if (item.Name == MENU_ITEM_NAVIGATE)
            {
                Factory.GUIController.ProjectTree.SelectNode(null, document.TreeNodeID);
 
            }
        }

        private void btnClose_Click(object sender, EventArgs e)
        {
            if (_currentPane != null)
            {
                RemoveDocument(_currentPane, true);
            }
        }

        private void DocumentListMenuEventHandler(object sender, EventArgs e)
        {
            ToolStripMenuItem item = (ToolStripMenuItem)sender;
            ContentDocument document = (ContentDocument)item.Tag;
            SetActiveDocument(document);
        }

        /*private void btnListAll_Click(object sender, EventArgs e)
        {
            EventHandler onClick = new EventHandler(DocumentListMenuEventHandler);
            ContextMenuStrip menu = new ContextMenuStrip();
            foreach (ContentDocument document in _panes)
            {
                ToolStripMenuItem menuItem = new ToolStripMenuItem(document.Name, null, onClick);
                menuItem.Tag = document;
                menu.Items.Add(menuItem);
            }
            menu.Show(tabsPanel, new Point(btnListAll.Left, btnListAll.Bottom));
        }*/

        /*private void tabsPanel_MouseEnter(object sender, EventArgs e)
        {
            HoveringTabs = true;
        }

        private void tabsPanel_MouseLeave(object sender, EventArgs e)
        {
           HoveringTabs = false;
        }*/
    }
}
