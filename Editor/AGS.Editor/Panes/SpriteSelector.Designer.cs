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
            this.splitContainer1 = new System.Windows.Forms.SplitContainer();
            this.panel1 = new System.Windows.Forms.Panel();
            this.button_importNew = new System.Windows.Forms.Button();
            this.sliderPreviewSize = new System.Windows.Forms.TrackBar();
            this.spriteList = new System.Windows.Forms.ListView();
            ((System.ComponentModel.ISupportInitialize)(this.splitWindow)).BeginInit();
            this.splitWindow.Panel1.SuspendLayout();
            this.splitWindow.Panel2.SuspendLayout();
            this.splitWindow.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.splitContainer1)).BeginInit();
            this.splitContainer1.Panel1.SuspendLayout();
            this.splitContainer1.Panel2.SuspendLayout();
            this.splitContainer1.SuspendLayout();
            this.panel1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.sliderPreviewSize)).BeginInit();
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
            this.splitWindow.Panel2.Controls.Add(this.splitContainer1);
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
            this.folderList.AfterSelect += new System.Windows.Forms.TreeViewEventHandler(this.folderList_AfterSelect);
            this.folderList.DragDrop += new System.Windows.Forms.DragEventHandler(this.folderList_DragDrop);
            this.folderList.DragOver += new System.Windows.Forms.DragEventHandler(this.folderList_DragOver);
            this.folderList.DragLeave += new System.EventHandler(this.folderList_DragLeave);
            this.folderList.MouseUp += new System.Windows.Forms.MouseEventHandler(this.folderList_MouseUp);
            // 
            // splitContainer1
            // 
            this.splitContainer1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.splitContainer1.FixedPanel = System.Windows.Forms.FixedPanel.Panel1;
            this.splitContainer1.IsSplitterFixed = true;
            this.splitContainer1.Location = new System.Drawing.Point(0, 0);
            this.splitContainer1.Name = "splitContainer1";
            this.splitContainer1.Orientation = System.Windows.Forms.Orientation.Horizontal;
            // 
            // splitContainer1.Panel1
            // 
            this.splitContainer1.Panel1.Controls.Add(this.panel1);
            this.splitContainer1.Panel1MinSize = 48;
            // 
            // splitContainer1.Panel2
            // 
            this.splitContainer1.Panel2.Controls.Add(this.spriteList);
            this.splitContainer1.Panel2MinSize = 48;
            this.splitContainer1.Size = new System.Drawing.Size(456, 484);
            this.splitContainer1.SplitterDistance = 48;
            this.splitContainer1.TabIndex = 0;
            // 
            // panel1
            // 
            this.panel1.AutoSize = true;
            this.panel1.Controls.Add(this.button_importNew);
            this.panel1.Controls.Add(this.sliderPreviewSize);
            this.panel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.panel1.Location = new System.Drawing.Point(0, 0);
            this.panel1.MaximumSize = new System.Drawing.Size(0, 48);
            this.panel1.MinimumSize = new System.Drawing.Size(0, 48);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(456, 48);
            this.panel1.TabIndex = 0;
            // 
            // button_importNew
            // 
            this.button_importNew.Dock = System.Windows.Forms.DockStyle.Left;
            this.button_importNew.Location = new System.Drawing.Point(0, 0);
            this.button_importNew.MinimumSize = new System.Drawing.Size(128, 45);
            this.button_importNew.Name = "button_importNew";
            this.button_importNew.Size = new System.Drawing.Size(128, 48);
            this.button_importNew.TabIndex = 1;
            this.button_importNew.Text = "Import new sprite(s) from files..";
            this.button_importNew.UseVisualStyleBackColor = true;
            this.button_importNew.Click += new System.EventHandler(this.button_importNew_Click);
            // 
            // sliderPreviewSize
            // 
            this.sliderPreviewSize.Dock = System.Windows.Forms.DockStyle.Right;
            this.sliderPreviewSize.LargeChange = 1;
            this.sliderPreviewSize.Location = new System.Drawing.Point(352, 0);
            this.sliderPreviewSize.Maximum = 4;
            this.sliderPreviewSize.Minimum = 1;
            this.sliderPreviewSize.Name = "sliderPreviewSize";
            this.sliderPreviewSize.Size = new System.Drawing.Size(104, 48);
            this.sliderPreviewSize.TabIndex = 0;
            this.sliderPreviewSize.Value = 1;
            this.sliderPreviewSize.ValueChanged += new System.EventHandler(this.sliderPreviewSize_ValueChanged);
            // 
            // spriteList
            // 
            this.spriteList.AllowDrop = true;
            this.spriteList.Dock = System.Windows.Forms.DockStyle.Fill;
            this.spriteList.HideSelection = false;
            this.spriteList.Location = new System.Drawing.Point(0, 0);
            this.spriteList.Name = "spriteList";
            this.spriteList.Size = new System.Drawing.Size(456, 432);
            this.spriteList.TabIndex = 1;
            this.spriteList.UseCompatibleStateImageBehavior = false;
            this.spriteList.ItemActivate += new System.EventHandler(this.spriteList_ItemActivate);
            this.spriteList.ItemDrag += new System.Windows.Forms.ItemDragEventHandler(this.spriteList_ItemDrag);
            this.spriteList.ItemSelectionChanged += new System.Windows.Forms.ListViewItemSelectionChangedEventHandler(this.spriteList_ItemSelectionChanged);
            this.spriteList.DragDrop += new System.Windows.Forms.DragEventHandler(this.spriteList_DragDrop);
            this.spriteList.DragEnter += new System.Windows.Forms.DragEventHandler(this.spriteList_DragEnter);
            this.spriteList.DragOver += new System.Windows.Forms.DragEventHandler(this.spriteList_DragOver);
            this.spriteList.MouseUp += new System.Windows.Forms.MouseEventHandler(this.spriteList_MouseUp);
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
            ((System.ComponentModel.ISupportInitialize)(this.splitWindow)).EndInit();
            this.splitWindow.ResumeLayout(false);
            this.splitContainer1.Panel1.ResumeLayout(false);
            this.splitContainer1.Panel1.PerformLayout();
            this.splitContainer1.Panel2.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.splitContainer1)).EndInit();
            this.splitContainer1.ResumeLayout(false);
            this.panel1.ResumeLayout(false);
            this.panel1.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.sliderPreviewSize)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.SplitContainer splitWindow;
        private System.Windows.Forms.TreeView folderList;
        private System.Windows.Forms.SplitContainer splitContainer1;
        private System.Windows.Forms.ListView spriteList;
        private System.Windows.Forms.Panel panel1;
        private System.Windows.Forms.TrackBar sliderPreviewSize;
        private System.Windows.Forms.Button button_importNew;
    }
}
