using AGS.Types;
using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Text;
using System.Text.RegularExpressions;

namespace AGS.Editor
{
    internal class EditorPlugin : IDisposable
    {
        private IAGSEditorPlugin _plugin;
        private string _fileName;

        public EditorPlugin(string fileName, AGSEditorController pluginEditorController)
        {
            Type mainClass = null;
            Assembly assembly = Assembly.LoadFile(Path.GetFullPath(fileName));
            foreach (Type type in assembly.GetExportedTypes())
            {
                if (type.IsClass)
                {
                    foreach (Type iface in type.GetInterfaces())
                    {
                        if (iface == typeof(IAGSEditorPlugin))
                        {
                            if (mainClass != null)
                            {
                                throw new AGSEditorException("Plugin has multiple classes implementing IAGSEditorPlugin");
                            }
                            mainClass = type;
                            break;
                        }
                    }
                }
            }
            if (mainClass == null)
            {
                throw new AGSEditorException("Plugin does not contain any classes that implement IAGSEditorPlugin");
            }

            VerifyRequiredAGSVersion(mainClass);

            ConstructorInfo ctor = mainClass.GetConstructor(new Type[] { typeof(IAGSEditor) });
            if (ctor == null)
            {
                throw new AGSEditorException("Class '" + mainClass.Name + "' does not have a (IAGSEditor) constructor");
            }

            _plugin = (IAGSEditorPlugin)ctor.Invoke(new object[] { pluginEditorController });
            _fileName = fileName;
        }

        private void VerifyRequiredAGSVersion(Type mainClass)
        {
            object[] attributes = mainClass.GetCustomAttributes(typeof(RequiredAGSVersionAttribute), true);
            if (attributes.Length != 1)
            {
                throw new AGSEditorException("Plugin class does not have RequiredAGSVersion attribute");
            }
            string pluginVersionNumber = ((RequiredAGSVersionAttribute)attributes[0]).RequiredVersion;
            if (!Regex.IsMatch(pluginVersionNumber, Settings.REGEX_FOUR_PART_VERSION))
            {
                throw new AGSEditorException("Plugin returned invalid version number: must be in format  a.b.c.d");
            }

            System.Version pluginVersion = new System.Version(pluginVersionNumber);
            System.Version editorVersion = new System.Version(AGS.Types.Version.AGS_EDITOR_VERSION);
            if (pluginVersion.CompareTo(editorVersion) > 0)
            {
                throw new AGSEditorException("Plugin requires a newer version of AGS (" + pluginVersion.ToString() + ")");
            }
        }

        public string FileName
        {
            get { return _fileName; }
        }

        public void Dispose()
        {
            _plugin.Dispose();
        }
    }
}
