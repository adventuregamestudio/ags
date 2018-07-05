namespace AGS.Editor
{
    partial class GUIEditor
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
            this.ctrlPanel = new System.Windows.Forms.Panel();
            this.lblZoomInfo = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.sldZoomLevel = new System.Windows.Forms.TrackBar();
            this.bgPanel = new AGS.Editor.BufferedPanel();
            this.lblDummyScrollSizer = new System.Windows.Forms.Label();
            this.ctrlPanel.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.sldZoomLevel)).BeginInit();
            this.bgPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // ctrlPanel
            // 
            this.ctrlPanel.Controls.Add(this.lblZoomInfo);
            this.ctrlPanel.Controls.Add(this.label3);
            this.ctrlPanel.Controls.Add(this.sldZoomLevel);
            this.ctrlPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this.ctrlPanel.Location = new System.Drawing.Point(0, 0);
            this.ctrlPanel.Name = "ctrlPanel";
            this.ctrlPanel.Size = new System.Drawing.Size(702, 51);
            this.ctrlPanel.TabIndex = 0;
            // 
            // lblZoomInfo
            // 
            this.lblZoomInfo.AutoSize = true;
            this.lblZoomInfo.Location = new System.Drawing.Point(206, 15);
            this.lblZoomInfo.Name = "lblZoomInfo";
            this.lblZoomInfo.Size = new System.Drawing.Size(33, 13);
            this.lblZoomInfo.TabIndex = 2;
            this.lblZoomInfo.Text = "100%";
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(3, 15);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(37, 13);
            this.label3.TabIndex = 0;
            this.label3.Text = "Zoom:";
            // 
            // sldZoomLevel
            // 
            this.sldZoomLevel.LargeChange = 1;
            this.sldZoomLevel.Location = new System.Drawing.Point(46, 6);
            this.sldZoomLevel.Minimum = 1;
            this.sldZoomLevel.Name = "sldZoomLevel";
            this.sldZoomLevel.Size = new System.Drawing.Size(154, 42);
            this.sldZoomLevel.TabIndex = 1;
            this.sldZoomLevel.Value = 1;
            this.sldZoomLevel.Scroll += new System.EventHandler(this.sldZoomLevel_Scroll);
            // 
            // bgPanel
            // 
            this.bgPanel.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.bgPanel.AutoScroll = true;
            this.bgPanel.BackColor = System.Drawing.Color.Transparent;
            this.bgPanel.Controls.Add(this.lblDummyScrollSizer);
            this.bgPanel.Location = new System.Drawing.Point(0, 57);
            this.bgPanel.Name = "bgPanel";
            this.bgPanel.Size = new System.Drawing.Size(702, 402);
            this.bgPanel.TabIndex = 1;
            this.bgPanel.TabStop = true;
            this.bgPanel.MouseLeave += new System.EventHandler(this.bgPanel_MouseLeave);
            this.bgPanel.Paint += new System.Windows.Forms.PaintEventHandler(this.bgPanel_Paint);
            this.bgPanel.MouseMove += new System.Windows.Forms.MouseEventHandler(this.bgPanel_MouseMove);
            this.bgPanel.MouseDoubleClick += new System.Windows.Forms.MouseEventHandler(this.bgPanel_MouseDoubleClick);
            this.bgPanel.MouseDown += new System.Windows.Forms.MouseEventHandler(this.bgPanel_MouseDown);
            this.bgPanel.MouseUp += new System.Windows.Forms.MouseEventHandler(this.bgPanel_MouseUp);
            this.bgPanel.MouseEnter += new System.EventHandler(this.bgPanel_MouseEnter);
            // 
            // lblDummyScrollSizer
            // 
            this.lblDummyScrollSizer.AutoSize = true;
            this.lblDummyScrollSizer.Location = new System.Drawing.Point(351, 223);
            this.lblDummyScrollSizer.Name = "lblDummyScrollSizer";
            this.lblDummyScrollSizer.Size = new System.Drawing.Size(0, 13);
            this.lblDummyScrollSizer.TabIndex = 0;
            // 
            // GUIEditor
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.ctrlPanel);
            this.Controls.Add(this.bgPanel);
            this.Name = "GUIEditor";
            this.Size = new System.Drawing.Size(702, 459);
            this.ctrlPanel.ResumeLayout(false);
            this.ctrlPanel.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.sldZoomLevel)).EndInit();
            this.bgPanel.ResumeLayout(false);
            this.bgPanel.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private BufferedPanel bgPanel;
        private System.Windows.Forms.Label lblDummyScrollSizer;
        private System.Windows.Forms.Panel ctrlPanel;
        private System.Windows.Forms.Label lblZoomInfo;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.TrackBar sldZoomLevel;
    }
}
