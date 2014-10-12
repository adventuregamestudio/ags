namespace AGS.Editor
{
    partial class CustomResolutionDialog
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
            this.udWidth = new System.Windows.Forms.NumericUpDown();
            this.label2 = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.udHeight = new System.Windows.Forms.NumericUpDown();
            this.btnCancel = new System.Windows.Forms.Button();
            this.btnOK = new System.Windows.Forms.Button();
            this.label1 = new System.Windows.Forms.Label();
            this.cbResolutionPreset = new System.Windows.Forms.ComboBox();
            ((System.ComponentModel.ISupportInitialize)(this.udWidth)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.udHeight)).BeginInit();
            this.SuspendLayout();
            // 
            // udWidth
            // 
            this.udWidth.AutoSize = true;
            this.udWidth.Location = new System.Drawing.Point(95, 89);
            this.udWidth.Maximum = new decimal(new int[] {
            65535,
            0,
            0,
            0});
            this.udWidth.Name = "udWidth";
            this.udWidth.Size = new System.Drawing.Size(120, 20);
            this.udWidth.TabIndex = 2;
            this.udWidth.TextChanged += new System.EventHandler(this.udWidthHeight_ValueChanged);
            this.udWidth.ValueChanged += new System.EventHandler(this.udWidthHeight_ValueChanged);
            this.udWidth.Validating += new System.ComponentModel.CancelEventHandler(this.udWidthHeight_Validating);
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(36, 91);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(38, 13);
            this.label2.TabIndex = 1;
            this.label2.Text = "Width:";
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(36, 117);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(41, 13);
            this.label3.TabIndex = 3;
            this.label3.Text = "Height:";
            // 
            // udHeight
            // 
            this.udHeight.AutoSize = true;
            this.udHeight.Location = new System.Drawing.Point(95, 115);
            this.udHeight.Maximum = new decimal(new int[] {
            65535,
            0,
            0,
            0});
            this.udHeight.Name = "udHeight";
            this.udHeight.Size = new System.Drawing.Size(120, 20);
            this.udHeight.TabIndex = 4;
            this.udHeight.TextChanged += new System.EventHandler(this.udWidthHeight_ValueChanged);
            this.udHeight.ValueChanged += new System.EventHandler(this.udWidthHeight_ValueChanged);
            this.udHeight.Validating += new System.ComponentModel.CancelEventHandler(this.udWidthHeight_Validating);
            // 
            // btnCancel
            // 
            this.btnCancel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnCancel.Location = new System.Drawing.Point(288, 163);
            this.btnCancel.Name = "btnCancel";
            this.btnCancel.Size = new System.Drawing.Size(94, 22);
            this.btnCancel.TabIndex = 6;
            this.btnCancel.Text = "Cancel";
            this.btnCancel.UseVisualStyleBackColor = true;
            // 
            // btnOK
            // 
            this.btnOK.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnOK.DialogResult = System.Windows.Forms.DialogResult.OK;
            this.btnOK.Location = new System.Drawing.Point(188, 163);
            this.btnOK.Name = "btnOK";
            this.btnOK.Size = new System.Drawing.Size(94, 22);
            this.btnOK.TabIndex = 5;
            this.btnOK.Text = "OK";
            this.btnOK.UseVisualStyleBackColor = true;
            // 
            // label1
            // 
            this.label1.Location = new System.Drawing.Point(12, 9);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(370, 44);
            this.label1.TabIndex = 7;
            this.label1.Text = "Select one of the standard resolution presets, or type in custom width and height" +
                ".";
            // 
            // cbResolutionPreset
            // 
            this.cbResolutionPreset.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cbResolutionPreset.FormattingEnabled = true;
            this.cbResolutionPreset.Location = new System.Drawing.Point(12, 56);
            this.cbResolutionPreset.Name = "cbResolutionPreset";
            this.cbResolutionPreset.Size = new System.Drawing.Size(370, 21);
            this.cbResolutionPreset.TabIndex = 8;
            this.cbResolutionPreset.SelectedIndexChanged += new System.EventHandler(this.cbResolutionPreset_SelectedIndexChanged);
            // 
            // CustomResolutionDialog
            // 
            this.AcceptButton = this.btnOK;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this.btnCancel;
            this.ClientSize = new System.Drawing.Size(394, 197);
            this.Controls.Add(this.cbResolutionPreset);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.btnCancel);
            this.Controls.Add(this.btnOK);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.udHeight);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.udWidth);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "CustomResolutionDialog";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Game resolution";
            ((System.ComponentModel.ISupportInitialize)(this.udWidth)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.udHeight)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.NumericUpDown udWidth;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.NumericUpDown udHeight;
        private System.Windows.Forms.Button btnCancel;
        private System.Windows.Forms.Button btnOK;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.ComboBox cbResolutionPreset;
    }
}