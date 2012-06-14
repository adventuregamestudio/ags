namespace AGS.Editor
{
    partial class ProjectPanel
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(ProjectPanel));
            this.projectTree = new System.Windows.Forms.TreeView();
            this.SuspendLayout();
            // 
            // projectTree
            // 
            this.projectTree.AllowDrop = true;
            this.projectTree.Dock = System.Windows.Forms.DockStyle.Fill;
            this.projectTree.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.projectTree.HideSelection = false;
            this.projectTree.LabelEdit = true;
            this.projectTree.Location = new System.Drawing.Point(1, 20);
            this.projectTree.Name = "projectTree";
            this.projectTree.Size = new System.Drawing.Size(290, 239);
            this.projectTree.TabIndex = 2;
            // 
            // ProjectPanel
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(8F, 16F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(292, 260);
            this.Controls.Add(this.projectTree);
            this.Font = new System.Drawing.Font("Microsoft Sans Serif", 7.8F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Name = "ProjectPanel";
            this.Padding = new System.Windows.Forms.Padding(1, 20, 1, 1);
            this.ResumeLayout(false);

        }

        #endregion

        internal System.Windows.Forms.TreeView projectTree;
    }
}
