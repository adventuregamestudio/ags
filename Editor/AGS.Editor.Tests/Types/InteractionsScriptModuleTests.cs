using NUnit.Framework;
using System.Xml;

namespace AGS.Types
{
    /// <summary>
    /// Characters and inventory items keep their interaction functions in their own script module;
    /// loading them must give the module to their interactions too, which the game data writes.
    /// </summary>
    [TestFixture]
    public class InteractionsScriptModuleTests
    {
        [Test]
        public void CharacterInteractionsKeepScriptModuleAfterLoading()
        {
            Character character = new Character();
            character.ScriptModule = "Other.asc";
            XmlDocument doc = character.ToXmlDocument();

            Character loaded = new Character(doc.SelectSingleNode("Character"));

            Assert.That(loaded.ScriptModule, Is.EqualTo("Other.asc"));
            Assert.That(loaded.Interactions.ScriptModule, Is.EqualTo("Other.asc"));
        }

        [Test]
        public void InventoryItemInteractionsKeepScriptModuleAfterLoading()
        {
            InventoryItem item = new InventoryItem();
            item.ScriptModule = "Other.asc";
            XmlDocument doc = item.ToXmlDocument();

            InventoryItem loaded = new InventoryItem(doc.SelectSingleNode("InventoryItem"));

            Assert.That(loaded.ScriptModule, Is.EqualTo("Other.asc"));
            Assert.That(loaded.Interactions.ScriptModule, Is.EqualTo("Other.asc"));
        }
    }
}
