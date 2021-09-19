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
            this.sldZoomLevel = new AGS.Editor.ZoomTrackbar();
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.panelScrollAreaCursor = new System.Windows.Forms.Panel();
            this.pnlCursorImage = new System.Windows.Forms.Panel();
            this.label2 = new System.Windows.Forms.Label();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.panelScrollAreaImage = new System.Windows.Forms.Panel();
            this.pnlInvWindowImage = new System.Windows.Forms.Panel();
            this.label1 = new System.Windows.Forms.Label();
            this.currentItemGroupBox.SuspendLayout();
            this.groupBox2.SuspendLayout();
            this.panelScrollAreaCursor.SuspendLayout();
            this.groupBox1.SuspendLayout();
            this.panelScrollAreaImage.SuspendLayout();
            this.SuspendLayout();
            // 
            // currentItemGroupBox
            // 
            this.currentItemGroupBox.BackColor = System.Drawing.SystemColors.Control;
            this.currentItemGroupBox.Controls.Add(this.sldZoomLevel);
            this.currentItemGroupBox.Controls.Add(this.groupBox2);
            this.currentItemGroupBox.Controls.Add(this.groupBox1);
            this.currentItemGroupBox.Controls.Add(this.label1);
            this.currentItemGroupBox.Location = new System.Drawing.Point(16, 20);
            this.currentItemGroupBox.Margin = new System.Windows.Forms.Padding(4);
            this.currentItemGroupBox.Name = "currentItemGroupBox";
            this.currentItemGroupBox.Padding = new System.Windows.Forms.Padding(4);
            this.currentItemGroupBox.Size = new System.Drawing.Size(748, 362);
            this.currentItemGroupBox.TabIndex = 1;
            this.currentItemGroupBox.TabStop = false;
            this.currentItemGroupBox.Text = "Selected inventory item settings";
            // 
            // sldZoomLevel
            // 
            this.sldZoomLevel.Location = new System.Drawing.Point(508, 22);
            this.sldZoomLevel.Maximum = 800;
            this.sldZoomLevel.Minimum = 100;
            this.sldZoomLevel.Name = "sldZoomLevel";
            this.sldZoomLevel.Size = new System.Drawing.Size(221, 31);
            this.sldZoomLevel.Step = 25;
            this.sldZoomLevel.TabIndex = 8;
            this.sldZoomLevel.Value = 400;
            this.sldZoomLevel.ZoomScale = 4F;
            this.sldZoomLevel.ValueChanged += new System.EventHandler(this.zoomSlider_ValueChanged);
            // 
            // groupBox2
            // 
            this.groupBox2.Controls.Add(this.panelScrollAreaCursor);
            this.groupBox2.Controls.Add(this.label2);
            this.groupBox2.Location = new System.Drawing.Point(395, 62);
            this.groupBox2.Margin = new System.Windows.Forms.Padding(4);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Padding = new System.Windows.Forms.Padding(4);
            this.groupBox2.Size = new System.Drawing.Size(338, 288);
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
            this.panelScrollAreaCursor.Location = new System.Drawing.Point(4, 38);
            this.panelScrollAreaCursor.Name = "panelScrollAreaCursor";
            this.panelScrollAreaCursor.Size = new System.Drawing.Size(330, 246);
            this.panelScrollAreaCursor.TabIndex = 5;
            // 
            // pnlCursorImage
            // 
            this.pnlCursorImage.Location = new System.Drawing.Point(4, 4);
            this.pnlCursorImage.Margin = new System.Windows.Forms.Padding(4);
            this.pnlCursorImage.Name = "pnlCursorImage";
            this.pnlCursorImage.Size = new System.Drawing.Size(304, 203);
            this.pnlCursorImage.TabIndex = 3;
            this.pnlCursorImage.Paint += new System.Windows.Forms.PaintEventHandler(this.pnlCursorImage_Paint);
            this.pnlCursorImage.MouseDown += new System.Windows.Forms.MouseEventHandler(this.pnlCursorImage_MouseDown);
            this.pnlCursorImage.MouseWheel += new System.Windows.Forms.MouseEventHandler(this.pnlCursorImage_MouseWheel);
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Dock = System.Windows.Forms.DockStyle.Top;
            this.label2.Location = new System.Drawing.Point(4, 21);
            this.label2.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(315, 17);
            this.label2.TabIndex = 4;
            this.label2.Text = "Click in the image below to set the cursor hotspot:";
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.panelScrollAreaImage);
            this.groupBox1.Location = new System.Drawing.Point(16, 62);
            this.groupBox1.Margin = new System.Windows.Forms.Padding(4);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Padding = new System.Windows.Forms.Padding(4);
            this.groupBox1.Size = new System.Drawing.Size(361, 288);
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
            this.panelScrollAreaImage.Size = new System.Drawing.Size(353, 263);
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
            this.label1.Location = new System.Drawing.Point(12, 22);
            this.label1.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(370, 17);
            this.label1.TabIndex = 0;
            this.label1.Text = "Use the property grid on the right to change basic settings.";
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
            this.currentItemGroupBox.PerformLayout();
            this.groupBox2.ResumeLayout(false);
            this.groupBox2.PerformLayout();
            this.panelScrollAreaCursor.ResumeLayout(false);
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.panelScrollAreaImage.ResumeLayout(false);
            this.ResumeLayout(false);

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
    }
}
