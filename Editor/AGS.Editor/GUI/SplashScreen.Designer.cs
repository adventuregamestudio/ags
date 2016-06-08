namespace AGS.Editor
{
    partial class SplashScreen
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
            this.splashPage = new AGS.Editor.SplashPage();
            this.SuspendLayout();
            // 
            // splashPage
            // 
            this.splashPage.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(59)))), ((int)(((byte)(130)))), ((int)(((byte)(210)))));
            this.splashPage.Location = new System.Drawing.Point(0, 0);
            this.splashPage.Margin = new System.Windows.Forms.Padding(0);
            this.splashPage.Name = "splashPage";
            this.splashPage.Size = new System.Drawing.Size(620, 300);
            this.splashPage.TabIndex = 0;
            // 
            // SplashScreen
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(620, 300);
            this.ControlBox = false;
            this.Controls.Add(this.splashPage);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.None;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "SplashScreen";
            this.ShowIcon = false;
            this.ShowInTaskbar = false;
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "SplashScreen";
            this.ResumeLayout(false);

        }

        #endregion

        private SplashPage splashPage;


    }
}