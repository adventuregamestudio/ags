using System;
using System.Runtime.InteropServices;

namespace AGS.Editor
{
    // Yuck -- this whole thing is horrible, but then so is interop
    // between C# and flat DLL's. Hey, be glad it works at all!

    /// <summary>
    /// The AGSEditor interface that is passed to the native code plugins
    /// </summary>
    [StructLayout(LayoutKind.Explicit)]
    public class IAGSEditorForNativePlugins
    {
        public const int VTABLE_BYTE_OFFSET = 12;

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
		public delegate IntPtr GetEditorHandleDelegate(IAGSEditorForNativePlugins instance);
        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
		public delegate IntPtr GetWindowHandleDelegate(IAGSEditorForNativePlugins instance);
        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
		public delegate void RegisterScriptHeaderDelegate(IAGSEditorForNativePlugins instance, string headerText);
        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
		public delegate void UnRegisterScriptHeaderDelegate(IAGSEditorForNativePlugins instance, string headerText);

        [FieldOffset(0)]
        public IntPtr vtablePtr;
        [FieldOffset(4)]
        public int version;
        [FieldOffset(8)]
        public int pluginID;

        [FieldOffset(VTABLE_BYTE_OFFSET)]
        public IntPtr GetEditorHandle;
        [FieldOffset(VTABLE_BYTE_OFFSET + 4)]
        public IntPtr GetWindowHandle;
        [FieldOffset(VTABLE_BYTE_OFFSET + 8)]
        public IntPtr RegisterScriptHeader;
        [FieldOffset(VTABLE_BYTE_OFFSET + 12)]
        public IntPtr UnregisterScriptHeader;
    }
}
