using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Reflection;

namespace AGS.Types
{
    /// <summary>
    /// The Enum Converter that allows to exclude certain values.
    /// TODO: is there a simplier way to do this?
    /// </summary>
    public abstract class ExclusiveEnumTypeConverter<T> : EnumTypeConverter
    {
        public ExclusiveEnumTypeConverter() : base(typeof(T))
        {
        }

        protected abstract int[] GetExcludedValues(ITypeDescriptorContext context);

        public override TypeConverter.StandardValuesCollection GetStandardValues(ITypeDescriptorContext context)
        {
            List<int> values = new List<int>();
            FieldInfo[] fis = myVal.GetFields();
            int[] excludes = GetExcludedValues(context);
            foreach (FieldInfo fi in fis)
            {
                if ((fi.Attributes & FieldAttributes.Literal) != 0)
                {
                    int ival = (int)fi.GetRawConstantValue();
                    if (Array.IndexOf(excludes, ival) < 0)
                        values.Add(ival);
                }
            }
            return new StandardValuesCollection(values);
        }
    }

    public class FullscreenGameScalingConverter : ExclusiveEnumTypeConverter<GameScaling>
    {
        protected override int[] GetExcludedValues(ITypeDescriptorContext context)
        {
            return new int[1]{(int)GameScaling.Integer};
        }
    }
}
