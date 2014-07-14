namespace AGS.Editor
{
    partial class ViewPreview
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
            StopTimer();
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
            this.mainGroupBox = new System.Windows.Forms.GroupBox();
            this.previewPanel = new AGS.Editor.BufferedPanel();
            this.chkSkipFrame0 = new System.Windows.Forms.CheckBox();
            this.chkCentrePivot = new System.Windows.Forms.CheckBox();
            this.chkAnimate = new System.Windows.Forms.CheckBox();
            this.label3 = new System.Windows.Forms.Label();
            this.udDelay = new System.Windows.Forms.NumericUpDown();
            this.label2 = new System.Windows.Forms.Label();
            this.udFrame = new System.Windows.Forms.NumericUpDown();
            this.label1 = new System.Windows.Forms.Label();
            this.udLoop = new System.Windows.Forms.NumericUpDown();
            this.mainGroupBox.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.udDelay)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.udFrame)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.udLoop)).BeginInit();
            this.SuspendLayout();
            // 
            // mainGroupBox
            // 
            this.mainGroupBox.Controls.Add(this.previewPanel);
            this.mainGroupBox.Controls.Add(this.chkSkipFrame0);
            this.mainGroupBox.Controls.Add(this.chkCentrePivot);
            this.mainGroupBox.Controls.Add(this.chkAnimate);
            this.mainGroupBox.Controls.Add(this.label3);
            this.mainGroupBox.Controls.Add(this.udDelay);
            this.mainGroupBox.Controls.Add(this.label2);
            this.mainGroupBox.Controls.Add(this.udFrame);
            this.mainGroupBox.Controls.Add(this.label1);
            this.mainGroupBox.Controls.Add(this.udLoop);
            this.mainGroupBox.Location = new System.Drawing.Point(4, 2);
            this.mainGroupBox.Name = "mainGroupBox";
            this.mainGroupBox.Size = new System.Drawing.Size(264, 321);
            this.mainGroupBox.TabIndex = 0;
            this.mainGroupBox.TabStop = false;
            this.mainGroupBox.Text = "groupBox1";
            // 
            // previewPanel
            // 
            this.previewPanel.Location = new System.Drawing.Point(12, 131);
            this.previewPanel.Name = "previewPanel";
            this.previewPanel.Size = new System.Drawing.Size(240, 179);
            this.previewPanel.TabIndex = 9;
            this.previewPanel.Paint += new System.Windows.Forms.PaintEventHandler(this.previewPanel_Paint);
            // 
            // chkSkipFrame0
            // 
            this.chkSkipFrame0.AutoSize = true;
            this.chkSkipFrame0.Location = new System.Drawing.Point(31, 106);
            this.chkSkipFrame0.Name = "chkSkipFrame0";
            this.chkSkipFrame0.Size = new System.Drawing.Size(163, 17);
            this.chkSkipFrame0.TabIndex = 8;
            this.chkSkipFrame0.Text = "Skip frame 0 (standing frame)";
            this.chkSkipFrame0.UseVisualStyleBackColor = true;
            // 
            // chkCentrePivot
            // 
            this.chkCentrePivot.AutoSize = true;
            this.chkCentrePivot.Location = new System.Drawing.Point(19, 62);
            this.chkCentrePivot.Name = "chkCentrePivot";
            this.chkCentrePivot.Size = new System.Drawing.Size(162, 17);
            this.chkCentrePivot.TabIndex = 6;
            this.chkCentrePivot.Text = "Character view (centre pivot)";
            this.chkCentrePivot.UseVisualStyleBackColor = true;
            this.chkCentrePivot.CheckedChanged += new System.EventHandler(this.chkCentrePivot_CheckedChanged);
            // 
            // chkAnimate
            // 
            this.chkAnimate.AutoSize = true;
            this.chkAnimate.Location = new System.Drawing.Point(19, 83);
            this.chkAnimate.Name = "chkAnimate";
            this.chkAnimate.Size = new System.Drawing.Size(64, 17);
            this.chkAnimate.TabIndex = 7;
            this.chkAnimate.Text = "Animate";
            this.chkAnimate.UseVisualStyleBackColor = true;
            this.chkAnimate.CheckedChanged += new System.EventHandler(this.chkAnimate_CheckedChanged);
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(157, 19);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(37, 13);
            this.label3.TabIndex = 5;
            this.label3.Text = "Delay:";
            // 
            // udDelay
            // 
            this.udDelay.Location = new System.Drawing.Point(160, 35);
            this.udDelay.Name = "udDelay";
            this.udDelay.Size = new System.Drawing.Size(53, 20);
            this.udDelay.TabIndex = 4;
            this.udDelay.Value = new decimal(new int[] {
            5,
            0,
            0,
            0});
            this.udDelay.ValueChanged += new System.EventHandler(this.udDelay_ValueChanged);
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(86, 19);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(39, 13);
            this.label2.TabIndex = 3;
            this.label2.Text = "Frame:";
            // 
            // udFrame
            // 
            this.udFrame.Location = new System.Drawing.Point(89, 35);
            this.udFrame.Name = "udFrame";
            this.udFrame.Size = new System.Drawing.Size(53, 20);
            this.udFrame.TabIndex = 2;
            this.udFrame.ValueChanged += new System.EventHandler(this.udFrame_ValueChanged);
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(16, 19);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(34, 13);
            this.label1.TabIndex = 1;
            this.label1.Text = "Loop:";
            // 
            // udLoop
            // 
            this.udLoop.Location = new System.Drawing.Point(19, 35);
            this.udLoop.Name = "udLoop";
            this.udLoop.Size = new System.Drawing.Size(53, 20);
            this.udLoop.TabIndex = 0;
            this.udLoop.ValueChanged += new System.EventHandler(this.udLoop_ValueChanged);
            // 
            // ViewPreview
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(96F, 96F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
            this.Controls.Add(this.mainGroupBox);
            this.Name = "ViewPreview";
            this.Size = new System.Drawing.Size(274, 327);
            this.mainGroupBox.ResumeLayout(false);
            this.mainGroupBox.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.udDelay)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.udFrame)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.udLoop)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.GroupBox mainGroupBox;
        private System.Windows.Forms.CheckBox chkAnimate;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.NumericUpDown udDelay;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.NumericUpDown udFrame;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.NumericUpDown udLoop;
        private BufferedPanel previewPanel;
        private System.Windows.Forms.CheckBox chkSkipFrame0;
        private System.Windows.Forms.CheckBox chkCentrePivot;
    }
}
