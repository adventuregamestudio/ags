namespace AGS.Editor
{
    partial class AdjustMasksDialog
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
            this.descLabel = new System.Windows.Forms.Label();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.radioScale = new System.Windows.Forms.RadioButton();
            this.radioResize = new System.Windows.Forms.RadioButton();
            this.radioReset = new System.Windows.Forms.RadioButton();
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.panel1 = new System.Windows.Forms.Panel();
            this.yOffset = new System.Windows.Forms.NumericUpDown();
            this.xOffset = new System.Windows.Forms.NumericUpDown();
            this.label2 = new System.Windows.Forms.Label();
            this.label1 = new System.Windows.Forms.Label();
            this.btnOK = new System.Windows.Forms.Button();
            this.btnCancel = new System.Windows.Forms.Button();
            this.groupBox1.SuspendLayout();
            this.groupBox2.SuspendLayout();
            this.panel1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.yOffset)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.xOffset)).BeginInit();
            this.SuspendLayout();
            // 
            // descLabel
            // 
            this.descLabel.Location = new System.Drawing.Point(12, 9);
            this.descLabel.Name = "descLabel";
            this.descLabel.Size = new System.Drawing.Size(526, 44);
            this.descLabel.TabIndex = 0;
            this.descLabel.Text = "The new background has a different size.  All the room masks will have to be resi" +
    "zed.  Please decide what to do with their contents:";
            // 
            // groupBox1
            // 
            this.groupBox1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.groupBox1.Controls.Add(this.radioScale);
            this.groupBox1.Controls.Add(this.radioResize);
            this.groupBox1.Controls.Add(this.radioReset);
            this.groupBox1.Location = new System.Drawing.Point(15, 56);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(523, 99);
            this.groupBox1.TabIndex = 1;
            this.groupBox1.TabStop = false;
            // 
            // radioScale
            // 
            this.radioScale.AutoSize = true;
            this.radioScale.Location = new System.Drawing.Point(17, 66);
            this.radioScale.Name = "radioScale";
            this.radioScale.Size = new System.Drawing.Size(253, 17);
            this.radioScale.TabIndex = 2;
            this.radioScale.Text = "Rescale (stretch existing areas to the new size)";
            this.radioScale.UseVisualStyleBackColor = true;
            // 
            // radioResize
            // 
            this.radioResize.AutoSize = true;
            this.radioResize.Checked = true;
            this.radioResize.Location = new System.Drawing.Point(17, 43);
            this.radioResize.Name = "radioResize";
            this.radioResize.Size = new System.Drawing.Size(289, 17);
            this.radioResize.TabIndex = 1;
            this.radioResize.TabStop = true;
            this.radioResize.Text = "Resize canvas (extend or crop, keeping existing areas)";
            this.radioResize.UseVisualStyleBackColor = true;
            // 
            // radioReset
            // 
            this.radioReset.AutoSize = true;
            this.radioReset.Location = new System.Drawing.Point(17, 20);
            this.radioReset.Name = "radioReset";
            this.radioReset.Size = new System.Drawing.Size(134, 17);
            this.radioReset.TabIndex = 0;
            this.radioReset.Text = "Reset (erase all areas)";
            this.radioReset.UseVisualStyleBackColor = true;
            // 
            // groupBox2
            // 
            this.groupBox2.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.groupBox2.Controls.Add(this.panel1);
            this.groupBox2.Location = new System.Drawing.Point(15, 161);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Size = new System.Drawing.Size(523, 88);
            this.groupBox2.TabIndex = 2;
            this.groupBox2.TabStop = false;
            this.groupBox2.Text = "Mask Offset";
            // 
            // panel1
            // 
            this.panel1.Controls.Add(this.yOffset);
            this.panel1.Controls.Add(this.xOffset);
            this.panel1.Controls.Add(this.label2);
            this.panel1.Controls.Add(this.label1);
            this.panel1.Location = new System.Drawing.Point(17, 20);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(224, 57);
            this.panel1.TabIndex = 1;
            // 
            // yOffset
            // 
            this.yOffset.Location = new System.Drawing.Point(87, 33);
            this.yOffset.Maximum = new decimal(new int[] {
            999999999,
            0,
            0,
            0});
            this.yOffset.Minimum = new decimal(new int[] {
            999999999,
            0,
            0,
            -2147483648});
            this.yOffset.Name = "yOffset";
            this.yOffset.Size = new System.Drawing.Size(120, 21);
            this.yOffset.TabIndex = 3;
            // 
            // xOffset
            // 
            this.xOffset.Location = new System.Drawing.Point(87, 6);
            this.xOffset.Maximum = new decimal(new int[] {
            999999999,
            0,
            0,
            0});
            this.xOffset.Minimum = new decimal(new int[] {
            999999999,
            0,
            0,
            -2147483648});
            this.xOffset.Name = "xOffset";
            this.xOffset.Size = new System.Drawing.Size(120, 21);
            this.xOffset.TabIndex = 2;
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(14, 35);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(51, 13);
            this.label2.TabIndex = 2;
            this.label2.Text = "Y Offset:";
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(14, 8);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(51, 13);
            this.label1.TabIndex = 1;
            this.label1.Text = "X Offset:";
            // 
            // btnOK
            // 
            this.btnOK.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.btnOK.Location = new System.Drawing.Point(12, 263);
            this.btnOK.Name = "btnOK";
            this.btnOK.Size = new System.Drawing.Size(109, 29);
            this.btnOK.TabIndex = 3;
            this.btnOK.Text = "&OK";
            this.btnOK.UseVisualStyleBackColor = true;
            this.btnOK.Click += new System.EventHandler(this.btnOK_Click);
            // 
            // btnCancel
            // 
            this.btnCancel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnCancel.Location = new System.Drawing.Point(128, 263);
            this.btnCancel.Name = "btnCancel";
            this.btnCancel.Size = new System.Drawing.Size(102, 29);
            this.btnCancel.TabIndex = 4;
            this.btnCancel.Text = "&Cancel";
            this.btnCancel.UseVisualStyleBackColor = true;
            // 
            // AdjustMasksDialog
            // 
            this.AcceptButton = this.btnOK;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this.btnCancel;
            this.ClientSize = new System.Drawing.Size(550, 304);
            this.Controls.Add(this.btnCancel);
            this.Controls.Add(this.btnOK);
            this.Controls.Add(this.groupBox2);
            this.Controls.Add(this.groupBox1);
            this.Controls.Add(this.descLabel);
            this.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "AdjustMasksDialog";
            this.ShowIcon = false;
            this.Text = "Adjust Masks to the new Room background";
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.groupBox2.ResumeLayout(false);
            this.panel1.ResumeLayout(false);
            this.panel1.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.yOffset)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.xOffset)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Label descLabel;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.RadioButton radioScale;
        private System.Windows.Forms.RadioButton radioResize;
        private System.Windows.Forms.RadioButton radioReset;
        private System.Windows.Forms.GroupBox groupBox2;
        private System.Windows.Forms.Panel panel1;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Button btnOK;
        private System.Windows.Forms.Button btnCancel;
        private System.Windows.Forms.NumericUpDown yOffset;
        private System.Windows.Forms.NumericUpDown xOffset;
    }
}
