using NUnit.Framework;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    /// <summary>
    /// The build tools in Tools/ read Game.agf with tables of the XML elements they know
    /// (Tools/data/agfreader.cpp). The list in Tools/data/agf_elements.txt names the elements
    /// that the editor writes for each class. This test keeps the list current, and the C++
    /// test AgfElements (Tools/test/agfreader_test.cpp) checks that every element on it has a
    /// row in a table. So adding a property to one of these classes fails here until the list
    /// is regenerated, and then fails there until the tools read the property, or list it as
    /// not needed for building (a row without a reader).
    ///
    /// To regenerate the list, run this test with AGS_UPDATE_AGF_ELEMENTS=1 in the environment.
    /// </summary>
    [TestFixture]
    public class AgfElementsTests
    {
        private const string ListFile = @"Tools\data\agf_elements.txt";
        private const string UpdateVariable = "AGS_UPDATE_AGF_ELEMENTS";

        // One default object for each class that the tools read from Game.agf
        private static SortedDictionary<string, Func<object>> Samples()
        {
            return new SortedDictionary<string, Func<object>>(StringComparer.Ordinal)
            {
                { "AudioClip", () => new AudioClip(1, "aClip", 0) },
                { "AudioClipType", () => new AudioClipType(1, "Type") },
                { "Character", () => new Character() },
                { "CustomProperty", () => new CustomProperty() },
                { "CustomPropertySchemaItem", () => new CustomPropertySchemaItem() },
                { "Dialog", () => new Dialog() },
                { "DialogRef", () => new DialogRef(new Dialog()) },
                { "DialogOption", () => new DialogOption() },
                { "Font", () => new Font() },
                { "FontFile", () => new FontFile() },
                { "Game", () => new Game() },
                { "GlobalVariable", () => new GlobalVariable() },
                { "GUIButton", () => new GUIButton() },
                { "GUIInventory", () => new GUIInventory() },
                { "GUILabel", () => new GUILabel() },
                { "GUIListBox", () => new GUIListBox() },
                { "GUISlider", () => new GUISlider() },
                { "GUITextBox", () => new GUITextBox() },
                { "GUITextWindowEdge", () => new GUITextWindowEdge() },
                { "InventoryItem", () => new InventoryItem() },
                { "LipSync", () => new LipSync() },
                { "MouseCursor", () => new MouseCursor() },
                { "NormalGUI", () => new NormalGUI(320, 200) },
                { "Plugin", () => new Plugin("plugin.dll", new byte[0]) },
                { "RuntimeSetup", () => new RuntimeSetup(new Settings()) },
                { "Script", () => new Script("script.asc", "", false) },
                { "ScriptAndHeader", () => new ScriptAndHeader(new Script("script.ash", "", true), new Script("script.asc", "", false)) },
                { "Settings", () => new Settings() },
                { "Sprite", () => new Sprite(1, 16, 16) },
                { "TextParserWord", () => new TextParserWord() },
                { "TextWindowGUI", () => new TextWindowGUI() },
                { "Translation", () => new Translation("Name", "Base") },
                { "UnloadedRoom", () => new UnloadedRoom(1) },
                { "View", () => new View() },
                { "ViewFrame", () => new ViewFrame() },
                { "ViewLoop", () => new ViewLoop() },
            };
        }

        // Elements that hold more elements of their own, which the tools read by another table:
        // they are listed as "Class.Element"
        private static readonly string[][] NestedElements = new[]
        {
            new[] { "Sprite", "Source" },
        };

        // The names of the elements that the class writes directly inside its own element
        private static XmlElement Serialize(object obj)
        {
            var text = new StringWriter();
            var writer = new XmlTextWriter(text);
            // not all of the classes implement IToXml, but all have this method
            obj.GetType().GetMethod("ToXml", new[] { typeof(XmlTextWriter) }).Invoke(obj, new object[] { writer });
            writer.Flush();
            var doc = new XmlDocument();
            doc.LoadXml(text.ToString());
            return doc.DocumentElement;
        }

        private static List<string> ChildNames(XmlElement element, string className)
        {
            // the GUIs write their properties in an element named like the class
            return element.ChildNodes.Cast<XmlNode>()
                .Where(n => n.NodeType == XmlNodeType.Element)
                .SelectMany(n => n.Name == className ? n.ChildNodes.Cast<XmlNode>() : new[] { n })
                .Where(n => n.NodeType == XmlNodeType.Element)
                .Select(n => n.Name)
                .Distinct()
                .OrderBy(n => n, StringComparer.Ordinal)
                .ToList();
        }

        private static string BuildList()
        {
            var list = new StringBuilder();
            list.Append("# The XML elements that the editor writes in Game.agf, for each class of AGS.Types that\n");
            list.Append("# the build tools read. Generated by AgfElementsTests (Editor/AGS.Editor.Tests/Types);\n");
            list.Append("# do not edit. Checked by the AgfElements test in Tools/test/agfreader_test.cpp.\n");
            foreach (var sample in Samples())
            {
                object obj = sample.Value();
                XmlElement root = Serialize(obj);
                list.Append(sample.Key).Append(": ").Append(string.Join(" ", ChildNames(root, obj.GetType().Name))).Append('\n');
                foreach (var nested in NestedElements.Where(n => n[0] == sample.Key))
                {
                    XmlElement child = root[nested[1]];
                    Assert.IsNotNull(child, sample.Key + " does not write " + nested[1]);
                    list.Append(nested[0]).Append('.').Append(nested[1]).Append(": ")
                        .Append(string.Join(" ", ChildNames(child, nested[1]))).Append('\n');
                }
            }
            return list.ToString();
        }

        private static string FindListFile()
        {
            string dir = TestContext.CurrentContext.TestDirectory;
            while (!string.IsNullOrEmpty(dir))
            {
                // the directory is there even when the list is yet to be written
                if (Directory.Exists(Path.Combine(dir, @"Tools\data")))
                    return Path.Combine(dir, ListFile);
                dir = Path.GetDirectoryName(dir);
            }
            Assert.Fail("Cannot find " + ListFile + " above " + TestContext.CurrentContext.TestDirectory);
            return null;
        }

        [Test]
        public void ListMatchesTheEditor()
        {
            string path = FindListFile();
            string expected = BuildList();
            if (Environment.GetEnvironmentVariable(UpdateVariable) == "1")
            {
                File.WriteAllText(path, expected, new UTF8Encoding(false));
                Assert.Inconclusive(ListFile + " was rewritten.");
            }
            string actual = File.ReadAllText(path).Replace("\r\n", "\n");
            Assert.AreEqual(expected, actual,
                "The editor writes other elements than " + ListFile + " lists. Make the tools read " +
                "them (or mark them as not needed), then regenerate the list: run this test with " +
                UpdateVariable + "=1.");
        }
    }
}
