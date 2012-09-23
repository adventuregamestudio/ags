package com.bigbluecup.android;

import android.content.SharedPreferences.Editor;
import android.content.res.Configuration;
import android.os.Bundle;
import android.preference.CheckBoxPreference;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceGroup;
import android.preference.PreferenceManager;

public class PreferencesActivity extends PreferenceActivity
{
	private String gameName;
	private String gameFilename;
	private String baseDirectory;
	private boolean isGlobalConfig;
	private String translations[];
	private int translationCount;
	
	static final int CONFIG_ENABLED = 14;
	static final int CONFIG_TRANSLATION = 17;

	private native boolean readConfigFile(String directory);
	private native boolean writeConfigFile();
	
	public static native int readIntConfigValue(int id);
	private native void setIntConfigValue(int id, int value);

	public static native String readStringConfigValue(int id);
	private native void setStringConfigValue(int id, String value);	

	private native int getAvailableTranslations(String translations[]);


	@Override
	protected void onCreate(Bundle bundle)
	{
		super.onCreate(bundle);
		
		System.loadLibrary("agsengine");
		
		gameName = getIntent().getExtras().getString("name");
		gameFilename = getIntent().getExtras().getString("filename");
		baseDirectory = getIntent().getExtras().getString("directory");
		
		isGlobalConfig = (gameName.length() == 0);
		
		addPreferencesFromResource(R.xml.preferences);

		if (isGlobalConfig)
		{
			setTitle("Global preferences");
			configureForGlobalPreferences();
		}
		else
		{
			setTitle(gameName);
		}
		
		boolean hasCustomConfig = readConfigFile(baseDirectory + '/' + gameName);

		if (!isGlobalConfig)
		{
			// Get available translations from the engine
			String tempTranslations[] = new String[100];
			translationCount = getAvailableTranslations(tempTranslations);
			translations = new String[translationCount + 1];
			for (int i = 0; i < translationCount; i++)
				translations[i + 1] = tempTranslations[i];
			translations[0] = "Default";
			
			// Populate the translation preference
			ListPreference listPref = (android.preference.ListPreference) getPreferenceScreen().findPreference(Integer.toString(CONFIG_TRANSLATION));
			listPref.setEntries(translations);
			listPref.setEntryValues(translations);
		}

		// Load preferences
		getPreferenceScreen().setPersistent(false);
		loadPreferencesRecursively(getPreferenceScreen());
	}
	
	
	private void configureForGlobalPreferences()
	{
		// Remove the dependence on the "Use custom settings" option 
		setPreferenceDependencyRecursively(getPreferenceScreen(), null);
		
		// Remove "Use custom settings"
		CheckBoxPreference enabledPreference = (android.preference.CheckBoxPreference) findPreference(Integer.toString(CONFIG_ENABLED));
		getPreferenceScreen().removePreference(enabledPreference);		

		// Remove translation setting
		ListPreference translationPreference = (android.preference.ListPreference) findPreference(Integer.toString(CONFIG_TRANSLATION));
		PreferenceGroup generalGroup = (android.preference.PreferenceGroup) findPreference("preference_key_general");
		generalGroup.removePreference(translationPreference);
	}
	
	// Set the dependency of all preference nodes to the give value
	private void setPreferenceDependencyRecursively(PreferenceGroup root, String value)
	{
		for (int i = 0; i < root.getPreferenceCount(); i++)
		{
			Preference preference = root.getPreference(i);
			
			if (preference instanceof android.preference.PreferenceCategory)
			{
				setPreferenceDependencyRecursively((PreferenceGroup) preference, value);
			}
			else
			{
				preference.setDependency(value);
			}
		}
	}
	
	// Load the preferences from the engine into the preference activity
	private void loadPreferencesRecursively(PreferenceGroup root)
	{
		Editor editor = PreferenceManager.getDefaultSharedPreferences(getBaseContext()).edit();
		
		for (int i = 0; i < root.getPreferenceCount(); i++)
		{
			Preference preference = root.getPreference(i);
			
			if (preference instanceof android.preference.CheckBoxPreference)
			{
				String key = preference.getKey();
				int key_value = Integer.valueOf(key);
				int value = PreferencesActivity.readIntConfigValue(key_value);
				
				editor.putBoolean(preference.getKey(), (value != 0));
			}
			else if (preference instanceof android.preference.ListPreference)
			{
				String key = preference.getKey();
				int key_value = Integer.valueOf(key);
				
				if (key_value == CONFIG_TRANSLATION)
				{
					String value = PreferencesActivity.readStringConfigValue(key_value);
					
					if (value.equals("default"))
						editor.putString(preference.getKey(), "Default");
					else
						editor.putString(preference.getKey(), value);
				}
				else
				{
					int value = PreferencesActivity.readIntConfigValue(key_value);
					editor.putString(preference.getKey(), Integer.toString(value));
				}
			}
			else if (preference instanceof android.preference.PreferenceCategory)
			{
				loadPreferencesRecursively((PreferenceGroup) preference);
			}
		}
		
		editor.commit();
	}
	
	// Save the preferences
	private void savePreferencesRecursively(PreferenceGroup root)
	{
		for (int i = 0; i < root.getPreferenceCount(); i++)
		{
			Preference preference = root.getPreference(i);
			
			if (preference instanceof android.preference.CheckBoxPreference)
			{
				String key = preference.getKey();
				int key_value = Integer.valueOf(key);
				int value = (((android.preference.CheckBoxPreference) preference).isChecked() ? 1 : 0);
				
				setIntConfigValue(key_value, value);
			}
			else if (preference instanceof android.preference.ListPreference)
			{
				String key = preference.getKey();
				int key_value = Integer.valueOf(key);
				String value_string = ((android.preference.ListPreference) preference).getValue();
				
				if (key_value == CONFIG_TRANSLATION)
				{
					if (value_string.equals("Default"))
						setStringConfigValue(key_value, "default");
					else
						setStringConfigValue(key_value, value_string);
				}
				else
				{
					int value = Integer.parseInt(value_string);
					setIntConfigValue(key_value, value);
				}
			}
			else if (preference instanceof android.preference.PreferenceCategory)
			{
				savePreferencesRecursively((PreferenceGroup) preference);
			}
		}
	}
	
	@Override
	public void onDestroy()
	{
		savePreferencesRecursively(getPreferenceScreen());
		
		writeConfigFile();
		
		super.onDestroy();
	}
	
	// Prevent the activity from being destroyed on a configuration change
	@Override
	public void onConfigurationChanged(Configuration newConfig)
	{
		super.onConfigurationChanged(newConfig);
	}
}
