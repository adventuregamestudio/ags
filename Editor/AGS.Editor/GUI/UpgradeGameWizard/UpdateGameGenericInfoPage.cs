using System;
using AGS.Types;

namespace AGS.Editor
{
    public class UpdateGameGenericInfoPage : UpgradeGameWizardPage
    {
        private System.Windows.Forms.RichTextBox richDescription;

        public UpdateGameGenericInfoPage(Game game, IUpgradeGameTask task)
            : base(game, task)
        {
            InitializeComponent();

            richDescription.Text = task.Description;
        }

        private void InitializeComponent()
        {
            this.richDescription = new System.Windows.Forms.RichTextBox();
            this.SuspendLayout();
            // 
            // richDescription
            // 
            this.richDescription.BackColor = System.Drawing.SystemColors.Control;
            this.richDescription.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.richDescription.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this.richDescription.Location = new System.Drawing.Point(16, 16);
            this.richDescription.Name = "richDescription";
            this.richDescription.ReadOnly = true;
            this.richDescription.ScrollBars = System.Windows.Forms.RichTextBoxScrollBars.None;
            this.richDescription.Size = new System.Drawing.Size(516, 262);
            this.richDescription.TabIndex = 1;
            this.richDescription.Text = "Info text\n";
            // 
            // UpdateGameGenericInfoPage
            // 
            this.Controls.Add(this.richDescription);
            this.Name = "UpdateGameGenericInfoPage";
            this.Padding = new System.Windows.Forms.Padding(13);
            this.Size = new System.Drawing.Size(1398, 628);
            this.ResumeLayout(false);

        }
    }
}
