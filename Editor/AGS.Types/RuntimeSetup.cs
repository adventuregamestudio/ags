using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    public class RuntimeSetup : ICustomTypeDescriptor
    {
        private GraphicsDriver _graphicsDriver = GraphicsDriver.D3D9;
        private bool _renderAtScreenRes = false;
        private string _titleText;
        private Settings _gameSettings;

        public RuntimeSetup(Settings gameSettings)
        {
            _gameSettings = gameSettings;
        }

        [DisplayName("Graphics driver")]
        [Description("The default graphics driver that your game will use. Direct3D allows fast high-resolution alpha-blended sprites, but DirectDraw is better at RawDrawing.")]
        [DefaultValue(GraphicsDriver.D3D9)]
        [Category("Graphics")]
        [TypeConverter(typeof(EnumTypeConverter))]
        public GraphicsDriver GraphicsDriver
        {
            get { return _graphicsDriver; }
            set { _graphicsDriver = value; }
        }

        [DisplayName("Render sprites at screen resolution")]
        [Description("When drawing zoomed character and object sprites, AGS will take advantage of higher runtime resolution to give scaled images more detail, than it would be possible if the game was displayed in its native resolution. The effect is stronger for low-res games. Keep disabled for pixel-perfect output. Currently supported only by Direct3D renderer.")]
        [DefaultValue(false)]
        [Category("Graphics")]
        public bool RenderAtScreenResolution
        {
            get { return _renderAtScreenRes; }
            set { _renderAtScreenRes = value; }
        }

        [DisplayName("Title text")]
        [Description("Text shown at the title bar of the setup program.")]
        [Category("(Setup appearance)")]
        public string TitleText
        {
            get { return _titleText; }
            set { _titleText = value; }
        }

        public void ToXml(XmlTextWriter writer)
        {
            SerializeUtils.SerializeToXML(this, writer);
        }

        public void FromXml(XmlNode node)
        {
            _graphicsDriver = GraphicsDriver.D3D9;
            _renderAtScreenRes = false;

            SerializeUtils.DeserializeFromXML(this, node);
        }

        #region ICustomTypeDescriptor Members

        public AttributeCollection GetAttributes()
        {
            return TypeDescriptor.GetAttributes(this, true);
        }

        public string GetClassName()
        {
            return TypeDescriptor.GetClassName(this, true);
        }

        public string GetComponentName()
        {
            return TypeDescriptor.GetComponentName(this, true);
        }

        public TypeConverter GetConverter()
        {
            return TypeDescriptor.GetConverter(this, true);
        }

        public EventDescriptor GetDefaultEvent()
        {
            return TypeDescriptor.GetDefaultEvent(this, true);
        }

        public PropertyDescriptor GetDefaultProperty()
        {
            return TypeDescriptor.GetDefaultProperty(this, true);
        }

        public object GetEditor(Type editorBaseType)
        {
            return TypeDescriptor.GetEditor(this, editorBaseType, true);
        }

        public EventDescriptorCollection GetEvents(Attribute[] attributes)
        {
            return TypeDescriptor.GetEvents(this, attributes, true);
        }

        public EventDescriptorCollection GetEvents()
        {
            return TypeDescriptor.GetEvents(this, true);
        }

        public PropertyDescriptorCollection GetProperties(Attribute[] attributes)
        {
            PropertyDescriptorCollection properties = TypeDescriptor.GetProperties(this, attributes, true);
            List<PropertyDescriptor> wantProperties = new List<PropertyDescriptor>();
            foreach (PropertyDescriptor property in properties)
            {
                bool wantThisProperty = true;
                // Only display RenderAtScreenRes if game settings have this property set to UserDefined
                if (property.Name == "RenderAtScreenResolution")
                    wantThisProperty = _gameSettings.RenderAtScreenResolution == AGS.Types.RenderAtScreenResolution.UserDefined;

                if (wantThisProperty)
                {
                    wantProperties.Add(property);
                }
            }
            return new PropertyDescriptorCollection(wantProperties.ToArray());
        }

        public PropertyDescriptorCollection GetProperties()
        {
            PropertyDescriptorCollection properties = TypeDescriptor.GetProperties(this, true);
            return properties;
        }

        public object GetPropertyOwner(PropertyDescriptor pd)
        {
            return this;
        }

        #endregion
    }
}
