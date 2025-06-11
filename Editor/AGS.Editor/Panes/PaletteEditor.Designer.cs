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
            this.lblPaletteIntro = new System.Windows.Forms.Label();
            this.colourFinderPage = new System.Windows.Forms.TabPage();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.panel1 = new System.Windows.Forms.Panel();
            this.label1 = new System.Windows.Forms.Label();
            this.panel2 = new System.Windows.Forms.Panel();
            this.txtLegacyColourNumber = new System.Windows.Forms.TextBox();
            this.label7 = new System.Windows.Forms.Label();
            this.label5 = new System.Windows.Forms.Label();
            this.txtCommaSeparated = new System.Windows.Forms.TextBox();
            this.label4 = new System.Windows.Forms.Label();
            this.txtWebColor = new System.Windows.Forms.TextBox();
            this.label3 = new System.Windows.Forms.Label();
            this.btnColorDialog = new System.Windows.Forms.Button();
            this.txtColourNumber = new System.Windows.Forms.TextBox();
            this.label2 = new System.Windows.Forms.Label();
            this.palettePanel = new AGS.Editor.BufferedPanel();
            this.blockOfColour = new AGS.Editor.BufferedPanel();
            this.tabControl.SuspendLayout();
            this.palettePage.SuspendLayout();
            this.groupBox2.SuspendLayout();
            this.colourFinderPage.SuspendLayout();
            this.groupBox1.SuspendLayout();
            this.panel1.SuspendLayout();
            this.panel2.SuspendLayout();
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
            this.colourFinderPage.Size = new System.Drawing.Size(447, 423);
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
            this.groupBox1.Size = new System.Drawing.Size(435, 411);
            this.groupBox1.TabIndex = 1;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Colour Finder";
            // 
            // panel1
            // 
            this.panel1.Controls.Add(this.label1);
            this.panel1.Location = new System.Drawing.Point(6, 20);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(420, 145);
            this.panel1.TabIndex = 22;
            // 
            // label1
            // 
            this.label1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.label1.Location = new System.Drawing.Point(0, 0);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(420, 145);
            this.label1.TabIndex = 1;
            this.label1.Text = resources.GetString("label1.Text");
            // 
            // panel2
            // 
            this.panel2.Controls.Add(this.txtLegacyColourNumber);
            this.panel2.Controls.Add(this.label7);
            this.panel2.Controls.Add(this.label5);
            this.panel2.Controls.Add(this.txtCommaSeparated);
            this.panel2.Controls.Add(this.label4);
            this.panel2.Controls.Add(this.txtWebColor);
            this.panel2.Controls.Add(this.label3);
            this.panel2.Controls.Add(this.btnColorDialog);
            this.panel2.Controls.Add(this.blockOfColour);
            this.panel2.Controls.Add(this.txtColourNumber);
            this.panel2.Controls.Add(this.label2);
            this.panel2.Location = new System.Drawing.Point(6, 168);
            this.panel2.Name = "panel2";
            this.panel2.Size = new System.Drawing.Size(420, 237);
            this.panel2.TabIndex = 23;
            // 
            // txtLegacyColourNumber
            // 
            this.txtLegacyColourNumber.Location = new System.Drawing.Point(207, 90);
            this.txtLegacyColourNumber.MaxLength = 0;
            this.txtLegacyColourNumber.Name = "txtLegacyColourNumber";
            this.txtLegacyColourNumber.Size = new System.Drawing.Size(99, 21);
            this.txtLegacyColourNumber.TabIndex = 32;
            this.txtLegacyColourNumber.Text = "0";
            this.txtLegacyColourNumber.TextChanged += new System.EventHandler(this.txtLegacyColourNumber_TextChanged);
            // 
            // label7
            // 
            this.label7.AutoSize = true;
            this.label7.Location = new System.Drawing.Point(10, 94);
            this.label7.Name = "label7";
            this.label7.Size = new System.Drawing.Size(168, 13);
            this.label7.TabIndex = 31;
            this.label7.Text = "Legacy (AGS 3.*) Colour number:\r\n";
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.Location = new System.Drawing.Point(188, 40);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(15, 13);
            this.label5.TabIndex = 30;
            this.label5.Text = "#";
            // 
            // txtCommaSeparated
            // 
            this.txtCommaSeparated.Location = new System.Drawing.Point(207, 63);
            this.txtCommaSeparated.MaxLength = 0;
            this.txtCommaSeparated.Name = "txtCommaSeparated";
            this.txtCommaSeparated.Size = new System.Drawing.Size(99, 21);
            this.txtCommaSeparated.TabIndex = 29;
            this.txtCommaSeparated.Text = "0";
            this.txtCommaSeparated.TextChanged += new System.EventHandler(this.txtCommaSeparated_TextChanged);
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(10, 67);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(157, 13);
            this.label4.TabIndex = 28;
            this.label4.Text = "Comma separated (R, G, B, A):";
            // 
            // txtWebColor
            // 
            this.txtWebColor.Location = new System.Drawing.Point(207, 36);
            this.txtWebColor.MaxLength = 8;
            this.txtWebColor.Name = "txtWebColor";
            this.txtWebColor.Size = new System.Drawing.Size(99, 21);
            this.txtWebColor.TabIndex = 27;
            this.txtWebColor.Text = "0";
            this.txtWebColor.TextChanged += new System.EventHandler(this.txtWebColor_TextChanged);
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(10, 40);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(124, 13);
            this.label3.TabIndex = 26;
            this.label3.Text = "Web color (RRGGBBAA):";
            // 
            // btnColorDialog
            // 
            this.btnColorDialog.Location = new System.Drawing.Point(312, 10);
            this.btnColorDialog.Name = "btnColorDialog";
            this.btnColorDialog.Size = new System.Drawing.Size(96, 21);
            this.btnColorDialog.TabIndex = 25;
            this.btnColorDialog.Text = "Find Colour...";
            this.btnColorDialog.UseVisualStyleBackColor = true;
            this.btnColorDialog.Click += new System.EventHandler(this.btnColorDialog_Click);
            // 
            // txtColourNumber
            // 
            this.txtColourNumber.Location = new System.Drawing.Point(207, 10);
            this.txtColourNumber.MaxLength = 10;
            this.txtColourNumber.Name = "txtColourNumber";
            this.txtColourNumber.Size = new System.Drawing.Size(99, 21);
            this.txtColourNumber.TabIndex = 23;
            this.txtColourNumber.Text = "0";
            this.txtColourNumber.TextChanged += new System.EventHandler(this.txtColourNumber_TextChanged);
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(10, 14);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(158, 13);
            this.label2.TabIndex = 22;
            this.label2.Text = "Colour number (0xAARRGGBB):";
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
            // blockOfColour
            // 
            this.blockOfColour.Location = new System.Drawing.Point(13, 131);
            this.blockOfColour.Name = "blockOfColour";
            this.blockOfColour.Size = new System.Drawing.Size(395, 47);
            this.blockOfColour.TabIndex = 24;
            this.blockOfColour.Paint += new System.Windows.Forms.PaintEventHandler(this.blockOfColour_Paint);
            // 
            // PaletteEditor
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.tabControl);
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Name = "PaletteEditor";
            this.Size = new System.Drawing.Size(476, 452);
            this.Load += new System.EventHandler(this.PaletteEditor_Load);
            this.tabControl.ResumeLayout(false);
            this.palettePage.ResumeLayout(false);
            this.groupBox2.ResumeLayout(false);
            this.groupBox2.PerformLayout();
            this.colourFinderPage.ResumeLayout(false);
            this.groupBox1.ResumeLayout(false);
            this.panel1.ResumeLayout(false);
            this.panel2.ResumeLayout(false);
            this.panel2.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.TabControl tabControl;
        private System.Windows.Forms.TabPage palettePage;
        private System.Windows.Forms.GroupBox groupBox2;
        private System.Windows.Forms.Label lblPaletteIntro;
        private System.Windows.Forms.TabPage colourFinderPage;
        private System.Windows.Forms.GroupBox groupBox1;
        private BufferedPanel palettePanel;
        private System.Windows.Forms.Label label6;
        private System.Windows.Forms.Panel panel2;
        private System.Windows.Forms.TextBox txtLegacyColourNumber;
        private System.Windows.Forms.Label label7;
        private System.Windows.Forms.Label label5;
        private System.Windows.Forms.TextBox txtCommaSeparated;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.TextBox txtWebColor;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Button btnColorDialog;
        private BufferedPanel blockOfColour;
        private System.Windows.Forms.TextBox txtColourNumber;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Panel panel1;
        private System.Windows.Forms.Label label1;
    }
}
