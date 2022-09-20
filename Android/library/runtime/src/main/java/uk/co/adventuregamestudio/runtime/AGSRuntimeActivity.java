package uk.co.adventuregamestudio.runtime;

import android.app.Activity;
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
import android.os.Looper;
import android.provider.DocumentsContract;
import android.provider.MediaStore;
import android.util.Log;

import org.libsdl.app.SDL;
import org.libsdl.app.SDLActivity;

import android.Manifest;

import android.content.Intent;
import android.view.ContextMenu;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.InputMethodManager;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.RequiresApi;

import java.io.File;
import java.io.FileNotFoundException;


/**
 * An example full-screen activity that shows and hides the system UI (i.e.
 * status bar and navigation/system bar) with user interaction.
 */
public class AGSRuntimeActivity extends SDLActivity {
    protected String _game_file_name = ""; // Can't be local variable since ags engine reads it. See acpland.cpp.
    protected String _android_base_directory = ""; // Can't be local variable since ags engine reads it. See acpland.cpp.
    protected String _android_app_directory = ""; // Can't be local variable since ags engine reads it. See acpland.cpp.
    protected boolean _loadLastSave = false; // Can't be local variable since ags engine reads it. See acpland.cpp.

    public static native void nativeSdlShowKeyboard();

    private static Boolean handledIntent = false;

    @Override
    protected String[] getLibraries() {
        return new String[] {
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
        View view = getContentView();
        registerForContextMenu(view);
    }

    // Exit confirmation dialog displayed when hitting the "back" button
    public void showExitConfirmation()
    {
        onPause();

        AlertDialog.Builder ad = new AlertDialog.Builder(this);
        ad.setMessage("Are you sure you want to quit?");

        ad.setPositiveButton("Yes", (dialog, which) -> {
            onResume();
            nativeSendQuit();
        });

        ad.setOnCancelListener(dialog -> onResume());
        ad.setNegativeButton("No", (dialog, which) -> onResume());

        ad.show();
    }

    public void toggleKeyboard()
    {
        new android.os.Handler(Looper.getMainLooper()).postDelayed(
                AGSRuntimeActivity::nativeSdlShowKeyboard,
                200
        );
    }

    public void simulateKeyPress(int key, boolean hold_ctrl){

        new android.os.Handler(Looper.getMainLooper()).postDelayed(
                () -> {
                    Activity a = (Activity) getContext();
                    a.getWindow().getDecorView().getRootView();
                    BaseInputConnection inputConnection = new BaseInputConnection(a.getWindow().getDecorView().getRootView(),
                            true);
                    if(hold_ctrl) inputConnection.sendKeyEvent(new KeyEvent(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_CTRL_LEFT));
                    inputConnection.sendKeyEvent(new KeyEvent(KeyEvent.ACTION_DOWN, key));
                    try {
                        Thread.sleep(100);
                    } catch (InterruptedException ignored) {

                    }
                    inputConnection.sendKeyEvent(new KeyEvent(KeyEvent.ACTION_UP, key));
                    if(hold_ctrl) inputConnection.sendKeyEvent(new KeyEvent(KeyEvent.ACTION_UP, KeyEvent.KEYCODE_CTRL_LEFT));
                },
                200
        );
    }

    public void simulateKeyPress(int key) {
        simulateKeyPress(key, false);
    }

    public boolean onInGameMenuItemSelected(MenuItem item)
    {
        int id = item.getItemId();

        // This must be if-else instead of switch-case because it is in a library
        if (id == R.id.key_f1) {
            simulateKeyPress(KeyEvent.KEYCODE_F1);
        } else if(id == R.id.key_f2) {
            simulateKeyPress(KeyEvent.KEYCODE_F2);
        } else if(id == R.id.key_f3) {
            simulateKeyPress(KeyEvent.KEYCODE_F3);
        } else if(id == R.id.key_f4) {
            simulateKeyPress(KeyEvent.KEYCODE_F4);
        } else if(id == R.id.key_f5) {
            simulateKeyPress(KeyEvent.KEYCODE_F5);
        } else if(id == R.id.key_f6) {
            simulateKeyPress(KeyEvent.KEYCODE_F6);
        } else if(id == R.id.key_f7) {
            simulateKeyPress(KeyEvent.KEYCODE_F7);
        } else if(id == R.id.key_f8) {
            simulateKeyPress(KeyEvent.KEYCODE_F8);
        } else if(id == R.id.key_f9) {
            simulateKeyPress(KeyEvent.KEYCODE_F9);
        } else if(id == R.id.key_f10) {
            simulateKeyPress(KeyEvent.KEYCODE_F10);
        } else if(id == R.id.key_f11) {
            simulateKeyPress(KeyEvent.KEYCODE_F11);
        } else if(id == R.id.key_f12) {
            simulateKeyPress(KeyEvent.KEYCODE_F12);
        } else if(id == R.id.key_ctrla) {
            simulateKeyPress(KeyEvent.KEYCODE_A, true);
        } else if(id == R.id.key_ctrlq) {
            simulateKeyPress(KeyEvent.KEYCODE_Q, true);
        } else if(id == R.id.key_ctrlv) {
            simulateKeyPress(KeyEvent.KEYCODE_V, true);
        } else if(id == R.id.key_ctrlx) {
            simulateKeyPress(KeyEvent.KEYCODE_X, true);
        } else if(id == R.id.exitgame) {
            showExitConfirmation();
        } else if(id == R.id.toggle_keyboard) {
            toggleKeyboard();
        } else {
            return false;
        }

        return true;
    }

    public int getInGameMenuID()
    {
        return R.menu.default_ingame;
    }

    @Override
    public void onCreateContextMenu(ContextMenu menu, View v,
                                    ContextMenu.ContextMenuInfo menuInfo)
    {
        super.onCreateContextMenu(menu, v, menuInfo);
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(getInGameMenuID(), menu);
    }

    @Override
    public boolean onContextItemSelected(@NonNull MenuItem item)
    {
        if (this.onInGameMenuItemSelected(item)) return true;
        return super.onOptionsItemSelected(item);
    }

    public void showInGameMenu()
    {
        View view = getContentView();
        openContextMenu(view);
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent ev)
    {
        int action = ev.getAction();
        int key = ev.getKeyCode();

        if(action == KeyEvent.ACTION_DOWN && key == KeyEvent.KEYCODE_BACK)
        {
            showInGameMenu();
            return true;
        }

        return super.dispatchKeyEvent(ev);
    }

}
