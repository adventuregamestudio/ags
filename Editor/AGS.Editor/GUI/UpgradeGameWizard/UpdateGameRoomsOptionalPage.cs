using System;
using AGS.Types;
using AGS.Editor.Components;

namespace AGS.Editor
{
    public class UpdateGameRoomsOptionalPage : UpgradeGameWizardPage
    {
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.TableLayoutPanel tableLayoutPanel1;
        private System.Windows.Forms.CheckBox chkAdjustAllObjectsBy1Pixel;
        private System.Windows.Forms.Label label1;

        private UpgradeGameRoomsOptionalTask _roomsOptionalTask;

        public UpdateGameRoomsOptionalPage(Game game, IUpgradeGameTask task)
            : base(game, task)
        {
            InitializeComponent();
            // NOTE: we may use game.SavedXmlVersion to decide which pages
            // and/or options to display!
            _roomsOptionalTask = task as UpgradeGameRoomsOptionalTask;
        }

        public override string TitleText
        {
            get { return Task.Title; }
        }

        public override bool NextButtonPressed()
        {
            RoomsComponent.UpgradeOptions options = new RoomsComponent.UpgradeOptions();
            options.AdjustObjectsBy1YPixel = chkAdjustAllObjectsBy1Pixel.Checked;
            _roomsOptionalTask.Options = options;
            return true;
        }

        private void InitializeComponent()
        {
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(UpdateGameRoomsOptionalPage));
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.chkAdjustAllObjectsBy1Pixel = new System.Windows.Forms.CheckBox();
            this.label2 = new System.Windows.Forms.Label();
            this.label1 = new System.Windows.Forms.Label();
            this.tableLayoutPanel1 = new System.Windows.Forms.TableLayoutPanel();
            this.groupBox1.SuspendLayout();
            this.tableLayoutPanel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.chkAdjustAllObjectsBy1Pixel);
            this.groupBox1.Controls.Add(this.label2);
            this.groupBox1.Location = new System.Drawing.Point(3, 65);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Padding = new System.Windows.Forms.Padding(10);
            this.groupBox1.Size = new System.Drawing.Size(597, 202);
            this.groupBox1.TabIndex = 0;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Room objects displayed 1 pixel lower";
            // 
            // chkAdjustAllObjectsBy1Pixel
            // 
            this.chkAdjustAllObjectsBy1Pixel.AutoSize = true;
            this.chkAdjustAllObjectsBy1Pixel.Checked = true;
            this.chkAdjustAllObjectsBy1Pixel.CheckState = System.Windows.Forms.CheckState.Checked;
            this.chkAdjustAllObjectsBy1Pixel.Location = new System.Drawing.Point(13, 173);
            this.chkAdjustAllObjectsBy1Pixel.Name = "chkAdjustAllObjectsBy1Pixel";
            this.chkAdjustAllObjectsBy1Pixel.Size = new System.Drawing.Size(263, 17);
            this.chkAdjustAllObjectsBy1Pixel.TabIndex = 1;
            this.chkAdjustAllObjectsBy1Pixel.Text = "Adjust ALL Room Objects in ALL Rooms by 1 pixel";
            this.chkAdjustAllObjectsBy1Pixel.UseVisualStyleBackColor = true;
            this.chkAdjustAllObjectsBy1Pixel.CheckedChanged += new System.EventHandler(this.chkAdjustAllObjectsBy1Pixel_CheckedChanged);
            // 
            // label2
            // 
            this.label2.Dock = System.Windows.Forms.DockStyle.Top;
            this.label2.Location = new System.Drawing.Point(10, 23);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(577, 147);
            this.label2.TabIndex = 0;
            this.label2.Text = resources.GetString("label2.Text");
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(3, 0);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(591, 52);
            this.label1.TabIndex = 1;
            this.label1.Text = resources.GetString("label1.Text");
            // 
            // tableLayoutPanel1
            // 
            this.tableLayoutPanel1.ColumnCount = 1;
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.tableLayoutPanel1.Controls.Add(this.label1, 0, 0);
            this.tableLayoutPanel1.Controls.Add(this.groupBox1, 0, 2);
            this.tableLayoutPanel1.Location = new System.Drawing.Point(16, 16);
            this.tableLayoutPanel1.Name = "tableLayoutPanel1";
            this.tableLayoutPanel1.RowCount = 3;
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 10F));
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanel1.Size = new System.Drawing.Size(603, 370);
            this.tableLayoutPanel1.TabIndex = 2;
            // 
            // UpdateGameRoomsOptionalPage
            // 
            this.Controls.Add(this.tableLayoutPanel1);
            this.MinimumSize = new System.Drawing.Size(640, 320);
            this.Name = "UpdateGameRoomsOptionalPage";
            this.Padding = new System.Windows.Forms.Padding(13);
            this.Size = new System.Drawing.Size(1193, 715);
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.tableLayoutPanel1.ResumeLayout(false);
            this.tableLayoutPanel1.PerformLayout();
            this.ResumeLayout(false);

        }

        private void chkAdjustAllObjectsBy1Pixel_CheckedChanged(object sender, EventArgs e)
        {

        }
    }
}
