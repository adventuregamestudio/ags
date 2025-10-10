using AGS.Types;
using System;
using System.Collections.Generic;
using System.Text;
using System.Xml;

namespace AGS.Editor
{
	public class ComponentController
	{
		private List<IEditorComponent> _components;
        private List<string> _supressedComponents;

		private static ComponentController _instance;

		public static ComponentController Instance
		{
			get
			{
				if (_instance == null)
				{
					_instance = new ComponentController();
				}
				return _instance;
			}
		}

		private ComponentController()
		{
			_components = new List<IEditorComponent>();
            _supressedComponents = new List<string>();
        }

		public void AddComponent(IEditorComponent component)
		{
			_components.Add(component);
		}

        public void AddSuppressedComponent(string componentName)
        {
            _supressedComponents.Add(componentName);
        }

        public IList<IEditorComponent> Components
        {
            get
            {
                return _components;
            }
        }

		public void ShutdownComponents()
		{
			foreach (IEditorComponent component in _components)
			{
                try
                {
                    component.EditorShutdown();
                }
                catch(Exception e)
                {
                    // TODO: write into the Editor's log or something?
                    GUIController.Instance.ShowError(
                        string.Format($"Editor component {component.ComponentID} was shut down with an exception.",
                        e, MessageBoxIconType.Warning));
                }
			}
		}

		public void NotifyGameSettingsChanged()
		{
			foreach (IEditorComponent component in _components)
			{
				component.GameSettingsChanged();
			}
		}

		public void NotifyImportedOldGame()
		{
			// Ensure that all components are informed that a new game has
			// been loaded
			foreach (IEditorComponent component in _components)
			{
				component.FromXml(null);
			}
		}

        public void NotifyLoadedUserData(XmlNode rootNode)
        {
            Dictionary<string, XmlNode> componentNodes = new Dictionary<string, XmlNode>();

            if (rootNode != null)
            {
                foreach (XmlNode componentNode in rootNode.SelectNodes("Component"))
                {
                    string componentID = componentNode.Attributes["Name"].InnerXml;
                    componentNodes.Add(componentID, componentNode);
                }
            }

            foreach (IEditorComponent component in _components)
            {
                if (component is IPersistUserData)
                {
                    if (componentNodes.ContainsKey(component.ComponentID))
                    {
                        ((IPersistUserData)component).DeSerialize(componentNodes[component.ComponentID]);
                        componentNodes.Remove(component.ComponentID);
                    }
                    else
                    {
                        ((IPersistUserData)component).DeSerialize(null);
                    }
                }
            }
        }

		public List<string> NotifyLoadedGameAndReturnMissingComponents(XmlNode rootNode)
		{
			Dictionary<string, XmlNode> componentNodes = new Dictionary<string, XmlNode>();

			foreach (XmlNode componentNode in rootNode.SelectNodes("Component"))
			{
				string componentID = componentNode.Attributes["Name"].InnerXml;
				componentNodes.Add(componentID, componentNode);
			}

			foreach (IEditorComponent component in _components)
			{
				if (componentNodes.ContainsKey(component.ComponentID))
				{
					component.FromXml(componentNodes[component.ComponentID]);
					componentNodes.Remove(component.ComponentID);
				}
				else
				{
					component.FromXml(null);
				}
			}

			List<string> missingComponents = new List<string>();
			foreach (string componentID in componentNodes.Keys)
			{
				if (componentNodes[componentID].HasChildNodes)
				{
                    if (!_supressedComponents.Contains(componentID))
					    missingComponents.Add(componentID);
				}
			}
			return missingComponents;
		}

		public void NotifyDataRefreshNeeded()
		{
			foreach (IEditorComponent component in _components)
			{
				component.RefreshDataFromGame();
			}
		}

		public void NotifyAboutToSaveGame()
		{
			foreach (IEditorComponent component in _components)
			{
				component.BeforeSaveGame();
			}
		}

		public void NotifySavingGame(XmlTextWriter writer)
		{
			foreach (IEditorComponent component in _components)
			{
				writer.WriteStartElement("Component");
				writer.WriteAttributeString("Name", component.ComponentID);

				component.ToXml(writer);

				writer.WriteEndElement();
			}
		}

        public void NotifySavingUserData(XmlTextWriter writer)
        {
            foreach (IEditorComponent component in _components)
            {
                if (component is IPersistUserData)
                {
                    writer.WriteStartElement("Component");
                    writer.WriteAttributeString("Name", component.ComponentID);

                    ((IPersistUserData)component).Serialize(writer);

                    writer.WriteEndElement();
                }
            }
        }

		public IEditorComponent FindComponentThatManageScriptElement(string scriptType)
		{
			foreach (IEditorComponent component in _components)
			{
				if (component is Components.BaseComponent)
				{
					Components.BaseComponent bComponent = component as Components.BaseComponent;
					if (bComponent.GetManagedScriptElements().Contains(scriptType)) return bComponent;
				}
			}
			return null;
		}

		public IEditorComponent FindComponentThatImplementsInterface(Type interfaceType)
		{
			foreach (IEditorComponent component in _components)
			{
				if (component.GetType().GetInterface(interfaceType.FullName) != null)
				{
					return component;
				}
			}
            // TODO: should we really throw here? Perhaps better throw in the caller's code where applicable?
			throw new AGSEditorException("No component found that implements " + interfaceType.Name);
		}

        /// <summary>
        /// Searches for the Editor's component of the given type and returns its instance,
        /// or null if one does not exist.
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <param name="componentType"></param>
        /// <returns></returns>
        public T FindComponent<T>()
            where T : class, IEditorComponent
        {
            foreach (IEditorComponent component in _components)
            {
                if (component is T)
                    return component as T;
            }
            return null;
        }
	}
}
