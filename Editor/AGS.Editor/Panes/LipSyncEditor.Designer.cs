namespace AGS.Editor
{
    partial class LipSyncEditor
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
            this.label1 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.tableLayoutPanelLP = new System.Windows.Forms.TableLayoutPanel();
            this.splitContainerMain = new System.Windows.Forms.SplitContainer();
            this.flowLayoutPanelTop = new System.Windows.Forms.FlowLayoutPanel();
            this.flowLayoutPanel1 = new System.Windows.Forms.FlowLayoutPanel();
            ((System.ComponentModel.ISupportInitialize)(this.splitContainerMain)).BeginInit();
            this.splitContainerMain.Panel1.SuspendLayout();
            this.splitContainerMain.Panel2.SuspendLayout();
            this.splitContainerMain.SuspendLayout();
            this.flowLayoutPanelTop.SuspendLayout();
            this.flowLayoutPanel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(13, 10);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(383, 22);
            this.label1.TabIndex = 0;
            this.label1.Text = "Use the property grid on the right to enable/disable lip sync.";
            this.label1.UseCompatibleTextRendering = true;
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(13, 32);
            this.label2.MaximumSize = new System.Drawing.Size(600, 0);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(567, 34);
            this.label2.TabIndex = 1;
            this.label2.Text = "In the text boxes, type the letters that will cause that frame to be shown. Separ" +
    "ate multiple letters with slashes, eg.   C/D/G/K";
            // 
            // tableLayoutPanelLP
            // 
            this.tableLayoutPanelLP.AutoSize = true;
            this.tableLayoutPanelLP.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.tableLayoutPanelLP.ColumnCount = 4;
            this.tableLayoutPanelLP.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 48F));
            this.tableLayoutPanelLP.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 150F));
            this.tableLayoutPanelLP.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 48F));
            this.tableLayoutPanelLP.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 150F));
            this.tableLayoutPanelLP.Location = new System.Drawing.Point(3, 3);
            this.tableLayoutPanelLP.MinimumSize = new System.Drawing.Size(364, 240);
            this.tableLayoutPanelLP.Name = "tableLayoutPanelLP";
            this.tableLayoutPanelLP.RowCount = 10;
            this.tableLayoutPanelLP.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanelLP.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanelLP.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanelLP.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanelLP.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanelLP.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanelLP.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanelLP.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanelLP.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanelLP.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanelLP.Size = new System.Drawing.Size(396, 240);
            this.tableLayoutPanelLP.TabIndex = 2;
            // 
            // splitContainerMain
            // 
            this.splitContainerMain.Dock = System.Windows.Forms.DockStyle.Fill;
            this.splitContainerMain.FixedPanel = System.Windows.Forms.FixedPanel.Panel1;
            this.splitContainerMain.IsSplitterFixed = true;
            this.splitContainerMain.Location = new System.Drawing.Point(0, 0);
            this.splitContainerMain.Name = "splitContainerMain";
            this.splitContainerMain.Orientation = System.Windows.Forms.Orientation.Horizontal;
            // 
            // splitContainerMain.Panel1
            // 
            this.splitContainerMain.Panel1.Controls.Add(this.flowLayoutPanelTop);
            // 
            // splitContainerMain.Panel2
            // 
            this.splitContainerMain.Panel2.Controls.Add(this.flowLayoutPanel1);
            this.splitContainerMain.Size = new System.Drawing.Size(800, 800);
            this.splitContainerMain.SplitterDistance = 80;
            this.splitContainerMain.TabIndex = 3;
            // 
            // flowLayoutPanelTop
            // 
            this.flowLayoutPanelTop.AutoSize = true;
            this.flowLayoutPanelTop.Controls.Add(this.label1);
            this.flowLayoutPanelTop.Controls.Add(this.label2);
            this.flowLayoutPanelTop.Dock = System.Windows.Forms.DockStyle.Fill;
            this.flowLayoutPanelTop.FlowDirection = System.Windows.Forms.FlowDirection.TopDown;
            this.flowLayoutPanelTop.Location = new System.Drawing.Point(0, 0);
            this.flowLayoutPanelTop.Margin = new System.Windows.Forms.Padding(10);
            this.flowLayoutPanelTop.Name = "flowLayoutPanelTop";
            this.flowLayoutPanelTop.Padding = new System.Windows.Forms.Padding(10);
            this.flowLayoutPanelTop.Size = new System.Drawing.Size(800, 80);
            this.flowLayoutPanelTop.TabIndex = 2;
            // 
            // flowLayoutPanel1
            // 
            this.flowLayoutPanel1.AutoScroll = true;
            this.flowLayoutPanel1.Controls.Add(this.tableLayoutPanelLP);
            this.flowLayoutPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.flowLayoutPanel1.Location = new System.Drawing.Point(0, 0);
            this.flowLayoutPanel1.Name = "flowLayoutPanel1";
            this.flowLayoutPanel1.Size = new System.Drawing.Size(800, 716);
            this.flowLayoutPanel1.TabIndex = 3;
            // 
            // LipSyncEditor
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(8F, 17F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.splitContainerMain);
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Name = "LipSyncEditor";
            this.Size = new System.Drawing.Size(800, 800);
            this.Load += new System.EventHandler(this.LipSyncEditor_Load);
            this.splitContainerMain.Panel1.ResumeLayout(false);
            this.splitContainerMain.Panel1.PerformLayout();
            this.splitContainerMain.Panel2.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.splitContainerMain)).EndInit();
            this.splitContainerMain.ResumeLayout(false);
            this.flowLayoutPanelTop.ResumeLayout(false);
            this.flowLayoutPanelTop.PerformLayout();
            this.flowLayoutPanel1.ResumeLayout(false);
            this.flowLayoutPanel1.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.TableLayoutPanel tableLayoutPanelLP;
        private System.Windows.Forms.SplitContainer splitContainerMain;
        private System.Windows.Forms.FlowLayoutPanel flowLayoutPanel1;
        private System.Windows.Forms.FlowLayoutPanel flowLayoutPanelTop;
    }
}
