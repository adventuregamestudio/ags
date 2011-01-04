namespace AGS.Editor
{
    partial class DialogOptionEditor
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
            this.lblOptionID = new System.Windows.Forms.Label();
            this.txtOptionText = new System.Windows.Forms.TextBox();
            this.chkShow = new System.Windows.Forms.CheckBox();
            this.chkSay = new System.Windows.Forms.CheckBox();
            this.SuspendLayout();
            // 
            // lblOptionID
            // 
            this.lblOptionID.AutoSize = true;
            this.lblOptionID.Location = new System.Drawing.Point(7, 6);
            this.lblOptionID.Name = "lblOptionID";
            this.lblOptionID.Size = new System.Drawing.Size(16, 13);
            this.lblOptionID.TabIndex = 0;
            this.lblOptionID.Text = "0:";
            // 
            // txtOptionText
            // 
            this.txtOptionText.Location = new System.Drawing.Point(29, 3);
            this.txtOptionText.Name = "txtOptionText";
            this.txtOptionText.Size = new System.Drawing.Size(230, 20);
            this.txtOptionText.TabIndex = 1;
            // 
            // chkShow
            // 
            this.chkShow.AutoSize = true;
            this.chkShow.Location = new System.Drawing.Point(274, 6);
            this.chkShow.Name = "chkShow";
            this.chkShow.Size = new System.Drawing.Size(15, 14);
            this.chkShow.TabIndex = 2;
            this.chkShow.UseVisualStyleBackColor = true;
            // 
            // chkSay
            // 
            this.chkSay.AutoSize = true;
            this.chkSay.Location = new System.Drawing.Point(305, 6);
            this.chkSay.Name = "chkSay";
            this.chkSay.Size = new System.Drawing.Size(15, 14);
            this.chkSay.TabIndex = 3;
            this.chkSay.UseVisualStyleBackColor = true;
            // 
            // DialogOptionEditor
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.chkSay);
            this.Controls.Add(this.chkShow);
            this.Controls.Add(this.txtOptionText);
            this.Controls.Add(this.lblOptionID);
            this.Margin = new System.Windows.Forms.Padding(3, 0, 3, 0);
            this.Name = "DialogOptionEditor";
            this.Size = new System.Drawing.Size(330, 26);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label lblOptionID;
        private System.Windows.Forms.TextBox txtOptionText;
        private System.Windows.Forms.CheckBox chkShow;
        private System.Windows.Forms.CheckBox chkSay;
    }
}
