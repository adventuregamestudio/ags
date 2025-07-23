
namespace AGS.Editor
{
    partial class UpgradeGameIntroPage
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(UpgradeGameIntroPage));
            this.richDescription = new System.Windows.Forms.RichTextBox();
            this.lblBackupLocation = new System.Windows.Forms.Label();
            this.chkBackup = new System.Windows.Forms.CheckBox();
            this.SuspendLayout();
            // 
            // richDescription
            // 
            this.richDescription.BackColor = System.Drawing.SystemColors.Control;
            this.richDescription.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.richDescription.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this.richDescription.Location = new System.Drawing.Point(16, 16);
            this.richDescription.Name = "richDescription";
            this.richDescription.ReadOnly = true;
            this.richDescription.ScrollBars = System.Windows.Forms.RichTextBoxScrollBars.None;
            this.richDescription.Size = new System.Drawing.Size(515, 184);
            this.richDescription.TabIndex = 0;
            this.richDescription.Text = resources.GetString("richDescription.Text");
            // 
            // lblBackupLocation
            // 
            this.lblBackupLocation.AutoSize = true;
            this.lblBackupLocation.Location = new System.Drawing.Point(14, 244);
            this.lblBackupLocation.Name = "lblBackupLocation";
            this.lblBackupLocation.Size = new System.Drawing.Size(35, 13);
            this.lblBackupLocation.TabIndex = 5;
            this.lblBackupLocation.Text = "label3";
            // 
            // chkBackup
            // 
            this.chkBackup.AutoSize = true;
            this.chkBackup.Checked = true;
            this.chkBackup.CheckState = System.Windows.Forms.CheckState.Checked;
            this.chkBackup.Location = new System.Drawing.Point(16, 217);
            this.chkBackup.Name = "chkBackup";
            this.chkBackup.Size = new System.Drawing.Size(208, 17);
            this.chkBackup.TabIndex = 4;
            this.chkBackup.Text = "Make a backup copy of my game files.";
            this.chkBackup.UseVisualStyleBackColor = true;
            this.chkBackup.CheckedChanged += new System.EventHandler(this.chkBackup_CheckedChanged);
            // 
            // UpgradeGameIntroPage
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.lblBackupLocation);
            this.Controls.Add(this.chkBackup);
            this.Controls.Add(this.richDescription);
            this.Name = "UpgradeGameIntroPage";
            this.Padding = new System.Windows.Forms.Padding(13);
            this.Size = new System.Drawing.Size(1398, 628);
            this.Load += new System.EventHandler(this.UpgradeGameIntroPage_Load);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.RichTextBox richDescription;
        private System.Windows.Forms.Label lblBackupLocation;
        private System.Windows.Forms.CheckBox chkBackup;
    }
}
