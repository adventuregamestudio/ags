using System;
using System.Collections.Generic;
using System.Drawing;
using System.Reflection;
using System.Text;
using System.Xml;
using System.Globalization;

namespace AGS.Types
{
    public class SerializeUtils
    {
        /// <summary>
        /// Wrapper function for SelectSingleNode that throws an exception
        /// mentioning the node name if it is not found. Returns the node text
        /// if successful.
        /// </summary>
        public static string GetElementString(XmlNode node, string elementName)
        {
            XmlNode foundNode = node.SelectSingleNode(elementName);
            if (foundNode == null) 
            {
                throw new InvalidDataException("Missing XML element: " + elementName);
            }
            return foundNode.InnerText;
        }

        /// <summary>
        /// Wrapper function for SelectSingleNode that returns the node text
        /// or some default text if it does not exist.
        /// </summary>
        public static string GetElementStringOrDefault(XmlNode node, string elementName, string defaultValue)
        {
            XmlNode foundNode = node.SelectSingleNode(elementName);
            if (foundNode == null)
            {
                return defaultValue;
            }
            return foundNode.InnerText;
        }

        /// <summary>
        /// Wrapper function for SelectSingleNode that throws an exception
        /// mentioning the node name if it is not found. Returns the node's
        /// children if successful.
        /// </summary>
        public static XmlNodeList GetChildNodes(XmlNode parent, string elementName)
        {
            XmlNode foundNode = parent.SelectSingleNode(elementName);
            if (foundNode == null)
            {
                throw new InvalidDataException("Missing XML element: " + elementName);
            }
            return foundNode.ChildNodes;
        }

        public static void SerializeToXML(object obj, XmlTextWriter writer)
        {
            SerializeToXML(obj, writer, true);
        }

        public static void SerializeToXML(object obj, XmlTextWriter writer, bool writeEndElement)
        {
            writer.WriteStartElement(obj.GetType().Name);
            PropertyInfo[] properties = obj.GetType().GetProperties();
            foreach (PropertyInfo prop in properties)
            {
                if (prop.GetCustomAttributes(typeof(AGSNoSerializeAttribute), true).Length > 0)
                {
                    continue;
                }
                if ((prop.CanRead) && (prop.CanWrite))
                {
                    bool canSerializeClass = false;
                    if (prop.GetCustomAttributes(typeof(AGSSerializeClassAttribute), true).Length > 0)
                    {
                        canSerializeClass = true;
                    }

                    if (prop.GetValue(obj, null) == null)
                    {
                        writer.WriteStartElement(prop.Name);
                        writer.WriteAttributeString("IsNull", "True");
                        writer.WriteEndElement();
                    }
                    else if ((prop.PropertyType.IsClass) && (canSerializeClass))
                    {
                        object theClass = prop.GetValue(obj, null);
                        theClass.GetType().GetMethod("ToXml").Invoke(theClass, new object[] { writer });
                    }
                    else
                    {
                        if (prop.PropertyType == typeof(string))
                        {
                            writer.WriteStartElement(prop.Name);
							string propValue = prop.GetValue(obj, null).ToString();
                            if (propValue.IndexOf(' ') >= 0)
                            {
                                writer.WriteAttributeString("xml", "space", null, "preserve");
                            }
                            writer.WriteString(propValue);
                            writer.WriteEndElement();
                        }
                        // We must use InvariantCulture for floats and doubles, because their
                        // format depends on local system settings used when the project was saved
                        else if (prop.PropertyType == typeof(float))
                        {
                            writer.WriteElementString(prop.Name, ((float)prop.GetValue(obj, null)).ToString(CultureInfo.InvariantCulture));
                        }
                        else if (prop.PropertyType == typeof(double))
                        {
                            writer.WriteElementString(prop.Name, ((double)prop.GetValue(obj, null)).ToString(CultureInfo.InvariantCulture));
                        }
                        else if (prop.PropertyType == typeof(DateTime))
                        {
                            writer.WriteElementString(prop.Name, ((DateTime)prop.GetValue(obj, null)).ToString("yyyy-MM-dd"));
                        }
                        // For compatibility with various Custom Resolution beta builds
                        // TODO: find a generic solution for doing a conversions like this without
                        // using hard-coded property name (some serialization attribute perhaps)
                        else if (prop.PropertyType == typeof(Size) && prop.Name == "CustomResolution")
                        {
                            writer.WriteElementString(prop.Name, ResolutionToCompatString((Size)prop.GetValue(obj, null)));
                        }
                        else
                        {
                            writer.WriteElementString(prop.Name, prop.GetValue(obj, null).ToString());
                        }
                    }
                }
            }
            if (writeEndElement)
            {
                writer.WriteEndElement();
            }
        }

        public static void DeserializeFromXML(object obj, XmlNode node)
        {
            XmlNode mainNode = node;

            if (node.Name != obj.GetType().Name)
            {
                mainNode = node.SelectSingleNode(obj.GetType().Name);
                if (mainNode == null)
                {
                    throw new InvalidDataException("Node not found in XML: " + obj.GetType().Name);
                }
            }

            DeserializeIgnoreAttribute[] ignoreAttributes =
                (DeserializeIgnoreAttribute[])obj.GetType().GetCustomAttributes(typeof(DeserializeIgnoreAttribute), true);

            foreach (XmlNode child in mainNode.ChildNodes)
            {
                string elementValue = child.InnerText;
                PropertyInfo prop = obj.GetType().GetProperty(child.Name);
                if (prop == null)
                {
                    if (ignoreAttributes.Length == 0 ||
                        !Array.Exists(ignoreAttributes, DeserializeIgnoreAttribute.MatchesPropertyName(child.Name)))
                    {
                        throw new InvalidDataException("The property '" + child.Name + "' could not be read. This game may require a newer version of AGS.");
                    }
                    continue;
                }

                // Process any existing value conversions; this helps to upgrade game from older version
                DeserializeConvertValueAttribute[] conversions =
                    (DeserializeConvertValueAttribute[])prop.PropertyType.GetCustomAttributes(typeof(DeserializeConvertValueAttribute), true);
                if (conversions.Length > 0)
                {
                    foreach (DeserializeConvertValueAttribute conversion in conversions)
                    {
                        elementValue = conversion.Convert(elementValue);
                    }
                }
                
                if (!prop.CanWrite)
                {
                    // do nothing, read-only
                }
                else if (child.Attributes.GetNamedItem("IsNull") != null)
                {
                    prop.SetValue(obj, null, null);
                }
                else if (prop.PropertyType == typeof(Boolean))
                {
                    prop.SetValue(obj, Convert.ToBoolean(elementValue), null);
                }
                else if (prop.PropertyType == typeof(int))
                {
                    prop.SetValue(obj, Convert.ToInt32(elementValue), null);
                }
                else if (prop.PropertyType == typeof(short))
                {
                    prop.SetValue(obj, Convert.ToInt16(elementValue), null);
                }
                // We must use InvariantCulture for floats and doubles, because their
                // format depends on local system settings used when the project was saved
                else if (prop.PropertyType == typeof(float))
                {
                    prop.SetValue(obj, Single.Parse(elementValue, CultureInfo.InvariantCulture), null);
                }
                else if (prop.PropertyType == typeof(double))
                {
                    prop.SetValue(obj, Double.Parse(elementValue, CultureInfo.InvariantCulture), null);
                }
                else if (prop.PropertyType == typeof(string))
                {
                    prop.SetValue(obj, elementValue, null);
                }
				else if (prop.PropertyType == typeof(DateTime))
				{
                    // Must use CultureInfo.InvariantCulture otherwise DateTime.Parse
                    // crashes if the system regional settings short date format has
                    // spaces in it (.NET bug)
                    DateTime dateTime = DateTime.MinValue;
                    if(DateTime.TryParseExact(elementValue, "u", CultureInfo.InvariantCulture, DateTimeStyles.None, out dateTime))
                    {
                        // Get and set audio files time stamps
                        prop.SetValue(obj, dateTime, null);
                    }
                    else
                    {
                        // Release Date timestamp doesn't store time of the day, and as such it ends up in the else-statement
                        prop.SetValue(obj, DateTime.Parse(elementValue, CultureInfo.InvariantCulture), null);
                    }					                    
				}
                else if (prop.PropertyType.IsEnum)
                {
                    prop.SetValue(obj, Enum.Parse(prop.PropertyType, elementValue), null);
                }
                else if (prop.PropertyType.IsClass)
                {
                    ConstructorInfo constructor = prop.PropertyType.GetConstructor(new Type[] { typeof(XmlNode) });
                    prop.SetValue(obj, constructor.Invoke(new object[] { child }), null);
                }
                // For compatibility with various Custom Resolution beta builds
                // TODO: find a generic solution for doing a conversions like this without
                // using hard-coded property name (some serialization attribute perhaps)
                else if (prop.PropertyType == typeof(Size) && prop.Name == "CustomResolution")
                {
                    prop.SetValue(obj, CompatStringToResolution(elementValue), null);
                }
                else
                {
                    throw new InvalidDataException("Unknown data type: " + prop.PropertyType.Name);
                }
            }
        }

        public static string GetAttributeString(XmlNode node, string attrName)
        {
            if (node.Attributes[attrName] == null)
            {
                throw new InvalidDataException("Missing attribute: " + attrName);
            }
            return node.Attributes[attrName].InnerText;
        }

        public static int GetAttributeInt(XmlNode node, string attrName)
        {
            return Convert.ToInt32(GetAttributeString(node, attrName));
        }

        public static Size CompatStringToResolution(String s)
        {
            String[] parts = s.Split(',');
            return new Size(Int32.Parse(parts[0]), Int32.Parse(parts[1]));
        }

        public static String ResolutionToCompatString(Size size)
        {
            return String.Format("{0},{1}", size.Width, size.Height);
        }
    }
}
