using AGS.Types;
using System;
using System.Runtime.InteropServices;
using System.Windows.Forms;

namespace AGS.Editor
{
    public class NativePluginCallbacks
    {
        [DllImport("User32.DLL")]
        private static extern IntPtr GetActiveWindow();

        public static Delegate GetEditorHandleDelegate = new IAGSEditorForNativePlugins.GetEditorHandleDelegate(NativePluginCallbacks.GetEditorHandle);
        public static Delegate GetWindowHandleDelegate = new IAGSEditorForNativePlugins.GetWindowHandleDelegate(NativePluginCallbacks.GetWindowHandle);
        public static Delegate RegisterScriptHeaderDelegate = new IAGSEditorForNativePlugins.RegisterScriptHeaderDelegate(NativePluginCallbacks.RegisterScriptHeader);
        public static Delegate UnregisterScriptHeaderDelegate = new IAGSEditorForNativePlugins.UnRegisterScriptHeaderDelegate(NativePluginCallbacks.UnregisterScriptHeader);

        public static IntPtr GetEditorHandle(IAGSEditorForNativePlugins instance)
        {
            return Factory.GUIController.TopLevelWindowHandle;
        }

        public static IntPtr GetWindowHandle(IAGSEditorForNativePlugins instance)
        {
            return GetActiveWindow();
        }

        public static void RegisterScriptHeader(IAGSEditorForNativePlugins instance, string headerText)
        {
            NativePlugin.PluginLookup[instance.pluginID].RegisterScriptHeader(headerText);
        }

        public static void UnregisterScriptHeader(IAGSEditorForNativePlugins instance, string headerText)
        {
            NativePlugin.PluginLookup[instance.pluginID].UnRegisterScriptHeader(headerText);
        }
    }
}
