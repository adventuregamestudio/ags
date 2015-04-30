using AGS.Types;
using Scintilla.Enums;
using Scintilla.Lexers;
using System;
using System.Drawing;
using System.Windows.Forms;
using WeifenLuo.WinFormsUI.Docking;

namespace AGS.Editor
{
    public class DraconianTheme : ColorTheme
    {
        public Color BorderColor
        {
            get;
            private set;
        }

        public Color Blue
        {
            get;
            private set;
        }

        public Color ControlColor
        {
            get;
            private set;
        }

        public Color EditorColor
        {
            get;
            private set;
        }

        public Color PanelColor
        {
            get;
            private set;
        }

        public Color White
        {
            get;
            private set;
        }

        public DraconianTheme()
        {
            this.BorderColor = Color.FromArgb(63, 63, 70);
            this.Blue = Color.FromArgb(0, 122, 204);
            this.ControlColor = Color.FromArgb(51, 51, 55);
            this.EditorColor = Color.FromArgb(45, 45, 48);
            this.PanelColor = Color.FromArgb(37, 37, 38);
            this.White = Color.FromArgb(241, 241, 241);
        }

        public override void Color_Button(Button button)
        {
            button.FlatStyle = FlatStyle.Flat;
            button.FlatAppearance.BorderSize = 1;
            button.FlatAppearance.BorderColor = this.BorderColor;
            button.BackColor = this.ControlColor;
            button.ForeColor = this.White;
        }

        public override void Color_ComboBox(ComboBox comboBox, Control panel, EventHandler selectedIndexChanged, EventHandler mouseEnter, EventHandler mouseLeave)
        {
            panel.Controls.Remove(comboBox);
            comboBox = new DraconianComboBox(comboBox);
            panel.Controls.Add(comboBox);
            if (selectedIndexChanged != null)
                comboBox.SelectedIndexChanged += new EventHandler(selectedIndexChanged);
            if (mouseEnter != null)
                comboBox.MouseEnter += new EventHandler(mouseEnter);
            if (mouseLeave != null)
                comboBox.MouseLeave += new EventHandler(mouseLeave);
        }

        public override void Color_ComboBox(ComboBox comboBox, PropertiesPanel panel)
        {
            panel.Controls.Remove(comboBox);
            comboBox = new DraconianComboBox(comboBox);
            panel.Controls.Add(comboBox);
        }

        public override void Color_DockContent(DockContent dockContent)
        {
            dockContent.BackColor = this.BorderColor;
            dockContent.ForeColor = this.White;
        }

        public override void Color_DockPanel(DockPanel dockPanel)
        {
            WeifenLuo.WinFormsUI.Docking.DockPanelSkin dockPanelSkin1 = new WeifenLuo.WinFormsUI.Docking.DockPanelSkin();
            WeifenLuo.WinFormsUI.Docking.AutoHideStripSkin autoHideStripSkin1 = new WeifenLuo.WinFormsUI.Docking.AutoHideStripSkin();
            WeifenLuo.WinFormsUI.Docking.DockPanelGradient dockPanelGradient1 = new WeifenLuo.WinFormsUI.Docking.DockPanelGradient();
            WeifenLuo.WinFormsUI.Docking.TabGradient tabGradient1 = new WeifenLuo.WinFormsUI.Docking.TabGradient();
            WeifenLuo.WinFormsUI.Docking.DockPaneStripSkin dockPaneStripSkin1 = new WeifenLuo.WinFormsUI.Docking.DockPaneStripSkin();
            WeifenLuo.WinFormsUI.Docking.DockPaneStripGradient dockPaneStripGradient1 = new WeifenLuo.WinFormsUI.Docking.DockPaneStripGradient();
            WeifenLuo.WinFormsUI.Docking.TabGradient tabGradient2 = new WeifenLuo.WinFormsUI.Docking.TabGradient();
            WeifenLuo.WinFormsUI.Docking.DockPanelGradient dockPanelGradient2 = new WeifenLuo.WinFormsUI.Docking.DockPanelGradient();
            WeifenLuo.WinFormsUI.Docking.TabGradient tabGradient3 = new WeifenLuo.WinFormsUI.Docking.TabGradient();
            WeifenLuo.WinFormsUI.Docking.DockPaneStripToolWindowGradient dockPaneStripToolWindowGradient1 = new WeifenLuo.WinFormsUI.Docking.DockPaneStripToolWindowGradient();
            WeifenLuo.WinFormsUI.Docking.TabGradient tabGradient4 = new WeifenLuo.WinFormsUI.Docking.TabGradient();
            WeifenLuo.WinFormsUI.Docking.TabGradient tabGradient5 = new WeifenLuo.WinFormsUI.Docking.TabGradient();
            WeifenLuo.WinFormsUI.Docking.DockPanelGradient dockPanelGradient3 = new WeifenLuo.WinFormsUI.Docking.DockPanelGradient();
            WeifenLuo.WinFormsUI.Docking.TabGradient tabGradient6 = new WeifenLuo.WinFormsUI.Docking.TabGradient();
            WeifenLuo.WinFormsUI.Docking.TabGradient tabGradient7 = new WeifenLuo.WinFormsUI.Docking.TabGradient();

            dockPanelGradient1.EndColor = this.EditorColor;
            dockPanelGradient1.StartColor = this.EditorColor;
            autoHideStripSkin1.DockStripGradient = dockPanelGradient1;
            tabGradient1.EndColor = this.EditorColor;
            tabGradient1.StartColor = this.EditorColor;
            tabGradient1.TextColor = this.White;
            autoHideStripSkin1.TabGradient = tabGradient1;
            autoHideStripSkin1.TextFont = new System.Drawing.Font("Tahoma", 8.400001F);
            dockPanelSkin1.AutoHideStripSkin = autoHideStripSkin1;
            tabGradient2.EndColor = this.Blue;
            tabGradient2.StartColor = this.Blue;
            tabGradient2.TextColor = this.White;
            dockPaneStripGradient1.ActiveTabGradient = tabGradient2;
            dockPanelGradient2.EndColor = this.EditorColor;
            dockPanelGradient2.StartColor = this.EditorColor;
            dockPaneStripGradient1.DockStripGradient = dockPanelGradient2;
            tabGradient3.EndColor = this.EditorColor;
            tabGradient3.StartColor = this.EditorColor;
            tabGradient3.TextColor = this.White;
            dockPaneStripGradient1.InactiveTabGradient = tabGradient3;
            dockPaneStripSkin1.DocumentGradient = dockPaneStripGradient1;
            dockPaneStripSkin1.TextFont = new System.Drawing.Font("Tahoma", 8.400001F);
            tabGradient4.LinearGradientMode = System.Drawing.Drawing2D.LinearGradientMode.Vertical;
            tabGradient4.EndColor = this.Blue;
            tabGradient4.StartColor = this.Blue;
            tabGradient4.TextColor = this.White;
            dockPaneStripToolWindowGradient1.ActiveCaptionGradient = tabGradient4;
            tabGradient5.EndColor = this.PanelColor;
            tabGradient5.StartColor = this.PanelColor;
            tabGradient5.TextColor = this.Blue;
            dockPaneStripToolWindowGradient1.ActiveTabGradient = tabGradient5;
            dockPanelGradient3.EndColor = this.EditorColor;
            dockPanelGradient3.StartColor = this.EditorColor;
            dockPaneStripToolWindowGradient1.DockStripGradient = dockPanelGradient3;
            tabGradient6.LinearGradientMode = System.Drawing.Drawing2D.LinearGradientMode.Vertical;
            tabGradient6.EndColor = this.EditorColor;
            tabGradient6.StartColor = this.EditorColor;
            tabGradient6.TextColor = this.White;
            dockPaneStripToolWindowGradient1.InactiveCaptionGradient = tabGradient6;
            tabGradient7.EndColor = this.EditorColor;
            tabGradient7.StartColor = this.EditorColor;
            tabGradient7.TextColor = this.White;
            dockPaneStripToolWindowGradient1.InactiveTabGradient = tabGradient7;
            dockPaneStripSkin1.ToolWindowGradient = dockPaneStripToolWindowGradient1;
            dockPanelSkin1.DockPaneStripSkin = dockPaneStripSkin1;

            dockPanel.Skin = dockPanelSkin1;
            dockPanel.BackColor = this.EditorColor;
            dockPanel.DockBackColor = this.EditorColor;
            dockPanel.ForeColor = this.White;
        }

        public override void Color_EditorContentPanel(EditorContentPanel editorContentPanel)
        {
            editorContentPanel.BackColor = this.PanelColor;
            editorContentPanel.ForeColor = this.White;
        }

        public override void Color_GroupBox(GroupBox groupBox)
        {
            groupBox.BackColor = this.PanelColor;
            groupBox.ForeColor = this.White;
        }

        public override void Color_ListView(ListView listView)
        {
            listView.BackColor = this.PanelColor;
            listView.ForeColor = this.White;
            listView.OwnerDraw = true;
            listView.GridLines = false;
            listView.Layout += delegate
            {
                listView.Columns[listView.Columns.Count - 1].Width = -2;
            };
            listView.DrawColumnHeader += new DrawListViewColumnHeaderEventHandler(this.ListView_Draconian_DrawColumnHeader);
            listView.DrawItem += new DrawListViewItemEventHandler(this.ListView_Draconian_DrawItem);
            listView.DrawSubItem += new DrawListViewSubItemEventHandler(this.ListView_Draconian_DrawSubItem);
        }

        public override void Color_MenuStripExtended(MenuStripExtended menuStripExtended)
        {
            menuStripExtended.Renderer = new DraconianMenuRenderer(new DraconianMenuColors());
            menuStripExtended.ForeColor = this.White;
        }

        public override void Color_NumericUpDown(NumericUpDown numericUpDown)
        {
            numericUpDown.BackColor = this.ControlColor;
            numericUpDown.ForeColor = this.White;
        }

        public override void Color_Panel(Panel panel)
        {
            panel.BackColor = this.EditorColor;
            panel.ForeColor = this.White;
        }

        public override void Color_PropertyGrid(PropertyGrid propertyGrid)
        {
            propertyGrid.BackColor = this.PanelColor;
            propertyGrid.ViewBackColor = this.PanelColor;
            propertyGrid.ViewForeColor = this.White;
            propertyGrid.LineColor = this.EditorColor;
            propertyGrid.CategoryForeColor = this.White;
            //propertyGrid.CommandsBackColor = Color.Pink;
            //propertyGrid.CommandsLinkColor = Color.Blue;
            //propertyGrid.CommandsActiveLinkColor = Color.Purple;
            //propertyGrid.CommandsDisabledLinkColor = Color.Gray;
            //propertyGrid.CommandsForeColor = Color.Green;
            propertyGrid.HelpBackColor = this.PanelColor;
            propertyGrid.HelpForeColor = this.White;
        }

        public override void Color_ScintillaControl(Scintilla.ScintillaControl scintillaControl, string default_font, int default_font_size, string user_font, int user_font_size, int breakpoint1, int breakpoint2, int statement1, int statement2)
        {
            Color DefaultBackColor = Color.FromArgb(22, 30, 32);
            scintillaControl.SetStyle(Cpp.GlobalDefault, Color.FromArgb(63, 78, 73), DefaultBackColor, false, false, default_font, default_font_size);
            scintillaControl.StyleSetFore(Cpp.Default, Color.FromArgb(200, 200, 200));
            scintillaControl.StyleSetBack(Cpp.Default, DefaultBackColor);

            scintillaControl.StyleSetFont((int)Cpp.BraceBad, default_font);
            scintillaControl.StyleSetFontSize((int)Cpp.BraceBad, default_font_size);
            scintillaControl.StyleSetBack(Cpp.BraceBad, Color.FromArgb(255, 0, 0));

            scintillaControl.StyleSetFont((int)Cpp.BraceLight, default_font);
            scintillaControl.StyleSetFontSize((int)Cpp.BraceLight, default_font_size);
            scintillaControl.StyleSetBold(Cpp.BraceLight, true);
            scintillaControl.StyleSetFore(Cpp.BraceLight, Color.FromArgb(255, 255, 255));
            scintillaControl.StyleSetBack(Cpp.BraceLight, Color.FromArgb(80, 80, 80));

            scintillaControl.StyleSetFore(Cpp.Word, Color.FromArgb(147, 199, 99));   // keyword
            scintillaControl.StyleSetBack(Cpp.Word, DefaultBackColor);
            scintillaControl.StyleSetFore(Cpp.Word2, Color.FromArgb(103, 140, 177));
            scintillaControl.StyleSetBack(Cpp.Word2, DefaultBackColor);
            scintillaControl.StyleSetFore(Cpp.Identifier, Color.FromArgb(241, 242, 243));
            scintillaControl.StyleSetBack(Cpp.Identifier, DefaultBackColor);
            scintillaControl.StyleSetFore(Cpp.Comment, Color.FromArgb(113, 126, 117));
            scintillaControl.StyleSetBack(Cpp.Comment, DefaultBackColor);
            scintillaControl.StyleSetFore(Cpp.CommentLine, Color.FromArgb(113, 126, 117));
            scintillaControl.StyleSetBack(Cpp.CommentLine, DefaultBackColor);
            scintillaControl.StyleSetFore(Cpp.CommentDoc, Color.FromArgb(128, 128, 128));
            scintillaControl.StyleSetBack(Cpp.CommentDoc, DefaultBackColor);
            scintillaControl.StyleSetFore(Cpp.CommentLineDoc, Color.FromArgb(128, 128, 128));
            scintillaControl.StyleSetBack(Cpp.CommentLineDoc, DefaultBackColor);
            scintillaControl.StyleSetFore(Cpp.CommentDocKeyword, Color.FromArgb(153, 163, 138));
            scintillaControl.StyleSetBack(Cpp.CommentDocKeyword, DefaultBackColor);
            scintillaControl.StyleSetFore(Cpp.CommentDocKeywordError, Color.FromArgb(153, 163, 138));
            scintillaControl.StyleSetBack(Cpp.CommentDocKeywordError, DefaultBackColor);
            scintillaControl.StyleSetFore(Cpp.Number, Color.FromArgb(255, 205, 34));
            scintillaControl.StyleSetBack(Cpp.Number, DefaultBackColor);
            scintillaControl.StyleSetFore(Cpp.Regex, Color.FromArgb(255, 205, 34));
            scintillaControl.StyleSetBack(Cpp.Regex, DefaultBackColor);
            scintillaControl.StyleSetFore(Cpp.String, Color.FromArgb(236, 118, 0));
            scintillaControl.StyleSetBack(Cpp.String, DefaultBackColor);
            scintillaControl.StyleSetFore(Cpp.Operator, Color.FromArgb(104, 170, 161));
            scintillaControl.StyleSetBack(Cpp.Operator, DefaultBackColor);
            scintillaControl.StyleSetFore(Cpp.Verbatim, Color.FromArgb(236, 118, 0));
            scintillaControl.StyleSetBack(Cpp.Verbatim, DefaultBackColor);

            scintillaControl.StyleSetBack(Cpp.StringEol, Color.FromArgb(106, 56, 56));

            scintillaControl.StyleSetFore(Cpp.Preprocessor, Color.FromArgb(160, 130, 189));
            scintillaControl.StyleSetBack(Cpp.Preprocessor, DefaultBackColor);

            scintillaControl.StyleSetFore(Cpp.LineNumber, Color.FromArgb(63, 78, 73));
            scintillaControl.StyleSetBack(Cpp.LineNumber, Color.FromArgb(41, 49, 52));

            scintillaControl.StyleSetFore(Cpp.IndentGuide, Color.FromArgb(41, 49, 52));
            scintillaControl.StyleSetBack(Cpp.IndentGuide, DefaultBackColor);

            scintillaControl.StyleSetFont((int)Cpp.CallTip, user_font);
            scintillaControl.StyleSetFontSize((int)Cpp.CallTip, user_font_size);
            scintillaControl.StyleSetFore(Cpp.CallTip, Color.Black);
            scintillaControl.StyleSetBack(Cpp.CallTip, Color.LightGoldenrodYellow);

            scintillaControl.SetFoldMarginColor(true, 0x2c271f);
            scintillaControl.SetFoldMarginHiColor(true, 0x2c271f);

            scintillaControl.MarkerSetFore((int)Scintilla.Constants.SC_MARKNUM_FOLDER, Color.FromArgb(123, 147, 151));
            scintillaControl.MarkerSetFore((int)Scintilla.Constants.SC_MARKNUM_FOLDEREND, Color.FromArgb(123, 147, 151));
            scintillaControl.MarkerSetFore((int)Scintilla.Constants.SC_MARKNUM_FOLDEROPEN, DefaultBackColor);
            scintillaControl.MarkerSetFore((int)Scintilla.Constants.SC_MARKNUM_FOLDEROPENMID, DefaultBackColor);
            scintillaControl.MarkerSetFore((int)Scintilla.Constants.SC_MARKNUM_FOLDERMIDTAIL, DefaultBackColor);

            scintillaControl.MarkerSetBack((int)Scintilla.Constants.SC_MARKNUM_FOLDER, Color.FromArgb(63, 78, 73));
            scintillaControl.MarkerSetBack((int)Scintilla.Constants.SC_MARKNUM_FOLDEROPEN, Color.FromArgb(63, 78, 73));
            scintillaControl.MarkerSetBack((int)Scintilla.Constants.SC_MARKNUM_FOLDEROPENMID, Color.FromArgb(63, 78, 73));
            scintillaControl.MarkerSetBack((int)Scintilla.Constants.SC_MARKNUM_FOLDERMIDTAIL, Color.FromArgb(63, 78, 73));
            scintillaControl.MarkerSetBack((int)Scintilla.Constants.SC_MARKNUM_FOLDEREND, Color.FromArgb(63, 78, 73));
            scintillaControl.MarkerSetBack((int)Scintilla.Constants.SC_MARKNUM_FOLDERSUB, Color.FromArgb(63, 78, 73));
            scintillaControl.MarkerSetBack((int)Scintilla.Constants.SC_MARKNUM_FOLDERTAIL, Color.FromArgb(63, 78, 73));
            scintillaControl.MarkerSetBack((int)Scintilla.Constants.SC_MARKNUM_FOLDEREND, Color.FromArgb(63, 78, 73));

            // override the selected text color
            scintillaControl.SetSelBack(true, Color.FromArgb(79, 97, 100));

            scintillaControl.MarkerDefine(breakpoint1, (int)MarkerSymbol.Background);
            scintillaControl.MarkerSetBack(breakpoint1, Color.FromArgb(150, 58, 70));
            scintillaControl.MarkerSetFore(breakpoint1, Color.White);

            scintillaControl.MarkerDefine(breakpoint2, (int)MarkerSymbol.Circle);
            scintillaControl.MarkerSetBack(breakpoint2, Color.FromArgb(212, 43, 44));
            scintillaControl.MarkerSetFore(breakpoint2, Color.FromArgb(106, 56, 56));

            scintillaControl.MarkerDefine(statement1, (int)MarkerSymbol.Arrow);
            scintillaControl.MarkerSetBack(statement1, Color.FromArgb(255, 238, 98));
            scintillaControl.MarkerSetFore(statement1, Color.FromArgb(63, 78, 73));

            scintillaControl.MarkerDefine(statement2, (int)MarkerSymbol.Background);
            scintillaControl.MarkerSetFore(statement2, Color.Black); // Doesn't seem to be working :\
            scintillaControl.MarkerSetBack(statement2, Color.FromArgb(80, 71, 38));

            scintillaControl.CaretFore = 0xFFFFFF;
        }

        public override void Color_StatusStrip(StatusStrip statusStrip)
        {
            statusStrip.BackColor = this.Blue;
            statusStrip.ForeColor = this.White;
        }

        public override void Color_TabControl(TabControl tabControl)
        {
            tabControl.DrawMode = TabDrawMode.OwnerDrawFixed;
            tabControl.DrawItem += new System.Windows.Forms.DrawItemEventHandler(TabControl_Draconian_DrawItem);
        }

        public override void Color_TabPage(TabPage tabPage)
        {
            tabPage.BackColor = this.PanelColor;
            tabPage.ForeColor = this.White;
        }

        public override void Color_TextBox(TextBox textBox)
        {
            textBox.BorderStyle = BorderStyle.FixedSingle;
            textBox.BackColor = this.ControlColor;
            textBox.ForeColor = this.White;
        }

        public override void Color_ToolStripExtended(ToolStripExtended toolStripExtended)
        {
            toolStripExtended.Renderer = new DraconianToolStripRenderer();
            toolStripExtended.BackColor = this.EditorColor;
            toolStripExtended.ForeColor = this.White;
        }

        public override void Color_TreeView(TreeView treeView)
        {
            treeView.BackColor = this.PanelColor;
            treeView.ForeColor = this.White;
            treeView.LineColor = this.PanelColor;
        }

        public override void Color_UserControl(UserControl userControl)
        {
            userControl.BackColor = this.EditorColor;
            userControl.ForeColor = this.White;
        }

        private void ComboBox_Draconian_DrawItem(object sender, DrawItemEventArgs e)
        {
            if (e.Index >= 0)
            {
                ComboBox comboBox = sender as ComboBox;
                e.DrawBackground();
                if ((e.State & DrawItemState.Selected) == DrawItemState.Selected)
                    e.Graphics.FillRectangle(new SolidBrush(this.BorderColor), e.Bounds);
                else
                    e.Graphics.FillRectangle(new SolidBrush(comboBox.BackColor), e.Bounds);
                e.Graphics.DrawString(comboBox.Items[e.Index].ToString(), comboBox.Font, new SolidBrush(comboBox.ForeColor), e.Bounds);
                e.DrawFocusRectangle();
            }
        }

        private void ComboBox_Draconian_DropDown(object sender, EventArgs e)
        {
            ComboBox comboBox = sender as ComboBox;
            comboBox.BackColor = this.ControlColor;
            comboBox.ForeColor = this.White;
        }

        private void ComboBox_Draconian_DropDownClosed(object sender, EventArgs e)
        {
            ComboBox comboBox = sender as ComboBox;
            comboBox.BackColor = this.ControlColor;
            comboBox.ForeColor = this.White;
        }

        private void ListView_Draconian_DrawColumnHeader(object sender, DrawListViewColumnHeaderEventArgs e)
        {
            ListView listView = sender as ListView;
            e.Graphics.FillRectangle(new SolidBrush(listView.BackColor), e.Bounds);
            e.Graphics.DrawRectangle(new Pen(new SolidBrush(this.BorderColor)), e.Bounds.X - 1, e.Bounds.Y - 1, e.Bounds.Size.Width, e.Bounds.Size.Height);
            e.Graphics.DrawString(e.Header.Text, listView.Font, new SolidBrush(listView.ForeColor), e.Bounds.X + 5, e.Bounds.Y + (e.Bounds.Size.Height / 5));
        }

        private void ListView_Draconian_DrawItem(object sender, DrawListViewItemEventArgs e)
        {
            e.DrawDefault = true;
        }

        private void ListView_Draconian_DrawSubItem(object sender, DrawListViewSubItemEventArgs e)
        {
            e.DrawDefault = true;
        }

        private void TabControl_Draconian_DrawItem(object sender, DrawItemEventArgs e)
        {
            TabControl tabControl = sender as TabControl;
            if (e.Index == tabControl.SelectedIndex)
            {
                e.Graphics.FillRectangle(new SolidBrush(this.Blue), e.Bounds);
            }
            else
            {
                e.Graphics.Clear(this.PanelColor);
            }
            e.Graphics.DrawString(tabControl.TabPages[e.Index].Text, tabControl.TabPages[e.Index].Font, new SolidBrush(this.White), e.Bounds.X, e.Bounds.Y + 5);
        }
    }
}