using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Text.RegularExpressions;
using System.Windows.Forms;
using AGS.Types;

namespace AGS.Editor
{
    public partial class GlobalVariableDialog : Form
    {
        private const string VARIABLE_NAME_REGEX = @"^[a-zA-Z_][a-zA-Z0-9_]*$";

        private GlobalVariable _variable;
        private Game _game;

        public GlobalVariableDialog(GlobalVariable variable, Game game)
        {
            InitializeComponent();
            _variable = variable;
            _game = game;
            PopulateTypeList();
            txtName.Text = variable.Name ?? string.Empty;
            txtDefaultValue.Text = variable.DefaultValue ?? string.Empty;

            foreach (GlobalVariableType varType in cmbType.Items)
            {
                if (varType.TypeName == variable.Type)
                {
                    cmbType.SelectedItem = varType;
                    break;
                }
            }

            Utilities.CheckLabelWidthsOnForm(this);
        }

        private void PopulateTypeList()
        {
            cmbType.Items.Clear();
            cmbType.Items.Add(new GlobalVariableType("int", @"^\-?[0-9]+$"));
            cmbType.Items.Add(new GlobalVariableType("String", @"^.*$"));
            cmbType.Items.Add(new GlobalVariableType("float", @"^\-?[0-9]+(\.[0-9]+)?$"));
            cmbType.Items.Add(new GlobalVariableType("bool", @"^true$|^false$"));
            cmbType.Items.Add(new GlobalVariableType("GUI*", null));
            cmbType.Items.Add(new GlobalVariableType("AudioChannel*", null));
            cmbType.Items.Add(new GlobalVariableType("Character*", null));
            cmbType.Items.Add(new GlobalVariableType("DynamicSprite*", null));
            cmbType.Items.Add(new GlobalVariableType("Overlay*", null));
            cmbType.Items.Add(new GlobalVariableType("ViewFrame*", null));
            cmbType.SelectedIndex = 0;
        }

        public static DialogResult Show(GlobalVariable variable, Game game)
        {
            GlobalVariableDialog dialog = new GlobalVariableDialog(variable, game);
            DialogResult result = dialog.ShowDialog();
            dialog.Dispose();
            return result;
        }

        private void btnOK_Click(object sender, EventArgs e)
        {
            if ((!string.IsNullOrEmpty(txtDefaultValue.Text)) &&
                (!((GlobalVariableType)cmbType.SelectedItem).IsDefaultValueValid(txtDefaultValue.Text)))
            {
                MessageBox.Show("The default value '" + txtDefaultValue.Text + "' is not valid for this type of variable. Please try again.", "Invalid default value", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }
            else if (txtName.Text == string.Empty)
            {
                MessageBox.Show("You must type in a name for the new variable.", "Invalid variable name", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }
            else if (!Regex.IsMatch(txtName.Text, VARIABLE_NAME_REGEX))
            {
                MessageBox.Show("The variable name '" + txtName.Text + "' is not a valid variable name. It can contain only letters, numbers and the underscore character; and cannot begin with a number.", "Invalid variable name", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }
            else if (_game.IsScriptNameAlreadyUsed(txtName.Text, _variable))
            {
                MessageBox.Show("The variable name '" + txtName.Text + "' is already being used by something else in the game. Please choose another name.", "Invalid variable name", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }
            else
            {
                _variable.Name = txtName.Text;
                _variable.DefaultValue = txtDefaultValue.Text;
                _variable.Type = cmbType.SelectedItem.ToString();
                this.DialogResult = DialogResult.OK;
                this.Close();
            }
        }

        private void cmbType_SelectedIndexChanged(object sender, EventArgs e)
        {
            GlobalVariableType chosenType = (GlobalVariableType)cmbType.SelectedItem;

            bool defaultValid = chosenType.CanHaveDefaultValue;
            txtDefaultValue.Enabled = defaultValid;
            if (!defaultValid)
            {
                txtDefaultValue.Text = string.Empty;
            }
        }

        private class GlobalVariableType
        {
            public string TypeName;
            private string _validationRegEx;

            public GlobalVariableType(string typeName, string validationRegEx)
            {
                this.TypeName = typeName;
                this._validationRegEx = validationRegEx;
            }

            public bool CanHaveDefaultValue
            {
                get { return (_validationRegEx != null); }
            }

            public bool IsDefaultValueValid(string defaultValue)
            {
                return Regex.IsMatch(defaultValue, _validationRegEx, RegexOptions.IgnoreCase | RegexOptions.CultureInvariant);
            }

            public override string ToString()
            {
                return TypeName;
            }
        }

    }
}