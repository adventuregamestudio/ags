using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    public class WizardPage : UserControl
    {
        public WizardPage()
            : base()
        {
            this.Dock = DockStyle.Fill;
        }

        public virtual bool NextButtonPressed()
        {
            return true;
        }

        public virtual string TitleText
        {
            get { return string.Empty; }
        }

        public virtual void PageShown()
        {
        }

        private void InitializeComponent()
        {
            this.SuspendLayout();
            // 
            // WizardPage
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(96F, 96F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
            this.Name = "WizardPage";
            this.ResumeLayout(false);

        }
    }
}
