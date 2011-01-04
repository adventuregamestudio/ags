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
            this.imagePanel = new System.Windows.Forms.Panel();
            this.label1 = new System.Windows.Forms.Label();
            this.btnImportFont = new System.Windows.Forms.Button();
            this.currentItemGroupBox.SuspendLayout();
            this.SuspendLayout();
            // 
            // currentItemGroupBox
            // 
            this.currentItemGroupBox.BackColor = System.Drawing.SystemColors.Control;
            this.currentItemGroupBox.Controls.Add(this.btnImportFont);
            this.currentItemGroupBox.Controls.Add(this.imagePanel);
            this.currentItemGroupBox.Controls.Add(this.label1);
            this.currentItemGroupBox.Location = new System.Drawing.Point(13, 13);
            this.currentItemGroupBox.Name = "currentItemGroupBox";
            this.currentItemGroupBox.Size = new System.Drawing.Size(474, 467);
            this.currentItemGroupBox.TabIndex = 3;
            this.currentItemGroupBox.TabStop = false;
            this.currentItemGroupBox.Text = "Selected font settings";
            // 
            // imagePanel
            // 
            this.imagePanel.Location = new System.Drawing.Point(13, 80);
            this.imagePanel.Name = "imagePanel";
            this.imagePanel.Size = new System.Drawing.Size(455, 381);
            this.imagePanel.TabIndex = 3;
            this.imagePanel.Paint += new System.Windows.Forms.PaintEventHandler(this.imagePanel_Paint);
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
            // FontEditor
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.currentItemGroupBox);
            this.Name = "FontEditor";
            this.Size = new System.Drawing.Size(534, 493);
            this.currentItemGroupBox.ResumeLayout(false);
            this.currentItemGroupBox.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.GroupBox currentItemGroupBox;
        private System.Windows.Forms.Panel imagePanel;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Button btnImportFont;
    }
}
