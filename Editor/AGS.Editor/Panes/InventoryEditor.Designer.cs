namespace AGS.Editor
{
    partial class InventoryEditor
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
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.label2 = new System.Windows.Forms.Label();
            this.pnlCursorImage = new System.Windows.Forms.Panel();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.pnlInvWindowImage = new System.Windows.Forms.Panel();
            this.label1 = new System.Windows.Forms.Label();
            this.currentItemGroupBox.SuspendLayout();
            this.groupBox2.SuspendLayout();
            this.groupBox1.SuspendLayout();
            this.SuspendLayout();
            // 
            // currentItemGroupBox
            // 
            this.currentItemGroupBox.BackColor = System.Drawing.SystemColors.Control;
            this.currentItemGroupBox.Controls.Add(this.groupBox2);
            this.currentItemGroupBox.Controls.Add(this.groupBox1);
            this.currentItemGroupBox.Controls.Add(this.label1);
            this.currentItemGroupBox.Location = new System.Drawing.Point(13, 16);
            this.currentItemGroupBox.Name = "currentItemGroupBox";
            this.currentItemGroupBox.Size = new System.Drawing.Size(598, 290);
            this.currentItemGroupBox.TabIndex = 1;
            this.currentItemGroupBox.TabStop = false;
            this.currentItemGroupBox.Text = "Selected inventory item settings";
            // 
            // groupBox2
            // 
            this.groupBox2.Controls.Add(this.label2);
            this.groupBox2.Controls.Add(this.pnlCursorImage);
            this.groupBox2.Location = new System.Drawing.Point(316, 50);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Size = new System.Drawing.Size(270, 230);
            this.groupBox2.TabIndex = 7;
            this.groupBox2.TabStop = false;
            this.groupBox2.Text = "Mouse cursor image";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(13, 23);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(247, 13);
            this.label2.TabIndex = 4;
            this.label2.Text = "Click in the image below to set the cursor hotspot:";
            // 
            // pnlCursorImage
            // 
            this.pnlCursorImage.Location = new System.Drawing.Point(16, 44);
            this.pnlCursorImage.Name = "pnlCursorImage";
            this.pnlCursorImage.Size = new System.Drawing.Size(244, 171);
            this.pnlCursorImage.TabIndex = 3;
            this.pnlCursorImage.Paint += new System.Windows.Forms.PaintEventHandler(this.pnlCursorImage_Paint);
            this.pnlCursorImage.MouseDown += new System.Windows.Forms.MouseEventHandler(this.pnlCursorImage_MouseDown);
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.pnlInvWindowImage);
            this.groupBox1.Location = new System.Drawing.Point(13, 50);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(289, 230);
            this.groupBox1.TabIndex = 6;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Image in inventory window";
            // 
            // pnlInvWindowImage
            // 
            this.pnlInvWindowImage.Location = new System.Drawing.Point(15, 20);
            this.pnlInvWindowImage.Name = "pnlInvWindowImage";
            this.pnlInvWindowImage.Size = new System.Drawing.Size(261, 195);
            this.pnlInvWindowImage.TabIndex = 5;
            this.pnlInvWindowImage.Paint += new System.Windows.Forms.PaintEventHandler(this.pnlInvWindowImage_Paint);
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(10, 18);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(292, 13);
            this.label1.TabIndex = 0;
            this.label1.Text = "Use the property grid on the right to change basic settings.";
            // 
            // InventoryEditor
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(96F, 96F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
            this.BackColor = System.Drawing.SystemColors.Control;
            this.Controls.Add(this.currentItemGroupBox);
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Name = "InventoryEditor";
            this.Size = new System.Drawing.Size(633, 331);
            this.currentItemGroupBox.ResumeLayout(false);
            this.currentItemGroupBox.PerformLayout();
            this.groupBox2.ResumeLayout(false);
            this.groupBox2.PerformLayout();
            this.groupBox1.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.GroupBox currentItemGroupBox;
        private System.Windows.Forms.Panel pnlCursorImage;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.GroupBox groupBox2;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.Panel pnlInvWindowImage;
    }
}
