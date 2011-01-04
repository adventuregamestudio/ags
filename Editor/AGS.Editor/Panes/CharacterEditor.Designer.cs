namespace AGS.Editor
{
    partial class CharacterEditor
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
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.viewPreview2 = new AGS.Editor.ViewPreview();
            this.viewPreview1 = new AGS.Editor.ViewPreview();
            this.btnMakePlayer = new System.Windows.Forms.Button();
            this.lblIsPlayer = new System.Windows.Forms.Label();
            this.groupBox1.SuspendLayout();
            this.SuspendLayout();
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.viewPreview2);
            this.groupBox1.Controls.Add(this.viewPreview1);
            this.groupBox1.Controls.Add(this.btnMakePlayer);
            this.groupBox1.Controls.Add(this.lblIsPlayer);
            this.groupBox1.Location = new System.Drawing.Point(5, 5);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(581, 424);
            this.groupBox1.TabIndex = 5;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Selected character settings";
            // 
            // viewPreview2
            // 
            this.viewPreview2.DynamicUpdates = false;
            this.viewPreview2.IsCharacterView = false;
            this.viewPreview2.Location = new System.Drawing.Point(294, 89);
            this.viewPreview2.Name = "viewPreview2";
            this.viewPreview2.Size = new System.Drawing.Size(274, 328);
            this.viewPreview2.TabIndex = 10;
            this.viewPreview2.Title = "Speech view";
            this.viewPreview2.ViewToPreview = null;
            // 
            // viewPreview1
            // 
            this.viewPreview1.DynamicUpdates = false;
            this.viewPreview1.IsCharacterView = false;
            this.viewPreview1.Location = new System.Drawing.Point(14, 89);
            this.viewPreview1.Name = "viewPreview1";
            this.viewPreview1.Size = new System.Drawing.Size(274, 328);
            this.viewPreview1.TabIndex = 9;
            this.viewPreview1.Title = "Normal view";
            this.viewPreview1.ViewToPreview = null;
            // 
            // btnMakePlayer
            // 
            this.btnMakePlayer.Location = new System.Drawing.Point(19, 60);
            this.btnMakePlayer.Name = "btnMakePlayer";
            this.btnMakePlayer.Size = new System.Drawing.Size(188, 23);
            this.btnMakePlayer.TabIndex = 6;
            this.btnMakePlayer.Text = "Make this the player character";
            this.btnMakePlayer.UseVisualStyleBackColor = true;
            this.btnMakePlayer.Click += new System.EventHandler(this.btnMakePlayer_Click);
            // 
            // lblIsPlayer
            // 
            this.lblIsPlayer.AutoSize = true;
            this.lblIsPlayer.Location = new System.Drawing.Point(16, 26);
            this.lblIsPlayer.MaximumSize = new System.Drawing.Size(400, 0);
            this.lblIsPlayer.Name = "lblIsPlayer";
            this.lblIsPlayer.Size = new System.Drawing.Size(398, 26);
            this.lblIsPlayer.TabIndex = 5;
            this.lblIsPlayer.Text = "This character is OR IS NOT the player; the game will startin this character\'s ro" +
                "om or something";
            // 
            // CharacterEditor
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(96F, 96F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
            this.Controls.Add(this.groupBox1);
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Name = "CharacterEditor";
            this.Size = new System.Drawing.Size(593, 441);
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.Button btnMakePlayer;
        private System.Windows.Forms.Label lblIsPlayer;
        private ViewPreview viewPreview2;
        private ViewPreview viewPreview1;

    }
}
