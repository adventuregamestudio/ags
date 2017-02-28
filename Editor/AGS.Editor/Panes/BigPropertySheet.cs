using System;
using AGS.Types;

namespace AGS.Editor
{
    public partial class BigPropertySheet : EditorContentPanel
    {
        private readonly object _propertyObject;

        public BigPropertySheet(object propertyObject)
        {
            InitializeComponent();

            _propertyObject = propertyObject;
            RefreshData();
        }

        public void RefreshData()
        {
            propertyGrid.SelectedObject = null;
            propertyGrid.SelectedObject = _propertyObject;
        }

    }
}
