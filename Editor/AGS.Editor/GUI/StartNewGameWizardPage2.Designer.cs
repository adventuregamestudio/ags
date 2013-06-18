namespace AGS.Editor
{
    partial class StartNewGameWizardPage2
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
            this.txtCreateInFolder = new System.Windows.Forms.TextBox();
            this.label3 = new System.Windows.Forms.Label();
            this.lblFilePath = new System.Windows.Forms.Label();
            this.txtFriendlyName = new System.Windows.Forms.TextBox();
            this.label2 = new System.Windows.Forms.Label();
            this.txtFileName = new System.Windows.Forms.TextBox();
            this.label1 = new System.Windows.Forms.Label();
            this.btnCreateInBrowse = new System.Windows.Forms.Button();
            this.groupBox1.SuspendLayout();
            this.SuspendLayout();
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.btnCreateInBrowse);
            this.groupBox1.Controls.Add(this.txtCreateInFolder);
            this.groupBox1.Controls.Add(this.label3);
            this.groupBox1.Controls.Add(this.lblFilePath);
            this.groupBox1.Controls.Add(this.txtFriendlyName);
            this.groupBox1.Controls.Add(this.label2);
            this.groupBox1.Controls.Add(this.txtFileName);
            this.groupBox1.Controls.Add(this.label1);
            this.groupBox1.Location = new System.Drawing.Point(9, 7);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(520, 219);
            this.groupBox1.TabIndex = 0;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "New game details";
            // 
            // txtCreateInFolder
            // 
            this.txtCreateInFolder.Location = new System.Drawing.Point(19, 167);
            this.txtCreateInFolder.MaxLength = 200;
            this.txtCreateInFolder.Name = "txtCreateInFolder";
            this.txtCreateInFolder.Size = new System.Drawing.Size(440, 21);
            this.txtCreateInFolder.TabIndex = 2;
            this.txtCreateInFolder.TextChanged += new System.EventHandler(this.txtCreateInFolder_TextChanged);
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(16, 151);
            this.label3.MaximumSize = new System.Drawing.Size(450, 0);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(86, 13);
            this.label3.TabIndex = 5;
            this.label3.Text = "Create in folder:";
            // 
            // lblFilePath
            // 
            this.lblFilePath.AutoSize = true;
            this.lblFilePath.Location = new System.Drawing.Point(16, 191);
            this.lblFilePath.MaximumSize = new System.Drawing.Size(470, 0);
            this.lblFilePath.Name = "lblFilePath";
            this.lblFilePath.Size = new System.Drawing.Size(0, 13);
            this.lblFilePath.TabIndex = 4;
            // 
            // txtFriendlyName
            // 
            this.txtFriendlyName.Location = new System.Drawing.Point(19, 52);
            this.txtFriendlyName.Name = "txtFriendlyName";
            this.txtFriendlyName.Size = new System.Drawing.Size(286, 21);
            this.txtFriendlyName.TabIndex = 0;
            this.txtFriendlyName.Text = "New game";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(16, 23);
            this.label2.MaximumSize = new System.Drawing.Size(500, 0);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(482, 26);
            this.label2.TabIndex = 2;
            this.label2.Text = "What do you want the game to be called? This name will be displayed in the title " +
                "bar of the window whilst playing. You can change this later in the Game Settings" +
                " editor.";
            // 
            // txtFileName
            // 
            this.txtFileName.Location = new System.Drawing.Point(19, 114);
            this.txtFileName.MaxLength = 40;
            this.txtFileName.Name = "txtFileName";
            this.txtFileName.Size = new System.Drawing.Size(286, 21);
            this.txtFileName.TabIndex = 1;
            this.txtFileName.TextChanged += new System.EventHandler(this.txtFileName_TextChanged);
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(16, 85);
            this.label1.MaximumSize = new System.Drawing.Size(500, 0);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(458, 26);
            this.label1.TabIndex = 0;
            this.label1.Text = "Please enter the file name of the game below. This will be the name of the game f" +
                "iles that are produced. You can\'t change this later on.";
            // 
            // btnCreateInBrowse
            // 
            this.btnCreateInBrowse.Location = new System.Drawing.Point(465, 167);
            this.btnCreateInBrowse.Name = "btnCreateInBrowse";
            this.btnCreateInBrowse.Size = new System.Drawing.Size(27, 21);
            this.btnCreateInBrowse.TabIndex = 6;
            this.btnCreateInBrowse.Text = "...";
            this.btnCreateInBrowse.UseVisualStyleBackColor = true;
            this.btnCreateInBrowse.Click += new System.EventHandler(this.btnCreateInBrowse_Click);
            // 
            // StartNewGameWizardPage2
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(96F, 96F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
            this.Controls.Add(this.groupBox1);
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Name = "StartNewGameWizardPage2";
            this.Size = new System.Drawing.Size(1017, 632);
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.TextBox txtFriendlyName;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.TextBox txtFileName;
        private System.Windows.Forms.Label lblFilePath;
		private System.Windows.Forms.TextBox txtCreateInFolder;
		private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Button btnCreateInBrowse;
    }
}
