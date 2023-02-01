package uk.co.adventuregamestudio.runtime;

import android.content.SharedPreferences;
import android.content.res.Configuration;
import android.os.Bundle;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.preference.CheckBoxPreference;
import androidx.preference.ListPreference;
import androidx.preference.Preference;
import androidx.preference.PreferenceCategory;
import androidx.preference.PreferenceGroup;
import androidx.preference.SeekBarPreference;

import androidx.preference.PreferenceScreen;

import java.util.Locale;


public class PreferencesActivity extends AppCompatActivity
{
    private boolean once = false;

    public String gameName;
    public String gameFilename;
    public String gamePath;
    public String baseDirectory;
    public boolean isGlobalConfig;
    public String translations[];
    public int translationCount;

    public static final int CONFIG_ENABLED = 14;
    public static final int CONFIG_TRANSLATION = 17;
    public static final int CONFIG_MOUSE_SPEED = 21;

    public native boolean readConfigFile(String directory);
    public native boolean writeConfigFile(String directory);

    public static native int readIntConfigValue(int id);
    public native void setIntConfigValue(int id, int value);

    public static native String readStringConfigValue(int id);
    public native void setStringConfigValue(int id, String value);

    public native int getAvailableTranslations(String translations[]);

    AgsSettingsFragment frag;

    @SuppressWarnings("unused")
    @Override
    public void onCreate(@Nullable Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.settings_activity);

        frag = new AgsSettingsFragment();

        getSupportFragmentManager()
                .beginTransaction()
                .replace(R.id.settings_container, frag)
                .commit();

        System.loadLibrary("engine");

        Bundle extras = getIntent().getExtras();
        gameName = extras.getString("name");
        gameFilename = extras.getString("filename");
        baseDirectory = extras.getString("directory");

        isGlobalConfig = (gameName.length() == 0);
    }

    @Override
    protected void onResume()
    {
        super.onResume();
        Preference.OnPreferenceChangeListener onMouseSpeedSeekBarChanged = (preference, newValue) -> {
            final int value = (int) newValue;
            preference.setSummary(String.format(Locale.US, "Sensibility: %.1f", ((float)value)/10.0));
            return true;
        };
        SeekBarPreference mouseSpeedSeekBar = frag.findPreference(Integer.toString(CONFIG_MOUSE_SPEED));
        if (mouseSpeedSeekBar != null) {
            if(!once) {
                once = true;
                mouseSpeedSeekBar.setOnPreferenceChangeListener(onMouseSpeedSeekBarChanged);
            }
            onMouseSpeedSeekBarChanged.onPreferenceChange(mouseSpeedSeekBar, mouseSpeedSeekBar.getValue());
        }
    }


    public void setTitle(String global_preferences) {
    }


    public void configureForGlobalPreferences()
    {
        // Remove the dependence on the "Use custom settings" option
        PreferenceScreen prefScreen = frag.getPreferenceScreen();
        setPreferenceDependencyRecursively(frag.getPreferenceScreen(), null);

        // Remove "Use custom settings"
        CheckBoxPreference enabledPreference = (CheckBoxPreference) frag.findPreference(Integer.toString(CONFIG_ENABLED));
        frag.getPreferenceScreen().removePreference(enabledPreference);

        // Remove translation setting
        ListPreference translationPreference = (ListPreference) frag.findPreference(Integer.toString(CONFIG_TRANSLATION));
        PreferenceCategory generalGroup = (PreferenceCategory) frag.findPreference("preference_key_general");
        generalGroup.removePreference(translationPreference);
    }

    // Set the dependency of all preference nodes to the give value
    public void setPreferenceDependencyRecursively(PreferenceGroup root, String value)
    {
        for (int i = 0; i < root.getPreferenceCount(); i++)
        {
            Preference preference = root.getPreference(i);

            if (preference instanceof PreferenceGroup)
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
    public void loadPreferencesRecursively(PreferenceGroup root, SharedPreferences.Editor editor)
    {
        for (int i = 0; i < root.getPreferenceCount(); i++)
        {
            Preference preference = root.getPreference(i);

            if (preference instanceof CheckBoxPreference)
            {
                String key = preference.getKey();
                int key_value = Integer.valueOf(key);
                int value = readIntConfigValue(key_value);

                editor.putBoolean(preference.getKey(), (value != 0));
                CheckBoxPreference checkBox = (CheckBoxPreference) frag.findPreference(preference.getKey());
                checkBox.setChecked(value != 0);
            }
            else if (preference instanceof SeekBarPreference)
            {
                String key = preference.getKey();
                int key_value = Integer.valueOf(key);
                int value = readIntConfigValue(key_value);

                editor.putInt(preference.getKey(), value);
                SeekBarPreference seekbar = (SeekBarPreference) frag.findPreference(preference.getKey());
                seekbar.setValue(value);
            }
            else if (preference instanceof ListPreference)
            {
                String key = preference.getKey();
                int key_value = Integer.valueOf(key);

                if (key_value == CONFIG_TRANSLATION)
                {
                    String value = readStringConfigValue(key_value);
                    ListPreference list = (ListPreference) frag.findPreference(preference.getKey());

                    if (value.equals("default"))
                    {
                        editor.putString(preference.getKey(), "Default");
                        list.setValue("Default");
                    }
                    else
                    {
                        editor.putString(preference.getKey(), value);
                        list.setValue(value);
                    }
                }
                else
                {
                    int value = readIntConfigValue(key_value);
                    editor.putString(preference.getKey(), Integer.toString(value));
                    ListPreference list = (ListPreference) frag.findPreference(preference.getKey());
                    list.setValue(Integer.toString(value));
                }
            }
            else if (preference instanceof PreferenceGroup)
            {
                loadPreferencesRecursively((PreferenceGroup) preference, editor);
            }
        }
    }

    // Save the preferences
    private void savePreferencesRecursively(PreferenceGroup root)
    {
        for (int i = 0; i < root.getPreferenceCount(); i++)
        {
            Preference preference = root.getPreference(i);

            if (preference instanceof CheckBoxPreference)
            {
                String key = preference.getKey();
                int key_value = Integer.valueOf(key);
                int value = (((CheckBoxPreference) preference).isChecked() ? 1 : 0);

                setIntConfigValue(key_value, value);
            }
            else if (preference instanceof SeekBarPreference) {
                String key = preference.getKey();
                int key_value = Integer.parseInt(key);

                int value = ((SeekBarPreference) preference).getValue();

                setIntConfigValue(key_value, value);
            }
            else if (preference instanceof ListPreference)
            {
                String key = preference.getKey();
                int key_value = Integer.parseInt(key);
                String value_string = ((ListPreference) preference).getValue();

                if (value_string == null)
                    continue;

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
            else if (preference instanceof PreferenceCategory)
            {
                savePreferencesRecursively((PreferenceCategory) preference);
            }
        }
    }


    @Override
    public void onDestroy()
    {
        savePreferencesRecursively( frag.getPreferenceScreen());

        frag.writeContigFile();

        frag = null;

        super.onDestroy();

        finish();
    }

    // Prevent the activity from being destroyed on a configuration change
    @Override
    public void onConfigurationChanged(Configuration newConfig)
    {
        super.onConfigurationChanged(newConfig);
    }
}