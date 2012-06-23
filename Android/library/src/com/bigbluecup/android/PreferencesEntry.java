package com.bigbluecup.android;

import java.util.EnumSet;

public class PreferencesEntry
{
	public enum Flags
	{
		HEADER, CHECKABLE, STRING, MENU_RENDERER, MENU_ORIENTATION, MENU_TRANSLATION, MENU_SCALING, ENABLED
	}
	
	public String caption;
	public String description;
	public EnumSet<Flags> flags;
	public int id;
	public int value;
	public String stringValue;
	
	public PreferencesEntry(String caption, String description, int id, EnumSet<Flags> flags)
	{
		this.caption = caption;
		this.description = description;
		this.flags = flags;
		this.id = id;
		
		if (id > -1)
		{
			if (flags.contains(PreferencesEntry.Flags.STRING))
				this.stringValue = PreferencesActivity.readStringConfigValue(id);
			else
				this.value = PreferencesActivity.readIntConfigValue(id);
		}
	}
}
