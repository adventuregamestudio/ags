using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace AGS.Editor
{
    partial class UpgradeGameVoiceClipsPage
    {
        private Controls.ReadOnlyRichTextBox richDescription;
        private System.Windows.Forms.CheckBox chkRenameVoiceClips;
        private System.Windows.Forms.TableLayoutPanel tableLayoutPanel1;
        private System.Windows.Forms.Panel panel1;

        private void InitializeComponent()
        {
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(UpgradeGameVoiceClipsPage));
            this.richDescription = new AGS.Controls.ReadOnlyRichTextBox();
            this.chkRenameVoiceClips = new System.Windows.Forms.CheckBox();
            this.tableLayoutPanel1 = new System.Windows.Forms.TableLayoutPanel();
            this.panel1 = new System.Windows.Forms.Panel();
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
            this.richDescription.Size = new System.Drawing.Size(515, 103);
            this.richDescription.TabIndex = 0;
            this.richDescription.Text = resources.GetString("richDescription.Text");
            // 
            // chkRenameVoiceClips
            // 
            this.chkRenameVoiceClips.AutoSize = true;
            this.chkRenameVoiceClips.Checked = true;
            this.chkRenameVoiceClips.CheckState = System.Windows.Forms.CheckState.Checked;
            this.chkRenameVoiceClips.Location = new System.Drawing.Point(18, 13);
            this.chkRenameVoiceClips.Name = "chkRenameVoiceClips";
            this.chkRenameVoiceClips.Size = new System.Drawing.Size(228, 17);
            this.chkRenameVoiceClips.TabIndex = 4;
            this.chkRenameVoiceClips.Text = "Rename voice clips to the new style format";
            this.chkRenameVoiceClips.UseVisualStyleBackColor = true;
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
            this.tableLayoutPanel1.TabIndex = 7;
            // 
            // panel1
            // 
            this.panel1.Controls.Add(this.chkRenameVoiceClips);
            this.panel1.Location = new System.Drawing.Point(3, 122);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(515, 40);
            this.panel1.TabIndex = 7;
            // 
            // UpgradeGameVoiceClipsPage
            // 
            this.Controls.Add(this.tableLayoutPanel1);
            this.Name = "UpgradeGameVoiceClipsPage";
            this.Size = new System.Drawing.Size(1523, 582);
            this.tableLayoutPanel1.ResumeLayout(false);
            this.panel1.ResumeLayout(false);
            this.panel1.PerformLayout();
            this.ResumeLayout(false);

        }
    }
}
