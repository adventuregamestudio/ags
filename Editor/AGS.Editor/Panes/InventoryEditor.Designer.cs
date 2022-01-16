namespace AGS.Editor
{
    partial class InventoryEditor
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
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.panelScrollAreaCursor = new System.Windows.Forms.Panel();
            this.pnlCursorImage = new System.Windows.Forms.Panel();
            this.label2 = new System.Windows.Forms.Label();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.panelScrollAreaImage = new System.Windows.Forms.Panel();
            this.pnlInvWindowImage = new System.Windows.Forms.Panel();
            this.label1 = new System.Windows.Forms.Label();
            this.sldZoomLevel = new AGS.Editor.ZoomTrackbar();
            this.splitContainer1 = new System.Windows.Forms.SplitContainer();
            this.flowLayoutPanel1 = new System.Windows.Forms.FlowLayoutPanel();
            this.currentItemGroupBox.SuspendLayout();
            this.groupBox2.SuspendLayout();
            this.panelScrollAreaCursor.SuspendLayout();
            this.groupBox1.SuspendLayout();
            this.panelScrollAreaImage.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.splitContainer1)).BeginInit();
            this.splitContainer1.Panel1.SuspendLayout();
            this.splitContainer1.Panel2.SuspendLayout();
            this.splitContainer1.SuspendLayout();
            this.flowLayoutPanel1.SuspendLayout();
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
            this.currentItemGroupBox.Size = new System.Drawing.Size(791, 414);
            this.currentItemGroupBox.TabIndex = 1;
            this.currentItemGroupBox.TabStop = false;
            this.currentItemGroupBox.Text = "Selected inventory item settings";
            // 
            // groupBox2
            // 
            this.groupBox2.AutoSize = true;
            this.groupBox2.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.groupBox2.Controls.Add(this.panelScrollAreaCursor);
            this.groupBox2.Dock = System.Windows.Forms.DockStyle.Fill;
            this.groupBox2.Location = new System.Drawing.Point(354, 4);
            this.groupBox2.Margin = new System.Windows.Forms.Padding(4);
            this.groupBox2.MinimumSize = new System.Drawing.Size(300, 300);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Padding = new System.Windows.Forms.Padding(4);
            this.groupBox2.Size = new System.Drawing.Size(320, 300);
            this.groupBox2.TabIndex = 7;
            this.groupBox2.TabStop = false;
            this.groupBox2.Text = "Mouse cursor image";
            // 
            // panelScrollAreaCursor
            // 
            this.panelScrollAreaCursor.AutoScroll = true;
            this.panelScrollAreaCursor.AutoSize = true;
            this.panelScrollAreaCursor.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.panelScrollAreaCursor.Controls.Add(this.pnlCursorImage);
            this.panelScrollAreaCursor.Dock = System.Windows.Forms.DockStyle.Fill;
            this.panelScrollAreaCursor.Location = new System.Drawing.Point(4, 21);
            this.panelScrollAreaCursor.Name = "panelScrollAreaCursor";
            this.panelScrollAreaCursor.Size = new System.Drawing.Size(312, 275);
            this.panelScrollAreaCursor.TabIndex = 5;
            // 
            // pnlCursorImage
            // 
            this.pnlCursorImage.Location = new System.Drawing.Point(4, 4);
            this.pnlCursorImage.Margin = new System.Windows.Forms.Padding(4);
            this.pnlCursorImage.Name = "pnlCursorImage";
            this.pnlCursorImage.Size = new System.Drawing.Size(304, 244);
            this.pnlCursorImage.TabIndex = 3;
            this.pnlCursorImage.Paint += new System.Windows.Forms.PaintEventHandler(this.pnlCursorImage_Paint);
            this.pnlCursorImage.MouseDown += new System.Windows.Forms.MouseEventHandler(this.pnlCursorImage_MouseDown);
            this.pnlCursorImage.MouseWheel += new System.Windows.Forms.MouseEventHandler(this.pnlCursorImage_MouseWheel);
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(4, 26);
            this.label2.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(314, 17);
            this.label2.TabIndex = 4;
            this.label2.Text = "Click in the Mouse cursor image to set it\'s hotspot.";
            // 
            // groupBox1
            // 
            this.groupBox1.AutoSize = true;
            this.groupBox1.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.groupBox1.Controls.Add(this.panelScrollAreaImage);
            this.groupBox1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.groupBox1.Location = new System.Drawing.Point(4, 4);
            this.groupBox1.Margin = new System.Windows.Forms.Padding(4);
            this.groupBox1.MinimumSize = new System.Drawing.Size(300, 300);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Padding = new System.Windows.Forms.Padding(4);
            this.groupBox1.Size = new System.Drawing.Size(342, 300);
            this.groupBox1.TabIndex = 6;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Image in inventory window";
            // 
            // panelScrollAreaImage
            // 
            this.panelScrollAreaImage.AutoScroll = true;
            this.panelScrollAreaImage.AutoSize = true;
            this.panelScrollAreaImage.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.panelScrollAreaImage.Controls.Add(this.pnlInvWindowImage);
            this.panelScrollAreaImage.Dock = System.Windows.Forms.DockStyle.Fill;
            this.panelScrollAreaImage.Location = new System.Drawing.Point(4, 21);
            this.panelScrollAreaImage.Name = "panelScrollAreaImage";
            this.panelScrollAreaImage.Size = new System.Drawing.Size(334, 275);
            this.panelScrollAreaImage.TabIndex = 6;
            // 
            // pnlInvWindowImage
            // 
            this.pnlInvWindowImage.Location = new System.Drawing.Point(4, 4);
            this.pnlInvWindowImage.Margin = new System.Windows.Forms.Padding(4);
            this.pnlInvWindowImage.Name = "pnlInvWindowImage";
            this.pnlInvWindowImage.Size = new System.Drawing.Size(326, 244);
            this.pnlInvWindowImage.TabIndex = 5;
            this.pnlInvWindowImage.Paint += new System.Windows.Forms.PaintEventHandler(this.pnlInvWindowImage_Paint);
            this.pnlInvWindowImage.MouseWheel += new System.Windows.Forms.MouseEventHandler(this.pnlInvWindowImage_MouseWheel);
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(4, 3);
            this.label1.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(370, 17);
            this.label1.TabIndex = 0;
            this.label1.Text = "Use the property grid on the right to change basic settings.";
            // 
            // sldZoomLevel
            // 
            this.sldZoomLevel.Location = new System.Drawing.Point(394, 3);
            this.sldZoomLevel.Maximum = 800;
            this.sldZoomLevel.Minimum = 100;
            this.sldZoomLevel.Name = "sldZoomLevel";
            this.sldZoomLevel.Size = new System.Drawing.Size(221, 40);
            this.sldZoomLevel.Step = 25;
            this.sldZoomLevel.TabIndex = 8;
            this.sldZoomLevel.Value = 400;
            this.sldZoomLevel.ZoomScale = 4F;
            this.sldZoomLevel.ValueChanged += new System.EventHandler(this.zoomSlider_ValueChanged);
            // 
            // splitContainer1
            // 
            this.splitContainer1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.splitContainer1.FixedPanel = System.Windows.Forms.FixedPanel.Panel1;
            this.splitContainer1.IsSplitterFixed = true;
            this.splitContainer1.Location = new System.Drawing.Point(4, 21);
            this.splitContainer1.Name = "splitContainer1";
            this.splitContainer1.Orientation = System.Windows.Forms.Orientation.Horizontal;
            // 
            // splitContainer1.Panel1
            // 
            this.splitContainer1.Panel1.Controls.Add(this.label1);
            this.splitContainer1.Panel1.Controls.Add(this.label2);
            this.splitContainer1.Panel1.Controls.Add(this.sldZoomLevel);
            // 
            // splitContainer1.Panel2
            // 
            this.splitContainer1.Panel2.Controls.Add(this.flowLayoutPanel1);
            this.splitContainer1.Size = new System.Drawing.Size(783, 389);
            this.splitContainer1.SplitterDistance = 60;
            this.splitContainer1.TabIndex = 9;
            // 
            // flowLayoutPanel1
            // 
            this.flowLayoutPanel1.AutoSize = true;
            this.flowLayoutPanel1.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.flowLayoutPanel1.Controls.Add(this.groupBox1);
            this.flowLayoutPanel1.Controls.Add(this.groupBox2);
            this.flowLayoutPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.flowLayoutPanel1.Location = new System.Drawing.Point(0, 0);
            this.flowLayoutPanel1.MinimumSize = new System.Drawing.Size(400, 400);
            this.flowLayoutPanel1.Name = "flowLayoutPanel1";
            this.flowLayoutPanel1.Size = new System.Drawing.Size(783, 400);
            this.flowLayoutPanel1.TabIndex = 8;
            // 
            // InventoryEditor
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(120F, 120F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
            this.BackColor = System.Drawing.SystemColors.Control;
            this.Controls.Add(this.currentItemGroupBox);
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Margin = new System.Windows.Forms.Padding(4);
            this.Name = "InventoryEditor";
            this.Size = new System.Drawing.Size(791, 414);
            this.currentItemGroupBox.ResumeLayout(false);
            this.groupBox2.ResumeLayout(false);
            this.groupBox2.PerformLayout();
            this.panelScrollAreaCursor.ResumeLayout(false);
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.panelScrollAreaImage.ResumeLayout(false);
            this.splitContainer1.Panel1.ResumeLayout(false);
            this.splitContainer1.Panel1.PerformLayout();
            this.splitContainer1.Panel2.ResumeLayout(false);
            this.splitContainer1.Panel2.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.splitContainer1)).EndInit();
            this.splitContainer1.ResumeLayout(false);
            this.flowLayoutPanel1.ResumeLayout(false);
            this.flowLayoutPanel1.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.GroupBox currentItemGroupBox;
        private System.Windows.Forms.Panel pnlCursorImage;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.GroupBox groupBox2;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.Panel pnlInvWindowImage;
        private AGS.Editor.ZoomTrackbar sldZoomLevel;
        private System.Windows.Forms.Panel panelScrollAreaCursor;
        private System.Windows.Forms.Panel panelScrollAreaImage;
        private System.Windows.Forms.SplitContainer splitContainer1;
        private System.Windows.Forms.FlowLayoutPanel flowLayoutPanel1;
    }
}
