using NUnit.Framework;
using NUnit.Framework.Internal;

namespace AGS.Editor.Extensions
{
    [TestFixture]
    public class AccessibilityExtensionsTests
    {
        private TestClass _dummy;

        [SetUp]
        public void SetUp()
        {
            _dummy = new TestClass();
        }

        [TestCase(0)]
        [TestCase(5)]
        [TestCase(int.MaxValue)]
        public void GetAndSetHiddenField(int value)
        {
            _dummy.SetHiddenFieldValue("testField", value);
            Assert.That(_dummy.GetHiddenFieldValue<int>("testField"), Is.EqualTo(value));
        }

        [TestCase(null)]
        [TestCase("")]
        [TestCase("test")]
        public void GetAndSetHiddenProperty(string property)
        {
            _dummy.SetHiddenPropertyValue("TestProperty", property);
            Assert.That(_dummy.GetHiddenPropertyValue<string>("TestProperty"), Is.EqualTo(property));
        }

        [Test]
        public void InvokesMethods()
        {
            _dummy.InvokeHiddenMethod("TestMethod");
            Assert.That(_dummy.GetHiddenPropertyValue<string>("TestProperty"), Is.EqualTo("Invoked"));
        }

        [Test]
        public void InvokesMethodsWithReturnValues()
        {
            Assert.That(_dummy.InvokeHiddenMethod<string>("TestMethodWithReturnValue"), Is.EqualTo("Invoked"));
        }

        private class TestClass
        {
            private int testField;

            protected string TestProperty { get; set; }

            private void TestMethod() => TestProperty = "Invoked";

            private string TestMethodWithReturnValue() => "Invoked";
        }
    }
}
