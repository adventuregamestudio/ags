using AGS.Types;
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

namespace AGS.Editor
{
    /// <summary>
    /// Yuck! All this interop business is horrid. Also, this class makes
    /// some assumptions about 32-bit with its use of IntPtr and Int32,
    /// so watch out if we ever port this to 64-bit.
    /// </summary>
    public class NativePlugin : IDisposable
    {
        private const int MAX_PLUGIN_DATA_SIZE = 5120;

        // the return type of GetPluginName must be IntPtr and not string
        // because if we use string the CLR will try to free it using CoFreeMem
        // but it hasn't been allocated
        [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        private delegate IntPtr AGS_GetPluginName();
        [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        private delegate int AGS_EditorStartup(IAGSEditorForNativePlugins editor);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        private delegate void AGS_EditorShutdown();
        [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        private delegate void AGS_EditorProperties(IntPtr parentWindowHwnd);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        private delegate int AGS_EditorSaveGame(byte[] buffer, int bufferSize);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        private delegate void AGS_EditorLoadGame(byte[] buffer, int bufferSize);

        private static int NextPluginID = 100;
        internal static Dictionary<int, NativePlugin> PluginLookup = new Dictionary<int, NativePlugin>();

        private IntPtr _dllHandle = IntPtr.Zero;
        private IAGSEditorForNativePlugins _editorInterface = new IAGSEditorForNativePlugins();
        private GCHandle _pinnedHandle;
        private AGS_GetPluginName _getPluginName;
        private AGS_EditorStartup _editorStartup;
        private AGS_EditorShutdown _editorShutdown;
        private AGS_EditorProperties _editorProperties;
        private AGS_EditorSaveGame _editorSaveGame;
        private AGS_EditorLoadGame _editorLoadGame;
        private string _pluginName;
        private string _fileName;
        private bool _enabled;
        private List<Script> _scriptHeaders = new List<Script>();

        public NativePlugin(string fileName)
        {
            _editorInterface.version = 1;
            _editorInterface.pluginID = NextPluginID;
            NextPluginID++;
            _editorInterface.GetEditorHandle = Marshal.GetFunctionPointerForDelegate(NativePluginCallbacks.GetEditorHandleDelegate);
            _editorInterface.GetWindowHandle = Marshal.GetFunctionPointerForDelegate(NativePluginCallbacks.GetWindowHandleDelegate);
            _editorInterface.RegisterScriptHeader = Marshal.GetFunctionPointerForDelegate(NativePluginCallbacks.RegisterScriptHeaderDelegate);
            _editorInterface.UnregisterScriptHeader = Marshal.GetFunctionPointerForDelegate(NativePluginCallbacks.UnregisterScriptHeaderDelegate);

            _dllHandle = NativeProxy.LoadLibrary(fileName);
            if (_dllHandle == IntPtr.Zero)
            {
                throw new AGSEditorException("Unable to load plugin '" + fileName + "'. It may depend on another DLL that is missing.");
            }

            _getPluginName = (AGS_GetPluginName)GetManagedDelegateForFunction("AGS_GetPluginName", typeof(AGS_GetPluginName), true);
            _editorStartup = (AGS_EditorStartup)GetManagedDelegateForFunction("AGS_EditorStartup", typeof(AGS_EditorStartup), true);
            _editorShutdown = (AGS_EditorShutdown)GetManagedDelegateForFunction("AGS_EditorShutdown", typeof(AGS_EditorShutdown), true);
            _editorProperties = (AGS_EditorProperties)GetManagedDelegateForFunction("AGS_EditorProperties", typeof(AGS_EditorProperties), false);
            _editorSaveGame = (AGS_EditorSaveGame)GetManagedDelegateForFunction("AGS_EditorSaveGame", typeof(AGS_EditorSaveGame), false);
            _editorLoadGame = (AGS_EditorLoadGame)GetManagedDelegateForFunction("AGS_EditorLoadGame", typeof(AGS_EditorLoadGame), false);

            _pluginName = Marshal.PtrToStringAnsi(_getPluginName());
            _fileName = fileName;
            PluginLookup.Add(_editorInterface.pluginID, this);
        }

        public string PluginName
        {
            get { return _pluginName; }
        }

        public string FileName
        {
            get { return _fileName; }
        }

        public bool Enabled
        {
            get { return _enabled; }
            set
            {
                if ((value == true) && (_enabled == false))
                {
                    StartPlugin();
                }
                if ((value == false) && (_enabled == true))
                {
                    StopPlugin();
                }
            }
        }

        public bool HasProperties
        {
            get { return (_editorProperties != null); }
        }

        public List<Script> ScriptHeaders
        {
            get { return _scriptHeaders; }
        }

        public void ShowPropertiesWindow(IntPtr parentWindowHwnd)
        {
            if (HasProperties)
            {
                _editorProperties(parentWindowHwnd);
            }
        }

        public void DeserializeData(byte[] data)
        {
            if (_editorLoadGame != null)
            {
                _editorLoadGame(data, data.Length);
            }
        }

        public byte[] SerializeData()
        {
            byte[] data = new byte[0];
            if (_editorSaveGame != null)
            {
                byte[] buffer = new byte[MAX_PLUGIN_DATA_SIZE];
                int bytesWritten = _editorSaveGame(buffer, buffer.Length);
                if ((bytesWritten < 0) || (bytesWritten > buffer.Length))
                {
                    Factory.GUIController.ShowMessage("Plugin tried to write an invalid amount of save data.", System.Windows.Forms.MessageBoxIcon.Warning);
                    bytesWritten = 0;
                }
                else if (bytesWritten > 0)
                {
                    data = new byte[bytesWritten];
                    Array.Copy(buffer, 0, data, 0, data.Length);
                }
            }
            return data;
        }

        public void RegisterScriptHeader(string header)
        {
            Script newHeader = new Script("__Plugin" + _editorInterface.pluginID + ".ash", header, true);
            AutoComplete.ConstructCache(newHeader);
            _scriptHeaders.Add(newHeader);
        }

        public void UnRegisterScriptHeader(string header)
        {
            foreach (Script script in _scriptHeaders)
            {
                if (script.Text == header)
                {
                    _scriptHeaders.Remove(script);
                    break;
                }
            }
        }

        private void StartPlugin()
        {
            _pinnedHandle = GCHandle.Alloc(_editorInterface, GCHandleType.Pinned);
            _editorInterface.vtablePtr = new IntPtr(_pinnedHandle.AddrOfPinnedObject().ToInt32() + IAGSEditorForNativePlugins.VTABLE_BYTE_OFFSET);
            int retVal = _editorStartup(_editorInterface);
            if (retVal != 0)
            {
                _pinnedHandle.Free();
                throw new AGSEditorException("Plugin '" + _fileName + "' failed to start");
            }
            _enabled = true;
        }

        private void StopPlugin()
        {
            if (_editorShutdown != null)
            {
                _editorShutdown();
            }
            _scriptHeaders.Clear();
            if (_pinnedHandle.IsAllocated)
            {
                _pinnedHandle.Free();
            }
            _enabled = false;
        }

        private Delegate GetManagedDelegateForFunction(string functionName, Type delegateType, bool mustExist)
        {
            IntPtr pAddressOfFunctionToCall = NativeProxy.GetProcAddress(_dllHandle, functionName);
            if (pAddressOfFunctionToCall == IntPtr.Zero)
            {
                if (mustExist)
                {
                    throw new AGSEditorException("Function '" + functionName + "' not exported from plugin");
                }
                return null;
            }

            return Marshal.GetDelegateForFunctionPointer(pAddressOfFunctionToCall, delegateType);
        }

        public void Dispose()
        {
            if (_dllHandle != IntPtr.Zero)
            {
                if (_enabled)
                {
                    StopPlugin();
                }
                NativeProxy.FreeLibrary(_dllHandle);
                _dllHandle = IntPtr.Zero;
            }
        }

        ~NativePlugin()
        {
            Dispose();
        }
    }
}
