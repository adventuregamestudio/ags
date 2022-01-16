namespace AGS.Editor
{
    partial class CursorEditor
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
            this.splitContainer1 = new System.Windows.Forms.SplitContainer();
            this.label1 = new System.Windows.Forms.Label();
            this.sldZoomLevel = new AGS.Editor.ZoomTrackbar();
            this.label2 = new System.Windows.Forms.Label();
            this.cursorPanelScrollArea = new System.Windows.Forms.Panel();
            this.imagePanel = new System.Windows.Forms.Panel();
            this.currentItemGroupBox.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.splitContainer1)).BeginInit();
            this.splitContainer1.Panel1.SuspendLayout();
            this.splitContainer1.Panel2.SuspendLayout();
            this.splitContainer1.SuspendLayout();
            this.cursorPanelScrollArea.SuspendLayout();
            this.SuspendLayout();
            // 
            // currentItemGroupBox
            // 
            this.currentItemGroupBox.AutoSize = true;
            this.currentItemGroupBox.BackColor = System.Drawing.SystemColors.Control;
            this.currentItemGroupBox.Controls.Add(this.splitContainer1);
            this.currentItemGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this.currentItemGroupBox.Location = new System.Drawing.Point(0, 0);
            this.currentItemGroupBox.Margin = new System.Windows.Forms.Padding(4);
            this.currentItemGroupBox.Name = "currentItemGroupBox";
            this.currentItemGroupBox.Padding = new System.Windows.Forms.Padding(4);
            this.currentItemGroupBox.Size = new System.Drawing.Size(647, 418);
            this.currentItemGroupBox.TabIndex = 2;
            this.currentItemGroupBox.TabStop = false;
            this.currentItemGroupBox.Text = "Selected mouse cursor settings";
            // 
            // splitContainer1
            // 
            this.splitContainer1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.splitContainer1.FixedPanel = System.Windows.Forms.FixedPanel.Panel1;
            this.splitContainer1.IsSplitterFixed = true;
            this.splitContainer1.Location = new System.Drawing.Point(4, 19);
            this.splitContainer1.Name = "splitContainer1";
            this.splitContainer1.Orientation = System.Windows.Forms.Orientation.Horizontal;
            // 
            // splitContainer1.Panel1
            // 
            this.splitContainer1.Panel1.Controls.Add(this.label1);
            this.splitContainer1.Panel1.Controls.Add(this.sldZoomLevel);
            this.splitContainer1.Panel1.Controls.Add(this.label2);
            // 
            // splitContainer1.Panel2
            // 
            this.splitContainer1.Panel2.Controls.Add(this.cursorPanelScrollArea);
            this.splitContainer1.Size = new System.Drawing.Size(639, 395);
            this.splitContainer1.SplitterDistance = 100;
            this.splitContainer1.TabIndex = 10;
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(4, 6);
            this.label1.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(379, 17);
            this.label1.TabIndex = 0;
            this.label1.Text = "Use the property grid on the right to change basic settings.";
            // 
            // sldZoomLevel
            // 
            this.sldZoomLevel.Location = new System.Drawing.Point(149, 50);
            this.sldZoomLevel.Maximum = 800;
            this.sldZoomLevel.Minimum = 100;
            this.sldZoomLevel.Name = "sldZoomLevel";
            this.sldZoomLevel.Size = new System.Drawing.Size(221, 31);
            this.sldZoomLevel.Step = 25;
            this.sldZoomLevel.TabIndex = 9;
            this.sldZoomLevel.Value = 400;
            this.sldZoomLevel.ZoomScale = 4F;
            this.sldZoomLevel.ValueChanged += new System.EventHandler(this.sldZoomLevel_ValueChanged);
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(4, 30);
            this.label2.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(320, 17);
            this.label2.TabIndex = 4;
            this.label2.Text = "Click in the image below to set the cursor hotspot:";
            // 
            // cursorPanelScrollArea
            // 
            this.cursorPanelScrollArea.AutoScroll = true;
            this.cursorPanelScrollArea.AutoSize = true;
            this.cursorPanelScrollArea.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.cursorPanelScrollArea.Controls.Add(this.imagePanel);
            this.cursorPanelScrollArea.Dock = System.Windows.Forms.DockStyle.Fill;
            this.cursorPanelScrollArea.Location = new System.Drawing.Point(0, 0);
            this.cursorPanelScrollArea.Name = "cursorPanelScrollArea";
            this.cursorPanelScrollArea.Size = new System.Drawing.Size(639, 291);
            this.cursorPanelScrollArea.TabIndex = 5;
            // 
            // imagePanel
            // 
            this.imagePanel.Location = new System.Drawing.Point(4, 4);
            this.imagePanel.Margin = new System.Windows.Forms.Padding(4);
            this.imagePanel.Name = "imagePanel";
            this.imagePanel.Size = new System.Drawing.Size(320, 195);
            this.imagePanel.TabIndex = 3;
            this.imagePanel.Paint += new System.Windows.Forms.PaintEventHandler(this.imagePanel_Paint);
            this.imagePanel.MouseDown += new System.Windows.Forms.MouseEventHandler(this.imagePanel_MouseDown);
            // 
            // CursorEditor
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(8F, 16F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.currentItemGroupBox);
            this.Margin = new System.Windows.Forms.Padding(4);
            this.Name = "CursorEditor";
            this.Size = new System.Drawing.Size(647, 418);
            this.currentItemGroupBox.ResumeLayout(false);
            this.splitContainer1.Panel1.ResumeLayout(false);
            this.splitContainer1.Panel1.PerformLayout();
            this.splitContainer1.Panel2.ResumeLayout(false);
            this.splitContainer1.Panel2.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.splitContainer1)).EndInit();
            this.splitContainer1.ResumeLayout(false);
            this.cursorPanelScrollArea.ResumeLayout(false);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.GroupBox currentItemGroupBox;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Panel imagePanel;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Panel cursorPanelScrollArea;
        private ZoomTrackbar sldZoomLevel;
        private System.Windows.Forms.SplitContainer splitContainer1;
    }
}
