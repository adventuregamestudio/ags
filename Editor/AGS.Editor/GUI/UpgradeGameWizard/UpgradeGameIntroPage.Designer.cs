
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
            this.richDescription = new AGS.Controls.ReadOnlyRichTextBox();
            this.lblBackupLocation = new System.Windows.Forms.Label();
            this.chkBackup = new System.Windows.Forms.CheckBox();
            this.tableLayoutPanel1 = new System.Windows.Forms.TableLayoutPanel();
            this.panel1 = new System.Windows.Forms.Panel();
            this.btnBrowse = new System.Windows.Forms.Button();
            this.tbBackupPath = new System.Windows.Forms.TextBox();
            this.tableLayoutPanel1.SuspendLayout();
            this.panel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // richDescription
            // 
            this.richDescription.BackColor = System.Drawing.SystemColors.Control;
            this.richDescription.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.richDescription.Cursor = System.Windows.Forms.Cursors.Default;
            this.richDescription.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this.richDescription.Location = new System.Drawing.Point(3, 3);
            this.richDescription.Name = "richDescription";
            this.richDescription.ScrollBars = System.Windows.Forms.RichTextBoxScrollBars.None;
            this.richDescription.Size = new System.Drawing.Size(515, 184);
            this.richDescription.TabIndex = 0;
            this.richDescription.Text = resources.GetString("richDescription.Text");
            // 
            // lblBackupLocation
            // 
            this.lblBackupLocation.AutoSize = true;
            this.lblBackupLocation.Location = new System.Drawing.Point(15, 33);
            this.lblBackupLocation.Name = "lblBackupLocation";
            this.lblBackupLocation.Size = new System.Drawing.Size(189, 13);
            this.lblBackupLocation.TabIndex = 5;
            this.lblBackupLocation.Text = "Backup files will be placed in directory:";
            // 
            // chkBackup
            // 
            this.chkBackup.AutoSize = true;
            this.chkBackup.Checked = true;
            this.chkBackup.CheckState = System.Windows.Forms.CheckState.Checked;
            this.chkBackup.Location = new System.Drawing.Point(18, 13);
            this.chkBackup.Name = "chkBackup";
            this.chkBackup.Size = new System.Drawing.Size(208, 17);
            this.chkBackup.TabIndex = 4;
            this.chkBackup.Text = "Make a backup copy of my game files.";
            this.chkBackup.UseVisualStyleBackColor = true;
            this.chkBackup.CheckedChanged += new System.EventHandler(this.chkBackup_CheckedChanged);
            // 
            // tableLayoutPanel1
            // 
            this.tableLayoutPanel1.ColumnCount = 1;
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.tableLayoutPanel1.Controls.Add(this.panel1, 0, 2);
            this.tableLayoutPanel1.Controls.Add(this.richDescription, 0, 0);
            this.tableLayoutPanel1.Location = new System.Drawing.Point(16, 16);
            this.tableLayoutPanel1.Name = "tableLayoutPanel1";
            this.tableLayoutPanel1.RowCount = 3;
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 10F));
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanel1.Size = new System.Drawing.Size(572, 340);
            this.tableLayoutPanel1.TabIndex = 6;
            // 
            // panel1
            // 
            this.panel1.Controls.Add(this.btnBrowse);
            this.panel1.Controls.Add(this.tbBackupPath);
            this.panel1.Controls.Add(this.lblBackupLocation);
            this.panel1.Controls.Add(this.chkBackup);
            this.panel1.Location = new System.Drawing.Point(3, 203);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(515, 85);
            this.panel1.TabIndex = 7;
            // 
            // btnBrowse
            // 
            this.btnBrowse.Location = new System.Drawing.Point(430, 48);
            this.btnBrowse.Name = "btnBrowse";
            this.btnBrowse.Size = new System.Drawing.Size(37, 22);
            this.btnBrowse.TabIndex = 7;
            this.btnBrowse.Text = "...";
            this.btnBrowse.UseVisualStyleBackColor = true;
            this.btnBrowse.Click += new System.EventHandler(this.btnBrowse_Click);
            // 
            // tbBackupPath
            // 
            this.tbBackupPath.Location = new System.Drawing.Point(18, 50);
            this.tbBackupPath.Name = "tbBackupPath";
            this.tbBackupPath.ReadOnly = true;
            this.tbBackupPath.Size = new System.Drawing.Size(406, 20);
            this.tbBackupPath.TabIndex = 6;
            // 
            // UpgradeGameIntroPage
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.tableLayoutPanel1);
            this.MinimumSize = new System.Drawing.Size(640, 320);
            this.Name = "UpgradeGameIntroPage";
            this.Padding = new System.Windows.Forms.Padding(13);
            this.Size = new System.Drawing.Size(1358, 586);
            this.Load += new System.EventHandler(this.UpgradeGameIntroPage_Load);
            this.VisibleChanged += new System.EventHandler(this.UpgradeGameIntroPage_VisibleChanged);
            this.tableLayoutPanel1.ResumeLayout(false);
            this.panel1.ResumeLayout(false);
            this.panel1.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private AGS.Controls.ReadOnlyRichTextBox richDescription;
        private System.Windows.Forms.Label lblBackupLocation;
        private System.Windows.Forms.CheckBox chkBackup;
        private System.Windows.Forms.TableLayoutPanel tableLayoutPanel1;
        private System.Windows.Forms.Panel panel1;
        private System.Windows.Forms.Button btnBrowse;
        private System.Windows.Forms.TextBox tbBackupPath;
    }
}
