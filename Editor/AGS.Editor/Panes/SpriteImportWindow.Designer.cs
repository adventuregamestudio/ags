namespace AGS.Editor
{
    partial class SpriteImportWindow
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

			if (disposing)
			{
				_tooltip.Dispose();
                foreach (System.Drawing.Bitmap bmp in _selectedBitmaps)
                {
					if (bmp != _image)
					{
						bmp.Dispose();
					}
                }
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(SpriteImportWindow));
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.btnImportWholeImage = new System.Windows.Forms.Button();
            this.lblHelpText = new System.Windows.Forms.Label();
            this.chkTiled = new System.Windows.Forms.CheckBox();
            this.lblMousePos = new System.Windows.Forms.Label();
            this.lblImageSize = new System.Windows.Forms.Label();
            this.cmbTransparentCol = new System.Windows.Forms.ComboBox();
            this.label1 = new System.Windows.Forms.Label();
            this.chkRoomBackground = new System.Windows.Forms.CheckBox();
            this.chkRemapCols = new System.Windows.Forms.CheckBox();
            this.zoomSlider = new System.Windows.Forms.TrackBar();
            this.lblZoom = new System.Windows.Forms.Label();
            this.btnCancel = new System.Windows.Forms.Button();
            this.previewPanel = new AGS.Editor.BufferedPanel();
            this.groupBox1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.zoomSlider)).BeginInit();
            this.SuspendLayout();
            // 
            // groupBox1
            // 
            this.groupBox1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.groupBox1.Controls.Add(this.btnImportWholeImage);
            this.groupBox1.Controls.Add(this.lblHelpText);
            this.groupBox1.Controls.Add(this.chkTiled);
            this.groupBox1.Controls.Add(this.lblMousePos);
            this.groupBox1.Controls.Add(this.lblImageSize);
            this.groupBox1.Controls.Add(this.cmbTransparentCol);
            this.groupBox1.Controls.Add(this.label1);
            this.groupBox1.Controls.Add(this.chkRoomBackground);
            this.groupBox1.Controls.Add(this.chkRemapCols);
            this.groupBox1.Location = new System.Drawing.Point(12, 3);
            this.groupBox1.MinimumSize = new System.Drawing.Size(740, 106);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(740, 106);
            this.groupBox1.TabIndex = 3;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Import options";
            // 
            // btnImportWholeImage
            // 
            this.btnImportWholeImage.Location = new System.Drawing.Point(603, 64);
            this.btnImportWholeImage.Name = "btnImportWholeImage";
            this.btnImportWholeImage.Size = new System.Drawing.Size(127, 32);
            this.btnImportWholeImage.TabIndex = 11;
            this.btnImportWholeImage.Text = "Import Whole Image";
            this.btnImportWholeImage.UseVisualStyleBackColor = true;
            this.btnImportWholeImage.Click += new System.EventHandler(this.btnImportWholeImage_Click);
            // 
            // lblHelpText
            // 
            this.lblHelpText.AutoSize = true;
            this.lblHelpText.Location = new System.Drawing.Point(321, 72);
            this.lblHelpText.MaximumSize = new System.Drawing.Size(270, 0);
            this.lblHelpText.Name = "lblHelpText";
            this.lblHelpText.Size = new System.Drawing.Size(267, 26);
            this.lblHelpText.TabIndex = 10;
            this.lblHelpText.Text = "Right-drag to determine the size of the area you wish to import; or use the Impor" +
                "t Whole Image button";
            // 
            // chkTiled
            // 
            this.chkTiled.AutoSize = true;
            this.chkTiled.Location = new System.Drawing.Point(14, 81);
            this.chkTiled.Name = "chkTiled";
            this.chkTiled.Size = new System.Drawing.Size(111, 17);
            this.chkTiled.TabIndex = 9;
            this.chkTiled.Text = "Tiled sprite import";
            this.chkTiled.UseVisualStyleBackColor = true;
            // 
            // lblMousePos
            // 
            this.lblMousePos.AutoSize = true;
            this.lblMousePos.Location = new System.Drawing.Point(11, 35);
            this.lblMousePos.Name = "lblMousePos";
            this.lblMousePos.Size = new System.Drawing.Size(82, 13);
            this.lblMousePos.TabIndex = 8;
            this.lblMousePos.Text = "Mouse location:";
            // 
            // lblImageSize
            // 
            this.lblImageSize.AutoSize = true;
            this.lblImageSize.Location = new System.Drawing.Point(11, 17);
            this.lblImageSize.Name = "lblImageSize";
            this.lblImageSize.Size = new System.Drawing.Size(62, 13);
            this.lblImageSize.TabIndex = 7;
            this.lblImageSize.Text = "Image size:";
            // 
            // cmbTransparentCol
            // 
            this.cmbTransparentCol.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cmbTransparentCol.FormattingEnabled = true;
            this.cmbTransparentCol.Items.AddRange(new object[] {
            "Palette index 0",
            "Top-left pixel",
            "Bottom-left pixel",
            "Top-right pixel",
            "Bottom-right pixel",
            "Leave as-is",
            "No transparency"});
            this.cmbTransparentCol.Location = new System.Drawing.Point(126, 54);
            this.cmbTransparentCol.Name = "cmbTransparentCol";
            this.cmbTransparentCol.Size = new System.Drawing.Size(126, 21);
            this.cmbTransparentCol.TabIndex = 6;
            this.cmbTransparentCol.SelectedIndexChanged += new System.EventHandler(this.cmbTransparentCol_SelectedIndexChanged);
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(11, 57);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(102, 13);
            this.label1.TabIndex = 5;
            this.label1.Text = "Transparent colour:";
            // 
            // chkRoomBackground
            // 
            this.chkRoomBackground.AutoSize = true;
            this.chkRoomBackground.Location = new System.Drawing.Point(324, 41);
            this.chkRoomBackground.Name = "chkRoomBackground";
            this.chkRoomBackground.Size = new System.Drawing.Size(183, 17);
            this.chkRoomBackground.TabIndex = 4;
            this.chkRoomBackground.Text = "Lock to room background palette";
            this.chkRoomBackground.UseVisualStyleBackColor = true;
            // 
            // chkRemapCols
            // 
            this.chkRemapCols.AutoSize = true;
            this.chkRemapCols.Checked = true;
            this.chkRemapCols.CheckState = System.Windows.Forms.CheckState.Checked;
            this.chkRemapCols.Location = new System.Drawing.Point(324, 17);
            this.chkRemapCols.Name = "chkRemapCols";
            this.chkRemapCols.Size = new System.Drawing.Size(175, 17);
            this.chkRemapCols.TabIndex = 3;
            this.chkRemapCols.Text = "Remap colours to game palette";
            this.chkRemapCols.UseVisualStyleBackColor = true;
            // 
            // zoomSlider
            // 
            this.zoomSlider.LargeChange = 2;
            this.zoomSlider.Location = new System.Drawing.Point(12, 124);
            this.zoomSlider.Maximum = 5;
            this.zoomSlider.Minimum = 1;
            this.zoomSlider.Name = "zoomSlider";
            this.zoomSlider.Orientation = System.Windows.Forms.Orientation.Vertical;
            this.zoomSlider.Size = new System.Drawing.Size(42, 145);
            this.zoomSlider.TabIndex = 4;
            this.zoomSlider.Value = 1;
            this.zoomSlider.Scroll += new System.EventHandler(this.zoomSlider_Scroll);
            // 
            // lblZoom
            // 
            this.lblZoom.AutoSize = true;
            this.lblZoom.Location = new System.Drawing.Point(12, 274);
            this.lblZoom.Name = "lblZoom";
            this.lblZoom.Size = new System.Drawing.Size(55, 13);
            this.lblZoom.TabIndex = 5;
            this.lblZoom.Text = "Zoom: x 1";
            // 
            // btnCancel
            // 
            this.btnCancel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnCancel.Location = new System.Drawing.Point(7, 486);
            this.btnCancel.Name = "btnCancel";
            this.btnCancel.Size = new System.Drawing.Size(60, 27);
            this.btnCancel.TabIndex = 6;
            this.btnCancel.Text = "Cancel";
            this.btnCancel.UseVisualStyleBackColor = true;
            this.btnCancel.Click += new System.EventHandler(this.btnCancel_Click);
            // 
            // previewPanel
            // 
            this.previewPanel.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.previewPanel.AutoScroll = true;
            this.previewPanel.Location = new System.Drawing.Point(73, 124);
            this.previewPanel.MinimumSize = new System.Drawing.Size(640, 320);
            this.previewPanel.Name = "previewPanel";
            this.previewPanel.Size = new System.Drawing.Size(679, 389);
            this.previewPanel.TabIndex = 0;
            this.previewPanel.Paint += new System.Windows.Forms.PaintEventHandler(this.previewPanel_Paint);
            this.previewPanel.MouseMove += new System.Windows.Forms.MouseEventHandler(this.previewPanel_MouseMove);
            this.previewPanel.Scroll += new System.Windows.Forms.ScrollEventHandler(this.previewPanel_Scroll);
            this.previewPanel.MouseDown += new System.Windows.Forms.MouseEventHandler(this.previewPanel_MouseDown);
            this.previewPanel.MouseUp += new System.Windows.Forms.MouseEventHandler(this.previewPanel_MouseUp);
            // 
            // SpriteImportWindow
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this.btnCancel;
            this.ClientSize = new System.Drawing.Size(764, 525);
            this.Controls.Add(this.btnCancel);
            this.Controls.Add(this.lblZoom);
            this.Controls.Add(this.zoomSlider);
            this.Controls.Add(this.groupBox1);
            this.Controls.Add(this.previewPanel);
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MinimizeBox = false;
            this.MinimumSize = new System.Drawing.Size(772, 552);
            this.Name = "SpriteImportWindow";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Import Sprite";
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.zoomSlider)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private BufferedPanel previewPanel;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.Label lblImageSize;
        private System.Windows.Forms.ComboBox cmbTransparentCol;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.CheckBox chkRoomBackground;
        private System.Windows.Forms.CheckBox chkRemapCols;
        private System.Windows.Forms.CheckBox chkTiled;
        private System.Windows.Forms.Label lblMousePos;
        private System.Windows.Forms.TrackBar zoomSlider;
        private System.Windows.Forms.Label lblZoom;
        private System.Windows.Forms.Button btnImportWholeImage;
        private System.Windows.Forms.Label lblHelpText;
        private System.Windows.Forms.Button btnCancel;
    }
}