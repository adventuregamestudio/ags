using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Drawing.Design;
using System.Reflection;
using System.Text;
using System.Windows.Forms.Design;

namespace AGS.Types
{
    public class RoomMessagesUIEditor : UITypeEditor
    {
        public delegate void RoomMessagesEditorType(List<RoomMessage> messages);
        public static RoomMessagesEditorType ShowRoomMessagesEditor;

        public RoomMessagesUIEditor()
        {
        }

        public override UITypeEditorEditStyle GetEditStyle(ITypeDescriptorContext context)
        {
            return UITypeEditorEditStyle.Modal;
        }

        public override object EditValue(ITypeDescriptorContext context, IServiceProvider provider, object value)
        {
            List<RoomMessage> messages = (List<RoomMessage>)value;

            if (ShowRoomMessagesEditor != null)
            {
                ShowRoomMessagesEditor(messages);

				if (context.Instance is IChangeNotification)
				{
					((IChangeNotification)context.Instance).ItemModified();
				}
			}

            return messages;
        }
    }
}
