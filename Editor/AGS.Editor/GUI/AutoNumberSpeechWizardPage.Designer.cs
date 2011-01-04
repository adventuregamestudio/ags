namespace AGS.Editor
{
    partial class AutoNumberSpeechWizardPage
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(AutoNumberSpeechWizardPage));
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.chkCombineIdenticalLines = new System.Windows.Forms.CheckBox();
            this.label2 = new System.Windows.Forms.Label();
            this.chkNarrator = new System.Windows.Forms.CheckBox();
            this.label1 = new System.Windows.Forms.Label();
            this.groupBox1.SuspendLayout();
            this.SuspendLayout();
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.chkCombineIdenticalLines);
            this.groupBox1.Controls.Add(this.label2);
            this.groupBox1.Controls.Add(this.chkNarrator);
            this.groupBox1.Controls.Add(this.label1);
            this.groupBox1.Location = new System.Drawing.Point(9, 7);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(520, 219);
            this.groupBox1.TabIndex = 0;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Automatically number speech files";
            // 
            // chkCombineIdenticalLines
            // 
            this.chkCombineIdenticalLines.AutoSize = true;
            this.chkCombineIdenticalLines.Checked = true;
            this.chkCombineIdenticalLines.CheckState = System.Windows.Forms.CheckState.Checked;
            this.chkCombineIdenticalLines.Location = new System.Drawing.Point(16, 150);
            this.chkCombineIdenticalLines.Name = "chkCombineIdenticalLines";
            this.chkCombineIdenticalLines.Size = new System.Drawing.Size(273, 17);
            this.chkCombineIdenticalLines.TabIndex = 3;
            this.chkCombineIdenticalLines.Text = "Yes, give identical lines of text the same voice files.";
            this.chkCombineIdenticalLines.UseVisualStyleBackColor = true;
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(13, 104);
            this.label2.MaximumSize = new System.Drawing.Size(450, 0);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(424, 39);
            this.label2.TabIndex = 2;
            this.label2.Text = resources.GetString("label2.Text");
            // 
            // chkNarrator
            // 
            this.chkNarrator.AutoSize = true;
            this.chkNarrator.Location = new System.Drawing.Point(16, 65);
            this.chkNarrator.Name = "chkNarrator";
            this.chkNarrator.Size = new System.Drawing.Size(334, 17);
            this.chkNarrator.TabIndex = 1;
            this.chkNarrator.Text = "Yes, I will have voice files for the narrator, so number them too.";
            this.chkNarrator.UseVisualStyleBackColor = true;
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(13, 19);
            this.label1.MaximumSize = new System.Drawing.Size(450, 0);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(446, 39);
            this.label1.TabIndex = 0;
            this.label1.Text = "Do you want lines of text said by the narrator (Display commands, NARRATOR dialog" +
                " lines) to be numbered? Enable this option if you intend to have voice speech fo" +
                "r your narrator as well as characters.";
            // 
            // AutoNumberSpeechWizardPage
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(96F, 96F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
            this.Controls.Add(this.groupBox1);
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Name = "AutoNumberSpeechWizardPage";
            this.Size = new System.Drawing.Size(773, 374);
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.CheckBox chkCombineIdenticalLines;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.CheckBox chkNarrator;
    }
}
