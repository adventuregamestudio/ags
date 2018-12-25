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
            this.groupBoxExport = new System.Windows.Forms.GroupBox();
            this.chkSkipValidSpriteSource = new System.Windows.Forms.CheckBox();
            this.chkRecurse = new System.Windows.Forms.CheckBox();
            this.radRootFolder = new System.Windows.Forms.RadioButton();
            this.radThisFolder = new System.Windows.Forms.RadioButton();
            this.groupBoxSaveAs = new System.Windows.Forms.GroupBox();
            this.btnBrowse = new System.Windows.Forms.Button();
            this.txtFolder = new System.Windows.Forms.TextBox();
            this.txtFilename = new System.Windows.Forms.TextBox();
            this.lblFolder = new System.Windows.Forms.Label();
            this.lblFilename = new System.Windows.Forms.Label();
            this.btnClose = new System.Windows.Forms.Button();
            this.btnExport = new System.Windows.Forms.Button();
            this.chkUpdateSpriteSource = new System.Windows.Forms.CheckBox();
            this.groupBoxExport.SuspendLayout();
            this.groupBoxSaveAs.SuspendLayout();
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
            this.groupBoxSaveAs.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.groupBoxSaveAs.Controls.Add(this.chkUpdateSpriteSource);
            this.groupBoxSaveAs.Controls.Add(this.btnBrowse);
            this.groupBoxSaveAs.Controls.Add(this.txtFolder);
            this.groupBoxSaveAs.Controls.Add(this.txtFilename);
            this.groupBoxSaveAs.Controls.Add(this.lblFolder);
            this.groupBoxSaveAs.Controls.Add(this.lblFilename);
            this.groupBoxSaveAs.Location = new System.Drawing.Point(12, 134);
            this.groupBoxSaveAs.Name = "groupBoxSaveAs";
            this.groupBoxSaveAs.Size = new System.Drawing.Size(410, 136);
            this.groupBoxSaveAs.TabIndex = 1;
            this.groupBoxSaveAs.TabStop = false;
            this.groupBoxSaveAs.Text = "Save As";
            // 
            // btnBrowse
            // 
            this.btnBrowse.Location = new System.Drawing.Point(63, 74);
            this.btnBrowse.Name = "btnBrowse";
            this.btnBrowse.Size = new System.Drawing.Size(75, 23);
            this.btnBrowse.TabIndex = 4;
            this.btnBrowse.Text = "Browse";
            this.btnBrowse.UseVisualStyleBackColor = true;
            this.btnBrowse.Click += new System.EventHandler(this.btnBrowse_Click);
            // 
            // txtFolder
            // 
            this.txtFolder.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.txtFolder.Location = new System.Drawing.Point(63, 47);
            this.txtFolder.Name = "txtFolder";
            this.txtFolder.Size = new System.Drawing.Size(341, 20);
            this.txtFolder.TabIndex = 3;
            // 
            // txtFilename
            // 
            this.txtFilename.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
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
            this.lblFolder.Size = new System.Drawing.Size(36, 13);
            this.lblFolder.TabIndex = 1;
            this.lblFolder.Text = "Folder";
            // 
            // lblFilename
            // 
            this.lblFilename.AutoSize = true;
            this.lblFilename.Location = new System.Drawing.Point(7, 22);
            this.lblFilename.Name = "lblFilename";
            this.lblFilename.Size = new System.Drawing.Size(49, 13);
            this.lblFilename.TabIndex = 0;
            this.lblFilename.Text = "Filename";
            this.lblFilename.TextAlign = System.Drawing.ContentAlignment.TopCenter;
            // 
            // btnClose
            // 
            this.btnClose.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.btnClose.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnClose.Location = new System.Drawing.Point(12, 276);
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
            this.btnExport.Location = new System.Drawing.Point(93, 276);
            this.btnExport.Name = "btnExport";
            this.btnExport.Size = new System.Drawing.Size(75, 23);
            this.btnExport.TabIndex = 3;
            this.btnExport.Text = "Export";
            this.btnExport.UseVisualStyleBackColor = true;
            // 
            // chkUpdateSpriteSource
            // 
            this.chkUpdateSpriteSource.AutoSize = true;
            this.chkUpdateSpriteSource.Location = new System.Drawing.Point(65, 108);
            this.chkUpdateSpriteSource.Name = "chkUpdateSpriteSource";
            this.chkUpdateSpriteSource.Size = new System.Drawing.Size(179, 17);
            this.chkUpdateSpriteSource.TabIndex = 5;
            this.chkUpdateSpriteSource.Text = "Set exported file as sprite source";
            this.chkUpdateSpriteSource.UseVisualStyleBackColor = true;
            // 
            // SpriteExportDialog
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(434, 311);
            this.Controls.Add(this.btnExport);
            this.Controls.Add(this.btnClose);
            this.Controls.Add(this.groupBoxSaveAs);
            this.Controls.Add(this.groupBoxExport);
            this.Name = "SpriteExportDialog";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "Sprite export";
            this.groupBoxExport.ResumeLayout(false);
            this.groupBoxExport.PerformLayout();
            this.groupBoxSaveAs.ResumeLayout(false);
            this.groupBoxSaveAs.PerformLayout();
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
    }
}