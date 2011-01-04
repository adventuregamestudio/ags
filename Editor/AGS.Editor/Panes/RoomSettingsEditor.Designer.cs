namespace AGS.Editor
{
    partial class RoomSettingsEditor
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
			this.mainFrame = new System.Windows.Forms.GroupBox();
			this.lblTransparency = new System.Windows.Forms.Label();
			this.label3 = new System.Windows.Forms.Label();
			this.lblMousePos = new System.Windows.Forms.Label();
			this.label2 = new System.Windows.Forms.Label();
			this.cmbViewType = new System.Windows.Forms.ComboBox();
			this.btnExport = new System.Windows.Forms.Button();
			this.btnDelete = new System.Windows.Forms.Button();
			this.btnChangeImage = new System.Windows.Forms.Button();
			this.label1 = new System.Windows.Forms.Label();
			this.cmbBackgrounds = new System.Windows.Forms.ComboBox();
			this.bufferedPanel1 = new AGS.Editor.BufferedPanel();
			this.lblDummyScrollSizer = new System.Windows.Forms.Label();
			this.sldZoomLevel = new System.Windows.Forms.TrackBar();
			this.sldTransparency = new System.Windows.Forms.TrackBar();
			this.mainFrame.SuspendLayout();
			this.bufferedPanel1.SuspendLayout();
			((System.ComponentModel.ISupportInitialize)(this.sldZoomLevel)).BeginInit();
			((System.ComponentModel.ISupportInitialize)(this.sldTransparency)).BeginInit();
			this.SuspendLayout();
			// 
			// mainFrame
			// 
			this.mainFrame.Controls.Add(this.lblTransparency);
			this.mainFrame.Controls.Add(this.label3);
			this.mainFrame.Controls.Add(this.lblMousePos);
			this.mainFrame.Controls.Add(this.label2);
			this.mainFrame.Controls.Add(this.cmbViewType);
			this.mainFrame.Controls.Add(this.btnExport);
			this.mainFrame.Controls.Add(this.btnDelete);
			this.mainFrame.Controls.Add(this.btnChangeImage);
			this.mainFrame.Controls.Add(this.label1);
			this.mainFrame.Controls.Add(this.cmbBackgrounds);
			this.mainFrame.Controls.Add(this.bufferedPanel1);
			this.mainFrame.Controls.Add(this.sldZoomLevel);
			this.mainFrame.Controls.Add(this.sldTransparency);
			this.mainFrame.Location = new System.Drawing.Point(3, 3);
			this.mainFrame.Name = "mainFrame";
			this.mainFrame.Size = new System.Drawing.Size(664, 559);
			this.mainFrame.TabIndex = 4;
			this.mainFrame.TabStop = false;
			this.mainFrame.Text = "Room details";
			// 
			// lblTransparency
			// 
			this.lblTransparency.AutoSize = true;
			this.lblTransparency.Location = new System.Drawing.Point(587, 16);
			this.lblTransparency.Name = "lblTransparency";
			this.lblTransparency.Size = new System.Drawing.Size(38, 13);
			this.lblTransparency.TabIndex = 15;
			this.lblTransparency.Text = "Trans:";
			this.lblTransparency.Visible = false;
			// 
			// label3
			// 
			this.label3.AutoSize = true;
			this.label3.Location = new System.Drawing.Point(463, 45);
			this.label3.Name = "label3";
			this.label3.Size = new System.Drawing.Size(37, 13);
			this.label3.TabIndex = 13;
			this.label3.Text = "Zoom:";
			// 
			// lblMousePos
			// 
			this.lblMousePos.AutoSize = true;
			this.lblMousePos.Location = new System.Drawing.Point(299, 45);
			this.lblMousePos.Name = "lblMousePos";
			this.lblMousePos.Size = new System.Drawing.Size(82, 13);
			this.lblMousePos.TabIndex = 11;
			this.lblMousePos.Text = "Mouse Position:";
			// 
			// label2
			// 
			this.label2.AutoSize = true;
			this.label2.Location = new System.Drawing.Point(10, 45);
			this.label2.Name = "label2";
			this.label2.Size = new System.Drawing.Size(91, 13);
			this.label2.TabIndex = 10;
			this.label2.Text = "Show this room\'s:";
			// 
			// cmbViewType
			// 
			this.cmbViewType.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
			this.cmbViewType.FormattingEnabled = true;
			this.cmbViewType.Location = new System.Drawing.Point(132, 42);
			this.cmbViewType.Name = "cmbViewType";
			this.cmbViewType.Size = new System.Drawing.Size(153, 21);
			this.cmbViewType.TabIndex = 9;
			this.cmbViewType.SelectedIndexChanged += new System.EventHandler(this.cmbViewType_SelectedIndexChanged);
			// 
			// btnExport
			// 
			this.btnExport.Location = new System.Drawing.Point(497, 17);
			this.btnExport.Name = "btnExport";
			this.btnExport.Size = new System.Drawing.Size(84, 21);
			this.btnExport.TabIndex = 8;
			this.btnExport.Text = "Export...";
			this.btnExport.UseVisualStyleBackColor = true;
			this.btnExport.Click += new System.EventHandler(this.btnExport_Click);
			// 
			// btnDelete
			// 
			this.btnDelete.Location = new System.Drawing.Point(397, 17);
			this.btnDelete.Name = "btnDelete";
			this.btnDelete.Size = new System.Drawing.Size(84, 21);
			this.btnDelete.TabIndex = 7;
			this.btnDelete.Text = "Delete";
			this.btnDelete.UseVisualStyleBackColor = true;
			this.btnDelete.Click += new System.EventHandler(this.btnDelete_Click);
			// 
			// btnChangeImage
			// 
			this.btnChangeImage.Location = new System.Drawing.Point(297, 17);
			this.btnChangeImage.Name = "btnChangeImage";
			this.btnChangeImage.Size = new System.Drawing.Size(84, 21);
			this.btnChangeImage.TabIndex = 3;
			this.btnChangeImage.Text = "Change...";
			this.btnChangeImage.UseVisualStyleBackColor = true;
			this.btnChangeImage.Click += new System.EventHandler(this.btnChangeImage_Click);
			// 
			// label1
			// 
			this.label1.AutoSize = true;
			this.label1.Location = new System.Drawing.Point(9, 19);
			this.label1.Name = "label1";
			this.label1.Size = new System.Drawing.Size(116, 13);
			this.label1.TabIndex = 5;
			this.label1.Text = "Background to display:";
			// 
			// cmbBackgrounds
			// 
			this.cmbBackgrounds.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
			this.cmbBackgrounds.FormattingEnabled = true;
			this.cmbBackgrounds.Location = new System.Drawing.Point(132, 16);
			this.cmbBackgrounds.Name = "cmbBackgrounds";
			this.cmbBackgrounds.Size = new System.Drawing.Size(153, 21);
			this.cmbBackgrounds.TabIndex = 4;
			this.cmbBackgrounds.SelectedIndexChanged += new System.EventHandler(this.cmbBackgrounds_SelectedIndexChanged);
			// 
			// bufferedPanel1
			// 
			this.bufferedPanel1.AutoScroll = true;
			this.bufferedPanel1.Controls.Add(this.lblDummyScrollSizer);
			this.bufferedPanel1.Location = new System.Drawing.Point(12, 73);
			this.bufferedPanel1.Name = "bufferedPanel1";
			this.bufferedPanel1.Size = new System.Drawing.Size(640, 480);
			this.bufferedPanel1.TabIndex = 1;
			this.bufferedPanel1.TabStop = true;
			this.bufferedPanel1.DoubleClick += new System.EventHandler(this.bufferedPanel1_DoubleClick);
			this.bufferedPanel1.Paint += new System.Windows.Forms.PaintEventHandler(this.bufferedPanel1_Paint);
			this.bufferedPanel1.MouseMove += new System.Windows.Forms.MouseEventHandler(this.bufferedPanel1_MouseMove);
			this.bufferedPanel1.MouseDown += new System.Windows.Forms.MouseEventHandler(this.bufferedPanel1_MouseDown);
			this.bufferedPanel1.MouseUp += new System.Windows.Forms.MouseEventHandler(this.bufferedPanel1_MouseUp);
			// 
			// lblDummyScrollSizer
			// 
			this.lblDummyScrollSizer.AutoSize = true;
			this.lblDummyScrollSizer.Location = new System.Drawing.Point(73, 92);
			this.lblDummyScrollSizer.Name = "lblDummyScrollSizer";
			this.lblDummyScrollSizer.Size = new System.Drawing.Size(0, 13);
			this.lblDummyScrollSizer.TabIndex = 0;
			// 
			// sldZoomLevel
			// 
			this.sldZoomLevel.LargeChange = 1;
			this.sldZoomLevel.Location = new System.Drawing.Point(497, 42);
			this.sldZoomLevel.Maximum = 5;
			this.sldZoomLevel.Minimum = 1;
			this.sldZoomLevel.Name = "sldZoomLevel";
			this.sldZoomLevel.Size = new System.Drawing.Size(84, 42);
			this.sldZoomLevel.TabIndex = 12;
			this.sldZoomLevel.Value = 1;
			this.sldZoomLevel.Scroll += new System.EventHandler(this.sldZoomLevel_Scroll);
			// 
			// sldTransparency
			// 
			this.sldTransparency.LargeChange = 20;
			this.sldTransparency.Location = new System.Drawing.Point(586, 25);
			this.sldTransparency.Margin = new System.Windows.Forms.Padding(1);
			this.sldTransparency.Maximum = 100;
			this.sldTransparency.Name = "sldTransparency";
			this.sldTransparency.Orientation = System.Windows.Forms.Orientation.Vertical;
			this.sldTransparency.Size = new System.Drawing.Size(42, 54);
			this.sldTransparency.SmallChange = 5;
			this.sldTransparency.TabIndex = 14;
			this.sldTransparency.TickFrequency = 20;
			this.sldTransparency.Value = 70;
			this.sldTransparency.Visible = false;
			this.sldTransparency.Scroll += new System.EventHandler(this.sldTransparency_Scroll);
			// 
			// RoomSettingsEditor
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF(96F, 96F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
			this.Controls.Add(this.mainFrame);
			this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.Name = "RoomSettingsEditor";
			this.Size = new System.Drawing.Size(685, 599);
			this.Resize += new System.EventHandler(this.RoomSettingsEditor_Resize);
			this.mainFrame.ResumeLayout(false);
			this.mainFrame.PerformLayout();
			this.bufferedPanel1.ResumeLayout(false);
			this.bufferedPanel1.PerformLayout();
			((System.ComponentModel.ISupportInitialize)(this.sldZoomLevel)).EndInit();
			((System.ComponentModel.ISupportInitialize)(this.sldTransparency)).EndInit();
			this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.GroupBox mainFrame;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.ComboBox cmbBackgrounds;
        private BufferedPanel bufferedPanel1;
        private System.Windows.Forms.Button btnExport;
        private System.Windows.Forms.Button btnDelete;
        private System.Windows.Forms.Button btnChangeImage;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.ComboBox cmbViewType;
        private System.Windows.Forms.Label lblDummyScrollSizer;
        private System.Windows.Forms.Label lblMousePos;
		private System.Windows.Forms.TrackBar sldZoomLevel;
		private System.Windows.Forms.Label label3;
		private System.Windows.Forms.Label lblTransparency;
		private System.Windows.Forms.TrackBar sldTransparency;
    }
}
