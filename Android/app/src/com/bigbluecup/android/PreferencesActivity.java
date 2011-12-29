package com.bigbluecup.android;

import java.util.ArrayList;
import java.util.EnumSet;
import android.app.ListActivity;
import android.content.res.Configuration;
import android.os.Bundle;
import android.view.ContextMenu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.widget.ListView;

public class PreferencesActivity extends ListActivity
{
	private PreferencesEntry.Flags activeMenu;
	private String gameName;
	private String gameFilename;
	private String baseDirectory;
	private boolean isGlobalConfig;
	
	private ArrayList<PreferencesEntry> values;
	
	static final int CONFIG_NONE = -1;
	static final int CONFIG_IGNORE_ACSETUP = 0;
	static final int CONFIG_CLEAR_CACHE = 1;
	static final int CONFIG_AUDIO_RATE = 2;
	static final int CONFIG_AUDIO_ENABLED = 3;
	static final int CONFIG_AUDIO_THREADED = 4;
	static final int CONFIG_AUDIO_CACHESIZE = 5;
	static final int CONFIG_MIDI_ENABLED = 6;
	static final int CONFIG_MIDI_PRELOAD = 7;
	static final int CONFIG_VIDEO_FRAMEDROP = 8;
	static final int CONFIG_GFX_RENDERER = 9;
	static final int CONFIG_GFX_SMOOTHING = 10;
	static final int CONFIG_GFX_SCALING = 11;
	static final int CONFIG_GFX_SS = 12;
	static final int CONFIG_ROTATION = 13;
	static final int CONFIG_ENABLED = 14;
	static final int CONFIG_DEBUG_FPS = 15;
	
	public static native int readIntConfigValue(int id);
	private native boolean readConfigFile(String directory);
	private native void setIntConfigValue(int id, int value);
	private native boolean writeConfigFile();
	
	public void onCreate(Bundle bundle)
	{
		super.onCreate(bundle);
		
		System.loadLibrary("agsengine");
		
		gameName = getIntent().getExtras().getString("name");
		gameFilename = getIntent().getExtras().getString("filename");
		baseDirectory = getIntent().getExtras().getString("directory");
		
		isGlobalConfig = (gameName.length() == 0);
		
		if (isGlobalConfig)
			setTitle("General preferences");
		else
			setTitle(gameName);
		
		boolean hasCustomConfig = readConfigFile(baseDirectory + '/' + gameName);

		values = new ArrayList<PreferencesEntry>();
		
		if (!isGlobalConfig)
		{
			values.add(new PreferencesEntry(
					"Use custom preferences", 
					"Check to override the global preferences for this game",
					CONFIG_ENABLED,
					EnumSet.of(PreferencesEntry.Flags.CHECKABLE, PreferencesEntry.Flags.ENABLED)
					));
		}

		values.add(new PreferencesEntry(
				"General",
				"",
				CONFIG_NONE,
				EnumSet.of(PreferencesEntry.Flags.HEADER)
				));
		values.add(new PreferencesEntry(
				"Lock screen orientation",
				"Prevents the screen from automatically rotating",
				CONFIG_ROTATION,
				EnumSet.of(PreferencesEntry.Flags.MENU_ORIENTATION)
				));	
		values.add(new PreferencesEntry(
				"Ignore acsetup.cfg",
				"Skip evaluating the standard AGS settings file",
				CONFIG_IGNORE_ACSETUP,
				EnumSet.of(PreferencesEntry.Flags.CHECKABLE)
				));
		
		values.add(new PreferencesEntry(
				"Sound", 
				"", 
				CONFIG_NONE,
				EnumSet.of(PreferencesEntry.Flags.HEADER)
				));	
		values.add(new PreferencesEntry(
				"Enabled",
				"",
				CONFIG_AUDIO_ENABLED,
				EnumSet.of(PreferencesEntry.Flags.CHECKABLE)
				));	
		values.add(new PreferencesEntry(
				"Use multithreading", 
				"Reduces stuttering but throws off lipsyncing",
				CONFIG_AUDIO_THREADED,
				EnumSet.of(PreferencesEntry.Flags.CHECKABLE)
				));	

		values.add(new PreferencesEntry(
				"Midi", 
				"",
				CONFIG_NONE,
				EnumSet.of(PreferencesEntry.Flags.HEADER)
				));	
		values.add(new PreferencesEntry(
				"Enabled", 
				"Needs MIDI patches on the SD card", 
				CONFIG_MIDI_ENABLED,
				EnumSet.of(PreferencesEntry.Flags.CHECKABLE)
				));	
		values.add(new PreferencesEntry(
				"Preload patches",
				"Less delay between MIDI tracks but causes a startup delay",
				CONFIG_MIDI_PRELOAD,
				EnumSet.of(PreferencesEntry.Flags.CHECKABLE)
				));	

		values.add(new PreferencesEntry(
				"Video",
				"", 
				CONFIG_NONE,
				EnumSet.of(PreferencesEntry.Flags.HEADER)
				));	
		values.add(new PreferencesEntry(
				"Drop frames if necessary",
				"On slow devices this can lead to all frames being skipped",
				CONFIG_VIDEO_FRAMEDROP,
				EnumSet.of(PreferencesEntry.Flags.CHECKABLE)
				));	
		values.add(new PreferencesEntry(
				"Graphics", 
				"", 
				CONFIG_NONE,
				EnumSet.of(PreferencesEntry.Flags.HEADER)
				));	
		values.add(new PreferencesEntry(
				"Select Renderer",
				"Choose between software and hardware rendering", 
				CONFIG_GFX_RENDERER,
				EnumSet.of(PreferencesEntry.Flags.MENU_RENDERER)
				));
		values.add(new PreferencesEntry(
				"Stretch to screen", 
				"",
				CONFIG_GFX_SCALING, 
				EnumSet.of(PreferencesEntry.Flags.CHECKABLE)
				));	
		values.add(new PreferencesEntry(
				"Linear filtering",
				"", 
				CONFIG_GFX_SMOOTHING, 
				EnumSet.of(PreferencesEntry.Flags.CHECKABLE)
				));	
		values.add(new PreferencesEntry(
				"Supersampling", 
				"Use a higher resolution for scaling objects", 
				CONFIG_GFX_SS, 
				EnumSet.of(PreferencesEntry.Flags.CHECKABLE)
				));

		values.add(new PreferencesEntry(
				"Debug", 
				"", 
				CONFIG_NONE,
				EnumSet.of(PreferencesEntry.Flags.HEADER)
				));	
		values.add(new PreferencesEntry(
				"Show framerate",
				"", 
				CONFIG_DEBUG_FPS,
				EnumSet.of(PreferencesEntry.Flags.CHECKABLE)
				));
		
		if (isGlobalConfig || (getValueForId(CONFIG_ENABLED) == 1))
		{
			for (int i = (isGlobalConfig ? 0 : 1); i < values.size(); i++)
				values.get(i).flags.add(PreferencesEntry.Flags.ENABLED);
		}
		
		PreferencesArrayAdapter adapter = new PreferencesArrayAdapter(this, values);
		setListAdapter(adapter);
	}
	
	
	@Override
	public void onDestroy()
	{
		for (int i = 0; i < values.size(); i++)
		{
			if (values.get(i).id != CONFIG_NONE)
				setIntConfigValue(values.get(i).id, values.get(i).value);
		}
		
		writeConfigFile();
		
		super.onDestroy();
	}
	
	private void setValueForId(int id, int value)
	{
		for (int i = 0; i < values.size(); i++)
		{
			if (values.get(i).id == id)
			{
				values.get(i).value = value;
				return;
			}
		}
	}
	
	private int getValueForId(int id)
	{
		for (int i = 0; i < values.size(); i++)
		{
			if (values.get(i).id == id)
			{
				return values.get(i).value;
			}
		}
		return 0;
	}	
	
	@Override
	public boolean onContextItemSelected(MenuItem item)
	{
		item.setChecked(true);

		switch (item.getItemId())
		{
			// Renderer menu
			case R.id.software:
				setValueForId(CONFIG_GFX_RENDERER, 0);
				return true;
			case R.id.hardware:
				setValueForId(CONFIG_GFX_RENDERER, 1);
				return true;
			case R.id.rtt:
				setValueForId(CONFIG_GFX_RENDERER, 2);
				return true;

			// Rotation menu
			case R.id.auto:
				setValueForId(CONFIG_ROTATION, 0);
				return true;
			case R.id.portrait:
				setValueForId(CONFIG_ROTATION, 1);
				return true;
			case R.id.landscape:
				setValueForId(CONFIG_ROTATION, 2);
				return true;
				
			default:
				return super.onOptionsItemSelected(item);
		}
	}	
	

	@Override
	public void onCreateContextMenu(ContextMenu menu, View v,ContextMenu.ContextMenuInfo menuInfo)
	{
		MenuInflater inflater = getMenuInflater();

		if (activeMenu == PreferencesEntry.Flags.MENU_ORIENTATION)
		{
			inflater.inflate(R.menu.preference_orientation, menu);
			menu.setHeaderTitle("Lock screen orientation");
			menu.getItem(getValueForId(CONFIG_ROTATION)).setChecked(true);
		}
		else if (activeMenu == PreferencesEntry.Flags.MENU_RENDERER)
		{
			inflater.inflate(R.menu.preference_renderer, menu);
			menu.setHeaderTitle("Select renderer");
			menu.getItem(getValueForId(CONFIG_GFX_RENDERER)).setChecked(true);
		}
	}	
	
	
	@Override
	protected void onListItemClick(ListView l, View v, int position, long id)
	{
		super.onListItemClick(l, v, position, id);
		
		if (!values.get(position).flags.contains(PreferencesEntry.Flags.ENABLED))
		{
			return;
		}	
		else if (values.get(position).flags.contains(PreferencesEntry.Flags.MENU_ORIENTATION))
		{
			activeMenu = PreferencesEntry.Flags.MENU_ORIENTATION;
			registerForContextMenu(v);
			openContextMenu(v);
			unregisterForContextMenu(v);
		}
		else if (values.get(position).flags.contains(PreferencesEntry.Flags.MENU_RENDERER))
		{
			activeMenu = PreferencesEntry.Flags.MENU_RENDERER;
			registerForContextMenu(v);
			openContextMenu(v);
			unregisterForContextMenu(v);
		}
		else if (values.get(position).flags.contains(PreferencesEntry.Flags.CHECKABLE))
		{
			values.get(position).value = (values.get(position).value == 1) ? 0 : 1;

			if (position == 0)
			{
				for (int i = 1; i < values.size(); i++)
				{
					if (values.get(0).value == 1)
						values.get(i).flags.add(PreferencesEntry.Flags.ENABLED);
					else
						values.get(i).flags.remove(PreferencesEntry.Flags.ENABLED);						
				}
			}
			
			PreferencesArrayAdapter adapter = (PreferencesArrayAdapter)getListAdapter();
			adapter.notifyDataSetChanged();
		}
	}
	
	// Prevent the activity from being destroyed on a configuration change
	@Override
	public void onConfigurationChanged(Configuration newConfig)
	{
		super.onConfigurationChanged(newConfig);
	}	
}
