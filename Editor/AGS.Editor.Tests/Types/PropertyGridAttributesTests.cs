using NUnit.Framework;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing.Design;
using System.Linq;
using System.Reflection;
using System.Windows.Forms.Design;

namespace AGS.Types
{
    /// <summary>
    /// AGS.Types names the property grid UI editors and tabs, which live in the editor
    /// project, by strings. A wrong string is not a compile error, and the property grid
    /// silently ignores an editor it cannot load, so these tests resolve every such
    /// string the way the property grid does.
    /// </summary>
    [TestFixture]
    public class PropertyGridAttributesTests
    {
        private static readonly BindingFlags AllMembers = BindingFlags.Public | BindingFlags.NonPublic |
            BindingFlags.Instance | BindingFlags.Static | BindingFlags.DeclaredOnly;

        private IEnumerable<Type> GetModelTypes()
        {
            return typeof(Sprite).Assembly.GetTypes();
        }

        [Test]
        public void TypeNameConstantsResolve()
        {
            var fields = typeof(EditorTypeNames).GetFields(BindingFlags.Public | BindingFlags.Static)
                .Where(f => f.IsLiteral);
            Assert.That(fields, Is.Not.Empty);
            foreach (var field in fields)
            {
                string name = (string)field.GetRawConstantValue();
                Assert.That(Type.GetType(name), Is.Not.Null, "EditorTypeNames." + field.Name + " does not resolve: " + name);
            }
        }

        [Test]
        public void EditorTypeNamesPointToTypesOfTheSameName()
        {
            foreach (var field in typeof(EditorTypeNames).GetFields(BindingFlags.Public | BindingFlags.Static)
                .Where(f => f.IsLiteral && f.Name != "UITypeEditor"))
            {
                Type type = Type.GetType((string)field.GetRawConstantValue());
                Assert.That(type, Is.Not.Null, field.Name);
                Assert.That(type.Name, Is.EqualTo(field.Name));
            }
        }

        [Test]
        public void EditorAttributesResolveToEditors()
        {
            var failures = new List<string>();
            int checkedCount = 0;
            foreach (Type type in GetModelTypes())
            {
                foreach (EditorAttribute attr in type.GetCustomAttributes(typeof(EditorAttribute), false))
                {
                    checkedCount++;
                    string error = CheckEditor(attr, TypeDescriptor.GetEditor(type, Type.GetType(attr.EditorBaseTypeName)));
                    if (error != null)
                        failures.Add(type.FullName + ": " + error);
                }

                foreach (PropertyInfo property in type.GetProperties(AllMembers))
                {
                    var attrs = property.GetCustomAttributes(typeof(EditorAttribute), false);
                    if (attrs.Length == 0)
                        continue;
                    PropertyDescriptor descriptor = TypeDescriptor.GetProperties(type)[property.Name];
                    foreach (EditorAttribute attr in attrs)
                    {
                        checkedCount++;
                        Type baseType = Type.GetType(attr.EditorBaseTypeName);
                        string error = baseType == null ? "base type " + attr.EditorBaseTypeName + " does not resolve" :
                            descriptor == null ? "no property descriptor" :
                            CheckEditor(attr, descriptor.GetEditor(baseType));
                        if (error != null)
                            failures.Add(type.FullName + "." + property.Name + ": " + error);
                    }
                }
            }

            Assert.That(checkedCount, Is.GreaterThan(100), "the test found suspiciously few EditorAttribute uses");
            Assert.That(failures, Is.Empty, string.Join(Environment.NewLine, failures.ToArray()));
        }

        private string CheckEditor(EditorAttribute attr, object editor)
        {
            Type editorType = Type.GetType(attr.EditorTypeName);
            if (editorType == null)
                return "editor type " + attr.EditorTypeName + " does not resolve";
            if (editor == null)
                return "the property grid gets no editor for " + attr.EditorTypeName;
            if (editor.GetType() != editorType)
                return "expected " + editorType.FullName + " but got " + editor.GetType().FullName;
            return null;
        }

        [Test]
        public void PropertyTabAttributesResolve()
        {
            int checkedCount = 0;
            foreach (Type type in GetModelTypes())
            {
                foreach (PropertyTabAttribute attr in type.GetCustomAttributes(typeof(PropertyTabAttribute), false))
                {
                    checkedCount++;
                    Type[] tabs = null;
                    Assert.DoesNotThrow(() => tabs = attr.TabClasses, type.FullName);
                    Assert.That(tabs, Is.Not.Null.And.Not.Empty, type.FullName);
                    Assert.That(tabs, Has.All.Not.Null, type.FullName);
                    Assert.That(typeof(PropertyTab).IsAssignableFrom(tabs[0]), type.FullName);
                }
            }
            Assert.That(checkedCount, Is.GreaterThan(0));
        }

        [Test]
        public void EventsTabLoadsItsImage()
        {
            var tab = new AGS.Controls.PropertyTabEvents();
            Assert.That(tab.Bitmap, Is.Not.Null);
        }

        [Test]
        public void TypesAssemblyHasNoPropertyGridUIClasses()
        {
            Assert.That(GetModelTypes().Where(t => typeof(UITypeEditor).IsAssignableFrom(t)), Is.Empty);
            Assert.That(GetModelTypes().Where(t => typeof(PropertyTab).IsAssignableFrom(t)), Is.Empty);
        }
    }
}
