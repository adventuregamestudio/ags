namespace AGS.Editor
{
    partial class FontEditor
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
            this.currentItemGroupBox = new System.Windows.Forms.GroupBox();
            this.panel1 = new System.Windows.Forms.Panel();
            this.splitContainer1 = new System.Windows.Forms.SplitContainer();
            this.tbTextPreview = new System.Windows.Forms.TextBox();
            this.textPreviewPanel = new AGS.Editor.BufferedPanel();
            this.fontViewPanel = new AGS.Editor.BufferedPanel();
            this.btnImportFont = new System.Windows.Forms.Button();
            this.label1 = new System.Windows.Forms.Label();
            this.currentItemGroupBox.SuspendLayout();
            this.panel1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.splitContainer1)).BeginInit();
            this.splitContainer1.Panel1.SuspendLayout();
            this.splitContainer1.Panel2.SuspendLayout();
            this.splitContainer1.SuspendLayout();
            this.SuspendLayout();
            // 
            // currentItemGroupBox
            // 
            this.currentItemGroupBox.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.currentItemGroupBox.BackColor = System.Drawing.SystemColors.Control;
            this.currentItemGroupBox.Controls.Add(this.panel1);
            this.currentItemGroupBox.Controls.Add(this.btnImportFont);
            this.currentItemGroupBox.Controls.Add(this.label1);
            this.currentItemGroupBox.Location = new System.Drawing.Point(13, 13);
            this.currentItemGroupBox.Name = "currentItemGroupBox";
            this.currentItemGroupBox.Size = new System.Drawing.Size(505, 467);
            this.currentItemGroupBox.TabIndex = 3;
            this.currentItemGroupBox.TabStop = false;
            this.currentItemGroupBox.Text = "Selected font settings";
            // 
            // panel1
            // 
            this.panel1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.panel1.Controls.Add(this.splitContainer1);
            this.panel1.Location = new System.Drawing.Point(6, 72);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(493, 389);
            this.panel1.TabIndex = 5;
            // 
            // splitContainer1
            // 
            this.splitContainer1.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.splitContainer1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.splitContainer1.Location = new System.Drawing.Point(0, 0);
            this.splitContainer1.Name = "splitContainer1";
            this.splitContainer1.Orientation = System.Windows.Forms.Orientation.Horizontal;
            // 
            // splitContainer1.Panel1
            // 
            this.splitContainer1.Panel1.Controls.Add(this.tbTextPreview);
            this.splitContainer1.Panel1.Controls.Add(this.textPreviewPanel);
            this.splitContainer1.Panel1.Padding = new System.Windows.Forms.Padding(7);
            this.splitContainer1.Panel1MinSize = 120;
            // 
            // splitContainer1.Panel2
            // 
            this.splitContainer1.Panel2.Controls.Add(this.fontViewPanel);
            this.splitContainer1.Panel2.Padding = new System.Windows.Forms.Padding(7);
            this.splitContainer1.Size = new System.Drawing.Size(493, 389);
            this.splitContainer1.SplitterDistance = 120;
            this.splitContainer1.TabIndex = 5;
            this.splitContainer1.SplitterMoved += new System.Windows.Forms.SplitterEventHandler(this.splitContainer1_SplitterMoved);
            // 
            // tbTextPreview
            // 
            this.tbTextPreview.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.tbTextPreview.Location = new System.Drawing.Point(10, 64);
            this.tbTextPreview.Multiline = true;
            this.tbTextPreview.Name = "tbTextPreview";
            this.tbTextPreview.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
            this.tbTextPreview.Size = new System.Drawing.Size(471, 44);
            this.tbTextPreview.TabIndex = 5;
            this.tbTextPreview.Text = "The quick brown fox jumps over the lazy dog.";
            this.tbTextPreview.TextChanged += new System.EventHandler(this.tbTextPreview_TextChanged);
            // 
            // textPreviewPanel
            // 
            this.textPreviewPanel.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.textPreviewPanel.AutoScroll = true;
            this.textPreviewPanel.Location = new System.Drawing.Point(10, 13);
            this.textPreviewPanel.Name = "textPreviewPanel";
            this.textPreviewPanel.Size = new System.Drawing.Size(471, 45);
            this.textPreviewPanel.TabIndex = 4;
            this.textPreviewPanel.Paint += new System.Windows.Forms.PaintEventHandler(this.textPreviewPanel_Paint);
            // 
            // fontViewPanel
            // 
            this.fontViewPanel.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.fontViewPanel.AutoScroll = true;
            this.fontViewPanel.Location = new System.Drawing.Point(10, 10);
            this.fontViewPanel.Name = "fontViewPanel";
            this.fontViewPanel.Size = new System.Drawing.Size(471, 243);
            this.fontViewPanel.TabIndex = 3;
            this.fontViewPanel.Scroll += new System.Windows.Forms.ScrollEventHandler(this.imagePanel_Scroll);
            this.fontViewPanel.SizeChanged += new System.EventHandler(this.imagePanel_SizeChanged);
            this.fontViewPanel.Paint += new System.Windows.Forms.PaintEventHandler(this.fontViewPanel_Paint);
            // 
            // btnImportFont
            // 
            this.btnImportFont.Location = new System.Drawing.Point(13, 40);
            this.btnImportFont.Name = "btnImportFont";
            this.btnImportFont.Size = new System.Drawing.Size(132, 26);
            this.btnImportFont.TabIndex = 4;
            this.btnImportFont.Text = "Import over this font...";
            this.btnImportFont.UseVisualStyleBackColor = true;
            this.btnImportFont.Click += new System.EventHandler(this.btnImportFont_Click);
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(10, 18);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(282, 13);
            this.label1.TabIndex = 0;
            this.label1.Text = "Use the property grid on the right to change basic settings.";
            // 
            // FontEditor
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.currentItemGroupBox);
            this.Name = "FontEditor";
            this.Size = new System.Drawing.Size(534, 493);
            this.Load += new System.EventHandler(this.FontEditor_Load);
            this.currentItemGroupBox.ResumeLayout(false);
            this.currentItemGroupBox.PerformLayout();
            this.panel1.ResumeLayout(false);
            this.splitContainer1.Panel1.ResumeLayout(false);
            this.splitContainer1.Panel1.PerformLayout();
            this.splitContainer1.Panel2.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.splitContainer1)).EndInit();
            this.splitContainer1.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.GroupBox currentItemGroupBox;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Button btnImportFont;
        private BufferedPanel fontViewPanel;
        private System.Windows.Forms.Panel panel1;
        private System.Windows.Forms.SplitContainer splitContainer1;
        private System.Windows.Forms.TextBox tbTextPreview;
        private BufferedPanel textPreviewPanel;
    }
}
