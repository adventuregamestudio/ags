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
            this.rbNominalSize = new System.Windows.Forms.RadioButton();
            this.rbRealPixelHeight = new System.Windows.Forms.RadioButton();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.udFullHeight = new System.Windows.Forms.NumericUpDown();
            ((System.ComponentModel.ISupportInitialize)(this.udSize)).BeginInit();
            this.groupBox1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.udFullHeight)).BeginInit();
            this.SuspendLayout();
            // 
            // btnCancel
            // 
            this.btnCancel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnCancel.Location = new System.Drawing.Point(277, 131);
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
            this.btnOK.Location = new System.Drawing.Point(177, 131);
            this.btnOK.Name = "btnOK";
            this.btnOK.Size = new System.Drawing.Size(94, 22);
            this.btnOK.TabIndex = 4;
            this.btnOK.Text = "OK";
            this.btnOK.UseVisualStyleBackColor = true;
            // 
            // udSize
            // 
            this.udSize.Location = new System.Drawing.Point(238, 13);
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
            this.lblHeader.Text = "Select the method to import this TTF font:";
            // 
            // rbNominalSize
            // 
            this.rbNominalSize.AutoSize = true;
            this.rbNominalSize.Checked = true;
            this.rbNominalSize.Location = new System.Drawing.Point(6, 13);
            this.rbNominalSize.Name = "rbNominalSize";
            this.rbNominalSize.Size = new System.Drawing.Size(87, 17);
            this.rbNominalSize.TabIndex = 1;
            this.rbNominalSize.TabStop = true;
            this.rbNominalSize.Text = "Nominal size:";
            this.rbNominalSize.UseVisualStyleBackColor = true;
            this.rbNominalSize.CheckedChanged += new System.EventHandler(this.rb_CheckedChanged);
            // 
            // rbRealPixelHeight
            // 
            this.rbRealPixelHeight.AutoSize = true;
            this.rbRealPixelHeight.Location = new System.Drawing.Point(6, 45);
            this.rbRealPixelHeight.Name = "rbRealPixelHeight";
            this.rbRealPixelHeight.Size = new System.Drawing.Size(187, 17);
            this.rbRealPixelHeight.TabIndex = 2;
            this.rbRealPixelHeight.TabStop = true;
            this.rbRealPixelHeight.Text = "Find closest size for this full height:";
            this.rbRealPixelHeight.UseVisualStyleBackColor = true;
            this.rbRealPixelHeight.CheckedChanged += new System.EventHandler(this.rb_CheckedChanged);
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.udFullHeight);
            this.groupBox1.Controls.Add(this.rbNominalSize);
            this.groupBox1.Controls.Add(this.rbRealPixelHeight);
            this.groupBox1.Controls.Add(this.udSize);
            this.groupBox1.Location = new System.Drawing.Point(12, 26);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(359, 79);
            this.groupBox1.TabIndex = 7;
            this.groupBox1.TabStop = false;
            // 
            // udFullHeight
            // 
            this.udFullHeight.Location = new System.Drawing.Point(238, 45);
            this.udFullHeight.Name = "udFullHeight";
            this.udFullHeight.Size = new System.Drawing.Size(115, 20);
            this.udFullHeight.TabIndex = 4;
            // 
            // ImportTTFDialog
            // 
            this.AcceptButton = this.btnOK;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this.btnCancel;
            this.ClientSize = new System.Drawing.Size(383, 165);
            this.Controls.Add(this.groupBox1);
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
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.udFullHeight)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Button btnCancel;
        private System.Windows.Forms.Button btnOK;
        private System.Windows.Forms.NumericUpDown udSize;
        private System.Windows.Forms.Label lblHeader;
        private System.Windows.Forms.RadioButton rbNominalSize;
        private System.Windows.Forms.RadioButton rbRealPixelHeight;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.NumericUpDown udFullHeight;
    }
}