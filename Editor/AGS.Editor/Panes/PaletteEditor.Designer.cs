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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(PaletteEditor));
            this.tabControl = new System.Windows.Forms.TabControl();
            this.palettePage = new System.Windows.Forms.TabPage();
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.label6 = new System.Windows.Forms.Label();
            this.palettePanel = new AGS.Editor.BufferedPanel();
            this.lblPaletteIntro = new System.Windows.Forms.Label();
            this.colourFinderPage = new System.Windows.Forms.TabPage();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.panel2 = new System.Windows.Forms.Panel();
            this.txtRGBColor = new System.Windows.Forms.TextBox();
            this.label7 = new System.Windows.Forms.Label();
            this.lblBlueFinal = new System.Windows.Forms.Label();
            this.lblGreenFinal = new System.Windows.Forms.Label();
            this.lblRedFinal = new System.Windows.Forms.Label();
            this.btnColorDialog = new System.Windows.Forms.Button();
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
            this.panel1 = new System.Windows.Forms.Panel();
            this.label1 = new System.Windows.Forms.Label();
            this.tabControl.SuspendLayout();
            this.palettePage.SuspendLayout();
            this.groupBox2.SuspendLayout();
            this.colourFinderPage.SuspendLayout();
            this.groupBox1.SuspendLayout();
            this.panel2.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.trackBarBlue)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBarGreen)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBarRed)).BeginInit();
            this.panel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // tabControl
            // 
            this.tabControl.Controls.Add(this.palettePage);
            this.tabControl.Controls.Add(this.colourFinderPage);
            this.tabControl.Location = new System.Drawing.Point(3, 3);
            this.tabControl.Name = "tabControl";
            this.tabControl.SelectedIndex = 0;
            this.tabControl.Size = new System.Drawing.Size(455, 487);
            this.tabControl.TabIndex = 2;
            // 
            // palettePage
            // 
            this.palettePage.Controls.Add(this.groupBox2);
            this.palettePage.Location = new System.Drawing.Point(4, 22);
            this.palettePage.Name = "palettePage";
            this.palettePage.Padding = new System.Windows.Forms.Padding(3);
            this.palettePage.Size = new System.Drawing.Size(447, 461);
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
            this.palettePanel.Paint += new System.Windows.Forms.PaintEventHandler(this.palettePanel_Paint);
            this.palettePanel.MouseDown += new System.Windows.Forms.MouseEventHandler(this.palettePanel_MouseDown);
            // 
            // lblPaletteIntro
            // 
            this.lblPaletteIntro.AutoSize = true;
            this.lblPaletteIntro.Location = new System.Drawing.Point(16, 17);
            this.lblPaletteIntro.MaximumSize = new System.Drawing.Size(400, 0);
            this.lblPaletteIntro.Name = "lblPaletteIntro";
            this.lblPaletteIntro.Size = new System.Drawing.Size(395, 26);
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
            this.colourFinderPage.Size = new System.Drawing.Size(447, 461);
            this.colourFinderPage.TabIndex = 1;
            this.colourFinderPage.Text = "Colour Finder";
            this.colourFinderPage.UseVisualStyleBackColor = true;
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.panel2);
            this.groupBox1.Controls.Add(this.panel1);
            this.groupBox1.Location = new System.Drawing.Point(6, 6);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(435, 449);
            this.groupBox1.TabIndex = 1;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Colour Finder";
            // 
            // panel2
            // 
            this.panel2.Controls.Add(this.txtRGBColor);
            this.panel2.Controls.Add(this.label7);
            this.panel2.Controls.Add(this.lblBlueFinal);
            this.panel2.Controls.Add(this.lblGreenFinal);
            this.panel2.Controls.Add(this.lblRedFinal);
            this.panel2.Controls.Add(this.btnColorDialog);
            this.panel2.Controls.Add(this.blockOfColour);
            this.panel2.Controls.Add(this.lblBlueVal);
            this.panel2.Controls.Add(this.lblGreenVal);
            this.panel2.Controls.Add(this.lblRedVal);
            this.panel2.Controls.Add(this.label5);
            this.panel2.Controls.Add(this.trackBarBlue);
            this.panel2.Controls.Add(this.label4);
            this.panel2.Controls.Add(this.trackBarGreen);
            this.panel2.Controls.Add(this.label3);
            this.panel2.Controls.Add(this.trackBarRed);
            this.panel2.Controls.Add(this.txtColourNumber);
            this.panel2.Controls.Add(this.label2);
            this.panel2.Location = new System.Drawing.Point(6, 183);
            this.panel2.Name = "panel2";
            this.panel2.Size = new System.Drawing.Size(423, 247);
            this.panel2.TabIndex = 19;
            // 
            // txtRGBColor
            // 
            this.txtRGBColor.Location = new System.Drawing.Point(103, 30);
            this.txtRGBColor.MaxLength = 0;
            this.txtRGBColor.Name = "txtRGBColor";
            this.txtRGBColor.Size = new System.Drawing.Size(99, 21);
            this.txtRGBColor.TabIndex = 49;
            this.txtRGBColor.Text = "0";
            this.txtRGBColor.TextChanged += new System.EventHandler(this.txtRGBColor_TextChanged);
            // 
            // label7
            // 
            this.label7.AutoSize = true;
            this.label7.Location = new System.Drawing.Point(9, 33);
            this.label7.Name = "label7";
            this.label7.Size = new System.Drawing.Size(53, 13);
            this.label7.TabIndex = 48;
            this.label7.Text = "(R, G, B):";
            // 
            // lblBlueFinal
            // 
            this.lblBlueFinal.AutoSize = true;
            this.lblBlueFinal.Location = new System.Drawing.Point(308, 145);
            this.lblBlueFinal.Name = "lblBlueFinal";
            this.lblBlueFinal.Size = new System.Drawing.Size(21, 13);
            this.lblBlueFinal.TabIndex = 47;
            this.lblBlueFinal.Text = "(0)";
            // 
            // lblGreenFinal
            // 
            this.lblGreenFinal.AutoSize = true;
            this.lblGreenFinal.Location = new System.Drawing.Point(308, 106);
            this.lblGreenFinal.Name = "lblGreenFinal";
            this.lblGreenFinal.Size = new System.Drawing.Size(21, 13);
            this.lblGreenFinal.TabIndex = 46;
            this.lblGreenFinal.Text = "(0)";
            // 
            // lblRedFinal
            // 
            this.lblRedFinal.AutoSize = true;
            this.lblRedFinal.Location = new System.Drawing.Point(308, 68);
            this.lblRedFinal.Name = "lblRedFinal";
            this.lblRedFinal.Size = new System.Drawing.Size(21, 13);
            this.lblRedFinal.TabIndex = 45;
            this.lblRedFinal.Text = "(0)";
            // 
            // btnColorDialog
            // 
            this.btnColorDialog.Location = new System.Drawing.Point(208, 5);
            this.btnColorDialog.Name = "btnColorDialog";
            this.btnColorDialog.Size = new System.Drawing.Size(96, 21);
            this.btnColorDialog.TabIndex = 44;
            this.btnColorDialog.Text = "Find Colour...";
            this.btnColorDialog.UseVisualStyleBackColor = true;
            this.btnColorDialog.Click += new System.EventHandler(this.btnColorDialog_Click);
            // 
            // blockOfColour
            // 
            this.blockOfColour.Location = new System.Drawing.Point(12, 189);
            this.blockOfColour.Name = "blockOfColour";
            this.blockOfColour.Size = new System.Drawing.Size(398, 47);
            this.blockOfColour.TabIndex = 43;
            this.blockOfColour.Paint += new System.Windows.Forms.PaintEventHandler(this.blockOfColour_Paint);
            // 
            // lblBlueVal
            // 
            this.lblBlueVal.AutoSize = true;
            this.lblBlueVal.Location = new System.Drawing.Point(271, 145);
            this.lblBlueVal.Name = "lblBlueVal";
            this.lblBlueVal.Size = new System.Drawing.Size(13, 13);
            this.lblBlueVal.TabIndex = 42;
            this.lblBlueVal.Text = "0";
            // 
            // lblGreenVal
            // 
            this.lblGreenVal.AutoSize = true;
            this.lblGreenVal.Location = new System.Drawing.Point(271, 106);
            this.lblGreenVal.Name = "lblGreenVal";
            this.lblGreenVal.Size = new System.Drawing.Size(13, 13);
            this.lblGreenVal.TabIndex = 41;
            this.lblGreenVal.Text = "0";
            // 
            // lblRedVal
            // 
            this.lblRedVal.AutoSize = true;
            this.lblRedVal.Location = new System.Drawing.Point(271, 68);
            this.lblRedVal.Name = "lblRedVal";
            this.lblRedVal.Size = new System.Drawing.Size(13, 13);
            this.lblRedVal.TabIndex = 40;
            this.lblRedVal.Text = "0";
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.Location = new System.Drawing.Point(11, 145);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(31, 13);
            this.label5.TabIndex = 39;
            this.label5.Text = "Blue:";
            // 
            // trackBarBlue
            // 
            this.trackBarBlue.LargeChange = 40;
            this.trackBarBlue.Location = new System.Drawing.Point(54, 141);
            this.trackBarBlue.Maximum = 255;
            this.trackBarBlue.Name = "trackBarBlue";
            this.trackBarBlue.Size = new System.Drawing.Size(208, 42);
            this.trackBarBlue.SmallChange = 8;
            this.trackBarBlue.TabIndex = 38;
            this.trackBarBlue.TickFrequency = 16;
            this.trackBarBlue.Scroll += new System.EventHandler(this.trackBarBlue_Scroll);
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(11, 106);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(40, 13);
            this.label4.TabIndex = 37;
            this.label4.Text = "Green:";
            // 
            // trackBarGreen
            // 
            this.trackBarGreen.LargeChange = 40;
            this.trackBarGreen.Location = new System.Drawing.Point(54, 102);
            this.trackBarGreen.Maximum = 255;
            this.trackBarGreen.Name = "trackBarGreen";
            this.trackBarGreen.Size = new System.Drawing.Size(208, 42);
            this.trackBarGreen.SmallChange = 8;
            this.trackBarGreen.TabIndex = 36;
            this.trackBarGreen.TickFrequency = 16;
            this.trackBarGreen.Scroll += new System.EventHandler(this.trackBarGreen_Scroll);
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(11, 68);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(30, 13);
            this.label3.TabIndex = 35;
            this.label3.Text = "Red:";
            // 
            // trackBarRed
            // 
            this.trackBarRed.LargeChange = 40;
            this.trackBarRed.Location = new System.Drawing.Point(54, 64);
            this.trackBarRed.Maximum = 255;
            this.trackBarRed.Name = "trackBarRed";
            this.trackBarRed.Size = new System.Drawing.Size(208, 42);
            this.trackBarRed.SmallChange = 8;
            this.trackBarRed.TabIndex = 34;
            this.trackBarRed.TickFrequency = 16;
            this.trackBarRed.Scroll += new System.EventHandler(this.trackBarRed_Scroll);
            // 
            // txtColourNumber
            // 
            this.txtColourNumber.Location = new System.Drawing.Point(103, 5);
            this.txtColourNumber.MaxLength = 5;
            this.txtColourNumber.Name = "txtColourNumber";
            this.txtColourNumber.Size = new System.Drawing.Size(99, 21);
            this.txtColourNumber.TabIndex = 33;
            this.txtColourNumber.Text = "0";
            this.txtColourNumber.TextChanged += new System.EventHandler(this.txtColourNumber_TextChanged);
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(9, 8);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(81, 13);
            this.label2.TabIndex = 32;
            this.label2.Text = "Colour number:";
            // 
            // panel1
            // 
            this.panel1.Controls.Add(this.label1);
            this.panel1.Location = new System.Drawing.Point(6, 18);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(423, 159);
            this.panel1.TabIndex = 18;
            // 
            // label1
            // 
            this.label1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.label1.Location = new System.Drawing.Point(0, 0);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(423, 159);
            this.label1.TabIndex = 0;
            this.label1.Text = resources.GetString("label1.Text");
            // 
            // PaletteEditor
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.tabControl);
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Name = "PaletteEditor";
            this.Size = new System.Drawing.Size(476, 493);
            this.Load += new System.EventHandler(this.PaletteEditor_Load);
            this.tabControl.ResumeLayout(false);
            this.palettePage.ResumeLayout(false);
            this.groupBox2.ResumeLayout(false);
            this.groupBox2.PerformLayout();
            this.colourFinderPage.ResumeLayout(false);
            this.groupBox1.ResumeLayout(false);
            this.panel2.ResumeLayout(false);
            this.panel2.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.trackBarBlue)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBarGreen)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBarRed)).EndInit();
            this.panel1.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.TabControl tabControl;
        private System.Windows.Forms.TabPage palettePage;
        private System.Windows.Forms.GroupBox groupBox2;
        private System.Windows.Forms.Label lblPaletteIntro;
        private System.Windows.Forms.TabPage colourFinderPage;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.Label label1;
        private BufferedPanel palettePanel;
        private System.Windows.Forms.Label label6;
        private System.Windows.Forms.Panel panel1;
        private System.Windows.Forms.Panel panel2;
        private System.Windows.Forms.TextBox txtRGBColor;
        private System.Windows.Forms.Label label7;
        private System.Windows.Forms.Label lblBlueFinal;
        private System.Windows.Forms.Label lblGreenFinal;
        private System.Windows.Forms.Label lblRedFinal;
        private System.Windows.Forms.Button btnColorDialog;
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
    }
}
