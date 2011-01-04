namespace AGS.Editor
{
    partial class SpriteChooser
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
			this.spriteSelector1 = new AGS.Editor.SpriteSelector();
			this.btnOK = new System.Windows.Forms.Button();
			this.btnCancel = new System.Windows.Forms.Button();
			this.btnUseNoSprite = new System.Windows.Forms.Button();
			this.SuspendLayout();
			// 
			// spriteSelector1
			// 
			this.spriteSelector1.Dock = System.Windows.Forms.DockStyle.Top;
			this.spriteSelector1.Location = new System.Drawing.Point(0, 0);
			this.spriteSelector1.Name = "spriteSelector1";
			this.spriteSelector1.SendUpdateNotifications = false;
			this.spriteSelector1.ShowUseThisSpriteOption = false;
			this.spriteSelector1.Size = new System.Drawing.Size(629, 386);
			this.spriteSelector1.TabIndex = 0;
			this.spriteSelector1.OnSelectionChanged += new AGS.Editor.SpriteSelector.SelectionChangedHandler(this.spriteSelector1_OnSelectionChanged);
			this.spriteSelector1.OnSpriteActivated += new AGS.Editor.SpriteSelector.SpriteActivatedHandler(this.spriteSelector1_OnSpriteActivated);
			// 
			// btnOK
			// 
			this.btnOK.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
			this.btnOK.Location = new System.Drawing.Point(13, 392);
			this.btnOK.Name = "btnOK";
			this.btnOK.Size = new System.Drawing.Size(112, 28);
			this.btnOK.TabIndex = 1;
			this.btnOK.Text = "Use this sprite";
			this.btnOK.UseVisualStyleBackColor = true;
			this.btnOK.Click += new System.EventHandler(this.btnOK_Click);
			// 
			// btnCancel
			// 
			this.btnCancel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
			this.btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
			this.btnCancel.Location = new System.Drawing.Point(267, 392);
			this.btnCancel.Name = "btnCancel";
			this.btnCancel.Size = new System.Drawing.Size(112, 28);
			this.btnCancel.TabIndex = 2;
			this.btnCancel.Text = "Cancel";
			this.btnCancel.UseVisualStyleBackColor = true;
			this.btnCancel.Click += new System.EventHandler(this.btnCancel_Click);
			// 
			// btnUseNoSprite
			// 
			this.btnUseNoSprite.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
			this.btnUseNoSprite.DialogResult = System.Windows.Forms.DialogResult.Cancel;
			this.btnUseNoSprite.Location = new System.Drawing.Point(140, 392);
			this.btnUseNoSprite.Name = "btnUseNoSprite";
			this.btnUseNoSprite.Size = new System.Drawing.Size(112, 28);
			this.btnUseNoSprite.TabIndex = 3;
			this.btnUseNoSprite.Text = "Don\'t use any sprite";
			this.btnUseNoSprite.UseVisualStyleBackColor = true;
			this.btnUseNoSprite.Click += new System.EventHandler(this.btnUseNoSprite_Click);
			// 
			// SpriteChooser
			// 
			this.AcceptButton = this.btnOK;
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.CancelButton = this.btnCancel;
			this.ClientSize = new System.Drawing.Size(629, 426);
			this.Controls.Add(this.btnUseNoSprite);
			this.Controls.Add(this.btnCancel);
			this.Controls.Add(this.btnOK);
			this.Controls.Add(this.spriteSelector1);
			this.MinimizeBox = false;
			this.Name = "SpriteChooser";
			this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
			this.Text = "Select a sprite to use";
			this.Resize += new System.EventHandler(this.SpriteChooser_Resize);
			this.Load += new System.EventHandler(this.SpriteChooser_Load);
			this.ResumeLayout(false);

        }

        #endregion

        private SpriteSelector spriteSelector1;
        private System.Windows.Forms.Button btnOK;
        private System.Windows.Forms.Button btnCancel;
		private System.Windows.Forms.Button btnUseNoSprite;
    }
}