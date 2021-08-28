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

    private void startGame(String writeDir) {
        Intent intent = new Intent(this, AGSRuntimeActivity.class);
        Bundle b = new Bundle();
        b.putString("filename", ""); // full path to game data
        b.putString("directory", writeDir); // writable location (saves, etc.)
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

    private void copyAssetListToDir(String[] list, String dst_dir) {
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
                os = new FileOutputStream(dst_dir+"/"+filename);
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

    private void copyAssetListIfDontExist(String[] list, String dst_dir)
    {
        String[] rlist = RemoveFileFromListIfExistInCache(list);
        if(rlist.length > 0)
        {
            copyAssetListToDir(rlist, dst_dir);
        }
    }

    private void copyAssets(String dst_dir)
    {
        String [] list = new String[0];
        try {
           list = getAssets().list("");
        } catch (IOException e) {
           //;
        }

        copyAssetListToDir(list, dst_dir);
    }

    private void copyConfigToDestination(String dst_dir)
    {
        copyAssetListIfDontExist(new String[]{"android.cfg", "acsetup.cfg"}, dst_dir);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        //setContentView(R.layout.activity_main);

        String writeDir = getApplicationContext().getFilesDir().toString(); //getCacheDir().toString(); // getApplicationInfo().dataDir;
        copyConfigToDestination(writeDir);
        startGame(writeDir);

    }

    @Override
    protected void onResume() {
        super.onResume();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {

    }
}
