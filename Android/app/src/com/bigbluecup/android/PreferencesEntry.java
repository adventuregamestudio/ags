package com.bigbluecup.android;

import java.util.EnumSet;

public class PreferencesEntry
{
	public enum Flags
	{
		HEADER, CHECKABLE, MENU_RENDERER, MENU_ORIENTATION, ENABLED
	}
	
	public String caption;
	public String description;
	public EnumSet<Flags> flags;
	public int id;
	public int value;
		
	public PreferencesEntry(String caption, String description, int id, EnumSet<Flags> flags)
	{
		this.caption = caption;
		this.description = description;
		this.flags = flags;
		this.id = id;
		
		if (id > -1)
			this.value = PreferencesActivity.readIntConfigValue(id);
	}	
}
