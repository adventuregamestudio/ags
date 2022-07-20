namespace AGS.Editor
{
    partial class ViewEditor
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
            this.chkShowPreview = new System.Windows.Forms.CheckBox();
            this.editorPanel = new System.Windows.Forms.Panel();
            this.btnDeleteLastLoop = new System.Windows.Forms.Button();
            this.btnNewLoop = new System.Windows.Forms.Button();
            this.splitContainer1 = new System.Windows.Forms.SplitContainer();
            this.viewPreview = new AGS.Editor.ViewPreview();
            this.sldZoomLevel = new AGS.Editor.ZoomTrackbar();
            this.editorPanel.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.splitContainer1)).BeginInit();
            this.splitContainer1.Panel1.SuspendLayout();
            this.splitContainer1.Panel2.SuspendLayout();
            this.splitContainer1.SuspendLayout();
            this.SuspendLayout();
            // 
            // chkShowPreview
            // 
            this.chkShowPreview.AutoSize = true;
            this.chkShowPreview.Location = new System.Drawing.Point(12, 12);
            this.chkShowPreview.Name = "chkShowPreview";
            this.chkShowPreview.Size = new System.Drawing.Size(93, 17);
            this.chkShowPreview.TabIndex = 2;
            this.chkShowPreview.Text = "Show Preview";
            this.chkShowPreview.UseVisualStyleBackColor = true;
            this.chkShowPreview.CheckedChanged += new System.EventHandler(this.chkShowPreview_CheckedChanged);
            // 
            // editorPanel
            // 
            this.editorPanel.AutoScroll = true;
            this.editorPanel.Controls.Add(this.btnDeleteLastLoop);
            this.editorPanel.Controls.Add(this.btnNewLoop);
            this.editorPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.editorPanel.Location = new System.Drawing.Point(0, 0);
            this.editorPanel.Name = "editorPanel";
            this.editorPanel.Size = new System.Drawing.Size(372, 463);
            this.editorPanel.TabIndex = 4;
            // 
            // btnDeleteLastLoop
            // 
            this.btnDeleteLastLoop.Location = new System.Drawing.Point(187, 72);
            this.btnDeleteLastLoop.Name = "btnDeleteLastLoop";
            this.btnDeleteLastLoop.Size = new System.Drawing.Size(132, 31);
            this.btnDeleteLastLoop.TabIndex = 3;
            this.btnDeleteLastLoop.Text = "Delete last loop";
            this.btnDeleteLastLoop.UseVisualStyleBackColor = true;
            this.btnDeleteLastLoop.Click += new System.EventHandler(this.btnDeleteLastLoop_Click);
            // 
            // btnNewLoop
            // 
            this.btnNewLoop.Location = new System.Drawing.Point(20, 72);
            this.btnNewLoop.Name = "btnNewLoop";
            this.btnNewLoop.Size = new System.Drawing.Size(132, 31);
            this.btnNewLoop.TabIndex = 2;
            this.btnNewLoop.Text = "Create new loop";
            this.btnNewLoop.UseVisualStyleBackColor = true;
            this.btnNewLoop.Click += new System.EventHandler(this.btnNewLoop_Click);
            // 
            // splitContainer1
            // 
            this.splitContainer1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.splitContainer1.Location = new System.Drawing.Point(12, 35);
            this.splitContainer1.Name = "splitContainer1";
            // 
            // splitContainer1.Panel1
            // 
            this.splitContainer1.Panel1.Controls.Add(this.viewPreview);
            // 
            // splitContainer1.Panel2
            // 
            this.splitContainer1.Panel2.Controls.Add(this.editorPanel);
            this.splitContainer1.Size = new System.Drawing.Size(647, 463);
            this.splitContainer1.SplitterDistance = 271;
            this.splitContainer1.TabIndex = 6;
            // 
            // viewPreview
            // 
            this.viewPreview.AutoResize = false;
            this.viewPreview.Dock = System.Windows.Forms.DockStyle.Fill;
            this.viewPreview.DynamicUpdates = false;
            this.viewPreview.IsCharacterView = false;
            this.viewPreview.Location = new System.Drawing.Point(0, 0);
            this.viewPreview.Margin = new System.Windows.Forms.Padding(4);
            this.viewPreview.Name = "viewPreview";
            this.viewPreview.Size = new System.Drawing.Size(271, 463);
            this.viewPreview.TabIndex = 3;
            this.viewPreview.Title = "Preview";
            this.viewPreview.ViewToPreview = null;
            this.viewPreview.ZoomLevel = 1F;
            // 
            // sldZoomLevel
            // 
            this.sldZoomLevel.Location = new System.Drawing.Point(172, 3);
            this.sldZoomLevel.Margin = new System.Windows.Forms.Padding(2, 2, 2, 2);
            this.sldZoomLevel.Maximum = 600;
            this.sldZoomLevel.Minimum = 75;
            this.sldZoomLevel.Name = "sldZoomLevel";
            this.sldZoomLevel.Size = new System.Drawing.Size(221, 31);
            this.sldZoomLevel.Step = 25;
            this.sldZoomLevel.TabIndex = 5;
            this.sldZoomLevel.Value = 100;
            this.sldZoomLevel.ZoomScale = 1F;
            this.sldZoomLevel.ValueChanged += new System.EventHandler(this.sldZoomLevel_ValueChanged);
            // 
            // ViewEditor
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.splitContainer1);
            this.Controls.Add(this.sldZoomLevel);
            this.Controls.Add(this.chkShowPreview);
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Name = "ViewEditor";
            this.Size = new System.Drawing.Size(674, 514);
            this.Load += new System.EventHandler(this.ViewEditor_Load);
            this.Resize += new System.EventHandler(this.ViewEditor_Resize);
            this.editorPanel.ResumeLayout(false);
            this.splitContainer1.Panel1.ResumeLayout(false);
            this.splitContainer1.Panel2.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.splitContainer1)).EndInit();
            this.splitContainer1.ResumeLayout(false);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

		private System.Windows.Forms.CheckBox chkShowPreview;
		private ViewPreview viewPreview;
		private System.Windows.Forms.Panel editorPanel;
		private System.Windows.Forms.Button btnDeleteLastLoop;
		private System.Windows.Forms.Button btnNewLoop;
		private AGS.Editor.ZoomTrackbar sldZoomLevel;
        private System.Windows.Forms.SplitContainer splitContainer1;
    }
}
