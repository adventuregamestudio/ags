namespace AGS.Editor
{
    partial class SpriteExportDialog
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
            this.components = new System.ComponentModel.Container();
            this.groupBoxExport = new System.Windows.Forms.GroupBox();
            this.chkSkipValidSpriteSource = new System.Windows.Forms.CheckBox();
            this.chkRecurse = new System.Windows.Forms.CheckBox();
            this.radRootFolder = new System.Windows.Forms.RadioButton();
            this.radThisFolder = new System.Windows.Forms.RadioButton();
            this.groupBoxSaveAs = new System.Windows.Forms.GroupBox();
            this.btnBrowse = new System.Windows.Forms.Button();
            this.txtFolder = new System.Windows.Forms.TextBox();
            this.contextMenuStripExport = new System.Windows.Forms.ContextMenuStrip(this.components);
            this.menuItemCopy = new System.Windows.Forms.ToolStripMenuItem();
            this.menuItemCut = new System.Windows.Forms.ToolStripMenuItem();
            this.menuItemPaste = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
            this.menuItemToken = new System.Windows.Forms.ToolStripMenuItem();
            this.txtFilename = new System.Windows.Forms.TextBox();
            this.lblFolder = new System.Windows.Forms.Label();
            this.lblFilename = new System.Windows.Forms.Label();
            this.chkUpdateSpriteSource = new System.Windows.Forms.CheckBox();
            this.btnClose = new System.Windows.Forms.Button();
            this.btnExport = new System.Windows.Forms.Button();
            this.groupBoxSpriteProperties = new System.Windows.Forms.GroupBox();
            this.groupBoxExport.SuspendLayout();
            this.groupBoxSaveAs.SuspendLayout();
            this.contextMenuStripExport.SuspendLayout();
            this.groupBoxSpriteProperties.SuspendLayout();
            this.SuspendLayout();
            // 
            // groupBoxExport
            // 
            this.groupBoxExport.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.groupBoxExport.Controls.Add(this.chkSkipValidSpriteSource);
            this.groupBoxExport.Controls.Add(this.chkRecurse);
            this.groupBoxExport.Controls.Add(this.radRootFolder);
            this.groupBoxExport.Controls.Add(this.radThisFolder);
            this.groupBoxExport.Location = new System.Drawing.Point(12, 12);
            this.groupBoxExport.Name = "groupBoxExport";
            this.groupBoxExport.Size = new System.Drawing.Size(410, 116);
            this.groupBoxExport.TabIndex = 0;
            this.groupBoxExport.TabStop = false;
            this.groupBoxExport.Text = "Export from";
            // 
            // chkSkipValidSpriteSource
            // 
            this.chkSkipValidSpriteSource.AutoSize = true;
            this.chkSkipValidSpriteSource.Location = new System.Drawing.Point(7, 92);
            this.chkSkipValidSpriteSource.Name = "chkSkipValidSpriteSource";
            this.chkSkipValidSpriteSource.Size = new System.Drawing.Size(192, 17);
            this.chkSkipValidSpriteSource.TabIndex = 3;
            this.chkSkipValidSpriteSource.Text = "Skip sprites where source file exists";
            this.chkSkipValidSpriteSource.UseVisualStyleBackColor = true;
            // 
            // chkRecurse
            // 
            this.chkRecurse.AutoSize = true;
            this.chkRecurse.Location = new System.Drawing.Point(7, 68);
            this.chkRecurse.Name = "chkRecurse";
            this.chkRecurse.Size = new System.Drawing.Size(115, 17);
            this.chkRecurse.TabIndex = 2;
            this.chkRecurse.Text = "Include sub-folders";
            this.chkRecurse.UseVisualStyleBackColor = true;
            // 
            // radRootFolder
            // 
            this.radRootFolder.AutoSize = true;
            this.radRootFolder.Location = new System.Drawing.Point(7, 44);
            this.radRootFolder.Name = "radRootFolder";
            this.radRootFolder.Size = new System.Drawing.Size(105, 17);
            this.radRootFolder.TabIndex = 1;
            this.radRootFolder.Text = "Root sprite folder";
            this.radRootFolder.UseVisualStyleBackColor = true;
            // 
            // radThisFolder
            // 
            this.radThisFolder.AutoSize = true;
            this.radThisFolder.Checked = true;
            this.radThisFolder.Location = new System.Drawing.Point(7, 20);
            this.radThisFolder.Name = "radThisFolder";
            this.radThisFolder.Size = new System.Drawing.Size(102, 17);
            this.radThisFolder.TabIndex = 0;
            this.radThisFolder.TabStop = true;
            this.radThisFolder.Text = "This sprite folder";
            this.radThisFolder.UseVisualStyleBackColor = true;
            // 
            // groupBoxSaveAs
            // 
            this.groupBoxSaveAs.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.groupBoxSaveAs.Controls.Add(this.btnBrowse);
            this.groupBoxSaveAs.Controls.Add(this.txtFolder);
            this.groupBoxSaveAs.Controls.Add(this.txtFilename);
            this.groupBoxSaveAs.Controls.Add(this.lblFolder);
            this.groupBoxSaveAs.Controls.Add(this.lblFilename);
            this.groupBoxSaveAs.Location = new System.Drawing.Point(12, 134);
            this.groupBoxSaveAs.Name = "groupBoxSaveAs";
            this.groupBoxSaveAs.Size = new System.Drawing.Size(410, 107);
            this.groupBoxSaveAs.TabIndex = 1;
            this.groupBoxSaveAs.TabStop = false;
            this.groupBoxSaveAs.Text = "Save as";
            // 
            // btnBrowse
            // 
            this.btnBrowse.Location = new System.Drawing.Point(63, 74);
            this.btnBrowse.Name = "btnBrowse";
            this.btnBrowse.Size = new System.Drawing.Size(46, 21);
            this.btnBrowse.TabIndex = 4;
            this.btnBrowse.Text = "...";
            this.btnBrowse.UseVisualStyleBackColor = true;
            this.btnBrowse.Click += new System.EventHandler(this.btnBrowse_Click);
            // 
            // txtFolder
            // 
            this.txtFolder.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.txtFolder.ContextMenuStrip = this.contextMenuStripExport;
            this.txtFolder.Location = new System.Drawing.Point(63, 47);
            this.txtFolder.Name = "txtFolder";
            this.txtFolder.Size = new System.Drawing.Size(341, 20);
            this.txtFolder.TabIndex = 3;
            // 
            // contextMenuStripExport
            // 
            this.contextMenuStripExport.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.menuItemCopy,
            this.menuItemCut,
            this.menuItemPaste,
            this.toolStripSeparator1,
            this.menuItemToken});
            this.contextMenuStripExport.Name = "contextMenuStripExport";
            this.contextMenuStripExport.Size = new System.Drawing.Size(137, 98);
            this.contextMenuStripExport.Opening += new System.ComponentModel.CancelEventHandler(this.contextMenuStripExport_Opening);
            // 
            // menuItemCopy
            // 
            this.menuItemCopy.Name = "menuItemCopy";
            this.menuItemCopy.Size = new System.Drawing.Size(136, 22);
            this.menuItemCopy.Text = "Copy";
            this.menuItemCopy.Click += new System.EventHandler(this.menuItemCopy_Click);
            // 
            // menuItemCut
            // 
            this.menuItemCut.Name = "menuItemCut";
            this.menuItemCut.Size = new System.Drawing.Size(136, 22);
            this.menuItemCut.Text = "Cut";
            this.menuItemCut.Click += new System.EventHandler(this.menuItemCut_Click);
            // 
            // menuItemPaste
            // 
            this.menuItemPaste.Name = "menuItemPaste";
            this.menuItemPaste.Size = new System.Drawing.Size(136, 22);
            this.menuItemPaste.Text = "Paste";
            this.menuItemPaste.Click += new System.EventHandler(this.menuItemPaste_Click);
            // 
            // toolStripSeparator1
            // 
            this.toolStripSeparator1.Name = "toolStripSeparator1";
            this.toolStripSeparator1.Size = new System.Drawing.Size(133, 6);
            // 
            // menuItemToken
            // 
            this.menuItemToken.Name = "menuItemToken";
            this.menuItemToken.Size = new System.Drawing.Size(136, 22);
            this.menuItemToken.Text = "Insert token";
            this.menuItemToken.DropDownItemClicked += new System.Windows.Forms.ToolStripItemClickedEventHandler(this.menuItemToken_DropDownItemClicked);
            // 
            // txtFilename
            // 
            this.txtFilename.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.txtFilename.ContextMenuStrip = this.contextMenuStripExport;
            this.txtFilename.Location = new System.Drawing.Point(63, 20);
            this.txtFilename.Name = "txtFilename";
            this.txtFilename.Size = new System.Drawing.Size(341, 20);
            this.txtFilename.TabIndex = 2;
            this.txtFilename.Text = "%Number%";
            // 
            // lblFolder
            // 
            this.lblFolder.AutoSize = true;
            this.lblFolder.Location = new System.Drawing.Point(8, 49);
            this.lblFolder.Name = "lblFolder";
            this.lblFolder.Size = new System.Drawing.Size(52, 13);
            this.lblFolder.TabIndex = 1;
            this.lblFolder.Text = "Directory:";
            // 
            // lblFilename
            // 
            this.lblFilename.AutoSize = true;
            this.lblFilename.Location = new System.Drawing.Point(7, 22);
            this.lblFilename.Name = "lblFilename";
            this.lblFilename.Size = new System.Drawing.Size(52, 13);
            this.lblFilename.TabIndex = 0;
            this.lblFilename.Text = "Filename:";
            this.lblFilename.TextAlign = System.Drawing.ContentAlignment.TopCenter;
            // 
            // chkUpdateSpriteSource
            // 
            this.chkUpdateSpriteSource.AutoSize = true;
            this.chkUpdateSpriteSource.Location = new System.Drawing.Point(7, 19);
            this.chkUpdateSpriteSource.Name = "chkUpdateSpriteSource";
            this.chkUpdateSpriteSource.Size = new System.Drawing.Size(179, 17);
            this.chkUpdateSpriteSource.TabIndex = 5;
            this.chkUpdateSpriteSource.Text = "Set exported file as sprite source";
            this.chkUpdateSpriteSource.UseVisualStyleBackColor = true;
            // 
            // btnClose
            // 
            this.btnClose.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.btnClose.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnClose.Location = new System.Drawing.Point(93, 304);
            this.btnClose.Name = "btnClose";
            this.btnClose.Size = new System.Drawing.Size(75, 23);
            this.btnClose.TabIndex = 2;
            this.btnClose.Text = "Close";
            this.btnClose.UseVisualStyleBackColor = true;
            // 
            // btnExport
            // 
            this.btnExport.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.btnExport.DialogResult = System.Windows.Forms.DialogResult.OK;
            this.btnExport.Location = new System.Drawing.Point(12, 304);
            this.btnExport.Name = "btnExport";
            this.btnExport.Size = new System.Drawing.Size(75, 23);
            this.btnExport.TabIndex = 3;
            this.btnExport.Text = "Export";
            this.btnExport.UseVisualStyleBackColor = true;
            // 
            // groupBoxSpriteProperties
            // 
            this.groupBoxSpriteProperties.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.groupBoxSpriteProperties.Controls.Add(this.chkUpdateSpriteSource);
            this.groupBoxSpriteProperties.Location = new System.Drawing.Point(12, 248);
            this.groupBoxSpriteProperties.Name = "groupBoxSpriteProperties";
            this.groupBoxSpriteProperties.Size = new System.Drawing.Size(409, 44);
            this.groupBoxSpriteProperties.TabIndex = 6;
            this.groupBoxSpriteProperties.TabStop = false;
            this.groupBoxSpriteProperties.Text = "Sprite properties";
            // 
            // SpriteExportDialog
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(434, 339);
            this.Controls.Add(this.btnExport);
            this.Controls.Add(this.btnClose);
            this.Controls.Add(this.groupBoxSaveAs);
            this.Controls.Add(this.groupBoxExport);
            this.Controls.Add(this.groupBoxSpriteProperties);
            this.MaximizeBox = false;
            this.MaximumSize = new System.Drawing.Size(4096, 378);
            this.MinimizeBox = false;
            this.MinimumSize = new System.Drawing.Size(450, 378);
            this.Name = "SpriteExportDialog";
            this.ShowInTaskbar = false;
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "Sprite export";
            this.groupBoxExport.ResumeLayout(false);
            this.groupBoxExport.PerformLayout();
            this.groupBoxSaveAs.ResumeLayout(false);
            this.groupBoxSaveAs.PerformLayout();
            this.contextMenuStripExport.ResumeLayout(false);
            this.groupBoxSpriteProperties.ResumeLayout(false);
            this.groupBoxSpriteProperties.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.GroupBox groupBoxExport;
        private System.Windows.Forms.CheckBox chkRecurse;
        private System.Windows.Forms.RadioButton radRootFolder;
        private System.Windows.Forms.RadioButton radThisFolder;
        private System.Windows.Forms.GroupBox groupBoxSaveAs;
        private System.Windows.Forms.TextBox txtFilename;
        private System.Windows.Forms.Label lblFolder;
        private System.Windows.Forms.Label lblFilename;
        private System.Windows.Forms.Button btnBrowse;
        private System.Windows.Forms.TextBox txtFolder;
        private System.Windows.Forms.Button btnClose;
        private System.Windows.Forms.Button btnExport;
        private System.Windows.Forms.CheckBox chkSkipValidSpriteSource;
        private System.Windows.Forms.CheckBox chkUpdateSpriteSource;
        private System.Windows.Forms.ContextMenuStrip contextMenuStripExport;
        private System.Windows.Forms.ToolStripMenuItem menuItemCopy;
        private System.Windows.Forms.ToolStripMenuItem menuItemPaste;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
        private System.Windows.Forms.ToolStripMenuItem menuItemToken;
        private System.Windows.Forms.ToolStripMenuItem menuItemCut;
        private System.Windows.Forms.GroupBox groupBoxSpriteProperties;
    }
}