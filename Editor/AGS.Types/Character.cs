using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;
using System.Xml;
using System.Drawing;

namespace AGS.Types
{
    [PropertyTab(typeof(PropertyTabInteractions), PropertyTabScope.Component)]
    public class Character : ICustomTypeDescriptor, IToXml, IComparable<Character>
    {
        public const string PROPERTY_NAME_SCRIPTNAME = "ScriptName";
        public const int NARRATOR_CHARACTER_ID = 999;

        private static InteractionSchema _interactionSchema;

        private int _id;
        private string _scriptName = string.Empty;
        private string _fullName = string.Empty;
        private int _view = 1;
        private int _speechView;
        private int _idleView;
        private int _thinkingView;
        private int _blinkingView;
        private int _startingRoom = 0;
        private int _startX = 160, _startY = 120;
        private bool _uniformMovementSpeed = true;
        private int _movementSpeed = 3;
        private int _movementSpeedX, _movementSpeedY;
        private int _animationDelay = 4;
        private int _speechAnimationDelay = 5;
        private int _speechColor = 12;
        private bool _solid = true;
        private bool _clickable = true;
        private bool _useRoomAreaScaling = true;
        private bool _useRoomAreaLighting = true;
        private bool _turnBeforeWalking = true;
        private bool _diagonalLoops = true;
        private bool _adjustSpeedWithScaling;
        private bool _adjustVolumeWithScaling;
        private bool _movementLinkedToAnimation = true;
        private CustomProperties _properties;
        private Interactions _interactions = new Interactions(_interactionSchema);

        static Character()
        {
            _interactionSchema = new InteractionSchema(new string[] {"$$01 character",
                "$$02 character","$$03 character","Use inventory on character",
                "Any click on character", "$$05 character","$$08 character", 
                "$$09 character"},
                new string[] { "Look", "Interact", "Talk", "UseInv", "AnyClick", "PickUp", "Mode8", "Mode9" });
        }

        public Character()
        {
            _properties = new CustomProperties();
        }

        [Description("The ID number of the character")]
        [Category("Design")]
        [ReadOnly(true)]
        public int ID
        {
            get { return _id; }
            set { _id = value; }
        }

        [DisplayName(PROPERTY_NAME_SCRIPTNAME)]
        [Description("The script name of the character")]
        [Category("Design")]
        public string ScriptName
        {
            get { return _scriptName; }
            set { _scriptName = Utilities.ValidateScriptName(value); }
        }

        [Description("The full name of the character")]
        [Category("Design")]
        public string RealName
        {
            get { return _fullName; }
            set { _fullName = value; }
        }

        [Description("The normal walking view for the character")]
        [Category("Appearance")]
        [EditorAttribute(typeof(ViewUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public int NormalView
        {
            get { return _view; }
            set { _view = value; }
        }

        [Description("The talking view for the character")]
        [Category("Appearance")]
        [EditorAttribute(typeof(ViewUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public int SpeechView
        {
            get { return _speechView; }
            set { _speechView = value; }
        }

        [Description("The idle view for the character")]
        [Category("Appearance")]
        [EditorAttribute(typeof(ViewUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public int IdleView
        {
            get { return _idleView; }
            set { _idleView = value; }
        }

        [Description("The thinking view for the character")]
        [Category("Appearance")]
        [EditorAttribute(typeof(ViewUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public int ThinkingView
        {
            get { return _thinkingView; }
            set { _thinkingView = value; }
        }

        [Description("The blinking view for the character")]
        [Category("Appearance")]
        [EditorAttribute(typeof(ViewUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public int BlinkingView
        {
            get { return _blinkingView; }
            set { _blinkingView = value; }
        }

        [Description("Room number that the character starts in")]
        [Category("Design")]
        [TypeConverter(typeof(RoomListTypeConverter))]
        public int StartingRoom
        {
            get { return _startingRoom; }
            set { _startingRoom = value; }
        }

        [Description("X co-ordinate within the room where the character starts")]
        [Category("Design")]
        public int StartX
        {
            get { return _startX; }
            set { _startX = value; }
        }

        [Description("Y co-ordinate within the room where the character starts")]
        [Category("Design")]
        public int StartY
        {
            get { return _startY; }
            set { _startY = value; }
        }

        [Description("Whether to move at the same speed for horizontal and vertical directions")]
        [Category("Movement")]
        [RefreshProperties(RefreshProperties.All)]
        public bool UniformMovementSpeed
        {
            get { return _uniformMovementSpeed; }
            set
            {
                _uniformMovementSpeed = value;
                if ((!_uniformMovementSpeed) && (_movementSpeedX == 0))
                {
                    _movementSpeedX = _movementSpeed;
                    _movementSpeedY = _movementSpeed;
                }
            }
        }

        [Description("The character's movement speed")]
        [Category("Movement")]
        public int MovementSpeed
        {
            get { return _movementSpeed; }
            set { _movementSpeed = value; }
        }

        [Description("X component of the movement speed")]
        [Category("Movement")]
        public int MovementSpeedX
        {
            get { return _movementSpeedX; }
            set { _movementSpeedX = value; }
        }

        [Description("Y component of the movement speed")]
        [Category("Movement")]
        public int MovementSpeedY
        {
            get { return _movementSpeedY; }
            set { _movementSpeedY = value; }
        }

        [Description("Delay between changing frames whilst animating or walking")]
        [Category("Movement")]
        public int AnimationDelay
        {
            get { return _animationDelay; }
            set { _animationDelay = value; }
        }

        [Description("Delay between changing frames whilst talking")]
        [Category("Appearance")]
        public int SpeechAnimationDelay
        {
            get { return _speechAnimationDelay; }
            set { _speechAnimationDelay = value; }
        }

        [Description("The AGS Colour Number of character's speech text")]
        [Category("Appearance")]
        [DisplayName("SpeechColorNumber")]
        [RefreshProperties(RefreshProperties.All)]
        public int SpeechColor
        {
            get { return _speechColor; }
            set { _speechColor = value; }
        }

        [Description("Character's Speech Color in RGB")]
        [Category("Appearance")]
        [DisplayName("SpeechColor")]
        [RefreshProperties(RefreshProperties.All)]
        [AGSNoSerialize]
        public Color SpeechColorRGB
        {
            get
            {
                return new AGSColor(_speechColor).ToRgb();
            }
            set
            {
                _speechColor = new AGSColor(value).ColorNumber;
            }
        }

        [Description("If true, this character cannot walk through any other characters marked as Solid")]
        [Category("Movement")]
        public bool Solid
        {
            get { return _solid; }
            set { _solid = value; }
        }

        [Description("If true, this character can be clicked on; otherwise mouse clicks will pass through it")]
        [Category("Design")]
        public bool Clickable
        {
            get { return _clickable; }
            set { _clickable = value; }
        }

        [Description("Whether the character should be affected by walkable area scaling")]
        [Category("Appearance")]
        public bool UseRoomAreaScaling
        {
            get { return _useRoomAreaScaling; }
            set { _useRoomAreaScaling = value; }
        }

        [Description("Whether the character should be affected by walkable area lighting")]
        [Category("Appearance")]
        public bool UseRoomAreaLighting
        {
            get { return _useRoomAreaLighting; }
            set { _useRoomAreaLighting = value; }
        }

        [Description("Whether the character will turn to face their new direction before walking")]
        [Category("Movement")]
        public bool TurnBeforeWalking
        {
            get { return _turnBeforeWalking; }
            set { _turnBeforeWalking = value; }
        }

        [Description("Specifies that the walking view is using loops 4-7 for diagonal directions")]
        [Category("Movement")]
        public bool DiagonalLoops
        {
            get { return _diagonalLoops; }
            set { _diagonalLoops = value; }
        }

        [Description("Adjusts the character's movement speed in line with their scaling level")]
        [Category("Movement")]
        public bool AdjustSpeedWithScaling
        {
            get { return _adjustSpeedWithScaling; }
            set { _adjustSpeedWithScaling = value; }
        }

        [Description("Adjusts the volume of any frame-linked sounds depending on the character's scaling level")]
        [Category("Appearance")]
        public bool AdjustVolumeWithScaling
        {
            get { return _adjustVolumeWithScaling; }
            set { _adjustVolumeWithScaling = value; }
        }

        [Description("The character will only move when its animation frame changes (this avoids a 'gliding' effect)")]
        [Category("Movement")]
        public bool MovementLinkedToAnimation
        {
            get { return _movementLinkedToAnimation; }
            set { _movementLinkedToAnimation = value; }
        }

        [Browsable(false)]
        public string WindowTitle
        {
            get { return "Char: " + this.ScriptName; }
        }

        [AGSSerializeClass()]
        [Description("Custom properties for this character")]
        [Category("Properties")]
        [EditorAttribute(typeof(CustomPropertiesUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public CustomProperties Properties
        {
            get { return _properties; }
            protected set { _properties = value; }
        }

        [AGSNoSerialize()]
        [Browsable(false)]
        public Interactions Interactions
        {
            get { return _interactions; }
        }

        [Browsable(false)]
        public string PropertyGridTitle
        {
            get { return _scriptName + " (Character; ID " + _id + ")"; }
        }

        public Character(XmlNode node)
        {
            SerializeUtils.DeserializeFromXML(this, node);
            _interactions.FromXml(node);
        }

        public void ToXml(XmlTextWriter writer)
        {
            SerializeUtils.SerializeToXML(this, writer, false);
            _interactions.ToXml(writer);
            writer.WriteEndElement();
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
            // We want to hide the X & Y movement speed properties if they aren't
            // using Uniform Movement, and hide the standard movement speed property
            // if they are.
            PropertyDescriptorCollection properties = TypeDescriptor.GetProperties(this, attributes, true);
            List<PropertyDescriptor> wantProperties = new List<PropertyDescriptor>();
            foreach (PropertyDescriptor property in properties)
            {
                bool wantThisProperty = true;
                if ((_uniformMovementSpeed) &&
                    ((property.Name == "MovementSpeedX") || (property.Name == "MovementSpeedY")))
                {
                    wantThisProperty = false;
                }
                else if ((!_uniformMovementSpeed) && (property.Name == "MovementSpeed"))
                {
                    wantThisProperty = false;
                }

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

        #region IComparable<Character> Members

        public int CompareTo(Character other)
        {
            return ID.CompareTo(other.ID);
        }

        #endregion
    }
}
