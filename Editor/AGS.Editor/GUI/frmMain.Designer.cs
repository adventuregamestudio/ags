namespace AGS.Editor
{
    partial class frmMain
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            WeifenLuo.WinFormsUI.Docking.DockPanelSkin dockPanelSkin1 = new WeifenLuo.WinFormsUI.Docking.DockPanelSkin();
            WeifenLuo.WinFormsUI.Docking.AutoHideStripSkin autoHideStripSkin1 = new WeifenLuo.WinFormsUI.Docking.AutoHideStripSkin();
            WeifenLuo.WinFormsUI.Docking.DockPanelGradient dockPanelGradient1 = new WeifenLuo.WinFormsUI.Docking.DockPanelGradient();
            WeifenLuo.WinFormsUI.Docking.TabGradient tabGradient1 = new WeifenLuo.WinFormsUI.Docking.TabGradient();
            WeifenLuo.WinFormsUI.Docking.DockPaneStripSkin dockPaneStripSkin1 = new WeifenLuo.WinFormsUI.Docking.DockPaneStripSkin();
            WeifenLuo.WinFormsUI.Docking.DockPaneStripGradient dockPaneStripGradient1 = new WeifenLuo.WinFormsUI.Docking.DockPaneStripGradient();
            WeifenLuo.WinFormsUI.Docking.TabGradient tabGradient2 = new WeifenLuo.WinFormsUI.Docking.TabGradient();
            WeifenLuo.WinFormsUI.Docking.DockPanelGradient dockPanelGradient2 = new WeifenLuo.WinFormsUI.Docking.DockPanelGradient();
            WeifenLuo.WinFormsUI.Docking.TabGradient tabGradient3 = new WeifenLuo.WinFormsUI.Docking.TabGradient();
            WeifenLuo.WinFormsUI.Docking.DockPaneStripToolWindowGradient dockPaneStripToolWindowGradient1 = new WeifenLuo.WinFormsUI.Docking.DockPaneStripToolWindowGradient();
            WeifenLuo.WinFormsUI.Docking.TabGradient tabGradient4 = new WeifenLuo.WinFormsUI.Docking.TabGradient();
            WeifenLuo.WinFormsUI.Docking.TabGradient tabGradient5 = new WeifenLuo.WinFormsUI.Docking.TabGradient();
            WeifenLuo.WinFormsUI.Docking.DockPanelGradient dockPanelGradient3 = new WeifenLuo.WinFormsUI.Docking.DockPanelGradient();
            WeifenLuo.WinFormsUI.Docking.TabGradient tabGradient6 = new WeifenLuo.WinFormsUI.Docking.TabGradient();
            WeifenLuo.WinFormsUI.Docking.TabGradient tabGradient7 = new WeifenLuo.WinFormsUI.Docking.TabGradient();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(frmMain));
            this.mainContainer = new WeifenLuo.WinFormsUI.Docking.DockPanel();
            this.tabbedDocumentContainer1 = new AGS.Editor.TabbedDocumentManager(mainContainer);
            this.pnlCallStack = new AGS.Editor.CallStackPanel();
            this.pnlFindResults = new AGS.Editor.FindResultsPanel();
            this.pnlOutput = new AGS.Editor.OutputPanel();
            this.projectPanel = new AGS.Editor.ProjectPanel();
            this.propertiesPanel = new AGS.Editor.PropertiesPanel();
            this.mainMenu = new MenuStripExtended();
            this.fileToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.windowsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStrip = new ToolStripExtended();
            this.statusStrip = new System.Windows.Forms.StatusStrip();
            this.statusLabel = new System.Windows.Forms.ToolStripStatusLabel();
            this.mainMenu.SuspendLayout();
            this.statusStrip.SuspendLayout();
            this.SuspendLayout();
            // 
            // frmMain
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(8F, 17F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(761, 514);
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.KeyPreview = true;
            this.Name = "frmMain";
            this.StartPosition = System.Windows.Forms.FormStartPosition.Manual;
            this.Text = "AGS Editor";
            this.Shown += new System.EventHandler(this.frmMain_Shown);
            this.KeyUp += new System.Windows.Forms.KeyEventHandler(this.frmMain_KeyUp);
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.frmMain_FormClosing);
            this.KeyDown += new System.Windows.Forms.KeyEventHandler(this.frmMain_KeyDown);
            this.IsMdiContainer = true;
            this.Controls.Add(this.mainContainer);
            this.Controls.Add(this.toolStrip);
            this.Controls.Add(this.mainMenu);
            this.Controls.Add(this.statusStrip);            
            // 
            // mainContainer
            //             
            this.mainContainer.ActiveAutoHideContent = null;
            this.mainContainer.Dock = System.Windows.Forms.DockStyle.Fill;
            this.mainContainer.DockBackColor = System.Drawing.SystemColors.Control;            
            this.mainContainer.Location = new System.Drawing.Point(0, 51);
            this.mainContainer.Name = "mainContainer";
            this.mainContainer.DocumentStyle = WeifenLuo.WinFormsUI.Docking.DocumentStyle.DockingMdi;
            this.mainContainer.Size = new System.Drawing.Size(761, 441);            
            dockPanelGradient1.EndColor = System.Drawing.SystemColors.ControlLight;
            dockPanelGradient1.StartColor = System.Drawing.SystemColors.ControlLight;
            autoHideStripSkin1.DockStripGradient = dockPanelGradient1;
            tabGradient1.EndColor = System.Drawing.SystemColors.Control;
            tabGradient1.StartColor = System.Drawing.SystemColors.Control;
            tabGradient1.TextColor = System.Drawing.SystemColors.ControlDarkDark;
            autoHideStripSkin1.TabGradient = tabGradient1;
            autoHideStripSkin1.TextFont = new System.Drawing.Font("Tahoma", 8.400001F);
            dockPanelSkin1.AutoHideStripSkin = autoHideStripSkin1;
            tabGradient2.EndColor = System.Drawing.SystemColors.ControlLightLight;
            tabGradient2.StartColor = System.Drawing.SystemColors.ControlLightLight;
            tabGradient2.TextColor = System.Drawing.SystemColors.ControlText;
            dockPaneStripGradient1.ActiveTabGradient = tabGradient2;
            dockPanelGradient2.EndColor = System.Drawing.SystemColors.Control;
            dockPanelGradient2.StartColor = System.Drawing.SystemColors.Control;
            dockPaneStripGradient1.DockStripGradient = dockPanelGradient2;
            tabGradient3.EndColor = System.Drawing.SystemColors.ControlLight;
            tabGradient3.StartColor = System.Drawing.SystemColors.ControlLight;
            tabGradient3.TextColor = System.Drawing.SystemColors.ControlText;
            dockPaneStripGradient1.InactiveTabGradient = tabGradient3;
            dockPaneStripSkin1.DocumentGradient = dockPaneStripGradient1;
            dockPaneStripSkin1.TextFont = new System.Drawing.Font("Tahoma", 8.400001F);            
            tabGradient4.EndColor = System.Drawing.SystemColors.ActiveCaption;
            tabGradient4.LinearGradientMode = System.Drawing.Drawing2D.LinearGradientMode.Vertical;
            tabGradient4.StartColor = System.Drawing.SystemColors.GradientActiveCaption;
            tabGradient4.TextColor = System.Drawing.SystemColors.ActiveCaptionText;
            dockPaneStripToolWindowGradient1.ActiveCaptionGradient = tabGradient4;
            tabGradient5.EndColor = System.Drawing.SystemColors.Control;
            tabGradient5.StartColor = System.Drawing.SystemColors.Control;
            tabGradient5.TextColor = System.Drawing.SystemColors.ControlText;
            dockPaneStripToolWindowGradient1.ActiveTabGradient = tabGradient5;
            dockPanelGradient3.EndColor = System.Drawing.SystemColors.ControlLight;
            dockPanelGradient3.StartColor = System.Drawing.SystemColors.ControlLight;
            dockPaneStripToolWindowGradient1.DockStripGradient = dockPanelGradient3;
            tabGradient6.EndColor = System.Drawing.SystemColors.InactiveCaption;
            tabGradient6.LinearGradientMode = System.Drawing.Drawing2D.LinearGradientMode.Vertical;
            tabGradient6.StartColor = System.Drawing.SystemColors.GradientInactiveCaption;
            tabGradient6.TextColor = System.Drawing.SystemColors.InactiveCaptionText;
            dockPaneStripToolWindowGradient1.InactiveCaptionGradient = tabGradient6;
            tabGradient7.EndColor = System.Drawing.Color.Transparent;
            tabGradient7.StartColor = System.Drawing.Color.Transparent;
            tabGradient7.TextColor = System.Drawing.SystemColors.ControlDarkDark;
            dockPaneStripToolWindowGradient1.InactiveTabGradient = tabGradient7;
            dockPaneStripSkin1.ToolWindowGradient = dockPaneStripToolWindowGradient1;
            dockPanelSkin1.DockPaneStripSkin = dockPaneStripSkin1;            
            this.mainContainer.Skin = dockPanelSkin1;
            this.mainContainer.TabIndex = 0;             
            // 
            // pnlCallStack
            // 
            this.pnlCallStack.CallStack = null;
            this.pnlCallStack.ClientSize = new System.Drawing.Size(489, 65);
            this.pnlCallStack.Dock = System.Windows.Forms.DockStyle.Fill;
            this.pnlCallStack.DockPanel = this.mainContainer;
            this.pnlCallStack.DockState = WeifenLuo.WinFormsUI.Docking.DockState.Unknown;
            this.pnlCallStack.FloatPane = null;
            this.pnlCallStack.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.pnlCallStack.IsFloat = false;
            this.pnlCallStack.IsHidden = true;
            this.pnlCallStack.HideOnClose = true;
            this.pnlCallStack.Location = new System.Drawing.Point(0, 0);
            this.pnlCallStack.Name = "pnlCallStack";
            this.pnlCallStack.Text = "Call Stack";
            this.pnlCallStack.Pane = null;
            this.pnlCallStack.PanelPane = null;
            this.pnlCallStack.ShowHint = WeifenLuo.WinFormsUI.Docking.DockState.DockBottom;
            this.pnlCallStack.Visible = false;
            this.pnlCallStack.VisibleState = WeifenLuo.WinFormsUI.Docking.DockState.DockBottom;
            // 
            // pnlFindResults
            // 
            this.pnlFindResults.ClientSize = new System.Drawing.Size(489, 65);
            this.pnlFindResults.Dock = System.Windows.Forms.DockStyle.Fill;
            this.pnlFindResults.DockPanel = this.mainContainer;
            this.pnlFindResults.DockState = WeifenLuo.WinFormsUI.Docking.DockState.Unknown;
            this.pnlFindResults.FloatPane = null;
            this.pnlFindResults.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.pnlFindResults.IsFloat = false;
            this.pnlFindResults.IsHidden = true;
            this.pnlFindResults.Location = new System.Drawing.Point(0, 0);
            this.pnlFindResults.Name = "pnlFindResults";
            this.pnlFindResults.Text = "Find Results";
            this.pnlFindResults.HideOnClose = true;
            this.pnlFindResults.Pane = null;
            this.pnlFindResults.PanelPane = null;
            this.pnlFindResults.Results = null;
            this.pnlFindResults.ShowHint = WeifenLuo.WinFormsUI.Docking.DockState.DockBottom;
            this.pnlFindResults.Visible = false;
            this.pnlFindResults.VisibleState = WeifenLuo.WinFormsUI.Docking.DockState.DockBottom;
            // 
            // pnlOutput
            // 
            this.pnlOutput.ClientSize = new System.Drawing.Size(489, 65);
            this.pnlOutput.Dock = System.Windows.Forms.DockStyle.Fill;
            this.pnlOutput.DockPanel = this.mainContainer;
            this.pnlOutput.DockState = WeifenLuo.WinFormsUI.Docking.DockState.Unknown;
            this.pnlOutput.ErrorsToList = null;
            this.pnlOutput.FloatPane = null;
            this.pnlOutput.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.pnlOutput.IsFloat = false;
            this.pnlOutput.IsHidden = true;
            this.pnlOutput.HideOnClose = true;
            this.pnlOutput.Location = new System.Drawing.Point(0, 0);
            this.pnlOutput.Name = "pnlOutput";
            this.pnlOutput.Text = "Output";
            this.pnlOutput.Pane = null;
            this.pnlOutput.PanelPane = null;
            this.pnlOutput.ShowHint = WeifenLuo.WinFormsUI.Docking.DockState.DockBottom;
            this.pnlOutput.Visible = true;
            this.pnlOutput.VisibleState = WeifenLuo.WinFormsUI.Docking.DockState.DockBottom;
            // 
            // projectPanel
            // 
            this.projectPanel.DockPanel = this.mainContainer;
            this.projectPanel.AllowDrop = true;
            this.projectPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.projectPanel.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.projectPanel.Location = new System.Drawing.Point(0, 0);
            this.projectPanel.Name = "projectPanel";
            this.projectPanel.Text = "Explore Project";
            this.projectPanel.Size = new System.Drawing.Size(258, 195);
            this.projectPanel.TabIndex = 2;
            this.projectPanel.HideOnClose = true;
            this.projectPanel.Enter += new System.EventHandler(this.projectTree_Enter);
            this.projectPanel.Pane = null;
            this.projectPanel.PanelPane = null;
            this.projectPanel.ShowHint = WeifenLuo.WinFormsUI.Docking.DockState.DockRight;
            this.projectPanel.Visible = true;
            this.projectPanel.VisibleState = WeifenLuo.WinFormsUI.Docking.DockState.DockRight;
            // 
            // propertiesPanel
            // 
            this.propertiesPanel.DockPanel = this.mainContainer;
            this.propertiesPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.propertiesPanel.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.propertiesPanel.Location = new System.Drawing.Point(0, 0);
            this.propertiesPanel.Name = "propertiesPanel";
            this.propertiesPanel.Size = new System.Drawing.Size(258, 242);
            this.propertiesPanel.TabIndex = 10;
            this.propertiesPanel.PropertyValueChanged += new System.Windows.Forms.PropertyValueChangedEventHandler(this.propertiesPanel_PropertyValueChanged);
            this.propertiesPanel.SelectedIndexChanged += new System.EventHandler(this.propertyObjectCombo_SelectedIndexChanged);
            this.propertiesPanel.Text = "Properties";
            this.propertiesPanel.HideOnClose = true;
            this.propertiesPanel.Pane = null;
            this.propertiesPanel.PanelPane = null;
            this.propertiesPanel.ShowHint = WeifenLuo.WinFormsUI.Docking.DockState.DockRight;
            this.propertiesPanel.Visible = true;
            this.propertiesPanel.VisibleState = WeifenLuo.WinFormsUI.Docking.DockState.DockRight;
            // 
            // mainMenu
            // 
            this.mainMenu.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.fileToolStripMenuItem,
            this.windowsToolStripMenuItem});
            this.mainMenu.Location = new System.Drawing.Point(0, 0);
            this.mainMenu.Name = "mainMenu";
            this.mainMenu.Size = new System.Drawing.Size(761, 26);
            this.mainMenu.TabIndex = 4;
            this.mainMenu.Text = "menuStrip1";
            //this.mainMenu.MdiWindowListItem = this.windowsToolStripMenuItem;
            // 
            // fileToolStripMenuItem
            // 
            this.fileToolStripMenuItem.Name = "fileToolStripMenuItem";
            this.fileToolStripMenuItem.Size = new System.Drawing.Size(40, 22);
            this.fileToolStripMenuItem.Text = "&File";
            //
            // windowsToolStripMenuItem
            //
            this.windowsToolStripMenuItem.Name = "windowsToolStripMenuItem";
            this.windowsToolStripMenuItem.Size = new System.Drawing.Size(40, 22);
            this.windowsToolStripMenuItem.Text = "&Window";
            //this.windowsToolStripMenuItem.DropDownItems.Add("TEST");
            // 
            // toolStrip
            // 
            this.toolStrip.Location = new System.Drawing.Point(0, 26);
            this.toolStrip.Name = "toolStrip";
            this.toolStrip.Size = new System.Drawing.Size(761, 25);
            this.toolStrip.TabIndex = 5;
            this.toolStrip.Text = "toolStrip1";
            // 
            // statusStrip
            // 
            this.statusStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.statusLabel});
            this.statusStrip.Location = new System.Drawing.Point(0, 492);
            this.statusStrip.Name = "statusStrip";
            this.statusStrip.Size = new System.Drawing.Size(761, 22);
            this.statusStrip.TabIndex = 6;
            this.statusStrip.Text = "statusStrip1";
            // 
            // statusLabel
            // 
            this.statusLabel.Name = "statusLabel";
            this.statusLabel.Size = new System.Drawing.Size(0, 17);
            this.mainMenu.ResumeLayout(false);
            this.mainMenu.PerformLayout();
            this.statusStrip.ResumeLayout(false);
            this.statusStrip.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        internal WeifenLuo.WinFormsUI.Docking.DockPanel mainContainer;
        internal AGS.Editor.ProjectPanel projectPanel;
        internal MenuStripExtended mainMenu;
        private System.Windows.Forms.ToolStripMenuItem fileToolStripMenuItem;
        internal System.Windows.Forms.ToolStripMenuItem windowsToolStripMenuItem;
        private AGS.Editor.PropertiesPanel propertiesPanel;
        private AGS.Editor.TabbedDocumentManager tabbedDocumentContainer1;
        internal ToolStripExtended toolStrip;
        internal OutputPanel pnlOutput;
        private System.Windows.Forms.StatusStrip statusStrip;
        internal System.Windows.Forms.ToolStripStatusLabel statusLabel;
        internal CallStackPanel pnlCallStack;
        internal FindResultsPanel pnlFindResults;
    }
}

