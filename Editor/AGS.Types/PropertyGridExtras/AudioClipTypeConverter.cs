using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Drawing.Design;
using System.Reflection;

namespace AGS.Types
{
    public class AudioClipTypeConverter : BaseListSelectTypeConverter<AudioClip>
    {
        private static Dictionary<int, string> _possibleValues = new Dictionary<int, string>();
        private static IList<AudioClip> _AudioClips = null;

        protected override Dictionary<int, string> GetValueList()
        {
            return _possibleValues;
        }

        public static void SetAudioClipList(IList<AudioClip> audioClips)
        {
            // Keep a refernce to the list so it can be updated whenever we need to
            _AudioClips = audioClips;
            RefreshAudioClipList();
        }

        public static void RefreshAudioClipList()
        {
            if (_AudioClips == null)
            {
                throw new InvalidOperationException("Static collection has not been set");
            }

            _possibleValues.Clear();
            _possibleValues.Add(0, "(None)");
            foreach (AudioClip type in _AudioClips)
            {
                _possibleValues.Add(type.Index, type.ScriptName);
            }
        }
    }
}