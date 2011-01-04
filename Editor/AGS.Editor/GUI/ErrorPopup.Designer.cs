namespace AGS.Editor
{
    partial class ErrorPopup
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
            this.lblErrorMessage = new System.Windows.Forms.Label();
            this.label1 = new System.Windows.Forms.Label();
            this.btnCloseGame = new System.Windows.Forms.Button();
            this.SuspendLayout();
            // 
            // lblErrorMessage
            // 
            this.lblErrorMessage.AutoSize = true;
            this.lblErrorMessage.BackColor = System.Drawing.Color.Yellow;
            this.lblErrorMessage.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.lblErrorMessage.Location = new System.Drawing.Point(15, 74);
            this.lblErrorMessage.MaximumSize = new System.Drawing.Size(250, 100);
            this.lblErrorMessage.Name = "lblErrorMessage";
            this.lblErrorMessage.Size = new System.Drawing.Size(244, 54);
            this.lblErrorMessage.TabIndex = 1;
            this.lblErrorMessage.Text = "the error message text goes here the error message text goes here the error messa" +
                "ge text goes here the error message text goes here the error message text goes h" +
                "ere ";
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(12, 9);
            this.label1.MaximumSize = new System.Drawing.Size(250, 0);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(247, 52);
            this.label1.TabIndex = 2;
            this.label1.Text = "AGS had a problem running your game. The error can be seen below, and is most lik" +
                "ely due to a scripting problem. The line in the script where this occurred is hi" +
                "ghlighted for you.";
            // 
            // btnCloseGame
            // 
            this.btnCloseGame.Location = new System.Drawing.Point(15, 175);
            this.btnCloseGame.Name = "btnCloseGame";
            this.btnCloseGame.Size = new System.Drawing.Size(253, 29);
            this.btnCloseGame.TabIndex = 3;
            this.btnCloseGame.Text = "I\'ve finished examining the error, close the game";
            this.btnCloseGame.UseVisualStyleBackColor = true;
            this.btnCloseGame.Click += new System.EventHandler(this.btnCloseGame_Click);
            // 
            // ErrorPopup
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(96F, 96F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
            this.BackColor = System.Drawing.Color.White;
            this.ClientSize = new System.Drawing.Size(298, 216);
            this.Controls.Add(this.btnCloseGame);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.lblErrorMessage);
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedToolWindow;
            this.Name = "ErrorPopup";
            this.Opacity = 0.85;
            this.ShowInTaskbar = false;
            this.StartPosition = System.Windows.Forms.FormStartPosition.Manual;
            this.Text = "An error has occurred";
            this.FormClosed += new System.Windows.Forms.FormClosedEventHandler(this.ErrorPopup_FormClosed);
            this.Paint += new System.Windows.Forms.PaintEventHandler(this.ErrorPopup_Paint);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label lblErrorMessage;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Button btnCloseGame;
    }
}