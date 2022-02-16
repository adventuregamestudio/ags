using System;
using System.Collections.Generic;
using System.Linq;
using System.ComponentModel;
using System.Reflection;
using System.Windows.Forms;
using AGS.Editor.Utils;

namespace AGS.Editor
{
    public partial class GenerateAndroidKeystore : Form
    {
        private AndroidKeystoreData _ks;
        private AndroidKeystoreData _lastGeneratedKS;

        private Dictionary<TextBox, PropertyInfo> _TbProp;

        public GenerateAndroidKeystoreResponse Response;

        public GenerateAndroidKeystore()
        {
            InitializeComponent();
            buttonOK.Enabled = false;
            _ks = new AndroidKeystoreData();
            _lastGeneratedKS = null;
            Response = null;
            _TbProp = new Dictionary<TextBox, PropertyInfo>
            {
                {textboxKeystorePath, typeof(AndroidKeystoreData).GetProperty("KeystorePath") },
                {textboxKeystorePassword, typeof(AndroidKeystoreData).GetProperty("Password") },
                {textboxKeystoreKeyAlias, typeof(AndroidKeystoreData).GetProperty("KeyAlias") },
                {textboxKeystoreKeyPassword, typeof(AndroidKeystoreData).GetProperty("KeyPassword") },
                {textboxKeystoreCertCN, typeof(AndroidKeystoreData).GetProperty("FirstAndLastName") },
                {textboxKeystoreCertOU, typeof(AndroidKeystoreData).GetProperty("OrganizationalUnit") },
                {textboxKeystoreCertO, typeof(AndroidKeystoreData).GetProperty("OrganizationName") },
                {textboxKeystoreCertL, typeof(AndroidKeystoreData).GetProperty("CityOrLocality") },
                {textboxKeystoreCertST, typeof(AndroidKeystoreData).GetProperty("StateOrProvince") },
                {textboxKeystoreCertC, typeof(AndroidKeystoreData).GetProperty("CountryCode") }
            };
        }

        private void UpdateAndValidate(object sender, bool inChangedEvent = false)
        {
            // AndroidKeystoreData Properties already check and only set non-invalid characters
            // We are going use this to filter unwanted data

            TextBox tb = sender as TextBox;
            if (tb == null) return;

            if (inChangedEvent && tb.Text.Length > 2)
            {
                // we allow typing a space character in this case, because some certificate info may have spaces

                int len = tb.Text.Length;
                if((tb.Text[len - 1] == ' ') && (tb.Text[len - 2] != ' '))
                {
                    return;
                }
            }

            string ks_val = (string)_TbProp[tb].GetValue(_ks);

            if (ks_val == tb.Text) return;

            _TbProp[tb].SetValue(_ks, tb.Text);

            // after evaluating and trimming, the property may be still empty
            ks_val = (string)_TbProp[tb].GetValue(_ks);
            if (string.IsNullOrEmpty(ks_val)) return;

            // if it's not, then update back
            tb.Text = ks_val;
            tb.SelectionStart = tb.Text.Length;
            tb.SelectionLength = 0;
        }

        private void UpdateAndValidateAllowSpaces(object sender)
        {
            UpdateAndValidate(sender, true);
        }

        private void numericValidityYears_Validating(object sender, CancelEventArgs e)
        {
            if (_ks.ValidityInYears == (int)numericValidityYears.Value) return;
            numericValidityYears.Value = _ks.ValidityInYears = (int)numericValidityYears.Value;
        }

        // TextBox Validating Events

        private void textboxKeystorePath_Validating(object sender, CancelEventArgs e)
        {
            UpdateAndValidate(sender);
        }

        private void textboxKeystorePassword_Validating(object sender, CancelEventArgs e)
        {
            UpdateAndValidate(sender);
        }

        private void textboxKeystoreKeyAlias_Validating(object sender, CancelEventArgs e)
        {
            UpdateAndValidate(sender);
        }

        private void textboxKeystoreKeyPassword_Validating(object sender, CancelEventArgs e)
        {
            UpdateAndValidate(sender);
        }


        private void textboxKeystoreCertCN_Validating(object sender, CancelEventArgs e)
        {
            UpdateAndValidate(sender);
        }

        private void textboxKeystoreCertOU_Validating(object sender, CancelEventArgs e)
        {
            UpdateAndValidate(sender);
        }

        private void textboxKeystoreCertO_Validating(object sender, CancelEventArgs e)
        {
            UpdateAndValidate(sender);
        }

        private void textboxKeystoreCertL_Validating(object sender, CancelEventArgs e)
        {
            UpdateAndValidate(sender);
        }

        private void textboxKeystoreCertST_Validating(object sender, CancelEventArgs e)
        {
            UpdateAndValidate(sender);
        }

        private void textboxKeystoreCertC_Validating(object sender, CancelEventArgs e)
        {
            UpdateAndValidate(sender);
        }

        // TextChanged Events

        private void textboxKeystorePath_TextChanged(object sender, EventArgs e)
        {
            UpdateAndValidateAllowSpaces(sender);
        }

        private void textboxKeystorePassword_TextChanged(object sender, EventArgs e)
        {
            UpdateAndValidateAllowSpaces(sender);
        }

        private void textboxKeystoreKeyAlias_TextChanged(object sender, EventArgs e)
        {
            UpdateAndValidateAllowSpaces(sender);
        }

        private void textboxKeystoreKeyPassword_TextChanged(object sender, EventArgs e)
        {
            UpdateAndValidateAllowSpaces(sender);
        }

        private void textboxKeystoreCertCN_TextChanged(object sender, EventArgs e)
        {
            UpdateAndValidateAllowSpaces(sender);
        }

        private void textboxKeystoreCertOU_TextChanged(object sender, EventArgs e)
        {
            UpdateAndValidateAllowSpaces(sender);
        }

        private void textboxKeystoreCertO_TextChanged(object sender, EventArgs e)
        {
            UpdateAndValidateAllowSpaces(sender);
        }

        private void textboxKeystoreCertL_TextChanged(object sender, EventArgs e)
        {
            UpdateAndValidateAllowSpaces(sender);
        }

        private void textboxKeystoreCertST_TextChanged(object sender, EventArgs e)
        {
            UpdateAndValidateAllowSpaces(sender);
        }

        private void textboxKeystoreCertC_TextChanged(object sender, EventArgs e)
        {
            UpdateAndValidateAllowSpaces(sender);
        }

        private void buttonGenerate_Click(object sender, EventArgs e)
        {
            string errors = String.Join("\n", AndroidUtilities.GetKeystoreErrors(_ks));

            if (!string.IsNullOrEmpty(errors))
            {
                string intro = "We found errors that have to be fixed before keystore geneation:";

                MessageBox.Show(intro + "\n\n" + errors);
                return;
            }

            if(AndroidUtilities.RunGenerateKeystore(_ks))
            {
                _lastGeneratedKS = _ks.Copy();
                buttonOK.Enabled = true;
            }
        }

        private void buttonOK_Click(object sender, EventArgs e)
        {
            if (_lastGeneratedKS == null) return;

            Response = new GenerateAndroidKeystoreResponse()
            {
                Keystore = _lastGeneratedKS.KeystorePath,
                Password = _lastGeneratedKS.Password,
                KeyAlias = _lastGeneratedKS.KeyAlias,
                KeyPassword = _lastGeneratedKS.KeyPassword,
            };

            DialogResult = DialogResult.OK;
            Close();
        }

        private void buttonBrowse_Click(object sender, EventArgs e)
        {
            using (OpenFileDialog openFileDialog = new OpenFileDialog())
            {
                openFileDialog.Title = "Set keystore filename for generation";
                openFileDialog.InitialDirectory = Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments);
                openFileDialog.Filter = "key store files (*.jks)|*.jks|All files (*.*)|*.*";
                openFileDialog.RestoreDirectory = true;
                openFileDialog.CheckFileExists = false;

                if (openFileDialog.ShowDialog() == DialogResult.OK)
                {
                    textboxKeystorePath.Text = openFileDialog.FileName;
                }
            }
        }
    }

    public class GenerateAndroidKeystoreResponse
    {
        public string Keystore;
        public string Password;
        public string KeyAlias;
        public string KeyPassword;
    }
}
