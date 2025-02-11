
namespace AGS.Editor
{
    partial class ReplaceFolderDialog
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
            this.lblOperationDescription = new System.Windows.Forms.Label();
            this.btnCancel = new System.Windows.Forms.Button();
            this.btnOK = new System.Windows.Forms.Button();
            this.tbOldPath = new System.Windows.Forms.TextBox();
            this.label1 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.tbNewPath = new System.Windows.Forms.TextBox();
            this.btnBrowseOldPath = new System.Windows.Forms.Button();
            this.btnBrowseNewPath = new System.Windows.Forms.Button();
            this.SuspendLayout();
            // 
            // lblOperationDescription
            // 
            this.lblOperationDescription.Location = new System.Drawing.Point(13, 13);
            this.lblOperationDescription.Name = "lblOperationDescription";
            this.lblOperationDescription.Size = new System.Drawing.Size(392, 52);
            this.lblOperationDescription.TabIndex = 0;
            this.lblOperationDescription.Text = "label1";
            // 
            // btnCancel
            // 
            this.btnCancel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnCancel.Location = new System.Drawing.Point(346, 145);
            this.btnCancel.Name = "btnCancel";
            this.btnCancel.Size = new System.Drawing.Size(92, 26);
            this.btnCancel.TabIndex = 11;
            this.btnCancel.Text = "Cancel";
            this.btnCancel.UseVisualStyleBackColor = true;
            this.btnCancel.Click += new System.EventHandler(this.btnCancel_Click);
            // 
            // btnOK
            // 
            this.btnOK.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnOK.Location = new System.Drawing.Point(248, 145);
            this.btnOK.Name = "btnOK";
            this.btnOK.Size = new System.Drawing.Size(92, 26);
            this.btnOK.TabIndex = 10;
            this.btnOK.Text = "OK";
            this.btnOK.UseVisualStyleBackColor = true;
            this.btnOK.Click += new System.EventHandler(this.btnOK_Click);
            // 
            // tbOldPath
            // 
            this.tbOldPath.Location = new System.Drawing.Point(77, 80);
            this.tbOldPath.Name = "tbOldPath";
            this.tbOldPath.Size = new System.Drawing.Size(321, 20);
            this.tbOldPath.TabIndex = 12;
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(13, 83);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(50, 13);
            this.label1.TabIndex = 13;
            this.label1.Text = "Old path:";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(13, 109);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(56, 13);
            this.label2.TabIndex = 15;
            this.label2.Text = "New path:";
            // 
            // tbNewPath
            // 
            this.tbNewPath.Location = new System.Drawing.Point(77, 106);
            this.tbNewPath.Name = "tbNewPath";
            this.tbNewPath.Size = new System.Drawing.Size(321, 20);
            this.tbNewPath.TabIndex = 14;
            // 
            // btnBrowseOldPath
            // 
            this.btnBrowseOldPath.Location = new System.Drawing.Point(404, 80);
            this.btnBrowseOldPath.Name = "btnBrowseOldPath";
            this.btnBrowseOldPath.Size = new System.Drawing.Size(33, 20);
            this.btnBrowseOldPath.TabIndex = 16;
            this.btnBrowseOldPath.Text = "...";
            this.btnBrowseOldPath.UseVisualStyleBackColor = true;
            this.btnBrowseOldPath.Click += new System.EventHandler(this.btnBrowseOldPath_Click);
            // 
            // btnBrowseNewPath
            // 
            this.btnBrowseNewPath.Location = new System.Drawing.Point(404, 105);
            this.btnBrowseNewPath.Name = "btnBrowseNewPath";
            this.btnBrowseNewPath.Size = new System.Drawing.Size(33, 20);
            this.btnBrowseNewPath.TabIndex = 17;
            this.btnBrowseNewPath.Text = "...";
            this.btnBrowseNewPath.UseVisualStyleBackColor = true;
            this.btnBrowseNewPath.Click += new System.EventHandler(this.btnBrowseNewPath_Click);
            // 
            // ReplaceFolderDialog
            // 
            this.AcceptButton = this.btnOK;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this.btnCancel;
            this.ClientSize = new System.Drawing.Size(450, 183);
            this.Controls.Add(this.btnBrowseNewPath);
            this.Controls.Add(this.btnBrowseOldPath);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.tbNewPath);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.tbOldPath);
            this.Controls.Add(this.btnCancel);
            this.Controls.Add(this.btnOK);
            this.Controls.Add(this.lblOperationDescription);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "ReplaceFolderDialog";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "ReplaceFolderDialog";
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label lblOperationDescription;
        private System.Windows.Forms.Button btnCancel;
        private System.Windows.Forms.Button btnOK;
        private System.Windows.Forms.TextBox tbOldPath;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.TextBox tbNewPath;
        private System.Windows.Forms.Button btnBrowseOldPath;
        private System.Windows.Forms.Button btnBrowseNewPath;
    }
}