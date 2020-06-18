﻿using System;
using System.Collections.Generic;
using System.ComponentModel;

namespace AGS.Types
{
    public class RoomListTypeConverter : BaseListSelectTypeConverter<int, string>
    {
        private static Dictionary<int, string> _possibleValues = new Dictionary<int, string>();
        private static IList<IRoom> _Rooms = null;

        protected override Dictionary<int, string> GetValueList(ITypeDescriptorContext context)
        {
            return _possibleValues;
        }

        public static void SetRoomList(IList<IRoom> rooms)
        {
            // Keep a refernce to the list so it can be updated whenever we need to
            _Rooms = rooms;
            RefreshRoomList();
        }

        public static void RefreshRoomList()
        {
            if (_Rooms == null)
            {
                throw new InvalidOperationException("Static collection has not been set");
            }

            _possibleValues.Clear();
            _possibleValues.Add(-1, "(None)");
            foreach (IRoom room in _Rooms)
            {
                _possibleValues.Add(room.Number, string.Format("{0}: {1}", room.Number, room.Description));
            }
        }
    }
}
