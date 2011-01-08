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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(frmMain));
            this.mainContainer = new System.Windows.Forms.SplitContainer();
            this.tabbedDocumentContainer1 = new AGS.Editor.TabbedDocumentContainer();
            this.pnlCallStack = new AGS.Editor.CallStackPanel();
            this.pnlFindResults = new AGS.Editor.FindResultsPanel();
            this.pnlOutput = new AGS.Editor.OutputPanel();
            this.leftSplitter = new System.Windows.Forms.SplitContainer();
            this.projectTree = new System.Windows.Forms.TreeView();
            this.propertiesWindowPanel = new System.Windows.Forms.Panel();
            this.propertiesPanel = new System.Windows.Forms.PropertyGrid();
            this.propertyObjectCombo = new System.Windows.Forms.ComboBox();
            this.mainMenu = new System.Windows.Forms.MenuStrip();
            this.fileToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStrip = new System.Windows.Forms.ToolStrip();
            this.statusStrip = new System.Windows.Forms.StatusStrip();
            this.statusLabel = new System.Windows.Forms.ToolStripStatusLabel();
            this.mainContainer.Panel1.SuspendLayout();
            this.mainContainer.Panel2.SuspendLayout();
            this.mainContainer.SuspendLayout();
            this.leftSplitter.Panel1.SuspendLayout();
            this.leftSplitter.Panel2.SuspendLayout();
            this.leftSplitter.SuspendLayout();
            this.propertiesWindowPanel.SuspendLayout();
            this.mainMenu.SuspendLayout();
            this.statusStrip.SuspendLayout();
            this.SuspendLayout();
            // 
            // mainContainer
            // 
            this.mainContainer.Dock = System.Windows.Forms.DockStyle.Fill;
            this.mainContainer.FixedPanel = System.Windows.Forms.FixedPanel.Panel2;
            this.mainContainer.Location = new System.Drawing.Point(0, 49);
            this.mainContainer.Name = "mainContainer";
            // 
            // mainContainer.Panel1
            // 
            this.mainContainer.Panel1.BackColor = System.Drawing.SystemColors.Control;
            this.mainContainer.Panel1.Controls.Add(this.tabbedDocumentContainer1);
            this.mainContainer.Panel1.Controls.Add(this.pnlCallStack);
            this.mainContainer.Panel1.Controls.Add(this.pnlFindResults);
            this.mainContainer.Panel1.Controls.Add(this.pnlOutput);
            // 
            // mainContainer.Panel2
            // 
            this.mainContainer.Panel2.Controls.Add(this.leftSplitter);
            this.mainContainer.Size = new System.Drawing.Size(761, 443);
            this.mainContainer.SplitterDistance = 499;
            this.mainContainer.SplitterWidth = 6;
            this.mainContainer.TabIndex = 3;
            // 
            // tabbedDocumentContainer1
            // 
            this.tabbedDocumentContainer1.BackColor = System.Drawing.Color.Gray;
            this.tabbedDocumentContainer1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tabbedDocumentContainer1.Location = new System.Drawing.Point(0, 0);
            this.tabbedDocumentContainer1.Name = "tabbedDocumentContainer1";
            this.tabbedDocumentContainer1.Size = new System.Drawing.Size(499, 112);
            this.tabbedDocumentContainer1.TabIndex = 0;
            this.tabbedDocumentContainer1.Enter += new System.EventHandler(this.tabbedDocumentContainer1_Enter);
            // 
            // pnlCallStack
            // 
            this.pnlCallStack.CallStack = null;
            this.pnlCallStack.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.pnlCallStack.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.pnlCallStack.Location = new System.Drawing.Point(0, 112);
            this.pnlCallStack.Name = "pnlCallStack";
            this.pnlCallStack.Size = new System.Drawing.Size(499, 105);
            this.pnlCallStack.TabIndex = 2;
            this.pnlCallStack.Visible = false;
            // 
            // pnlFindResults
            // 
            this.pnlFindResults.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.pnlFindResults.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.pnlFindResults.Location = new System.Drawing.Point(0, 217);
            this.pnlFindResults.Name = "pnlFindResults";
            this.pnlFindResults.Results = null;
            this.pnlFindResults.Scintilla = null;
            this.pnlFindResults.Size = new System.Drawing.Size(499, 105);
            this.pnlFindResults.TabIndex = 2;
            this.pnlFindResults.Visible = false;
            // 
            // pnlOutput
            // 
            this.pnlOutput.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.pnlOutput.ErrorsToList = null;
            this.pnlOutput.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.pnlOutput.Location = new System.Drawing.Point(0, 322);
            this.pnlOutput.Name = "pnlOutput";
            this.pnlOutput.Size = new System.Drawing.Size(499, 121);
            this.pnlOutput.TabIndex = 1;
            this.pnlOutput.Visible = false;
            // 
            // leftSplitter
            // 
            this.leftSplitter.Dock = System.Windows.Forms.DockStyle.Fill;
            this.leftSplitter.Location = new System.Drawing.Point(0, 0);
            this.leftSplitter.Name = "leftSplitter";
            this.leftSplitter.Orientation = System.Windows.Forms.Orientation.Horizontal;
            // 
            // leftSplitter.Panel1
            // 
            this.leftSplitter.Panel1.Controls.Add(this.projectTree);
            this.leftSplitter.Panel1.RightToLeft = System.Windows.Forms.RightToLeft.No;
            // 
            // leftSplitter.Panel2
            // 
            this.leftSplitter.Panel2.Controls.Add(this.propertiesWindowPanel);
            this.leftSplitter.Panel2.RightToLeft = System.Windows.Forms.RightToLeft.No;
            this.leftSplitter.Size = new System.Drawing.Size(256, 443);
            this.leftSplitter.SplitterDistance = 196;
            this.leftSplitter.TabIndex = 3;
            // 
            // projectTree
            // 
            this.projectTree.AllowDrop = true;
            this.projectTree.Dock = System.Windows.Forms.DockStyle.Fill;
            this.projectTree.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.projectTree.HideSelection = false;
            this.projectTree.LabelEdit = true;
            this.projectTree.Location = new System.Drawing.Point(0, 0);
            this.projectTree.Name = "projectTree";
            this.projectTree.Size = new System.Drawing.Size(256, 196);
            this.projectTree.TabIndex = 2;
            this.projectTree.Enter += new System.EventHandler(this.projectTree_Enter);
            // 
            // propertiesWindowPanel
            // 
            this.propertiesWindowPanel.Controls.Add(this.propertiesPanel);
            this.propertiesWindowPanel.Controls.Add(this.propertyObjectCombo);
            this.propertiesWindowPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.propertiesWindowPanel.Location = new System.Drawing.Point(0, 0);
            this.propertiesWindowPanel.Name = "propertiesWindowPanel";
            this.propertiesWindowPanel.Size = new System.Drawing.Size(256, 243);
            this.propertiesWindowPanel.TabIndex = 0;
            // 
            // propertiesPanel
            // 
            this.propertiesPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.propertiesPanel.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.propertiesPanel.Location = new System.Drawing.Point(0, 21);
            this.propertiesPanel.Name = "propertiesPanel";
            this.propertiesPanel.Size = new System.Drawing.Size(256, 222);
            this.propertiesPanel.TabIndex = 10;
            this.propertiesPanel.PropertyValueChanged += new System.Windows.Forms.PropertyValueChangedEventHandler(this.propertiesPanel_PropertyValueChanged);
            // 
            // propertyObjectCombo
            // 
            this.propertyObjectCombo.Dock = System.Windows.Forms.DockStyle.Top;
            this.propertyObjectCombo.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.propertyObjectCombo.FormattingEnabled = true;
            this.propertyObjectCombo.Location = new System.Drawing.Point(0, 0);
            this.propertyObjectCombo.Name = "propertyObjectCombo";
            this.propertyObjectCombo.Size = new System.Drawing.Size(256, 21);
            this.propertyObjectCombo.TabIndex = 0;
            this.propertyObjectCombo.SelectedIndexChanged += new System.EventHandler(this.propertyObjectCombo_SelectedIndexChanged);
            // 
            // mainMenu
            // 
            this.mainMenu.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.fileToolStripMenuItem});
            this.mainMenu.Location = new System.Drawing.Point(0, 0);
            this.mainMenu.Name = "mainMenu";
            this.mainMenu.Size = new System.Drawing.Size(761, 24);
            this.mainMenu.TabIndex = 4;
            this.mainMenu.Text = "menuStrip1";
            // 
            // fileToolStripMenuItem
            // 
            this.fileToolStripMenuItem.Name = "fileToolStripMenuItem";
            this.fileToolStripMenuItem.Size = new System.Drawing.Size(37, 20);
            this.fileToolStripMenuItem.Text = "&File";
            // 
            // toolStrip
            // 
            this.toolStrip.Location = new System.Drawing.Point(0, 24);
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
            // 
            // frmMain
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(761, 514);
            this.Controls.Add(this.mainContainer);
            this.Controls.Add(this.toolStrip);
            this.Controls.Add(this.mainMenu);
            this.Controls.Add(this.statusStrip);
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.KeyPreview = true;
            this.Name = "frmMain";
            this.StartPosition = System.Windows.Forms.FormStartPosition.Manual;
            this.Text = "AGS Editor";
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.frmMain_FormClosing);
            this.Shown += new System.EventHandler(this.frmMain_Shown);
            this.KeyDown += new System.Windows.Forms.KeyEventHandler(this.frmMain_KeyDown);
            this.KeyUp += new System.Windows.Forms.KeyEventHandler(this.frmMain_KeyUp);
            this.mainContainer.Panel1.ResumeLayout(false);
            this.mainContainer.Panel2.ResumeLayout(false);
            this.mainContainer.ResumeLayout(false);
            this.leftSplitter.Panel1.ResumeLayout(false);
            this.leftSplitter.Panel2.ResumeLayout(false);
            this.leftSplitter.ResumeLayout(false);
            this.propertiesWindowPanel.ResumeLayout(false);
            this.mainMenu.ResumeLayout(false);
            this.mainMenu.PerformLayout();
            this.statusStrip.ResumeLayout(false);
            this.statusStrip.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.SplitContainer mainContainer;
        private System.Windows.Forms.SplitContainer leftSplitter;
        internal System.Windows.Forms.TreeView projectTree;
        internal System.Windows.Forms.MenuStrip mainMenu;
        private System.Windows.Forms.ToolStripMenuItem fileToolStripMenuItem;
        private System.Windows.Forms.Panel propertiesWindowPanel;
        private System.Windows.Forms.PropertyGrid propertiesPanel;
        private System.Windows.Forms.ComboBox propertyObjectCombo;
        private AGS.Editor.TabbedDocumentContainer tabbedDocumentContainer1;
        internal System.Windows.Forms.ToolStrip toolStrip;
        internal OutputPanel pnlOutput;
        private System.Windows.Forms.StatusStrip statusStrip;
        internal System.Windows.Forms.ToolStripStatusLabel statusLabel;
        internal CallStackPanel pnlCallStack;
        internal FindResultsPanel pnlFindResults;
    }
}

