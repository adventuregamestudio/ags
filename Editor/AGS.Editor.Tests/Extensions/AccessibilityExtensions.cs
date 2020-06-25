using System.Reflection;

namespace AGS.Editor.Extensions
{
    /// <summary>
    /// Minor framework for accessing any members from any class through .NET reflection. Useful for testing certain
    /// classes without public accessors, but should be used sparingly since referencing the members by string names
    /// break easily when refactoring. Prefer to have a public accessible property or something that you can use to
    /// test with, if not, then use these extensions method to get the values you need.
    /// </summary>
    /// <example>
    /// <code>
    /// ClassWithHiddenMembers testClass = new ClassWithHiddenMembers();
    /// testClass.SetHiddenFieldValue("_theHiddenField", "Example value");
    /// Console.WriteLine(testClassGetHiddenFieldValue<string>("_theHiddenField"));
    /// </code>
    /// </example>
    public static class AccessibilityExtensions
    {
        /// <summary>
        /// Public: Gets any public or internal members.
        /// NonPublic: Gets any private or protected members.
        /// Instance: Gets any instance members, as opposed to static members
        /// </summary>
        private const BindingFlags SEARCHABLE_MEMBERS_FLAGS = BindingFlags.NonPublic | BindingFlags.Instance;

        /// <summary>
        /// Gets a private or protected field instance from a object using reflection.
        /// </summary>
        /// <typeparam name="T">The type we want to cast the result to.</typeparam>
        /// <param name="obj">The object to get the field instance from.</param>
        /// <param name="name">The name of the field.</param>
        /// <returns>An object instance with the field data. Or default value for type if the field could not be found.</returns>
        public static T GetHiddenFieldValue<T>(this object obj, string name)
        {
            return (T)obj.GetType().GetField(name, SEARCHABLE_MEMBERS_FLAGS).GetValue(obj);
        }

        /// <summary>
        /// Sets a private or protected field instance from a object using reflection.
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <param name="obj">The object to get the field instance from.</param>
        /// <param name="name">The name of the field.</param>
        /// <param name="value">The new value for the field.</param>
        public static void SetHiddenFieldValue<T>(this object obj, string name, T value)
        {
            obj.GetType().GetField(name, SEARCHABLE_MEMBERS_FLAGS).SetValue(obj, value);
        }

        /// <summary>
        /// Gets a private or protected property instance from a object using reflection.
        /// </summary>
        /// <typeparam name="T">The type we want to cast the result to.</typeparam>
        /// <param name="obj">The object to get the property instance from.</param>
        /// <param name="name">The name of the property.</param>
        /// <returns>An object instance with the property data. Or default value for type if the property could not be found.</returns>
        public static T GetHiddenPropertyValue<T>(this object obj, string name)
        {
            return (T)obj.GetType().GetProperty(name, SEARCHABLE_MEMBERS_FLAGS).GetValue(obj);
        }

        /// <summary>
        /// Sets a private or protected property instance from a object using reflection.
        /// </summary>
        /// <typeparam name="T">The type we want to cast the result to.</typeparam>
        /// <param name="obj">The object to get the property instance from.</param>
        /// <param name="name">The name of the property.</param>
        /// <param name="value">The new value for the property.</param>
        public static void SetHiddenPropertyValue<T>(this object obj, string name, T value)
        {
            obj.GetType().GetProperty(name, SEARCHABLE_MEMBERS_FLAGS).SetValue(obj, value);
        }

        /// <summary>
        /// Invoke hidden methods from a object using reflection and returns the result.
        /// </summary>
        /// <param name="obj">The object to get the method from.</param>
        /// <param name="name">The name of the method.</param>
        /// <param name="parameters">The parameters for the method.</param>
        /// <returns>The result of the invoked method.</returns>
        public static void InvokeHiddenMethod(this object obj, string name, params object[] parameters)
        {
            obj.GetType().GetMethod(name, SEARCHABLE_MEMBERS_FLAGS).Invoke(obj, parameters);
        }

        /// <summary>
        /// Invoke hidden methods from a object using reflection and returns the result.
        /// </summary>
        /// <param name="obj">The object to get the method from.</param>
        /// <param name="name">The name of the method.</param>
        /// <param name="parameters">The parameters for the method.</param>
        /// <returns>The result of the invoked method.</returns>
        public static T InvokeHiddenMethod<T>(this object obj, string name, params object[] parameters)
        {
            return (T)obj.GetType().GetMethod(name, SEARCHABLE_MEMBERS_FLAGS).Invoke(obj, parameters);
        }
    }
}
