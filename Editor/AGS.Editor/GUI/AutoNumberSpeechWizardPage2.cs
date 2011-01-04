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
    public partial class AutoNumberSpeechWizardPage2 : WizardPage
    {
        public AutoNumberSpeechWizardPage2(IList<Character> characters)
        {
            InitializeComponent();
            cmbWhichCharacter.Items.Add("Re-number everything");
            cmbWhichCharacter.Items.Add("Only re-number the narrator");
            foreach (Character character in characters)
            {
                cmbWhichCharacter.Items.Add("Only re-number " + character.RealName);
            }
            cmbWhichCharacter.SelectedIndex = 0;
        }

        public int? SelectedCharacterID
        {
            get
            {
                if (cmbWhichCharacter.SelectedIndex == 0)
                {
                    return null;
                }
                if (cmbWhichCharacter.SelectedIndex == 1)
                {
                    return Character.NARRATOR_CHARACTER_ID;
                }
                return cmbWhichCharacter.SelectedIndex - 2;
            }
        }

        public bool RemoveNumbering
        {
            get { return chkRemoveLines.Checked; }
        }

        public override string TitleText
        {
            get
            {
                return "Specify advanced options for the speech numbering.";
            }
        }

        public override void PageShown()
        {
            cmbWhichCharacter.Focus();
        }

    }
}
