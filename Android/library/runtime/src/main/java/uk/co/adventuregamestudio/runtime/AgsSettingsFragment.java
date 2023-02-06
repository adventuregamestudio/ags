package uk.co.adventuregamestudio.runtime;

import android.content.SharedPreferences;
import android.os.Bundle;

import androidx.annotation.Nullable;
import androidx.preference.ListPreference;
import androidx.preference.PreferenceFragmentCompat;
import androidx.preference.PreferenceManager;

import java.io.File;

public class AgsSettingsFragment extends PreferenceFragmentCompat {

    String configDirectory;

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        setPreferencesFromResource(R.xml.preferences, rootKey);
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        PreferencesActivity prefAct = (PreferencesActivity) getActivity();

        if (prefAct.isGlobalConfig) {
            prefAct.setTitle("Global preferences");
            prefAct.configureForGlobalPreferences();
            prefAct.gamePath = prefAct.baseDirectory;
        } else {
            prefAct.setTitle(prefAct.gameName);
            File f = new File(prefAct.gameFilename);
            File g = f.getParentFile();
            if (g != null)
                prefAct.gamePath = g.getPath();
            else
                prefAct.gamePath = prefAct.baseDirectory;
        }

        configDirectory = prefAct.gamePath;
        boolean hasCustomConfig = prefAct.readConfigFile(configDirectory);

        if (!prefAct.isGlobalConfig) {
            // Get available translations from the engine
            String tempTranslations[] = new String[100];
            prefAct.translationCount = prefAct.getAvailableTranslations(tempTranslations);
            prefAct.translations = new String[prefAct.translationCount + 1];
            for (int i = 0; i < prefAct.translationCount; i++)
                prefAct.translations[i + 1] = tempTranslations[i];
            prefAct.translations[0] = "Default";

            // Populate the translation preference
            ListPreference listPref = (ListPreference) findPreference(Integer.toString(PreferencesActivity.CONFIG_TRANSLATION));
            listPref.setEntries(prefAct.translations);
            listPref.setEntryValues(prefAct.translations);
        }

        // Load preferences
        SharedPreferences.Editor editor = PreferenceManager.getDefaultSharedPreferences(getActivity()).edit();
        prefAct.loadPreferencesRecursively(getPreferenceScreen(), editor);
        editor.commit();
    }

    public void writeContigFile()
    {
        PreferencesActivity prefAct = (PreferencesActivity) getActivity();
        assert prefAct != null;
        prefAct.writeConfigFile(configDirectory);
    }
}
