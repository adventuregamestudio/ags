using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace AGS.Editor
{
    public partial class WizardIntroPage : WizardPage
    {
        public WizardIntroPage()
        {
            InitializeComponent();
            // this is to make cup picture appear transparent over background
            pictureBox1.Controls.Add(pictureBox2);
        }

        public WizardIntroPage(string wizardName, string introText)
        {
            InitializeComponent();
            // this is to make cup picture appear transparent over background
            pictureBox1.Controls.Add(pictureBox2);

            lblTitle.Text = "Welcome to the " + wizardName + " Wizard";
            lblIntroText.Text = introText;
        }
    }
}
