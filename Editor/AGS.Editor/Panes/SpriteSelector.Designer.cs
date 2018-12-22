namespace AGS.Editor
{
    partial class SpriteSelector
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
			this.splitWindow = new System.Windows.Forms.SplitContainer();
			this.folderList = new System.Windows.Forms.TreeView();
			this.spriteList = new System.Windows.Forms.ListView();
			this.splitWindow.Panel1.SuspendLayout();
			this.splitWindow.Panel2.SuspendLayout();
			this.splitWindow.SuspendLayout();
			this.SuspendLayout();
			// 
			// splitWindow
			// 
			this.splitWindow.Dock = System.Windows.Forms.DockStyle.Fill;
			this.splitWindow.FixedPanel = System.Windows.Forms.FixedPanel.Panel1;
			this.splitWindow.Location = new System.Drawing.Point(0, 0);
			this.splitWindow.Name = "splitWindow";
			// 
			// splitWindow.Panel1
			// 
			this.splitWindow.Panel1.Controls.Add(this.folderList);
			// 
			// splitWindow.Panel2
			// 
			this.splitWindow.Panel2.Controls.Add(this.spriteList);
			this.splitWindow.Size = new System.Drawing.Size(640, 484);
			this.splitWindow.SplitterDistance = 180;
			this.splitWindow.TabIndex = 0;
			// 
			// folderList
			// 
			this.folderList.AllowDrop = true;
			this.folderList.Dock = System.Windows.Forms.DockStyle.Fill;
			this.folderList.HideSelection = false;
			this.folderList.LabelEdit = true;
			this.folderList.Location = new System.Drawing.Point(0, 0);
			this.folderList.Name = "folderList";
			this.folderList.Size = new System.Drawing.Size(180, 484);
			this.folderList.TabIndex = 1;
			this.folderList.AfterLabelEdit += new System.Windows.Forms.NodeLabelEditEventHandler(this.folderList_AfterLabelEdit);
			this.folderList.MouseUp += new System.Windows.Forms.MouseEventHandler(this.folderList_MouseUp);
			this.folderList.DragDrop += new System.Windows.Forms.DragEventHandler(this.folderList_DragDrop);
			this.folderList.AfterSelect += new System.Windows.Forms.TreeViewEventHandler(this.folderList_AfterSelect);
			this.folderList.DragOver += new System.Windows.Forms.DragEventHandler(this.folderList_DragOver);
			this.folderList.DragLeave += new System.EventHandler(this.folderList_DragLeave);
			// 
			// spriteList
			// 
			this.spriteList.AllowDrop = true;
			this.spriteList.Dock = System.Windows.Forms.DockStyle.Fill;
			this.spriteList.HideSelection = false;
			this.spriteList.Location = new System.Drawing.Point(0, 0);
			this.spriteList.Name = "spriteList";
			this.spriteList.Size = new System.Drawing.Size(456, 484);
			this.spriteList.TabIndex = 0;
			this.spriteList.UseCompatibleStateImageBehavior = false;
			this.spriteList.ItemActivate += new System.EventHandler(this.spriteList_ItemActivate);
			this.spriteList.ItemSelectionChanged += new System.Windows.Forms.ListViewItemSelectionChangedEventHandler(this.spriteList_ItemSelectionChanged);
			this.spriteList.MouseUp += new System.Windows.Forms.MouseEventHandler(this.spriteList_MouseUp);
			this.spriteList.DragDrop += new System.Windows.Forms.DragEventHandler(this.spriteList_DragDrop);
			this.spriteList.ItemDrag += new System.Windows.Forms.ItemDragEventHandler(this.spriteList_ItemDrag);
			this.spriteList.DragOver += new System.Windows.Forms.DragEventHandler(this.spriteList_DragOver);
			// 
			// SpriteSelector
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.Controls.Add(this.splitWindow);
			this.Name = "SpriteSelector";
			this.Size = new System.Drawing.Size(640, 484);
			this.splitWindow.Panel1.ResumeLayout(false);
			this.splitWindow.Panel2.ResumeLayout(false);
			this.splitWindow.ResumeLayout(false);
			this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.SplitContainer splitWindow;
        private System.Windows.Forms.TreeView folderList;
        private System.Windows.Forms.ListView spriteList;
    }
}
