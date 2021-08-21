package uk.co.adventuregamestudio.runtime;

import android.app.AlertDialog;
import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.Context;
import android.content.DialogInterface;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.database.Cursor;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.provider.DocumentsContract;
import android.provider.MediaStore;
import android.util.Log;

import org.libsdl.app.SDLActivity;

import android.Manifest;

import android.content.Intent;
import androidx.annotation.Keep;
import androidx.annotation.RequiresApi;
import androidx.core.app.ActivityCompat;

import java.io.File;
import java.io.FileNotFoundException;


/**
 * An example full-screen activity that shows and hides the system UI (i.e.
 * status bar and navigation/system bar) with user interaction.
 */
public class AGSRuntimeActivity extends SDLActivity {
    private String _game_file_name = ""; // Can't be local variable since ags engine reads it. See acpland.cpp.
    private String _android_base_directory = ""; // Can't be local variable since ags engine reads it. See acpland.cpp.
    private String _android_app_directory = ""; // Can't be local variable since ags engine reads it. See acpland.cpp.
    private boolean _loadLastSave = false; // Can't be local variable since ags engine reads it. See acpland.cpp.

    protected final int[] externalStorageRequestDummy = new int[1];
    public static final int EXTERNAL_STORAGE_REQUEST_CODE = 2;
    private static Boolean handledIntent = false;

    @Override
    protected String[] getLibraries() {
        return new String[] {
                "hidapi",
                "SDL2",
                "engine",
                "ags"
        };
    }


    /**
     * This method is called by SDL before starting the native application thread.
     * It can be overridden to provide the arguments after the application name.
     * The default implementation returns an empty array. It never returns null.
     * @return arguments for the native application.
     */
    @Override
    protected String[] getArguments() {
        String[] args  = new String[1];
        args[0] = _game_file_name;
        return args;
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        Log.d("AGSRuntimeActivity", "started");

        // must be reset or it will use the existing value.
        //gamePath = "";

        if(!hasExternalStoragePermission())
        {
            showExternalStoragePermissionMissingDialog();
        }

        // Get filename from "Open with" of another application
        // Get the game filename from the launcher activity
        Bundle extras = getIntent().getExtras();
        _game_file_name = extras.getString("filename");
        _android_base_directory = extras.getString("directory");
        _loadLastSave = extras.getBoolean("loadLastSave");

        // Get app directory
        _android_app_directory = "";
        try
        {
            _android_app_directory = getPackageManager().getPackageInfo(getPackageName(), 0).applicationInfo.dataDir;
        }
        catch (PackageManager.NameNotFoundException ignored) { }

        {
            // this code is a little hack, basically we peer into the android.cfg to look into screen
            // rotation and if necessary, we pre rotate accordingly before passing along for SDL2
            // this prevents a problem I had in one of my old devices where SDL had the device screen
            // before rotation as it's screen size.
            String game_dir;
            if(_game_file_name.length() != 0) {
                File game_file = new File(_game_file_name);
                game_dir = game_file.getParent();
            } else {
                game_dir = _android_base_directory;
            }

            ReadOnlyINI android_cfg = new ReadOnlyINI(game_dir);
            if (android_cfg.load()) {
                String isConfigEnabled = android_cfg.get("config_enabled");
                if (Integer.parseInt(isConfigEnabled) != 0) {
                    String rotation_string = android_cfg.get("rotation");
                    int rotation = Integer.parseInt(rotation_string);
                    if (rotation == 1) {
                        // portrait
                        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
                    } else if (rotation == 2) {
                        // landscape
                        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
                    }
                }
            }
        }


        super.onCreate(savedInstanceState);
    }


    public void showExternalStoragePermissionMissingDialog() {
        AlertDialog dialog = new AlertDialog.Builder(mSingleton)
                .setTitle("Storage Permission Missing")
                .setMessage("AGS for Android will not be able to run non-packaged games without storage permission.")
                .setNeutralButton("Continue", null)
                .create();
        dialog.show();
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        if (grantResults.length > 0) {
            Log.d("AGSRuntimeActivity", "Received a request permission result");

            switch (requestCode) {
                case EXTERNAL_STORAGE_REQUEST_CODE: {
                    if (grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                        Log.d("AGSRuntimeActivity", "Permission granted");
                    } else {
                        Log.d("AGSRuntimeActivity", "Did not get permission.");
                        if (ActivityCompat.shouldShowRequestPermissionRationale(this, Manifest.permission.READ_EXTERNAL_STORAGE)) {
                            showExternalStoragePermissionMissingDialog();
                        }
                    }

                    Log.d("AGSRuntimeActivity", "Unlocking AGS thread");
                    synchronized (externalStorageRequestDummy) {
                        externalStorageRequestDummy[0] = grantResults[0];
                        externalStorageRequestDummy.notify();
                    }
                    break;
                }
                default:
                    super.onRequestPermissionsResult(requestCode, permissions, grantResults);
            }
        }
    }


    @Keep
    public boolean hasExternalStoragePermission() {
        if (ActivityCompat.checkSelfPermission(this,
                Manifest.permission.READ_EXTERNAL_STORAGE)
                == PackageManager.PERMISSION_GRANTED) {
            return true;
        }

        Log.d("AGSRuntimeActivity", "Requesting permission and locking AGS thread until we have an answer.");
        ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.READ_EXTERNAL_STORAGE}, EXTERNAL_STORAGE_REQUEST_CODE);

        synchronized (externalStorageRequestDummy) {
            try {
                externalStorageRequestDummy.wait();
            } catch (InterruptedException e) {
                Log.d("AGSRuntimeActivity", "requesting external storage permission", e);
                return false;
            }
        }

        return ActivityCompat.checkSelfPermission(this, Manifest.permission.READ_EXTERNAL_STORAGE) == PackageManager.PERMISSION_GRANTED;
    }

    @Keep
    private  void AgsEnableLongclick() {

    }

}
