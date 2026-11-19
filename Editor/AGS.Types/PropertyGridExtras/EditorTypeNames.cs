using System;

namespace AGS.Types
{
    /// <summary>
    /// Assembly-qualified names of the property grid UI editors and tabs, which
    /// are implemented in the AGS.Controls project. They are written as strings
    /// to keep AGS.Types independent from the UI code. The property grid
    /// resolves them with Type.GetType.
    /// </summary>
    public static class EditorTypeNames
    {
        private const string ControlsAssembly = "AGS.Controls";

        public const string UITypeEditor = "System.Drawing.Design.UITypeEditor, System.Drawing, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a";

        public const string AudioClipSourceFileUIEditor = "AGS.Controls.AudioClipSourceFileUIEditor, " + ControlsAssembly;
        public const string BuildTargetUIEditor = "AGS.Controls.BuildTargetUIEditor, " + ControlsAssembly;
        public const string ColorUIEditor = "AGS.Controls.ColorUIEditor, " + ControlsAssembly;
        public const string CustomPropertiesUIEditor = "AGS.Controls.CustomPropertiesUIEditor, " + ControlsAssembly;
        public const string CustomResolutionUIEditor = "AGS.Controls.CustomResolutionUIEditor, " + ControlsAssembly;
        public const string FontSizeUIEditor = "AGS.Controls.FontSizeUIEditor, " + ControlsAssembly;
        public const string MultiLineStringUIEditor = "AGS.Controls.MultiLineStringUIEditor, " + ControlsAssembly;
        public const string ScriptFunctionUIEditor = "AGS.Controls.ScriptFunctionUIEditor, " + ControlsAssembly;
        public const string SpriteFileNameEditor = "AGS.Controls.SpriteFileNameEditor, " + ControlsAssembly;
        public const string SpriteSelectUIEditor = "AGS.Controls.SpriteSelectUIEditor, " + ControlsAssembly;
        public const string ViewUIEditor = "AGS.Controls.ViewUIEditor, " + ControlsAssembly;

        public const string PropertyTabEvents = "AGS.Controls.PropertyTabEvents, " + ControlsAssembly;
    }
}
