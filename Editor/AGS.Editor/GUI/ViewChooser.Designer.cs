namespace AGS.Editor
{
    partial class ViewChooser
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
            this.viewTree = new System.Windows.Forms.TreeView();
            this.btnOK = new System.Windows.Forms.Button();
            this.btnCancel = new System.Windows.Forms.Button();
            this.viewPreview = new AGS.Editor.ViewPreview();
            this.SuspendLayout();
            // 
            // viewTree
            // 
            this.viewTree.Dock = System.Windows.Forms.DockStyle.Left;
            this.viewTree.Location = new System.Drawing.Point(0, 0);
            this.viewTree.Name = "viewTree";
            this.viewTree.Size = new System.Drawing.Size(240, 383);
            this.viewTree.TabIndex = 0;
            this.viewTree.NodeMouseDoubleClick += new System.Windows.Forms.TreeNodeMouseClickEventHandler(this.viewTree_NodeMouseDoubleClick);
            this.viewTree.AfterSelect += new System.Windows.Forms.TreeViewEventHandler(this.viewTree_AfterSelect);
            // 
            // btnOK
            // 
            this.btnOK.Location = new System.Drawing.Point(260, 345);
            this.btnOK.Name = "btnOK";
            this.btnOK.Size = new System.Drawing.Size(105, 28);
            this.btnOK.TabIndex = 1;
            this.btnOK.Text = "Use this view";
            this.btnOK.UseVisualStyleBackColor = true;
            this.btnOK.Click += new System.EventHandler(this.btnOK_Click);
            // 
            // btnCancel
            // 
            this.btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnCancel.Location = new System.Drawing.Point(385, 346);
            this.btnCancel.Name = "btnCancel";
            this.btnCancel.Size = new System.Drawing.Size(98, 26);
            this.btnCancel.TabIndex = 2;
            this.btnCancel.Text = "Cancel";
            this.btnCancel.UseVisualStyleBackColor = true;
            this.btnCancel.Click += new System.EventHandler(this.btnCancel_Click);
            // 
            // viewPreview
            // 
            this.viewPreview.Location = new System.Drawing.Point(257, 12);
            this.viewPreview.Name = "viewPreview";
            this.viewPreview.Size = new System.Drawing.Size(274, 328);
            this.viewPreview.TabIndex = 11;
            this.viewPreview.Title = "Preview selected view";
            this.viewPreview.ViewToPreview = null;
            // 
            // ViewChooser
            // 
            this.AcceptButton = this.btnOK;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this.btnCancel;
            this.ClientSize = new System.Drawing.Size(535, 383);
            this.Controls.Add(this.viewPreview);
            this.Controls.Add(this.btnCancel);
            this.Controls.Add(this.btnOK);
            this.Controls.Add(this.viewTree);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "ViewChooser";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Select a View";
            this.Load += new System.EventHandler(this.ViewChooser_Load);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.TreeView viewTree;
        private System.Windows.Forms.Button btnOK;
        private System.Windows.Forms.Button btnCancel;
        private ViewPreview viewPreview;
    }
}