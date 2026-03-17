using AGS.Types;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace AGS.Editor.Components
{
    class CustomPropertiesComponent : BaseComponent
    {
        public CustomPropertiesComponent(GUIController guiController, AGSEditor agsEditor)
           : base(guiController, agsEditor)
        {
        }

        public override string ComponentID
        {
            get { return ComponentIDs.CustomProperties; }
        }

        public override void BeforeSaveGame()
        {
            SyncCustomPropertiesWithSchema();
        }

        public override void RefreshDataFromGame()
        {
            SyncCustomPropertiesWithSchema();
        }

        private void SyncCustomPropertiesWithSchema()
        {
            // Note that this syncs only global objects.
            // Room elements cannot be synced without loading each and every room into memory,
            // which could be a pretty long process. So they are dealt with separately, by the RoomsComponent.
            foreach (var c in _agsEditor.CurrentGame.Characters)
                c.Properties.SyncWithSchema();
            foreach (var i in _agsEditor.CurrentGame.InventoryItems)
                i.Properties.SyncWithSchema();
        }
    }
}
