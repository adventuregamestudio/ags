using System;
using System.Collections.Generic;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Xml;
using AGS.Types;

namespace AGS.Editor.Components
{
    class GlobalVariablesComponent : BaseComponent, IGlobalVariablesController
    {
        private const string TOP_LEVEL_COMMAND_ID = "GlobalVariables";
        public const string GLOBAL_VARS_HEADER_FILE_NAME = "_GlobalVariables.ash";
        private const string GLOBAL_VARS_SCRIPT_FILE_NAME = "_GlobalVariables.asc";
        private const string ICON_KEY = "GlobalVarsIcon";

        private GlobalVariablesEditor _editor;
        private ContentDocument _document;
        private Script _scriptModule;
        private Script _scriptHeader;

        public GlobalVariablesComponent(GUIController guiController, AGSEditor agsEditor)
            : base(guiController, agsEditor)
        {
            RecreateDocument();
            _scriptHeader = new Script(GLOBAL_VARS_HEADER_FILE_NAME, string.Empty, true);
            _scriptModule = new Script(GLOBAL_VARS_SCRIPT_FILE_NAME, string.Empty, false);
            _guiController.RegisterIcon(ICON_KEY, Resources.ResourceManager.GetIcon("globalvars.ico"));
            _guiController.ProjectTree.AddTreeRoot(this, TOP_LEVEL_COMMAND_ID, "Global variables", ICON_KEY);
            _editor.GlobalVariableChanged += new GlobalVariablesEditor.GlobalVariableChangedHandler(_editor_GlobalVariableChanged);
            _agsEditor.GetScriptHeaderList += new GetScriptHeaderListHandler(_agsEditor_GetScriptHeaderList);
            _agsEditor.GetScriptModuleList += new GetScriptModuleListHandler(_agsEditor_GetScriptModuleList);
        }

        public void SelectGlobalVariable(string variableName)
        {
            ShowGlobalVariablesPane(TOP_LEVEL_COMMAND_ID);
            _editor.SelectGlobalVariable(variableName);
        }

        private void _editor_GlobalVariableChanged()
        {
            UpdateScriptHeader();
        }

        private void _agsEditor_GetScriptModuleList(GetScriptModuleListEventArgs evArgs)
        {
            IList<GlobalVariable> variables = _agsEditor.CurrentGame.GlobalVariables.ToList();
            if (variables.Count > 0)
            {
                _scriptModule.Text = GenerateGlobalVariablesScriptModule(variables);
                evArgs.Scripts.Add(_scriptModule);
            }
        }

        private string GenerateGlobalVariablesScriptModule(IList<GlobalVariable> variables)
        {
            StringBuilder sb = new StringBuilder();

            foreach (GlobalVariable variable in variables)
            {
                string declaration = variable.Type + " " + variable.Name;
                if (((variable.Type == "int") ||
                     (variable.Type == "bool") ||
                     (variable.Type == "float")) &&
                    (!string.IsNullOrEmpty(variable.DefaultValue)))
                {
                    declaration += " = " + variable.DefaultValue.ToLower();
                }
                sb.AppendLine(declaration + ";");
                sb.AppendLine("export " + variable.Name + ";");
            }

            sb.AppendLine("function game_start() {");
            foreach (GlobalVariable variable in variables)
            {
                if (variable.Type == "String")
                {
                    string varValue = variable.DefaultValue.Replace("\\", "\\\\").Replace("\"", "\\\"");
                    sb.AppendLine(variable.Name + " = \"" + varValue + "\";");
                }
            }
            sb.AppendLine("}");

            return sb.ToString();
        }

        private void UpdateScriptHeader()
        {
            StringBuilder sb = new StringBuilder();
            foreach (GlobalVariable variable in _agsEditor.CurrentGame.GlobalVariables.ToList())
            {
                sb.AppendLine("import " + variable.Type + " " + variable.Name + ";");
            }
            _scriptHeader.Text = sb.ToString();
            AutoComplete.ConstructCache(_scriptHeader);
        }

        private void _agsEditor_GetScriptHeaderList(GetScriptHeaderListEventArgs evArgs)
        {
            UpdateScriptHeader();

            if (_scriptHeader.Text.Length > 0)
            {
                evArgs.ScriptHeaders.Add(_scriptHeader);
            }
        }

        private void RecreateDocument()
        {
            if (_document != null)
            {
                _document.Dispose();
            }
            _editor = new GlobalVariablesEditor(_agsEditor.CurrentGame);
            _document = new ContentDocument(_editor, "Global Variables", this, ICON_KEY);
        }

        public override string ComponentID
        {
            get { return ComponentIDs.GlobalVariables; }
        }

        public override void CommandClick(string controlID)
        {
            ShowGlobalVariablesPane(controlID);
        }

        private void ShowGlobalVariablesPane(string controlID)
        {
            if (_document.Control.IsDisposed)
            {
                RecreateDocument();
            }
            _document.TreeNodeID = controlID;
            _guiController.AddOrShowPane(_document);
        }

        public override void RefreshDataFromGame()
        {
            _guiController.RemovePaneIfExists(_document);
            RecreateDocument();
        }

    }
}
