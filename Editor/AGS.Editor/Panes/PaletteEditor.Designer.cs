namespace AGS.Editor
{
    partial class PaletteEditor
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
			this.tabControl = new System.Windows.Forms.TabControl();
			this.palettePage = new System.Windows.Forms.TabPage();
			this.groupBox2 = new System.Windows.Forms.GroupBox();
			this.label6 = new System.Windows.Forms.Label();
			this.palettePanel = new AGS.Editor.BufferedPanel();
			this.lblPaletteIntro = new System.Windows.Forms.Label();
			this.colourFinderPage = new System.Windows.Forms.TabPage();
			this.groupBox1 = new System.Windows.Forms.GroupBox();
			this.btnColorDialog = new System.Windows.Forms.Button();
			this.lblFixedColorsWarning = new System.Windows.Forms.Label();
			this.blockOfColour = new AGS.Editor.BufferedPanel();
			this.lblBlueVal = new System.Windows.Forms.Label();
			this.lblGreenVal = new System.Windows.Forms.Label();
			this.lblRedVal = new System.Windows.Forms.Label();
			this.label5 = new System.Windows.Forms.Label();
			this.trackBarBlue = new System.Windows.Forms.TrackBar();
			this.label4 = new System.Windows.Forms.Label();
			this.trackBarGreen = new System.Windows.Forms.TrackBar();
			this.label3 = new System.Windows.Forms.Label();
			this.trackBarRed = new System.Windows.Forms.TrackBar();
			this.txtColourNumber = new System.Windows.Forms.TextBox();
			this.label2 = new System.Windows.Forms.Label();
			this.label1 = new System.Windows.Forms.Label();
			this.tabControl.SuspendLayout();
			this.palettePage.SuspendLayout();
			this.groupBox2.SuspendLayout();
			this.colourFinderPage.SuspendLayout();
			this.groupBox1.SuspendLayout();
			((System.ComponentModel.ISupportInitialize)(this.trackBarBlue)).BeginInit();
			((System.ComponentModel.ISupportInitialize)(this.trackBarGreen)).BeginInit();
			((System.ComponentModel.ISupportInitialize)(this.trackBarRed)).BeginInit();
			this.SuspendLayout();
			// 
			// tabControl
			// 
			this.tabControl.Controls.Add(this.palettePage);
			this.tabControl.Controls.Add(this.colourFinderPage);
			this.tabControl.Location = new System.Drawing.Point(3, 3);
			this.tabControl.Name = "tabControl";
			this.tabControl.SelectedIndex = 0;
			this.tabControl.Size = new System.Drawing.Size(455, 449);
			this.tabControl.TabIndex = 2;
			// 
			// palettePage
			// 
			this.palettePage.Controls.Add(this.groupBox2);
			this.palettePage.Location = new System.Drawing.Point(4, 22);
			this.palettePage.Name = "palettePage";
			this.palettePage.Padding = new System.Windows.Forms.Padding(3);
			this.palettePage.Size = new System.Drawing.Size(447, 423);
			this.palettePage.TabIndex = 0;
			this.palettePage.Text = "Palette";
			this.palettePage.UseVisualStyleBackColor = true;
			// 
			// groupBox2
			// 
			this.groupBox2.Controls.Add(this.label6);
			this.groupBox2.Controls.Add(this.palettePanel);
			this.groupBox2.Controls.Add(this.lblPaletteIntro);
			this.groupBox2.Location = new System.Drawing.Point(6, 6);
			this.groupBox2.Name = "groupBox2";
			this.groupBox2.Size = new System.Drawing.Size(435, 411);
			this.groupBox2.TabIndex = 2;
			this.groupBox2.TabStop = false;
			this.groupBox2.Text = "Palette";
			// 
			// label6
			// 
			this.label6.AutoSize = true;
			this.label6.Location = new System.Drawing.Point(16, 47);
			this.label6.MaximumSize = new System.Drawing.Size(400, 0);
			this.label6.Name = "label6";
			this.label6.Size = new System.Drawing.Size(397, 26);
			this.label6.TabIndex = 2;
			this.label6.Text = "Click in the grid below to select a colour. Control-click to select additional co" +
				"lours; Shift-click to select a range.  Right click to import/export.";
			// 
			// palettePanel
			// 
			this.palettePanel.Location = new System.Drawing.Point(19, 85);
			this.palettePanel.Name = "palettePanel";
			this.palettePanel.Size = new System.Drawing.Size(364, 320);
			this.palettePanel.TabIndex = 1;
			this.palettePanel.MouseDown += new System.Windows.Forms.MouseEventHandler(this.palettePanel_MouseDown);
			this.palettePanel.Paint += new System.Windows.Forms.PaintEventHandler(this.palettePanel_Paint);
			// 
			// lblPaletteIntro
			// 
			this.lblPaletteIntro.AutoSize = true;
			this.lblPaletteIntro.Location = new System.Drawing.Point(16, 17);
			this.lblPaletteIntro.MaximumSize = new System.Drawing.Size(400, 0);
			this.lblPaletteIntro.Name = "lblPaletteIntro";
			this.lblPaletteIntro.Size = new System.Drawing.Size(394, 26);
			this.lblPaletteIntro.TabIndex = 0;
			this.lblPaletteIntro.Text = "This palette information will only be used for drawing any 8-bit graphics that yo" + 
				"u may have imported.";
			// 
			// colourFinderPage
			// 
			this.colourFinderPage.Controls.Add(this.groupBox1);
			this.colourFinderPage.Location = new System.Drawing.Point(4, 22);
			this.colourFinderPage.Name = "colourFinderPage";
			this.colourFinderPage.Padding = new System.Windows.Forms.Padding(3);
			this.colourFinderPage.Size = new System.Drawing.Size(447, 423);
			this.colourFinderPage.TabIndex = 1;
			this.colourFinderPage.Text = "Colour Finder";
			this.colourFinderPage.UseVisualStyleBackColor = true;
			// 
			// groupBox1
			// 
			this.groupBox1.Controls.Add(this.btnColorDialog);
			this.groupBox1.Controls.Add(this.lblFixedColorsWarning);
			this.groupBox1.Controls.Add(this.blockOfColour);
			this.groupBox1.Controls.Add(this.lblBlueVal);
			this.groupBox1.Controls.Add(this.lblGreenVal);
			this.groupBox1.Controls.Add(this.lblRedVal);
			this.groupBox1.Controls.Add(this.label5);
			this.groupBox1.Controls.Add(this.trackBarBlue);
			this.groupBox1.Controls.Add(this.label4);
			this.groupBox1.Controls.Add(this.trackBarGreen);
			this.groupBox1.Controls.Add(this.label3);
			this.groupBox1.Controls.Add(this.trackBarRed);
			this.groupBox1.Controls.Add(this.txtColourNumber);
			this.groupBox1.Controls.Add(this.label2);
			this.groupBox1.Controls.Add(this.label1);
			this.groupBox1.Location = new System.Drawing.Point(6, 6);
			this.groupBox1.Name = "groupBox1";
			this.groupBox1.Size = new System.Drawing.Size(395, 337);
			this.groupBox1.TabIndex = 1;
			this.groupBox1.TabStop = false;
			this.groupBox1.Text = "Colour Finder";
			// 
			// btnColorDialog
			// 
			this.btnColorDialog.Location = new System.Drawing.Point(201, 74);
			this.btnColorDialog.Name = "btnColorDialog";
			this.btnColorDialog.Size = new System.Drawing.Size(96, 21);
			this.btnColorDialog.TabIndex = 14;
			this.btnColorDialog.Text = "Find Colour...";
			this.btnColorDialog.UseVisualStyleBackColor = true;
			this.btnColorDialog.Click += new System.EventHandler(this.btnColorDialog_Click);
			// 
			// lblFixedColorsWarning
			// 
			this.lblFixedColorsWarning.AutoSize = true;
			this.lblFixedColorsWarning.Location = new System.Drawing.Point(16, 300);
			this.lblFixedColorsWarning.MaximumSize = new System.Drawing.Size(350, 0);
			this.lblFixedColorsWarning.Name = "lblFixedColorsWarning";
			this.lblFixedColorsWarning.Size = new System.Drawing.Size(331, 26);
			this.lblFixedColorsWarning.TabIndex = 13;
			this.lblFixedColorsWarning.Text = "NOTE: Colours 1-31 are locked to reflect special colours in the 8-bit palette. Fo" +
				"r shades of blue, set the Green slider to 4.";
			this.lblFixedColorsWarning.Visible = false;
			// 
			// blockOfColour
			// 
			this.blockOfColour.Location = new System.Drawing.Point(23, 242);
			this.blockOfColour.Name = "blockOfColour";
			this.blockOfColour.Size = new System.Drawing.Size(245, 47);
			this.blockOfColour.TabIndex = 12;
			this.blockOfColour.Paint += new System.Windows.Forms.PaintEventHandler(this.blockOfColour_Paint);
			// 
			// lblBlueVal
			// 
			this.lblBlueVal.AutoSize = true;
			this.lblBlueVal.Location = new System.Drawing.Point(278, 192);
			this.lblBlueVal.Name = "lblBlueVal";
			this.lblBlueVal.Size = new System.Drawing.Size(13, 13);
			this.lblBlueVal.TabIndex = 11;
			this.lblBlueVal.Text = "0";
			// 
			// lblGreenVal
			// 
			this.lblGreenVal.AutoSize = true;
			this.lblGreenVal.Location = new System.Drawing.Point(278, 153);
			this.lblGreenVal.Name = "lblGreenVal";
			this.lblGreenVal.Size = new System.Drawing.Size(13, 13);
			this.lblGreenVal.TabIndex = 10;
			this.lblGreenVal.Text = "0";
			// 
			// lblRedVal
			// 
			this.lblRedVal.AutoSize = true;
			this.lblRedVal.Location = new System.Drawing.Point(278, 115);
			this.lblRedVal.Name = "lblRedVal";
			this.lblRedVal.Size = new System.Drawing.Size(13, 13);
			this.lblRedVal.TabIndex = 9;
			this.lblRedVal.Text = "0";
			// 
			// label5
			// 
			this.label5.AutoSize = true;
			this.label5.Location = new System.Drawing.Point(18, 192);
			this.label5.Name = "label5";
			this.label5.Size = new System.Drawing.Size(31, 13);
			this.label5.TabIndex = 8;
			this.label5.Text = "Blue:";
			// 
			// trackBarBlue
			// 
			this.trackBarBlue.LargeChange = 40;
			this.trackBarBlue.Location = new System.Drawing.Point(61, 188);
			this.trackBarBlue.Maximum = 255;
			this.trackBarBlue.Name = "trackBarBlue";
			this.trackBarBlue.Size = new System.Drawing.Size(208, 42);
			this.trackBarBlue.SmallChange = 8;
			this.trackBarBlue.TabIndex = 7;
			this.trackBarBlue.TickFrequency = 16;
			this.trackBarBlue.Scroll += new System.EventHandler(this.trackBarBlue_Scroll);
			// 
			// label4
			// 
			this.label4.AutoSize = true;
			this.label4.Location = new System.Drawing.Point(18, 153);
			this.label4.Name = "label4";
			this.label4.Size = new System.Drawing.Size(40, 13);
			this.label4.TabIndex = 6;
			this.label4.Text = "Green:";
			// 
			// trackBarGreen
			// 
			this.trackBarGreen.LargeChange = 40;
			this.trackBarGreen.Location = new System.Drawing.Point(61, 149);
			this.trackBarGreen.Maximum = 255;
			this.trackBarGreen.Name = "trackBarGreen";
			this.trackBarGreen.Size = new System.Drawing.Size(208, 42);
			this.trackBarGreen.SmallChange = 8;
			this.trackBarGreen.TabIndex = 5;
			this.trackBarGreen.TickFrequency = 16;
			this.trackBarGreen.Scroll += new System.EventHandler(this.trackBarGreen_Scroll);
			// 
			// label3
			// 
			this.label3.AutoSize = true;
			this.label3.Location = new System.Drawing.Point(18, 115);
			this.label3.Name = "label3";
			this.label3.Size = new System.Drawing.Size(30, 13);
			this.label3.TabIndex = 4;
			this.label3.Text = "Red:";
			// 
			// trackBarRed
			// 
			this.trackBarRed.LargeChange = 40;
			this.trackBarRed.Location = new System.Drawing.Point(61, 111);
			this.trackBarRed.Maximum = 255;
			this.trackBarRed.Name = "trackBarRed";
			this.trackBarRed.Size = new System.Drawing.Size(208, 42);
			this.trackBarRed.SmallChange = 8;
			this.trackBarRed.TabIndex = 3;
			this.trackBarRed.TickFrequency = 16;
			this.trackBarRed.Scroll += new System.EventHandler(this.trackBarRed_Scroll);
			// 
			// txtColourNumber
			// 
			this.txtColourNumber.Location = new System.Drawing.Point(110, 74);
			this.txtColourNumber.MaxLength = 5;
			this.txtColourNumber.Name = "txtColourNumber";
			this.txtColourNumber.Size = new System.Drawing.Size(85, 21);
			this.txtColourNumber.TabIndex = 2;
			this.txtColourNumber.Text = "0";
			this.txtColourNumber.TextChanged += new System.EventHandler(this.txtColourNumber_TextChanged);
			// 
			// label2
			// 
			this.label2.AutoSize = true;
			this.label2.Location = new System.Drawing.Point(16, 77);
			this.label2.Name = "label2";
			this.label2.Size = new System.Drawing.Size(81, 13);
			this.label2.TabIndex = 1;
			this.label2.Text = "Colour number:";
			// 
			// label1
			// 
			this.label1.AutoSize = true;
			this.label1.Location = new System.Drawing.Point(16, 26);
			this.label1.MaximumSize = new System.Drawing.Size(350, 0);
			this.label1.Name = "label1";
			this.label1.Size = new System.Drawing.Size(335, 26);
			this.label1.TabIndex = 0;
			this.label1.Text = "You can use the controls below to find the AGS Colour Number for a particular col" +
				"our.";
			// 
			// PaletteEditor
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.Controls.Add(this.tabControl);
			this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.Name = "PaletteEditor";
			this.Size = new System.Drawing.Size(476, 452);
			this.tabControl.ResumeLayout(false);
			this.palettePage.ResumeLayout(false);
			this.groupBox2.ResumeLayout(false);
			this.groupBox2.PerformLayout();
			this.colourFinderPage.ResumeLayout(false);
			this.groupBox1.ResumeLayout(false);
			this.groupBox1.PerformLayout();
			((System.ComponentModel.ISupportInitialize)(this.trackBarBlue)).EndInit();
			((System.ComponentModel.ISupportInitialize)(this.trackBarGreen)).EndInit();
			((System.ComponentModel.ISupportInitialize)(this.trackBarRed)).EndInit();
			this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.TabControl tabControl;
        private System.Windows.Forms.TabPage palettePage;
        private System.Windows.Forms.GroupBox groupBox2;
        private System.Windows.Forms.Label lblPaletteIntro;
        private System.Windows.Forms.TabPage colourFinderPage;
        private System.Windows.Forms.GroupBox groupBox1;
        private BufferedPanel blockOfColour;
        private System.Windows.Forms.Label lblBlueVal;
        private System.Windows.Forms.Label lblGreenVal;
        private System.Windows.Forms.Label lblRedVal;
        private System.Windows.Forms.Label label5;
        private System.Windows.Forms.TrackBar trackBarBlue;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.TrackBar trackBarGreen;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.TrackBar trackBarRed;
        private System.Windows.Forms.TextBox txtColourNumber;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label1;
        private BufferedPanel palettePanel;
        private System.Windows.Forms.Label label6;
        private System.Windows.Forms.Label lblFixedColorsWarning;
		private System.Windows.Forms.Button btnColorDialog;

    }
}
