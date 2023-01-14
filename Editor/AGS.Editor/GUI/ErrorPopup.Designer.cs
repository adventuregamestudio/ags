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
            this.lblErrorMessage.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.lblErrorMessage.BackColor = System.Drawing.Color.Yellow;
            this.lblErrorMessage.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.lblErrorMessage.Location = new System.Drawing.Point(22, 111);
            this.lblErrorMessage.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this.lblErrorMessage.Name = "lblErrorMessage";
            this.lblErrorMessage.Size = new System.Drawing.Size(412, 130);
            this.lblErrorMessage.TabIndex = 1;
            this.lblErrorMessage.Text = "the error message text goes here the error message text goes here the error messa" +
    "ge text goes here the error message text goes here the error message text goes h" +
    "ere ";
            // 
            // label1
            // 
            this.label1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.label1.Location = new System.Drawing.Point(18, 14);
            this.label1.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(416, 87);
            this.label1.TabIndex = 2;
            this.label1.Text = "AGS had a problem running your game. The error can be seen below, and is most lik" +
    "ely due to a scripting problem. The line in the script where this occurred is hi" +
    "ghlighted for you.";
            // 
            // btnCloseGame
            // 
            this.btnCloseGame.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.btnCloseGame.Location = new System.Drawing.Point(22, 262);
            this.btnCloseGame.Margin = new System.Windows.Forms.Padding(4, 4, 4, 4);
            this.btnCloseGame.Name = "btnCloseGame";
            this.btnCloseGame.Size = new System.Drawing.Size(380, 44);
            this.btnCloseGame.TabIndex = 3;
            this.btnCloseGame.Text = "I\'ve finished examining the error, close the game";
            this.btnCloseGame.UseVisualStyleBackColor = true;
            this.btnCloseGame.Click += new System.EventHandler(this.btnCloseGame_Click);
            // 
            // ErrorPopup
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(144F, 144F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
            this.BackColor = System.Drawing.Color.White;
            this.ClientSize = new System.Drawing.Size(447, 324);
            this.Controls.Add(this.btnCloseGame);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.lblErrorMessage);
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedToolWindow;
            this.Margin = new System.Windows.Forms.Padding(4, 4, 4, 4);
            this.Name = "ErrorPopup";
            this.Opacity = 0.85D;
            this.ShowInTaskbar = false;
            this.StartPosition = System.Windows.Forms.FormStartPosition.Manual;
            this.Text = "An error has occurred";
            this.FormClosed += new System.Windows.Forms.FormClosedEventHandler(this.ErrorPopup_FormClosed);
            this.Paint += new System.Windows.Forms.PaintEventHandler(this.ErrorPopup_Paint);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Label lblErrorMessage;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Button btnCloseGame;
    }
}