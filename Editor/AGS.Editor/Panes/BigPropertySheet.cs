using System;
using AGS.Types;

namespace AGS.Editor
{
    public partial class BigPropertySheet : EditorContentPanel
    {
        private readonly object _propertyObject;

        public BigPropertySheet()
        {
            InitializeComponent();
        }

        public BigPropertySheet(object propertyObject) : this()
        {
            _propertyObject = propertyObject;
            RefreshData();
        }

        public void RefreshData()
        {
            propertyGrid.SelectedObject = _propertyObject;
        }
    }
}
