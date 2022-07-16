namespace AGS.Editor
{
    partial class CharacterEditor
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
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.splitContainer1 = new System.Windows.Forms.SplitContainer();
            this.lblIsPlayer = new System.Windows.Forms.Label();
            this.btnMakePlayer = new System.Windows.Forms.Button();
            this.tableLayoutPanel1 = new System.Windows.Forms.TableLayoutPanel();
            this.sldZoomLevel = new AGS.Editor.ZoomTrackbar();
            this.viewPreview1 = new AGS.Editor.ViewPreview();
            this.viewPreview2 = new AGS.Editor.ViewPreview();
            this.groupBox1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.splitContainer1)).BeginInit();
            this.splitContainer1.Panel1.SuspendLayout();
            this.splitContainer1.Panel2.SuspendLayout();
            this.splitContainer1.SuspendLayout();
            this.tableLayoutPanel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.splitContainer1);
            this.groupBox1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.groupBox1.Location = new System.Drawing.Point(0, 0);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(588, 451);
            this.groupBox1.TabIndex = 5;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Selected character settings";
            // 
            // splitContainer1
            // 
            this.splitContainer1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.splitContainer1.FixedPanel = System.Windows.Forms.FixedPanel.Panel1;
            this.splitContainer1.IsSplitterFixed = true;
            this.splitContainer1.Location = new System.Drawing.Point(3, 17);
            this.splitContainer1.Name = "splitContainer1";
            this.splitContainer1.Orientation = System.Windows.Forms.Orientation.Horizontal;
            // 
            // splitContainer1.Panel1
            // 
            this.splitContainer1.Panel1.Controls.Add(this.sldZoomLevel);
            this.splitContainer1.Panel1.Controls.Add(this.lblIsPlayer);
            this.splitContainer1.Panel1.Controls.Add(this.btnMakePlayer);
            // 
            // splitContainer1.Panel2
            // 
            this.splitContainer1.Panel2.AutoScroll = true;
            this.splitContainer1.Panel2.Controls.Add(this.tableLayoutPanel1);
            this.splitContainer1.Size = new System.Drawing.Size(582, 431);
            this.splitContainer1.SplitterDistance = 80;
            this.splitContainer1.SplitterWidth = 3;
            this.splitContainer1.TabIndex = 11;
            // 
            // lblIsPlayer
            // 
            this.lblIsPlayer.AutoSize = true;
            this.lblIsPlayer.Location = new System.Drawing.Point(8, 9);
            this.lblIsPlayer.MaximumSize = new System.Drawing.Size(400, 0);
            this.lblIsPlayer.Name = "lblIsPlayer";
            this.lblIsPlayer.Size = new System.Drawing.Size(398, 26);
            this.lblIsPlayer.TabIndex = 5;
            this.lblIsPlayer.Text = "This character is OR IS NOT the player; the game will startin this character\'s ro" +
    "om or something";
            // 
            // btnMakePlayer
            // 
            this.btnMakePlayer.Location = new System.Drawing.Point(11, 43);
            this.btnMakePlayer.Name = "btnMakePlayer";
            this.btnMakePlayer.Size = new System.Drawing.Size(188, 23);
            this.btnMakePlayer.TabIndex = 6;
            this.btnMakePlayer.Text = "Make this the player character";
            this.btnMakePlayer.UseVisualStyleBackColor = true;
            this.btnMakePlayer.Click += new System.EventHandler(this.btnMakePlayer_Click);
            // 
            // tableLayoutPanel1
            // 
            this.tableLayoutPanel1.AutoSize = true;
            this.tableLayoutPanel1.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.tableLayoutPanel1.ColumnCount = 2;
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.tableLayoutPanel1.Controls.Add(this.viewPreview2, 1, 0);
            this.tableLayoutPanel1.Controls.Add(this.viewPreview1, 0, 0);
            this.tableLayoutPanel1.GrowStyle = System.Windows.Forms.TableLayoutPanelGrowStyle.FixedSize;
            this.tableLayoutPanel1.Location = new System.Drawing.Point(3, 3);
            this.tableLayoutPanel1.Name = "tableLayoutPanel1";
            this.tableLayoutPanel1.RowCount = 1;
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.tableLayoutPanel1.Size = new System.Drawing.Size(572, 334);
            this.tableLayoutPanel1.TabIndex = 0;
            // 
            // sldZoomLevel
            // 
            this.sldZoomLevel.Location = new System.Drawing.Point(356, 37);
            this.sldZoomLevel.Margin = new System.Windows.Forms.Padding(2, 2, 2, 2);
            this.sldZoomLevel.Maximum = 800;
            this.sldZoomLevel.Minimum = 100;
            this.sldZoomLevel.Name = "sldZoomLevel";
            this.sldZoomLevel.Size = new System.Drawing.Size(177, 32);
            this.sldZoomLevel.Step = 25;
            this.sldZoomLevel.TabIndex = 9;
            this.sldZoomLevel.Value = 100;
            this.sldZoomLevel.ZoomScale = 1F;
            this.sldZoomLevel.ValueChanged += new System.EventHandler(this.sldZoomLevel_ValueChanged);
            // 
            // viewPreview1
            // 
            this.viewPreview1.AutoResize = false;
            this.viewPreview1.DynamicUpdates = false;
            this.viewPreview1.IsCharacterView = false;
            this.viewPreview1.Location = new System.Drawing.Point(3, 3);
            this.viewPreview1.MinimumSize = new System.Drawing.Size(280, 320);
            this.viewPreview1.Name = "viewPreview1";
            this.viewPreview1.Size = new System.Drawing.Size(280, 328);
            this.viewPreview1.TabIndex = 9;
            this.viewPreview1.Title = "Normal view";
            this.viewPreview1.ViewToPreview = null;
            this.viewPreview1.ZoomLevel = 1F;
            // 
            // viewPreview2
            // 
            this.viewPreview2.AutoResize = false;
            this.viewPreview2.DynamicUpdates = false;
            this.viewPreview2.IsCharacterView = false;
            this.viewPreview2.Location = new System.Drawing.Point(289, 3);
            this.viewPreview2.MinimumSize = new System.Drawing.Size(280, 320);
            this.viewPreview2.Name = "viewPreview2";
            this.viewPreview2.Size = new System.Drawing.Size(280, 328);
            this.viewPreview2.TabIndex = 10;
            this.viewPreview2.Title = "Speech view";
            this.viewPreview2.ViewToPreview = null;
            this.viewPreview2.ZoomLevel = 1F;
            // 
            // CharacterEditor
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(96F, 96F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
            this.Controls.Add(this.groupBox1);
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Name = "CharacterEditor";
            this.Size = new System.Drawing.Size(588, 451);
            this.Load += new System.EventHandler(this.CharacterEditor_Load);
            this.groupBox1.ResumeLayout(false);
            this.splitContainer1.Panel1.ResumeLayout(false);
            this.splitContainer1.Panel1.PerformLayout();
            this.splitContainer1.Panel2.ResumeLayout(false);
            this.splitContainer1.Panel2.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.splitContainer1)).EndInit();
            this.splitContainer1.ResumeLayout(false);
            this.tableLayoutPanel1.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.Button btnMakePlayer;
        private System.Windows.Forms.Label lblIsPlayer;
        private ViewPreview viewPreview2;
        private ViewPreview viewPreview1;
        private System.Windows.Forms.SplitContainer splitContainer1;
        private ZoomTrackbar sldZoomLevel;
        private System.Windows.Forms.TableLayoutPanel tableLayoutPanel1;
    }
}
