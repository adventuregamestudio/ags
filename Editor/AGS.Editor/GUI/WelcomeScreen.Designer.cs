namespace AGS.Editor
{
    partial class WelcomeScreen
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
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.label4 = new System.Windows.Forms.Label();
            this.lstRecentGames = new System.Windows.Forms.ListView();
            this.columnHeader1 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.columnHeader2 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.label3 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.radRecent = new System.Windows.Forms.RadioButton();
            this.radLoadGame = new System.Windows.Forms.RadioButton();
            this.radNewGame = new System.Windows.Forms.RadioButton();
            this.label1 = new System.Windows.Forms.Label();
            this.btnContinue = new System.Windows.Forms.Button();
            this.btnExit = new System.Windows.Forms.Button();
            this.panelLogo = new System.Windows.Forms.Panel();
            this.lblLogo = new System.Windows.Forms.Label();
            this.groupBox1.SuspendLayout();
            this.panelLogo.SuspendLayout();
            this.SuspendLayout();
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.label4);
            this.groupBox1.Controls.Add(this.lstRecentGames);
            this.groupBox1.Controls.Add(this.label3);
            this.groupBox1.Controls.Add(this.label2);
            this.groupBox1.Controls.Add(this.radRecent);
            this.groupBox1.Controls.Add(this.radLoadGame);
            this.groupBox1.Controls.Add(this.radNewGame);
            this.groupBox1.Controls.Add(this.label1);
            this.groupBox1.Location = new System.Drawing.Point(11, 74);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(498, 293);
            this.groupBox1.TabIndex = 1;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Welcome";
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(35, 168);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(296, 13);
            this.label4.TabIndex = 7;
            this.label4.Text = "Continue making a game that you were recently working on:";
            // 
            // lstRecentGames
            // 
            this.lstRecentGames.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.columnHeader1,
            this.columnHeader2});
            this.lstRecentGames.FullRowSelect = true;
            this.lstRecentGames.HeaderStyle = System.Windows.Forms.ColumnHeaderStyle.None;
            this.lstRecentGames.HideSelection = false;
            this.lstRecentGames.Location = new System.Drawing.Point(38, 190);
            this.lstRecentGames.MultiSelect = false;
            this.lstRecentGames.Name = "lstRecentGames";
            this.lstRecentGames.Size = new System.Drawing.Size(422, 92);
            this.lstRecentGames.TabIndex = 6;
            this.lstRecentGames.UseCompatibleStateImageBehavior = false;
            this.lstRecentGames.View = System.Windows.Forms.View.Details;
            this.lstRecentGames.ItemActivate += new System.EventHandler(this.lstRecentGames_ItemActivate);
            // 
            // columnHeader1
            // 
            this.columnHeader1.Width = 150;
            // 
            // columnHeader2
            // 
            this.columnHeader2.Width = 600;
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(35, 120);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(138, 13);
            this.label3.TabIndex = 5;
            this.label3.Text = "Load one you made earlier!";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(35, 72);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(335, 13);
            this.label2.TabIndex = 4;
            this.label2.Text = "Start making a new game, from scratch. Turn your ideas into reality!";
            // 
            // radRecent
            // 
            this.radRecent.AutoSize = true;
            this.radRecent.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(178)));
            this.radRecent.Location = new System.Drawing.Point(19, 148);
            this.radRecent.Name = "radRecent";
            this.radRecent.Size = new System.Drawing.Size(209, 17);
            this.radRecent.TabIndex = 3;
            this.radRecent.TabStop = true;
            this.radRecent.Text = "Continue a recently edited game";
            this.radRecent.UseVisualStyleBackColor = true;
            this.radRecent.CheckedChanged += new System.EventHandler(this.radRecent_CheckedChanged);
            // 
            // radLoadGame
            // 
            this.radLoadGame.AutoSize = true;
            this.radLoadGame.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(178)));
            this.radLoadGame.Location = new System.Drawing.Point(19, 100);
            this.radLoadGame.Name = "radLoadGame";
            this.radLoadGame.Size = new System.Drawing.Size(175, 17);
            this.radLoadGame.TabIndex = 2;
            this.radLoadGame.TabStop = true;
            this.radLoadGame.Text = "Continue an existing game";
            this.radLoadGame.UseVisualStyleBackColor = true;
            // 
            // radNewGame
            // 
            this.radNewGame.AutoSize = true;
            this.radNewGame.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(178)));
            this.radNewGame.Location = new System.Drawing.Point(19, 52);
            this.radNewGame.Name = "radNewGame";
            this.radNewGame.Size = new System.Drawing.Size(125, 17);
            this.radNewGame.TabIndex = 1;
            this.radNewGame.TabStop = true;
            this.radNewGame.Text = "Start a new game";
            this.radNewGame.UseVisualStyleBackColor = true;
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(16, 27);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(215, 13);
            this.label1.TabIndex = 0;
            this.label1.Text = "Welcome to AGS! What do you want to do?";
            // 
            // btnContinue
            // 
            this.btnContinue.Location = new System.Drawing.Point(313, 373);
            this.btnContinue.Name = "btnContinue";
            this.btnContinue.Size = new System.Drawing.Size(94, 28);
            this.btnContinue.TabIndex = 2;
            this.btnContinue.Text = "&Continue >";
            this.btnContinue.UseVisualStyleBackColor = true;
            this.btnContinue.Click += new System.EventHandler(this.btnContinue_Click);
            // 
            // btnExit
            // 
            this.btnExit.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnExit.Location = new System.Drawing.Point(412, 373);
            this.btnExit.Name = "btnExit";
            this.btnExit.Size = new System.Drawing.Size(97, 28);
            this.btnExit.TabIndex = 3;
            this.btnExit.Text = "&Exit";
            this.btnExit.UseVisualStyleBackColor = true;
            // 
            // panelLogo
            // 
            this.panelLogo.BackColor = System.Drawing.Color.RoyalBlue;
            this.panelLogo.Controls.Add(this.lblLogo);
            this.panelLogo.Dock = System.Windows.Forms.DockStyle.Top;
            this.panelLogo.ForeColor = System.Drawing.Color.White;
            this.panelLogo.Location = new System.Drawing.Point(0, 0);
            this.panelLogo.Name = "panelLogo";
            this.panelLogo.Size = new System.Drawing.Size(519, 68);
            this.panelLogo.TabIndex = 4;
            // 
            // lblLogo
            // 
            this.lblLogo.AutoSize = true;
            this.lblLogo.Font = new System.Drawing.Font("Tahoma", 26F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lblLogo.Location = new System.Drawing.Point(19, 10);
            this.lblLogo.Name = "lblLogo";
            this.lblLogo.Size = new System.Drawing.Size(384, 42);
            this.lblLogo.TabIndex = 0;
            this.lblLogo.Text = "Adventure Game Studio";
            // 
            // WelcomeScreen
            // 
            this.AcceptButton = this.btnContinue;
            this.AutoScaleDimensions = new System.Drawing.SizeF(96F, 96F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
            this.CancelButton = this.btnExit;
            this.ClientSize = new System.Drawing.Size(519, 410);
            this.Controls.Add(this.panelLogo);
            this.Controls.Add(this.btnExit);
            this.Controls.Add(this.btnContinue);
            this.Controls.Add(this.groupBox1);
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(178)));
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "WelcomeScreen";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Welcome to AGS!";
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.panelLogo.ResumeLayout(false);
            this.panelLogo.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.RadioButton radRecent;
        private System.Windows.Forms.RadioButton radLoadGame;
        private System.Windows.Forms.RadioButton radNewGame;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Button btnContinue;
        private System.Windows.Forms.Button btnExit;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.ListView lstRecentGames;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.ColumnHeader columnHeader1;
        private System.Windows.Forms.ColumnHeader columnHeader2;
        private System.Windows.Forms.Panel panelLogo;
        private System.Windows.Forms.Label lblLogo;
    }
}