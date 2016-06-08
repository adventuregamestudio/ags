namespace AGS.Editor
{
    partial class SplashPage
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
            this.lblTagLine = new System.Windows.Forms.Label();
            this.lblSeparator = new System.Windows.Forms.Label();
            this.lblTitle = new System.Windows.Forms.Label();
            this.picCup = new System.Windows.Forms.PictureBox();
            this.lblMainVersion = new System.Windows.Forms.Label();
            this.lblBottomTagLine = new System.Windows.Forms.Label();
            ((System.ComponentModel.ISupportInitialize)(this.picCup)).BeginInit();
            this.SuspendLayout();
            // 
            // lblTagLine
            // 
            this.lblTagLine.AutoSize = true;
            this.lblTagLine.Font = new System.Drawing.Font("Segoe UI", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(204)));
            this.lblTagLine.ForeColor = System.Drawing.Color.White;
            this.lblTagLine.Location = new System.Drawing.Point(199, 218);
            this.lblTagLine.Name = "lblTagLine";
            this.lblTagLine.Size = new System.Drawing.Size(273, 21);
            this.lblTagLine.TabIndex = 11;
            this.lblTagLine.Text = "You can’t spell handbags without AGS";
            this.lblTagLine.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            // 
            // lblSeparator
            // 
            this.lblSeparator.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(177)))), ((int)(((byte)(205)))), ((int)(((byte)(237)))));
            this.lblSeparator.Location = new System.Drawing.Point(97, 198);
            this.lblSeparator.Name = "lblSeparator";
            this.lblSeparator.Size = new System.Drawing.Size(430, 2);
            this.lblSeparator.TabIndex = 10;
            // 
            // lblTitle
            // 
            this.lblTitle.AutoSize = true;
            this.lblTitle.Font = new System.Drawing.Font("Segoe UI", 24F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(204)));
            this.lblTitle.ForeColor = System.Drawing.Color.White;
            this.lblTitle.Location = new System.Drawing.Point(93, 148);
            this.lblTitle.Margin = new System.Windows.Forms.Padding(0);
            this.lblTitle.Name = "lblTitle";
            this.lblTitle.Size = new System.Drawing.Size(360, 45);
            this.lblTitle.TabIndex = 9;
            this.lblTitle.Text = "Adventure Game Studio";
            this.lblTitle.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            // 
            // picCup
            // 
            this.picCup.Location = new System.Drawing.Point(297, 73);
            this.picCup.Margin = new System.Windows.Forms.Padding(0);
            this.picCup.Name = "picCup";
            this.picCup.Size = new System.Drawing.Size(73, 73);
            this.picCup.SizeMode = System.Windows.Forms.PictureBoxSizeMode.AutoSize;
            this.picCup.TabIndex = 8;
            this.picCup.TabStop = false;
            // 
            // lblMainVersion
            // 
            this.lblMainVersion.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.lblMainVersion.AutoSize = true;
            this.lblMainVersion.BackColor = System.Drawing.Color.White;
            this.lblMainVersion.Font = new System.Drawing.Font("Segoe UI", 27.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(204)));
            this.lblMainVersion.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(59)))), ((int)(((byte)(130)))), ((int)(((byte)(210)))));
            this.lblMainVersion.Location = new System.Drawing.Point(445, 2);
            this.lblMainVersion.Margin = new System.Windows.Forms.Padding(0);
            this.lblMainVersion.Name = "lblMainVersion";
            this.lblMainVersion.Size = new System.Drawing.Size(173, 50);
            this.lblMainVersion.TabIndex = 7;
            this.lblMainVersion.Text = "3.4.0 RC1";
            this.lblMainVersion.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            // 
            // lblBottomTagLine
            // 
            this.lblBottomTagLine.BackColor = System.Drawing.Color.Transparent;
            this.lblBottomTagLine.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.lblBottomTagLine.Font = new System.Drawing.Font("Tahoma", 9.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lblBottomTagLine.ForeColor = System.Drawing.Color.White;
            this.lblBottomTagLine.Location = new System.Drawing.Point(2, 266);
            this.lblBottomTagLine.Name = "lblBottomTagLine";
            this.lblBottomTagLine.Padding = new System.Windows.Forms.Padding(7);
            this.lblBottomTagLine.Size = new System.Drawing.Size(616, 32);
            this.lblBottomTagLine.TabIndex = 6;
            this.lblBottomTagLine.Text = "You can’t spell handbags without AGS";
            this.lblBottomTagLine.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            // 
            // SplashPage
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(59)))), ((int)(((byte)(130)))), ((int)(((byte)(210)))));
            this.Controls.Add(this.lblTagLine);
            this.Controls.Add(this.lblSeparator);
            this.Controls.Add(this.lblTitle);
            this.Controls.Add(this.picCup);
            this.Controls.Add(this.lblMainVersion);
            this.Controls.Add(this.lblBottomTagLine);
            this.Margin = new System.Windows.Forms.Padding(0);
            this.Name = "SplashPage";
            this.Padding = new System.Windows.Forms.Padding(2);
            this.Size = new System.Drawing.Size(620, 300);
            ((System.ComponentModel.ISupportInitialize)(this.picCup)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label lblTagLine;
        private System.Windows.Forms.Label lblSeparator;
        private System.Windows.Forms.Label lblTitle;
        private System.Windows.Forms.PictureBox picCup;
        private System.Windows.Forms.Label lblMainVersion;
        private System.Windows.Forms.Label lblBottomTagLine;
    }
}
