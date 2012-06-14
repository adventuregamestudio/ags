namespace AGS.Editor
{
    partial class PreferencesEditor
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

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.btnOK = new System.Windows.Forms.Button();
            this.btnCancel = new System.Windows.Forms.Button();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.label4 = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.cmbTestGameStyle = new System.Windows.Forms.ComboBox();
            this.label1 = new System.Windows.Forms.Label();
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.udTabWidth = new System.Windows.Forms.NumericUpDown();
            this.cmbIndentStyle = new System.Windows.Forms.ComboBox();
            this.label10 = new System.Windows.Forms.Label();
            this.label8 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.groupBox3 = new System.Windows.Forms.GroupBox();
            this.chkKeepHelpOnTop = new System.Windows.Forms.CheckBox();
            this.chkAlwaysShowViewPreview = new System.Windows.Forms.CheckBox();
            this.cmbMessageOnCompile = new System.Windows.Forms.ComboBox();
            this.label9 = new System.Windows.Forms.Label();
            this.cmbEditorStartup = new System.Windows.Forms.ComboBox();
            this.label5 = new System.Windows.Forms.Label();
            this.groupBox4 = new System.Windows.Forms.GroupBox();
            this.btnChooseFolder = new System.Windows.Forms.Button();
            this.txtImportPath = new System.Windows.Forms.TextBox();
            this.label6 = new System.Windows.Forms.Label();
            this.radFolderPath = new System.Windows.Forms.RadioButton();
            this.radGamePath = new System.Windows.Forms.RadioButton();
            this.groupBox5 = new System.Windows.Forms.GroupBox();
            this.cmbSpriteImportTransparency = new System.Windows.Forms.ComboBox();
            this.label12 = new System.Windows.Forms.Label();
            this.btnSelectPaintProgram = new System.Windows.Forms.Button();
            this.txtPaintProgram = new System.Windows.Forms.TextBox();
            this.label11 = new System.Windows.Forms.Label();
            this.radPaintProgram = new System.Windows.Forms.RadioButton();
            this.radDefaultPaintProgram = new System.Windows.Forms.RadioButton();
            this.groupBox6 = new System.Windows.Forms.GroupBox();
            this.btnNewGameChooseFolder = new System.Windows.Forms.Button();
            this.txtNewGamePath = new System.Windows.Forms.TextBox();
            this.label13 = new System.Windows.Forms.Label();
            this.radNewGameSpecificPath = new System.Windows.Forms.RadioButton();
            this.radNewGameMyDocs = new System.Windows.Forms.RadioButton();
            this.groupBox7 = new System.Windows.Forms.GroupBox();
            this.lnkUsageInfo = new System.Windows.Forms.LinkLabel();
            this.chkUsageInfo = new System.Windows.Forms.CheckBox();
            this.groupBox8 = new System.Windows.Forms.GroupBox();
            this.udBackupInterval = new System.Windows.Forms.NumericUpDown();
            this.label14 = new System.Windows.Forms.Label();
            this.chkBackupReminders = new System.Windows.Forms.CheckBox();
            this.groupBox9 = new System.Windows.Forms.GroupBox();
            this.chkRemapBgImport = new System.Windows.Forms.CheckBox();
            this.groupBox1.SuspendLayout();
            this.groupBox2.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.udTabWidth)).BeginInit();
            this.groupBox3.SuspendLayout();
            this.groupBox4.SuspendLayout();
            this.groupBox5.SuspendLayout();
            this.groupBox6.SuspendLayout();
            this.groupBox7.SuspendLayout();
            this.groupBox8.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.udBackupInterval)).BeginInit();
            this.groupBox9.SuspendLayout();
            this.SuspendLayout();
            // 
            // btnOK
            // 
            this.btnOK.DialogResult = System.Windows.Forms.DialogResult.OK;
            this.btnOK.Location = new System.Drawing.Point(18, 582);
            this.btnOK.Margin = new System.Windows.Forms.Padding(4, 4, 4, 4);
            this.btnOK.Name = "btnOK";
            this.btnOK.Size = new System.Drawing.Size(122, 34);
            this.btnOK.TabIndex = 0;
            this.btnOK.Text = "&OK";
            this.btnOK.UseVisualStyleBackColor = true;
            this.btnOK.Click += new System.EventHandler(this.btnOK_Click);
            // 
            // btnCancel
            // 
            this.btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnCancel.Location = new System.Drawing.Point(159, 582);
            this.btnCancel.Margin = new System.Windows.Forms.Padding(4, 4, 4, 4);
            this.btnCancel.Name = "btnCancel";
            this.btnCancel.Size = new System.Drawing.Size(125, 34);
            this.btnCancel.TabIndex = 1;
            this.btnCancel.Text = "&Cancel";
            this.btnCancel.UseVisualStyleBackColor = true;
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.label4);
            this.groupBox1.Controls.Add(this.label3);
            this.groupBox1.Controls.Add(this.cmbTestGameStyle);
            this.groupBox1.Controls.Add(this.label1);
            this.groupBox1.Location = new System.Drawing.Point(9, 4);
            this.groupBox1.Margin = new System.Windows.Forms.Padding(4, 4, 4, 4);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Padding = new System.Windows.Forms.Padding(4, 4, 4, 4);
            this.groupBox1.Size = new System.Drawing.Size(456, 134);
            this.groupBox1.TabIndex = 2;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Test game style";
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(15, 99);
            this.label4.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(214, 17);
            this.label4.TabIndex = 3;
            this.label4.Text = "When running without debugger:";
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label3.Location = new System.Drawing.Point(14, 58);
            this.label3.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this.label3.MaximumSize = new System.Drawing.Size(438, 0);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(431, 34);
            this.label3.TabIndex = 2;
            this.label3.Text = "NOTE: When using the F5 (Run) option, the game will always run in a window.";
            // 
            // cmbTestGameStyle
            // 
            this.cmbTestGameStyle.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cmbTestGameStyle.FormattingEnabled = true;
            this.cmbTestGameStyle.Items.AddRange(new object[] {
            "Use game setup configuration",
            "Always run full screen",
            "Always run in a window"});
            this.cmbTestGameStyle.Location = new System.Drawing.Point(230, 95);
            this.cmbTestGameStyle.Margin = new System.Windows.Forms.Padding(4, 4, 4, 4);
            this.cmbTestGameStyle.Name = "cmbTestGameStyle";
            this.cmbTestGameStyle.Size = new System.Drawing.Size(213, 25);
            this.cmbTestGameStyle.TabIndex = 1;
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(14, 25);
            this.label1.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this.label1.MaximumSize = new System.Drawing.Size(438, 0);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(428, 34);
            this.label1.TabIndex = 0;
            this.label1.Text = "Would you like the game to run in a window or full-screen when you test it?";
            // 
            // groupBox2
            // 
            this.groupBox2.Controls.Add(this.udTabWidth);
            this.groupBox2.Controls.Add(this.cmbIndentStyle);
            this.groupBox2.Controls.Add(this.label10);
            this.groupBox2.Controls.Add(this.label8);
            this.groupBox2.Controls.Add(this.label2);
            this.groupBox2.Location = new System.Drawing.Point(472, 4);
            this.groupBox2.Margin = new System.Windows.Forms.Padding(4, 4, 4, 4);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Padding = new System.Windows.Forms.Padding(4, 4, 4, 4);
            this.groupBox2.Size = new System.Drawing.Size(419, 134);
            this.groupBox2.TabIndex = 3;
            this.groupBox2.TabStop = false;
            this.groupBox2.Text = "Script editor";
            // 
            // udTabWidth
            // 
            this.udTabWidth.Location = new System.Drawing.Point(90, 60);
            this.udTabWidth.Margin = new System.Windows.Forms.Padding(4, 4, 4, 4);
            this.udTabWidth.Maximum = new decimal(new int[] {
            10,
            0,
            0,
            0});
            this.udTabWidth.Minimum = new decimal(new int[] {
            1,
            0,
            0,
            0});
            this.udTabWidth.Name = "udTabWidth";
            this.udTabWidth.Size = new System.Drawing.Size(94, 24);
            this.udTabWidth.TabIndex = 9;
            this.udTabWidth.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            this.udTabWidth.Value = new decimal(new int[] {
            1,
            0,
            0,
            0});
            // 
            // cmbIndentStyle
            // 
            this.cmbIndentStyle.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cmbIndentStyle.FormattingEnabled = true;
            this.cmbIndentStyle.Items.AddRange(new object[] {
            "Use spaces",
            "Use tabs"});
            this.cmbIndentStyle.Location = new System.Drawing.Point(90, 92);
            this.cmbIndentStyle.Margin = new System.Windows.Forms.Padding(4, 4, 4, 4);
            this.cmbIndentStyle.Name = "cmbIndentStyle";
            this.cmbIndentStyle.Size = new System.Drawing.Size(222, 25);
            this.cmbIndentStyle.TabIndex = 8;
            // 
            // label10
            // 
            this.label10.AutoSize = true;
            this.label10.Location = new System.Drawing.Point(11, 96);
            this.label10.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this.label10.Name = "label10";
            this.label10.Size = new System.Drawing.Size(53, 17);
            this.label10.TabIndex = 7;
            this.label10.Text = "Indent:";
            // 
            // label8
            // 
            this.label8.AutoSize = true;
            this.label8.Location = new System.Drawing.Point(10, 21);
            this.label8.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this.label8.MaximumSize = new System.Drawing.Size(400, 0);
            this.label8.Name = "label8";
            this.label8.Size = new System.Drawing.Size(377, 34);
            this.label8.TabIndex = 2;
            this.label8.Text = "Changing these settings require you to restart the editor for them to take effect" +
                ".";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(10, 68);
            this.label2.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(73, 17);
            this.label2.TabIndex = 0;
            this.label2.Text = "Tab width:";
            // 
            // groupBox3
            // 
            this.groupBox3.Controls.Add(this.chkKeepHelpOnTop);
            this.groupBox3.Controls.Add(this.chkAlwaysShowViewPreview);
            this.groupBox3.Controls.Add(this.cmbMessageOnCompile);
            this.groupBox3.Controls.Add(this.label9);
            this.groupBox3.Controls.Add(this.cmbEditorStartup);
            this.groupBox3.Controls.Add(this.label5);
            this.groupBox3.Location = new System.Drawing.Point(9, 145);
            this.groupBox3.Margin = new System.Windows.Forms.Padding(4, 4, 4, 4);
            this.groupBox3.Name = "groupBox3";
            this.groupBox3.Padding = new System.Windows.Forms.Padding(4, 4, 4, 4);
            this.groupBox3.Size = new System.Drawing.Size(456, 188);
            this.groupBox3.TabIndex = 4;
            this.groupBox3.TabStop = false;
            this.groupBox3.Text = "Editor appearance";
            // 
            // chkKeepHelpOnTop
            // 
            this.chkKeepHelpOnTop.AutoSize = true;
            this.chkKeepHelpOnTop.Location = new System.Drawing.Point(18, 159);
            this.chkKeepHelpOnTop.Margin = new System.Windows.Forms.Padding(4, 4, 4, 4);
            this.chkKeepHelpOnTop.Name = "chkKeepHelpOnTop";
            this.chkKeepHelpOnTop.Size = new System.Drawing.Size(290, 21);
            this.chkKeepHelpOnTop.TabIndex = 8;
            this.chkKeepHelpOnTop.Text = "Keep Help window on top of editor window";
            this.chkKeepHelpOnTop.UseVisualStyleBackColor = true;
            // 
            // chkAlwaysShowViewPreview
            // 
            this.chkAlwaysShowViewPreview.AutoSize = true;
            this.chkAlwaysShowViewPreview.Location = new System.Drawing.Point(18, 131);
            this.chkAlwaysShowViewPreview.Margin = new System.Windows.Forms.Padding(4, 4, 4, 4);
            this.chkAlwaysShowViewPreview.Name = "chkAlwaysShowViewPreview";
            this.chkAlwaysShowViewPreview.Size = new System.Drawing.Size(301, 21);
            this.chkAlwaysShowViewPreview.TabIndex = 7;
            this.chkAlwaysShowViewPreview.Text = "Show view preview by default in view editors";
            this.chkAlwaysShowViewPreview.UseVisualStyleBackColor = true;
            // 
            // cmbMessageOnCompile
            // 
            this.cmbMessageOnCompile.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cmbMessageOnCompile.FormattingEnabled = true;
            this.cmbMessageOnCompile.Items.AddRange(new object[] {
            "When there are warnings or errors",
            "When there are errors",
            "Never"});
            this.cmbMessageOnCompile.Location = new System.Drawing.Point(190, 62);
            this.cmbMessageOnCompile.Margin = new System.Windows.Forms.Padding(4, 4, 4, 4);
            this.cmbMessageOnCompile.Name = "cmbMessageOnCompile";
            this.cmbMessageOnCompile.Size = new System.Drawing.Size(253, 25);
            this.cmbMessageOnCompile.TabIndex = 6;
            // 
            // label9
            // 
            this.label9.AutoSize = true;
            this.label9.Location = new System.Drawing.Point(15, 66);
            this.label9.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this.label9.Name = "label9";
            this.label9.Size = new System.Drawing.Size(180, 17);
            this.label9.TabIndex = 5;
            this.label9.Text = "Popup message on compile:";
            // 
            // cmbEditorStartup
            // 
            this.cmbEditorStartup.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cmbEditorStartup.FormattingEnabled = true;
            this.cmbEditorStartup.Items.AddRange(new object[] {
            "Show Start Page",
            "Show Game Settings",
            "No panes open"});
            this.cmbEditorStartup.Location = new System.Drawing.Point(190, 30);
            this.cmbEditorStartup.Margin = new System.Windows.Forms.Padding(4, 4, 4, 4);
            this.cmbEditorStartup.Name = "cmbEditorStartup";
            this.cmbEditorStartup.Size = new System.Drawing.Size(253, 25);
            this.cmbEditorStartup.TabIndex = 2;
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.Location = new System.Drawing.Point(15, 34);
            this.label5.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(171, 17);
            this.label5.TabIndex = 0;
            this.label5.Text = "When the editor starts up:";
            // 
            // groupBox4
            // 
            this.groupBox4.Controls.Add(this.btnChooseFolder);
            this.groupBox4.Controls.Add(this.txtImportPath);
            this.groupBox4.Controls.Add(this.label6);
            this.groupBox4.Controls.Add(this.radFolderPath);
            this.groupBox4.Controls.Add(this.radGamePath);
            this.groupBox4.Location = new System.Drawing.Point(472, 460);
            this.groupBox4.Margin = new System.Windows.Forms.Padding(4, 4, 4, 4);
            this.groupBox4.Name = "groupBox4";
            this.groupBox4.Padding = new System.Windows.Forms.Padding(4, 4, 4, 4);
            this.groupBox4.Size = new System.Drawing.Size(419, 112);
            this.groupBox4.TabIndex = 5;
            this.groupBox4.TabStop = false;
            this.groupBox4.Text = "Import directory";
            // 
            // btnChooseFolder
            // 
            this.btnChooseFolder.Location = new System.Drawing.Point(372, 75);
            this.btnChooseFolder.Margin = new System.Windows.Forms.Padding(4, 4, 4, 4);
            this.btnChooseFolder.Name = "btnChooseFolder";
            this.btnChooseFolder.Size = new System.Drawing.Size(34, 26);
            this.btnChooseFolder.TabIndex = 4;
            this.btnChooseFolder.Text = "...";
            this.btnChooseFolder.UseVisualStyleBackColor = true;
            this.btnChooseFolder.Click += new System.EventHandler(this.btnChooseFolder_Click);
            // 
            // txtImportPath
            // 
            this.txtImportPath.Location = new System.Drawing.Point(121, 75);
            this.txtImportPath.Margin = new System.Windows.Forms.Padding(4, 4, 4, 4);
            this.txtImportPath.MaxLength = 0;
            this.txtImportPath.Name = "txtImportPath";
            this.txtImportPath.Size = new System.Drawing.Size(248, 24);
            this.txtImportPath.TabIndex = 3;
            // 
            // label6
            // 
            this.label6.AutoSize = true;
            this.label6.Location = new System.Drawing.Point(15, 22);
            this.label6.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(348, 17);
            this.label6.TabIndex = 2;
            this.label6.Text = "When you import files, where do you want to look first?";
            // 
            // radFolderPath
            // 
            this.radFolderPath.AutoSize = true;
            this.radFolderPath.Location = new System.Drawing.Point(18, 78);
            this.radFolderPath.Margin = new System.Windows.Forms.Padding(4, 4, 4, 4);
            this.radFolderPath.Name = "radFolderPath";
            this.radFolderPath.Size = new System.Drawing.Size(94, 21);
            this.radFolderPath.TabIndex = 1;
            this.radFolderPath.Text = "Default to:";
            this.radFolderPath.UseVisualStyleBackColor = true;
            this.radFolderPath.CheckedChanged += new System.EventHandler(this.radFolderPath_CheckedChanged);
            // 
            // radGamePath
            // 
            this.radGamePath.AutoSize = true;
            this.radGamePath.Checked = true;
            this.radGamePath.Location = new System.Drawing.Point(18, 49);
            this.radGamePath.Margin = new System.Windows.Forms.Padding(4, 4, 4, 4);
            this.radGamePath.Name = "radGamePath";
            this.radGamePath.Size = new System.Drawing.Size(313, 21);
            this.radGamePath.TabIndex = 0;
            this.radGamePath.TabStop = true;
            this.radGamePath.Text = "Default to the game folder when importing files";
            this.radGamePath.UseVisualStyleBackColor = true;
            this.radGamePath.CheckedChanged += new System.EventHandler(this.radGamePath_CheckedChanged);
            // 
            // groupBox5
            // 
            this.groupBox5.Controls.Add(this.cmbSpriteImportTransparency);
            this.groupBox5.Controls.Add(this.label12);
            this.groupBox5.Controls.Add(this.btnSelectPaintProgram);
            this.groupBox5.Controls.Add(this.txtPaintProgram);
            this.groupBox5.Controls.Add(this.label11);
            this.groupBox5.Controls.Add(this.radPaintProgram);
            this.groupBox5.Controls.Add(this.radDefaultPaintProgram);
            this.groupBox5.Location = new System.Drawing.Point(472, 145);
            this.groupBox5.Margin = new System.Windows.Forms.Padding(4, 4, 4, 4);
            this.groupBox5.Name = "groupBox5";
            this.groupBox5.Padding = new System.Windows.Forms.Padding(4, 4, 4, 4);
            this.groupBox5.Size = new System.Drawing.Size(419, 188);
            this.groupBox5.TabIndex = 6;
            this.groupBox5.TabStop = false;
            this.groupBox5.Text = "Sprite editor";
            // 
            // cmbSpriteImportTransparency
            // 
            this.cmbSpriteImportTransparency.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cmbSpriteImportTransparency.FormattingEnabled = true;
            this.cmbSpriteImportTransparency.Items.AddRange(new object[] {
            "Palette index 0",
            "Top-left pixel",
            "Bottom-left pixel",
            "Top-right pixel",
            "Bottom-right pixel",
            "Leave as-is",
            "No transparency"});
            this.cmbSpriteImportTransparency.Location = new System.Drawing.Point(236, 144);
            this.cmbSpriteImportTransparency.Margin = new System.Windows.Forms.Padding(4, 4, 4, 4);
            this.cmbSpriteImportTransparency.Name = "cmbSpriteImportTransparency";
            this.cmbSpriteImportTransparency.Size = new System.Drawing.Size(168, 25);
            this.cmbSpriteImportTransparency.TabIndex = 9;
            // 
            // label12
            // 
            this.label12.AutoSize = true;
            this.label12.Location = new System.Drawing.Point(15, 148);
            this.label12.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this.label12.Name = "label12";
            this.label12.Size = new System.Drawing.Size(222, 17);
            this.label12.TabIndex = 5;
            this.label12.Text = "Default sprite import transparency:";
            // 
            // btnSelectPaintProgram
            // 
            this.btnSelectPaintProgram.Location = new System.Drawing.Point(371, 90);
            this.btnSelectPaintProgram.Margin = new System.Windows.Forms.Padding(4, 4, 4, 4);
            this.btnSelectPaintProgram.Name = "btnSelectPaintProgram";
            this.btnSelectPaintProgram.Size = new System.Drawing.Size(34, 26);
            this.btnSelectPaintProgram.TabIndex = 4;
            this.btnSelectPaintProgram.Text = "...";
            this.btnSelectPaintProgram.UseVisualStyleBackColor = true;
            this.btnSelectPaintProgram.Click += new System.EventHandler(this.btnSelectPaintProgram_Click);
            // 
            // txtPaintProgram
            // 
            this.txtPaintProgram.Location = new System.Drawing.Point(139, 90);
            this.txtPaintProgram.Margin = new System.Windows.Forms.Padding(4, 4, 4, 4);
            this.txtPaintProgram.MaxLength = 0;
            this.txtPaintProgram.Name = "txtPaintProgram";
            this.txtPaintProgram.Size = new System.Drawing.Size(230, 24);
            this.txtPaintProgram.TabIndex = 3;
            // 
            // label11
            // 
            this.label11.AutoSize = true;
            this.label11.Location = new System.Drawing.Point(15, 22);
            this.label11.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this.label11.MaximumSize = new System.Drawing.Size(400, 0);
            this.label11.Name = "label11";
            this.label11.Size = new System.Drawing.Size(394, 34);
            this.label11.TabIndex = 2;
            this.label11.Text = "When you double-click a sprite, what program do you want to use to edit it? This " +
                "program must support PNG and BMP files.";
            // 
            // radPaintProgram
            // 
            this.radPaintProgram.AutoSize = true;
            this.radPaintProgram.Location = new System.Drawing.Point(18, 92);
            this.radPaintProgram.Margin = new System.Windows.Forms.Padding(4, 4, 4, 4);
            this.radPaintProgram.Name = "radPaintProgram";
            this.radPaintProgram.Size = new System.Drawing.Size(115, 21);
            this.radPaintProgram.TabIndex = 1;
            this.radPaintProgram.Text = "This program:";
            this.radPaintProgram.UseVisualStyleBackColor = true;
            this.radPaintProgram.CheckedChanged += new System.EventHandler(this.radPaintProgram_CheckedChanged);
            // 
            // radDefaultPaintProgram
            // 
            this.radDefaultPaintProgram.AutoSize = true;
            this.radDefaultPaintProgram.Checked = true;
            this.radDefaultPaintProgram.Location = new System.Drawing.Point(18, 64);
            this.radDefaultPaintProgram.Margin = new System.Windows.Forms.Padding(4, 4, 4, 4);
            this.radDefaultPaintProgram.Name = "radDefaultPaintProgram";
            this.radDefaultPaintProgram.Size = new System.Drawing.Size(341, 21);
            this.radDefaultPaintProgram.TabIndex = 0;
            this.radDefaultPaintProgram.TabStop = true;
            this.radDefaultPaintProgram.Text = "The default paint program registered with Windows";
            this.radDefaultPaintProgram.UseVisualStyleBackColor = true;
            this.radDefaultPaintProgram.CheckedChanged += new System.EventHandler(this.radDefaultPaintProgram_CheckedChanged);
            // 
            // groupBox6
            // 
            this.groupBox6.Controls.Add(this.btnNewGameChooseFolder);
            this.groupBox6.Controls.Add(this.txtNewGamePath);
            this.groupBox6.Controls.Add(this.label13);
            this.groupBox6.Controls.Add(this.radNewGameSpecificPath);
            this.groupBox6.Controls.Add(this.radNewGameMyDocs);
            this.groupBox6.Location = new System.Drawing.Point(472, 340);
            this.groupBox6.Margin = new System.Windows.Forms.Padding(4, 4, 4, 4);
            this.groupBox6.Name = "groupBox6";
            this.groupBox6.Padding = new System.Windows.Forms.Padding(4, 4, 4, 4);
            this.groupBox6.Size = new System.Drawing.Size(419, 112);
            this.groupBox6.TabIndex = 7;
            this.groupBox6.TabStop = false;
            this.groupBox6.Text = "New game directory";
            // 
            // btnNewGameChooseFolder
            // 
            this.btnNewGameChooseFolder.Location = new System.Drawing.Point(372, 75);
            this.btnNewGameChooseFolder.Margin = new System.Windows.Forms.Padding(4, 4, 4, 4);
            this.btnNewGameChooseFolder.Name = "btnNewGameChooseFolder";
            this.btnNewGameChooseFolder.Size = new System.Drawing.Size(34, 26);
            this.btnNewGameChooseFolder.TabIndex = 4;
            this.btnNewGameChooseFolder.Text = "...";
            this.btnNewGameChooseFolder.UseVisualStyleBackColor = true;
            this.btnNewGameChooseFolder.Click += new System.EventHandler(this.btnNewGameChooseFolder_Click);
            // 
            // txtNewGamePath
            // 
            this.txtNewGamePath.Location = new System.Drawing.Point(121, 75);
            this.txtNewGamePath.Margin = new System.Windows.Forms.Padding(4, 4, 4, 4);
            this.txtNewGamePath.MaxLength = 0;
            this.txtNewGamePath.Name = "txtNewGamePath";
            this.txtNewGamePath.Size = new System.Drawing.Size(248, 24);
            this.txtNewGamePath.TabIndex = 3;
            // 
            // label13
            // 
            this.label13.AutoSize = true;
            this.label13.Location = new System.Drawing.Point(15, 22);
            this.label13.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this.label13.Name = "label13";
            this.label13.Size = new System.Drawing.Size(375, 17);
            this.label13.TabIndex = 2;
            this.label13.Text = "When you create a new game, where do you want it to go?";
            // 
            // radNewGameSpecificPath
            // 
            this.radNewGameSpecificPath.AutoSize = true;
            this.radNewGameSpecificPath.Location = new System.Drawing.Point(18, 78);
            this.radNewGameSpecificPath.Margin = new System.Windows.Forms.Padding(4, 4, 4, 4);
            this.radNewGameSpecificPath.Name = "radNewGameSpecificPath";
            this.radNewGameSpecificPath.Size = new System.Drawing.Size(94, 21);
            this.radNewGameSpecificPath.TabIndex = 1;
            this.radNewGameSpecificPath.Text = "Default to:";
            this.radNewGameSpecificPath.UseVisualStyleBackColor = true;
            this.radNewGameSpecificPath.CheckedChanged += new System.EventHandler(this.radNewGameSpecificPath_CheckedChanged);
            // 
            // radNewGameMyDocs
            // 
            this.radNewGameMyDocs.AutoSize = true;
            this.radNewGameMyDocs.Checked = true;
            this.radNewGameMyDocs.Location = new System.Drawing.Point(18, 49);
            this.radNewGameMyDocs.Margin = new System.Windows.Forms.Padding(4, 4, 4, 4);
            this.radNewGameMyDocs.Name = "radNewGameMyDocs";
            this.radNewGameMyDocs.Size = new System.Drawing.Size(186, 21);
            this.radNewGameMyDocs.TabIndex = 0;
            this.radNewGameMyDocs.TabStop = true;
            this.radNewGameMyDocs.Text = "Default to My Documents";
            this.radNewGameMyDocs.UseVisualStyleBackColor = true;
            this.radNewGameMyDocs.CheckedChanged += new System.EventHandler(this.radNewGameMyDocs_CheckedChanged);
            // 
            // groupBox7
            // 
            this.groupBox7.Controls.Add(this.lnkUsageInfo);
            this.groupBox7.Controls.Add(this.chkUsageInfo);
            this.groupBox7.Location = new System.Drawing.Point(9, 340);
            this.groupBox7.Margin = new System.Windows.Forms.Padding(4, 4, 4, 4);
            this.groupBox7.Name = "groupBox7";
            this.groupBox7.Padding = new System.Windows.Forms.Padding(4, 4, 4, 4);
            this.groupBox7.Size = new System.Drawing.Size(456, 85);
            this.groupBox7.TabIndex = 8;
            this.groupBox7.TabStop = false;
            this.groupBox7.Text = "Usage statistics";
            // 
            // lnkUsageInfo
            // 
            this.lnkUsageInfo.AutoSize = true;
            this.lnkUsageInfo.Location = new System.Drawing.Point(15, 54);
            this.lnkUsageInfo.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this.lnkUsageInfo.Name = "lnkUsageInfo";
            this.lnkUsageInfo.Size = new System.Drawing.Size(249, 17);
            this.lnkUsageInfo.TabIndex = 9;
            this.lnkUsageInfo.TabStop = true;
            this.lnkUsageInfo.Text = "What is this and why should I enable it?";
            this.lnkUsageInfo.LinkClicked += new System.Windows.Forms.LinkLabelLinkClickedEventHandler(this.lnkUsageInfo_LinkClicked);
            // 
            // chkUsageInfo
            // 
            this.chkUsageInfo.AutoSize = true;
            this.chkUsageInfo.Location = new System.Drawing.Point(18, 25);
            this.chkUsageInfo.Margin = new System.Windows.Forms.Padding(4, 4, 4, 4);
            this.chkUsageInfo.Name = "chkUsageInfo";
            this.chkUsageInfo.Size = new System.Drawing.Size(370, 21);
            this.chkUsageInfo.TabIndex = 8;
            this.chkUsageInfo.Text = "Send anonymous usage information to the AGS website";
            this.chkUsageInfo.UseVisualStyleBackColor = true;
            // 
            // groupBox8
            // 
            this.groupBox8.Controls.Add(this.udBackupInterval);
            this.groupBox8.Controls.Add(this.label14);
            this.groupBox8.Controls.Add(this.chkBackupReminders);
            this.groupBox8.Location = new System.Drawing.Point(9, 432);
            this.groupBox8.Margin = new System.Windows.Forms.Padding(4, 4, 4, 4);
            this.groupBox8.Name = "groupBox8";
            this.groupBox8.Padding = new System.Windows.Forms.Padding(4, 4, 4, 4);
            this.groupBox8.Size = new System.Drawing.Size(456, 66);
            this.groupBox8.TabIndex = 9;
            this.groupBox8.TabStop = false;
            this.groupBox8.Text = "Backup reminders";
            // 
            // udBackupInterval
            // 
            this.udBackupInterval.Location = new System.Drawing.Point(86, 24);
            this.udBackupInterval.Margin = new System.Windows.Forms.Padding(4, 4, 4, 4);
            this.udBackupInterval.Maximum = new decimal(new int[] {
            90,
            0,
            0,
            0});
            this.udBackupInterval.Minimum = new decimal(new int[] {
            1,
            0,
            0,
            0});
            this.udBackupInterval.Name = "udBackupInterval";
            this.udBackupInterval.Size = new System.Drawing.Size(52, 24);
            this.udBackupInterval.TabIndex = 10;
            this.udBackupInterval.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            this.udBackupInterval.Value = new decimal(new int[] {
            1,
            0,
            0,
            0});
            // 
            // label14
            // 
            this.label14.AutoSize = true;
            this.label14.Location = new System.Drawing.Point(146, 26);
            this.label14.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this.label14.MaximumSize = new System.Drawing.Size(288, 62);
            this.label14.Name = "label14";
            this.label14.Size = new System.Drawing.Size(265, 34);
            this.label14.TabIndex = 9;
            this.label14.Text = "days, pop up a message reminding me to back up my game";
            // 
            // chkBackupReminders
            // 
            this.chkBackupReminders.AutoSize = true;
            this.chkBackupReminders.Location = new System.Drawing.Point(18, 25);
            this.chkBackupReminders.Margin = new System.Windows.Forms.Padding(4, 4, 4, 4);
            this.chkBackupReminders.Name = "chkBackupReminders";
            this.chkBackupReminders.Size = new System.Drawing.Size(66, 21);
            this.chkBackupReminders.TabIndex = 8;
            this.chkBackupReminders.Text = "Every";
            this.chkBackupReminders.UseVisualStyleBackColor = true;
            this.chkBackupReminders.CheckedChanged += new System.EventHandler(this.chkBackupReminders_CheckedChanged);
            // 
            // groupBox9
            // 
            this.groupBox9.Controls.Add(this.chkRemapBgImport);
            this.groupBox9.Location = new System.Drawing.Point(9, 505);
            this.groupBox9.Margin = new System.Windows.Forms.Padding(4, 4, 4, 4);
            this.groupBox9.Name = "groupBox9";
            this.groupBox9.Padding = new System.Windows.Forms.Padding(4, 4, 4, 4);
            this.groupBox9.Size = new System.Drawing.Size(456, 68);
            this.groupBox9.TabIndex = 10;
            this.groupBox9.TabStop = false;
            this.groupBox9.Text = "8-bit background import";
            // 
            // chkRemapBgImport
            // 
            this.chkRemapBgImport.Location = new System.Drawing.Point(18, 20);
            this.chkRemapBgImport.Margin = new System.Windows.Forms.Padding(4, 4, 4, 4);
            this.chkRemapBgImport.Name = "chkRemapBgImport";
            this.chkRemapBgImport.Size = new System.Drawing.Size(412, 38);
            this.chkRemapBgImport.TabIndex = 8;
            this.chkRemapBgImport.Text = "Remap palette of room backgrounds into allocated background palette slots (8-bit " +
                "games only)";
            this.chkRemapBgImport.UseVisualStyleBackColor = true;
            // 
            // PreferencesEditor
            // 
            this.AcceptButton = this.btnOK;
            this.AutoScaleDimensions = new System.Drawing.SizeF(120F, 120F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
            this.CancelButton = this.btnCancel;
            this.ClientSize = new System.Drawing.Size(902, 626);
            this.Controls.Add(this.groupBox9);
            this.Controls.Add(this.groupBox8);
            this.Controls.Add(this.groupBox7);
            this.Controls.Add(this.groupBox6);
            this.Controls.Add(this.groupBox5);
            this.Controls.Add(this.groupBox4);
            this.Controls.Add(this.groupBox3);
            this.Controls.Add(this.groupBox2);
            this.Controls.Add(this.groupBox1);
            this.Controls.Add(this.btnCancel);
            this.Controls.Add(this.btnOK);
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.Margin = new System.Windows.Forms.Padding(4, 4, 4, 4);
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "PreferencesEditor";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Preferences";
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.groupBox2.ResumeLayout(false);
            this.groupBox2.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.udTabWidth)).EndInit();
            this.groupBox3.ResumeLayout(false);
            this.groupBox3.PerformLayout();
            this.groupBox4.ResumeLayout(false);
            this.groupBox4.PerformLayout();
            this.groupBox5.ResumeLayout(false);
            this.groupBox5.PerformLayout();
            this.groupBox6.ResumeLayout(false);
            this.groupBox6.PerformLayout();
            this.groupBox7.ResumeLayout(false);
            this.groupBox7.PerformLayout();
            this.groupBox8.ResumeLayout(false);
            this.groupBox8.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.udBackupInterval)).EndInit();
            this.groupBox9.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Button btnOK;
        private System.Windows.Forms.Button btnCancel;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.ComboBox cmbTestGameStyle;
        private System.Windows.Forms.Label label1;
		private System.Windows.Forms.GroupBox groupBox2;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label3;
		private System.Windows.Forms.Label label4;
		private System.Windows.Forms.GroupBox groupBox3;
		private System.Windows.Forms.Label label5;
		private System.Windows.Forms.ComboBox cmbEditorStartup;
		private System.Windows.Forms.GroupBox groupBox4;
		private System.Windows.Forms.RadioButton radFolderPath;
		private System.Windows.Forms.RadioButton radGamePath;
		private System.Windows.Forms.TextBox txtImportPath;
		private System.Windows.Forms.Label label6;
		private System.Windows.Forms.Button btnChooseFolder;
		private System.Windows.Forms.Label label8;
		private System.Windows.Forms.ComboBox cmbMessageOnCompile;
		private System.Windows.Forms.Label label9;
		private System.Windows.Forms.ComboBox cmbIndentStyle;
		private System.Windows.Forms.Label label10;
		private System.Windows.Forms.CheckBox chkAlwaysShowViewPreview;
		private System.Windows.Forms.NumericUpDown udTabWidth;
		private System.Windows.Forms.GroupBox groupBox5;
		private System.Windows.Forms.Button btnSelectPaintProgram;
		private System.Windows.Forms.TextBox txtPaintProgram;
		private System.Windows.Forms.Label label11;
		private System.Windows.Forms.RadioButton radPaintProgram;
		private System.Windows.Forms.RadioButton radDefaultPaintProgram;
		private System.Windows.Forms.ComboBox cmbSpriteImportTransparency;
		private System.Windows.Forms.Label label12;
		private System.Windows.Forms.GroupBox groupBox6;
		private System.Windows.Forms.Button btnNewGameChooseFolder;
		private System.Windows.Forms.TextBox txtNewGamePath;
		private System.Windows.Forms.Label label13;
		private System.Windows.Forms.RadioButton radNewGameSpecificPath;
		private System.Windows.Forms.RadioButton radNewGameMyDocs;
        private System.Windows.Forms.GroupBox groupBox7;
        private System.Windows.Forms.LinkLabel lnkUsageInfo;
        private System.Windows.Forms.CheckBox chkUsageInfo;
        private System.Windows.Forms.GroupBox groupBox8;
        private System.Windows.Forms.CheckBox chkBackupReminders;
        private System.Windows.Forms.NumericUpDown udBackupInterval;
        private System.Windows.Forms.Label label14;
        private System.Windows.Forms.GroupBox groupBox9;
        private System.Windows.Forms.CheckBox chkRemapBgImport;
        private System.Windows.Forms.CheckBox chkKeepHelpOnTop;
    }
}