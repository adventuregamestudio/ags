namespace AGS.Editor
{
    partial class ImportTTFDialog
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
            this.btnCancel = new System.Windows.Forms.Button();
            this.btnOK = new System.Windows.Forms.Button();
            this.udSize = new System.Windows.Forms.NumericUpDown();
            this.lblHeader = new System.Windows.Forms.Label();
            this.rbPointSize = new System.Windows.Forms.RadioButton();
            this.rbRealPixelHeight = new System.Windows.Forms.RadioButton();
            this.panel1 = new System.Windows.Forms.Panel();
            ((System.ComponentModel.ISupportInitialize)(this.udSize)).BeginInit();
            this.panel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // btnCancel
            // 
            this.btnCancel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnCancel.Location = new System.Drawing.Point(186, 131);
            this.btnCancel.Name = "btnCancel";
            this.btnCancel.Size = new System.Drawing.Size(94, 22);
            this.btnCancel.TabIndex = 5;
            this.btnCancel.Text = "Cancel";
            this.btnCancel.UseVisualStyleBackColor = true;
            // 
            // btnOK
            // 
            this.btnOK.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnOK.DialogResult = System.Windows.Forms.DialogResult.OK;
            this.btnOK.Location = new System.Drawing.Point(86, 131);
            this.btnOK.Name = "btnOK";
            this.btnOK.Size = new System.Drawing.Size(94, 22);
            this.btnOK.TabIndex = 4;
            this.btnOK.Text = "OK";
            this.btnOK.UseVisualStyleBackColor = true;
            // 
            // udSize
            // 
            this.udSize.Location = new System.Drawing.Point(22, 56);
            this.udSize.Name = "udSize";
            this.udSize.Size = new System.Drawing.Size(115, 20);
            this.udSize.TabIndex = 3;
            // 
            // lblHeader
            // 
            this.lblHeader.Location = new System.Drawing.Point(12, 9);
            this.lblHeader.Name = "lblHeader";
            this.lblHeader.Size = new System.Drawing.Size(268, 20);
            this.lblHeader.TabIndex = 0;
            this.lblHeader.Text = "Select the font size to import this TTF font at:";
            // 
            // rbPointSize
            // 
            this.rbPointSize.AutoSize = true;
            this.rbPointSize.Checked = true;
            this.rbPointSize.Location = new System.Drawing.Point(3, 3);
            this.rbPointSize.Name = "rbPointSize";
            this.rbPointSize.Size = new System.Drawing.Size(100, 17);
            this.rbPointSize.TabIndex = 1;
            this.rbPointSize.TabStop = true;
            this.rbPointSize.Text = "Font\'s point size";
            this.rbPointSize.UseVisualStyleBackColor = true;
            // 
            // rbRealPixelHeight
            // 
            this.rbRealPixelHeight.AutoSize = true;
            this.rbRealPixelHeight.Location = new System.Drawing.Point(3, 27);
            this.rbRealPixelHeight.Name = "rbRealPixelHeight";
            this.rbRealPixelHeight.Size = new System.Drawing.Size(189, 17);
            this.rbRealPixelHeight.TabIndex = 2;
            this.rbRealPixelHeight.Text = "Find closest size to this pixel height";
            this.rbRealPixelHeight.UseVisualStyleBackColor = true;
            // 
            // panel1
            // 
            this.panel1.Controls.Add(this.rbPointSize);
            this.panel1.Controls.Add(this.rbRealPixelHeight);
            this.panel1.Controls.Add(this.udSize);
            this.panel1.Location = new System.Drawing.Point(12, 32);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(268, 93);
            this.panel1.TabIndex = 6;
            // 
            // ImportTTFDialog
            // 
            this.AcceptButton = this.btnOK;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this.btnCancel;
            this.ClientSize = new System.Drawing.Size(292, 165);
            this.Controls.Add(this.panel1);
            this.Controls.Add(this.lblHeader);
            this.Controls.Add(this.btnCancel);
            this.Controls.Add(this.btnOK);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "ImportTTFDialog";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Import TTF font";
            ((System.ComponentModel.ISupportInitialize)(this.udSize)).EndInit();
            this.panel1.ResumeLayout(false);
            this.panel1.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Button btnCancel;
        private System.Windows.Forms.Button btnOK;
        private System.Windows.Forms.NumericUpDown udSize;
        private System.Windows.Forms.Label lblHeader;
        private System.Windows.Forms.RadioButton rbPointSize;
        private System.Windows.Forms.RadioButton rbRealPixelHeight;
        private System.Windows.Forms.Panel panel1;
    }
}