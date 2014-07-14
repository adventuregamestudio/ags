using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Drawing.Design;
using System.Reflection;

namespace AGS.Types
{
    public class AudioClipTypeTypeConverter : BaseListSelectTypeConverter
    {
        private static Dictionary<int, string> _possibleValues = new Dictionary<int, string>();
        private static IList<AudioClipType> _AudioClipTypes = null;

        protected override Dictionary<int, string> GetValueList()
        {
            return _possibleValues;
        }

        public static void SetAudioClipTypeList(IList<AudioClipType> audioClipTypes)
        {
            // Keep a refernce to the list so it can be updated whenever we need to
            _AudioClipTypes = audioClipTypes;
            RefreshAudioClipTypeList();
        }

        public static void RefreshAudioClipTypeList()
        {
            if (_AudioClipTypes == null)
            {
                throw new InvalidOperationException("AudioClipTypes static collection has not been set");
            }

            _possibleValues.Clear();
            foreach (AudioClipType type in _AudioClipTypes)
            {
                _possibleValues.Add(type.TypeID, type.Name);
            }
        }

    }
}