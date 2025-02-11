using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
using System.Text;
using System.Windows.Forms;
using AGS.Types;
using AGS.Editor.TextProcessing;
using WeifenLuo.WinFormsUI.Docking;
using System.Diagnostics;

namespace AGS.Editor
{
    public partial class frmMain : Form
    {
        public delegate bool EditorShutdownHandler();
        public delegate void ActiveDocumentChangedHandler();
        public delegate void PropertyChangedHandler(string propertyName, object oldValue);
        public delegate void PropertyObjectChangedHandler(object newPropertyObject);
        public event EditorShutdownHandler OnEditorShutdown;
        public event PropertyChangedHandler OnPropertyChanged;
        public event PropertyObjectChangedHandler OnPropertyObjectChanged;
        public event ActiveDocumentChangedHandler OnActiveDocumentChanged;
        public event EventHandler OnMainWindowActivated;

        private TabbedDocumentManager.ActiveDocumentChangeHandler _activeDocumentChanged;
        private TabbedDocumentManager.ActiveDocumentChangeHandler _activeDocumentChanging;

        private Dictionary<string, object> _propertyObjectList = null;
        private bool _ignorePropertyListChange = false;
		private bool _suspendDrawing = false;
        private List<DockContent> _dockPanes = new List<DockContent>();
        private WindowsLayoutManager _layoutManager;

        public frmMain()
        {
            InitializeComponent();

            _dockPanes.AddRange(GetStartupPanes());
            _layoutManager = new WindowsLayoutManager(mainContainer, _dockPanes);
            _activeDocumentChanged = new TabbedDocumentManager.ActiveDocumentChangeHandler(tabbedDocumentContainer1_ActiveDocumentChanged);
            _activeDocumentChanging = new TabbedDocumentManager.ActiveDocumentChangeHandler(tabbedDocumentContainer1_ActiveDocumentChanging);
            tabbedDocumentContainer1.ActiveDocumentChanged += _activeDocumentChanged;
            tabbedDocumentContainer1.ActiveDocumentChanging += _activeDocumentChanging;
			this.Load += new EventHandler(frmMain_Load);
            this.Activated += new EventHandler(frmMain_Activated);
            this.Deactivate += new EventHandler(frmMain_Deactivated);
        }

        private List<DockContent> GetStartupPanes()
        {
            return new List<DockContent>
            {
                projectPanel,
                propertiesPanel,
                pnlOutput,
                pnlFindResults,
                pnlCallStack,
                pnlWatchVariables
            };
        }

        public WindowsLayoutManager GetLayoutManager()
        {
            return _layoutManager;
        }

        private void frmMain_Activated(object sender, EventArgs e)
        {
            if (OnMainWindowActivated != null)
            {
                OnMainWindowActivated(this, e);
            }
        }

        private void frmMain_Deactivated(object sender, EventArgs e)
        {
            if (Form.ActiveForm == null)
            {
                FindReplace.CloseDialogIfNeeded();
            }
        }

		private void frmMain_Load(object sender, EventArgs e)
		{
            if (!DesignMode)
            {
                LoadLayout();
                Factory.GUIController.LoadWindowConfig();
                Factory.GUIController.ColorThemes.Apply(LoadColorTheme);
            }
        }

        public void RefreshPropertyGridForDocument(ContentDocument document)
        {
            if (document == null)
            {
                this.SetPropertyObjectList(null);
                this.SetPropertyObject(null);
            }
            else
            {
                this.SetPropertyObjectList(document.PropertyGridObjectList);
                if (document.SelectedPropertyGridObjects != null)
                {
                    this.SetPropertyObjects(document.SelectedPropertyGridObjects);
                }
                else
                {
                    this.SetPropertyObject(document.SelectedPropertyGridObject);
                }
            }
        }

        private void removeTabbedDocumentEventHandlers()
        {
            // we remove these events to prevent an exception when closing AGS with an object selected in the property grid
            tabbedDocumentContainer1.ActiveDocumentChanged -= _activeDocumentChanged;
            tabbedDocumentContainer1.ActiveDocumentChanging -= _activeDocumentChanging;
        }

        private void tabbedDocumentContainer1_ActiveDocumentChanging(ContentDocument newActiveDocument)
        {
            if (tabbedDocumentContainer1.ActiveDocument != null)
            {
                // Remember which pane and item were selected on the property grid,
                // so that we can restore them later
                tabbedDocumentContainer1.ActiveDocument.SelectedPropertyGridTab = 
                    propertiesPanel.SelectedTab.TabName;
                if (propertiesPanel.SelectedGridItem != null)
                {
                    tabbedDocumentContainer1.ActiveDocument.SelectedPropertyGridItem = 
                        propertiesPanel.SelectedGridItem.Label;
                }                
            }
        }

        private GridItem RecursiveFindGridItem(GridItem startFromHere, string fullTypeName)
        {
            foreach (GridItem item in startFromHere.GridItems)
            {
                if (item.Label == fullTypeName)
                {
                    return item;
                }

                if (item.GridItems.Count > 0)
                {
                    GridItem returned = RecursiveFindGridItem(item, fullTypeName);
                    if (returned != null)
                    {
                        return returned;
                    }
                }
            }

            return null;
        }

        private GridItem FindPropertyGridItemForType(string fullTypeName)
        {
            GridItem startFromHere = propertiesPanel.SelectedGridItem;
            if (startFromHere == null)
            {
                return null;
            }

            while (startFromHere.Parent != null)
            {
                startFromHere = startFromHere.Parent;
            }

            return RecursiveFindGridItem(startFromHere, fullTypeName);
        }

        private void RestoreSelectedPropertyGridItem(ContentDocument newActiveDocument)
        {
            if (!string.IsNullOrEmpty(newActiveDocument.SelectedPropertyGridTab))
            {
                SelectTabInPropertyGrid(newActiveDocument.SelectedPropertyGridTab);
            }
            if (!string.IsNullOrEmpty(newActiveDocument.SelectedPropertyGridItem))
            {
                GridItem itemToSelect = FindPropertyGridItemForType(newActiveDocument.SelectedPropertyGridItem);
                if (itemToSelect != null)
                {
                    if ((itemToSelect.Parent != null) && (!itemToSelect.Parent.Expanded))
                    {
                        propertiesPanel.ExpandAllGridItems();
                    }
                    propertiesPanel.SelectedGridItem = itemToSelect;
                }
            }
        }

        private void tabbedDocumentContainer1_ActiveDocumentChanged(ContentDocument newActiveDocument)
        {
            if (this == null) return;
            RefreshPropertyGridForDocument(newActiveDocument);

            if (newActiveDocument != null)
            {
                RestoreSelectedPropertyGridItem(newActiveDocument);
            }

            if (OnActiveDocumentChanged != null)
            {
                OnActiveDocumentChanged();
            }
        }

		public void SetDrawingSuspended(bool isSuspended)
		{
			_suspendDrawing = isSuspended;

			foreach (Control control in this.Controls)
			{
				control.Visible = !_suspendDrawing;
			}
		}

        public void AddDockPane(DockContent pane, DockData defaultDock)
        {
            if (!_dockPanes.Contains(pane))
            {
                _dockPanes.Add(pane);
                pane.ShowHint = (DockState)defaultDock.DockState;
                if (defaultDock.DockState == DockingState.Float)
                    pane.Show(mainContainer, defaultDock.Location);
                else
                    pane.Show(mainContainer, (DockState)defaultDock.DockState);
            }
        }

        public void AddOrShowPane(ContentDocument pane)
        {
            if (!tabbedDocumentContainer1.ContainsDocument(pane))
            {
                tabbedDocumentContainer1.AddDocument(pane);
            }
            tabbedDocumentContainer1.SetActiveDocument(pane);
        }

        public void RemovePaneIfExists(ContentDocument pane)
        {
            if (tabbedDocumentContainer1.ContainsDocument(pane))
            {
                tabbedDocumentContainer1.RemoveDocument(pane);
            }
        }

        public IList<DockContent> DockPanes
        {
            get
            {
                return _dockPanes;
            }
        }

        public IList<ContentDocument> Panes
        {
            get
            {
                return tabbedDocumentContainer1.Documents;
            }
        }

        public ContentDocument ActivePane
        {
            get { return tabbedDocumentContainer1.ActiveDocument; }
        }

        public void DocumentTitlesChanged()
        {
            tabbedDocumentContainer1.DocumentTitlesChanged();
        }

        public void SetTreeImageList(ImageList imageList)
        {
            projectPanel.projectTree.ImageList = imageList;
        }

        public bool SelectTabInPropertyGrid(string tabName)
        {
            return propertiesPanel.SelectTabInPropertyGrid(tabName);
        }

        public Point GetPropertyGridScreenCoordinates()
        {
            return propertiesPanel.PointToScreen(propertiesPanel.Location);
        }

        public void SelectPropertyByName(string propertyName)
        {
            propertiesPanel.Focus();
            // The property grid provides no RootGridItem property,
            // so we must find it manually
            GridItem rootItem = propertiesPanel.SelectedGridItem;
            while (rootItem.GridItemType != GridItemType.Root)
            {
                rootItem = rootItem.Parent;
            }

            foreach (GridItem item in rootItem.GridItems)
            {
                if ((item.GridItemType == GridItemType.Property) && (item.Label == propertyName))
                {
                    item.Select();
                    return;
                }

                // search within categories too
                foreach (GridItem subItem in item.GridItems)
                {
                    if ((subItem.GridItemType == GridItemType.Property) && (subItem.Label == propertyName))
                    {
                        subItem.Select();
                        return;
                    }
                }
            }
        }

        public void SetPropertyObjectList(Dictionary<string, object> propertyObjects)
        {
            try
            {
                _ignorePropertyListChange = true;

                object previouslySelected = null;
                if ((_propertyObjectList != null) && (propertiesPanel.propertyObjectCombo.SelectedItem != null))
                {
                    previouslySelected = _propertyObjectList[(string)propertiesPanel.propertyObjectCombo.SelectedItem];
                }

                _propertyObjectList = propertyObjects;

                propertiesPanel.propertyObjectCombo.Items.Clear();
                if (_propertyObjectList != null)
                {
                    foreach (string name in _propertyObjectList.Keys)
                    {
                        propertiesPanel.propertyObjectCombo.Items.Add(name);
                        if (_propertyObjectList[name] == previouslySelected)
                        {
                            propertiesPanel.propertyObjectCombo.SelectedItem = 
                                propertiesPanel.propertyObjectCombo.Items[propertiesPanel.propertyObjectCombo.Items.Count - 1];
                        }
                    }
                    propertiesPanel.propertyObjectCombo.Enabled = true;
                }
                else
                {
                    propertiesPanel.propertyObjectCombo.Enabled = false;
                }
            }
            finally
            {
                _ignorePropertyListChange = false;
            }
        }

        public void SetPropertyObject(object propertiesObject)
        {
            propertiesPanel.SelectedObject = propertiesObject;
            SelectObjectInPropertyList(propertiesObject);
        }

        public void SetPropertyObjects(object[] propertiesObjects)
        {
            propertiesPanel.SelectedObjects = propertiesObjects;
            propertiesPanel.propertyObjectCombo.SelectedIndex = -1;
        }

        private void SelectObjectInPropertyList(object propertiesObject)
        {
            try
            {
                _ignorePropertyListChange = true;
                if (_propertyObjectList != null)
                {
                    foreach (string name in _propertyObjectList.Keys)
                    {
                        if (_propertyObjectList[name] == propertiesObject)
                        {
                            propertiesPanel.propertyObjectCombo.SelectedIndex =
                                propertiesPanel.propertyObjectCombo.Items.IndexOf(name);
                            break;
                        }
                    }
                }
            }
            finally
            {
                _ignorePropertyListChange = false;
            }
        }

        private void frmMain_FormClosing(object sender, FormClosingEventArgs e)
        {
            if (OnEditorShutdown != null)
            {
                e.Cancel = !OnEditorShutdown();
            }
            if (!e.Cancel)
            {
                // Explicitly remove all document panes in order to trigger their OnPanelClosing,
                // where they possibly save their states to a WindowConfig.
                tabbedDocumentContainer1.RemoveAllDocuments(false);
                Factory.GUIController.SaveWindowConfig();
                _layoutManager.SaveLayout();
            }
        }

        private void propertiesPanel_PropertyValueChanged(object s, PropertyValueChangedEventArgs e)
        {
            //tabbedDocumentContainer1.Invalidate(true);

            if (OnPropertyChanged != null)
            {
                OnPropertyChanged(e.ChangedItem.Label, e.OldValue);
            }
        }

        private void frmMain_Shown(object sender, EventArgs e)
        {
            this.tabbedDocumentContainer1.Init();

            ShowTabIcons = Factory.AGSEditor.Settings.ShowIconInTab;
            Factory.GUIController.ShowWelcomeScreen();
        }

        private void LoadLayout()
        {
            var res = _layoutManager.LoadLayout();
            if (res == WindowsLayoutManager.LayoutResult.LayoutException)
                Factory.GUIController.ShowMessage("Failed to load panel layout: the layout file may be corrupt or contains invalid data. The layout will be reset.", MessageBoxIcon.Error);
            if (res != WindowsLayoutManager.LayoutResult.OK)
            {
                ResetLayoutToDefaults();
            }
        }

        /// <summary>
        /// Resets Editor layout.
        /// </summary>
        public void ResetLayoutToDefaults()
        {
            // Ask layout manager to restore using its own defaults
            if (_layoutManager.ResetToDefaults() != WindowsLayoutManager.LayoutResult.OK)
            {
                SetDefaultLayout(); // last chance fallback
            }
        }

        /// <summary>
        /// Sets hard-coded layout; this is a last resort action.
        /// </summary>
        private void SetDefaultLayout()
        {
            _layoutManager.DetachAll();
            mainContainer.DockLeftPortion = 0.25f;
            mainContainer.DockRightPortion = 0.25f;
            mainContainer.DockTopPortion = 0.25f;
            mainContainer.DockBottomPortion = 0.25f;
            projectPanel.Show(mainContainer, DockState.DockRight);
            propertiesPanel.Show(projectPanel.Pane, DockAlignment.Bottom, 0.5f);
            pnlCallStack.Show(mainContainer, DockState.DockBottom);
            pnlFindResults.Show(pnlCallStack.Pane, pnlCallStack);
            pnlOutput.Show(pnlCallStack.Pane, pnlFindResults);
            _layoutManager.RestoreDetached();
        }

        private void propertyObjectCombo_SelectedIndexChanged(object sender, EventArgs e)
        {
            if ((propertiesPanel.propertyObjectCombo.SelectedItem != null) && 
                (!_ignorePropertyListChange))
            {
                object newObject = _propertyObjectList[
                    (string)propertiesPanel.propertyObjectCombo.SelectedItem];
                SetPropertyObject(newObject);

                if (OnPropertyObjectChanged != null)
                {
                    OnPropertyObjectChanged(newObject);
                }
            }
        }

        private void tabbedDocumentContainer1_Enter(object sender, EventArgs e)
        {
            RefreshPropertyGridForDocument(tabbedDocumentContainer1.ActiveDocument);
        }

        private void frmMain_KeyUp(object sender, KeyEventArgs e)
        {
			if (!projectPanel.Focused)
			{
				e.Handled = tabbedDocumentContainer1.ProcessKeyUp(e.KeyData);
			}
        }

        private void frmMain_KeyDown(object sender, KeyEventArgs e)
        {
            if (!projectPanel.Focused)
			{
				e.Handled = tabbedDocumentContainer1.ProcessKeyDown(e.KeyData);
				e.SuppressKeyPress = e.Handled;
			}
        }

        private void projectTree_Enter(object sender, EventArgs e)
        {
			// Actually, let's not clear the property grid when
			// you click in the tree. It's an unpopular feature!
            //SetPropertyObjectList(null);
            //SetPropertyObject(null);
        }

        public bool ShowTabIcons
        {
            get { return mainContainer.ShowDocumentIcon; }
            set { mainContainer.ShowDocumentIcon = value; }
        }

        private void LoadColorTheme(ColorTheme t)
        {
            t.SetColor("background", c => BackColor = c);
            t.SetColor(new string[] { "main-container/dock-background", "background" }, c => mainContainer.DockBackColor = c);
            t.SetColor("main-container/background", c => mainContainer.BackColor = c);
            t.SetColor("main-container/foreground", c => mainContainer.ForeColor = c);
            // VS2005 theme only (skin)
            t.SetColor("main-container/skin/auto-hide/tab-gradient/start", c => mainContainer.Theme.Skin.AutoHideStripSkin.TabGradient.StartColor = c);
            t.SetColor("main-container/skin/auto-hide/tab-gradient/end", c => mainContainer.Theme.Skin.AutoHideStripSkin.TabGradient.EndColor = c);
            t.SetColor("main-container/skin/auto-hide/tab-gradient/text", c => mainContainer.Theme.Skin.AutoHideStripSkin.TabGradient.TextColor = c);
            t.SetColor("main-container/skin/auto-hide/dock-strip-gradient/start", c => mainContainer.Theme.Skin.AutoHideStripSkin.DockStripGradient.StartColor = c);
            t.SetColor("main-container/skin/auto-hide/dock-strip-gradient/end", c => mainContainer.Theme.Skin.AutoHideStripSkin.DockStripGradient.EndColor = c);
            t.SetColor("main-container/skin/dock-pane/document-gradient/dock-strip-gradient/start", c => mainContainer.Theme.Skin.DockPaneStripSkin.DocumentGradient.DockStripGradient.StartColor = c);
            t.SetColor("main-container/skin/dock-pane/document-gradient/dock-strip-gradient/end", c => mainContainer.Theme.Skin.DockPaneStripSkin.DocumentGradient.DockStripGradient.EndColor = c);
            t.SetColor("main-container/skin/dock-pane/document-gradient/active-tab-gradient/start", c => mainContainer.Theme.Skin.DockPaneStripSkin.DocumentGradient.ActiveTabGradient.StartColor = c);
            t.SetColor("main-container/skin/dock-pane/document-gradient/active-tab-gradient/end", c => mainContainer.Theme.Skin.DockPaneStripSkin.DocumentGradient.ActiveTabGradient.EndColor = c);
            t.SetColor("main-container/skin/dock-pane/document-gradient/active-tab-gradient/text", c => mainContainer.Theme.Skin.DockPaneStripSkin.DocumentGradient.ActiveTabGradient.TextColor = c);
            t.SetColor("main-container/skin/dock-pane/document-gradient/inactive-tab-gradient/start", c => mainContainer.Theme.Skin.DockPaneStripSkin.DocumentGradient.InactiveTabGradient.StartColor = c);
            t.SetColor("main-container/skin/dock-pane/document-gradient/inactive-tab-gradient/end", c => mainContainer.Theme.Skin.DockPaneStripSkin.DocumentGradient.InactiveTabGradient.EndColor = c);
            t.SetColor("main-container/skin/dock-pane/document-gradient/inactive-tab-gradient/text", c => mainContainer.Theme.Skin.DockPaneStripSkin.DocumentGradient.InactiveTabGradient.TextColor = c);
            t.SetColor("main-container/skin/dock-pane/tool-window/active-caption-gradient/start", c => mainContainer.Theme.Skin.DockPaneStripSkin.ToolWindowGradient.ActiveCaptionGradient.StartColor = c);
            t.SetColor("main-container/skin/dock-pane/tool-window/active-caption-gradient/end", c => mainContainer.Theme.Skin.DockPaneStripSkin.ToolWindowGradient.ActiveCaptionGradient.EndColor = c);
            t.SetColor("main-container/skin/dock-pane/tool-window/active-caption-gradient/text", c => mainContainer.Theme.Skin.DockPaneStripSkin.ToolWindowGradient.ActiveCaptionGradient.TextColor = c);
            t.SetColor("main-container/skin/dock-pane/tool-window/inactive-caption-gradient/start", c => mainContainer.Theme.Skin.DockPaneStripSkin.ToolWindowGradient.InactiveCaptionGradient.StartColor = c);
            t.SetColor("main-container/skin/dock-pane/tool-window/inactive-caption-gradient/end", c => mainContainer.Theme.Skin.DockPaneStripSkin.ToolWindowGradient.InactiveCaptionGradient.EndColor = c);
            t.SetColor("main-container/skin/dock-pane/tool-window/inactive-caption-gradient/text", c => mainContainer.Theme.Skin.DockPaneStripSkin.ToolWindowGradient.InactiveCaptionGradient.TextColor = c);
            t.SetColor("main-container/skin/dock-pane/tool-window/active-tab-gradient/start", c => mainContainer.Theme.Skin.DockPaneStripSkin.ToolWindowGradient.ActiveTabGradient.StartColor = c);
            t.SetColor("main-container/skin/dock-pane/tool-window/active-tab-gradient/end", c => mainContainer.Theme.Skin.DockPaneStripSkin.ToolWindowGradient.ActiveTabGradient.EndColor = c);
            t.SetColor("main-container/skin/dock-pane/tool-window/active-tab-gradient/text", c => mainContainer.Theme.Skin.DockPaneStripSkin.ToolWindowGradient.ActiveTabGradient.TextColor = c);
            t.SetColor("main-container/skin/dock-pane/tool-window/inactive-tab-gradient/start", c => mainContainer.Theme.Skin.DockPaneStripSkin.ToolWindowGradient.InactiveTabGradient.StartColor = c);
            t.SetColor("main-container/skin/dock-pane/tool-window/inactive-tab-gradient/end", c => mainContainer.Theme.Skin.DockPaneStripSkin.ToolWindowGradient.InactiveTabGradient.EndColor = c);
            t.SetColor("main-container/skin/dock-pane/tool-window/inactive-tab-gradient/text", c => mainContainer.Theme.Skin.DockPaneStripSkin.ToolWindowGradient.InactiveTabGradient.TextColor = c);
            t.SetColor("main-container/skin/dock-pane/tool-window/dock-strip-gradient/start", c => mainContainer.Theme.Skin.DockPaneStripSkin.ToolWindowGradient.DockStripGradient.StartColor = c);
            t.SetColor("main-container/skin/dock-pane/tool-window/dock-strip-gradient/end", c => mainContainer.Theme.Skin.DockPaneStripSkin.ToolWindowGradient.DockStripGradient.EndColor = c);

            if ( t.Has("main-menu") )
            {
                t.SetColor("main-menu/background", c => mainMenu.BackColor = c);
                mainMenu.Renderer = t.GetMainMenuRenderer("main-menu");
                mainMenu.Renderer.RenderItemText += (s, a) => a.ToolStrip.ForeColor = t.GetColor("main-menu/foreground");
                mainMenu.Renderer.RenderItemCheck += (s, a) => a.Graphics.DrawImage(t.GetImage("main-menu/check/foreground", a.Image), a.ImageRectangle);
            }

            if (t.Has("tool-bar"))
            {
                t.SetColor("tool-bar/background", c => mainMenu.BackColor = c);
                toolStrip.Renderer = t.GetToolStripRenderer("tool-bar");
            }
            
            t.SetColor("status-strip/background", c => statusStrip.BackColor = c);
            t.SetColor("status-strip/foreground", c => statusStrip.ForeColor = c);
        }
    }
}