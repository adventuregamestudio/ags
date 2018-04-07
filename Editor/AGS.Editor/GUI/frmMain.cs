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

        private Dictionary<string, object> _propertyObjectList = null;
        private bool _ignorePropertyListChange = false;
		//private int _splitterXtoSet = 0;
		//private int _splitterYtoSet = 0;
		private bool _suspendDrawing = false;
        private WindowsLayoutManager _layoutManager;

        public frmMain()
        {
            InitializeComponent();
            Factory.GUIController.ColorThemes.Apply(LoadColorTheme);

            _layoutManager = new WindowsLayoutManager(mainContainer, GetStartupPanes());            
            tabbedDocumentContainer1.ActiveDocumentChanged += new TabbedDocumentManager.ActiveDocumentChangeHandler(tabbedDocumentContainer1_ActiveDocumentChanged);
            tabbedDocumentContainer1.ActiveDocumentChanging += new TabbedDocumentManager.ActiveDocumentChangeHandler(tabbedDocumentContainer1_ActiveDocumentChanging);
			this.Load += new EventHandler(frmMain_Load);
            this.Activated += new EventHandler(frmMain_Activated);
            this.Deactivate += new EventHandler(frmMain_Deactivated);
        }

        public List<DockContent> GetStartupPanes()
        {
            return new List<DockContent> 
            {
                projectPanel,
                propertiesPanel,
                pnlOutput,
                pnlFindResults,
                pnlCallStack
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
            LoadLayout();            			
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

        private void tabbedDocumentContainer1_ActiveDocumentChanging(ContentDocument newActiveDocument)
        {
            if (tabbedDocumentContainer1.ActiveDocument != null)
            {
                // Remember which pane and item were selected on the property grid,
                // so that we can restore them later
                tabbedDocumentContainer1.ActiveDocument.SelectedPropertyGridTab = 
                    propertiesPanel.propertiesGrid.SelectedTab.TabName;
                if (propertiesPanel.propertiesGrid.SelectedGridItem != null)
                {
                    tabbedDocumentContainer1.ActiveDocument.SelectedPropertyGridItem = 
                        propertiesPanel.propertiesGrid.SelectedGridItem.Label;
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
            GridItem startFromHere = propertiesPanel.propertiesGrid.SelectedGridItem;
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
                        propertiesPanel.propertiesGrid.ExpandAllGridItems();
                    }
                    propertiesPanel.propertiesGrid.SelectedGridItem = itemToSelect;
                }
            }
        }

        private void tabbedDocumentContainer1_ActiveDocumentChanged(ContentDocument newActiveDocument)
        {
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

        public IList<ContentDocument> Panes
        {
            get
            {
                return tabbedDocumentContainer1.Documents;
            }
        }

		/*public void SetProjectTreeLocation(bool rightHandSide)
		{
			SplitterPanel leftHandPanel = this.mainContainer.Panel1;
			SplitterPanel rightHandPanel = this.mainContainer.Panel2;
			if ((rightHandSide) && (rightHandPanel.Controls.Contains(this.leftSplitter)))
			{
				// already on the right
				return;
			}
			if ((!rightHandSide) && (leftHandPanel.Controls.Contains(this.leftSplitter)))
			{
				// already on the left
				return;
			}
			leftHandPanel.Controls.Clear();
			rightHandPanel.Controls.Clear();
			SplitterPanel panelWithProjectTree = leftHandPanel;
			SplitterPanel panelWithMainPane = rightHandPanel;
			this.mainContainer.FixedPanel = System.Windows.Forms.FixedPanel.Panel1;
			if (rightHandSide)
			{
				panelWithProjectTree = rightHandPanel;
				panelWithMainPane = leftHandPanel;
				this.mainContainer.FixedPanel = System.Windows.Forms.FixedPanel.Panel2;
			}

			panelWithMainPane.Controls.Add(this.tabbedDocumentContainer1);
			panelWithMainPane.Controls.Add(this.pnlOutput);
            panelWithMainPane.Controls.Add(this.pnlCallStack);
            panelWithMainPane.Controls.Add(this.pnlFindResults);
			panelWithProjectTree.Controls.Add(this.leftSplitter);
			this.mainContainer.SplitterDistance = this.mainContainer.ClientSize.Width - this.mainContainer.SplitterDistance;
		}*/

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
            GridItem rootItem = propertiesPanel.propertiesGrid.SelectedGridItem;
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
            propertiesPanel.propertiesGrid.SelectedObject = propertiesObject;
            SelectObjectInPropertyList(propertiesObject);
        }

        public void SetPropertyObjects(object[] propertiesObjects)
        {
            propertiesPanel.propertiesGrid.SelectedObjects = propertiesObjects;

            propertiesPanel.propertyObjectCombo.SelectedIndex = -1;
        }

        private void SelectObjectInPropertyList(object propertiesObject)
        {
            if (_propertyObjectList != null)
            {
                foreach (string name in _propertyObjectList.Keys)
                {
                    if (_propertyObjectList[name] == propertiesObject)
                    {
                        propertiesPanel.propertyObjectCombo.SelectedIndex =
                            propertiesPanel.propertyObjectCombo.Items.IndexOf(name);
                    }
                }
            }
        }

		/*public void SetSplitterPositions(int mainSplitterX, int propertiesSplitterY)
		{
			_splitterXtoSet = mainSplitterX;
			_splitterYtoSet = propertiesSplitterY;
		}

		public void GetSplitterPositions(out int mainSplitterX, out int propertiesSplitterY)
		{
            mainSplitterX = 0;// this.mainContainer.SplitterDistance;
            propertiesSplitterY = 0;// this.leftSplitter.SplitterDistance;
		}*/

        private void frmMain_FormClosing(object sender, FormClosingEventArgs e)
        {
            if (OnEditorShutdown != null)
            {
                e.Cancel = !OnEditorShutdown();
            }
            if (!e.Cancel)
            {
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
            
            if (AGS.Types.Version.IS_BETA_VERSION)
			{
				Factory.GUIController.ShowMessage("This is a BETA version of AGS. BE VERY CAREFUL and MAKE SURE YOU BACKUP YOUR GAME before loading it in this editor.", MessageBoxIcon.Warning);
			}

            Factory.GUIController.ShowWelcomeScreen();
        }

        private void LoadLayout()
        {
            if (!_layoutManager.LoadLayout())
            {
                SetDefaultLayout();
            }            
        }

        private void SetDefaultLayout()
        {
            this.pnlCallStack.Show(mainContainer, DockState.DockBottom);
            this.pnlFindResults.Show(pnlCallStack.Pane, pnlCallStack);
            this.pnlOutput.Show(pnlCallStack.Pane, pnlFindResults);            
            this.projectPanel.Show();
            this.propertiesPanel.Show();                        
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

        private void LoadColorTheme(ColorTheme t)
        {
            BackColor = t.GetColor("background");
            mainContainer.DockBackColor = t.GetColor("main-container/dock-background");
            mainContainer.BackColor = t.GetColor("main-container/background");
            mainContainer.ForeColor = t.GetColor("main-container/foreground");
            mainContainer.Skin.AutoHideStripSkin.TabGradient.StartColor = t.GetColor("main-container/skin/auto-hide/tab-gradient/start");
            mainContainer.Skin.AutoHideStripSkin.TabGradient.EndColor = t.GetColor("main-container/skin/auto-hide/tab-gradient/end");
            mainContainer.Skin.AutoHideStripSkin.TabGradient.TextColor = t.GetColor("main-container/skin/auto-hide/tab-gradient/text");
            mainContainer.Skin.AutoHideStripSkin.DockStripGradient.StartColor = t.GetColor("main-container/skin/auto-hide/dock-strip-gradient/start");
            mainContainer.Skin.AutoHideStripSkin.DockStripGradient.EndColor = t.GetColor("main-container/skin/auto-hide/dock-strip-gradient/end");
            mainContainer.Skin.DockPaneStripSkin.DocumentGradient.DockStripGradient.StartColor = t.GetColor("main-container/skin/dock-pane/document-gradient/dock-strip-gradient/start");
            mainContainer.Skin.DockPaneStripSkin.DocumentGradient.DockStripGradient.EndColor = t.GetColor("main-container/skin/dock-pane/document-gradient/dock-strip-gradient/end");
            mainContainer.Skin.DockPaneStripSkin.DocumentGradient.ActiveTabGradient.StartColor = t.GetColor("main-container/skin/dock-pane/document-gradient/active-tab-gradient/start");
            mainContainer.Skin.DockPaneStripSkin.DocumentGradient.ActiveTabGradient.EndColor = t.GetColor("main-container/skin/dock-pane/document-gradient/active-tab-gradient/end");
            mainContainer.Skin.DockPaneStripSkin.DocumentGradient.ActiveTabGradient.TextColor = t.GetColor("main-container/skin/dock-pane/document-gradient/active-tab-gradient/text");
            mainContainer.Skin.DockPaneStripSkin.DocumentGradient.InactiveTabGradient.StartColor = t.GetColor("main-container/skin/dock-pane/document-gradient/inactive-tab-gradient/start");
            mainContainer.Skin.DockPaneStripSkin.DocumentGradient.InactiveTabGradient.EndColor = t.GetColor("main-container/skin/dock-pane/document-gradient/inactive-tab-gradient/end");
            mainContainer.Skin.DockPaneStripSkin.DocumentGradient.InactiveTabGradient.TextColor = t.GetColor("main-container/skin/dock-pane/document-gradient/inactive-tab-gradient/text");
            mainContainer.Skin.DockPaneStripSkin.ToolWindowGradient.ActiveCaptionGradient.StartColor = t.GetColor("main-container/skin/dock-pane/tool-window/active-caption-gradient/start");
            mainContainer.Skin.DockPaneStripSkin.ToolWindowGradient.ActiveCaptionGradient.EndColor = t.GetColor("main-container/skin/dock-pane/tool-window/active-caption-gradient/end");
            mainContainer.Skin.DockPaneStripSkin.ToolWindowGradient.ActiveCaptionGradient.TextColor = t.GetColor("main-container/skin/dock-pane/tool-window/active-caption-gradient/text");
            mainContainer.Skin.DockPaneStripSkin.ToolWindowGradient.InactiveCaptionGradient.StartColor = t.GetColor("main-container/skin/dock-pane/tool-window/inactive-caption-gradient/start");
            mainContainer.Skin.DockPaneStripSkin.ToolWindowGradient.InactiveCaptionGradient.EndColor = t.GetColor("main-container/skin/dock-pane/tool-window/inactive-caption-gradient/end");
            mainContainer.Skin.DockPaneStripSkin.ToolWindowGradient.InactiveCaptionGradient.TextColor = t.GetColor("main-container/skin/dock-pane/tool-window/inactive-caption-gradient/text");
            mainContainer.Skin.DockPaneStripSkin.ToolWindowGradient.ActiveTabGradient.StartColor = t.GetColor("main-container/skin/dock-pane/tool-window/active-tab-gradient/start");
            mainContainer.Skin.DockPaneStripSkin.ToolWindowGradient.ActiveTabGradient.EndColor = t.GetColor("main-container/skin/dock-pane/tool-window/active-tab-gradient/end");
            mainContainer.Skin.DockPaneStripSkin.ToolWindowGradient.ActiveTabGradient.TextColor = t.GetColor("main-container/skin/dock-pane/tool-window/active-tab-gradient/text");
            mainContainer.Skin.DockPaneStripSkin.ToolWindowGradient.InactiveTabGradient.StartColor = t.GetColor("main-container/skin/dock-pane/tool-window/inactive-tab-gradient/start");
            mainContainer.Skin.DockPaneStripSkin.ToolWindowGradient.InactiveTabGradient.EndColor = t.GetColor("main-container/skin/dock-pane/tool-window/inactive-tab-gradient/end");
            mainContainer.Skin.DockPaneStripSkin.ToolWindowGradient.InactiveTabGradient.TextColor = t.GetColor("main-container/skin/dock-pane/tool-window/inactive-tab-gradient/text");
            mainContainer.Skin.DockPaneStripSkin.ToolWindowGradient.DockStripGradient.StartColor = t.GetColor("main-container/skin/dock-pane/tool-window/dock-strip-gradient/start");
            mainContainer.Skin.DockPaneStripSkin.ToolWindowGradient.DockStripGradient.EndColor = t.GetColor("main-container/skin/dock-pane/tool-window/dock-strip-gradient/end");

            mainMenu.BackColor = t.GetColor("main-menu/background");
            mainMenu.Renderer = t.GetMainMenuRenderer("main-menu");
            mainMenu.Renderer.RenderItemText += (s, a) => a.ToolStrip.ForeColor = t.GetColor("main-menu/foreground");
            mainMenu.Renderer.RenderItemCheck += (s, a) => a.Graphics.DrawImage(t.GetImage("main-menu/check/foreground", a.Image), a.ImageRectangle);
            toolStrip.BackColor = t.GetColor("tool-bar/background");
            toolStrip.Renderer = t.GetToolStripRenderer("tool-bar");
            statusStrip.BackColor = t.GetColor("status-strip/background");
        }
    }
}