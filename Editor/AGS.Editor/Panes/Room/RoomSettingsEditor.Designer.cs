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
            this.splitWindow = new System.Windows.Forms.SplitContainer();
            this._layerTreeView = new AGS.Editor.Panes.Room.RoomLayerTreeView();
            this.mainFrame = new System.Windows.Forms.GroupBox();
            this.chkObjectOffset = new System.Windows.Forms.CheckBox();
            this.lblMouse = new System.Windows.Forms.Label();
            this.sldTransparency = new System.Windows.Forms.TrackBar();
            this.lblTransparency = new System.Windows.Forms.Label();
            this.lblZoomInfo = new System.Windows.Forms.Label();
            this.sldZoomLevel = new System.Windows.Forms.TrackBar();
            this.label3 = new System.Windows.Forms.Label();
            this.lblMousePos = new System.Windows.Forms.Label();
            this.btnExport = new System.Windows.Forms.Button();
            this.btnDelete = new System.Windows.Forms.Button();
            this.btnChangeImage = new System.Windows.Forms.Button();
            this.label1 = new System.Windows.Forms.Label();
            this.cmbBackgrounds = new System.Windows.Forms.ComboBox();
            this.bufferedPanel1 = new AGS.Editor.FreePanControl();
            ((System.ComponentModel.ISupportInitialize)(this.splitWindow)).BeginInit();
            this.splitWindow.Panel1.SuspendLayout();
            this.splitWindow.Panel2.SuspendLayout();
            this.splitWindow.SuspendLayout();
            this.mainFrame.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.sldTransparency)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.sldZoomLevel)).BeginInit();
            this.SuspendLayout();
            //
            // splitWindow
            //
            this.splitWindow.Dock = System.Windows.Forms.DockStyle.Fill;
            this.splitWindow.FixedPanel = System.Windows.Forms.FixedPanel.Panel1;
            this.splitWindow.Location = new System.Drawing.Point(0, 0);
            this.splitWindow.Name = "splitWindow";
            //
            // splitWindow.Panel1
            //
            this.splitWindow.Panel1.Controls.Add(this._layerTreeView);
            this.splitWindow.Panel1MinSize = 120;
            //
            // splitWindow.Panel2
            //
            this.splitWindow.Panel2.Controls.Add(this.mainFrame);
            this.splitWindow.Size = new System.Drawing.Size(968, 491);
            this.splitWindow.SplitterDistance = 200;
            this.splitWindow.TabIndex = 0;
            //
            // _layerTreeView
            //
            this._layerTreeView.Dock = System.Windows.Forms.DockStyle.Fill;
            this._layerTreeView.Location = new System.Drawing.Point(0, 0);
            this._layerTreeView.Name = "_layerTreeView";
            this._layerTreeView.Size = new System.Drawing.Size(200, 491);
            this._layerTreeView.TabIndex = 0;
            //
            // mainFrame
            //
            this.mainFrame.Controls.Add(this.chkObjectOffset);
            this.mainFrame.Controls.Add(this.lblMouse);
            this.mainFrame.Controls.Add(this.sldTransparency);
            this.mainFrame.Controls.Add(this.lblTransparency);
            this.mainFrame.Controls.Add(this.lblZoomInfo);
            this.mainFrame.Controls.Add(this.sldZoomLevel);
            this.mainFrame.Controls.Add(this.label3);
            this.mainFrame.Controls.Add(this.lblMousePos);
            this.mainFrame.Controls.Add(this.btnExport);
            this.mainFrame.Controls.Add(this.btnDelete);
            this.mainFrame.Controls.Add(this.btnChangeImage);
            this.mainFrame.Controls.Add(this.label1);
            this.mainFrame.Controls.Add(this.cmbBackgrounds);
            this.mainFrame.Controls.Add(this.bufferedPanel1);
            this.mainFrame.Dock = System.Windows.Forms.DockStyle.Fill;
            this.mainFrame.Location = new System.Drawing.Point(0, 0);
            this.mainFrame.Name = "mainFrame";
            this.mainFrame.Size = new System.Drawing.Size(764, 491);
            this.mainFrame.TabIndex = 4;
            this.mainFrame.TabStop = false;
            this.mainFrame.Text = "Room details";
            // 
            // chkObjectOffset
            // 
            this.chkObjectOffset.AutoSize = true;
            this.chkObjectOffset.Location = new System.Drawing.Point(308, 52);
            this.chkObjectOffset.Name = "chkObjectOffset";
            this.chkObjectOffset.Size = new System.Drawing.Size(171, 17);
            this.chkObjectOffset.TabIndex = 21;
            this.chkObjectOffset.Text = "Move Objects By Anchor Point";
            this.chkObjectOffset.UseVisualStyleBackColor = true;
            this.chkObjectOffset.Visible = false;
            this.chkObjectOffset.CheckedChanged += new System.EventHandler(this.chkObjectOffset_CheckedChanged);
            // 
            // lblMouse
            // 
            this.lblMouse.AutoSize = true;
            this.lblMouse.Location = new System.Drawing.Point(16, 53);
            this.lblMouse.Name = "lblMouse";
            this.lblMouse.Size = new System.Drawing.Size(77, 13);
            this.lblMouse.TabIndex = 20;
            this.lblMouse.Text = "Mouse coords:";
            // 
            // sldTransparency
            // 
            this.sldTransparency.AutoSize = false;
            this.sldTransparency.LargeChange = 20;
            this.sldTransparency.Location = new System.Drawing.Point(558, 48);
            this.sldTransparency.Margin = new System.Windows.Forms.Padding(1);
            this.sldTransparency.Maximum = 100;
            this.sldTransparency.Name = "sldTransparency";
            this.sldTransparency.Size = new System.Drawing.Size(91, 26);
            this.sldTransparency.SmallChange = 5;
            this.sldTransparency.TabIndex = 14;
            this.sldTransparency.TickFrequency = 20;
            this.sldTransparency.TickStyle = System.Windows.Forms.TickStyle.None;
            this.sldTransparency.Value = 70;
            this.sldTransparency.Visible = false;
            this.sldTransparency.Scroll += new System.EventHandler(this.sldTransparency_Scroll);
            // 
            // lblTransparency
            // 
            this.lblTransparency.AutoSize = true;
            this.lblTransparency.Location = new System.Drawing.Point(523, 53);
            this.lblTransparency.Name = "lblTransparency";
            this.lblTransparency.Size = new System.Drawing.Size(38, 13);
            this.lblTransparency.TabIndex = 15;
            this.lblTransparency.Text = "Trans:";
            this.lblTransparency.Visible = false;
            // 
            // lblZoomInfo
            // 
            this.lblZoomInfo.AutoSize = true;
            this.lblZoomInfo.Location = new System.Drawing.Point(646, 21);
            this.lblZoomInfo.Name = "lblZoomInfo";
            this.lblZoomInfo.Size = new System.Drawing.Size(36, 13);
            this.lblZoomInfo.TabIndex = 19;
            this.lblZoomInfo.Text = "100%";
            //
            // sldZoomLevel
            // 
            this.sldZoomLevel.LargeChange = 1;
            this.sldZoomLevel.Location = new System.Drawing.Point(558, 18);
            this.sldZoomLevel.Maximum = 6;
            this.sldZoomLevel.Minimum = 1;
            this.sldZoomLevel.Name = "sldZoomLevel";
            this.sldZoomLevel.Size = new System.Drawing.Size(91, 45);
            this.sldZoomLevel.TabIndex = 12;
            this.sldZoomLevel.Value = 1;
            this.sldZoomLevel.Scroll += new System.EventHandler(this.sldZoomLevel_Scroll);
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(524, 21);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(37, 13);
            this.label3.TabIndex = 13;
            this.label3.Text = "Zoom:";
            // 
            // lblMousePos
            // 
            this.lblMousePos.AutoSize = true;
            this.lblMousePos.Location = new System.Drawing.Point(95, 53);
            this.lblMousePos.Name = "lblMousePos";
            this.lblMousePos.Size = new System.Drawing.Size(24, 13);
            this.lblMousePos.TabIndex = 11;
            this.lblMousePos.Text = "?, ?";
            //
            // btnExport
            // 
            this.btnExport.Location = new System.Drawing.Point(433, 16);
            this.btnExport.Name = "btnExport";
            this.btnExport.Size = new System.Drawing.Size(84, 23);
            this.btnExport.TabIndex = 8;
            this.btnExport.Text = "Export...";
            this.btnExport.UseVisualStyleBackColor = true;
            this.btnExport.Click += new System.EventHandler(this.btnExport_Click);
            // 
            // btnDelete
            // 
            this.btnDelete.Location = new System.Drawing.Point(343, 16);
            this.btnDelete.Name = "btnDelete";
            this.btnDelete.Size = new System.Drawing.Size(84, 23);
            this.btnDelete.TabIndex = 7;
            this.btnDelete.Text = "Delete";
            this.btnDelete.UseVisualStyleBackColor = true;
            this.btnDelete.Click += new System.EventHandler(this.btnDelete_Click);
            // 
            // btnChangeImage
            // 
            this.btnChangeImage.Location = new System.Drawing.Point(253, 16);
            this.btnChangeImage.Name = "btnChangeImage";
            this.btnChangeImage.Size = new System.Drawing.Size(84, 23);
            this.btnChangeImage.TabIndex = 3;
            this.btnChangeImage.Text = "Change...";
            this.btnChangeImage.UseVisualStyleBackColor = true;
            this.btnChangeImage.Click += new System.EventHandler(this.btnChangeImage_Click);
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(25, 21);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(67, 13);
            this.label1.TabIndex = 5;
            this.label1.Text = "Background:";
            // 
            // cmbBackgrounds
            // 
            this.cmbBackgrounds.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cmbBackgrounds.FormattingEnabled = true;
            this.cmbBackgrounds.Location = new System.Drawing.Point(98, 17);
            this.cmbBackgrounds.Name = "cmbBackgrounds";
            this.cmbBackgrounds.Size = new System.Drawing.Size(148, 21);
            this.cmbBackgrounds.TabIndex = 4;
            this.cmbBackgrounds.SelectedIndexChanged += new System.EventHandler(this.cmbBackgrounds_SelectedIndexChanged);
            // 
            // bufferedPanel1
            // 
            this.bufferedPanel1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.bufferedPanel1.AutoScroll = true;
            this.bufferedPanel1.BackColor = System.Drawing.SystemColors.Control;
            this.bufferedPanel1.Location = new System.Drawing.Point(12, 76);
            this.bufferedPanel1.Name = "bufferedPanel1";
            this.bufferedPanel1.PanButtons = System.Windows.Forms.MouseButtons.Middle;
            this.bufferedPanel1.Size = new System.Drawing.Size(740, 403);
            this.bufferedPanel1.TabIndex = 1;
            this.bufferedPanel1.TabStop = true;
            this.bufferedPanel1.Paint += new System.Windows.Forms.PaintEventHandler(this.bufferedPanel1_Paint);
            this.bufferedPanel1.DoubleClick += new System.EventHandler(this.bufferedPanel1_DoubleClick);
            this.bufferedPanel1.MouseDown += new System.Windows.Forms.MouseEventHandler(this.bufferedPanel1_MouseDown);
            this.bufferedPanel1.MouseMove += new System.Windows.Forms.MouseEventHandler(this.bufferedPanel1_MouseMove);
            this.bufferedPanel1.MouseUp += new System.Windows.Forms.MouseEventHandler(this.bufferedPanel1_MouseUp);
            // 
            // RoomSettingsEditor
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(96F, 96F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
            this.Controls.Add(this.splitWindow);
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Name = "RoomSettingsEditor";
            this.Size = new System.Drawing.Size(968, 491);
            this.Load += new System.EventHandler(this.RoomSettingsEditor_Load);
            this.splitWindow.Panel1.ResumeLayout(false);
            this.splitWindow.Panel2.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.splitWindow)).EndInit();
            this.splitWindow.ResumeLayout(false);
            this.mainFrame.ResumeLayout(false);
            this.mainFrame.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.sldTransparency)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.sldZoomLevel)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.SplitContainer splitWindow;
        private AGS.Editor.Panes.Room.RoomLayerTreeView _layerTreeView;
        private System.Windows.Forms.GroupBox mainFrame;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.ComboBox cmbBackgrounds;
        private FreePanControl bufferedPanel1;
        private System.Windows.Forms.Button btnExport;
        private System.Windows.Forms.Button btnDelete;
        private System.Windows.Forms.Button btnChangeImage;
        private System.Windows.Forms.Label lblMousePos;
		private System.Windows.Forms.TrackBar sldZoomLevel;
		private System.Windows.Forms.Label label3;
		private System.Windows.Forms.Label lblTransparency;
		private System.Windows.Forms.TrackBar sldTransparency;
        private System.Windows.Forms.Label lblZoomInfo;
        private System.Windows.Forms.Label lblMouse;
        private System.Windows.Forms.CheckBox chkObjectOffset;
    }
}
