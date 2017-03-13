namespace AGS.Editor
{
    partial class FontEditor
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
            this.currentItemGroupBox = new System.Windows.Forms.GroupBox();
            this.btnImportFont = new System.Windows.Forms.Button();
            this.imagePanel = new System.Windows.Forms.Panel();
            this.pictureBox = new System.Windows.Forms.PictureBox();
            this.label1 = new System.Windows.Forms.Label();
            this.currentItemGroupBox.SuspendLayout();
            this.imagePanel.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox)).BeginInit();
            this.SuspendLayout();
            // 
            // currentItemGroupBox
            // 
            this.currentItemGroupBox.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.currentItemGroupBox.BackColor = System.Drawing.SystemColors.Control;
            this.currentItemGroupBox.Controls.Add(this.btnImportFont);
            this.currentItemGroupBox.Controls.Add(this.imagePanel);
            this.currentItemGroupBox.Controls.Add(this.label1);
            this.currentItemGroupBox.Location = new System.Drawing.Point(13, 13);
            this.currentItemGroupBox.Name = "currentItemGroupBox";
            this.currentItemGroupBox.Size = new System.Drawing.Size(505, 467);
            this.currentItemGroupBox.TabIndex = 3;
            this.currentItemGroupBox.TabStop = false;
            this.currentItemGroupBox.Text = "Selected font settings";
            // 
            // btnImportFont
            // 
            this.btnImportFont.Location = new System.Drawing.Point(13, 40);
            this.btnImportFont.Name = "btnImportFont";
            this.btnImportFont.Size = new System.Drawing.Size(132, 26);
            this.btnImportFont.TabIndex = 4;
            this.btnImportFont.Text = "Import over this font...";
            this.btnImportFont.UseVisualStyleBackColor = true;
            this.btnImportFont.Click += new System.EventHandler(this.btnImportFont_Click);
            // 
            // imagePanel
            // 
            this.imagePanel.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.imagePanel.AutoScroll = true;
            this.imagePanel.Controls.Add(this.pictureBox);
            this.imagePanel.Location = new System.Drawing.Point(13, 80);
            this.imagePanel.Name = "imagePanel";
            this.imagePanel.Size = new System.Drawing.Size(477, 370);
            this.imagePanel.TabIndex = 3;
            this.imagePanel.SizeChanged += new System.EventHandler(this.imagePanel_SizeChanged);
            // 
            // pictureBox
            // 
            this.pictureBox.Location = new System.Drawing.Point(0, 0);
            this.pictureBox.Name = "pictureBox";
            this.pictureBox.Size = new System.Drawing.Size(100, 50);
            this.pictureBox.SizeMode = System.Windows.Forms.PictureBoxSizeMode.AutoSize;
            this.pictureBox.TabIndex = 0;
            this.pictureBox.TabStop = false;
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(10, 18);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(282, 13);
            this.label1.TabIndex = 0;
            this.label1.Text = "Use the property grid on the right to change basic settings.";
            // 
            // FontEditor
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.currentItemGroupBox);
            this.Name = "FontEditor";
            this.Size = new System.Drawing.Size(534, 493);
            this.currentItemGroupBox.ResumeLayout(false);
            this.currentItemGroupBox.PerformLayout();
            this.imagePanel.ResumeLayout(false);
            this.imagePanel.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.GroupBox currentItemGroupBox;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Button btnImportFont;
        private System.Windows.Forms.Panel imagePanel;
        private System.Windows.Forms.PictureBox pictureBox;
    }
}
