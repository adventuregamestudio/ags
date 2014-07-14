namespace AGS.Editor
{
    partial class FindReplaceDialog
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
            this.label1 = new System.Windows.Forms.Label();
            this.lblReplaceWith = new System.Windows.Forms.Label();
            this.btnOK = new System.Windows.Forms.Button();
            this.btnCancel = new System.Windows.Forms.Button();
            this.cmdToggleReplace = new System.Windows.Forms.Button();
            this.btnReplace = new System.Windows.Forms.Button();
            this.chkCaseSensitive = new System.Windows.Forms.CheckBox();
            this.cmbFind = new System.Windows.Forms.ComboBox();
            this.cmbLookIn = new System.Windows.Forms.ComboBox();
            this.lblLookIn = new System.Windows.Forms.Label();
            this.cmbReplace = new System.Windows.Forms.ComboBox();
            this.SuspendLayout();
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(12, 19);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(61, 13);
            this.label1.TabIndex = 0;
            this.label1.Text = "Search for:";
            // 
            // lblReplaceWith
            // 
            this.lblReplaceWith.AutoSize = true;
            this.lblReplaceWith.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lblReplaceWith.Location = new System.Drawing.Point(12, 51);
            this.lblReplaceWith.Name = "lblReplaceWith";
            this.lblReplaceWith.Size = new System.Drawing.Size(72, 13);
            this.lblReplaceWith.TabIndex = 1;
            this.lblReplaceWith.Text = "Replace with:";
            this.lblReplaceWith.Visible = false;
            // 
            // btnOK
            // 
            this.btnOK.DialogResult = System.Windows.Forms.DialogResult.OK;
            this.btnOK.Location = new System.Drawing.Point(12, 147);
            this.btnOK.Name = "btnOK";
            this.btnOK.Size = new System.Drawing.Size(103, 26);
            this.btnOK.TabIndex = 6;
            this.btnOK.Text = "&Find Next";
            this.btnOK.UseVisualStyleBackColor = true;
            this.btnOK.Click += new System.EventHandler(this.btnOK_Click);
            // 
            // btnCancel
            // 
            this.btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnCancel.Location = new System.Drawing.Point(270, 147);
            this.btnCancel.Name = "btnCancel";
            this.btnCancel.Size = new System.Drawing.Size(103, 26);
            this.btnCancel.TabIndex = 8;
            this.btnCancel.Text = "&Cancel";
            this.btnCancel.UseVisualStyleBackColor = true;
            this.btnCancel.Click += new System.EventHandler(this.btnCancel_Click);
            // 
            // cmdToggleReplace
            // 
            this.cmdToggleReplace.Location = new System.Drawing.Point(298, 15);
            this.cmdToggleReplace.Name = "cmdToggleReplace";
            this.cmdToggleReplace.Size = new System.Drawing.Size(75, 21);
            this.cmdToggleReplace.TabIndex = 2;
            this.cmdToggleReplace.Text = ">> R&eplace";
            this.cmdToggleReplace.UseVisualStyleBackColor = true;
            this.cmdToggleReplace.Click += new System.EventHandler(this.cmdToggleReplace_Click);
            // 
            // btnReplace
            // 
            this.btnReplace.DialogResult = System.Windows.Forms.DialogResult.OK;
            this.btnReplace.Location = new System.Drawing.Point(141, 147);
            this.btnReplace.Name = "btnReplace";
            this.btnReplace.Size = new System.Drawing.Size(103, 26);
            this.btnReplace.TabIndex = 7;
            this.btnReplace.Text = "&Replace";
            this.btnReplace.UseVisualStyleBackColor = true;
            this.btnReplace.Visible = false;
            this.btnReplace.Click += new System.EventHandler(this.btnReplace_Click);
            // 
            // chkCaseSensitive
            // 
            this.chkCaseSensitive.AutoSize = true;
            this.chkCaseSensitive.Location = new System.Drawing.Point(15, 115);
            this.chkCaseSensitive.Name = "chkCaseSensitive";
            this.chkCaseSensitive.Size = new System.Drawing.Size(96, 17);
            this.chkCaseSensitive.TabIndex = 5;
            this.chkCaseSensitive.Text = "C&ase Sensitive";
            this.chkCaseSensitive.UseVisualStyleBackColor = true;
            // 
            // cmbFind
            // 
            this.cmbFind.AutoCompleteSource = System.Windows.Forms.AutoCompleteSource.ListItems;
            this.cmbFind.FormattingEnabled = true;
            this.cmbFind.Location = new System.Drawing.Point(89, 15);
            this.cmbFind.Name = "cmbFind";
            this.cmbFind.Size = new System.Drawing.Size(203, 21);
            this.cmbFind.TabIndex = 1;
            this.cmbFind.TextChanged += new System.EventHandler(this.cmbFind_TextChanged);
            // 
            // cmbLookIn
            // 
            this.cmbLookIn.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cmbLookIn.Location = new System.Drawing.Point(89, 81);
            this.cmbLookIn.Name = "cmbLookIn";
            this.cmbLookIn.Size = new System.Drawing.Size(203, 21);
            this.cmbLookIn.TabIndex = 4;
            // 
            // lblLookIn
            // 
            this.lblLookIn.AutoSize = true;
            this.lblLookIn.Location = new System.Drawing.Point(12, 83);
            this.lblLookIn.Name = "lblLookIn";
            this.lblLookIn.Size = new System.Drawing.Size(46, 13);
            this.lblLookIn.TabIndex = 8;
            this.lblLookIn.Text = "Look In:";
            // 
            // cmbReplace
            // 
            this.cmbReplace.AutoCompleteSource = System.Windows.Forms.AutoCompleteSource.ListItems;
            this.cmbReplace.FormattingEnabled = true;
            this.cmbReplace.Location = new System.Drawing.Point(89, 48);
            this.cmbReplace.Name = "cmbReplace";
            this.cmbReplace.Size = new System.Drawing.Size(203, 21);
            this.cmbReplace.TabIndex = 3;
            // 
            // FindReplaceDialog
            // 
            this.AcceptButton = this.btnOK;
            this.AutoScaleDimensions = new System.Drawing.SizeF(96F, 96F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
            this.CancelButton = this.btnCancel;
            this.ClientSize = new System.Drawing.Size(386, 181);
            this.Controls.Add(this.cmbReplace);
            this.Controls.Add(this.cmbLookIn);
            this.Controls.Add(this.lblLookIn);
            this.Controls.Add(this.cmbFind);
            this.Controls.Add(this.chkCaseSensitive);
            this.Controls.Add(this.btnReplace);
            this.Controls.Add(this.cmdToggleReplace);
            this.Controls.Add(this.btnCancel);
            this.Controls.Add(this.btnOK);
            this.Controls.Add(this.lblReplaceWith);
            this.Controls.Add(this.label1);
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "FindReplaceDialog";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Find and Replace";
            this.TopMost = true;
            this.Deactivate += new System.EventHandler(this.onFormDeactivated);
            this.Activated += new System.EventHandler(this.onFormActivated);
            this.FormClosed += new System.Windows.Forms.FormClosedEventHandler(this.onFormClosed);
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.FindReplaceDialog_FormClosing);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label lblReplaceWith;
        private System.Windows.Forms.Button btnOK;
        private System.Windows.Forms.Button btnCancel;
        private System.Windows.Forms.Button cmdToggleReplace;
        private System.Windows.Forms.Button btnReplace;
        private System.Windows.Forms.CheckBox chkCaseSensitive;
        private System.Windows.Forms.ComboBox cmbFind;
        private System.Windows.Forms.ComboBox cmbLookIn;
        private System.Windows.Forms.Label lblLookIn;
        private System.Windows.Forms.ComboBox cmbReplace;
    }
}