namespace AGS.Editor
{
    partial class ViewLoopEditor
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
            this.lblLoopTitle = new System.Windows.Forms.Label();
            this.chkRunNextLoop = new System.Windows.Forms.CheckBox();
            this.btnNewFrame = new System.Windows.Forms.Button();
            this.SuspendLayout();
            // 
            // lblLoopTitle
            // 
            this.lblLoopTitle.AutoSize = true;
            this.lblLoopTitle.Location = new System.Drawing.Point(0, 0);
            this.lblLoopTitle.Name = "lblLoopTitle";
            this.lblLoopTitle.Size = new System.Drawing.Size(35, 13);
            this.lblLoopTitle.TabIndex = 0;
            this.lblLoopTitle.Text = "label1";
            // 
            // chkRunNextLoop
            // 
            this.chkRunNextLoop.AutoSize = true;
            this.chkRunNextLoop.Location = new System.Drawing.Point(3, 18);
            this.chkRunNextLoop.Name = "chkRunNextLoop";
            this.chkRunNextLoop.Size = new System.Drawing.Size(274, 17);
            this.chkRunNextLoop.TabIndex = 1;
            this.chkRunNextLoop.Text = "Run the next loop after this to make a long animation";
            this.chkRunNextLoop.UseVisualStyleBackColor = true;
            // 
            // btnNewFrame
            // 
            this.btnNewFrame.Location = new System.Drawing.Point(37, 39);
            this.btnNewFrame.Name = "btnNewFrame";
            this.btnNewFrame.Size = new System.Drawing.Size(62, 55);
            this.btnNewFrame.TabIndex = 2;
            this.btnNewFrame.Text = "Create New Frame";
            this.btnNewFrame.UseVisualStyleBackColor = true;
            this.btnNewFrame.Click += new System.EventHandler(this.btnNewFrame_Click);
            // 
            // ViewLoopEditor
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(96F, 96F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
            this.Controls.Add(this.btnNewFrame);
            this.Controls.Add(this.chkRunNextLoop);
            this.Controls.Add(this.lblLoopTitle);
            this.Name = "ViewLoopEditor";
            this.Size = new System.Drawing.Size(728, 116);
            this.Paint += new System.Windows.Forms.PaintEventHandler(this.ViewLoopEditor_Paint);
            this.MouseDoubleClick += new System.Windows.Forms.MouseEventHandler(this.ViewLoopEditor_MouseDoubleClick);
            this.MouseUp += new System.Windows.Forms.MouseEventHandler(this.ViewLoopEditor_MouseUp);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label lblLoopTitle;
        private System.Windows.Forms.CheckBox chkRunNextLoop;
        private System.Windows.Forms.Button btnNewFrame;
    }
}
