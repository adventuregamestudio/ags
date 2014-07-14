namespace AGS.Editor
{
    partial class ExceptionDialog
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(ExceptionDialog));
            this.errorIcon = new System.Windows.Forms.PictureBox();
            this.lblError = new System.Windows.Forms.Label();
            this.txtErrorDetails = new System.Windows.Forms.TextBox();
            this.btnOK = new System.Windows.Forms.Button();
            this.btnSendErrorReport = new System.Windows.Forms.Button();
            this.lblReportSucceeded = new System.Windows.Forms.Label();
            ((System.ComponentModel.ISupportInitialize)(this.errorIcon)).BeginInit();
            this.SuspendLayout();
            // 
            // errorIcon
            // 
            this.errorIcon.Image = global::AGS.Editor.Properties.Resources.error;
            this.errorIcon.Location = new System.Drawing.Point(32, 21);
            this.errorIcon.Name = "errorIcon";
            this.errorIcon.Size = new System.Drawing.Size(37, 40);
            this.errorIcon.TabIndex = 0;
            this.errorIcon.TabStop = false;
            // 
            // lblError
            // 
            this.lblError.AutoSize = true;
            this.lblError.Location = new System.Drawing.Point(81, 22);
            this.lblError.MaximumSize = new System.Drawing.Size(400, 0);
            this.lblError.Name = "lblError";
            this.lblError.Size = new System.Drawing.Size(396, 39);
            this.lblError.TabIndex = 1;
            this.lblError.Text = resources.GetString("lblError.Text");
            // 
            // txtErrorDetails
            // 
            this.txtErrorDetails.Location = new System.Drawing.Point(32, 75);
            this.txtErrorDetails.Multiline = true;
            this.txtErrorDetails.Name = "txtErrorDetails";
            this.txtErrorDetails.ReadOnly = true;
            this.txtErrorDetails.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
            this.txtErrorDetails.Size = new System.Drawing.Size(486, 190);
            this.txtErrorDetails.TabIndex = 2;
            // 
            // btnOK
            // 
            this.btnOK.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnOK.Location = new System.Drawing.Point(416, 279);
            this.btnOK.Name = "btnOK";
            this.btnOK.Size = new System.Drawing.Size(102, 32);
            this.btnOK.TabIndex = 3;
            this.btnOK.Text = "Close";
            this.btnOK.UseVisualStyleBackColor = true;
            this.btnOK.Click += new System.EventHandler(this.btnOK_Click);
            // 
            // btnSendErrorReport
            // 
            this.btnSendErrorReport.Location = new System.Drawing.Point(253, 279);
            this.btnSendErrorReport.Name = "btnSendErrorReport";
            this.btnSendErrorReport.Size = new System.Drawing.Size(144, 32);
            this.btnSendErrorReport.TabIndex = 4;
            this.btnSendErrorReport.Text = "Send Error Report";
            this.btnSendErrorReport.UseVisualStyleBackColor = true;
            this.btnSendErrorReport.Click += new System.EventHandler(this.btnSendErrorReport_Click);
            // 
            // lblReportSucceeded
            // 
            this.lblReportSucceeded.AutoSize = true;
            this.lblReportSucceeded.Location = new System.Drawing.Point(55, 279);
            this.lblReportSucceeded.MaximumSize = new System.Drawing.Size(350, 0);
            this.lblReportSucceeded.Name = "lblReportSucceeded";
            this.lblReportSucceeded.Size = new System.Drawing.Size(342, 26);
            this.lblReportSucceeded.TabIndex = 5;
            this.lblReportSucceeded.Text = "Your error report has been accepted. If you are experiencing regular errors, plea" +
                "se post on the AGS Technical Forum for help.";
            this.lblReportSucceeded.Visible = false;
            // 
            // ExceptionDialog
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this.btnOK;
            this.ClientSize = new System.Drawing.Size(546, 323);
            this.Controls.Add(this.btnSendErrorReport);
            this.Controls.Add(this.btnOK);
            this.Controls.Add(this.txtErrorDetails);
            this.Controls.Add(this.lblError);
            this.Controls.Add(this.errorIcon);
            this.Controls.Add(this.lblReportSucceeded);
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "ExceptionDialog";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Unhandled Error";
            ((System.ComponentModel.ISupportInitialize)(this.errorIcon)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.PictureBox errorIcon;
        private System.Windows.Forms.Label lblError;
        private System.Windows.Forms.TextBox txtErrorDetails;
        private System.Windows.Forms.Button btnOK;
        private System.Windows.Forms.Button btnSendErrorReport;
        private System.Windows.Forms.Label lblReportSucceeded;
    }
}