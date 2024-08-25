using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System.IO;

namespace AGS.Editor
{
    /// <summary>
    /// Implementation of IObjectConfig, that stores data as JSON object
    /// in memory, and lets serialize it as a json file.
    /// 
    /// TODO: consider sharing code this with ColorThemeJson somehow?
    /// </summary>
    public class ObjectConfigJson : IObjectConfig
    {
        private JToken _root;

        public ObjectConfigJson()
        {
            _root = JObject.Parse("{}");
        }

        public ObjectConfigJson(JToken root)
        {
            _root = root;
        }

        public IObjectConfig GetObject(string id)
        {
            JToken jtoken = GetJToken(_root, id);
            return new ObjectConfigJson(jtoken);
        }

        public IObjectConfig GetOrAddObject(string id)
        {
            JToken jtoken = GetJToken(_root, id);
            if (jtoken == null)
                jtoken = SetJToken(_root, id, new JObject());
            return new ObjectConfigJson(jtoken);
        }

        public int GetInt(string id, int defValue)
        {
            JToken token = GetJToken(_root, id);
            if (token == null || token.Type != JTokenType.Integer)
                return defValue;
            return (int)token;
        }

        public void SetInt(string id, int value)
        {
            SetJToken(_root, id, value);
        }

        private static JToken GetJToken(JToken root, string id)
        {
            string[] tokens = id.Replace('.', '/').Split('/');
            JToken token = root;
            tokens.ToList().ForEach( t => {
                if (token == null)
                    return;
                token = token[t];
            });
            return token;
        }

        private static JToken SetJToken(JToken root, string id, JToken value)
        {
            string[] tokens = id.Replace('.', '/').Split('/');
            JToken token = root;
            // For all parts except last: make sure that there are nested containers present
            tokens.Take(tokens.Length - 1).ToList().ForEach(t => {
                JToken next_token = token[t];
                if (next_token == null)
                {
                    JObject jobj = new JObject();
                    token[t] = jobj;
                    token = jobj;
                }
                else
                {
                    token = next_token;
                }
            });
            // For the final part: assign a new property value
            token[tokens.Last()] = value;
            return value;
        }

        /// <summary>
        /// Loads this config from the file, overwriting all contents.
        /// </summary>
        public bool LoadFromFile(string filepath)
        {
            bool result = false;
            string data = "{}";
            try
            {
                data = File.ReadAllText(filepath);
                result = true;
            }
            catch (Exception)
            {
            }
            finally
            {
                _root = JObject.Parse(data);
            }
            return result;
        }

        /// <summary>
        /// Saves this config to the file.
        /// </summary>
        public void SaveToFile(string filepath)
        {
            using (StreamWriter file = File.CreateText(filepath))
            using (JsonTextWriter writer = new JsonTextWriter(file))
            {
                writer.Formatting = Formatting.Indented;
                _root.WriteTo(writer);
            }
        }
    }
}
