using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    public partial class GotoLineDialog : Form
    {
        public GotoLineDialog()
        {
            InitializeComponent();
        }

        public int LineNumber
        {
            get { return (int)upDownLineNumber.Value; }
            set { upDownLineNumber.Value = value; }
        }

        public int Minimum
        {
            get { return (int)upDownLineNumber.Minimum; }
            set 
            { 
                upDownLineNumber.Minimum = value;
                SetLabelText();
            }
        }

        public int Maximum
        {
            get { return (int)upDownLineNumber.Maximum; }
            set
            {
                upDownLineNumber.Maximum = value;
                SetLabelText();
            }
        }

        private void SetLabelText()
        {
            lblLineNumber.Text = String.Format("Line Number ({0} - {1}):", Minimum, Maximum);
        }

    }
}
