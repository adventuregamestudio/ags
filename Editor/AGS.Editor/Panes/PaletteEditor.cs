using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.IO;
using System.Text;
using System.Windows.Forms;
using AGS.Types;

namespace AGS.Editor
{
    public partial class PaletteEditor : EditorContentPanel
    {
        private const string PALETTE_FILE_FILTER = "All supported palettes (*.pal; *.bmp)|*.pal;*.bmp|256-color palette files (*.pal)|*.pal|Windows bitmap files (*.bmp)|*.bmp";

        private const string MENU_ITEM_EXPORT_TO_FILE = "ExportToFile";
        private const string MENU_ITEM_REPLACE_FROM_FILE = "ReplaceFromFile";
        private const string MENU_ITEM_IMPORT_FROM_FILE = "ImportFromFile";

        private bool _noUpdates = false;
        private List<int> _selectedIndexes = new List<int>();
        private TabPage _colourFinder;
        private Color _currentColor = Color.Black;

        public PaletteEditor()
        {
            InitializeComponent();
            _colourFinder = tabControl.TabPages[1];
            Factory.GUIController.OnPropertyObjectChanged += GUIController_OnPropertyObjectChanged;
            _selectedIndexes.Add(0);
            GameChanged();
            UpdateColor();
        }

        protected override void OnPropertyChanged(string propertyName, object oldValue)
        {
            palettePage.Invalidate(true);
        }

        protected override string OnGetHelpKeyword()
        {
            return "Colours Editor";
        }
        
        public void GameChanged()
        {
            if (Factory.AGSEditor.CurrentGame.Settings.ColorDepth == GameColorDepth.Palette)
            {
                lblPaletteIntro.Text = "Your game's palette is shown below.";
                tabControl.SelectedTab = tabControl.TabPages[0];
                if (tabControl.TabPages.Contains(_colourFinder))
                {
                    tabControl.TabPages.Remove(_colourFinder);
                }
            }
            else
            {
                lblPaletteIntro.Text = "This palette information will only be used for drawing any 8-bit graphics that you may have imported.";
                if (!tabControl.TabPages.Contains(_colourFinder))
                {
                    tabControl.TabPages.Add(_colourFinder);
                }
                tabControl.SelectedTab = tabControl.TabPages[1];
            }
            
            palettePanel.Invalidate();
        }

        public void OnShow()
        {
            UpdatePropertyGrid();
        }

        private void txtColourNumber_TextChanged(object sender, EventArgs e)
        {
            if (!_noUpdates)
            {
                _currentColor = AGS.Types.Utilities.ColorFromARGBHex(txtColourNumber.Text);
                UpdateColor(txtColourNumber);
            }
        }

        private void txtWebColor_TextChanged(object sender, EventArgs e)
        {
            if (!_noUpdates)
            {
                _currentColor = AGS.Types.Utilities.ColorFromHTMLHex(txtWebColor.Text);
                UpdateColor(txtWebColor);
            }
        }

        private void txtCommaSeparated_TextChanged(object sender, EventArgs e)
        {
            if (!_noUpdates)
            {
                _currentColor = AGS.Types.Utilities.ColorFromSeparatedRGBA(txtCommaSeparated.Text, ',');
                if (_currentColor.IsEmpty)
                    _currentColor = AGS.Types.Utilities.ColorFromSeparatedRGBA(txtCommaSeparated.Text, ';');
                UpdateColor(txtCommaSeparated);
            }
        }

        private void txtLegacyColourNumber_TextChanged(object sender, EventArgs e)
        {
            if (!_noUpdates)
            {
                int oldValue = 0;
                int.TryParse(txtLegacyColourNumber.Text, out oldValue);
                int newValue = ColorMapper.RemapFromLegacyColourNumber(oldValue,
                    Factory.AGSEditor.CurrentGame.Palette,
                    Factory.AGSEditor.CurrentGame.Settings.ColorDepth, false);
                _currentColor = Color.FromArgb(newValue);
                UpdateColor(txtLegacyColourNumber);
            }
        }

        private void UpdateColor(TextBox ignoreText = null)
        {
            _noUpdates = true;
            if (ignoreText != txtColourNumber)
            {
                txtColourNumber.Text = $"0x{AGS.Types.Utilities.ColorToARGBInt32(_currentColor).ToString("X8")}";
            }
            if (ignoreText != txtWebColor)
            {
                if (_currentColor.A == 255)
                {
                    txtWebColor.Text = AGS.Types.Utilities.ColorToRGBInt32(_currentColor).ToString("X6");
                }
                else
                {
                    txtWebColor.Text = AGS.Types.Utilities.ColorToRGBAInt32(_currentColor).ToString("X8");
                }
            }
            if (ignoreText != txtCommaSeparated)
            {
                if (_currentColor.A == 255)
                {
                    txtCommaSeparated.Text = $"{_currentColor.R}, {_currentColor.G}, {_currentColor.B}";
                }
                else
                {
                    txtCommaSeparated.Text = $"{_currentColor.R}, {_currentColor.G}, {_currentColor.B}, {_currentColor.A}";
                }
            }
            if (ignoreText != txtLegacyColourNumber)
            {
                txtLegacyColourNumber.Text =
                    ColorMapper.MapRgbColorToLegacyColourNumber(_currentColor,
                    Factory.AGSEditor.CurrentGame.Palette,
                    Factory.AGSEditor.CurrentGame.Settings.ColorDepth).ToString();
            }

            blockOfColour.Invalidate();
            _noUpdates = false;
        }

        private void blockOfColour_Paint(object sender, PaintEventArgs e)
        {
            AGS.Types.Utilities.PaintColorBlock(_currentColor, e.Graphics, e.ClipRectangle);
        }

        private void palettePanel_Paint(object sender, PaintEventArgs e)
        {
            Game game = Factory.AGSEditor.CurrentGame;
            System.Drawing.Font boldFont = new System.Drawing.Font(this.Font, FontStyle.Bold);

            for (int i = 0; i < game.Palette.Length; i++)
            {
                int x = (i % 16) * 20 + 30;
                int y = (i / 16) * 20;
                if (i % 16 == 0)
                {
                    int textXpos = 25 - (int)e.Graphics.MeasureString(i.ToString(), this.Font).Width;
                    e.Graphics.DrawString(i.ToString(), this.Font, Brushes.Black, textXpos, y + 3);
                }
                if (game.Palette[i].ColourType == PaletteColourType.Background)
                {
                    e.Graphics.FillRectangle(Brushes.Black, x, y, 20, 20);
                    e.Graphics.DrawString("X", boldFont, Brushes.White, x + 5, y + 3);
                }
                else
                {
                    Color thisColor = game.Palette[i].Colour;
                    e.Graphics.FillRectangle(new SolidBrush(thisColor), x, y, 20, 20);
                }
                if (_selectedIndexes.Contains(i))
                {
                    e.Graphics.DrawRectangle(Pens.White, x, y, 19, 19);
                    e.Graphics.DrawRectangle(Pens.DarkBlue, x + 1, y + 1, 17, 17);
                }
            }
        }

        private void ToggleColourSelected(int selectedIndex)
        {
            if (_selectedIndexes.Contains(selectedIndex))
            {
                _selectedIndexes.Remove(selectedIndex);
            }
            else
            {
                _selectedIndexes.Add(selectedIndex);
            }
        }

        private void AddRangeToSelection(int selectedIndex)
        {
            if (_selectedIndexes.Count > 0)
            {
                int start = selectedIndex;
                int stop = _selectedIndexes[_selectedIndexes.Count - 1];
                if (start > stop)
                {
                    int temp = start;
                    start = stop;
                    stop = temp;
                }
                for (int i = start; i <= stop; i++)
                {
                    if (!_selectedIndexes.Contains(i))
                    {
                        _selectedIndexes.Add(i);
                    }
                }
            }
            else
            {
                _selectedIndexes.Add(selectedIndex);
            }
        }

        private void palettePanel_MouseDown(object sender, MouseEventArgs e)
        {
            if ((e.X > 30) && (e.X < 30 + 16 * 20) && (e.Y > 0) && (e.Y < 16 * 20))
            {
                int selectedIndex = (e.X - 30) / 20 + (e.Y / 20) * 16;

                if (e.Button == MouseButtons.Right)
                {
                    ShowContextMenu(selectedIndex, e.Location);
                }
                else if (Utilities.IsControlPressed())
                {
                    ToggleColourSelected(selectedIndex);
                }
                else if (Utilities.IsShiftPressed())
                {
                    AddRangeToSelection(selectedIndex);
                }
                else
                {
                    _selectedIndexes.Clear();
                    _selectedIndexes.Add(selectedIndex);
                }

                UpdatePropertyGrid();
                palettePanel.Invalidate();
            }
        }

        private void ContextMenuEventHandler(object sender, EventArgs e)
        {
            ToolStripMenuItem item = (ToolStripMenuItem)sender;
            if (item.Name == MENU_ITEM_EXPORT_TO_FILE)
            {
                string fileName = Factory.GUIController.ShowSaveFileDialog("Export palette...", PALETTE_FILE_FILTER);
                if (fileName != null)
                {
                    try
                    {
                        ImportExport.ExportPaletteToFile(fileName);
                        Factory.GUIController.ShowMessage("Palette exported successfully.", MessageBoxIcon.Information);
                    }
                    catch (Exception ex)
                    {
                        Factory.GUIController.ShowMessage("There was an error exporting the palette. The error message was '" + ex.Message + "'. Please retry.", MessageBoxIcon.Warning);
                    }
                }
            }
            else if (item.Name == MENU_ITEM_REPLACE_FROM_FILE)
            {
                if (Factory.GUIController.ShowQuestion("This will replace your entire palette with one from the file you select. Are you sure you want to do this?") == DialogResult.Yes)
                {
                    PaletteEntry[] newPalette = ImportPalette();
                    if (newPalette != null)
                    {
                        for (int i = 0; i < newPalette.Length; i++)
                        {
                            newPalette[i].ColourType = Factory.AGSEditor.CurrentGame.Palette[i].ColourType;
                            Factory.AGSEditor.CurrentGame.Palette[i] = newPalette[i];
                        }
                        palettePanel.Invalidate();
                        Factory.NativeProxy.PaletteColoursChanged(Factory.AGSEditor.CurrentGame);
                    }
                }
            }
            else if (item.Name == MENU_ITEM_IMPORT_FROM_FILE)
            {
                PaletteEntry[] newPalette = ImportPalette();
                if (newPalette != null)
                {
                    for (int i = 0; i < _selectedIndexes.Count; i++)
                    {
                        int palIndex = _selectedIndexes[i];
                        newPalette[palIndex].ColourType = Factory.AGSEditor.CurrentGame.Palette[palIndex].ColourType;
                        Factory.AGSEditor.CurrentGame.Palette[palIndex] = newPalette[palIndex];
                    }
                    palettePanel.Invalidate();
                }
            }
        }

        private PaletteEntry[] ImportPalette()
        {
            PaletteEntry[] newPalette = null;
            string fileName = Factory.GUIController.ShowOpenFileDialog("Import palette...", PALETTE_FILE_FILTER);
            if (fileName != null)
            {
                try
                {
                    newPalette = ImportExport.ImportPaletteFromFile(fileName);
                    Factory.GUIController.ShowMessage("Palette imported successfully.", MessageBoxIcon.Information);
                }
                catch (Exception ex)
                {
                    Factory.GUIController.ShowMessage("There was an error importing the palette. The error message was '" + ex.Message + "'. Please retry.", MessageBoxIcon.Warning);
                }
            }
            return newPalette;
        }

        private void ShowContextMenu(int atIndex, Point menuPosition)
        {
            EventHandler onClick = new EventHandler(ContextMenuEventHandler);
            ContextMenuStrip menu = new ContextMenuStrip();
            menu.Items.Add(new ToolStripMenuItem("Export to file...", null, onClick, MENU_ITEM_EXPORT_TO_FILE));
            menu.Items.Add(new ToolStripMenuItem("Replace palette from file...", null, onClick, MENU_ITEM_REPLACE_FROM_FILE));

            if (_selectedIndexes.Contains(atIndex))
            {
                menu.Items.Add(new ToolStripMenuItem("Replace selected slots from file...", null, onClick, MENU_ITEM_IMPORT_FROM_FILE));
            }

            menu.Show(palettePanel, menuPosition);
        }

        private void UpdatePropertyGrid()
        {
            if (_selectedIndexes.Count < 1)
            {
                Factory.GUIController.SetPropertyGridObject(null);
            }
            else if (_selectedIndexes.Count == 1)
            {
                Factory.GUIController.SetPropertyGridObject(Factory.AGSEditor.CurrentGame.Palette[_selectedIndexes[0]]);
            }
            else
            {
                object[] palEntryList = new object[_selectedIndexes.Count];
                for (int i = 0; i < _selectedIndexes.Count; i++)
                {
                    palEntryList[i] = Factory.AGSEditor.CurrentGame.Palette[_selectedIndexes[i]];
                }
                Factory.GUIController.SetPropertyGridObjects(palEntryList);
            }
        }

        private void GUIController_OnPropertyObjectChanged(object newPropertyObject)
        {
            if (GUIController.Instance.ActivePane != ContentDocument)
                return; // not this pane

            if (newPropertyObject is PaletteEntry)
            {
                _selectedIndexes.Clear();
                _selectedIndexes.Add(((PaletteEntry)newPropertyObject).Index);
                palettePanel.Invalidate();
            }
        }

        protected override void OnDispose()
        {
            Factory.GUIController.OnPropertyObjectChanged -= GUIController_OnPropertyObjectChanged;
        }

        private void btnColorDialog_Click(object sender, EventArgs e)
        {
            ColorDialog dialog = new ColorDialog();
            dialog.Color = _currentColor;
            dialog.AnyColor = true;
            dialog.FullOpen = true;
            if (dialog.ShowDialog() == DialogResult.OK)
            {
                _currentColor = dialog.Color;
                UpdateColor();
            }
            dialog.Dispose();
        }

        private void LoadColorTheme(ColorTheme t)
        {
            t.ControlHelper(this, "palette");
            t.ControlHelper(colourFinderPage, "palette/color-finder");
            t.ControlHelper(palettePage, "palette/palette-page");
            t.GroupBoxHelper(groupBox1, "palette/color-finder");
            t.TextBoxHelper(txtColourNumber, "palette/color-finder/color-number-box");
            t.ButtonHelper(btnColorDialog, "palette/color-finder/btn-color-dialog");
            t.GroupBoxHelper(groupBox2, "palette/palette-page");

            if (t.Has("palette/draw-mode"))
                tabControl.DrawMode = (TabDrawMode)t.GetInt("palette/draw-mode");
            if (t.Has("palette/draw-item"))
            {
                tabControl.DrawItem += (s, a) =>
                {
                    if (a.Index == tabControl.SelectedIndex)
                    {
                        a.Graphics.FillRectangle(new SolidBrush(t.GetColor("palette/draw-item/background/selected")), a.Bounds);
                    }
                    else
                    {
                        a.Graphics.Clear(t.GetColor("palette/draw-item/background/not-selected"));
                    }

                    TabPage tab = tabControl.TabPages[a.Index];
                    a.Graphics.DrawString(tab.Text, tab.Font, new SolidBrush(t.GetColor("palette/draw-item/foreground")), a.Bounds.X, a.Bounds.Y + 5);
                };
            }
        }

        private void PaletteEditor_Load(object sender, EventArgs e)
        {
            if (!DesignMode)
            {
                Factory.GUIController.ColorThemes.Apply(LoadColorTheme);
            }
        }
    }
}
