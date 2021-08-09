package com.mystudioname.mygamename;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import uk.co.adventuregamestudio.runtime.AGSRuntimeActivity;

import android.Manifest;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;

import android.content.res.AssetManager;

import android.os.Environment;

public class MainActivity extends Activity {
    private static final String PACKAGE_NAME = App.getContext().getPackageName();
    private static final String GAME_FILE_NAME = App.getContext().getResources()
            .getString(R.string.game_file_name);

    private static final boolean GAME_EMBEDDED = getEmbedded();

    private static boolean getEmbedded() {
        try {
            return Arrays.asList(App.getContext().getResources().getAssets().list(""))
                    .contains(GAME_FILE_NAME);
        }
        catch (IOException e) {
            Log.d("INIT", "IOException occurred during initialization: " + e.getMessage());
            return false;
        }
    }

    // once the expansion file is mounted, this starts the activity that launches the game
    private void startGame(String fileName) {
        Intent intent = new Intent(this, AGSRuntimeActivity.class);
        Bundle b = new Bundle();
        b.putString("filename", fileName); // full path to game data
        b.putString("directory", getApplicationInfo().dataDir); // writable location (saves, etc.)
        b.putBoolean("loadLastSave", false); // TODO: auto-load last save?
        intent.putExtras(b);
        startActivity(intent);
        finish(); // TODO: do something other than just exit the app when the game exits?
    }

    private boolean FileExists(String fname) {
        File file = new File(fname);
        return file.exists();
    }

    private String[] RemoveFileFromListIfExistInCache(String[] list)
    {
        List<String> reslist = new ArrayList<String>();
        for (String filename : list) {
            if(!FileExists(getCacheDir()+"/"+filename))
            {
                reslist.add(filename);
            }
        }
        String[] resArray = new String[ reslist.size() ];
        reslist.toArray(resArray);
        return resArray;
    }

    private void copyAssetListToCache(String[] list) {
        final int BUFFER_SIZE = 102400; // update as needed, your mileage may vary
        InputStream is = null;
        OutputStream os = null;

        for (String filename : list) {
            try {
                is = getAssets().open(filename);
            } catch (IOException e) {
                continue;
            }
            if(is!=null){
                Log.d("ITWORKED",""+is.toString());
            }

            try {
                os = new FileOutputStream(getCacheDir()+"/"+filename);
            } catch (IOException e) {
                continue;
            }
            byte[] buffer = new byte[BUFFER_SIZE];
            try {
                for (int len; (len = is.read(buffer)) != -1; ) {
                    os.write(buffer, 0, len);
                }
            } catch (IOException e) {

            }
        }
    }

    private void copyAssetListToCacheIfDontExist(String[] list)
    {
        String[] rlist = RemoveFileFromListIfExistInCache(list);
        if(rlist.length > 0)
        {
            copyAssetListToCache(rlist);
        }
    }

    private void copyAssetsToCache()
    {
        String [] list = new String[0];
        try {
           list = getAssets().list("");
        } catch (IOException e) {
           //;
        }

        copyAssetListToCache(list);
    }

    private void copyConfigToCache()
    {
        copyAssetListToCacheIfDontExist(new String[]{"android.cfg", "acsetup.cfg"});
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        //setContentView(R.layout.activity_main);
        if(GAME_EMBEDDED)
        {
            //copyAssetsToCache();
            //startGame(getCacheDir() + GAME_FILE_NAME);
            copyConfigToCache();
            startGame(GAME_FILE_NAME);
        }

    }

    @Override
    protected void onResume() {
        super.onResume();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {

    }
}
