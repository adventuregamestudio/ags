namespace AGS.Editor
{
    partial class ViewLoopEditor
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

        #region Component Designer generated code

        /// <summary> 
        /// Required method for Designer support - do not modify 
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            this.lblLoopTitle = new System.Windows.Forms.Label();
            this.chkRunNextLoop = new System.Windows.Forms.CheckBox();
            this.btnNewFrame = new System.Windows.Forms.Button();
            this.loopContextMenu = new System.Windows.Forms.ContextMenuStrip(this.components);
            this.copyToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.cutToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.pasteToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.flipAllToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.createFromFolderToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.pasteFlippedToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.loopContextMenu.SuspendLayout();
            this.SuspendLayout();
            // 
            // lblLoopTitle
            // 
            this.lblLoopTitle.AutoSize = true;
            this.lblLoopTitle.Location = new System.Drawing.Point(0, 0);
            this.lblLoopTitle.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this.lblLoopTitle.Name = "lblLoopTitle";
            this.lblLoopTitle.Size = new System.Drawing.Size(46, 17);
            this.lblLoopTitle.TabIndex = 0;
            this.lblLoopTitle.Text = "label1";
            // 
            // chkRunNextLoop
            // 
            this.chkRunNextLoop.AutoSize = true;
            this.chkRunNextLoop.Location = new System.Drawing.Point(4, 22);
            this.chkRunNextLoop.Margin = new System.Windows.Forms.Padding(4);
            this.chkRunNextLoop.Name = "chkRunNextLoop";
            this.chkRunNextLoop.Size = new System.Drawing.Size(362, 21);
            this.chkRunNextLoop.TabIndex = 1;
            this.chkRunNextLoop.Text = "Run the next loop after this to make a long animation";
            this.chkRunNextLoop.UseVisualStyleBackColor = true;
            // 
            // btnNewFrame
            // 
            this.btnNewFrame.Location = new System.Drawing.Point(46, 49);
            this.btnNewFrame.Margin = new System.Windows.Forms.Padding(4);
            this.btnNewFrame.Name = "btnNewFrame";
            this.btnNewFrame.Size = new System.Drawing.Size(78, 69);
            this.btnNewFrame.TabIndex = 2;
            this.btnNewFrame.Text = "Create New Frame";
            this.btnNewFrame.UseVisualStyleBackColor = true;
            this.btnNewFrame.Click += new System.EventHandler(this.btnNewFrame_Click);
            // 
            // loopContextMenu
            // 
            this.loopContextMenu.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.copyToolStripMenuItem,
            this.cutToolStripMenuItem,
            this.pasteToolStripMenuItem,
            this.pasteFlippedToolStripMenuItem,
            this.flipAllToolStripMenuItem,
            this.createFromFolderToolStripMenuItem});
            this.loopContextMenu.Name = "loopContextMenu";
            this.loopContextMenu.Size = new System.Drawing.Size(245, 136);
            this.loopContextMenu.Opening += new System.ComponentModel.CancelEventHandler(this.onContextMenuForLoopOpening);
            // 
            // copyToolStripMenuItem
            // 
            this.copyToolStripMenuItem.Name = "copyToolStripMenuItem";
            this.copyToolStripMenuItem.Size = new System.Drawing.Size(238, 22);
            this.copyToolStripMenuItem.Text = "Copy Loop";
            this.copyToolStripMenuItem.Click += new System.EventHandler(this.onCopyLoopClicked);
            // 
            // cutToolStripMenuItem
            // 
            this.cutToolStripMenuItem.Name = "cutToolStripMenuItem";
            this.cutToolStripMenuItem.Size = new System.Drawing.Size(238, 22);
            this.cutToolStripMenuItem.Text = "Cut Loop";
            this.cutToolStripMenuItem.Click += new System.EventHandler(this.onCutLoopClicked);
            // 
            // pasteToolStripMenuItem
            // 
            this.pasteToolStripMenuItem.Enabled = false;
            this.pasteToolStripMenuItem.Name = "pasteToolStripMenuItem";
            this.pasteToolStripMenuItem.Size = new System.Drawing.Size(238, 22);
            this.pasteToolStripMenuItem.Text = "Paste Loop";
            this.pasteToolStripMenuItem.Click += new System.EventHandler(this.onPasteLoopClicked);
            // 
            // flipAllToolStripMenuItem
            // 
            this.flipAllToolStripMenuItem.Name = "flipAllToolStripMenuItem";
            this.flipAllToolStripMenuItem.Size = new System.Drawing.Size(238, 22);
            this.flipAllToolStripMenuItem.Text = "Flip All Frames";
            this.flipAllToolStripMenuItem.Click += new System.EventHandler(this.onFlipAllClicked);
            // 
            // createFromFolderToolStripMenuItem
            // 
            this.createFromFolderToolStripMenuItem.Name = "createFromFolderToolStripMenuItem";
            this.createFromFolderToolStripMenuItem.Size = new System.Drawing.Size(244, 22);
            this.createFromFolderToolStripMenuItem.Text = "Quick Import From Folder";
            this.createFromFolderToolStripMenuItem.Click += new System.EventHandler(this.onQuickImportFromFolderClicked);
            // 
            // pasteFlippedToolStripMenuItem
            // 
            this.pasteFlippedToolStripMenuItem.Enabled = false;
            this.pasteFlippedToolStripMenuItem.Name = "pasteFlippedToolStripMenuItem";
            this.pasteFlippedToolStripMenuItem.Size = new System.Drawing.Size(238, 22);
            this.pasteFlippedToolStripMenuItem.Text = "Paste Loop Flipped";
            this.pasteFlippedToolStripMenuItem.Click += new System.EventHandler(this.onPasteFlippedClicked);
            // 
            // ViewLoopEditor
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(120F, 120F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
            this.ContextMenuStrip = this.loopContextMenu;
            this.Controls.Add(this.btnNewFrame);
            this.Controls.Add(this.chkRunNextLoop);
            this.Controls.Add(this.lblLoopTitle);
            this.Margin = new System.Windows.Forms.Padding(4);
            this.Name = "ViewLoopEditor";
            this.Size = new System.Drawing.Size(910, 145);
            this.Paint += new System.Windows.Forms.PaintEventHandler(this.ViewLoopEditor_Paint);
            this.MouseDoubleClick += new System.Windows.Forms.MouseEventHandler(this.ViewLoopEditor_MouseDoubleClick);
            this.MouseUp += new System.Windows.Forms.MouseEventHandler(this.ViewLoopEditor_MouseUp);
            this.loopContextMenu.ResumeLayout(false);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label lblLoopTitle;
        private System.Windows.Forms.CheckBox chkRunNextLoop;
        private System.Windows.Forms.Button btnNewFrame;
        private System.Windows.Forms.ContextMenuStrip loopContextMenu;
        private System.Windows.Forms.ToolStripMenuItem copyToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem cutToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem pasteToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem flipAllToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem createFromFolderToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem pasteFlippedToolStripMenuItem;
    }
}
