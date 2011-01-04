using AGS.Types;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.IO;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    public partial class AutoNumberSpeechWizardPage : WizardPage
    {
        public AutoNumberSpeechWizardPage()
        {
            InitializeComponent();
        }

        public bool EnableNarrator
        {
            get { return chkNarrator.Checked; }
        }

        public bool CombineIdenticalLines
        {
            get { return chkCombineIdenticalLines.Checked; }
        }

        public override string TitleText
        {
            get
            {
                return "Specify options for the speech numbering.";
            }
        }

        public override void PageShown()
        {
            chkNarrator.Focus();
        }

    }
}
