using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using Microsoft.Win32;

namespace AGS.Editor
{
    public partial class HelpPopup : Form
    {
        private const string HIDDEN_HELP_REGISTRY_KEY = @"Software\Adventure Game Studio\AGS Editor\HelpMessages";

        private string _helpTextID;
        private bool _isModal;
        private Dictionary<string, object> _hiddenMessages = new Dictionary<string, object>();
        private Timer _timer;
        private Bitmap _backImage1;
        private Bitmap _backImage2;
        private bool _image2Displayed;

        public HelpPopup()
        {
            InitializeComponent();

            _backImage1 = Resources.ResourceManager.GetBitmap("cuppit-back.gif");
            _backImage2 = Resources.ResourceManager.GetBitmap("cuppit-back2.gif");
            this.BackgroundImage = _backImage1;
            this.TransparencyKey = Color.FromArgb(254, 254, 254);

            _timer = new Timer();
            _timer.Interval = 500;
            _timer.Tick += new EventHandler(_timer_Tick);
            _timer.Enabled = true;

            ReadSettingsFromRegistry();
        }

        private void _timer_Tick(object sender, EventArgs e)
        {
            _image2Displayed = !_image2Displayed;
            this.BackgroundImage = (_image2Displayed) ? _backImage2 : _backImage1;
        }

        private void ReadSettingsFromRegistry()
        {
            RegistryKey key = Registry.CurrentUser.OpenSubKey(HIDDEN_HELP_REGISTRY_KEY);
            if (key != null)
            {
                foreach (string valueName in key.GetValueNames())
                {
                    _hiddenMessages.Add(valueName, null);
                }
                key.Close();
            }
        }

        private void WriteSettingsToRegistry()
        {
            RegistryKey key = Registry.CurrentUser.CreateSubKey(HIDDEN_HELP_REGISTRY_KEY);
            if (key != null)
            {
                foreach (string message in _hiddenMessages.Keys)
                {
                    key.SetValue(message, "1");
                }
                key.Close();
            }
        }

        public void SetHelpText(string newText)
        {
            lblHelpText.Text = newText;
        }

        public bool ShouldShow(string id, bool isModal)
        {
            _isModal = isModal;

            if (_hiddenMessages.ContainsKey(id))
            {
                return false;
            }

            if (this.Visible)
            {
                // already shown, hide it first
                this.Hide();
            }

            _helpTextID = id;
            _timer.Enabled = true;
            return true;
        }

        private void btnOK_Click(object sender, EventArgs e)
        {
            _timer.Enabled = false;

            if (_isModal)
            {
                this.Close();
            }
            else
            {
                this.Hide();
            }
        }

        private void btnStopBuggingMe_Click(object sender, EventArgs e)
        {
            _hiddenMessages.Add(_helpTextID, null);
            WriteSettingsToRegistry();

            btnOK_Click(sender, e);
        }

    }
}