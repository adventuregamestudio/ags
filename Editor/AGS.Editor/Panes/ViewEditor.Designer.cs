namespace AGS.Editor
{
    partial class ViewEditor
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
            this.chkShowPreview = new System.Windows.Forms.CheckBox();
            this.editorPanel = new System.Windows.Forms.Panel();
            this.btnDeleteLastLoop = new System.Windows.Forms.Button();
            this.btnNewLoop = new System.Windows.Forms.Button();
            this.viewPreview = new AGS.Editor.ViewPreview();
            this.editorPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // chkShowPreview
            // 
            this.chkShowPreview.AutoSize = true;
            this.chkShowPreview.Location = new System.Drawing.Point(12, 12);
            this.chkShowPreview.Name = "chkShowPreview";
            this.chkShowPreview.Size = new System.Drawing.Size(93, 17);
            this.chkShowPreview.TabIndex = 2;
            this.chkShowPreview.Text = "Show Preview";
            this.chkShowPreview.UseVisualStyleBackColor = true;
            this.chkShowPreview.CheckedChanged += new System.EventHandler(this.chkShowPreview_CheckedChanged);
            // 
            // editorPanel
            // 
            this.editorPanel.AutoScroll = true;
            this.editorPanel.Controls.Add(this.btnDeleteLastLoop);
            this.editorPanel.Controls.Add(this.btnNewLoop);
            this.editorPanel.Location = new System.Drawing.Point(293, 35);
            this.editorPanel.Name = "editorPanel";
            this.editorPanel.Size = new System.Drawing.Size(359, 444);
            this.editorPanel.TabIndex = 4;
            // 
            // btnDeleteLastLoop
            // 
            this.btnDeleteLastLoop.Location = new System.Drawing.Point(187, 72);
            this.btnDeleteLastLoop.Name = "btnDeleteLastLoop";
            this.btnDeleteLastLoop.Size = new System.Drawing.Size(132, 31);
            this.btnDeleteLastLoop.TabIndex = 3;
            this.btnDeleteLastLoop.Text = "Delete last loop";
            this.btnDeleteLastLoop.UseVisualStyleBackColor = true;
            this.btnDeleteLastLoop.Click += new System.EventHandler(this.btnDeleteLastLoop_Click);
            // 
            // btnNewLoop
            // 
            this.btnNewLoop.Location = new System.Drawing.Point(20, 72);
            this.btnNewLoop.Name = "btnNewLoop";
            this.btnNewLoop.Size = new System.Drawing.Size(132, 31);
            this.btnNewLoop.TabIndex = 2;
            this.btnNewLoop.Text = "Create new loop";
            this.btnNewLoop.UseVisualStyleBackColor = true;
            this.btnNewLoop.Click += new System.EventHandler(this.btnNewLoop_Click);
            // 
            // viewPreview
            // 
            this.viewPreview.DynamicUpdates = false;
            this.viewPreview.IsCharacterView = false;
            this.viewPreview.Location = new System.Drawing.Point(12, 35);
            this.viewPreview.Name = "viewPreview";
            this.viewPreview.Size = new System.Drawing.Size(275, 332);
            this.viewPreview.TabIndex = 3;
            this.viewPreview.Title = "Preview";
            this.viewPreview.ViewToPreview = null;
            this.viewPreview.Visible = false;
            // 
            // ViewEditor
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.editorPanel);
            this.Controls.Add(this.chkShowPreview);
            this.Controls.Add(this.viewPreview);
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Name = "ViewEditor";
            this.Size = new System.Drawing.Size(674, 514);
            this.Load += new System.EventHandler(this.ViewEditor_Load);
            this.Resize += new System.EventHandler(this.ViewEditor_Resize);
            this.editorPanel.ResumeLayout(false);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.CheckBox chkShowPreview;
        private ViewPreview viewPreview;
        private System.Windows.Forms.Panel editorPanel;
        private System.Windows.Forms.Button btnDeleteLastLoop;
        private System.Windows.Forms.Button btnNewLoop;

    }
}
