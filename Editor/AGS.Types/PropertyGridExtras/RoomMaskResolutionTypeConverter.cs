using System;
using System.Collections.Generic;
using System.ComponentModel;

namespace AGS.Types
{
    public class RoomMaskResolutionTypeConverter : BaseListSelectTypeConverter<int, string>
    {
        private static int _minValue, _maxValue;
        private static Dictionary<int, string> _possibleValues = new Dictionary<int, string>();

        protected override Dictionary<int, string> GetValueList(ITypeDescriptorContext context)
        {
            return _possibleValues;
        }

        public static void SetResolutionRange(int min, int max)
        {
            _minValue = Math.Min(min, max);
            _maxValue = Math.Max(min, max);
            _possibleValues.Clear();
            for (int i = min; i <= max; ++i)
                _possibleValues.Add(i, String.Format("1:{0}", i));
        }
    }
}
