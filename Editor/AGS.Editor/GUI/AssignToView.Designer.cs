namespace AGS.Editor
{
    partial class AssignToView
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
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.panel1 = new System.Windows.Forms.Panel();
            this.radOverwriteNextLoop = new System.Windows.Forms.RadioButton();
            this.radStopAssigning = new System.Windows.Forms.RadioButton();
            this.chkReverse = new System.Windows.Forms.CheckBox();
            this.chkFlipped = new System.Windows.Forms.CheckBox();
            this.label5 = new System.Windows.Forms.Label();
            this.radAddToExisting = new System.Windows.Forms.RadioButton();
            this.radOverwrite = new System.Windows.Forms.RadioButton();
            this.label4 = new System.Windows.Forms.Label();
            this.cmbLoop = new System.Windows.Forms.ComboBox();
            this.btnChooseView = new System.Windows.Forms.Button();
            this.udView = new System.Windows.Forms.NumericUpDown();
            this.label3 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.label1 = new System.Windows.Forms.Label();
            this.btnOK = new System.Windows.Forms.Button();
            this.btnCancel = new System.Windows.Forms.Button();
            this.groupBox1.SuspendLayout();
            this.panel1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.udView)).BeginInit();
            this.SuspendLayout();
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.panel1);
            this.groupBox1.Controls.Add(this.chkReverse);
            this.groupBox1.Controls.Add(this.chkFlipped);
            this.groupBox1.Controls.Add(this.label5);
            this.groupBox1.Controls.Add(this.radAddToExisting);
            this.groupBox1.Controls.Add(this.radOverwrite);
            this.groupBox1.Controls.Add(this.label4);
            this.groupBox1.Controls.Add(this.cmbLoop);
            this.groupBox1.Controls.Add(this.btnChooseView);
            this.groupBox1.Controls.Add(this.udView);
            this.groupBox1.Controls.Add(this.label3);
            this.groupBox1.Controls.Add(this.label2);
            this.groupBox1.Controls.Add(this.label1);
            this.groupBox1.Location = new System.Drawing.Point(12, 10);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(298, 287);
            this.groupBox1.TabIndex = 0;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Select destination";
            // 
            // panel1
            // 
            this.panel1.Controls.Add(this.radOverwriteNextLoop);
            this.panel1.Controls.Add(this.radStopAssigning);
            this.panel1.Location = new System.Drawing.Point(21, 191);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(243, 43);
            this.panel1.TabIndex = 14;
            // 
            // radOverwriteNextLoop
            // 
            this.radOverwriteNextLoop.AutoSize = true;
            this.radOverwriteNextLoop.Location = new System.Drawing.Point(3, 22);
            this.radOverwriteNextLoop.Name = "radOverwriteNextLoop";
            this.radOverwriteNextLoop.Size = new System.Drawing.Size(240, 17);
            this.radOverwriteNextLoop.TabIndex = 12;
            this.radOverwriteNextLoop.TabStop = true;
            this.radOverwriteNextLoop.Text = "Automatically overwrite the next loop as well";
            this.radOverwriteNextLoop.UseVisualStyleBackColor = true;
            // 
            // radStopAssigning
            // 
            this.radStopAssigning.AutoSize = true;
            this.radStopAssigning.Location = new System.Drawing.Point(3, 3);
            this.radStopAssigning.Name = "radStopAssigning";
            this.radStopAssigning.Size = new System.Drawing.Size(177, 17);
            this.radStopAssigning.TabIndex = 11;
            this.radStopAssigning.TabStop = true;
            this.radStopAssigning.Text = "Stop assigning at the last frame";
            this.radStopAssigning.UseVisualStyleBackColor = true;
            // 
            // chkReverse
            // 
            this.chkReverse.AutoSize = true;
            this.chkReverse.Location = new System.Drawing.Point(21, 258);
            this.chkReverse.Name = "chkReverse";
            this.chkReverse.Size = new System.Drawing.Size(161, 17);
            this.chkReverse.TabIndex = 13;
            this.chkReverse.Text = "Add frames in reverse order";
            this.chkReverse.UseVisualStyleBackColor = true;
            // 
            // chkFlipped
            // 
            this.chkFlipped.AutoSize = true;
            this.chkFlipped.Location = new System.Drawing.Point(21, 237);
            this.chkFlipped.Name = "chkFlipped";
            this.chkFlipped.Size = new System.Drawing.Size(165, 17);
            this.chkFlipped.TabIndex = 12;
            this.chkFlipped.Text = "Set all new frames as Flipped";
            this.chkFlipped.UseVisualStyleBackColor = true;
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.Location = new System.Drawing.Point(16, 175);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(228, 13);
            this.label5.TabIndex = 9;
            this.label5.Text = "If there aren\'t enough frames available, then:";
            // 
            // radAddToExisting
            // 
            this.radAddToExisting.AutoSize = true;
            this.radAddToExisting.Location = new System.Drawing.Point(25, 150);
            this.radAddToExisting.Name = "radAddToExisting";
            this.radAddToExisting.Size = new System.Drawing.Size(156, 17);
            this.radAddToExisting.TabIndex = 8;
            this.radAddToExisting.TabStop = true;
            this.radAddToExisting.Text = "Add frames to existing loop";
            this.radAddToExisting.UseVisualStyleBackColor = true;
            // 
            // radOverwrite
            // 
            this.radOverwrite.AutoSize = true;
            this.radOverwrite.Location = new System.Drawing.Point(25, 131);
            this.radOverwrite.Name = "radOverwrite";
            this.radOverwrite.Size = new System.Drawing.Size(136, 17);
            this.radOverwrite.TabIndex = 7;
            this.radOverwrite.TabStop = true;
            this.radOverwrite.Text = "Overwrite existing loop";
            this.radOverwrite.UseVisualStyleBackColor = true;
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(18, 114);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(227, 13);
            this.label4.TabIndex = 6;
            this.label4.Text = "How would you like to assign the new frames?";
            // 
            // cmbLoop
            // 
            this.cmbLoop.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cmbLoop.FormattingEnabled = true;
            this.cmbLoop.Location = new System.Drawing.Point(64, 78);
            this.cmbLoop.Name = "cmbLoop";
            this.cmbLoop.Size = new System.Drawing.Size(136, 21);
            this.cmbLoop.TabIndex = 5;
            // 
            // btnChooseView
            // 
            this.btnChooseView.Location = new System.Drawing.Point(134, 50);
            this.btnChooseView.Name = "btnChooseView";
            this.btnChooseView.Size = new System.Drawing.Size(65, 21);
            this.btnChooseView.TabIndex = 4;
            this.btnChooseView.Text = "Choose...";
            this.btnChooseView.UseVisualStyleBackColor = true;
            this.btnChooseView.Click += new System.EventHandler(this.btnChooseView_Click);
            // 
            // udView
            // 
            this.udView.Location = new System.Drawing.Point(64, 50);
            this.udView.Name = "udView";
            this.udView.Size = new System.Drawing.Size(64, 21);
            this.udView.TabIndex = 3;
            this.udView.ValueChanged += new System.EventHandler(this.udView_ValueChanged);
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(20, 79);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(34, 13);
            this.label3.TabIndex = 2;
            this.label3.Text = "Loop:";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(20, 54);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(33, 13);
            this.label2.TabIndex = 1;
            this.label2.Text = "View:";
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(16, 17);
            this.label1.MaximumSize = new System.Drawing.Size(280, 0);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(261, 26);
            this.label1.TabIndex = 0;
            this.label1.Text = "The selected sprites will be assigned to the view you choose below:";
            // 
            // btnOK
            // 
            this.btnOK.Location = new System.Drawing.Point(16, 305);
            this.btnOK.Name = "btnOK";
            this.btnOK.Size = new System.Drawing.Size(82, 22);
            this.btnOK.TabIndex = 1;
            this.btnOK.Text = "OK";
            this.btnOK.UseVisualStyleBackColor = true;
            this.btnOK.Click += new System.EventHandler(this.btnOK_Click);
            // 
            // btnCancel
            // 
            this.btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnCancel.Location = new System.Drawing.Point(111, 305);
            this.btnCancel.Name = "btnCancel";
            this.btnCancel.Size = new System.Drawing.Size(82, 21);
            this.btnCancel.TabIndex = 2;
            this.btnCancel.Text = "Cancel";
            this.btnCancel.UseVisualStyleBackColor = true;
            this.btnCancel.Click += new System.EventHandler(this.btnCancel_Click);
            // 
            // AssignToView
            // 
            this.AcceptButton = this.btnOK;
            this.AutoScaleDimensions = new System.Drawing.SizeF(96F, 96F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
            this.CancelButton = this.btnCancel;
            this.ClientSize = new System.Drawing.Size(322, 336);
            this.Controls.Add(this.btnCancel);
            this.Controls.Add(this.btnOK);
            this.Controls.Add(this.groupBox1);
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "AssignToView";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Assign Sprites to View";
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.panel1.ResumeLayout(false);
            this.panel1.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.udView)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.RadioButton radAddToExisting;
        private System.Windows.Forms.RadioButton radOverwrite;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.ComboBox cmbLoop;
        private System.Windows.Forms.Button btnChooseView;
        private System.Windows.Forms.NumericUpDown udView;
        private System.Windows.Forms.CheckBox chkReverse;
        private System.Windows.Forms.CheckBox chkFlipped;
        private System.Windows.Forms.Label label5;
        private System.Windows.Forms.Button btnOK;
        private System.Windows.Forms.Button btnCancel;
        private System.Windows.Forms.Panel panel1;
        private System.Windows.Forms.RadioButton radOverwriteNextLoop;
        private System.Windows.Forms.RadioButton radStopAssigning;
    }
}