using AGS.Types;
using Scintilla.Enums;
using Scintilla.Lexers;
using System;
using System.Drawing;
using System.Windows.Forms;
using WeifenLuo.WinFormsUI.Docking;

namespace AGS.Editor
{
    public class VanillaTheme : ColorTheme
    {
        public override void Color_ScintillaControl(Scintilla.ScintillaControl scintillaControl, string default_font, int default_font_size, string user_font, int user_font_size, int breakpoint1, int breakpoint2, int statement1, int statement2)
        {
            scintillaControl.StyleSetFont((int)Cpp.BraceBad, default_font);
            scintillaControl.StyleSetFontSize((int)Cpp.BraceBad, default_font_size);
            scintillaControl.StyleSetBack(Cpp.BraceBad, Color.FromArgb(255, 0, 0));
            scintillaControl.StyleSetFont((int)Cpp.BraceLight, default_font);
            scintillaControl.StyleSetFontSize((int)Cpp.BraceLight, default_font_size);
            scintillaControl.StyleSetBold(Cpp.BraceLight, true);
            scintillaControl.StyleSetBack(Cpp.BraceLight, Color.FromArgb(210, 210, 0));

            scintillaControl.StyleSetFore(Cpp.Word, Color.FromArgb(0, 0, 244));
            scintillaControl.StyleSetFore(Cpp.Word2, Color.FromArgb(43, 145, 175));
            scintillaControl.StyleSetFore(Cpp.Comment, Color.FromArgb(27, 127, 27));
            scintillaControl.StyleSetFore(Cpp.CommentLine, Color.FromArgb(27, 127, 27));
            scintillaControl.StyleSetFore(Cpp.CommentDoc, Color.FromArgb(27, 127, 27));
            scintillaControl.StyleSetFore(Cpp.CommentLineDoc, Color.FromArgb(27, 127, 27));
            scintillaControl.StyleSetFore(Cpp.Number, Color.FromArgb(150, 27, 27));
            scintillaControl.StyleSetFore(Cpp.String, Color.FromArgb(70, 7, 7));
            scintillaControl.StyleSetFore(Cpp.Operator, Color.FromArgb(0, 70, 0));
            scintillaControl.StyleSetBack(Cpp.Preprocessor, Color.FromArgb(210, 210, 210));

            scintillaControl.StyleSetFont((int)Cpp.CallTip, user_font);
            scintillaControl.StyleSetFontSize((int)Cpp.CallTip, user_font_size);
            scintillaControl.StyleSetFore(Cpp.CallTip, Color.Black);
            scintillaControl.StyleSetBack(Cpp.CallTip, Color.LightGoldenrodYellow);

            // override the selected text colour
            scintillaControl.SetSelFore(true, Color.FromArgb(255, 255, 255));
            scintillaControl.SetSelBack(true, Color.FromArgb(0, 34, 130));

            scintillaControl.MarkerDefine(breakpoint1, (int)MarkerSymbol.Background);
            scintillaControl.MarkerSetBack(breakpoint1, Color.FromArgb(255, 100, 100));
            scintillaControl.MarkerSetFore(breakpoint1, Color.White);

            scintillaControl.MarkerDefine(breakpoint2, (int)MarkerSymbol.Circle);
            scintillaControl.MarkerSetBack(breakpoint2, Color.Red);
            scintillaControl.MarkerSetFore(breakpoint2, Color.Black);

            scintillaControl.MarkerDefine(statement1, (int)MarkerSymbol.Arrow);
            scintillaControl.MarkerSetBack(statement1, Color.Yellow);
            scintillaControl.MarkerSetFore(statement1, Color.White);

            scintillaControl.MarkerDefine(statement2, (int)MarkerSymbol.Background);
            scintillaControl.MarkerSetBack(statement2, Color.Yellow);
            scintillaControl.MarkerSetFore(statement2, Color.White);
        }

        /// <summary>
        /// Since the Vanilla theme has most of its coloring loaded by default we want the rest of the functions
        /// to do nothing. Therefore they are empty.
        /// </summary>
        #region Unused functions
        public override void Color_Button(Button button) { }
        public override void Color_ComboBox(ComboBox comboBox, Control panel, EventHandler selectedIndexChanged, EventHandler mouseEnter, EventHandler mouseLeave) { }
        public override void Color_ComboBox(ComboBox comboBox, PropertiesPanel panel) { }
        public override void Color_DockContent(DockContent dockContent) { }
        public override void Color_DockPanel(DockPanel dockPanel) { }
        public override void Color_EditorContentPanel(EditorContentPanel editorContentPanel) { }
        public override void Color_GroupBox(GroupBox groupBox) { }
        public override void Color_ListView(ListView listView) { }
        public override void Color_MenuStripExtended(MenuStripExtended menuStripExtended) { }
        public override void Color_NumericUpDown(NumericUpDown numericUpDown) { }
        public override void Color_Panel(Panel panel) { }
        public override void Color_PropertyGrid(PropertyGrid propertyGrid) { }
        public override void Color_StatusStrip(StatusStrip statusStrip) { }
        public override void Color_TabControl(TabControl tabControl) { }
        public override void Color_TabPage(TabPage tabPage) { }
        public override void Color_TextBox(TextBox textBox) { }
        public override void Color_ToolStripExtended(ToolStripExtended toolStripExtended) { }
        public override void Color_TreeView(TreeView treeView) { }
        public override void Color_UserControl(UserControl userControl) { }
        #endregion
    }
}