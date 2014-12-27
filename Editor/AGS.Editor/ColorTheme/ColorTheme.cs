using AGS.Types;
using System;
using System.Windows.Forms;
using WeifenLuo.WinFormsUI.Docking;

namespace AGS.Editor
{
    public abstract class ColorTheme
    {
        public abstract void Color_Button(Button button);
        public abstract void Color_ComboBox(ComboBox comboBox, Control panel, EventHandler selectedIndexChanged, EventHandler mouseEnter, EventHandler mouseLeave);
        public abstract void Color_ComboBox(ComboBox comboBox, PropertiesPanel panel);
        public abstract void Color_DockContent(DockContent dockContent);
        public abstract void Color_DockPanel(DockPanel dockPanel);
        public abstract void Color_EditorContentPanel(EditorContentPanel editorContentPanel);
        public abstract void Color_GroupBox(GroupBox groupBox);
        public abstract void Color_ListView(ListView listView);
        public abstract void Color_MenuStripExtended(MenuStripExtended menuStripExtended);
        public abstract void Color_NumericUpDown(NumericUpDown numericUpDown);
        public abstract void Color_Panel(Panel panel);
        public abstract void Color_PropertyGrid(PropertyGrid propertyGrid);
        public abstract void Color_ScintillaControl(Scintilla.ScintillaControl scintillaControl, string default_font, int default_font_size, string user_font, int user_font_size, int breakpoint1, int breakpoint2, int statement1, int statement2);
        public abstract void Color_StatusStrip(StatusStrip statusStrip);
        public abstract void Color_TabControl(TabControl tabControl);
        public abstract void Color_TabPage(TabPage tabPage);
        public abstract void Color_TextBox(TextBox textBox);
        public abstract void Color_ToolStripExtended(ToolStripExtended toolStripExtended);
        public abstract void Color_TreeView(TreeView treeView);
        public abstract void Color_UserControl(UserControl userControl);
    }
}