namespace AGS.Editor
{
    partial class FontEditor
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
            this.panel1 = new System.Windows.Forms.Panel();
            this.splitContainer1 = new System.Windows.Forms.SplitContainer();
            this.tbTextPreview = new System.Windows.Forms.TextBox();
            this.textPreviewPanel = new AGS.Editor.BufferedPanel();
            this.rbPreviewAuto = new System.Windows.Forms.RadioButton();
            this.rbANSI = new System.Windows.Forms.RadioButton();
            this.rbUnicode = new System.Windows.Forms.RadioButton();
            this.label2 = new System.Windows.Forms.Label();
            this.chkDisplayCodes = new System.Windows.Forms.CheckBox();
            this.btnGotoChar = new System.Windows.Forms.Button();
            this.tbCharInput = new System.Windows.Forms.TextBox();
            this.label3 = new System.Windows.Forms.Label();
            this.udCharCode = new System.Windows.Forms.NumericUpDown();
            this.lblCharCode = new System.Windows.Forms.Label();
            this.fontViewPanel = new AGS.Editor.FontPreviewGrid();
            this.label1 = new System.Windows.Forms.Label();
            this.currentItemGroupBox.SuspendLayout();
            this.panel1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.splitContainer1)).BeginInit();
            this.splitContainer1.Panel1.SuspendLayout();
            this.splitContainer1.Panel2.SuspendLayout();
            this.splitContainer1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.udCharCode)).BeginInit();
            this.SuspendLayout();
            // 
            // currentItemGroupBox
            // 
            this.currentItemGroupBox.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.currentItemGroupBox.BackColor = System.Drawing.SystemColors.Control;
            this.currentItemGroupBox.Controls.Add(this.panel1);
            this.currentItemGroupBox.Controls.Add(this.label1);
            this.currentItemGroupBox.Location = new System.Drawing.Point(13, 13);
            this.currentItemGroupBox.Name = "currentItemGroupBox";
            this.currentItemGroupBox.Size = new System.Drawing.Size(550, 467);
            this.currentItemGroupBox.TabIndex = 0;
            this.currentItemGroupBox.TabStop = false;
            this.currentItemGroupBox.Text = "Selected font settings";
            // 
            // panel1
            // 
            this.panel1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.panel1.Controls.Add(this.splitContainer1);
            this.panel1.Location = new System.Drawing.Point(6, 34);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(538, 427);
            this.panel1.TabIndex = 5;
            // 
            // splitContainer1
            // 
            this.splitContainer1.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.splitContainer1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.splitContainer1.FixedPanel = System.Windows.Forms.FixedPanel.Panel1;
            this.splitContainer1.Location = new System.Drawing.Point(0, 0);
            this.splitContainer1.Name = "splitContainer1";
            this.splitContainer1.Orientation = System.Windows.Forms.Orientation.Horizontal;
            // 
            // splitContainer1.Panel1
            // 
            this.splitContainer1.Panel1.Controls.Add(this.tbTextPreview);
            this.splitContainer1.Panel1.Controls.Add(this.textPreviewPanel);
            this.splitContainer1.Panel1.Padding = new System.Windows.Forms.Padding(7);
            this.splitContainer1.Panel1MinSize = 120;
            // 
            // splitContainer1.Panel2
            // 
            this.splitContainer1.Panel2.Controls.Add(this.rbPreviewAuto);
            this.splitContainer1.Panel2.Controls.Add(this.rbANSI);
            this.splitContainer1.Panel2.Controls.Add(this.rbUnicode);
            this.splitContainer1.Panel2.Controls.Add(this.label2);
            this.splitContainer1.Panel2.Controls.Add(this.chkDisplayCodes);
            this.splitContainer1.Panel2.Controls.Add(this.btnGotoChar);
            this.splitContainer1.Panel2.Controls.Add(this.tbCharInput);
            this.splitContainer1.Panel2.Controls.Add(this.label3);
            this.splitContainer1.Panel2.Controls.Add(this.udCharCode);
            this.splitContainer1.Panel2.Controls.Add(this.lblCharCode);
            this.splitContainer1.Panel2.Controls.Add(this.fontViewPanel);
            this.splitContainer1.Panel2.Padding = new System.Windows.Forms.Padding(7);
            this.splitContainer1.Panel2MinSize = 120;
            this.splitContainer1.Size = new System.Drawing.Size(538, 427);
            this.splitContainer1.SplitterDistance = 120;
            this.splitContainer1.TabIndex = 0;
            this.splitContainer1.SplitterMoved += new System.Windows.Forms.SplitterEventHandler(this.splitContainer1_SplitterMoved);
            this.splitContainer1.Resize += new System.EventHandler(this.splitContainer1_Resize);
            // 
            // tbTextPreview
            // 
            this.tbTextPreview.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.tbTextPreview.Location = new System.Drawing.Point(10, 62);
            this.tbTextPreview.Multiline = true;
            this.tbTextPreview.Name = "tbTextPreview";
            this.tbTextPreview.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
            this.tbTextPreview.Size = new System.Drawing.Size(516, 44);
            this.tbTextPreview.TabIndex = 1;
            this.tbTextPreview.Text = "The quick brown fox jumps over the lazy dog.";
            this.tbTextPreview.TextChanged += new System.EventHandler(this.tbTextPreview_TextChanged);
            // 
            // textPreviewPanel
            // 
            this.textPreviewPanel.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.textPreviewPanel.AutoScroll = true;
            this.textPreviewPanel.Location = new System.Drawing.Point(10, 13);
            this.textPreviewPanel.Name = "textPreviewPanel";
            this.textPreviewPanel.Size = new System.Drawing.Size(516, 44);
            this.textPreviewPanel.TabIndex = 0;
            this.textPreviewPanel.SizeChanged += new System.EventHandler(this.textPreviewPanel_SizeChanged);
            this.textPreviewPanel.Paint += new System.Windows.Forms.PaintEventHandler(this.textPreviewPanel_Paint);
            // 
            // rbPreviewAuto
            // 
            this.rbPreviewAuto.AutoSize = true;
            this.rbPreviewAuto.Checked = true;
            this.rbPreviewAuto.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(204)));
            this.rbPreviewAuto.Location = new System.Drawing.Point(91, 6);
            this.rbPreviewAuto.Name = "rbPreviewAuto";
            this.rbPreviewAuto.Size = new System.Drawing.Size(138, 17);
            this.rbPreviewAuto.TabIndex = 10;
            this.rbPreviewAuto.TabStop = true;
            this.rbPreviewAuto.Text = "Auto (use Game setting)";
            this.rbPreviewAuto.UseVisualStyleBackColor = true;
            this.rbPreviewAuto.CheckedChanged += new System.EventHandler(this.rbPreviewAuto_CheckedChanged);
            // 
            // rbANSI
            // 
            this.rbANSI.AutoSize = true;
            this.rbANSI.Location = new System.Drawing.Point(306, 6);
            this.rbANSI.Name = "rbANSI";
            this.rbANSI.Size = new System.Drawing.Size(88, 17);
            this.rbANSI.TabIndex = 2;
            this.rbANSI.Text = "ASCII / ANSI";
            this.rbANSI.UseVisualStyleBackColor = true;
            this.rbANSI.CheckedChanged += new System.EventHandler(this.rbANSI_CheckedChanged);
            // 
            // rbUnicode
            // 
            this.rbUnicode.AutoSize = true;
            this.rbUnicode.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(204)));
            this.rbUnicode.Location = new System.Drawing.Point(235, 6);
            this.rbUnicode.Name = "rbUnicode";
            this.rbUnicode.Size = new System.Drawing.Size(65, 17);
            this.rbUnicode.TabIndex = 1;
            this.rbUnicode.Text = "Unicode";
            this.rbUnicode.UseVisualStyleBackColor = true;
            this.rbUnicode.CheckedChanged += new System.EventHandler(this.rbUnicode_CheckedChanged);
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(10, 7);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(77, 13);
            this.label2.TabIndex = 0;
            this.label2.Text = "Preview mode:";
            // 
            // chkDisplayCodes
            // 
            this.chkDisplayCodes.AutoSize = true;
            this.chkDisplayCodes.Location = new System.Drawing.Point(408, 33);
            this.chkDisplayCodes.Name = "chkDisplayCodes";
            this.chkDisplayCodes.Size = new System.Drawing.Size(92, 17);
            this.chkDisplayCodes.TabIndex = 8;
            this.chkDisplayCodes.Text = "Display codes";
            this.chkDisplayCodes.UseVisualStyleBackColor = true;
            this.chkDisplayCodes.CheckedChanged += new System.EventHandler(this.chkDisplayCodes_CheckedChanged);
            // 
            // btnGotoChar
            // 
            this.btnGotoChar.Location = new System.Drawing.Point(313, 29);
            this.btnGotoChar.Name = "btnGotoChar";
            this.btnGotoChar.Size = new System.Drawing.Size(75, 23);
            this.btnGotoChar.TabIndex = 7;
            this.btnGotoChar.Text = "Go To...";
            this.btnGotoChar.UseVisualStyleBackColor = true;
            this.btnGotoChar.Click += new System.EventHandler(this.btnGotoChar_Click);
            // 
            // tbCharInput
            // 
            this.tbCharInput.Location = new System.Drawing.Point(231, 31);
            this.tbCharInput.MaxLength = 1;
            this.tbCharInput.Name = "tbCharInput";
            this.tbCharInput.Size = new System.Drawing.Size(60, 20);
            this.tbCharInput.TabIndex = 6;
            this.tbCharInput.TextChanged += new System.EventHandler(this.tbCharInput_TextChanged);
            this.tbCharInput.KeyDown += new System.Windows.Forms.KeyEventHandler(this.tbCharInput_KeyDown);
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(169, 34);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(56, 13);
            this.label3.TabIndex = 5;
            this.label3.Text = "Character:";
            // 
            // udCharCode
            // 
            this.udCharCode.Hexadecimal = true;
            this.udCharCode.Location = new System.Drawing.Point(65, 30);
            this.udCharCode.Maximum = new decimal(new int[] {
            65535,
            0,
            0,
            0});
            this.udCharCode.Name = "udCharCode";
            this.udCharCode.Size = new System.Drawing.Size(87, 20);
            this.udCharCode.TabIndex = 4;
            this.udCharCode.ValueChanged += new System.EventHandler(this.udCharCode_ValueChanged);
            this.udCharCode.KeyDown += new System.Windows.Forms.KeyEventHandler(this.udCharCode_KeyDown);
            // 
            // lblCharCode
            // 
            this.lblCharCode.AutoSize = true;
            this.lblCharCode.Location = new System.Drawing.Point(11, 34);
            this.lblCharCode.Name = "lblCharCode";
            this.lblCharCode.Size = new System.Drawing.Size(52, 13);
            this.lblCharCode.TabIndex = 3;
            this.lblCharCode.Text = "Code: U+";
            // 
            // fontViewPanel
            // 
            this.fontViewPanel.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.fontViewPanel.ANSIMode = false;
            this.fontViewPanel.AutoScroll = true;
            this.fontViewPanel.DisplayCodes = false;
            this.fontViewPanel.GameFontNumber = -1;
            this.fontViewPanel.HideMissingCharacters = true;
            this.fontViewPanel.Location = new System.Drawing.Point(6, 57);
            this.fontViewPanel.Name = "fontViewPanel";
            this.fontViewPanel.Scaling = 1F;
            this.fontViewPanel.SelectedCharCode = -1;
            this.fontViewPanel.Size = new System.Drawing.Size(520, 234);
            this.fontViewPanel.TabIndex = 9;
            this.fontViewPanel.TabStop = true;
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(10, 18);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(300, 13);
            this.label1.TabIndex = 0;
            this.label1.Text = "Use the property grid on the right to change the font\'s settings.";
            // 
            // FontEditor
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.currentItemGroupBox);
            this.Name = "FontEditor";
            this.Size = new System.Drawing.Size(579, 493);
            this.Load += new System.EventHandler(this.FontEditor_Load);
            this.currentItemGroupBox.ResumeLayout(false);
            this.currentItemGroupBox.PerformLayout();
            this.panel1.ResumeLayout(false);
            this.splitContainer1.Panel1.ResumeLayout(false);
            this.splitContainer1.Panel1.PerformLayout();
            this.splitContainer1.Panel2.ResumeLayout(false);
            this.splitContainer1.Panel2.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.splitContainer1)).EndInit();
            this.splitContainer1.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.udCharCode)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.GroupBox currentItemGroupBox;
        private System.Windows.Forms.Label label1;
        private FontPreviewGrid fontViewPanel;
        private System.Windows.Forms.Panel panel1;
        private System.Windows.Forms.SplitContainer splitContainer1;
        private System.Windows.Forms.TextBox tbTextPreview;
        private BufferedPanel textPreviewPanel;
        private System.Windows.Forms.TextBox tbCharInput;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.NumericUpDown udCharCode;
        private System.Windows.Forms.Label lblCharCode;
        private System.Windows.Forms.Button btnGotoChar;
        private System.Windows.Forms.CheckBox chkDisplayCodes;
        private System.Windows.Forms.RadioButton rbANSI;
        private System.Windows.Forms.RadioButton rbUnicode;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.RadioButton rbPreviewAuto;
    }
}
