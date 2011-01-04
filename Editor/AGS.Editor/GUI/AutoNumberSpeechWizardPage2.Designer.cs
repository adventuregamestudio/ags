namespace AGS.Editor
{
    partial class AutoNumberSpeechWizardPage2
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
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.cmbWhichCharacter = new System.Windows.Forms.ComboBox();
            this.chkRemoveLines = new System.Windows.Forms.CheckBox();
            this.label2 = new System.Windows.Forms.Label();
            this.label1 = new System.Windows.Forms.Label();
            this.groupBox1.SuspendLayout();
            this.SuspendLayout();
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.cmbWhichCharacter);
            this.groupBox1.Controls.Add(this.chkRemoveLines);
            this.groupBox1.Controls.Add(this.label2);
            this.groupBox1.Controls.Add(this.label1);
            this.groupBox1.Location = new System.Drawing.Point(9, 7);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(520, 219);
            this.groupBox1.TabIndex = 0;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Advanced options";
            // 
            // cmbWhichCharacter
            // 
            this.cmbWhichCharacter.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cmbWhichCharacter.FormattingEnabled = true;
            this.cmbWhichCharacter.Location = new System.Drawing.Point(16, 38);
            this.cmbWhichCharacter.Name = "cmbWhichCharacter";
            this.cmbWhichCharacter.Size = new System.Drawing.Size(231, 21);
            this.cmbWhichCharacter.TabIndex = 4;
            // 
            // chkRemoveLines
            // 
            this.chkRemoveLines.AutoSize = true;
            this.chkRemoveLines.Location = new System.Drawing.Point(16, 111);
            this.chkRemoveLines.Name = "chkRemoveLines";
            this.chkRemoveLines.Size = new System.Drawing.Size(367, 17);
            this.chkRemoveLines.TabIndex = 3;
            this.chkRemoveLines.Text = "Remove speech numbers from all game text for the above character(s)";
            this.chkRemoveLines.UseVisualStyleBackColor = true;
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(13, 79);
            this.label2.MaximumSize = new System.Drawing.Size(450, 0);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(442, 26);
            this.label2.TabIndex = 2;
            this.label2.Text = "If you have already numbered some speech lines but no longer want them, you can s" +
                "elect to remove the speech numbering rather than adding it.";
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(13, 19);
            this.label1.MaximumSize = new System.Drawing.Size(450, 0);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(245, 13);
            this.label1.TabIndex = 0;
            this.label1.Text = "Which characters text to you want to re-number?";
            // 
            // AutoNumberSpeechWizardPage2
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(96F, 96F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
            this.Controls.Add(this.groupBox1);
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Name = "AutoNumberSpeechWizardPage2";
            this.Size = new System.Drawing.Size(773, 374);
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.CheckBox chkRemoveLines;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.ComboBox cmbWhichCharacter;
    }
}
