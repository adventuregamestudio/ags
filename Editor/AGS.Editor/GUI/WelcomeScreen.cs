using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    public enum WelcomeScreenSelection
    {
        StartNewGame,
        LoadExistingGame,
        ContinueRecentGame
    }

    public partial class WelcomeScreen : Form
    {
        private List<RecentlyEditedGame> _recentGames;

        public WelcomeScreen(RecentGamesList recentGames)
        {
            InitializeComponent();
			this.Shown += new EventHandler(WelcomeScreen_Shown);
            _recentGames = recentGames.RecentGames;

            if (_recentGames.Count == 0)
            {
                radRecent.Enabled = false;
                lstRecentGames.Enabled = false;
                radNewGame.Checked = true;
            }
            else
            {
                radRecent.Checked = true;
                foreach (RecentlyEditedGame game in _recentGames)
                {
                    lstRecentGames.Items.Add(game.GameName).SubItems.Add(game.DirectoryPath);
                }
                lstRecentGames.SelectedIndices.Add(0);
            }

		}

		private void WelcomeScreen_Shown(object sender, EventArgs e)
		{
			Factory.GUIController.ShowCuppit("Welcome to AGS! I'm Cuppit, here to help you out along the way. As you're new, you'll probably want to choose the Start New Game option.\nIf you press the Stop Bugging Me button, I won't tell you this hint again.", "Welcome text");
		}

        public RecentlyEditedGame SelectedRecentGame
        {
            get { return _recentGames[lstRecentGames.SelectedIndices[0]]; }
        }

        public WelcomeScreenSelection SelectedOption
        {
            get { return (radNewGame.Checked) ? WelcomeScreenSelection.StartNewGame : (radLoadGame.Checked ? WelcomeScreenSelection.LoadExistingGame : WelcomeScreenSelection.ContinueRecentGame); }
        }

        private void lstRecentGames_ItemActivate(object sender, EventArgs e)
        {
            btnContinue_Click(sender, e);
        }

        private void btnContinue_Click(object sender, EventArgs e)
        {
            if ((radRecent.Checked) && (lstRecentGames.SelectedIndices.Count == 0))
            {
                MessageBox.Show("You must select a game to edit.", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Information);
                return;
            }
            this.DialogResult = DialogResult.OK;
            this.Close();
        }

        private void radRecent_CheckedChanged(object sender, EventArgs e)
        {
            lstRecentGames.Enabled = radRecent.Checked;
        }
    }
}