using AGS.Types;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    public partial class AssignToView : Form
    {
        private const int DEFAULT_LOOPS_IN_VIEW = 8;

        private static int _LastViewNumber = 0;
        private static int _LastLoopNumber = 0;

        private int _viewNumber = 1;
        private int _loopNumber;
        private bool _addFramesToExistingLoop;
        private bool _continueIntoNextLoop;
        private bool _flipFrames;
        private bool _reverseFrames;

        public AssignToView()
        {
            InitializeComponent();
            udView.Minimum = 1;
            udView.Maximum = Factory.AGSEditor.CurrentGame.ViewCount;
            if ((_LastViewNumber > 0) && (_LastViewNumber <= udView.Maximum))
            {
                udView.Value = _LastViewNumber;
            }
            else
            {
                _LastLoopNumber = 0;
            }
            PopulateLoopList(_LastLoopNumber);
            radOverwrite.Checked = true;
            radStopAssigning.Checked = true;
            Utilities.CheckLabelWidthsOnForm(this);
        }

        public int ViewNumber
        {
            get { return _viewNumber; }
        }

        public int LoopNumber
        {
            get { return _loopNumber; }
        }

        public bool AddFramesToExistingLoop
        {
            get { return _addFramesToExistingLoop; }
        }

        public bool ContinueIntoNextLoop
        {
            get { return _continueIntoNextLoop; }
        }

        public bool FlipFrames
        {
            get { return _flipFrames; }
        }

        public bool ReverseFrames
        {
            get { return _reverseFrames; }
        }

        private void btnChooseView_Click(object sender, EventArgs e)
        {
            AGS.Types.View chosen = ViewChooser.ShowViewChooser(this, (int)udView.Value);
            if (chosen != null)
            {
                udView.Value = chosen.ID;
            }
        }

        private void btnOK_Click(object sender, EventArgs e)
        {
            _viewNumber = (int)udView.Value;
            _loopNumber = cmbLoop.SelectedIndex;
            _addFramesToExistingLoop = radAddToExisting.Checked;
            _continueIntoNextLoop = radOverwriteNextLoop.Checked;
            _flipFrames = chkFlipped.Checked;
            _reverseFrames = chkReverse.Checked;
            _LastViewNumber = _viewNumber;
            _LastLoopNumber = _loopNumber;
            this.DialogResult = DialogResult.OK;
            this.Close();
        }

        private void btnCancel_Click(object sender, EventArgs e)
        {
            this.DialogResult = DialogResult.Cancel;
            this.Close();
        }

        private void PopulateLoopList(int selectedLoopNumber)
        {
            int numLoops = DEFAULT_LOOPS_IN_VIEW;
            AGS.Types.View selectedView = Factory.AGSEditor.CurrentGame.FindViewByID(_viewNumber);
            if (selectedView != null)
            {
                numLoops = Math.Max(selectedView.Loops.Count, numLoops);
            }

            cmbLoop.Items.Clear();
            for (int i = 0; i < numLoops; i++)
            {
                string description = string.Empty;
                if (i < ViewLoop.DirectionNames.Length)
                {
                    description = "(" + ViewLoop.DirectionNames[i] + ")";
                }
                cmbLoop.Items.Add("Loop " + i + " " + description);
            }

            if (selectedLoopNumber < cmbLoop.Items.Count)
            {
                cmbLoop.SelectedIndex = selectedLoopNumber;
            }
            else
            {
                cmbLoop.SelectedIndex = 0;
            }
        }

        private void udView_ValueChanged(object sender, EventArgs e)
        {
            _viewNumber = (int)udView.Value;

            PopulateLoopList(cmbLoop.SelectedIndex);
        }
    }
}